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
#include "SleAdapterWrapper.h"
#include "SleCoexist.h"
#include "SleControllerService.h"
#include "SleProfileConnectManager.h"
#include "SleReconnectManager.h"
#include "SleRemoteDeviceAdapter.h"
#include "SleSecurity.h"
#include "SleInterfaceManager.h"
#include "nearlink_def.h"
#include "nearlink_device_model.h"
#include "nearlink_safe_list.h"
#include "sle_service_data.h"

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
constexpr int SLE_CONTROLLER_FUZZ_CONN_STATE_NUM = 5;       // SleConnState: DISCONNECTED~ENCRYPTED
constexpr int SLE_CONTROLLER_FUZZ_CONN_DIRECT_NUM = 2;      // SleConnDirect: PASSIVE/ACTIVE
constexpr int SLE_CONTROLLER_FUZZ_ADDR_TYPE_NUM = 8;        // SLE_ADDR_TYPE: PUBLIC~ADDR_TYPE_END(边界)
constexpr int SLE_CONTROLLER_FUZZ_ROLE_NUM = 2;             // primary/secondary
constexpr int SLE_CONTROLLER_FUZZ_DISC_REASON_NUM = 3;      // SleDiscReason 成员数（稀疏值，走静态表）
constexpr int SLE_CONTROLLER_FUZZ_PAIR_STATE_NUM = 4;       // SlePairState 成员数（稀疏值，走静态表）
constexpr int SLE_CONTROLLER_FUZZ_APPEARANCE_TABLE_SIZE = 10;
constexpr int SLE_CONTROLLER_FUZZ_RECONN_BG_MAX_NUM = 20;   // 覆盖 16(默认上限)/0/1 边界
constexpr int SLE_CONTROLLER_FUZZ_RECONN_LIST_NUM = 4;
constexpr int SLE_CONTROLLER_FUZZ_HID_REPORT_TYPE_NUM = 4;  // RESERVED/INPUT/OUTPUT/FEATURE
// 稀疏枚举值静态表（PR7 R7 先例）：断链原因/配对状态/外观
const uint8_t SLE_CONTROLLER_FUZZ_DISC_REASON_TABLE[SLE_CONTROLLER_FUZZ_DISC_REASON_NUM] = {
    static_cast<uint8_t>(SleDiscReason::SLE_DISC_REASON_COMMAND_TIMEOUT),
    static_cast<uint8_t>(SleDiscReason::SLE_DISC_REASON_REMOTE_USER_TERMINATED),
    static_cast<uint8_t>(SleDiscReason::SLE_DISC_REASON_CANCEL_PAIR),
};
const int SLE_CONTROLLER_FUZZ_PAIR_STATE_TABLE[SLE_CONTROLLER_FUZZ_PAIR_STATE_NUM] = {
    static_cast<int>(SlePairState::SLE_PAIR_NONE),
    static_cast<int>(SlePairState::SLE_PAIR_PAIRING),
    static_cast<int>(SlePairState::SLE_PAIR_PAIRED),
    static_cast<int>(SlePairState::SLE_PAIR_CANCELING),
};
const int SLE_CONTROLLER_FUZZ_APPEARANCE_TABLE[SLE_CONTROLLER_FUZZ_APPEARANCE_TABLE_SIZE] = {
    INVALID_APPEARANCE,
    0,
    static_cast<int>(DeviceClassForService::DEVICE_SMART_WATCH),
    static_cast<int>(DeviceClassForService::DEVICE_KEYBOARD),
    static_cast<int>(DeviceClassForService::DEVICE_MOUSE),
    static_cast<int>(DeviceClassForService::DEVICE_HANDLE),
    static_cast<int>(DeviceClassForService::DEVICE_STYLUS),
    static_cast<int>(DeviceClassForService::DEVICE_IN_EAR_EARPHONE),
    static_cast<int>(DeviceClassForService::DEVICE_CLASS_VEHICLE_LOCK),
    static_cast<int>(DeviceClassForService::DEVICE_NETWORKING),
};
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

    /* ---- 以下为 PR-8 追加段（仅尾部追加消费，旧 corpus 读序不变）----
     * FetchInterval/GetConnectionParams/EnableSleHidCoexMode/DisableSleHidCoexMode/
     * UpdateSleHidCoexIntervalForEach/StartSleHidCoexUpdateTimer 均为 private，
     * 经公有入口 UpdateConnectInterval / SetSleCoexMode 驱动。 */

    /* FetchInterval：intervalType 全域单次 FDP（随机性整改：合法 10 型+非法型统一全域，
     * 合法型由引擎/corpus 命中；GetConnectionParams 在命中后同链触达） */
    std::string fetchDevice = BuildAddressString(provider);
    int32_t intervalTypeFetch = provider.ConsumeIntegral<int32_t>();
    controller.UpdateConnectInterval(fetchDevice, intervalTypeFetch);

    /* Enable 成功链：非空 deviceList + 等长合法 paramList → EnableSleHidCoexMode →
     * UpdateSleHidCoexIntervalForEach(0)。裸进程无连接记录，SleCoexistManager 查不到
     * 地址 → currentInterval 恒 0 → updateInterval=0 → StartSleHidCoexUpdateTimer 以
     * protectTime=0 立即递归（不创建 NearlinkTimer），index 到尾后 STARTING→STARTED */
    int32_t enableSize = provider.ConsumeIntegral<uint8_t>() % SLE_CONTROLLER_FUZZ_COEX_DEVICE_NUM + 1;
    std::vector<std::string> enableDeviceList;
    std::vector<ConnectionInterval> enableParamList;
    for (int32_t i = 0; i < enableSize; i++) {
        enableDeviceList.push_back(BuildAddressString(provider));
        enableParamList.push_back(static_cast<ConnectionInterval>(
            provider.ConsumeIntegral<uint8_t>() % SLE_CONTROLLER_FUZZ_INTERVAL_TYPE_NUM));
    }
    controller.SetSleCoexMode(SLE_HID_COEX_MODE_ENABLE, enableDeviceList, enableParamList);

    /* state=STARTED 后的深分支：复用 Enable 链登记的同一地址，addr 命中 deviceList */
    uint16_t deepCurrentInterval = provider.ConsumeIntegral<uint16_t>();
    uint16_t deepCoexInterval = 0;
    controller.GetSleHidCoexInterval(enableDeviceList.front(), deepCurrentInterval, deepCoexInterval);
    controller.UpdateSleHidCoexModePendingInterval(enableDeviceList.front(), deepCurrentInterval);

    /* Disable 链：STOPPING → UpdateSleHidCoexIntervalForEach → STOPPED（清空 deviceList） */
    controller.SetSleCoexMode(SLE_HID_COEX_MODE_DISABLE, {}, {});

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

