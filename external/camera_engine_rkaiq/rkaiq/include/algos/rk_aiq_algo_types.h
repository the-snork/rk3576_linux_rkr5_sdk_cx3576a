/*
 * Copyright (c) 2019-2022 Rockchip Eletronics Co., Ltd.
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
 */
#ifndef _RK_AIQ_ALGO_TYPES_H_
#define _RK_AIQ_ALGO_TYPES_H_

#include "algos/a3dlut/rk_aiq_types_a3dlut_algo_int.h"
#include "algos/abayer2dnr2/rk_aiq_types_abayer2dnr_algo_int_v2.h"
#include "algos/abayer2dnrV23/rk_aiq_types_abayer2dnr_algo_int_v23.h"
#include "algos/abayertnr2/rk_aiq_types_abayertnr_algo_int_v2.h"
#include "algos/abayertnrV23/rk_aiq_types_abayertnr_algo_int_v23.h"
#include "algos/abayertnrV30/rk_aiq_types_abayertnr_algo_int_v30.h"
#include "algos/ablc/rk_aiq_types_ablc_algo_int.h"
#include "algos/ablcV32/rk_aiq_types_ablc_algo_int_v32.h"
#include "algos/accm/rk_aiq_types_accm_algo_int.h"
#include "algos/acnr/rk_aiq_types_acnr_algo_int_v1.h"
#include "algos/acnr2/rk_aiq_types_acnr_algo_int_v2.h"
#include "algos/acnrV30/rk_aiq_types_acnr_algo_int_v30.h"
#include "algos/acnrV31/rk_aiq_types_acnr_algo_int_v31.h"
#include "algos/acp/rk_aiq_types_acp_algo_int.h"
#include "algos/adebayer/rk_aiq_types_adebayer_algo_int.h"
#include "algos/adegamma/rk_aiq_types_adegamma_algo_int.h"
#include "algos/adehaze/rk_aiq_types_adehaze_algo_int.h"
#include "algos/adpcc/rk_aiq_types_adpcc_algo_int.h"
#include "algos/adrc/rk_aiq_types_adrc_algo_int.h"
#include "algos/ae/rk_aiq_types_ae_algo_int.h"
#include "algos/af/rk_aiq_types_af_algo_int.h"
#include "algos/afd/rk_aiq_types_afd_algo_int.h"
#include "algos/afec/rk_aiq_types_afec_algo_int.h"
#include "algos/again/rk_aiq_types_again_algo_int.h"
#include "algos/again2/rk_aiq_types_again_algo_int_v2.h"
#include "algos/agamma/rk_aiq_types_agamma_algo_int.h"
#include "algos/aie/rk_aiq_types_aie_algo_int.h"
#include "algos/aldc/rk_aiq_types_aldc_algo_int.h"
#include "algos/aldch/rk_aiq_types_aldch_algo_int.h"
#include "algos/alsc/rk_aiq_types_alsc_algo_int.h"
#include "algos/amd/rk_aiq_types_algo_amd_int.h"
#include "algos/amerge/rk_aiq_types_amerge_algo_int.h"
#include "algos/amfnr/rk_aiq_types_amfnr_algo_int_v1.h"
#include "algos/anr/rk_aiq_types_anr_algo_int.h"
#include "algos/aorb/rk_aiq_types_orb_algo.h"
#include "algos/arawnr/rk_aiq_types_abayernr_algo_int_v1.h"
#include "algos/arawnr2/rk_aiq_types_abayernr_algo_int_v2.h"
#include "algos/argbir/rk_aiq_types_argbir_algo_int.h"
#include "algos/asharp/rk_aiq_types_asharp_algo_int.h"
#include "algos/asharp3/rk_aiq_types_asharp_algo_int_v3.h"
#include "algos/asharp4/rk_aiq_types_asharp_algo_int_v4.h"
#include "algos/asharpV33/rk_aiq_types_asharp_algo_int_v33.h"
#include "algos/asharpV34/rk_aiq_types_asharp_algo_int_v34.h"
#include "algos/atmo/rk_aiq_types_atmo_algo_int.h"
#include "algos/auvnr/rk_aiq_types_auvnr_algo_int_v1.h"
#include "algos/awb/rk_aiq_types_awb_algo_int.h"
#include "algos/aynr/rk_aiq_types_aynr_algo_int_v1.h"
#include "algos/aynr2/rk_aiq_types_aynr_algo_int_v2.h"
#include "algos/aynr3/rk_aiq_types_aynr_algo_int_v3.h"
#include "algos/aynrV22/rk_aiq_types_aynr_algo_int_v22.h"
#include "algos/aynrV24/rk_aiq_types_aynr_algo_int_v24.h"
#include "algos/ayuvmeV1/rk_aiq_types_ayuvme_algo_int_v1.h"
#include "eis_head.h"
#include "orb_head.h"
#include "algos/rk_aiq_algo_des.h"
#include "common/rk_aiq_types.h"

#include "algos/algo_types_priv.h"

// anr
typedef struct _RkAiqAlgoConfigAnr {
    RkAiqAlgoCom com;
    ANRConfig_t stANRConfig;
} RkAiqAlgoConfigAnr ;

typedef struct _RkAiqAlgoPreAnr {
    RkAiqAlgoCom com;
    ANRConfig_t stANRConfig;
} RkAiqAlgoPreAnr;

typedef struct _RkAiqAlgoPreResAnr {
    RkAiqAlgoResCom res_com;
} RkAiqAlgoPreResAnr;

typedef struct _RkAiqAlgoProcAnr {
    RkAiqAlgoCom com;
    int iso;
    int hdr_mode;
} RkAiqAlgoProcAnr;

typedef struct _RkAiqAlgoProcResAnr {
    RkAiqAlgoResCom res_com;
    ANRProcResult_t stAnrProcResult;
} RkAiqAlgoProcResAnr;

typedef struct _RkAiqAlgoPostAnr {
    RkAiqAlgoCom com;
} RkAiqAlgoPostAnr;

typedef struct _RkAiqAlgoPostResAnr {
    RkAiqAlgoResCom res_com;
} RkAiqAlgoPostResAnr;

// asharp
typedef struct _RkAiqAlgoConfigAsharp {
    RkAiqAlgoCom com;
    AsharpConfig_t stAsharpConfig;
} RkAiqAlgoConfigAsharp ;

typedef struct _RkAiqAlgoPreAsharp {
    RkAiqAlgoCom com;
} RkAiqAlgoPreAsharp;

typedef struct _RkAiqAlgoPreResAsharp {
    RkAiqAlgoResCom res_com;
} RkAiqAlgoPreResAsharp;

typedef struct _RkAiqAlgoProcAsharp {
    RkAiqAlgoCom com;
    int iso;
    int hdr_mode;
} RkAiqAlgoProcAsharp;

typedef struct _RkAiqAlgoProcResAsharp {
    RkAiqAlgoResCom res_com;
    AsharpProcResult_t stAsharpProcResult;
} RkAiqAlgoProcResAsharp;

