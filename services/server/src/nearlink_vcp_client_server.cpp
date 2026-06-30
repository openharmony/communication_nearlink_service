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

#include "nearlink_vcp_client_server.h"
#include "SleInterfaceProfileManager.h"
#include "SleInterfaceProfileVcp.h"
#include "log_util.h"

namespace OHOS {
namespace Nearlink {

NearlinkVcpClientServer::NearlinkVcpClientServer()
{
    HILOGI("NearlinkVcpClientServer called.");
}

NearlinkVcpClientServer::~NearlinkVcpClientServer()
{
    HILOGI("~NearlinkVcpClientServer called.");
}

NlErrCode NearlinkVcpClientServer::SetDeviceAbsoluteVolume(const NearlinkRawAddress &addr, int32_t volumeLevel,
    uint8_t streamType)
{
    HILOGI("SetDeviceAbsoluteVolume address=%{public}s, volumeLevel=%{public}d", GET_ENCRYPT_ADDR(addr), volumeLevel);
    ProfileVcp *vcpService = static_cast<ProfileVcp *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_VCP));
    NL_CHECK_RETURN_RET(vcpService, NL_ERR_INTERNAL_ERROR, "VcpService is nullptr.");
    NL_CHECK_RETURN_RET(streamType < VolumeStreamType::SLE_STREAM_MAX, NL_ERR_INVALID_PARAM, "streamType is invalid.");
    vcpService->SetDeviceAbsoluteVolume(addr, volumeLevel, streamType);
    return NL_NO_ERROR;
}

NlErrCode NearlinkVcpClientServer::GetDeviceMediaVolume(const NearlinkRawAddress &addr, int32_t &mediaVolume)
{
    HILOGI("address=%{public}s", GET_ENCRYPT_ADDR(addr));
    ProfileVcp *vcpService = static_cast<ProfileVcp *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_VCP));
    NL_CHECK_RETURN_RET(vcpService, NL_ERR_INTERNAL_ERROR, "VcpService is nullptr.");
    mediaVolume = vcpService->GetDeviceMediaVolume(addr);
    return NL_NO_ERROR;
}

NlErrCode NearlinkVcpClientServer::GetDeviceCallVolume(const NearlinkRawAddress &addr, int32_t &callVolume)
{
    HILOGI("address=%{public}s", GET_ENCRYPT_ADDR(addr));
    ProfileVcp *vcpService = static_cast<ProfileVcp *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_VCP));
    NL_CHECK_RETURN_RET(vcpService, NL_ERR_INTERNAL_ERROR, "VcpService is nullptr.");
    callVolume = vcpService->GetDeviceCallVolume(addr);
    return NL_NO_ERROR;
}
}
}