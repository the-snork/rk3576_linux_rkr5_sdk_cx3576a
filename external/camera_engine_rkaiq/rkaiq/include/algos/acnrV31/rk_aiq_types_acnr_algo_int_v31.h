/*
 *rk_aiq_types_alsc_algo_int.h
 *
 *  Copyright (c) 2024 Rockchip Corporation
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

#ifndef _RK_AIQ_TYPE_ACNR_ALGO_INT_V31_H_
#define _RK_AIQ_TYPE_ACNR_ALGO_INT_V31_H_

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "common/rk_aiq_comm.h"
#include "iq_parser/RkAiqCalibDbTypes.h"
#include "algos/acnrV31/rk_aiq_types_acnr_algo_v31.h"
#include "iq_parser_v2/cnr_head_v31.h"
#include "iq_parser_v2/cnr_uapi_head_v31.h"


//RKAIQ_BEGIN_DECLARE
#define RK_CNR_V31_MAX_ISO_NUM (CALIBDB_MAX_ISO_LEVEL)

#define ACNRV31_RECALCULATE_DELTA_ISO       (10)
//////////////////////////////////////////////////////////////////////////
#define     RKCNR_V31_MAX_DOWN_SCALE            8
// iir params
#define     RKCNR_V31_FIX_BIT_BF_RATIO          10
#define     RKCNR_V31_FIX_BIT_IIR_WGT           6//10
#define     RKCNR_V31_FIX_BIT_INV_SIGMA         15
#define     RKCNR_V31_FIX_BIT_SLOPE             7
#define     RKCNR_V31_FIX_BIT_WGT_RATIO         3
#define     RKCNR_V31_FIX_BIT_INT_TO_FLOAT      7
// gain params
#define     RKCNR_V31_SIGMA_FIX_BIT             8
#define     RKCNR_V31_LOCAL_GAIN_FIX_BITS       4
#define     RKCNR_V31_G_GAIN_FIX_BITS           4
#define     RKCNR_V31_G_GAIN_ALPHA_FIX_BITS     3
#define     RKCNR_V31_GAIN_ISO_FIX_BITS         7
// 3x3 bf params
#define     RKCNR_V31_FIX_BIT_GAUS_RATIO        10
#define     RKCNR_V31_sgmRatio                  6
#define     RKCNR_V31_log2e                     6
#define     RKCNR_V31_uvgain                    4
#define     RKCNR_V31_exp2_lut_y                7
#define     RKCNR_V31_bfRatio                   RKCNR_V31_exp2_lut_y
// saturation adj param fix bit
#define     RKCNR_V31_FIX_BIT_SATURATION        10
// global alpha fix bit
#define     RKCNR_V31_FIX_BIT_GLOBAL_ALPHA      10
// div sum wgt fix bit
#define     RKCNR_V31_FIX_BIT_DIV_SUM_WGT       22//
#define     RKCNR_V31_FIX_BIT_DIST_SCALE        7
//////////////////////////////////////////////////////////////////////////
// params
#define     RKCNR_V31_THUMB_BF_RADIUS           3
#define     RKCNR_V31_IIR_RADIUSY               1
#define     RKCNR_V31_IIR_RADIUSX               3//1
#define     RKCNR_V31_BF_RADIUS                 1
#define     RKCNR_V31_IIR_COEFF_NUM             10
#define     RKCNR_V31_SGM_ADJ_TABLE_LEN         13
// extend of down scale yuv, 3x3 IIR, 3x3 bf
#define     RKCNR_V31_DS_EXTEND_Y               (RKCNR_V31_IIR_RADIUSY)
#define     RKCNR_V31_DS_EXTEND_X               (RKCNR_V31_IIR_RADIUSX)
#define     RKCNR_V31_GAUS_RADIUS               2
#define     RKCNR_V31_CLIP_RADIUS               1
#define     cnr_422_5x5_gaus                    0 // 422: 5x3 or 5x5 gaussian filter


typedef enum AcnrV31_result_e {
    ACNRV31_RET_SUCCESS             = 0,   // this has to be 0, if clauses rely on it
    ACNRV31_RET_FAILURE             = 1,   // process failure
    ACNRV31_RET_INVALID_PARM        = 2,   // invalid parameter
    ACNRV31_RET_WRONG_CONFIG        = 3,   // feature not supported
    ACNRV31_RET_BUSY                = 4,   // there's already something going on...
    ACNRV31_RET_CANCELED            = 5,   // operation canceled
    ACNRV31_RET_OUTOFMEM            = 6,   // out of memory
    ACNRV31_RET_OUTOFRANGE          = 7,   // parameter/value out of range
    ACNRV31_RET_NULL_POINTER        = 8,   // the/one/all parameter(s) is a(are) NULL pointer(s)
    ACNRV31_RET_DIVISION_BY_ZERO    = 9,   // a divisor equals ZERO
    ACNRV31_RET_NO_INPUTIMAGE       = 10   // no input image
} AcnrV31_result_t;

typedef enum AcnrV31_State_e {
    ACNRV31_STATE_INVALID           = 0,                   /**< initialization value */
    ACNRV31_STATE_INITIALIZED       = 1,                   /**< instance is created, but not initialized */
    ACNRV31_STATE_STOPPED           = 2,                   /**< instance is confiured (ready to start) or stopped */
    ACNRV31_STATE_RUNNING           = 3,                   /**< instance is running (processes frames) */
    ACNRV31_STATE_LOCKED            = 4,                   /**< instance is locked (for taking snapshots) */
    ACNRV31_STATE_MAX                                      /**< max */
} AcnrV31_State_t;

