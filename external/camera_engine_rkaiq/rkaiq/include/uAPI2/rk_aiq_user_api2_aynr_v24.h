/*
 *  Copyright (c) 2019 Rockchip Corporation
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

#ifndef _RK_AIQ_USER_API2_AYNRV24_H_
#define _RK_AIQ_USER_API2_AYNRV24_H_

#include "algos/aynrV24/rk_aiq_uapi_aynr_int_v24.h"

typedef struct rk_aiq_sys_ctx_s rk_aiq_sys_ctx_t;

RKAIQ_BEGIN_DECLARE

XCamReturn
rk_aiq_user_api2_aynrV24_SetAttrib(const rk_aiq_sys_ctx_t* sys_ctx, const rk_aiq_ynr_attrib_v24_t* attr);

XCamReturn
rk_aiq_user_api2_aynrV24_GetAttrib(const rk_aiq_sys_ctx_t* sys_ctx, rk_aiq_ynr_attrib_v24_t* attr);

XCamReturn
rk_aiq_user_api2_aynrV24_SetStrength(const rk_aiq_sys_ctx_t* sys_ctx, const rk_aiq_ynr_strength_v24_t* pStrength);

XCamReturn
rk_aiq_user_api2_aynrV24_GetStrength(const rk_aiq_sys_ctx_t* sys_ctx, rk_aiq_ynr_strength_v24_t* pStrength);

XCamReturn
rk_aiq_user_api2_aynrV24_GetInfo(const rk_aiq_sys_ctx_t* sys_ctx, rk_aiq_ynr_info_v24_t* pInfo);

RKAIQ_END_DECLARE

#endif
