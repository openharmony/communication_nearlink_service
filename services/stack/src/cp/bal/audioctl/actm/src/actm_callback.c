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
#include "actm_callback.h"
#include "nlstk_log.h"

static NLSTK_ActmCbk_S g_Cbk;

void ActmSetCallback(NLSTK_ActmCbk_S *cbk)
{
    NLSTK_CHECK_RETURN_VOID(cbk != NULL, "[ACTM] cbk is null");
    g_Cbk.eventCbk = cbk->eventCbk;
    g_Cbk.propCbk = cbk->propCbk;
    g_Cbk.bitCbk = cbk->bitCbk;
    g_Cbk.locationCbk = cbk->locationCbk;
    g_Cbk.callBitUpDownCbk = cbk->callBitUpDownCbk;
}

void ActmEventCbk(SLE_Addr_S *addr, uint8_t event, uint8_t result, void *param)
{
    if (g_Cbk.eventCbk != NULL) {
        g_Cbk.eventCbk(addr, event, result, param);
    }
}

void ActmPropCbk(SLE_Addr_S *addr, uint8_t num, NLSTK_ActmProp_S *prop)
{
    if (g_Cbk.propCbk != NULL) {
        g_Cbk.propCbk(addr, num, prop);
    }
}

void ActmBitCbk(NLSTK_ActmBitrateChange_S *bitrate)
{
    if (g_Cbk.bitCbk != NULL) {
        g_Cbk.bitCbk(bitrate);
    }
}

void ActmLocationCbk(SLE_Addr_S *addr, bool isLeft)
{
    if (g_Cbk.locationCbk != NULL) {
        g_Cbk.locationCbk(addr, isLeft);
    }
}

void ActmStreamTypeCbk(SLE_Addr_S *addr, uint32_t availableStreamType)
{
    if (g_Cbk.streamTypeCbk != NULL) {
        g_Cbk.streamTypeCbk(addr, availableStreamType);
    }
}

void ActmCallBitUpDownCbk(NLSTK_ActmAutoRateSendMsg_S *upDownParam)
{
    if (g_Cbk.callBitUpDownCbk != NULL) {
        g_Cbk.callBitUpDownCbk(upDownParam);
    }
}