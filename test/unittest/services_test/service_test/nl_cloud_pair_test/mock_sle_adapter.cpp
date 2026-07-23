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

#include "SleAdapter.h"
#include "ThreadUtil.h"
#include "interface_cloud_pair_service.h"
#include "log.h"
#include "log_util.h"

namespace OHOS {
namespace Nearlink {
namespace {
static SleAdapter *g_sleAdapterImpl = nullptr;
}

struct SleAdapter::impl {
    impl(SleAdapter &sleAdapter);
    impl(const impl &);
    impl &operator=(const impl &);
    ~impl();

    BaseObserverList<ISlePeripheralCallback> slePeripheralCallback_;
};

SleAdapter::impl::impl(SleAdapter &sleAdapter)
{}

SleAdapter::impl::~impl()
{}

SleAdapter::SleAdapter() : utility::Context(ADAPTER_NAME_SLE, "5.0"), pimpl(std::make_unique<SleAdapter::impl>(*this))
{
    HILOGI("[SleAdapter] %{public}s:Create", Name().c_str());
    g_sleAdapterImpl = this;
}

SleAdapter::~SleAdapter() {}

utility::Context *SleAdapter::GetContext()
{
    return this;
}

void SleAdapter::RegisterSlePeripheralCallback(ISlePeripheralCallback &callback) const
{
    HILOGI("[SleAdapter Mocker] enter");

    pimpl->slePeripheralCallback_.Register(callback);
}

void SleAdapter::DeregisterSlePeripheralCallback(ISlePeripheralCallback &callback) const
{
    HILOGI("[SleAdapter Mocker] enter");
    pimpl->slePeripheralCallback_.Deregister(callback);
}

void SleAdapter::OnAcbStateChanged(const RawAddress &device, int connectState, int reason) const
{
    HILOGI("[SleAdapter Mocker]:addr:%{public}s,acb state change,connectState:%{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), connectState);
    InterfaceCloudPairService::GetInstance().HandleAcbStateChanged(device, connectState, reason);
    pimpl->slePeripheralCallback_.ForEach([device, connectState, reason](ISlePeripheralCallback &observer) {
        observer.OnAcbStateChanged(device, connectState, reason);
    });
}

void SleAdapter::NotifyPairStatusChanged(const RawAddress &device, int preStatus, int status, int reason) const
{
    HILOGI("[SleAdapter Mocker]:addr:%{public}s,pair state change,status:%{public}d->%{public}d,reason:%{public}u",
        GetEncryptAddr(device.GetAddress()).c_str(), preStatus, status, reason);
    InterfaceCloudPairService::GetInstance().HandlePairStatusChanged(device, preStatus, status, reason);
    pimpl->slePeripheralCallback_.ForEach([device, preStatus, status, reason](ISlePeripheralCallback &observer) {
        observer.OnPairStatusChanged(device, preStatus, status, reason);
    });
}

void SleAdapter::ConnectAcb(const RawAddress &device)
{
    HILOGI("[SleAdapter Mocker]:addr:%{public}s,call ConnectAcb", GetEncryptAddr(device.GetAddress()).c_str());
    OnAcbStateChanged(device, static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED), ACB_CONNECT_SUCCESS);
}

void SleAdapter::ClearBgConnDevice() const
{
    HILOGI("[SleAdapter Mocker]:call ClearBgConnDevice");
}

bool SleAdapter::DisconnectAcb(const RawAddress &device, uint8_t discReason) const
{
    HILOGI("[SleAdapter Mocker]:addr:%{public}s,call DisconnectAcb", GetEncryptAddr(device.GetAddress()).c_str());
    OnAcbStateChanged(device, static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED), discReason);
    return true;
}

bool SleAdapter::DisconnectAction(const RawAddress &device, uint8_t discReason) const
{
    HILOGI("[SleAdapter Mocker]:addr:%{public}s,call DisconnectAction", GetEncryptAddr(device.GetAddress()).c_str());
    OnAcbStateChanged(device, static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED), discReason);
    return true;
}

bool SleAdapter::SetAliasName(const RawAddress &device, const std::string &name) const
{
    HILOGI("[SleAdapter Mocker]:addr:%{public}s,set AliasName:%{public}s", GetEncryptAddr(device.GetAddress()).c_str(),
        name.c_str());
    return true;
}

bool SleAdapter::CancelPairing(const RawAddress &device)
{
    InterfaceCloudPairService::GetInstance().CancelCloudPairing(device);
    InterfaceCloudPairService::GetInstance().CancelCloudPairComplete(device,
        static_cast<int>(SlePairState::SLE_PAIR_PAIRED),
        static_cast<uint8_t>(PairingStateChangeReason::PAIRING_FAILURE),
        true, static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED));
    return true;
}

void SleAdapter::RemoveNotPairedCloudDevice(const RawAddress &device) const
{
    return;
}

