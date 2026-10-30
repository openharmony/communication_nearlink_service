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

#include <memory>
#include <string>
#include <thread>
#include <vector>
#include "slecontroller_fuzzer.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "nearlink_native_token_mock.h"
#include "../nl_utils/fuzztest_utils.h"
#include "nearlink_sle_controller_stub.h"
#include "nearlink_sle_controller_server.h"
#include "nearlink_host_server.h"
#include "ThreadUtil.h"
#include "log.h"
#include "securec.h"
#include "raw_address.h"
#include "SleAdapter.h"
#include "SleCoexist.h"
#include "SleControllerService.h"
#include "SleProfileConnectManager.h"
#include "SleSecurity.h"
#include "SleInterfaceManager.h"

using namespace std;
using namespace OHOS::Nearlink;

namespace OHOS {
namespace {
constexpr int SLE_CONTROLLER_FUZZ_DELAY_50_MS = 50;
constexpr int SLE_CONTROLLER_FUZZ_DELAY_100_MS = 100;
constexpr int SLE_CONTROLLER_FUZZ_DELAY_5000_MS = 5000;
constexpr int SLE_CONTROLLER_FUZZ_ADDR_LEN = 6;
constexpr int SLE_CONTROLLER_FUZZ_INTERVAL_TYPE_NUM = 10;    // ConnectionInterval: 0~9
constexpr int SLE_CONTROLLER_FUZZ_COEX_DEVICE_NUM = 9;
constexpr int SLE_CONTROLLER_FUZZ_NAME_MAX_LEN = 32;
constexpr int SLE_CONTROLLER_FUZZ_PASSCODE_MOD = 1000000;
sptr<NearlinkSleControllerServer> g_sleController = new NearlinkSleControllerServer();
ThreadUtil &g_threadUtil = ThreadUtil::GetInstance();
// counters for SleProfileConnectManagerFucs callbacks
int g_pcmNotifyConnCount = 0;
int g_pcmDisconnectAcbCount = 0;
int g_pcmSendImgSecuCount = 0;
int g_pcmAllProfileDiscCount = 0;
bool g_pcmInited = false;
/* 永不析构：析构不回置全局 g_sleProfileConnectManageImpl_，且退出窗静态析构次序不可控 */
SleProfileConnectManager &g_profileConnectManager = *new SleProfileConnectManager();
}

int32_t SleControllerOnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply)
{
    NL_CHECK_RETURN_RET(g_sleController, TRANSACTION_ERR, "g_sleController is nullptr");
    MessageOption option = { MessageOption::TF_SYNC };
    HILOGI("g_sleController OnRemoteRequest, cmd(%{public}d)", code);
    return g_sleController->OnRemoteRequest(code, data, reply, option);
}

void SetSleCoexParamFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSleControllerStub::GetDescriptor());
    uint16_t maxBitRate = provider.ConsumeIntegral<uint16_t>();
    uint8_t dutyCycle = provider.ConsumeIntegral<uint8_t>();
    data.WriteUint16(maxBitRate);
    data.WriteUint8(dutyCycle);
    int32_t ret = SleControllerOnRemoteRequest(
        NearlinkSleControllerInterfaceCode::NL_SET_SLE_COEX_PARAM, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(SLE_CONTROLLER_FUZZ_DELAY_50_MS));
}

void UpdateConnectIntervalFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSleControllerStub::GetDescriptor());
    std::string address = BuildAddressString(provider);
    int32_t intervalType = provider.ConsumeIntegral<int32_t>();
    data.WriteString(address);
    data.WriteInt32(intervalType);

    int32_t ret = SleControllerOnRemoteRequest(
        NearlinkSleControllerInterfaceCode::NL_SLE_UPDATE_INTERVAL, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(SLE_CONTROLLER_FUZZ_DELAY_50_MS));
}

void SetSleCoexModeFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;
    constexpr int32_t MAX_FUZZ_DEVICE_NUM = 5;
 
    data.WriteInterfaceToken(NearlinkSleControllerStub::GetDescriptor());
    int32_t mode = provider.ConsumeIntegral<int32_t>();
    int32_t deviceSize = provider.ConsumeIntegralInRange<int32_t>(0, MAX_FUZZ_DEVICE_NUM);
    data.WriteInt32(mode);
    data.WriteInt32(deviceSize);
    for (int32_t i = 0; i < deviceSize; i++) {
        std::string device = BuildAddressString(provider);
        int32_t param = provider.ConsumeIntegral<int32_t>();
        data.WriteString(device);
        data.WriteInt32(param);
    }
    int32_t ret = SleControllerOnRemoteRequest(
        NearlinkSleControllerInterfaceCode::NL_SET_SLE_COEX_MODE, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(SLE_CONTROLLER_FUZZ_DELAY_50_MS));
}

// build SLE_Addr_S (uint8_t type + uint8_t addr[6]) from fuzz data
SLE_Addr_S BuildSleAddr(FuzzedDataProvider &provider)
{
    SLE_Addr_S addr = {};
    addr.type = provider.ConsumeIntegral<uint8_t>();
    for (int i = 0; i < SLE_CONTROLLER_FUZZ_ADDR_LEN; i++) {
        addr.addr[i] = provider.ConsumeIntegral<uint8_t>();
    }
    return addr;
}

// fetch the enabled SLE adapter created by LLVMFuzzerInitialize (OnStart + EnableSle)
SleInterfaceAdapterSub *GetSleAdapterSub()
{
    SleInterfaceManager *manager = SleInterfaceManager::GetInstance();
    NL_CHECK_RETURN_RET(manager != nullptr, nullptr, "SleInterfaceManager is nullptr");
    return static_cast<SleInterfaceAdapterSub *>(manager->GetAdapter(SleTransport::ADAPTER_SLE));
}

// A. SleControllerService direct calls
void SleControllerServiceFuzzTest(FuzzedDataProvider &provider)
{
    SleControllerService &controller = SleControllerService::GetInstance();

    uint16_t maxBitRate = provider.ConsumeIntegral<uint16_t>();
    uint8_t dutyCycle = provider.ConsumeIntegral<uint8_t>();
    controller.SetSleCoexParam(maxBitRate, dutyCycle);

    std::string intervalDevice = BuildAddressString(provider);
    int32_t intervalType = provider.ConsumeIntegral<int32_t>();
    controller.UpdateConnectInterval(intervalDevice, intervalType);

    int32_t mode = provider.ConsumeIntegral<uint8_t>() % SLE_COEX_MODE_BUTT;
    int32_t deviceSize = provider.ConsumeIntegral<uint8_t>() % SLE_CONTROLLER_FUZZ_COEX_DEVICE_NUM;
    std::vector<std::string> deviceList;
    std::vector<ConnectionInterval> paramList;
    for (int32_t i = 0; i < deviceSize; i++) {
        std::string device = BuildAddressString(provider);
        ConnectionInterval interval =
            static_cast<ConnectionInterval>(provider.ConsumeIntegral<uint8_t>() %
            SLE_CONTROLLER_FUZZ_INTERVAL_TYPE_NUM);
        deviceList.push_back(device);
        paramList.push_back(interval);
    }
    controller.SetSleCoexMode(mode, deviceList, paramList);

    std::string coexDevice = BuildAddressString(provider);
    uint16_t currentInterval = provider.ConsumeIntegral<uint16_t>();
    uint16_t coexInterval = 0;
    controller.GetSleHidCoexInterval(coexDevice, currentInterval, coexInterval);

    std::string pendingDevice = BuildAddressString(provider);
    uint16_t pendingInterval = provider.ConsumeIntegral<uint16_t>();
    controller.UpdateSleHidCoexModePendingInterval(pendingDevice, pendingInterval);

    std::this_thread::sleep_for(std::chrono::milliseconds(SLE_CONTROLLER_FUZZ_DELAY_50_MS));
}

