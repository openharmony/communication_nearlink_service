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
#include "ssapserveradapter_fuzzer.h"
#include "../../nl_utils/fuzztest_utils.h"
#include "ssap_def.h"
#include "log.h"
#include "securec.h"
#include "raw_address.h"
#include "ThreadUtil.h"
#include "sdf_thread.h"
#include "sdf_evc.h"
#include "nlstk_schedule.h"
#include "sdf_mem.h"
#include "sdf_vector.h"
#include "ssaps_server_app.h"
#include "slem.h"

/* On* 静态入口为 private，须在 private=public 区域内包含该头（与
 * ssapclient_fuzzer.cpp 同款写法） */
#define private public
#include "ssap_server_stack_adapter.h"
#undef private

using namespace std;
using namespace OHOS::Nearlink;
namespace OHOS {
ThreadUtil &g_threadUtil = ThreadUtil::GetInstance();
constexpr int SSAP_SERVER_ADAPTER_FUZZ_DELAY_50_MS = 50;
constexpr size_t SSAP_SERVER_ADAPTER_FUZZ_VAL_LEN = 8;

namespace {
NLSTK_SsapUuid_S g_serverAdapterUuid = {.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02}};
}
#define DECODE2BYTE(_ptr) (static_cast<uint16_t>(*reinterpret_cast<const uint8_t*>(_ptr)) | \
    (static_cast<uint16_t>(*reinterpret_cast<const uint8_t*>((_ptr) + 1)) << 8))
#define DECODE4BYTE(_ptr) (static_cast<uint32_t>(*reinterpret_cast<const uint8_t*>((_ptr))) | \
    (static_cast<uint32_t>(*reinterpret_cast<const uint8_t*>(((_ptr) + 1))) << 8) | \
    (static_cast<uint32_t>(*reinterpret_cast<const uint8_t*>(((_ptr) + 2))) << 16) | \
    (static_cast<uint32_t>(*reinterpret_cast<const uint8_t*>(((_ptr) + 3))) << 24))
class SsapServerStackCallbackFuzzMock : public SsapServerStackCallback {
public:
    void OnMtuChanged(int appId, const RawAddress &addr, uint16_t mtu) override {}
    void OnAddService(int appId, Service &service, int ret) override {}
    void OnSetPropertyValue(int appId, Property &property, int ret) override {}
    void OnSetDescriptorValue(int appId, Descriptor &descriptor, int ret) override {}
    void OnReadPropertyAuthorizeRequest(
        int appId, const RawAddress &addr, uint16_t requestId, Property &property) override {}
    void OnReadDescriptorAuthorizeRequest(
        int appId, const RawAddress &addr, uint16_t requestId, Descriptor &descriptor) override {}
    void OnWritePropertyAuthorizeRequest(
        int appId, const RawAddress &addr, uint16_t requestId, Property &property) override {}
    void OnWriteDescriptorAuthorizeRequest(
        int appId, const RawAddress &addr, uint16_t requestId, Descriptor &descriptor) override {}
    void OnReadProperty(int appId, const RawAddress &addr, Property &property, int ret) override {}
    void OnReadDescriptor(int appId, const RawAddress &addr, Descriptor &descriptor, int ret) override {}
    void OnWriteProperty(int appId, const RawAddress &addr, Property &property, int ret) override {}
    void OnWriteDescriptor(int appId, const RawAddress &addr, Descriptor &descriptor, int ret) override {}
    void OnNotifyProperty(int appId, const RawAddress &addr, Property &property, int ret) override {}
    void OnConnectionStateChanged(int appId, const RawAddress &addr, int state, int ret, int reason) override {}
};

void SsapServerAdapterApiFuzzTest(const uint8_t* data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    struct Service service = {};
    int appId = provider.ConsumeIntegral<int>();

    SsapServerStackCallbackFuzzMock cb;
    SsapServerStackAdapter adapter(cb);
    (void)adapter.RegisterApplication(appId);

    adapter.DeregisterApplication(provider.ConsumeIntegral<int>());
    adapter.SetMtu(provider.ConsumeIntegral<uint16_t>());
    adapter.AddService(provider.ConsumeIntegral<int>(), service);
    adapter.RemoveService(provider.ConsumeIntegral<int>(), provider.ConsumeIntegral<uint16_t>());
    adapter.ClearServices(provider.ConsumeIntegral<int>());
    (void)adapter.CheckServiceExistByUuid(provider.ConsumeIntegral<int>(), BuildUuid(provider));
    adapter.AuthorizeResponse(provider.ConsumeIntegral<int>(),
        provider.ConsumeIntegral<uint16_t>(), provider.ConsumeBool());

    struct Property pro = {};
    pro.handle_ = provider.ConsumeIntegral<uint16_t>();

    adapter.SetPropertyValue(provider.ConsumeIntegral<int>(), pro);
    adapter.Notify(provider.ConsumeIntegral<int>(), pro, BuildRawAddress(provider));

    struct Descriptor des = {};
    des.handle_ = provider.ConsumeIntegral<uint16_t>();

    adapter.SetDescriptorValue(provider.ConsumeIntegral<int>(), des);
}

