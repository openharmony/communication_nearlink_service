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
#include "CcpStackAdapter.h"

#include <cstring>
#include <memory>

#include "call_manager_info.h"
#include "CcpService.h"
#include "nearlink_dft_exception.h"
#include "nlstk_ccp_ccs_server.h"
#include "SleServiceFfrtLog.h"
#include "ThreadUtil.h"

namespace OHOS {
namespace Nearlink {
namespace {
    /* 实例名称暂定为 NearlinkCall */
    constexpr const char* NEARLINK_CALL_INSTANCE_NAME = "NearlinkCall";
    constexpr const int32_t MAX_COUNT_SIZE = 255;
}

CcpStackAdapter::CcpStackAdapter()
{
    HILOGI("[CcpService]Enter");
}

CcpStackAdapter::~CcpStackAdapter()
{
    HILOGI("[CcpService]Enter");
}

/* 接听 */
static void CcpAnswerCbk(SLE_Addr_S *addr, int32_t instanceId, uint32_t requestId, uint8_t callId)
{
    HILOGI("[CcpService]Enter");
    NL_CHECK_RETURN(addr, "addr is null.");
    const RawAddress &device = RawAddress::ConvertToString(addr->addr);
    HILOGI("[CcpService]AnswerCbk: device=%{public}s, instanceId=%{public}d, requestId=%{public}d, callId=%{public}d",
        GET_ENCRYPT_ADDR(device), instanceId, requestId, callId);
    CcpService *ccpService = CcpService::GetService();
    NL_CHECK_RETURN(ccpService, "ccpService is null.");
    ccpService->HandleAnswer(device, instanceId, requestId, callId);
}

/* 挂断 */
static void CcpHangUpCbk(SLE_Addr_S *addr, int32_t instanceId, uint32_t requestId, uint8_t callId)
{
    HILOGI("[CcpService]Enter");
    NL_CHECK_RETURN(addr, "addr is null.");
    const RawAddress &device = RawAddress::ConvertToString(addr->addr);
    HILOGI("[CcpService]HangUpCbk: device=%{public}s, instanceId=%{public}d, requestId=%{public}d, callId=%{public}d",
        GET_ENCRYPT_ADDR(device), instanceId, requestId, callId);
    CcpService *ccpService = CcpService::GetService();
    NL_CHECK_RETURN(ccpService, "ccpService is null.");
    ccpService->HandleHangUp(device, instanceId, requestId, callId);
}

static void CcpStartCcsInstanceCbk(int32_t instanceId, NLSTK_Errcode_E ret)
{
    HILOGI("[CcpService]Enter");
    CcpService *ccpService = CcpService::GetService();
    NL_CHECK_RETURN(ccpService, "ccpService is null.");
    bool isSuccess = (ret == NLSTK_ERRCODE_SUCCESS);
    ccpService->HandleStartInstance(instanceId, isSuccess);
}

static void CcpCallControlServiceAuthorizeCbk(uint32_t requestId, int32_t instanceId,
    NLSTK_CcpCcsPropertyType_E property, NLSTK_ServicePropertyOpType_E operation)
{
    HILOGI("[CcpService]Enter");
    CcpService *ccpService = CcpService::GetService();
    NL_CHECK_RETURN(ccpService, "ccpService is null.");
    ccpService->HandleAuthorize(instanceId, requestId, property);
}

void CcpStackAdapter::CreateCcsInstance(uint8_t instanceId)
{
    HILOGI("[CcpService]Enter");
    NLSTK_CcpCallControlInfo_S info{};
    info.type = NLSTK_CCP_CALL_CONTROL_COMMON_SERVICE;
    info.instanceName.data = const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(NEARLINK_CALL_INSTANCE_NAME));
    info.instanceName.len = strlen(NEARLINK_CALL_INSTANCE_NAME);
    /**< 特性状态 */
    info.featureStatus = NLSTK_CCP_IN_BAND_RING_ON;  // bit0：0: 带内铃音关闭 1: 带内铃音开启
    /**< 协议支持 */
    info.protocolSupport.data = const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(CCP_SUPPORT_PROTOCOLS));
    info.protocolSupport.len = strlen(CCP_SUPPORT_PROTOCOLS);
    /**< 媒体实例标识 */
    info.mediaInstanceId = instanceId;  // 媒体实例标识属性用于标识多媒体服务实例，媒体实例标识在设备内是唯一的.
    /**< 通话请求支持 */
    // Bit0 0：不支持本地保持和本地恢复 1：支持本地保持和本地恢复
    // Bit1 0：不支持合并 1：支持合并
    // 其他	预留
    info.callRequestSupport = NLSTK_CCP_CALL_REQUEST_SUPPORT_NOT;
    /**< 通话控制点 */
    info.callControlPoint.answer = &CcpAnswerCbk;
    info.callControlPoint.hangUp = &CcpHangUpCbk;
    /**< 通话控制服务添加回调 */
    info.startCcsInst = &CcpStartCcsInstanceCbk;
    /**< 通话控制授权回调 */
    info.authorize = &CcpCallControlServiceAuthorizeCbk;
    for (int i = 0; i < NLSTK_CCP_CCS_MAX_PROPERTY; i++) {
        info.propertyRights[i] = NLSTK_CCS_READ_AUTHEN | NLSTK_CCS_READ_ENCRYPT;
    }
    uint32_t ret = NLSTK_CcpCreateCcsInstance(&info);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        DftReportAudioError("", ERROR_NEARLINK_INNER, SUB_ERRCODE_CASE1);
        HILOGE("[CcpService] Create ccs instance fail, ret = 0x%{public}x", ret);
    }
}