// B. SleCoexist direct calls (kept alive by shared_ptr, Init() needs shared_from_this)
void SleCoexistFuzzTest(FuzzedDataProvider &provider)
{
    std::shared_ptr<SleCoexist> sleCoexist = std::make_shared<SleCoexist>();
    NL_CHECK_RETURN(sleCoexist != nullptr, "sleCoexist is nullptr");
    sleCoexist->Init();

    CM_ConnectUpdateParamRsp_S updateRsp = {};
    updateRsp.lcid = provider.ConsumeIntegral<uint8_t>();
    updateRsp.extension.interval = provider.ConsumeIntegral<uint16_t>();
    updateRsp.extension.latency = provider.ConsumeIntegral<uint16_t>();
    updateRsp.extension.supervisionTimeout = provider.ConsumeIntegral<uint16_t>();
    updateRsp.addr = BuildSleAddr(provider);
    sleCoexist->ConnectionParamChanged(updateRsp);

    uint16_t lcid = provider.ConsumeIntegral<uint8_t>();
    SLE_Addr_S statusAddr = BuildSleAddr(provider);
    sleCoexist->ConnectionStatusChanged(lcid, statusAddr);

    std::this_thread::sleep_for(std::chrono::milliseconds(SLE_CONTROLLER_FUZZ_DELAY_50_MS));
}

// C. SleSecurity direct calls (StartPair is static; instance built on the enabled adapter)
void SleSecurityFuzzTest(FuzzedDataProvider &provider)
{
    SleInterfaceAdapterSub *adapterSub = GetSleAdapterSub();
    NL_CHECK_RETURN(adapterSub != nullptr, "sleAdapter is nullptr, skip SleSecurityFuzzTest");

    RawAddress pairDevice = BuildRawAddress(provider);
    uint8_t pairAddrType = provider.ConsumeIntegral<uint8_t>();
    SleSecurity::StartPair(pairDevice, pairAddrType);

    /* static 实例永不析构：栈实例 dtor 会置空 g_sleSecurityImpl 断掉 SM 事件链，
     * 且静态回调按值快照该指针异步投递，析构后响应悬垂 */
    static SleSecurity sleSecurity(*adapterSub);

    uint16_t connHandle = provider.ConsumeIntegral<uint16_t>();
    SLE_Addr_S secAddr = BuildSleAddr(provider);
    uint8_t role = provider.ConsumeIntegral<uint8_t>();
    sleSecurity.GapSleRequestSecurity(connHandle, secAddr, role);

    RawAddress cancelDevice = BuildRawAddress(provider);
    uint8_t cancelAddrType = provider.ConsumeIntegral<uint8_t>();
    sleSecurity.CancelPairing(cancelDevice, cancelAddrType);

    RawAddress passCodeDevice = BuildRawAddress(provider);
    uint8_t passCodeAddrType = provider.ConsumeIntegral<uint8_t>();
    std::string passCode = std::to_string(provider.ConsumeIntegral<uint32_t>() %
        SLE_CONTROLLER_FUZZ_PASSCODE_MOD);
    sleSecurity.SetPairingPassCode(passCodeDevice, passCode, passCodeAddrType);

    RawAddress confirmDevice = BuildRawAddress(provider);
    uint8_t confirmAddrType = provider.ConsumeIntegral<uint8_t>();
    sleSecurity.SetPairingConfirmation(confirmDevice, confirmAddrType);

    RawAddress replyDevice = BuildRawAddress(provider);
    uint8_t replyAddrType = provider.ConsumeIntegral<uint8_t>();
    bool accept = provider.ConsumeBool();
    sleSecurity.PairRequestReply(replyDevice, replyAddrType, accept);

    sleSecurity.RegisterCallbackToSm();
    sleSecurity.RegisterCryptoFunctionsToSm();
    sleSecurity.DeregisterCallbackToGap();

    RawAddress imgDevice = BuildRawAddress(provider);
    uint32_t groupId = provider.ConsumeIntegral<uint32_t>();
    sleSecurity.SendImgSecuConfig(imgDevice, groupId);

    RawAddress cfgDevice = BuildRawAddress(provider);
    std::string encryptGroupKeyStr = "";
    uint64_t giv = 0;
    NLSTK_SmImgSecuConfig_S imgConfig = {};
    sleSecurity.GetCdsmImgSecuConfig(cfgDevice, encryptGroupKeyStr, giv, imgConfig);

    std::this_thread::sleep_for(std::chrono::milliseconds(SLE_CONTROLLER_FUZZ_DELAY_50_MS));
}

