/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 Rockchip Electronics Co., Ltd.
 *
 * Author: Chen Shunqing <csq@rock-chips.com>
 */

#ifndef HDMITX_H
#define HDMITX_H

#include "rk628.h"

#define HDMI_BASE			0x70000
#define HDMI_REG_STRIDE			4
#define HDMITX_REG(x)			((x * HDMI_REG_STRIDE) + HDMI_BASE)

#define DDC_SEGMENT_ADDR		0x30

enum PWR_MODE {
	NORMAL,
	LOWER_PWR,
};

#define HDMI_SCL_RATE			(100 * 1000)
#define DDC_BUS_FREQ_L			HDMITX_REG(0x4b)
#define DDC_BUS_FREQ_H			HDMITX_REG(0x4c)

#define HDMI_SYS_CTRL			HDMITX_REG(0x00)
#define RST_ANALOG_MASK			BIT(6)
#define NOT_RST_ANALOG(x)		UPDATE(x, 6, 6)
#define RST_DIGITAL_MASK		BIT(5)
#define NOT_RST_DIGITAL(x)		UPDATE(x, 5, 5)
#define REG_CLK_INV_MASK		BIT(4)
#define REG_CLK_INV(x)			UPDATE(x, 4, 4)
#define VCLK_INV_MASK			BIT(3)
#define VCLK_INV(x)			UPDATE(x, 3, 3)
#define REG_CLK_SOURCE_MASK		BIT(2)
#define REG_CLK_SOURCE(x)		UPDATE(x, 2, 2)
#define POWER_MASK			BIT(1)
#define PWR_OFF(x)			UPDATE(x, 1, 1)
#define INT_POL_MASK			BIT(0)
#define INT_POL(x)			UPDATE(x, 0, 0)

#define HDMI_VIDEO_CONTROL1		HDMITX_REG(0x01)
#define VIDEO_INPUT_FORMAT_MASK		GENMASK(3, 1)
#define VIDEO_INPUT_SDR_RGB444		UPDATE(0x0, 3, 1)
#define VIDEO_INPUT_DDR_RGB444		UPDATE(0x5, 3, 1)
#define VIDEO_INPUT_DDR_YCBCR422	UPDATE(0x6, 3, 1)
#define DE_SOURCE_MASK			BIT(0)
#define DE_SOURCE(x)			UPDATE(x, 0, 0)

#define HDMI_VIDEO_CONTROL2		HDMITX_REG(0x02)
#define VIDEO_OUTPUT_COLOR_MASK		GENMASK(7, 6)
#define VIDEO_OUTPUT_RRGB444		UPDATE(0x0, 7, 6)
#define VIDEO_OUTPUT_YCBCR444		UPDATE(0x1, 7, 6)
#define VIDEO_OUTPUT_YCBCR422		UPDATE(0x2, 7, 6)
#define VIDEO_INPUT_BITS_MASK		GENMASK(5, 4)
#define VIDEO_INPUT_12BITS		UPDATE(0x0, 5, 4)
#define VIDEO_INPUT_10BITS		UPDATE(0x1, 5, 4)
#define VIDEO_INPUT_REVERT		UPDATE(0x2, 5, 4)
#define VIDEO_INPUT_8BITS		UPDATE(0x3, 5, 4)
#define VIDEO_INPUT_CSP_MASK		BIT(1)
#define VIDEO_INPUT_CSP(x)		UPDATE(x, 0, 0)

#define HDMI_VIDEO_CONTROL		HDMITX_REG(0x03)
#define VIDEO_AUTO_CSC_MASK		BIT(7)
#define VIDEO_AUTO_CSC(x)		UPDATE(x, 7, 7)
#define VIDEO_C0_C2_SWAP_MASK		BIT(0)
#define VIDEO_C0_C2_SWAP(x)		UPDATE(x, 0, 0)
enum {
	C0_C2_CHANGE_ENABLE = 0,
	C0_C2_CHANGE_DISABLE = 1,
	AUTO_CSC_DISABLE = 0,
	AUTO_CSC_ENABLE = 1,
};

