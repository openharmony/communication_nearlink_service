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
#include "nearlink_host.h"
#include "nearlink_host_proxy.h"
#include "log_util.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"
#include "nearlink_remote_device.h"
#include "nearlink_utils.h"
#include "i_nearlink_sle_controller.h"
#include "nearlink_sa_manager.h"

namespace OHOS {
namespace Nearlink {
namespace {
constexpr uint8_t PASS_CODE_LEN = 6;
}

sptr<NearlinkHostProxy> GetHostProxy()
{
    sptr<ISystemAbilityManager> samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    sptr<IRemoteObject> remote = samgr->GetSystemAbility(NEARLINK_HOST_SYS_ABILITY_ID);

    sptr<NearlinkHostProxy> hostProxy = new (std::nothrow) NearlinkHostProxy(remote);
    return hostProxy;
}

NearlinkRemoteDevice::NearlinkRemoteDevice(const std::string &addr, const int transport)
{
    address_ = addr;
    transport_ = transport;
}

bool NearlinkRemoteDevice::IsValidNearlinkRemoteDevice() const
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

int NearlinkRemoteDevice::GetTransportType() const
{
    return transport_;
}

NlErrCode NearlinkRemoteDevice::GetDeviceName(std::string &name) const
{
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidNearlinkRemoteDevice(), NL_ERR_INTERNAL_ERROR, "Invalid remote device");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<NearlinkHostProxy> hostProxy = GetHostProxy();
    NL_CHECK_RETURN_RET(hostProxy != nullptr, NL_ERR_UNAVAILABLE_PROXY, "fails: no proxy");
    return hostProxy->GetDeviceName(transport_, address_, name);
}

NlErrCode NearlinkRemoteDevice::GetDeviceAlias(std::string &alias) const
{
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidNearlinkRemoteDevice(), NL_ERR_INTERNAL_ERROR, "Invalid remote device");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<NearlinkHostProxy> hostProxy = GetHostProxy();
    NL_CHECK_RETURN_RET(hostProxy != nullptr, NL_ERR_INTERNAL_ERROR, "fails: no proxy");
    return hostProxy->GetDeviceAlias(transport_, address_, alias);
}

