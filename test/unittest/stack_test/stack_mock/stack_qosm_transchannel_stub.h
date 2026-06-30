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

#ifndef STACK_QOSM_TRANS_CHANNEL_STUB_H
#define STACK_QOSM_TRANS_CHANNEL_STUB_H


#include <stdint.h>

#include "securec.h"

#include "nai_log.h"
#include "qosm_trans_channel.h"
#include "qosm_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

uint32_t TEST_QOSM_TransChannelCbksRegister(const QOSM_TransChannelCbks_S *cbks);

uint32_t TEST_QOSM_TransChannelCbksUnregister(void);

uint32_t TEST_QOSM_TransChannelCreate(const QOSM_TransChannelParams_S *params);

uint32_t TEST_QOSM_TransChannelDestroy(const QOSM_TransChannelReleaseParams_S *params);

uint32_t TEST_QOSM_TransChannelInit(void);

void TEST_QOSM_TransChannelDeInit(void);

QOSM_TransChannelStatusCbk TEST_QOSM_TransChannel_GetStatusCbk();
#ifdef __cplusplus
}
#endif

#endif