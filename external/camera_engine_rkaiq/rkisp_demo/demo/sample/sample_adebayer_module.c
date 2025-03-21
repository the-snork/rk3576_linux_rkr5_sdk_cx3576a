/*
 *  Copyright (c) 2021 Rockchip Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "sample_comm.h"

#include "uAPI2/rk_aiq_user_api2_helper.h"
//#include <string>

#define INTERP_SAMPLE(x0, x1, ratio)    ((ratio) * ((x1) - (x0)) + x0)
#define ISO_STEP_MAX 13

static void sample_adebayer_usage()
{
    printf("Usage : \n");
    printf("  Module API: \n");
    printf("\t 0) ADEBAYER v1:         enable/disable with AUTO mode in sync.\n");
    printf("\t 1) ADEBAYER v1:         set sharp strength of 250 with AUTO modein sync.\n");
    printf("\t 2) ADEBAYER v1:         set sharp strength of 0 with AUTO modein sync.\n");
    printf("\t 3) ADEBAYER v1:         set high freq thresh of 250 with AUTO modein sync.\n");
    printf("\t 4) ADEBAYER v1:         set high freq thresh of 0 with AUTO mode in sync.\n");
    printf("\t 5) ADEBAYER v1:         set low freq thresh of 250 with AUTO mode in sync.\n");
    printf("\t 6) ADEBAYER v1:         set low freq thresh of 0 with AUTO mode in sync.\n");
    printf("\n");

    printf("\t 7) ADEBAYER v1:         enable/disable with MANUAL mode in sync.\n");
    printf("\t 8) ADEBAYER v1:         set manual params from json with MANUAL mode in sync, iso is 50.\n");
    printf("\t 9) ADEBAYER v1:         set manual params from json with MANUAL mode in sync, iso is 6400.\n");

    printf("\t a) ADEBAYER v2:         set manual params with MANUAL mode in sync, iso is 50.\n");
    printf("\t b) ADEBAYER v2:         set manual params with MANUAL mode in sync, iso is 6400.\n");
    printf("\t c) ADEBAYER v2:         set auto params with AUTO mode in sync.\n");

    printf("\t d) ADEBAYER v2-lite:    set manual params with MANUAL mode in sync, iso is 900.\n");
    printf("\t e) ADEBAYER v2-lite:    set manual params with MANUAL mode in sync, iso is 2800.\n");
    printf("\t f) ADEBAYER v2-lite:    set auto params with AUTO mode in sync.\n");

    printf("\t g) ADEBAYER v3:         set manual params with MANUAL mode in sync, iso is 900.\n");
    printf("\t i) ADEBAYER v3:         set manual params with MANUAL mode in sync, iso is 2800.\n");
    printf("\t j) ADEBAYER v3:         set auto params with AUTO mode in sync.\n");
    printf("\t k) DM:                  test mode/en switch.\n");

    printf("\n");

    printf("\t h) ADEBAYER: help.\n");
    printf("\t q/Q) ADEBAYER:       return to main sample screen.\n");
    printf("\n");
    printf("\t please press the key: \n\n");

    return;
}

void sample_print_adebayer_info(const void *arg)
{
    printf ("enter ADEBAYER test!\n");
}

XCamReturn sample_adebayer_en_v1(const rk_aiq_sys_ctx_t* ctx, rk_aiq_debayer_op_mode_t mode, bool en)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    if (ctx == NULL) {
        ret = XCAM_RETURN_ERROR_PARAM;
        RKAIQ_SAMPLE_CHECK_RET(ret, "param error!");
    }

    adebayer_attrib_t attr;
    ret = rk_aiq_user_api2_adebayer_GetAttrib(ctx, &attr);
    RKAIQ_SAMPLE_CHECK_RET(ret, "get debayer attrib failed!");
    attr.enable = en;
    attr.mode = mode;
    rk_aiq_user_api2_adebayer_SetAttrib(ctx, attr);

    ret = rk_aiq_user_api2_adebayer_GetAttrib(ctx, &attr);
    RKAIQ_SAMPLE_CHECK_RET(ret, "get ldch attrib failed!");

    printf ("sync_mode: %d, done: %d\n", attr.sync.sync_mode, attr.sync.done);

    return ret;
}

XCamReturn sample_adebayer_setSharpStrength_v1(const rk_aiq_sys_ctx_t* ctx, unsigned char *strength)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    if (ctx == NULL) {
        ret = XCAM_RETURN_ERROR_PARAM;
        RKAIQ_SAMPLE_CHECK_RET(ret, "param error!");
    }

    adebayer_attrib_t attr;
    ret = rk_aiq_user_api2_adebayer_GetAttrib(ctx, &attr);
    RKAIQ_SAMPLE_CHECK_RET(ret, "get debayer attrib failed!");
    attr.mode = RK_AIQ_DEBAYER_MODE_AUTO;
    memcpy(attr.stAuto.sharp_strength, strength, sizeof(attr.stAuto.sharp_strength));
    rk_aiq_user_api2_adebayer_SetAttrib(ctx, attr);

    ret = rk_aiq_user_api2_adebayer_GetAttrib(ctx, &attr);
    RKAIQ_SAMPLE_CHECK_RET(ret, "get ldch attrib failed!");

    printf ("mode: %d, sync_mode: %d, done: %d\n", attr.mode, attr.sync.sync_mode, attr.sync.done);

    return ret;
}

XCamReturn sample_adebayer_setLowFreqThresh_v1(const rk_aiq_sys_ctx_t* ctx, __u8 thresh)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    if (ctx == NULL) {
        ret = XCAM_RETURN_ERROR_PARAM;
        RKAIQ_SAMPLE_CHECK_RET(ret, "param error!");
    }

    adebayer_attrib_t attr;
    ret = rk_aiq_user_api2_adebayer_GetAttrib(ctx, &attr);
    RKAIQ_SAMPLE_CHECK_RET(ret, "get debayer attrib failed!");
    attr.mode = RK_AIQ_DEBAYER_MODE_AUTO;
    attr.stAuto.low_freq_thresh = thresh;
    rk_aiq_user_api2_adebayer_SetAttrib(ctx, attr);

    ret = rk_aiq_user_api2_adebayer_GetAttrib(ctx, &attr);
    RKAIQ_SAMPLE_CHECK_RET(ret, "get ldch attrib failed!");

    printf ("mode: %d, sync_mode: %d, done: %d\n", attr.mode, attr.sync.sync_mode, attr.sync.done);

    return ret;
}

XCamReturn sample_adebayer_setHighFreqThresh_v1(const rk_aiq_sys_ctx_t* ctx, __u8 thresh)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    if (ctx == NULL) {
        ret = XCAM_RETURN_ERROR_PARAM;
        RKAIQ_SAMPLE_CHECK_RET(ret, "param error!");
    }

    adebayer_attrib_t attr;
    ret = rk_aiq_user_api2_adebayer_GetAttrib(ctx, &attr);
    RKAIQ_SAMPLE_CHECK_RET(ret, "get debayer attrib failed!");
    attr.mode = RK_AIQ_DEBAYER_MODE_AUTO;
    attr.stAuto.high_freq_thresh = thresh;
    rk_aiq_user_api2_adebayer_SetAttrib(ctx, attr);

    printf ("mode: %d, sync_mode: %d, done: %d\n", attr.mode, attr.sync.sync_mode, attr.sync.done);

    return ret;
}

/* debayer json params from os04a10_DH3588AVS6_default.json*/
XCamReturn
sample_adebayer_translate_params_v1(adebayer_attrib_manual_t* stManual, int32_t ISO)
{
    int8_t filter1_coe[5] = {2, -6, 0, 6, -2};
    int8_t filter2_coe[5] = {2, -4, 4, -4, 2};

    for (int i = 0; i < 5; i++)
    {
        stManual->filter1[i] =  filter1_coe[i];
        stManual->filter2[i] =  filter2_coe[i];
    }
    stManual->gain_offset = 4;

    int sharp_strength_tmp[9];
    uint16_t iso[9]           = {50, 100, 200, 400, 800, 1600, 3200, 6400, 12800};
    uint8_t sharp_strength[9] = {1, 4, 4, 4, 4, 4, 4, 4, 4};
    uint8_t hf_offset[9]      = {1, 1, 1, 1, 1, 1, 1, 1, 1};

    for (int i = 0; i < 9; i ++)
    {
        float iso_index = iso[i];
        int gain = (int)(log((float)iso_index / 50) / log((float)2));
        sharp_strength_tmp[gain] = sharp_strength[i];
    }
    stManual->offset = 1;

    int hfOffset_tmp[9];
    for (int i = 0; i < 9; i ++)
    {
        float iso_index = iso[i];
        int gain = (int)(log((float)iso_index / 50) / log((float)2));
        hfOffset_tmp[gain]  = hf_offset[i];
    }
    stManual->clip_en        = 1;
    stManual->filter_g_en    = 1;
    stManual->filter_c_en    = 1;
    stManual->thed0          = 3;
    stManual->thed1          = 6;
    stManual->dist_scale     = 8;
    stManual->shift_num      = 2;

    //select sharp params
    int iso_low = ISO, iso_high = ISO;
    int gain_high, gain_low;
    float ratio = 0.0f;
    int iso_div             = 50;
    int max_iso_step        = 9;
    for (int i = max_iso_step - 1; i >= 0; i--)
    {
        if (ISO < iso_div * (2 << i))
        {
            iso_low = iso_div * (2 << (i)) / 2;
            iso_high = iso_div * (2 << i);
        }
    }
    ratio = (float)(ISO - iso_low) / (iso_high - iso_low);
    if (iso_low == ISO)
    {
        iso_high = ISO;
        ratio = 0;
    }
    if (iso_high == ISO )
    {
        iso_low = ISO;
        ratio = 1;
    }
    gain_high = (int)(log((float)iso_high / 50) / log((float)2));
    gain_low = (int)(log((float)iso_low / 50) / log((float)2));

    gain_low = MIN(MAX(gain_low, 0), 8);
    gain_high = MIN(MAX(gain_high, 0), 8);

    stManual->sharp_strength = ((ratio) * (sharp_strength_tmp[gain_high] - sharp_strength_tmp[gain_low]) + sharp_strength_tmp[gain_low]);
    stManual->hf_offset = ((ratio) * (hfOffset_tmp[gain_high] - hfOffset_tmp[gain_low]) + hfOffset_tmp[gain_low]);

    printf ("sharp_strength: %d, hf_offset: %d\n", stManual->sharp_strength, stManual->hf_offset);

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn sample_adebayer_setManualAtrrib_v1(const rk_aiq_sys_ctx_t* ctx, int32_t ISO)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    if (ctx == NULL) {
        ret = XCAM_RETURN_ERROR_PARAM;
        RKAIQ_SAMPLE_CHECK_RET(ret, "param error!");
    }

    adebayer_attrib_t attr;
    ret = rk_aiq_user_api2_adebayer_GetAttrib(ctx, &attr);
    RKAIQ_SAMPLE_CHECK_RET(ret, "get debayer attrib failed!");
    attr.mode = RK_AIQ_DEBAYER_MODE_MANUAL;
    sample_adebayer_translate_params_v1(&attr.stManual, ISO);
    rk_aiq_user_api2_adebayer_SetAttrib(ctx, attr);

    printf ("mode: %d, sync_mode: %d, done: %d\n", attr.mode, attr.sync.sync_mode, attr.sync.done);

    return ret;
}