#define HDMI_VIDEO_CONTROL3		HDMITX_REG(0x04)
#define COLOR_DEPTH_NOT_INDICATED_MASK	BIT(4)
#define COLOR_DEPTH_NOT_INDICATED(x)	UPDATE(x, 4, 4)
#define SOF_MASK			BIT(3)
#define SOF_DISABLE(x)			UPDATE(x, 3, 3)
#define CSC_MASK			BIT(0)
#define CSC_ENABLE(x)			UPDATE(x, 0, 0)

#define HDMI_AV_MUTE			HDMITX_REG(0x05)
#define AVMUTE_CLEAR_MASK		BIT(7)
#define AVMUTE_CLEAR(x)			UPDATE(x, 7, 7)
#define AVMUTE_ENABLE_MASK		BIT(6)
#define AVMUTE_ENABLE(x)		UPDATE(x, 6, 6)
#define AUDIO_PD_MASK			BIT(2)
#define AUDIO_PD(x)			UPDATE(x, 2, 2)
#define AUDIO_MUTE_MASK			BIT(1)
#define AUDIO_MUTE(x)			UPDATE(x, 1, 1)
#define VIDEO_BLACK_MASK		BIT(0)
#define VIDEO_MUTE(x)			UPDATE(x, 0, 0)

#define HDMI_VIDEO_TIMING_CTL		HDMITX_REG(0x08)
#define HSYNC_POLARITY(x)		UPDATE(x, 3, 3)
#define VSYNC_POLARITY(x)		UPDATE(x, 2, 2)
#define INETLACE(x)			UPDATE(x, 1, 1)
#define EXTERANL_VIDEO(x)		UPDATE(x, 0, 0)

#define HDMI_VIDEO_EXT_HTOTAL_L		HDMITX_REG(0x09)
#define HDMI_VIDEO_EXT_HTOTAL_H		HDMITX_REG(0x0a)
#define HDMI_VIDEO_EXT_HBLANK_L		HDMITX_REG(0x0b)
#define HDMI_VIDEO_EXT_HBLANK_H		HDMITX_REG(0x0c)
#define HDMI_VIDEO_EXT_HDELAY_L		HDMITX_REG(0x0d)
#define HDMI_VIDEO_EXT_HDELAY_H		HDMITX_REG(0x0e)
#define HDMI_VIDEO_EXT_HDURATION_L	HDMITX_REG(0x0f)
#define HDMI_VIDEO_EXT_HDURATION_H	HDMITX_REG(0x10)
#define HDMI_VIDEO_EXT_VTOTAL_L		HDMITX_REG(0x11)
#define HDMI_VIDEO_EXT_VTOTAL_H		HDMITX_REG(0x12)
#define HDMI_VIDEO_EXT_VBLANK		HDMITX_REG(0x13)
#define HDMI_VIDEO_EXT_VDELAY		HDMITX_REG(0x14)
#define HDMI_VIDEO_EXT_VDURATION	HDMITX_REG(0x15)

#define HDMI_VIDEO_CSC_COEF		HDMITX_REG(0x18)

#define HDMI_AUDIO_CTRL1		HDMITX_REG(0x35)
enum {
	CTS_SOURCE_INTERNAL = 0,
	CTS_SOURCE_EXTERNAL = 1,
};

#define CTS_SOURCE(x)			UPDATE(x, 7, 7)

enum {
	DOWNSAMPLE_DISABLE = 0,
	DOWNSAMPLE_1_2 = 1,
	DOWNSAMPLE_1_4 = 2,
};

#define DOWN_SAMPLE(x)			UPDATE(x, 6, 5)

enum {
	AUDIO_SOURCE_IIS = 0,
	AUDIO_SOURCE_SPDIF = 1,
};

#define AUDIO_SOURCE(x)			UPDATE(x, 4, 3)
#define MCLK_ENABLE(x)			UPDATE(x, 2, 2)

enum {
	MCLK_128FS = 0,
	MCLK_256FS = 1,
	MCLK_384FS = 2,
	MCLK_512FS = 3,
};