typedef struct _RkAiqAlgoPostAsharp {
    RkAiqAlgoCom com;
} RkAiqAlgoPostAsharp;

typedef struct _RkAiqAlgoPostResAsharp {
    RkAiqAlgoResCom res_com;
} RkAiqAlgoPostResAsharp;

//sharp v3
typedef struct _RkAiqAlgoConfigAsharpV3 {
    RkAiqAlgoCom com;
    Asharp_Config_V3_t stAsharpConfig;
} RkAiqAlgoConfigAsharpV3;

typedef struct _RkAiqAlgoPreAsharpV3 {
    RkAiqAlgoCom com;
    Asharp_Config_V3_t stAsharpConfig;
} RkAiqAlgoPreAsharpV3;

typedef struct _RkAiqAlgoPreResAsharpV3 {
    RkAiqAlgoResCom res_com;
} RkAiqAlgoPreResAsharpV3;

typedef struct _RkAiqAlgoProcAsharpV3 {
    RkAiqAlgoCom com;
    int iso;
    int hdr_mode;
} RkAiqAlgoProcAsharpV3;

typedef struct _RkAiqAlgoProcResAsharpV3 {
    RkAiqAlgoResCom res_com;
    Asharp_ProcResult_V3_t stAsharpProcResult;
} RkAiqAlgoProcResAsharpV3;

typedef struct _RkAiqAlgoPostAsharpV3 {
    RkAiqAlgoCom com;
} RkAiqAlgoPostAsharpV3;

typedef struct _RkAiqAlgoPostResAsharpV3 {
    RkAiqAlgoResCom res_com;
} RkAiqAlgoPostResAsharpV3;

//sharp v4
typedef struct _RkAiqAlgoConfigAsharpV4 {
    RkAiqAlgoCom com;
    Asharp_Config_V4_t stAsharpConfig;
} RkAiqAlgoConfigAsharpV4;

typedef struct _RkAiqAlgoPreAsharpV4 {
    RkAiqAlgoCom com;
    Asharp_Config_V4_t stAsharpConfig;
} RkAiqAlgoPreAsharpV4;

typedef struct _RkAiqAlgoPreResAsharpV4 {
    RkAiqAlgoResCom res_com;
} RkAiqAlgoPreResAsharpV4;

typedef struct _RkAiqAlgoProcAsharpV4 {
    RkAiqAlgoCom com;
    int iso;
    int hdr_mode;
} RkAiqAlgoProcAsharpV4;

typedef struct _RkAiqAlgoProcResAsharpV4 {
    RkAiqAlgoResCom res_com;
    Asharp_ProcResult_V4_t stAsharpProcResult;
} RkAiqAlgoProcResAsharpV4;

typedef struct _RkAiqAlgoPostAsharpV4 {
    RkAiqAlgoCom com;
} RkAiqAlgoPostAsharpV4;

typedef struct _RkAiqAlgoPostResAsharpV4 {
    RkAiqAlgoResCom res_com;
} RkAiqAlgoPostResAsharpV4;

// sharp V33
typedef struct _RkAiqAlgoConfigAsharpV33 {
    RkAiqAlgoCom com;
    Asharp_Config_V33_t stAsharpConfig;
} RkAiqAlgoConfigAsharpV33;

typedef struct _RkAiqAlgoProcAsharpV33 {
    RkAiqAlgoCom com;
    int iso;
    int hdr_mode;
    AblcProc_V32_t* stAblcV32_proc_res;
} RkAiqAlgoProcAsharpV33;

typedef struct _RkAiqAlgoProcResAsharpV33 {
    RkAiqAlgoResCom res_com;
    Asharp_ProcResult_V33_t stAsharpProcResult;
} RkAiqAlgoProcResAsharpV33;

// sharp V34
typedef struct _RkAiqAlgoConfigAsharpV34 {
    RkAiqAlgoCom com;
    Asharp_Config_V34_t stAsharpConfig;
} RkAiqAlgoConfigAsharpV34;

typedef struct _RkAiqAlgoProcAsharpV34 {
    RkAiqAlgoCom com;
    int iso;
    int hdr_mode;
    AblcProc_V32_t* stAblcV32_proc_res;
} RkAiqAlgoProcAsharpV34;

typedef struct _RkAiqAlgoProcResAsharpV34 {
    RkAiqAlgoResCom res_com;
    Asharp_ProcResult_V34_t stAsharpProcResult;
} RkAiqAlgoProcResAsharpV34;

// asd
typedef struct _RkAiqAlgoConfigAsd {
    RkAiqAlgoCom com;
} RkAiqAlgoConfigAsd ;

typedef struct _RkAiqAlgoPreAsd {
    RkAiqAlgoCom com;
    asd_preprocess_in_t pre_params;
} RkAiqAlgoPreAsd;

typedef struct _RkAiqAlgoPreResAsd {
    RkAiqAlgoResCom res_com;
    asd_preprocess_result_t asd_result;
} RkAiqAlgoPreResAsd;

typedef struct _RkAiqAlgoProcAsd {
    RkAiqAlgoCom com;
} RkAiqAlgoProcAsd;

typedef struct _RkAiqAlgoProcResAsd {
    RkAiqAlgoResCom res_com;
} RkAiqAlgoProcResAsd;

// amerge
typedef struct _RkAiqAlgoConfigAmerge {
    RkAiqAlgoCom com;
    int working_mode;
    unsigned short  rawWidth;               // raw图宽
    unsigned short  rawHeight;
} RkAiqAlgoConfigAmerge;

typedef struct _RkAiqAlgoProcAmerge {
    RkAiqAlgoCom com;
    bool LongFrmMode;
} RkAiqAlgoProcAmerge;

typedef struct _RkAiqAlgoProcResAmerge {
    RkAiqAlgoResCom res_com;
    RkAiqAmergeProcResult_t* AmergeProcRes;
} RkAiqAlgoProcResAmerge;

// atmo
typedef struct _RkAiqAlgoConfigAtmo {
    RkAiqAlgoCom com;
    AtmoConfig_t* atmoCfg;
    int working_mode;
    unsigned short  rawWidth;               // raw图宽
    unsigned short  rawHeight;
} RkAiqAlgoConfigAtmo;

typedef struct _RkAiqAlgoPreAtmo {
    RkAiqAlgoCom com;
} RkAiqAlgoPreAtmo;

typedef struct _RkAiqAlgoPreResAtmo {
    RkAiqAlgoResCom res_com;
} RkAiqAlgoPreResAtmo;

typedef struct _RkAiqAlgoProcAtmo {
    RkAiqAlgoCom com;
    rkisp_atmo_stats_t ispAtmoStats;
    uint32_t width;
    uint32_t height;
} RkAiqAlgoProcAtmo;

