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
#include "centralmanager_fuzzer.h"
#include "nearlink_native_token_mock.h"
#include "../nl_utils/fuzztest_utils.h"
#include "nearlink_sle_central_manager_stub.h"
#include "nearlink_sle_central_manager_server.h"
#include "nearlink_host_server.h"
#include "ThreadUtil.h"
#include "log.h"
#include "securec.h"
#include "raw_address.h"
#include "SleRemoteDeviceAdapter.h"
#include "SleReconnectManager.h"

using namespace std;
using namespace OHOS::Nearlink;
namespace OHOS {

class MockNearlinkSleCentralManagerStub : public IRemoteStub<INearlinkSleCentralManagerCallback> {
public:
    void OnScanCallback(const NearlinkSleScanResult &result) {}
    void OnSleBatchScanResultsEvent(std::vector<NearlinkSleScanResult> &results) {}
    void OnStartOrStopScanEvent(int resultCode, bool isStartScan) {}
};

namespace {
constexpr int HOST_FUZZ_DELAY_50_MS = 50;
constexpr int HOST_FUZZ_DELAY_100_MS = 100;
constexpr int HOST_FUZZ_DELAY_5000_MS = 5000;
sptr<NearlinkSleCentralManagerServer> g_centralManager = new (std::nothrow) NearlinkSleCentralManagerServer();
sptr<INearlinkSleCentralManagerCallback> g_centralManagerCb = new (std::nothrow) MockNearlinkSleCentralManagerStub();
ThreadUtil &g_threadUtil = ThreadUtil::GetInstance();
uint32_t g_scannerId = 0;
}

int32_t CentralManagerOnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply)
{
    NL_CHECK_RETURN_RET(g_centralManager, TRANSACTION_ERR, "g_centralManager is nullptr");
    MessageOption option = {MessageOption::TF_SYNC};
    HILOGI("CentralManagerOnRemoteRequest, cmd(%{public}d)", code);
    return g_centralManager->OnRemoteRequest(code, data, reply, option);
}

void RegisterSleCentralManagerCallbackFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSleCentralManagerStub::GetDescriptor());
    if (g_centralManagerCb != nullptr) {
        data.WriteRemoteObject(g_centralManagerCb->AsObject());
    }
    data.WriteBool(provider.ConsumeBool());

    int32_t ret = CentralManagerOnRemoteRequest(
        NearlinkSleCentralManagerInterfaceCode::SLE_REGISTER_SLE_CENTRAL_MANAGER_CALLBACK, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
        return;
    }
    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        g_scannerId = reply.ReadUint32();
        HILOGI("g_scannerId(%{public}d)", g_scannerId);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void DeRegisterSleCentralManagerCallbackFuzzTest()
{
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSleCentralManagerStub::GetDescriptor());
    data.WriteInt32(g_scannerId); // scannerId
    if (g_centralManagerCb != nullptr) {
        data.WriteRemoteObject(g_centralManagerCb->AsObject());
    }

    int32_t ret = CentralManagerOnRemoteRequest(
        NearlinkSleCentralManagerInterfaceCode::SLE_DE_REGISTER_SLE_CENTRAL_MANAGER_CALLBACK, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void StartFullScanInnerFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSleCentralManagerStub::GetDescriptor());
    data.WriteUint32(g_scannerId); // scannerId
    NearlinkSleScanSettings settings;
    settings.SetReportDelay(provider.ConsumeIntegral<long>());
    settings.SetScanMode(provider.ConsumeIntegral<int>());
    settings.SetLegacy(provider.ConsumeBool());
    settings.SetPhy(provider.ConsumeIntegral<int>());
    data.WriteParcelable(&settings);

    int32_t ret = CentralManagerOnRemoteRequest(
        NearlinkSleCentralManagerInterfaceCode::SLE_START_FULL_SCAN, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void StartScanWithFilterFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSleCentralManagerStub::GetDescriptor());
    data.WriteUint32(g_scannerId); // scannerId
    NearlinkSleScanSettings settings;
    settings.SetReportDelay(provider.ConsumeIntegral<long>());
    settings.SetScanMode(provider.ConsumeIntegral<int>());
    settings.SetLegacy(provider.ConsumeBool());
    settings.SetPhy(provider.ConsumeIntegral<int>());
    data.WriteParcelable(&settings);
    data.WriteUint32(1); // filter size
    NearlinkSleScanFilter scanFilter;
    scanFilter.SetDeviceId("");
    scanFilter.SetName("test");
    data.WriteParcelable(&scanFilter);

    int32_t ret = CentralManagerOnRemoteRequest(
        NearlinkSleCentralManagerInterfaceCode::SLE_START_SCAN_WITH_FILTER, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void StopScanFuzzTest()
{
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSleCentralManagerStub::GetDescriptor());
    data.WriteUint32(g_scannerId); // scannerId

    int32_t ret = CentralManagerOnRemoteRequest(
        NearlinkSleCentralManagerInterfaceCode::SLE_STOP_SCAN, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void AdapterDeviceLifecycleFuzzTest(const uint8_t *fuzzData, size_t size)
{
    SleRemoteDeviceAdapter *adapter = SleRemoteDeviceAdapter::GetInstance();
    if (adapter == nullptr) {
        HILOGI("AdapterDeviceLifecycleFuzzTest adapter is nullptr");
        return;
    }
    FuzzedDataProvider provider(fuzzData, size);
    std::string addr1 = BuildAddressString(provider);
    std::string addr2 = "11:22:33:44:55:66"; // 固定地址，保证与 FDP 地址不同
    RawAddress device1(addr1);
    RawAddress device2(addr2);
    HILOGI("AdapterDeviceLifecycleFuzzTest enter");

    std::shared_ptr<SlePeripheralDevice> peerDevice1 = std::make_shared<SlePeripheralDevice>();
    std::shared_ptr<SlePeripheralDevice> peerDevice2 = std::make_shared<SlePeripheralDevice>();
    peerDevice1->SetAddress(device1);
    peerDevice2->SetAddress(device2);
    adapter->AddPeripheralDevice(addr1, peerDevice1);
    adapter->AddPeripheralDevice(addr2, peerDevice2);

    adapter->SetPairStatus(device1, static_cast<int>(SlePairState::SLE_PAIR_PAIRING));
    int fuzzPairState = static_cast<int>(SlePairState::SLE_PAIR_NONE) +
        static_cast<int>(provider.ConsumeIntegral<uint8_t>() % 4); // 1..4
    adapter->SetPairStatus(device2, fuzzPairState);
    adapter->SetPrePairStatus(device1, fuzzPairState);
    int acbState = static_cast<int>(static_cast<SleConnState>(provider.ConsumeIntegral<uint8_t>() % 5)); // 0..4
    adapter->SetAcbState(addr1, acbState);
    adapter->SetAcbState(addr2, static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED));
    uint16_t lcid = provider.ConsumeIntegral<uint16_t>();
    adapter->SetLcid(addr1, lcid);
    uint8_t role = provider.ConsumeIntegral<uint8_t>() % 2;
    uint8_t addrType = provider.ConsumeIntegral<uint8_t>() % 2;
    adapter->ConnectionCompleteHelper(device1, lcid, role, addrType);
    adapter->SetAliasName(device1, provider.ConsumeRandomLengthString(16));
    adapter->SetName(device1, provider.ConsumeRandomLengthString(16));
    adapter->SetAppearance(device1, provider.ConsumeIntegral<int32_t>());
    uint8_t cryptoAlgo = provider.ConsumeIntegral<uint8_t>();
    uint8_t keyDerivAlgo = provider.ConsumeIntegral<uint8_t>();
    uint8_t integrChkInd = provider.ConsumeIntegral<uint8_t>();
    adapter->SetPairAlgoInfo(device1, cryptoAlgo, keyDerivAlgo, integrChkInd);
    std::string encryptGroupKeyStr = provider.ConsumeRandomLengthString(32);
    uint64_t giv = provider.ConsumeIntegral<uint64_t>();
    adapter->SetGroupAndGiv(device1, encryptGroupKeyStr, giv);

    // 查询族
    std::string deviceName = adapter->GetDeviceName(device1);
    int pairState = adapter->GetPairState(device1);
    std::string aliasName = adapter->GetAliasName(device1);
    int appearance = adapter->GetDeviceAppearance(device1);
    std::vector<Uuid> deviceUuids = adapter->GetDeviceUuids(device1);
    bool isAcbConnected = adapter->IsAcbConnected(device1);
    bool isAcbEncrypted = adapter->IsAcbEncrypted(device1);
    bool isBondedFromLocal = adapter->IsBondedFromLocal(device1);
    uint8_t linkRole = adapter->GetLinkRole(device1);
    uint16_t lcidRet = adapter->GetLcidByAddress(device1);
    adapter->GetAddressByLcid(lcid);
    int acbStateRet = adapter->GetAcbState(addr1);
    std::vector<RawAddress> pairedDevices = adapter->GetPairedDevices();
    std::vector<RawAddress> connectedDevices = adapter->GetConnectedDevices();
    bool hasConnectedDevice = adapter->HasConnectedDevice();
    int pairedCnt = adapter->GetPairedCnt(device1);
    int connectedCnt = adapter->GetConnectedCnt();
    bool isAudioDevice = adapter->IsAudioDevice(addr1);
    int businessType = adapter->GetManufacturerBusinessType(device1);
    int connDirect = adapter->GetConnDirect(device1);
    uint8_t peerAddrType = adapter->GetPeerDeviceAddrType(device1);
    uint8_t cryptoRet = 0;
    uint8_t keyDerivRet = 0;
    uint8_t integrChkRet = 0;
    adapter->GetPairAlgoInfo(device1, cryptoRet, keyDerivRet, integrChkRet);
    std::string groupKeyRet;
    uint64_t givRet = 0;
    adapter->GetGroupAndGiv(device1, groupKeyRet, givRet);
    HILOGI("AdapterDeviceLifecycleFuzzTest pairState(%{public}d) appearance(%{public}d) acbState(%{public}d) "
        "isAcbConnected(%{public}d) isAcbEncrypted(%{public}d) isBondedFromLocal(%{public}d) "
        "linkRole(%{public}u) lcidRet(%{public}u) hasConnectedDevice(%{public}d) pairedCnt(%{public}d) "
        "connectedCnt(%{public}d) isAudioDevice(%{public}d) businessType(%{public}d) connDirect(%{public}d) "
        "peerAddrType(%{public}u) pairedSize(%{public}zu) connectedSize(%{public}zu) nameLen(%{public}zu) "
        "aliasLen(%{public}zu) uuidSize(%{public}zu) givRet(%{public}llu)",
        pairState, appearance, acbStateRet, isAcbConnected, isAcbEncrypted, isBondedFromLocal, linkRole, lcidRet,
        hasConnectedDevice, pairedCnt, connectedCnt, isAudioDevice, businessType, connDirect, peerAddrType,
        pairedDevices.size(), connectedDevices.size(), deviceName.size(), aliasName.size(), deviceUuids.size(),
        static_cast<unsigned long long>(givRet));

    adapter->AddBgConnDevice(addr1);
    NearlinkSafeList<RawAddress> bgList;
    std::vector<std::string> reconnectList;
    adapter->GetBgList(bgList, reconnectList);
    NearlinkSafeList<RawAddress> directConnList;
    adapter->GetDirectConnList(directConnList);

    std::string btAddr = provider.ConsumeRandomLengthString(17);
    adapter->SetBtAddrBySleAddr(addr1, btAddr);
    std::string btAddrRet;
    adapter->GetBtAddrBySleAddrTask(addr1, btAddrRet);
    std::string sleAddrRet;
    adapter->GetSleAddrByBtAddrTask(btAddr, sleAddrRet);

    adapter->GetRealAddress(device1);
    adapter->SavePairDirect(static_cast<int>(provider.ConsumeIntegral<uint8_t>() % 2), device1);

    adapter->RemovePeripheralDevice(addr2);
    adapter->RemoveAllPeripheralDevices();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void ReconnectLifecycleFuzzTest(const uint8_t *fuzzData, size_t size)
{
    SleReconnectManager &reconnectManager = SleReconnectManager::GetInstance();
    FuzzedDataProvider provider(fuzzData, size);
    std::string addr = BuildAddressString(provider);
    RawAddress device(addr);
    HILOGI("ReconnectLifecycleFuzzTest enter");

    reconnectManager.SetBgConnMaxNum(provider.ConsumeIntegral<uint8_t>() % 17);
    std::vector<std::string> reconnectList = reconnectManager.GetReconnectList();
    std::string reconnDevice = reconnectManager.GetReconnDevice();
    reconnectManager.SetReconnDeviceParam();
    reconnectManager.RemoveAutoConnAudioDevConf(device);
    bool isNeedToIgnore = reconnectManager.IsNeedToIgnore(addr);
    int pairState = static_cast<int>(SlePairState::SLE_PAIR_NONE) +
        static_cast<int>(provider.ConsumeIntegral<uint8_t>() % 4); // 1..4
    reconnectManager.SetAutoConnectDevice(device, provider.ConsumeBool(), pairState,
        provider.ConsumeRandomLengthString(17));
    reconnectManager.OnDeviceStartPair(device);
    reconnectManager.OnDeviceConnected(device, true);
    reconnectManager.OnDeviceConnected(device, false);
    reconnectManager.OnDeviceDisConnected(device, true);
    reconnectManager.OnDeviceDisConnected(device, false);
    reconnectManager.OnDeviceUnpaired(device);
    HILOGI("ReconnectLifecycleFuzzTest reconnectListSize(%{public}zu) isNeedToIgnore(%{public}d) "
        "reconnDeviceLen(%{public}zu)", reconnectList.size(), isNeedToIgnore, reconnDevice.size());
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}
}

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
    HILOGI("advertiser_fuzzer OnStart");
    hostServer->OnStart();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_100_MS));
    HILOGI("advertiser_fuzzer EnableSle");
    hostServer->EnableSle();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_5000_MS));
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr) {
        OHOS::g_threadUtil.ClearThreadStateMap();
        return 0;
    }

    /* Run your code on data */
    OHOS::g_threadUtil.InitThreadStateMap();
    OHOS::RegisterSleCentralManagerCallbackFuzzTest(data, size);
    OHOS::StartFullScanInnerFuzzTest(data, size);
    OHOS::StartScanWithFilterFuzzTest(data, size);
    OHOS::StopScanFuzzTest();
    OHOS::DeRegisterSleCentralManagerCallbackFuzzTest();
    OHOS::AdapterDeviceLifecycleFuzzTest(data, size);
    OHOS::ReconnectLifecycleFuzzTest(data, size);
    OHOS::g_threadUtil.ClearThreadStateMap();
    return 0;
}

