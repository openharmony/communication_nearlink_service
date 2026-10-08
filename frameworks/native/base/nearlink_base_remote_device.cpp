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

#include <string>

#include "nearlink_raw_address.h"
#include "nearlink_def.h"
#include "nearlink_base_host.h"
#include "nearlink_base_host_proxy.h"
#include "log_util.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"
#include "nearlink_base_remote_device.h"
#include "nearlink_utils.h"
#include "i_nearlink_sle_controller.h"
#include "nearlink_base_sa_manager.h"

namespace OHOS {
namespace Nearlink {

sptr<NearlinkBaseHostProxy> GetHostProxy();

namespace {
constexpr uint8_t PASS_CODE_LEN = 6;

bool IsRemoteProfileSupported(const std::string &address, int32_t transport, std::string profUuid)
{
    sptr<NearlinkBaseHostProxy> hostProxy = GetHostProxy();
    NL_CHECK_RETURN_RET(hostProxy != nullptr, NL_ERR_UNAVAILABLE_PROXY, "fails: no proxy");
    std::vector<std::string> uuids;
    NlErrCode ret = hostProxy->GetDeviceUuids(transport, address, uuids);
    if (ret != NL_NO_ERROR) {
        HILOGE("get device uuids failed, ret=%{public}d", ret);
        return true;
    }
    if (uuids.empty()) {
        HILOGW("device uuids is empty, cannot determine profile supported, allow request");
        return true;
    }
    ConvertUuidToUpperCase(profUuid);
    for (auto &uuid : uuids) {
        std::string remoteUuid = uuid;
        ConvertUuidToUpperCase(remoteUuid);
        if (remoteUuid == profUuid) {
            return true;
        }
    }
    return false;
}
}

sptr<NearlinkBaseHostProxy> GetHostProxy()
{
    sptr<ISystemAbilityManager> samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    NL_CHECK_RETURN_RET(samgr, nullptr, "GetSystemAbilityManager fail");
    sptr<IRemoteObject> remote = samgr->GetSystemAbility(NEARLINK_HOST_SYS_ABILITY_ID);
    NL_CHECK_RETURN_RET(remote, nullptr, "GetSystemAbility fail");

    sptr<NearlinkBaseHostProxy> hostProxy = new (std::nothrow) NearlinkBaseHostProxy(remote);
    return hostProxy;
}

NearlinkBaseRemoteDevice::NearlinkBaseRemoteDevice(const std::string &addr, const int transport)
{
    address_ = addr;
    transport_ = transport;
}

bool NearlinkBaseRemoteDevice::IsValidNearlinkRemoteDevice() const
{
    if (!IsValidAddress(address_)) {
        HILOGI("invalid nearlink addr, address_: %{public}s", GetEncryptAddr(address_).c_str());
        return false;
    }

    if ((transport_ != static_cast<int>(NlTransportType::NL_TRANSPORT_SLB)) &&
        (transport_ != static_cast<int>(NlTransportType::NL_TRANSPORT_SLE))) {
        HILOGI("invalid transport type.");
        return false;
    }
    return true;
}

int NearlinkBaseRemoteDevice::GetTransportType() const
{
    return transport_;
}

NlErrCode NearlinkBaseRemoteDevice::GetDeviceName(std::string &name) const
{
    NL_CHECK_RETURN_RET(NearlinkBaseHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidNearlinkRemoteDevice(), NL_ERR_INTERNAL_ERROR, "Invalid remote device");
    NL_CHECK_RETURN_RET(NearlinkBaseHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<NearlinkBaseHostProxy> hostProxy = GetHostProxy();
    NL_CHECK_RETURN_RET(hostProxy != nullptr, NL_ERR_UNAVAILABLE_PROXY, "fails: no proxy");
    return hostProxy->GetDeviceName(transport_, address_, name);
}

NlErrCode NearlinkBaseRemoteDevice::GetDeviceAlias(std::string &alias) const
{
    NL_CHECK_RETURN_RET(NearlinkBaseHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidNearlinkRemoteDevice(), NL_ERR_INTERNAL_ERROR, "Invalid remote device");
    NL_CHECK_RETURN_RET(NearlinkBaseHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<NearlinkBaseHostProxy> hostProxy = GetHostProxy();
    NL_CHECK_RETURN_RET(hostProxy != nullptr, NL_ERR_INTERNAL_ERROR, "fails: no proxy");
    return hostProxy->GetDeviceAlias(transport_, address_, alias);
}

NlErrCode NearlinkBaseRemoteDevice::SetDeviceAlias(const std::string &alias)
{
    HILOGD("enter");
    NL_CHECK_RETURN_RET(NearlinkBaseHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidNearlinkRemoteDevice(), NL_ERR_INTERNAL_ERROR, "Invalid remote device");
    NL_CHECK_RETURN_RET(NearlinkBaseHost::GetInstance().IsSleEnabled(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<NearlinkBaseHostProxy> hostProxy = GetHostProxy();
    NL_CHECK_RETURN_RET(hostProxy != nullptr, NL_ERR_INTERNAL_ERROR, "fails: no proxy");
    return hostProxy->SetDeviceAlias(transport_, address_, alias);
}

NlErrCode NearlinkBaseRemoteDevice::GetPairState(int &pairState) const
{
    HILOGI("enter");
    NL_CHECK_RETURN_RET(NearlinkBaseHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidNearlinkRemoteDevice(), NL_ERR_INTERNAL_ERROR, "Invalid remote device");
    NL_CHECK_RETURN_RET(NearlinkBaseHost::GetInstance().IsSleEnabled(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<NearlinkBaseHostProxy> hostProxy = GetHostProxy();
    NL_CHECK_RETURN_RET(hostProxy != nullptr, NL_ERR_UNAVAILABLE_PROXY, "fails: no proxy");
    return hostProxy->GetPairState(transport_, address_, pairState);
}

NlErrCode NearlinkBaseRemoteDevice::StartPair()
{
    HILOGI("enter");
    NL_CHECK_RETURN_RET(NearlinkBaseHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidNearlinkRemoteDevice(), NL_ERR_INTERNAL_ERROR, "Invalid remote device");
    NL_CHECK_RETURN_RET(NearlinkBaseHost::GetInstance().IsSleEnabled(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<NearlinkBaseHostProxy> hostProxy = GetHostProxy();
    NL_CHECK_RETURN_RET(hostProxy != nullptr, NL_ERR_UNAVAILABLE_PROXY, "fails: no proxy");
    return hostProxy->StartPair(transport_, address_);
}

NlErrCode NearlinkBaseRemoteDevice::StartCrediblePair()
{
    HILOGI("enter");
    NL_CHECK_RETURN_RET(NearlinkBaseHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidNearlinkRemoteDevice(), NL_ERR_INTERNAL_ERROR, "Invalid remote device");
    NL_CHECK_RETURN_RET(NearlinkBaseHost::GetInstance().IsSleEnabled(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<NearlinkBaseHostProxy> hostProxy = GetHostProxy();
    NL_CHECK_RETURN_RET(hostProxy != nullptr, NL_ERR_INTERNAL_ERROR, "fails: no proxy");
    return hostProxy->StartCrediblePair(transport_, address_);
}

NlErrCode NearlinkBaseRemoteDevice::CancelDevicePairing()
{
    HILOGI("enter");
    NL_CHECK_RETURN_RET(NearlinkBaseHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidNearlinkRemoteDevice(), NL_ERR_INTERNAL_ERROR, "Invalid remote device");
    NL_CHECK_RETURN_RET(NearlinkBaseHost::GetInstance().IsSleEnabled(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<NearlinkBaseHostProxy> hostProxy = GetHostProxy();
    NL_CHECK_RETURN_RET(hostProxy != nullptr, NL_ERR_INTERNAL_ERROR, "fails: no proxy");
    return hostProxy->CancelPairing(transport_, address_);
}

NlErrCode NearlinkBaseRemoteDevice::SetPairingPassCode(const std::string &passCode)
{
    HILOGD("enter");
    NL_CHECK_RETURN_RET(NearlinkBaseHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidNearlinkRemoteDevice(), NL_ERR_INTERNAL_ERROR, "Invalid remote device");
    NL_CHECK_RETURN_RET(NearlinkBaseHost::GetInstance().IsSleEnabled(), NL_ERR_SLE_OFF, "nearlink is off.");
    NL_CHECK_RETURN_RET(IsValidPassCode(passCode), NL_ERR_INVALID_PARAM, "passCode invalid.");
    sptr<NearlinkBaseHostProxy> hostProxy = GetHostProxy();
    NL_CHECK_RETURN_RET(hostProxy != nullptr, NL_ERR_INTERNAL_ERROR, "fails: no proxy");
    return hostProxy->SetPairingPassCode(transport_, address_, passCode);
}

NlErrCode NearlinkBaseRemoteDevice::SetPairingConfirmation(bool cfm)
{
    HILOGD("enter");
    NL_CHECK_RETURN_RET(NearlinkBaseHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidNearlinkRemoteDevice(), NL_ERR_INTERNAL_ERROR, "Invalid remote device");
    NL_CHECK_RETURN_RET(NearlinkBaseHost::GetInstance().IsSleEnabled(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<NearlinkBaseHostProxy> hostProxy = GetHostProxy();
    NL_CHECK_RETURN_RET(hostProxy != nullptr, NL_ERR_INTERNAL_ERROR, "fails: no proxy");
    return hostProxy->SetPairingConfirmation(transport_, address_, cfm);
}

NlErrCode NearlinkBaseRemoteDevice::IsBondedFromLocal(bool &isBondedFromLocal) const
{
    HILOGI("enter");
    NL_CHECK_RETURN_RET(NearlinkBaseHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidNearlinkRemoteDevice(), NL_ERR_INTERNAL_ERROR, "Invalid remote device");
    sptr<NearlinkBaseHostProxy> hostProxy = GetHostProxy();
    NL_CHECK_RETURN_RET(hostProxy != nullptr, NL_ERR_UNAVAILABLE_PROXY, "fails: no proxy");
    return hostProxy->IsBondedFromLocal(transport_, address_, isBondedFromLocal);
}

NlErrCode NearlinkBaseRemoteDevice::IsAcbConnected(bool &isAcbConnected) const
{
    HILOGD("enter");
    NL_CHECK_RETURN_RET(NearlinkBaseHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidNearlinkRemoteDevice(), NL_ERR_INTERNAL_ERROR, "Invalid remote device");
    sptr<NearlinkBaseHostProxy> hostProxy = GetHostProxy();
    NL_CHECK_RETURN_RET(hostProxy != nullptr, NL_ERR_UNAVAILABLE_PROXY, "fails: no proxy");
    return hostProxy->IsAcbConnected(transport_, address_, isAcbConnected);
}

NlErrCode NearlinkBaseRemoteDevice::IsAcbEncrypted(bool &isAcbEncrypted) const
{
    HILOGI("enter");
    NL_CHECK_RETURN_RET(NearlinkBaseHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidNearlinkRemoteDevice(), NL_ERR_INTERNAL_ERROR, "Invalid remote device");
    sptr<NearlinkBaseHostProxy> hostProxy = GetHostProxy();
    NL_CHECK_RETURN_RET(hostProxy != nullptr, NL_ERR_UNAVAILABLE_PROXY, "fails: no proxy");
    return hostProxy->IsAcbEncrypted(transport_, address_, isAcbEncrypted);
}

NlErrCode NearlinkBaseRemoteDevice::GetDeviceUuids(std::vector<std::string> &uuids) const
{
    HILOGI("enter");
    NL_CHECK_RETURN_RET(NearlinkBaseHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidNearlinkRemoteDevice(), NL_ERR_INTERNAL_ERROR, "Invalid remote device");
    NL_CHECK_RETURN_RET(NearlinkBaseHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<NearlinkBaseHostProxy> hostProxy = GetHostProxy();
    NL_CHECK_RETURN_RET(hostProxy != nullptr, NL_ERR_UNAVAILABLE_PROXY, "fails: no proxy");
    return hostProxy->GetDeviceUuids(transport_, address_, uuids);
}

NlErrCode NearlinkBaseRemoteDevice::PairRequestReply(bool accept)
{
    HILOGI("enter, accept: %{public}d", accept);
    NL_CHECK_RETURN_RET(NearlinkBaseHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidNearlinkRemoteDevice(), NL_ERR_INTERNAL_ERROR, "Invalid remote device");
    NL_CHECK_RETURN_RET(NearlinkBaseHost::GetInstance().IsSleEnabled(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<NearlinkBaseHostProxy> hostProxy = GetHostProxy();
    NL_CHECK_RETURN_RET(hostProxy != nullptr, NL_ERR_UNAVAILABLE_PROXY, "fails: no proxy");
    return hostProxy->PairRequestReply(transport_, address_, accept);
}

NlErrCode NearlinkBaseRemoteDevice::UpdateConnectInterval(ConnectionInterval intervalType) const
{
    HILOGI("enter");
    NL_CHECK_RETURN_RET(
        NearlinkBaseHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT, "nearlink is not support.");
    NL_CHECK_RETURN_RET(((intervalType <= ConnectionInterval::LOW_SPEED_INTERVAL_500) &&
                            ((intervalType >= ConnectionInterval::HIGH_SPEED_INTERVAL_4_5))),
        NL_ERR_INVALID_PARAM,
        "intervalType invalid");
    NL_CHECK_RETURN_RET(IsValidNearlinkRemoteDevice(), NL_ERR_INTERNAL_ERROR, "Invalid remote device");
    NL_CHECK_RETURN_RET(NearlinkBaseHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<INearlinkSleController> proxy = GetProxy<INearlinkSleController>(NEARLINK_SLE_CONTROLLER_SERVER);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr.");

    NlErrCode errCode = proxy->UpdateConnectInterval(address_, static_cast<int32_t>(intervalType));
    NL_CHECK_RETURN_RET(errCode == NL_NO_ERROR, errCode,
        "UpdateConnectInterval failed, error code: %{public}d", errCode);
    HILOGI("intervalType = %{public}d", intervalType);
    return errCode;
}

NlErrCode NearlinkBaseRemoteDevice::ReadRemoteRssiValue()
{
    HILOGI("enter");
    NL_CHECK_RETURN_RET(NearlinkBaseHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidNearlinkRemoteDevice(), NL_ERR_INTERNAL_ERROR, "Invalid remote device");
    NL_CHECK_RETURN_RET(NearlinkBaseHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<NearlinkBaseHostProxy> hostProxy = GetHostProxy();
    NL_CHECK_RETURN_RET(hostProxy != nullptr, NL_ERR_UNAVAILABLE_PROXY, "fails: no proxy");
    return hostProxy->ReadRemoteRssiValue(address_);
}

NlErrCode NearlinkBaseRemoteDevice::GetDeviceAppearance(int &appearance) const
{
    HILOGD("enter");
    NL_CHECK_RETURN_RET(NearlinkBaseHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidNearlinkRemoteDevice(), NL_ERR_INTERNAL_ERROR, "Invalid remote device");
    NL_CHECK_RETURN_RET(NearlinkBaseHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<NearlinkBaseHostProxy> hostProxy = GetHostProxy();
    NL_CHECK_RETURN_RET(hostProxy != nullptr, NL_ERR_UNAVAILABLE_PROXY, "fails: no proxy");
    return hostProxy->GetDeviceAppearance(address_, appearance);
}

NlErrCode NearlinkBaseRemoteDevice::GetDeviceProductId(uint16_t &productId) const
{
    HILOGD("enter");
    NL_CHECK_RETURN_RET(NearlinkBaseHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidNearlinkRemoteDevice(), NL_ERR_INTERNAL_ERROR, "Invalid remote device.");
    NL_CHECK_RETURN_RET(NearlinkBaseHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<NearlinkBaseHostProxy> hostProxy = GetHostProxy();
    NL_CHECK_RETURN_RET(hostProxy != nullptr, NL_ERR_INTERNAL_ERROR, "fails: no proxy");
    return hostProxy->GetDeviceProductId(address_, productId);
}

NlErrCode NearlinkBaseRemoteDevice::GetDeviceVendorId(uint16_t &vendorId) const
{
    HILOGD("enter");
    NL_CHECK_RETURN_RET(NearlinkBaseHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidNearlinkRemoteDevice(), NL_ERR_INTERNAL_ERROR, "Invalid remote device.");
    NL_CHECK_RETURN_RET(NearlinkBaseHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<NearlinkBaseHostProxy> hostProxy = GetHostProxy();
    NL_CHECK_RETURN_RET(hostProxy != nullptr, NL_ERR_INTERNAL_ERROR, "fails: no proxy");
    return hostProxy->GetDeviceVendorId(address_, vendorId);
}

NlErrCode NearlinkBaseRemoteDevice::GetDeviceModel(DeviceModel &model) const
{
    HILOGD("enter");
    NL_CHECK_RETURN_RET(IsValidNearlinkRemoteDevice(), NL_ERR_INTERNAL_ERROR, "Invalid remote device");
    NL_CHECK_RETURN_RET(NearlinkBaseHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<NearlinkBaseHostProxy> hostProxy = GetHostProxy();
    NL_CHECK_RETURN_RET(hostProxy != nullptr, NL_ERR_UNAVAILABLE_PROXY, "fails: no proxy");
    NearlinkDeviceModel deviceModel;
    NlErrCode ret = hostProxy->GetDeviceModel(address_, deviceModel);
    NL_CHECK_RETURN_RET(ret == NL_NO_ERROR, ret, "get device model failed, ret=%{public}d", ret);
    model = deviceModel;
    return NL_NO_ERROR;
}

NlErrCode NearlinkBaseRemoteDevice::GetDeviceInformation(DeviceInformation &information) const
{
    HILOGD("enter");
    NL_CHECK_RETURN_RET(NearlinkBaseHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidNearlinkRemoteDevice(), NL_ERR_INTERNAL_ERROR, "Invalid remote device");
    NL_CHECK_RETURN_RET(NearlinkBaseHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<NearlinkBaseHostProxy> hostProxy = GetHostProxy();
    NL_CHECK_RETURN_RET(hostProxy != nullptr, NL_ERR_INTERNAL_ERROR, "fails: no proxy");

    NearlinkDeviceInformation deviceInformation;
    NlErrCode ret = hostProxy->GetDeviceInformation(address_, deviceInformation);
    NL_CHECK_RETURN_RET(ret == NL_NO_ERROR, ret, "get device infomation failed, ret=%{public}d", ret);
    information = deviceInformation;
    return NL_NO_ERROR;
}
NlErrCode NearlinkBaseRemoteDevice::GetAcbState(int &acbState) const
{
    HILOGI("enter");
    NL_CHECK_RETURN_RET(NearlinkBaseHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidNearlinkRemoteDevice(), NL_ERR_INTERNAL_ERROR, "Invalid remote device");
    NL_CHECK_RETURN_RET(NearlinkBaseHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<NearlinkBaseHostProxy> hostProxy = GetHostProxy();
    NL_CHECK_RETURN_RET(hostProxy != nullptr, NL_ERR_UNAVAILABLE_PROXY, "fails: no proxy");
    return hostProxy->GetAcbState(address_, acbState);
}

NlErrCode NearlinkBaseRemoteDevice::GetBatteryLevel()
{
    HILOGI("enter");
    NL_CHECK_RETURN_RET(NearlinkBaseHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(NearlinkBaseHost::GetInstance().IsBasSupported(), NL_ERR_FEATURE_NOT_SUPPORT,
                        "Battery service is not supported.");
    NL_CHECK_RETURN_RET(IsValidNearlinkRemoteDevice(), NL_ERR_INTERNAL_ERROR, "Invalid remote device");
    NL_CHECK_RETURN_RET(NearlinkBaseHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");
    NL_CHECK_RETURN_RET(IsRemoteProfileSupported(address_, transport_, SLE_UUID_BAS_PROFILE),
        NL_ERR_PEER_NOT_SUPPORT_BATTERY_SERVICE, "remote device does not support BAS service.");
    sptr<NearlinkBaseHostProxy> hostProxy = GetHostProxy();
    NL_CHECK_RETURN_RET(hostProxy != nullptr, NL_ERR_UNAVAILABLE_PROXY, "fails: no proxy");
    return hostProxy->GetBatteryLevel(address_);
}

bool NearlinkBaseRemoteDevice::IsValidPassCode(const std::string& passCode)
{
    if (passCode.length() != PASS_CODE_LEN) {
        return false;
    }
    for (char c : passCode) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            return false;
        }
    }
    return true;
}
}  // namespace Nearlink
}  // namespace OHOS