typedef struct _RkAiqAlgoProcResAtmo {
    RkAiqAlgoResCom res_com;
    RkAiqAtmoProcResult_t AtmoProcRes;
} RkAiqAlgoProcResAtmo;

typedef struct _RkAiqAlgoPostAtmo {
    RkAiqAlgoCom com;
} RkAiqAlgoPostAtmo;

typedef struct _RkAiqAlgoPostResAtmo {
    RkAiqAlgoResCom res_com;
} RkAiqAlgoPostResAtmo;

// acp
typedef struct _RkAiqAlgoConfigAcp {
    RkAiqAlgoCom com;
} RkAiqAlgoConfigAcp;

typedef struct _RkAiqAlgoProcAcp {
    RkAiqAlgoCom com;
} RkAiqAlgoProcAcp;

typedef struct _RkAiqAlgoProcResAcp {
    RkAiqAlgoResCom res_com;
    rk_aiq_acp_params_t* acp_res;
} RkAiqAlgoProcResAcp;

//adehaze
typedef struct _RkAiqAlgoConfigAdhaz {
    RkAiqAlgoCom com;
#ifdef RKAIQ_ENABLE_PARSER_V1
    const CamCalibDbContext_t *calib;
#endif
    int working_mode;
    bool is_multi_isp_mode;
} RkAiqAlgoConfigAdhaz;

typedef struct _RkAiqAlgoProcAdhaz {
    RkAiqAlgoCom com;
    union {
        dehaze_stats_v10_t* dehaze_stats_v10;
        dehaze_stats_v11_t* dehaze_stats_v11;
        dehaze_stats_v11_duo_t* dehaze_stats_v11_duo;
        dehaze_stats_v12_t* dehaze_stats_v12;
        dehaze_stats_v14_t* dehaze_stats_v14;
    };
    bool stats_true;
#ifdef RKAIQ_ENABLE_PARSER_V1
    const CamCalibDbContext_t *pCalibDehaze;
#endif
#if RKAIQ_HAVE_DEHAZE_V11_DUO
    float sigma_v3[YNR_V3_ISO_CURVE_POINT_NUM];
#endif
#if RKAIQ_HAVE_DEHAZE_V12
    AdehazeOBResV12_t OBResV12;
    float sigma_v22[YNR_V22_ISO_CURVE_POINT_NUM];
#endif
#if RKAIQ_HAVE_DEHAZE_V14
    AdehazeOBResV12_t OBResV12;
    float sigma_v24[YNR_V24_ISO_CURVE_POINT_NUM];
#endif
} RkAiqAlgoProcAdhaz;

typedef struct _RkAiqAlgoProcResAdhaz {
    RkAiqAlgoResCom res_com;
    RkAiqAdehazeProcResult_t* AdehzeProcRes;
} RkAiqAlgoProcResAdhaz;

// a3dlut
typedef struct _RkAiqAlgoConfigA3dlut {
    RkAiqAlgoCom com;
} RkAiqAlgoConfigA3dlut;

typedef struct _RkAiqAlgoProcA3dlut {
    RkAiqAlgoCom com;
    float sensorGain;
    float awbGain[2];
    float awbIIRDampCoef;
    bool awbConverged;
} RkAiqAlgoProcA3dlut;

typedef struct _RkAiqAlgoProcResA3dlut {
    RkAiqAlgoResCom res_com;
    rk_aiq_lut3d_cfg_t* lut3d_hw_conf;
} RkAiqAlgoProcResA3dlut;

typedef struct _RkAiqAlgoPreAblc {
    RkAiqAlgoCom com;
} RkAiqAlgoPreAblc;

typedef struct _RkAiqAlgoPreResAblc {
    RkAiqAlgoResCom res_com;
} RkAiqAlgoPreResAblc;

typedef struct _RkAiqAlgoPostAblc {
    RkAiqAlgoCom com;
} RkAiqAlgoPostAblc;

typedef struct _RkAiqAlgoPostResAblc {
    RkAiqAlgoResCom res_com;
} RkAiqAlgoPostResAblc;

// ablcV32
typedef struct _RkAiqAlgoConfigAblcV32 {
    RkAiqAlgoCom com;
} RkAiqAlgoConfigAblcV32;

typedef struct _RkAiqAlgoProcAblcV32 {
    RkAiqAlgoCom com;
    int iso;
    int hdr_mode;
} RkAiqAlgoProcAblcV32;

typedef struct _RkAiqAlgoProcResAblcV32 {
    RkAiqAlgoResCom res_com;
    AblcProc_V32_t* ablcV32_proc_res;
} RkAiqAlgoProcResAblcV32;

// accm
typedef struct _RkAiqAlgoConfigAccm {
    RkAiqAlgoCom com;
} RkAiqAlgoConfigAccm;

typedef struct _RkAiqAlgoProcAccm {
    RkAiqAlgoCom com;
    accm_sw_info_t   accm_sw_info;
} RkAiqAlgoProcAccm;

typedef struct _RkAiqAlgoProcResAccm {
    RkAiqAlgoResCom res_com;
    union {
        rk_aiq_ccm_cfg_t* accm_hw_conf;
        rk_aiq_ccm_cfg_v2_t* accm_hw_conf_v2;
    };
} RkAiqAlgoProcResAccm;

// cgc
typedef struct _RkAiqAlgoConfigAcgc {
    RkAiqAlgoCom com;
} RkAiqAlgoConfigAcgc;

typedef struct _RkAiqAlgoProcAcgc {
    RkAiqAlgoCom com;
} RkAiqAlgoProcAcgc;

typedef struct _RkAiqAlgoProcResAcgc {
    RkAiqAlgoResCom res_com;
    rk_aiq_acgc_params_t* acgc_res;
} RkAiqAlgoProcResAcgc;

// adebayer
typedef struct _RkAiqAlgoConfigAdebayer {
    RkAiqAlgoCom com;
    int          compr_bit;
} RkAiqAlgoConfigAdebayer;

typedef struct _RkAiqAlgoProcAdebayer {
    RkAiqAlgoCom com;
    int hdr_mode;
} RkAiqAlgoProcAdebayer;

typedef struct _RkAiqAlgoProcResAdebayer {
    RkAiqAlgoResCom res_com;
    union {
        AdebayerProcResultV1_t debayerResV1;
        AdebayerProcResultV2_t debayerResV2;
        AdebayerProcResultV3_t debayerResV3;
    };
} RkAiqAlgoProcResAdebayer;

// adpcc
typedef struct _RkAiqAlgoConfigAdpcc {
    RkAiqAlgoCom com;
    AdpccConfig_t stAdpccConfig;
} RkAiqAlgoConfigAdpcc;

typedef struct _RkAiqAlgoProcAdpcc {
    RkAiqAlgoCom com;
    int iso;
    int hdr_mode;
} RkAiqAlgoProcAdpcc;

