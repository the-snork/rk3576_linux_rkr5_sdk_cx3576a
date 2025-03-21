/*
 *  Copyright (c) 2023 Rockchip Corporation
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

#ifndef _RK_AIQ_USER_API2_ISP32_H_
#define _RK_AIQ_USER_API2_ISP32_H_

#ifndef ISP_HW_V32
#define ISP_HW_V32 1
#endif

#ifndef USE_NEWSTRUCT
#define USE_NEWSTRUCT 1
#endif
// demosaic head
// real types are defined in isp/rk_aiq_isp_dm21.h, algos/rk_aiq_api_types_dm.h
#include "isp/rk_aiq_isp_dm21.h"
#include "uAPI2/rk_aiq_user_api2_dm.h"
#ifdef RKAIQ_HAVE_BAYERTNR_V32
#include "isp/rk_aiq_isp_btnr32.h"
#include "uAPI2/rk_aiq_user_api2_btnr.h"
#endif
#include "isp/rk_aiq_isp_gamma21.h"
#include "uAPI2/rk_aiq_user_api2_gamma.h"
#include "isp/rk_aiq_isp_ynr32.h"
#include "uAPI2/rk_aiq_user_api2_ynr.h"
#ifdef RKAIQ_HAVE_DRC_V12
#include "isp/rk_aiq_isp_drc32.h"
#include "uAPI2/rk_aiq_user_api2_drc.h"
#endif
#include "isp/rk_aiq_isp_dehaze22.h"
#include "uAPI2/rk_aiq_user_api2_dehaze.h"
#include "isp/rk_aiq_isp_sharp32.h"
#include "uAPI2/rk_aiq_user_api2_sharp.h"
#include "isp/rk_aiq_isp_cnr32.h"
#include "uAPI2/rk_aiq_user_api2_cnr.h"
#include "isp/rk_aiq_isp_dpcc20.h"
#include "isp/rk_aiq_isp_gic21.h"
#include "uAPI2/rk_aiq_user_api2_gic.h"
#include "isp/rk_aiq_isp_cac21.h"
#include "uAPI2/rk_aiq_user_api2_cac.h"
#include "isp/rk_aiq_isp_merge22.h"
#include "uAPI2/rk_aiq_user_api2_merge.h"
#include "isp/rk_aiq_isp_lsc21.h"
#include "uAPI2/rk_aiq_user_api2_lsc.h"
#endif
