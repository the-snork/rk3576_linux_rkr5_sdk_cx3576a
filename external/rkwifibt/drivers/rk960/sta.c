/*
 * Copyright (c) 2022, Fuzhou Rockchip Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/sched.h>
#include <linux/firmware.h>
#include <linux/if_arp.h>
#include <linux/ipv6.h>
#include <linux/icmpv6.h>
#include <net/ndisc.h>

#include "rk960.h"
#include "sta.h"
#include "ap.h"
#include "fwio.h"
#include "bh.h"
#include "debug.h"
#include "wsm.h"
#ifdef ROAM_OFFLOAD
#include <net/netlink.h>
#endif /*ROAM_OFFLOAD */
#ifdef CONFIG_RK960_TESTMODE
#include "rk960_nl80211_testmode_msg_copy.h"
#include <net/netlink.h>
#endif /* CONFIG_RK960_TESTMODE */

#include "net/mac80211.h"

static int disable_ps = 0;
module_param(disable_ps, int, 0644);
MODULE_PARM_DESC(disable_ps, "disable_ps");

static int listen_interval = 1;
module_param(listen_interval, int, 0644);
MODULE_PARM_DESC(listen_interval, "listen interval");

#define WEP_ENCRYPT_HDR_SIZE    4
#define WEP_ENCRYPT_TAIL_SIZE   4
#define WPA_ENCRYPT_HDR_SIZE    8
#define WPA_ENCRYPT_TAIL_SIZE   12
#define WPA2_ENCRYPT_HDR_SIZE   8
#define WPA2_ENCRYPT_TAIL_SIZE  8
#define WAPI_ENCRYPT_HDR_SIZE   18
#define WAPI_ENCRYPT_TAIL_SIZE  16
#define MAX_ARP_REPLY_TEMPLATE_SIZE     120

#ifdef CONFIG_RK960_TESTMODE
const int rk960_1d_to_ac[8] = {
	IEEE80211_AC_BE,
	IEEE80211_AC_BK,
	IEEE80211_AC_BK,
	IEEE80211_AC_BE,
	IEEE80211_AC_VI,
	IEEE80211_AC_VI,
	IEEE80211_AC_VO,
	IEEE80211_AC_VO
};

/**
 * enum rk960_ac_numbers - AC numbers as used in rk960
 * @RK960_AC_VO: voice
 * @RK960_AC_VI: video
 * @RK960_AC_BE: best effort
 * @RK960_AC_BK: background
 */
enum rk960_ac_numbers {
	RK960_AC_VO = 0,
	RK960_AC_VI = 1,
	RK960_AC_BE = 2,
	RK960_AC_BK = 3,
};
#endif /*CONFIG_RK960_TESTMODE */

#ifdef IPV6_FILTERING
#define MAX_NEIGHBOR_ADVERTISEMENT_TEMPLATE_SIZE 144
#endif /*IPV6_FILTERING */

static inline void __rk960_free_event_queue(struct list_head *list)
{
	while (!list_empty(list)) {
		struct rk960_wsm_event *event =
		    list_first_entry(list, struct rk960_wsm_event,
				     link);
		list_del(&event->link);
		kfree(event);
	}
}

#ifdef CONFIG_RK960_TESTMODE
/* User priority to WSM queue mapping */
const int rk960_priority_to_queueId[8] = {
	WSM_QUEUE_BEST_EFFORT,
	WSM_QUEUE_BACKGROUND,
	WSM_QUEUE_BACKGROUND,
	WSM_QUEUE_BEST_EFFORT,
	WSM_QUEUE_VIDEO,
	WSM_QUEUE_VIDEO,
	WSM_QUEUE_VOICE,
	WSM_QUEUE_VOICE
};
#endif /*CONFIG_RK960_TESTMODE */
static inline void __rk960_bf_configure(struct rk960_vif *priv)
{
	priv->bf_table.numOfIEs = __cpu_to_le32(3);
	priv->bf_table.entry[0].ieId = WLAN_EID_VENDOR_SPECIFIC;
	priv->bf_table.entry[0].actionFlags = WSM_BEACON_FILTER_IE_HAS_CHANGED |
	    WSM_BEACON_FILTER_IE_NO_LONGER_PRESENT |
	    WSM_BEACON_FILTER_IE_HAS_APPEARED;
	priv->bf_table.entry[0].oui[0] = 0x50;
	priv->bf_table.entry[0].oui[1] = 0x6F;
	priv->bf_table.entry[0].oui[2] = 0x9A;

	priv->bf_table.entry[1].ieId = WLAN_EID_ERP_INFO;
	priv->bf_table.entry[1].actionFlags = WSM_BEACON_FILTER_IE_HAS_CHANGED |
	    WSM_BEACON_FILTER_IE_NO_LONGER_PRESENT |
	    WSM_BEACON_FILTER_IE_HAS_APPEARED;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
	priv->bf_table.entry[2].ieId = WLAN_EID_HT_OPERATION;
#else
	priv->bf_table.entry[2].ieId = WLAN_EID_HT_INFORMATION;
#endif
	priv->bf_table.entry[2].actionFlags = WSM_BEACON_FILTER_IE_HAS_CHANGED |
	    WSM_BEACON_FILTER_IE_NO_LONGER_PRESENT |
	    WSM_BEACON_FILTER_IE_HAS_APPEARED;

	priv->bf_control.enabled = WSM_BEACON_FILTER_ENABLE;
}

/* ******************************************************************** */
/* STA API								*/

int rk960_start(struct ieee80211_hw *dev)
{
	struct rk960_common *hw_priv = dev->priv;
	int ret = 0;
#if 0
	/* Assign Max SSIDs supported based on the firmware revision */
	if (hw_priv->hw_revision <= RK960_HW_REV_CUT22) {
		dev->wiphy->max_scan_ssids = 2;	/* for backward compatibility */
	}
#endif
	mutex_lock(&hw_priv->conf_mutex);

	RK960_DEBUG_STA("%s\n", __func__);
#ifdef RK960_CSYNC_ADJUST
	mod_timer(&hw_priv->csync_timer, jiffies + RK960_CSYNC_ADJUST_INTERVAL);
#endif

#ifdef CONFIG_RK960_TESTMODE
	spin_lock_bh(&hw_priv->tsm_lock);
	memset(&hw_priv->tsm_stats, 0, sizeof(struct ste_tsm_stats));
	memset(&hw_priv->tsm_info, 0, sizeof(struct rk960_tsm_info));
	spin_unlock_bh(&hw_priv->tsm_lock);
#endif /*CONFIG_RK960_TESTMODE */
	memcpy(hw_priv->mac_addr, dev->wiphy->perm_addr, ETH_ALEN);
	hw_priv->softled_state = 0;

	ret = rk960_setup_mac(hw_priv);
	if (WARN_ON(ret))
		goto out;

out:
	mutex_unlock(&hw_priv->conf_mutex);
	return ret;
}

void rk960_stop(struct ieee80211_hw *dev)
{
	struct rk960_common *hw_priv = dev->priv;
	struct rk960_vif *priv = NULL;
	LIST_HEAD(list);
	int i;

	RK960_DEBUG_STA("%s\n", __func__);

	wsm_lock_tx(hw_priv);

	while (down_trylock(&hw_priv->scan.lock)) {
		/* Scan is in progress. Force it to stop. */
		hw_priv->scan.req = NULL;
		schedule();
	}
	up(&hw_priv->scan.lock);

	cancel_delayed_work_sync(&hw_priv->scan.probe_work);
	cancel_delayed_work_sync(&hw_priv->scan.sched_scan_stop_work);
	cancel_delayed_work_sync(&hw_priv->scan.timeout);
#ifdef CONFIG_RK960_TESTMODE
	cancel_delayed_work_sync(&hw_priv->advance_scan_timeout);
#endif
	flush_workqueue(hw_priv->workqueue);
	del_timer_sync(&hw_priv->ba_timer);
#ifdef RK960_CSYNC_ADJUST
	del_timer_sync(&hw_priv->csync_timer);
#endif

	mutex_lock(&hw_priv->conf_mutex);

	hw_priv->softled_state = 0;
	/* rk960_set_leds(hw_priv); */

	spin_lock(&hw_priv->event_queue_lock);
	list_splice_init(&hw_priv->event_queue, &list);
	spin_unlock(&hw_priv->event_queue_lock);
	__rk960_free_event_queue(&list);

	for (i = 0; i < 4; i++)
		rk960_queue_clear(&hw_priv->tx_queue[i], RK960_ALL_IFS);

#if 1
	while (atomic_read(&hw_priv->tx_lock) != 1) {
		schedule();
	}
#else
	/* HACK! */
	if (atomic_xchg(&hw_priv->tx_lock, 1) != 1)
		RK960_DEBUG_STA("[STA] TX is force-unlocked "
				"due to stop request.\n");
#endif
	//rk960_for_each_vif(hw_priv, priv, i) {
	for (i = 0; i < RK960_MAX_VIFS; i++) {
		priv = hw_priv->vif_list[i] ?
		    rk960_get_vif_from_ieee80211(hw_priv->vif_list[i]) : NULL;
		if (!priv)
			continue;
		priv->mode = NL80211_IFTYPE_UNSPECIFIED;
		priv->listening = false;
		priv->delayed_link_loss = 0;
		priv->join_status = RK960_JOIN_STATUS_PASSIVE;
		mutex_unlock(&hw_priv->conf_mutex);
		cancel_delayed_work_sync(&priv->join_timeout);
		mutex_lock(&hw_priv->conf_mutex);
		cancel_delayed_work_sync(&priv->bss_loss_work);
		cancel_delayed_work_sync(&priv->connection_loss_work);
		cancel_delayed_work_sync(&priv->link_id_gc_work);
		del_timer_sync(&priv->mcast_timeout);
	}

	wsm_unlock_tx(hw_priv);

	mutex_unlock(&hw_priv->conf_mutex);
}

int rk960_add_interface(struct ieee80211_hw *dev, struct ieee80211_vif *vif)
{
	int ret;
	struct rk960_common *hw_priv = dev->priv;
	struct rk960_vif *priv;
	struct rk960_vif **drv_priv = (void *)vif->drv_priv;
#ifndef P2P_MULTIVIF
	//int i;
	int if_id;
	int set;

	if (atomic_read(&hw_priv->num_vifs) >= RK960_MAX_VIFS)
		return -EOPNOTSUPP;
#endif
	RK960_DEBUG_STA("%s: vif->type %d, vif->p2p %d, addr %pM\n",
			__func__, vif->type, vif->p2p, vif->addr);

	priv = rk960_get_vif_from_ieee80211(vif);
	atomic_set(&priv->enabled, 0);

	*drv_priv = priv;
	/* __le32 auto_calibration_mode = __cpu_to_le32(1); */

	mutex_lock(&hw_priv->conf_mutex);

	priv->mode = vif->type;

	rk960_hw_vif_write_lock(&hw_priv->vif_list_lock);
	if (atomic_read(&hw_priv->num_vifs) < RK960_MAX_VIFS) {
#ifdef P2P_MULTIVIF
		if (!memcmp(vif->addr, hw_priv->addresses[0].addr, ETH_ALEN)) {
			priv->if_id = 0;
		} else if (!memcmp(vif->addr, hw_priv->addresses[1].addr,
				   ETH_ALEN)) {
			priv->if_id = 2;
		} else if (!memcmp(vif->addr, hw_priv->addresses[2].addr,
				   ETH_ALEN)) {
			priv->if_id = 1;
		}
		RK960_DEBUG_STA("%s: if_id %d mac %pM\n",
				__func__, priv->if_id, vif->addr);
#else
#if 0
		for (i = 0; i < RK960_MAX_VIFS; i++)
			if (!memcmp(vif->addr, hw_priv->addresses[i].addr,
				    ETH_ALEN))
				break;
		if (i == RK960_MAX_VIFS) {
			rk960_hw_vif_write_unlock(&hw_priv->vif_list_lock);
			mutex_unlock(&hw_priv->conf_mutex);
			return -EINVAL;
		}
		priv->if_id = i;
		set = 0;
#else
		for (if_id = 0; if_id < RK960_MAX_VIFS; if_id++) {
			if (!(hw_priv->if_id_slot & BIT(if_id)))
				break;
		}
		if (if_id == RK960_MAX_VIFS) {
			rk960_hw_vif_write_unlock(&hw_priv->vif_list_lock);
			mutex_unlock(&hw_priv->conf_mutex);
			return -EINVAL;
		}
		priv->if_id = if_id;
		set = 1;
#endif
#endif
		hw_priv->if_id_slot |= BIT(priv->if_id);
		priv->hw_priv = hw_priv;
		priv->hw = dev;
		priv->vif = vif;
		//WARN_ON(RK960_HW_VIF_GET(hw_priv->vif_list[priv->if_id]) != NULL);
		RK960_HW_VIF_SET(hw_priv->vif_list[priv->if_id], vif);
		atomic_inc(&hw_priv->num_vifs);
		if (hw_priv->def_vif_id == -1)
			hw_priv->def_vif_id = priv->if_id;
	} else {
		rk960_hw_vif_write_unlock(&hw_priv->vif_list_lock);
		mutex_unlock(&hw_priv->conf_mutex);
		return -EOPNOTSUPP;
	}
	rk960_hw_vif_write_unlock(&hw_priv->vif_list_lock);

#ifdef RK960_VIF_LIST_USE_RCU_LOCK
	synchronize_rcu();
#endif

	/* TODO:COMBO :Check if MAC address matches the one expected by FW */
	memcpy(hw_priv->mac_addr, vif->addr, ETH_ALEN);

	/* Enable auto-calibration */
	/* Exception in subsequent channel switch; disabled.
	   WARN_ON(wsm_write_mib(hw_priv, WSM_MIB_ID_SET_AUTO_CALIBRATION_MODE,
	   &auto_calibration_mode, sizeof(auto_calibration_mode)));
	 */
	RK960_DEBUG_STA("[STA] Interface ID:%d of type:%d added\n",
			priv->if_id, priv->mode);
	mutex_unlock(&hw_priv->conf_mutex);

	rk960_vif_setup(priv, set);

	ret = WARN_ON(rk960_setup_mac_pvif(priv));

	return ret;
}

void rk960_remove_interface(struct ieee80211_hw *dev, struct ieee80211_vif *vif)
{
	struct rk960_common *hw_priv = dev->priv;
	struct rk960_vif *priv = rk960_get_vif_from_ieee80211(vif);
	struct wsm_reset reset = {
		.reset_statistics = true,
	};
	int i;
	bool is_htcapie = false;
	struct rk960_vif *tmp_priv;
	struct wsm_operational_mode mode = {
		.power_mode = wsm_power_mode_quiescent,
		.disableMoreFlagUsage = true,
	};

	RK960_DEBUG_STA(" !!! %s: enter, priv=%p type %d p2p %d addr %pM\n",
			__func__, priv, vif->type, vif->p2p, vif->addr);

	atomic_set(&priv->enabled, 0);
	down(&hw_priv->scan.lock);
	mutex_lock(&hw_priv->conf_mutex);
	rk960_tx_queues_lock(hw_priv);
	wsm_lock_tx(hw_priv);
	switch (priv->join_status) {
	case RK960_JOIN_STATUS_STA:
		wsm_lock_tx(hw_priv);
#if 0
		if (queue_work(hw_priv->workqueue, &priv->unjoin_work) <= 0)
			wsm_unlock_tx(hw_priv);
#else
		mutex_unlock(&hw_priv->conf_mutex);
		rk960_unjoin_work(&priv->unjoin_work);
		mutex_lock(&hw_priv->conf_mutex);
#endif
		break;
	case RK960_JOIN_STATUS_AP:
		for (i = 0; priv->link_id_map; ++i) {
			if (priv->link_id_map & BIT(i)) {
				rk960_unmap_link(priv, i);
				priv->link_id_map &= ~BIT(i);
			}
		}
		memset(priv->link_id_db, 0, sizeof(priv->link_id_db));
		priv->sta_asleep_mask = 0;
		priv->enable_beacon = false;
		priv->tx_multicast = false;
		priv->aid0_bit_set = false;
		priv->buffered_multicasts = false;
		priv->pspoll_mask = 0;
		priv->tim_length = 0;
		priv->deauthAll = false;
		reset.link_id = 0;
		wsm_reset(hw_priv, &reset, priv->if_id);
		WARN_ON(wsm_set_operational_mode(hw_priv, &mode, priv->if_id));
		//rk960_for_each_vif(hw_priv, tmp_priv, i) {
		for (i = 0; i < RK960_MAX_VIFS; i++) {
			tmp_priv = hw_priv->vif_list[i] ?
			    rk960_get_vif_from_ieee80211(hw_priv->
							 vif_list[i]) : NULL;
#ifdef P2P_MULTIVIF
			if ((i == (RK960_MAX_VIFS - 1)) || !tmp_priv)
#else
			if (!tmp_priv)
#endif
				continue;
			if ((tmp_priv->join_status == RK960_JOIN_STATUS_STA)
			    && tmp_priv->htcap)
				is_htcapie = true;
		}

		if (is_htcapie) {
			hw_priv->vif0_throttle = RK960_HOST_VIF0_11N_THROTTLE;
			hw_priv->vif1_throttle = RK960_HOST_VIF1_11N_THROTTLE;
			RK960_DEBUG_STA("AP REMOVE HTCAP 11N %d\n",
					hw_priv->vif0_throttle);
		} else {
			hw_priv->vif0_throttle = RK960_HOST_VIF0_11BG_THROTTLE;
			hw_priv->vif1_throttle = RK960_HOST_VIF1_11BG_THROTTLE;
			RK960_DEBUG_STA("AP REMOVE 11BG %d\n",
					hw_priv->vif0_throttle);
		}
		break;
	case RK960_JOIN_STATUS_MONITOR:
		rk960_disable_listening(priv);
		break;
	default:
		break;
	}
	/* TODO:COMBO: Change Queue Module */
	if (!__rk960_flush(hw_priv, true, priv->if_id))
		wsm_unlock_tx(hw_priv);

	cancel_delayed_work_sync(&priv->bss_loss_work);
	cancel_delayed_work_sync(&priv->connection_loss_work);
	cancel_delayed_work_sync(&priv->link_id_gc_work);
	mutex_unlock(&hw_priv->conf_mutex);
	cancel_delayed_work_sync(&priv->join_timeout);
	mutex_lock(&hw_priv->conf_mutex);
	cancel_delayed_work_sync(&priv->set_cts_work);
	cancel_delayed_work_sync(&priv->pending_offchanneltx_work);

	del_timer_sync(&priv->mcast_timeout);
	/* TODO:COMBO: May be reset of these variables "delayed_link_loss and
	 * join_status to default can be removed as dev_priv will be freed by
	 * mac80211 */
	priv->delayed_link_loss = 0;
	priv->join_status = RK960_JOIN_STATUS_PASSIVE;
	wsm_unlock_tx(hw_priv);

	if ((priv->if_id == 1) && (priv->mode == NL80211_IFTYPE_AP
				   || priv->mode == NL80211_IFTYPE_P2P_GO)) {
		hw_priv->is_go_thru_go_neg = false;
	}
	rk960_hw_vif_write_lock(&hw_priv->vif_list_lock);
	rk960_priv_vif_list_write_lock(&priv->vif_lock);
	RK960_HW_VIF_SET(hw_priv->vif_list[priv->if_id], NULL);
	hw_priv->if_id_slot &= (~BIT(priv->if_id));
	atomic_dec(&hw_priv->num_vifs);
	if (atomic_read(&hw_priv->num_vifs) == 0) {
		rk960_free_keys(hw_priv);
		memset(hw_priv->mac_addr, 0, ETH_ALEN);
	}
	rk960_priv_vif_list_write_unlock(&priv->vif_lock);
	rk960_hw_vif_write_unlock(&hw_priv->vif_list_lock);
	priv->listening = false;

#ifdef RK960_VIF_LIST_USE_RCU_LOCK
	synchronize_rcu();
#endif

	debugfs_remove_recursive(priv->debug->debugfs_phy);
	rk960_debug_release_priv(priv);

	rk960_tx_queues_unlock(hw_priv);
	mutex_unlock(&hw_priv->conf_mutex);

	if (atomic_read(&hw_priv->num_vifs) == 0)
		flush_workqueue(hw_priv->workqueue);
	memset(priv, 0, sizeof(struct rk960_vif));
	up(&hw_priv->scan.lock);
}

int rk960_change_interface(struct ieee80211_hw *dev,
			   struct ieee80211_vif *vif,
			   enum nl80211_iftype new_type, bool p2p)
{
	int ret = 0;
	RK960_DEBUG_STA("rk960_change_interface: type %d (%d), p2p %d (%d)\n",
			new_type, vif->type, p2p, vif->p2p);
	if (new_type != vif->type || vif->p2p != p2p) {
		rk960_remove_interface(dev, vif);
		vif->type = new_type;
		vif->p2p = p2p;
		ret = rk960_add_interface(dev, vif);
	}

	return ret;
}

