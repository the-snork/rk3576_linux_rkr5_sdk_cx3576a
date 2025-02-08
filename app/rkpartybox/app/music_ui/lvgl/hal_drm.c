#include <lvgl/lvgl.h>
//#include <lvgl/lv_conf.h>
#include <lvgl/lv_drivers/display/drm.h>
#include "rk_utils.h"

void hal_drm_init(lv_coord_t hor_res, lv_coord_t ver_res, int rotated)
{
    /*Create a display*/
    os_env_set_u32("lv_disp_witdh", hor_res);
    os_env_set_u32("lv_disp_height", ver_res);
    //os_env_set_u32("lv_disp_fps", DISP_FRAME_RATE);

    drm_disp_drv_init(rotated * 90 % 360);
}

void hal_drm_deinit(void)
{
    drm_disp_drv_deinit();
}