// D. SleAdapter direct calls (safe subset: getters / conn frame type / pair family / notify family)
void SleAdapterFuzzTest(FuzzedDataProvider &provider)
{
    SleInterfaceAdapterSub *adapterSub = GetSleAdapterSub();
    NL_CHECK_RETURN(adapterSub != nullptr, "sleAdapter is nullptr, skip SleAdapterFuzzTest");
    SleAdapter *sleAdapter = static_cast<SleAdapter *>(adapterSub);

    (void)sleAdapter->GetLocalAddress();
    (void)sleAdapter->GetLocalName();
    std::string localName = provider.ConsumeRandomLengthString(SLE_CONTROLLER_FUZZ_NAME_MAX_LEN);
    sleAdapter->SetLocalName(localName);

    (void)sleAdapter->GetIoCapability();
    int ioCapability = provider.ConsumeIntegral<uint8_t>();
    sleAdapter->SetIoCapability(ioCapability);

    (void)sleAdapter->GetBondableMode();
    int bondableMode = provider.ConsumeIntegral<uint8_t>();
    sleAdapter->SetBondableMode(bondableMode);

    (void)sleAdapter->IsSleEnabled();
    (void)sleAdapter->GetAcbCount();
    (void)sleAdapter->GetLocalSleAddress();
    (void)sleAdapter->IsLlPrivacySupported();

    RawAddress acbDevice = BuildRawAddress(provider);
    (void)sleAdapter->GetAcbState(acbDevice);

    RawAddress rssiDevice = BuildRawAddress(provider);
    (void)sleAdapter->ReadRemoteRssiValue(rssiDevice);

    std::string frameAddr = BuildAddressString(provider);
    uint8_t frameType = provider.ConsumeIntegral<uint8_t>();
    sleAdapter->SetConnFrameType(frameAddr, frameType);
    uint8_t curFrameType = 0;
    (void)sleAdapter->GetConnFrameType(frameAddr, curFrameType);
    sleAdapter->DelConnFrameType(frameAddr);

    RawAddress connDevice = BuildRawAddress(provider);
    uint16_t timeout = 0;
    uint16_t maxLatency = 0;
    uint16_t interval = 0;
    (void)sleAdapter->GetConnectionParam(connDevice.GetAddress(), timeout, maxLatency);
    (void)sleAdapter->GetConnectionParam(connDevice.GetAddress(), timeout, maxLatency, interval);

    RawAddress startDevice = BuildRawAddress(provider);
    (void)sleAdapter->StartPair(startDevice);

    RawAddress cancelDevice = BuildRawAddress(provider);
    (void)sleAdapter->CancelPairing(cancelDevice);

    RawAddress removeDevice = BuildRawAddress(provider);
    (void)sleAdapter->RemovePair(removeDevice);
    (void)sleAdapter->RemoveAllPairs();

    RawAddress passCodeDevice = BuildRawAddress(provider);
    std::string passCode = std::to_string(provider.ConsumeIntegral<uint32_t>() %
        SLE_CONTROLLER_FUZZ_PASSCODE_MOD);
    (void)sleAdapter->SetPairingPassCode(passCodeDevice, passCode);

    RawAddress confirmDevice = BuildRawAddress(provider);
    (void)sleAdapter->SetPairingConfirmation(confirmDevice);

    RawAddress replyDevice = BuildRawAddress(provider);
    bool accept = provider.ConsumeBool();
    (void)sleAdapter->PairRequestReply(replyDevice, accept);

    RawAddress pairStateDevice = BuildRawAddress(provider);
    (void)sleAdapter->GetPairState(pairStateDevice);

    RawAddress bondedDevice = BuildRawAddress(provider);
    (void)sleAdapter->IsBondedFromLocal(bondedDevice);

    RawAddress notifyDevice = BuildRawAddress(provider);
    int preStatus = provider.ConsumeIntegral<int32_t>();
    int status = provider.ConsumeIntegral<int32_t>();
    int reason = provider.ConsumeIntegral<int32_t>();
    sleAdapter->NotifyPairStatusChanged(notifyDevice, preStatus, status, reason);

    RawAddress connStateDevice = BuildRawAddress(provider);
    SleConnectionChangedParam connParam = {};
    connParam.connState = provider.ConsumeIntegral<int32_t>();
    connParam.connPreState = provider.ConsumeIntegral<int32_t>();
    connParam.connReason = provider.ConsumeIntegral<int32_t>();
    sleAdapter->NotifyConnectionStateChanged(connStateDevice, connParam);

    std::this_thread::sleep_for(std::chrono::milliseconds(SLE_CONTROLLER_FUZZ_DELAY_50_MS));
}