int rk960_config(struct ieee80211_hw *dev, u32 changed)
{
	int ret = 0;
	struct rk960_common *hw_priv = dev->priv;
	struct ieee80211_conf *conf = &dev->conf;
#ifdef CONFIG_RK960_TESTMODE
	int max_power_level = 0;
	int min_power_level = 0;
#endif
	/* TODO:COMBO: adjust to multi vif interface
	 * IEEE80211_CONF_CHANGE_IDLE is still handled per rk960_vif*/
	int if_id = 0;
	struct rk960_vif *priv;

	RK960_DEBUG_STA("%s: changed %x\n", __func__, changed);

#if 0
	if (changed &
	    (IEEE80211_CONF_CHANGE_MONITOR | IEEE80211_CONF_CHANGE_IDLE)) {
		/* TBD: It looks like it's transparent
		 * there's a monitor interface present -- use this
		 * to determine for example whether to calculate
		 * timestamps for packets or not, do not use instead
		 * of filter flags! */
		sta_printk(KERN_DEBUG
			   "ignore IEEE80211_CONF_CHANGE_MONITOR (%d)"
			   "IEEE80211_CONF_CHANGE_IDLE (%d)\n",
			   (changed & IEEE80211_CONF_CHANGE_MONITOR) ? 1 : 0,
			   (changed & IEEE80211_CONF_CHANGE_IDLE) ? 1 : 0);
		return ret;
	}
#endif

	down(&hw_priv->scan.lock);
	mutex_lock(&hw_priv->conf_mutex);
	priv = __rk960_hwpriv_to_vifpriv(hw_priv, hw_priv->scan.if_id);
	/* TODO: IEEE80211_CONF_CHANGE_QOS */
	/* TODO:COMBO:Change when support is available mac80211 */
	if (changed & IEEE80211_CONF_CHANGE_POWER) {
		/*hw_priv->output_power = conf->power_level; */
		RK960_DEBUG_STA("Output power ++%d\n", conf->power_level);
		hw_priv->output_power = RK960_DEFAULT_TX_POWER;
		RK960_DEBUG_STA("Output power --%d\n", hw_priv->output_power);
#ifdef CONFIG_RK960_TESTMODE
		/* Testing if Power Level to set is out of device power range */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
		if (conf->chandef.chan->band ==
#else
		if (conf->chan_conf->channel->band ==
#endif
		    IEEE80211_BAND_2GHZ) {
			max_power_level =
			    hw_priv->txPowerRange[0].max_power_level;
			min_power_level =
			    hw_priv->txPowerRange[0].min_power_level;
		} else {
			max_power_level =
			    hw_priv->txPowerRange[1].max_power_level;
			min_power_level =
			    hw_priv->txPowerRange[1].min_power_level;
		}
		if (hw_priv->output_power > max_power_level)
			hw_priv->output_power = max_power_level;
		else if (hw_priv->output_power < min_power_level)
			hw_priv->output_power = min_power_level;
#endif /* CONFIG_RK960_TESTMODE */
		RK960_DEBUG_STA("[STA] TX power: %d\n", hw_priv->output_power);
		WARN_ON(wsm_set_output_power(hw_priv,
					     hw_priv->output_power * 10,
					     if_id));
	}

	if ((changed & IEEE80211_CONF_CHANGE_CHANNEL)	/* &&
							   #if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
							   (hw_priv->channel != conf->chandef.chan)) {
							   #else
							   (hw_priv->channel != conf->chan_conf->channel) */ ) {
/*#endif*/
		/* Switch Channel commented for CC Mode */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
		struct ieee80211_channel *ch = conf->chandef.chan;
#else
		struct ieee80211_channel *ch = conf->chan_conf->channel;
#endif
		RK960_INFO_STA("[STA] Freq %d (wsm ch: %d).\n",
			       ch->center_freq, ch->hw_value);
		/* Earlier there was a call to __rk960_flush().
		   Removed as deemed unnecessary */

		hw_priv->channel = ch;
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
	if (changed & IEEE80211_CONF_CHANGE_RETRY_LIMITS) {
		RK960_DEBUG_STA("[STA] Retry limits: %d (long), "
				"%d (short).\n", conf->long_frame_max_tx_count,
				conf->short_frame_max_tx_count);
		//spin_lock_bh(&hw_priv->tx_policy_cache.lock);
		/*TODO:COMBO: for now it's still handled per hw and kept
		 * in rk960_common */
		hw_priv->long_frame_max_tx_count =
		    conf->long_frame_max_tx_count;
		hw_priv->short_frame_max_tx_count =
		    (conf->short_frame_max_tx_count <
		     0x0F) ? conf->short_frame_max_tx_count : 0x0F;
#if 1
		if (hw_priv->long_frame_max_tx_count < RK960_MAX_TX_COUNT)
			hw_priv->long_frame_max_tx_count = RK960_MAX_TX_COUNT;
		if (hw_priv->short_frame_max_tx_count < RK960_MAX_TX_COUNT)
			hw_priv->short_frame_max_tx_count = RK960_MAX_TX_COUNT;
#endif
		hw_priv->hw->max_rate_tries = hw_priv->short_frame_max_tx_count;
		//spin_unlock_bh(&hw_priv->tx_policy_cache.lock);
		/* TBD: I think we don't need tx_policy_force_upload().
		 * Outdated policies will leave cache in a normal way. */
		/* WARN_ON(tx_policy_force_upload(priv)); */
	}
#endif

	if (changed & IEEE80211_CONF_CHANGE_PS) {
		for (if_id = 0; if_id < 2; if_id++) {
			priv = __rk960_hwpriv_to_vifpriv(hw_priv, if_id);
			if (priv && priv->vif && !priv->vif->p2p)
				break;
		}
		if (!priv)
			goto config_out;

		if (!(conf->flags & IEEE80211_CONF_PS))
			priv->powersave_mode.pmMode = WSM_PSM_ACTIVE;
		else if (conf->dynamic_ps_timeout <= 0)
			priv->powersave_mode.pmMode = WSM_PSM_PS;
		else
			priv->powersave_mode.pmMode = WSM_PSM_FAST_PS;

		RK960_DEBUG_STA("[STA] if_id %d Aid: %d, flags %x, "
				"Joined: %s, Powersave: %s %d\n",
				priv->if_id, priv->bss_params.aid, conf->flags,
				priv->join_status ==
				RK960_JOIN_STATUS_STA ? "yes" : "no",
				priv->powersave_mode.pmMode ==
				WSM_PSM_ACTIVE ? "WSM_PSM_ACTIVE" : priv->
				powersave_mode.pmMode ==
				WSM_PSM_PS ? "WSM_PSM_PS" : priv->
				powersave_mode.pmMode ==
				WSM_PSM_FAST_PS ? "WSM_PSM_FAST_PS" : "UNKNOWN",
				conf->dynamic_ps_timeout);

		/* Firmware requires that value for this 1-byte field must
		 * be specified in units of 500us. Values above the 128ms
		 * threshold are not supported.
		 */
		if (conf->dynamic_ps_timeout >= 0x80)
			priv->powersave_mode.fastPsmIdlePeriod = 0xFF;
		else
			priv->powersave_mode.fastPsmIdlePeriod =
			    conf->dynamic_ps_timeout << 1;

		if (priv->join_status == RK960_JOIN_STATUS_STA &&
		    priv->bss_params.aid)
			rk960_set_pm(priv, &priv->powersave_mode);
	}

config_out:
	mutex_unlock(&hw_priv->conf_mutex);
	up(&hw_priv->scan.lock);
	return ret;
}

void rk960_update_filtering(struct rk960_vif *priv)
{
	int ret;
	bool bssid_filtering = !priv->rx_filter.bssid;
	struct rk960_common *hw_priv = rk960_vifpriv_to_hwpriv(priv);
	static struct wsm_beacon_filter_control bf_disabled = {
		.enabled = 0,
		.bcn_count = 1,
	};
	bool ap_mode = 0;
	static struct wsm_beacon_filter_table bf_table_auto = {
		.numOfIEs = __cpu_to_le32(2),
		.entry[0].ieId = WLAN_EID_VENDOR_SPECIFIC,
		.entry[0].actionFlags = WSM_BEACON_FILTER_IE_HAS_CHANGED |
		    WSM_BEACON_FILTER_IE_NO_LONGER_PRESENT |
		    WSM_BEACON_FILTER_IE_HAS_APPEARED,
		.entry[0].oui[0] = 0x50,
		.entry[0].oui[1] = 0x6F,
		.entry[0].oui[2] = 0x9A,

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
		.entry[1].ieId = WLAN_EID_HT_OPERATION,
#else
		.entry[1].ieId = WLAN_EID_HT_INFORMATION,
#endif
		.entry[1].actionFlags = WSM_BEACON_FILTER_IE_HAS_CHANGED |
		    WSM_BEACON_FILTER_IE_NO_LONGER_PRESENT |
		    WSM_BEACON_FILTER_IE_HAS_APPEARED,
	};
	static struct wsm_beacon_filter_control bf_auto = {
		.enabled = WSM_BEACON_FILTER_ENABLE |
		    WSM_BEACON_FILTER_AUTO_ERP,
		.bcn_count = 1,
	};
	bf_auto.bcn_count = priv->bf_control.bcn_count;

	if (priv->join_status == RK960_JOIN_STATUS_PASSIVE)
		return;
	else if (priv->join_status == RK960_JOIN_STATUS_MONITOR)
		bssid_filtering = false;

	if (priv->vif && (priv->vif->type == NL80211_IFTYPE_AP))
		ap_mode = true;
	/*
	 * When acting as p2p client being connected to p2p GO, in order to
	 * receive frames from a different p2p device, turn off bssid filter.
	 *
	 * WARNING: FW dependency!
	 * This can only be used with FW WSM371 and its successors.
	 * In that FW version even with bssid filter turned off,
	 * device will block most of the unwanted frames.
	 */
	if (priv->vif && priv->vif->p2p)
		bssid_filtering = false;

	ret = wsm_set_rx_filter(hw_priv, &priv->rx_filter, priv->if_id);
	if (!ret && !ap_mode) {
		if (priv->vif) {
			if (priv->vif->p2p
			    || NL80211_IFTYPE_STATION != priv->vif->type)
				ret =
				    wsm_set_beacon_filter_table(hw_priv,
								&priv->bf_table,
								priv->if_id);
			else
				ret =
				    wsm_set_beacon_filter_table(hw_priv,
								&bf_table_auto,
								priv->if_id);
		} else
			WARN_ON(1);
	}
	if (!ret && !ap_mode) {
		if (priv->disable_beacon_filter)
			ret = wsm_beacon_filter_control(hw_priv,
							&bf_disabled,
							priv->if_id);
		else {
			if (priv->vif) {
				if (priv->vif->p2p
				    || NL80211_IFTYPE_STATION !=
				    priv->vif->type)
					ret =
					    wsm_beacon_filter_control(hw_priv,
								      &priv->bf_control,
								      priv->if_id);
				else
					ret = wsm_beacon_filter_control(hw_priv,
									&bf_auto,
									priv->if_id);
			} else
				WARN_ON(1);
		}
	}

	if (!ret)
		ret = wsm_set_bssid_filtering(hw_priv, bssid_filtering,
					      priv->if_id);
#if 0
	if (!ret) {
		ret = wsm_set_multicast_filter(hw_priv, &priv->multicast_filter,
					       priv->if_id);
	}
#endif
	if (ret)
		RK960_ERROR_STA("%s: Update filtering failed: %d.\n",
				__func__, ret);
	return;
}

void rk960_update_filtering_work(struct work_struct *work)
{
	struct rk960_vif *priv = container_of(work, struct rk960_vif,
					      update_filtering_work);

	mutex_lock(&priv->hw_priv->conf_mutex);
	rk960_update_filtering(priv);
	mutex_unlock(&priv->hw_priv->conf_mutex);
}

void rk960_set_beacon_wakeup_period_work(struct work_struct *work)
{
	struct rk960_vif *priv = container_of(work, struct rk960_vif,
					      set_beacon_wakeup_period_work);

	WARN_ON(wsm_set_beacon_wakeup_period(priv->hw_priv,
					     priv->beacon_int *
					     priv->join_dtim_period >
					     MAX_BEACON_SKIP_TIME_MS ? 1 :
					     priv->join_dtim_period, 0,
					     priv->if_id));
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
static void rk960_prepare_multicast_iter(void *data, u8 * mac,
					 struct ieee80211_vif *vif)
{
	static u8 broadcast_ipv6[ETH_ALEN] = {
		0x33, 0x33, 0x00, 0x00, 0x00, 0x01
	};
	static u8 broadcast_ipv4[ETH_ALEN] = {
		0x01, 0x00, 0x5e, 0x00, 0x00, 0x01
	};
	struct netdev_hw_addr *ha;
	int count = 0;
	struct rk960_vif *priv = rk960_get_vif_from_ieee80211(vif);
	struct netdev_hw_addr_list *mc_list =
	    (struct netdev_hw_addr_list *)data;

	if (WARN_ON(!priv))
		return;

#ifdef P2P_MULTIVIF
	if (priv->if_id == RK960_GENERIC_IF_ID)
		return;
#endif

	RK960_DEBUG_STA("%s: if_id %d\n", __func__, priv->if_id);

	/* Disable multicast filtering */
	priv->has_multicast_subscription = false;
	memset(&priv->multicast_filter, 0x00, sizeof(priv->multicast_filter));

	/* Enable if requested */
	netdev_hw_addr_list_for_each(ha, mc_list) {
		RK960_DEBUG_STA("[STA] multicast: %pM\n", ha->addr);
		memcpy(&priv->multicast_filter.macAddress[count],
		       ha->addr, ETH_ALEN);
		if (memcmp(ha->addr, broadcast_ipv4, ETH_ALEN) &&
		    memcmp(ha->addr, broadcast_ipv6, ETH_ALEN))
			priv->has_multicast_subscription = true;
		count++;
	}

	if (count) {

		priv->multicast_filter.enable = __cpu_to_le32(1);
		priv->multicast_filter.numOfAddresses = __cpu_to_le32(count);
	}
}

u64 rk960_prepare_multicast(struct ieee80211_hw *hw,
			    struct netdev_hw_addr_list *mc_list)
{
	RK960_DEBUG_STA("%s: \n", __func__);

	if (netdev_hw_addr_list_count(mc_list) > WSM_MAX_GRP_ADDRTABLE_ENTRIES)
		return 0;

/*{
    struct netdev_hw_addr *ha;

	netdev_hw_addr_list_for_each(ha, mc_list) {
		RK960_DEBUG_STA("multicast: %pM\n", ha->addr);
    }
}*/

	ieee80211_iterate_active_interfaces_atomic(hw,
						   IEEE80211_IFACE_ITER_NORMAL,
						   rk960_prepare_multicast_iter,
						   mc_list);

	return netdev_hw_addr_list_count(mc_list);
}
#else
u64 rk960_prepare_multicast(struct ieee80211_hw *hw,
			    struct ieee80211_vif *vif,
			    struct netdev_hw_addr_list *mc_list)
{
	static u8 broadcast_ipv6[ETH_ALEN] = {
		0x33, 0x33, 0x00, 0x00, 0x00, 0x01
	};
	static u8 broadcast_ipv4[ETH_ALEN] = {
		0x01, 0x00, 0x5e, 0x00, 0x00, 0x01
	};
	struct netdev_hw_addr *ha;
	int count = 0;
	struct rk960_vif *priv = rk960_get_vif_from_ieee80211(vif);

	if (WARN_ON(!priv))
		return netdev_hw_addr_list_count(mc_list);

#ifdef P2P_MULTIVIF
	if (priv->if_id == RK960_GENERIC_IF_ID)
		return 0;
#endif

	/* Disable multicast filtering */
	priv->has_multicast_subscription = false;
	memset(&priv->multicast_filter, 0x00, sizeof(priv->multicast_filter));

	if (netdev_hw_addr_list_count(mc_list) > WSM_MAX_GRP_ADDRTABLE_ENTRIES)
		return 0;

	/* Enable if requested */
	netdev_hw_addr_list_for_each(ha, mc_list) {
		RK960_DEBUG_STA("[STA] multicast: %pM\n", ha->addr);
		memcpy(&priv->multicast_filter.macAddress[count],
		       ha->addr, ETH_ALEN);
		if (memcmp(ha->addr, broadcast_ipv4, ETH_ALEN) &&
		    memcmp(ha->addr, broadcast_ipv6, ETH_ALEN))
			priv->has_multicast_subscription = true;
		count++;
	}

	if (count) {

		priv->multicast_filter.enable = __cpu_to_le32(1);
		priv->multicast_filter.numOfAddresses = __cpu_to_le32(count);
	}

	return netdev_hw_addr_list_count(mc_list);
}
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
struct rk960_configure_filter_param {
	unsigned int changed_flags;
	unsigned int *total_flags;
	u64 multicast;
};

#if 0
static void rk960_configure_filter_iter(void *data, u8 * mac,
					struct ieee80211_vif *vif)
{
	struct rk960_vif *priv = rk960_get_vif_from_ieee80211(vif);
	struct rk960_common *hw_priv = priv->hw_priv;
	struct rk960_configure_filter_param *param =
	    (struct rk960_configure_filter_param *)data;

	RK960_DEBUG_STA("%s: if_id %d\n", __func__, priv->if_id);

	if (!priv)
		return;
#ifdef P2P_MULTIVIF
	if (priv->if_id == RK960_GENERIC_IF_ID) {
		*(param->total_flags) &= ~(1 << 31);
		return;
	}
#endif
#if 0
	bool listening = ! !(*total_flags &
			     (FIF_PROMISC_IN_BSS |
			      FIF_OTHER_BSS |
			      FIF_BCN_PRBRESP_PROMISC | FIF_PROBE_REQ));
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
	*(param->total_flags) &=
	    FIF_OTHER_BSS |
	    FIF_FCSFAIL | FIF_BCN_PRBRESP_PROMISC | FIF_PROBE_REQ;

	down(&hw_priv->scan.lock);
	mutex_lock(&hw_priv->conf_mutex);

	priv->rx_filter.promiscuous = 1;
	priv->rx_filter.bssid = (*(param->total_flags) & (FIF_OTHER_BSS |
							  FIF_PROBE_REQ)) ? 1 :
	    0;
	priv->rx_filter.fcs = (*(param->total_flags) & FIF_FCSFAIL) ? 1 : 0;
	priv->bf_control.bcn_count = (*(param->total_flags) &
				      (FIF_BCN_PRBRESP_PROMISC |
				       FIF_PROBE_REQ)) ? 1 : 0;
#else
	*(param->total_flags) &= FIF_PROMISC_IN_BSS |
	    FIF_OTHER_BSS |
	    FIF_FCSFAIL | FIF_BCN_PRBRESP_PROMISC | FIF_PROBE_REQ;

	down(&hw_priv->scan.lock);
	mutex_lock(&hw_priv->conf_mutex);

	priv->rx_filter.promiscuous =
	    (*(param->total_flags) & FIF_PROMISC_IN_BSS)
	    ? 1 : 0;
	priv->rx_filter.bssid = (*(param->total_flags) & (FIF_OTHER_BSS |
							  FIF_PROBE_REQ)) ? 1 :
	    0;
	priv->rx_filter.fcs = (*(param->total_flags) & FIF_FCSFAIL) ? 1 : 0;
	priv->bf_control.bcn_count = (*(param->total_flags) &
				      (FIF_BCN_PRBRESP_PROMISC |
				       FIF_PROMISC_IN_BSS |
				       FIF_PROBE_REQ)) ? 1 : 0;
#endif
#if 0
	if (priv->listening ^ listening) {
		priv->listening = listening;
		wsm_lock_tx(hw_priv);
		rk960_update_listening(priv, listening);
		wsm_unlock_tx(hw_priv);
	}
#endif
	rk960_update_filtering(priv);
	mutex_unlock(&hw_priv->conf_mutex);
	up(&hw_priv->scan.lock);
}
#endif

void rk960_configure_filter(struct ieee80211_hw *hw,
			    unsigned int changed_flags,
			    unsigned int *total_flags, u64 multicast)
{
#if 0
	struct rk960_configure_filter_param param;
#endif
	RK960_DEBUG_STA("%s: \n", __func__);

#if 0
	param.changed_flags = changed_flags;
	param.total_flags = total_flags;
	param.multicast = multicast;

	ieee80211_iterate_active_interfaces_atomic(hw,
						   IEEE80211_IFACE_ITER_NORMAL,
						   rk960_configure_filter_iter,
						   &param);
#endif
	*total_flags &= ~(1 << 31);
}
#else
void rk960_configure_filter(struct ieee80211_hw *hw,
			    struct ieee80211_vif *vif,
			    unsigned int changed_flags,
			    unsigned int *total_flags, u64 multicast)
{
	struct rk960_common *hw_priv = hw->priv;
	struct rk960_vif *priv = rk960_get_vif_from_ieee80211(vif);

#ifdef P2P_MULTIVIF
	if (priv->if_id == RK960_GENERIC_IF_ID) {
		*total_flags &= ~(1 << 31);
		return;
	}
#endif
#if 0
	bool listening = ! !(*total_flags &
			     (FIF_PROMISC_IN_BSS |
			      FIF_OTHER_BSS |
			      FIF_BCN_PRBRESP_PROMISC | FIF_PROBE_REQ));
#endif
	*total_flags &= FIF_PROMISC_IN_BSS |
	    FIF_OTHER_BSS |
	    FIF_FCSFAIL | FIF_BCN_PRBRESP_PROMISC | FIF_PROBE_REQ;

	down(&hw_priv->scan.lock);
	mutex_lock(&hw_priv->conf_mutex);

	priv->rx_filter.promiscuous = (*total_flags & FIF_PROMISC_IN_BSS)
	    ? 1 : 0;
	priv->rx_filter.bssid = (*total_flags & (FIF_OTHER_BSS |
						 FIF_PROBE_REQ)) ? 1 : 0;
	priv->rx_filter.fcs = (*total_flags & FIF_FCSFAIL) ? 1 : 0;
	priv->bf_control.bcn_count = (*total_flags &
				      (FIF_BCN_PRBRESP_PROMISC |
				       FIF_PROMISC_IN_BSS |
				       FIF_PROBE_REQ)) ? 1 : 0;
#if 0
	if (priv->listening ^ listening) {
		priv->listening = listening;
		wsm_lock_tx(hw_priv);
		rk960_update_listening(priv, listening);
		wsm_unlock_tx(hw_priv);
	}
#endif
	rk960_update_filtering(priv);
	mutex_unlock(&hw_priv->conf_mutex);
	up(&hw_priv->scan.lock);
}
#endif

int rk960_conf_tx(struct ieee80211_hw *dev, struct ieee80211_vif *vif,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
		unsigned int link_id,
#endif
		  u16 queue, const struct ieee80211_tx_queue_params *params)
{
	struct rk960_common *hw_priv = dev->priv;
	struct rk960_vif *priv = rk960_get_vif_from_ieee80211(vif);
	int ret = 0;
	/* To prevent re-applying PM request OID again and again */
	bool old_uapsdFlags;

	if (WARN_ON(!priv))
		return -EOPNOTSUPP;

#ifdef P2P_MULTIVIF
	if (priv->if_id == RK960_GENERIC_IF_ID)
		return 0;
#endif

	mutex_lock(&hw_priv->conf_mutex);

	if (queue < dev->queues) {
		old_uapsdFlags = priv->uapsd_info.uapsdFlags;

		WSM_TX_QUEUE_SET(&priv->tx_queue_params, queue, 0, 0, 0);
		ret = wsm_set_tx_queue_params(hw_priv,
					      &priv->tx_queue_params.
					      params[queue], queue,
					      priv->if_id);
		if (ret) {
			ret = -EINVAL;
			goto out;
		}

		WSM_EDCA_SET(&priv->edca, queue, params->aifs,
			     params->cw_min, params->cw_max, params->txop, 0xc8,
			     params->uapsd);
		ret = wsm_set_edca_params(hw_priv, &priv->edca, priv->if_id);
		if (ret) {
			ret = -EINVAL;
			goto out;
		}

		if (priv->mode == NL80211_IFTYPE_STATION) {
			ret = rk960_set_uapsd_param(priv, &priv->edca);
			if (!ret && priv->setbssparams_done &&
			    (priv->join_status == RK960_JOIN_STATUS_STA) &&
			    (old_uapsdFlags != priv->uapsd_info.uapsdFlags))
				rk960_set_pm(priv, &priv->powersave_mode);
		}
	} else
		ret = -EINVAL;

out:
	mutex_unlock(&hw_priv->conf_mutex);
	return ret;
}

int rk960_get_stats(struct ieee80211_hw *dev,
		    struct ieee80211_low_level_stats *stats)
{
	struct rk960_common *hw_priv = dev->priv;

	memcpy(stats, &hw_priv->stats, sizeof(*stats));
	return 0;
}

/*
int rk960_get_tx_stats(struct ieee80211_hw *dev,
			struct ieee80211_tx_queue_stats *stats)
{
	int i;
	struct rk960_common *priv = dev->priv;

	for (i = 0; i < dev->queues; ++i)
		rk960_queue_get_stats(&priv->tx_queue[i], &stats[i]);

	return 0;
}
*/

bool rk960_is_p2p_connect(struct rk960_common * hw_priv)
{
	struct rk960_vif *vif;
	int i;

	for (i = 0; i < RK960_MAX_VIFS; i++) {
		vif = hw_priv->vif_list[i] ?
		    rk960_get_vif_from_ieee80211(hw_priv->vif_list[i]) : NULL;
		if (vif && vif->vif && vif->vif->p2p &&
		    vif->join_status >= RK960_JOIN_STATUS_STA)
			return true;
	}
	return false;
}

int rk960_set_pm(struct rk960_vif *priv, const struct wsm_set_pm *arg)
{
	struct wsm_set_pm pm = *arg;

	if (priv->uapsd_info.uapsdFlags != 0)
		pm.pmMode &= ~WSM_PSM_FAST_PS_FLAG;

	if (memcmp(&pm, &priv->firmware_ps_mode, sizeof(struct wsm_set_pm))) {

		pm.fastPsmIdlePeriod = 0;
		if (disable_ps) {
			if (pm.pmMode != WSM_PSM_PS) {
				pm.pmMode = WSM_PSM_ACTIVE;
			}
		} else {
			if (rk960_is_p2p_connect(priv->hw_priv) &&
			    pm.pmMode != WSM_PSM_PS) {
				pm.pmMode = WSM_PSM_ACTIVE;
			} else {
				pm.pmMode = WSM_PSM_FAST_PS;
			}
		}

		priv->firmware_ps_mode = pm;
		return wsm_set_pm(priv->hw_priv, &pm, priv->if_id);
	} else {
		return 0;
	}
}

int rk960_set_key(struct ieee80211_hw *dev, enum set_key_cmd cmd,
		  struct ieee80211_vif *vif, struct ieee80211_sta *sta,
		  struct ieee80211_key_conf *key)
{
	int ret = -EOPNOTSUPP;
	struct rk960_common *hw_priv = dev->priv;
	struct rk960_vif *priv = rk960_get_vif_from_ieee80211(vif);

#ifdef SUPPORT_FWCR
        rk960_fwcr_wait_resumed(hw_priv);
#endif

#ifdef P2P_MULTIVIF
	WARN_ON(priv->if_id == RK960_GENERIC_IF_ID);
#endif
	mutex_lock(&hw_priv->conf_mutex);

	priv->havePMF = false;

	if (cmd == SET_KEY) {
		u8 *peer_addr = NULL;
		int pairwise = (key->flags & IEEE80211_KEY_FLAG_PAIRWISE) ?
		    1 : 0;
		int idx = rk960_alloc_key(hw_priv);
		struct wsm_add_key *wsm_key = &hw_priv->keys[idx];

		if (idx < 0) {
			ret = -EINVAL;
			goto finally;
		}

		BUG_ON(pairwise && !sta);
		if (sta)
			peer_addr = sta->addr;

//#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
//              key->flags |= IEEE80211_KEY_FLAG_PUT_IV_SPACE;
//#else
//              key->flags |= IEEE80211_KEY_FLAG_ALLOC_IV;
//#endif

		if (key->cipher != WLAN_CIPHER_SUITE_AES_CMAC)
			priv->cipherType = key->cipher;
		switch (key->cipher) {
		case WLAN_CIPHER_SUITE_WEP40:
		case WLAN_CIPHER_SUITE_WEP104:
			if (key->keylen > 16) {
				rk960_free_key(hw_priv, idx);
				ret = -EINVAL;
				goto finally;
			}

			if (pairwise) {
				wsm_key->type = WSM_KEY_TYPE_WEP_PAIRWISE;
				memcpy(wsm_key->wepPairwiseKey.peerAddress,
				       peer_addr, ETH_ALEN);
				memcpy(wsm_key->wepPairwiseKey.keyData,
				       &key->key[0], key->keylen);
				wsm_key->wepPairwiseKey.keyLength = key->keylen;
			} else {
				wsm_key->type = WSM_KEY_TYPE_WEP_DEFAULT;
				memcpy(wsm_key->wepGroupKey.keyData,
				       &key->key[0], key->keylen);
				wsm_key->wepGroupKey.keyLength = key->keylen;
				wsm_key->wepGroupKey.keyId = key->keyidx;
#ifdef SUPPORT_FWCR
                                rk960_fwcr_group_frame_capture(
                                                hw_priv, key->keyidx);
#endif
			}
			break;
		case WLAN_CIPHER_SUITE_TKIP:
			if (pairwise) {
				wsm_key->type = WSM_KEY_TYPE_TKIP_PAIRWISE;
				memcpy(wsm_key->tkipPairwiseKey.peerAddress,
				       peer_addr, ETH_ALEN);
				memcpy(wsm_key->tkipPairwiseKey.tkipKeyData,
				       &key->key[0], 16);
				memcpy(wsm_key->tkipPairwiseKey.txMicKey,
				       &key->key[16], 8);
				memcpy(wsm_key->tkipPairwiseKey.rxMicKey,
				       &key->key[24], 8);
				RK960_DEBUG_STA("%s: PTK: type %d %pM\n",
						__func__, wsm_key->type,
						peer_addr);
			} else {
				int mic_offset =
				    (priv->mode == NL80211_IFTYPE_AP) ? 16 : 24;
				wsm_key->type = WSM_KEY_TYPE_TKIP_GROUP;
				memcpy(wsm_key->tkipGroupKey.tkipKeyData,
				       &key->key[0], 16);
				memcpy(wsm_key->tkipGroupKey.rxMicKey,
				       &key->key[mic_offset], 8);

				/* TODO: Where can I find TKIP SEQ? */
				memset(wsm_key->tkipGroupKey.rxSeqCounter,
				       0, 8);
				wsm_key->tkipGroupKey.keyId = key->keyidx;
				RK960_DEBUG_STA
				    ("%s: GTK: type %d keyidx %d mic_offset = %d\n",
				     __func__, wsm_key->type, key->keyidx,
				     mic_offset);
#ifdef SUPPORT_FWCR
                                rk960_fwcr_group_frame_capture(
                                                hw_priv, key->keyidx);
#endif
			}
			break;
		case WLAN_CIPHER_SUITE_CCMP:
			if (pairwise) {
				wsm_key->type = WSM_KEY_TYPE_AES_PAIRWISE;
				memcpy(wsm_key->aesPairwiseKey.peerAddress,
				       peer_addr, ETH_ALEN);
				memcpy(wsm_key->aesPairwiseKey.aesKeyData,
				       &key->key[0], 16);
				RK960_DEBUG_STA("%s: PTK: type %d %pM\n",
						__func__, wsm_key->type,
						peer_addr);
                                //print_hex_dump(KERN_INFO, " ", DUMP_PREFIX_NONE,
                                //        16, 1, &key->key[0], 16, 1);
			} else {
				wsm_key->type = WSM_KEY_TYPE_AES_GROUP;
				memcpy(wsm_key->aesGroupKey.aesKeyData,
				       &key->key[0], 16);
				/* TODO: Where can I find AES SEQ? */
				memset(wsm_key->aesGroupKey.rxSeqCounter, 0, 8);
				wsm_key->aesGroupKey.keyId = key->keyidx;
				RK960_DEBUG_STA("%s: GTK: type %d idx %d\n",
						__func__, wsm_key->type,
						wsm_key->aesGroupKey.keyId);
                                //print_hex_dump(KERN_INFO, " ", DUMP_PREFIX_NONE,
                                //        16, 1, &key->key[0], 16, 1);
#ifdef SUPPORT_FWCR
                                rk960_fwcr_group_frame_capture(
                                                hw_priv, key->keyidx);
#endif                                
			}
			break;
		case WLAN_CIPHER_SUITE_AES_CMAC:	/* 802.11w */
			{
				struct wsm_protected_mgmt_policy mgmt_policy;
				mgmt_policy.protectedMgmtEnable = 1;
				mgmt_policy.unprotectedMgmtFramesAllowed = 1;
				mgmt_policy.encryptionForAuthFrame = 1;
				wsm_set_protected_mgmt_policy(hw_priv,
							      &mgmt_policy,
							      priv->if_id);
				RK960_DEBUG_STA("%s: IGTK_GROUP keyidx %d\n",
						__func__, key->keyidx);
				wsm_key->type = WSM_KEY_TYPE_IGTK_GROUP;
				memcpy(wsm_key->igtkGroupKey.igtKeyData,
				       &key->key[0], 16);
				memset(wsm_key->igtkGroupKey.ipn, 0, 8);
				wsm_key->igtkGroupKey.keyId = key->keyidx;
				hw_priv->igtk_key_index = key->keyidx;
				priv->havePMF = true;
			}
			break;
#ifdef CONFIG_RK960_WAPI_SUPPORT
		case WLAN_CIPHER_SUITE_SMS4:
			if (pairwise) {
				wsm_key->type = WSM_KEY_TYPE_WAPI_PAIRWISE;
				memcpy(wsm_key->wapiPairwiseKey.peerAddress,
				       peer_addr, ETH_ALEN);
				memcpy(wsm_key->wapiPairwiseKey.wapiKeyData,
				       &key->key[0], 16);
				memcpy(wsm_key->wapiPairwiseKey.micKeyData,
				       &key->key[16], 16);
				wsm_key->wapiPairwiseKey.keyId = key->keyidx;
			} else {
				wsm_key->type = WSM_KEY_TYPE_WAPI_GROUP;
				memcpy(wsm_key->wapiGroupKey.wapiKeyData,
				       &key->key[0], 16);
				memcpy(wsm_key->wapiGroupKey.micKeyData,
				       &key->key[16], 16);
				wsm_key->wapiGroupKey.keyId = key->keyidx;
			}
			break;
#endif /* CONFIG_RK960_WAPI_SUPPORT */
		default:
			WARN_ON(1);
			rk960_free_key(hw_priv, idx);
			ret = -EOPNOTSUPP;
			goto finally;
		}
#ifdef SUPPORT_FWCR
                if (!hw_priv->fwcr_update_key) {
                        RK960_INFO_STA("%s: use fwcr_keys %d\n",
                                __func__, idx);
                        WARN_ON(idx != hw_priv->fwcr_keys[idx].entryIndex);
                        WARN_ON(wsm_key->type != hw_priv->fwcr_keys[idx].type);
                        wsm_key = &hw_priv->fwcr_keys[idx];
                }
#endif
		ret = WARN_ON(wsm_add_key(hw_priv, wsm_key, priv->if_id));
		if (!ret)
			key->hw_key_idx = idx;
		else
			rk960_free_key(hw_priv, idx);

		if (!ret && (pairwise
			     || wsm_key->type == WSM_KEY_TYPE_WEP_DEFAULT)
		    && (priv->filter4.enable & 0x2))
			rk960_set_arpreply(dev, vif);
#ifdef IPV6_FILTERING
		if (!ret && (pairwise
			     || wsm_key->type == WSM_KEY_TYPE_WEP_DEFAULT)
		    && (priv->filter6.enable & 0x2))
			rk960_set_na(dev, vif);
#endif /*IPV6_FILTERING */

	} else if (cmd == DISABLE_KEY) {
		struct wsm_remove_key wsm_key = {
			.entryIndex = key->hw_key_idx,
		};

		if (wsm_key.entryIndex > WSM_KEY_MAX_IDX) {
			ret = -EINVAL;
			goto finally;
		}

		ret = wsm_remove_key(hw_priv, &wsm_key, priv->if_id);
		rk960_free_key(hw_priv, wsm_key.entryIndex);
	} else {
		BUG_ON("Unsupported command");
	}

finally:
	mutex_unlock(&hw_priv->conf_mutex);
	return ret;
}

void rk960_wep_key_work(struct work_struct *work)
{
	struct rk960_vif *priv =
	    container_of(work, struct rk960_vif, wep_key_work);
	struct rk960_common *hw_priv = rk960_vifpriv_to_hwpriv(priv);
	u8 queueId = rk960_queue_get_queue_id(hw_priv->pending_frame_id);
	struct rk960_queue *queue = &hw_priv->tx_queue[queueId];
	__le32 wep_default_key_id = __cpu_to_le32(priv->wep_default_key_id);

	BUG_ON(queueId >= 4);

	RK960_DEBUG_STA("[STA] Setting default WEP key: %d\n",
			priv->wep_default_key_id);
	wsm_flush_tx(hw_priv);
	WARN_ON(wsm_write_mib(hw_priv, WSM_MIB_ID_DOT11_WEP_DEFAULT_KEY_ID,
			      &wep_default_key_id, sizeof(wep_default_key_id),
			      priv->if_id));
#ifdef CONFIG_RK960_TESTMODE
	rk960_queue_requeue(hw_priv, queue, hw_priv->pending_frame_id, true);
#else
	rk960_queue_requeue(queue, hw_priv->pending_frame_id, true);
#endif
	wsm_unlock_tx(hw_priv);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
static void rk960_set_rts_threshold_iter(void *data, u8 * mac,
					 struct ieee80211_vif *vif)
{
	int ret;
	__le32 val32;
	struct rk960_vif *priv = rk960_get_vif_from_ieee80211(vif);
	int if_id = priv ? priv->if_id : 0;
	struct rk960_common *hw_priv = priv->hw_priv;
	u32 value = *((u32 *) data);

	RK960_DEBUG_STA("%s: if_id %d %x\n", __func__, priv->if_id, value);

#ifdef P2P_MULTIVIF
	WARN_ON(priv->if_id == RK960_GENERIC_IF_ID);
#endif

	if (value != (u32) - 1)
		val32 = __cpu_to_le32(value);
	else
#if 0
		val32 = 0;	/* disabled */
#else
		val32 = value;
#endif

	/* mutex_lock(&priv->conf_mutex); */
	ret = WARN_ON(wsm_write_mib(hw_priv, WSM_MIB_ID_DOT11_RTS_THRESHOLD,
				    &val32, sizeof(val32), if_id));
	/* mutex_unlock(&priv->conf_mutex); */
	return;
}

int rk960_set_rts_threshold(struct ieee80211_hw *hw, u32 value)
{
	u32 value_t = value;

	RK960_DEBUG_STA("%s: %x\n", __func__, value);

	ieee80211_iterate_active_interfaces_atomic(hw,
						   IEEE80211_IFACE_ITER_NORMAL,
						   rk960_set_rts_threshold_iter,
						   (void *)&value_t);
	return 0;
}
#else
int rk960_set_rts_threshold(struct ieee80211_hw *hw,
			    struct ieee80211_vif *vif, u32 value)
{
	struct rk960_common *hw_priv = hw->priv;
	int ret;
	__le32 val32;
	struct rk960_vif *priv = rk960_get_vif_from_ieee80211(vif);
	int if_id = priv ? priv->if_id : 0;

#ifdef P2P_MULTIVIF
	WARN_ON(priv->if_id == RK960_GENERIC_IF_ID);
#endif

	if (value != (u32) - 1)
		val32 = __cpu_to_le32(value);
	else
#if 0
		val32 = 0;	/* disabled */
#else
		val32 = value;
#endif

	/* mutex_lock(&priv->conf_mutex); */
	ret = WARN_ON(wsm_write_mib(hw_priv, WSM_MIB_ID_DOT11_RTS_THRESHOLD,
				    &val32, sizeof(val32), if_id));
	/* mutex_unlock(&priv->conf_mutex); */
	return ret;
}
#endif

/* TODO: COMBO: Flush only a particular interface specific parts */
int __rk960_flush(struct rk960_common *hw_priv, bool drop, int if_id)
{
	int i, ret;
	struct rk960_vif *priv = __rk960_hwpriv_to_vifpriv(hw_priv, if_id);

	RK960_DEBUG_STA("%s: drop %d if_id %d\n", __func__, drop, if_id);

	for (;;) {
		/* TODO: correct flush handling is required when dev_stop.
		 * Temporary workaround: 2s
		 */
		if (drop) {
			/* overround low probility no set_pm indicate issue */
			wsm_set_pm_indication(hw_priv, NULL);

			wsm_lock_tx(hw_priv);
			wsm_flush_tx(hw_priv);
			for (i = 0; i < 4; ++i)
				rk960_queue_clear(&hw_priv->tx_queue[i], if_id);
			wsm_unlock_tx(hw_priv);
		} else {
			rk960_bh_wakeup(hw_priv);
			ret =
			    wait_event_timeout(hw_priv->tx_queue_stats.
					       wait_link_id_empty,
					       rk960_queue_stats_is_empty
					       (&hw_priv->tx_queue_stats, -1,
						if_id),
#ifdef FPGA_SETUP
					       10 * HZ);
#else
					       10 * HZ);
#endif
		}

		if (!drop && unlikely(ret <= 0) &&
		    !hw_priv->fw_error_processing) {
			RK960_ERROR_STA("__rk960_flush: ETIMEDOUT.....\n");
			ret = -ETIMEDOUT;
			break;
		} else {
			ret = 0;
		}

		wsm_vif_lock_tx(priv);
		if (unlikely
		    (!rk960_queue_stats_is_empty
		     (&hw_priv->tx_queue_stats, -1, if_id))) {
			/* Highly unlekely: WSM requeued frames. */
			wsm_unlock_tx(hw_priv);
			continue;
		}
		break;
	}
	return ret;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
void rk960_flush(struct ieee80211_hw *hw,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0))
		 struct ieee80211_vif *vif,
#endif
		 u32 queues, bool drop)
#else
void rk960_flush(struct ieee80211_hw *hw, struct ieee80211_vif *vif, bool drop)
#endif
{
	struct rk960_common *hw_priv = hw->priv;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0))
	struct rk960_vif *priv;

	if (!vif) {
		return;
	}
	priv = rk960_get_vif_from_ieee80211(vif);
#else
	struct rk960_vif *priv = hw_priv->vif_list[hw_priv->def_vif_id] ?
	    rk960_get_vif_from_ieee80211(hw_priv->
					 vif_list[hw_priv->def_vif_id]) : NULL;
#endif
#else
	struct rk960_vif *priv = rk960_get_vif_from_ieee80211(vif);
#endif

	if (!priv) {
		return;
	}

	/*TODO:COMBO: reenable this part of code when flush callback
	 * is implemented per vif */
	/*switch (hw_priv->mode) {
	   case NL80211_IFTYPE_MONITOR:
	   drop = true;
	   break;
	   case NL80211_IFTYPE_AP:
	   if (!hw_priv->enable_beacon)
	   drop = true;
	   break;
	   } */

	if (!(hw_priv->if_id_slot & BIT(priv->if_id)))
		return;

	if (!WARN_ON(__rk960_flush(hw_priv, drop, priv->if_id)))
		wsm_unlock_tx(hw_priv);

	return;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
int rk960_remain_on_channel(struct ieee80211_hw *hw,
			    struct ieee80211_vif *vif,
			    struct ieee80211_channel *chan,
			    int duration, enum ieee80211_roc_type type)
#else
int rk960_remain_on_channel(struct ieee80211_hw *hw,
			    struct ieee80211_vif *vif,
			    struct ieee80211_channel *chan,
			    enum nl80211_channel_type channel_type,
			    int duration, u64 cookie)
#endif
{
	int ret;
	struct rk960_common *hw_priv = hw->priv;
	struct rk960_vif *priv = rk960_get_vif_from_ieee80211(vif);
	int if_id = priv->if_id;

	down(&hw_priv->scan.lock);
	mutex_lock(&hw_priv->conf_mutex);

#ifdef ROC_DEBUG
	RK960_DEBUG_STA("ROC IN %d ch %d duration %d\n",
			priv->if_id, chan->hw_value, duration);
#endif
	hw_priv->roc_start_time = jiffies;
	hw_priv->roc_duration = duration;
	hw_priv->roc_if_id = priv->if_id;
	hw_priv->roc_if_id_last = priv->if_id;
	ret = WARN_ON(__rk960_flush(hw_priv, false, if_id));
	wsm_unlock_tx(hw_priv);
#ifdef RK960_CSYNC_ADJUST
	rk960_csync_scan(hw_priv);
#endif
	rk960_enable_listening(priv, chan);

	if (!ret) {
		atomic_set(&hw_priv->remain_on_channel, 1);
#if 1
		queue_delayed_work(hw_priv->workqueue,
				   &hw_priv->rem_chan_timeout,
				   duration * HZ / 1000);
#else
		ieee80211_queue_delayed_work(hw_priv->hw,
					     &hw_priv->rem_chan_timeout,
					     msecs_to_jiffies(duration));
#endif
		priv->join_status_restore = priv->join_status;
		priv->join_status = RK960_JOIN_STATUS_MONITOR;
		ieee80211_ready_on_channel(hw);
	} else {
		hw_priv->roc_if_id = -1;
		up(&hw_priv->scan.lock);
		BUG_ON(1);
	}

#ifdef ROC_DEBUG
	RK960_DEBUG_STA("ROC OUT %d\n", priv->if_id);
#endif
	/* set the channel to supplied ieee80211_channel pointer, if it
	   is not set. This is to remove the crash while sending a probe res
	   in listen state. Later channel will updated on
	   IEEE80211_CONF_CHANGE_CHANNEL event */
	if (!hw_priv->channel) {
		hw_priv->channel = chan;
	}
	hw_priv->ro_channel = chan;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0))
	hw_priv->roc_cookie = cookie;