/* 尾部追加段：真实注册成功后以真 appId 驱动 SsapServerStackAdapter::On* 静态
 * 入口 14 个全集——经 DoInSsapThread 落到 On*Task→g_ssapServerStackAdapter
 * （指向本函数局部的 adapter）→SsapServerStackCallback mock；失败保 -1 跳过 */
void ServerAdapterCallbacksFuzzTest(const uint8_t* data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    /* static（cb+adapter 成对）：On* 投递任务按值捕获 g_ssapServerStackAdapter，栈对象析构后
     * 残余任务即悬垂（红队 M3）；adapter 构造持 cb 引用，两者须同为进程常驻 */
    static SsapServerStackCallbackFuzzMock cb;
    static SsapServerStackAdapter adapter(cb);
    int appId = -1;
    (void)adapter.RegisterApplication(appId);
    if (appId < 0) {
        return;
    }
    SLE_Addr_S peer = {};
    for (size_t i = 0; i < SLE_ADDR_LEN; i++) {
        peer.addr[i] = provider.ConsumeIntegral<uint8_t>();
    }
    uint8_t valBuf[SSAP_SERVER_ADAPTER_FUZZ_VAL_LEN] = {};
    for (size_t i = 0; i < sizeof(valBuf); i++) {
        valBuf[i] = provider.ConsumeIntegral<uint8_t>();
    }
    uint16_t handle = provider.ConsumeIntegral<uint16_t>();

    SsapServerStackAdapter::OnMtuChanged(appId, &peer, provider.ConsumeIntegral<uint16_t>());
    /* OnAddService 需真实 SDF_Vector（properties 为空指针时直接早退） */
    SDF_Vector_S *descVec = SDF_CreateVector(MAKE_TRAITS(NULL, NULL));
    SSAP_Descriptor_S *stackDesc = static_cast<SSAP_Descriptor_S *>(SDF_MemZalloc(sizeof(SSAP_Descriptor_S)));
    SDF_Vector_S *propVec = SDF_CreateVector(MAKE_TRAITS(NULL, NULL));
    SSAP_Property_S *stackProp = static_cast<SSAP_Property_S *>(SDF_MemZalloc(sizeof(SSAP_Property_S)));
    if (descVec != nullptr && stackDesc != nullptr && propVec != nullptr && stackProp != nullptr) {
        stackDesc->type = static_cast<NLSTK_SsapDescriptorType_E>(
            provider.ConsumeIntegral<uint8_t>() % DESC_TYPE_MAX);
        (void)SDF_VectorEmplaceBack(descVec, stackDesc);
        stackProp->handle = handle;
        stackProp->uuid = g_serverAdapterUuid;
        stackProp->descriptors = descVec;
        (void)SDF_VectorEmplaceBack(propVec, stackProp);
        SSAP_Service_S stackService = {};
        stackService.appId = appId;
        stackService.handle = handle;
        stackService.endHandle = static_cast<uint16_t>(handle + 8);
        stackService.uuid = g_serverAdapterUuid;
        stackService.properties = propVec;
        SsapServerStackAdapter::OnAddService(appId, &stackService, NLSTK_ERRCODE_SUCCESS);
    }
    SDF_DestroyVector(propVec);
    SDF_MemFree(stackProp);
    SDF_DestroyVector(descVec);
    SDF_MemFree(stackDesc);

    NLSTK_SsapServerOnSetPropertyParam_S setPropParam = {};
    setPropParam.handle = handle;
    setPropParam.uuid = g_serverAdapterUuid;
    setPropParam.value.len = sizeof(valBuf);
    setPropParam.value.data = valBuf;
    SsapServerStackAdapter::OnSetPropertyValue(appId, &setPropParam, NLSTK_ERRCODE_SUCCESS);
    NLSTK_SsapServerOnSetDescriptorParam_S setDescParam = {};
    setDescParam.handle = handle;
    setDescParam.uuid = g_serverAdapterUuid;
    setDescParam.type = provider.ConsumeIntegral<uint8_t>() % DESC_TYPE_MAX;
    setDescParam.value.len = sizeof(valBuf);
    setDescParam.value.data = valBuf;
    SsapServerStackAdapter::OnSetDescriptorValue(appId, &setDescParam, NLSTK_ERRCODE_SUCCESS);

    NLSTK_SsapServerReadPropertyInfo_S readProp = {};
    readProp.addr = peer;
    readProp.uuid = g_serverAdapterUuid;
    readProp.handle = handle;
    uint16_t requestId = provider.ConsumeIntegral<uint16_t>();
    SsapServerStackAdapter::OnReadPropertyAuthorizeRequest(appId, requestId, &readProp);
    NLSTK_SsapServerReadDescriptorInfo_S readDesc = {};
    readDesc.addr = peer;
    readDesc.handle = handle;
    readDesc.uuid = g_serverAdapterUuid;
    readDesc.type = provider.ConsumeIntegral<uint8_t>() % DESC_TYPE_MAX;
    SsapServerStackAdapter::OnReadDescriptorAuthorizeRequest(appId, requestId, &readDesc);
    NLSTK_SsapServerWritePropertyInfo_S writeProp = {};
    writeProp.addr = peer;
    writeProp.handle = handle;
    writeProp.uuid = g_serverAdapterUuid;
    writeProp.value.len = sizeof(valBuf);
    writeProp.value.data = valBuf;
    SsapServerStackAdapter::OnWritePropertyAuthorizeRequest(appId, requestId, &writeProp);
    NLSTK_SsapServerWriteDescriptorInfo_S writeDesc = {};
    writeDesc.addr = peer;
    writeDesc.handle = handle;
    writeDesc.uuid = g_serverAdapterUuid;
    writeDesc.type = provider.ConsumeIntegral<uint8_t>() % DESC_TYPE_MAX;
    writeDesc.value.len = sizeof(valBuf);
    writeDesc.value.data = valBuf;
    SsapServerStackAdapter::OnWriteDescriptorAuthorizeRequest(appId, requestId, &writeDesc);

    SsapServerStackAdapter::OnReadProperty(appId, &readProp);
    SsapServerStackAdapter::OnReadDescriptor(appId, &readDesc);
    SsapServerStackAdapter::OnWriteProperty(appId, &writeProp);
    SsapServerStackAdapter::OnWriteDescriptor(appId, &writeDesc);
    NLSTK_SsapServerOnNotifyPropertyParam_S notifyParam = {};
    notifyParam.addr = peer;
    notifyParam.handle = handle;
    notifyParam.uuid = g_serverAdapterUuid;
    notifyParam.value.len = sizeof(valBuf);
    notifyParam.value.data = valBuf;
    SsapServerStackAdapter::OnNotifyProperty(appId, &notifyParam, NLSTK_ERRCODE_SUCCESS);
    SsapServerStackAdapter::OnConnectionStateChanged(appId, &peer, provider.ConsumeIntegral<uint8_t>(),
        NLSTK_ERRCODE_SUCCESS, provider.ConsumeIntegral<int>());

    adapter.DeregisterApplication(appId);
    /* 既有段直调注册成功后无人注销，栈侧 64 槽会跨输入泄漏耗尽使真实注册恒败；
     * 全量清扫残留（test-only，进程内 server app 仅本驱动持有） */
    for (int id = 0; id < NLSTK_SSAP_SERVER_APP_MAX_NUM; id++) {
        NLSTK_SsapServerDeregisterApplication(id);
    }
    /* adapter/cb 为本函数局部对象：排空 DoInSsapThread 已投任务再析构 */
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::SSAP_SERVER_ADAPTER_FUZZ_DELAY_50_MS));
}
}

// fuzzer init
extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void) argc;
    (void) argv;
    int ret = slem_initialize();
    HILOGI("slem_initialize %{public}d", ret);
    printf("slem_initialize %d\n", ret);
    /* 仅 slem_initialize 时缺调度三件套，NLSTK_SsapServerRegApp 的
     * SchedulePostTaskBlocked 必超时失败（appId=-1）。PR-4 已在
     * stack_fuzzer/ssapclientuapi_fuzzer 实证缺 ThreadInit/EvcInit 时 RegApp
     * 必败；ScheduleEnable 幂等，末尾重建保证调度器存活（test-only）。 */
    (void)SDF_ThreadInit(10);
    (void)SDF_EvcInit();
    (void)ScheduleEnable();
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr) {
        return 0;
    }
    /* On* 静态入口经 DoInSsapThread 派发，依赖 ssap 线程态（未初始化时任务被
     * 静默丢弃），与 ssapservicesserver_fuzzer 同款首尾配对 */
    OHOS::g_threadUtil.InitThreadStateMap();
    OHOS::SsapServerAdapterApiFuzzTest(data, size);
    OHOS::ServerAdapterCallbacksFuzzTest(data, size);
    OHOS::g_threadUtil.ClearThreadStateMap();
    return 0;
}
