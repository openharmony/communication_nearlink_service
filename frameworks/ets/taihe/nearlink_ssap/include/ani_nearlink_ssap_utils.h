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

#ifndef ANI_NEARLINK_SSAP_UTILS_H
#define ANI_NEARLINK_SSAP_UTILS_H
#include <vector>
#include "nearlink_remote_device.h"
#include "ohos.nearlink.ssap.proj.hpp"
#include "ohos.nearlink.ssap.impl.hpp"
#include "taihe/runtime.hpp"
#include "nearlink_ssap_client.h"
#include "nearlink_ssap_server.h"
#include "nearlink_ssap_service.h"
#include "nearlink_ssap_property.h"
#include "nearlink_ssap_method.h"
#include "nearlink_ssap_event.h"
#include "nearlink_ssap_descriptor.h"

namespace OHOS {
namespace Nearlink {
constexpr size_t ANI_ARRAY_MAX_LENGTH = 0xFFFF;
constexpr size_t UUID_128_BIT_LENGTH = 36;
constexpr size_t BASE_UUID_LENGTH = 32;
constexpr uint32_t OPERATION_READ = 1;
constexpr uint32_t OPERATION_WRITE_NO_RESPONSE = 2;
constexpr int32_t MIN_MTU_SIZE = 22;
constexpr int32_t MAX_MTU_SIZE = 1024;
constexpr uint32_t OPERATION_WRITE_CLIENT_CONFIG_MASK = 0x200;

struct AniSsapDescriptor {
    UUID serviceUuid_;
    UUID propertyUuid_;
    int32_t descriptorType_;
    std::vector<uint8_t> descriptorValue_;
    bool writeable_;
};

struct AniSsapEvent {
    UUID serviceUuid_;
    UUID uuid_;
    uint16_t handle_;
};

struct AniSsapMethod {
    UUID serviceUuid_;
    UUID methodUuid_;
    std::vector<uint8_t> parameter_;
    std::vector<uint8_t> result_;
};

struct AniSsapProperty {
    UUID serviceUuid_;
    UUID propertyUuid_;
    std::vector<uint8_t> value_;
    std::vector<AniSsapDescriptor> descriptors_;
    uint32_t operation_;
};

struct AniSsapService {
    UUID uuid_;
    std::vector<AniSsapProperty> properties_;
    std::vector<AniSsapMethod> methods_;
    std::vector<AniSsapEvent> events_;
};

struct AniSsapServerResponse {
    std::string address_;
    int requestId_;
    std::vector<uint8_t> value_;
};

enum class AniSsapDescriptorType {
    PROPERTY = 0x01,
    CLIENT_PROPERTY_CONFIG = 0x02,
    SERVER_PROPERTY_CONFIG = 0x03,
    PROPERTY_FORMAT = 0x04,
    TYPE_VENDOR = 0xFF,
};

bool CheckBaseUuid(const std::string &uuid);
void CheckDescriptorWriteOp(const AniSsapDescriptor &descriptor, uint32_t &operationIndication,
    uint32_t &opDescriptorBitMask);
::taihe::array<uint8_t> ConvertValueArray(const std::unique_ptr<uint8_t[]> &value, size_t size);
::ohos::nearlink::ssap::PropertyDescriptor ConvertDescriptorToTaihe(const SsapDescriptor &descriptor);
::ohos::nearlink::ssap::Property ConvertPropertyToTaihe(const SsapProperty &property);
::ohos::nearlink::ssap::Method ConvertMethodToTaihe(const SsapMethod &method);
::ohos::nearlink::ssap::Event ConvertEventToTaihe(const SsapEvent &event);
::ohos::nearlink::ssap::Service ConvertServiceToTaihe(const SsapService &service);
int32_t ConvertDescriptorToNative(const ::ohos::nearlink::ssap::PropertyDescriptor &descriptor,
    AniSsapDescriptor &aniDescriptor);
int32_t ConvertPropertyToNative(const ::ohos::nearlink::ssap::Property &property,
    AniSsapProperty &aniProperty);
int32_t ConvertMethodToNative(const ::ohos::nearlink::ssap::Method &method,
    AniSsapMethod &aniMethod);
int32_t ConvertEventToNative(const ::ohos::nearlink::ssap::Event &event,
    AniSsapEvent &aniEvent);
int32_t ConvertServiceToNative(const ::ohos::nearlink::ssap::Service &service,
    AniSsapService &aniService);
int32_t AniSsapServiceToSsapService(const AniSsapService &aniService, std::unique_ptr<SsapService> &outService);
int32_t ConvertTaiheServerResponseToNative(
    const ::ohos::nearlink::ssap::ServerResponse &response, AniSsapServerResponse &aniResponse);
::ohos::nearlink::ssap::ConnectionChangeState ConvertChangeStateToTaihe(
    const NearlinkRemoteDevice &device, int state);
::ohos::nearlink::ssap::PropertyReadRequest ConvertReadRequestToTaihe(
    const NearlinkRemoteDevice &device, const SsapProperty &property, int requestId);
::ohos::nearlink::ssap::PropertyWriteRequest ConvertWriteRequestToTaihe(
    const NearlinkRemoteDevice &device, const SsapProperty &property, int requestId);
}  // namespace Nearlink
}  // namespace OHOS

#endif // ANI_NEARLINK_SSAP_UTILS_H