#define MCLK_RATIO(x)			UPDATE(x, 1, 0)

#define AUDIO_SAMPLE_RATE		HDMITX_REG(0x37)
enum {
	AUDIO_32K = 0x3,
	AUDIO_441K = 0x0,
	AUDIO_48K = 0x2,
	AUDIO_882K = 0x8,
	AUDIO_96K = 0xa,
	AUDIO_1764K = 0xc,
	AUDIO_192K = 0xe,
};

#define AUDIO_I2S_MODE			HDMITX_REG(0x38)
enum {
	I2S_CHANNEL_1_2 = 1,
	I2S_CHANNEL_3_4 = 3,
	I2S_CHANNEL_5_6 = 7,
	I2S_CHANNEL_7_8 = 0xf
};

#define I2S_CHANNEL(x)			UPDATE(x, 5, 2)

enum {
	I2S_STANDARD = 0,
	I2S_LEFT_JUSTIFIED = 1,
	I2S_RIGHT_JUSTIFIED = 2,
};

#define I2S_MODE(x)			UPDATE(x, 1, 0)

#define AUDIO_I2S_MAP			HDMITX_REG(0x39)
#define AUDIO_I2S_SWAPS_SPDIF		HDMITX_REG(0x3a)
#define N_32K				0x1000
#define N_441K				0x1880
#define N_882K				0x3100
#define N_1764K				0x6200
#define N_48K				0x1800
#define N_96K				0x3000
#define N_192K				0x6000

#define HDMI_AUDIO_CHANNEL_STATUS	HDMITX_REG(0x3e)
#define AUDIO_STATUS_NLPCM_MASK		BIT(7)
#define AUDIO_STATUS_NLPCM(x)		UPDATE(x, 7, 7)
#define AUDIO_STATUS_USE_MASK		BIT(6)
#define AUDIO_STATUS_COPYRIGHT_MASK	BIT(5)
#define AUDIO_STATUS_ADDITION_MASK	GENMASK(3, 2)
#define AUDIO_STATUS_CLK_ACCURACY_MASK	GENMASK(1, 1)

#define AUDIO_N_H			HDMITX_REG(0x3f)
#define AUDIO_N_M			HDMITX_REG(0x40)
#define AUDIO_N_L			HDMITX_REG(0x41)

#define HDMI_AUDIO_CTS_H		HDMITX_REG(0x45)
#define HDMI_AUDIO_CTS_M		HDMITX_REG(0x46)
#define HDMI_AUDIO_CTS_L		HDMITX_REG(0x47)

#define HDMI_DDC_CLK_L			HDMITX_REG(0x4b)
#define HDMI_DDC_CLK_H			HDMITX_REG(0x4c)

#define HDMI_EDID_SEGMENT_POINTER	HDMITX_REG(0x4d)
#define HDMI_EDID_WORD_ADDR		HDMITX_REG(0x4e)
#define HDMI_EDID_FIFO_OFFSET		HDMITX_REG(0x4f)
#define HDMI_EDID_FIFO_ADDR		HDMITX_REG(0x50)

#define HDMI_PACKET_SEND_MANUAL		HDMITX_REG(0x9c)
#define HDMI_PACKET_SEND_AUTO		HDMITX_REG(0x9d)
#define PACKET_GCP_EN_MASK		BIT(7)
#define PACKET_GCP_EN(x)		UPDATE(x, 7, 7)
#define PACKET_MSI_EN_MASK		BIT(6)
#define PACKET_MSI_EN(x)		UPDATE(x, 6, 6)
#define PACKET_SDI_EN_MASK		BIT(5)
#define PACKET_SDI_EN(x)		UPDATE(x, 5, 5)
#define PACKET_VSI_EN_MASK		BIT(4)
#define PACKET_VSI_EN(x)		UPDATE(x, 4, 4)

#define HDMI_CONTROL_PACKET_BUF_INDEX	HDMITX_REG(0x9f)
enum {
	INFOFRAME_VSI = 0x05,
	INFOFRAME_AVI = 0x06,
	INFOFRAME_AAI = 0x08,
};

