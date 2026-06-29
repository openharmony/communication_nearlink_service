/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_NEARLINK_STANDARD_HOST_PROXY_H
#define OHOS_NEARLINK_STANDARD_HOST_PROXY_H

#include "i_nearlink_host.h"
#include "iremote_proxy.h"
#include "nearlink_remote_device.h"

namespace OHOS {
namespace Nearlink {
class NearlinkHostProxy : public IRemoteProxy<INearlinkHost> {
public:
    explicit NearlinkHostProxy(const sptr<IRemoteObject> &impl) : IRemoteProxy<INearlinkHost>(impl)
    {}
    ~NearlinkHostProxy()
    {}

    NlErrCode GetProfile(const std::string &name, sptr<IRemoteObject> &remoteProfile) override;

    NlErrCode DisableSle() override;
    NlErrCode EnableSle(const SleAutoConnectPolicy autoConnPolicy =
        SleAutoConnectPolicy::AUTO_CONN_GENERAL) override;
    NlErrCode DisableSleToOff() override;
    NlErrCode EnableSleToHalf() override;
    NlErrCode GetSleFullState(int &sleCurrentState) override;
    NlErrCode IsSleEnabled(bool &isSleEnabled) override;
    NlErrCode IsSleHalfDisabled(bool &isSleHalfDisabled) override;
    NlErrCode IsSleDisabled(bool &isSleDisabled) override;
    NlErrCode IsSleAvailableToCaller(bool &isSleAvailable) override;
    NlErrCode GetAdapterConnectState(int32_t &state) override;
    NlErrCode GetProfileConnState(const std::string &remoteAddr, int32_t &state) override;
    NlErrCode GetLocalName(std::string &name) override;
    NlErrCode SetLocalName(const std::string &name) override;
    NlErrCode GetLocalAddress(std::string &addr) override;
    NlErrCode GetPairedDevices(std::vector<NearlinkRawAddress> &pairedAddr) override;
    NlErrCode SetConnectionMode(int32_t connectionMode, int32_t duration) override;
    NlErrCode RemovePair(const sptr<NearlinkRawAddress> &device) override;
    NlErrCode RemoveAllPairs() override;
    NlErrCode RegisterSlePeripheralCallback(const sptr<INearlinkSlePeripheralObserver> &observer) override;
    NlErrCode DeregisterSlePeripheralCallback(const sptr<INearlinkSlePeripheralObserver> &observer) override;
    NlErrCode RegisterSleAdapterObserver(const sptr<INearlinkHostObserver> &observer) override;
    NlErrCode DeregisterSleAdapterObserver(const sptr<INearlinkHostObserver> &observer) override;
    NlErrCode RegisterDeviceBatteryObserver(const sptr<INearlinkDeviceBatteryObserver> &observer) override;
    NlErrCode DeregisterDeviceBatteryObserver(const sptr<INearlinkDeviceBatteryObserver> &observer) override;
    NlErrCode RegisterDeviceRssiObserver(const sptr<INearlinkDeviceRssiObserver> &observer) override;
    NlErrCode DeregisterDeviceRssiObserver(const sptr<INearlinkDeviceRssiObserver> &observer) override;
    NlErrCode GetSleMaxAdvertisingDataLength(uint32_t &maxAdvDataLen) override;
    NlErrCode GetDeviceName(int32_t transport, const std::string &address, std::string &name) override;
    NlErrCode GetDeviceAlias(int32_t transport, const std::string &address, std::string &alias) override;
    NlErrCode SetDeviceAlias(int32_t transport, const std::string &address, const std::string &alias) override;
    NlErrCode GetPairState(int32_t transport, const std::string &address, int &pairState) override;
    NlErrCode StartPair(int32_t transport, const std::string &address) override;
    NlErrCode StartCrediblePair(int32_t transport, const std::string &address) override;
    NlErrCode CancelPairing(int32_t transport, const std::string &address) override;
    NlErrCode SetPairingConfirmation(int32_t transport, const std::string &address, bool cfm) override;
    NlErrCode SetPairingPassCode(int32_t transport, const std::string &address, const std::string &passCode) override;
    NlErrCode IsBondedFromLocal(int32_t transport, const std::string &address, bool &isBondedFromLocal) override;
    NlErrCode IsAcbConnected(int32_t transport, const std::string &address, bool &isAcbConnected) override;
    NlErrCode IsAcbEncrypted(int32_t transport, const std::string &address, bool &isAcbEncrypted) override;
    NlErrCode GetLinkRole(int32_t transport, const std::string &address, uint8_t &role) override;
    NlErrCode GetDeviceUuids(int32_t transport, const std::string &address, std::vector<std::string> &uuids) override;
    NlErrCode PairRequestReply(int32_t transport, const std::string &address, bool accept) override;
    NlErrCode ReadRemoteRssiValue(const std::string &address) override;
    NlErrCode GetDeviceAppearance(const std::string &address, int &appearance) override;
    NlErrCode ConnectAllowedProfiles(const std::string &remoteAddr) override;
    NlErrCode DisconnectAllowedProfiles(const std::string &remoteAddr) override;
    NlErrCode GetDeviceProductId(const std::string &address, uint16_t &productId) override;
    NlErrCode GetDeviceVendorId(const std::string &address, uint16_t &vendorId) override;
    NlErrCode GetDeviceModel(const std::string &address, NearlinkDeviceModel &model) override;
    NlErrCode GetDeviceInformation(const std::string &address, NearlinkDeviceInformation &information) override;
    NlErrCode GetAcbState(const std::string &address, int &acbState) override;
    NlErrCode FactoryReset() override;
    NlErrCode SetFreqHopping(const std::vector<uint8_t> &freq) override;
    NlErrCode UpdateRefusePolicy(const int32_t protocolType, const int32_t pid, const int64_t refuseTime) override;
    NlErrCode CheckPermissionForNapi(const std::string &permission, bool &isGranted) override;
    NlErrCode UpdateSleVirtualDevice(int32_t cmd, const std::string &address) override;
    NlErrCode GetSleAddrByBtAddr(const std::string &btAddr, std::string &sleAddr) override;
    NlErrCode GetBtAddrBySleAddr(const std::string &sleAddr, std::string &btAddr) override;
    NlErrCode SetBtAddrBySleAddr(const std::string &sleAddr, const std::string &btAddr) override;
    NlErrCode GetBatteryLevel(const std::string &address) override;
    NlErrCode IsFeatureSupported(int32_t feature, bool &isSupported) override;
    NlErrCode IsConnectionExist(bool &isConnectionExist) override;
private:
    bool IsSleAvailableToCallerInner();
    ErrCode InnerTransact(uint32_t code, MessageOption &flags, MessageParcel &data, MessageParcel &reply);
    static inline BrokerDelegator<NearlinkHostProxy> delegator_;
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // OHOS_NEARLINK_STANDARD_HOST_PROXY_H