// F. SleRemoteDeviceAdapter direct calls (GetInstance singleton: take only, never destruct;
//    most methods go DoInDeviceAdapterThread + future.get(), must stay in InitThreadStateMap window)
void SleRemoteDeviceAdapterFuzzTest(FuzzedDataProvider &provider)
{
    SleRemoteDeviceAdapter *deviceAdapter = SleRemoteDeviceAdapter::GetInstance();
    NL_CHECK_RETURN(deviceAdapter != nullptr, "deviceAdapter is nullptr");

    /* 连接态组合矩阵：建档 -> ACL/业务状态置位 -> 连接完成 -> 查询族 -> 断链顺序 */
    RawAddress connDevice = BuildRawAddress(provider);
    SleConnState connState = static_cast<SleConnState>(
        provider.ConsumeIntegral<uint8_t>() % SLE_CONTROLLER_FUZZ_CONN_STATE_NUM);
    uint16_t lcid = provider.ConsumeIntegral<uint16_t>();
    uint8_t role = provider.ConsumeIntegral<uint8_t>() % SLE_CONTROLLER_FUZZ_ROLE_NUM;
    uint8_t addrType = static_cast<uint8_t>(static_cast<SLE_ADDR_TYPE>(
        provider.ConsumeIntegral<uint8_t>() % SLE_CONTROLLER_FUZZ_ADDR_TYPE_NUM));
    uint8_t discReason = SLE_CONTROLLER_FUZZ_DISC_REASON_TABLE[
        provider.ConsumeIntegral<uint8_t>() % SLE_CONTROLLER_FUZZ_DISC_REASON_NUM];
    int pairStatus = SLE_CONTROLLER_FUZZ_PAIR_STATE_TABLE[
        provider.ConsumeIntegral<uint8_t>() % SLE_CONTROLLER_FUZZ_PAIR_STATE_NUM];

    deviceAdapter->SleAddPeerList(connDevice);
    deviceAdapter->SetAcbState(connDevice.GetAddress(), static_cast<int>(connState));
    deviceAdapter->SetLcid(connDevice.GetAddress(), lcid);
    deviceAdapter->SetPairStatus(connDevice, pairStatus);
    deviceAdapter->SetPrePairStatus(connDevice, pairStatus);
    deviceAdapter->ConnectionCompleteHelper(connDevice, lcid, role, addrType);
    (void)deviceAdapter->IsAcbConnected(connDevice);
    (void)deviceAdapter->IsAcbEncrypted(connDevice);
    (void)deviceAdapter->GetAcbState(connDevice.GetAddress());
    (void)deviceAdapter->GetLcidByAddress(connDevice);
    (void)deviceAdapter->GetAddressByLcid(lcid);
    (void)deviceAdapter->GetPairState(connDevice);
    (void)deviceAdapter->GetPairStatus(connDevice);
    (void)deviceAdapter->GetLinkRole(connDevice);
    (void)deviceAdapter->GetConnectedCnt();
    (void)deviceAdapter->HasConnectedDevice();
    (void)deviceAdapter->GetConnectedDevices();
    (void)deviceAdapter->GetPairedDevices();
    int connDirect = static_cast<int>(
        provider.ConsumeIntegral<uint8_t>() % SLE_CONTROLLER_FUZZ_CONN_DIRECT_NUM);
    deviceAdapter->SavePairDirect(connDirect, connDevice);
    (void)deviceAdapter->GetConnDirect(connDevice);
    deviceAdapter->SetConnDirect(connDevice, connDirect);
    deviceAdapter->SetConnDirectActive(connDevice);
    deviceAdapter->DisconnectAcbAction(connDevice, discReason);
    deviceAdapter->CancelAllConnection();

    /* 查询族 + 名称/外观/配对算法/组密钥错误分支 */
    std::string deviceName = provider.ConsumeRandomLengthString(SLE_CONTROLLER_FUZZ_NAME_MAX_LEN);
    std::string aliasName = provider.ConsumeRandomLengthString(SLE_CONTROLLER_FUZZ_NAME_MAX_LEN);
    (void)deviceAdapter->GetDeviceName(connDevice);
    (void)deviceAdapter->SetName(connDevice, deviceName);
    (void)deviceAdapter->GetAliasName(connDevice);
    (void)deviceAdapter->SetAliasName(connDevice, aliasName);
    int appearance = SLE_CONTROLLER_FUZZ_APPEARANCE_TABLE[
        provider.ConsumeIntegral<uint8_t>() % SLE_CONTROLLER_FUZZ_APPEARANCE_TABLE_SIZE];
    (void)deviceAdapter->SetAppearance(connDevice, appearance);
    (void)deviceAdapter->GetDeviceAppearance(connDevice);
    (void)deviceAdapter->GetDeviceUuids(connDevice);
    (void)deviceAdapter->IsBondedFromLocal(connDevice);
    (void)deviceAdapter->GetManufacturerBusinessType(connDevice);
    (void)deviceAdapter->GetNotPairNoneCnt(connDevice);
    (void)deviceAdapter->GetPairedCnt(connDevice);
    uint8_t cryptoAlgo = provider.ConsumeIntegral<uint8_t>();
    uint8_t keyDerivAlgo = provider.ConsumeIntegral<uint8_t>();
    uint8_t integrChkInd = provider.ConsumeIntegral<uint8_t>();
    (void)deviceAdapter->SetPairAlgoInfo(connDevice, cryptoAlgo, keyDerivAlgo, integrChkInd);
    (void)deviceAdapter->GetPairAlgoInfo(connDevice, cryptoAlgo, keyDerivAlgo, integrChkInd);
    std::string groupKey = provider.ConsumeRandomLengthString(SLE_CONTROLLER_FUZZ_NAME_MAX_LEN);
    uint64_t giv = provider.ConsumeIntegral<uint64_t>();
    (void)deviceAdapter->SetGroupAndGiv(connDevice, groupKey, giv);
    (void)deviceAdapter->GetGroupAndGiv(connDevice, groupKey, giv);
    bool isAvailable = provider.ConsumeBool();
    deviceAdapter->SetDeviceIsAvailable(connDevice, isAvailable);
    int cdsmAddrType = static_cast<int>(static_cast<SLE_ADDR_TYPE>(
        provider.ConsumeIntegral<uint8_t>() % SLE_CONTROLLER_FUZZ_ADDR_TYPE_NUM));
    (void)deviceAdapter->SetCdsmAddrType(connDevice, cdsmAddrType);
    (void)deviceAdapter->IsServiceSupportedConn(connDevice);
    (void)deviceAdapter->IsAudioDevice(connDevice.GetAddress());
    (void)deviceAdapter->IsVendorDevice(connDevice);
    RawAddress otherDevice;
    (void)deviceAdapter->GetCdsmOtherAddr(connDevice, otherDevice);
    (void)deviceAdapter->GetRealAddress(connDevice);
    (void)deviceAdapter->GetPeerDeviceAddrType(connDevice);
    (void)deviceAdapter->GetRemoteDevice(connDevice);

    /* 地址转换族：SLE 地址 <-> BT 地址互转（random/public 两档由 addrType 矩阵覆盖） */
    std::string sleAddr = BuildAddressString(provider);
    std::string btAddr = BuildAddressString(provider);
    (void)deviceAdapter->SetBtAddrBySleAddr(sleAddr, btAddr);
    std::string btAddrResult = "";
    (void)deviceAdapter->GetBtAddrBySleAddrTask(sleAddr, btAddrResult);
    std::string sleAddrResult = "";
    (void)deviceAdapter->GetSleAddrByBtAddrTask(btAddr, sleAddrResult);

    /* 持久化/合作集/设备类型下发族 */
    DeviceModel deviceModel;
    std::string newModelId = provider.ConsumeRandomLengthString(SLE_CONTROLLER_FUZZ_NAME_MAX_LEN);
    deviceAdapter->SaveDeviceModelInfo(sleAddr, deviceModel, newModelId);
    deviceAdapter->FindDeviceModelInfoInCache(connDevice, deviceModel, newModelId);
    std::array<uint8_t, SLE_MANU_ABILITY_LEN> manuAbility = {};
    for (int i = 0; i < SLE_MANU_ABILITY_LEN; i++) {
        manuAbility[i] = provider.ConsumeIntegral<uint8_t>();
    }
    deviceAdapter->UpdateDeviceManufacturerAbility(connDevice, manuAbility);
    deviceAdapter->SaveHidManufacturerAbilityFromAdv(connDevice);
    uint8_t ability = provider.ConsumeIntegral<uint8_t>();
    (void)deviceAdapter->GetManufacturerAbility(connDevice, ability);
    deviceAdapter->SaveDeviceManufacturerAbility(connDevice);
    (void)deviceAdapter->SavePeerDeviceInfoToConf();
    deviceAdapter->SavePeerDevices2Smp();
    (void)deviceAdapter->CollectPairedDevicesForSmp();
    deviceAdapter->RestorePairedDevicesAbility2Cfgdb();
    deviceAdapter->SetAudioDeviceFlag(connDevice);
    deviceAdapter->UpdateDefaultRole(connDevice);
    deviceAdapter->HandleCdsmMemberFirstPairing(connDevice);
    deviceAdapter->CdsmSaveData(connDevice);
    (void)deviceAdapter->IsCdsmMemberPair(connDevice, otherDevice);
    deviceAdapter->CdsmAddOtherRecord(connDevice, otherDevice);
    SLE_Addr_S addrInfo = {};
    uint32_t devType = 0;
    deviceAdapter->GetDeviceTypeInfo(connDevice, addrInfo, devType);
    deviceAdapter->SetPeerDeviceTypeToController(connDevice);
    deviceAdapter->RemovePeerDeviceTypeToController(connDevice);

    /* 背景回连列表族 */
    NearlinkSafeList<RawAddress> bgList;
    bgList.EnsureInsert(connDevice);
    std::vector<std::string> reconnectList;
    int32_t reconnSize = provider.ConsumeIntegral<uint8_t>() % SLE_CONTROLLER_FUZZ_RECONN_LIST_NUM;
    for (int32_t i = 0; i < reconnSize; i++) {
        reconnectList.push_back(BuildAddressString(provider));
    }
    deviceAdapter->GetBgList(bgList, reconnectList);
    deviceAdapter->GetDirectConnList(bgList);
    deviceAdapter->SendBgConnList(bgList);
    deviceAdapter->SendDirectConnList(bgList);
    deviceAdapter->AddBgConnDevice(connDevice.GetAddress());
    deviceAdapter->SetReconnDeviceParam();

    /* 收尾：批量移除配对与清档 */
    std::vector<RawAddress> removeDevices;
    deviceAdapter->RemoveAllPairsProcess(removeDevices);
    deviceAdapter->ClearPeerDeviceGroupId();
    std::shared_ptr<SlePeripheralDevice> peerDevice = std::make_shared<SlePeripheralDevice>();
    deviceAdapter->AddPeripheralDevice(sleAddr, peerDevice);
    deviceAdapter->RemovePeripheralDevice(sleAddr);
    deviceAdapter->RemoveAllPeripheralDevices();

    std::this_thread::sleep_for(std::chrono::milliseconds(SLE_CONTROLLER_FUZZ_DELAY_50_MS));
}

