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
#include "ssap_client_stack_adapter.h"
#include "slem.h"

using namespace std;
using namespace OHOS::Nearlink;
namespace OHOS {
size_t FUZZ_DATA_SIZE = 50;

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
        return 0;
    }
    OHOS::SsapClientAdapterApiFuzzTest(data, size);
    return 0;
}
