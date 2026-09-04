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

#ifndef TAIHE_NATIVE_OBJECT_H
#define TAIHE_NATIVE_OBJECT_H

#include <vector>
#include "ani_nearlink_utils.h"
#include "nearlink_ssap_descriptor.h"
#include "nearlink_ssap_property.h"
#include "nearlink_ssap_method.h"
#include "taihe/runtime.hpp"

namespace OHOS {
namespace Nearlink {
class TaiheNativeObject {
public:
    virtual ~TaiheNativeObject() = default;
    virtual ani_ref ToTaiheValue(ani_env *env) const = 0;
};

class TaiheNativeEmpty : public TaiheNativeObject {
public:
    ~TaiheNativeEmpty() override = default;

    ani_ref ToTaiheValue(ani_env *env) const override
    {
        return TaiheGetNull(env);
    }
};

class TaiheNativeUndefined : public TaiheNativeObject {
public:
    ~TaiheNativeUndefined() override = default;

    ani_ref ToTaiheValue(ani_env *env) const override
    {
        return TaiheGetUndefined(env);
    }
};

class TaiheNativeInt : public TaiheNativeObject {
public:
    explicit TaiheNativeInt(int value) : value_(value) {}
    ~TaiheNativeInt() override = default;

    ani_ref ToTaiheValue(ani_env *env) const override;
private:
    int value_;
};

class TaiheNativeBool : public TaiheNativeObject {
public:
    explicit TaiheNativeBool(bool value) : value_(value) {}
    ~TaiheNativeBool() override = default;

    ani_ref ToTaiheValue(ani_env *env) const override;
private:
    bool value_;
};

class TaiheNativeSsapProperty : public TaiheNativeObject {
public:
    explicit TaiheNativeSsapProperty(const SsapProperty &property) : property_(property) {}
    ~TaiheNativeSsapProperty() override = default;

    ani_ref ToTaiheValue(ani_env *env) const override;
private:
    SsapProperty property_;
};

class TaiheNativeSsapMethod : public TaiheNativeObject {
public:
    explicit TaiheNativeSsapMethod(const SsapMethod &method) : method_(method) {}
    ~TaiheNativeSsapMethod() override = default;

    ani_ref ToTaiheValue(ani_env *env) const override;
private:
    SsapMethod method_;
};

class TaiheNativeSsapDescriptor : public TaiheNativeObject {
public:
    explicit TaiheNativeSsapDescriptor(const SsapDescriptor &descriptor) : descriptor_(descriptor) {}
    ~TaiheNativeSsapDescriptor() override = default;

    ani_ref ToTaiheValue(ani_env *env) const override;
private:
    SsapDescriptor descriptor_;
};

bool HasWriteable(uint32_t operationIndication, size_t idx);
ani_object ConvertPropertyDescriptorToAni(ani_env *env, SsapDescriptor& descriptor, SsapProperty& property, size_t idx);
ani_object ConvertPropertyDescriptorVectorToAni(ani_env *env, std::vector<SsapDescriptor>& descriptors,
    SsapProperty& property);
ani_object ConvertPropertyToAni(ani_env *env, SsapProperty &property);
ani_object ConvertMethodToAni(ani_env *env, SsapMethod &method);
ani_enum_item ConvertDescriptorTypeToAni(ani_env* env, int type);
ani_object ConvertDescriptorToAni(ani_env *env, SsapDescriptor &descriptor);
}  // namespace Nearlink
}  // namespace OHOS

#endif  // TAIHE_NATIVE_OBJECT_H
