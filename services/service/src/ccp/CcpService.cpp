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
#include "CcpService.h"
#include "CdsmService.h"
#include <cstring>
#include "SleAudioFrameworkAdapter.h"
#include "SleInterfaceProfileASC.h"
#include "singleton.h"
#include "nearlink_call_client.h"
#include "telephony_errors.h"
#include "ClassCreator.h"
#include "SleServiceFfrtLog.h"
#include "ThreadUtil.h"
#include "CcpStackAdapter.h"
#include "CcpSystemInterface.h"
#include "ASCService.h"
#include "TwsService.h"
#include "nearlink_dft_ue.h"

namespace OHOS {
namespace Nearlink {
struct CcpService::impl {
    impl() = default;
    ~impl() = default;
    bool isEnabled_ = false;
    CcpStackAdapter stackAdapter_;

    class CallInfo {
    public:
        uint8_t callId;
        Telephony::TelCallState status;
    };

    /* 将CallManager中的callId与本地id进行映射 */
    NearlinkSafeMap<int32_t, CallInfo> callIdMap_;
    std::set<uint8_t> callIdSet_;
    std::set<int32_t> voipIdSet_;
    /* 当前voip通话的callId */
    int32_t currentVoipCallId_ = INVALID_CCP_VOIP_ID;
    bool isInMeetimeCall = false;

    uint8_t GetIndexForCall(const int32_t callId, const Telephony::TelCallState callStatus);
    void RemoveCallIndex(const int32_t callId);