XCamReturn
sample_adebayer_translate_params_v2(adebayer_v2_attrib_t* attr, int32_t iso)
{
    //select sharp params
    int i = 0;
    int iso_low = 0, iso_high = 0, iso_low_index = 0, iso_high_index = 0;
    float ratio = 0.0f;

    //g_interp

    for(i = 0; i < ISO_STEP_MAX; i++) {
        if (iso < attr->stAuto.g_interp.iso[i])
        {

            iso_low = attr->stAuto.g_interp.iso[MAX(0, i - 1)];
            iso_high = attr->stAuto.g_interp.iso[i];
            iso_low_index = MAX(0, i - 1);
            iso_high_index = i;

            if(i == 0)
                ratio = 0.0f;
            else
                ratio = (float)(iso - iso_low) / (iso_high - iso_low);

            break;
        }
    }

    if(i == ISO_STEP_MAX) {
        // iso_low = attr->stAuto.g_interp.iso[i - 1];
        // iso_high = attr->stAuto.g_interp.iso[i - 1];
        iso_low_index = i - 1;
        iso_high_index = i - 1;
        ratio = 1;
    }

    attr->stManual.debayer_gain_offset = ROUND_F(INTERP_SAMPLE(attr->stAuto.g_interp.debayer_gain_offset[iso_low_index], attr->stAuto.g_interp.debayer_gain_offset[iso_high_index], ratio));
    attr->stManual.debayer_max_ratio = ROUND_F(INTERP_SAMPLE(attr->stAuto.g_interp.debayer_max_ratio[iso_low_index], attr->stAuto.g_interp.debayer_max_ratio[iso_high_index], ratio));
    attr->stManual.debayer_clip_en = ROUND_F(INTERP_SAMPLE(attr->stAuto.g_interp.debayer_clip_en[iso_low_index], attr->stAuto.g_interp.debayer_clip_en[iso_high_index], ratio));

    attr->stManual.debayer_thed0 = ROUND_F(INTERP_SAMPLE(attr->stAuto.g_drctwgt.debayer_thed0[iso_low_index], attr->stAuto.g_drctwgt.debayer_thed0[iso_high_index], ratio));
    attr->stManual.debayer_thed1 = ROUND_F(INTERP_SAMPLE(attr->stAuto.g_drctwgt.debayer_thed1[iso_low_index], attr->stAuto.g_drctwgt.debayer_thed1[iso_high_index], ratio));

    attr->stManual.debayer_dist_scale = ROUND_F(INTERP_SAMPLE(attr->stAuto.g_drctwgt.debayer_dist_scale[iso_low_index], attr->stAuto.g_drctwgt.debayer_dist_scale[iso_high_index], ratio));
    attr->stManual.debayer_hf_offset = ROUND_F(INTERP_SAMPLE(attr->stAuto.g_drctwgt.debayer_hf_offset[iso_low_index], attr->stAuto.g_drctwgt.debayer_hf_offset[iso_high_index], ratio));
    attr->stManual.debayer_select_thed = ROUND_F(INTERP_SAMPLE(attr->stAuto.g_drctwgt.debayer_select_thed[iso_low_index], attr->stAuto.g_drctwgt.debayer_select_thed[iso_high_index], ratio));


    attr->stManual.debayer_gfilter_en = ROUND_F(INTERP_SAMPLE(attr->stAuto.g_filter.debayer_gfilter_en[iso_low_index], attr->stAuto.g_filter.debayer_gfilter_en[iso_high_index], ratio));
    attr->stManual.debayer_gfilter_offset = ROUND_F(INTERP_SAMPLE(attr->stAuto.g_filter.debayer_gfilter_offset[iso_low_index], attr->stAuto.g_filter.debayer_gfilter_offset[iso_high_index], ratio));


    attr->stManual.debayer_cfilter_en = ROUND_F(INTERP_SAMPLE(attr->stAuto.c_filter.debayer_cfilter_en[iso_low_index], attr->stAuto.c_filter.debayer_cfilter_en[iso_high_index], ratio));

    attr->stManual.debayer_alpha_offset = ROUND_F(INTERP_SAMPLE(attr->stAuto.c_filter.debayer_alpha_offset[iso_low_index], attr->stAuto.c_filter.debayer_alpha_offset[iso_high_index], ratio));
    attr->stManual.debayer_alpha_scale = INTERP_SAMPLE(attr->stAuto.c_filter.debayer_alpha_scale[iso_low_index], attr->stAuto.c_filter.debayer_alpha_scale[iso_high_index], ratio);
    attr->stManual.debayer_edge_offset = ROUND_F(INTERP_SAMPLE(attr->stAuto.c_filter.debayer_edge_offset[iso_low_index], attr->stAuto.c_filter.debayer_edge_offset[iso_high_index], ratio));
    attr->stManual.debayer_edge_scale = INTERP_SAMPLE(attr->stAuto.c_filter.debayer_edge_scale[iso_low_index], attr->stAuto.c_filter.debayer_edge_scale[iso_high_index], ratio);

    attr->stManual.debayer_bf_sgm = INTERP_SAMPLE(attr->stAuto.c_filter.debayer_bf_sgm[iso_low_index], attr->stAuto.c_filter.debayer_bf_sgm[iso_high_index], ratio);
    attr->stManual.debayer_bf_curwgt = ROUND_F(INTERP_SAMPLE(attr->stAuto.c_filter.debayer_bf_curwgt[iso_low_index], attr->stAuto.c_filter.debayer_bf_curwgt[iso_high_index], ratio));
    attr->stManual.debayer_bf_clip = ROUND_F(INTERP_SAMPLE(attr->stAuto.c_filter.debayer_bf_clip[iso_low_index], attr->stAuto.c_filter.debayer_bf_clip[iso_high_index], ratio));

    attr->stManual.debayer_loggd_offset = ROUND_F(INTERP_SAMPLE(attr->stAuto.c_filter.debayer_loggd_offset[iso_low_index], attr->stAuto.c_filter.debayer_loggd_offset[iso_high_index], ratio));
    attr->stManual.debayer_loghf_offset = ROUND_F(INTERP_SAMPLE(attr->stAuto.c_filter.debayer_loghf_offset[iso_low_index], attr->stAuto.c_filter.debayer_loghf_offset[iso_high_index], ratio));

    attr->stManual.debayer_cfilter_str = INTERP_SAMPLE(attr->stAuto.c_filter.debayer_cfilter_str[iso_low_index], attr->stAuto.c_filter.debayer_cfilter_str[iso_high_index], ratio);
    attr->stManual.debayer_wet_clip = INTERP_SAMPLE(attr->stAuto.c_filter.debayer_wet_clip[iso_low_index], attr->stAuto.c_filter.debayer_wet_clip[iso_high_index], ratio);
    attr->stManual.debayer_wet_ghost = INTERP_SAMPLE(attr->stAuto.c_filter.debayer_wet_ghost[iso_low_index], attr->stAuto.c_filter.debayer_wet_ghost[iso_high_index], ratio);
    attr->stManual.debayer_wgtslope = INTERP_SAMPLE(attr->stAuto.c_filter.debayer_wgtslope[iso_low_index], attr->stAuto.c_filter.debayer_wgtslope[iso_high_index], ratio);

    printf ("sharp_strength: %d, hf_offset: %d\n", attr->stManual.debayer_max_ratio, attr->stManual.debayer_loghf_offset);

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn sample_adebayer_setManualAtrrib_v2(const rk_aiq_sys_ctx_t* ctx, int32_t ISO)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    if (ctx == NULL) {
        ret = XCAM_RETURN_ERROR_PARAM;
        RKAIQ_SAMPLE_CHECK_RET(ret, "param error!");
    }

    adebayer_v2_attrib_t attr;
    ret = rk_aiq_user_api2_adebayer_v2_GetAttrib(ctx, &attr);
    RKAIQ_SAMPLE_CHECK_RET(ret, "get debayer v2 attrib failed!");
    attr.mode = RK_AIQ_DEBAYER_MODE_MANUAL;
    sample_adebayer_translate_params_v2(&attr, ISO);
    rk_aiq_user_api2_adebayer_v2_SetAttrib(ctx, attr);

    printf ("mode: %d, sync_mode: %d, done: %d\n", attr.mode, attr.sync.sync_mode, attr.sync.done);

    return ret;
}

