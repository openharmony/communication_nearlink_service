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
#include "CcpSystemInterface.h"

#include "singleton.h"
#include "iservice_registry.h"
#include "nearlink_call_client.h"
#include "telephony_errors.h"
#include "ThreadUtil.h"
#include "ASCService.h"
#include "nearlink_dft_exception.h"

namespace OHOS {
namespace Nearlink {
CcpSystemInterface &CcpSystemInterface::GetInstance()
{
    static CcpSystemInterface ccpSystemInterface;
    return ccpSystemInterface;
}

void CcpSystemInterface::Start()
{
    HILOGI("[CcpService]enter");
    sptr<ISystemAbilityManager> samgrProxy = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgrProxy == nullptr) {
        HILOGE("[CcpService]failed to get samgrProxy");
        return;
    }
    statusChangeListener_ = sptr<SystemAbilityStatusChange>::MakeSptr();
    NL_CHECK_RETURN(statusChangeListener_, "statusChangeListener_ is nullptr.");
    int32_t ret = samgrProxy->SubscribeSystemAbility(TELEPHONY_CALL_MANAGER_SYS_ABILITY_ID, statusChangeListener_);
    if (ret != ERR_OK) {
        HILOGE("[CcpService]subscribe systemAbilityId: call manager service failed!");
        statusChangeListener_ = nullptr;
        return;
    }
}

void CcpSystemInterface::Stop()
{
    HILOGI("[CcpService]enter");
    sptr<ISystemAbilityManager> samgrProxy = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    NL_CHECK_RETURN(samgrProxy, "failed to get samgrProxy");
    NL_CHECK_RETURN(statusChangeListener_, "statusChangeListener_ is nullptr.");
    samgrProxy->UnSubscribeSystemAbility(TELEPHONY_CALL_MANAGER_SYS_ABILITY_ID, statusChangeListener_);
}

void CcpSystemInterface::InitNearlinkCallClient()
{
    HILOGI("[CcpService]enter");
    DelayedRefSingleton<Telephony::NearlinkCallClient>::GetInstance().Init();
    int32_t ret = DelayedRefSingleton<Telephony::NearlinkCallClient>::GetInstance().RegisterCallBack(
        std::make_unique<CallManagerCallbackImpl>());
    if (ret != Telephony::TELEPHONY_SUCCESS) {
        DftReportAudioError("", ERROR_TELEPHONY, SUB_ERRCODE_CASE1);
        HILOGE("[CcpService] NearlinkCallClient register telephony callback fail, ret = %{public}d", ret);
    }
}

void CcpSystemInterface::DeInitNearlinkCallClient()
{
    HILOGI("[CcpService]enter");
    DelayedRefSingleton<Telephony::NearlinkCallClient>::GetInstance().UnInit();
    int32_t ret = DelayedRefSingleton<Telephony::NearlinkCallClient>::GetInstance().UnRegisterCallBack();
    if (ret != Telephony::TELEPHONY_SUCCESS) {
        DftReportAudioError("", ERROR_TELEPHONY, SUB_ERRCODE_CASE1);
        HILOGE("[CcpService] unregister telephony callback fail, ret = %{public}d", ret);
    }
}

bool CcpSystemInterface::CheckNowHasDeviceConnected()
{
    ProfileASC *ascService = ASCService::GetService();
    NL_CHECK_RETURN_RET(ascService, false, "[CcpService]Get ascService error!");
    // CCP做服务端，Service中不感知当前已连接的设备，所以通过ASC中获取来判断
    std::list<RawAddress> connectDevices = ascService->GetConnectDevices();
    return !connectDevices.empty();
}

bool CcpSystemInterface::IsRinging(Telephony::TelCallState state) const
{
    return (state == Telephony::TelCallState::CALL_STATUS_INCOMING) ||
        (state == Telephony::TelCallState::CALL_STATUS_WAITING);
}

NlErrCode CcpSystemInterface::AnswerCall()
{
    HILOGI("[CcpService]enter");
    int32_t ret = DelayedRefSingleton<Telephony::NearlinkCallClient>::GetInstance().AnswerCall();
    if (ret != Telephony::TELEPHONY_SUCCESS) {
        HILOGE("[CcpService] process telephony answer fail, ret = %{public}d", ret);
        return NL_ERR_INTERNAL_ERROR;
    }
    return NL_NO_ERROR;
}

NlErrCode CcpSystemInterface::HangUpCall()
{
    HILOGI("[CcpService]enter");
    int32_t ret = DelayedRefSingleton<Telephony::NearlinkCallClient>::GetInstance().HangUpCall();
    if (ret != Telephony::TELEPHONY_SUCCESS) {
        HILOGE("[CcpService] process telephony hang up fail, ret = %{public}d", ret);
        return NL_ERR_INTERNAL_ERROR;
    }
    return NL_NO_ERROR;
}

NlErrCode CcpSystemInterface::RejectCall()
{
    HILOGI("[CcpService]enter");
    int32_t ret = DelayedRefSingleton<Telephony::NearlinkCallClient>::GetInstance().RejectCall();
    if (ret != Telephony::TELEPHONY_SUCCESS) {
        HILOGE("[CcpService] process telephony reject fail, ret = %{public}d", ret);
        return NL_ERR_INTERNAL_ERROR;
    }
    return NL_NO_ERROR;
}

std::vector<Telephony::CallAttributeInfo> CcpSystemInterface::GetCurrentCallList()
{
    HILOGD("[CcpService]enter");
    return DelayedRefSingleton<Telephony::NearlinkCallClient>::GetInstance().GetCurrentCallList(slotId_);
}

void CcpSystemInterface::SystemAbilityStatusChange::OnAddSystemAbility(
    int32_t systemAbilityId, const std::string &deviceId)
{
    HILOGI("[CcpService]systemAbilityId(%{public}d) is added", systemAbilityId);
    DoInCcpThread([systemAbilityId]() {
        if (systemAbilityId == TELEPHONY_CALL_MANAGER_SYS_ABILITY_ID) {
            HILOGI("[CcpService]OnAddSystemAbility: call manager service add");
            CcpSystemInterface::GetInstance().InitNearlinkCallClient();
        } else {
            HILOGW("[CcpService]unhandled sysabilityId:%{public}d", systemAbilityId);
        }
    });
}

void CcpSystemInterface::SystemAbilityStatusChange::OnRemoveSystemAbility(
    int32_t systemAbilityId, const std::string &deviceId)
{
    HILOGI("[CcpService]systemAbilityId(%{public}d) is removed", systemAbilityId);
    DoInCcpThread([systemAbilityId]() {
        if (systemAbilityId == TELEPHONY_CALL_MANAGER_SYS_ABILITY_ID) {
            HILOGI("[CcpService]OnRemoveSystemAbility: call manager service remove");
            CcpSystemInterface::GetInstance().DeInitNearlinkCallClient();
        } else {
            HILOGW("[CcpService]unhandled sysabilityId:%{public}d", systemAbilityId);
        }
    });
}

// 蜂窝通话的状态回调
int32_t CcpSystemInterface::CallManagerCallbackImpl::OnPhoneStateChange(
    int32_t numActive, int32_t numHeld, int32_t callState, const std::string &number)
{
    HILOGI("[CcpService]numActive=%{public}d, numHeld=%{public}d, state=%{public}d", numActive, numHeld, callState);
    DoInCcpThread([numActive, numHeld, callState, number]() {
        CcpService *service = CcpService::GetService();
        NL_CHECK_RETURN(service, "[CcpService]ccpService is null.");
        NearlinkCallPhoneState state;
        state.SetActiveNum(numActive);
        state.SetHeldNum(numHeld);
        state.SetNumber(number);
        service->HandlePhoneStateChange(state);
    });
    return NL_NO_ERROR;
}

// 只回调某一路的回调信息
int32_t CcpSystemInterface::CallManagerCallbackImpl::OnCallDetailsChange(const Telephony::CallAttributeInfo &info)
{
    HILOGI("[CcpService]OnCallDetailsChange: id=%{public}d, state=%{public}d", info.callId, info.callState);
    NL_CHECK_RETURN_RET(info.callType != Telephony::CallType::TYPE_VOIP, NL_NO_ERROR, "not support voip call state.");
    DoInCcpThread([info]() {
        CcpService *service = CcpService::GetService();
        NL_CHECK_RETURN(service, "[CcpService]ccpService is null.");
        service->HandleCallDetailChange(info);
    });
    return NL_NO_ERROR;
}

int32_t CcpSystemInterface::CallManagerCallbackImpl::OnMeeTimeDetailsChange(const Telephony::CallAttributeInfo &info)
{
    HILOGI("[CcpService]OnMeeTimeDetailsChange: id=%{public}d, state=%{public}d", info.callId, info.callState);
    DoInCcpThread([info]() {
        CcpService *service = CcpService::GetService();
        NL_CHECK_RETURN(service, "[CcpService]ccpService is null.");
        service->HandleMeeTimeDetailsChange(info);
    });
    return NL_NO_ERROR;
}
}  // namespace Nearlink
}  // namespace OHOS
