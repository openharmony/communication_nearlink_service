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

#include "nearlink_ccp_client_server.h"
#include "SleInterfaceProfileManager.h"
#include "SleInterfaceProfileCcp.h"
#include "ThreadUtil.h"
#include "log_util.h"
#include "nearlink_ccp_export.h"
#include "nearlink_host_server.h"

namespace OHOS {
namespace Nearlink {
namespace {
/* 调用方缓存了 ccp proxy，星闪关闭后仍需在此丢弃推送，避免访问已释放的协议栈实例。
   返回 NL_NO_ERROR 表示可用；NL_CCP_ERR_SLE_OFF 表示星闪已关闭；
   NL_CCP_ERR_UNAVAILABLE_PROXY 表示 host 服务对象取不到或开关状态查询失败 */
int32_t GetSleStateInService()
{
    sptr<NearlinkHostServer> hostServer = NearlinkHostServer::GetInstance();
    if (hostServer == nullptr) {
        return NL_CCP_ERR_UNAVAILABLE_PROXY;
    }
    /* IsSleEnabled 通过出参返回开关状态，返回值只表示查询是否成功 */
    bool isSleEnabled = false;
    if (hostServer->IsSleEnabled(isSleEnabled) != NL_NO_ERROR) {
        return NL_CCP_ERR_UNAVAILABLE_PROXY;
    }
    if (!isSleEnabled) {
        return NL_CCP_ERR_SLE_OFF;
    }
    return NL_NO_ERROR;
}

/* 投递后执行期的失败无法回传调用方，只能在受理前把可判定的“服务不可用”暴露出去 */
bool IsCcpServiceAvailable()
{
    return SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_CCP) != nullptr;
}
}  // namespace

NearlinkCcpClientServer::NearlinkCcpClientServer()
{
    HILOGI("NearlinkCcpClientServer called.");
}

NearlinkCcpClientServer::~NearlinkCcpClientServer()
{
    HILOGI("~NearlinkCcpClientServer called.");
}

int32_t NearlinkCcpClientServer::PhoneStateChanged(int32_t callId, int32_t numActive, int32_t numHeld,
    int32_t callState, const std::string &number)
{
    int32_t sleState = GetSleStateInService();
    if (sleState == NL_CCP_ERR_SLE_OFF) {
        HILOGD("sle is off, drop phone state change.");
        return sleState;
    }
    if (sleState != NL_NO_ERROR) {
        HILOGW("sle state is unavailable, drop phone state change.");
        return sleState;
    }
    /* 同步预判：服务对象未创建或正在停止时，执行期才失败会变成假成功 */
    if (!IsCcpServiceAvailable()) {
        HILOGW("ccp service is unavailable, drop phone state change.");
        return NL_CCP_ERR_UNAVAILABLE_PROXY;
    }
    HILOGI("PhoneStateChanged callId=%{public}d, numActive=%{public}d, numHeld=%{public}d, callState=%{public}d",
        callId, numActive, numHeld, callState);
    // 立即回包，业务逻辑投递到 CCP 串行线程执行，避免阻塞蜂窝调用方；
    // 执行期若服务对象已销毁，任务只记录日志并丢弃，失败不回报调用方：
    // 丢事件的窗口仅在预判通过后、任务执行前服务销毁的瞬间，下一次通话事件会全量纠正，
    // 末次事件丢失由对端连接断开自愈
    DoInCcpThread([callId, numActive, numHeld, callState, number]() {
        ProfileCcp *ccpService = static_cast<ProfileCcp *>(
            SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_CCP));
        NL_CHECK_RETURN(ccpService, "CcpService is nullptr after accepted, drop phone state change.");
        ccpService->PhoneStateChanged(callId, numActive, numHeld, callState, number);
    });
    return NL_NO_ERROR;
}

int32_t NearlinkCcpClientServer::CallDetailsChanged(int32_t callId, int32_t callState, int32_t callType)
{
    int32_t sleState = GetSleStateInService();
    if (sleState == NL_CCP_ERR_SLE_OFF) {
        HILOGD("sle is off, drop call details change.");
        return sleState;
    }
    if (sleState != NL_NO_ERROR) {
        HILOGW("sle state is unavailable, drop call details change.");
        return sleState;
    }
    /* 同步预判：服务对象未创建或正在停止时，执行期才失败会变成假成功 */
    if (!IsCcpServiceAvailable()) {
        HILOGW("ccp service is unavailable, drop call details change.");
        return NL_CCP_ERR_UNAVAILABLE_PROXY;
    }
    HILOGI("CallDetailsChanged callId=%{public}d, callState=%{public}d, callType=%{public}d",
        callId, callState, callType);
    // 立即回包，业务逻辑投递到 CCP 串行线程执行，避免阻塞蜂窝调用方；
    // 执行期若服务对象已销毁，任务只记录日志并丢弃，失败不回报调用方：
    // 丢事件的窗口仅在预判通过后、任务执行前服务销毁的瞬间，下一次通话事件会全量纠正，
    // 末次事件丢失由对端连接断开自愈
    DoInCcpThread([callId, callState, callType]() {
        ProfileCcp *ccpService = static_cast<ProfileCcp *>(
            SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_CCP));
        NL_CHECK_RETURN(ccpService, "CcpService is nullptr after accepted, drop call details change.");
        ccpService->CallDetailsChanged(callId, callState, callType);
    });
    return NL_NO_ERROR;
}
}  // namespace Nearlink
}  // namespace OHOS