#endif
	mutex_unlock(&hw_priv->conf_mutex);
	return ret;
}

#if (LINUX_VERSION_CODE > KERNEL_VERSION(5, 10, 0))
int rk960_cancel_remain_on_channel(struct ieee80211_hw *hw,
				   struct ieee80211_vif *vif)
#else
int rk960_cancel_remain_on_channel(struct ieee80211_hw *hw)
#endif
{
	struct rk960_common *hw_priv = hw->priv;

	RK960_DEBUG_STA("[STA] Cancel remain on channel\n");

	atomic_set(&hw_priv->cancel_roc, 1);
	if (atomic_read(&hw_priv->remain_on_channel))
		cancel_delayed_work_sync(&hw_priv->rem_chan_timeout);

	if (atomic_read(&hw_priv->remain_on_channel))
		rk960_rem_chan_timeout(&hw_priv->rem_chan_timeout.work);
	atomic_set(&hw_priv->cancel_roc, 0);

	return 0;
}

/* ******************************************************************** */
/* WSM callbacks							*/

void rk960_channel_switch_cb(struct rk960_common *hw_priv)
{
	wsm_unlock_tx(hw_priv);
}

void rk960_free_event_queue(struct rk960_common *hw_priv)
{
	LIST_HEAD(list);

	spin_lock(&hw_priv->event_queue_lock);
	list_splice_init(&hw_priv->event_queue, &list);
	spin_unlock(&hw_priv->event_queue_lock);

	__rk960_free_event_queue(&list);
}

