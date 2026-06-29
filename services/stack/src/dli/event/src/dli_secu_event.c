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
#include "dli_secu_event.h"
#include "dli_log.h"
#include "dli_opcode.h"
#include "dli_errno.h"
#include "dli_event.h"

void DLI_EncryptParamReqCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    DLI_LOGI("encryption parameter request cbk in, evtOpcode: 0x%04X.", evtOpcode);
    // arg在回调入口处有做统一校验
    DLI_CHECK_RETURN(
        len >= sizeof(DLI_EncryptParamReqEvt), "check len=%u, minDataLen=%zu", len, sizeof(DLI_EncryptParamReqEvt));
    DLI_RunRegCbk(
        DLI_CBK_ENCRYPT_PARAM_REQ, context, arg, (uint32_t)(sizeof(DLI_EncryptParamReqEvt)), evtOpcode, DLI_SUCCESS);
}

void DLI_EncryptChangeCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    DLI_LOGI("encryption change cbk in, evtOpcode: 0x%04X.", evtOpcode);
    // arg在回调入口处有做统一校验
    DLI_CHECK_RETURN(
        len >= sizeof(DLI_EncryptChangeEvt), "check len=%u, minDataLen=%zu", len, sizeof(DLI_EncryptChangeEvt));
    DLI_EncryptChangeEvt *param = (DLI_EncryptChangeEvt *)arg;
    if ((param->status == DLI_COMMAND_TIMEOUT || param->status == DLI_MEMORY_CAPACITY_EXCEEDED) && context != NULL) {
        DLI_ManagerContext *managerContext = (DLI_ManagerContext *)context;
        if (managerContext->cbkContext != NULL) {
            DLI_SmCbkContext *cbkContext = (DLI_SmCbkContext *)managerContext->cbkContext;
            param->connHandle = cbkContext->connHandle & 0xFFFF;
        }
    }
    DLI_RunRegCbk(
        DLI_CBK_ENCRYPT_CHANGE, context, param, (uint32_t)(sizeof(DLI_EncryptChangeEvt)), evtOpcode, param->status);
}

void DLI_ControllerDataCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    DLI_LOGI("controller data cbk in, evtOpcode: 0x%04X.", evtOpcode);
    // arg在回调入口处有做统一校验
    DLI_CHECK_RETURN(
        len >= sizeof(DLI_ControllerDataEvt), "check len=%u, minDataLen=%zu", len, sizeof(DLI_ControllerDataEvt));
    DLI_ControllerDataEvt *param = (DLI_ControllerDataEvt *)arg;
    uint32_t size = (uint32_t)(sizeof(DLI_ControllerDataEvt) + param->len);
    DLI_CHECK_RETURN(len >= size, "check len=%u, realDataLen=%zu", len, size);
    DLI_RunRegCbk(DLI_CBK_RECV_CONTROLLER_DATA, context, arg, size, evtOpcode, DLI_SUCCESS);
}