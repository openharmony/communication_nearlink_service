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
#include "ssapservicesserver_fuzzer.h"
#include "../../nl_utils/fuzztest_utils.h"
#include "nearlink_hadm_client_stub.h"
#include "nearlink_hadm_client_server.h"
#include "nearlink_host_server.h"
#include "ThreadUtil.h"
#include "log.h"
#include "securec.h"
#include "slem.h"
#include "raw_address.h"
#include "ssap_server_service.h"
#include "sdf_thread.h"
#include "sdf_evc.h"
#include "nlstk_schedule.h"
#include "sdf_mem.h"
#include "sdf_vector.h"
#include "ssaps_server_app.h"

/* On* 静态入口为 private，须在 private=public 区域内首次包含该头（与
 * ssapclient_fuzzer.cpp 同款写法） */
#define private public
#include "ssap_server_stack_adapter.h"
#undef private

using namespace std;
using namespace OHOS::Nearlink;
namespace OHOS {
ThreadUtil &g_threadUtil = ThreadUtil::GetInstance();
constexpr int SSAP_FUZZ_DELAY_50_MS = 50;
constexpr size_t SSAP_SERVER_FUZZ_VAL_LEN = 8;

namespace {
NLSTK_SsapUuid_S g_serverFuzzUuid = {.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02}};
}
class InterfaceSsapServerCb : public InterfaceSsapServerCallback {
public:
    void OnMtuChanged(const RawAddress &addr, uint8_t transport, uint16_t mtu) override {}
    void OnAddService(Service &service, int ret) override {}
    void OnSetPropertyValue(Property &property, int ret) override {}
    void OnSetDescriptorValue(Descriptor &descriptor, int ret) override {}
    void OnReadPropertyAuthorizeRequest(
        const RawAddress &addr, uint8_t transport, uint16_t requestId, Property &property) override {}
    void OnReadDescriptorAuthorizeRequest(
        const RawAddress &addr, uint8_t transport, uint16_t requestId, Descriptor &descriptor) override {}
    void OnWritePropertyAuthorizeRequest(
        const RawAddress &addr, uint8_t transport, uint16_t requestId, Property &property) override {}
    void OnWriteDescriptorAuthorizeRequest(
        const RawAddress &addr, uint8_t transport, uint16_t requestId, Descriptor &descriptor) override {}
    void OnReadProperty(const RawAddress &addr, uint8_t transport, Property &property, int ret) override {}
    void OnReadDescriptor(const RawAddress &addr, uint8_t transport, Descriptor &descriptor, int ret) override {}
    void OnWriteProperty(const RawAddress &addr, uint8_t transport, Property &property, int ret) override {}
    void OnWriteDescriptor(const RawAddress &addr, uint8_t transport, Descriptor &descriptor, int ret) override {}
    void OnNotifyProperty(const RawAddress &addr, uint8_t transport, Property &property, int ret) override {}
    void OnIndicateProperty(const RawAddress &addr, uint8_t transport, Property &property, int ret) override {}
    void OnNotifyEvent(const RawAddress &addr, uint8_t transport, Event &event, int ret) override {}
    void OnIndicateEvent(const RawAddress &addr, uint8_t transport, Event &event, int ret) override {}
    void OnCallMethod(const RawAddress &addr, uint8_t transport, Method &method,
                      std::vector<uint8_t> &value, bool needReturn) override {}
    void OnConnectionStateChanged(const RawAddress &addr, uint8_t transport, uint8_t state,
        int ret, int reason) override {}
};

/* 真实注册后观测 service AdapterCallback 下发的回调（其余虚函数沿用基类空实现） */
class SsapServerObserveFuzzCallback : public InterfaceSsapServerCallback {
public:
    void OnMtuChanged(const RawAddress &addr, uint8_t transport, uint16_t mtu) override { mtu_ = mtu; }
    void OnAddService(Service &service, int ret) override { handle_ = service.handle_; errCode_ = ret; }
    void OnSetPropertyValue(Property &property, int ret) override { handle_ = property.handle_; errCode_ = ret; }
    void OnReadPropertyAuthorizeRequest(
        const RawAddress &addr, uint8_t transport, uint16_t requestId, Property &property) override
    {
        requestId_ = requestId;
        handle_ = property.handle_;
    }
    void OnConnectionStateChanged(const RawAddress &addr, uint8_t transport, uint8_t state,
        int ret, int reason) override { state_ = state; errCode_ = ret; }