XCamReturn sample_adebayer_setAutoAtrrib_v2(const rk_aiq_sys_ctx_t* ctx)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    if (ctx == NULL) {
        ret = XCAM_RETURN_ERROR_PARAM;
        RKAIQ_SAMPLE_CHECK_RET(ret, "param error!");
    }

    adebayer_v2_attrib_t attr;
    ret = rk_aiq_user_api2_adebayer_v2_GetAttrib(ctx, &attr);
    RKAIQ_SAMPLE_CHECK_RET(ret, "get debayer v2 attrib failed!");
    attr.mode = RK_AIQ_DEBAYER_MODE_AUTO;

    attr.stAuto.c_filter.debayer_cfilter_en[0] = false;
    attr.stAuto.c_filter.debayer_cfilter_en[1] = false;
    attr.stAuto.c_filter.debayer_cfilter_en[2] = false;

    rk_aiq_user_api2_adebayer_v2_SetAttrib(ctx, attr);

    printf ("mode: %d, sync_mode: %d, done: %d\n", attr.mode, attr.sync.sync_mode, attr.sync.done);

    return ret;
}

XCamReturn
sample_adebayer_translate_params_v2lite(adebayer_v2lite_attrib_t* attr, int32_t iso)
{
    //select sharp params
    int i = 0;
    int iso_low = 0, iso_high = 0, iso_low_index = 0, iso_high_index = 0;
    float ratio = 0.0f;

    //g_interp

    for(i = 0; i < ISO_STEP_MAX; i++) {
        if (iso < attr->stAuto.g_interp.iso[i])
        {

            iso_low = attr->stAuto.g_interp.iso[MAX(0, i - 1)];
            iso_high = attr->stAuto.g_interp.iso[i];
            iso_low_index = MAX(0, i - 1);
            iso_high_index = i;

            if(i == 0)
                ratio = 0.0f;
            else
                ratio = (float)(iso - iso_low) / (iso_high - iso_low);

            break;
        }
    }

    if(i == ISO_STEP_MAX) {
        // iso_low = attr->stAuto.g_interp.iso[i - 1];
        // iso_high = attr->stAuto.g_interp.iso[i - 1];
        iso_low_index = i - 1;
        iso_high_index = i - 1;
        ratio = 1;
    }

    attr->stManual.debayer_gain_offset = ROUND_F(INTERP_SAMPLE(attr->stAuto.g_interp.debayer_gain_offset[iso_low_index], attr->stAuto.g_interp.debayer_gain_offset[iso_high_index], ratio));
    attr->stManual.debayer_max_ratio = ROUND_F(INTERP_SAMPLE(attr->stAuto.g_interp.debayer_max_ratio[iso_low_index], attr->stAuto.g_interp.debayer_max_ratio[iso_high_index], ratio));
    attr->stManual.debayer_clip_en = ROUND_F(INTERP_SAMPLE(attr->stAuto.g_interp.debayer_clip_en[iso_low_index], attr->stAuto.g_interp.debayer_clip_en[iso_high_index], ratio));

    attr->stManual.debayer_thed0 = ROUND_F(INTERP_SAMPLE(attr->stAuto.g_drctwgt.debayer_thed0[iso_low_index], attr->stAuto.g_drctwgt.debayer_thed0[iso_high_index], ratio));
    attr->stManual.debayer_thed1 = ROUND_F(INTERP_SAMPLE(attr->stAuto.g_drctwgt.debayer_thed1[iso_low_index], attr->stAuto.g_drctwgt.debayer_thed1[iso_high_index], ratio));

    attr->stManual.debayer_dist_scale = ROUND_F(INTERP_SAMPLE(attr->stAuto.g_drctwgt.debayer_dist_scale[iso_low_index], attr->stAuto.g_drctwgt.debayer_dist_scale[iso_high_index], ratio));
    attr->stManual.debayer_hf_offset = ROUND_F(INTERP_SAMPLE(attr->stAuto.g_drctwgt.debayer_hf_offset[iso_low_index], attr->stAuto.g_drctwgt.debayer_hf_offset[iso_high_index], ratio));
    attr->stManual.debayer_select_thed = ROUND_F(INTERP_SAMPLE(attr->stAuto.g_drctwgt.debayer_select_thed[iso_low_index], attr->stAuto.g_drctwgt.debayer_select_thed[iso_high_index], ratio));


    attr->stManual.debayer_gfilter_en = ROUND_F(INTERP_SAMPLE(attr->stAuto.g_filter.debayer_gfilter_en[iso_low_index], attr->stAuto.g_filter.debayer_gfilter_en[iso_high_index], ratio));
    attr->stManual.debayer_gfilter_offset = ROUND_F(INTERP_SAMPLE(attr->stAuto.g_filter.debayer_gfilter_offset[iso_low_index], attr->stAuto.g_filter.debayer_gfilter_offset[iso_high_index], ratio));


    printf ("sharp_strength: %d, gflter_offset: %d\n", attr->stManual.debayer_max_ratio, attr->stManual.debayer_gfilter_offset);

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn sample_adebayer_setManualAtrrib_v2lite(const rk_aiq_sys_ctx_t* ctx, int32_t ISO)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    if (ctx == NULL) {
        ret = XCAM_RETURN_ERROR_PARAM;
        RKAIQ_SAMPLE_CHECK_RET(ret, "param error!");
    }

    adebayer_v2lite_attrib_t attr;
    ret = rk_aiq_user_api2_adebayer_v2_lite_GetAttrib(ctx, &attr);
    RKAIQ_SAMPLE_CHECK_RET(ret, "get debayer v2 attrib failed!");
    attr.mode = RK_AIQ_DEBAYER_MODE_MANUAL;
    sample_adebayer_translate_params_v2lite(&attr, ISO);
    rk_aiq_user_api2_adebayer_v2_lite_SetAttrib(ctx, attr);

    printf ("mode: %d, sync_mode: %d, done: %d\n", attr.mode, attr.sync.sync_mode, attr.sync.done);

    return ret;
}

