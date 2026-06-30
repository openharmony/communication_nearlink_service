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

#include "hidhost_fuzzer.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "nearlink_native_token_mock.h"
#include "../nl_utils/fuzztest_utils.h"
#include "nearlink_hid_host_stub.h"
#include "nearlink_hid_host_server.h"
#include "HidHostService.h"
#include "HidHostUhid.h"
#include "nearlink_host_server.h"
#include "nearlink_service_ipc_interface_code.h"
#include "ThreadUtil.h"
#include "log.h"
#include "securec.h"
#include "raw_address.h"
#include <thread>

using namespace std;
using namespace OHOS::Nearlink;
namespace OHOS {

namespace {
constexpr uint32_t MESSAGE_SIZE = NearlinkHostInterfaceCode::NL_HOST_BUTT;
sptr<NearlinkHidHostServer> g_hidHost = new (std::nothrow) NearlinkHidHostServer();
HidHostService *hidHostService = new (std::nothrow) HidHostService();
std::string g_report = "";

constexpr int HOST_FUZZ_DELAY_50_MS = 50;
constexpr int HOST_FUZZ_DELAY_100_MS = 100;
constexpr int HOST_FUZZ_DELAY_5000_MS = 5000;
constexpr int MESSAGE_BUTT = 4;
constexpr int HID_HOST_DATA_BUTT = 3;
constexpr int HID_HOST_DATA_BASE_BUTT = 3;
constexpr int HID_HOST_DATA_BASE = 200;
constexpr int REMOVE_STATE_MACHINE_EVENT = 13;
constexpr int CONNECT_STATE = 1;
constexpr int DECRIPTOR_SIZE = 10;
constexpr int DECRIPTOR_VAULE_SIZE = 5;
constexpr int PROPERTY_SIZE = 41;
constexpr int PROPERTY_VAULE_SIZE = 8;
constexpr int PROPERTY_DECRIPTOR_SIZE = 3;
constexpr int SEND_DATA_LEN = 10;
ThreadUtil &g_threadUtil = ThreadUtil::GetInstance();
}

int32_t HidHostOnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply)
{
    NL_CHECK_RETURN_RET(g_hidHost, TRANSACTION_ERR, "g_ssapClient is nullptr");
    MessageOption option = {MessageOption::TF_SYNC};
    HILOGI("HidHostOnRemoteRequest, cmd(%{public}d)", code);
    return g_hidHost->OnRemoteRequest(code, data, reply, option);
}

