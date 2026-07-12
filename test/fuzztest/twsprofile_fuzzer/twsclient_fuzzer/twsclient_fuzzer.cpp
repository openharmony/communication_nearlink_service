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
#include "twsclient_fuzzer.h"
#include "nearlink_native_token_mock.h"
#include "../../nl_utils/fuzztest_utils.h"
#include "log.h"
#include "nearlink_host_server.h"
#include "nearlink_tws_client_server.h"
#include "nearlink_tws_client_stub.h"
#include "nearlink_raw_address.h"
#include "nearlink_ASC_source.h"
#include "nearlink_asc_audio_stream_info.h"

using namespace std;
using namespace OHOS::Nearlink;
namespace OHOS {
class MockNearlinkTwsClientCallbackStub : public IRemoteStub<INearlinkTwsClientObserver> {
public:
    void OnTwsRemoteInfo(const std::string &address, const std::vector<uint8_t> &value) {}
};

namespace {
    constexpr int HOST_FUZZ_DELAY_50_MS = 50;
    constexpr int HOST_FUZZ_DELAY_100_MS = 100;
    constexpr int HOST_FUZZ_DELAY_5000_MS = 5000;
    sptr<INearlinkTwsClientObserver> g_twsClientCb = new (std::nothrow) MockNearlinkTwsClientCallbackStub();
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
    AudioStreamType TWS_AUDIO_STREAM_TYPE_LIST[] = {
        AUDIO_STREAM_UNDEFINED,       // 未定义
        AUDIO_STREAM_MUSIC,           // 媒体音乐，指的是播放歌曲
        AUDIO_STREAM_VOICE_CALL,      // 移动网络通话
        AUDIO_STREAM_VOICE_ASSISTANT, // 语音助手
        AUDIO_STREAM_RING,            // 铃声
        AUDIO_STREAM_VOIP,            // IP通话
        AUDIO_STREAM_GAME,            // 低时延：游戏
        AUDIO_STREAM_RECORD,          // 录音
        AUDIO_STREAM_ALERT,           // 提示音
        AUDIO_STREAM_VIDEO,           // 视频声
        AUDIO_STREAM_GUID,            // 导航声
        AUDIO_STREAM_ALARM,           // 告警声
    };

    std::vector<AudioStreamState> streamType = {
        AUDIO_STREAM_STATE_INVALID, AUDIO_STREAM_STATE_INVALID, AUDIO_STREAM_STATE_INVALID,
        AUDIO_STREAM_STATE_AVAILABLE, AUDIO_STREAM_STATE_AVAILABLE, AUDIO_STREAM_STATE_AVAILABLE,
        AUDIO_STREAM_STATE_NOT_AVAILABLE, AUDIO_STREAM_STATE_NOT_AVAILABLE, AUDIO_STREAM_STATE_NOT_AVAILABLE,
        AUDIO_STREAM_STATE_NOT_AVAILABLE, AUDIO_STREAM_STATE_INVALID, AUDIO_STREAM_STATE_AVAILABLE };

    data.WriteInterfaceToken(NearlinkTwsClientStub::GetDescriptor());
    std::string addrStr = BuildAddressString(provider);
    data.WriteString(addrStr);
    NearlinkASCAudioStreamInfo streamInfo {};
    std::vector<struct AudioStreamInfo> streamData;
    for (uint16_t i = 0; i < sizeof(TWS_AUDIO_STREAM_TYPE_LIST) / sizeof(uint32_t); i++) {
        struct AudioStreamInfo data;
        data.streamType = TWS_AUDIO_STREAM_TYPE_LIST[i];
        data.streamState = streamType[i];
        streamData.push_back(data);
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

    int connectType = 0;
    int businessType = 0;

    data.WriteInterfaceToken(NearlinkTwsClientStub::GetDescriptor());
    std::string addrStr = BuildAddressString(provider);
    data.WriteString(addrStr);
    data.WriteInt32(connectType);
    data.WriteInt32(businessType);

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
    return 0;
}