void CcpStackAdapter::DeleteCcsInstance(int32_t instanceId)
{
    HILOGI("[CcpService]Enter");
    uint32_t ret = NLSTK_CcpDeleteCcsInstance(instanceId);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        DftReportAudioError("", ERROR_NEARLINK_INNER, SUB_ERRCODE_CASE1);
        HILOGE("[CcpService] CcpDeleteCcsInstance fail ret = 0x%{public}x", ret);
        return;
    }
    HILOGI("[CcpService]Delete success, instanceId_=%{public}d", instanceId);
}

bool CcpStackAdapter::NewCallStateInfo(NLSTK_CcpCallStatues_S *callState, size_t count)
{
    NL_CHECK_RETURN_RET(callState, false, "[CcpService]callState is null.");
    if (count > MAX_COUNT_SIZE || count == 0) {
        callState->callId = nullptr;
        return false;
    }
    callState->callId = new (std::nothrow) uint8_t[count];
    NL_CHECK_RETURN_RET(callState->callId, false, "callId is nullptr.");
    callState->networkId = new (std::nothrow) uint8_t[count];
    if (callState->networkId == nullptr) {
        delete[] callState->callId;
        return false;
    }
    callState->callStatus = new (std::nothrow) uint8_t[count];
    if (callState->callStatus == nullptr) {
        delete[] callState->callId;
        delete[] callState->networkId;
        return false;
    }
    callState->callFlag = new (std::nothrow) uint8_t[count];
    if (callState->callFlag == nullptr) {
        delete[] callState->callId;
        delete[] callState->networkId;
        delete[] callState->callStatus;
        return false;
    }
    return true;
}

void CcpStackAdapter::FreeCallStateInfo(NLSTK_CcpCallStatues_S *callState, size_t count)
{
    NL_CHECK_RETURN(callState, "[CcpService]callState is null.");
    if (count != 0) {
        delete[] callState->callId;
        delete[] callState->networkId;
        delete[] callState->callStatus;
        delete[] callState->callFlag;
    }
}

void CcpStackAdapter::UpdateCallStateInfo(int32_t instanceId, std::shared_ptr<TotalCallStateProp> prop)
{
    HILOGD("[CcpService]Enter");
    NLSTK_CcpCallStatues_S callState{};
    size_t count = prop->callStateVec.size();
    callState.callCount = count;
    if (count == 0) {
        callState.callId = nullptr;
        callState.networkId = nullptr;
        callState.callStatus = nullptr;
        callState.callFlag = nullptr;
    } else {
        NL_CHECK_RETURN(NewCallStateInfo(&callState, count), "New callStatues fail.");
        for (size_t i = 0; i < count; ++i) {
            const TotalCallStateProp::SingleCallStateProp &singleCallInfo = prop->callStateVec.at(i);
            // 通话标识
            callState.callId[i] = singleCallInfo.callId;
            // 网络标识(发给耳机可暂时不填)
            callState.networkId[i] = singleCallInfo.networkId;
            // 通话状态
            callState.callStatus[i] = singleCallInfo.callStatus;
            // 通话标记
            callState.callFlag[i] = singleCallInfo.callFlag;
        }
    }
    uint32_t ret = NLSTK_CcpUpdateCcsProperty(instanceId, NLSTK_CCP_CCS_CALL_STATUS, &callState);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        FreeCallStateInfo(&callState, count);
        DftReportAudioError("", ERROR_NEARLINK_INNER, SUB_ERRCODE_CASE1);
        HILOGE("[CcpService] CallStatues fail ret = %{public}d", ret);
        return;
    }
    FreeCallStateInfo(&callState, count);
    HILOGI("[CcpService]CallStatues update success, instanceId_=%{public}d, callCount=%{public}d", instanceId,
           callState.callCount);
}

