/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#include "dli_nbc_event.h"
#include "dli_log.h"
#include "dli_opcode.h"
#include "dli_errno.h"
#include "dli_event.h"

void DLI_ChanInfoCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    DLI_LOGI("chanInfo cbk in, evtOpcode: 0x%04X.", evtOpcode);
    // arg在回调入口处有做统一校验
    DLI_CHECK_RETURN(len >= sizeof(uint8_t), "check len=%u, minDataLen=%zu", len, sizeof(uint8_t));

    uint8_t subEvtCode = *(uint8_t *)arg;
    if (subEvtCode == DLI_SUBEVENT_RSSI) {
        DLI_LOGI("rssi sub event, subEvtCode: %u.", subEvtCode);
        DLI_CHECK_RETURN(len - 1 >= sizeof(DLI_RssiEvt), "check len=%u, minDataLen=%zu", len - 1, sizeof(DLI_RssiEvt));
        DLI_RssiEvt *rssi = (DLI_RssiEvt *)((uint8_t *)arg + 1);
        rssi->connHandle = DLI_DECODE2BYTE((uint8_t *)&rssi->connHandle);
        DLI_RunRegCbk(DLI_CBK_RSSI_CHANGE, context, rssi, (uint32_t)(sizeof(DLI_RssiEvt)), evtOpcode, DLI_SUCCESS);
    } else if (subEvtCode == DLI_SUBEVENT_POWER_LEVEL) {
        DLI_LOGI("power level sub event, subEvtCode: %u.", subEvtCode);
        DLI_CHECK_RETURN(len - 1 >= sizeof(DLI_PowerLevelEvt), "check len=%u, minDataLen=%zu", len - 1,
            sizeof(DLI_PowerLevelEvt));
        DLI_PowerLevelEvt *powerLevel = (DLI_PowerLevelEvt *)((uint8_t *)arg + 1);
        powerLevel->connHandle = DLI_DECODE2BYTE((uint8_t *)&powerLevel->connHandle);
        DLI_RunRegCbk(DLI_CBK_POWER_LEVEL_CHANGE, context, powerLevel, (uint32_t)(sizeof(DLI_PowerLevelEvt)),
            evtOpcode, DLI_SUCCESS);
    } else {
        DLI_LOGE("DLI_ChanInfoCbk, unknown subEvtCode: %u.", subEvtCode);
        return;
    }
}

void DLI_ChipResetNotifyCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    DLI_LOGI("chipResetNotify cbk in, evtOpcode: 0x%04X.", evtOpcode);
    DLI_CHECK_RETURN(
        len >= sizeof(DLI_ChipResetNotifyEvt), "check len=%u, minDataLen=%zu", len, sizeof(DLI_ChipResetNotifyEvt));
    DLI_ChipResetNotifyEvt *param = (DLI_ChipResetNotifyEvt *)arg;
    DLI_RunRegCbk(
        DLI_CBK_CHIP_RESET_NOTIFY, context, arg, (uint32_t)(sizeof(DLI_ChipResetNotifyEvt)), evtOpcode, param->status);
}