XCamReturn sample_adebayer_setAutoAtrrib_v2lite(const rk_aiq_sys_ctx_t* ctx)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    if (ctx == NULL) {
        ret = XCAM_RETURN_ERROR_PARAM;
        RKAIQ_SAMPLE_CHECK_RET(ret, "param error!");
    }

    adebayer_v2lite_attrib_t attr;
    ret = rk_aiq_user_api2_adebayer_v2_lite_GetAttrib(ctx, &attr);
    RKAIQ_SAMPLE_CHECK_RET(ret, "get debayer v2 attrib failed!");
    attr.mode = RK_AIQ_DEBAYER_MODE_AUTO;

    attr.stAuto.g_filter.debayer_gfilter_offset[0] = 10;
    attr.stAuto.g_filter.debayer_gfilter_offset[1] = 10;
    attr.stAuto.g_filter.debayer_gfilter_offset[2] = 10;

    rk_aiq_user_api2_adebayer_v2_lite_SetAttrib(ctx, attr);

    printf ("mode: %d, sync_mode: %d, done: %d\n", attr.mode, attr.sync.sync_mode, attr.sync.done);

    return ret;
}

XCamReturn
sample_adebayer_translate_params_v3(adebayer_v3_attrib_t* attr, int32_t iso)
{
    //select sharp params
    int i = 0;
    int iso_low = 0, iso_high = 0, iso_low_index = 0, iso_high_index = 0;
    float ratio = 0.0f;

    //g_interp

    for(i = 0; i < ISO_STEP_MAX; i++) {
        if (iso < attr->stAuto.dyn.gDrctWgt.iso[i])
        {

            iso_low = attr->stAuto.dyn.gDrctWgt.iso[MAX(0, i - 1)];
            iso_high = attr->stAuto.dyn.gDrctWgt.iso[i];
            iso_low_index = MAX(0, i - 1);
            iso_high_index = i;

            if(i == 0)
                ratio = 0.0f;
            else
                ratio = (float)(iso - iso_low) / (iso_high - iso_low);

            break;
        }
    }

    if(i == ISO_STEP_MAX) {
        iso_low = attr->stAuto.dyn.gDrctWgt.iso[i - 1];
        iso_high = attr->stAuto.dyn.gDrctWgt.iso[i - 1];
        iso_low_index = i - 1;
        iso_high_index = i - 1;
        ratio = 1;
    }

    //0) static

    attr->stManual.hw_dmT_en = attr->stAuto.en;

    attr->stManual.hw_dmT_hiDrctFlt_coeff[0] = attr->stAuto.sta.gDrctWgt.hw_dmT_hiDrctFlt_coeff[0];
    attr->stManual.hw_dmT_hiDrctFlt_coeff[1] = attr->stAuto.sta.gDrctWgt.hw_dmT_hiDrctFlt_coeff[1];
    attr->stManual.hw_dmT_hiDrctFlt_coeff[2] = attr->stAuto.sta.gDrctWgt.hw_dmT_hiDrctFlt_coeff[2];
    attr->stManual.hw_dmT_hiDrctFlt_coeff[3] = attr->stAuto.sta.gDrctWgt.hw_dmT_hiDrctFlt_coeff[3];

    attr->stManual.hw_dmT_loDrctFlt_coeff[0] = attr->stAuto.sta.gDrctWgt.hw_dmT_loDrctFlt_coeff[0];
    attr->stManual.hw_dmT_loDrctFlt_coeff[1] = attr->stAuto.sta.gDrctWgt.hw_dmT_loDrctFlt_coeff[1];
    attr->stManual.hw_dmT_loDrctFlt_coeff[2] = attr->stAuto.sta.gDrctWgt.hw_dmT_loDrctFlt_coeff[2];
    attr->stManual.hw_dmT_loDrctFlt_coeff[3] = attr->stAuto.sta.gDrctWgt.hw_dmT_loDrctFlt_coeff[3];

    attr->stManual.sw_dmT_luma_val[0] = attr->stAuto.sta.gDrctWgt.sw_dmT_luma_val[0];
    attr->stManual.sw_dmT_luma_val[1] = attr->stAuto.sta.gDrctWgt.sw_dmT_luma_val[1];
    attr->stManual.sw_dmT_luma_val[2] = attr->stAuto.sta.gDrctWgt.sw_dmT_luma_val[2];
    attr->stManual.sw_dmT_luma_val[3] = attr->stAuto.sta.gDrctWgt.sw_dmT_luma_val[3];
    attr->stManual.sw_dmT_luma_val[4] = attr->stAuto.sta.gDrctWgt.sw_dmT_luma_val[4];
    attr->stManual.sw_dmT_luma_val[5] = attr->stAuto.sta.gDrctWgt.sw_dmT_luma_val[5];
    attr->stManual.sw_dmT_luma_val[6] = attr->stAuto.sta.gDrctWgt.sw_dmT_luma_val[6];
    attr->stManual.sw_dmT_luma_val[7] = attr->stAuto.sta.gDrctWgt.sw_dmT_luma_val[7];

    attr->stManual.hw_dmT_cnrAlphaLpf_coeff[0] = attr->stAuto.sta.cFlt.hw_dmT_cnrAlphaLpf_coeff[0];
    attr->stManual.hw_dmT_cnrAlphaLpf_coeff[1] = attr->stAuto.sta.cFlt.hw_dmT_cnrAlphaLpf_coeff[1];
    attr->stManual.hw_dmT_cnrAlphaLpf_coeff[2] = attr->stAuto.sta.cFlt.hw_dmT_cnrAlphaLpf_coeff[2];

    attr->stManual.hw_dmT_cnrLoGuideLpf_coeff[0] = attr->stAuto.sta.cFlt.hw_dmT_cnrLoGuideLpf_coeff[0];
    attr->stManual.hw_dmT_cnrLoGuideLpf_coeff[1] = attr->stAuto.sta.cFlt.hw_dmT_cnrLoGuideLpf_coeff[1];
    attr->stManual.hw_dmT_cnrLoGuideLpf_coeff[2] = attr->stAuto.sta.cFlt.hw_dmT_cnrLoGuideLpf_coeff[2];

    attr->stManual.hw_dmT_cnrPreFlt_coeff[0] = attr->stAuto.sta.cFlt.hw_dmT_cnrPreFlt_coeff[0];
    attr->stManual.hw_dmT_cnrPreFlt_coeff[1] = attr->stAuto.sta.cFlt.hw_dmT_cnrPreFlt_coeff[1];
    attr->stManual.hw_dmT_cnrPreFlt_coeff[2] = attr->stAuto.sta.cFlt.hw_dmT_cnrPreFlt_coeff[2];


    //1) dynamic

    //g_interp

    attr->stManual.hw_dmT_gInterpClip_en = ROUND_F(INTERP_SAMPLE(attr->stAuto.dyn.gInterp.hw_dmT_gInterpClip_en[iso_low_index], attr->stAuto.dyn.gInterp.hw_dmT_gInterpClip_en[iso_high_index], ratio));
    attr->stManual.hw_dmT_gInterpSharpStrg_maxLimit = ROUND_F(INTERP_SAMPLE(attr->stAuto.dyn.gInterp.hw_dmT_gInterpSharpStrg_maxLim[iso_low_index], attr->stAuto.dyn.gInterp.hw_dmT_gInterpSharpStrg_maxLim[iso_high_index], ratio));
    attr->stManual.hw_dmT_gInterpSharpStrg_offset = ROUND_F(INTERP_SAMPLE(attr->stAuto.dyn.gInterp.hw_dmT_gInterpSharpStrg_offset[iso_low_index], attr->stAuto.dyn.gInterp.hw_dmT_gInterpSharpStrg_offset[iso_high_index], ratio));
    attr->stManual.sw_dmT_gInterpWgtFlt_alpha = INTERP_SAMPLE(attr->stAuto.dyn.gInterp.sw_dmT_gInterpWgtFlt_alpha[iso_low_index], attr->stAuto.dyn.gInterp.sw_dmT_gInterpWgtFlt_alpha[iso_high_index], ratio);

    //g_drctwgt

    attr->stManual.hw_dmT_loDrct_thred = ROUND_F(INTERP_SAMPLE(attr->stAuto.dyn.gDrctWgt.hw_dmT_loDrct_thred[iso_low_index], attr->stAuto.dyn.gDrctWgt.hw_dmT_loDrct_thred[iso_high_index], ratio));
    attr->stManual.hw_dmT_hiDrct_thred = ROUND_F(INTERP_SAMPLE(attr->stAuto.dyn.gDrctWgt.hw_dmT_hiDrct_thred[iso_low_index], attr->stAuto.dyn.gDrctWgt.hw_dmT_hiDrct_thred[iso_high_index], ratio));

    attr->stManual.hw_dmT_hiTexture_thred = ROUND_F(INTERP_SAMPLE(attr->stAuto.dyn.gDrctWgt.hw_dmT_hiTexture_thred[iso_low_index], attr->stAuto.dyn.gDrctWgt.hw_dmT_hiTexture_thred[iso_high_index], ratio));
    attr->stManual.hw_dmT_drctMethod_thred = ROUND_F(INTERP_SAMPLE(attr->stAuto.dyn.gDrctWgt.hw_dmT_drctMethod_thred[iso_low_index], attr->stAuto.dyn.gDrctWgt.hw_dmT_drctMethod_thred[iso_high_index], ratio));
    attr->stManual.sw_dmT_gradLoFlt_alpha = INTERP_SAMPLE(attr->stAuto.dyn.gDrctWgt.sw_dmT_gradLoFlt_alpha[iso_low_index], attr->stAuto.dyn.gDrctWgt.sw_dmT_gradLoFlt_alpha[iso_high_index], ratio);

    attr->stManual.sw_dmT_drct_offset[0] = ROUND_F(INTERP_SAMPLE(attr->stAuto.dyn.gDrctWgt.sw_dmT_drct_offset0[iso_low_index], attr->stAuto.dyn.gDrctWgt.sw_dmT_drct_offset0[iso_high_index], ratio));
    attr->stManual.sw_dmT_drct_offset[1] = ROUND_F(INTERP_SAMPLE(attr->stAuto.dyn.gDrctWgt.sw_dmT_drct_offset1[iso_low_index], attr->stAuto.dyn.gDrctWgt.sw_dmT_drct_offset1[iso_high_index], ratio));
    attr->stManual.sw_dmT_drct_offset[2] = ROUND_F(INTERP_SAMPLE(attr->stAuto.dyn.gDrctWgt.sw_dmT_drct_offset2[iso_low_index], attr->stAuto.dyn.gDrctWgt.sw_dmT_drct_offset2[iso_high_index], ratio));
    attr->stManual.sw_dmT_drct_offset[3] = ROUND_F(INTERP_SAMPLE(attr->stAuto.dyn.gDrctWgt.sw_dmT_drct_offset3[iso_low_index], attr->stAuto.dyn.gDrctWgt.sw_dmT_drct_offset3[iso_high_index], ratio));
    attr->stManual.sw_dmT_drct_offset[4] = ROUND_F(INTERP_SAMPLE(attr->stAuto.dyn.gDrctWgt.sw_dmT_drct_offset4[iso_low_index], attr->stAuto.dyn.gDrctWgt.sw_dmT_drct_offset4[iso_high_index], ratio));
    attr->stManual.sw_dmT_drct_offset[5] = ROUND_F(INTERP_SAMPLE(attr->stAuto.dyn.gDrctWgt.sw_dmT_drct_offset5[iso_low_index], attr->stAuto.dyn.gDrctWgt.sw_dmT_drct_offset5[iso_high_index], ratio));
    attr->stManual.sw_dmT_drct_offset[6] = ROUND_F(INTERP_SAMPLE(attr->stAuto.dyn.gDrctWgt.sw_dmT_drct_offset6[iso_low_index], attr->stAuto.dyn.gDrctWgt.sw_dmT_drct_offset6[iso_high_index], ratio));
    attr->stManual.sw_dmT_drct_offset[7] = ROUND_F(INTERP_SAMPLE(attr->stAuto.dyn.gDrctWgt.sw_dmT_drct_offset7[iso_low_index], attr->stAuto.dyn.gDrctWgt.sw_dmT_drct_offset7[iso_high_index], ratio));

    //g_filter

    attr->stManual.hw_dmT_gOutlsFlt_en = ROUND_F(INTERP_SAMPLE(attr->stAuto.dyn.gOutlsFlt.hw_dmT_gOutlsFlt_en[iso_low_index], attr->stAuto.dyn.gOutlsFlt.hw_dmT_gOutlsFlt_en[iso_high_index], ratio));
    attr->stManual.hw_dmT_gOutlsFlt_mode = ROUND_F(INTERP_SAMPLE(attr->stAuto.dyn.gOutlsFlt.hw_dmT_gOutlsFlt_mode[iso_low_index], attr->stAuto.dyn.gOutlsFlt.hw_dmT_gOutlsFlt_mode[iso_high_index], ratio));
    attr->stManual.hw_dmT_gOutlsFltRange_offset = ROUND_F(INTERP_SAMPLE(attr->stAuto.dyn.gOutlsFlt.hw_dmT_gOutlsFltRange_offset[iso_low_index], attr->stAuto.dyn.gOutlsFlt.hw_dmT_gOutlsFltRange_offset[iso_high_index], ratio));

    attr->stManual.sw_dmT_gOutlsFltRsigma_en = ROUND_F(INTERP_SAMPLE(attr->stAuto.dyn.gOutlsFlt.sw_dmT_gOutlsFltRsigma_en[iso_low_index], attr->stAuto.dyn.gOutlsFlt.sw_dmT_gOutlsFltRsigma_en[iso_high_index], ratio));
    attr->stManual.sw_dmT_gOutlsFlt_rsigma = INTERP_SAMPLE(attr->stAuto.dyn.gOutlsFlt.sw_dmT_gOutlsFlt_rsigma[iso_low_index], attr->stAuto.dyn.gOutlsFlt.sw_dmT_gOutlsFlt_rsigma[iso_high_index], ratio);
    attr->stManual.sw_dmT_gOutlsFlt_ratio = INTERP_SAMPLE(attr->stAuto.dyn.gOutlsFlt.sw_dmT_gOutlsFlt_ratio[iso_low_index], attr->stAuto.dyn.gOutlsFlt.sw_dmT_gOutlsFlt_ratio[iso_high_index], ratio);

    attr->stManual.sw_dmT_gOutlsFlt_vsigma[0] = INTERP_SAMPLE(attr->stAuto.dyn.gOutlsFlt.sw_dmT_gOutlsFlt_vsigma0[iso_low_index], attr->stAuto.dyn.gOutlsFlt.sw_dmT_gOutlsFlt_vsigma0[iso_high_index], ratio);
    attr->stManual.sw_dmT_gOutlsFlt_vsigma[1] = INTERP_SAMPLE(attr->stAuto.dyn.gOutlsFlt.sw_dmT_gOutlsFlt_vsigma1[iso_low_index], attr->stAuto.dyn.gOutlsFlt.sw_dmT_gOutlsFlt_vsigma1[iso_high_index], ratio);
    attr->stManual.sw_dmT_gOutlsFlt_vsigma[2] = INTERP_SAMPLE(attr->stAuto.dyn.gOutlsFlt.sw_dmT_gOutlsFlt_vsigma2[iso_low_index], attr->stAuto.dyn.gOutlsFlt.sw_dmT_gOutlsFlt_vsigma2[iso_high_index], ratio);
    attr->stManual.sw_dmT_gOutlsFlt_vsigma[3] = INTERP_SAMPLE(attr->stAuto.dyn.gOutlsFlt.sw_dmT_gOutlsFlt_vsigma3[iso_low_index], attr->stAuto.dyn.gOutlsFlt.sw_dmT_gOutlsFlt_vsigma3[iso_high_index], ratio);
    attr->stManual.sw_dmT_gOutlsFlt_vsigma[4] = INTERP_SAMPLE(attr->stAuto.dyn.gOutlsFlt.sw_dmT_gOutlsFlt_vsigma4[iso_low_index], attr->stAuto.dyn.gOutlsFlt.sw_dmT_gOutlsFlt_vsigma4[iso_high_index], ratio);
    attr->stManual.sw_dmT_gOutlsFlt_vsigma[5] = INTERP_SAMPLE(attr->stAuto.dyn.gOutlsFlt.sw_dmT_gOutlsFlt_vsigma5[iso_low_index], attr->stAuto.dyn.gOutlsFlt.sw_dmT_gOutlsFlt_vsigma5[iso_high_index], ratio);
    attr->stManual.sw_dmT_gOutlsFlt_vsigma[6] = INTERP_SAMPLE(attr->stAuto.dyn.gOutlsFlt.sw_dmT_gOutlsFlt_vsigma6[iso_low_index], attr->stAuto.dyn.gOutlsFlt.sw_dmT_gOutlsFlt_vsigma6[iso_high_index], ratio);
    attr->stManual.sw_dmT_gOutlsFlt_vsigma[7] = INTERP_SAMPLE(attr->stAuto.dyn.gOutlsFlt.sw_dmT_gOutlsFlt_vsigma7[iso_low_index], attr->stAuto.dyn.gOutlsFlt.sw_dmT_gOutlsFlt_vsigma7[iso_high_index], ratio);

    attr->stManual.sw_dmT_gOutlsFlt_coeff[0] = ROUND_F(INTERP_SAMPLE(attr->stAuto.dyn.gOutlsFlt.sw_dmT_gOutlsFlt_coeff0[iso_low_index], attr->stAuto.dyn.gOutlsFlt.sw_dmT_gOutlsFlt_coeff0[iso_high_index], ratio));
    attr->stManual.sw_dmT_gOutlsFlt_coeff[1] = ROUND_F(INTERP_SAMPLE(attr->stAuto.dyn.gOutlsFlt.sw_dmT_gOutlsFlt_coeff1[iso_low_index], attr->stAuto.dyn.gOutlsFlt.sw_dmT_gOutlsFlt_coeff1[iso_high_index], ratio));
    attr->stManual.sw_dmT_gOutlsFlt_coeff[2] = ROUND_F(INTERP_SAMPLE(attr->stAuto.dyn.gOutlsFlt.sw_dmT_gOutlsFlt_coeff2[iso_low_index], attr->stAuto.dyn.gOutlsFlt.sw_dmT_gOutlsFlt_coeff2[iso_high_index], ratio));

    //c_filter

    attr->stManual.hw_dmT_cnrFlt_en = ROUND_F(INTERP_SAMPLE(attr->stAuto.dyn.cFlt.hw_dmT_cnrFlt_en[iso_low_index], attr->stAuto.dyn.cFlt.hw_dmT_cnrFlt_en[iso_high_index], ratio));
    attr->stManual.hw_dmT_cnrEdgeAlpha_offset = ROUND_F(INTERP_SAMPLE(attr->stAuto.dyn.cFlt.hw_dmT_cnrEdgeAlpha_offset[iso_low_index], attr->stAuto.dyn.cFlt.hw_dmT_cnrEdgeAlpha_offset[iso_high_index], ratio));
    attr->stManual.hw_dmT_cnrMoireAlpha_offset = ROUND_F(INTERP_SAMPLE(attr->stAuto.dyn.cFlt.hw_dmT_cnrMoireAlpha_offset[iso_low_index], attr->stAuto.dyn.cFlt.hw_dmT_cnrMoireAlpha_offset[iso_high_index], ratio));
    attr->stManual.hw_dmT_cnrLogGrad_offset = ROUND_F(INTERP_SAMPLE(attr->stAuto.dyn.cFlt.hw_dmT_cnrLogGrad_offset[iso_low_index], attr->stAuto.dyn.cFlt.hw_dmT_cnrLogGrad_offset[iso_high_index], ratio));
    attr->stManual.hw_dmT_cnrLogGuide_offset = ROUND_F(INTERP_SAMPLE(attr->stAuto.dyn.cFlt.hw_dmT_cnrLogGuide_offset[iso_low_index], attr->stAuto.dyn.cFlt.hw_dmT_cnrLogGuide_offset[iso_high_index], ratio));
    attr->stManual.hw_dmT_cnrHiFltCur_wgt = INTERP_SAMPLE(attr->stAuto.dyn.cFlt.hw_dmT_cnrHiFltCur_wgt[iso_low_index], attr->stAuto.dyn.cFlt.hw_dmT_cnrHiFltCur_wgt[iso_high_index], ratio);
    attr->stManual.hw_dmT_cnrHiFltWgt_minLimit = INTERP_SAMPLE(attr->stAuto.dyn.cFlt.hw_dmT_cnrHiFltWgt_minLimit[iso_low_index], attr->stAuto.dyn.cFlt.hw_dmT_cnrHiFltWgt_minLimit[iso_high_index], ratio);

    attr->stManual.sw_dmT_cnrEdgeAlpha_scale = INTERP_SAMPLE(attr->stAuto.dyn.cFlt.sw_dmT_cnrEdgeAlpha_scale[iso_low_index], attr->stAuto.dyn.cFlt.sw_dmT_cnrEdgeAlpha_scale[iso_high_index], ratio);
    attr->stManual.sw_dmT_cnrMoireAlpha_scale = INTERP_SAMPLE(attr->stAuto.dyn.cFlt.sw_dmT_cnrMoireAlpha_scale[iso_low_index], attr->stAuto.dyn.cFlt.sw_dmT_cnrMoireAlpha_scale[iso_high_index], ratio);
    attr->stManual.sw_dmT_cnrLoFltWgt_maxLimit = INTERP_SAMPLE(attr->stAuto.dyn.cFlt.sw_dmT_cnrLoFltWgt_maxLimit[iso_low_index], attr->stAuto.dyn.cFlt.sw_dmT_cnrLoFltWgt_maxLimit[iso_high_index], ratio);
    attr->stManual.sw_dmT_cnrLoFltWgt_minThred = INTERP_SAMPLE(attr->stAuto.dyn.cFlt.sw_dmT_cnrLoFltWgt_minThred[iso_low_index], attr->stAuto.dyn.cFlt.sw_dmT_cnrLoFltWgt_minThred[iso_high_index], ratio);
    attr->stManual.sw_dmT_cnrLoFltWgt_slope = INTERP_SAMPLE(attr->stAuto.dyn.cFlt.sw_dmT_cnrLoFltWgt_slope[iso_low_index], attr->stAuto.dyn.cFlt.sw_dmT_cnrLoFltWgt_slope[iso_high_index], ratio);
    attr->stManual.sw_dmT_cnrLoFlt_vsigma = INTERP_SAMPLE(attr->stAuto.dyn.cFlt.sw_dmT_cnrLoFlt_vsigma[iso_low_index], attr->stAuto.dyn.cFlt.sw_dmT_cnrLoFlt_vsigma[iso_high_index], ratio);
    attr->stManual.sw_dmT_cnrHiFlt_vsigma = INTERP_SAMPLE(attr->stAuto.dyn.cFlt.sw_dmT_cnrHiFlt_vsigma[iso_low_index], attr->stAuto.dyn.cFlt.sw_dmT_cnrHiFlt_vsigma[iso_high_index], ratio);

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn sample_adebayer_setManualAtrrib_v3(const rk_aiq_sys_ctx_t* ctx, int32_t ISO)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    if (ctx == NULL) {
        ret = XCAM_RETURN_ERROR_PARAM;
        RKAIQ_SAMPLE_CHECK_RET(ret, "param error!");
    }

    adebayer_v3_attrib_t attr;
    ret = rk_aiq_user_api2_adebayer_v3_GetAttrib(ctx, &attr);
    RKAIQ_SAMPLE_CHECK_RET(ret, "get debayer v3 attrib failed!");
    attr.mode = RK_AIQ_DEBAYER_MODE_MANUAL;
    sample_adebayer_translate_params_v3(&attr, ISO);
    rk_aiq_user_api2_adebayer_v3_SetAttrib(ctx, attr);

    printf ("mode: %d, sync_mode: %d, done: %d\n", attr.mode, attr.sync.sync_mode, attr.sync.done);

    return ret;
}