typedef struct _RkAiqAlgoProcResAdpcc {
    RkAiqAlgoResCom res_com;
    //Sensor_dpcc_res_t SenDpccRes;
    AdpccProcResult_t* stAdpccProcResult;
} RkAiqAlgoProcResAdpcc;

// afec
typedef struct _RkAiqAlgoConfigAfec {
    RkAiqAlgoCom com;
    CalibDb_FEC_t afec_calib_cfg;
    const char* resource_path;
    isp_drv_share_mem_ops_t *mem_ops_ptr;
} RkAiqAlgoConfigAfec;

typedef struct _RkAiqAlgoProcAfec {
    RkAiqAlgoCom com;
} RkAiqAlgoProcAfec;

typedef struct _RkAiqAlgoProcResAfec {
    RkAiqAlgoResCom res_com;
    fec_preprocess_result_t* afec_result;
} RkAiqAlgoProcResAfec;

// agamma
typedef struct _RkAiqAlgoConfigAgamma {
    RkAiqAlgoCom com;
#ifdef RKAIQ_ENABLE_PARSER_V1
    const CamCalibDbContext_t *calib;
#endif
} RkAiqAlgoConfigAgamma;

typedef struct _RkAiqAlgoProcAgamma {
    RkAiqAlgoCom com;
#ifdef RKAIQ_ENABLE_PARSER_V1
    const CamCalibDbContext_t *calib;
#endif
} RkAiqAlgoProcAgamma;

typedef struct _RkAiqAlgoProcResAgamma {
    RkAiqAlgoResCom res_com;
    AgammaProcRes_t* GammaProcRes;
} RkAiqAlgoProcResAgamma;

// adegamma
typedef struct _RkAiqAlgoConfigAdegamma {
    RkAiqAlgoCom com;
#ifdef RKAIQ_ENABLE_PARSER_V1
    const CamCalibDbContext_t *calib;
#endif
} RkAiqAlgoConfigAdegamma;

typedef struct _RkAiqAlgoProcAdegamma {
    RkAiqAlgoCom com;
#ifdef RKAIQ_ENABLE_PARSER_V1
    const CamCalibDbContext_t *calib;
#endif
} RkAiqAlgoProcAdegamma;

typedef struct _RkAiqAlgoProcResAdegamma {
    RkAiqAlgoResCom res_com;
    AdegammaProcRes_t* adegamma_proc_res;
} RkAiqAlgoProcResAdegamma;

// agic
typedef struct _RkAiqAlgoConfigAgic {
    RkAiqAlgoCom com;
} RkAiqAlgoConfigAgic;

typedef struct _RkAiqAlgoProcAgic {
    RkAiqAlgoCom com;
    int hdr_mode;
    uint8_t raw_bits;
    uint32_t iso;
} RkAiqAlgoProcAgic;

typedef struct _RkAiqAlgoProcResAgic {
    RkAiqAlgoResCom res_com;
    AgicProcResult_t* gicRes;
} RkAiqAlgoProcResAgic;

// aie
typedef struct _RkAiqAlgoConfigAie {
    RkAiqAlgoCom com;
} RkAiqAlgoConfigAie;

typedef struct _RkAiqAlgoProcAie {
    RkAiqAlgoCom com;
} RkAiqAlgoProcAie;

typedef struct _RkAiqAlgoProcResAie {
    RkAiqAlgoResCom res_com;
    rk_aiq_isp_ie_t* ieRes;
} RkAiqAlgoProcResAie;

// aldch
typedef struct _RkAiqAlgoConfigAldch {
    RkAiqAlgoCom com;
    CalibDb_LDCH_t aldch_calib_cfg;
    const char* resource_path;
    isp_drv_share_mem_ops_t *mem_ops_ptr;
    bool is_multi_isp;
    uint8_t multi_isp_extended_pixel;
} RkAiqAlgoConfigAldch;

typedef struct _RkAiqAlgoProcAldch {
    RkAiqAlgoCom com;
} RkAiqAlgoProcAldch;

typedef struct _RkAiqAlgoProcResAldch {
    RkAiqAlgoResCom res_com;
    ldch_process_result_t* ldch_result;
} RkAiqAlgoProcResAldch;

typedef struct _RkAiqAlgoProcResAldchV21 {
    RkAiqAlgoResCom res_com;
    ldch_v21_process_result_t* ldch_result;
} RkAiqAlgoProcResAldchV21;

// alsc
typedef struct _RkAiqAlgoConfigAlsc {
    RkAiqAlgoCom com;
    alsc_sw_info_t alsc_sw_info;
} RkAiqAlgoConfigAlsc;

typedef struct _RkAiqAlgoProcAlsc {
    RkAiqAlgoCom com;
    alsc_sw_info_t   alsc_sw_info;
    XCamVideoBuffer* tx;
} RkAiqAlgoProcAlsc;

typedef struct _RkAiqAlgoProcResAlsc {
    RkAiqAlgoResCom res_com;
    rk_aiq_lsc_cfg_t* alsc_hw_conf;
} RkAiqAlgoProcResAlsc;

// aorb
typedef struct _RkAiqAlgoConfigAorb {
    RkAiqAlgoCom com;
    CalibDbV2_Orb_t orb_calib_cfg;
} RkAiqAlgoConfigAorb;

typedef struct _RkAiqAlgoPreAorb {
    RkAiqAlgoCom com;
} RkAiqAlgoPreAorb;

typedef struct _RkAiqAlgoPreResAorb {
    RkAiqAlgoResCom res_com;
} RkAiqAlgoPreResAorb;

typedef struct _RkAiqAlgoProcAorb {
    RkAiqAlgoCom com;
    XCamVideoBuffer* orb_stats_buf;
} RkAiqAlgoProcAorb;

typedef struct _RkAiqAlgoProcResAorb {
    RkAiqAlgoResCom res_com;
    rk_aiq_orb_algo_meas_t aorb_meas;
    rk_aiq_isp_orb_meas_t aorb_isp_meas;
    orb_preprocess_result_t orb_pre_result;
} RkAiqAlgoProcResAorb;

typedef struct _RkAiqAlgoPostAorb {
    RkAiqAlgoCom com;
} RkAiqAlgoPostAorb;

typedef struct _RkAiqAlgoPostResAorb {
    RkAiqAlgoResCom res_com;
} RkAiqAlgoPostResAorb;

// awdr
typedef struct _RkAiqAlgoConfigAwdr {
    RkAiqAlgoCom com;
} RkAiqAlgoConfigAwdr;

typedef struct _RkAiqAlgoProcAwdr {
    RkAiqAlgoCom com;
} RkAiqAlgoProcAwdr;

typedef struct _RkAiqAlgoProcResAwdr {
    RkAiqAlgoResCom res_com;
} RkAiqAlgoProcResAwdr;

// rawnr v1
typedef struct _RkAiqAlgoConfigArawnr {
    RkAiqAlgoCom com;
    Abayernr_Config_V1_t stArawnrConfig;
} RkAiqAlgoConfigArawnr;

