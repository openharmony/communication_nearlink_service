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
#include "slem.h"
#include "ssap_server_stack_adapter.h"

using namespace std;
using namespace OHOS::Nearlink;
namespace OHOS {
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
    OHOS::SsapServerAdapterApiFuzzTest(data, size);
    return 0;
}
