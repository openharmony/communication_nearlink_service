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

#ifndef NAPI_PARSER_UTILS_H
#define NAPI_PARSER_UTILS_H

#include "napi_nearlink_utils.h"
#include "napi_nearlink_ssap_utils.h"
#include "nearlink_utils.h"
#include "nearlink_errorcode.h"

namespace OHOS {
namespace Nearlink {
struct NapiDataTransferParam {
    std::string address_;
    std::string uuid_;
    std::vector<uint8_t> data_;
};

struct NapiSsapDescriptor {
    UUID serviceUuid_;
    UUID propertyUuid_;
    int32_t descriptorType_;
    std::vector<uint8_t> descriptorValue_;
    bool writeable_;
};

struct NapiSsapEvent {
    UUID serviceUuid_;
    UUID uuid_;
    uint16_t handle_;
};

struct NapiSsapMethod {
    UUID serviceUuid_;
    UUID methodUuid_;
    std::vector<uint8_t> parameter_;
    std::vector<uint8_t> result_;
};

struct NapiSsapProperty {
    UUID serviceUuid_;
    UUID propertyUuid_;
    std::vector<uint8_t> value_;
    std::vector<NapiSsapDescriptor> descriptors_;
    uint32_t operation_;
};

struct NapiSsapService {
    UUID uuid_;
    std::vector<NapiSsapProperty> properties_;
    std::vector<NapiSsapMethod> methods_;
    std::vector<NapiSsapEvent> events_;
};

struct NapiSsapServerResponse {
    std::string address_;
    int requestId_;
    std::vector<uint8_t> value_;
};

enum class NapiSsapDescriptorType {
    PROPERTY = 0x01,
    CLIENT_PROPERTY_CONFIG = 0x02,
    SERVER_PROPERTY_CONFIG = 0x03,
    PROPERTY_FORMAT = 0x04,
    TYPE_VENDOR = 0xFF,
};

enum class NapiOperationIndication {
    READABLE = 0x01,
    WRITE_NO_RESPONSE = 0x02,
    WRITE_WITH_RESPONSE = 0x04,
    NOTIFY = 0x08,
};

enum class NapiPermission {
    SSAP_PERMISSION_WRITE_ENCRYPTION_NEED = 0x01,
    SSAP_PERMISSION_WRITE_AUTHENTICATION_NEED = 0x02,
    SSAP_PERMISSION_WRITE_AUTHORIZATION_NEED = 0x04,
    SSAP_PERMISSION_READ_ENCRYPTION_NEED = 0x08,
    SSAP_PERMISSION_READ_AUTHENTICATION_NEED = 0x10,
    SSAP_PERMISSION_READ_AUTHORIZATION_NEED = 0x20
};

enum class PropertyWriteType {
    WRITE = 0x01,
    WRITE_NO_RESPONSE = 0x02,
};

bool NapiIsObjectPropertyExist(napi_env env, napi_value object, const char *name);
napi_status NapiParseBoolean(napi_env env, napi_value value, bool &outBoolean);
napi_status NapiParseInt32(napi_env env, napi_value value, int32_t &outNum);
napi_status NapiParseUint32(napi_env env, napi_value value, uint32_t &outNum);
napi_status NapiParseString(napi_env env, napi_value value, std::string &outStr);
napi_status NapiParseUuid(napi_env env, napi_value value, std::string &outUuid);
napi_status NapiParseArrayBuffer(napi_env env, napi_value value, std::vector<uint8_t> &outVec);
int32_t NapiParseSsapService(napi_env env, napi_value object, NapiSsapService &outService);
int32_t NapiParseSsapProperty(napi_env env, napi_value object, NapiSsapProperty &outProperty);
int32_t NapiParseSsapMethod(napi_env env, napi_value object, NapiSsapMethod &outMethod);
int32_t NapiParseSsapEvent(napi_env env, napi_value object, NapiSsapEvent &outEvent);
int32_t NapiParseSsapDescriptor(napi_env env, napi_value object, NapiSsapDescriptor &outDescriptor);
napi_status NapiParseSsapDescriptorType(napi_env env, napi_value value, int &outWriteType);
napi_status NapiParseSsapServerResponse(napi_env env, napi_value object, NapiSsapServerResponse &response);
napi_status NapiParseTransMode(napi_env env, napi_value value, uint8_t &outTransMode);
napi_status NapiGetObjectProperty(napi_env env, napi_value object, const char *name, napi_value &outProperty);
napi_status NapiParseObjectBoolean(napi_env env, napi_value object, const char *name, bool &outBoolean);
napi_status NapiParseObjectUuid(napi_env env, napi_value object, const char *name, std::string &outUuid);
napi_status NapiParseArrayBuffer(napi_env env, napi_value object, const char *name, std::vector<uint8_t> &outVec);
napi_status NapiParseObjectInt32(napi_env env, napi_value object, const char *name, int32_t &outNum);
napi_status NapiParseObjectUint32(napi_env env, napi_value object, const char *name, uint32_t &outNum);

napi_status NapiGetObjectPropertyOptional(napi_env env, napi_value object, const char *name, napi_value &outProperty,
    bool &outExist);
napi_status NapiParseObjectBooleanOptional(napi_env env, napi_value object, const char *name, bool &outBoolean,
    bool &outExist);
napi_status NapiParseObjectInt32Optional(napi_env env, napi_value object, const char *name, int32_t &outNum,
    bool &outExist);
napi_status NapiParseObjectUint32Optional(napi_env env, napi_value object, const char *name, uint32_t &outNum,
    bool &outExist);
napi_status NapiParseObjectStringOptional(napi_env env, napi_value object, const char *name, std::string &outString,
    bool &outExist);
napi_status NapiParseArrayBufferOptional(napi_env env, napi_value object, const char *name,
    std::vector<uint8_t> &outVec, bool &outExist);

template <typename T>
napi_status NapiParseArrayOptional(napi_env env, napi_value object, const char *name,
    std::vector<T> &outVec, bool &outExist);

template <typename T>
napi_status NapiParseObjectOptional(napi_env env, napi_value object, const char *name,
    T &outObj, bool &outExist);

// // Parse type Array<XXX>, must implete NapiParseObject<XXX> in napi_parser_utils.cpp.
template <typename T>
napi_status NapiParseArray(napi_env env, napi_value array, std::vector<T> &outVec);
template <typename T>
napi_status NapiParseObjectArray(napi_env env, napi_value object, const char *name, std::vector<T> &outVec);

bool CheckBaseUuid(const std::string &uuid);
}  // namespace Nearlink
}  // namespace OHOS
#endif  // NAPI_PARSER_UTILS_H