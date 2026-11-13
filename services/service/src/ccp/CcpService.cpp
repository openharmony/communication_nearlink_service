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
#include "CcpService.h"
#include "CdsmService.h"
#include <algorithm>
#include <cstring>
#include "SleInterfaceProfileASC.h"
#include "SleInterfaceProfileManager.h"
#include "SleInterfaceProfileTws.h"
#include "singleton.h"
#include "nearlink_call_client.h"
#include "telephony_errors.h"
#include "ClassCreator.h"
#include "SleServiceFfrtLog.h"
#include "ThreadUtil.h"
#include "CcpStackAdapter.h"
#include "CcpSystemInterface.h"
#include "nearlink_dft_ue.h"
#include "ServiceManagerPluginLoader.h"

namespace OHOS {
namespace Nearlink {
struct CcpService::impl {
    impl() = default;
    ~impl() = default;
    bool isEnabled_ = false;
    CcpStackAdapter stackAdapter_;

    class CallInfo {
    public:
        uint8_t callId{0};
        Telephony::TelCallState status{Telephony::TelCallState::CALL_STATUS_UNKNOWN};
    };

    /* 将CallManager中的callId与本地id进行映射 */
    NearlinkSafeMap<int32_t, CallInfo> callIdMap_;
    std::set<uint8_t> callIdSet_;
    std::set<int32_t> voipIdSet_;
    /* 当前voip通话的callId */
    int32_t currentVoipCallId_ = INVALID_CCP_VOIP_ID;

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
    CallInfo info{};
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
            info.status = callStatus;
            callIdSet_.insert(info.callId);
            // 传入的callId为key，本地自增后的id为value
            callIdMap_.EnsureInsert(callId, info);
            break;
        }
    }
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
        // 清理通话缓存信息
        pimpl->callIdMap_.Clear();
        pimpl->callIdSet_.clear();
        pimpl->voipIdSet_.clear();
        pimpl->currentVoipCallId_ = INVALID_CCP_VOIP_ID;
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

void CcpService::NotifyAudioDeviceAction(const RawAddress &devAddr, int action)
{
    HILOGI("[CcpService]:NotifyAudioDeviceAction, addr:%{public}s, action:%{public}d",
        GetEncryptAddr(devAddr.GetAddress()).c_str(), action);
    ProfileASC *ascService = static_cast<ProfileASC *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_ASC));
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

void CcpService::PhoneStateChanged(int32_t callId, int32_t numActive, int32_t numHeld, int32_t callState,
    const std::string &number)
{
    // numActive/numHeld 仅作诊断打印：通话数以下方回查的 GetCurrentCallList() 为准， 调用方传入值不参与状态计算
    HILOGI("[CcpService]PhoneStateChanged: numActive=%{public}d, numHeld=%{public}d, state=%{public}d, "
        "callId=%{public}d", numActive, numHeld, callState, callId);
    // 一次查询同时用于全量状态与单路明细，避免重复 IPC
    std::vector<Telephony::CallAttributeInfo> callInfoVec = CcpSystemInterface::GetInstance().GetCurrentCallList();
    HandlePhoneStateChange(callInfoVec);
    // 处理单路通话明细，而CallDetailsChanged仅处理蜂窝挂断消息，和蓝牙保持一致
    HandleCallDetailChange(callId, callState, number, callInfoVec);
}

void CcpService::CallDetailsChanged(int32_t callId, int32_t callState, int32_t callType)
{
    HILOGI("[CcpService]CallDetailsChanged: callId=%{public}d, state=%{public}d, type=%{public}d",
        callId, callState, callType);
    if (callState == static_cast<int32_t>(Telephony::TelCallState::CALL_STATUS_DISCONNECTED) &&
        callType != static_cast<int32_t>(Telephony::CallType::TYPE_VOIP)) {
        // 只有蜂窝通话需要走该流程，因为蜂窝挂断瞬间PhoneStateChanged拿到的callId是-1，无法正常处理蜂窝挂断
        // voip和ccp构造的场景由PhoneStateChanged处理
        NL_CHECK_RETURN(pimpl->callIdMap_.FindIf(callId), "[CcpService]unknown callId, skip call details.");
        // 挂断对象已从 telephony 对象表移除，列表反查不到
        Telephony::CallAttributeInfo info{};
        info.callId = callId;
        info.callState = Telephony::TelCallState::CALL_STATUS_DISCONNECTED;
        info.callType = static_cast<Telephony::CallType>(callType);
        std::vector<Telephony::CallAttributeInfo> infoVec = { info };
        HandleCallDetailChange(callId, callState, "", infoVec);
    }
}