void CcpStackAdapter::UpdateCallInOutInfo(int32_t instanceId, std::shared_ptr<CallInOutInfoProp> prop)
{
    HILOGI("[CcpService]Enter");
    // 更新呼入呼出信息
    NLSTK_CcpCallInOutInfo_S info{};
    info.callId = prop->callId;
    info.networkId = prop->networkId;
    info.callFlag = prop->callFlag;
    if (strlen(prop->userInfo) != 0) {
        info.userInfo.data = reinterpret_cast<uint8_t *>(prop->userInfo);
        info.userInfo.len = strlen(prop->userInfo);
    }
    if (strlen(prop->userAlias) != 0) {
        info.userAlias.data = reinterpret_cast<uint8_t *>(prop->userAlias);
        info.userAlias.len = strlen(prop->userAlias);
    }
    uint32_t ret = NLSTK_CcpUpdateCcsProperty(instanceId, NLSTK_CCP_CCS_CALLIN_OUT_INFO, &info);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        DftReportAudioError("", ERROR_NEARLINK_INNER, SUB_ERRCODE_CASE1);
        HILOGE("[CcpService] CallInOutInfo fail ret = %{public}d", ret);
        return;
    }
    HILOGI("[CcpService]CallInOutInfo update success, instanceId_=%{public}d, callId=%{public}d",
        instanceId, info.callId);
}

void CcpStackAdapter::UpdateCallTerminateInfo(int32_t instanceId, std::shared_ptr<CallTerminateInfoProp> prop)
{
    HILOGI("[CcpService]Enter");
    NLSTK_CcpCallTermination_S termination{};
    termination.callId = prop->callId;
    termination.terminateReason = prop->terminateReason;
    uint32_t ret = NLSTK_CcpUpdateCcsProperty(instanceId, NLSTK_CCP_CCS_CALL_TERMINATION, &termination);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        DftReportAudioError("", ERROR_NEARLINK_INNER, SUB_ERRCODE_CASE1);
        HILOGE("[CcpService] CallTermination fail ret = %{public}d", ret);
        return;
    }
    HILOGI("[CcpService]CallTermination update success, instanceId_=%{public}d, callId=%{public}d",
        instanceId, termination.callId);
}

void CcpStackAdapter::NotifyCallControlSuccess(uint32_t requestId, int32_t instanceId)
{
    HILOGI("[CcpService]Enter");
    // 通知协议栈处理结果
    NLSTK_CcpCallControlResult(requestId, instanceId, NLSTK_CCP_CCS_OPERATION_SUCCESS);
}

void CcpStackAdapter::NotifyCallControlFail(uint32_t requestId, int32_t instanceId)
{
    HILOGI("[CcpService]Enter");
    // 通知协议栈处理结果
    NLSTK_CcpCallControlResult(requestId, instanceId, NLSTK_CCP_CCS_OPERATION_FAILED);
}

void CcpStackAdapter::AuthorizeResult(int32_t instanceId, uint32_t requestId, int32_t prop)
{
    HILOGI("[CcpService]Enter");
    uint32_t ret = NLSTK_CcpCcsAuthorizeResult(requestId, instanceId, static_cast<NLSTK_CcpCcsPropertyType_E>(prop), 0);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        DftReportAudioError("", ERROR_NEARLINK_INNER, SUB_ERRCODE_CASE1);
        HILOGE("[CcpService] AuthorizeResult fail ret = %{public}d", ret);
        return;
    }
}

}  // namespace Nearlink
}  // namespace OHOS
