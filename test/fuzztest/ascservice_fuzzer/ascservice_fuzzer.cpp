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
#include "ascservice_fuzzer.h"
#include "nearlink_native_token_mock.h"
#include "../nl_utils/fuzztest_utils.h"
#include "nearlink_asc_stub.h"
#include "nearlink_asc_server.h"
#include "nearlink_host_server.h"
#include "nearlink_ssap_property.h"
#include "ThreadUtil.h"
#include "log.h"
#include "securec.h"
#include "raw_address.h"
#ifdef NL_FUZZ_AUDIO_ASC
#include "ASCService.h"
#include "QosM.h"
#include "ASCUtils.h"
#include "SleAudioFrameworkAdapter.h"
#endif

using namespace std;
using namespace OHOS::Nearlink;
namespace OHOS {

class MockNearlinkASCCallbackStub : public IRemoteStub<INearlinkASCCallback> {
public:
    void OnAudioControl(const NearlinkRawAddress &device, const NearlinkASCAudioControlResult& result) {}
    void OnAddSleAudioDevice(const NearlinkRawAddress &device, uint32_t streamType, int32_t mediaVolume,
        int32_t callVolume) {}
    void OnDeleteSleAudioDevice(const NearlinkRawAddress &device) {}
    void OnSleAudioDeviceActionChanged(const NearlinkRawAddress &device, const NearlinkASCAudioStreamInfo &streamInfo,
        int action) {}
    void OnAddSleVirtualAudioDevice(const NearlinkRawAddress &device, AudioStreamType streamType) {}
    void OnDeleteSleVirtualAudioDevice(const NearlinkRawAddress &device) {}
};

namespace {
constexpr int HOST_FUZZ_DELAY_100_MS = 100;
constexpr int HOST_FUZZ_DELAY_5000_MS = 5000;
constexpr uint32_t MESSAGE_SIZE = NearlinkASCInterfaceCode::NL_ASC_BUTT;
sptr<NearlinkASCServer> g_ascClient = new (std::nothrow) NearlinkASCServer();
sptr<INearlinkASCCallback> g_ascClientCb = new (std::nothrow) MockNearlinkASCCallbackStub();
ThreadUtil &g_threadUtil = ThreadUtil::GetInstance();
}

int32_t ASCOnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply)
{
    NL_CHECK_RETURN_RET(g_ascClient, TRANSACTION_ERR, "g_ascClient is nullptr");
    MessageOption option = {MessageOption::TF_SYNC};
    HILOGI("ASCOnRemoteRequest, cmd(%{public}d)", code);
    return g_ascClient->OnRemoteRequest(code, data, reply, option);
}

void RegisterApplicationFuzzTest()
{
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkASCStub::GetDescriptor());
    if (g_ascClientCb != nullptr) {
        data.WriteRemoteObject(g_ascClientCb->AsObject());
    }

    int32_t ret = ASCOnRemoteRequest(NearlinkASCInterfaceCode::NL_ASC_REGISTER_APP, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
}

void DeregisterApplicationFuzzTest()
{
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkASCStub::GetDescriptor());

    int32_t ret = ASCOnRemoteRequest(
        NearlinkASCInterfaceCode::NL_ASC_DEREGISTER_APP, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
}

void ControlFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    RegisterApplicationFuzzTest();
    data.WriteInterfaceToken(NearlinkASCStub::GetDescriptor());
    std::string addrStr = BuildAddressString(provider);
    NearlinkRawAddress addr = NearlinkRawAddress(addrStr);
    data.WriteParcelable(&addr);
    data.WriteInt32(provider.ConsumeIntegral<uint32_t>()); // streamType
    data.WriteInt32(provider.ConsumeIntegral<int32_t>()); // cmd

    int32_t ret = ASCOnRemoteRequest(NearlinkASCInterfaceCode::NL_ASC_CONTROL, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
}

void GetAudioDeviceListFuzzTest()
{
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkASCStub::GetDescriptor());

    int32_t ret = ASCOnRemoteRequest(NearlinkASCInterfaceCode::NL_ASC_GET_AUDIO_DEVICE_LIST, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
}

void GetVirtualAudioDeviceListFuzzTest()
{
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkASCStub::GetDescriptor());

    int32_t ret = ASCOnRemoteRequest(NearlinkASCInterfaceCode::NL_ASC_GET_VIRTUAL_AUDIO_DEVICE_LIST, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
}

void GetSupportStreamTypeFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkASCStub::GetDescriptor());
    std::string addrStr = BuildAddressString(provider);
    NearlinkRawAddress addr = NearlinkRawAddress(addrStr);
    data.WriteParcelable(&addr);

    int32_t ret = ASCOnRemoteRequest(
        NearlinkASCInterfaceCode::NL_ASC_GET_SUPPORT_STREAM_TYPE, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
}

void GetAudioDeviceCodecInfoFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkASCStub::GetDescriptor());
    std::string addrStr = BuildAddressString(provider);
    NearlinkRawAddress addr = NearlinkRawAddress(addrStr);
    data.WriteParcelable(&addr);

    int32_t ret = ASCOnRemoteRequest(
        NearlinkASCInterfaceCode::NL_ASC_GET_AUDIO_DEVICE_CODEC_INFO, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
}

void SetActiveSinkFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkASCStub::GetDescriptor());
    std::string addrStr = BuildAddressString(provider);
    NearlinkRawAddress addr = NearlinkRawAddress(addrStr);
    data.WriteParcelable(&addr);
    data.WriteUint32(provider.ConsumeIntegral<uint32_t>()); // supportStreamType

    int32_t ret = ASCOnRemoteRequest(
        NearlinkASCInterfaceCode::NL_ASC_SET_ACTIVE_SINK_DEVICE, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
}

void GetDualRecordAbilityFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkASCStub::GetDescriptor());
    std::string addrStr = BuildAddressString(provider);
    NearlinkRawAddress addr = NearlinkRawAddress(addrStr);
    data.WriteParcelable(&addr);

    int32_t ret = ASCOnRemoteRequest(
        NearlinkASCInterfaceCode::NL_ASC_GET_DUAL_RECORD_CAP, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
}

void GetKaraokeAbilityFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkASCStub::GetDescriptor());
    std::string addrStr = BuildAddressString(provider);
    NearlinkRawAddress addr = NearlinkRawAddress(addrStr);
    data.WriteParcelable(&addr);

    int32_t ret = ASCOnRemoteRequest(
        NearlinkASCInterfaceCode::NL_ASC_GET_KARAOKE_ABILITY, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
}

void ASCFuzzTest(const uint8_t* fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    uint32_t code = (provider.ConsumeIntegral<uint32_t>() % MESSAGE_SIZE);
    auto addr = BuildAddressString(provider);

    MessageParcel data;
    MessageParcel reply;

    std::u16string descriptor = NearlinkASCStub::GetDescriptor();
    data.WriteInterfaceToken(descriptor);
    data.WriteInt32(provider.ConsumeIntegral<int32_t>());
    data.WriteString(addr.c_str());
    data.RewindRead(0);

    int32_t ret = ASCOnRemoteRequest(code, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
}

#ifdef NL_FUZZ_AUDIO_ASC
class FuzzAscCallback : public InterfaceASCCallback {
public:
    void OnAudioControlComplete(const NearlinkRawAddress &device, const NearlinkASCAudioControlResult &result) {}
    void OnAddSleAudioDevice(const NearlinkRawAddress &device, uint32_t streamType, int32_t mediaVolume,
        int32_t callVolume) {}
    void OnDeleteSleAudioDevice(const NearlinkRawAddress &device) {}
    void OnSleAudioDeviceActionChanged(const NearlinkRawAddress &device, const NearlinkASCAudioStreamInfo &streamInfo,
        int action) {}
    void OnAddSleVirtualAudioDevice(const NearlinkRawAddress &device, AudioStreamType streamType) {}
    void OnDeleteSleVirtualAudioDevice(const NearlinkRawAddress &device) {}
};

/* ASCService 直调：Enable->RegisterApplication->Connect->AudioControl(START/STOP)->StopSink->查询族->Disconnect */
void AscServiceDirectFuzzTest(const uint8_t *fuzzData, size_t size)
{
    ASCService *ascService = ASCService::GetService();
    if (ascService == nullptr) {
        HILOGI("AscServiceDirectFuzzTest: ascService is nullptr");
        return;
    }
    FuzzedDataProvider provider(fuzzData, size);
    ascService->Enable();
    std::this_thread::sleep_for(std::chrono::milliseconds(HOST_FUZZ_DELAY_100_MS));

    std::shared_ptr<InterfaceASCCallback> callback = std::make_shared<FuzzAscCallback>();
    ascService->RegisterApplication(callback);

    const std::string addrStr = BuildAddressString(provider);
    const RawAddress device(addrStr);
    const NearlinkRawAddress nearlinkAddr(addrStr);
    ascService->Connect(device);

    const AudioStreamType streamTypes[] = { AUDIO_STREAM_MUSIC, AUDIO_STREAM_VOICE_CALL, AUDIO_STREAM_RING,
        AUDIO_STREAM_VOIP, AUDIO_STREAM_GAME, AUDIO_STREAM_SING };
    const size_t streamTypeCount = sizeof(streamTypes) / sizeof(streamTypes[0]);
    AudioStreamType streamType = streamTypes[provider.ConsumeIntegral<uint8_t>() % streamTypeCount];
    ascService->AudioControl(device, streamType, NL_SLE_ASC_CONTROL_CMD_START);
    ascService->AudioControl(device, streamType, NL_SLE_ASC_CONTROL_CMD_STOP);
    ascService->StopSink();

    std::vector<NearlinkRawAddress> devices;
    ascService->GetAudioDeviceList(devices);
    ascService->GetVirtualAudioDeviceList(devices);
    uint32_t supportStreamType = 0;
    ascService->GetSupportStreamType(nearlinkAddr, supportStreamType);
    std::map<AudioStreamType, AudioStreamCodecInfo> codecInfo;
    ascService->GetAudioDeviceCodecInfo(nearlinkAddr, codecInfo);
    ascService->SetActiveSinkDevice(nearlinkAddr, supportStreamType);
    ascService->UpdateDeviceRole(device, provider.ConsumeIntegral<uint8_t>());
    (void)ascService->GetDualRecordAbility(device);
    (void)ascService->GetKaraokeAbility(device);

    ascService->Disconnect(device);
    ascService->DeregisterApplication();
    ascService->Disable();
    std::this_thread::sleep_for(std::chrono::milliseconds(HOST_FUZZ_DELAY_100_MS));
}

/* QosM 单例直调：AddQos->JudgeQosPriority->SyncQos->查询族->Clear/Delete + ASCUtils static 族 */
void QosMGearFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    QosM &qosM = QosM::GetInstance();
    const std::string addrStr = BuildAddressString(provider);
    const RawAddress device(addrStr);
    const RawAddress coSetDevice(BuildAddressString(provider));

    constexpr uint8_t qosCount = 10; // NL_SLE_QOS_1..NL_SLE_QOS_10
    Qos qos = static_cast<Qos>(provider.ConsumeIntegral<uint8_t>() % qosCount + 1);
    qosM.AddQos(device, qos);

    DevQosmStru qosm = {};
    qosm.cos = qos;
    qosm.gos.push_back(qos);
    (void)qosM.JudgeQosPriority(device, qosm, qos);
    qosM.SyncQos(device, coSetDevice);
    (void)qosM.IsQos4Exist(device);
    (void)qosM.IsQosExist(device, qos);
    (void)qosM.GetCOS(device);
    (void)qosM.CalculateNos(device);
    (void)qosM.GetQosPriority(qos);
    qosM.ClearQosIfStartFailed(device, qos);
    bool isStopStream = false;
    bool isStopImmediately = false;
    (void)qosM.DeleteQos(device, qos, isStopStream, isStopImmediately);
    qosM.ClearQosM(device);

    (void)ASCUtils::GetEncodeSoundChannelNum();
    (void)ASCUtils::GetEncodeCodecClan(provider.ConsumeIntegral<uint8_t>());
    (void)ASCUtils::GetBpsBitIndexByBps(provider.ConsumeIntegral<uint16_t>());
    (void)ASCUtils::IsNeedSetDirection(qos);
    (void)ASCUtils::GetDirection(provider.ConsumeBool(), qos);
    (void)ASCUtils::GetAutorateTargetFrameType(provider.ConsumeBool());
}

