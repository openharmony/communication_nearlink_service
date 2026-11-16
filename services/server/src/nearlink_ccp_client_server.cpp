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
#include "log_util.h"
#include "nearlink_errorcode.h"

namespace OHOS {
namespace Nearlink {

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
    ProfileCcp *ccpService =
        static_cast<ProfileCcp *>(SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_CCP));
    NL_CHECK_RETURN_RET(ccpService, NL_ERR_UNAVAILABLE_PROXY,
        "ccp service is unavailable, drop phone state change.");
    HILOGI("PhoneStateChanged callId=%{public}d, numActive=%{public}d, numHeld=%{public}d, callState=%{public}d",
        callId, numActive, numHeld, callState);
    /* 方法内部投递到 ccp 串行线程并立即返回，不阻塞蜂窝调用方 */
    ccpService->PhoneStateChanged(callId, numActive, numHeld, callState, number);
    return NL_NO_ERROR;
}

int32_t NearlinkCcpClientServer::CallDetailsChanged(int32_t callId, int32_t callState, int32_t callType)
{
    ProfileCcp *ccpService =
        static_cast<ProfileCcp *>(SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_CCP));
    NL_CHECK_RETURN_RET(ccpService, NL_ERR_UNAVAILABLE_PROXY,
        "ccp service is unavailable, drop call details change.");
    HILOGI("CallDetailsChanged callId=%{public}d, callState=%{public}d, callType=%{public}d",
        callId, callState, callType);
    /* 方法内部投递到 ccp 串行线程并立即返回，不阻塞蜂窝调用方 */
    ccpService->CallDetailsChanged(callId, callState, callType);
    return NL_NO_ERROR;
}
}  // namespace Nearlink
}  // namespace OHOS
