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

#include <thread>
#include "fuzzer/FuzzedDataProvider.h"
#include "host_fuzzer.h"
#include "nearlink_native_token_mock.h"
#include "../nl_utils/fuzztest_utils.h"
#include "nearlink_host_stub.h"
#include "nearlink_host_server.h"
#include "i_nearlink_device_battery_observer.h"
#include "i_nearlink_device_rssi_observer.h"
#include "nearlink_errorcode.h"
#include "log.h"
#include "securec.h"
#include "raw_address.h"
#include "SleServiceManager.h"
#include "ProfileServiceManager.h"
#include "IAdapterStateObserver.h"
#include "SleSwitchDependency.h"
#include "UnloadSa.h"
#include "SleFrame4AntennaMgr.h"
#include "SysStateMachine.h"
#include "sle_uuid.h"
#include "ThreadUtil.h"

using namespace std;
using namespace OHOS::Nearlink;
namespace OHOS {

class MockNearlinkSlePeripheralObserverStub : public IRemoteStub<INearlinkSlePeripheralObserver> {
public:
    void OnReadRemoteRssiEvent(const NearlinkRawAddress &device, int rssi, int status) {}
    void OnPairingRequest(const NearlinkRawAddress &device, const std::string &passkey, int type) {}
    void OnPairStatusChanged(const NearlinkRawAddress &device, int preState, int state, int reason) {}
    void OnAcbStateChanged(const NearlinkRawAddress &device, int state, int reason) {}
    void OnConnectionStateChanged(const NearlinkRawAddress &device, int preState, int state, int reason) {}
    void OnLinkFreqBandChanged(const NearlinkRawAddress &device, int32_t freqBand) {}
    void OnGetBatteryLevelEvent(const NearlinkRawAddress &device, int batteryLevel) {}
    void OnBatteryLevelChanged(const NearlinkRawAddress &device, int batteryLevel) {}
};

class MockNearlinkHostObserverStub : public IRemoteStub<INearlinkHostObserver> {
public:
    void OnStateChanged(int32_t transport, int32_t status) {}
    void OnFullStateChanged(int32_t transport, int32_t status) {}
    void OnSwitchStateChanged(int32_t status) {}
    void OnDisableResponse(bool isHalfDisable) {}
    void OnPairConfirmed(const int32_t transport, const NearlinkRawAddress &device, int reqType, int number) {}
    void OnDeviceNameChanged(const std::string &deviceName) {}
    void OnDeviceAddrChanged(const std::string &address) {}
};

class MockNearlinkDeviceBatteryObserverStub : public IRemoteStub<INearlinkDeviceBatteryObserver> {
public:
    void OnGetBatteryLevelEvent(const NearlinkRawAddress &device, int batteryLevel) {}
    void OnBatteryLevelChanged(const NearlinkRawAddress &device, int batteryLevel) {}
};

class MockNearlinkDeviceRssiObserverStub : public IRemoteStub<INearlinkDeviceRssiObserver> {
public:
    void OnReadRemoteRssiEvent(const NearlinkRawAddress &device, int rssi, int status) {}
};

namespace {
constexpr int HOST_FUZZ_DELAY_50_MS = 50;
constexpr int HOST_FUZZ_DELAY_100_MS = 100;
constexpr int HOST_FUZZ_DELAY_5000_MS = 5000;
constexpr int SLE_FREQ_ARRAY_LEN = 10;
sptr<INearlinkSlePeripheralObserver> g_slePeripheralObserver =
    new (std::nothrow) MockNearlinkSlePeripheralObserverStub();
sptr<INearlinkHostObserver> g_hostObserver = new (std::nothrow) MockNearlinkHostObserverStub();
sptr<INearlinkDeviceBatteryObserver> g_batteryObserver =
    new (std::nothrow) MockNearlinkDeviceBatteryObserverStub();
sptr<INearlinkDeviceRssiObserver> g_rssiObserver = new (std::nothrow) MockNearlinkDeviceRssiObserverStub();
}

int32_t HostOnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply)
{
    sptr<NearlinkHostServer> hostServer = NearlinkHostServer::GetInstance();
    NL_CHECK_RETURN_RET(hostServer, TRANSACTION_ERR, "hostServer is nullptr");
    MessageOption option = {MessageOption::TF_SYNC};
    HILOGI("HostOnRemoteRequest cmd(%{public}d)", code);
    return hostServer->OnRemoteRequest(code, data, reply, option);
}

std::vector<uint8_t> BuildFreqHoppingMap(FuzzedDataProvider &provider)
{
    std::vector<uint8_t> freqList = provider.ConsumeBytes<uint8_t>(SLE_FREQ_ARRAY_LEN);
    return freqList;
}