    uint16_t mtu_ = 0;
    uint16_t handle_ = 0;
    uint16_t requestId_ = 0;
    uint8_t state_ = 0;
    int errCode_ = 0;
};

void DoSomethingSsapServiceServerWithMyAPI(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    std::shared_ptr<InterfaceSsapServerCallback> cb = std::make_shared<InterfaceSsapServerCb>();
    SsapServerService service;
    struct Service ce;
    struct Property pro;
    struct Descriptor des;
    struct Event event;
    struct Method me;
    std::vector<uint8_t> value;

    service.EnableTask();
    service.RegisterApplicationTask(cb, 1, 1);
    service.DeregisterApplicationTask(provider.ConsumeIntegral<int>());
    service.SetMtuTask(provider.ConsumeIntegral<int>(), provider.ConsumeIntegral<uint16_t>());
    service.AddServiceTask(provider.ConsumeIntegral<int>(), ce);
    service.RemoveServiceTask(provider.ConsumeIntegral<int>(), ce.handle_);
    service.ClearServicesTask(provider.ConsumeIntegral<int>());
    service.CheckServiceExistByUuidTask(provider.ConsumeIntegral<int>(), BuildUuid(provider));
    service.SetPropertyValueTask(provider.ConsumeIntegral<int>(), pro);
    service.SetDescriptorValueTask(provider.ConsumeIntegral<int>(), des);
    service.GetPropertyValue(provider.ConsumeIntegral<int>(), pro);
    service.GetDescriptorValue(provider.ConsumeIntegral<int>(), des);
    service.AuthorizeResponseTask(provider.ConsumeIntegral<int>(), data[0], provider.ConsumeBool());
    service.NotifyPropertyTask(provider.ConsumeIntegral<int>(), pro, BuildRawAddress(provider));
    service.IndicateProperty(provider.ConsumeIntegral<int>(), pro,
        BuildRawAddress(provider), provider.ConsumeIntegral<uint8_t>());
    service.NotifyEvent(provider.ConsumeIntegral<int>(), event, value,
        BuildRawAddress(provider), provider.ConsumeIntegral<uint8_t>());
    service.IndicateEvent(provider.ConsumeIntegral<int>(), event, value,
        BuildRawAddress(provider), provider.ConsumeIntegral<uint8_t>());
    service.ReturnMethod(provider.ConsumeIntegral<int>(), me, value,
        BuildRawAddress(provider), provider.ConsumeIntegral<uint8_t>());
    service.Connect(provider.ConsumeIntegral<int>(), BuildRawAddress(provider), provider.ConsumeIntegral<uint8_t>());
    service.Disconnect(provider.ConsumeIntegral<int>(), BuildRawAddress(provider), provider.ConsumeIntegral<uint8_t>());
    service.Connect(BuildRawAddress(provider));
    service.Disconnect(BuildRawAddress(provider));
    service.GetConnectDevices();
    service.GetConnectState();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::SSAP_FUZZ_DELAY_50_MS));
}

/* 尾部追加段：Enable 后走真实注册（NLSTK_SsapServerRegApp），成功则以真 appId
 * 驱动服务段+静态 On* 代表（g_ssapServerStackAdapter 指向本 service 的 adapter，
 * 回调落回 AdapterCallback→appSafeMap 命中→mock 观测；授权回调记录 requestId 后
 * AuthorizeResponse 回闭环）；失败保 -1 直接跳过 */