void HidHostSetReportFuzzTest(const uint8_t* data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    HILOGI("start");
    const uint32_t addressLen = 6;
    std::string device = BuildAddressString(provider);
    uint8_t type = provider.ConsumeIntegral<uint8_t>();
    uint8_t vecLen = provider.ConsumeIntegral<uint8_t>();
    std::vector<uint8_t> rep = provider.ConsumeBytes<uint8_t>(vecLen);
    rep.emplace(rep.end(), static_cast<uint8_t>('\0'));
    std::string report(rep.begin(), rep.end());
    g_report = report;

    MessageParcel datas;
    std::u16string descriptor = NearlinkHidHostStub::GetDescriptor();
    datas.WriteInterfaceToken(descriptor);
    datas.WriteString(device);
    datas.WriteUint8(type);
    datas.WriteString(report);
    MessageParcel reply;

    int32_t ret = HidHostOnRemoteRequest(NearlinkHidHostInterfaceCode::NL_SET_REPORT, datas, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void HidHostFuzzTest(const uint8_t *fuzzData, size_t size)
{
    HILOGI("start");
    FuzzedDataProvider provider(fuzzData, size);
    uint32_t code = (provider.ConsumeIntegral<uint32_t>() % MESSAGE_SIZE);
    const int requiredMinDataLen = 11;
    const uint32_t addressLen = 6;
    std::string device = BuildAddressString(provider);
    uint8_t type = provider.ConsumeIntegral<uint8_t>();
    uint8_t vecLen = provider.ConsumeIntegral<uint8_t>();
    std::vector<uint8_t> rep = provider.ConsumeBytes<uint8_t>(vecLen);
    rep.emplace(rep.end(), static_cast<uint8_t>('\0'));
    std::string report(rep.begin(), rep.end());

    MessageParcel datas;
    std::u16string descriptor = NearlinkHidHostStub::GetDescriptor();
    datas.WriteInterfaceToken(descriptor);
    datas.WriteString(device);
    datas.WriteUint8(type);
    datas.WriteString(report);
    MessageParcel reply;

    int32_t ret = HidHostOnRemoteRequest(code, datas, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void DoSomethingInterestingWithHidHostUhidAPI(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    const int descLength = 10;

    auto addr = BuildAddressString(provider);
    RawAddress rawAddr(addr.c_str());
    std::shared_ptr<HidHostUhid> hidUhidPtr = std::make_shared<HidHostUhid>(addr);
    hidUhidPtr->Open();
    HidInformation hidInfo;
    hidInfo.attrMask = provider.ConsumeIntegral<uint16_t>();
    hidInfo.supTimeout = provider.ConsumeIntegral<uint16_t>();
    hidInfo.ssrMaxLatency = provider.ConsumeIntegral<uint16_t>();
    hidInfo.ssrMinTout = provider.ConsumeIntegral<uint16_t>();
    hidInfo.relNum = provider.ConsumeIntegral<uint16_t>();
    hidInfo.ctryCode = provider.ConsumeIntegral<uint8_t>();
    hidInfo.subClass = provider.ConsumeIntegral<uint8_t>();
    hidInfo.hparsVer = provider.ConsumeIntegral<uint16_t>();

    hidInfo.serviceName = BuildAddressString(provider);
    hidInfo.serviceDescription = BuildAddressString(provider);
    hidInfo.providerName = BuildAddressString(provider);
    hidInfo.descLength = provider.ConsumeIntegral<uint16_t>() % descLength;
    hidInfo.descInfo = std::make_unique<uint8_t[]>(hidInfo.descLength);

    for (int i = 0; i < hidInfo.descLength; i++) {
        hidInfo.descInfo[i] = provider.ConsumeIntegral<uint8_t>();
    }

    hidUhidPtr->SendHidInfo(hidInfo);
    uint16_t dataLen = provider.ConsumeIntegral<uint16_t>() % SEND_DATA_LEN;
    std::vector<uint8_t> dataVec = provider.ConsumeBytes<uint8_t>(dataLen);
    uint8_t *ptr = dataVec.data();
    hidUhidPtr->SendData(ptr, dataVec.size());

    uint16_t controlData = provider.ConsumeIntegral<uint16_t>();
    dataLen = provider.ConsumeIntegral<uint16_t>() % SEND_DATA_LEN;
    dataVec = provider.ConsumeBytes<uint8_t>(dataLen);
    ptr = dataVec.data();
    hidUhidPtr->SendControlData(ptr, dataVec.size());
    hidUhidPtr->SendHandshake(provider.ConsumeIntegral<uint16_t>());
    hidUhidPtr->Close();
    hidUhidPtr->Destroy();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void DoSomethingInterestingWithHidHostServiceAPI(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    const int statusSize = 4;

    auto addr = BuildAddressString(provider);
    RawAddress rawAddr(addr.c_str());
    HidHostObserver hidHostCallback_;

    NL_CHECK_RETURN(hidHostService, "hidHostService is nullptr");
    hidHostService->GetService();
    hidHostService->GetContext();
    hidHostService->Enable();
    hidHostService->Connect(rawAddr);
    hidHostService->GetConnectDevices();
    hidHostService->GetConnectState();
    hidHostService->GetDeviceState(rawAddr);

    uint8_t vecLen = provider.ConsumeIntegral<uint8_t>();
    std::vector<int> states;
    for (int i = 0; i < vecLen; i++) {
        states.push_back(provider.ConsumeIntegral<int>());
    }
    hidHostService->GetDevicesByStates(states);
    hidHostService->RegisterObserver(hidHostCallback_);
    hidHostService->ConnectHidInterface(rawAddr);

    uint8_t id = provider.ConsumeIntegral<uint8_t>();
    uint8_t type = provider.ConsumeIntegral<uint8_t>();
    uint16_t hidSize = static_cast<uint16_t>(g_report.length());
    int infoType = provider.ConsumeIntegral<int>();

    hidHostService->HidHostVCUnplug(addr, id, hidSize, type);
    hidHostService->HidHostSendReport(addr, type, hidSize,
        static_cast<const uint8_t*>(static_cast<const void*>(g_report.c_str())));
    hidHostService->HidHostSendReport(addr, type, hidSize, g_report);
    hidHostService->HidHostSetReport(addr, type, hidSize,
        static_cast<const uint8_t*>(static_cast<const void*>(g_report.c_str())));
    hidHostService->HidHostGetReport(addr, id, hidSize, type);
    hidHostService->GetHidDeviceInfo(rawAddr, infoType);

    hidHostService->DeregisterObserver(hidHostCallback_);
    hidHostService->Disconnect(rawAddr);
    hidHostService->Disable();

    bool isConnect = provider.ConsumeBool();
    hidHostService->ShutDownDone(isConnect);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}
}

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
    HILOGI("HidHostFuzzTest OnStart");
    hostServer->OnStart();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_100_MS));
    HILOGI("HidHostFuzzTest EnableSle");
    hostServer->EnableSle();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_5000_MS));
    HILOGI("HidHostFuzzTest endEnableSle");
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
    OHOS::HidHostSetReportFuzzTest(data, size);
    OHOS::HidHostFuzzTest(data, size);
    OHOS::DoSomethingInterestingWithHidHostServiceAPI(data, size);
    OHOS::DoSomethingInterestingWithHidHostUhidAPI(data, size);
    OHOS::g_threadUtil.ClearThreadStateMap();
    return 0;
}