#define HDMI_CONTROL_PACKET_ADDR	HDMITX_REG(0xa0)
#define HDMI_MAXIMUM_INFO_FRAME_SIZE	0x11
enum {
	AVI_COLOR_MODE_RGB = 0,
	AVI_COLOR_MODE_YCBCR422 = 1,
	AVI_COLOR_MODE_YCBCR444 = 2,
	AVI_COLORIMETRY_NO_DATA = 0,

	AVI_COLORIMETRY_SMPTE_170M = 1,
	AVI_COLORIMETRY_ITU709 = 2,
	AVI_COLORIMETRY_EXTENDED = 3,

	AVI_CODED_FRAME_ASPECT_NO_DATA = 0,
	AVI_CODED_FRAME_ASPECT_4_3 = 1,
	AVI_CODED_FRAME_ASPECT_16_9 = 2,

	ACTIVE_ASPECT_RATE_SAME_AS_CODED_FRAME = 0x08,
	ACTIVE_ASPECT_RATE_4_3 = 0x09,
	ACTIVE_ASPECT_RATE_16_9 = 0x0A,
	ACTIVE_ASPECT_RATE_14_9 = 0x0B,
};

#define HDMI_HDCP_CTRL			HDMITX_REG(0x52)
#define HDMI_DVI_MASK			BIT(1)
#define HDMI_DVI(x)			UPDATE(x, 1, 1)

#define HDMI_INTERRUPT_MASK1		HDMITX_REG(0xc0)
#define INT_EDID_READY_MASK		BIT(2)
#define HDMI_INTERRUPT_STATUS1		HDMITX_REG(0xc1)
#define	INT_ACTIVE_VSYNC_MASK		BIT(5)
#define INT_EDID_READY			BIT(2)

#define HDMI_INTERRUPT_MASK2		HDMITX_REG(0xc2)
#define HDMI_INTERRUPT_STATUS2		HDMITX_REG(0xc3)
#define INT_HDCP_ERR			BIT(7)
#define INT_BKSV_FLAG			BIT(6)
#define INT_HDCP_OK			BIT(4)

#define HDMI_STATUS			HDMITX_REG(0xc8)
#define HOTPLUG_STATUS			BIT(7)
#define MASK_INT_HOTPLUG_MASK		BIT(5)
#define MASK_INT_HOTPLUG(x)		UPDATE(x, 5, 5)
#define INT_HOTPLUG			BIT(1)

#define HDMI_COLORBAR                   HDMITX_REG(0xc9)

#define HDMI_PHY_SYNC			HDMITX_REG(0xce)
#define HDMI_PHY_SYS_CTL		HDMITX_REG(0xe0)
#define TMDS_CLK_SOURCE_MASK		BIT(5)
#define TMDS_CLK_SOURCE(x)		UPDATE(x, 5, 5)
#define PHASE_CLK_MASK			BIT(4)
#define PHASE_CLK(x)			UPDATE(x, 4, 4)
#define TMDS_PHASE_SEL_MASK		BIT(3)
#define TMDS_PHASE_SEL(x)		UPDATE(x, 3, 3)
#define BANDGAP_PWR_MASK		BIT(2)
#define BANDGAP_PWR(x)			UPDATE(x, 2, 2)
#define PLL_PWR_DOWN_MASK		BIT(1)
#define PLL_PWR_DOWN(x)			UPDATE(x, 1, 1)
#define TMDS_CHG_PWR_DOWN_MASK		BIT(0)
#define TMDS_CHG_PWR_DOWN(x)		UPDATE(x, 0, 0)

#define HDMI_PHY_CHG_PWR		HDMITX_REG(0xe1)
#define CLK_CHG_PWR(x)			UPDATE(x, 3, 3)
#define DATA_CHG_PWR(x)			UPDATE(x, 2, 0)

#define HDMI_PHY_DRIVER			HDMITX_REG(0xe2)
#define CLK_MAIN_DRIVER(x)		UPDATE(x, 7, 4)
#define DATA_MAIN_DRIVER(x)		UPDATE(x, 3, 0)

