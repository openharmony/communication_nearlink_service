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
#include "vcpservice_fuzzer.h"
#include "nearlink_native_token_mock.h"
#include "../nl_utils/fuzztest_utils.h"
#include "ThreadUtil.h"
#include "log.h"
#include "nearlink_host_server.h"
// 产品头 nearlink_vcp_client_server.h 与 VcpStackAdapter.h 在 OHOS::Nearlink 内
// 重复定义 VolumeStreamType（产品码双份同文，红线不可改产品头）。本驱动 stub 路径
// 只按 uint8_t 写 parcel、不引用 server 侧枚举名：宏窗口内 include server 头
// （连带其依赖树统一改名），adapter 侧原名保留给 VcpService API。
#define VolumeStreamType NearlinkVcpServerVolumeStreamType
#define SLE_STREAM_MEDIA NearlinkVcpServerSleStreamMedia
#define SLE_STREAM_CALL NearlinkVcpServerSleStreamCall
#define SLE_STREAM_MAX NearlinkVcpServerSleStreamMax
#include "nearlink_vcp_client_server.h"
#undef VolumeStreamType
#undef SLE_STREAM_MEDIA
#undef SLE_STREAM_CALL
#undef SLE_STREAM_MAX
#include "nearlink_vcp_client_stub.h"
#include "nearlink_raw_address.h"
#ifdef NEARLINK_FUZZ_AUDIO_PARTS
#include "VcpService.h"
#endif
#ifdef NEARLINK_FUZZ_TELEPHONY_PARTS
#include "CcpService.h"
#include "CcpStackAdapter.h"
#endif

using namespace std;
using namespace OHOS::Nearlink;
namespace OHOS {
namespace {
    constexpr int HOST_FUZZ_DELAY_50_MS = 50;
    constexpr int HOST_FUZZ_DELAY_100_MS = 100;
    constexpr int HOST_FUZZ_DELAY_5000_MS = 5000;
    ThreadUtil &g_threadUtil = ThreadUtil::GetInstance();
#ifdef NEARLINK_FUZZ_AUDIO_PARTS
    /* VolumeStreamType 成员数：SLE_STREAM_MEDIA/SLE_STREAM_CALL（SLE_STREAM_MAX 为哨兵） */
    constexpr uint8_t VCP_STREAM_TYPE_NUM = 2;
#endif
}

int32_t VcpClientOnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply)
{
    sptr<NearlinkVcpClientServer> vcpClient = new (std::nothrow) NearlinkVcpClientServer();
    NL_CHECK_RETURN_RET(vcpClient, TRANSACTION_ERR, "vcpClient is nullptr");
    MessageOption option = {MessageOption::TF_SYNC};
    HILOGI("VcpClientOnRemoteRequest, cmd(%{public}d)", code);
    return vcpClient->OnRemoteRequest(code, data, reply, option);
}

void SetDeviceAbsoluteVolumeFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkVcpClientStub::GetDescriptor());
    std::string addrStr = BuildAddressString(provider);
    NearlinkRawAddress addr = NearlinkRawAddress(addrStr);
    data.WriteParcelable(&addr);
    data.WriteInt32(provider.ConsumeIntegral<int32_t>()); // volumeLevel（stub 读 Int32）
    data.WriteUint8(provider.ConsumeIntegral<uint8_t>()); // streamType（stub 读 Uint8）

    int32_t ret = VcpClientOnRemoteRequest(NL_VCP_CLIENT_SET_DEVICE_ABSOLUTE_VOLUME, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
}

void GetDeviceMediaVolumeFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkVcpClientStub::GetDescriptor());
    std::string addrStr = BuildAddressString(provider);
    NearlinkRawAddress addr = NearlinkRawAddress(addrStr);
    data.WriteParcelable(&addr);

    int32_t ret = VcpClientOnRemoteRequest(NL_VCP_CLIENT_GET_MEDIA_VOLUME, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void GetDeviceCallVolumeFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkVcpClientStub::GetDescriptor());
    std::string addrStr = BuildAddressString(provider);
    NearlinkRawAddress addr = NearlinkRawAddress(addrStr);
    data.WriteParcelable(&addr);

    int32_t ret = VcpClientOnRemoteRequest(NL_VCP_CLIENT_GET_CALL_VOLUME, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

#ifdef NEARLINK_FUZZ_AUDIO_PARTS
class MockVcpClientObserver : public VcpClientObserver {
public:
    void OnConnectionStateChanged(const RawAddress &deviceAddress, int state, int oldState) {}
};

/* VcpService 直调驱动体：服务实例与局部实例共用 */
void DriveVcpService(VcpService &vcpSvc, FuzzedDataProvider &provider)
{
    RawAddress devAddr = BuildRawAddress(provider);

    MockVcpClientObserver vcpObserver;
    vcpSvc.RegisterObserver(vcpObserver);
    vcpSvc.DeregisterObserver(vcpObserver);

    vcpSvc.Enable();
    vcpSvc.Disable();
    vcpSvc.Connect(devAddr);
    vcpSvc.Disconnect(devAddr);
    vcpSvc.GetConnectState();
    vcpSvc.GetConnectDevices();

    /* SetDeviceAbsoluteVolumeTask 校验 volume∈[0, musicMaxVolumeLevel_] 与 streamType<SLE_STREAM_MAX：合法/越界两档 */
    int32_t volumeInBound = static_cast<int32_t>(provider.ConsumeIntegral<uint8_t>());
    int32_t volumeOutBound = -1 - static_cast<int32_t>(provider.ConsumeIntegral<uint8_t>());
    uint8_t streamType = provider.ConsumeIntegral<uint8_t>() % VCP_STREAM_TYPE_NUM;
    vcpSvc.SetDeviceAbsoluteVolume(devAddr, volumeInBound, streamType);
    vcpSvc.SetDeviceAbsoluteVolume(devAddr, volumeOutBound, VCP_STREAM_TYPE_NUM);
    vcpSvc.GetDeviceMediaVolume(devAddr);
    vcpSvc.GetDeviceCallVolume(devAddr);
    vcpSvc.SwitchAbsVolumeDevice(devAddr, provider.ConsumeBool());
    vcpSvc.SetAllStreamVolume(devAddr);

    /* VcpStackAdapter 不做直调（BLOCKED）：其全局注册表为单实例语义——静态 g_vcpStackAdapter
     * 指针 + NLSTK 全局回调注册，栈/临时实例的构造与 DeregisterStackCbk 都会覆盖常驻
     * VcpService::impl 的注册链且异步响应按值快照裸指针，进程内多实例必然悬垂。
     * SetVolume/SetStreamVolume 的 NLSTK 路径已由上方 SetDeviceAbsoluteVolumeTask 链覆盖。 */
}

/* 局部回退实例：new 出后永不析构——退出窗静态析构会调 NLSTK/samgr，次序不可控；指针经 static 可达 */
VcpService &GetLocalVcpService()
{
    static VcpService *instance = new VcpService();
    return *instance;
}

void VcpServiceDirectDriveFuzzTest(FuzzedDataProvider &provider)
{
    HILOGI("VcpFuzzTest,start run VcpServiceDirectDriveFuzzTest");
    VcpService *vcpService = VcpService::GetService();
    if (vcpService != nullptr) {
        DriveVcpService(*vcpService, provider);
        return;
    }
    /* 环境前置不满足：profile 管理器未加载，退化为局部实例驱动（ctor public） */
    HILOGI("VcpService is nullptr, drive with local instance");
    DriveVcpService(GetLocalVcpService(), provider);
}
#endif

#ifdef NEARLINK_FUZZ_TELEPHONY_PARTS
/* CcpService 直调驱动体（CcpSystemInterface 的 Start/Stop 依赖 binder SA 订阅，不在 fuzz 环境直调） */
void DriveCcpService(CcpService &ccpSvc, FuzzedDataProvider &provider)
{
    RawAddress devAddr = BuildRawAddress(provider);

    ccpSvc.Enable();
    ccpSvc.Disable();

    int32_t instanceId = provider.ConsumeIntegral<int32_t>();
    uint32_t requestId = provider.ConsumeIntegral<uint32_t>();
    uint8_t callId = provider.ConsumeIntegral<uint8_t>();
    ccpSvc.HandleAnswer(devAddr, instanceId, requestId, callId);
    ccpSvc.HandleHangUp(devAddr, instanceId, requestId, callId);
    ccpSvc.HandleStartInstance(instanceId, provider.ConsumeBool());
    ccpSvc.HandleAuthorize(instanceId, requestId, provider.ConsumeIntegral<int32_t>());
    ccpSvc.SetGeneralInstanceId(instanceId);
    ccpSvc.TryResumeCurrentCalls();
    ccpSvc.IsNearlinkActiveDevice(devAddr);

    NearlinkCallPhoneState phoneState = {};
    phoneState.SetActiveNum(provider.ConsumeIntegral<int32_t>());
    phoneState.SetHeldNum(provider.ConsumeIntegral<int32_t>());
    phoneState.SetCallState(provider.ConsumeIntegral<int32_t>());
    phoneState.SetCallType(provider.ConsumeIntegral<int32_t>());
    ccpSvc.HandlePhoneStateChange(phoneState);

    CcpStackAdapter ccpStackAdapter;
    ccpStackAdapter.CreateCcsInstance(provider.ConsumeIntegral<uint8_t>());
    ccpStackAdapter.DeleteCcsInstance(provider.ConsumeIntegral<int32_t>());
    ccpStackAdapter.NotifyCallControlSuccess(provider.ConsumeIntegral<uint32_t>(),
        provider.ConsumeIntegral<int32_t>());
    ccpStackAdapter.NotifyCallControlFail(provider.ConsumeIntegral<uint32_t>(),
        provider.ConsumeIntegral<int32_t>());
    ccpStackAdapter.AuthorizeResult(provider.ConsumeIntegral<int32_t>(), provider.ConsumeIntegral<uint32_t>(),
        provider.ConsumeIntegral<int32_t>());
}

/* 局部回退实例：new 出后永不析构——退出窗静态析构会调 samgr UnSubscribe，次序不可控（CcpService ctor
 * 隐式 CcpSystemInterface::Start 对 samgr 不可用会提前 return，fuzz 环境安全） */
CcpService &GetLocalCcpService()
{
    static CcpService *instance = new CcpService();
    return *instance;
}

void CcpServiceDirectDriveFuzzTest(FuzzedDataProvider &provider)
{
    HILOGI("VcpFuzzTest,start run CcpServiceDirectDriveFuzzTest");
    CcpService *ccpService = CcpService::GetService();
    if (ccpService != nullptr) {
        DriveCcpService(*ccpService, provider);
        return;
    }
    /* 环境前置不满足：profile 管理器未加载，退化为局部实例驱动 */
    HILOGI("CcpService is nullptr, drive with local instance");
    DriveCcpService(GetLocalCcpService(), provider);
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
    HILOGI("VcpFuzzTest OnStart");
    hostServer->OnStart();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_100_MS));
    HILOGI("VcpFuzzTest EnableSle");
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

    OHOS::g_threadUtil.InitThreadStateMap();
    OHOS::SetDeviceAbsoluteVolumeFuzzTest(data, size);
    OHOS::GetDeviceMediaVolumeFuzzTest(data, size);
    OHOS::GetDeviceCallVolumeFuzzTest(data, size);
#ifdef NEARLINK_FUZZ_AUDIO_PARTS
    /* VcpService 直调驱动（audio 部件存在时） */
    {
        FuzzedDataProvider fdp(data, size);
        OHOS::VcpServiceDirectDriveFuzzTest(fdp);
    }
#endif
#ifdef NEARLINK_FUZZ_TELEPHONY_PARTS
    /* Ccp 族直调驱动（telephony 部件存在时） */
    {
        FuzzedDataProvider fdp(data, size);
        OHOS::CcpServiceDirectDriveFuzzTest(fdp);
    }
#endif
    OHOS::g_threadUtil.ClearThreadStateMap();
    return 0;
}
