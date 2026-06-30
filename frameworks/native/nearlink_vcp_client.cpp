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

#include "nearlink_vcp_client.h"

#include "log.h"
#include "i_nearlink_vcp_client.h"
#include "nearlink_host.h"
#include "nearlink_sa_manager.h"

namespace OHOS {
namespace Nearlink {
struct VolumeControllerClient::impl {
    impl();
    ~impl();

    int32_t profileRegisterId_ = 0;
};

VolumeControllerClient::impl::impl()
{
    HILOGI("start");
    std::shared_ptr<NearlinkRegisterInfo> info = std::make_shared<NearlinkRegisterInfo>(PROFILE_VCP_SERVER);
    info->serviceStartedFunc_ = [this](sptr<IRemoteObject> remote) -> void {
        sptr<INearlinkVcpClient> proxy = iface_cast<INearlinkVcpClient>(remote);
        NL_CHECK_RETURN(proxy, "proxy is nullptr");
    };

    profileRegisterId_ = NearlinkSaManager::GetInstance().RegisterFunc(info);
    if (profileRegisterId_ == INVALID_PROFILE_ID) {
        HILOGE("profileRegisterId_ is invalid");
    }
}

VolumeControllerClient::impl::~impl()
{
    HILOGI("starts");
    NearlinkSaManager::GetInstance().DeregisterFunc(profileRegisterId_);
}

VolumeControllerClient *VolumeControllerClient::GetProfile()
{
    HILOGI("enter");
    static VolumeControllerClient instance;
    return &instance;
}

NlErrCode VolumeControllerClient::SetDeviceAbsoluteVolume(const NearlinkRemoteDevice &device, int32_t volumeLevel,
    uint8_t streamType)
{
    NL_CHECK_RETURN_RET(device.IsValidNearlinkRemoteDevice(), NL_ERR_INVALID_PARAM, "Invalid remote device");
    NL_CHECK_RETURN_RET(IS_SLE_ENABLED(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<INearlinkVcpClient> proxy = GetProxy<INearlinkVcpClient>(PROFILE_VCP_SERVER);
    NL_CHECK_RETURN_RET(proxy != nullptr, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr");
    NL_CHECK_RETURN_RET(streamType < VolumeStreamType::SLE_STREAM_MAX, NL_ERR_INVALID_PARAM, "Invalid streamType");
    return proxy->SetDeviceAbsoluteVolume(NearlinkRawAddress(device.GetDeviceAddr()), volumeLevel, streamType);
}

NlErrCode VolumeControllerClient::GetDeviceMediaVolume(const NearlinkRemoteDevice &device, int &mediaVolume)
{
    NL_CHECK_RETURN_RET(device.IsValidNearlinkRemoteDevice(), NL_ERR_INVALID_PARAM, "Invalid remote device");
    NL_CHECK_RETURN_RET(IS_SLE_ENABLED(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<INearlinkVcpClient> proxy = GetProxy<INearlinkVcpClient>(PROFILE_VCP_SERVER);
    NL_CHECK_RETURN_RET(proxy != nullptr, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr");
    return proxy->GetDeviceMediaVolume(NearlinkRawAddress(device.GetDeviceAddr()), mediaVolume);
}

NlErrCode VolumeControllerClient::GetDeviceCallVolume(const NearlinkRemoteDevice &device, int &callVolume)
{
    NL_CHECK_RETURN_RET(device.IsValidNearlinkRemoteDevice(), NL_ERR_INVALID_PARAM, "Invalid remote device");
    NL_CHECK_RETURN_RET(IS_SLE_ENABLED(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<INearlinkVcpClient> proxy = GetProxy<INearlinkVcpClient>(PROFILE_VCP_SERVER);
    NL_CHECK_RETURN_RET(proxy != nullptr, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr");
    return proxy->GetDeviceCallVolume(NearlinkRawAddress(device.GetDeviceAddr()), callVolume);
}

VolumeControllerClient::VolumeControllerClient()
{
    HILOGI("enter");
    pimpl = std::make_unique<impl>();
}

VolumeControllerClient::~VolumeControllerClient()
{
    HILOGI("enter");
    pimpl = nullptr;
}

} // namespace Nearlink
} // namespace OHOS