typedef enum AcnrV31_OPMode_e {
    ACNRV31_OP_MODE_INVALID           = 0,                   /**< initialization value */
    ACNRV31_OP_MODE_AUTO              = 1,                   /**< instance is created, but not initialized */
    ACNRV31_OP_MODE_MANUAL            = 2,                   /**< instance is confiured (ready to start) or stopped */
    ACNRV31_OP_MODE_REG_MANUAL        = 3,
    ACNRV31_OP_MODE_MAX                                      /**< max */
} AcnrV31_OPMode_t;

typedef enum AcnrV31_ParamMode_e {
    ACNRV31_PARAM_MODE_INVALID           = 0,
    ACNRV31_PARAM_MODE_NORMAL          = 1,                   /**< initialization value */
    ACNRV31_PARAM_MODE_HDR              = 2,                   /**< instance is created, but not initialized */
    ACNRV31_PARAM_MODE_GRAY            = 3,                   /**< instance is confiured (ready to start) or stopped */
    ACNRV31_PARAM_MODE_MAX                                      /**< max */
} AcnrV31_ParamMode_t;




#if 0
typedef struct RK_CNR_Params_V31_Select_s
{
    int enable;

    int down_scale_x;
    int down_scale_y;

    float thumb_sigma;
    float thumb_bf_ratio;

    float chroma_filter_strength;
    float chroma_filter_wgt_clip;
    float anti_chroma_ghost;
    float chroma_filter_uv_gain;
    float wgt_slope;

    float gaus_ratio;

    float bf_sigmaR;
    float bf_uvgain;
    float bf_ratio;
    float hbf_wgt_clip;
    float bf_wgt0_sel;
    float global_alpha;

    float saturation_adj_offset;
    float saturation_adj_ratio;

    float global_gain;
    float global_gain_alpha;
    float local_gain_scale;
    float global_gain_thumb;
    float global_gain_alpha_thumb;

    float gain_adj_strength_ratio[13];

    float thumb_filter_wgt_coeff[4];
    float gaus_coeff[6];


} RK_CNR_Params_V31_Select_t;
#endif

typedef struct RK_CNR_Params_V31_s
{
    int enable;
    float iso[RK_CNR_V31_MAX_ISO_NUM];

    RK_CNR_Params_V31_Select_t CnrParamsISO[RK_CNR_V31_MAX_ISO_NUM];

} RK_CNR_Params_V31_t;


typedef struct Acnr_Manual_Attr_V31_s
{
    RK_CNR_Params_V31_Select_t stSelect;

    RK_CNR_Fix_V31_t stFix;

} Acnr_Manual_Attr_V31_t;

typedef struct Acnr_Auto_Attr_V31_s
{
    //all ISO params and select param

    RK_CNR_Params_V31_t stParams;
    RK_CNR_Params_V31_Select_t stSelect;

} Acnr_Auto_Attr_V31_t;

typedef struct Acnr_ProcResult_V31_s {

    //for sw simultaion
    //RK_CNR_Params_V31_Select_t stSelect;

    //for hw register
    RK_CNR_Fix_V31_t* stFix;

} Acnr_ProcResult_V31_t;


typedef struct Acnr_Config_V31_s {
    AcnrV31_State_t eState;
    AcnrV31_OPMode_t eMode;
    int rawHeight;
    int rawWidth;
} Acnr_Config_V31_t;


typedef struct rk_aiq_cnr_attrib_v31_s {
    rk_aiq_uapi_sync_t sync;
    AcnrV31_OPMode_t eMode;
    Acnr_Auto_Attr_V31_t stAuto;
    Acnr_Manual_Attr_V31_t stManual;
} rk_aiq_cnr_attrib_v31_t;


typedef struct rk_aiq_cnr_strength_v31_s {
    rk_aiq_uapi_sync_t sync;
    float percent;
    bool strength_enable;
} rk_aiq_cnr_strength_v31_t;



//calibdb


//RKAIQ_END_DECLARE

#endif

