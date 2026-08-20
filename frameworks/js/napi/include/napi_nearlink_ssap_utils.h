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
#ifndef NAPI_NEARLINK_SSAP_UTILS_H
#define NAPI_NEARLINK_SSAP_UTILS_H

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <cstdint>
#include <string>
#include <vector>
#include "uv.h"
#include "napi_native_object.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "nearlink_ssap_client.h"
#include "nearlink_ssap_server.h"
#include "nearlink_ssap_descriptor.h"
#include "nearlink_ssap_property.h"
#include "nearlink_ssap_service.h"

namespace OHOS {
namespace Nearlink {
void ConvertSsapServiceToJS(napi_env env, napi_value result, SsapService &service);
void ConvertSsapServiceVectorToJS(napi_env env, napi_value result, std::vector<SsapService> &services);
void ConvertSsapPropertyToJS(napi_env env, napi_value result, SsapProperty& property);
void ConvertSsapPropertyVectorToJS(
    napi_env env, napi_value result, std::vector<SsapProperty> &characteristics);
void ConvertSsapMethodToJS(napi_env env, napi_value result, SsapMethod& method);
void ConvertSsapMethodVectorToJS(
    napi_env env, napi_value result, std::vector<SsapMethod> &characteristics);
void ConvertSsapEventToJS(napi_env env, napi_value result, SsapEvent& event);
void ConvertSsapEventVectorToJS(
    napi_env env, napi_value result, std::vector<SsapEvent> &characteristics);
void ConvertSsapDescriptorCommonToJS(napi_env env, napi_value result, SsapDescriptor& descriptor);
void ConvertSsapDescriptorToJS(napi_env env, napi_value result, SsapDescriptor &descriptor, size_t idx,
                               uint32_t operationIndication);
void ConvertSsapDescriptorVectorToJS(napi_env env, napi_value result, std::vector<SsapDescriptor> &descriptors,
                                     SsapProperty& property);

class NapiNativeSsapServiceArray : public NapiNativeObject {
public:
    NapiNativeSsapServiceArray(const std::vector<SsapService> &ssapServices) : ssapServices_(ssapServices) {}
    ~NapiNativeSsapServiceArray() override = default;

    napi_value ToNapiValue(napi_env env) const override;
private:
    std::vector<SsapService> ssapServices_ {};
};


class NapiNativeSsapConnectionState : public NapiNativeObject {
public:
    NapiNativeSsapConnectionState(std::string deviceId, int state) : deviceId_(deviceId), state_(state) {}
    ~NapiNativeSsapConnectionState() override = default;

    napi_value ToNapiValue(napi_env env) const override;
private:
    std::string deviceId_;
    int state_;
};

class NapiNativeSsapProperty : public NapiNativeObject {
public:
    explicit NapiNativeSsapProperty(SsapProperty &property) : property_(property) {}
    ~NapiNativeSsapProperty() override = default;

    napi_value ToNapiValue(napi_env env) const override;
private:
    SsapProperty property_;
};

class NapiNativeSsapMethod : public NapiNativeObject {
public:
    explicit NapiNativeSsapMethod(SsapMethod &method) : method_(method) {}
    ~NapiNativeSsapMethod() override = default;

    napi_value ToNapiValue(napi_env env) const override;
private:
    SsapMethod method_;
};

class NapiNativeSsapEvent : public NapiNativeObject {
public:
    explicit NapiNativeSsapEvent(SsapEvent &event) : event_(event) {}
    ~NapiNativeSsapEvent() override = default;

    napi_value ToNapiValue(napi_env env) const override;
private:
    SsapEvent event_;
};

class NapiNativeSsapPropertyArray : public NapiNativeObject {
public:
    NapiNativeSsapPropertyArray(const std::vector<SsapProperty> &ssapProperties) : ssapProperties_(ssapProperties) {}
    ~NapiNativeSsapPropertyArray() override = default;

    napi_value ToNapiValue(napi_env env) const override;
private:
    std::vector<SsapProperty> ssapProperties_ {};
};

class NapiNativeSsapDescriptor : public NapiNativeObject {
public:
    explicit NapiNativeSsapDescriptor(SsapDescriptor &descriptor) : descriptor_(descriptor) {}
    ~NapiNativeSsapDescriptor() override = default;

    napi_value ToNapiValue(napi_env env) const override;
private:
    SsapDescriptor descriptor_;
};

class NapiNativeSsapPropertyReadRequest : public NapiNativeObject {
public:
    explicit NapiNativeSsapPropertyReadRequest(std::string deviceId, SsapProperty &property, int requestId)
        : deviceId_(deviceId), property_(property), requestId_(requestId) {}
    ~NapiNativeSsapPropertyReadRequest() override = default;

    napi_value ToNapiValue(napi_env env) const override;
private:
    std::string deviceId_;
    SsapProperty property_;
    int requestId_;
};

class NapiNativeSsapPropertyWriteRequest : public NapiNativeObject {
public:
    explicit NapiNativeSsapPropertyWriteRequest(
        std::string deviceId, SsapProperty &property, int requestId, int writeType)
        : deviceId_(deviceId), property_(property), requestId_(requestId), writeType_(writeType) {}
    ~NapiNativeSsapPropertyWriteRequest() override = default;

    napi_value ToNapiValue(napi_env env) const override;
private:
    std::string deviceId_;
    SsapProperty property_;
    int requestId_;
    int writeType_;
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // NAPI_NEARLINK_SSAP_UTILS_H