void CcpService::HandlePhoneStateChange(const std::vector<Telephony::CallAttributeInfo> &callInfoVec)
{
    /* 即使当前没有通话了，更新通话状态时，也需要将当前通话列表为0更新给对端 */
    /* 更新通话状态 */
    ProcessCallStateInfo(callInfoVec);
}

Telephony::CallAttributeInfo CcpService::FilterCallAttributeInfo(int32_t callId, int32_t callState,
    const std::string &number, const std::vector<Telephony::CallAttributeInfo> &callVec)
{
    Telephony::CallAttributeInfo info{};
    info.callId = callId;
    info.callState = static_cast<Telephony::TelCallState>(callState);
    /* 事件只携带 callId/callState，其余明细从通话列表补齐 */
    bool matched = false;
    for (const auto &callAttr : callVec) {
        if (callAttr.callId != callId) {
            continue;
        }
        info = callAttr;
        info.callState = static_cast<Telephony::TelCallState>(callState);
        matched = true;
        break;
    }
    if (matched || number.empty()) {
        return info;
    }
    /* 反查不到时用事件携带的号码兜底：目标容量取自数组自身，不依赖仓外上限声明 */
    const size_t destSize = sizeof(info.accountNumber);
    size_t numberCopyLen = std::min(number.size(), destSize - 1);
    if (numberCopyLen > 0 && strncpy_s(info.accountNumber, destSize, number.c_str(), numberCopyLen) != EOK) {
        HILOGE("[CcpService]copy accountNumber error");
    }
    return info;
}

void CcpService::HandleCallDetailChange(int32_t callId, int32_t callState, const std::string &number,
    const std::vector<Telephony::CallAttributeInfo> &callInfoVec)
{
    // -1仅表示无特定通话，不得入表；自造VoIP的callId为负值但不等于该常量，仍正常处理
    NL_CHECK_RETURN(callId != INVALID_CCP_VOIP_ID, "[CcpService]invalid callId, skip detail update");
    // 从全量通话列表中过滤出需要处理的单路通话明细
    Telephony::CallAttributeInfo info = FilterCallAttributeInfo(callId, callState, number, callInfoVec);
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
            ProfileASC *ascService = static_cast<ProfileASC *>(
                SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_ASC));
            ProfileTws *twsService = static_cast<ProfileTws *>(
                SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_TWS));
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
        // 恢复时不需要考虑未接入call kit的VoIP，因为耳机会重连走重新起流
        std::vector<Telephony::CallAttributeInfo> callVec = CcpSystemInterface::GetInstance().GetCurrentCallList();
        for (auto &callAttr : callVec) {
            ProcessCallDetailChange(callAttr);
        }
        ProcessCallStateInfo(callVec);
    });
}