/* SleAudioFrameworkAdapter 单例直调：状态查询 + 输出管道查询 + listener 注册/去注册 */
void AudioAdapterQueryFuzzTest(const uint8_t *fuzzData, size_t size)
{
    (void)fuzzData;
    (void)size;
    SleAudioFrameworkAdapter &adapter = SleAudioFrameworkAdapter::GetInstance();
    (void)adapter.IsMusicActive();
    (void)adapter.IsVoiceCallActive();
    (void)adapter.IsAudioServiceActivate();
    (void)adapter.IsNearlinkOut();
    (void)adapter.IsBtOut();
    (void)adapter.IsInVoipCallKit();
    adapter.GetCurrentOutputPipeInfos();
    adapter.RegisterAudioFrameworkAdapterListener();
    adapter.UnregisterAudioFrameworkAdapterListener();
    /* 产品侧无注销接口，每输入注册会向 AudioCollaborativeManager 泄漏 listener 对象——每进程一次 */
    static bool collaborativeRegistered = false;
    if (!collaborativeRegistered) {
        adapter.RegisterCollaborativeAudioListener();
        collaborativeRegistered = true;
    }
}
#endif
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
    HILOGI("ASCFuzzTest OnStart");
    hostServer->OnStart();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_100_MS));
    HILOGI("ASCFuzzTest EnableSle");
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

    OHOS::g_threadUtil.InitThreadStateMap();
    OHOS::RegisterApplicationFuzzTest();
    OHOS::ControlFuzzTest(data, size);
    OHOS::GetAudioDeviceListFuzzTest();
    OHOS::GetSupportStreamTypeFuzzTest(data, size);
    OHOS::GetAudioDeviceCodecInfoFuzzTest(data, size);
    OHOS::SetActiveSinkFuzzTest(data, size);
    OHOS::GetDualRecordAbilityFuzzTest(data, size);
    OHOS::GetKaraokeAbilityFuzzTest(data, size);
    OHOS::ASCFuzzTest(data, size);
    OHOS::DeregisterApplicationFuzzTest();
#ifdef NL_FUZZ_AUDIO_ASC
    OHOS::AscServiceDirectFuzzTest(data, size);
    OHOS::QosMGearFuzzTest(data, size);
    OHOS::AudioAdapterQueryFuzzTest(data, size);
#endif
    OHOS::g_threadUtil.ClearThreadStateMap();
    return 0;
}

