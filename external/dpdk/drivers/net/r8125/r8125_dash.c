/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2020 Realtek Technologies Co., Ltd
 */

#include <sys/queue.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <stdarg.h>

#include <rte_ether.h>
#include <ethdev_driver.h>

#include "r8125_base.h"
#include "r8125_dash.h"
#include "r8125_hw.h"

bool
rtl8125_is_allow_access_dash_ocp(struct rtl8125_hw *hw)
{
	bool allow_access = false;
	u16 mac_ocp_data;

	if (!HW_DASH_SUPPORT_DASH(hw))
		goto exit;

	allow_access = true;
	switch (hw->mcfg) {
	case CFG_METHOD_2:
	case CFG_METHOD_3:
		mac_ocp_data = rtl8125_mac_ocp_read(hw, 0xd460);
		if (mac_ocp_data == 0xffff || !(mac_ocp_data & BIT_0))
			allow_access = false;
		break;
	case CFG_METHOD_8:
	case CFG_METHOD_9:
		mac_ocp_data = rtl8125_mac_ocp_read(hw, 0xd4c0);
		if (mac_ocp_data == 0xffff || (mac_ocp_data & BIT_3))
			allow_access = false;
		break;
	default:
		goto exit;
	}
exit:
	return allow_access;
}

static u32
rtl8125_get_dash_fw_ver(struct rtl8125_hw *hw)
{
	u32 ver = 0xffffffff;

	if (FALSE == HW_DASH_SUPPORT_GET_FIRMWARE_VERSION(hw))
		goto exit;

	ver = rtl8125_ocp_read(hw, OCP_REG_FIRMWARE_MAJOR_VERSION, 4);

exit:
	return ver;
}

static int
_rtl8125_check_dash(struct rtl8125_hw *hw)
{
	if (!hw->AllowAccessDashOcp)
		return 0;

	if (HW_DASH_SUPPORT_TYPE_2(hw) || HW_DASH_SUPPORT_TYPE_4(hw)) {
		if (rtl8125_ocp_read(hw, 0x128, 1) & BIT_0)
			return 1;
	}

	return 0;
}

int
rtl8125_check_dash(struct rtl8125_hw *hw)
{
	if (_rtl8125_check_dash(hw)) {
		u32 ver = rtl8125_get_dash_fw_ver(hw);
		if (!(ver == 0 || ver == 0xffffffff))
			return 1;
	}

	return 0;
}

static void rtl8125_dash2_disable_tx(struct rtl8125_hw *hw)
{
	u16 wait_cnt = 0;
	u8 tmp_uchar;

	if (!HW_DASH_SUPPORT_CMAC(hw))
		return;

	if (!hw->DASH)
		return;

	/* Disable oob Tx */
	RTL_CMAC_W8(hw, CMAC_IBCR2, RTL_CMAC_R8(hw, CMAC_IBCR2) & ~BIT_0);

	/* Wait oob Tx disable */
	do {
		tmp_uchar = RTL_CMAC_R8(hw, CMAC_IBISR0);
		if (tmp_uchar & ISRIMR_DASH_TYPE2_TX_DISABLE_IDLE)
			break;

		udelay(50);
		wait_cnt++;
	} while (wait_cnt < 2000);

	/* Clear ISRIMR_DASH_TYPE2_TX_DISABLE_IDLE */
	RTL_CMAC_W8(hw, CMAC_IBISR0, RTL_CMAC_R8(hw, CMAC_IBISR0) |
	            ISRIMR_DASH_TYPE2_TX_DISABLE_IDLE);
}

static void rtl8125_dash2_disable_rx(struct rtl8125_hw *hw)
{
	if (!HW_DASH_SUPPORT_CMAC(hw))
		return;

	if (!hw->DASH)
		return;

	RTL_CMAC_W8(hw, CMAC_IBCR0, RTL_CMAC_R8(hw, CMAC_IBCR0) & ~BIT_0);
}

void rtl8125_dash2_disable_txrx(struct rtl8125_hw *hw)
{
	if (!HW_DASH_SUPPORT_CMAC(hw))
		return;

	rtl8125_dash2_disable_tx(hw);
	rtl8125_dash2_disable_rx(hw);
}

static void
rtl8125_notify_dash_oob_cmac(struct rtl8125_hw *hw, u32 cmd)
{
	u32 tmp_value;

	if (!HW_DASH_SUPPORT_CMAC(hw))
		return;

	rtl8125_ocp_write(hw, 0x180, 4, cmd);
	tmp_value = rtl8125_ocp_read(hw, 0x30, 4);
	tmp_value |= BIT_0;
	rtl8125_ocp_write(hw, 0x30, 4, tmp_value);
}

static void
rtl8125_notify_dash_oob_ipc2(struct rtl8125_hw *hw, u32 cmd)
{
	if (FALSE == HW_DASH_SUPPORT_TYPE_4(hw))
		return;

	rtl8125_ocp_write(hw, IB2SOC_DATA, 4, cmd);
	rtl8125_ocp_write(hw, IB2SOC_CMD, 4, 0x00);
	rtl8125_ocp_write(hw, IB2SOC_SET, 4, 0x01);
}

static void
rtl8125_notify_dash_oob(struct rtl8125_hw *hw, u32 cmd)
{
	switch (hw->HwSuppDashVer) {
	case 2:
	case 3:
		rtl8125_notify_dash_oob_cmac(hw, cmd);
		break;
	case 4:
		rtl8125_notify_dash_oob_ipc2(hw, cmd);
		break;
	default:
		break;
	}
}

static int
rtl8125_wait_dash_fw_ready(struct rtl8125_hw *hw)
{
	int rc = -1;
	int timeout;

	if (!hw->DASH)
		goto out;

	for (timeout = 0; timeout < 10; timeout++) {
		mdelay(10);
		if (rtl8125_ocp_read(hw, 0x124, 1) & BIT_0) {
			rc = 1;
			goto out;
		}
	}

	rc = 0;

out:
	return rc;
}

void
rtl8125_driver_start(struct rtl8125_hw *hw)
{
	if (!hw->AllowAccessDashOcp)
		return;

	rtl8125_notify_dash_oob(hw, OOB_CMD_DRIVER_START);

	rtl8125_wait_dash_fw_ready(hw);
}

void
rtl8125_driver_stop(struct rtl8125_hw *hw)
{
	if (!hw->AllowAccessDashOcp)
		return;

	if (HW_DASH_SUPPORT_CMAC(hw))
		rtl8125_dash2_disable_txrx(hw);

	rtl8125_notify_dash_oob(hw, OOB_CMD_DRIVER_STOP);

	rtl8125_wait_dash_fw_ready(hw);
}