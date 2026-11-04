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
#include "ssap_inner_def.h"
#include "ThreadUtil.h"
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

// D-3 PR9：公共 API 错误码回传链 + exchangeInfo(ExchangeMtu)后续态 + notify/indicate 订阅族 + future 族。
// 消费序：本函数为新增函数，消费序独立；所有 FDP 值在函数头部一次性消费，驱动段零消费。
void SsapClientServiceApiFuzzTest(const uint8_t *fuzzData, size_t size)
{
    // future 族前置（纪律-2）：Register/Deregister/GetServices 走 promise.get_future().get()，
    // SsapThread 与 Connect 链的 DeviceAdapterThread 状态表必须 ENABLED（map 空=DISABLED=投递被丢=future 永悬）。
    // 本 target 无人调用 ClearThreadStateMap，Init 幂等兜底；新代码绝不调 Clear。
    ThreadUtil::GetInstance().InitThreadStateMap();
    FuzzedDataProvider provider(fuzzData, size);
    std::shared_ptr<InterfaceSsapClientCallback> cb = std::make_shared<InterfaceSsapClientCb>();
    // 泄漏式永生实例（R9/R10 判例）：公共 API 的 fire-and-forget DoInSsapThread lambda 捕获 this，
    // 栈实例析构后 lambda 执行即悬垂；且析构会把 g_ssapClientStackAdapter 置空、打断后续输入。
    static SsapClientService *svc = new SsapClientService();

    const RawAddress addr = BuildRawAddress(provider); // 尾部 6 字节倒序
    const uint8_t transport = provider.ConsumeIntegral<uint8_t>() % SSAP_TRANSPORT_MAX; // SsapTransportType 2 成员
    const uint8_t secure = provider.ConsumeIntegral<uint8_t>() % SSAP_SEC_MAX; // SsapSecureType 2 成员
    const uint8_t valLen = provider.ConsumeIntegral<uint8_t>() % 9; // 0=空值分支
    const std::vector<uint8_t> valVec = provider.ConsumeBytes<uint8_t>(valLen); // 头部
    const uint16_t handle = provider.ConsumeIntegral<uint16_t>();
    const Uuid uuid = BuildUuid(provider); // 头部 16 字节
    const uint8_t descType = provider.ConsumeIntegral<uint8_t>() % 5; // DESC_TYPE 5 成员(0-4)
    const uint16_t mtu = static_cast<uint16_t>(SSAP_MTU_DEFAULT + provider.ConsumeIntegral<uint8_t>() % 774); // 251..1024
    const bool enable = provider.ConsumeBool();
    const bool withoutRsp = provider.ConsumeBool();
    const bool autoConnect = provider.ConsumeBool();
    // 整改 P9-7：错误码/越界档改 FDP 全域消费（追加尾部、旧序不动；边界语义对下沉种子）
    const uint8_t badTransport = provider.ConsumeIntegral<uint8_t>(); // 全域，含 >= SSAP_TRANSPORT_MAX 档
    const uint8_t badSecure = provider.ConsumeIntegral<uint8_t>(); // 全域，含 >= SSAP_SEC_MAX 档
    const uint16_t lowMtu = provider.ConsumeIntegral<uint16_t>(); // 全域，含 < 251 档
    const uint16_t badMtu = provider.ConsumeIntegral<uint16_t>(); // 全域，含 > 1024 档
    const uint8_t badDescTypeA = provider.ConsumeIntegral<uint8_t>(); // 全域，含 RESERVE 档
    const uint8_t badDescTypeB = provider.ConsumeIntegral<uint8_t>(); // 全域，含 CLIENT_CONFIG 档
    const uint16_t startHA = provider.ConsumeIntegral<uint16_t>();
    const uint16_t endHA = provider.ConsumeIntegral<uint16_t>();
    const uint16_t startHB = provider.ConsumeIntegral<uint16_t>();
    const uint16_t endHB = provider.ConsumeIntegral<uint16_t>();
    Property emptyProp(handle, uuid); // value_ 为空 → 适配层 INVALID_PARAM 回传
    Property valProp(handle, uuid, std::vector<uint8_t>(valVec));
    Descriptor emptyDesc(handle, descType);
    Descriptor valDesc(handle, descType, std::vector<uint8_t>(valVec));
    Method emptyMethod(handle); // parameter_ 为空 → 适配层 INVALID_PARAM 回传
    Method valMethod(handle, valVec);

    // ---- 未使能窗口：全 API SSAP_NOT_ENABLE 错误码回传 ----
    svc->DisableTask(); // 幂等窗口重置（未运行时走 OnDisable 通知分支）
    (void)svc->RegisterApplication(cb, addr, transport, secure);
    (void)svc->DeregisterApplication(0);
    (void)svc->ExchangeMtu(0, mtu);
    (void)svc->DiscoverServices(0);
    (void)svc->DiscoverServicesByUuid(0, uuid, 1, 0xFFFF);
    (void)svc->GetServices(0);
    (void)svc->GetServicesByUuid(0, uuid);
    (void)svc->ReadProperty(0, emptyProp);
    (void)svc->ReadDescriptor(0, emptyDesc);
    (void)svc->ReadPropertiesByUuid(0, uuid, 1, 0xFFFF);
    (void)svc->ReadDescriptorsByUuid(0, uuid, descType, 1, 0xFFFF); // NOT_SUPPORT 桩
    (void)svc->WriteProperty(0, emptyProp, withoutRsp);
    (void)svc->WriteDescriptor(0, emptyDesc, withoutRsp);
    (void)svc->GetPropertyNotification(0, emptyProp);
    (void)svc->SetPropertyNotification(0, emptyProp, enable);
    (void)svc->SetEventNotification(0, Event(handle, uuid), enable); // NOT_SUPPORT 桩
    (void)svc->SetPropertyIndication(0, emptyProp, enable);
    (void)svc->SetEventIndication(0, Event(handle, uuid), enable); // NOT_SUPPORT 桩
    (void)svc->CallMethod(0, emptyMethod, withoutRsp);
    (void)svc->Connect(0, autoConnect);
    (void)svc->Disconnect(0);

    // ---- 已使能 + 非法参数：SSAP_INVALID_PARAM 回传 ----
    svc->EnableTask();
    (void)svc->RegisterApplication(nullptr, addr, transport, secure); // 空 callback 档
    (void)svc->RegisterApplication(cb, addr, badTransport, secure); // transport 越界档（FDP 全域）
    (void)svc->RegisterApplication(cb, addr, transport, badSecure); // secure 越界档（FDP 全域）
    (void)svc->DeregisterApplication(-1); // IsValidAppId 负值档
    (void)svc->ExchangeMtu(0, lowMtu); // mtu 全域（< 251 档由种子/变异命中）
    (void)svc->ExchangeMtu(0, badMtu); // mtu 全域（> 1024 档由种子/变异命中）
    (void)svc->DiscoverServicesByUuid(0, uuid, startHA, endHA); // startHandle 越界/倒挂档（FDP 全域）
    (void)svc->DiscoverServicesByUuid(0, uuid, startHB, endHB); // startHandle 越界/倒挂档（FDP 全域）
    Descriptor descA(handle, badDescTypeA); // 非常量引用形参不可绑临时，改局部变量
    (void)svc->ReadDescriptor(0, descA); // type 全域（含 RESERVE 档）
    Descriptor descB(handle, badDescTypeB);
    (void)svc->WriteDescriptor(0, descB, withoutRsp); // type 全域（含 CLIENT_CONFIG 档）
    (void)svc->ReadPropertiesByUuid(-1, uuid, 1, 0xFFFF);
    (void)svc->CallMethod(-1, emptyMethod, withoutRsp);
    (void)svc->Connect(-1, autoConnect);
    (void)svc->Disconnect(-1);

    // ---- 注册成功态：真 appId 驱动 task 层命中分支（future 族：Register/Deregister/GetServices）----
    const int appId = svc->RegisterApplication(cb, addr, transport, secure);
    if (appId >= 0) {
        (void)svc->ExchangeMtu(appId, mtu); // exchangeInfo(MTU 交换)链：service→adapter→NLSTK_SsapClientExchangeMtu
        (void)svc->DiscoverServices(appId);
        (void)svc->DiscoverServicesByUuid(appId, uuid, 1, 0xFFFF);
        std::list<Service> services = svc->GetServices(appId);
        services = svc->GetServicesByUuid(appId, uuid);
        (void)services;
        (void)svc->ReadProperty(appId, emptyProp);
        (void)svc->ReadDescriptor(appId, emptyDesc);
        (void)svc->ReadPropertiesByUuid(appId, uuid, 1, 0xFFFF);
        // 订阅族：notify/indication enable/disable 两档
        (void)svc->GetPropertyNotification(appId, emptyProp);
        (void)svc->SetPropertyNotification(appId, emptyProp, enable);
        (void)svc->SetPropertyIndication(appId, emptyProp, enable);
        // 写/调用族：空值(INVALID_PARAM 回传)与带值两条链
        Property p1 = emptyProp;
        (void)svc->WriteProperty(appId, p1, withoutRsp);
        Property p2 = valProp;
        (void)svc->WriteProperty(appId, p2, withoutRsp);
        Descriptor d1 = emptyDesc;
        (void)svc->WriteDescriptor(appId, d1, withoutRsp);
        Descriptor d2 = valDesc;
        (void)svc->WriteDescriptor(appId, d2, withoutRsp);
        Method m1 = emptyMethod;
        (void)svc->CallMethod(appId, m1, withoutRsp);
        Method m2 = valMethod;
        (void)svc->CallMethod(appId, m2, withoutRsp);
        (void)svc->Connect(appId, autoConnect);
        (void)svc->Disconnect(appId);
        // 注销：命中分支 + 二次注销 SSAP_NO_APP
        (void)svc->DeregisterApplication(appId);
        (void)svc->DeregisterApplication(appId);
    } else {
        // 注册失败档：appId 保持非法值，task 层走 no-app 分支
        (void)svc->ExchangeMtu(appId, mtu);
        (void)svc->DiscoverServices(appId);
    }

    // ---- NOT_SUPPORT / 查询桩 ----
    (void)svc->Connect(addr);
    (void)svc->Disconnect(addr);
    std::list<RawAddress> devices = svc->GetConnectDevices();
    (void)devices;
    (void)svc->GetConnectState();
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
    OHOS::SsapClientServiceApiFuzzTest(data, size);
    OHOS::RealRegisterAndObserveFuzzTest(data, size);
    OHOS::g_threadUtil.ClearThreadStateMap();
    return 0;
}

