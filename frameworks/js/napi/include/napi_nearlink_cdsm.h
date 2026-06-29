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
#ifndef NAPI_NEARLINK_CDSM_H_
#define NAPI_NEARLINK_CDSM_H_

#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "napi_native_object.h"
#include "nearlink_cdsm_info.h"
#include "napi_nearlink_cdsm_callback.h"

namespace OHOS {
namespace Nearlink {
class NapiNativeCdsmInfo : public NapiNativeObject {
public:
    explicit NapiNativeCdsmInfo(const NearlinkCdsInfo &cdsInfo) : cdsInfo_(cdsInfo) {}
    ~NapiNativeCdsmInfo() override = default;

    napi_value ToNapiValue(napi_env env) const override;
private:
    NearlinkCdsInfo cdsInfo_;
};

class NapiNativeCdsmMemberInfo : public NapiNativeObject {
public:
    explicit NapiNativeCdsmMemberInfo(const NearlinkCdsMemberInfo &cdsMemberInfo) : cdsMemberInfo_(cdsMemberInfo) {}
    ~NapiNativeCdsmMemberInfo() override = default;

    napi_value ToNapiValue(napi_env env) const override;
private:
    NearlinkCdsMemberInfo cdsMemberInfo_;
};

enum class CdsmConnectionState {
    DISCONNECTED = 0,   // the current member all profile is disconnected
    CONNECTED = 1,      // the current member all profile is connected
};

class NapiNearlinkCdsm {
public:
    explicit NapiNearlinkCdsm(const std::string &address);
    ~NapiNearlinkCdsm() = default;

    static void DefineCdsmJSClass(napi_env env, napi_value exports);

    static napi_value CreateCdsmClient(napi_env env, napi_callback_info info);
    static napi_value CdsmConstructor(napi_env env, napi_callback_info info);

    static napi_value GetCdsmInfo(napi_env env, napi_callback_info info);
    static napi_value OnCdsmInfoChange(napi_env env, napi_callback_info info);
    static napi_value OffCdsmInfoChange(napi_env env, napi_callback_info info);

    std::shared_ptr<NearlinkCdsmClient> GetCdsmClient() const;

    std::shared_ptr<NapiNearlinkCdsmClientCallback> GetCdsmCallback() const;

private:
    static napi_value CdsmPropertyValueInit(napi_env env, napi_value exports);
    static napi_value CdsmTsConnectionStateInit(napi_env env);

    std::shared_ptr<NapiNearlinkCdsmClientCallback> cdsmCallback_;
    std::shared_ptr<NearlinkCdsmClient> cdsmClient_;

    static thread_local napi_ref consRef_;
};
} // namespace Nearlink
} // namespace OHOS
#endif /* NAPI_NEARLINK_CDSM_H_ */