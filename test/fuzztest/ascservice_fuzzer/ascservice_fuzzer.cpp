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
#ifdef NEARLINK_FUZZ_AUDIO_PARTS
#include <stdlib.h>
#include "SleASC.h"
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

#ifdef NEARLINK_FUZZ_AUDIO_PARTS
// B. SleASC（services/service/src/adapter/SleASC.cpp）进程内直调段。
// SleASC.cpp 与 VcpService 同在 nearlink_deps_audio_exist 门控内编译，
// 故按 vcpservice_fuzzer 的部件开关模式条件编译；NLSTK_Actm* 底层动作
// 与 stack_fuzzer/actm_fuzzer 直调同一批栈接口，裸进程安全。
static void FillAscConfig(FuzzedDataProvider &fdp, NLSTK_ActmConfig_S &config)
{
    config.pointType = fdp.ConsumeIntegral<uint8_t>();
    config.codec.codecId = fdp.ConsumeIntegral<uint8_t>();
    config.codec.companyId = fdp.ConsumeIntegral<uint16_t>();
    config.codec.vendorId = fdp.ConsumeIntegral<uint16_t>();
    config.codec.l2hc.version = fdp.ConsumeIntegral<uint8_t>();
    config.codec.l2hc.rateConf = fdp.ConsumeIntegral<uint8_t>();
    config.codec.l2hc.depthConf = fdp.ConsumeIntegral<uint8_t>();
    config.codec.l2hc.channelConf = fdp.ConsumeIntegral<uint8_t>();
    config.codec.l2hc.frameConf = fdp.ConsumeIntegral<uint8_t>();
    config.codec.l2hc.bpsConf = fdp.ConsumeIntegral<uint8_t>();
    config.codec.l2hc.bpsRange = fdp.ConsumeIntegral<uint64_t>();
    config.channel.comm = fdp.ConsumeIntegral<uint8_t>();
    config.channel.trans = fdp.ConsumeIntegral<uint8_t>();
    config.channel.qosId = fdp.ConsumeIntegral<uint8_t>();
    config.streamType = fdp.ConsumeIntegral<uint32_t>();
    config.duration = fdp.ConsumeIntegral<uint8_t>();
    config.mediaId = fdp.ConsumeIntegral<uint8_t>();
    config.src = fdp.ConsumeIntegral<uint16_t>();
    config.dst = fdp.ConsumeIntegral<uint16_t>();
}

void SleASCDirectFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    std::string addrStr = BuildAddressString(fdp);
    RawAddress device(addrStr);
    uint8_t pointType = fdp.ConsumeIntegral<uint8_t>();
    uint8_t commType = fdp.ConsumeIntegral<uint8_t>();
    uint8_t streamId = fdp.ConsumeIntegral<uint8_t>();
    uint8_t openStreamId = fdp.ConsumeIntegral<uint8_t>();
    uint8_t startStreamId = fdp.ConsumeIntegral<uint8_t>();
    uint8_t stopStreamId = fdp.ConsumeIntegral<uint8_t>();
    uint8_t rateStreamId = fdp.ConsumeIntegral<uint8_t>();
    uint8_t releaseStreamId = fdp.ConsumeIntegral<uint8_t>();
    uint8_t direction = fdp.ConsumeIntegral<uint8_t>();
    uint64_t bps = fdp.ConsumeIntegral<uint64_t>();
    NLSTK_ActmConfig_S srcConfig;
    NLSTK_ActmConfig_S sinkConfig;
    (void)memset_s(&srcConfig, sizeof(srcConfig), 0x00, sizeof(srcConfig));
    (void)memset_s(&sinkConfig, sizeof(sinkConfig), 0x00, sizeof(sinkConfig));
    FillAscConfig(fdp, srcConfig);
    FillAscConfig(fdp, sinkConfig);
    NLSTK_ActmConfigParam_S cfgPara;
    (void)memset_s(&cfgPara, sizeof(cfgPara), 0x00, sizeof(cfgPara));
    cfgPara.streamId = streamId;
    cfgPara.srcConfig = &srcConfig;
    cfgPara.sinkConfig = &sinkConfig;
    cfgPara.isImg = fdp.ConsumeBool();
    NLSTK_ActmAutoRateRecvMsg_S autoRatePara;
    (void)memset_s(&autoRatePara, sizeof(autoRatePara), 0x00, sizeof(autoRatePara));
    autoRatePara.qosIndex = fdp.ConsumeIntegral<uint8_t>();
    autoRatePara.labelId = fdp.ConsumeIntegral<uint8_t>();
    autoRatePara.msgType = fdp.ConsumeIntegral<uint8_t>();
    autoRatePara.result = fdp.ConsumeIntegral<uint32_t>();

    // 全 API 生命周期直调：创建→查询→配置→开流/起停→速率→方向→释放→断开
    (void)SleASC::CreateStream(device, pointType, commType);
    (void)SleASC::GetAudioProperty(device);
    (void)SleASC::ConfigStream(device, cfgPara);
    (void)SleASC::ConfigStreamCustomize(device, cfgPara);
    (void)SleASC::OpenStream(device, openStreamId);
    (void)SleASC::StartStream(device, startStreamId);
    (void)SleASC::StopStream(device, stopStreamId);
    (void)SleASC::UpdateBitRate(device, bps, rateStreamId);
    (void)SleASC::SendVoiceCallAutoRateMsg(device, autoRatePara);
    (void)SleASC::SetDirection(device, direction);
    (void)SleASC::ReleaseStream(device, releaseStreamId);
    (void)SleASC::Disconnect(device);
}
#endif

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
#ifdef NEARLINK_FUZZ_AUDIO_PARTS
    // FDP 尾部追加：SleASC 直调段（新消费仅在尾部，不改旧读序；拷贝传入，dtap/actm 同款范式）
    uint8_t *ascFuzzData = static_cast<uint8_t *>(malloc(size));
    if (ascFuzzData != nullptr) {
        if (memcpy_s(ascFuzzData, size, data, size) == EOK) {
            OHOS::SleASCDirectFuzzTest(ascFuzzData, size);
        }
        free(ascFuzzData);
    }
#endif
    OHOS::g_threadUtil.ClearThreadStateMap();
    return 0;
}

