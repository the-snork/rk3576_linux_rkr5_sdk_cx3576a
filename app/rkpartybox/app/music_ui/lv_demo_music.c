/**
 * @file lv_demo_music.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_demo_music.h"

#if LV_USE_DEMO_PARTY

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lvgl/lv_conf.h>
#include "lvgl/lv_port_file.h"
#include "lvgl/lv_port_indev.h"

#include "lv_demo_music_main.h"
#include "lv_demo_music_list.h"
#include "pbox_app.h"
#include "pbox_common.h"
#include "hal_drm.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
/**********************
 *  STATIC VARIABLES
 **********************/
static lv_obj_t * ctrl;
static lv_obj_t * list;

//#define TRACK_MAX_NUM 10
//#define MUSIC_PATH "/data/"
#define MAIN_FONT         "/oem/SmileySans-Oblique.ttf"
//int track_num = 0;
//char title_list[TRACK_MAX_NUM][256] = {};
char artist_list[TRACK_MAX_NUM][256] = {};
char genre_list[TRACK_MAX_NUM][256] = {};
uint32_t time_list[TRACK_MAX_NUM] = {};

lv_ft_info_t ttf_main_s;
lv_ft_info_t ttf_main_m;
// lv_ft_info_t ttf_main_l;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
static void font_init(void)
{
    //lv_freetype_init(64, 1, 0);

    ttf_main_s.name = MAIN_FONT;
    ttf_main_s.weight = 16;
    ttf_main_s.style = FT_FONT_STYLE_NORMAL;
    lv_ft_font_init(&ttf_main_s);

    ttf_main_m.name = MAIN_FONT;
    ttf_main_m.weight = 22;
    ttf_main_m.style = FT_FONT_STYLE_NORMAL;
    lv_ft_font_init(&ttf_main_m);

    // ttf_main_l.name = MAIN_FONT;
    // ttf_main_l.weight = 90;
    // ttf_main_l.style = FT_FONT_STYLE_NORMAL;
    // lv_ft_font_init(&ttf_main_l);
}

static void font_deinit(void)
{
    lv_ft_font_destroy(ttf_main_s.font);
    lv_ft_font_destroy(ttf_main_m.font);
    // lv_ft_font_destroy(ttf_main_l.font);
   // lv_freetype_destroy();
}

static void lvgl_deinit(void)
{
    lv_port_indev_deinit(0);
    lv_port_fs_deinit();
    hal_drm_deinit();
    lv_deinit();
}

static void lvgl_init(void) {
    lv_init();
    hal_drm_init(600, 360, LV_DISP_ROT_NONE);
    lv_port_fs_init();
    lv_port_indev_init(0);
}

void lv_demo_music_create(void)
{
    lvgl_init();
    font_init();

    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x343247), 0);

    //scan_dir(MUSIC_PATH, 3);

    list = _lv_demo_music_list_create(lv_scr_act());
    ctrl = _lv_demo_music_main_create(lv_scr_act());
}

void lv_demo_music_destroy(void) {
    printf("lv_demo_music_destroy\n");
    _lv_demo_music_destroy();
    if (ctrl != NULL)
        lv_obj_del(ctrl);
    if (list != NULL)
        lv_obj_del(list);
    font_deinit();
    lvgl_deinit();
}

int _lv_demo_music_get_track_num() {
    return pboxTrackdata->track_num;
}

int _lv_demo_music_get_track_id() {
    return pboxTrackdata->track_id;
}

void _lv_demo_music_update_list(uint32_t trackId) {
    _lv_demo_music_update_track_list(list);
    _lv_demo_music_update_track_info(trackId);
}

const char *_lv_demo_music_get_title(uint32_t track_id)
{
    if (track_id >= TRACK_MAX_NUM)
        return NULL;
    //return title_list[track_id];
    return pbox_app_usb_get_title(track_id);
}

const char *_lv_demo_music_get_artist(uint32_t track_id)
{
    if (track_id >= TRACK_MAX_NUM)
        return NULL;
    return artist_list[track_id];
}

const char *_lv_demo_music_get_genre(uint32_t track_id)
{
    if (track_id >= TRACK_MAX_NUM)
        return NULL;
    return genre_list[track_id];
}

uint32_t _lv_demo_music_get_track_length(uint32_t track_id)
{
    if (track_id >= TRACK_MAX_NUM)
        return 0;
    return time_list[track_id];
}

#endif /*LV_USE_DEMO_PARTY*/