typedef struct _RkAiqAlgoPreArawnr {
    RkAiqAlgoCom com;
    Abayernr_Config_V1_t stArawnrConfig;
} RkAiqAlgoPreArawnr;

typedef struct _RkAiqAlgoPreResArawnr {
    RkAiqAlgoResCom res_com;
} RkAiqAlgoPreResArawnr;

typedef struct _RkAiqAlgoProcArawnr {
    RkAiqAlgoCom com;
    int iso;
    int hdr_mode;
} RkAiqAlgoProcArawnr;

typedef struct _RkAiqAlgoProcResArawnr {
    RkAiqAlgoResCom res_com;
    Abayernr_ProcResult_V1_t stArawnrProcResult;
} RkAiqAlgoProcResArawnr;

typedef struct _RkAiqAlgoPostArawnr {
    RkAiqAlgoCom com;
} RkAiqAlgoPostArawnr;

typedef struct _RkAiqAlgoPostResArawnr {
    RkAiqAlgoResCom res_com;
} RkAiqAlgoPostResArawnr;

// arawnr v2
typedef struct _RkAiqAlgoConfigArawnrV2 {
    RkAiqAlgoCom com;
    Abayernr_Config_V2_t stArawnrConfig;
} RkAiqAlgoConfigArawnrV2;

typedef struct _RkAiqAlgoPreArawnrV2 {
    RkAiqAlgoCom com;
    Abayernr_Config_V2_t stArawnrConfig;
} RkAiqAlgoPreArawnrV2;

typedef struct _RkAiqAlgoPreResArawnrV2 {
    RkAiqAlgoResCom res_com;
} RkAiqAlgoPreResArawnrV2;

typedef struct _RkAiqAlgoProcArawnrV2 {
    RkAiqAlgoCom com;
    int iso;
    int hdr_mode;
} RkAiqAlgoProcArawnrV2;

typedef struct _RkAiqAlgoProcResArawnrV2 {
    RkAiqAlgoResCom res_com;
    Abayernr_ProcResult_V2_t stArawnrProcResult;
} RkAiqAlgoProcResArawnrV2;

typedef struct _RkAiqAlgoPostArawnrV2 {
    RkAiqAlgoCom com;
} RkAiqAlgoPostArawnrV2;

typedef struct _RkAiqAlgoPostResArawnrV2 {
    RkAiqAlgoResCom res_com;
} RkAiqAlgoPostResArawnrV2;

// amfnr
typedef struct _RkAiqAlgoConfigAmfnr {
    RkAiqAlgoCom com;
    Amfnr_Config_V1_t stAmfnrConfig;
} RkAiqAlgoConfigAmfnr;

typedef struct _RkAiqAlgoPreAmfnr {
    RkAiqAlgoCom com;
    Amfnr_Config_V1_t stAmfnrConfig;
} RkAiqAlgoPreAmfnr;

typedef struct _RkAiqAlgoPreResAmfnr {
    RkAiqAlgoResCom res_com;
} RkAiqAlgoPreResAmfnr;

typedef struct _RkAiqAlgoProcAmfnr {
    RkAiqAlgoCom com;
    int iso;
    int hdr_mode;
} RkAiqAlgoProcAmfnr;

typedef struct _RkAiqAlgoProcResAmfnr {
    RkAiqAlgoResCom res_com;
    Amfnr_ProcResult_V1_t stAmfnrProcResult;
} RkAiqAlgoProcResAmfnr;

typedef struct _RkAiqAlgoPostAmfnr {
    RkAiqAlgoCom com;
} RkAiqAlgoPostAmfnr;

typedef struct _RkAiqAlgoPostResAmfnr {
    RkAiqAlgoResCom res_com;
} RkAiqAlgoPostResAmfnr;

//abayernr v2
typedef struct _RkAiqAlgoConfigAbayer2dnrV2 {
    RkAiqAlgoCom com;
    Abayer2dnr_Config_V2_t stArawnrConfig;
} RkAiqAlgoConfigAbayer2dnrV2;

typedef struct _RkAiqAlgoPreAbayer2dnrV2 {
    RkAiqAlgoCom com;
    Abayer2dnr_Config_V2_t stArawnrConfig;
} RkAiqAlgoPreAbayer2dnrV2;

typedef struct _RkAiqAlgoPreResAbayer2dnrV2 {
    RkAiqAlgoResCom res_com;
} RkAiqAlgoPreResAbayer2dnrV2;

typedef struct _RkAiqAlgoProcAbayer2dnrV2 {
    RkAiqAlgoCom com;
    int iso;
    int hdr_mode;
} RkAiqAlgoProcAbayer2dnrV2;

typedef struct _RkAiqAlgoProcResAbayer2dnrV2 {
    RkAiqAlgoResCom res_com;
    Abayer2dnr_ProcResult_V2_t stArawnrProcResult;
} RkAiqAlgoProcResAbayer2dnrV2;

typedef struct _RkAiqAlgoPostAbayer2dnrV2 {
    RkAiqAlgoCom com;
} RkAiqAlgoPostAbayer2dnrV2;

typedef struct _RkAiqAlgoPostResAbayer2dnrV2 {
    RkAiqAlgoResCom res_com;
} RkAiqAlgoPostResAbayer2dnrV2;

//bayertnr v2
typedef struct _RkAiqAlgoConfigAbayertnrV2 {
    RkAiqAlgoCom com;
    Abayertnr_Config_V2_t stAbayertnrConfig;
} RkAiqAlgoConfigAbayertnrV2;

typedef struct _RkAiqAlgoPreAbayertnrV2 {
    RkAiqAlgoCom com;
    Abayertnr_Config_V2_t stAbayertnrConfig;
} RkAiqAlgoPreAbayertnrV2;

typedef struct _RkAiqAlgoPreResAbayertnrV2 {
    RkAiqAlgoResCom res_com;
} RkAiqAlgoPreResAbayertnrV2;

typedef struct _RkAiqAlgoProcAbayertnrV2 {
    RkAiqAlgoCom com;
    int iso;
    int hdr_mode;
} RkAiqAlgoProcAbayertnrV2;

typedef struct _RkAiqAlgoProcResAbayertnrV2 {
    RkAiqAlgoResCom res_com;
    Abayertnr_ProcResult_V2_t stAbayertnrProcResult;
} RkAiqAlgoProcResAbayertnrV2;

typedef struct _RkAiqAlgoPostAbayertnrV2 {
    RkAiqAlgoCom com;
} RkAiqAlgoPostAbayertnrV2;

typedef struct _RkAiqAlgoPostResAbayertnrV2 {
    RkAiqAlgoResCom res_com;
} RkAiqAlgoPostResAbayertnrV2;

//ynr v1
typedef struct _RkAiqAlgoConfigAynr {
    RkAiqAlgoCom com;
    Aynr_Config_V1_t stAynrConfig;
} RkAiqAlgoConfigAynr;