void CcpService::ProcessCallStateInfo(const std::vector<Telephony::CallAttributeInfo> &callInfoVec)
{
    HILOGD("[CcpService]ProcessCallStateInfo current callInfos size=%{public}d", callInfoVec.size());
    std::shared_ptr<TotalCallStateProp> prop = std::make_shared<TotalCallStateProp>();
    for (const auto &callAttr : callInfoVec) {
        TotalCallStateProp::SingleCallStateProp singleCallState;
        // 将CallManager的state转换为协议定义的值
        auto iter = pimpl->callStatusMap_.find(callAttr.callState);
        if (iter == pimpl->callStatusMap_.end()) {
            HILOGW("[CcpService]not support state, skip update this call: callId=%{public}d, callState=%{public}d",
                callAttr.callId, callAttr.callState);
            continue;
        }
        // 通话标识
        uint8_t callId = pimpl->GetIndexForCall(callAttr.callId, callAttr.callState);
        if (callId == 0) {
            HILOGW("[CcpService]invalid callId=%{public}d", callAttr.callId);
            continue;
        }
        singleCallState.callId = callId;
        // 通话状态
        singleCallState.callStatus = static_cast<uint8_t>(iter->second);
        // 通话标记
        singleCallState.callFlag = GetCallFlag(callAttr).value;
        HILOGI("[CcpService] callId=%{public}d, telStatus=%{public}d, ccpStatus=%{public}d, flag=0x%{public}02x",
            singleCallState.callId, callAttr.callState, singleCallState.callStatus, singleCallState.callFlag);
        prop->callStateVec.push_back(singleCallState);
    }

    bool hasVoipCallId = HasVoipCallId();
    if (hasVoipCallId && (callInfoVec.empty() || prop->callStateVec.empty())) {
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

bool CcpService::HasPhoneCallId()
{
    bool hasPhoneCallId = pimpl->callIdMap_.Find([](int32_t key, const impl::CallInfo &value) {
        if (key > 0) {
            return true;
        }
        return false;
    });
    return hasPhoneCallId;
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
    uint8_t callId = pimpl->GetIndexForCall(info.callId, info.callState);
    NL_CHECK_RETURN(callId != 0, "[CcpService]invalid callId=%{public}d", info.callId);
    prop->callId = static_cast<uint8_t>(callId);
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
    // 映射中没有该 callId，说明本次挂断已被另一入口处理过（或该路从未上报），避免重复处理挂断事件
    NL_CHECK_RETURN(pimpl->callIdMap_.FindIf(info.callId), "[CcpService]unknown callId %{public}d", info.callId);
    std::shared_ptr<CallTerminateInfoProp> prop = std::make_shared<CallTerminateInfoProp>();
    // 通话标识
    uint8_t callId = pimpl->GetIndexForCall(info.callId, info.callState);
    prop->callId = static_cast<uint8_t>(callId);
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
        // 避免自己造的这个callId和后续蜂窝的CallId重复，避开蜂窝的id区间
        int32_t voipId = NEARLINK_CCP_VOIP_MAX - 1;
        pimpl->currentVoipCallId_ = voipId;
        pimpl->voipIdSet_.insert(voipId);
        // 蜂窝通话期间来voip通话，voip流启动时只登记callId不建状态，待蜂窝挂断后由 ProcessCallTerminateInfo 补建
        NL_CHECK_RETURN(!HasPhoneCallId(), "Now is in tellphone call, no need to create voip state.");
        CreateVoipCallInfo();
    });
}

void CcpService::HandleVoipStop(const RawAddress &device)
{
    HILOGI("[CcpService]Enter");
    DoInCcpThread([this]() {
        NL_CHECK_RETURN(pimpl->currentVoipCallId_ != INVALID_CCP_VOIP_ID, "invalid callId.");
        Telephony::CallAttributeInfo info{};
        info.callState = TelCallState::CALL_STATUS_DISCONNECTED;
        info.callId = pimpl->currentVoipCallId_;
        std::vector<Telephony::CallAttributeInfo> infoVec = { info };
        HandleCallDetailChange(info.callId, static_cast<int32_t>(info.callState), "", infoVec);
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
    Telephony::CallAttributeInfo info{};
    info.callState = TelCallState::CALL_STATUS_DIALING;
    info.callDirection = Telephony::CallDirection::CALL_DIRECTION_OUT;
    info.callId = pimpl->currentVoipCallId_;
    HILOGI("[CcpService]HandleVoipStart: id=%{public}d", info.callId);
    /* 自造 VoIP 不在通话列表中，直接放进快照以保留 callDirection */
    std::vector<Telephony::CallAttributeInfo> infoVec = { info };
    HandleCallDetailChange(info.callId, static_cast<int32_t>(info.callState), "", infoVec);
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