NlErrCode NearlinkRemoteDevice::SetDeviceAlias(const std::string &alias)
{
    HILOGD("enter");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidNearlinkRemoteDevice(), NL_ERR_INTERNAL_ERROR, "Invalid remote device");
    NL_CHECK_RETURN_RET(IS_SLE_ENABLED(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<NearlinkHostProxy> hostProxy = GetHostProxy();
    NL_CHECK_RETURN_RET(hostProxy != nullptr, NL_ERR_INTERNAL_ERROR, "fails: no proxy");
    return hostProxy->SetDeviceAlias(transport_, address_, alias);
}

NlErrCode NearlinkRemoteDevice::GetPairState(int &pairState) const
{
    HILOGI("enter");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidNearlinkRemoteDevice(), NL_ERR_INTERNAL_ERROR, "Invalid remote device");
    NL_CHECK_RETURN_RET(IS_SLE_ENABLED(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<NearlinkHostProxy> hostProxy = GetHostProxy();
    NL_CHECK_RETURN_RET(hostProxy != nullptr, NL_ERR_UNAVAILABLE_PROXY, "fails: no proxy");
    return hostProxy->GetPairState(transport_, address_, pairState);
}

NlErrCode NearlinkRemoteDevice::StartPair()
{
    HILOGI("enter");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidNearlinkRemoteDevice(), NL_ERR_INTERNAL_ERROR, "Invalid remote device");
    NL_CHECK_RETURN_RET(IS_SLE_ENABLED(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<NearlinkHostProxy> hostProxy = GetHostProxy();
    NL_CHECK_RETURN_RET(hostProxy != nullptr, NL_ERR_UNAVAILABLE_PROXY, "fails: no proxy");
    return hostProxy->StartPair(transport_, address_);
}

NlErrCode NearlinkRemoteDevice::StartCrediblePair()
{
    HILOGI("enter");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidNearlinkRemoteDevice(), NL_ERR_INTERNAL_ERROR, "Invalid remote device");
    NL_CHECK_RETURN_RET(IS_SLE_ENABLED(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<NearlinkHostProxy> hostProxy = GetHostProxy();
    NL_CHECK_RETURN_RET(hostProxy != nullptr, NL_ERR_INTERNAL_ERROR, "fails: no proxy");
    return hostProxy->StartCrediblePair(transport_, address_);
}

NlErrCode NearlinkRemoteDevice::CancelDevicePairing()
{
    HILOGI("enter");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidNearlinkRemoteDevice(), NL_ERR_INTERNAL_ERROR, "Invalid remote device");
    NL_CHECK_RETURN_RET(IS_SLE_ENABLED(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<NearlinkHostProxy> hostProxy = GetHostProxy();
    NL_CHECK_RETURN_RET(hostProxy != nullptr, NL_ERR_INTERNAL_ERROR, "fails: no proxy");
    return hostProxy->CancelPairing(transport_, address_);
}

NlErrCode NearlinkRemoteDevice::SetPairingPassCode(const std::string &passCode)
{
    HILOGD("enter");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidNearlinkRemoteDevice(), NL_ERR_INTERNAL_ERROR, "Invalid remote device");
    NL_CHECK_RETURN_RET(IS_SLE_ENABLED(), NL_ERR_SLE_OFF, "nearlink is off.");
    NL_CHECK_RETURN_RET(IsValidPassCode(passCode), NL_ERR_INVALID_PARAM, "passCode invalid.");
    sptr<NearlinkHostProxy> hostProxy = GetHostProxy();
    NL_CHECK_RETURN_RET(hostProxy != nullptr, NL_ERR_INTERNAL_ERROR, "fails: no proxy");
    return hostProxy->SetPairingPassCode(transport_, address_, passCode);
}

NlErrCode NearlinkRemoteDevice::SetPairingConfirmation(bool cfm)
{
    HILOGD("enter");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidNearlinkRemoteDevice(), NL_ERR_INTERNAL_ERROR, "Invalid remote device");
    NL_CHECK_RETURN_RET(IS_SLE_ENABLED(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<NearlinkHostProxy> hostProxy = GetHostProxy();
    NL_CHECK_RETURN_RET(hostProxy != nullptr, NL_ERR_INTERNAL_ERROR, "fails: no proxy");
    return hostProxy->SetPairingConfirmation(transport_, address_, cfm);
}

NlErrCode NearlinkRemoteDevice::IsBondedFromLocal(bool &isBondedFromLocal) const
{
    HILOGI("enter");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidNearlinkRemoteDevice(), NL_ERR_INTERNAL_ERROR, "Invalid remote device");
    sptr<NearlinkHostProxy> hostProxy = GetHostProxy();
    NL_CHECK_RETURN_RET(hostProxy != nullptr, NL_ERR_UNAVAILABLE_PROXY, "fails: no proxy");
    return hostProxy->IsBondedFromLocal(transport_, address_, isBondedFromLocal);
}

NlErrCode NearlinkRemoteDevice::IsAcbConnected(bool &isAcbConnected) const
{
    HILOGD("enter");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidNearlinkRemoteDevice(), NL_ERR_INTERNAL_ERROR, "Invalid remote device");
    sptr<NearlinkHostProxy> hostProxy = GetHostProxy();
    NL_CHECK_RETURN_RET(hostProxy != nullptr, NL_ERR_UNAVAILABLE_PROXY, "fails: no proxy");
    return hostProxy->IsAcbConnected(transport_, address_, isAcbConnected);
}

NlErrCode NearlinkRemoteDevice::IsAcbEncrypted(bool &isAcbEncrypted) const
{
    HILOGI("enter");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidNearlinkRemoteDevice(), NL_ERR_INTERNAL_ERROR, "Invalid remote device");
    sptr<NearlinkHostProxy> hostProxy = GetHostProxy();
    NL_CHECK_RETURN_RET(hostProxy != nullptr, NL_ERR_UNAVAILABLE_PROXY, "fails: no proxy");
    return hostProxy->IsAcbEncrypted(transport_, address_, isAcbEncrypted);
}

NlErrCode NearlinkRemoteDevice::GetDeviceUuids(std::vector<std::string> &uuids) const
{
    HILOGI("enter");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidNearlinkRemoteDevice(), NL_ERR_INTERNAL_ERROR, "Invalid remote device");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<NearlinkHostProxy> hostProxy = GetHostProxy();
    NL_CHECK_RETURN_RET(hostProxy != nullptr, NL_ERR_UNAVAILABLE_PROXY, "fails: no proxy");
    return hostProxy->GetDeviceUuids(transport_, address_, uuids);
}

NlErrCode NearlinkRemoteDevice::PairRequestReply(bool accept)
{
    HILOGI("enter, accept: %{public}d", accept);
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidNearlinkRemoteDevice(), NL_ERR_INTERNAL_ERROR, "Invalid remote device");
    NL_CHECK_RETURN_RET(IS_SLE_ENABLED(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<NearlinkHostProxy> hostProxy = GetHostProxy();
    NL_CHECK_RETURN_RET(hostProxy != nullptr, NL_ERR_UNAVAILABLE_PROXY, "fails: no proxy");
    return hostProxy->PairRequestReply(transport_, address_, accept);
}

NlErrCode NearlinkRemoteDevice::UpdateConnectInterval(ConnectionInterval intervalType) const
{
    HILOGI("enter");
    NL_CHECK_RETURN_RET(
        NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT, "nearlink is not support.");
    NL_CHECK_RETURN_RET(((intervalType <= ConnectionInterval::LOW_SPEED_INTERVAL_500) &&
                            ((intervalType >= ConnectionInterval::HIGH_SPEED_INTERVAL_4_5))),
        NL_ERR_INVALID_PARAM,
        "intervalType invalid");
    NL_CHECK_RETURN_RET(IsValidNearlinkRemoteDevice(), NL_ERR_INTERNAL_ERROR, "Invalid remote device");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<INearlinkSleController> proxy = GetProxy<INearlinkSleController>(NEARLINK_SLE_CONTROLLER_SERVER);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr.");

    NlErrCode errCode = proxy->UpdateConnectInterval(address_, static_cast<int32_t>(intervalType));
    NL_CHECK_RETURN_RET(errCode == NL_NO_ERROR, errCode,
        "UpdateConnectInterval failed, error code: %{public}d", errCode);
    HILOGI("intervalType = %{public}d", intervalType);
    return errCode;
}

NlErrCode NearlinkRemoteDevice::ReadRemoteRssiValue()
{
    HILOGI("enter");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidNearlinkRemoteDevice(), NL_ERR_INTERNAL_ERROR, "Invalid remote device");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<NearlinkHostProxy> hostProxy = GetHostProxy();
    NL_CHECK_RETURN_RET(hostProxy != nullptr, NL_ERR_UNAVAILABLE_PROXY, "fails: no proxy");
    return hostProxy->ReadRemoteRssiValue(address_);
}

NlErrCode NearlinkRemoteDevice::GetDeviceAppearance(int &appearance) const
{
    HILOGD("enter");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidNearlinkRemoteDevice(), NL_ERR_INTERNAL_ERROR, "Invalid remote device");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<NearlinkHostProxy> hostProxy = GetHostProxy();
    NL_CHECK_RETURN_RET(hostProxy != nullptr, NL_ERR_UNAVAILABLE_PROXY, "fails: no proxy");
    return hostProxy->GetDeviceAppearance(address_, appearance);
}

