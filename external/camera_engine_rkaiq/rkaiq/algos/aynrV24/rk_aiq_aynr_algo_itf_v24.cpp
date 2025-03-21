/*
 * rk_aiq_algo_anr_itf_v24.cpp
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

#include "aynrV24/rk_aiq_aynr_algo_itf_v24.h"

#include "aynrV24/rk_aiq_aynr_algo_v24.h"
#include "rk_aiq_algo_types.h"

RKAIQ_BEGIN_DECLARE

typedef struct _RkAiqAlgoContext {
    Aynr_Context_V24_t AynrCtx;
} RkAiqAlgoContext;

static XCamReturn create_context(RkAiqAlgoContext** context, const AlgoCtxInstanceCfg* cfg) {
    XCamReturn result = XCAM_RETURN_NO_ERROR;
    LOG1_ANR("%s: (enter)\n", __FUNCTION__);

    Aynr_Context_V24_t* pAynrCtx = NULL;
#if (AYNR_USE_JSON_FILE_V24)
    Aynr_result_V24_t ret = Aynr_Init_V24(&pAynrCtx, cfg->calibv2);
#endif
    if (ret != AYNRV24_RET_SUCCESS) {
        result = XCAM_RETURN_ERROR_FAILED;
        LOGE_ANR("%s: Initializaion ANR failed (%d)\n", __FUNCTION__, ret);
    } else {
        *context = (RkAiqAlgoContext*)(pAynrCtx);
    }

    LOG1_ANR("%s: (exit)\n", __FUNCTION__);
    return result;
}

static XCamReturn destroy_context(RkAiqAlgoContext* context) {
    XCamReturn result = XCAM_RETURN_NO_ERROR;

    LOG1_ANR("%s: (enter)\n", __FUNCTION__);

    Aynr_Context_V24_t* pAynrCtx = (Aynr_Context_V24_t*)context;
    Aynr_result_V24_t ret        = Aynr_Release_V24(pAynrCtx);
    if (ret != AYNRV24_RET_SUCCESS) {
        result = XCAM_RETURN_ERROR_FAILED;
        LOGE_ANR("%s: release ANR failed (%d)\n", __FUNCTION__, ret);
    }

    LOG1_ANR("%s: (exit)\n", __FUNCTION__);
    return result;
}

static XCamReturn prepare(RkAiqAlgoCom* params) {
    XCamReturn result = XCAM_RETURN_NO_ERROR;

    LOG1_ANR("%s: (enter)\n", __FUNCTION__);

    Aynr_Context_V24_t* pAynrCtx      = (Aynr_Context_V24_t*)params->ctx;
    RkAiqAlgoConfigAynrV24* pCfgParam = (RkAiqAlgoConfigAynrV24*)params;
    pAynrCtx->prepare_type            = params->u.prepare.conf_type;

    if (!!(params->u.prepare.conf_type & RK_AIQ_ALGO_CONFTYPE_UPDATECALIB)) {
        // just update calib ptr
        if (params->u.prepare.conf_type & RK_AIQ_ALGO_CONFTYPE_UPDATECALIB_PTR) {
            return XCAM_RETURN_NO_ERROR;
        }
#if AYNR_USE_JSON_FILE_V24
        void* pCalibdbV2 = (void*)(pCfgParam->com.u.prepare.calibv2);
        CalibDbV2_YnrV24_t* ynr_v24 =
            (CalibDbV2_YnrV24_t*)(CALIBDBV2_GET_MODULE_PTR((void*)pCalibdbV2, ynr_v24));
        pAynrCtx->ynr_v24 = *ynr_v24;
#endif
        pAynrCtx->isIQParaUpdate = true;
        pAynrCtx->isReCalculate |= 1;
    }

    Aynr_result_V24_t ret = Aynr_Prepare_V24(pAynrCtx, &pCfgParam->stAynrConfig);
    if (ret != AYNRV24_RET_SUCCESS) {
        result = XCAM_RETURN_ERROR_FAILED;
        LOGE_ANR("%s: config ANR failed (%d)\n", __FUNCTION__, ret);
    }

    LOG1_ANR("%s: (exit)\n", __FUNCTION__);
    return result;
}

static XCamReturn processing(const RkAiqAlgoCom* inparams, RkAiqAlgoResCom* outparams) {
    XCamReturn result = XCAM_RETURN_NO_ERROR;
    int deltaIso      = 0;
    LOG1_ANR("%s: (enter)\n", __FUNCTION__);

    RkAiqAlgoProcAynrV24* pAynrProcParams       = (RkAiqAlgoProcAynrV24*)inparams;
    RkAiqAlgoProcResAynrV24* pAynrProcResParams = (RkAiqAlgoProcResAynrV24*)outparams;
    Aynr_Context_V24_t* pAynrCtx                = (Aynr_Context_V24_t*)inparams->ctx;
    Aynr_ExpInfo_V24_t stExpInfo;
    memset(&stExpInfo, 0x00, sizeof(Aynr_ExpInfo_V24_t));

    LOGD_ANR("%s:%d frame_id:%d init:%d hdr mode:%d  \n", __FUNCTION__, __LINE__,
             inparams->frame_id, inparams->u.proc.init, pAynrProcParams->hdr_mode);

    bool oldGrayMode = false;
    oldGrayMode      = pAynrCtx->isGrayMode;
    if (inparams->u.proc.gray_mode) {
        pAynrCtx->isGrayMode = true;
    } else {
        pAynrCtx->isGrayMode = false;
    }

    if (oldGrayMode != pAynrCtx->isGrayMode) {
        pAynrCtx->isReCalculate |= 1;
    }

    Aynr_result_V24_t ret = Aynr_PreProcess_V24(pAynrCtx);
    if (ret != AYNRV24_RET_SUCCESS) {
        result = XCAM_RETURN_ERROR_FAILED;
        LOGE_ANR("%s: ANRPreProcess failed (%d)\n", __FUNCTION__, ret);
    }

    stExpInfo.hdr_mode = 0;
    for (int i = 0; i < 3; i++) {
        stExpInfo.arIso[i]   = 50;
        stExpInfo.arAGain[i] = 1.0;
        stExpInfo.arDGain[i] = 1.0;
        stExpInfo.arTime[i]  = 0.01;
    }

    if (pAynrProcParams->hdr_mode == RK_AIQ_WORKING_MODE_NORMAL) {
        stExpInfo.hdr_mode = 0;
    } else if (pAynrProcParams->hdr_mode == RK_AIQ_ISP_HDR_MODE_2_FRAME_HDR ||
               pAynrProcParams->hdr_mode == RK_AIQ_ISP_HDR_MODE_2_LINE_HDR) {
        stExpInfo.hdr_mode = 1;
    } else if (pAynrProcParams->hdr_mode == RK_AIQ_ISP_HDR_MODE_3_FRAME_HDR ||
               pAynrProcParams->hdr_mode == RK_AIQ_ISP_HDR_MODE_3_LINE_HDR) {
        stExpInfo.hdr_mode = 2;
    }
    stExpInfo.snr_mode = 0;

    stExpInfo.blc_ob_predgain = 1.0;
#ifndef USE_NEWSTRUCT
    if (pAynrProcParams != NULL) {
        stExpInfo.blc_ob_predgain = pAynrProcParams->stAblcV32_proc_res->isp_ob_predgain;
        if (stExpInfo.blc_ob_predgain != pAynrCtx->stExpInfo.blc_ob_predgain) {
            pAynrCtx->isReCalculate |= 1;
        }
    }
#endif
#if 0  // TODO Merge:
    XCamVideoBuffer* xCamAePreRes = pAynrProcParams->com.u.proc.res_comb->ae_pre_res;
    RkAiqAlgoPreResAe* pAEPreRes = nullptr;
    if (xCamAePreRes) {
        // xCamAePreRes->ref(xCamAePreRes);
        pAEPreRes = (RkAiqAlgoPreResAe*)xCamAePreRes->map(xCamAePreRes);
        if (!pAEPreRes) {
            LOGE_ANR("ae pre result is null");
        } else {
            LOGD_ANR("ae_pre_result: meanluma:%f time:%f gain:%f env_luma:%f \n",
                     pAEPreRes->ae_pre_res_rk.MeanLuma[0],
                     pAEPreRes->ae_pre_res_rk.LinearExp.exp_real_params.integration_time,
                     pAEPreRes->ae_pre_res_rk.LinearExp.exp_real_params.analog_gain,
                     pAEPreRes->ae_pre_res_rk.GlobalEnvLux);
        }
        // xCamAePreRes->unref(xCamAePreRes);
    }
#endif

    RKAiqAecExpInfo_t* curExp = pAynrProcParams->com.u.proc.curExp;
    if (curExp != NULL) {
        stExpInfo.snr_mode = curExp->CISFeature.SNR;
        if (pAynrProcParams->hdr_mode == RK_AIQ_WORKING_MODE_NORMAL) {
            stExpInfo.hdr_mode = 0;
            if (curExp->LinearExp.exp_real_params.analog_gain < 1.0) {
                stExpInfo.arAGain[0] = 1.0;
                LOGW_ANR("leanr mode again is wrong, use 1.0 instead\n");
            } else {
                stExpInfo.arAGain[0] = curExp->LinearExp.exp_real_params.analog_gain;
            }
            if (curExp->LinearExp.exp_real_params.digital_gain < 1.0) {
                stExpInfo.arDGain[0] = 1.0;
                LOGW_ANR("leanr mode dgain is wrong, use 1.0 instead\n");
            } else {
                stExpInfo.arDGain[0] = curExp->LinearExp.exp_real_params.digital_gain;
            }
            if (curExp->LinearExp.exp_real_params.isp_dgain < 1.0) {
                stExpInfo.isp_dgain[0] = 1.0;
                LOGW_ANR("leanr mode isp_dgain is wrong, use 1.0 instead\n");
            } else {
                stExpInfo.isp_dgain[0] = curExp->LinearExp.exp_real_params.isp_dgain;
            }
#ifndef USE_NEWSTRUCT
            if (stExpInfo.blc_ob_predgain < 1.0) {
                stExpInfo.blc_ob_predgain = 1.0;
            }
            // stExpInfo.arAGain[0] = 64.0;
            stExpInfo.arTime[0] = curExp->LinearExp.exp_real_params.integration_time;
            stExpInfo.arIso[0]  = stExpInfo.arAGain[0] * stExpInfo.arDGain[0] *
                                 stExpInfo.blc_ob_predgain * 50 * stExpInfo.isp_dgain[0];
#else
            // stExpInfo.arAGain[0] = 64.0;
            stExpInfo.arTime[0] = curExp->LinearExp.exp_real_params.integration_time;
            stExpInfo.arIso[0]  = stExpInfo.arAGain[0] * stExpInfo.arDGain[0] *
                                  50 * stExpInfo.isp_dgain[0];
#endif
        } else {
            for (int i = 0; i < 3; i++) {
                if (curExp->HdrExp[i].exp_real_params.analog_gain < 1.0) {
                    stExpInfo.arAGain[i] = 1.0;
                    LOGW_ANR("hdr mode again is wrong, use 1.0 instead\n");
                } else {
                    stExpInfo.arAGain[i] = curExp->HdrExp[i].exp_real_params.analog_gain;
                }
                if (curExp->HdrExp[i].exp_real_params.digital_gain < 1.0) {
                    stExpInfo.arDGain[i] = 1.0;
                    LOGW_ANR("hdr mode dgain is wrong, use 1.0 instead\n");
                } else {
                    stExpInfo.arDGain[i] = curExp->HdrExp[i].exp_real_params.digital_gain;
                }
                if (curExp->HdrExp[i].exp_real_params.isp_dgain < 1.0) {
                    stExpInfo.isp_dgain[i] = 1.0;
                    LOGW_ANR("hdr mode isp_dgain is wrong, use 1.0 instead\n");
                } else {
                    stExpInfo.isp_dgain[i] = curExp->HdrExp[i].exp_real_params.isp_dgain;
                }
                stExpInfo.blc_ob_predgain = 1.0;
                stExpInfo.arTime[i]       = curExp->HdrExp[i].exp_real_params.integration_time;
                stExpInfo.arIso[i] =
                    stExpInfo.arAGain[i] * stExpInfo.arDGain[i] * 50 * stExpInfo.isp_dgain[i];

                LOGD_ANR(
                    "%s:%d index:%d again:%f dgain:%f isp_dgain:%f time:%f iso:%d hdr_mode:%d\n",
                    __FUNCTION__, __LINE__, i, stExpInfo.arAGain[i], stExpInfo.arDGain[i],
                    stExpInfo.isp_dgain[i], stExpInfo.arTime[i], stExpInfo.arIso[i],
                    stExpInfo.hdr_mode);
            }
        }
    } else {
        LOGE_ANR("%s:%d curExp is NULL, so use default instead \n", __FUNCTION__, __LINE__);
    }

    deltaIso =
        abs(stExpInfo.arIso[stExpInfo.hdr_mode] - pAynrCtx->stExpInfo.arIso[stExpInfo.hdr_mode]);
    if (deltaIso > AYNRV24_RECALCULATE_DELTA_ISO) {
        pAynrCtx->isReCalculate |= 1;
    }

    if (pAynrCtx->isReCalculate) {
        Aynr_result_V24_t ret = Aynr_Process_V24(pAynrCtx, &stExpInfo);
        if (ret != AYNRV24_RET_SUCCESS) {
            result = XCAM_RETURN_ERROR_FAILED;
            LOGE_ANR("%s: processing ANR failed (%d)\n", __FUNCTION__, ret);
        }

        outparams->cfg_update = true;
        LOGD_ANR("recalculate: %d delta_iso:%d \n ", pAynrCtx->isReCalculate, deltaIso);
    } else {
        outparams->cfg_update = false;
    }

    Aynr_GetProcResult_V24(pAynrCtx, &pAynrProcResParams->stAynrProcResult);
    pAynrCtx->isReCalculate = 0;

    LOG1_ANR("%s: (exit)\n", __FUNCTION__);
    return XCAM_RETURN_NO_ERROR;
}

RkAiqAlgoDescription g_RkIspAlgoDescAynrV24 = {
    .common =
        {
            .version         = RKISP_ALGO_AYNR_VERSION_V24,
            .vendor          = RKISP_ALGO_AYNR_VENDOR_V24,
            .description     = RKISP_ALGO_AYNR_DESCRIPTION_V24,
            .type            = RK_AIQ_ALGO_TYPE_AYNR,
            .id              = 0,
            .create_context  = create_context,
            .destroy_context = destroy_context,
        },
    .prepare      = prepare,
    .pre_process  = NULL,
    .processing   = processing,
    .post_process = NULL,
    .dump = NULL,
};

RKAIQ_END_DECLARE