struct sk_buff *rk960_build_deauth_frame(u8 * sa, u8 * da,
					 u8 * bssid, bool tods)
{
	struct sk_buff *skb;
	struct ieee80211_mgmt *deauth;

	skb = dev_alloc_skb(sizeof(struct ieee80211_mgmt) + 64);
	if (!skb) {
		return NULL;
	}

	skb_reserve(skb, 64);
	deauth = (struct ieee80211_mgmt *)skb_put(skb,
						  sizeof(struct
							 ieee80211_mgmt));
	deauth->duration = 0;
	memcpy(deauth->da, da, ETH_ALEN);
	memcpy(deauth->sa, sa, ETH_ALEN);
	memcpy(deauth->bssid, bssid, ETH_ALEN);
	deauth->frame_control = cpu_to_le16(IEEE80211_FTYPE_MGMT |
					    IEEE80211_STYPE_DEAUTH |
					    (tods ? IEEE80211_FCTL_TODS :
					     IEEE80211_FCTL_FROMDS));
	deauth->u.deauth.reason_code = WLAN_REASON_DEAUTH_LEAVING;
	deauth->seq_ctrl = 0;

	return skb;
}

void rk960_event_handler(struct work_struct *work)
{
	struct rk960_common *hw_priv =
	    container_of(work, struct rk960_common, event_handler);
	struct rk960_vif *priv;
	struct rk960_wsm_event *event;
	LIST_HEAD(list);

	spin_lock(&hw_priv->event_queue_lock);
	list_splice_init(&hw_priv->event_queue, &list);
	spin_unlock(&hw_priv->event_queue_lock);

	mutex_lock(&hw_priv->conf_mutex);
	list_for_each_entry(event, &list, link) {
		priv = __rk960_hwpriv_to_vifpriv(hw_priv, event->if_id);
		if (!priv) {
			RK960_DEBUG_STA("[CQM] Event for non existing "
					"interface, ignoring.\n");
			continue;
		}
		switch (event->evt.eventId) {
		case WSM_EVENT_ERROR:
			/* I even don't know what is it about.. */
			//STUB();
			break;
		case WSM_EVENT_BSS_LOST:
			{
				rk960_pm_stay_awake(&hw_priv->pm_state, 3 * HZ);

				spin_lock(&priv->bss_loss_lock);
				if (priv->bss_loss_status > RK960_BSS_LOSS_NONE) {
					spin_unlock(&priv->bss_loss_lock);
					break;
				}
				priv->bss_loss_status = RK960_BSS_LOSS_CHECKING;
				spin_unlock(&priv->bss_loss_lock);

				RK960_INFO_STA("[CQM] BSS lost.\n");
				cancel_delayed_work_sync(&priv->bss_loss_work);
				cancel_delayed_work_sync(&priv->
							 connection_loss_work);
				if (!down_trylock(&hw_priv->scan.lock)) {
					up(&hw_priv->scan.lock);
					priv->delayed_link_loss = 0;
					queue_delayed_work(hw_priv->workqueue,
							   &priv->bss_loss_work,
							   0);
				} else {
					/* Scan is in progress. Delay reporting. */
					/* Scan complete will trigger bss_loss_work */
					priv->delayed_link_loss = 1;
					/* Also we're starting watchdog. */
					queue_delayed_work(hw_priv->workqueue,
							   &priv->bss_loss_work,
							   10 * HZ);
				}
				break;
			}
		case WSM_EVENT_BSS_REGAINED:
			{
				RK960_DEBUG_STA("[CQM] BSS regained.\n");
				priv->delayed_link_loss = 0;
				spin_lock(&priv->bss_loss_lock);
				priv->bss_loss_status = RK960_BSS_LOSS_NONE;
				spin_unlock(&priv->bss_loss_lock);
				cancel_delayed_work_sync(&priv->bss_loss_work);
				cancel_delayed_work_sync(&priv->
							 connection_loss_work);
				break;
			}
		case WSM_EVENT_RADAR_DETECTED:
			//STUB();
			break;
		case WSM_EVENT_RCPI_RSSI:
			{
				/* RSSI: signed Q8.0, RCPI: unsigned Q7.1
				 * RSSI = RCPI / 2 - 110 */
				int rcpiRssi =
				    (int)(event->evt.eventData & 0xFF);
				int cqm_evt;
				if (priv->cqm_use_rssi)
					rcpiRssi = (s8) rcpiRssi;
				else
					rcpiRssi = rcpiRssi / 2 - 110;

				cqm_evt = (rcpiRssi <= priv->cqm_rssi_thold) ?
				    NL80211_CQM_RSSI_THRESHOLD_EVENT_LOW :
				    NL80211_CQM_RSSI_THRESHOLD_EVENT_HIGH;
				RK960_DEBUG_STA("[CQM] RSSI event: %d(%d)",
						rcpiRssi, priv->cqm_rssi_thold);
				ieee80211_cqm_rssi_notify(priv->vif, cqm_evt,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
							  rcpiRssi,
#endif
							  GFP_KERNEL);
				break;
			}
		case WSM_EVENT_BT_INACTIVE:
			//STUB();
			break;
		case WSM_EVENT_BT_ACTIVE:
			//STUB();
			break;
		case WSM_EVENT_INACTIVITY:
			{
				int link_id =
				    ffs((u32) (event->evt.eventData)) - 1;
				struct sk_buff *skb;
				struct rk960_link_entry *entry = NULL;

				RK960_INFO_STA("Inactivity Event Recieved for "
					       "link_id %d\n", link_id);
				rk960_unmap_link(priv, link_id);

				entry = &priv->link_id_db[link_id - 1];
				skb =
				    rk960_build_deauth_frame(entry->mac,
							     priv->vif->addr,
							     priv->vif->addr,
							     true);
				if (skb)
					ieee80211_rx_irqsafe(priv->hw, skb);
				RK960_INFO_STA
				    (" Inactivity Deauth Frame sent for MAC SA %pM \t and DA %pM\n",
				     entry->mac, priv->vif->addr);
				queue_work(priv->hw_priv->workqueue,
					   &priv->set_tim_work);
				break;
			}
		case WSM_EVENT_PS_MODE_ERROR:
			{
				if (!priv->uapsd_info.uapsdFlags &&
				    (priv->user_pm_mode != WSM_PSM_PS)) {
					struct wsm_set_pm pm =
					    priv->powersave_mode;
					int ret = 0;

					priv->powersave_mode.pmMode =
					    WSM_PSM_ACTIVE;
					ret =
					    rk960_set_pm(priv,
							 &priv->powersave_mode);
					if (ret)
						priv->powersave_mode = pm;
				}
				break;
			}
		}
	}
	mutex_unlock(&hw_priv->conf_mutex);
	__rk960_free_event_queue(&list);
}

void rk960_bss_loss_work(struct work_struct *work)
{
	struct rk960_vif *priv =
	    container_of(work, struct rk960_vif, bss_loss_work.work);
	struct rk960_common *hw_priv = rk960_vifpriv_to_hwpriv(priv);
	int timeout;		/* in beacons */
#if 0
	struct sk_buff *skb;
#endif

	timeout = priv->cqm_link_loss_count - priv->cqm_beacon_loss_count;

	/* Skip the confimration procedure in P2P case */
	if (priv->vif->p2p)
		goto report;

	spin_lock(&priv->bss_loss_lock);
	if (priv->bss_loss_status == RK960_BSS_LOSS_CHECKING) {
#if 0
		struct ieee80211_tx_info *info;
#endif
//              spin_unlock(&priv->bss_loss_lock);
#if 0
		skb = ieee80211_nullfunc_get(priv->hw, priv->vif);
		if (!(WARN_ON(!skb))) {
			info = IEEE80211_SKB_CB(skb);
			info->control.vif = priv->vif;
			info->control.sta = NULL;
			rk960_tx(priv->hw, skb);
			/* Start watchdog -- if nullfunc TX doesn't fail
			 * in 1 sec, forward event to upper layers */
			queue_delayed_work(hw_priv->workqueue,
					   &priv->bss_loss_work, 1 * HZ);
		}
#endif
		priv->bss_loss_status = RK960_BSS_LOSS_CONFIRMED;
#if 0
		return;
#endif
	} else if (priv->bss_loss_status == RK960_BSS_LOSS_CONFIRMING) {
		priv->bss_loss_status = RK960_BSS_LOSS_NONE;
		spin_unlock(&priv->bss_loss_lock);
		return;
	}
	spin_unlock(&priv->bss_loss_lock);

report:
	if (priv->cqm_beacon_loss_count) {
		RK960_DEBUG_STA("[CQM] Beacon loss.\n");
		if (timeout <= 0)
			timeout = 0;
#if defined(CONFIG_RK960_USE_STE_EXTENSIONS)
		ieee80211_cqm_beacon_miss_notify(priv->vif, GFP_KERNEL);
#endif /* CONFIG_RK960_USE_STE_EXTENSIONS */
	} else {
		timeout = 0;
	}

	cancel_delayed_work_sync(&priv->connection_loss_work);
	queue_delayed_work(hw_priv->workqueue,
			   &priv->connection_loss_work, timeout * HZ / 10);

	spin_lock(&priv->bss_loss_lock);
	priv->bss_loss_status = RK960_BSS_LOSS_NONE;
	spin_unlock(&priv->bss_loss_lock);
}

void rk960_connection_loss_work(struct work_struct *work)
{
	struct rk960_vif *priv = container_of(work, struct rk960_vif,
					      connection_loss_work.work);
	struct rk960_common *hw_priv = priv->hw_priv;

	RK960_DEBUG_STA("[CQM] Reporting connection loss.\n");

	if (atomic_read(&hw_priv->remain_on_channel)) {
		cancel_delayed_work_sync(&hw_priv->rem_chan_timeout);
		rk960_rem_chan_timeout(&hw_priv->rem_chan_timeout.work);
	}

	ieee80211_connection_loss(priv->vif);
}

void rk960_tx_failure_work(struct work_struct *work)
{
	struct rk960_vif *priv =
	    container_of(work, struct rk960_vif, tx_failure_work);
	RK960_DEBUG_STA("[CQM] Reporting TX failure.\n");
#if defined(CONFIG_RK960_USE_STE_EXTENSIONS)
	ieee80211_cqm_tx_fail_notify(priv->vif, GFP_KERNEL);
#else /* CONFIG_RK960_USE_STE_EXTENSIONS */
	(void)priv;
#endif /* CONFIG_RK960_USE_STE_EXTENSIONS */
}

#ifdef CONFIG_RK960_TESTMODE
/**
 * rk960_device_power_calc- Device power calculation
 * from values fetch from SDD File.
 *
 * @priv: the private structure
 * @Max_output_power: Power fetch from SDD
 * @fe_cor: front-end loss correction
 * @band: Either 2GHz or 5GHz
 *
 */
void rk960_device_power_calc(struct rk960_common *hw_priv,
			     s16 max_output_power, s16 fe_cor, u32 band)
{
	s16 power_calc;

	power_calc = max_output_power - fe_cor;
	if ((power_calc % 16) != 0)
		power_calc += 16;

	hw_priv->txPowerRange[band].max_power_level = power_calc / 16;
	/*
	 * 12dBm is control range supported by firmware.
	 * This means absolute min power is
	 * max_power_level - 12.
	 */
	hw_priv->txPowerRange[band].min_power_level =
	    hw_priv->txPowerRange[band].max_power_level - 12;
	hw_priv->txPowerRange[band].stepping = 1;

}
#endif
/* ******************************************************************** */
/* Internal API								*/

#if 0
/*
* This function is called to Parse the SDD file
 *to extract listen_interval and PTA related information
*/
static int rk960_parse_SDD_file(struct rk960_common *hw_priv)
{
	u8 *sdd_data = (u8 *) hw_priv->sdd->data;
#ifdef CONFIG_RK960_TESTMODE
	s16 max_output_power_2G;
	s16 max_output_power_5G;
	s16 fe_cor_2G;
	s16 fe_cor_5G;
	int i;
#endif
	struct rk960_sdd {
		u8 id;
		u8 length;
		u8 data[];
	} *pElement;
	int parsedLength = 0;
#define SDD_PTA_CFG_ELT_ID 0xEB
#ifdef CONFIG_RK960_TESTMODE
#define SDD_MAX_OUTPUT_POWER_2G4_ELT_ID 0xE3
#define SDD_MAX_OUTPUT_POWER_5G_ELT_ID  0xE4
#define SDD_FE_COR_2G4_ELT_ID   0x30
#define SDD_FE_COR_5G_ELT_ID    0x31
#define MIN(x, y, z) (x < y ? (x < z ? x : z) : (y < z ? y : z))
#endif
#define FIELD_OFFSET(type, field) ((u8 *)&((type *)0)->field - (u8 *)0)

	hw_priv->is_BT_Present = false;

	pElement = (struct rk960_sdd *)sdd_data;

	pElement = (struct rk960_sdd *)((u8 *) pElement +
					FIELD_OFFSET(struct rk960_sdd,
						     data) + pElement->length);

	parsedLength += (FIELD_OFFSET(struct rk960_sdd, data) +
			 pElement->length);

	while (parsedLength <= hw_priv->sdd->size) {
		switch (pElement->id) {
		case SDD_PTA_CFG_ELT_ID:
			{
				hw_priv->conf_listen_interval =
				    (*((u16 *) pElement->data + 1) >> 7) & 0x1F;
				hw_priv->is_BT_Present = true;
				RK960_DEBUG_STA("PTA element found.\n");
				RK960_DEBUG_STA("Listen Interval %d\n",
						hw_priv->conf_listen_interval);
			}
			break;
#ifdef CONFIG_RK960_TESTMODE
		case SDD_MAX_OUTPUT_POWER_2G4_ELT_ID:
			{
				max_output_power_2G = *((s16 *) pElement->data);
			}
			break;
		case SDD_FE_COR_2G4_ELT_ID:
			{
				fe_cor_2G = *((s16 *) pElement->data);
			}
			break;
		case SDD_MAX_OUTPUT_POWER_5G_ELT_ID:
			{
				max_output_power_5G =
				    *((s16 *) (pElement->data + 4));
			}
			break;
		case SDD_FE_COR_5G_ELT_ID:
			{
				fe_cor_5G = MIN(*((s16 *) pElement->data),
						*((s16 *) (pElement->data + 2)),
						*((s16 *) (pElement->data + 4)));

				fe_cor_5G = MIN(fe_cor_5G,
						*((s16 *) (pElement->data + 6)),
						*((s16 *) (pElement->data + 8)));
			}
			break;
#endif

		default:
			break;
		}

		pElement = (struct rk960_sdd *)
		    ((u8 *) pElement + FIELD_OFFSET(struct rk960_sdd, data)
		     + pElement->length);
		parsedLength +=
		    (FIELD_OFFSET(struct rk960_sdd, data) + pElement->length);
	}

	if (hw_priv->is_BT_Present == false) {
		RK960_DEBUG_STA("PTA element NOT found.\n");
		hw_priv->conf_listen_interval = 0;
	}
#ifdef CONFIG_RK960_TESTMODE
	/* Max/Min Power Calculation for 2.4G */
	rk960_device_power_calc(hw_priv, max_output_power_2G,
				fe_cor_2G, IEEE80211_BAND_2GHZ);
	/* Max/Min Power Calculation for 5G */
	rk960_device_power_calc(hw_priv, max_output_power_5G,
				fe_cor_5G, IEEE80211_BAND_5GHZ);

	for (i = 0; i < 2; ++i) {
		RK960_DEBUG_STA("[STA] Power Values Read from SDD %s:"
				"min_power_level[%d]: %d max_power_level[%d]:"
				"%d stepping[%d]: %d\n", __func__, i,
				hw_priv->txPowerRange[i].min_power_level, i,
				hw_priv->txPowerRange[i].max_power_level, i,
				hw_priv->txPowerRange[i].stepping);
	}

	RK960_DEBUG_STA("%s output power before %d\n", __func__,
			hw_priv->output_power);
	/*BUG:TX output power is not set untill config_rk960 is called */
	/*This would lead to 0 power set in fw and would effect scan & p2p-find */
	/*Setting to default value here from sdd which would be overwritten when */
	/*we make connection to AP.This value is used only during scan & p2p-ops */
	/*untill AP connection is made */
	if (!hw_priv->output_power)
		hw_priv->output_power =
		    hw_priv->txPowerRange[IEEE80211_BAND_2GHZ].max_power_level;

	RK960_DEBUG_STA("%s output power after %d\n", __func__,
			hw_priv->output_power);
#else
	RK960_DEBUG_STA("%s output power before %d\n", __func__,
			hw_priv->output_power);
	/*BUG:TX output power: Hardcoding to 20dbm if CCX is not enabled */
	/*TODO: This might change */
	if (!hw_priv->output_power)
		hw_priv->output_power = 20;
	RK960_DEBUG_STA("%s output power after %d\n", __func__,
			hw_priv->output_power);
#endif
	return 0;

#undef SDD_PTA_CFG_ELT_ID
#undef FIELD_OFFSET
}
#endif

