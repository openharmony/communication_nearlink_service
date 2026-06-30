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

using namespace std;
using namespace OHOS::Nearlink;
namespace OHOS {
ThreadUtil &g_threadUtil = ThreadUtil::GetInstance();
constexpr int SSAP_FUZZ_DELAY_50_MS = 50;
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
}

// fuzzer init
extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void) argc;
    (void) argv;
    int ret = slem_initialize();
    HILOGI("slem_initialize %{public}d", ret);
    printf("slem_initialize %d\n", ret);
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
    OHOS::g_threadUtil.ClearThreadStateMap();
    return 0;
}

