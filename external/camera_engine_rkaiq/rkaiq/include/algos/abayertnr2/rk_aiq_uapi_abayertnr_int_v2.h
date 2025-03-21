#ifndef _RK_AIQ_UAPI_ABAYERTNR_INT_V2_H_
#define _RK_AIQ_UAPI_ABAYERTNR_INT_V2_H_

#include "xcore/base/xcam_common.h"
#include "algos/rk_aiq_algo_des.h"
#include "algos/abayertnr2/rk_aiq_types_abayertnr_algo_int_v2.h"

// need_sync means the implementation should consider
// the thread synchronization
// if called by RkAiqAlscHandleInt, the sync has been done
// in framework. And if called by user app directly,
// sync should be done in inner. now we just need implement
// the case of need_sync == false; need_sync is for future usage.


XCamReturn
rk_aiq_uapi_abayertnrV2_SetAttrib(RkAiqAlgoContext *ctx,
                                  rk_aiq_bayertnr_attrib_v2_t *attr,
                                  bool need_sync);

XCamReturn
rk_aiq_uapi_abayertnrV2_GetAttrib(const RkAiqAlgoContext *ctx,
                                  rk_aiq_bayertnr_attrib_v2_t *attr);


XCamReturn
rk_aiq_uapi_abayertnrV2_SetStrength(const RkAiqAlgoContext *ctx,
                                    rk_aiq_bayertnr_strength_v2_t *pStrength);

XCamReturn
rk_aiq_uapi_abayertnrV2_GetStrength(const RkAiqAlgoContext *ctx,
                                    rk_aiq_bayertnr_strength_v2_t *pStrength);


XCamReturn
rk_aiq_uapi_abayertnrV2_GetInfo (const RkAiqAlgoContext *ctx,
                                 rk_aiq_bayertnr_info_v2_t *pInfo);


#endif
