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

#include <thread>
#include "fuzzer/FuzzedDataProvider.h"
#include "host_fuzzer.h"
#include "nearlink_native_token_mock.h"
#include "../nl_utils/fuzztest_utils.h"
#include "nearlink_host_stub.h"
#include "nearlink_host_server.h"
#include "nearlink_errorcode.h"
#include "log.h"
#include "securec.h"
#include "raw_address.h"

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

namespace {
constexpr int HOST_FUZZ_DELAY_50_MS = 50;
constexpr int HOST_FUZZ_DELAY_100_MS = 100;
constexpr int HOST_FUZZ_DELAY_5000_MS = 5000;
constexpr int SLE_FREQ_ARRAY_LEN = 10;
sptr<INearlinkSlePeripheralObserver> g_slePeripheralObserver =
    new (std::nothrow) MockNearlinkSlePeripheralObserverStub();
sptr<INearlinkHostObserver> g_hostObserver = new (std::nothrow) MockNearlinkHostObserverStub();
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

    return 0;
}