int rk960_setup_mac(struct rk960_common *hw_priv)
{
	int ret = 0, if_id;

	if (!hw_priv->sdd) {
		//const char *sdd_path = NULL;
		struct wsm_configuration cfg = {
			//.dot11RtsThreshold = 1548,
		};
		memcpy(cfg.dot11StationId, &hw_priv->mac_addr[0], 6);

		/*ret = request_firmware(&hw_priv->sdd,
		   sdd_path, hw_priv->pdev);

		   if (unlikely(ret)) {
		   rk960_dbg(RK960_DBG_ERROR,
		   "%s: can't load sdd file %s.\n",
		   __func__, sdd_path);
		   return ret;
		   } */

		cfg.dpdData = NULL;	//hw_priv->sdd->data;
		cfg.dpdData_size = 0;	//hw_priv->sdd->size;

		for (if_id = 0; if_id < rk960_get_nr_hw_ifaces(hw_priv);
		     if_id++) {
			/* Set low-power mode. */
			ret |= WARN_ON(wsm_configuration(hw_priv, &cfg, if_id));
		}
		/* Parse SDD file for PTA element */
		//rk960_parse_SDD_file(hw_priv);
		hw_priv->conf_listen_interval = listen_interval;
		hw_priv->is_BT_Present = true;
	}
	if (ret)
		return ret;

	return 0;
}

void rk960_pending_offchanneltx_work(struct work_struct *work)
{
	struct rk960_vif *priv =
	    container_of(work, struct rk960_vif,
			 pending_offchanneltx_work.work);
	struct rk960_common *hw_priv = rk960_vifpriv_to_hwpriv(priv);

	mutex_lock(&hw_priv->conf_mutex);
#ifdef ROC_DEBUG
	RK960_DEBUG_STA("OFFCHAN PEND IN\n");
#endif
	rk960_disable_listening(priv);
	hw_priv->roc_if_id = -1;
#ifdef ROC_DEBUG
	RK960_DEBUG_STA("OFFCHAN PEND OUT\n");
#endif
	up(&hw_priv->scan.lock);
	mutex_unlock(&hw_priv->conf_mutex);
}

void rk960_offchannel_work(struct work_struct *work)
{
	struct rk960_vif *priv =
	    container_of(work, struct rk960_vif, offchannel_work);
	struct rk960_common *hw_priv = rk960_vifpriv_to_hwpriv(priv);
	u8 queueId = rk960_queue_get_queue_id(hw_priv->pending_frame_id);
	struct rk960_queue *queue = &hw_priv->tx_queue[queueId];

	BUG_ON(queueId >= 4);
	BUG_ON(!hw_priv->channel);

	if (unlikely(down_trylock(&hw_priv->scan.lock))) {
		int ret;
		RK960_ERROR_STA("rk960_offchannel_work***** drop frame\n");
#ifdef CONFIG_RK960_TESTMODE
		rk960_queue_remove(hw_priv, queue, hw_priv->pending_frame_id);
#else
		ret = rk960_queue_remove(queue, hw_priv->pending_frame_id);
#endif
		if (ret)
			RK960_ERROR_STA("rk960_offchannel_work: "
					"queue_remove failed %d\n", ret);
		wsm_unlock_tx(hw_priv);
		return;
	}
	mutex_lock(&hw_priv->conf_mutex);
#ifdef ROC_DEBUG
	RK960_DEBUG_STA("OFFCHAN WORK IN %d\n", priv->if_id);
#endif
	hw_priv->roc_if_id = priv->if_id;
	if (likely(!priv->join_status)) {
		wsm_vif_flush_tx(priv);
		if (!hw_priv->ro_channel)
			rk960_enable_listening(priv, hw_priv->channel);
		else
			rk960_enable_listening(priv, hw_priv->ro_channel);
		/* rk960_update_filtering(priv); */
		priv->join_status = RK960_JOIN_STATUS_MONITOR;
	}
	if (unlikely(!priv->join_status))
#ifdef CONFIG_RK960_TESTMODE
		rk960_queue_remove(hw_priv, queue, hw_priv->pending_frame_id);
#else
		rk960_queue_remove(queue, hw_priv->pending_frame_id);
#endif /*CONFIG_RK960_TESTMODE */
	else
#ifdef CONFIG_RK960_TESTMODE
		rk960_queue_requeue(hw_priv, queue,
				    hw_priv->pending_frame_id, false);
#else
		rk960_queue_requeue(queue, hw_priv->pending_frame_id, false);
#endif

	queue_delayed_work(hw_priv->workqueue,
			   &priv->pending_offchanneltx_work, 204 * HZ / 1000);
#ifdef ROC_DEBUG
	RK960_DEBUG_STA("OFFCHAN WORK OUT %d\n", priv->if_id);
#endif
	mutex_unlock(&hw_priv->conf_mutex);
	wsm_unlock_tx(hw_priv);
}

#ifdef RK960_CHANNEL_CONFLICT_OPT
int rk960_ap_channel_switch(struct rk960_common *hw_priv,
			    int if_id, int channel)
{
	int if_id2;
	struct rk960_vif *priv2;

	RK960_DEBUG_STA("%s: if_id %d channel %d\n", __func__, if_id, channel);

	if (if_id)
		if_id2 = 0;
	else
		if_id2 = 1;
	if (hw_priv->if_id_slot & BIT(if_id2)) {
		priv2 = rk960_get_vif_from_ieee80211(hw_priv->vif_list[if_id2]);
		if (priv2 && priv2->join_status == RK960_JOIN_STATUS_AP &&
		    priv2->current_channel != channel) {
			struct wsm_switch_channel arg_sc;
			struct wsm_template_frame arg_tf;
			int i;

			RK960_INFO_STA("%s: channel conflit STA ch%d AP ch%d\n",
				       __func__, channel,
				       priv2->current_channel);
			priv2->current_channel = channel;

#if 1
			arg_sc.channelMode = 1;
			arg_sc.channelSwitchCount = 0;
			arg_sc.newChannelNumber = channel;

			wsm_lock_tx_async(hw_priv);
			mutex_lock(&hw_priv->conf_mutex);

			wsm_switch_channel(hw_priv, &arg_sc, if_id2);
#else
			mutex_lock(&hw_priv->conf_mutex);

			rk960_update_beaconing(priv2, 1);
#endif
			/* reset beacon/probe resp template frame */
			for (i = 0; i < 2; i++) {
				arg_tf.disable =
				    priv2->beacon_template[i].disable;
				arg_tf.frame_type =
				    priv2->beacon_template[i].frame_type;
				arg_tf.rate = priv2->beacon_template[i].rate;
				arg_tf.skb =
				    dev_alloc_skb(priv2->
						  beacon_template_len[i]);
				if (arg_tf.skb) {
					memcpy(skb_put
					       (arg_tf.skb,
						priv2->beacon_template_len[i]),
					       priv2->beacon_template_frame[i],
					       priv2->beacon_template_len[i]);
					wsm_set_template_frame(hw_priv, &arg_tf,
							       if_id2);
					dev_kfree_skb(arg_tf.skb);
				}
			}

			mutex_unlock(&hw_priv->conf_mutex);
		}
	}

	return 0;
}

void rk960_channel_switch_work(struct work_struct *work)
{
	struct rk960_vif *priv =
	    container_of(work, struct rk960_vif, channel_switch_work);
	struct rk960_common *hw_priv = rk960_vifpriv_to_hwpriv(priv);

	BUG_ON(!hw_priv->channel);

	rk960_ap_channel_switch(hw_priv, priv->if_id,
				hw_priv->channel->hw_value);
}
#endif

void rk960_join_work(struct work_struct *work)
{
	struct rk960_vif *priv =
	    container_of(work, struct rk960_vif, join_work);
	struct rk960_common *hw_priv = rk960_vifpriv_to_hwpriv(priv);
	u8 queueId = rk960_queue_get_queue_id(hw_priv->pending_frame_id);
	struct rk960_queue *queue = &hw_priv->tx_queue[queueId];
	const struct rk960_txpriv *txpriv = NULL;
	struct sk_buff *skb = NULL;
	const struct wsm_tx *wsm;
	const struct ieee80211_hdr *frame;
	const u8 *bssid;
	struct cfg80211_bss *bss;
	const u8 *ssidie;
	const u8 *dtimie;
	const struct ieee80211_tim_ie *tim = NULL;
	struct wsm_protected_mgmt_policy mgmt_policy;
	/*struct wsm_reset reset = {
	   .reset_statistics = true,
	   }; */
	struct wsm_operational_mode mode = {
		.power_mode = wsm_power_mode_quiescent,
		.disableMoreFlagUsage = true,
	};
#ifdef RK960_CHANNEL_CONFLICT_OPT
	int if_id2;
	struct rk960_vif *priv2;
#endif

	RK960_DEBUG_STA("%s: if_id %d\n", __func__, priv->if_id);

#ifdef SUPPORT_FWCR
        if (hw_priv->fwcr_recovery &&
                is_valid_ether_addr(hw_priv->fwcr_bssid)) {
                bssid = hw_priv->fwcr_bssid;
        } else {
#endif
	BUG_ON(queueId >= 4);
	if (rk960_queue_get_skb(queue, hw_priv->pending_frame_id,
				&skb, &txpriv)) {
		wsm_unlock_tx(hw_priv);
		return;
	}
	wsm = (struct wsm_tx *)&skb->data[0];
	frame = (struct ieee80211_hdr *)&skb->data[txpriv->offset];
	bssid = &frame->addr1[0];	/* AP SSID in a 802.11 frame */

	BUG_ON(!wsm);
	BUG_ON(!hw_priv->channel);
#ifdef SUPPORT_FWCR        
        }
#endif

#ifdef RK960_CHANNEL_CONFLICT_OPT
	priv->current_channel = hw_priv->channel->hw_value;
	if (priv->if_id)
		if_id2 = 0;
	else
		if_id2 = 1;
	if (hw_priv->if_id_slot & BIT(if_id2)) {
		priv2 = rk960_get_vif_from_ieee80211(hw_priv->vif_list[if_id2]);
		if (priv2 && priv2->join_status == RK960_JOIN_STATUS_STA &&
		    priv2->current_channel != hw_priv->channel->hw_value) {
			RK960_INFO_STA
			    ("force already used if_id %d channel %d(%d)\n",
			     if_id2, priv2->current_channel,
			     hw_priv->channel->hw_value);
			priv->current_channel = priv2->current_channel;
		}
	}
#endif

	if (unlikely(priv->join_status)) {
		wsm_lock_tx(hw_priv);
		rk960_unjoin_work(&priv->unjoin_work);
	}

	cancel_delayed_work_sync(&priv->join_timeout);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
	bss = cfg80211_get_bss(hw_priv->hw->wiphy, NULL /*hw_priv->channel */ ,
			       bssid, NULL, 0, IEEE80211_BSS_TYPE_ANY,
			       IEEE80211_PRIVACY_ANY);
#else
	bss = cfg80211_get_bss(hw_priv->hw->wiphy, NULL /*hw_priv->channel */ ,
			       bssid, NULL, 0, 0, 0);
#endif
	if (!bss) {
		RK960_DEBUG_STA("%s: cfg80211_get_bss fail\n", __func__);
#ifdef CONFIG_RK960_TESTMODE
		rk960_queue_remove(hw_priv, queue, hw_priv->pending_frame_id);
#else
		rk960_queue_remove(queue, hw_priv->pending_frame_id);
#endif /*CONFIG_RK960_TESTMODE */
		wsm_unlock_tx(hw_priv);
		return;
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
	rcu_read_lock();
	ssidie = ieee80211_bss_get_ie(bss, WLAN_EID_SSID);
	dtimie = ieee80211_bss_get_ie(bss, WLAN_EID_TIM);
	rcu_read_unlock();
#else
	ssidie = cfg80211_find_ie(WLAN_EID_SSID,
				  bss->information_elements,
				  bss->len_information_elements);
	dtimie = cfg80211_find_ie(WLAN_EID_TIM,
				  bss->information_elements,
				  bss->len_information_elements);
#endif
	if (dtimie)
		tim = (struct ieee80211_tim_ie *)&dtimie[2];

	mutex_lock(&hw_priv->conf_mutex);
	{
		struct wsm_join join = {
			.mode = /*(bss->capability & WLAN_CAPABILITY_IBSS) ?
			    WSM_JOIN_MODE_IBSS : */WSM_JOIN_MODE_BSS,
			.preambleType = WSM_JOIN_PREAMBLE_LONG,
			//.preambleType = priv->association_mode.preambleType,
			.probeForJoin = 1,
			/* dtimPeriod will be updated after association */
			.dtimPeriod = 1,
			.beaconInterval = bss->beacon_interval,
		};

                BUG_ON(bss->capability & WLAN_CAPABILITY_IBSS);

		if (priv->if_id)
			join.flags |= WSM_FLAG_MAC_INSTANCE_1;
		else
			join.flags &= ~WSM_FLAG_MAC_INSTANCE_1;

		/* BT Coex related changes */
		if (hw_priv->is_BT_Present) {
			if (((hw_priv->conf_listen_interval * 100) %
			     bss->beacon_interval) == 0)
				priv->listen_interval =
				    ((hw_priv->conf_listen_interval * 100) /
				     bss->beacon_interval);
			else
				priv->listen_interval =
				    ((hw_priv->conf_listen_interval * 100) /
				     bss->beacon_interval + 1);
		}

		if (tim && tim->dtim_period > 1) {
			join.dtimPeriod = tim->dtim_period;
			priv->join_dtim_period = tim->dtim_period;
		}
		priv->beacon_int = bss->beacon_interval;
		RK960_DEBUG_STA("[STA] Join DTIM: %d, "
				"interval: %d rssi %d "
				"listen_interval %d "
				"channel: %d\n",
				join.dtimPeriod, priv->beacon_int,
				bss->signal/100,
				priv->listen_interval,
#ifdef RK960_CHANNEL_CONFLICT_OPT
				priv->current_channel);
#else
				hw_priv->channel->hw_value);
#endif

		hw_priv->is_go_thru_go_neg = false;
#ifdef RK960_CHANNEL_CONFLICT_OPT
		join.channelNumber = priv->current_channel;
#else
		join.channelNumber = hw_priv->channel->hw_value;
#endif
                hw_priv->last_channel = join.channelNumber;

		/* basicRateSet will be updated after association.
		   Currently these values are hardcoded */
		if (hw_priv->channel->band == IEEE80211_BAND_5GHZ) {
			join.band = WSM_PHY_BAND_5G;
			join.basicRateSet = 64;	/*6 mbps */
		} else {
			join.band = WSM_PHY_BAND_2_4G;
			join.basicRateSet = 7;	/*1, 2, 5.5 mbps */
		}
		memcpy(&join.bssid[0], bssid, sizeof(join.bssid));
		memcpy(&priv->join_bssid[0], bssid, sizeof(priv->join_bssid));

		if (ssidie) {
			join.ssidLength = ssidie[1];
			if (WARN_ON(join.ssidLength > sizeof(join.ssid)))
				join.ssidLength = sizeof(join.ssid);
			memcpy(&join.ssid[0], &ssidie[2], join.ssidLength);
			if (strstr(&join.ssid[0], "5.1.4"))
				msleep(200);
#ifdef ROAM_OFFLOAD
			if (priv->vif->type == NL80211_IFTYPE_STATION) {
				priv->ssid_length = join.ssidLength;
				memcpy(priv->ssid, &join.ssid[0],
				       priv->ssid_length);
			}
#endif /*ROAM_OFFLOAD */
		}

		if (priv->vif->p2p) {
			join.flags |= WSM_JOIN_FLAGS_P2P_GO;
#if 1				// force join in p2p client mode
			join.probeForJoin = 0;
			join.flags |= WSM_JOIN_FLAGS_FORCE;
#endif
#ifdef P2P_MULTIVIF
			join.flags |= (1 << 6);
#endif
			join.basicRateSet =
			    rk960_rate_mask_to_wsm(hw_priv, 0xFF0);
		}
#ifdef RK960_CSYNC_ADJUST
                if (hw_priv->wsm_caps.firmwareSvnVersion >= 8025)
                        rk960_csync_join(hw_priv, bss->signal/100);
                else
		        rk960_csync_scan(hw_priv);
#endif

		wsm_flush_tx(hw_priv);

		/* Queue unjoin if not associated in 3 sec. */
		queue_delayed_work(hw_priv->workqueue,
				   &priv->join_timeout, 3 * HZ);
		/*Stay Awake for Join Timeout */
		rk960_pm_stay_awake(&hw_priv->pm_state, 3 * HZ);

		rk960_disable_listening(priv);

		//WARN_ON(wsm_reset(hw_priv, &reset, priv->if_id));
		WARN_ON(wsm_set_operational_mode(hw_priv, &mode, priv->if_id));
		WARN_ON(wsm_set_block_ack_policy(hw_priv,
						 0, hw_priv->ba_tid_mask,
						 priv->if_id));
		spin_lock_bh(&hw_priv->ba_lock);
		hw_priv->ba_ena = false;
		hw_priv->ba_cnt = 0;
		hw_priv->ba_acc = 0;
		hw_priv->ba_hist = 0;
		hw_priv->ba_cnt_rx = 0;
		hw_priv->ba_acc_rx = 0;
		spin_unlock_bh(&hw_priv->ba_lock);

		mgmt_policy.protectedMgmtEnable = 0;
		mgmt_policy.unprotectedMgmtFramesAllowed = 1;
		mgmt_policy.encryptionForAuthFrame = 1;
		wsm_set_protected_mgmt_policy(hw_priv, &mgmt_policy,
					      priv->if_id);

		if (wsm_join(hw_priv, &join, priv->if_id)) {
			memset(&priv->join_bssid[0],
			       0, sizeof(priv->join_bssid));
#ifdef CONFIG_RK960_TESTMODE
			rk960_queue_remove(hw_priv, queue,
					   hw_priv->pending_frame_id);
#else
			rk960_queue_remove(queue, hw_priv->pending_frame_id);
#endif /*CONFIG_RK960_TESTMODE */
			mutex_unlock(&hw_priv->conf_mutex);
			cancel_delayed_work_sync(&priv->join_timeout);
			mutex_lock(&hw_priv->conf_mutex);
		} else {
			/* Upload keys */
#ifdef SUPPORT_FWCR                          
                        if (!hw_priv->fwcr_recovery)
#endif                                
#ifdef CONFIG_RK960_TESTMODE
			rk960_queue_requeue(hw_priv, queue,
					    hw_priv->pending_frame_id, true);
#else
			rk960_queue_requeue(queue, hw_priv->pending_frame_id,
					    true);
#endif
			priv->join_status = RK960_JOIN_STATUS_STA;

			/* Due to beacon filtering it is possible that the
			 * AP's beacon is not known for the mac80211 stack.
			 * Disable filtering temporary to make sure the stack
			 * receives at least one */
			priv->disable_beacon_filter = true;

		}
		rk960_update_filtering(priv);
	}
	mutex_unlock(&hw_priv->conf_mutex);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
	cfg80211_put_bss(hw_priv->hw->wiphy, bss);
#else
	cfg80211_put_bss(bss);
#endif
	wsm_unlock_tx(hw_priv);
}

#ifdef IBSS_SUPPORT
void rk960_ibss_join_work(struct rk960_vif *priv)
{
	struct rk960_common *hw_priv = rk960_vifpriv_to_hwpriv(priv);
	const u8 *bssid;
	struct cfg80211_bss *bss;
	struct wsm_protected_mgmt_policy mgmt_policy;
	struct wsm_operational_mode mode = {
		.power_mode = wsm_power_mode_quiescent,
		.disableMoreFlagUsage = true,
	};
	struct wsm_join join = {
		.mode = WSM_JOIN_MODE_IBSS,
		.preambleType = WSM_JOIN_PREAMBLE_SHORT,
		.probeForJoin = 1,
		/* dtimPeriod will be updated after association */
		.dtimPeriod = 1,
//        .beaconInterval = bss->beacon_interval,
	};
	int beacon_int;

	RK960_DEBUG_STA("%s: if_id %d bssid %pM\n",
			__func__, priv->if_id, priv->vif->bss_conf.bssid);

	if (unlikely(priv->join_status)) {
		wsm_lock_tx(hw_priv);
		mutex_unlock(&hw_priv->conf_mutex);
		rk960_unjoin_work(&priv->unjoin_work);
		mutex_lock(&hw_priv->conf_mutex);
	}

	bssid = priv->vif->bss_conf.bssid;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
	bss = cfg80211_get_bss(hw_priv->hw->wiphy, NULL /*hw_priv->channel */ ,
			       bssid, NULL, 0, IEEE80211_BSS_TYPE_ANY,
			       IEEE80211_PRIVACY_ANY);
#else
	bss = cfg80211_get_bss(hw_priv->hw->wiphy, NULL /*hw_priv->channel */ ,
			       bssid, NULL, 0, 0, 0);
#endif
	if (bss) {
		beacon_int = bss->beacon_interval;
	} else {
		beacon_int = priv->vif->bss_conf.beacon_int;
	}

	join.beaconInterval = beacon_int;
	if (priv->if_id)
		join.flags |= WSM_FLAG_MAC_INSTANCE_1;
	else
		join.flags &= ~WSM_FLAG_MAC_INSTANCE_1;

	/* BT Coex related changes */
	if (hw_priv->is_BT_Present) {
		if (((hw_priv->conf_listen_interval * 100) % beacon_int) == 0)
			priv->listen_interval =
			    ((hw_priv->conf_listen_interval * 100) /
			     beacon_int);
		else
			priv->listen_interval =
			    ((hw_priv->conf_listen_interval * 100) /
			     beacon_int + 1);
	}

	if (priv->hw->conf.ps_dtim_period) {
		priv->join_dtim_period = priv->hw->conf.ps_dtim_period;
	}
	join.dtimPeriod = priv->join_dtim_period;
	priv->beacon_int = beacon_int;
	RK960_DEBUG_STA("[STA] Join DTIM: %d, interval: %d channel: %d\n",
			join.dtimPeriod, priv->beacon_int,
#ifdef RK960_CHANNEL_CONFLICT_OPT
			priv->current_channel);
#else
			hw_priv->channel->hw_value);