// E. SleProfileConnectManager direct calls (static instance: dtor would leave a dangling global)
void SleProfileConnectManagerFuzzTest(FuzzedDataProvider &provider)
{
    if (!g_pcmInited) {
        SleProfileConnectManagerFucs funcs = {};
        funcs.notifyConnectionStateChanged =
            [](const RawAddress &device, const SleConnectionChangedParam &param) {
                g_pcmNotifyConnCount++;
                HILOGI("notifyConnectionStateChanged, state(%{public}d)", param.connState);
            };
        funcs.disconnectAcb = [](const RawAddress &device, uint8_t discReason) {
            g_pcmDisconnectAcbCount++;
            HILOGI("disconnectAcb, reason(%{public}u)", discReason);
        };
        funcs.sendImgSecuConfig = [](const RawAddress &device) {
            g_pcmSendImgSecuCount++;
            HILOGI("sendImgSecuConfig, count(%{public}d)", g_pcmSendImgSecuCount);
        };
        funcs.onAllProfileDisconnected = [](const RawAddress &device) {
            g_pcmAllProfileDiscCount++;
            HILOGI("onAllProfileDisconnected, count(%{public}d)", g_pcmAllProfileDiscCount);
        };
        g_profileConnectManager.Init(funcs);
        g_pcmInited = true;
    }

    RawAddress device = BuildRawAddress(provider);
    g_profileConnectManager.NotifyConnectAcb(device);
    g_profileConnectManager.SleConnectAllProfile(device);
    (void)g_profileConnectManager.GetProfileConnectState(device);
    (void)g_profileConnectManager.GetConnectedProfileNum(device);
    (void)g_profileConnectManager.IsAllProfileConnected(device);
    (void)g_profileConnectManager.GetProfileConnState(device);
    (void)g_profileConnectManager.GetSilentPortState(device);
    bool isSilentPort = provider.ConsumeBool();
    g_profileConnectManager.SetSilentPortState(device, isSilentPort);
    g_profileConnectManager.StopTimerIfRePair(device);
    g_profileConnectManager.NotifyAcbDisconnected(device);
    g_profileConnectManager.SleDisconnectAllProfileForSilentPort(device);
    g_profileConnectManager.ClearProfileConnectInfo(device);
    g_profileConnectManager.ClearAllProfileConnectInfo();

    std::this_thread::sleep_for(std::chrono::milliseconds(SLE_CONTROLLER_FUZZ_DELAY_50_MS));
}
} // namespace

// fuzzer init
extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc;
    (void)argv;
    // mock token
    NearlinkMockNativeToken mock("nearlink_service");

    OHOS::sptr<NearlinkHostServer> hostServer = NearlinkHostServer::GetInstance();
    HILOGI("slecontroller_fuzzer OnStart");
    hostServer->OnStart();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::SLE_CONTROLLER_FUZZ_DELAY_100_MS));
    HILOGI("slecontroller_fuzzer EnableSle");
    hostServer->EnableSle();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::SLE_CONTROLLER_FUZZ_DELAY_5000_MS));
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (data == nullptr) {
        OHOS::g_threadUtil.ClearThreadStateMap();
        return 0;
    }

    OHOS::g_threadUtil.InitThreadStateMap();
    OHOS::SetSleCoexParamFuzzTest(data, size);
    OHOS::UpdateConnectIntervalFuzzTest(data, size);
    OHOS::SetSleCoexModeFuzzTest(data, size);
    {
        FuzzedDataProvider fdp(data, size);
        OHOS::SleControllerServiceFuzzTest(fdp);
    }
    {
        FuzzedDataProvider fdp(data, size);
        OHOS::SleCoexistFuzzTest(fdp);
    }
    {
        FuzzedDataProvider fdp(data, size);
        OHOS::SleSecurityFuzzTest(fdp);
    }
    {
        FuzzedDataProvider fdp(data, size);
        OHOS::SleAdapterFuzzTest(fdp);
    }
    {
        FuzzedDataProvider fdp(data, size);
        OHOS::SleProfileConnectManagerFuzzTest(fdp);
    }
    OHOS::g_threadUtil.ClearThreadStateMap();
    return 0;
}
