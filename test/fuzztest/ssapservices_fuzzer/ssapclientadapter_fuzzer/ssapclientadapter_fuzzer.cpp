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
#include "ssapclientadapter_fuzzer.h"
#include "../../nl_utils/fuzztest_utils.h"
#include "ssap_def.h"
#include "log.h"
#include "securec.h"
#include "raw_address.h"
#include "ThreadUtil.h"
#include "sdf_thread.h"
#include "sdf_evc.h"
#include "nlstk_schedule.h"

/* On* 静态入口为 private，须在 private=public 区域内包含该头（与
 * ssapclient_fuzzer.cpp 同款写法） */
#define private public
#include "ssap_client_stack_adapter.h"
#undef private
#include "ThreadUtil.h"
#include "slem.h"

using namespace std;
using namespace OHOS::Nearlink;
namespace OHOS {
ThreadUtil &g_threadUtil = ThreadUtil::GetInstance();
size_t FUZZ_DATA_SIZE = 50;
constexpr size_t SSAP_CLIENT_ADAPTER_FUZZ_VAL_LEN = 8;

namespace {
NLSTK_SsapUuid_S g_clientAdapterUuid = {.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02}};
}

#define DECODE2BYTE(_ptr) (uint16_t)(*(uint8_t *)(_ptr) | (*(uint8_t *)((_ptr) + 1) << 8))
#define DECODE4BYTE(_ptr) \
    (uint32_t)(*(uint8_t *)(_ptr) | (*(uint8_t *)((_ptr) + 1) << 8) \
    | (*(uint8_t *)((_ptr) + 2) << 16) | (*(uint8_t *)((_ptr) + 3) << 24))

class SsapClientStackCallbackFuzzMock : public SsapClientStackCallback {
public:
    void OnMtuChanged(int appId, uint16_t mtu, int ret) override {}
    void OnDiscoverComplete(int appId, int ret) override {}
    void OnDiscoverByUuidComplete(int appId, const Uuid &uuid, int ret) override {}
    void OnReadProperty(int appId, Property &property, int ret) override {}
    void OnReadDescriptor(int appId, Descriptor &descriptor, int ret) override {}
    void OnReadPropertiesByUuid(int appId, std::list<Property> &properties, int ret) override {}
    void OnWriteProperty(int appId, Property &property, int ret) override {}
    void OnWriteDescriptor(int appId, Descriptor &descriptor, int ret) override {}
    void OnGetPropertyNotification(int appId, const Property &property, bool enable, int ret) override {}
    void OnGetPropertyIndication(int appId, const Property &property, bool enable, int ret) override {}
    void OnSetPropertyNotification(int appId, const Property &property, bool enable, int ret) override {}
    void OnSetPropertyIndication(int appId, const Property &property, bool enable, int ret) override {}
    void OnPropertyChanged(int appId, const Property &property) override {}
    void OnConnectionStateChanged(int appId, int state, int ret, int reason) override {}
};

// extern struct SsapClientStackAdapter::impl;
static SsapClientStackCallbackFuzzMock cb;
static SsapClientStackAdapter adapter(cb);

void SsapClientAdapterApiFuzzTest(const uint8_t* data, size_t size)
{
    FuzzedDataProvider provider(data, size);

    int appid = provider.ConsumeIntegral<int>();
    adapter.RegisterApplication(appid, BuildRawAddress(provider), SSAP_SEC_ENCRYPT);

    adapter.DeregisterApplication(provider.ConsumeIntegral<int>());
    adapter.ExchangeMtu(provider.ConsumeIntegral<int>(), provider.ConsumeIntegral<uint16_t>());
    adapter.DiscoverServices(provider.ConsumeIntegral<int>());
    adapter.DiscoverServicesByUuid(provider.ConsumeIntegral<int>(), BuildUuid(provider),
        provider.ConsumeIntegral<uint16_t>(), provider.ConsumeIntegral<uint16_t>());

    std::list<Service> service1 = adapter.GetServices(provider.ConsumeIntegral<int>());
    std::list<Service> service2 = adapter.GetServicesByUuid(provider.ConsumeIntegral<int>(), BuildUuid(provider));

    adapter.ReadPropertiesByUuid(provider.ConsumeIntegral<int>(), BuildUuid(provider),
        provider.ConsumeIntegral<uint16_t>(), provider.ConsumeIntegral<uint16_t>());
    adapter.Connect(provider.ConsumeIntegral<int>(), BuildRawAddress(provider), provider.ConsumeBool());
    adapter.Disconnect(provider.ConsumeIntegral<int>(), BuildRawAddress(provider));

    struct Property pro = {};
    pro.handle_ = provider.ConsumeIntegral<uint16_t>();

    adapter.ReadProperty(provider.ConsumeIntegral<int>(), pro);
    adapter.WriteProperty(provider.ConsumeIntegral<int>(), pro, provider.ConsumeBool());
    adapter.GetPropertyNotification(provider.ConsumeIntegral<int>(), pro);
    adapter.SetPropertyNotification(provider.ConsumeIntegral<int>(), pro, provider.ConsumeBool());
    adapter.GetPropertyIndication(provider.ConsumeIntegral<int>(), pro);
    adapter.SetPropertyIndication(provider.ConsumeIntegral<int>(), pro, provider.ConsumeBool());

    struct Descriptor des = {};
    des.handle_ = provider.ConsumeIntegral<uint16_t>();

    adapter.ReadDescriptor(provider.ConsumeIntegral<int>(), des);
    adapter.WriteDescriptor(provider.ConsumeIntegral<int>(), des, provider.ConsumeBool());

}

