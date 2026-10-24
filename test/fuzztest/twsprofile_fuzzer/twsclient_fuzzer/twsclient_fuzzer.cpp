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

#include <array>
#include <memory>
#include <thread>
#include "fuzzer/FuzzedDataProvider.h"
#include "twsclient_fuzzer.h"
#include "nearlink_native_token_mock.h"
#include "../../nl_utils/fuzztest_utils.h"
#include "ThreadUtil.h"
#include "log.h"
#include "nearlink_host_server.h"
#include "nearlink_tws_client_server.h"
#include "nearlink_tws_client_stub.h"
#include "nearlink_raw_address.h"
#include "nearlink_ASC_source.h"
#include "nearlink_asc_audio_stream_info.h"
#include "TwsService.h"

using namespace std;
using namespace OHOS::Nearlink;
namespace OHOS {
class MockNearlinkTwsClientCallbackStub : public IRemoteStub<INearlinkTwsClientObserver> {
public:
    void OnTwsRemoteInfo(const std::string &address, const std::vector<uint8_t> &value) {}
};

class MockTwsObserver : public TwsObserver {
public:
    void OnConnectionStateChanged(const RawAddress &deviceAddress, int state, int oldState) {}
};

/* 文件域 mock 实例：观察者无状态，跨输入复用 */
MockTwsObserver g_twsObserver;

class MockInterfaceTwsClientObserver : public InterfaceTwsClientObserver {
public:
    void OnTwsRemoteInfo(const std::string &address, const std::vector<uint8_t> &value) {}
};

namespace {
    constexpr int HOST_FUZZ_DELAY_50_MS = 50;
    constexpr int HOST_FUZZ_DELAY_100_MS = 100;
    constexpr int HOST_FUZZ_DELAY_5000_MS = 5000;
    sptr<INearlinkTwsClientObserver> g_twsClientCb = new (std::nothrow) MockNearlinkTwsClientCallbackStub();

    /* AudioStreamType 枚举成员数（nearlink_ASC_source.h: NONE..SING 共 14 个） */
    constexpr uint8_t TWS_AUDIO_STREAM_TYPE_NUM = 14;
    /* AudioStreamType 为位标志枚举（成员值=2 的幂），取模映射会产生非成员值，改用成员表索引 */
    const AudioStreamType TWS_AUDIO_STREAM_TYPE_LIST[TWS_AUDIO_STREAM_TYPE_NUM] = {
        AUDIO_STREAM_NONE, AUDIO_STREAM_UNDEFINED, AUDIO_STREAM_MUSIC, AUDIO_STREAM_VOICE_CALL,
        AUDIO_STREAM_VOICE_ASSISTANT, AUDIO_STREAM_RING, AUDIO_STREAM_VOIP, AUDIO_STREAM_GAME,
        AUDIO_STREAM_RECORD, AUDIO_STREAM_ALERT, AUDIO_STREAM_VIDEO, AUDIO_STREAM_GUID,
        AUDIO_STREAM_ALARM, AUDIO_STREAM_SING,
    };
    /* AudioStreamState 枚举成员数（INVALID/AVAILABLE/NOT_AVAILABLE 共 3 个） */
    constexpr uint8_t TWS_AUDIO_STREAM_STATE_NUM = 3;
    /* AudioStreamInfo 列表条数上限 */
    constexpr uint8_t TWS_AUDIO_STREAM_INFO_MAX_NUM = 13;
    /* TwsRoleType 有效值域 0x01/0x02 */
    constexpr uint8_t TWS_ROLE_TYPE_NUM = 2;
    /* TwsClientState 成员数；NotifyStateChanged 内 stateMap_.at() 对越界枚举会抛异常，禁止喂 >=4 的值 */
    constexpr uint8_t TWS_CLIENT_STATE_NUM = 4;
    constexpr uint8_t TWS_EVENT_LIST_NUM = 6;
    constexpr uint8_t TWS_DATA_TYPE_LIST_NUM = 3;
}

int32_t TwsClientOnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply)
{
    sptr<NearlinkTwsClientServer> twsClient = new (std::nothrow) NearlinkTwsClientServer();
    NL_CHECK_RETURN_RET(twsClient, TRANSACTION_ERR, "twsClient is nullptr");
    MessageOption option = {MessageOption::TF_SYNC};
    HILOGI("TwsClientOnRemoteRequest, cmd(%{public}d)", code);
    return twsClient->OnRemoteRequest(code, data, reply, option);
}

