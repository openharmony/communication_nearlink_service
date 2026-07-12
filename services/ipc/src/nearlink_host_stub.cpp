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

#include "nearlink_host_stub.h"
#include "log.h"
#include "nearlink_errorcode.h"
#include "ipc_types.h"
#include "nearlink_def.h"
#include "nearlink_permission_manager.h"
#include "nearlink_remote_device.h"
#ifdef HICOLLIE_ENABLE
#include "nearlink_hicollie_adapter.h"
#endif

#ifdef STUB_FUNC
#undef STUB_FUNC
#endif
#define STUB_FUNC(code, func, perm) NearlinkHostInterfaceCode::code, {NearlinkHostStub::func, perm}

namespace OHOS {
namespace Nearlink {
constexpr size_t PAIRED_DEVICE_MAX_NUM = 0xFFFF;
constexpr size_t DEVICE_UUID_MAX_NUM = 0xFFFF;
constexpr int32_t API_VERSION_18 = 18;

NearlinkHostStub::NearlinkHostStub()
{
    HILOGD("Enter");
    // Note: Permissions need to be configured when the itf to be used. "nullptr" means no permission needed.
    memberFuncMap_ = {
        {STUB_FUNC(NL_DISABLE_SLE, DisableSleInner, CHECK_PERM(true, MULTI_PERM(ACCESS_NEARLINK, MANAGE_NEARLINK)))},
        {STUB_FUNC(NL_ENABLE_SLE, EnableSleInner, CHECK_PERM(true, MULTI_PERM(ACCESS_NEARLINK, MANAGE_NEARLINK)))},
        {STUB_FUNC(NL_DISABLE_SLE_TO_OFF, DisableSleToOffInner, CHECK_PERM(true,
            MULTI_PERM(ACCESS_NEARLINK, MANAGE_NEARLINK)))},
        {STUB_FUNC(NL_ENABLE_SLE_TO_HALF, EnableSleToHalfInner, CHECK_PERM(true,
            MULTI_PERM(ACCESS_NEARLINK, MANAGE_NEARLINK)))},
        {STUB_FUNC(NL_GET_SLE_FULL_STATE, GetSleFullStateInner, CHECK_PERM(false, {}))},
        {STUB_FUNC(NL_IS_SLE_ENABLED, IsSleEnabledInner, CHECK_PERM(false, {}))},
        {STUB_FUNC(NL_IS_SLE_HALF_DISABLED, IsSleHalfDisabledInner, CHECK_PERM(false, {}))},
        {STUB_FUNC(NL_IS_SLE_DISABLED, IsSleDisabledInner, CHECK_PERM(false, {}))},
        {STUB_FUNC(NL_IS_SLE_AVAILABLE, IsSleAvailableToCallerInner, CHECK_PERM(false, {}))},
        {STUB_FUNC(NL_GET_ACB_STATE, GetAcbStateInner, CHECK_PERM(false, {ACCESS_NEARLINK}))},
        {STUB_FUNC(NL_GET_ADAPTER_CONNECTION_STATE, GetAdapterConnectStateInner, CHECK_PERM(false, {MANAGE_NEARLINK}))},
        {STUB_FUNC(NL_GET_PROFILE_CONNECTION_STATE, GetProfileConnStateInner, CHECK_PERM(false, {ACCESS_NEARLINK}))},
        {STUB_FUNC(NL_GETPROFILE, GetProfileInner, CHECK_PERM(false, {}))},
        {STUB_FUNC(NL_GET_LOCAL_NAME, GetLocalNameInner, CHECK_PERM(false, {ACCESS_NEARLINK}))},
        {STUB_FUNC(NL_SET_LOCAL_NAME, SetLocalNameInner, CHECK_PERM(true,
            MULTI_PERM(ACCESS_NEARLINK, MANAGE_NEARLINK)))},
        {STUB_FUNC(NL_GET_LOCAL_ADDR, GetLocalAddressInner, CHECK_PERM(true,
            MULTI_PERM(ACCESS_NEARLINK, GET_NEARLINK_LOCAL_MAC)))},
        {STUB_FUNC(NL_GET_PAIRED_DEVICES, GetPairedDevicesInner,
            CHECK_PERM(false, MULTI_PERM(ACCESS_NEARLINK)))},
        {STUB_FUNC(NL_SET_CONNECTION_MODE, SetConnectionModeInner,
            CHECK_PERM(true, MULTI_PERM(ACCESS_NEARLINK, MANAGE_NEARLINK)))},
        {STUB_FUNC(NL_REMOVE_PAIR, RemovePairInner, CHECK_PERM(true, MULTI_PERM(ACCESS_NEARLINK, MANAGE_NEARLINK)))},
        {STUB_FUNC(NL_REMOVE_ALL_PAIRS, RemoveAllPairsInner, CHECK_PERM(false, {MANAGE_NEARLINK}))},
        {STUB_FUNC(NL_REGISTER_SLE_PERIPHERAL_OBSERVER, RegisterSlePeripheralCallbackInner,
            CHECK_PERM(false, {}))},
        {STUB_FUNC(NL_DEREGISTER_SLE_PERIPHERAL_OBSERVER, DeregisterSlePeripheralCallbackInner,
            CHECK_PERM(false, {}))},
        {STUB_FUNC(NL_REGISTER_SLE_ADAPTER_OBSERVER, RegisterSleAdapterObserverInner,
            CHECK_PERM(false, {}))},
        {STUB_FUNC(NL_DEREGISTER_SLE_ADAPTER_OBSERVER, DeregisterSleAdapterObserverInner,
            CHECK_PERM(false, {}))},
        {STUB_FUNC(NL_REGISTER_DEVICE_BATTERY_OBSERVER, RegisterDeviceBatteryObserverInner,
            CHECK_PERM(false, {}))},
        {STUB_FUNC(NL_DEREGISTER_DEVICE_BATTERY_OBSERVER, DeregisterDeviceBatteryObserverInner,
            CHECK_PERM(false, {}))},
        {STUB_FUNC(NL_REGISTER_DEVICE_RSSI_OBSERVER, RegisterDeviceRssiObserverInner,
                   CHECK_PERM(false, {}))},
        {STUB_FUNC(NL_DEREGISTER_DEVICE_RSSI_OBSERVER, DeregisterDeviceRssiObserverInner,
                   CHECK_PERM(false, {}))},
        {STUB_FUNC(NL_GET_SLE_MAX_ADVERTISING_DATALENGTH, GetSleMaxAdvertisingDataLengthInner,
            CHECK_PERM(false, {MANAGE_NEARLINK}))},
        {STUB_FUNC(NL_GET_DEVICE_NAME, GetDeviceNameInner, CHECK_PERM(false, {ACCESS_NEARLINK}))},
        {STUB_FUNC(NL_GET_DEVICE_ALIAS, GetDeviceAliasInner,
            CHECK_PERM(true, MULTI_PERM(ACCESS_NEARLINK, MANAGE_NEARLINK)))},
        {STUB_FUNC(NL_SET_DEVICE_ALIAS, SetDeviceAliasInner,
            CHECK_PERM(true, MULTI_PERM(ACCESS_NEARLINK, MANAGE_NEARLINK)))},
        {STUB_FUNC(NL_GET_PAIR_STATE, GetPairStateInner, CHECK_PERM(false, {ACCESS_NEARLINK}))},
        {STUB_FUNC(NL_START_PAIR, StartPairInner, CHECK_PERM(false, {}))}, // 权限区分版本，校验在函数体内
        {STUB_FUNC(NL_START_CREDIBLE_PAIR, StartCrediblePairInner,
            CHECK_PERM(true, MULTI_PERM(ACCESS_NEARLINK, MANAGE_NEARLINK)))},
        {STUB_FUNC(NL_CANCEL_PAIRING, CancelPairingInner,
            CHECK_PERM(true, MULTI_PERM(ACCESS_NEARLINK, MANAGE_NEARLINK)))},
        {STUB_FUNC(NL_SET_PASSCODE, SetPairingPassCodeInner,
            CHECK_PERM(true, MULTI_PERM(ACCESS_NEARLINK, MANAGE_NEARLINK)))},
        {STUB_FUNC(NL_SET_CFM, SetPairingConfirmationInner,
            CHECK_PERM(true, MULTI_PERM(ACCESS_NEARLINK, MANAGE_NEARLINK)))},
        {STUB_FUNC(NL_IS_BONDED_FROM_LOCAL, IsBondedFromLocalInner, CHECK_PERM(false, {MANAGE_NEARLINK}))},
        {STUB_FUNC(NL_IS_ACB_CONNECTED, IsAcbConnectedInner, CHECK_PERM(false, {MANAGE_NEARLINK}))},
        {STUB_FUNC(NL_IS_ACB_ENCRYPTED, IsAcbEncryptedInner, CHECK_PERM(false, {MANAGE_NEARLINK}))},
        {STUB_FUNC(NL_GET_LINK_ROLE, GetLinkRoleInner, CHECK_PERM(false, {MANAGE_NEARLINK}))},
        {STUB_FUNC(NL_GET_DEVICE_UUIDS, GetDeviceUuidsInner, CHECK_PERM(false, {MANAGE_NEARLINK}))},
        {STUB_FUNC(NL_PAIR_REQUEST_PEPLY, PairRequestReplyInner, CHECK_PERM(false, {MANAGE_NEARLINK}))},
        {STUB_FUNC(NL_READ_REMOTE_RSSI_VALUE, ReadRemoteRssiValueInner, CHECK_PERM(true, {MANAGE_NEARLINK}))},
        {STUB_FUNC(NL_GET_DEVICE_APPEARANCE, GetDeviceAppearanceInner, CHECK_PERM(false, {ACCESS_NEARLINK}))},
        {STUB_FUNC(NL_CONNECT_ALLOWED_PROFILES, ConnectAllowedProfilesInner,
            CHECK_PERM(true, MULTI_PERM(ACCESS_NEARLINK, MANAGE_NEARLINK)))},
        {STUB_FUNC(NL_DISCONNECT_ALLOWED_PROFILES, DisconnectAllowedProfilesInner,
            CHECK_PERM(true, MULTI_PERM(ACCESS_NEARLINK, MANAGE_NEARLINK)))},
        {STUB_FUNC(NL_GET_DEVICE_PRODUCT_ID, GetDeviceProductIdInner, CHECK_PERM(false, {MANAGE_NEARLINK}))},
        {STUB_FUNC(NL_GET_DEVICE_VENDOR_ID, GetDeviceVendorIdInner, CHECK_PERM(false, {MANAGE_NEARLINK}))},
        {STUB_FUNC(NL_GET_DEVICE_MODEL, GetDeviceModelInner, CHECK_PERM(true, {}))},
        {STUB_FUNC(NL_GET_DEVICE_INFORMATION, GetDeviceInformationInner, CHECK_PERM(false, {ACCESS_NEARLINK}))},
        {STUB_FUNC(NL_FACTORY_RESET, FactoryResetInner, CHECK_PERM(true, {MANAGE_NEARLINK}))},
        {STUB_FUNC(NL_SET_FREQ_HOPPING, SetFreqHoppingInner, CHECK_PERM(true, {MANAGE_NEARLINK}))},
        {STUB_FUNC(NL_UPDATE_REFUSE_POLICY, UpdateRefusePolicyInner, CHECK_PERM(true, {MANAGE_NEARLINK}))},
        {STUB_FUNC(NL_CHECK_PERMISSION_FOR_NAPI, CheckPermissionForNapiInner, CHECK_PERM(false, {}))},
        {STUB_FUNC(NL_UPDATE_SLE_VIRTUAL_DEVICE, UpdateSleVirtualDeviceInner,
            CHECK_PERM(true, MULTI_PERM(ACCESS_NEARLINK, MANAGE_NEARLINK)))},
        {STUB_FUNC(NL_SET_BT_ADDR, SetBtAddrBySleAddrInner,
            CHECK_PERM(true, MULTI_PERM(MANAGE_NEARLINK, GET_NEARLINK_PEER_MAC)))},
        {STUB_FUNC(NL_GET_SLE_ADDR, GetSleAddrByBtAddrInner,
            CHECK_PERM(true, MULTI_PERM(ACCESS_NEARLINK, MANAGE_NEARLINK)))},
        {STUB_FUNC(NL_GET_BT_ADDR, GetBtAddrBySleAddrInner,
            CHECK_PERM(true, MULTI_PERM(ACCESS_NEARLINK, MANAGE_NEARLINK)))},
        {STUB_FUNC(NL_IS_SLE_FEATURE_SUPPORTED, IsFeatureSupportedInner,
            CHECK_PERM(true, MULTI_PERM(ACCESS_NEARLINK, MANAGE_NEARLINK)))},
        {STUB_FUNC(NL_IS_SLE_CONNECTION_EXIST, IsConnectionExistInner,
            CHECK_PERM(true, MULTI_PERM(ACCESS_NEARLINK, MANAGE_NEARLINK)))},
        {STUB_FUNC(NL_GET_BATTERY_LEVEL, GetBatteryLevelInner,
            CHECK_PERM(true, MULTI_PERM(ACCESS_NEARLINK, MANAGE_NEARLINK)))},
    };
}

NearlinkHostStub::~NearlinkHostStub()
{}

int32_t NearlinkHostStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
#ifdef HICOLLIE_ENABLE
    NearlinkHicollieAdapter hicollieAdapter("NearlinkHost", code);
#endif
    CHECK_PERMISSION_AND_EXECUTE(NearlinkHostStub);
}

int32_t NearlinkHostStub::GetProfileInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("enter");
    std::string name = data.ReadString();
    sptr<IRemoteObject> remoteProfile = nullptr;
    ErrCode status = stub->GetProfile(name, remoteProfile);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");

