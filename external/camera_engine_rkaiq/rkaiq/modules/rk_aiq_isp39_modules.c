#include "rk_aiq.h"
#include "rk_aiq_comm.h"
#include "common/rk-isp39-config.h"
#include "uAPI2/rk_aiq_user_api2_isp39.h"
#include "rk_aiq_isp39_modules.h"

#include "range_check.h"
#include "rk_aiq_module_common.c"
#include "rk_aiq_module_ae25.c"
#include "rk_aiq_module_blc30.c"
#include "rk_aiq_module_dpcc21.c"
#include "rk_aiq_module_csm21.c"
#include "rk_aiq_module_merge22.c"
#include "rk_aiq_module_lsc21.c"
#include "rk_aiq_module_cgc10.c"
#include "rk_aiq_module_cp10.c"
#include "rk_aiq_module_gain20.c"
#include "rk_aiq_module_ccm22.c"
#include "rk_aiq_module_drc40.c"
#include "rk_aiq_module_btnr_common.c"
#include "rk_aiq_module_btnr40.c"
#include "rk_aiq_module_ynr34.c"
#include "rk_aiq_module_cnr34.c"
#include "rk_aiq_module_sharp34.c"
#include "rk_aiq_module_gic21.c"
#include "rk_aiq_module_yme10.c"
#include "rk_aiq_module_cac21.c"
#include "rk_aiq_module_rgbir10.c"
#include "rk_aiq_module_3dlut20.c"
#include "rk_aiq_module_dehaze23.c"
#include "rk_aiq_module_ldch22.c"
#include "rk_aiq_module_ldcv22.c"
#include "rk_aiq_module_dm23.c"
