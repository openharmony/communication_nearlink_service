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
#include "ssapservicesclient_fuzzer.h"
#include "../../nl_utils/fuzztest_utils.h"
#include "ClassCreator.h"
#include "ssap_service_base.h"
#include "log.h"
#include "securec.h"
#include "slem.h"
#include "raw_address.h"
#include "ssap_client_service.h"
#include "ThreadUtil.h"
#include "sdf_thread.h"
#include "sdf_evc.h"
#include "nlstk_schedule.h"

/* On* 静态入口为 private，须在 private=public 区域内首次包含该头（与
 * ssapclient_fuzzer.cpp 同款写法） */
#define private public
#include "ssap_client_stack_adapter.h"
#undef private

using namespace std;
using namespace OHOS::Nearlink;
namespace OHOS {
ThreadUtil &g_threadUtil = ThreadUtil::GetInstance();
constexpr int SSAP_CLIENT_FUZZ_DELAY_50_MS = 50;
constexpr size_t SSAP_CLIENT_FUZZ_VAL_LEN = 8;

namespace {
NLSTK_SsapUuid_S g_clientFuzzUuid = {.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02}};
}
class InterfaceSsapClientCb : public InterfaceSsapClientCallback {
public:
    void OnMtuChanged(uint16_t mtu, int ret) override {}
    void OnDiscoverComplete(int ret) override {}
    void OnDiscoverByUuidComplete(const Uuid &uuid, int ret) override {}
    void OnReadProperty(Property &property, int ret) override {}
    void OnReadDescriptor(Descriptor &descriptor, int ret) override {}
    void OnReadPropertiesByUuid(std::list<Property> &list, int ret) override {}
    void OnReadDescriptorsByUuid(std::list<Descriptor> &list, int ret) override {}
    void OnWriteProperty(Property &property, int ret) override {}
    void OnWriteDescriptor(Descriptor &descriptor, int ret) override {}
    void OnGetPropertyNotification(const Property &property, bool enable, int ret) override {}
    void OnGetPropertyIndication(const Property &property, bool enable, int ret) override {}
    void OnSetPropertyNotification(const Property &property, bool enable, int ret) override {}
    void OnSetEventNotification(const Event &event, bool enable, int ret) override {}
    void OnSetPropertyIndication(const Property &property, bool enable, int ret) override {}
    void OnSetEventIndication(const Event &event, bool enable, int ret) override {}
    void OnPropertyChanged(const Property &property) override {}
    void OnEvent(const Event &event) override {}
    void OnCallMethod(Method &method, int ret) override {}
    void OnConnectionStateChanged(uint8_t state, int ret) override {}
};

/* 真实注册后观测 service AdapterCallback 下发的回调（其余虚函数沿用基类空实现） */
class SsapClientObserveFuzzCallback : public InterfaceSsapClientCallback {
public:
    void OnMtuChanged(uint16_t mtu, int ret) override { mtu_ = mtu; errCode_ = ret; }
    void OnConnectionStateChanged(uint8_t state, int ret) override { state_ = state; errCode_ = ret; }
    void OnReadProperty(Property &property, int ret) override { handle_ = property.handle_; errCode_ = ret; }
    void OnWriteProperty(Property &property, int ret) override { handle_ = property.handle_; errCode_ = ret; }
    void OnPropertyChanged(const Property &property) override { handle_ = property.handle_; }

    uint16_t mtu_ = 0;
    uint8_t state_ = 0;
    uint16_t handle_ = 0;
    int errCode_ = 0;
};

void DoSomethingSsapServiceClientWithMyAPI(const uint8_t* fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    struct Property pro;
    struct Descriptor des;
    struct Event event;
    struct Method me;
    std::vector<uint8_t> value;
    std::shared_ptr<InterfaceSsapClientCallback> cb = std::make_shared<InterfaceSsapClientCb>();
    SsapClientService clientService;
    clientService.EnableTask();
    SsapSecureType sec = static_cast<SsapSecureType>(provider.ConsumeIntegral<uint8_t>());
    clientService.RegisterApplicationTask(cb, BuildRawAddress(provider), sec, 1, 1);
    clientService.DeregisterApplicationTask(provider.ConsumeIntegral<int>());
    clientService.ExchangeMtuTask(provider.ConsumeIntegral<int>(), provider.ConsumeIntegral<uint16_t>());
    clientService.DiscoverServicesTask(provider.ConsumeIntegral<int>());
    clientService.DiscoverServicesByUuidTask(provider.ConsumeIntegral<int>(), BuildUuid(provider),
        provider.ConsumeIntegral<uint16_t>(), provider.ConsumeIntegral<uint16_t>());
    clientService.GetServicesTask(provider.ConsumeIntegral<int>());
    clientService.GetServicesByUuidTask(provider.ConsumeIntegral<int>(), BuildUuid(provider));
    clientService.ReadPropertyTask(provider.ConsumeIntegral<int>(), pro);
    clientService.ReadDescriptorTask(provider.ConsumeIntegral<int>(), des);
    clientService.ReadPropertiesByUuidTask(provider.ConsumeIntegral<int>(), BuildUuid(provider),
        provider.ConsumeIntegral<uint16_t>(), provider.ConsumeIntegral<uint16_t>());
    clientService.ReadDescriptorsByUuid(provider.ConsumeIntegral<int>(), BuildUuid(provider),
        provider.ConsumeIntegral<uint16_t>(), provider.ConsumeIntegral<uint16_t>(),
        provider.ConsumeIntegral<uint16_t>());
    clientService.WritePropertyTask(provider.ConsumeIntegral<int>(), pro, provider.ConsumeBool());
    clientService.WriteDescriptorTask(provider.ConsumeIntegral<int>(), des, provider.ConsumeBool());
    clientService.GetPropertyNotificationTask(provider.ConsumeIntegral<int>(), pro);
    clientService.SetPropertyNotificationTask(provider.ConsumeIntegral<int>(), pro, provider.ConsumeBool());
    clientService.SetEventNotification(provider.ConsumeIntegral<int>(), event, provider.ConsumeBool());
    clientService.SetPropertyIndicationTask(provider.ConsumeIntegral<int>(), pro, provider.ConsumeBool());
    clientService.SetEventIndication(provider.ConsumeIntegral<int>(), event, provider.ConsumeBool());
    clientService.CallMethodTask(provider.ConsumeIntegral<int>(), me, provider.ConsumeBool());
    clientService.ConnectTask(provider.ConsumeIntegral<int>(), provider.ConsumeBool());
    clientService.DisconnectTask(provider.ConsumeIntegral<int>());
}

