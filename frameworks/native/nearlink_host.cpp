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

#include "nearlink_host.h"
#include "nearlink_base_host.h"
#include "nearlink_base_remote_device.h"
#include "nearlink_observer_adapter.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {

NearlinkHost& NearlinkHost::GetInstance()
{
    static NearlinkHost instance;
    return instance;
}

NlErrCode NearlinkHost::RegisterObserver(std::shared_ptr<NearlinkHostObserver> observer)
{
    auto adapter = WrapHostObserver(observer);
    if (adapter == nullptr) {
        return NL_ERR_INVALID_PARAM;
    }
    NlErrCode ret = impl_.RegisterObserver(adapter);
    if (ret != NL_NO_ERROR) {
        EraseHostObserver(observer);
    }
    return ret;
}

NlErrCode NearlinkHost::DeregisterObserver(std::shared_ptr<NearlinkHostObserver> observer)
{
    NL_CHECK_RETURN_RET(impl_.IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT, "nearlink is not support.");
    std::shared_ptr<NearlinkBaseHostObserver> adapter;
    if (!UnwrapHostObserver(observer, adapter)) {
        HILOGW("observer has not been registered");
        return NL_NO_ERROR;
    }
    NlErrCode ret = impl_.DeregisterObserver(adapter);
    if (ret == NL_NO_ERROR) {
        EraseHostObserver(observer);
    }
    return ret;
}

NlErrCode NearlinkHost::RegisterBatteryObserver(std::shared_ptr<NearlinkRemoteDeviceBatteryObserver> observer)
{
    auto adapter = WrapBatteryObserver(observer);
    if (adapter == nullptr) {
        return NL_ERR_INVALID_PARAM;
    }
    NlErrCode ret = impl_.RegisterBatteryObserver(adapter);
    if (ret != NL_NO_ERROR) {
        EraseBatteryObserver(observer);
    }
    return ret;
}

NlErrCode NearlinkHost::DeregisterBatteryObserver(std::shared_ptr<NearlinkRemoteDeviceBatteryObserver> observer)
{
    NL_CHECK_RETURN_RET(impl_.IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT, "nearlink is not support.");
    NL_CHECK_RETURN_RET(impl_.IsBasSupported(), NL_ERR_FEATURE_NOT_SUPPORT, "Battery service is not supported.");
    std::shared_ptr<NearlinkBaseRemoteDeviceBatteryObserver> adapter;
    if (!UnwrapBatteryObserver(observer, adapter)) {
        HILOGW("battery observer has not been registered");
        return NL_NO_ERROR;
    }
    NlErrCode ret = impl_.DeregisterBatteryObserver(adapter);
    if (ret == NL_NO_ERROR) {
        EraseBatteryObserver(observer);
    }
    return ret;
}

NlErrCode NearlinkHost::RegisterRssiObserver(std::shared_ptr<NearlinkRemoteDeviceRssiObserver> observer)
{
    auto adapter = WrapRssiObserver(observer);
    if (adapter == nullptr) {
        return NL_ERR_INVALID_PARAM;
    }
    NlErrCode ret = impl_.RegisterRssiObserver(adapter);
    if (ret != NL_NO_ERROR) {
        EraseRssiObserver(observer);
    }
    return ret;
}

NlErrCode NearlinkHost::DeregisterRssiObserver(std::shared_ptr<NearlinkRemoteDeviceRssiObserver> observer)
{
    NL_CHECK_RETURN_RET(impl_.IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT, "nearlink is not support.");
    std::shared_ptr<NearlinkBaseRemoteDeviceRssiObserver> adapter;
    if (!UnwrapRssiObserver(observer, adapter)) {
        HILOGW("rssi observer has not been registered");
        return NL_NO_ERROR;
    }
    NlErrCode ret = impl_.DeregisterRssiObserver(adapter);
    if (ret == NL_NO_ERROR) {
        EraseRssiObserver(observer);
    }
    return ret;
}

NlErrCode NearlinkHost::LoadNearlinkSa()
{
    return impl_.LoadNearlinkSa();
}