// G. SleReconnectManager direct calls (GetInstance singleton; facade itself has no thread switch,
//    inner AddBgConnDevice/SetPeerDeviceTypeToController go DoInDeviceAdapterThread + future)
void SleReconnectManagerFuzzTest(FuzzedDataProvider &provider)
{
    SleReconnectManager &reconnManager = SleReconnectManager::GetInstance();
    uint32_t bgConnMaxNum = provider.ConsumeIntegral<uint8_t>() % SLE_CONTROLLER_FUZZ_RECONN_BG_MAX_NUM;
    reconnManager.SetBgConnMaxNum(bgConnMaxNum);

    /* 策略切换：NONE 不发布/不落盘，非 NONE 发布事件 + LastASCActive/Connected 两路 */
    RawAddress reconnDevice = BuildRawAddress(provider);
    bool isActive = provider.ConsumeBool();
    int pairState = SLE_CONTROLLER_FUZZ_PAIR_STATE_TABLE[
        provider.ConsumeIntegral<uint8_t>() % SLE_CONTROLLER_FUZZ_PAIR_STATE_NUM];
    std::string btAddr = BuildAddressString(provider);
    reconnManager.SetAutoConnectDevice(reconnDevice, isActive, pairState, btAddr);
    (void)reconnManager.GetReconnDevice();
    reconnManager.SetReconnDeviceParam();
    (void)reconnManager.IsNeedToIgnore(reconnDevice.GetAddress());

    /* 队列生命周期：建档 -> 断连(黑名单标记) -> 连接(可用/不可用两分支) -> 解配对 -> 重连列表 */
    reconnManager.OnDeviceStartPair(reconnDevice);
    bool isNeedBgConn = provider.ConsumeBool();
    reconnManager.OnDeviceDisConnected(reconnDevice, isNeedBgConn);
    bool isDeviceAvailable = provider.ConsumeBool();
    reconnManager.OnDeviceConnected(reconnDevice, isDeviceAvailable);
    reconnManager.OnDeviceUnpaired(reconnDevice);
    (void)reconnManager.GetReconnectList();
    reconnManager.RemoveAutoConnAudioDevConf(reconnDevice);

    std::this_thread::sleep_for(std::chrono::milliseconds(SLE_CONTROLLER_FUZZ_DELAY_50_MS));
}

