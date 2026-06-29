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
#include "nlstk_log.h"
#include "micp_dev.h"
#include "micp_callback.h"

NLSTK_MicpCbk_S g_cbk = {0};

void MicpRegCallback(void *arg)
{
    NLSTK_MicpCbk_S *cbk = (NLSTK_MicpCbk_S *)arg;
    NLSTK_CHECK_RETURN_VOID(cbk != NULL, "[MICP] cbk is null");
    g_cbk.eventCbk = cbk->eventCbk;
    g_cbk.micStateCbk = cbk->micStateCbk;
}

void MicpConnectCbk(MicpDevice_S *device, NLSTK_MicpConnectState_E newState, uint8_t errorCode)
{
    if (g_cbk.eventCbk != NULL) {
        g_cbk.eventCbk(&device->addr, newState, device->state, errorCode);
    }
    device->state = newState;
}

void MicpMicStateCbk(MicpDevice_S *device)
{
    if (g_cbk.micStateCbk != NULL) {
        g_cbk.micStateCbk(&device->addr, device->micState);
    }
}