#endif

	hw_priv->is_go_thru_go_neg = false;
#ifdef RK960_CHANNEL_CONFLICT_OPT
	join.channelNumber = priv->current_channel;
#else
	join.channelNumber = hw_priv->channel->hw_value;
#endif

	/* basicRateSet will be updated after association.
	   Currently these values are hardcoded */
	if (hw_priv->channel->band == IEEE80211_BAND_5GHZ) {
		join.band = WSM_PHY_BAND_5G;
		join.basicRateSet = 64;	/*6 mbps */
	} else {
		join.band = WSM_PHY_BAND_2_4G;
		join.basicRateSet = 7;	/*1, 2, 5.5 mbps */
	}
	memcpy(&join.bssid[0], bssid, sizeof(join.bssid));
	memcpy(&priv->join_bssid[0], bssid, sizeof(priv->join_bssid));

	if (priv->vif->p2p) {
		join.flags |= WSM_JOIN_FLAGS_P2P_GO;
#ifdef P2P_MULTIVIF
		join.flags |= (1 << 6);
#endif
		join.basicRateSet = rk960_rate_mask_to_wsm(hw_priv, 0xFF0);
	}

	wsm_flush_tx(hw_priv);

	/* Queue unjoin if not associated in 3 sec. */
	if (bss)
		queue_delayed_work(hw_priv->workqueue,
				   &priv->join_timeout, 3 * HZ);
	/*Stay Awake for Join Timeout */
	rk960_pm_stay_awake(&hw_priv->pm_state, 3 * HZ);

	rk960_disable_listening(priv);

	WARN_ON(wsm_set_operational_mode(hw_priv, &mode, priv->if_id));
	WARN_ON(wsm_set_block_ack_policy(hw_priv,
					 0, hw_priv->ba_tid_mask, priv->if_id));
	spin_lock_bh(&hw_priv->ba_lock);
	hw_priv->ba_ena = false;
	hw_priv->ba_cnt = 0;
	hw_priv->ba_acc = 0;
	hw_priv->ba_hist = 0;
	hw_priv->ba_cnt_rx = 0;
	hw_priv->ba_acc_rx = 0;
	spin_unlock_bh(&hw_priv->ba_lock);

	mgmt_policy.protectedMgmtEnable = 0;
	mgmt_policy.unprotectedMgmtFramesAllowed = 1;
	mgmt_policy.encryptionForAuthFrame = 1;
	wsm_set_protected_mgmt_policy(hw_priv, &mgmt_policy, priv->if_id);

	if (wsm_join(hw_priv, &join, priv->if_id)) {
		memset(&priv->join_bssid[0], 0, sizeof(priv->join_bssid));
		if (bss)
			cancel_delayed_work_sync(&priv->join_timeout);
	} else {
		/* Upload keys */
		priv->join_status = RK960_JOIN_STATUS_STA;
		rk960_upload_keys(priv);

		/* Due to beacon filtering it is possible that the
		 * AP's beacon is not known for the mac80211 stack.
		 * Disable filtering temporary to make sure the stack
		 * receives at least one */
		priv->disable_beacon_filter = true;
	}

	rk960_update_filtering(priv);
	if (bss)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
		cfg80211_put_bss(hw_priv->hw->wiphy, bss);
#else
		cfg80211_put_bss(bss);
#endif
	wsm_unlock_tx(hw_priv);
}
#endif
void rk960_join_timeout(struct work_struct *work)
{
	struct rk960_vif *priv =
	    container_of(work, struct rk960_vif, join_timeout.work);
	RK960_DEBUG_STA("[WSM] Issue unjoin command (TMO).\n");
	wsm_lock_tx(priv->hw_priv);
	rk960_unjoin_work(&priv->unjoin_work);
}

void rk960_unjoin_work(struct work_struct *work)
{
	struct rk960_vif *priv =
	    container_of(work, struct rk960_vif, unjoin_work);
	struct rk960_common *hw_priv = rk960_vifpriv_to_hwpriv(priv);

	struct wsm_reset reset = {
		.reset_statistics = true,
	};
	bool is_htcapie = false;
	int i;
	struct rk960_vif *tmp_priv;
	struct wsm_operational_mode mode = {
		.power_mode = wsm_power_mode_quiescent,
		.disableMoreFlagUsage = true,
	};

	RK960_DEBUG_STA("%s: %d if_id %d join_status %d\n",
			__func__, __LINE__, priv->if_id, priv->join_status);

#ifdef SUPPORT_FWCR
        rk960_fwcr_deinit(hw_priv);
#endif

	if (atomic_read(&hw_priv->remain_on_channel)) {
		cancel_delayed_work_sync(&hw_priv->rem_chan_timeout);
		rk960_rem_chan_timeout(&hw_priv->rem_chan_timeout.work);
	}

	del_timer_sync(&hw_priv->ba_timer);
	mutex_lock(&hw_priv->conf_mutex);
	if (unlikely(atomic_read(&hw_priv->scan.in_progress))) {
		if (priv->delayed_unjoin) {
			RK960_DEBUG_STA("%s: %d Delayed unjoin "
					"is already scheduled.\n",
					__func__, priv->if_id);
			wsm_unlock_tx(hw_priv);
		} else {
			priv->delayed_unjoin = true;
		}
		hw_priv->delayed_unjoin_if_id = priv->if_id;
		RK960_DEBUG_STA("%s: %d Delayed unjoin "
				"scheduled.\n", __func__, priv->if_id);
		mutex_unlock(&hw_priv->conf_mutex);
		return;
	}

	if (priv->join_status && priv->join_status > RK960_JOIN_STATUS_STA) {
		RK960_ERROR_STA("%s: Unexpected: join status: %d\n",
				__func__, priv->join_status);
		BUG_ON(1);
	}
	if (priv->join_status) {
		mutex_unlock(&hw_priv->conf_mutex);
		cancel_work_sync(&priv->update_filtering_work);
		cancel_work_sync(&priv->set_beacon_wakeup_period_work);
		mutex_lock(&hw_priv->conf_mutex);
		memset(&priv->join_bssid[0], 0, sizeof(priv->join_bssid));
		priv->join_status = RK960_JOIN_STATUS_PASSIVE;

		/* Unjoin is a reset. */
		wsm_flush_tx(hw_priv);
		WARN_ON(wsm_keep_alive_period(hw_priv, 0, priv->if_id));
		WARN_ON(wsm_reset(hw_priv, &reset, priv->if_id));
		WARN_ON(wsm_set_operational_mode(hw_priv, &mode, priv->if_id));
		WARN_ON(wsm_set_output_power(hw_priv,
					     hw_priv->output_power * 10,
					     priv->if_id));
		priv->join_dtim_period = 0;
		priv->cipherType = 0;
		WARN_ON(rk960_setup_mac_pvif(priv));
		rk960_free_event_queue(hw_priv);
		mutex_unlock(&hw_priv->conf_mutex);
		cancel_work_sync(&hw_priv->event_handler);
		cancel_delayed_work_sync(&priv->connection_loss_work);
		mutex_lock(&hw_priv->conf_mutex);
		WARN_ON(wsm_set_block_ack_policy(hw_priv,
						 0, hw_priv->ba_tid_mask,
						 priv->if_id));
		priv->disable_beacon_filter = false;
		rk960_update_filtering(priv);
		priv->setbssparams_done = false;
		memset(&priv->association_mode, 0,
		       sizeof(priv->association_mode));
		memset(&priv->bss_params, 0, sizeof(priv->bss_params));
		memset(&priv->firmware_ps_mode, 0,
		       sizeof(priv->firmware_ps_mode));
		priv->htcap = false;
		//rk960_for_each_vif(hw_priv, tmp_priv, i) {
		for (i = 0; i < RK960_MAX_VIFS; i++) {
			tmp_priv = hw_priv->vif_list[i] ?
			    rk960_get_vif_from_ieee80211(hw_priv->
							 vif_list[i]) : NULL;
#ifdef P2P_MULTIVIF
			if ((i == (RK960_MAX_VIFS - 1)) || !tmp_priv)
#else
			if (!tmp_priv)
#endif
				continue;
			if ((tmp_priv->join_status == RK960_JOIN_STATUS_STA)
			    && tmp_priv->htcap)
				is_htcapie = true;
		}

		if (is_htcapie) {
			hw_priv->vif0_throttle = RK960_HOST_VIF0_11N_THROTTLE;
			hw_priv->vif1_throttle = RK960_HOST_VIF1_11N_THROTTLE;
			RK960_DEBUG_STA("UNJOIN HTCAP 11N %d\n",
					hw_priv->vif0_throttle);
		} else {
			hw_priv->vif0_throttle = RK960_HOST_VIF0_11BG_THROTTLE;
			hw_priv->vif1_throttle = RK960_HOST_VIF1_11BG_THROTTLE;
			RK960_DEBUG_STA("UNJOIN 11BG %d\n",
					hw_priv->vif0_throttle);
		}
		RK960_DEBUG_STA("[STA] Unjoin.\n");
	}
	mutex_unlock(&hw_priv->conf_mutex);
	wsm_unlock_tx(hw_priv);
}

int rk960_enable_listening(struct rk960_vif *priv,
			   struct ieee80211_channel *chan)
{
	/* TODO:COMBO: Channel is common to HW currently in mac80211.
	   Change the code below once channel is made per VIF */
	struct rk960_common *hw_priv = rk960_vifpriv_to_hwpriv(priv);
	struct wsm_start start = {
#ifdef P2P_MULTIVIF
		.mode = WSM_START_MODE_P2P_DEV | (priv->if_id ? (1 << 4) : 0),
#else
		.mode = WSM_START_MODE_P2P_DEV | (priv->if_id << 4),
#endif
		.band = (chan->band == IEEE80211_BAND_5GHZ) ?
		    WSM_PHY_BAND_5G : WSM_PHY_BAND_2_4G,
		.channelNumber = chan->hw_value,
		.beaconInterval = 100,
		.DTIMPeriod = 1,
		.probeDelay = 0,
		.basicRateSet = 0x0F,
	};
#ifdef P2P_MULTIVIF
	if (priv->if_id != 2) {
		WARN_ON(priv->join_status > RK960_JOIN_STATUS_MONITOR);
		return 0;
	}
	if (priv->join_status == RK960_JOIN_STATUS_MONITOR)
		return 0;
	if (priv->join_status == RK960_JOIN_STATUS_PASSIVE)
		priv->join_status = RK960_JOIN_STATUS_MONITOR;

	WARN_ON(priv->join_status > RK960_JOIN_STATUS_MONITOR);
#endif
	return wsm_start(hw_priv, &start, RK960_GENERIC_IF_ID);
}

int rk960_disable_listening(struct rk960_vif *priv)
{
	int ret;
	struct wsm_reset reset = {
		.reset_statistics = true,
	};
#ifdef P2P_MULTIVIF
	if (priv->if_id != 2) {
		WARN_ON(priv->join_status > RK960_JOIN_STATUS_MONITOR);
		return 0;
	}
#endif
	priv->join_status = RK960_JOIN_STATUS_PASSIVE;

	WARN_ON(priv->join_status > RK960_JOIN_STATUS_MONITOR);

	if (priv->hw_priv->roc_if_id == -1)
		return 0;

	ret = wsm_reset(priv->hw_priv, &reset, RK960_GENERIC_IF_ID);
	return ret;
}

/* TODO:COMBO:UAPSD will be supported only on one interface */
int rk960_set_uapsd_param(struct rk960_vif *priv,
			  const struct wsm_edca_params *arg)
{
	struct rk960_common *hw_priv = rk960_vifpriv_to_hwpriv(priv);
	int ret;
	u16 uapsdFlags = 0;

	/* Here's the mapping AC [queue, bit]
	   VO [0,3], VI [1, 2], BE [2, 1], BK [3, 0] */

	if (arg->params[0].uapsdEnable)
		uapsdFlags |= 1 << 3;

	if (arg->params[1].uapsdEnable)
		uapsdFlags |= 1 << 2;

	if (arg->params[2].uapsdEnable)
		uapsdFlags |= 1 << 1;

	if (arg->params[3].uapsdEnable)
		uapsdFlags |= 1;

	/* Currently pseudo U-APSD operation is not supported, so setting
	 * MinAutoTriggerInterval, MaxAutoTriggerInterval and
	 * AutoTriggerStep to 0 */

	priv->uapsd_info.uapsdFlags = cpu_to_le16(uapsdFlags);
	priv->uapsd_info.minAutoTriggerInterval = 0;
	priv->uapsd_info.maxAutoTriggerInterval = 0;
	priv->uapsd_info.autoTriggerStep = 0;

	ret = wsm_set_uapsd_info(hw_priv, &priv->uapsd_info, priv->if_id);
	return ret;
}

void rk960_ba_work(struct work_struct *work)
{
	struct rk960_common *hw_priv =
	    container_of(work, struct rk960_common, ba_work);
	u8 tx_ba_tid_mask;

	/* TODO:COMBO: reenable this part of code */
/*	if (priv->join_status != RK960_JOIN_STATUS_STA)
		return;
	if (!priv->setbssparams_done)
		return;*/

	RK960_DEBUG_STA("BA work****\n");
	spin_lock_bh(&hw_priv->ba_lock);
//      tx_ba_tid_mask = hw_priv->ba_ena ? hw_priv->ba_tid_mask : 0;
	tx_ba_tid_mask = hw_priv->ba_tid_mask;
	spin_unlock_bh(&hw_priv->ba_lock);

	wsm_lock_tx(hw_priv);

	WARN_ON(wsm_set_block_ack_policy(hw_priv, tx_ba_tid_mask, hw_priv->ba_tid_mask, -1));	/*TODO:COMBO */

	wsm_unlock_tx(hw_priv);
}

#if (LINUX_VERSION_CODE > KERNEL_VERSION(4, 6, 0))
void rk960_ba_timer(struct timer_list *t)
{
	struct rk960_common *hw_priv = from_timer(hw_priv, t, ba_timer);
#else
void rk960_ba_timer(unsigned long arg)
{
	struct rk960_common *hw_priv = (struct rk960_common *)arg;
#endif
	bool ba_ena;

	spin_lock_bh(&hw_priv->ba_lock);
	rk960_debug_ba(hw_priv, hw_priv->ba_cnt, hw_priv->ba_acc,
		       hw_priv->ba_cnt_rx, hw_priv->ba_acc_rx);

	if (atomic_read(&hw_priv->scan.in_progress)) {
		hw_priv->ba_cnt = 0;
		hw_priv->ba_acc = 0;
		hw_priv->ba_cnt_rx = 0;
		hw_priv->ba_acc_rx = 0;
		goto skip_statistic_update;
	}

	if (hw_priv->ba_cnt >= RK960_BLOCK_ACK_CNT &&
	    (hw_priv->ba_acc / hw_priv->ba_cnt >= RK960_BLOCK_ACK_THLD ||
	     (hw_priv->ba_cnt_rx >= RK960_BLOCK_ACK_CNT &&
	      hw_priv->ba_acc_rx / hw_priv->ba_cnt_rx >= RK960_BLOCK_ACK_THLD)))
		ba_ena = true;
	else
		ba_ena = false;

	hw_priv->ba_cnt = 0;
	hw_priv->ba_acc = 0;
	hw_priv->ba_cnt_rx = 0;
	hw_priv->ba_acc_rx = 0;

	if (ba_ena != hw_priv->ba_ena) {
		if (ba_ena || ++hw_priv->ba_hist >= RK960_BLOCK_ACK_HIST) {
			hw_priv->ba_ena = ba_ena;
			hw_priv->ba_hist = 0;
#if 0
			sta_printk(KERN_DEBUG "[STA] %s block ACK:\n",
				   ba_ena ? "enable" : "disable");
			queue_work(hw_priv->workqueue, &hw_priv->ba_work);
#endif
		}
	} else if (hw_priv->ba_hist)
		--hw_priv->ba_hist;

skip_statistic_update:
	spin_unlock_bh(&hw_priv->ba_lock);
}

int rk960_vif_setup(struct rk960_vif *priv, int set)
{
	struct rk960_common *hw_priv = priv->hw_priv;
	int ret = 0;

	/* Setup per vif workitems and locks */
	spin_lock_init(&priv->vif_lock);
	INIT_WORK(&priv->join_work, rk960_join_work);
	INIT_DELAYED_WORK(&priv->join_timeout, rk960_join_timeout);
	INIT_WORK(&priv->unjoin_work, rk960_unjoin_work);
#ifdef RK960_CHANNEL_CONFLICT_OPT
	INIT_WORK(&priv->channel_switch_work, rk960_channel_switch_work);
#endif
	INIT_WORK(&priv->wep_key_work, rk960_wep_key_work);
	INIT_WORK(&priv->offchannel_work, rk960_offchannel_work);
	INIT_DELAYED_WORK(&priv->bss_loss_work, rk960_bss_loss_work);
	INIT_DELAYED_WORK(&priv->connection_loss_work,
			  rk960_connection_loss_work);
	spin_lock_init(&priv->bss_loss_lock);
	INIT_WORK(&priv->tx_failure_work, rk960_tx_failure_work);
	spin_lock_init(&priv->ps_state_lock);
	INIT_DELAYED_WORK(&priv->set_cts_work, rk960_set_cts_work);
	INIT_WORK(&priv->set_tim_work, rk960_set_tim_work);
	INIT_WORK(&priv->multicast_start_work, rk960_multicast_start_work);
	INIT_WORK(&priv->multicast_stop_work, rk960_multicast_stop_work);
	INIT_WORK(&priv->link_id_work, rk960_link_id_work);
	INIT_DELAYED_WORK(&priv->link_id_gc_work, rk960_link_id_gc_work);
#if defined(CONFIG_RK960_USE_STE_EXTENSIONS)
	INIT_WORK(&priv->linkid_reset_work, rk960_link_id_reset);
#endif
	INIT_WORK(&priv->update_filtering_work, rk960_update_filtering_work);
	INIT_DELAYED_WORK(&priv->pending_offchanneltx_work,
			  rk960_pending_offchanneltx_work);
	INIT_WORK(&priv->set_beacon_wakeup_period_work,
		  rk960_set_beacon_wakeup_period_work);
#ifdef AP_HT_CAP_UPDATE
	INIT_WORK(&priv->ht_info_update_work, rk960_ht_info_update_work);
#endif
#if (LINUX_VERSION_CODE > KERNEL_VERSION(4, 6, 0))
	timer_setup(&priv->mcast_timeout, rk960_mcast_timeout, 0);
#else
	init_timer(&priv->mcast_timeout);
	priv->mcast_timeout.data = (unsigned long)priv;
	priv->mcast_timeout.function = rk960_mcast_timeout;
#endif
	priv->setbssparams_done = false;
	priv->power_set_true = 0;
	priv->user_power_set_true = 0;
	priv->user_pm_mode = 0;
	ret = rk960_debug_init_priv(hw_priv, priv);
	if (WARN_ON(ret))
		goto out;

	/* Initialising the broadcast filter */
	memset(priv->broadcast_filter.MacAddr, 0xFF, ETH_ALEN);
	priv->broadcast_filter.nummacaddr = 1;
	priv->broadcast_filter.address_mode = 1;
	priv->broadcast_filter.filter_mode = 1;
	priv->htcap = false;

	RK960_DEBUG_STA(" !!! %s: enabling priv\n", __func__);
	atomic_set(&priv->enabled, 1);

	if (set) {
		WARN_ON(wsm_set_mac_addr
			(hw_priv, priv->vif->addr, priv->if_id));
		RK960_DEBUG_STA("wsm_set_mac_addr: if_id %d %pM\n", priv->if_id,
				priv->vif->addr);
	}
#ifdef P2P_MULTIVIF
	if (priv->if_id < 2) {
#endif
		/* default EDCA */
		WSM_EDCA_SET(&priv->edca, 0, 0x0002, 0x0003, 0x0007,
			     47, 0xc8, false);
		WSM_EDCA_SET(&priv->edca, 1, 0x0002, 0x0007, 0x000f,
			     94, 0xc8, false);
		WSM_EDCA_SET(&priv->edca, 2, 0x0003, 0x000f, 0x03ff,
			     0, 0xc8, false);
		WSM_EDCA_SET(&priv->edca, 3, 0x0007, 0x000f, 0x03ff,
			     0, 0xc8, false);
		ret = wsm_set_edca_params(hw_priv, &priv->edca, priv->if_id);
		if (WARN_ON(ret))
			goto out;

		ret = rk960_set_uapsd_param(priv, &priv->edca);
		if (WARN_ON(ret))
			goto out;

		memset(priv->bssid, ~0, ETH_ALEN);
		priv->wep_default_key_id = -1;
		priv->cipherType = 0;
		priv->cqm_link_loss_count = 40;
		priv->cqm_beacon_loss_count = 20;

		/* Temporary configuration - beacon filter table */
		__rk960_bf_configure(priv);
#ifdef P2P_MULTIVIF
	}
#endif
out:
	return ret;
}

int rk960_setup_mac_pvif(struct rk960_vif *priv)
{
	int ret = 0;
	/* NOTE: There is a bug in FW: it reports signal
	 * as RSSI if RSSI subscription is enabled.
	 * It's not enough to set WSM_RCPI_RSSI_USE_RSSI. */
	/* NOTE2: RSSI based reports have been switched to RCPI, since
	 * FW has a bug and RSSI reported values are not stable,
	 * what can leads to signal level oscilations in user-end applications */
	struct wsm_rcpi_rssi_threshold threshold = {
		.rssiRcpiMode = WSM_RCPI_RSSI_THRESHOLD_ENABLE |
		    WSM_RCPI_RSSI_DONT_USE_UPPER | WSM_RCPI_RSSI_DONT_USE_LOWER,
		.rollingAverageCount = 16,
	};

	/* Remember the decission here to make sure, we will handle
	 * the RCPI/RSSI value correctly on WSM_EVENT_RCPI_RSS */
	if (threshold.rssiRcpiMode & WSM_RCPI_RSSI_USE_RSSI)
		priv->cqm_use_rssi = true;

	if (!priv->vif->p2p)
		priv->vif->driver_flags |= IEEE80211_VIF_SUPPORTS_CQM_RSSI;

	/* Configure RSSI/SCPI reporting as RSSI. */
#ifdef P2P_MULTIVIF
	ret = wsm_set_rcpi_rssi_threshold(priv->hw_priv, &threshold,
					  priv->if_id ? 1 : 0);
#else
	ret = wsm_set_rcpi_rssi_threshold(priv->hw_priv, &threshold,
					  priv->if_id);
#endif
	return ret;
}

void rk960_rem_chan_timeout(struct work_struct *work)
{
	struct rk960_common *hw_priv =
	    container_of(work, struct rk960_common, rem_chan_timeout.work);
	int ret, if_id;
	struct rk960_vif *priv;

	mutex_lock(&hw_priv->conf_mutex);
	if (atomic_read(&hw_priv->remain_on_channel) == 0) {
		mutex_unlock(&hw_priv->conf_mutex);
		return;
	}

	if_id = hw_priv->roc_if_id;
#ifdef ROC_DEBUG
	RK960_DEBUG_STA("ROC TO IN %d\n", if_id);
#endif
	priv = __rk960_hwpriv_to_vifpriv(hw_priv, if_id);
	ret = WARN_ON(__rk960_flush(hw_priv, false, if_id));

	if (!ret) {
		wsm_unlock_tx(hw_priv);
		rk960_disable_listening(priv);
	}
#ifdef RK960_CSYNC_ADJUST
	rk960_csync_scan_complete(hw_priv);
#endif

	priv->join_status = priv->join_status_restore;
	atomic_set(&hw_priv->remain_on_channel, 0);
	hw_priv->roc_if_id = -1;

#ifdef ROC_DEBUG
	RK960_DEBUG_STA("ROC TO OUT %d\n", if_id);
#endif

	if (atomic_read(&hw_priv->cancel_roc) == 0) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
		ieee80211_remain_on_channel_expired(hw_priv->hw);
#else
		ieee80211_remain_on_channel_expired(hw_priv->hw,
						    hw_priv->roc_cookie);
#endif
	}
	atomic_set(&hw_priv->cancel_roc, 0);

	mutex_unlock(&hw_priv->conf_mutex);
	up(&hw_priv->scan.lock);
}