// H. SleAdapterWrapper direct calls (static instance: ctor/dtor both trivial, kept per R3/R6 lessons)
void SleAdapterWrapperFuzzTest(FuzzedDataProvider &provider)
{
    static SleAdapterWrapper adapterWrapper;
    RawAddress wrapperDevice = BuildRawAddress(provider);

    (void)adapterWrapper.GetConnectedDevices();
    (void)adapterWrapper.GetLcidByAddress(wrapperDevice);
    adapterWrapper.AddBgConnDevice(wrapperDevice.GetAddress());
    (void)adapterWrapper.GetAliasName(wrapperDevice.GetAddress());
    (void)adapterWrapper.GetPeerDeviceAddrType(wrapperDevice);
    SLE_Addr_S addrInfo = {};
    uint32_t devType = 0;
    adapterWrapper.GetDeviceTypeInfo(wrapperDevice, addrInfo, devType);

    HidReportInfo reportInfo = {};
    reportInfo.dev_ = wrapperDevice;
    reportInfo.reportType_ =
        provider.ConsumeIntegral<uint8_t>() % SLE_CONTROLLER_FUZZ_HID_REPORT_TYPE_NUM;
    reportInfo.reportId_ = provider.ConsumeIntegral<uint8_t>();
    (void)adapterWrapper.HidSendData(reportInfo);

    (void)adapterWrapper.IsAudioDevice(wrapperDevice.GetAddress());
    RawAddress otherAddr;
    (void)adapterWrapper.GetCdsmOtherAddr(wrapperDevice, otherAddr);
    NearlinkSafeList<RawAddress> bgList;
    bgList.EnsureInsert(wrapperDevice);
    adapterWrapper.SendBgConnList(bgList);
    adapterWrapper.SendDirectConnList(bgList);
    (void)adapterWrapper.GetDeviceAppearance(wrapperDevice);
    (void)adapterWrapper.DisconnectAllProfile(wrapperDevice);
    std::vector<RawAddress> cdsmInfoAddr;
    (void)adapterWrapper.WrapperCdsmGetAllMemberInfo(wrapperDevice, cdsmInfoAddr);
    std::string devAddress = wrapperDevice.GetAddress();
    (void)adapterWrapper.IsProxyConnectExisted(devAddress);

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
    {
        FuzzedDataProvider fdp(data, size);
        OHOS::SleRemoteDeviceAdapterFuzzTest(fdp);
    }
    {
        FuzzedDataProvider fdp(data, size);
        OHOS::SleReconnectManagerFuzzTest(fdp);
    }
    {
        FuzzedDataProvider fdp(data, size);
        OHOS::SleAdapterWrapperFuzzTest(fdp);
    }
    OHOS::g_threadUtil.ClearThreadStateMap();
    return 0;
}