XCamReturn sample_adebayer_setAutoAtrrib_v3(const rk_aiq_sys_ctx_t* ctx)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    if (ctx == NULL) {
        ret = XCAM_RETURN_ERROR_PARAM;
        RKAIQ_SAMPLE_CHECK_RET(ret, "param error!");
    }

    adebayer_v3_attrib_t attr;
    ret = rk_aiq_user_api2_adebayer_v3_GetAttrib(ctx, &attr);
    RKAIQ_SAMPLE_CHECK_RET(ret, "get debayer v3 attrib failed!");
    attr.mode = RK_AIQ_DEBAYER_MODE_AUTO;

    attr.stAuto.dyn.gOutlsFlt.sw_dmT_gOutlsFltRsigma_en[0] = 1;
    attr.stAuto.dyn.gOutlsFlt.sw_dmT_gOutlsFltRsigma_en[1] = 1;
    attr.stAuto.dyn.cFlt.sw_dmT_cnrMoireAlpha_scale[0] = 10;
    attr.stAuto.dyn.cFlt.sw_dmT_cnrMoireAlpha_scale[1] = 10;

    rk_aiq_user_api2_adebayer_v3_SetAttrib(ctx, attr);

    printf ("mode: %d, sync_mode: %d, done: %d\n", attr.mode, attr.sync.sync_mode, attr.sync.done);

    return ret;
}