typedef struct _RkAiqAlgoPreAynr {
    RkAiqAlgoCom com;
    Aynr_Config_V1_t stAynrConfig;
} RkAiqAlgoPreAynr;

typedef struct _RkAiqAlgoPreResAynr {
    RkAiqAlgoResCom res_com;
} RkAiqAlgoPreResAynr;

typedef struct _RkAiqAlgoProcAynr {
    RkAiqAlgoCom com;
    int iso;
    int hdr_mode;
} RkAiqAlgoProcAynr;

typedef struct _RkAiqAlgoProcResAynr {
    RkAiqAlgoResCom res_com;
    Aynr_ProcResult_V1_t stAynrProcResult;
} RkAiqAlgoProcResAynr;

typedef struct _RkAiqAlgoPostAynr {
    RkAiqAlgoCom com;
} RkAiqAlgoPostAynr;

typedef struct _RkAiqAlgoPostResAynr {
    RkAiqAlgoResCom res_com;
} RkAiqAlgoPostResAynr;

//ynr v2
typedef struct _RkAiqAlgoConfigAynrV2 {
    RkAiqAlgoCom com;
    Aynr_Config_V2_t stAynrConfig;
} RkAiqAlgoConfigAynrV2;

typedef struct _RkAiqAlgoPreAynrV2 {
    RkAiqAlgoCom com;
    Aynr_Config_V2_t stAynrConfig;
} RkAiqAlgoPreAynrV2;

typedef struct _RkAiqAlgoPreResAynrV2 {
    RkAiqAlgoResCom res_com;
} RkAiqAlgoPreResAynrV2;

typedef struct _RkAiqAlgoProcAynrV2 {
    RkAiqAlgoCom com;
    int iso;
    int hdr_mode;
} RkAiqAlgoProcAynrV2;

typedef struct _RkAiqAlgoProcResAynrV2 {
    RkAiqAlgoResCom res_com;
    Aynr_ProcResult_V2_t stAynrProcResult;
} RkAiqAlgoProcResAynrV2;

typedef struct _RkAiqAlgoPostAynrV2 {
    RkAiqAlgoCom com;
} RkAiqAlgoPostAynrV2;

typedef struct _RkAiqAlgoPostResAynrV2 {
    RkAiqAlgoResCom res_com;
} RkAiqAlgoPostResAynrV2;

//ynr v3
typedef struct _RkAiqAlgoConfigAynrV3 {
    RkAiqAlgoCom com;
    Aynr_Config_V3_t stAynrConfig;
} RkAiqAlgoConfigAynrV3;

typedef struct _RkAiqAlgoPreAynrV3 {
    RkAiqAlgoCom com;
    Aynr_Config_V2_t stAynrConfig;
} RkAiqAlgoPreAynrV3;

typedef struct _RkAiqAlgoPreResAynrV3 {
    RkAiqAlgoResCom res_com;
} RkAiqAlgoPreResAynrV3;

typedef struct _RkAiqAlgoProcAynrV3 {
    RkAiqAlgoCom com;
    int iso;
    int hdr_mode;
} RkAiqAlgoProcAynrV3;

typedef struct _RkAiqAlgoProcResAynrV3 {
    RkAiqAlgoResCom res_com;
    Aynr_ProcResult_V3_t stAynrProcResult;
} RkAiqAlgoProcResAynrV3;

typedef struct _RkAiqAlgoPostAynrV3 {
    RkAiqAlgoCom com;
} RkAiqAlgoPostAynrV3;

typedef struct _RkAiqAlgoPostResAynrV3 {
    RkAiqAlgoResCom res_com;
} RkAiqAlgoPostResAynrV3;

// acnr
typedef struct _RkAiqAlgoConfigAcnr {
    RkAiqAlgoCom com;
    Auvnr_Config_V1_t stAuvnrConfig;
} RkAiqAlgoConfigAcnr;

typedef struct _RkAiqAlgoPreAcnr {
    RkAiqAlgoCom com;
    Auvnr_Config_V1_t stAuvnrConfig;
} RkAiqAlgoPreAcnr;

typedef struct _RkAiqAlgoPreResAcnr {
    RkAiqAlgoResCom res_com;
} RkAiqAlgoPreResAcnr;

typedef struct _RkAiqAlgoProcAcnr {
    RkAiqAlgoCom com;
    int iso;
    int hdr_mode;
} RkAiqAlgoProcAcnr;

typedef struct _RkAiqAlgoProcResAcnr {
    RkAiqAlgoResCom res_com;
    Auvnr_ProcResult_V1_t stAuvnrProcResult;
} RkAiqAlgoProcResAcnr;

typedef struct _RkAiqAlgoPostAcnr {
    RkAiqAlgoCom com;
} RkAiqAlgoPostAcnr;

typedef struct _RkAiqAlgoPostResAcnr {
    RkAiqAlgoResCom res_com;
} RkAiqAlgoPostResAcnr;

//cnr v1
typedef struct _RkAiqAlgoConfigAcnrV1 {
    RkAiqAlgoCom com;
    Acnr_Config_V1_t stAcnrConfig;
} RkAiqAlgoConfigAcnrV1;

typedef struct _RkAiqAlgoPreAcnrV1 {
    RkAiqAlgoCom com;
    Acnr_Config_V1_t stAcnrConfig;
} RkAiqAlgoPreAcnrV1;

typedef struct _RkAiqAlgoPreResAcnrV1 {
    RkAiqAlgoResCom res_com;
} RkAiqAlgoPreResAcnrV1;

typedef struct _RkAiqAlgoProcAcnrV1 {
    RkAiqAlgoCom com;
    int iso;
    int hdr_mode;
} RkAiqAlgoProcAcnrV1;

typedef struct _RkAiqAlgoProcResAcnrV1 {
    RkAiqAlgoResCom res_com;
    Acnr_ProcResult_V1_t stAcnrProcResult;
} RkAiqAlgoProcResAcnrV1;

typedef struct _RkAiqAlgoPostAcnrV1 {
    RkAiqAlgoCom com;
} RkAiqAlgoPostAcnrV1;

typedef struct _RkAiqAlgoPostResAcnrV1 {
    RkAiqAlgoResCom res_com;
} RkAiqAlgoPostResAcnrV1;

// acnrV2
typedef struct _RkAiqAlgoConfigAcnrV2 {
    RkAiqAlgoCom com;
    Acnr_Config_V2_t stAcnrConfig;
} RkAiqAlgoConfigAcnrV2;

typedef struct _RkAiqAlgoPreAcnrV2 {
    RkAiqAlgoCom com;
    Acnr_Config_V2_t stAcnrConfig;
} RkAiqAlgoPreAcnrV2;

typedef struct _RkAiqAlgoPreResAcnrV2 {
    RkAiqAlgoResCom res_com;
} RkAiqAlgoPreResAcnrV2;

