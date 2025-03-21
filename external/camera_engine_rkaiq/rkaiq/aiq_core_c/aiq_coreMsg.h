/*
 * Copyright (c) 2024 Rockchip Eletronics Co., Ltd.
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
 */

#ifndef _AIQ_CORE_MESSAGE_H_
#define _AIQ_CORE_MESSAGE_H_

#include "aiq_core_c/AiqCoreConfig.h"

typedef struct AiqCoreMsg_s {
    enum XCamMessageType msg_id;
    unsigned frame_id;
    char buf[64];
} AiqCoreMsg_t;

typedef struct GroupMessage_s {
    AiqCoreMsg_t msgList[MAX_MESSAGES];
    uint64_t msg_flags;
    int msg_cnts;
} GroupMessage_t;

#endif  // _AIQ_CORE_MESSAGE_H_