    /* TelCallState与Stack映射表 */
    const std::unordered_map<Telephony::TelCallState, CallStatus> callStatusMap_ = {
        {Telephony::TelCallState::CALL_STATUS_INCOMING, CallStatus::CALL_STATUS_INCOMING},     // 来电暂未接通
        {Telephony::TelCallState::CALL_STATUS_DIALING, CallStatus::CALL_STATUS_OUTGOING},      // 主叫
        {Telephony::TelCallState::CALL_STATUS_ALERTING, CallStatus::CALL_STATUS_ALERTING},     // 主叫, 手机响铃
        {Telephony::TelCallState::CALL_STATUS_ACTIVE, CallStatus::CALL_STATUS_ACTIVE},         // 接通
        {Telephony::TelCallState::CALL_STATUS_HOLDING, CallStatus::CALL_STATUS_HOLDING_LOCAL}, // 多路通话, 保持第一路通话, 接通第二路
        {Telephony::TelCallState::CALL_STATUS_WAITING, CallStatus::CALL_STATUS_INCOMING},      // 主叫, 但对端正在通话
    };
};

uint8_t CcpService::impl::GetIndexForCall(const int32_t callId, const Telephony::TelCallState callStatus)
{
    CallInfo info;
    // 先从map中取，如果能获取到该callId对应的value，说明当前非呼入呼出状态，直接更新并返回对应的value
    auto func = [callId, callStatus, &info](int32_t key, CallInfo &value) {
        value.status = callStatus;
        info = value;
    };
    if (callIdMap_.GetValueAndOpt(callId, func)) {
        HILOGI("[CcpService]GetIndexForCall: telCallId=%{public}d, newCallId=%{public}d", callId, info.callId);
        return info.callId;
    }
    // 找到一个未使用的id
    for (uint8_t i = NEARLINK_CCP_ID_MIN; i < NEARLINK_CCP_ID_MAX; i++) {
        if (callIdSet_.find(i) == callIdSet_.end()) {
            info.callId = i;
            callIdSet_.insert(info.callId);
            break;
        }
    }
    // 传入的callId为key，本地自增后的id为value
    info.status = callStatus;
    callIdMap_.EnsureInsert(callId, info);
    HILOGI("[CcpService]GetIndexForCall: telCallId=%{public}d, newCallId=%{public}d", callId, info.callId);
    return info.callId;
}

void CcpService::impl::RemoveCallIndex(const int32_t callId)
{
    callIdMap_.Erase(callId);
}

CcpService::CcpService() : utility::Context(PROFILE_NAME_CCP, "1.0.0"), pimpl(std::make_unique<impl>())
{
    HILOGI("[CcpService]%{public}s Create", PROFILE_NAME_CCP.c_str());
    CcpSystemInterface::GetInstance().Start();
}

CcpService::~CcpService()
{
    HILOGI("[CcpService]%{public}s Destroy", PROFILE_NAME_CCP.c_str());
    CcpSystemInterface::GetInstance().Stop();
}

utility::Context *CcpService::GetContext()
{
    return this;
}

CcpService *CcpService::GetService()
{
    return static_cast<CcpService *>(SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_CCP));
}

void CcpService::Enable()
{
    DoInCcpThread([this]() {
        HILOGI("[CcpService] start enable ccp service");
        if (pimpl->isEnabled_) {
            HILOGW("[CcpService]CcpService has already been started before.");
            return;
        }
        // 调用协议栈进行初始化
        pimpl->stackAdapter_.CreateCcsInstance(instanceId_);

        GetContext()->OnEnable(PROFILE_NAME_CCP, true);
        pimpl->isEnabled_ = true;
        HILOGI("[CcpService]CcpService enabled");
    });
}

void CcpService::Disable()
{
    DoInCcpThread([this]() {
        HILOGI("[CcpService] start disable ccp service");
        if (!pimpl->isEnabled_) {
            GetContext()->OnDisable(PROFILE_NAME_CCP, true);
            HILOGW("[CcpService]CcpService has already been shutdown before.");
            return;
        }
        // 调用协议栈去初始化
        pimpl->stackAdapter_.DeleteCcsInstance(instanceId_);

        GetContext()->OnDisable(PROFILE_NAME_CCP, true);
        pimpl->isEnabled_ = false;
        HILOGI("[CcpService]CcpService disabled");
    });
}

void CcpService::HandleAnswer(const RawAddress &device, int32_t instanceId, uint32_t requestId, uint8_t callId)
{
    HILOGI("[CcpService]Enter");
    DoInCcpThread([this, instanceId, requestId, device]() {
        const NlErrCode ret = CcpSystemInterface::GetInstance().AnswerCall();
        RawAddress reportAddr(device);
        CdsmService *cdsmService = CdsmService::GetService();
        if (cdsmService != nullptr) {
            cdsmService->CdsmGetReportAddr(device, reportAddr);
        }
        if (ret == NL_NO_ERROR) {
            // 接听成功，触发抢占
            NotifyAudioDeviceAction(reportAddr, static_cast<int>(UpdateOutputStackAction::ACTION_USER_OPERATION));
        }
        NearlinkDftUe::GetInstance().WriteAudioControlUeAndExcep(reportAddr, CALL_CONTROL_SCENE,
                CALL_CONTROL_SUB_SCENE_ANSWER, UPDATE_VOICE_STACK_REASON_INVALID, ret);
        // 通知协议栈处理结果
        pimpl->stackAdapter_.NotifyCallControlSuccess(requestId, instanceId);
    });
}

void CcpService::HandleHangUp(const RawAddress &device, int32_t instanceId, uint32_t requestId, uint8_t callId)
{
    HILOGI("[CcpService]Enter");
    DoInCcpThread([this, instanceId, requestId, device, callId]() {
        RawAddress reportAddr(device);
        CdsmService *cdsmService = CdsmService::GetService();
        if (cdsmService != nullptr) {
            cdsmService->CdsmGetReportAddr(device, reportAddr);
        }

        // 非激活设备不处理挂断消息
        if (!IsNearlinkActiveDevice(reportAddr)) {
            HILOGI("[CcpService]HandleHangUp: %{public}s is not active device",
                GetEncryptAddr(device.GetAddress()).c_str());
            return;
        }

        Telephony::TelCallState status = Telephony::TelCallState::CALL_STATUS_DISCONNECTED;
        bool has = pimpl->callIdMap_.Find([&status, callId](int32_t key, const impl::CallInfo &info) {
            if (info.callId == callId) {
                status = info.status;
                return true;
            }
            return false;
        });
        NL_CHECK_RETURN(has, "[CcpService]can not find callId");
        if (CcpSystemInterface::GetInstance().IsRinging(status)) {
            HILOGI("[CcpService]HandleHangUp: current is ringing, do reject call");
            const NlErrCode ret = CcpSystemInterface::GetInstance().RejectCall();
            NearlinkDftUe::GetInstance().WriteAudioControlUeAndExcep(reportAddr, CALL_CONTROL_SCENE,
                CALL_CONTROL_SUB_SCENE_REJECT, UPDATE_VOICE_STACK_REASON_INVALID, ret);
        } else {
            HILOGI("[CcpService]HandleHangUp: otherwise, do hang up call");
            const NlErrCode ret = CcpSystemInterface::GetInstance().HangUpCall();
            NearlinkDftUe::GetInstance().WriteAudioControlUeAndExcep(reportAddr, CALL_CONTROL_SCENE,
                CALL_CONTROL_SUB_SCENE_HANG_UP, UPDATE_VOICE_STACK_REASON_INVALID, ret);
        }
        // 通知协议栈处理结果
        pimpl->stackAdapter_.NotifyCallControlSuccess(requestId, instanceId);
    });
}

bool CcpService::IsNearlinkActiveDevice(const RawAddress &reportAddr)
{
    ASCService *ascService = ASCService::GetService();
    NL_CHECK_RETURN_RET(ascService, false, "cant find ASC service");

    // 获取到的是report地址
    NearlinkRawAddress activeDevice = ascService->GetActiveSinkDevice();

    /* 是否星闪设备出声 */
    bool isNearlinkOut = SleAudioFrameworkAdapter::GetInstance().IsNearlinkOut();
    return isNearlinkOut && (activeDevice.GetAddress() == reportAddr.GetAddress());
}

void CcpService::NotifyAudioDeviceAction(const RawAddress &devAddr, int action)
{
    HILOGI("[CcpService]:NotifyAudioDeviceAction, addr:%{public}s, action:%{public}d",
        GetEncryptAddr(devAddr.GetAddress()).c_str(), action);
    ASCService *ascService = ASCService::GetService();
    NL_CHECK_RETURN(ascService, "cant find ASC service");
    ascService->SleAudioDeviceActionChanged(NearlinkRawAddress(devAddr), action);
}

void CcpService::HandleStartInstance(int32_t instanceId, bool isSuccess)
{
    HILOGI("[CcpService]Enter");
}

void CcpService::HandleAuthorize(int32_t instanceId, uint32_t requestId, int32_t property)
{
    HILOGI("[CcpService]Enter");
    DoInCcpThread([this, instanceId, requestId, property]() {
        // 默认支持授权
        pimpl->stackAdapter_.AuthorizeResult(instanceId, requestId, property);
    });
}

CallFlagBits CcpService::GetCallFlag(const CallAttributeInfo &info)
{
    // bit0: 0表示呼入，1表示呼出。
    // bit1: 0表示呼入用户信息未在网络侧隐藏，1表示呼入用户的信息被网络侧隐藏。当通话为呼出通话时，该位应置0。
    // bit2: 0表示服务端未隐藏用户信息，1表示服务端隐藏了用户信息。
    // 当bit1或bit2任一置1时，不携带用户标识和用户别名字段。
    CallFlagBits callFlag{};
    if (info.callDirection == Telephony::CallDirection::CALL_DIRECTION_OUT) {   // 呼出
        callFlag.bits.callDirect = static_cast<uint8_t>(BitSet::USE);
        callFlag.bits.hideUserInfoInNet = static_cast<uint8_t>(BitSet::RESET);
    } else {    // 呼入
        callFlag.bits.callDirect = static_cast<uint8_t>(BitSet::RESET);
        if (strlen(info.accountNumber) == 0) {
            // 传入的用户信息为空，置为1
            callFlag.bits.hideUserInfoInNet = static_cast<uint8_t>(BitSet::USE);
        } else {
            callFlag.bits.hideUserInfoInNet = static_cast<uint8_t>(BitSet::RESET);
        }
    }
    // 默认服务端不隐藏用户信息
    callFlag.bits.hideUserInfoInServer = static_cast<uint8_t>(BitSet::RESET);
    return callFlag;
}

void CcpService::HandlePhoneStateChange(const NearlinkCallPhoneState &phoneState)
{
    int32_t numActive = phoneState.GetActiveNum();
    int32_t numHeld = phoneState.GetHeldNum();
    int32_t callState = phoneState.GetCallState();
    int32_t callType = phoneState.GetCallType();
    HILOGI("[CcpService]numActive: %{public}d, numHeld: %{public}d, callState: %{public}d, type: %{public}d",
        numActive, numHeld, callState, callType);
    std::vector<Telephony::CallAttributeInfo> callInfoVec = CcpSystemInterface::GetInstance().GetCurrentCallList();
    /* 即使当前没有通话了，更新通话状态时，也需要将当前通话列表为0更新给对端 */
    /* 更新通话状态 */
    ProcessCallStateInfo(callInfoVec);
}

void CcpService::HandleCallDetailChange(const Telephony::CallAttributeInfo &info)
{
    ProcessCallDetailChange(info);
}

void CcpService::HandleMeeTimeDetailsChange(const Telephony::CallAttributeInfo &info)
{
    switch (info.callState) {
        case TelCallState::CALL_STATUS_DIALING:
        case TelCallState::CALL_STATUS_INCOMING:
            pimpl->isInMeetimeCall = true;
            break;
        case TelCallState::CALL_STATUS_DISCONNECTED:
            /* 通话挂断 */
            pimpl->isInMeetimeCall = false;
            break;
        default:
            break;
    }
    ProcessCallDetailChange(info);
}

void CcpService::ProcessCallDetailChange(const Telephony::CallAttributeInfo &info)
{
    HILOGI("[CcpService]ProcessCallDetailChange: id=%{public}d, state=%{public}d", info.callId, info.callState);
    switch (info.callState) {
        case TelCallState::CALL_STATUS_DIALING:
        case TelCallState::CALL_STATUS_INCOMING:
        case TelCallState::CALL_STATUS_WAITING:
            /* 更新呼入呼出信息 */
            ProcessCallInOutInfo(info);
            break;
        case TelCallState::CALL_STATUS_DISCONNECTED: {
            /* 更新通话终止信息 */
            ProcessCallTerminateInfo(info);
            /* 更新挂断时间戳 */
            ASCService *ascService = ASCService::GetService();
            TwsService *twsService = TwsService::GetService();
            if (ascService != nullptr && twsService != nullptr) {
                NearlinkRawAddress device = ascService->GetActiveSinkDevice(); // 获取到的是report地址
                if (!device.GetAddress().empty()) {
                    RawAddress peerAddr(device.GetAddress());
                    twsService->UpdateHangUpTimeStamp(peerAddr);
                }
            }
            break;
        }
        default:
            HILOGW("[CcpService]abort this state");
            break;
    }
}

void CcpService::TryResumeCurrentCalls()
{
    HILOGD("[CcpService]Enter");
    DoInCcpThread([this] {
        // 恢复时不需要考虑VoIP，因为耳机会重连走重新起流
        std::vector<Telephony::CallAttributeInfo> callVec = CcpSystemInterface::GetInstance().GetCurrentCallList();
        for (auto &callAttr : callVec) {
            // 过滤掉 VoIP 通话
            if (callAttr.callType == Telephony::CallType::TYPE_VOIP) {
                continue;
            }
            ProcessCallDetailChange(callAttr);
        }
        ProcessCallStateInfo(callVec);
    });
}

void CcpService::ProcessCallStateInfo(std::vector<Telephony::CallAttributeInfo> callInfoVec)
{
    HILOGD("[CcpService]ProcessCallStateInfo current callInfos size=%{public}d", callInfoVec.size());
    std::shared_ptr<TotalCallStateProp> prop = std::make_shared<TotalCallStateProp>();
    for (auto &callAttr : callInfoVec) {
        TotalCallStateProp::SingleCallStateProp singleCallState;
        // 将CallManager的state转换为协议定义的值
        auto iter = pimpl->callStatusMap_.find(callAttr.callState);
        if (iter == pimpl->callStatusMap_.end()) {
            HILOGW("[CcpService]not support state, skip update this call: callId=%{public}d, callState=%{public}d",
                callAttr.callId, callAttr.callState);
            continue;
        }
        // 通话标识
        singleCallState.callId = pimpl->GetIndexForCall(callAttr.callId, callAttr.callState);
        // 通话状态
        singleCallState.callStatus = static_cast<uint8_t>(iter->second);
        // 通话标记
        singleCallState.callFlag = GetCallFlag(callAttr).value;
        HILOGI("[CcpService] callId=%{public}d, telStatus=%{public}d, ccpStatus=%{public}d, flag=0x%{public}02x",
            singleCallState.callId, callAttr.callState, singleCallState.callStatus, singleCallState.callFlag);
        prop->callStateVec.push_back(singleCallState);
    }

    bool hasVoipCallId = HasVoipCallId();
    if (hasVoipCallId && (callInfoVec.size() == 0 || prop->callStateVec.size() == 0)) {
        // 正在voip通话且callInfoVec size为0，不更新通话状态
        HILOGI("[CcpService]:hasVoipCallId");
        return;
    }
    pimpl->stackAdapter_.UpdateCallStateInfo(instanceId_, prop);
}

bool CcpService::HasVoipCallId()
{
    bool hasVoipCallId = pimpl->callIdMap_.Find([](int32_t key, const impl::CallInfo &value) {
        if (key < 0) {
            return true;
        }
        return false;
    });
    return hasVoipCallId;
}

bool CcpService::ProcessAccountInfo(std::shared_ptr<CallInOutInfoProp> prop, const Telephony::CallAttributeInfo &info)
{
    /* 用户信息 */
    // CallManage中获取到的电话号码信息，无法确定是否携带国家码
    // 1.先拷贝协议字段到目标缓冲区
    if (strncpy_s(prop->userInfo, MAX_USER_INFO_LEN, CCP_SUPPORT_PROTOCOLS, strlen(CCP_SUPPORT_PROTOCOLS)) != EOK) {
        HILOGE("[CcpService]: memcpy protocol error");
        return false;
    }
    // 2.检查是否可以安全拼接
    size_t destLen = strlen(prop->userInfo);
    size_t srcLen = strlen(info.accountNumber);

    if (destLen + srcLen >= MAX_USER_INFO_LEN) {
        HILOGE("[CcpService]: Buffer overflow occurred during concatenation: destLen=%{public}d, srcLen=%{public}d",
            destLen, srcLen);
        return false;
    }
    // 3.使用 strcat_s 进行安全拼接
    if (strcat_s(prop->userInfo, MAX_USER_INFO_LEN, info.accountNumber) != EOK) {
        HILOGE("[CcpService]: concat number error");
        return false;
    }

    /* 用户别名 */
    size_t contactNameLen = strlen(info.contactName);
    NL_CHECK_RETURN_RET(contactNameLen != 0, true, "user contact name is empty.");
    size_t contactNameCpyLen = (contactNameLen < MAX_USER_INFO_LEN) ? contactNameLen : MAX_USER_INFO_LEN;
    if (strncpy_s(prop->userAlias, MAX_USER_INFO_LEN + 1, info.contactName, contactNameCpyLen) != EOK) {
        HILOGE("[CcpService]: memcpy alias error");
        return false;
    }
    return true;
}

void CcpService::ProcessCallInOutInfo(const Telephony::CallAttributeInfo &info)
{
    HILOGI("[CcpService]ProcessCallInOutInfo: id=%{public}d, telStatus=%{public}d", info.callId, info.callState);
    std::shared_ptr<CallInOutInfoProp> prop = std::make_shared<CallInOutInfoProp>();
    /**< 通话标识 */
    prop->callId = pimpl->GetIndexForCall(info.callId, info.callState);
    /**< 通话标记 */
    CallFlagBits flag = GetCallFlag(info);
    prop->callFlag = flag.value;
    // 当bit1或bit2任一置1时，不携带用户标识和用户别名字段 ---> 二者都为0，携带相关信息
    if (!flag.bits.hideUserInfoInNet && !flag.bits.hideUserInfoInServer) {
        NL_CHECK_RETURN(ProcessAccountInfo(prop, info), "ProcessAccountInfo error.");
    }
    HILOGI("[CcpService]ProcessCallInOutInfo id=0x%{public}02x, flag=0x%{public}02x", prop->callId, prop->callFlag);
    pimpl->stackAdapter_.UpdateCallInOutInfo(instanceId_, prop);
}

void CcpService::ProcessCallTerminateInfo(const Telephony::CallAttributeInfo &info)
{
    HILOGI("[CcpService]ProcessCallTerminateInfo: id=%{public}d, telStatus=%{public}d", info.callId, info.callState);
    std::shared_ptr<CallTerminateInfoProp> prop = std::make_shared<CallTerminateInfoProp>();
    // 通话标识
    prop->callId = pimpl->GetIndexForCall(info.callId, info.callState);
    // 原因(no use)
    prop->terminateReason = 0x01;  // CallManage当前无法提供确切的断开原因
    pimpl->stackAdapter_.UpdateCallTerminateInfo(instanceId_, prop);
    bool hasVoipCallId = HasVoipCallId();
    if (info.callId > 0 && !hasVoipCallId && !pimpl->voipIdSet_.empty()) {
        CreateVoipCallInfo();
    }
    pimpl->RemoveCallIndex(info.callId);
    pimpl->callIdSet_.erase(prop->callId);
}

void CcpService::HandleVoipStart(const RawAddress &device)
{
    HILOGI("[CcpService]Enter");
    DoInCcpThread([this]() {
        NL_CHECK_RETURN(!pimpl->isInMeetimeCall, "Now Is in meetime, not need to create new call state.");
        // 避免自己造的这个callId和后续蜂窝的CallId重复，避开蜂窝的id区间
        int32_t voipId = NEARLINK_CCP_VOIP_MAX - 1;
        pimpl->currentVoipCallId_ = voipId;
        pimpl->voipIdSet_.insert(voipId);
        bool hasPhoneCallId = pimpl->callIdMap_.Find([](int32_t key, const impl::CallInfo &info) {
            if (key > 0) {
                return true;
            }
            return false;
        });
        NL_CHECK_RETURN(!hasPhoneCallId, "Now is in tellphone call, no need to create voip state.");
        NL_CHECK_RETURN(pimpl->currentVoipCallId_ != INVALID_CCP_VOIP_ID, "can not assign callId.");
        CreateVoipCallInfo();
    });
}

void CcpService::HandleVoipStop(const RawAddress &device)
{
    HILOGI("[CcpService]Enter");
    DoInCcpThread([this]() {
        if (pimpl->isInMeetimeCall) {
            HILOGI("[CcpService]Now Is in meetime, not need to create new call state");
            pimpl->isInMeetimeCall = false;
            return;
        }
        Telephony::CallAttributeInfo info;
        info.callState = TelCallState::CALL_STATUS_DISCONNECTED;
        info.callId = pimpl->currentVoipCallId_;
        HandleCallDetailChange(info);
        pimpl->voipIdSet_.erase(pimpl->currentVoipCallId_);
        HILOGI("[CcpService]HandleVoipStop: id=%{public}d", info.callId);
        pimpl->currentVoipCallId_ = INVALID_CCP_VOIP_ID;
        std::vector<Telephony::CallAttributeInfo> callInfoVec = CcpSystemInterface::GetInstance().GetCurrentCallList();
        /* 即使当前没有通话了，更新通话状态时，也需要将当前通话列表为0更新给对端 */
        /* 更新通话状态 */
        ProcessCallStateInfo(callInfoVec);
    });
}

void CcpService::CreateVoipCallInfo()
{
    // 呼出（这里只是模拟一个通话状态，来去电都可以）
    Telephony::CallAttributeInfo info;
    info.callState = TelCallState::CALL_STATUS_DIALING;
    info.callDirection = Telephony::CallDirection::CALL_DIRECTION_OUT;
    info.callId = pimpl->currentVoipCallId_;
    HILOGI("[CcpService]HandleVoipStart: id=%{public}d", info.callId);
    HandleCallDetailChange(info);
    // 响铃
    std::vector<Telephony::CallAttributeInfo> callVec = CcpSystemInterface::GetInstance().GetCurrentCallList();
    info.callState = TelCallState::CALL_STATUS_ALERTING;
    callVec.push_back(info);
    ProcessCallStateInfo(callVec);
    callVec.pop_back();
    // 接通
    info.callState = TelCallState::CALL_STATUS_ACTIVE;
    callVec.push_back(info);
    ProcessCallStateInfo(callVec);
}

REGISTER_CLASS_CREATOR(CcpService);

}  // namespace Nearlink
}  // namespace OHOS