bool SleAdapter::StartCrediblePair(const RawAddress &device)
{
    HILOGI("[SleAdapter Mocker]:addr:%{public}s call StartCrediblePair", GetEncryptAddr(device.GetAddress()).c_str());
    StartPair(device);
    InterfaceCloudPairService::GetInstance().SetCrediblePairState(device);
    return true;
}

bool SleAdapter::StartPair(const RawAddress &device)
{
    HILOGI("[SleAdapter Mocker]:addr:%{public}s call StartPair", GetEncryptAddr(device.GetAddress()).c_str());
    return true;
}

bool SleAdapter::ConnectAllProfile(const RawAddress &device)
{
    HILOGI("[SleAdapter Mocker]:addr:%{public}s call ConnectAllProfile", GetEncryptAddr(device.GetAddress()).c_str());
    InterfaceCloudPairService::GetInstance().ConnectCloudDeviceAllProfile(device);
    return true;
}

int SleAdapter::GetPairState(const RawAddress &device) const
{
    if (InterfaceCloudPairService::GetInstance().ChkCloudDeviceAndPermission(device)) {
        return static_cast<int>(SlePairState::SLE_PAIR_PAIRED);
    }
    int32_t curCloudPairState = NL_CLOUD_PAIR_STATE::CLOUD_PAIR_INVALID;
    InterfaceCloudPairService::GetInstance().GetCloudPairState(device.GetAddress(), curCloudPairState);
    if (curCloudPairState == NL_CLOUD_PAIR_STATE::CLOUD_PAIR_PAIRED) {
        return static_cast<int>(SlePairState::SLE_PAIR_PAIRED);
    }
    return static_cast<int>(SlePairState::SLE_PAIR_NONE);
}

bool SleAdapter::GetSleAddrByBtAddr(const std::string &btAddr, std::string &sleAddr) const
{
    sleAddr = InterfaceCloudPairService::GetInstance().GetReportAddrByBtAddr(btAddr);
    if (!sleAddr.empty()) {
        return true;
    }
    return false;
}

bool SleAdapter::GetBtAddrBySleAddr(const std::string &sleAddr, std::string &btAddr) const
{
    btAddr = InterfaceCloudPairService::GetInstance().GetBtAddrByReportAddr(sleAddr);
    if (!btAddr.empty()) {
        return true;
    }
    return false;
}

void SleAdapter::NotifyConnectionStateChanged(
    const RawAddress &device, const SleConnectionChangedParam &connChangedParam) const
{
    RawAddress reportAddr(device);
    pimpl->slePeripheralCallback_.ForEach([reportAddr, &connChangedParam](ISlePeripheralCallback &observer) {
        LOG_INFO("OnConnectionStateChanged, newConnState=%{public}d, oldConnState=%{public}d",
            connChangedParam.connState, connChangedParam.connPreState);
        observer.OnConnectionStateChanged(reportAddr, connChangedParam.connState, connChangedParam.connPreState);
    });
}

std::string SleAdapter::GetDeviceName(const RawAddress &device) const
{
    std::string remoteName = "";
    remoteName = InterfaceCloudPairService::GetInstance().GetCloudDeviceAliasName(device);
    return remoteName;
}

std::string SleAdapter::GetAliasName(const RawAddress &device) const
{
    std::string name = "";
    name = InterfaceCloudPairService::GetInstance().GetCloudDeviceAliasName(device);
    return name;
}

std::vector<RawAddress> SleAdapter::GetPairedDevices() const
{
    std::vector<RawAddress> pairedList;
    InterfaceCloudPairService::GetInstance().AddCloudPairDevices(pairedList);
    return pairedList;
}

bool SleAdapter::SetBtAddrBySleAddr(const std::string &sleAddr, const std::string &btAddr) const
{
    return true;
}

std::string SleAdapter::GetLocalAddress() const
{
    return "";
}

std::string SleAdapter::GetLocalName() const
{
    return "";
}

bool SleAdapter::SetLocalName(const std::string &name) const
{
    return true;
}

bool SleAdapter::SleFreqHopping(const std::vector<uint8_t> &freq)
{
    return true;
}

bool SleAdapter::SetBondableMode(int mode) const
{
    return true;
}

int SleAdapter::GetBondableMode() const
{
    return 0;
}

int SleAdapter::GetDeviceAppearance(const RawAddress &device) const
{
    return 0;
}

uint16_t SleAdapter::GetDeviceVendorId(const RawAddress &device) const
{
    return 0;
}

uint16_t SleAdapter::GetDeviceProductId(const RawAddress &device) const
{
    return 0;
}

uint16_t SleAdapter::GetDeviceVersion(const RawAddress &device) const
{
    return 0;
}

DeviceModel SleAdapter::GetDeviceModel(const RawAddress &device) const
{
    DeviceModel model;
    return model;
}