NlErrCode NearlinkRemoteDevice::GetDeviceProductId(uint16_t &productId) const
{
    HILOGD("enter");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidNearlinkRemoteDevice(), NL_ERR_INTERNAL_ERROR, "Invalid remote device.");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<NearlinkHostProxy> hostProxy = GetHostProxy();
    NL_CHECK_RETURN_RET(hostProxy != nullptr, NL_ERR_INTERNAL_ERROR, "fails: no proxy");
    return hostProxy->GetDeviceProductId(address_, productId);
}

NlErrCode NearlinkRemoteDevice::GetDeviceVendorId(uint16_t &vendorId) const
{
    HILOGD("enter");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidNearlinkRemoteDevice(), NL_ERR_INTERNAL_ERROR, "Invalid remote device.");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<NearlinkHostProxy> hostProxy = GetHostProxy();
    NL_CHECK_RETURN_RET(hostProxy != nullptr, NL_ERR_INTERNAL_ERROR, "fails: no proxy");
    return hostProxy->GetDeviceVendorId(address_, vendorId);
}

NlErrCode NearlinkRemoteDevice::GetDeviceModel(DeviceModel &model) const
{
    HILOGD("enter");
    NL_CHECK_RETURN_RET(IsValidNearlinkRemoteDevice(), NL_ERR_INTERNAL_ERROR, "Invalid remote device");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<NearlinkHostProxy> hostProxy = GetHostProxy();
    NL_CHECK_RETURN_RET(hostProxy != nullptr, NL_ERR_UNAVAILABLE_PROXY, "fails: no proxy");
    NearlinkDeviceModel deviceModel;
    NlErrCode ret = hostProxy->GetDeviceModel(address_, deviceModel);
    NL_CHECK_RETURN_RET(ret == NL_NO_ERROR, ret, "get device model failed, ret=%{public}d", ret);
    model = deviceModel;
    return NL_NO_ERROR;
}

