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

#ifndef OHOS_NEARLINK_STANDARD_HOST_INTERFACE_H
#define OHOS_NEARLINK_STANDARD_HOST_INTERFACE_H

#include "nearlink_service_ipc_interface_code.h"
#include "i_nearlink_host_observer.h"
#include "i_nearlink_sle_peripheral_observer.h"
#include "i_nearlink_device_battery_observer.h"
#include "i_nearlink_device_rssi_observer.h"
#include "iremote_broker.h"
#include "nearlink_errorcode.h"
#include "nearlink_raw_address.h"
#include "nearlink_device_model_parcel.h"
#include "nearlink_device_information_parcel.h"
#include "nearlink_def.h"

namespace OHOS {
namespace Nearlink {

class INearlinkHost : public OHOS::IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.ipc.INearlinkHost");

    virtual NlErrCode GetProfile(const std::string &name, sptr<IRemoteObject> &remoteProfile) = 0;
    virtual NlErrCode DisableSle() = 0;
    virtual NlErrCode EnableSle(const SleAutoConnectPolicy autoConnPolicy =
        SleAutoConnectPolicy::AUTO_CONN_GENERAL) = 0;
    virtual NlErrCode DisableSleToOff() = 0;
    virtual NlErrCode EnableSleToHalf() = 0;
    virtual NlErrCode GetSleFullState(int &sleCurrentState) = 0;
    virtual NlErrCode IsSleEnabled(bool &isSleEnabled) = 0;
    virtual NlErrCode IsSleHalfDisabled(bool &isSleHalfDisabled) = 0;
    virtual NlErrCode IsSleDisabled(bool &isSleDisabled) = 0;
    virtual NlErrCode IsSleAvailableToCaller(bool &isSleAvailable) = 0;
    virtual NlErrCode GetAdapterConnectState(int32_t &state) = 0;
    virtual NlErrCode GetProfileConnState(const std::string &remoteAddr, int32_t &state) = 0;
    virtual NlErrCode GetLocalName(std::string &name) = 0;
    virtual NlErrCode SetLocalName(const std::string &name) = 0;
    virtual NlErrCode GetLocalAddress(std::string &addr) = 0;
    virtual NlErrCode GetPairedDevices(std::vector<NearlinkRawAddress> &pairedAddr) = 0;
    virtual NlErrCode SetConnectionMode(int32_t connectionMode, int32_t duration) = 0;
    virtual NlErrCode RemovePair(const sptr<NearlinkRawAddress> &device) = 0;
    virtual NlErrCode RemoveAllPairs() = 0;
    virtual NlErrCode RegisterSlePeripheralCallback(const sptr<INearlinkSlePeripheralObserver> &observer) = 0;
    virtual NlErrCode DeregisterSlePeripheralCallback(const sptr<INearlinkSlePeripheralObserver> &observer) = 0;
    virtual NlErrCode RegisterSleAdapterObserver(const sptr<INearlinkHostObserver> &observer) = 0;
    virtual NlErrCode DeregisterSleAdapterObserver(const sptr<INearlinkHostObserver> &observer) = 0;
    virtual NlErrCode RegisterDeviceBatteryObserver(const sptr<INearlinkDeviceBatteryObserver> &observer) = 0;
    virtual NlErrCode DeregisterDeviceBatteryObserver(const sptr<INearlinkDeviceBatteryObserver> &observer) = 0;
    virtual NlErrCode RegisterDeviceRssiObserver(const sptr<INearlinkDeviceRssiObserver> &observer) = 0;
    virtual NlErrCode DeregisterDeviceRssiObserver(const sptr<INearlinkDeviceRssiObserver> &observer) = 0;
    virtual NlErrCode GetSleMaxAdvertisingDataLength(uint32_t &maxAdvDataLen) = 0;
    virtual NlErrCode GetDeviceName(int32_t transport, const std::string &address, std::string &name) = 0;
    virtual NlErrCode GetDeviceAlias(int32_t transport, const std::string &address, std::string &alias) = 0;
    virtual NlErrCode SetDeviceAlias(int32_t transport, const std::string &address, const std::string &alias) = 0;
    virtual NlErrCode GetPairState(int32_t transport, const std::string &address, int &pairState) = 0;
    virtual NlErrCode StartPair(int32_t transport, const std::string &address) = 0;
    virtual NlErrCode StartCrediblePair(int32_t transport, const std::string &address) = 0;
    virtual NlErrCode CancelPairing(int32_t transport, const std::string &address) = 0;
    virtual NlErrCode SetPairingConfirmation(int32_t transport, const std::string &address, bool cfm) = 0;
    virtual NlErrCode SetPairingPassCode(int32_t transport, const std::string &address,
                                         const std::string &passCode) = 0;
    virtual NlErrCode IsBondedFromLocal(int32_t transport, const std::string &address, bool &isBondedFromLocal) = 0;
    virtual NlErrCode IsAcbConnected(int32_t transport, const std::string &address, bool &isAcbConnected) = 0;
    virtual NlErrCode IsAcbEncrypted(int32_t transport, const std::string &address, bool &isAcbEncrypted) = 0;
    virtual NlErrCode GetLinkRole(int32_t transport, const std::string &address, uint8_t &role) = 0;
    virtual NlErrCode GetDeviceUuids(int32_t transport, const std::string &address,
                                     std::vector<std::string> &uuids) = 0;
    virtual NlErrCode PairRequestReply(int32_t transport, const std::string &address, bool accept) = 0;
    virtual NlErrCode ReadRemoteRssiValue(const std::string &address) = 0;
    virtual NlErrCode GetDeviceAppearance(const std::string &address, int &appearance) = 0;
    virtual NlErrCode ConnectAllowedProfiles(const std::string &remoteAddr) = 0;
    virtual NlErrCode DisconnectAllowedProfiles(const std::string &remoteAddr) = 0;
    virtual NlErrCode GetDeviceProductId(const std::string &address, uint16_t &productId) = 0;
    virtual NlErrCode GetDeviceVendorId(const std::string &address, uint16_t &vendorId) = 0;
    virtual NlErrCode GetDeviceModel(const std::string &address, NearlinkDeviceModel &model) = 0;
    virtual NlErrCode GetDeviceInformation(const std::string &address, NearlinkDeviceInformation &information) = 0;

    virtual NlErrCode GetAcbState(const std::string &address, int &acbState) = 0;
    virtual NlErrCode FactoryReset() = 0;
    virtual NlErrCode SetFreqHopping(const std::vector<uint8_t> &freq) = 0;
    virtual NlErrCode UpdateSleVirtualDevice(int32_t cmd, const std::string &address) = 0;
    virtual NlErrCode UpdateRefusePolicy(const int32_t protocolType, const int32_t pid, const int64_t refuseTime) = 0;
    virtual NlErrCode CheckPermissionForNapi(const std::string &permission, bool &isGranted) = 0;
    virtual NlErrCode GetSleAddrByBtAddr(const std::string &btAddr, std::string &sleAddr) = 0;
    virtual NlErrCode GetBtAddrBySleAddr(const std::string &sleAddr, std::string &btAddr) = 0;
    virtual NlErrCode SetBtAddrBySleAddr(const std::string &sleAddr, const std::string &btAddr) = 0;
    virtual NlErrCode GetBatteryLevel(const std::string &address) = 0;
    virtual NlErrCode IsFeatureSupported(int32_t feature, bool &isSupported) = 0;
    virtual NlErrCode IsConnectionExist(bool &isConnectionExist) = 0;
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // OHOS_NEARLINK_STANDARD_HOST_INTERFACE_H