    if (status == NL_NO_ERROR) {
        NL_CHECK_RETURN_RET(reply.WriteRemoteObject(remoteProfile), TRANSACTION_ERR, "WriteRemoteObject failed.");
    }
    return NO_ERROR;
}

int32_t NearlinkHostStub::GetLocalNameInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("Enter");
    std::string name = "";
    NlErrCode status = stub->GetLocalName(name);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    NL_CHECK_RETURN_RET(reply.WriteString(name), TRANSACTION_ERR, "WriteString failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::SetLocalNameInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("Enter");
    std::string name;
    data.ReadString(name);
    NlErrCode result = stub->SetLocalName(name);
    NL_CHECK_RETURN_RET(reply.WriteInt32(result), TRANSACTION_ERR, "WriteInt32 failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::GetLocalAddressInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("enter");
    std::string addr = "";
    NlErrCode status = stub->GetLocalAddress(addr);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    NL_CHECK_RETURN_RET(reply.WriteString(addr), TRANSACTION_ERR, "WriteString failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::DisableSleInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("Enter");
    NlErrCode status = stub->DisableSle();
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::EnableSleInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("enter");
    SleAutoConnectPolicy autoConnPolicy = static_cast<SleAutoConnectPolicy>(data.ReadInt32());
    NlErrCode status = stub->EnableSle(autoConnPolicy);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::DisableSleToOffInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("Enter");
    NlErrCode status = stub->DisableSleToOff();
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::EnableSleToHalfInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("Enter");
    NlErrCode status = stub->EnableSleToHalf();
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::GetSleFullStateInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("Enter");
    int sleCurrentState = 0;
    NlErrCode status = stub->GetSleFullState(sleCurrentState);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    NL_CHECK_RETURN_RET(reply.WriteInt32(sleCurrentState), TRANSACTION_ERR, "WriteInt32 failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::IsSleEnabledInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("enter");
    bool isSleEnabled = false;
    NlErrCode status = stub->IsSleEnabled(isSleEnabled);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    NL_CHECK_RETURN_RET(reply.WriteBool(isSleEnabled), TRANSACTION_ERR, "WriteBool failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::IsSleHalfDisabledInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("Enter");
    bool isSleHalfDisabled = false;
    NlErrCode status = stub->IsSleHalfDisabled(isSleHalfDisabled);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    NL_CHECK_RETURN_RET(reply.WriteBool(isSleHalfDisabled), TRANSACTION_ERR, "WriteBool failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::IsSleDisabledInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("Enter");
    bool isSleDisabled = false;
    NlErrCode status = stub->IsSleDisabled(isSleDisabled);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    NL_CHECK_RETURN_RET(reply.WriteBool(isSleDisabled), TRANSACTION_ERR, "WriteBool failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::IsSleAvailableToCallerInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("enter");
    bool isSleAvailable = false;
    NlErrCode status = stub->IsSleAvailableToCaller(isSleAvailable);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    NL_CHECK_RETURN_RET(reply.WriteBool(isSleAvailable), TRANSACTION_ERR, "WriteBool failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::GetAcbStateInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("Enter");
    std::string address;
    NL_CHECK_RETURN_RET(data.ReadString(address), TRANSACTION_ERR, "ReadString failed.");

    int32_t acbState;
    NlErrCode status = stub->GetAcbState(address, acbState);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    NL_CHECK_RETURN_RET(reply.WriteInt32(acbState), TRANSACTION_ERR, "WriteInt32 failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::GetBatteryLevelInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("Enter");
    std::string address;
    NL_CHECK_RETURN_RET(data.ReadString(address), TRANSACTION_ERR, "ReadString failed.");
    NlErrCode status = stub->GetBatteryLevel(address);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::GetAdapterConnectStateInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("Enter");
    int32_t state;
    NlErrCode status = stub->GetAdapterConnectState(state);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    NL_CHECK_RETURN_RET(reply.WriteInt32(state), TRANSACTION_ERR, "WriteInt32 failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::GetProfileConnStateInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("enter");
    std::string address;
    NL_CHECK_RETURN_RET(data.ReadString(address), TRANSACTION_ERR, "ReadString failed.");

    int32_t state;
    NlErrCode status = stub->GetProfileConnState(address, state);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    NL_CHECK_RETURN_RET(reply.WriteInt32(state), TRANSACTION_ERR, "WriteInt32 failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::GetPairedDevicesInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("enter");
    std::vector<NearlinkRawAddress> pairDevice;
    NlErrCode result = stub->GetPairedDevices(pairDevice);
    NL_CHECK_RETURN_RET(reply.WriteInt32(result), TRANSACTION_ERR, "WriteInt32 failed.");

    if (result == NL_NO_ERROR) {
        uint32_t size = pairDevice.size();
        if (size > PAIRED_DEVICE_MAX_NUM) {
            HILOGE("size is too big, size=%{public}d", size);
            size = PAIRED_DEVICE_MAX_NUM;
        }
        NL_CHECK_RETURN_RET(reply.WriteUint32(size), TRANSACTION_ERR, "WriteUint32 failed.");

        for (uint32_t i = 0; i < size; i++) {
            NL_CHECK_RETURN_RET(reply.WriteParcelable(&pairDevice[i]), TRANSACTION_ERR, "WriteParcelable failed.");
        }
    }
    return NO_ERROR;
}

int32_t NearlinkHostStub::SetConnectionModeInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("enter");
    int32_t connectionMode;
    NL_CHECK_RETURN_RET(data.ReadInt32(connectionMode), TRANSACTION_ERR, "ReadInt32 failed.");
    int32_t duration;
    NL_CHECK_RETURN_RET(data.ReadInt32(duration), TRANSACTION_ERR, "ReadInt32 failed.");

    NlErrCode result = stub->SetConnectionMode(connectionMode, duration);
    NL_CHECK_RETURN_RET(reply.WriteInt32(result), TRANSACTION_ERR, "WriteInt32 failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::RemovePairInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("Enter");
    sptr<NearlinkRawAddress> device(data.ReadParcelable<NearlinkRawAddress>());
    NL_CHECK_RETURN_RET(device, TRANSACTION_ERR, "device is nullptr");
    NlErrCode result = stub->RemovePair(device);
    NL_CHECK_RETURN_RET(reply.WriteInt32(result), TRANSACTION_ERR, "WriteInt32 failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::RemoveAllPairsInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("Enter");
    NlErrCode result = stub->RemoveAllPairs();
    NL_CHECK_RETURN_RET(reply.WriteBool(result), TRANSACTION_ERR, "WriteBool failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::RegisterSlePeripheralCallbackInner(NearlinkHostStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    HILOGD("Enter");
    auto tempObject = data.ReadRemoteObject();
    sptr<INearlinkSlePeripheralObserver> observer = iface_cast<INearlinkSlePeripheralObserver>(tempObject);
    NL_CHECK_RETURN_RET(observer, TRANSACTION_ERR, "observer is nullptr");
    NlErrCode status = stub->RegisterSlePeripheralCallback(observer);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::DeregisterSlePeripheralCallbackInner(NearlinkHostStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    HILOGD("Enter");
    auto tempObject = data.ReadRemoteObject();
    sptr<INearlinkSlePeripheralObserver> observer = iface_cast<INearlinkSlePeripheralObserver>(tempObject);
    NL_CHECK_RETURN_RET(observer, TRANSACTION_ERR, "observer is nullptr");
    NlErrCode status = stub->DeregisterSlePeripheralCallback(observer);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::RegisterSleAdapterObserverInner(NearlinkHostStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    HILOGD("Enter");
    auto tempObject = data.ReadRemoteObject();
    sptr<INearlinkHostObserver> observer = iface_cast<INearlinkHostObserver>(tempObject);
    NL_CHECK_RETURN_RET(observer, TRANSACTION_ERR, "observer is nullptr");
    NlErrCode status = stub->RegisterSleAdapterObserver(observer);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::DeregisterSleAdapterObserverInner(NearlinkHostStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    HILOGD("Enter");
    auto tempObject = data.ReadRemoteObject();
    sptr<INearlinkHostObserver> observer = iface_cast<INearlinkHostObserver>(tempObject);
    NL_CHECK_RETURN_RET(observer, TRANSACTION_ERR, "observer is nullptr");
    NlErrCode status = stub->DeregisterSleAdapterObserver(observer);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::RegisterDeviceBatteryObserverInner(NearlinkHostStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    HILOGD("Enter");
    auto tempObject = data.ReadRemoteObject();
    sptr<INearlinkDeviceBatteryObserver> observer = iface_cast<INearlinkDeviceBatteryObserver>(tempObject);
    NL_CHECK_RETURN_RET(observer, TRANSACTION_ERR, "observer is nullptr");
    NlErrCode status = stub->RegisterDeviceBatteryObserver(observer);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::DeregisterDeviceBatteryObserverInner(NearlinkHostStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    HILOGD("Enter");
    auto tempObject = data.ReadRemoteObject();
    sptr<INearlinkDeviceBatteryObserver> observer = iface_cast<INearlinkDeviceBatteryObserver>(tempObject);
    NL_CHECK_RETURN_RET(observer, TRANSACTION_ERR, "observer is nullptr");
    NlErrCode status = stub->DeregisterDeviceBatteryObserver(observer);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::RegisterDeviceRssiObserverInner(NearlinkHostStub *stub, MessageParcel &data,
                                                             MessageParcel &reply)
{
    HILOGD("Enter");
    auto tempObject = data.ReadRemoteObject();
    sptr<INearlinkDeviceRssiObserver> observer = iface_cast<INearlinkDeviceRssiObserver>(tempObject);
    NL_CHECK_RETURN_RET(observer, TRANSACTION_ERR, "observer is nullptr");
    NlErrCode status = stub->RegisterDeviceRssiObserver(observer);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::DeregisterDeviceRssiObserverInner(NearlinkHostStub *stub, MessageParcel &data,
                                                               MessageParcel &reply)
{
    HILOGD("Enter");
    auto tempObject = data.ReadRemoteObject();
    sptr<INearlinkDeviceRssiObserver> observer = iface_cast<INearlinkDeviceRssiObserver>(tempObject);
    NL_CHECK_RETURN_RET(observer, TRANSACTION_ERR, "observer is nullptr");
    NlErrCode status = stub->DeregisterDeviceRssiObserver(observer);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::GetSleMaxAdvertisingDataLengthInner(NearlinkHostStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    HILOGD("Enter");
    uint32_t maxAdvDataLen = 0;
    NlErrCode status = stub->GetSleMaxAdvertisingDataLength(maxAdvDataLen);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    NL_CHECK_RETURN_RET(reply.WriteInt32(static_cast<int32_t>(maxAdvDataLen)), TRANSACTION_ERR, "WriteInt32 failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::GetDeviceNameInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("Enter");
    int32_t transport;
    NL_CHECK_RETURN_RET(data.ReadInt32(transport), TRANSACTION_ERR, "ReadInt32 failed.");
    std::string address;
    NL_CHECK_RETURN_RET(data.ReadString(address), TRANSACTION_ERR, "ReadString failed.");

    std::string name = "";
    NlErrCode status = stub->GetDeviceName(transport, address, name);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    NL_CHECK_RETURN_RET(reply.WriteString(name), TRANSACTION_ERR, "WriteString failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::GetDeviceAliasInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("enter");
    int32_t transport;
    NL_CHECK_RETURN_RET(data.ReadInt32(transport), TRANSACTION_ERR, "ReadInt32 failed.");
    std::string address;
    NL_CHECK_RETURN_RET(data.ReadString(address), TRANSACTION_ERR, "ReadString failed.");

    std::string alias = "";
    NlErrCode result = stub->GetDeviceAlias(transport, address, alias);
    NL_CHECK_RETURN_RET(reply.WriteInt32(result), TRANSACTION_ERR, "WriteInt32 failed.");
    NL_CHECK_RETURN_RET(reply.WriteString(alias), TRANSACTION_ERR, "WriteString failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::SetDeviceAliasInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("Enter");
    int32_t transport;
    NL_CHECK_RETURN_RET(data.ReadInt32(transport), TRANSACTION_ERR, "ReadInt32 failed.");
    std::string address;
    NL_CHECK_RETURN_RET(data.ReadString(address), TRANSACTION_ERR, "ReadString failed.");
    std::string alias;
    NL_CHECK_RETURN_RET(data.ReadString(alias), TRANSACTION_ERR, "ReadString failed.");

    NlErrCode result = stub->SetDeviceAlias(transport, address, alias);
    NL_CHECK_RETURN_RET(reply.WriteInt32(result), TRANSACTION_ERR, "WriteInt32 failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::GetPairStateInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("Enter");
    int32_t transport;
    NL_CHECK_RETURN_RET(data.ReadInt32(transport), TRANSACTION_ERR, "ReadInt32 failed.");
    std::string address;
    NL_CHECK_RETURN_RET(data.ReadString(address), TRANSACTION_ERR, "ReadString failed.");

    int pairState = 0;
    NlErrCode status = stub->GetPairState(transport, address, pairState);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    NL_CHECK_RETURN_RET(reply.WriteInt32(pairState), TRANSACTION_ERR, "WriteInt32 failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::StartPairInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("Enter");
    int32_t hapNearLinkApiVersion = NearLinkPermissionManager::GetNearlinkApiVersion();
    if (hapNearLinkApiVersion < API_VERSION_18) {
        VERIFY_PERMISSIONS(CHECK_PERM(true, MULTI_PERM(ACCESS_NEARLINK, MANAGE_NEARLINK)));
    } else {
        VERIFY_PERMISSIONS(CHECK_PERM(false, {ACCESS_NEARLINK}));
    }
    int32_t transport;
    NL_CHECK_RETURN_RET(data.ReadInt32(transport), TRANSACTION_ERR, "ReadInt32 failed.");
    std::string address;
    NL_CHECK_RETURN_RET(data.ReadString(address), TRANSACTION_ERR, "ReadString failed.");

    NlErrCode status = stub->StartPair(transport, address);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::StartCrediblePairInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("Enter");
    int32_t transport;
    NL_CHECK_RETURN_RET(data.ReadInt32(transport), TRANSACTION_ERR, "ReadInt32 failed.");
    std::string address;
    NL_CHECK_RETURN_RET(data.ReadString(address), TRANSACTION_ERR, "ReadString failed.");

    NlErrCode status = stub->StartCrediblePair(transport, address);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::CancelPairingInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("Enter");
    int32_t transport;
    NL_CHECK_RETURN_RET(data.ReadInt32(transport), TRANSACTION_ERR, "ReadInt32 failed.");
    std::string address;
    NL_CHECK_RETURN_RET(data.ReadString(address), TRANSACTION_ERR, "ReadString failed.");

    NlErrCode status = stub->CancelPairing(transport, address);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::SetPairingConfirmationInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("Enter");
    int32_t transport;
    NL_CHECK_RETURN_RET(data.ReadInt32(transport), TRANSACTION_ERR, "ReadInt32 failed.");
    std::string address;
    NL_CHECK_RETURN_RET(data.ReadString(address), TRANSACTION_ERR, "ReadString failed.");
    bool cfm;
    NL_CHECK_RETURN_RET(data.ReadBool(cfm), TRANSACTION_ERR, "ReadString failed.");
    NlErrCode result = stub->SetPairingConfirmation(transport, address, cfm);
    NL_CHECK_RETURN_RET(reply.WriteInt32(result), TRANSACTION_ERR, "WriteInt32 failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::SetPairingPassCodeInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("Enter");
    int32_t transport;
    NL_CHECK_RETURN_RET(data.ReadInt32(transport), TRANSACTION_ERR, "ReadInt32 failed.");
    std::string address;
    NL_CHECK_RETURN_RET(data.ReadString(address), TRANSACTION_ERR, "ReadString failed.");
    std::string passCode;
    NL_CHECK_RETURN_RET(data.ReadString(passCode), TRANSACTION_ERR, "ReadString failed.");

    NlErrCode result = stub->SetPairingPassCode(transport, address, passCode);
    NL_CHECK_RETURN_RET(reply.WriteInt32(result), TRANSACTION_ERR, "WriteInt32 failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::IsBondedFromLocalInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("Enter");
    int32_t transport;
    NL_CHECK_RETURN_RET(data.ReadInt32(transport), TRANSACTION_ERR, "ReadInt32 failed.");
    std::string address;
    NL_CHECK_RETURN_RET(data.ReadString(address), TRANSACTION_ERR, "ReadString failed.");

    bool isBondedFromLocal = false;
    ErrCode status = stub->IsBondedFromLocal(transport, address, isBondedFromLocal);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    NL_CHECK_RETURN_RET(reply.WriteBool(isBondedFromLocal), TRANSACTION_ERR, "WriteBool failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::IsAcbConnectedInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("enter");
    int32_t transport;
    NL_CHECK_RETURN_RET(data.ReadInt32(transport), TRANSACTION_ERR, "ReadInt32 failed.");
    std::string address;
    NL_CHECK_RETURN_RET(data.ReadString(address), TRANSACTION_ERR, "ReadString failed.");

    bool isAcbConnected = false;
    ErrCode status = stub->IsAcbConnected(transport, address, isAcbConnected);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    NL_CHECK_RETURN_RET(reply.WriteBool(isAcbConnected), TRANSACTION_ERR, "WriteBool failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::IsAcbEncryptedInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("Enter");
    int32_t transport;
    NL_CHECK_RETURN_RET(data.ReadInt32(transport), TRANSACTION_ERR, "ReadInt32 failed.");
    std::string address;
    NL_CHECK_RETURN_RET(data.ReadString(address), TRANSACTION_ERR, "ReadString failed.");

    bool isAcbEncrypted = false;
    ErrCode status = stub->IsAcbEncrypted(transport, address, isAcbEncrypted);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    NL_CHECK_RETURN_RET(reply.WriteBool(isAcbEncrypted), TRANSACTION_ERR, "WriteBool failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::GetLinkRoleInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("Enter");
    int32_t transport;
    NL_CHECK_RETURN_RET(data.ReadInt32(transport), TRANSACTION_ERR, "ReadInt32 failed.");
    std::string address;
    NL_CHECK_RETURN_RET(data.ReadString(address), TRANSACTION_ERR, "ReadString failed.");

    uint8_t role = SLE_INVALID_LINK_ROLE;
    NlErrCode status = stub->GetLinkRole(transport, address, role);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    NL_CHECK_RETURN_RET(reply.WriteUint8(role), TRANSACTION_ERR, "WriteInt32 failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::GetDeviceUuidsInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("Enter");
    int32_t transport = 0;
    NL_CHECK_RETURN_RET(data.ReadInt32(transport), TRANSACTION_ERR, "ReadInt32 failed.");
    std::string address;
    NL_CHECK_RETURN_RET(data.ReadString(address), TRANSACTION_ERR, "ReadString failed.");

    std::vector<std::string> uuids;
    NlErrCode status = stub->GetDeviceUuids(transport, address, uuids);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");

    if (status == NL_NO_ERROR) {
        size_t size = uuids.size();
        if (size > DEVICE_UUID_MAX_NUM) {
            HILOGE("size is too big, size=%{public}zu", size);
            size = DEVICE_UUID_MAX_NUM;
        }
        NL_CHECK_RETURN_RET(reply.WriteUint32(size), TRANSACTION_ERR, "WriteUint32 failed.");
        for (auto uuid : uuids) {
            NL_CHECK_RETURN_RET(reply.WriteString(uuid), TRANSACTION_ERR, "WriteString failed.");
        }
    }
    return NO_ERROR;
}

int32_t NearlinkHostStub::PairRequestReplyInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("Enter");
    int32_t transport;
    NL_CHECK_RETURN_RET(data.ReadInt32(transport), TRANSACTION_ERR, "ReadInt32 failed.");
    std::string address;
    NL_CHECK_RETURN_RET(data.ReadString(address), TRANSACTION_ERR, "ReadString failed.");
    bool accept = false;
    NL_CHECK_RETURN_RET(data.ReadBool(accept), TRANSACTION_ERR, "ReadBool failed.");

    NlErrCode status = stub->PairRequestReply(transport, address, accept);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::ReadRemoteRssiValueInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("Enter");
    std::string address;
    NL_CHECK_RETURN_RET(data.ReadString(address), TRANSACTION_ERR, "ReadString failed.");

    NlErrCode status = stub->ReadRemoteRssiValue(address);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::GetDeviceAppearanceInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("enter");
    std::string address;
    NL_CHECK_RETURN_RET(data.ReadString(address), TRANSACTION_ERR, "ReadString failed.");

    int32_t deviceAppearance;
    NlErrCode status = stub->GetDeviceAppearance(address, deviceAppearance);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    NL_CHECK_RETURN_RET(reply.WriteInt32(deviceAppearance), TRANSACTION_ERR, "WriteInt32 failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::ConnectAllowedProfilesInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("Enter");
    std::string address;
    NL_CHECK_RETURN_RET(data.ReadString(address), TRANSACTION_ERR, "ReadString failed.");

    NlErrCode result = stub->ConnectAllowedProfiles(address);
    NL_CHECK_RETURN_RET(reply.WriteInt32(result), TRANSACTION_ERR, "WriteInt32 failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::DisconnectAllowedProfilesInner(NearlinkHostStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    HILOGD("Enter");
    std::string address;
    NL_CHECK_RETURN_RET(data.ReadString(address), TRANSACTION_ERR, "ReadString failed.");

    NlErrCode result = stub->DisconnectAllowedProfiles(address);
    NL_CHECK_RETURN_RET(reply.WriteInt32(result), TRANSACTION_ERR, "WriteInt32 failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::GetDeviceProductIdInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGI("starts");
    std::string address;
    NL_CHECK_RETURN_RET(data.ReadString(address), TRANSACTION_ERR, "ReadString failed.");

    uint16_t productId;
    NlErrCode result = stub->GetDeviceProductId(address, productId);
    NL_CHECK_RETURN_RET(reply.WriteInt32(result), TRANSACTION_ERR, "WriteInt32 failed.");
    NL_CHECK_RETURN_RET(reply.WriteUint16(productId), TRANSACTION_ERR, "WriteUint16 failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::GetDeviceVendorIdInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGI("starts");
    std::string address;
    NL_CHECK_RETURN_RET(data.ReadString(address), TRANSACTION_ERR, "ReadString failed.");

    uint16_t vendorId;
    NlErrCode result = stub->GetDeviceVendorId(address, vendorId);
    NL_CHECK_RETURN_RET(reply.WriteInt32(result), TRANSACTION_ERR, "WriteInt32 failed");
    NL_CHECK_RETURN_RET(reply.WriteUint16(vendorId), TRANSACTION_ERR, "WriteUint16 failed");
    return NO_ERROR;
}

int32_t NearlinkHostStub::GetDeviceModelInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("enter");
    std::string address;
    NL_CHECK_RETURN_RET(data.ReadString(address), TRANSACTION_ERR, "ReadString failed.");
    NearlinkDeviceModel model;
    NlErrCode result = stub->GetDeviceModel(address, model);
    NL_CHECK_RETURN_RET(reply.WriteInt32(result), TRANSACTION_ERR, "WriteInt32 failed.");
    NL_CHECK_RETURN_RET(reply.WriteParcelable(&model), TRANSACTION_ERR, "WriteParcelable failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::GetDeviceInformationInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("enter");
    std::string address;
    NL_CHECK_RETURN_RET(data.ReadString(address), TRANSACTION_ERR, "ReadString failed.");
    NearlinkDeviceInformation information;
    NlErrCode result = stub->GetDeviceInformation(address, information);
    NL_CHECK_RETURN_RET(reply.WriteInt32(result), TRANSACTION_ERR, "WriteInt32 failed.");
    NL_CHECK_RETURN_RET(reply.WriteParcelable(&information), TRANSACTION_ERR, "WriteParcelable failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::FactoryResetInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    NlErrCode result = stub->FactoryReset();
    NL_CHECK_RETURN_RET(reply.WriteInt32(result), TRANSACTION_ERR, "Result writing failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::SetFreqHoppingInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("Enter");
    std::vector<uint8_t> freqMap;

    bool val = data.ReadUInt8Vector(&freqMap);
    if (!val) {
        HILOGE("Read freqMap failed");
        return TRANSACTION_ERR;
    }
    if (freqMap.size() != SLE_FREQ_HOPPING_LEN) {
        HILOGE("Param_Freq len is invalid!");
        return TRANSACTION_ERR;
    }
    NlErrCode result = stub->SetFreqHopping(freqMap);
    bool ret = reply.WriteInt32(result);
    if (!ret) {
        HILOGE("reply writing failed");
        return TRANSACTION_ERR;
    }
    return NO_ERROR;
}

int32_t NearlinkHostStub::UpdateSleVirtualDeviceInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("Enter");
    int32_t cmd = 0;
    std::string address {};
    NL_CHECK_RETURN_RET(data.ReadInt32(cmd), TRANSACTION_ERR, "ReadInt32 failed.");
    NL_CHECK_RETURN_RET(data.ReadString(address), TRANSACTION_ERR, "ReadString failed.");
    NlErrCode result = stub->UpdateSleVirtualDevice(cmd, address);
    NL_CHECK_RETURN_RET(reply.WriteInt32(result), TRANSACTION_ERR, "WriteInt32 failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::UpdateRefusePolicyInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("Enter");
    int32_t protocolType = 0;
    NL_CHECK_RETURN_RET(data.ReadInt32(protocolType), TRANSACTION_ERR, "read type error");
    int32_t pid = 0;
    NL_CHECK_RETURN_RET(data.ReadInt32(pid), TRANSACTION_ERR, "read pid error");
    int64_t refuseTime = 0;
    NL_CHECK_RETURN_RET(data.ReadInt64(refuseTime), TRANSACTION_ERR, "read time error");
    int32_t result = stub->UpdateRefusePolicy(protocolType, pid, refuseTime);

    NL_CHECK_RETURN_RET(reply.WriteInt32(result), NL_ERR_INTERNAL_ERROR, "reply writing failed.");
    return NL_NO_ERROR;
}

int32_t NearlinkHostStub::CheckPermissionForNapiInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    std::string perm = data.ReadString();
    bool isGranted = false;
    NlErrCode result = stub->CheckPermissionForNapi(perm, isGranted);
    NL_CHECK_RETURN_RET(reply.WriteInt32(result), TRANSACTION_ERR, "Result writing failed.");
    NL_CHECK_RETURN_RET(reply.WriteBool(isGranted), TRANSACTION_ERR, "isGranted writing failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::GetSleAddrByBtAddrInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("Enter");
    std::string btAddr;
    NL_CHECK_RETURN_RET(data.ReadString(btAddr), TRANSACTION_ERR, "ReadString failed.");
    std::string sleAddr;
    NlErrCode result = stub->GetSleAddrByBtAddr(btAddr, sleAddr);
    NL_CHECK_RETURN_RET(reply.WriteInt32(result), TRANSACTION_ERR, "WriteInt32 failed");
    NL_CHECK_RETURN_RET(reply.WriteString(sleAddr), TRANSACTION_ERR, "WriteString failed");
    return NO_ERROR;
}

int32_t NearlinkHostStub::GetBtAddrBySleAddrInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("Enter");
    std::string sleAddr;
    NL_CHECK_RETURN_RET(data.ReadString(sleAddr), TRANSACTION_ERR, "ReadString failed.");
    std::string btAddr;
    NlErrCode result = stub->GetBtAddrBySleAddr(sleAddr, btAddr);
    NL_CHECK_RETURN_RET(reply.WriteInt32(result), TRANSACTION_ERR, "WriteInt32 failed");
    NL_CHECK_RETURN_RET(reply.WriteString(btAddr), TRANSACTION_ERR, "WriteString failed");
    return NO_ERROR;
}

int32_t NearlinkHostStub::SetBtAddrBySleAddrInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("Enter");
    std::string btAddr;
    std::string sleAddr;
    NL_CHECK_RETURN_RET(data.ReadString(sleAddr), TRANSACTION_ERR, "ReadString failed.");
    NL_CHECK_RETURN_RET(data.ReadString(btAddr), TRANSACTION_ERR, "ReadString failed.");
    NlErrCode result = stub->SetBtAddrBySleAddr(sleAddr, btAddr);
    NL_CHECK_RETURN_RET(reply.WriteInt32(result), TRANSACTION_ERR, "Result writing failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::IsFeatureSupportedInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("Enter");
    bool isSupported = false;
    int32_t feature = 0;
    NL_CHECK_RETURN_RET(data.ReadInt32(feature), TRANSACTION_ERR, "ReadInt32 failed.");
    NlErrCode status = stub->IsFeatureSupported(feature, isSupported);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    NL_CHECK_RETURN_RET(reply.WriteBool(isSupported), TRANSACTION_ERR, "WriteBool failed.");
    return NO_ERROR;
}

int32_t NearlinkHostStub::IsConnectionExistInner(NearlinkHostStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("Enter");
    bool isConnectionExist = false;
    NlErrCode status = stub->IsConnectionExist(isConnectionExist);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    NL_CHECK_RETURN_RET(reply.WriteBool(isConnectionExist), TRANSACTION_ERR, "WriteBool failed.");
    return NO_ERROR;
}
}  // namespace Nearlink
}  // namespace OHOS