NlErrCode NearlinkHost::EnableNl(const SleAutoConnectPolicy autoConnPolicy)
{
    return impl_.EnableNl(autoConnPolicy);
}

NlErrCode NearlinkHost::EnableNlAsync(const SleAutoConnectPolicy autoConnPolicy)
{
    return impl_.EnableNlAsync(autoConnPolicy);
}

NlErrCode NearlinkHost::DisableNl()
{
    return impl_.DisableNl();
}

NlErrCode NearlinkHost::DisableNlToOff()
{
    return impl_.DisableNlToOff();
}

NlErrCode NearlinkHost::EnableNlToHalf()
{
    return impl_.EnableNlToHalf();
}

NlErrCode NearlinkHost::EnableNlToHalfAsync()
{
    return impl_.EnableNlToHalfAsync();
}

SleStateID NearlinkHost::GetSleFullState()
{
    return impl_.GetSleFullState();
}

bool NearlinkHost::IsSleEnabled() const
{
    return impl_.IsSleEnabled();
}

bool NearlinkHost::IsSleHalfDisabled() const
{
    return impl_.IsSleHalfDisabled();
}

bool NearlinkHost::IsSleDisabled() const
{
    return impl_.IsSleDisabled();
}

bool NearlinkHost::IsSleAvailableToCaller() const
{
    return impl_.IsSleAvailableToCaller();
}

bool NearlinkHost::IsNearlinkSupport() const
{
    return impl_.IsNearlinkSupport();
}

bool NearlinkHost::IsNearlinkSupportFrame4() const
{
    return impl_.IsNearlinkSupportFrame4();
}

bool NearlinkHost::IsNearlinkAudioSupport() const
{
    return impl_.IsNearlinkAudioSupport();
}

NlErrCode NearlinkHost::NearlinkFactoryReset()
{
    return impl_.NearlinkFactoryReset();
}

NlErrCode NearlinkHost::SetFreqHopping(const std::vector<uint8_t> &freq)
{
    return impl_.SetFreqHopping(freq);
}

NlErrCode NearlinkHost::GetAdapterConnectState(int &state) const
{
    return impl_.GetAdapterConnectState(state);
}

NlErrCode NearlinkHost::GetProfileConnState(const std::string &remoteAddr, int &connState) const
{
    return impl_.GetProfileConnState(remoteAddr, connState);
}

NlErrCode NearlinkHost::GetLocalAddress(std::string &localDeviceAddres) const
{
    return impl_.GetLocalAddress(localDeviceAddres);
}

NlErrCode NearlinkHost::GetLocalName(std::string &localDeviceName) const
{
    return impl_.GetLocalName(localDeviceName);
}

NlErrCode NearlinkHost::SetLocalName(const std::string &name)
{
    return impl_.SetLocalName(name);
}

NlErrCode NearlinkHost::GetPairedDevices(int transport, std::vector<NearlinkRemoteDevice> &pairedDevices) const
{
    std::vector<NearlinkBaseRemoteDevice> baseDevices;
    NlErrCode ret = impl_.GetPairedDevices(transport, baseDevices);
    if (ret != NL_NO_ERROR) {
        return ret;
    }

    for (auto &device : baseDevices) {
        NearlinkRemoteDevice mirrorDevice;
        static_cast<NearlinkBaseRemoteDevice&>(mirrorDevice) = device;
        pairedDevices.push_back(std::move(mirrorDevice));
    }
    return NL_NO_ERROR;
}

NlErrCode NearlinkHost::SetConnectionMode(int32_t connectionMode, int32_t duration)
{
    return impl_.SetConnectionMode(connectionMode, duration);
}

NlErrCode NearlinkHost::RemovePair(const NearlinkRemoteDevice &device)
{
    return impl_.RemovePair(device);
}

NlErrCode NearlinkHost::RemoveAllPairs()
{
    return impl_.RemoveAllPairs();
}

NlErrCode NearlinkHost::GetLinkRole(const NearlinkRemoteDevice &device, uint8_t &role) const
{
    return impl_.GetLinkRole(device, role);
}

