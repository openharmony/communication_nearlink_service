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

using namespace std;
using namespace OHOS::Nearlink;
namespace OHOS {
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
}

// fuzzer init
extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void) argc;
    (void) argv;
    int ret = slem_initialize();
    HILOGI("slem_initialize %{public}d", ret);
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr) {
        return 0;
    }
    OHOS::DoSomethingSsapServiceClientWithMyAPI(data, size);
    return 0;
}

