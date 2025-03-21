/**
 * @file lv_demo_music_list.h
 *
 */

#ifndef LV_DEMO_MUSIC_LIST_H
#define LV_DEMO_MUSIC_LIST_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lv_demo_music.h"
#if LV_USE_DEMO_PARTY

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
lv_obj_t * _lv_demo_music_list_create(lv_obj_t * parent);
void _lv_demo_music_list_btn_check(uint32_t track_id, bool state);
void _lv_demo_music_update_track_list(lv_obj_t *list);

/**********************
 *      MACROS
 **********************/

#endif /*LV_USE_DEMO_PARTY*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_DEMO_MUSIC_LIST_H*/
