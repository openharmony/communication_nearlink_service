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
#include "dli_dev_discovery_event.h"

#include "sdf_mem.h"

#include "dli_log.h"
#include "dli_errno.h"
#include "dli_opcode.h"
#include "dli_event_struct.h"
#include "dli_event.h"

void DLI_AdvReportCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    DLI_LOGD("adv report cbk in, evtOpcode: 0x%04X.", evtOpcode);
    // arg在回调入口处有做统一校验
    DLI_CHECK_RETURN(len >= sizeof(DLI_AdvReportEvt), "check len=%u, minDataLen=%zu", len, sizeof(DLI_AdvReportEvt));
    DLI_AdvReportEvt *data = (DLI_AdvReportEvt *)arg;
    DLI_CHECK_RETURN(len >= sizeof(DLI_AdvReportEvt) + data->dataLength,
        "check len=%u, dataLength=%hhu", len, data->dataLength);
    uint32_t size = (uint32_t)(sizeof(DLI_AdvReportEvt) + data->dataLength);
    DLI_RunRegCbk(DLI_CBK_ADV_REPORT, context, data, size, evtOpcode, DLI_SUCCESS);
}

void DLI_AdvTerminatedCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    DLI_LOGI("adv terminated cbk in, evtOpcode: 0x%04X.", evtOpcode);
    // arg在回调入口处有做统一校验
    DLI_CHECK_RETURN(len >= sizeof(DLI_AdvertisingTerminatedEvt),
        "check len=%u, minDataLen=%zu",
        len,
        sizeof(DLI_AdvertisingTerminatedEvt));
    DLI_AdvertisingTerminatedEvt *param = (DLI_AdvertisingTerminatedEvt *)arg;
    DLI_RunRegCbk(DLI_CBK_ADV_TERMINATED,
        context,
        arg,
        (uint32_t)(sizeof(DLI_AdvertisingTerminatedEvt)),
        evtOpcode,
        param->status);
}