NlErrCode NearlinkRemoteDevice::GetDeviceInformation(DeviceInformation &information) const
{
    HILOGD("enter");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidNearlinkRemoteDevice(), NL_ERR_INTERNAL_ERROR, "Invalid remote device");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<NearlinkHostProxy> hostProxy = GetHostProxy();
    NL_CHECK_RETURN_RET(hostProxy != nullptr, NL_ERR_INTERNAL_ERROR, "fails: no proxy");

    NearlinkDeviceInformation deviceInformation;
    NlErrCode ret = hostProxy->GetDeviceInformation(address_, deviceInformation);
    NL_CHECK_RETURN_RET(ret == NL_NO_ERROR, ret, "get device infomation failed, ret=%{public}d", ret);
    information = deviceInformation;
    return NL_NO_ERROR;
}
NlErrCode NearlinkRemoteDevice::GetAcbState(int &acbState) const
{
    HILOGI("enter");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidNearlinkRemoteDevice(), NL_ERR_INTERNAL_ERROR, "Invalid remote device");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<NearlinkHostProxy> hostProxy = GetHostProxy();
    NL_CHECK_RETURN_RET(hostProxy != nullptr, NL_ERR_UNAVAILABLE_PROXY, "fails: no proxy");
    return hostProxy->GetAcbState(address_, acbState);
}

NlErrCode NearlinkRemoteDevice::GetBatteryLevel()
{
    HILOGI("enter");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        "nearlink is not support.");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsBasSupported(), NL_ERR_FEATURE_NOT_SUPPORT,
                        "Battery service is not supported.");
    NL_CHECK_RETURN_RET(IsValidNearlinkRemoteDevice(), NL_ERR_INTERNAL_ERROR, "Invalid remote device");
    NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<NearlinkHostProxy> hostProxy = GetHostProxy();
    NL_CHECK_RETURN_RET(hostProxy != nullptr, NL_ERR_UNAVAILABLE_PROXY, "fails: no proxy");
    return hostProxy->GetBatteryLevel(address_);
}

bool NearlinkRemoteDevice::IsValidPassCode(const std::string& passCode)
{
    if (passCode.length() != PASS_CODE_LEN) {
        return false;
    }
    for (char c : passCode) {
        if (!std::isdigit(c)) {
            return false;
        }
    }
    return true;
}
}  // namespace Nearlink
}  // namespace OHOS