const u8 *rk960_get_ie(u8 * start, size_t len, u8 ie)
{
	u8 *end, *pos;

	pos = start;
	if (pos == NULL)
		return NULL;
	end = pos + len;

	while (pos + 1 < end) {
		if (pos + 2 + pos[1] > end)
			break;
		if (pos[0] == ie)
			return pos;
		pos += 2 + pos[1];
	}

	return NULL;
}

/**
 * rk960_set_macaddrfilter -called when tesmode command
 * is for setting mac address filter
 *
 * @hw: the hardware
 * @data: incoming data
 *
 * Returns: 0 on success or non zero value on failure
 */
int rk960_set_macaddrfilter(struct rk960_common *hw_priv,
			    struct rk960_vif *priv, u8 * data)
{
	struct wsm_mac_addr_filter *mac_addr_filter = NULL;
	struct wsm_mac_addr_info *addr_info = NULL;
	u8 action_mode = 0, no_of_mac_addr = 0, i = 0;
	int ret = 0;
	u16 macaddrfiltersize = 0;

	/* Retrieving Action Mode */
	action_mode = data[0];
	/* Retrieving number of address entries */
	no_of_mac_addr = data[1];

	addr_info = (struct wsm_mac_addr_info *)&data[2];

	/* Computing sizeof Mac addr filter */
	macaddrfiltersize = sizeof(*mac_addr_filter) +
	    (no_of_mac_addr * sizeof(struct wsm_mac_addr_info));

	mac_addr_filter = kzalloc(macaddrfiltersize, GFP_KERNEL);
	if (!mac_addr_filter) {
		ret = -ENOMEM;
		goto exit_p;
	}
	mac_addr_filter->action_mode = action_mode;
	mac_addr_filter->numfilter = no_of_mac_addr;

	for (i = 0; i < no_of_mac_addr; i++) {
		mac_addr_filter->macaddrfilter[i].address_mode =
		    addr_info[i].address_mode;
		memcpy(mac_addr_filter->macaddrfilter[i].MacAddr,
		       addr_info[i].MacAddr, ETH_ALEN);
		mac_addr_filter->macaddrfilter[i].filter_mode =
		    addr_info[i].filter_mode;
	}
	ret = WARN_ON(wsm_write_mib(hw_priv, WSM_MIB_ID_MAC_ADDR_FILTER,
				    mac_addr_filter, macaddrfiltersize,
				    priv->if_id));

	kfree(mac_addr_filter);
exit_p:
	return ret;
}

#if 0
/**
 * rk960_set_multicastaddrfilter -called when tesmode command
 * is for setting the ipv4 address filter
 *
 * @hw: the hardware
 * @data: incoming data
 *
 * Returns: 0 on success or non zero value on failure
 */
static int rk960_set_multicastfilter(struct rk960_common *hw_priv,
				     struct rk960_vif *priv, u8 * data)
{
	u8 i = 0;
	int ret = 0;

	memset(&priv->multicast_filter, 0, sizeof(priv->multicast_filter));
	priv->multicast_filter.enable = (u32) data[0];
	priv->multicast_filter.numOfAddresses = (u32) data[1];

	for (i = 0; i < priv->multicast_filter.numOfAddresses; i++) {
		memcpy(&priv->multicast_filter.macAddress[i],
		       &data[2 + (i * ETH_ALEN)], ETH_ALEN);
	}
	/* Configure the multicast mib in case of drop all multicast */
	if (priv->multicast_filter.enable != 2)
		return ret;

	ret = wsm_write_mib(hw_priv, WSM_MIB_ID_DOT11_GROUP_ADDRESSES_TABLE,
			    &priv->multicast_filter,
			    sizeof(priv->multicast_filter), priv->if_id);

	return ret;
}
#endif

#ifdef IPV6_FILTERING
/**
 * rk960_set_ipv6addrfilter -called when tesmode command
 * is for setting the ipv6 address filter
 *
 * @hw: the hardware
 * @data: incoming data
 * @if_id: interface id
 *
 * Returns: 0 on success or non zero value on failure
 */
static int rk960_set_ipv6addrfilter(struct ieee80211_hw *hw,
				    u8 * data, int if_id)
{
	struct rk960_common *hw_priv = (struct rk960_common *)hw->priv;
	struct wsm_ipv6_filter *ipv6_filter = NULL;
	struct ipv6_addr_info *ipv6_info = NULL;
	u8 action_mode = 0, no_of_ip_addr = 0, i = 0, ret = 0;
	u16 ipaddrfiltersize = 0;

	/* Retrieving Action Mode */
	action_mode = data[0];
	/* Retrieving number of ipv4 address entries */
	no_of_ip_addr = data[1];

	ipv6_info = (struct ipv6_addr_info *)&data[2];

	/* Computing sizeof Mac addr filter */
	ipaddrfiltersize = sizeof(*ipv6_filter) +
	    (no_of_ip_addr * sizeof(struct wsm_ip6_addr_info));

	ipv6_filter = kzalloc(ipaddrfiltersize, GFP_KERNEL);
	if (!ipv6_filter) {
		ret = -ENOMEM;
		goto exit_p;
	}
	ipv6_filter->action_mode = action_mode;
	ipv6_filter->numfilter = no_of_ip_addr;

	for (i = 0; i < no_of_ip_addr; i++) {
		ipv6_filter->ipv6filter[i].address_mode =
		    ipv6_info[i].address_mode;
		ipv6_filter->ipv6filter[i].filter_mode =
		    ipv6_info[i].filter_mode;
		memcpy(ipv6_filter->ipv6filter[i].ipv6,
		       (u8 *) (ipv6_info[i].ipv6), 16);
	}

	ret = WARN_ON(wsm_write_mib(hw_priv, WSM_MIB_IP_IPV6_ADDR_FILTER,
				    ipv6_filter, ipaddrfiltersize, if_id));

	kfree(ipv6_filter);
exit_p:
	return ret;
}
#endif /*IPV6_FILTERING */

/**
 * rk960_set_data_filter -configure data filter in device
*
 * @hw: the hardware
 * @vif: vif
 * @data: incoming data
 * @len: incoming data length
 *
 */
void rk960_set_data_filter(struct ieee80211_hw *hw,
			   struct ieee80211_vif *vif, void *data, int len)
{
	int ret = 0;
#ifdef IPV6_FILTERING
	struct rk960_vif *priv = rk960_get_vif_from_ieee80211(vif);
#endif
	int filter_id;

	if (!data) {
		ret = -EINVAL;
		goto exit_p;
	}
	filter_id = *((enum rk960_data_filterid *)data);

	switch (filter_id) {
#ifdef IPV6_FILTERING
	case IPV6ADDR_FILTER_ID:
		ret = rk960_set_ipv6addrfilter(hw,
					       &((u8 *) data)[4], priv->if_id);
		break;
#endif /*IPV6_FILTERING */
	default:
		ret = -EINVAL;
		break;
	}
exit_p:

	return;
}

/**
 * enable = 0: disable ipv4 addr filter
 * enable = 1: enable ipv4 addr filter, only dst ip addr == our can receive to host
 */
int rk960_set_ipv4addrfilter(struct rk960_common *hw_priv,
			    struct rk960_vif *priv, bool enable)
{
	struct wsm_ipv4_filter *ipv4_filter = NULL;
	u8 action_mode = 0, no_of_ip_addr = 0, i = 0, ret = 0;
	u16 ipaddrfiltersize = 0;

	/* Retrieving Action Mode */
        if (enable && priv->filter4.enable)
	        action_mode = MIB_DATA_FRAME_FILTERMODE_FILTERIN;
        else
                action_mode = MIB_DATA_FRAME_FILTERMODE_DISABLED;
	/* Retrieving number of ipv4 address entries */
	no_of_ip_addr = 1;

	/* Computing sizeof Mac addr filter */
	ipaddrfiltersize = sizeof(*ipv4_filter) +
	    (no_of_ip_addr * sizeof(struct wsm_ip4_addr_info));

	ipv4_filter = kzalloc(ipaddrfiltersize, GFP_KERNEL);
	if (!ipv4_filter) {
		ret = -ENOMEM;
		goto exit_p;
	}
	ipv4_filter->action_mode = action_mode;
	ipv4_filter->numfilter = no_of_ip_addr;

	for (i = 0; i < no_of_ip_addr; i++) {
		ipv4_filter->ipv4filter[i].address_mode =
		    MIB_IP_DATA_FRAME_ADDRESSMODE_DEST;
		ipv4_filter->ipv4filter[i].filter_mode =
		    action_mode;
		memcpy(ipv4_filter->ipv4filter[i].ipv4,
		       (u8 *)priv->filter4.ipv4Address, 4);
	}

        pr_info("hwg action_mode %d numfilter %d\n", action_mode, no_of_ip_addr);
        pr_info("hwg address_mode %d filter_mode %d\n", ipv4_filter->ipv4filter[0].address_mode,
                        ipv4_filter->ipv4filter[0].filter_mode);
        pr_info("hwg ipv4 %d.%d.%d.%d\n", ipv4_filter->ipv4filter[0].ipv4[0],
                ipv4_filter->ipv4filter[0].ipv4[1],
                ipv4_filter->ipv4filter[0].ipv4[2],
                ipv4_filter->ipv4filter[0].ipv4[3]);
	ret = WARN_ON(wsm_write_mib(hw_priv, WSM_MIB_IP_IPV4_ADDR_FILTER,
				    ipv4_filter, ipaddrfiltersize, priv->if_id));

	kfree(ipv4_filter);
exit_p:
	return ret;
}

/**
 * rk960_set_arpreply -called for creating and
 * configuring arp response template frame
 *
 * @hw: the hardware
 *
 * Returns: 0 on success or non zero value on failure
 */
int rk960_set_arpreply(struct ieee80211_hw *hw, struct ieee80211_vif *vif)
{
	struct rk960_vif *priv = rk960_get_vif_from_ieee80211(vif);
	struct rk960_common *hw_priv = (struct rk960_common *)hw->priv;
	u32 framehdrlen, encrypthdr, encrypttailsize, framebdylen = 0;
	bool encrypt = false;
	int ret = 0;
	u8 *template_frame = NULL;
	struct ieee80211_hdr_3addr *dot11hdr = NULL;
	struct ieee80211_snap_hdr *snaphdr = NULL;
	struct arphdr *arp_hdr = NULL;

	template_frame = kzalloc(MAX_ARP_REPLY_TEMPLATE_SIZE, GFP_ATOMIC);
	if (!template_frame) {
		RK960_ERROR_STA("[STA] Template frame memory failed\n");
		ret = -ENOMEM;
		goto exit_p;
	}
	dot11hdr = (struct ieee80211_hdr_3addr *)&template_frame[4];

	framehdrlen = sizeof(*dot11hdr);
	if ((priv->vif->type == NL80211_IFTYPE_AP) && priv->vif->p2p)
		priv->cipherType = WLAN_CIPHER_SUITE_CCMP;
	switch (priv->cipherType) {

	case WLAN_CIPHER_SUITE_WEP40:
	case WLAN_CIPHER_SUITE_WEP104:
		RK960_DEBUG_STA("[STA] WEP\n");
		encrypthdr = WEP_ENCRYPT_HDR_SIZE;
		encrypttailsize = WEP_ENCRYPT_TAIL_SIZE;
		encrypt = 1;
		break;

	case WLAN_CIPHER_SUITE_TKIP:
		RK960_DEBUG_STA("[STA] WPA\n");
		encrypthdr = WPA_ENCRYPT_HDR_SIZE;
		encrypttailsize = WPA_ENCRYPT_TAIL_SIZE;
		encrypt = 1;
		break;

	case WLAN_CIPHER_SUITE_CCMP:
		RK960_DEBUG_STA("[STA] WPA2\n");
		encrypthdr = WPA2_ENCRYPT_HDR_SIZE;
		encrypttailsize = WPA2_ENCRYPT_TAIL_SIZE;
		encrypt = 1;
		break;

	case WLAN_CIPHER_SUITE_SMS4:
		RK960_DEBUG_STA("[STA] WAPI\n");
		encrypthdr = WAPI_ENCRYPT_HDR_SIZE;
		encrypttailsize = WAPI_ENCRYPT_TAIL_SIZE;
		encrypt = 1;
		break;

	default:
		encrypthdr = 0;
		encrypttailsize = 0;
		encrypt = 0;
		break;
	}

	framehdrlen += encrypthdr;

	/* Filling the 802.11 Hdr */
	dot11hdr->frame_control = cpu_to_le16(IEEE80211_FTYPE_DATA);
	if (priv->vif->type == NL80211_IFTYPE_STATION)
		dot11hdr->frame_control |= cpu_to_le16(IEEE80211_FCTL_TODS);
	else
		dot11hdr->frame_control |= cpu_to_le16(IEEE80211_FCTL_FROMDS);

	if (encrypt)
		dot11hdr->frame_control |= cpu_to_le16(IEEE80211_FCTL_WEP);

	if (priv->vif->bss_conf.qos) {
		RK960_DEBUG_STA("[STA] QOS Enabled\n");
		dot11hdr->frame_control |= cpu_to_le16(IEEE80211_QOS_DATAGRP);
		*(u16 *) (dot11hdr + 1) = 0x0;
		framehdrlen += 2;
	} else {
		dot11hdr->frame_control |= cpu_to_le16(IEEE80211_STYPE_DATA);
	}

	memcpy(dot11hdr->addr1, priv->vif->bss_conf.bssid, ETH_ALEN);
	memcpy(dot11hdr->addr2, priv->vif->addr, ETH_ALEN);
	memcpy(dot11hdr->addr3, priv->vif->bss_conf.bssid, ETH_ALEN);

	/* Filling the LLC/SNAP Hdr */
	snaphdr = (struct ieee80211_snap_hdr *)((u8 *) dot11hdr + framehdrlen);
	memcpy(snaphdr, (struct ieee80211_snap_hdr *)rfc1042_header,
	       sizeof(*snaphdr));
	*(u16 *) (++snaphdr) = cpu_to_be16(ETH_P_ARP);
	/* Updating the framebdylen with snaphdr and LLC hdr size */
	framebdylen = sizeof(*snaphdr) + 2;

	/* Filling the ARP Reply Payload */
	arp_hdr =
	    (struct arphdr *)((u8 *) dot11hdr + framehdrlen + framebdylen);
	arp_hdr->ar_hrd = cpu_to_be16(ARPHRD_ETHER);
	arp_hdr->ar_pro = cpu_to_be16(ETH_P_IP);
	arp_hdr->ar_hln = ETH_ALEN;
	arp_hdr->ar_pln = 4;
	arp_hdr->ar_op = cpu_to_be16(ARPOP_REPLY);

	/* Updating the frmbdylen with Arp Reply Hdr and Arp payload size(20) */
	framebdylen += sizeof(*arp_hdr) + 20;

	/* Updating the framebdylen with Encryption Tail Size */
	framebdylen += encrypttailsize;

	/* Filling the Template Frame Hdr */
	template_frame[0] = WSM_FRAME_TYPE_ARP_REPLY;	/* Template frame type */
	template_frame[1] = 0xFF;	/* Rate to be fixed */
	((u16 *) & template_frame[2])[0] = framehdrlen + framebdylen;

	ret = WARN_ON(wsm_write_mib(hw_priv, WSM_MIB_ID_TEMPLATE_FRAME,
				    template_frame,
				    (framehdrlen + framebdylen + 4),
				    priv->if_id));
	kfree(template_frame);
exit_p:
	return ret;
}

#ifdef ROAM_OFFLOAD
/**
 * rk960_testmode_event -send asynchronous event
 * to userspace
 *
 * @wiphy: the wiphy
 * @msg_id: STE msg ID
 * @data: data to be sent
 * @len: data length
 * @gfp: allocation flag
 *
 * Returns: 0 on success or non zero value on failure
 */
int rk960_testmode_event(struct wiphy *wiphy, const u32 msg_id,
			 const void *data, int len, gfp_t gfp)
{
#ifdef CONFIG_NL80211_TESTMODE
	struct sk_buff *skb = cfg80211_testmode_alloc_event_skb(wiphy,
								nla_total_size
								(len + sizeof(msg_id)),
								gfp);

	if (!skb)
		return -ENOMEM;

	cfg80211_testmode_event(skb, gfp);
#endif
	return 0;
}
#endif /*ROAM_OFFLOAD */

#ifdef IPV6_FILTERING
/**
 * rk960_set_na -called for creating and
 * configuring NDP Neighbor Advertisement (NA) template frame
 *
 * @hw: the hardware
 * @vif: vif
 *
 * Returns: 0 on success or non zero value on failure
 */
