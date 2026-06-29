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
#include "VasStackAdapter.h"

#include <cstring>

#include "nearlink_dft_exception.h"
#include "VasService.h"
#include "SleServiceFfrtLog.h"
#include "nlstk_public_define.h"

namespace OHOS {
namespace Nearlink {
VasStackAdapter::VasStackAdapter()
{
    HILOGI("[VasService]Enter");
}

VasStackAdapter::~VasStackAdapter()
{
    HILOGI("[VasService]Enter");
}

static void VasActivateCbk(SLE_Addr_S *addr, uint32_t requestId)
{
    NL_CHECK_RETURN(addr, "addr is null.");
    const RawAddress &device = RawAddress::ConvertToString(addr->addr);
    HILOGI("[VasService]VasActivateCbk: device=%{public}s, id=%{public}d", GET_ENCRYPT_ADDR(device), requestId);
    VasService *vasService = VasService::GetService();
    NL_CHECK_RETURN(vasService, "vasService is null.");
    vasService->HandleActivateVoiceAssistant(device, requestId);
}

static void VasTerminateCbk(SLE_Addr_S *addr, uint32_t requestId)
{
    NL_CHECK_RETURN(addr, "addr is null.");
    const RawAddress &device = RawAddress::ConvertToString(addr->addr);
    HILOGI("[VasService]VasTerminateCbk: device=%{public}s, id=%{public}d", GET_ENCRYPT_ADDR(device), requestId);
    VasService *vasService = VasService::GetService();
    NL_CHECK_RETURN(vasService, "vasService is null.");
    vasService->HandleCloseVoiceAssistant(device, requestId);
}

void VasStackAdapter::CreateVasInstance()
{
    NLSTK_CcpVasInfo_S vasInfo{};
    vasInfo.state = NLSTK_VAS_STATE_IDLE;
    vasInfo.stateRight = NLSTK_VAS_READ_AUTHEN | NLSTK_VAS_READ_ENCRYPT;
    vasInfo.vasControlPoint.activate = &VasActivateCbk;
    vasInfo.vasControlPoint.terminate = &VasTerminateCbk;
    uint32_t ret = NLSTK_CcpCreateVoiceAssistantService(&vasInfo);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        DftReportAudioError("", ERROR_NEARLINK_INNER, SUB_ERRCODE_CASE1);
        HILOGE("[VasService] Create vas instance fail, ret = 0x%{public}x", ret);
        return;
    }
    HILOGI("[VasService]Init vas instance success");
}

void VasStackAdapter::DeleteVasInstance()
{
    uint32_t ret = NLSTK_CcpDeleteVoiceAssistantService();
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        DftReportAudioError("", ERROR_NEARLINK_INNER, SUB_ERRCODE_CASE1);
        HILOGE("[VasService] Delete vas instance fail, ret = 0x%{public}x", ret);
        return;
    }
    HILOGI("[VasService]Delete vas instance success");
}

void VasStackAdapter::SetVoiceAssistantOpened()
{
    HILOGI("[VasService]Enter");
    UpdateVoiceAssistantState(NLSTK_VAS_STATE_ACTIVATED);
}

void VasStackAdapter::SetVoiceAssistantClosed()
{
    HILOGI("[VasService]Enter");
    UpdateVoiceAssistantState(NLSTK_VAS_STATE_IDLE);
}

void VasStackAdapter::OpenVoiceAssistantSuccess(uint32_t requestId)
{
    HILOGI("[VasService]Enter");
    VasControlResult(requestId, NLSTK_CCP_VAS_ACTIVATE, NLSTK_VAS_CONTROL_SUCCESS);
}

void VasStackAdapter::OpenVoiceAssistantFail(uint32_t requestId)
{
    HILOGI("[VasService]Enter");
    VasControlResult(requestId, NLSTK_CCP_VAS_ACTIVATE, NLSTK_VAS_CONTROL_FAIL);
}

void VasStackAdapter::CloseVoiceAssistantSuccess(uint32_t requestId)
{
    HILOGI("[VasService]Enter");
    VasControlResult(requestId, NLSTK_CCP_VAS_TERMINATE, NLSTK_VAS_CONTROL_SUCCESS);
}

void VasStackAdapter::CloseVoiceAssistantFail(uint32_t requestId)
{
    HILOGI("[VasService]Enter");
    VasControlResult(requestId, NLSTK_CCP_VAS_TERMINATE, NLSTK_VAS_CONTROL_FAIL);
}

void VasStackAdapter::VasControlResult(uint32_t requestId, uint8_t opCode, uint8_t errorCode)
{
    uint32_t ret = NLSTK_CcpVasControlResult(requestId, opCode, errorCode);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        DftReportAudioError("", ERROR_NEARLINK_INNER, SUB_ERRCODE_CASE1);
        HILOGE("[VasService] send vas control result fail, ret = 0x%{public}x", ret);
        return;
    }
    HILOGI("[VasService]send vas control result success");
}

void VasStackAdapter::UpdateVoiceAssistantState(NLSTK_CcpVasState_E state)
{
    uint32_t ret = NLSTK_CcpUpdateVasState(state);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        DftReportAudioError("", ERROR_NEARLINK_INNER, SUB_ERRCODE_CASE1);
        HILOGE("[VasService] update vas state fail, ret = 0x%{public}x", ret);
        return;
    }
    HILOGI("[VasService]update vas state success");
}

}  // namespace Nearlink
}  // namespace OHOS