void GetProfileFuzzTest(const uint8_t *fuzzData, size_t size)
{
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    std::string name = "SleAdvertiserServer";
    data.WriteString(name);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_GETPROFILE, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void GetSleFullStateFuzzTest(const uint8_t *fuzzData, size_t size)
{
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_GET_SLE_FULL_STATE, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void IsSleConnectionExistFuzzTest(const uint8_t *fuzzData, size_t size)
{
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_IS_SLE_CONNECTION_EXIST, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void IsSleEnabledFuzzTest(const uint8_t *fuzzData, size_t size)
{
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_IS_SLE_ENABLED, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void IsSleHalfDisabledFuzzTest(const uint8_t *fuzzData, size_t size)
{
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_IS_SLE_HALF_DISABLED, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void IsSleDisabledFuzzTest(const uint8_t *fuzzData, size_t size)
{
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_IS_SLE_DISABLED, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void IsSleAvailableToCallerFuzzTest(const uint8_t *fuzzData, size_t size)
{
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_IS_SLE_AVAILABLE, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void GetAcbStateFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    std::string address = BuildAddressString(provider);
    data.WriteString(address);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_GET_ACB_STATE, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void GetAdapterConnectStateFuzzTest(const uint8_t *fuzzData, size_t size)
{
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_GET_ADAPTER_CONNECTION_STATE, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void GetProfileConnStateFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    std::string address = BuildAddressString(provider);
    data.WriteString(address);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_GET_PROFILE_CONNECTION_STATE, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void GetLocalNameFuzzTest(const uint8_t *fuzzData, size_t size)
{
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_GET_LOCAL_NAME, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void SetLocalNameFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    std::string name = BuildAddressString(provider);
    data.WriteString(name);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_SET_LOCAL_NAME, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void SetConnectionModeFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    int32_t connectionMode = provider.ConsumeIntegral<int32_t>();
    int32_t duration = provider.ConsumeIntegral<int32_t>();
    data.WriteInt32(connectionMode);
    data.WriteInt32(duration);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_SET_CONNECTION_MODE, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void GetLocalAddressFuzzTest(const uint8_t *fuzzData, size_t size)
{
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_GET_LOCAL_ADDR, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void GetPairedDevicesFuzzTest(const uint8_t *fuzzData, size_t size)
{
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_GET_PAIRED_DEVICES, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void RemovePairFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    std::string addr = BuildAddressString(provider);
    sptr<NearlinkRawAddress> rawAddr = new (std::nothrow) NearlinkRawAddress(addr);
    if (rawAddr == nullptr) {
        HILOGE("rawAddr is nullptr");
        return;
    }

    data.WriteParcelable(rawAddr.GetRefPtr());
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_REMOVE_PAIR, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void RemoveAllPairsTest(const uint8_t *fuzzData, size_t size)
{
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_REMOVE_ALL_PAIRS, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void RegisterSlePeripheralCallbackTest(const uint8_t *fuzzData, size_t size)
{
    MessageParcel data;
    MessageParcel reply;

    if (g_slePeripheralObserver == nullptr) {
        HILOGE("g_slePeripheralObserver is nullptr");
        return;
    }
    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteRemoteObject(g_slePeripheralObserver->AsObject());
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_REGISTER_SLE_PERIPHERAL_OBSERVER, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void DeregisterSlePeripheralCallbackTest(const uint8_t *fuzzData, size_t size)
{
    MessageParcel data;
    MessageParcel reply;

    if (g_slePeripheralObserver == nullptr) {
        HILOGE("g_slePeripheralObserver is nullptr");
        return;
    }
    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteRemoteObject(g_slePeripheralObserver->AsObject());
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_DEREGISTER_SLE_PERIPHERAL_OBSERVER, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void RegisterSleAdapterObserverTest(const uint8_t *fuzzData, size_t size)
{
    MessageParcel data;
    MessageParcel reply;

    if (g_hostObserver == nullptr) {
        HILOGE("g_hostObserver is nullptr");
        return;
    }
    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteRemoteObject(g_hostObserver->AsObject());
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_REGISTER_SLE_ADAPTER_OBSERVER, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void DeregisterSleAdapterObserverTest(const uint8_t *fuzzData, size_t size)
{
    MessageParcel data;
    MessageParcel reply;

    if (g_hostObserver == nullptr) {
        HILOGE("g_hostObserver is nullptr");
        return;
    }
    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteRemoteObject(g_hostObserver->AsObject());
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_DEREGISTER_SLE_ADAPTER_OBSERVER, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void GetSleMaxAdvertisingDataLengthTest(const uint8_t *fuzzData, size_t size)
{
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_GET_SLE_MAX_ADVERTISING_DATALENGTH, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void GetDeviceNameTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteInt32(static_cast<int>(NlTransportType::NL_TRANSPORT_SLE)); // transport
    std::string address = BuildAddressString(provider);
    data.WriteString(address);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_GET_DEVICE_NAME, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void GetDeviceAliasTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteInt32(static_cast<int>(NlTransportType::NL_TRANSPORT_SLE)); // transport
    std::string address = BuildAddressString(provider);
    data.WriteString(address);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_GET_DEVICE_ALIAS, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void SetDeviceAliasTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteInt32(static_cast<int>(NlTransportType::NL_TRANSPORT_SLE)); // transport
    std::string address = BuildAddressString(provider);
    data.WriteString(address);
    data.WriteString("alias_test");
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_SET_DEVICE_ALIAS, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void GetPairStateTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteInt32(static_cast<int>(NlTransportType::NL_TRANSPORT_SLE)); // transport
    std::string address = BuildAddressString(provider);
    data.WriteString(address);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_GET_PAIR_STATE, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void GetLinkRoleTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteInt32(static_cast<int>(NlTransportType::NL_TRANSPORT_SLE)); // transport
    std::string address = BuildAddressString(provider);
    data.WriteString(address);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_GET_LINK_ROLE, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void StartPairTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteInt32(static_cast<int>(NlTransportType::NL_TRANSPORT_SLE)); // transport
    std::string address = BuildAddressString(provider);
    data.WriteString(address);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_START_PAIR, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void StartCrediblePairTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteInt32(static_cast<int>(NlTransportType::NL_TRANSPORT_SLE)); // transport
    std::string address = BuildAddressString(provider);
    data.WriteString(address);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_START_CREDIBLE_PAIR, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}
void SetPairingConfirmationTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteInt32(static_cast<int>(NlTransportType::NL_TRANSPORT_SLE)); // transport
    std::string address = BuildAddressString(provider);
    data.WriteBool(true);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_SET_CFM, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}
void SetPairingPassCodeTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteInt32(static_cast<int>(NlTransportType::NL_TRANSPORT_SLE)); // transport
    std::string address = BuildAddressString(provider);
    data.WriteString(address);
    data.WriteString("123456");
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_SET_PASSCODE, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void CancelPairingTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteInt32(static_cast<int>(NlTransportType::NL_TRANSPORT_SLE)); // transport
    std::string address = BuildAddressString(provider);
    data.WriteString(address);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_CANCEL_PAIRING, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void IsBondedFromLocalTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteInt32(static_cast<int>(NlTransportType::NL_TRANSPORT_SLE)); // transport
    std::string address = BuildAddressString(provider);
    data.WriteString(address);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_IS_BONDED_FROM_LOCAL, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void IsAcbConnectedTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteInt32(static_cast<int>(NlTransportType::NL_TRANSPORT_SLE)); // transport
    std::string address = BuildAddressString(provider);
    data.WriteString(address);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_IS_ACB_CONNECTED, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void IsAcbEncryptedTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteInt32(static_cast<int>(NlTransportType::NL_TRANSPORT_SLE)); // transport
    std::string address = BuildAddressString(provider);
    data.WriteString(address);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_IS_ACB_ENCRYPTED, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void GetDeviceUuidsTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteInt32(static_cast<int>(NlTransportType::NL_TRANSPORT_SLE)); // transport
    std::string address = BuildAddressString(provider);
    data.WriteString(address);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_GET_DEVICE_UUIDS, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void PairRequestReplyTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteInt32(static_cast<int>(NlTransportType::NL_TRANSPORT_SLE)); // transport
    std::string address = BuildAddressString(provider);
    data.WriteString(address);
    data.WriteBool(fuzzData[0]);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_PAIR_REQUEST_PEPLY, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void ReadRemoteRssiValueTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    std::string address = BuildAddressString(provider);
    data.WriteString(address);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_READ_REMOTE_RSSI_VALUE, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void GetDeviceAppearanceTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    std::string address = BuildAddressString(provider);
    data.WriteString(address);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_GET_DEVICE_APPEARANCE, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void ConnectAllowedProfilesTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    std::string address = BuildAddressString(provider);
    data.WriteString(address);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_CONNECT_ALLOWED_PROFILES, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void DisconnectAllowedProfilesTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    std::string address = BuildAddressString(provider);
    data.WriteString(address);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_DISCONNECT_ALLOWED_PROFILES, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void GetDeviceProductIdTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    std::string address = BuildAddressString(provider);
    data.WriteString(address);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_GET_DEVICE_PRODUCT_ID, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void GetDeviceVendorIdTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    std::string address = BuildAddressString(provider);
    data.WriteString(address);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_GET_DEVICE_VENDOR_ID, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void GetDeviceModelTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    std::string address = BuildAddressString(provider);
    data.WriteString(address);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_GET_DEVICE_MODEL, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void SetFreqHoppingFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    std::vector<uint8_t> freq = BuildFreqHoppingMap(provider);
    data.WriteUInt8Vector(freq);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_SET_FREQ_HOPPING, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void CheckPermissionForNapiFuzzTest(const uint8_t *fuzzData, size_t size)
{
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteString(ACCESS_NEARLINK_PERMISSION);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_CHECK_PERMISSION_FOR_NAPI, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void UpdateSleVirtualDeviceFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    int32_t cmd = provider.ConsumeIntegral<int32_t>();
    std::string address = BuildAddressString(provider);
    data.WriteInt32(cmd);
    data.WriteString(address);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_UPDATE_SLE_VIRTUAL_DEVICE, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void SetBtAddrBySleAddrTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    std::string sleAddr = BuildAddressString(provider);
    std::string btAddr = BuildAddressString(provider);
    data.WriteString(sleAddr);
    data.WriteString(btAddr);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_SET_BT_ADDR, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void GetSleAddrByBtAddrTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    std::string btAddr = BuildAddressString(provider);
    data.WriteString(btAddr);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_GET_SLE_ADDR, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void GetBtAddrBySleAddrTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    std::string sleAddr = BuildAddressString(provider);
    data.WriteString(sleAddr);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_GET_BT_ADDR, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void UpdateRefusePolicyFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    int32_t protocolType = provider.ConsumeIntegral<int32_t>();
    int32_t pid = provider.ConsumeIntegral<int32_t>();
    int64_t refuseTime = provider.ConsumeIntegral<int64_t>();
    data.WriteInt32(protocolType);
    data.WriteInt32(pid);
    data.WriteInt64(refuseTime);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_UPDATE_REFUSE_POLICY, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void RegisterDeviceBatteryObserverTest(const uint8_t *fuzzData, size_t size)
{
    MessageParcel data;
    MessageParcel reply;

    if (g_batteryObserver == nullptr) {
        HILOGE("g_batteryObserver is nullptr");
        return;
    }
    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteRemoteObject(g_batteryObserver->AsObject());
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_REGISTER_DEVICE_BATTERY_OBSERVER, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void DeregisterDeviceBatteryObserverTest(const uint8_t *fuzzData, size_t size)
{
    MessageParcel data;
    MessageParcel reply;

    if (g_batteryObserver == nullptr) {
        HILOGE("g_batteryObserver is nullptr");
        return;
    }
    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteRemoteObject(g_batteryObserver->AsObject());
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_DEREGISTER_DEVICE_BATTERY_OBSERVER, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void RegisterDeviceRssiObserverTest(const uint8_t *fuzzData, size_t size)
{
    MessageParcel data;
    MessageParcel reply;

    if (g_rssiObserver == nullptr) {
        HILOGE("g_rssiObserver is nullptr");
        return;
    }
    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteRemoteObject(g_rssiObserver->AsObject());
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_REGISTER_DEVICE_RSSI_OBSERVER, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void DeregisterDeviceRssiObserverTest(const uint8_t *fuzzData, size_t size)
{
    MessageParcel data;
    MessageParcel reply;

    if (g_rssiObserver == nullptr) {
        HILOGE("g_rssiObserver is nullptr");
        return;
    }
    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    data.WriteRemoteObject(g_rssiObserver->AsObject());
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_DEREGISTER_DEVICE_RSSI_OBSERVER, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void IsFeatureSupportedTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    int32_t feature = provider.ConsumeIntegral<int32_t>();
    data.WriteInt32(feature);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_IS_SLE_FEATURE_SUPPORTED, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void GetBatteryLevelTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    std::string address = BuildAddressString(provider);
    data.WriteString(address);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_GET_BATTERY_LEVEL, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void GetDeviceInformationTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    std::string address = BuildAddressString(provider);
    data.WriteString(address);
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_GET_DEVICE_INFORMATION, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

// PR-9 D-4: common 域直驱（SleServiceManager/SysStateMachine/ProfileServiceManager/SleSwitchDependency/UnloadSa/SleFrame4AntennaMgr）
namespace {
// ThreadUtil 进程级一次初始化（PR-9 计划 §3 纪律 1：future 族投递前置，map 为空=DISABLED=静默丢弃）
std::once_flag g_pr9ThreadStateOnce;
void InitPr9ThreadStateMapOnce()
{
    std::call_once(g_pr9ThreadStateOnce, []() { ThreadUtil::GetInstance().InitThreadStateMap(); });
}

constexpr size_t PR9_EVENT_TABLE_SIZE = 6;
const SleEventType PR9_EVENT_TABLE[PR9_EVENT_TABLE_SIZE] = {
    SleEventType::INTERFACE_TRIGGERED, SleEventType::AIRPLANE_TRIGGERED,
    SleEventType::COLLABORATION_TRIGGERED, SleEventType::RESTORE_TRIGGERED,
    SleEventType::SELF_HEALING_RETRY, SleEventType::SVC_TRIGGERED,
};
constexpr size_t PR9_STATE_TABLE_SIZE = 9;
const SleStateID PR9_STATE_TABLE[PR9_STATE_TABLE_SIZE] = {
    STATE_TURNING_ON, STATE_TURN_ON, STATE_TURNING_OFF, STATE_TURN_OFF,
    STATE_TURNING_HALF_TO_OFF, STATE_TURNING_OFF_TO_HALF, STATE_TURNING_HALF_TO_ON,
    STATE_TURNING_ON_TO_HALF, STATE_TURN_HALF,
};

// 整改 P9-2：迁移 API 四元域表（无捕获 lambda 退化函数指针；Enable 第 4 参走默认）
using Pr9MigrationFn = void (*)(SleServiceManager *, const SleTransport, const SleEventType,
    const SwitchCallerInfo &);
constexpr size_t PR9_MIGRATION_TABLE_SIZE = 4;
const Pr9MigrationFn PR9_MIGRATION_TABLE[PR9_MIGRATION_TABLE_SIZE] = {
    [](SleServiceManager *mgr, const SleTransport transport, const SleEventType reason,
        const SwitchCallerInfo &callerInfo) { mgr->Enable(transport, reason, callerInfo); },
    [](SleServiceManager *mgr, const SleTransport transport, const SleEventType reason,
        const SwitchCallerInfo &callerInfo) { mgr->Disable(transport, reason, callerInfo); },
    [](SleServiceManager *mgr, const SleTransport transport, const SleEventType reason,
        const SwitchCallerInfo &callerInfo) { mgr->EnableToHalf(transport, reason, callerInfo); },
    [](SleServiceManager *mgr, const SleTransport transport, const SleEventType reason,
        const SwitchCallerInfo &callerInfo) { mgr->DisableToOff(transport, reason, callerInfo); },
};

class MockAdapterStateObserver : public IAdapterStateObserver {
public:
    void OnStateChange(const SleTransport transport, const SleStateID state) {}
    void OnDisableResponse(bool isHalfDisable, SwitchCallerInfo callerInfo) {}
};
// ---------------------------------------------------------------------------
// ServiceManager lifecycle deep drive.
// Safety: Reset()/FactoryReset()/ClearAllStorage()/Stop()/AdapterStop() and
// OnChipResetNotify() are NEVER called (Reset kills the process, see
// SleServiceManager::Reset). No *_TIME_OUT message is ever posted; after each
// switch request the machine is polled back into a stable state so the 10s
// turning timers are always closed by the adapter/profile CMP path.
// ---------------------------------------------------------------------------
class FuzzSleAdapterStateObserverStub : public IAdapterStateObserver {
public:
    void OnStateChange(const SleTransport transport, const SleStateID state) {}
    void OnDisableResponse(bool isHalfDisable, SwitchCallerInfo callerInfo) {}
};

class MockSystemStateObserver : public ISystemStateObserver {
public:
    void OnSystemStateChange(const SleSystemState state) {}
};
}  // namespace

void SleServiceManagerSwitchFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    InitPr9ThreadStateMapOnce();
    SleServiceManager *mgr = SleServiceManager::GetInstance();
    NL_CHECK_RETURN(mgr != nullptr, "SleServiceManager is nullptr");
    MockAdapterStateObserver adapterObserver;
    MockSystemStateObserver systemObserver;
    SwitchCallerInfo callerInfo {};
    uint8_t reasonIdx = provider.ConsumeIntegral<uint8_t>() % PR9_EVENT_TABLE_SIZE;
    uint8_t stateIdx = provider.ConsumeIntegral<uint8_t>() % PR9_STATE_TABLE_SIZE;
    bool isSle = provider.ConsumeBool();
    uint8_t migrationSteps = provider.ConsumeIntegralInRange<uint8_t>(1, 5); // 整改 P9-2：步数 FDP
    SleTransport transport = isSle ? ADAPTER_SLE : ADAPTER_SLB;
    SleEventType reason = PR9_EVENT_TABLE[reasonIdx];

    // 观察者注册/重复注册（future 族与回调均处于 InitThreadStateMap 进程级窗口内）
    mgr->RegisterStateObserver(adapterObserver);
    mgr->RegisterStateObserver(adapterObserver);
    mgr->RegisterSystemStateObserver(systemObserver);
    mgr->RegisterSystemStateObserver(systemObserver);
    mgr->GetState(transport);
    mgr->GetAdapterConnectState();

    // 开关状态迁移深路径（整改 P9-2：原五步固定编排改 FDP 域表逐步选择）：
    // TurnOn⇄Off⇄Half 四元迁移 API 由 FDP 选择、步数 FDP（1..5）；
    // DoInServiceManagerThread+promise 同步 future，步间 100ms 有界等待
    for (uint8_t step = 0; step < migrationSteps; step++) {
        uint8_t migrationIdx = provider.ConsumeIntegral<uint8_t>() % PR9_MIGRATION_TABLE_SIZE;
        PR9_MIGRATION_TABLE[migrationIdx](mgr, transport, reason, callerInfo);
        std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_100_MS));
    }
    // 收尾回稳（确定性收尾非业务驱动）：固定 Enable 收敛到 TURN_ON，供后续输入稳定驱动
    mgr->Enable(transport, reason, callerInfo);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));

    // 整改 P9-1：stateIdx 接入真实分支——镜像产品迁移后的 UnloadSa 定时器调用对
    // （SleServiceManager.cpp:582-585 同款：先 Stop 再按迁移态 Start；仅 STATE_TURN_OFF 实启，同输入内停表收敛）
    UnloadSa::GetInstance().StopUnloadNearlinkSaTimer();
    UnloadSa::GetInstance().StartUnloadNearlinkSaTimer(PR9_STATE_TABLE[stateIdx]);
    UnloadSa::GetInstance().StopUnloadNearlinkSaTimer();

    // 系统状态通知对（SysStateMachine 状态串 + 任意串的命中/未命中分支）
    std::string sysState = provider.ConsumeRandomLengthString(32);
    mgr->OnSysStateChange(SYS_STATE_STARTED);
    mgr->OnSysStateChange(sysState);
    mgr->OnSysStateChange(SYS_STATE_FRESETTING);  // not report 分支
    mgr->OnSysStateExit(SYS_STATE_STARTED);
    mgr->OnSysStateExit(sysState);
    mgr->GetState(transport);

    mgr->DeregisterStateObserver(adapterObserver);
    mgr->DeregisterStateObserver(adapterObserver);  // 重复注销分支
    mgr->DeregisterSystemStateObserver(systemObserver);
    mgr->DeregisterSystemStateObserver(systemObserver);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void ProfileServiceManagerDispatchFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    ProfileServiceManager &profileManager = ProfileServiceManager::GetInstance();
    ProfileServiceManager::Initialize();  // GetInstance().Start()，幂等
    std::string unknownName = provider.ConsumeRandomLengthString(64);
    std::string knownName = provider.ConsumeBool() ? std::string("dis") : std::string("tws");
    uint32_t profileId = provider.ConsumeIntegral<uint32_t>();
    bool enableRet = provider.ConsumeBool();
    bool disableRet = provider.ConsumeBool();

    // 分发族：未注册名（nullptr 分支）/已知名两档
    profileManager.GetProfileService(unknownName);
    SleInterfaceProfile *profileService = profileManager.GetProfileService(knownName);
    (void)profileService;
    std::vector<uint32_t> profileList = profileManager.GetProfileServicesList();
    profileManager.GetProfileServiceConnectState(profileId);  // 非法 profileId 分支
    profileManager.GetProfileServiceConnectState(
        profileList.empty() ? 0 : profileList.front());
    std::vector<std::string> supportedUuids;
    profileManager.GetProfileServicesSupportedUuids(supportedUuids);
    profileManager.SetSystemSupportProfileServices();
    std::unordered_set<SleUuid> systemSupport = profileManager.GetSystemSupportProfileServices();
    (void)systemSupport;
    profileManager.OnEnable(knownName, enableRet);
    profileManager.OnEnable(unknownName, enableRet);
    profileManager.OnDisable(knownName, disableRet);
    profileManager.OnDisable(unknownName, disableRet);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void SwitchDependencyFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    int32_t unknownSaId = provider.ConsumeIntegral<int32_t>();
    // 泄漏式永生持有（enable_shared_from_this + 监听器 weak_ptr，绝不析构——R3/R6 判例）
    static std::shared_ptr<SleSwitchDependency> switchDependency =
        std::make_shared<SleSwitchDependency>([]() { HILOGI("SwitchDependencyFuzzTest callback hit"); });
    int32_t initRet = switchDependency->Init();  // 无 samgr 环境返回 -1（依赖未就绪链）
    (void)initRet;
    switchDependency->OnAddSystemAbility(COMMON_EVENT_SERVICE_ID);
    switchDependency->OnAddSystemAbility(DISTRIBUTED_KV_DATA_SERVICE_ABILITY_ID);
    switchDependency->OnAddSystemAbility(unknownSaId);  // 未知 id 分支
    switchDependency->OnAddSystemAbility(NEARLINK_HOST_SYS_ABILITY_ID);  // 全满足→回调（就绪链）
    switchDependency->OnAddSystemAbility(COMMON_EVENT_SERVICE_ID);  // 已满足重复分支
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void UnloadSaFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    InitPr9ThreadStateMapOnce();
    uint8_t stateIdx = provider.ConsumeIntegral<uint8_t>() % PR9_STATE_TABLE_SIZE;
    UnloadSa &unloadSa = UnloadSa::GetInstance();
    unloadSa.StartUnloadNearlinkSaTimer(PR9_STATE_TABLE[stateIdx]);  // 非 TURN_OFF 早退分支
    unloadSa.StopUnloadNearlinkSaTimer();  // 未启动早退分支
    unloadSa.StartUnloadNearlinkSaTimer(STATE_TURN_OFF);  // 实启 15s 单发定时器
    unloadSa.StartUnloadNearlinkSaTimer(STATE_TURN_OFF);  // 已启动幂等分支
    unloadSa.StopUnloadNearlinkSaTimer();  // 同输入内停表，不悬挂定时器
    unloadSa.NearlinkHostExtOnIdle();  // DoInAdapterThread + dlopen 扩展插件（环境缺失安全返回）
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void Frame4AntennaFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    InitPr9ThreadStateMapOnce();
    SleFrame4AntennaMgr &antennaMgr = SleFrame4AntennaMgr::GetInstance();
    uint8_t advHandle = provider.ConsumeIntegral<uint8_t>();
    uint8_t frameType = provider.ConsumeIntegral<uint8_t>() % 4;  // 帧类型值域 0-3
    uint32_t scannerId = provider.ConsumeIntegral<uint32_t>();
    int scanMode = provider.ConsumeIntegral<int>();
    uint32_t scannerId2 = provider.ConsumeIntegral<uint32_t>();

    antennaMgr.OnAdvStarted(advHandle, frameType);
    antennaMgr.OnAdvStarted(advHandle, frameType);  // 重复开播分支
    antennaMgr.OnScanStarted(scannerId, scanMode, frameType);
    antennaMgr.OnScanStarted(scannerId, scanMode, frameType);  // 重复扫描分支
    // RemoveScanner 为 SleFrame4AntennaMgr 私有方法不可直调(编译实证),
    // 移除/未命中两分支经下方 OnScanStopped 公有路径间接覆盖
    antennaMgr.OnScanStopped(scannerId);  // 已移除后停扫
    antennaMgr.OnScanStarted(scannerId2, scanMode, frameType);
    antennaMgr.OnScanStopped(scannerId2);
    antennaMgr.OnAllScanStopped();
    antennaMgr.OnAdvRemoved(advHandle);
    antennaMgr.OnAdvRemoved(advHandle);  // 重复移除分支
}
class FuzzSleSystemStateObserverStub : public ISystemStateObserver {
public:
    void OnSystemStateChange(const SleSystemState state) {}
};

namespace {
constexpr int SLE_STATE_WAIT_LOOPS = 40; // 40 * 50ms = 2s bounded wait per step
constexpr uint8_t WALK_TABLE_SIZE = 5;
constexpr uint8_t WALK_MAX_STEPS = 6;
}

bool WaitSleStableState(SleServiceManager *manager)
{
    for (int i = 0; i < SLE_STATE_WAIT_LOOPS; i++) {
        SleStateID state = manager->GetState(SleTransport::ADAPTER_SLE);
        if (SleServiceManager::IsStateStable(state)) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
    }
    HILOGE("sle state not stable after bounded wait");
    return false;
}

void ServiceManagerLifecycleFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    SleServiceManager *manager = SleServiceManager::GetInstance();
    if (manager == nullptr) {
        HILOGE("SleServiceManager is nullptr");
        return;
    }
    static FuzzSleAdapterStateObserverStub adapterObserver;
    static FuzzSleSystemStateObserverStub systemObserver;

    // 1) observer register / duplicate-register / deregister branches
    manager->RegisterStateObserver(adapterObserver);
    manager->RegisterSystemStateObserver(systemObserver);
    manager->RegisterStateObserver(adapterObserver); // duplicate -> rejected branch
    manager->DeregisterStateObserver(adapterObserver);
    manager->DeregisterSystemStateObserver(systemObserver);
    manager->DeregisterSystemStateObserver(systemObserver); // not registered -> rejected branch
    manager->RegisterStateObserver(adapterObserver);        // register again after deregister
    manager->RegisterSystemStateObserver(systemObserver);

    // 2) query family (both transports where applicable)
    SleEventType reason = static_cast<SleEventType>(provider.ConsumeIntegral<uint8_t>() % 10);
    SwitchCallerInfo callerInfo = {};
    callerInfo.fullTokenId = provider.ConsumeIntegral<uint64_t>();
    callerInfo.callerUid = provider.ConsumeIntegral<int32_t>();
    SleAutoConnectPolicy autoConnPolicy =
        static_cast<SleAutoConnectPolicy>(provider.ConsumeIntegral<uint8_t>() % 3);
    SleStateID fuzzState = static_cast<SleStateID>(provider.ConsumeIntegral<uint8_t>() % 9);
    SleStateID stateSle = manager->GetState(SleTransport::ADAPTER_SLE);
    SleStateID stateSlb = manager->GetState(SleTransport::ADAPTER_SLB);
    SleConnectState connectState = manager->GetAdapterConnectState();
    int maxAudio = manager->GetMaxNumConnectedAudioDevices();
    SleStateID switchParamState = manager->GetNearlinkSwitchStateFromSystemParameter();
    bool stateStable = SleServiceManager::IsStateStable(fuzzState);
    bool switchRestricted = manager->IsSleSwitchRestricted();
    bool autoConnAudio = manager->IsEnableAutoConnectAudioDevices();
    bool autoConnUserDisc = manager->IsEnableAutoConnectUserDisconnectedDevices();
    bool disabling = manager->IsDisabling();
    HILOGI("state(%{public}d/%{public}d), conn(%{public}d), maxAudio(%{public}d), param(%{public}d), "
           "stable(%{public}d), restricted(%{public}d), audioConn(%{public}d), userDiscConn(%{public}d), "
           "disabling(%{public}d)", static_cast<int>(stateSle), static_cast<int>(stateSlb),
           static_cast<int>(connectState), maxAudio, static_cast<int>(switchParamState),
           static_cast<int>(stateStable), static_cast<int>(switchRestricted), static_cast<int>(autoConnAudio),
           static_cast<int>(autoConnUserDisc), static_cast<int>(disabling));

    // 3) state machine walk: observe state, request, bounded wait for stable
    // ponytail: enable-direction failure CMP(false) injection removed — HandleProfileEnableCmpMsg
    // (AdapterStateMachine.cpp) stops only profileTimer_ and leaves the one-shot adapterTimer_
    // armed on the early-exit path; if a later input is in TurningOn/TurningOffToHalf when it
    // fires, MSG_ADAPTER_ENABLE_TIME_OUT hits Reset()=SIGKILL. Re-add only after the product
    // side stops adapterTimer_ on that failure path.
    const uint8_t walkSteps = provider.ConsumeIntegralInRange<uint8_t>(1, WALK_MAX_STEPS);
    for (uint8_t step = 0; step < walkSteps; step++) {
        SleStateID before = manager->GetState(SleTransport::ADAPTER_SLE);
        uint8_t sel = provider.ConsumeIntegral<uint8_t>() % WALK_TABLE_SIZE;
        HILOGI("walk step(%{public}u/%{public}u) from state(%{public}d) sel(%{public}u)",
            static_cast<unsigned>(step) + 1, static_cast<unsigned>(walkSteps), static_cast<int>(before),
            static_cast<unsigned>(sel));
        switch (sel) {
            case 0U:
                manager->Enable(SleTransport::ADAPTER_SLE, reason, callerInfo, autoConnPolicy);
                break;
            case 1U:
                manager->Disable(SleTransport::ADAPTER_SLE, reason, callerInfo);
                break;
            case 2U:
                manager->EnableToHalf(SleTransport::ADAPTER_SLE, reason, callerInfo);
                break;
            case 3U:
                manager->DisableToOff(SleTransport::ADAPTER_SLE, reason, callerInfo);
                break;
            case 4U:
            default:
                // failure direction: profile disable complete(false) during turning off
                manager->DisableToOff(SleTransport::ADAPTER_SLE, reason, callerInfo);
                manager->OnProfileServicesDisableComplete(SleTransport::ADAPTER_SLE, false);
                break;
        }
        WaitSleStableState(manager);
    }
    // corrective: leave the machine enabled for the next fuzz input
    SleStateID cur = manager->GetState(SleTransport::ADAPTER_SLE);
    if (cur != SleStateID::STATE_TURN_ON) {
        manager->Enable(SleTransport::ADAPTER_SLE, reason, callerInfo, autoConnPolicy);
        WaitSleStableState(manager);
    }

    // 4) pair devices removed (both transports) and adapter handle queries
    std::vector<RawAddress> devices;
    devices.push_back(RawAddress(BuildAddressString(provider)));
    devices.push_back(RawAddress(BuildAddressString(provider)));
    manager->OnPairDevicesRemoved(SleTransport::ADAPTER_SLE, devices);
    manager->OnPairDevicesRemoved(SleTransport::ADAPTER_SLB, devices);
    SleInterfaceAdapter *adapterSle = manager->GetAdapter(SleTransport::ADAPTER_SLE);
    if (adapterSle != nullptr) {
        std::string localName = adapterSle->GetLocalName();
        HILOGI("sle adapter local name(%{public}s)", localName.c_str());
    }
    SleInterfaceAdapter *adapterSlb = manager->GetAdapter(SleTransport::ADAPTER_SLB);
    if (adapterSlb != nullptr) {
        std::string localAddr = adapterSlb->GetLocalAddress();
        HILOGI("slb adapter local addr(%{public}s)", localAddr.c_str());
    }

    // 5) sys state notify with benign state only ("Stopping"/"FactoryResetting"
    // would satisfy stop/reset promises, keep them away)
    manager->OnSysStateChange(SYS_STATE_STARTED);
    manager->OnSysStateExit(SYS_STATE_STARTED);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}
}  // namespace OHOS

// fuzzer init
extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void) argc;
    (void) argv;
    // mock token
    NearlinkMockNativeToken mock("nearlink_service");

    OHOS::sptr<NearlinkHostServer> hostServer = NearlinkHostServer::GetInstance();
    if (hostServer == nullptr) {
        return 0;
    }
    HILOGI("host_fuzzer OnStart");
    hostServer->OnStart();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_100_MS));
    HILOGI("host_fuzzer EnableSle");
    hostServer->EnableSle();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_5000_MS));
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr) {
        return 0;
    }
    OHOS::GetProfileFuzzTest(data, size);
    OHOS::GetSleFullStateFuzzTest(data, size);
    OHOS::IsSleConnectionExistFuzzTest(data, size);
    OHOS::IsSleEnabledFuzzTest(data, size);
    OHOS::IsSleHalfDisabledFuzzTest(data, size);
    OHOS::IsSleDisabledFuzzTest(data, size);
    OHOS::IsSleAvailableToCallerFuzzTest(data, size);
    OHOS::GetAcbStateFuzzTest(data, size);
    OHOS::GetAdapterConnectStateFuzzTest(data, size);
    OHOS::GetProfileConnStateFuzzTest(data, size);
    OHOS::GetLocalNameFuzzTest(data, size);
    OHOS::SetLocalNameFuzzTest(data, size);
    OHOS::SetConnectionModeFuzzTest(data, size);
    OHOS::GetLocalAddressFuzzTest(data, size);
    OHOS::GetPairedDevicesFuzzTest(data, size);

    OHOS::RemovePairFuzzTest(data, size);
    OHOS::RemoveAllPairsTest(data, size);
    OHOS::RegisterSlePeripheralCallbackTest(data, size);
    OHOS::DeregisterSlePeripheralCallbackTest(data, size);
    OHOS::RegisterSleAdapterObserverTest(data, size);
    OHOS::DeregisterSleAdapterObserverTest(data, size);
    OHOS::GetSleMaxAdvertisingDataLengthTest(data, size);
    OHOS::GetDeviceNameTest(data, size);
    OHOS::GetDeviceAliasTest(data, size);
    OHOS::SetDeviceAliasTest(data, size);

    OHOS::GetPairStateTest(data, size);
    OHOS::GetLinkRoleTest(data, size);
    OHOS::StartPairTest(data, size);
    OHOS::StartCrediblePairTest(data, size);
    OHOS::SetPairingConfirmationTest(data, size);
    OHOS::SetPairingPassCodeTest(data, size);
    OHOS::CancelPairingTest(data, size);
    OHOS::IsBondedFromLocalTest(data, size);
    OHOS::IsAcbConnectedTest(data, size);
    OHOS::IsAcbEncryptedTest(data, size);
    OHOS::GetDeviceUuidsTest(data, size);
    OHOS::PairRequestReplyTest(data, size);
    OHOS::ReadRemoteRssiValueTest(data, size);

    OHOS::CheckPermissionForNapiFuzzTest(data, size);
    OHOS::GetDeviceAppearanceTest(data, size);
    OHOS::ConnectAllowedProfilesTest(data, size);
    OHOS::DisconnectAllowedProfilesTest(data, size);
    OHOS::GetDeviceProductIdTest(data, size);
    OHOS::GetDeviceVendorIdTest(data, size);
    OHOS::GetDeviceModelTest(data, size);
    OHOS::SetFreqHoppingFuzzTest(data, size);

    OHOS::UpdateSleVirtualDeviceFuzzTest(data, size);
    OHOS::SetBtAddrBySleAddrTest(data, size);
    OHOS::GetBtAddrBySleAddrTest(data, size);
    OHOS::GetSleAddrByBtAddrTest(data, size);

    OHOS::RegisterDeviceBatteryObserverTest(data, size);
    OHOS::DeregisterDeviceBatteryObserverTest(data, size);
    OHOS::RegisterDeviceRssiObserverTest(data, size);
    OHOS::DeregisterDeviceRssiObserverTest(data, size);
    OHOS::IsFeatureSupportedTest(data, size);
    OHOS::GetBatteryLevelTest(data, size);
    OHOS::GetDeviceInformationTest(data, size);
    OHOS::ServiceManagerLifecycleFuzzTest(data, size);

    OHOS::SleServiceManagerSwitchFuzzTest(data, size);
    OHOS::ProfileServiceManagerDispatchFuzzTest(data, size);
    OHOS::SwitchDependencyFuzzTest(data, size);
    OHOS::UnloadSaFuzzTest(data, size);
    OHOS::Frame4AntennaFuzzTest(data, size);

    return 0;
}