void RegisterApplicationFuzzTest(const uint8_t *fuzzData, size_t size)
{
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkTwsClientStub::GetDescriptor());
    if (g_twsClientCb != nullptr) {
        data.WriteRemoteObject(g_twsClientCb->AsObject());
    }

    int32_t ret = TwsClientOnRemoteRequest(NearlinkTwsClientInterfaceCode::NL_TWS_REGISTER_APP, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void DeregisterApplicationFuzzTest(const uint8_t *fuzzData, size_t size)
{
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkTwsClientStub::GetDescriptor());
    if (g_twsClientCb != nullptr) {
        data.WriteRemoteObject(g_twsClientCb->AsObject());
    }

    int32_t ret = TwsClientOnRemoteRequest(NearlinkTwsClientInterfaceCode::NL_TWS_DEREGISTER_APP, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void EnableWearDetectionFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkTwsClientStub::GetDescriptor());
    std::string addrStr = BuildAddressString(provider);
    data.WriteString(addrStr);

    int32_t ret = TwsClientOnRemoteRequest(NearlinkTwsClientInterfaceCode::NL_ENABLE_WEAR_DETECTION, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void DisableWearDetectionFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkTwsClientStub::GetDescriptor());
    std::string addrStr = BuildAddressString(provider);
    data.WriteString(addrStr);

    int32_t ret = TwsClientOnRemoteRequest(NearlinkTwsClientInterfaceCode::NL_DISABLE_WEAR_DETECTION, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void GetWearDetectionStateFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkTwsClientStub::GetDescriptor());
    std::string addrStr = BuildAddressString(provider);
    data.WriteString(addrStr);

    int32_t ret = TwsClientOnRemoteRequest(NearlinkTwsClientInterfaceCode::NL_GET_WEAR_DETECTION_STATE, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void IsDeviceWearingFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkTwsClientStub::GetDescriptor());
    std::string addrStr = BuildAddressString(provider);
    data.WriteString(addrStr);

    int32_t ret = TwsClientOnRemoteRequest(NearlinkTwsClientInterfaceCode::NL_IS_DEVICE_WEARING, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void IsWearDetectionSupportedFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkTwsClientStub::GetDescriptor());
    std::string addrStr = BuildAddressString(provider);
    data.WriteString(addrStr);

    int32_t ret = TwsClientOnRemoteRequest(NearlinkTwsClientInterfaceCode::NL_IS_WEAR_DETECTION_SUPPORTED, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void GetTwsRoleInfoFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkTwsClientStub::GetDescriptor());
    std::string addrStr = BuildAddressString(provider);
    data.WriteString(addrStr);

    int32_t ret = TwsClientOnRemoteRequest(NearlinkTwsClientInterfaceCode::NL_GET_TWS_ROLE_INFO, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void GetTwsAudioDelayFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkTwsClientStub::GetDescriptor());
    std::string addrStr = BuildAddressString(provider);
    data.WriteString(addrStr);

    int32_t ret = TwsClientOnRemoteRequest(NearlinkTwsClientInterfaceCode::NL_GET_TWS_AUDIO_DELAY, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void SendUserSelectionFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkTwsClientStub::GetDescriptor());
    std::string addrStr = BuildAddressString(provider);
    data.WriteString(addrStr);
    NearlinkASCAudioStreamInfo streamInfo {};
    std::vector<struct AudioStreamInfo> streamData;
    uint8_t streamCount = provider.ConsumeIntegral<uint8_t>() % TWS_AUDIO_STREAM_INFO_MAX_NUM;
    for (uint8_t i = 0; i < streamCount; i++) {
        struct AudioStreamInfo item = {};
        item.streamType = TWS_AUDIO_STREAM_TYPE_LIST[provider.ConsumeIntegral<uint8_t>() %
            TWS_AUDIO_STREAM_TYPE_NUM];
        item.streamState = static_cast<AudioStreamState>(provider.ConsumeIntegral<uint8_t>() %
            TWS_AUDIO_STREAM_STATE_NUM);
        streamData.push_back(item);
    }
    streamInfo.SetStreamState(streamData);
    data.WriteParcelable(&streamInfo);

    int32_t ret = TwsClientOnRemoteRequest(NearlinkTwsClientInterfaceCode::NL_SEND_USER_SELECTION, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void IsSupportVirtualAutoConnectFuzzTest(const uint8_t *fuzzData, size_t size)
{
    HILOGI("TwsFuzzTest,start run IsSupportVirtualAutoConnectFuzzTest");
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkTwsClientStub::GetDescriptor());
    std::string addrStr = BuildAddressString(provider);
    data.WriteString(addrStr);

    int32_t ret = TwsClientOnRemoteRequest(NearlinkTwsClientInterfaceCode::NL_IS_SUPPORT_VIRTUAL_AUTO_CONNECT,
        data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void SetVirtualAutoConnectTypeFuzzTest(const uint8_t *fuzzData, size_t size)
{
    HILOGI("TwsFuzzTest,start run SetVirtualAutoConnectTypeFuzzTest");
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkTwsClientStub::GetDescriptor());
    std::string addrStr = BuildAddressString(provider);
    data.WriteString(addrStr);
    data.WriteInt32(provider.ConsumeIntegral<int32_t>()); // connType
    data.WriteInt32(provider.ConsumeIntegral<int32_t>()); // businessType

    int32_t ret = TwsClientOnRemoteRequest(NearlinkTwsClientInterfaceCode::NL_SET_VIRTUAL_AUTO_CONNECT_TYPE,
        data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void QueryStreamStateFuzzTest(const uint8_t *fuzzData, size_t size)
{
    HILOGI("TwsFuzzTest,start run QueryStreamStateFuzzTest");
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkTwsClientStub::GetDescriptor());
    std::string addrStr = BuildAddressString(provider);
    data.WriteString(addrStr);

    int32_t ret = TwsClientOnRemoteRequest(NearlinkTwsClientInterfaceCode::NL_QUERY_STREAM_STATE,
        data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

/* 直调 TwsService：业务状态机 + 双端角色序列（OnStart 引导后 ProfileServiceManager 可能已加载） */
void TwsServiceDirectDriveFuzzTest(FuzzedDataProvider &provider)
{
    HILOGI("TwsFuzzTest,start run TwsServiceDirectDriveFuzzTest");
    TwsService *twsService = TwsService::GetService();
    if (twsService == nullptr) {
        HILOGI("TwsService is nullptr, skip direct drive");
        return;
    }

    ThreadUtil::GetInstance().InitThreadStateMap();
    RawAddress devAddr = BuildRawAddress(provider);

    /* 观察者注册/去注册 */
    twsService->RegisterObserver(g_twsObserver);
    std::shared_ptr<InterfaceTwsClientObserver> clientObserver = std::make_shared<MockInterfaceTwsClientObserver>();
    twsService->RegisterApplication(clientObserver);

    /* 连接族 */
    twsService->Connect(devAddr);
    twsService->Disconnect(devAddr);
    twsService->GetConnectState();
    twsService->GetConnectDevices();

    /* 查询族 */
    uint8_t devRole = 0;
    twsService->TwsGetDeviceRole(devAddr, devRole);
    twsService->TwsGetDeviceNature(devAddr);
    twsService->TwsGetDeviceAudioMusicType(devAddr);
    uint32_t audioDelay = 0;
    twsService->GetTwsAudioDelay(devAddr, audioDelay);
    twsService->TwsIsSupportWearDetect(devAddr);
    TwsDevWearStatus wearStatus = {};
    twsService->TwsGetDeviceWearStatus(devAddr, wearStatus);
    twsService->TwsIsDeviceWearing(devAddr);
    twsService->TwsGetWearDetectionState(devAddr);

    /* 角色/业务族 */
    uint8_t devRoleType = static_cast<uint8_t>(provider.ConsumeIntegral<uint8_t>() % TWS_ROLE_TYPE_NUM) + 1;
    twsService->TwsUpdateDeviceDefaultRole(devAddr, devRoleType);
    std::vector<struct AudioStreamInfo> streamData;
    uint8_t streamCount = provider.ConsumeIntegral<uint8_t>() % TWS_AUDIO_STREAM_INFO_MAX_NUM;
    for (uint8_t i = 0; i < streamCount; i++) {
        struct AudioStreamInfo item = {};
        item.streamType = TWS_AUDIO_STREAM_TYPE_LIST[provider.ConsumeIntegral<uint8_t>() %
            TWS_AUDIO_STREAM_TYPE_NUM];
        item.streamState = static_cast<AudioStreamState>(provider.ConsumeIntegral<uint8_t>() %
            TWS_AUDIO_STREAM_STATE_NUM);
        streamData.push_back(item);
    }
    twsService->TwsSendUserSelection(devAddr, streamData);
    twsService->TwsIsSupportVirtualAutoConnect(devAddr);
    twsService->SetVirtualAutoConnectType(devAddr, provider.ConsumeIntegral<int32_t>(),
        provider.ConsumeIntegral<int32_t>());
    twsService->QueryStreamState(devAddr, streamData);
    twsService->ProcPauseRecordMap();
    twsService->UpdateHangUpTimeStamp(devAddr);
    twsService->SendProfileConnected(devAddr);
    std::array<uint8_t, SLE_MANU_ABILITY_LEN> manuAbility = {};
    for (size_t i = 0; i < manuAbility.size(); i++) {
        manuAbility[i] = provider.ConsumeIntegral<uint8_t>();
    }
    twsService->SetDeviceManufacturerAbility(devAddr, manuAbility);
    twsService->GetReportAddr(devAddr);
    twsService->ResetVirtualAutoSwitch(devAddr);
    twsService->RecoverDataFromConf(devAddr);
    twsService->GetPrimaryAddr(devAddr);

    /* 状态机迁移：CONNECTED 分支不触碰 twsClient_（SendMessage 对空 service 已判空），安全可驱动 */
    twsService->NotifyStateChanged(devAddr, TwsClientState::TWS_STATE_CONNECTING, TwsClientState::TWS_STATE_CONNECTED);
    twsService->NotifyStateChanged(devAddr, TwsClientState::TWS_STATE_CONNECTED,
        TwsClientState::TWS_STATE_DISCONNECTING);
    twsService->NotifyStateChanged(devAddr, TwsClientState::TWS_STATE_DISCONNECTING,
        TwsClientState::TWS_STATE_DISCONNECTED);
    TwsClientState preState = static_cast<TwsClientState>(provider.ConsumeIntegral<uint8_t>() %
        TWS_CLIENT_STATE_NUM);
    TwsClientState toState = static_cast<TwsClientState>(provider.ConsumeIntegral<uint8_t>() %
        TWS_CLIENT_STATE_NUM);
    twsService->NotifyStateChanged(devAddr, preState, toState);

    /* 消息事件驱动：仅取编解码/收发类事件 id，避免 STARTUP/SHUTDOWN 扰乱服务生命周期 */
    static const int twsEventList[TWS_EVENT_LIST_NUM] = {
        TWS_SERVICE_SSAP_RECV_DATA_EVENT,
        TWS_SERVICE_RECV_REQ_EVENT,
        TWS_SERVICE_RECV_RSP_EVENT,
        TWS_SERVICE_SEND_REQ_EVENT,
        TWS_SERVICE_SEND_RSP_EVENT,
        TWS_SERVICE_SEND_REMOTE_INFO_EVENT,
    };
    TwsMessage event(twsEventList[provider.ConsumeIntegral<uint8_t>() % TWS_EVENT_LIST_NUM]);
    event.dev_ = devAddr.GetAddress();
    event.msgDirect_ = TWS_MSG_DIRECT_REQ;
    event.msgType_ = static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_ROLE_TYPE);
    twsService->PostEvent(event);

    /* 数据更新驱动：dataType 含 HIBOX_ROLE_TYPE=0x2A */
    static const uint8_t twsDataTypeList[TWS_DATA_TYPE_LIST_NUM] = {
        static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_ROLE_TYPE),
        static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_EARBUDS_NATRUE),
        static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_AUDIO_PROFILE_STATE),
    };
    TwsClientData clientData = {};
    clientData.peerAddr_ = devAddr;
    twsService->UpdateClientData(twsDataTypeList[provider.ConsumeIntegral<uint8_t>() % TWS_DATA_TYPE_LIST_NUM],
        clientData);

    twsService->DeregisterApplication();
    twsService->DeregisterObserver(g_twsObserver);
    ThreadUtil::GetInstance().ClearThreadStateMap();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_100_MS));
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
    HILOGI("TwsFuzzTest OnStart");
    hostServer->OnStart();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_100_MS));
    HILOGI("TwsFuzzTest EnableSle");
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

    /* 入口先开线程状态表：13 个 IPC 段的部分 server 路径经线程投递，表空则任务被丢弃 */
    ThreadUtil::GetInstance().InitThreadStateMap();

    OHOS::RegisterApplicationFuzzTest(data, size);

    OHOS::EnableWearDetectionFuzzTest(data, size);
    OHOS::DisableWearDetectionFuzzTest(data, size);
    OHOS::GetWearDetectionStateFuzzTest(data, size);
    OHOS::IsDeviceWearingFuzzTest(data, size);
    OHOS::IsWearDetectionSupportedFuzzTest(data, size);
    OHOS::GetTwsRoleInfoFuzzTest(data, size);
    OHOS::GetTwsAudioDelayFuzzTest(data, size);
    OHOS::SendUserSelectionFuzzTest(data, size);
    OHOS::QueryStreamStateFuzzTest(data, size);
    OHOS::IsSupportVirtualAutoConnectFuzzTest(data, size);
    OHOS::SetVirtualAutoConnectTypeFuzzTest(data, size);

    OHOS::DeregisterApplicationFuzzTest(data, size);

    /* TwsService 直调驱动（新增段，置于既有 13 个 IPC 函数之后） */
    {
        FuzzedDataProvider fdp(data, size);
        OHOS::TwsServiceDirectDriveFuzzTest(fdp);
    }
    return 0;
}