#define HDMI_PHY_PRE_EMPHASIS		HDMITX_REG(0xe3)
#define PRE_EMPHASIS(x)			UPDATE(x, 6, 4)
#define CLK_PRE_DRIVER(x)		UPDATE(x, 3, 2)
#define DATA_PRE_DRIVER(x)		UPDATE(x, 1, 0)

#define PHY_FEEDBACK_DIV_RATIO_LOW	HDMITX_REG(0xe7)
#define FEEDBACK_DIV_LOW(x)		UPDATE(x, 7, 0)
#define PHY_FEEDBACK_DIV_RATIO_HIGH	HDMITX_REG(0xe8)
#define FEEDBACK_DIV_HIGH(x)		UPDATE(x, 0, 0)

#define HDMI_PHY_PRE_DIV_RATIO		HDMITX_REG(0xed)
#define PRE_DIV_RATIO(x)		UPDATE(x, 4, 0)

#define HDMI_CEC_CTRL			HDMITX_REG(0xd0)
#define ADJUST_FOR_HISENSE_MASK		BIT(6)
#define REJECT_RX_BROADCAST_MASK	BIT(5)
#define BUSFREETIME_ENABLE_MASK		BIT(2)
#define REJECT_RX_MASK			BIT(1)
#define START_TX_MASK			BIT(0)

#define HDMI_CEC_DATA			HDMITX_REG(0xd1)
#define HDMI_CEC_TX_OFFSET		HDMITX_REG(0xd2)
#define HDMI_CEC_RX_OFFSET		HDMITX_REG(0xd3)
#define HDMI_CEC_CLK_H			HDMITX_REG(0xd4)
#define HDMI_CEC_CLK_L			HDMITX_REG(0xd5)
#define HDMI_CEC_TX_LENGTH		HDMITX_REG(0xd6)
#define HDMI_CEC_RX_LENGTH		HDMITX_REG(0xd7)
#define HDMI_CEC_TX_INT_MASK		HDMITX_REG(0xd8)
#define TX_DONE_MASK			BIT(3)
#define TX_NOACK_MASK			BIT(2)
#define TX_BROADCAST_REJ_MASK		BIT(1)
#define TX_BUSNOTFREE_MASK		BIT(0)

#define HDMI_CEC_RX_INT_MASK		HDMITX_REG(0xd9)
#define RX_LA_ERR_MASK			BIT(4)
#define RX_GLITCH_MASK			BIT(3)
#define RX_DONE_MASK			BIT(0)

#define HDMI_CEC_TX_INT			HDMITX_REG(0xda)
#define HDMI_CEC_RX_INT			HDMITX_REG(0xdb)
#define HDMI_CEC_BUSFREETIME_L		HDMITX_REG(0xdc)
#define HDMI_CEC_BUSFREETIME_H		HDMITX_REG(0xdd)
#define HDMI_CEC_LOGICADDR		HDMITX_REG(0xde)
#define HDMI_COLOR_BAR			HDMITX_REG(0xc9)
#define VIDEO_BIST_MODE_MASK		GENMASK(7, 6)
#define VIDEO_BIST_MODE(x)		UPDATE(x, 7, 6)
#define DISABLE_COLORBAR_BIST_MASK	BIT(4)
#define DISABLE_COLORBAR_BIST(x)	UPDATE(x, 4, 4)
#define HDMI_MAX_REG			HDMITX_REG(0xed)

#ifdef CONFIG_RK628_MISC_HDMITX
void rk628_hdmitx_disable(struct rk628 *rk628);
int rk628_hdmitx_enable(struct rk628 *rk628);
void rk628_hdmitx_create_debugfs_file(struct rk628 *rk628);
#else
static inline void rk628_hdmitx_disable(struct rk628 *rk628)
{
}

static inline int rk628_hdmitx_enable(struct rk628 *rk628)
{
	return 0;
}

static inline void rk628_hdmitx_create_debugfs_file(struct rk628 *rk628)
{
}
#endif

#endif
