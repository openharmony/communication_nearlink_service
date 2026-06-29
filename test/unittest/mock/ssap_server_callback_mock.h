/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#ifndef SSAP_SERVER_CALLBACK_MOCK_H
#define SSAP_SERVER_CALLBACK_MOCK_H

#include <gmock/gmock.h>
#include "interface_profile_ssap_server.h"

namespace OHOS {
namespace Nearlink {
class SsapServerCallbackMock : public InterfaceSsapServerCallback {
public:
    MOCK_METHOD(void, OnMtuChanged, (const RawAddress &addr, uint8_t transport, uint16_t mtu), (override));
    MOCK_METHOD(void, OnAddService, (Service &service, int ret), (override));
    MOCK_METHOD(void, OnSetPropertyValue, (Property &property, int ret), (override));
    MOCK_METHOD(void, OnSetDescriptorValue, (Descriptor &descriptor, int ret), (override));
    MOCK_METHOD(void, OnReadPropertyAuthorizeRequest, (
        const RawAddress &addr, uint8_t transport, uint16_t requestId, Property &property), (override));
    MOCK_METHOD(void, OnReadDescriptorAuthorizeRequest, (
        const RawAddress &addr, uint8_t transport, uint16_t requestId, Descriptor &descriptor), (override));
    MOCK_METHOD(void, OnWritePropertyAuthorizeRequest, (
        const RawAddress &addr, uint8_t transport, uint16_t requestId, Property &property), (override));
    MOCK_METHOD(void, OnWriteDescriptorAuthorizeRequest, (
        const RawAddress &addr, uint8_t transport, uint16_t requestId, Descriptor &descriptor), (override));
    MOCK_METHOD(void, OnReadProperty,
        (const RawAddress &addr, uint8_t transport, Property &property, int ret), (override));
    MOCK_METHOD(void, OnReadDescriptor,
        (const RawAddress &addr, uint8_t transport, Descriptor &descriptor, int ret), (override));
    MOCK_METHOD(void, OnWriteProperty,
        (const RawAddress &addr, uint8_t transport, Property &property, int ret), (override));
    MOCK_METHOD(void, OnWriteDescriptor,
        (const RawAddress &addr, uint8_t transport, Descriptor &descriptor, int ret), (override));
    MOCK_METHOD(void, OnNotifyProperty,
        (const RawAddress &addr, uint8_t transport, Property &property, int ret), (override));
    MOCK_METHOD(void, OnIndicateProperty,
        (const RawAddress &addr, uint8_t transport, Property &property, int ret), (override));
    MOCK_METHOD(void, OnNotifyEvent,
        (const RawAddress &addr, uint8_t transport, Event &event, int ret), (override));
    MOCK_METHOD(void, OnIndicateEvent,
        (const RawAddress &addr, uint8_t transport, Event &event, int ret), (override));
    MOCK_METHOD(void, OnCallMethod, (const RawAddress &addr, uint8_t transport,
        Method &method, std::vector<uint8_t> &value, bool needReturn), (override));
    MOCK_METHOD(void, OnConnectionStateChanged, (const RawAddress &addr, uint8_t transport, uint8_t state,
        int ret, int reason), (override));

    ~SsapServerCallbackMock() override = default;
};

}  // namespace Nearlink
}  // namespace OHOS
#endif // SSAP_SERVER_CALLBACK_MOCK_H