#ifdef USE_NEWSTRUCT
static void sample_dm_tuningtool_test(const rk_aiq_sys_ctx_t* ctx)
{
    char *ret_str = NULL;

    printf(">>> start tuning tool test: op attrib get ...\n");

    const char* json_dm_status_str = " \n\
        [{ \n\
            \"op\":\"get\", \n\
            \"path\": \"/uapi/0/dm_uapi/info\", \n\
            \"value\": \n\
            { \"opMode\": \"RK_AIQ_OP_MODE_MANUAL\", \"en\": 0,\"bypass\": 3} \n\
        }]";

    rkaiq_uapi_unified_ctl((rk_aiq_sys_ctx_t*)(ctx),
                           json_dm_status_str, &ret_str, RKAIQUAPI_OPMODE_GET);

    if (ret_str) {
        printf("dm status json str: %s\n", ret_str);
    }

    printf("  start tuning tool test: op attrib set ...\n");
    const char* json_dm_str = " \n\
        [{ \n\
            \"op\":\"replace\", \n\
            \"path\": \"/uapi/0/dm_uapi/attr\", \n\
            \"value\": \n\
            { \"opMode\": \"RK_AIQ_OP_MODE_MANUAL\", \"en\": 1,\"bypass\": 1} \n\
        }]";
    printf("dm json_cmd_str: %s\n", json_dm_str);
    ret_str = NULL;
    rkaiq_uapi_unified_ctl((rk_aiq_sys_ctx_t*)(ctx),
                           json_dm_str, &ret_str, RKAIQUAPI_OPMODE_SET);

    // wait more than 2 frames
    usleep(90 * 1000);

    dm_status_t status;
    memset(&status, 0, sizeof(dm_status_t));

    rk_aiq_user_api2_dm_QueryStatus(ctx, &status);

    if (status.opMode != RK_AIQ_OP_MODE_MANUAL || status.en != 1 || status.bypass != 1) {
        printf("dm op set_attrib failed !\n");
        printf("dm status: opmode:%d(EXP:%d), en:%d(EXP:%d), bypass:%d(EXP:%d)\n",
               status.opMode, RK_AIQ_OP_MODE_MANUAL, status.en, 1, status.bypass, 1);
    } else {
        printf("dm op set_attrib success !\n");
    }

    printf(">>> tuning tool test done \n");
}

