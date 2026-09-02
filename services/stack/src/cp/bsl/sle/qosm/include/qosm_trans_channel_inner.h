/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef QOSM_TRANS_CHANNEL_INNER_H
#define QOSM_TRANS_CHANNEL_INNER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t lcid;
    uint8_t role;
    SLE_Addr_S addr;
    QOSM_TransChannelSlqi_E slqi;
    SDF_DListHead_S list;
} QOSM_LogicLink_S;

typedef QOSM_LogicLink_S *(*QOSM_LogicLinkFindCbk)(uint16_t lcid);
typedef void (*QOSM_DecreaseChannelSizeCbk)(uint16_t lcid);

#ifdef __cplusplus
}
#endif

#endif  // QOSM_TRANS_CHANNEL_INNER_H