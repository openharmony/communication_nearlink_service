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
#include "enable_fuzzer.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "nearlink_native_token_mock.h"
#include "../nl_utils/fuzztest_utils.h"
#include "nearlink_host_stub.h"
#include "nearlink_host_server.h"
#include "nearlink_errorcode.h"
#include "log.h"
#include "securec.h"
#include "ScanService.h"
#include "ProfileServiceManager.h"
#include "raw_address.h"

using namespace std;
using namespace OHOS::Nearlink;
namespace OHOS {
namespace {
constexpr int HOST_FUZZ_DELAY_10_MS = 50;
constexpr int HOST_FUZZ_DELAY_100_MS = 100;
constexpr int HOST_FUZZ_DELAY_5000_MS = 5000;
constexpr int HOST_FUZZ_SCAN_DELAY_MS = 100;
// 广播厂商数据布局(星闪耳机私有格式): business + {type,len...} 系列 TLV
constexpr uint16_t HOST_FUZZ_HUAWEI_COMPANY_ID = 0x0009;  // SLE_COMPANY_ID_HUAWEI
constexpr uint8_t HOST_FUZZ_CDSM_REPORT_ADDR = 1;
constexpr uint8_t HOST_FUZZ_CDSM_PAIR_ADDR = 2;
constexpr uint8_t HOST_FUZZ_EARPHONE_DISPLAY_CONTROL = 3;
constexpr uint8_t HOST_FUZZ_MANUFACTURER_ABILITY = 4;
constexpr size_t HOST_FUZZ_ADV_DATA_MAX_LEN = 31;
constexpr size_t HOST_FUZZ_LOCAL_NAME_MAX_LEN = 16;
constexpr size_t HOST_FUZZ_SLE_ADDR_LEN = RawAddress::SLE_ADDRESS_BYTE_LEN;
constexpr size_t HOST_FUZZ_MANU_DATA_LEN = 1 + (1 + HOST_FUZZ_SLE_ADDR_LEN) + (1 + HOST_FUZZ_SLE_ADDR_LEN) +
    (1 + 1) + (1 + SLE_MANU_ABILITY_LEN);
}

int32_t HostOnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply)
{
    sptr<NearlinkHostServer> hostServer = NearlinkHostServer::GetInstance();
    NL_CHECK_RETURN_RET(hostServer, TRANSACTION_ERR, "hostServer is nullptr");
    MessageOption option = {MessageOption::TF_SYNC};
    HILOGI("HostOnRemoteRequest cmd(%{public}d)", code);
    return hostServer->OnRemoteRequest(code, data, reply, option);
}

void DisableSleFuzzTest(const uint8_t *fuzzData, size_t size)
{
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_DISABLE_SLE, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
}

void EnableSleFuzzTest(const uint8_t *fuzzData, size_t size)
{
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_ENABLE_SLE, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
}

void DisableSleToOffFuzzTest(const uint8_t *fuzzData, size_t size)
{
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_DISABLE_SLE_TO_OFF, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
}

void EnableSleToHalfFuzzTest(const uint8_t *fuzzData, size_t size)
{
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_ENABLE_SLE_TO_HALF, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
}

void FactoryResetFuzzTest(const uint8_t *fuzzData, size_t size)
{
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_FACTORY_RESET, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
}

// minimal ISleCentralManagerCallback stub: all pure virtuals from interface_scan_service.h
class FuzzSleCentralManagerCallback : public ISleCentralManagerCallback {
public:
    void OnScanCallback(const std::vector<uint32_t> &scannerIds, const SleScanResultImpl &result) override {}
    void OnSleBatchScanResultsEvent(std::vector<SleScanResultImpl> &results) override {}
    void OnStartOrStopScanEvent(int resultCode, bool isStartScan) override {}
};

// fill fixed-size byte array from fuzz provider(remaining bytes kept as-is)
static void FillBytesFromProvider(FuzzedDataProvider &provider, uint8_t *dst, size_t dstLen)
{
    std::vector<uint8_t> seed = provider.ConsumeBytes<uint8_t>(dstLen);
    for (size_t i = 0; i < seed.size() && i < dstLen; ++i) {
        dst[i] = seed[i];
    }
}

void ScanSessionLifecycleFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    ScanService &scanService = ScanService::GetInstance();
    /* static：ScanService 异步持有回调裸指针（DoInScanThread 存 pimpl->sleCentralManagerCallback_），
     * 栈对象在 Deregister 后仅 50ms 排空即析构，CI 高负载下有 UAF 竞态；进程级存活消除窗口 */
    static FuzzSleCentralManagerCallback callback;

    // register then wait for scan-thread callback bookkeeping
    scanService.RegisterSleCentralManagerCallback(callback);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));

    uint32_t scannerId = scanService.AllocScannerId();
    uint32_t scannerId2 = scanService.AllocScannerId();

    NearlinkSleScanSettings settings;
    settings.SetReportDelay(provider.ConsumeIntegral<long>());
    settings.SetScanMode(provider.ConsumeIntegral<int>());
    settings.SetLegacy(provider.ConsumeBool());
    settings.SetPhy(provider.ConsumeIntegral<int>());
    settings.SetFrameType(provider.ConsumeIntegral<uint8_t>());

    SleScanFilterImpl filter;
    filter.SetDeviceId(BuildAddressString(provider));
    filter.SetName(provider.ConsumeRandomLengthString(32));
    filter.SetServiceUuid(BuildUuid(provider));
    filter.SetManufacturerId(provider.ConsumeIntegral<uint16_t>());
    filter.SetClientId(provider.ConsumeIntegral<int>());
    filter.SetFiltIndex(provider.ConsumeIntegral<uint8_t>());
    filter.SetFilterAction(provider.ConsumeIntegral<uint8_t>());
    std::vector<SleScanFilterImpl> filters;
    filters.push_back(filter);

    scanService.StartScan(scannerId, settings, filters);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    scanService.StartScan(scannerId, settings, filters);  // duplicate start branch

    // ---- adv result, two variants: with/without manufacturer data ----
    uint8_t addrBytes[HOST_FUZZ_SLE_ADDR_LEN] = {};
    uint8_t addrBytes2[HOST_FUZZ_SLE_ADDR_LEN] = {};
    uint8_t reportBytes[HOST_FUZZ_SLE_ADDR_LEN] = {};
    uint8_t pairBytes[HOST_FUZZ_SLE_ADDR_LEN] = {};
    FillBytesFromProvider(provider, addrBytes, sizeof(addrBytes));
    FillBytesFromProvider(provider, addrBytes2, sizeof(addrBytes2));
    FillBytesFromProvider(provider, reportBytes, sizeof(reportBytes));
    FillBytesFromProvider(provider, pairBytes, sizeof(pairBytes));
    uint8_t advBuf[HOST_FUZZ_ADV_DATA_MAX_LEN] = {};
    uint8_t nameBuf[HOST_FUZZ_LOCAL_NAME_MAX_LEN] = {};
    FillBytesFromProvider(provider, advBuf, sizeof(advBuf));
    FillBytesFromProvider(provider, nameBuf, sizeof(nameBuf));
    SLE_Addr_S advAddr = {};
    advAddr.type = provider.ConsumeIntegral<uint8_t>();
    for (size_t i = 0; i < HOST_FUZZ_SLE_ADDR_LEN; ++i) {
        advAddr.addr[i] = addrBytes[i];
    }
    SLE_Addr_S advAddr2 = {};
    advAddr2.type = provider.ConsumeIntegral<uint8_t>();
    for (size_t i = 0; i < HOST_FUZZ_SLE_ADDR_LEN; ++i) {
        advAddr2.addr[i] = addrBytes2[i];
    }

    // empty SDF vector: non-null(checked by ParseAdvResult), size 0(skip element loops)
    SDF_Vector_S emptyVec = {};
    // scannerIds: two ids so NotifyScanResults reaches the callback path
    uint32_t scannerIdArr[2] = {scannerId, scannerId2};
    void *scannerIdPtrs[2] = {&scannerIdArr[0], &scannerIdArr[1]};
    SDF_Vector_S scannerIdsVec = {};
    scannerIdsVec.buf = scannerIdPtrs;
    scannerIdsVec.size = sizeof(scannerIdPtrs) / sizeof(scannerIdPtrs[0]);
    // one service uuid entry
    NLSTK_DevdAdvServiceUuid_S svcUuid = {};
    FillBytesFromProvider(provider, svcUuid.uuid, sizeof(svcUuid.uuid));
    void *svcUuidPtrs[1] = {&svcUuid};
    SDF_Vector_S svcUuidVec = {};
    svcUuidVec.buf = svcUuidPtrs;
    svcUuidVec.size = 1;
    // one manufacturer data entry: huawei company id + audio business TLVs(report/pair/display/ability)
    alignas(8) uint8_t manuStorage[sizeof(NLSTK_DevdAdvManufacturerData_S) + HOST_FUZZ_MANU_DATA_LEN] = {};
    NLSTK_DevdAdvManufacturerData_S *manuData = reinterpret_cast<NLSTK_DevdAdvManufacturerData_S *>(manuStorage);
    manuData->manufacturerId = HOST_FUZZ_HUAWEI_COMPANY_ID;
    manuData->len = HOST_FUZZ_MANU_DATA_LEN;
    manuData->data[0] = static_cast<uint8_t>(SLE_PRIVATE_AUDIO_BUSINESS_TYPE);
    manuData->data[1] = HOST_FUZZ_CDSM_REPORT_ADDR;
    manuData->data[8] = HOST_FUZZ_CDSM_PAIR_ADDR;
    manuData->data[15] = HOST_FUZZ_EARPHONE_DISPLAY_CONTROL;
    manuData->data[16] = provider.ConsumeIntegral<uint8_t>();
    manuData->data[17] = HOST_FUZZ_MANUFACTURER_ABILITY;
    for (size_t i = 0; i < HOST_FUZZ_SLE_ADDR_LEN; ++i) {
        manuData->data[2 + i] = reportBytes[i];  // report addr payload [2..7]
        manuData->data[9 + i] = pairBytes[i];    // pair addr payload [9..14]
    }
    FillBytesFromProvider(provider, &manuData->data[18], SLE_MANU_ABILITY_LEN);  // ability bitmap [18..33]
    void *manuPtrs[1] = {manuStorage};
    SDF_Vector_S manuVec = {};
    manuVec.buf = manuPtrs;
    manuVec.size = 1;

    // variant 1: full adv result with manufacturer data
    NLSTK_DevdAdvResult_S advResult = {};
    advResult.scannerIds = &scannerIdsVec;
    advResult.addr = advAddr;
    advResult.rssi = provider.ConsumeIntegral<int8_t>();
    advResult.txPower = provider.ConsumeIntegral<int8_t>();
    advResult.frameType = provider.ConsumeIntegral<uint8_t>();
    advResult.discoveryLevel = provider.ConsumeIntegral<uint8_t>();
    advResult.isConnectable = provider.ConsumeBool();
    advResult.isPencil = false;
    advResult.localName.len = HOST_FUZZ_LOCAL_NAME_MAX_LEN;
    advResult.localName.data = nameBuf;
    advResult.advData.len = HOST_FUZZ_ADV_DATA_MAX_LEN;
    advResult.advData.data = advBuf;
    advResult.serviceDataList = &emptyVec;
    advResult.serviceUuids = &svcUuidVec;
    advResult.manufacturerDataList = &manuVec;
    scanService.NotifyScanResults(&advResult);

    // variant 2: no manufacturer data, pencil flag from fuzz input
    NLSTK_DevdAdvResult_S advResult2 = {};
    advResult2.scannerIds = &scannerIdsVec;
    advResult2.addr = advAddr2;
    advResult2.rssi = provider.ConsumeIntegral<int8_t>();
    advResult2.txPower = provider.ConsumeIntegral<int8_t>();
    advResult2.frameType = provider.ConsumeIntegral<uint8_t>();
    advResult2.discoveryLevel = provider.ConsumeIntegral<uint8_t>();
    advResult2.isConnectable = provider.ConsumeBool();
    advResult2.isPencil = provider.ConsumeBool();
    advResult2.serviceDataList = &emptyVec;
    advResult2.serviceUuids = &emptyVec;
    advResult2.manufacturerDataList = &emptyVec;
    scanService.NotifyScanResults(&advResult2);

    scanService.NotifyStartOrStopScanEvent(provider.ConsumeIntegral<int>(), provider.ConsumeBool());
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_SCAN_DELAY_MS));

    // ---- query family against the just-fed addresses ----
    std::string advAddrStr = RawAddress::ConvertToString(addrBytes).GetAddress();
    std::string reportAddrStr = RawAddress::ConvertToString(reportBytes).GetAddress();
    std::string advAddrStr2 = RawAddress::ConvertToString(addrBytes2).GetAddress();
    scanService.GetDeviceName(advAddrStr);
    scanService.GetDeviceAppearance(advAddrStr);
    scanService.GetDeviceManufacturerAbility(advAddrStr);
    scanService.GetDeviceAddrType(advAddrStr);
    scanService.GetManufacturerBusinessType(reportAddrStr);
    scanService.IsAudioDevice(reportAddrStr);
    scanService.GetModelId(advAddrStr2);
    scanService.GetNewModelId(advAddrStr2);
    scanService.GetSubModelId(advAddrStr2);
    scanService.GetIconId(advAddrStr2);
    scanService.GetBtAddr(advAddrStr2);
    scanService.GetCurrentAddress(RawAddress(reportAddrStr));
    scanService.GetCollaborateAddress(RawAddress(reportAddrStr));
    scanService.GetReportAddrByCurrentAddress(RawAddress(advAddrStr));

    // ---- teardown ----
    scanService.StopScan(scannerId);
    scanService.StopScan(scannerId);  // second stop: not-started branch
    scanService.StopAllScan();
    scanService.RemoveScannerId(scannerId);
    scanService.RemoveScannerId(scannerId2);
    scanService.ClearScanResultInfo();
    scanService.DeregisterSleCentralManagerCallback();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
}

void ProfileServiceDispatchFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    ProfileServiceManager &profileServiceManager = ProfileServiceManager::GetInstance();
    std::string profileName = provider.ConsumeRandomLengthString(32);

    profileServiceManager.Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    profileServiceManager.GetProfileService(profileName);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    profileServiceManager.GetProfileServicesList();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    profileServiceManager.SetSystemSupportProfileServices();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    profileServiceManager.GetSystemSupportProfileServices();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    profileServiceManager.GetProfileServiceConnectState(static_cast<uint32_t>(provider.ConsumeIntegral<int32_t>()));
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    std::vector<std::string> supportUuids;
    profileServiceManager.GetProfileServicesSupportedUuids(supportUuids);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    profileServiceManager.Enable(SleTransport::ADAPTER_SLE);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    profileServiceManager.OnAllEnabled(SleTransport::ADAPTER_SLE);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    profileServiceManager.OnEnable(profileName, provider.ConsumeBool());
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    profileServiceManager.Disable(SleTransport::ADAPTER_SLE);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    profileServiceManager.OnAllDisabled(SleTransport::ADAPTER_SLE);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    profileServiceManager.OnDisable(profileName, provider.ConsumeBool());
    /* Stop() 会逐个 delete profile；Enable/Disable 的 profile 线程任务须先排空，
     * 100ms 排空窗口（红队 M2 加固；确定性排空需产品侧接缝，留档） */
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_100_MS));
    profileServiceManager.Stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
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
    OHOS::DisableSleFuzzTest(data, size);
    OHOS::EnableSleFuzzTest(data, size);
    OHOS::DisableSleToOffFuzzTest(data, size);
    OHOS::EnableSleToHalfFuzzTest(data, size);
    OHOS::FactoryResetFuzzTest(data, size);
    OHOS::ScanSessionLifecycleFuzzTest(data, size);
    OHOS::ProfileServiceDispatchFuzzTest(data, size);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_5000_MS));
    return 0;
}