static void get_auto_attr(dm_api_attrib_t* attr) {
    dm_param_auto_t* stAuto = &attr->stAuto;
    for (int i = 0;i < 13;i++) {
    }
}

static void get_manual_attr(dm_api_attrib_t* attr) {
    dm_param_t* stMan = &attr->stMan;
}

void sample_dm_test(const rk_aiq_sys_ctx_t* ctx)
{
    // get cur mode
    printf("+++++++ DM module test start ++++++++\n");

    // sample_dm_tuningtool_test(ctx);

    dm_api_attrib_t attr;
    memset(&attr, 0, sizeof(attr));

    rk_aiq_user_api2_dm_GetAttrib(ctx, &attr);

    printf("dm attr: opmode:%d, en:%d, bypass:%d\n", attr.opMode, attr.en, attr.bypass);

    srand(time(0));
    int rand_num = rand() % 101;

    if (rand_num <70) {
        printf("update dm arrrib!\n");
        if (attr.opMode == RK_AIQ_OP_MODE_AUTO) {
            attr.opMode = RK_AIQ_OP_MODE_MANUAL;
            get_manual_attr(&attr);
        }
        else {
            get_auto_attr(&attr);
            attr.opMode = RK_AIQ_OP_MODE_AUTO;
        }
    }
    else {
        // reverse en
        printf("reverse dm en!\n");
        attr.en = !attr.en;
    }

    rk_aiq_user_api2_dm_SetAttrib(ctx, &attr);

    // wait more than 2 frames
    usleep(90 * 1000);

    dm_status_t status;
    memset(&status, 0, sizeof(dm_status_t));

    rk_aiq_user_api2_dm_QueryStatus(ctx, &status);

    printf("dm status: opmode:%d, en:%d, bypass:%d\n", status.opMode, status.en, status.bypass);

    if (status.opMode != attr.opMode || status.en != attr.en)
        printf("dm arrib api test failed\n");
    else
        printf("dm arrib api test success\n");

    printf("-------- DM module test done --------\n");
}
#endif