/* 尾部追加段：Enable 后走真实注册（NLSTK_SsapClientRegApp），成功则以真 appId
 * 驱动依赖段并经静态 On*（g_ssapClientStackAdapter 指向本 service 的 adapter）
 * 让回调落回 AdapterCallback→appSafeMap 命中→mock 观测；失败保 -1 直接跳过 */
void RealRegisterAndObserveFuzzTest(const uint8_t* fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    std::shared_ptr<SsapClientObserveFuzzCallback> cb = std::make_shared<SsapClientObserveFuzzCallback>();
    /* static：On* 投递任务按值捕获 g_ssapClientStackAdapter（指向本 service 的 adapter 成员），
     * 栈对象析构后残余任务即悬垂（红队 M3）；进程常驻消除窗口 */
    static SsapClientService clientService;
    clientService.EnableTask();
    SsapSecureType sec = static_cast<SsapSecureType>(provider.ConsumeIntegral<uint8_t>() % 2);
    int appId = clientService.RegisterApplicationTask(cb, BuildRawAddress(provider), sec, 1, 1);
    if (appId < 0) {
        return;
    }
    struct Property pro = {};
    pro.handle_ = provider.ConsumeIntegral<uint16_t>();
    struct Method me = {};
    me.handle_ = provider.ConsumeIntegral<uint16_t>();
    clientService.ReadPropertyTask(appId, pro);
    clientService.WritePropertyTask(appId, pro, provider.ConsumeBool());
    clientService.GetPropertyNotificationTask(appId, pro);
    clientService.SetPropertyNotificationTask(appId, pro, provider.ConsumeBool());
    clientService.ExchangeMtuTask(appId, provider.ConsumeIntegral<uint16_t>());
    clientService.DiscoverServicesTask(appId);
    clientService.GetServicesTask(appId);
    clientService.CallMethodTask(appId, me, provider.ConsumeBool());

    SsapClientStackAdapter::OnMtuChanged(appId, provider.ConsumeIntegral<uint16_t>(), NLSTK_ERRCODE_SUCCESS);
    SsapClientStackAdapter::OnConnectionStateChanged(appId, provider.ConsumeIntegral<uint8_t>(),
        NLSTK_ERRCODE_SUCCESS, provider.ConsumeIntegral<int>());
    uint8_t valBuf[SSAP_CLIENT_FUZZ_VAL_LEN] = {};
    for (size_t i = 0; i < sizeof(valBuf); i++) {
        valBuf[i] = provider.ConsumeIntegral<uint8_t>();
    }
    NLSTK_SsapClientReadPropertyInfo_S info = {};
    info.handle = pro.handle_;
    info.uuid = g_clientFuzzUuid;
    info.value.len = sizeof(valBuf);
    info.value.data = valBuf;
    SsapClientStackAdapter::OnReadProperty(appId, &info, NLSTK_ERRCODE_SUCCESS);
    SsapClientStackAdapter::OnWriteProperty(appId, &info, NLSTK_ERRCODE_SUCCESS);
    SsapClientStackAdapter::OnPropertyChanged(appId, &info);

    clientService.DisconnectTask(appId);
    clientService.DeregisterApplicationTask(appId);
    /* 既有段直调注册成功后无人注销，栈侧 50 槽会跨输入泄漏耗尽使真实注册恒败；
     * 全量清扫残留（test-only，进程内 client app 仅本驱动持有） */
    for (int id = 0; id < NLSTK_SSAP_CLIENT_APP_MAX_NUM; id++) {
        NLSTK_SsapClientDeregApp(id);
    }
    /* 排空 DoInSsapThread 已投任务，避免 service 栈对象析构后任务悬挂 */
    std::this_thread::sleep_for(std::chrono::milliseconds(SSAP_CLIENT_FUZZ_DELAY_50_MS));
}
}

// fuzzer init
extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void) argc;
    (void) argv;
    int ret = slem_initialize();
    HILOGI("slem_initialize %{public}d", ret);
    /* 仅 slem_initialize 时缺调度三件套，NLSTK_SsapClientRegApp 的
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
    /* DoInSsapThread 依赖 ssap 线程态（未初始化时任务被静默丢弃），
     * 与 ssapservicesserver_fuzzer 同款首尾配对 */
    OHOS::g_threadUtil.InitThreadStateMap();
    OHOS::DoSomethingSsapServiceClientWithMyAPI(data, size);
    OHOS::RealRegisterAndObserveFuzzTest(data, size);
    OHOS::g_threadUtil.ClearThreadStateMap();
    return 0;
}