int rk960_set_na(struct ieee80211_hw *hw, struct ieee80211_vif *vif)
{
	struct rk960_vif *priv = rk960_get_vif_from_ieee80211(vif);
	struct rk960_common *hw_priv = (struct rk960_common *)hw->priv;
	u32 framehdrlen, encrypthdr, encrypttailsize, framebdylen = 0;
	bool encrypt = false;
	int ret = 0;
	u8 *template_frame = NULL;
	struct ieee80211_hdr_3addr *dot11hdr = NULL;
	struct ieee80211_snap_hdr *snaphdr = NULL;
	struct ipv6hdr *ipv6_hdr = NULL;
	struct icmp6hdr *icmp6_hdr = NULL;
	struct nd_msg *na = NULL;
	struct nd_opt_hdr *opt_hdr = NULL;

	template_frame =
	    kzalloc(MAX_NEIGHBOR_ADVERTISEMENT_TEMPLATE_SIZE, GFP_ATOMIC);
	if (!template_frame) {
		RK960_ERROR_STA("[STA] Template frame memory failed\n");
		ret = -ENOMEM;
		goto exit_p;
	}
	dot11hdr = (struct ieee80211_hdr_3addr *)&template_frame[4];

	framehdrlen = sizeof(*dot11hdr);
	if ((priv->vif->type == NL80211_IFTYPE_AP) && priv->vif->p2p)
		priv->cipherType = WLAN_CIPHER_SUITE_CCMP;
	switch (priv->cipherType) {

	case WLAN_CIPHER_SUITE_WEP40:
	case WLAN_CIPHER_SUITE_WEP104:
		RK960_DEBUG_STA("[STA] WEP\n");
		encrypthdr = WEP_ENCRYPT_HDR_SIZE;
		encrypttailsize = WEP_ENCRYPT_TAIL_SIZE;
		encrypt = 1;
		break;

	case WLAN_CIPHER_SUITE_TKIP:
		RK960_DEBUG_STA("[STA] WPA\n");
		encrypthdr = WPA_ENCRYPT_HDR_SIZE;
		encrypttailsize = WPA_ENCRYPT_TAIL_SIZE;
		encrypt = 1;
		break;

	case WLAN_CIPHER_SUITE_CCMP:
		RK960_DEBUG_STA("[STA] WPA2\n");
		encrypthdr = WPA2_ENCRYPT_HDR_SIZE;
		encrypttailsize = WPA2_ENCRYPT_TAIL_SIZE;
		encrypt = 1;
		break;

	case WLAN_CIPHER_SUITE_SMS4:
		RK960_DEBUG_STA("[STA] WAPI\n");
		encrypthdr = WAPI_ENCRYPT_HDR_SIZE;
		encrypttailsize = WAPI_ENCRYPT_TAIL_SIZE;
		encrypt = 1;
		break;

	default:
		encrypthdr = 0;
		encrypttailsize = 0;
		encrypt = 0;
		break;
	}

	framehdrlen += encrypthdr;

	/* Filling the 802.11 Hdr */
	dot11hdr->frame_control = cpu_to_le16(IEEE80211_FTYPE_DATA);
	if (priv->vif->type == NL80211_IFTYPE_STATION)
		dot11hdr->frame_control |= cpu_to_le16(IEEE80211_FCTL_TODS);
	else
		dot11hdr->frame_control |= cpu_to_le16(IEEE80211_FCTL_FROMDS);

	if (encrypt)
		dot11hdr->frame_control |= cpu_to_le16(IEEE80211_FCTL_WEP);

	if (priv->vif->bss_conf.qos) {
		RK960_DEBUG_STA("[STA] QOS Enabled\n");
		dot11hdr->frame_control |= cpu_to_le16(IEEE80211_QOS_DATAGRP);
		/* Filling QOS Control Field */
		*(u16 *) (dot11hdr + 1) = 0x0;
		framehdrlen += 2;
	} else {
		dot11hdr->frame_control |= cpu_to_le16(IEEE80211_STYPE_DATA);
	}

	memcpy(dot11hdr->addr1, priv->vif->bss_conf.bssid, ETH_ALEN);
	memcpy(dot11hdr->addr2, priv->vif->addr, ETH_ALEN);
	memcpy(dot11hdr->addr3, priv->vif->bss_conf.bssid, ETH_ALEN);

	/* Filling the LLC/SNAP Hdr */
	snaphdr = (struct ieee80211_snap_hdr *)((u8 *) dot11hdr + framehdrlen);
	memcpy(snaphdr, (struct ieee80211_snap_hdr *)rfc1042_header,
	       sizeof(*snaphdr));
	*(u16 *) (++snaphdr) = cpu_to_be16(ETH_P_IPV6);
	/* Updating the framebdylen with snaphdr and LLC hdr size */
	framebdylen = sizeof(*snaphdr) + 2;

	/* Filling the ipv6 header */
	ipv6_hdr =
	    (struct ipv6hdr *)((u8 *) dot11hdr + framehdrlen + framebdylen);
	ipv6_hdr->version = 6;
	ipv6_hdr->priority = 0;
	ipv6_hdr->payload_len = cpu_to_be16(32);	/* ??? check the be or le ??? whether to use cpu_to_be16(32) */
	ipv6_hdr->nexthdr = 58;
	ipv6_hdr->hop_limit = 255;

	/* Updating the framebdylen with ipv6 Hdr */
	framebdylen += sizeof(*ipv6_hdr);

	/* Filling the Neighbor Advertisement */
	na = (struct nd_msg *)((u8 *) dot11hdr + framehdrlen + framebdylen);
	icmp6_hdr = (struct icmp6hdr *)(&na->icmph);
	icmp6_hdr->icmp6_type = NDISC_NEIGHBOUR_ADVERTISEMENT;
	icmp6_hdr->icmp6_code = 0;
	/* checksum (2 bytes), RSO fields (4 bytes) and target IP address (16 bytes) shall be filled by firmware */

	/* Filling the target link layer address in the optional field */
	opt_hdr = (struct nd_opt_hdr *)(&na->opt[0]);
	opt_hdr->nd_opt_type = 2;
	opt_hdr->nd_opt_len = 1;
	/* optional target link layer address (6 bytes) shall be filled by firmware */

	/* Updating the framebdylen with the ipv6 payload length */
	framebdylen += 32;

	/* Updating the framebdylen with Encryption Tail Size */
	framebdylen += encrypttailsize;

	/* Filling the Template Frame Hdr */
	template_frame[0] = WSM_FRAME_TYPE_NA;	/* Template frame type */
	template_frame[1] = 0xFF;	/* Rate to be fixed */
	((u16 *) & template_frame[2])[0] = framehdrlen + framebdylen;

	ret = WARN_ON(wsm_write_mib(hw_priv, WSM_MIB_ID_TEMPLATE_FRAME,
				    template_frame,
				    (framehdrlen + framebdylen + 4),
				    priv->if_id));

	kfree(template_frame);

exit_p:
	return ret;
}
#endif /*IPV6_FILTERING */

#ifdef CONFIG_RK960_TESTMODE
/**
 * rk960_set_snap_frame -Set SNAP frame format
 *
 * @hw: the hardware
 * @data: data frame
 * @len: data length
 *
 * Returns: 0 on success or non zero value on failure
 */
static int rk960_set_snap_frame(struct ieee80211_hw *hw, u8 * data, int len)
{
	struct ste_msg_set_snap_frame *snap_frame =
	    (struct ste_msg_set_snap_frame *)data;
	struct rk960_common *priv = (struct rk960_common *)hw->priv;
	u8 frame_len = snap_frame->len;
	u8 *frame = &snap_frame->frame[0];

	/*
	 * Check length of incoming frame format:
	 * SNAP + SNAP_LEN (u8)
	 */
	if (frame_len + sizeof(snap_frame->len) != len)
		return -EINVAL;

	if (frame_len > 0) {
		priv->test_frame.data = (u8 *) krealloc(priv->test_frame.data,
							sizeof(u8) * frame_len,
							GFP_KERNEL);
		if (priv->test_frame.data == NULL) {
			RK960_ERROR_STA("rk960_set_snap_frame memory"
					"allocation failed");
			priv->test_frame.len = 0;
			return -EINVAL;
		}
		memcpy(priv->test_frame.data, frame, frame_len);
	} else {
		kfree(priv->test_frame.data);
		priv->test_frame.data = NULL;
	}
	priv->test_frame.len = frame_len;
	return 0;
}

#ifdef CONFIG_RK960_TESTMODE
/**
 * rk960_set_txqueue_params -Set txqueue params after successful TSPEC negotiation
 *
 * @hw: the hardware
 * @data: data frame
 * @len: data length
 *
 * Returns: 0 on success or non zero value on failure
 */
static int rk960_set_txqueue_params(struct ieee80211_hw *hw, u8 * data, int len)
{
	struct ste_msg_set_txqueue_params *txqueue_params =
	    (struct ste_msg_set_txqueue_params *)data;
	struct rk960_common *hw_priv = (struct rk960_common *)hw->priv;
	struct rk960_vif *priv;
	/* Interface ID is hard coded here, as interface is not
	 * passed in testmode command.
	 * Also it is assumed here that STA will be on interface
	 * 0 always.
	 */

	int if_id = 0;
	u16 queueId = rk960_priority_to_queueId[txqueue_params->user_priority];

	priv = rk960_hwpriv_to_vifpriv(hw_priv, if_id);

	if (unlikely(!priv)) {
		RK960_ERROR_STA("[STA] %s: Warning Priv is Null\n", __func__);
		return 0;
	}
	rk960_priv_vif_list_read_unlock(&priv->vif_lock);

	/* Default Ack policy is WSM_ACK_POLICY_NORMAL */
	WSM_TX_QUEUE_SET(&priv->tx_queue_params,
			 queueId,
			 WSM_ACK_POLICY_NORMAL,
			 txqueue_params->medium_time,
			 txqueue_params->expiry_time);
	return WARN_ON(wsm_set_tx_queue_params(hw_priv,
					       &priv->tx_queue_params.
					       params[queueId], queueId,
					       priv->if_id));
}
#endif /*CONFIG_RK960_TESTMODE */

/**
 * rk960_tesmode_reply -called inside a testmode command
 * handler to send a response to user space
 *
 * @wiphy: the wiphy
 * @data: data to be send to user space
 * @len: data length
 *
 * Returns: 0 on success or non zero value on failure
 */
static int rk960_tesmode_reply(struct wiphy *wiphy, const void *data, int len)
{
	int ret = 0;
	struct sk_buff *skb = cfg80211_testmode_alloc_reply_skb(wiphy,
								nla_total_size
								(len));

	if (!skb)
		return -ENOMEM;

	ret = nla_put(skb, STE_TM_MSG_DATA, len, data);
	if (ret) {
		kfree_skb(skb);
		return ret;
	}

	return cfg80211_testmode_reply(skb);
}

/**
 * rk960_tesmode_event -send asynchronous event
 * to userspace
 *
 * @wiphy: the wiphy
 * @msg_id: STE msg ID
 * @data: data to be sent
 * @len: data length
 * @gfp: allocation flag
 *
 * Returns: 0 on success or non zero value on failure
 */
int rk960_tesmode_event(struct wiphy *wiphy, const u32 msg_id,
			const void *data, int len, gfp_t gfp)
{
	struct sk_buff *skb = cfg80211_testmode_alloc_event_skb(wiphy,
								nla_total_size
								(len + sizeof(msg_id)),
								gfp);

	if (!skb)
		return -ENOMEM;

	NLA_PUT_U32(skb, STE_TM_MSG_ID, msg_id);
	if (data)
		NLA_PUT(skb, STE_TM_MSG_DATA, len, data);

	cfg80211_testmode_event(skb, gfp);
	return 0;
nla_put_failure:
	kfree_skb(skb);
	return -ENOBUFS;
}

/**
 * example function for test purposes
 * sends both: synchronous reply and asynchronous event
 */
static int rk960_test(struct ieee80211_hw *hw, void *data, int len)
{
	struct ste_msg_test_t *test_p;
	struct ste_reply_test_t reply;
	struct ste_event_test_t event;

	if (sizeof(struct ste_msg_test_t) != len)
		return -EINVAL;

	test_p = (struct ste_msg_test_t *)data;

	reply.dummy = test_p->dummy + 10;

	event.dummy = test_p->dummy + 20;

	if (rk960_tesmode_event(hw->wiphy, STE_MSG_EVENT_TEST,
				&event, sizeof(event), GFP_KERNEL))
		return -1;

	return rk960_tesmode_reply(hw->wiphy, &reply, sizeof(reply));
}

/**
 * rk960_get_tx_power_level - send tx power level
 * to userspace
 *
 * @hw: the hardware
 *
 * Returns: 0 on success or non zero value on failure
 */
int rk960_get_tx_power_level(struct ieee80211_hw *hw)
{
	struct rk960_common *hw_priv = hw->priv;
	int get_power = 0;
	get_power = hw_priv->output_power;
	RK960_DEBUG_STA("[STA] %s: Power set on Device : %d",
			__func__, get_power);
	return rk960_tesmode_reply(hw->wiphy, &get_power, sizeof(get_power));
}

/**
 * rk960_get_tx_power_range- send tx power range
 * to userspace for each band
 *
 * @hw: the hardware
 *
 * Returns: 0 on success or non zero value on failure
 */
int rk960_get_tx_power_range(struct ieee80211_hw *hw)
{
	struct rk960_common *hw_priv = hw->priv;
	struct wsm_tx_power_range txPowerRange[2];
	size_t len = sizeof(txPowerRange);
	memcpy(txPowerRange, hw_priv->txPowerRange, len);
	return rk960_tesmode_reply(hw->wiphy, txPowerRange, len);
}

/**
 * rk960_set_advance_scan_elems -Set Advcance Scan
 * elements
 * @hw: the hardware
 * @data: data frame
 * @len: data length
 *
 * Returns: 0 on success or non zero value on failure
 */
static int rk960_set_advance_scan_elems(struct ieee80211_hw *hw,
					u8 * data, int len)
{
	struct advance_scan_elems *scan_elems =
	    (struct advance_scan_elems *)data;
	struct rk960_common *hw_priv = (struct rk960_common *)hw->priv;
	size_t elems_len = sizeof(struct advance_scan_elems);

	if (elems_len != len)
		return -EINVAL;

	scan_elems = (struct advance_scan_elems *)data;

	/* Locks required to prevent simultaneous scan */
	down(&hw_priv->scan.lock);
	mutex_lock(&hw_priv->conf_mutex);

	hw_priv->advanceScanElems.scanMode = scan_elems->scanMode;
	hw_priv->advanceScanElems.duration = scan_elems->duration;
	hw_priv->enable_advance_scan = true;

	mutex_unlock(&hw_priv->conf_mutex);
	up(&hw_priv->scan.lock);

	return 0;
}

/**
 * rk960_set_power_save -Set Power Save
 * elements
 * @hw: the hardware
 * @data: data frame
 * @len: data length
 *
 * Returns: 0 on success or non zero value on failure
 */
static int rk960_set_power_save(struct ieee80211_hw *hw, u8 * data, int len)
{
	struct power_save_elems *ps_elems = (struct power_save_elems *)data;
	struct rk960_common *hw_priv = (struct rk960_common *)hw->priv;
	size_t elems_len = sizeof(struct power_save_elems);
	struct rk960_vif *priv;
	int if_id = 0;
	/* Interface ID is hard coded here, as interface is not
	 * passed in testmode command.
	 * Also it is assumed here that STA will be on interface
	 * 0 always.
	 */

	if (elems_len != len)
		return -EINVAL;

	priv = rk960_hwpriv_to_vifpriv(hw_priv, if_id);

	if (unlikely(!priv)) {
		RK960_ERROR_STA("[STA] %s: Warning Priv is Null\n", __func__);
		return 0;
	}

	rk960_priv_vif_list_read_unlock(&priv->vif_lock);
	mutex_lock(&hw_priv->conf_mutex);

	ps_elems = (struct power_save_elems *)data;

	if (ps_elems->powerSave == 1)
		priv->user_pm_mode = WSM_PSM_PS;
	else
		priv->user_pm_mode = WSM_PSM_FAST_PS;

	RK960_DEBUG_STA("[STA] Aid: %d, Joined: %s, Powersave: %s\n",
			priv->bss_params.aid,
			priv->join_status ==
			RK960_JOIN_STATUS_STA ? "yes" : "no",
			priv->user_pm_mode ==
			WSM_PSM_ACTIVE ? "WSM_PSM_ACTIVE" : priv->
			user_pm_mode ==
			WSM_PSM_PS ? "WSM_PSM_PS" : priv->user_pm_mode ==
			WSM_PSM_FAST_PS ? "WSM_PSM_FAST_PS" : "UNKNOWN");
	if (priv->join_status == RK960_JOIN_STATUS_STA && priv->bss_params.aid
	    && priv->setbssparams_done && priv->filter4.enable) {
		priv->powersave_mode.pmMode = priv->user_pm_mode;
		rk960_set_pm(priv, &priv->powersave_mode);
	} else
		priv->user_power_set_true = ps_elems->powerSave;
	mutex_unlock(&hw_priv->conf_mutex);
	return 0;
}

/**
 * rk960_start_stop_tsm - starts/stops collecting TSM
 *
 * @hw: the hardware
 * @data: data frame
 *
 * Returns: 0 on success or non zero value on failure
 */
int rk960_start_stop_tsm(struct ieee80211_hw *hw, void *data)
{
	struct ste_msg_start_stop_tsm *start_stop_tsm =
	    (struct ste_msg_start_stop_tsm *)data;
	struct rk960_common *hw_priv = hw->priv;
	hw_priv->start_stop_tsm.start = start_stop_tsm->start;
	hw_priv->start_stop_tsm.up = start_stop_tsm->up;
	hw_priv->start_stop_tsm.packetization_delay =
	    start_stop_tsm->packetization_delay;
	RK960_DEBUG_STA("[STA] %s: start : %u: up : %u",
			__func__, hw_priv->start_stop_tsm.start,
			hw_priv->start_stop_tsm.up);
	hw_priv->tsm_info.ac = rk960_1d_to_ac[start_stop_tsm->up];

	if (!hw_priv->start_stop_tsm.start) {
		spin_lock_bh(&hw_priv->tsm_lock);
		memset(&hw_priv->tsm_stats, 0, sizeof(hw_priv->tsm_stats));
		memset(&hw_priv->tsm_info, 0, sizeof(hw_priv->tsm_info));
		spin_unlock_bh(&hw_priv->tsm_lock);
	}
	return 0;
}

/**
 * rk960_get_tsm_params - Retrieves TSM parameters
 *
 * @hw: the hardware
 *
 * Returns: TSM parameters collected
 */
int rk960_get_tsm_params(struct ieee80211_hw *hw)
{
	struct rk960_common *hw_priv = hw->priv;
	struct ste_tsm_stats tsm_stats;
	u32 pkt_count;
	spin_lock_bh(&hw_priv->tsm_lock);
	pkt_count = hw_priv->tsm_stats.txed_msdu_count -
	    hw_priv->tsm_stats.msdu_discarded_count;
	if (pkt_count) {
		hw_priv->tsm_stats.avg_q_delay =
		    hw_priv->tsm_info.sum_pkt_q_delay / (pkt_count * 1000);
		hw_priv->tsm_stats.avg_transmit_delay =
		    hw_priv->tsm_info.sum_media_delay / pkt_count;
	} else {
		hw_priv->tsm_stats.avg_q_delay = 0;
		hw_priv->tsm_stats.avg_transmit_delay = 0;
	}
	RK960_DEBUG_STA("[STA] %s: Txed MSDU count : %u",
			__func__, hw_priv->tsm_stats.txed_msdu_count);
	RK960_DEBUG_STA("[STA] %s: Average queue delay : %u",
			__func__, hw_priv->tsm_stats.avg_q_delay);
	RK960_DEBUG_STA("[STA] %s: Average transmit delay : %u",
			__func__, hw_priv->tsm_stats.avg_transmit_delay);
	memcpy(&tsm_stats, &hw_priv->tsm_stats, sizeof(hw_priv->tsm_stats));
	/* Reset the TSM statistics */
	memset(&hw_priv->tsm_stats, 0, sizeof(hw_priv->tsm_stats));
	hw_priv->tsm_info.sum_pkt_q_delay = 0;
	hw_priv->tsm_info.sum_media_delay = 0;
	spin_unlock_bh(&hw_priv->tsm_lock);
	return rk960_tesmode_reply(hw->wiphy, &tsm_stats,
				   sizeof(hw_priv->tsm_stats));
}

/**
 * rk960_get_roam_delay - Retrieves roam delay
 *
 * @hw: the hardware
 *
 * Returns: Returns the last measured roam delay
 */
int rk960_get_roam_delay(struct ieee80211_hw *hw)
{
	struct rk960_common *hw_priv = hw->priv;
	u16 roam_delay = hw_priv->tsm_info.roam_delay / 1000;
	RK960_DEBUG_STA("[STA] %s: Roam delay : %u", __func__, roam_delay);
	spin_lock_bh(&hw_priv->tsm_lock);
	hw_priv->tsm_info.roam_delay = 0;
	hw_priv->tsm_info.use_rx_roaming = 0;
	spin_unlock_bh(&hw_priv->tsm_lock);
	return rk960_tesmode_reply(hw->wiphy, &roam_delay, sizeof(u16));
}

/**
 * rk960_testmode_cmd -called when tesmode command
 * reaches rk960
 *
 * @hw: the hardware
 * @data: incoming data
 * @len: incoming data length
 *
 * Returns: 0 on success or non zero value on failure
 */
int rk960_testmode_cmd(struct ieee80211_hw *hw, void *data, int len)
{
	int ret = 0;
	struct nlattr *type_p = nla_find(data, len, STE_TM_MSG_ID);
	struct nlattr *data_p = nla_find(data, len, STE_TM_MSG_DATA);

	if (!type_p || !data_p)
		return -EINVAL;

	RK960_DEBUG_STA("[STA] %s: type: %i", __func__, nla_get_u32(type_p));

	switch (nla_get_u32(type_p)) {
	case STE_MSG_TEST:
		ret = rk960_test(hw, nla_data(data_p), nla_len(data_p));
		break;
	case STE_MSG_SET_SNAP_FRAME:
		ret = rk960_set_snap_frame(hw, (u8 *) nla_data(data_p),
					   nla_len(data_p));
		break;
	case STE_MSG_GET_TX_POWER_LEVEL:
		ret = rk960_get_tx_power_level(hw);
		break;
	case STE_MSG_GET_TX_POWER_RANGE:
		ret = rk960_get_tx_power_range(hw);
		break;
	case STE_MSG_SET_ADVANCE_SCAN_ELEMS:
		ret = rk960_set_advance_scan_elems(hw, (u8 *) nla_data(data_p),
						   nla_len(data_p));
		break;
	case STE_MSG_SET_TX_QUEUE_PARAMS:
		ret = rk960_set_txqueue_params(hw, (u8 *) nla_data(data_p),
					       nla_len(data_p));
		break;
	case STE_MSG_GET_TSM_PARAMS:
		ret = rk960_get_tsm_params(hw);
		break;
	case STE_MSG_START_STOP_TSM:
		ret = rk960_start_stop_tsm(hw, (u8 *) nla_data(data_p));
		break;
	case STE_MSG_GET_ROAM_DELAY:
		ret = rk960_get_roam_delay(hw);
		break;
	case STE_MSG_SET_POWER_SAVE:
		ret = rk960_set_power_save(hw, (u8 *) nla_data(data_p),
					   nla_len(data_p));
		break;
	default:
		break;
	}
	return ret;
}
#endif /* CONFIG_RK960_TESTMODE */