XCamReturn sample_adebayer_module (const void *arg)
{
    int key = -1;
    CLEAR();

    const demo_context_t *demo_ctx = (demo_context_t *)arg;
    const rk_aiq_sys_ctx_t* ctx;
    if (demo_ctx->camGroup) {
        ctx = (rk_aiq_sys_ctx_t*)(demo_ctx->camgroup_ctx);
    } else {
        ctx = (rk_aiq_sys_ctx_t*)(demo_ctx->aiq_ctx);
    }

    if (ctx == NULL) {
        ERR ("%s, ctx is nullptr\n", __FUNCTION__);
        return XCAM_RETURN_ERROR_PARAM;
    }

    sample_adebayer_usage ();
    do {

        key = getchar ();
        while (key == '\n' || key == '\r')
            key = getchar ();
        printf ("\n");

        switch (key)
        {
        case 'h':
            CLEAR();
            sample_adebayer_usage ();
            break;
        case '0': {
            static bool on = false;
            on = !on;
            sample_adebayer_en_v1(ctx, RK_AIQ_DEBAYER_MODE_AUTO, on);
            printf("%s adebayer\n\n", on ? "enable" : "disable");
            break;
        }
        case '1': {
            unsigned char sharp_strength[9] = {250, 250, 250, 250, 250, 250, 250, 250, 250};
            sample_adebayer_setSharpStrength_v1(ctx, sharp_strength);
            printf("test the sharp_strength of 255 in sync mode...\n");
            break;
        }
        case '2': {
            unsigned char sharp_strength[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
            sample_adebayer_setSharpStrength_v1(ctx, sharp_strength);
            printf("test the sharp_strength of 0 in sync mode...\n");
            break;
        }
        case '3':
            sample_adebayer_setHighFreqThresh_v1(ctx, 250);
            printf("test the high freq thresh of 250 in sync mode...\n");
            break;
        case '4':
            sample_adebayer_setHighFreqThresh_v1(ctx, 0);
            printf("test the high freq thresh of 0 in sync mode...\n");
            break;
        case '5':
            sample_adebayer_setLowFreqThresh_v1(ctx, 250);
            printf("test the low freq thresh of 250 in sync mode...\n");
            break;
        case '6':
            sample_adebayer_setLowFreqThresh_v1(ctx, 0);
            printf("test the low freq thresh of 0 in sync mode...\n");
            break;
        case '7':
            {
            static bool on = false;
            on = !on;
            sample_adebayer_en_v1(ctx, RK_AIQ_DEBAYER_MODE_MANUAL, on);
            printf("%s adebayer\n\n", on ? "enable" : "disable");
            }
            break;
        case '8':
            sample_adebayer_setManualAtrrib_v1(ctx, 50);
            printf("set manual params from json with MANUAL mode in sync, ISO: 50\n");
            break;
        case '9':
            sample_adebayer_setManualAtrrib_v1(ctx, 6400);
            printf("set manual params from json with MANUAL mode in sync, ISO: 6400\n");
            break;
        case 'a':
            sample_adebayer_setManualAtrrib_v2(ctx, 900);
            printf("set manual params from json with MANUAL mode in sync, ISO: 900\n");
            break;
        case 'b':
            sample_adebayer_setManualAtrrib_v2(ctx, 2800);
            printf("set manual params from json with MANUAL mode in sync, ISO: 2800\n");
            break;
        case 'c':
            // TODO:
            sample_adebayer_setAutoAtrrib_v2(ctx);
            printf("set manual params from json with Auto mode in sync,set cfilter off\n");
            break;
        case 'd':
            sample_adebayer_setManualAtrrib_v2lite(ctx, 900);
            printf("set manual params from json with MANUAL mode in sync, ISO: 900\n");
            break;
        case 'e':
            sample_adebayer_setManualAtrrib_v2lite(ctx, 2800);
            printf("set manual params from json with MANUAL mode in sync, ISO: 2800\n");
            break;
        case 'f':
            // TODO:
            sample_adebayer_setAutoAtrrib_v2lite(ctx);
            printf("set manual params from json with Auto mode in sync,set cfilter off\n");
            break;
        case 'g':
            sample_adebayer_setManualAtrrib_v3(ctx, 900);
            printf("set manual params from json with MANUAL mode in sync, ISO: 900\n");
            break;
        case 'i':
            sample_adebayer_setManualAtrrib_v3(ctx, 2800);
            printf("set manual params from json with MANUAL mode in sync, ISO: 2800\n");
            break;
        case 'j':
            sample_adebayer_setAutoAtrrib_v3(ctx);
            printf("set auto params from json with Auto mode in sync\n");
            break;
#ifdef USE_NEWSTRUCT
        case 'k':
            sample_dm_test(ctx);
            break;
#endif
        default:
            break;
        }
    } while (key != 'q' && key != 'Q');

    return XCAM_RETURN_NO_ERROR;
}

