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

#include "nearlink_sle_controller_server.h"
#include "interface_sle_controller.h"
#include "SleInterfaceAdapterSub.h"
#include "SleInterfaceManager.h"
#include "nearlink_permission_manager.h"
#include "nearlink_verification_manager.h"
#include "log_util.h"
#include "nearlink_device_manager.h"
#include "ipc_skeleton.h"

namespace OHOS {
namespace Nearlink {

struct NearlinkSleControllerServer::impl {
    impl();
    ~impl();
};

NearlinkSleControllerServer::impl::impl()
{
    HILOGI("NearlinkSleControllerServer impl initialized");
}

NearlinkSleControllerServer::impl::~impl()
{
    HILOGI("NearlinkSleControllerServer impl destroyed");
}

NearlinkSleControllerServer::NearlinkSleControllerServer() :
    pimpl(std::make_unique<NearlinkSleControllerServer::impl>())
{
    HILOGI("NearlinkSleControllerServer created");
}

NearlinkSleControllerServer::~NearlinkSleControllerServer()
{
    HILOGW("NearlinkSleControllerServer destroyed");
}

bool NearlinkSleControllerServer::IsSetSleCoexParamAllowed()
{
    uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
    int32_t uid = IPCSkeleton::GetCallingUid();
    VerificationContext ctx = { .tokenId = tokenId, .uid = uid };

    bool result = NearlinkVerificationManager::GetInstance()
        .CheckVerification(VerificationType::CONTROLLER_COEX, ctx);
    HILOGI("tokenId: %{public}u, result: %{public}d", tokenId, result);
    return result;
}

NlErrCode NearlinkSleControllerServer::SetSleCoexParam(uint16_t maxBitRate, uint8_t dutyCycle)
{
    HILOGI("Enter, maxBitRate: %{public}u, dutyCycle: %{public}u", maxBitRate, dutyCycle);

    // Permission check
    NL_CHECK_RETURN_RET(IsSetSleCoexParamAllowed(),
        NL_ERR_INTERNAL_ERROR, "Permission check failed");

    // Call Service layer
    if (InterfaceSleController::GetInstance().SetSleCoexParam(maxBitRate, dutyCycle)) {
        return NL_NO_ERROR;
    }

    HILOGE("SetSleCoexParam failed");
    return NL_ERR_INTERNAL_ERROR;
}

NlErrCode NearlinkSleControllerServer::UpdateConnectInterval(const std::string &device, int32_t intervalType)
{
    HILOGI("Enter, device: %{public}s, intervalType: %{public}d",
           GetEncryptAddr(device).c_str(), intervalType);

    // Validate device
    NL_CHECK_RETURN_RET(!device.empty(),
        NL_ERR_INVALID_PARAM, "Device address is empty");

    std::string realAddr = "";
    NearlinkDeviceManager::GetInstance()->GetDeviceRealAddr(device, realAddr);
    NL_CHECK_RETURN_RET(realAddr != "", NL_ERR_INTERNAL_ERROR, "device is invalid.");
    SleInterfaceAdapterSub *sleService = static_cast<SleInterfaceAdapterSub *>
        (SleInterfaceManager::GetInstance()->GetAdapter(SleTransport::ADAPTER_SLE));
    NL_CHECK_RETURN_RET(sleService, NL_ERR_INTERNAL_ERROR, "sleService invalid.");
    RawAddress addr(realAddr);
    int acbState = sleService->GetAcbState(addr);
    NL_CHECK_RETURN_RET(acbState >= static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED),
        NL_ERR_INTERNAL_ERROR, "need connected.");

    // Call Service layer
    if (InterfaceSleController::GetInstance().UpdateConnectInterval(realAddr, intervalType)) {
        return NL_NO_ERROR;
    }

    HILOGE("UpdateConnectInterval failed");
    return NL_ERR_INTERNAL_ERROR;
}

NlErrCode NearlinkSleControllerServer::SetSleCoexMode(int32_t mode, const std::vector<std::string> &deviceList,
    const std::vector<ConnectionInterval> &paramList)
{
    HILOGI("Enter, mode: %{public}d", mode);
 
    // Permission check
    NL_CHECK_RETURN_RET(IsSetSleCoexParamAllowed(),
        NL_ERR_INTERNAL_ERROR, "Permission check failed");
 
    // Call Service layer
    if (InterfaceSleController::GetInstance().SetSleCoexMode(mode, deviceList, paramList)) {
        return NL_NO_ERROR;
    }
 
    HILOGE("SetSleCoexMode failed");
    return NL_ERR_INTERNAL_ERROR;
}

} // namespace Nearlink
} // namespace OHOS