/* 尾部追加段：真实注册（栈侧登记静态回调表，appId 由栈分配）成功后，
 * 以真 appId 驱动 SsapClientStackAdapter::On* 静态入口 15 个全集——经
 * DoInSsapThread 落到 On*Task→文件级 adapter→SsapClientStackCallback mock，
 * 覆盖 On*Task 分派内部；失败保 -1 跳过依赖段 */
void ClientAdapterCallbacksFuzzTest(const uint8_t* data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    int appId = -1;
    SsapSecureType sec = static_cast<SsapSecureType>(provider.ConsumeIntegral<uint8_t>() % 2);
    adapter.RegisterApplication(appId, BuildRawAddress(provider), sec);
    if (appId < 0) {
        return;
    }
    uint8_t valBuf[SSAP_CLIENT_ADAPTER_FUZZ_VAL_LEN] = {};
    for (size_t i = 0; i < sizeof(valBuf); i++) {
        valBuf[i] = provider.ConsumeIntegral<uint8_t>();
    }
    /* 四个 *Info_S 均为同一结构体的多别名 typedef，info/desc 可复用 */
    NLSTK_SsapClientReadPropertyInfo_S info = {};
    info.handle = provider.ConsumeIntegral<uint16_t>();
    info.uuid = g_clientAdapterUuid;
    info.value.len = sizeof(valBuf);
    info.value.data = valBuf;
    NLSTK_SsapClientReadDescriptorInfo_S desc = {};
    desc.handle = provider.ConsumeIntegral<uint16_t>();
    desc.type = provider.ConsumeIntegral<uint8_t>() % DESC_TYPE_MAX;
    desc.uuid = g_clientAdapterUuid;
    desc.value.len = sizeof(valBuf);
    desc.value.data = valBuf;

    SsapClientStackAdapter::OnMtuChanged(appId, provider.ConsumeIntegral<uint16_t>(), provider.ConsumeBool() ? NLSTK_ERRCODE_SUCCESS : NLSTK_ERRCODE_PARAM_ERR);
    SsapClientStackAdapter::OnDiscoverComplete(appId, provider.ConsumeBool() ? NLSTK_ERRCODE_SUCCESS : NLSTK_ERRCODE_PARAM_ERR);
    SsapClientStackAdapter::OnDiscoverByUuidComplete(appId, &g_clientAdapterUuid, provider.ConsumeBool() ? NLSTK_ERRCODE_SUCCESS : NLSTK_ERRCODE_PARAM_ERR);
    SsapClientStackAdapter::OnConnectionStateChanged(appId, provider.ConsumeIntegral<uint8_t>(),
        provider.ConsumeBool() ? NLSTK_ERRCODE_SUCCESS : NLSTK_ERRCODE_PARAM_ERR, provider.ConsumeIntegral<int>());
    SsapClientStackAdapter::OnReadProperty(appId, &info, provider.ConsumeBool() ? NLSTK_ERRCODE_SUCCESS : NLSTK_ERRCODE_PARAM_ERR);
    SsapClientStackAdapter::OnCallMethod(appId, &info, provider.ConsumeBool() ? NLSTK_ERRCODE_SUCCESS : NLSTK_ERRCODE_PARAM_ERR);
    SsapClientStackAdapter::OnReadDescriptor(appId, &desc, provider.ConsumeBool() ? NLSTK_ERRCODE_SUCCESS : NLSTK_ERRCODE_PARAM_ERR);
    SsapClientStackAdapter::OnWriteProperty(appId, &info, provider.ConsumeBool() ? NLSTK_ERRCODE_SUCCESS : NLSTK_ERRCODE_PARAM_ERR);
    SsapClientStackAdapter::OnWriteDescriptor(appId, &desc, provider.ConsumeBool() ? NLSTK_ERRCODE_SUCCESS : NLSTK_ERRCODE_PARAM_ERR);
    SsapClientStackAdapter::OnGetPropertyNotification(appId, &g_clientAdapterUuid,
        info.handle, provider.ConsumeBool(), provider.ConsumeBool() ? NLSTK_ERRCODE_SUCCESS : NLSTK_ERRCODE_PARAM_ERR);
    SsapClientStackAdapter::OnGetPropertyIndication(appId, &g_clientAdapterUuid,
        info.handle, provider.ConsumeBool(), provider.ConsumeBool() ? NLSTK_ERRCODE_SUCCESS : NLSTK_ERRCODE_PARAM_ERR);
    SsapClientStackAdapter::OnSetPropertyNotification(appId, &g_clientAdapterUuid,
        info.handle, provider.ConsumeBool(), provider.ConsumeBool() ? NLSTK_ERRCODE_SUCCESS : NLSTK_ERRCODE_PARAM_ERR);
    SsapClientStackAdapter::OnSetPropertyIndication(appId, &g_clientAdapterUuid,
        info.handle, provider.ConsumeBool(), provider.ConsumeBool() ? NLSTK_ERRCODE_SUCCESS : NLSTK_ERRCODE_PARAM_ERR);
    SsapClientStackAdapter::OnPropertyChanged(appId, &info);
    SsapClientStackAdapter::OnEvent(appId, &info);

    adapter.DeregisterApplication(appId);
    /* 既有段直调注册成功后无人注销，栈侧 50 槽会跨输入泄漏耗尽使真实注册恒败；
     * 全量清扫残留（test-only，进程内 client app 仅本驱动持有）。adapter 与
     * mock 均为文件级静态对象，已投任务不会悬挂 */
    for (int id = 0; id < NLSTK_SSAP_CLIENT_APP_MAX_NUM; id++) {
        NLSTK_SsapClientDeregApp(id);
    }
}

