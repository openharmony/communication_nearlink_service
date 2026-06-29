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
    uid_t GetUid()
    {
        return 0;
    }
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
    data.WriteUint64(provider.ConsumeIntegral<uint64_t>()); // supportStreamType

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
    OHOS::ASCFuzzTest(data, size);
    OHOS::DeregisterApplicationFuzzTest();
    OHOS::g_threadUtil.ClearThreadStateMap();
    return 0;
}