typedef struct _RkAiqAlgoProcAcnrV2 {
    RkAiqAlgoCom com;
    int iso;
    int hdr_mode;
} RkAiqAlgoProcAcnrV2;

typedef struct _RkAiqAlgoProcResAcnrV2 {
    RkAiqAlgoResCom res_com;
    Acnr_ProcResult_V2_t stAcnrProcResult;
} RkAiqAlgoProcResAcnrV2;

typedef struct _RkAiqAlgoPostAcnrV2 {
    RkAiqAlgoCom com;
} RkAiqAlgoPostAcnrV2;

typedef struct _RkAiqAlgoPostResAcnrV2 {
    RkAiqAlgoResCom res_com;
} RkAiqAlgoPostResAcnrV2;

// adrc
typedef struct _RkAiqAlgoConfigAdrc {
    RkAiqAlgoCom com;
    int working_mode;
    unsigned short rawWidth;
    unsigned short  rawHeight;
    unsigned char compr_bit;
} RkAiqAlgoConfigAdrc;

typedef struct _RkAiqAlgoProcAdrc {
    RkAiqAlgoCom com;
#if RKAIQ_HAVE_DRC_V12 || RKAIQ_HAVE_DRC_V12_LITE || RKAIQ_HAVE_DRC_V20
    adrc_blcRes_V32_t ablcV32_proc_res;
#endif
    bool LongFrmMode;
} RkAiqAlgoProcAdrc;

typedef struct _RkAiqAlgoProcResAdrc {
    RkAiqAlgoResCom res_com;
    RkAiqAdrcProcResult_t* AdrcProcRes;
} RkAiqAlgoProcResAdrc;

// argbir
typedef struct _RkAiqAlgoConfigArgbir {
    RkAiqAlgoCom com;
    int working_mode;
    unsigned short rawWidth;
    unsigned short rawHeight;
} RkAiqAlgoConfigArgbir;

typedef struct _RkAiqAlgoProcArgbir {
    RkAiqAlgoCom com;
} RkAiqAlgoProcArgbir;

typedef struct _RkAiqAlgoProcResArgbir {
    RkAiqAlgoResCom res_com;
    RkAiqArgbirProcResult_t* ArgbirProcRes;
} RkAiqAlgoProcResArgbir;

// aeis
typedef struct _RkAiqAlgoConfigAeis {
    RkAiqAlgoCom com;
    const rk_aiq_mems_sensor_intf_t *mems_sensor_intf;
    CalibDbV2_Eis_t calib;
    isp_drv_share_mem_ops_t *mem_ops;
} RkAiqAlgoConfigAeis;

typedef struct _RkAiqAlgoProcAeis {
    RkAiqAlgoCom com;
    int64_t sof;
    double rolling_shutter_skew;
    float integration_time;
    int analog_gain;
    uint32_t frame_width;
    uint32_t frame_height;
    XCamVideoBuffer* orb_stats_buf;
    XCamVideoBuffer* nr_img_buf;
} RkAiqAlgoProcAeis;

typedef struct _RkAiqAlgoProcResAeis {
    RkAiqAlgoResCom res_com;
    bool fec_en;
    uint32_t frame_id;
    int fd;
    int mesh_size;
    int mesh_density;
    int img_buf_index;
    unsigned int img_size;
} RkAiqAlgoProcResAeis;

// amd
typedef struct _RkAiqAlgoConfigAmd {
    RkAiqAlgoCom com;
    int spWidth;
    int spHeight;
    int spAlignedW;
    int spAlignedH;
} RkAiqAlgoConfigAmd;

typedef struct _RkAiqAlgoProcAmd {
    RkAiqAlgoCom com;
    int iso;
    int hdr_mode;
    rk_aiq_amd_algo_stat_t stats;
} RkAiqAlgoProcAmd;

typedef struct _RkAiqAlgoProcResAmd {
    RkAiqAlgoResCom res_com;
    rk_aiq_amd_params_t amd_proc_res;
} RkAiqAlgoProcResAmd;

// again
typedef struct _RkAiqAlgoConfigAgain {
    RkAiqAlgoCom com;
    Again_Config_V1_t stAgainConfig;
} RkAiqAlgoConfigAgain;

typedef struct _RkAiqAlgoPreAgain {
    RkAiqAlgoCom com;
    Again_Config_V1_t stAgainConfig;
} RkAiqAlgoPreAgain;

typedef struct _RkAiqAlgoPreResAgain {
    RkAiqAlgoResCom res_com;
} RkAiqAlgoPreResAgain;

typedef struct _RkAiqAlgoProcAgain {
    RkAiqAlgoCom com;
    int iso;
    int hdr_mode;
} RkAiqAlgoProcAgain;

typedef struct _RkAiqAlgoProcResAgain {
    RkAiqAlgoResCom res_com;
    Again_ProcResult_V1_t stAgainProcResult;
} RkAiqAlgoProcResAgain;

typedef struct _RkAiqAlgoPostAgain {
    RkAiqAlgoCom com;
} RkAiqAlgoPostAgain;

typedef struct _RkAiqAlgoPostResAgain {
    RkAiqAlgoResCom res_com;
} RkAiqAlgoPostResAgain;

//again v2
typedef struct _RkAiqAlgoConfigAgainV2 {
    RkAiqAlgoCom com;
    Again_Config_V2_t stAgainConfig;
    isp_drv_share_mem_ops_t *mem_ops_ptr;
} RkAiqAlgoConfigAgainV2;

typedef struct _RkAiqAlgoProcAgainV2 {
    RkAiqAlgoCom com;
    rk_aiq_again_stat_t stats;
    int iso;
    int hdr_mode;
#if RKAIQ_HAVE_BLC_V32
    AblcProc_V32_t* stAblcV32_proc_res;
#endif
} RkAiqAlgoProcAgainV2;

typedef struct _RkAiqAlgoProcResAgainV2 {
    RkAiqAlgoResCom res_com;
    Again_ProcResult_V2_t stAgainProcResult;
} RkAiqAlgoProcResAgainV2;

// acac
typedef struct _RkAiqAlgoConfigAcac {
    RkAiqAlgoCom com;
    isp_drv_share_mem_ops_t* mem_ops;
    uint32_t width;
    uint32_t height;
    bool is_multi_sensor;
    bool is_multi_isp;
    uint8_t multi_isp_extended_pixel;
    char iqpath[255];
} RkAiqAlgoConfigAcac;

typedef struct _RkAiqAlgoProcAcac {
    RkAiqAlgoCom com;
    int iso;
    int hdr_mode;
    float hdr_ratio;
    uint8_t raw_bits;
} RkAiqAlgoProcAcac;

typedef struct _RkAiqAlgoProcResAcac {
    RkAiqAlgoResCom res_com;
    bool enable;
#if (RKAIQ_HAVE_CAC_V03 || RKAIQ_HAVE_CAC_V10) && defined(ISP_HW_V30)
    rkaiq_cac_v10_hw_param_t* config;
#elif RKAIQ_HAVE_CAC_V11
    rkaiq_cac_v11_hw_param_t* config;
#endif
} RkAiqAlgoProcResAcac;