// D-3 PR9：带值/空值写链（WriteProperty/WriteDescriptor/CallMethod 的 INVALID_PARAM 回传与 NLSTK 正常链）
// + Disable/OnDisable 清理链。本函数为新增函数，消费序独立；FDP 值头部一次性消费，驱动段零消费。
void SsapClientAdapterValueApiFuzzTest(const uint8_t *data, size_t size)
{
    // Disable → NLSTK_SsapCleanClientApp(OnDisable)：OnDisable 静态分发走 DoInSsapThread，状态表必须 ENABLED。
    ThreadUtil::GetInstance().InitThreadStateMap();
    FuzzedDataProvider provider(data, size);

    // RegisterApplication 形参为非 const 引用（栈侧重写真实 appId），不可声明 const（红蓝 R1）
    int appId = provider.ConsumeIntegral<int>();
    const uint16_t handle = provider.ConsumeIntegral<uint16_t>();
    const uint8_t valLen = provider.ConsumeIntegral<uint8_t>() % 9; // 0=空值分支
    const std::vector<uint8_t> valVec = provider.ConsumeBytes<uint8_t>(valLen); // 头部
    const uint8_t descType = provider.ConsumeIntegral<uint8_t>() % 5; // DESC_TYPE 5 成员(0-4)
    const bool withoutRsp = provider.ConsumeBool();
    const RawAddress addr = BuildRawAddress(provider); // 尾部 6 字节倒序

    adapter.RegisterApplication(appId, addr, SSAP_SEC_ENCRYPT);

    // CallMethod：parameter_ 为空 → OnCallMethodTask(SSAP_INVALID_PARAM) 回传；带值 → NLSTK 调用链
    Method emptyMethod(handle);
    adapter.CallMethod(appId, emptyMethod, withoutRsp);
    Method valMethod(handle, valVec);
    adapter.CallMethod(appId, valMethod, withoutRsp);

    // WriteProperty：value_ 为空 → INVALID_PARAM 回传；带值 → NLSTK_SsapClientWriteProperty + delete[] 链
    Property emptyProp(handle);
    adapter.WriteProperty(appId, emptyProp, withoutRsp);
    Property valProp(handle, std::vector<uint8_t>(valVec));
    adapter.WriteProperty(appId, valProp, withoutRsp);

    // WriteDescriptor：同上两链
    Descriptor emptyDesc(handle, descType);
    adapter.WriteDescriptor(appId, emptyDesc, withoutRsp);
    Descriptor valDesc(handle, descType, std::vector<uint8_t>(valVec));
    adapter.WriteDescriptor(appId, valDesc, withoutRsp);

    // 清理链：NLSTK_SsapCleanClientApp(OnDisable) → DoInSsapThread → OnDisableTask → 回调 OnDisable
    adapter.Disable();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
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
    /* On* 静态入口经 DoInSsapThread 派发，依赖 ssap 线程态（未初始化时任务被
     * 静默丢弃），与 ssapservicesserver_fuzzer 同款首尾配对 */
    OHOS::g_threadUtil.InitThreadStateMap();
    OHOS::SsapClientAdapterApiFuzzTest(data, size);
    OHOS::SsapClientAdapterValueApiFuzzTest(data, size);
    OHOS::ClientAdapterCallbacksFuzzTest(data, size);
    OHOS::g_threadUtil.ClearThreadStateMap();
    return 0;
}
