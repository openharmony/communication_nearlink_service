/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include <stdint.h>
#include "stack_qosm_transchannel_stub.h"

QOSM_TransChannelCbks_S g_cbks;
uint32_t TEST_QOSM_TransChannelCbksRegister(const QOSM_TransChannelCbks_S *cbks)
{
    g_cbks = *cbks;
    return 0;
}

uint32_t TEST_QOSM_TransChannelCbksUnregister(void)
{
    return 0;
}

uint32_t TEST_QOSM_TransChannelCreate(const QOSM_TransChannelParams_S *params)
{
    return 0;
}

uint32_t TEST_QOSM_TransChannelDestroy(const QOSM_TransChannelReleaseParams_S *params)
{
    return 0;
}

uint32_t TEST_QOSM_TransChannelInit(void)
{
    return 0;
}

void TEST_QOSM_TransChannelDeInit(void) {};

QOSM_TransChannelStatusCbk TEST_QOSM_TransChannel_GetStatusCbk()
{
    return g_cbks.statusCbk;
}