//bayer2dnrv23
typedef struct _RkAiqAlgoConfigAbayer2dnrV23 {
    RkAiqAlgoCom com;
    Abayer2dnr_Config_V23_t stArawnrConfig;
} RkAiqAlgoConfigAbayer2dnrV23;

typedef struct _RkAiqAlgoProcAbayer2dnrV23 {
    RkAiqAlgoCom com;
    int iso;
    int hdr_mode;
    AblcProc_V32_t* stAblcV32_proc_res;
    int bayertnr_en;
} RkAiqAlgoProcAbayer2dnrV23;

typedef struct _RkAiqAlgoProcResAbayer2dnrV23 {
    RkAiqAlgoResCom res_com;
    Abayer2dnr_ProcResult_V23_t stArawnrProcResult;
} RkAiqAlgoProcResAbayer2dnrV23;

//bayertnr v23
typedef struct _RkAiqAlgoConfigAbayertnrV23 {
    RkAiqAlgoCom com;
    Abayertnr_Config_V23_t stAbayertnrConfig;
} RkAiqAlgoConfigAbayertnrV23;

typedef struct _RkAiqAlgoProcAbayertnrV23 {
    RkAiqAlgoCom com;
    int iso;
    int hdr_mode;
    AblcProc_V32_t* stAblcV32_proc_res;
} RkAiqAlgoProcAbayertnrV23;

typedef struct _RkAiqAlgoProcResAbayertnrV23 {
    RkAiqAlgoResCom res_com;
    Abayertnr_ProcResult_V23_t stAbayertnrProcResult;
} RkAiqAlgoProcResAbayertnrV23;

//ynr v22
typedef struct _RkAiqAlgoConfigAynrV22 {
    RkAiqAlgoCom com;
    Aynr_Config_V22_t stAynrConfig;
} RkAiqAlgoConfigAynrV22;

typedef struct _RkAiqAlgoProcAynrV22 {
    RkAiqAlgoCom com;
    int iso;
    int hdr_mode;
    AblcProc_V32_t* stAblcV32_proc_res;
} RkAiqAlgoProcAynrV22;

typedef struct _RkAiqAlgoProcResAynrV22 {
    RkAiqAlgoResCom res_com;
    Aynr_ProcResult_V22_t stAynrProcResult;
} RkAiqAlgoProcResAynrV22;

// ynr v24
typedef struct _RkAiqAlgoConfigAynrV24 {
    RkAiqAlgoCom com;
    Aynr_Config_V24_t stAynrConfig;
} RkAiqAlgoConfigAynrV24;

typedef struct _RkAiqAlgoProcAynrV24 {
    RkAiqAlgoCom com;
    int iso;
    int hdr_mode;
    AblcProc_V32_t* stAblcV32_proc_res;
} RkAiqAlgoProcAynrV24;

typedef struct _RkAiqAlgoProcResAynrV24 {
    RkAiqAlgoResCom res_com;
    Aynr_ProcResult_V24_t stAynrProcResult;
} RkAiqAlgoProcResAynrV24;

//cnr v30
typedef struct _RkAiqAlgoConfigAcnrV30 {
    RkAiqAlgoCom com;
    Acnr_Config_V30_t stAcnrConfig;
} RkAiqAlgoConfigAcnrV30;

typedef struct _RkAiqAlgoProcAcnrV30 {
    RkAiqAlgoCom com;
    int iso;
    int hdr_mode;
    AblcProc_V32_t* stAblcV32_proc_res;
} RkAiqAlgoProcAcnrV30;

typedef struct _RkAiqAlgoProcResAcnrV30 {
    RkAiqAlgoResCom res_com;
    Acnr_ProcResult_V30_t stAcnrProcResult;
} RkAiqAlgoProcResAcnrV30;

//bayertnr v30
typedef struct _RkAiqAlgoConfigAbayertnrV30 {
    RkAiqAlgoCom com;
    Abayertnr_Config_V30_t stAbayertnrConfig;
} RkAiqAlgoConfigAbayertnrV30;

typedef struct _RkAiqAlgoProcAbayertnrV30 {
    RkAiqAlgoCom com;
    int iso;
    int hdr_mode;
    AblcProc_V32_t* stAblcV32_proc_res;
    Abayertnr_Stats_V30_t stTnr_stats;
} RkAiqAlgoProcAbayertnrV30;

typedef struct _RkAiqAlgoProcResAbayertnrV30 {
    RkAiqAlgoResCom res_com;
    Abayertnr_ProcResult_V30_t stAbayertnrProcResult;
} RkAiqAlgoProcResAbayertnrV30;

//bayertnr v30
typedef struct _RkAiqAlgoConfigAyuvmeV1 {
    RkAiqAlgoCom com;
    Ayuvme_Config_V1_t stAyuvmeConfig;
} RkAiqAlgoConfigAyuvmeV1;

typedef struct _RkAiqAlgoProcAyuvmeV1 {
    RkAiqAlgoCom com;
    int iso;
    int hdr_mode;
    AblcProc_V32_t* stAblcV32_proc_res;
    int bayertnr_en;
} RkAiqAlgoProcAyuvmeV1;

typedef struct _RkAiqAlgoProcResAyuvmeV1 {
    RkAiqAlgoResCom res_com;
    Ayuvme_ProcResult_V1_t stAyuvmeProcResult;
} RkAiqAlgoProcResAyuvmeV1;

//cnr v30
typedef struct _RkAiqAlgoConfigAcnrV31 {
    RkAiqAlgoCom com;
    Acnr_Config_V31_t stAcnrConfig;
} RkAiqAlgoConfigAcnrV31;

typedef struct _RkAiqAlgoProcAcnrV31 {
    RkAiqAlgoCom com;
    int iso;
    int hdr_mode;
    AblcProc_V32_t* stAblcV32_proc_res;
} RkAiqAlgoProcAcnrV31;

typedef struct _RkAiqAlgoProcResAcnrV31 {
    RkAiqAlgoResCom res_com;
    Acnr_ProcResult_V31_t stAcnrProcResult;
} RkAiqAlgoProcResAcnrV31;

// aldc
typedef struct _RkAiqAlgoConfigAldc {
    RkAiqAlgoCom com;
    isp_drv_share_mem_ops_t* mem_ops;
    bool is_multi_isp;
    char iqpath[255];
} RkAiqAlgoConfigAldc;

typedef struct _RkAiqAlgoProcAldc {
    RkAiqAlgoCom com;
} RkAiqAlgoProcAldc;

typedef struct _RkAiqAlgoProcResAldc {
    RkAiqAlgoResCom res_com;
    bool ldch_en;
    bool ldcv_en;
    rkaiq_ldc_hw_param_t* config;
} RkAiqAlgoProcResAldc;

#endif