NlErrCode NearlinkHost::RegisterRemoteDeviceObserver(std::shared_ptr<NearlinkRemoteDeviceObserver> observer)
{
    auto adapter = WrapRemoteDeviceObserver(observer);
    if (adapter == nullptr) {
        return NL_ERR_INVALID_PARAM;
    }
    NlErrCode ret = impl_.RegisterRemoteDeviceObserver(adapter);
    if (ret != NL_NO_ERROR) {
        EraseRemoteDeviceObserver(observer);
    }
    return ret;
}

NlErrCode NearlinkHost::DeregisterRemoteDeviceObserver(std::shared_ptr<NearlinkRemoteDeviceObserver> observer)
{
    NL_CHECK_RETURN_RET(impl_.IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT, "nearlink is not support.");
    std::shared_ptr<NearlinkBaseRemoteDeviceObserver> adapter;
    if (!UnwrapRemoteDeviceObserver(observer, adapter)) {
        HILOGW("remote device observer has not been registered");
        return NL_NO_ERROR;
    }
    NlErrCode ret = impl_.DeregisterRemoteDeviceObserver(adapter);
    if (ret == NL_NO_ERROR) {
        EraseRemoteDeviceObserver(observer);
    }
    return ret;
}

NlErrCode NearlinkHost::GetSleMaxAdvertisingDataLength(uint32_t &maxAdvDataLen) const
{
    return impl_.GetSleMaxAdvertisingDataLength(maxAdvDataLen);
}

void NearlinkHost::LoadSystemAbilitySuccess(const sptr<IRemoteObject> &remoteObject)
{
    impl_.LoadSystemAbilitySuccess(remoteObject);
}

void NearlinkHost::LoadSystemAbilityFail()
{
    impl_.LoadSystemAbilityFail();
}

NlErrCode NearlinkHost::ConnectAllowedProfiles(const std::string &remoteAddr) const
{
    return impl_.ConnectAllowedProfiles(remoteAddr);
}

NlErrCode NearlinkHost::DisconnectAllowedProfiles(const std::string &remoteAddr) const
{
    return impl_.DisconnectAllowedProfiles(remoteAddr);
}

NlErrCode NearlinkHost::UpdateSleVirtualDevice(int32_t cmd, const std::string &address)
{
    return impl_.UpdateSleVirtualDevice(cmd, address);
}

NlErrCode NearlinkHost::UpdateRefusePolicy(const int32_t protocolType, const int32_t pid, const int64_t refuseTime)
{
    return impl_.UpdateRefusePolicy(protocolType, pid, refuseTime);
}

NlErrCode NearlinkHost::CheckPermissionForNapi(const std::string &permission, bool &isGranted)
{
    return impl_.CheckPermissionForNapi(permission, isGranted);
}

NlErrCode NearlinkHost::GetSleAddrByBtAddr(const std::string &btAddr, std::string &sleAddr)
{
    return impl_.GetSleAddrByBtAddr(btAddr, sleAddr);
}

NlErrCode NearlinkHost::GetBtAddrBySleAddr(const std::string &sleAddr, std::string &btAddr)
{
    return impl_.GetBtAddrBySleAddr(sleAddr, btAddr);
}

NlErrCode NearlinkHost::SetBtAddrBySleAddr(const std::string &sleAddr, const std::string &btAddr)
{
    return impl_.SetBtAddrBySleAddr(sleAddr, btAddr);
}

bool NearlinkHost::IsFeatureSupported(SleFeatureSupported feature)
{
    return impl_.IsFeatureSupported(feature);
}

bool NearlinkHost::IsConnectionExist()
{
    return impl_.IsConnectionExist();
}

bool NearlinkHost::IsBasSupported()
{
    return impl_.IsBasSupported();
}

NearlinkHost::NearlinkHost() : impl_(NearlinkBaseHost::GetInstance())
{
}

NearlinkHost::~NearlinkHost()
{
}

}  // namespace Nearlink
}  // namespace OHOS
