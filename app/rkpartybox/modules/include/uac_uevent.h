/*
 * Copyright (C) 2019 Rockchip Electronics Co., Ltd.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL), available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef SRC_INCLUDE_UAC_UEVENT_H_
#define SRC_INCLUDE_UAC_UEVENT_H_

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

enum UAC_UEVENT_KEY {
    UAC_KEY_AUDIO = 2,
    UAC_KEY_USB_STATE = 3,
    UAC_KEY_DIRECTION = 4,
    UAC_KEY_PPM = 4,
    UAC_KEY_STREAM_STATE = 5,
    UAC_KEY_SAMPLE_RATE = UAC_KEY_STREAM_STATE,
    UAC_KEY_VOLUME      = UAC_KEY_STREAM_STATE,
    UAC_KEY_MUTE        = UAC_KEY_STREAM_STATE,
};

struct _uevent {
    char *strs[30];
    int size;
};

struct UACControl {
    void (*uacRoleChange)(uint32_t, bool);
    void (*uacSetSampleRate)(uint32_t, uint32_t);
    void (*uacSetVolume)(uint32_t, uint32_t);
    void (*uacSetMute)(uint32_t, bool);
    void (*uacSetPpm)(uint32_t, int32_t);
    void (*adbSetConnect)(bool);
};

void audio_event(const struct _uevent *uevent);
bool isUacEnabled(void);

void uac_control_create(struct UACControl *uac);
void uac_control_destory();
#ifdef __cplusplus
}
#endif
#endif  // SRC_INCLUDE_UEVENT_H_