void RealRegisterServerFuzzTest(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    std::shared_ptr<SsapServerObserveFuzzCallback> cb = std::make_shared<SsapServerObserveFuzzCallback>();
    /* static：On* 投递任务按值捕获 g_ssapServerStackAdapter（指向本 service 的 adapter 成员），
     * 栈对象析构后残余任务即悬垂（红队 M3）；进程常驻消除窗口，析构排空 sleep 保留兜底 */
    static SsapServerService service;
    service.EnableTask();
    int appId = service.RegisterApplicationTask(cb, 1, 1);
    if (appId < 0) {
        return;
    }
    struct Service fuzzService = {};
    fuzzService.handle_ = provider.ConsumeIntegral<uint16_t>();
    fuzzService.startHandle_ = fuzzService.handle_;
    fuzzService.endHandle_ = static_cast<uint16_t>(fuzzService.handle_ + 8);
    fuzzService.isPrimary_ = provider.ConsumeBool();
    fuzzService.uuid_ = BuildUuid(provider);
    struct Property fuzzProperty = {};
    fuzzProperty.handle_ = static_cast<uint16_t>(fuzzService.handle_ + 1);
    fuzzProperty.uuid_ = BuildUuid(provider);
    fuzzProperty.permission_ = provider.ConsumeIntegral<uint16_t>();
    fuzzService.properties_.push_back(fuzzProperty);
    service.AddServiceTask(appId, fuzzService);
    service.CheckServiceExistByUuidTask(appId, BuildUuid(provider));
    struct Property setProperty = {};
    setProperty.handle_ = fuzzProperty.handle_;
    service.SetPropertyValueTask(appId, setProperty);
    service.GetPropertyValue(appId, setProperty);

    SLE_Addr_S peer = {};
    for (size_t i = 0; i < SLE_ADDR_LEN; i++) {
        peer.addr[i] = provider.ConsumeIntegral<uint8_t>();
    }
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
        stackProp->handle = fuzzProperty.handle_;
        stackProp->uuid = g_serverFuzzUuid;
        stackProp->descriptors = descVec;
        (void)SDF_VectorEmplaceBack(propVec, stackProp);
        SSAP_Service_S stackService = {};
        stackService.appId = appId;
        stackService.handle = fuzzService.handle_;
        stackService.endHandle = fuzzService.endHandle_;
        stackService.uuid = g_serverFuzzUuid;
        stackService.properties = propVec;
        SsapServerStackAdapter::OnAddService(appId, &stackService, NLSTK_ERRCODE_SUCCESS);
    }
    SDF_DestroyVector(propVec);
    SDF_MemFree(stackProp);
    SDF_DestroyVector(descVec);
    SDF_MemFree(stackDesc);

    uint8_t valBuf[SSAP_SERVER_FUZZ_VAL_LEN] = {};
    for (size_t i = 0; i < sizeof(valBuf); i++) {
        valBuf[i] = provider.ConsumeIntegral<uint8_t>();
    }
    NLSTK_SsapServerOnSetPropertyParam_S setParam = {};
    setParam.handle = fuzzProperty.handle_;
    setParam.uuid = g_serverFuzzUuid;
    setParam.value.len = sizeof(valBuf);
    setParam.value.data = valBuf;
    SsapServerStackAdapter::OnSetPropertyValue(appId, &setParam, NLSTK_ERRCODE_SUCCESS);
    SsapServerStackAdapter::OnConnectionStateChanged(appId, &peer, provider.ConsumeIntegral<uint8_t>(),
        NLSTK_ERRCODE_SUCCESS, provider.ConsumeIntegral<int>());
    NLSTK_SsapServerReadPropertyInfo_S readParam = {};
    readParam.addr = peer;
    readParam.uuid = g_serverFuzzUuid;
    readParam.handle = fuzzProperty.handle_;
    uint16_t requestId = provider.ConsumeIntegral<uint16_t>();
    SsapServerStackAdapter::OnReadPropertyAuthorizeRequest(appId, requestId, &readParam);
    /* 授权回闭环：requestId 与下发值一致（mock 侧异步记录仅作观测） */
    service.AuthorizeResponseTask(appId, requestId, provider.ConsumeBool());

    service.NotifyPropertyTask(appId, setProperty, BuildRawAddress(provider));
    service.IndicateProperty(appId, setProperty, BuildRawAddress(provider), provider.ConsumeIntegral<uint8_t>());
    service.RemoveServiceTask(appId, fuzzService.handle_);
    service.ClearServicesTask(appId);
    service.DeregisterApplicationTask(appId);
    /* 既有段直调注册成功后无人注销，栈侧 64 槽会跨输入泄漏耗尽使真实注册恒败；
     * 全量清扫残留（test-only，进程内 server app 仅本驱动持有） */
    for (int id = 0; id < NLSTK_SSAP_SERVER_APP_MAX_NUM; id++) {
        NLSTK_SsapServerDeregisterApplication(id);
    }
    /* 排空 DoInSsapThread 已投任务，避免 service 栈对象析构后任务悬挂 */
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::SSAP_FUZZ_DELAY_50_MS));
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
        OHOS::g_threadUtil.ClearThreadStateMap();
        return 0;
    }
    OHOS::g_threadUtil.InitThreadStateMap();
    OHOS::DoSomethingSsapServiceServerWithMyAPI(data, size);
    OHOS::RealRegisterServerFuzzTest(data, size);
    OHOS::g_threadUtil.ClearThreadStateMap();
    return 0;
}