std::vector<Uuid> SleAdapter::GetDeviceUuids(const RawAddress &device) const
{
    return {};
}

std::vector<RawAddress> SleAdapter::GetConnectedDevices() const
{
    return {};
}

void SleAdapter::SetSleConnectionMode(int32_t connectionMode, int32_t duration)
{}

bool SleAdapter::DisconnectAllProfile(const RawAddress &device)
{
    return true;
}

bool SleAdapter::IsBondedFromLocal(const RawAddress &device) const
{
    return true;
}

bool SleAdapter::RemovePair(const RawAddress &device)
{
    return true;
}

bool SleAdapter::RemoveAllPairs()
{
    return true;
}

bool SleAdapter::SetPairingConfirmation(const RawAddress &device) const
{
    return true;
}

bool SleAdapter::SetPairingPassCode(const RawAddress &device, const std::string &passCode)
{
    return true;
}

int SleAdapter::GetAcbState(const RawAddress &device) const
{
    return 0;
}

uint32_t SleAdapter::GetAcbCount() const
{
    return 0;
}

bool SleAdapter::PairRequestReply(const RawAddress &device, bool accept) const
{
    return true;
}

bool SleAdapter::IsAcbConnected(const RawAddress &device) const
{
    return false;
}

bool SleAdapter::IsAcbEncrypted(const RawAddress &device) const
{
    return false;
}

uint8_t SleAdapter::GetLinkRole(const RawAddress &device) const
{
    return 0;
}

bool SleAdapter::FactoryReset() const
{
    return true;
}

void SleAdapter::CancelAllConnection()
{}

int SleAdapter::GetProfileConnState(const RawAddress &device)
{
    return 0;
}

bool SleAdapter::WrapperCdsmGetAllMemberInfo(const RawAddress &rawAddr, std::vector<RawAddress> &cdsmInfoAddr)
{
    return true;
}

bool SleAdapter::UpdateSleVirtualDevice(int32_t cmd, const RawAddress &device)
{
    return true;
}

void SleAdapter::UpdateDeviceModelInfo(const std::string &address, const DeviceModel &model,
    const std::string &newModelId)
{}

void SleAdapter::DisconnectAllProfileForSilentPort(const RawAddress &device)
{}

bool SleAdapter::UpdateRefusePolicy(const int32_t protocolType, const int32_t pid, const int64_t refuseTime) const
{
    return true;
}

void SleAdapter::DelConnFrameType(const std::string &addr)
{}

void SleAdapter::SetConnFrameType(const std::string &addr, uint8_t frameType)
{}

bool SleAdapter::GetConnFrameType(const std::string &addr, uint8_t &frameType) const
{
    return true;
}

int SleAdapter::ReadRemoteRssiValue(const RawAddress &device)
{
    return 0;
}

bool SleAdapter::RegisterSleAdapterObserver(IAdapterSleObserver &observer) const
{
    return true;
}

bool SleAdapter::DeregisterSleAdapterObserver(IAdapterSleObserver &observer) const
{
    return true;
}

void SleAdapter::RegisterSleDeviceRssiCallback(ISleDeviceRssiCallback &callback) const
{}

void SleAdapter::DeregisterSleDeviceRssiCallback(ISleDeviceRssiCallback &callback) const
{}

void SleAdapter::RegisterSleConnectionCallback(ISleConnectionCallback &callback) const
{}

void SleAdapter::DeregisterSleConnectionCallback(ISleConnectionCallback &callback) const
{}

int SleAdapter::GetIoCapability() const
{
    return 0;
}

bool SleAdapter::SetIoCapability(int ioCapability) const
{
    return true;
}

uint32_t SleAdapter::GetSleMaxAdvertisingDataLength() const
{
    return 0;
}

bool SleAdapter::IsLlPrivacySupported() const
{
    return true;
}

bool SleAdapter::IsFeatureSupported(int32_t feature) const
{
    return true;
}

bool SleAdapter::HasConnectedDevice()
{
    return true;
}

bool SleAdapter::GetConnectionParam(std::string device, uint16_t &timeout, uint16_t &maxLatency) const
{
    return true;
}

bool SleAdapter::GetConnectionParam(std::string device, uint16_t &timeout, uint16_t &maxLatency,
    uint16_t &interval) const
{
    return true;
}

void SleAdapter::Enable()
{}

void SleAdapter::Disable()
{}

void SleAdapter::PostEnable()
{}

bool SleAdapter::IsScanConnTypeAndFrameType4(OHOS::Nearlink::RawAddress const&, unsigned char) const
{
    return false;
}

void SleAdapter::RemoveBgConnDevice(const std::string &delAddr) const
{}

void SleAdapter::SetPhy(const RawAddress &device, uint8_t frameType, uint8_t phyType)
{}

} // namespace Nearlink
} // namespace OHOS