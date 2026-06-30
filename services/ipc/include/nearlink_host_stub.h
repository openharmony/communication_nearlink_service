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

#ifndef OHOS_NEARLINK_STANDARD_HOST_STUB_H
#define OHOS_NEARLINK_STANDARD_HOST_STUB_H

#include <map>
#include <cstdint>

#include "i_nearlink_host.h"
#include "iremote_stub.h"
#include "nearlink_permission_item.h"

namespace OHOS {
namespace Nearlink {
class NearlinkHostStub : public IRemoteStub<INearlinkHost> {
public:
    NearlinkHostStub();
    virtual ~NearlinkHostStub();

    int32_t OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;

    using NearlinkHostFunc = int32_t (*)(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    using NearlinkHostFuncPerm = std::pair<NearlinkHostFunc, std::shared_ptr<NearLinkPermissionItem>>;

private:
    static int32_t DisableSleInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t EnableSleInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t DisableSleToOffInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t EnableSleToHalfInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t GetSleFullStateInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t IsSleEnabledInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t IsSleHalfDisabledInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t IsSleDisabledInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t IsSleAvailableToCallerInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t GetAdapterConnectStateInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t GetProfileConnStateInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t GetProfileInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t GetLocalNameInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t SetLocalNameInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t GetPairedDevicesInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t SetConnectionModeInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t RemovePairInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t RemoveAllPairsInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t RegisterSlePeripheralCallbackInner(NearlinkHostStub *stub, MessageParcel &data,
        MessageParcel &reply);
    static int32_t DeregisterSlePeripheralCallbackInner(NearlinkHostStub *stub, MessageParcel &data,
        MessageParcel &reply);
    static int32_t RegisterSleAdapterObserverInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t DeregisterSleAdapterObserverInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t RegisterDeviceBatteryObserverInner(
        NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t DeregisterDeviceBatteryObserverInner(
        NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t RegisterDeviceRssiObserverInner(
            NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t DeregisterDeviceRssiObserverInner(
            NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t GetSleMaxAdvertisingDataLengthInner(NearlinkHostStub *stub, MessageParcel &data,
        MessageParcel &reply);
    static int32_t GetDeviceNameInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t GetDeviceAliasInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t SetDeviceAliasInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t GetLocalAddressInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t GetPairStateInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t StartPairInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t StartCrediblePairInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t CancelPairingInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t SetPairingConfirmationInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t SetPairingPassCodeInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t IsBondedFromLocalInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t IsAcbConnectedInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t IsAcbEncryptedInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t GetLinkRoleInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t GetDeviceUuidsInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t PairRequestReplyInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t ReadRemoteRssiValueInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t GetDeviceAppearanceInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t ConnectAllowedProfilesInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t DisconnectAllowedProfilesInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t GetDeviceProductIdInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t GetDeviceVendorIdInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t GetDeviceModelInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t GetAcbStateInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t FactoryResetInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t SetFreqHoppingInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t UpdateSleVirtualDeviceInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t UpdateRefusePolicyInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t CheckPermissionForNapiInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t GetSleAddrByBtAddrInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t GetBtAddrBySleAddrInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t SetBtAddrBySleAddrInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t IsFeatureSupportedInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t IsConnectionExistInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t GetBatteryLevelInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t GetDeviceInformationInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply);

private:
    std::map<uint32_t, NearlinkHostFuncPerm> memberFuncMap_;
    DISALLOW_COPY_AND_MOVE(NearlinkHostStub);
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // OHOS_NEARLINK_STANDARD_HOST_STUB_H
