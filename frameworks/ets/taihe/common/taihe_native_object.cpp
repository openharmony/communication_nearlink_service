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

#include "taihe_native_object.h"
#include "taihe/array.hpp"
#include "taihe/optional.hpp"
#include "taihe/platform/ani.hpp"

namespace OHOS {
namespace Nearlink {

bool HasWriteable(uint32_t operationIndication, size_t idx)
{
    bool isWriteable = (operationIndication & (0x0100U << idx)) != 0;
    return isWriteable;
}

ani_object ConvertPropertyDescriptorToAni(ani_env *env, SsapDescriptor& descriptor, SsapProperty& property, size_t idx)
{
    ani_string serviceUuid = {};
    HILOGI("serviceUuid: %{public}s", GET_ENCRYPT_UUID(descriptor.GetServiceUuid()));
    env->String_NewUTF8(descriptor.GetServiceUuid().ToString().c_str(),
        descriptor.GetServiceUuid().ToString().size(), &serviceUuid);

    ani_string propertyUuid = {};
    HILOGI("uuid: %{public}s", GET_ENCRYPT_UUID(descriptor.GetPropertyUuid()));
    env->String_NewUTF8(descriptor.GetPropertyUuid().ToString().c_str(),
        descriptor.GetPropertyUuid().ToString().size(), &propertyUuid);

    size_t valueSize = 0;
    uint8_t* valueData = descriptor.GetValue(&valueSize).get();
    ani_arraybuffer value = {};
    {
        void* valueValueData = {};
        env->CreateArrayBuffer(valueSize,
            &valueValueData, &value);
        if (memcpy_s(
            reinterpret_cast<uint8_t*>(valueValueData), valueSize, valueData, valueSize) != EOK) {
            HILOGE("memcpy_s error");
            return {};
        }
    }
    HILOGI("descriptorType: %{public}d", descriptor.GetDescriptorType());
    ani_enum_item descriptorType = ConvertDescriptorTypeToAni(env, descriptor.GetDescriptorType());

    ani_object isWriteable = {};
    ani_boolean writeValue = static_cast<ani_boolean>(HasWriteable(property.GetOperationIndication(), idx));
    env->Object_New(TH_ANI_FIND_CLASS(env, "std.core.Boolean"),
        TH_ANI_FIND_CLASS_METHOD(env, "std.core.Boolean", "<ctor>", "z:"), &isWriteable, writeValue);

    ani_object object = {};
    env->Object_New(TH_ANI_FIND_CLASS(env, "@ohos.nearlink.ssap.ssap.PropertyDescriptor"),
        TH_ANI_FIND_CLASS_METHOD(env, "@ohos.nearlink.ssap.ssap.PropertyDescriptor", "<ctor>", nullptr),
        &object, serviceUuid, propertyUuid, value, descriptorType, isWriteable);
    return object;
}

ani_object ConvertPropertyDescriptorVectorToAni(ani_env *env, std::vector<SsapDescriptor>& descriptors,
    SsapProperty& property)
{
    HILOGI("size: %{public}zu", descriptors.size());

    if (descriptors.empty()) {
        return nullptr;
    }

    ani_array descriptorsObject = {};
    ani_ref descriptorsNone = {};
    env->GetUndefined(&descriptorsNone);
    env->Array_New(descriptors.size(), descriptorsNone, &descriptorsObject);
    for (size_t iterator = 0; iterator < descriptors.size(); iterator++) {
        ani_object item =
            ConvertPropertyDescriptorToAni(env, descriptors[iterator], property, iterator);
        env->Array_Set(descriptorsObject, iterator, item);
    }
    return descriptorsObject;
}

ani_object ConvertPropertyToAni(ani_env *env, SsapProperty &property)
{
    ani_string serviceUuid = {};
    HILOGI("serviceUuid: %{public}s", GET_ENCRYPT_UUID(property.GetServiceUuid()));
    env->String_NewUTF8(property.GetServiceUuid().ToString().c_str(),
        property.GetServiceUuid().ToString().size(), &serviceUuid);

    ani_string propertyUuid = {};
    HILOGI("uuid: %{public}s", GET_ENCRYPT_UUID(property.GetUuid()));
    env->String_NewUTF8(property.GetUuid().ToString().c_str(),
        property.GetUuid().ToString().size(), &propertyUuid);

    size_t valueSize = 0;
    uint8_t* valueData = property.GetValue(&valueSize).get();
    ani_arraybuffer value = {};
    {
        void* valueValueData = {};
        env->CreateArrayBuffer(valueSize,
            &valueValueData, &value);
        if (memcpy_s(
            reinterpret_cast<uint8_t*>(valueValueData), valueSize, valueData, valueSize) != EOK) {
            HILOGE("memcpy_s error");
            return {};
        }
    }

    ani_array descriptors {};
    ani_object descriptorsObject = ConvertPropertyDescriptorVectorToAni(env, property.GetDescriptors(), property);
    descriptors = reinterpret_cast<ani_array>(descriptorsObject);

    ani_ref operation = {};
    ani_object operationObject = {};
    int32_t operationValue = static_cast<unsigned int>(property.GetOperationIndication() & 0xFF);
    ani_int handle = static_cast<ani_int>(operationValue);
    env->Object_New(TH_ANI_FIND_CLASS(env, "std.core.Int"),
        TH_ANI_FIND_CLASS_METHOD(env, "std.core.Int", "<ctor>", "i:"),
        &operationObject, handle);
    operation = operationObject;

    ani_object object = {};
    env->Object_New(TH_ANI_FIND_CLASS(env, "@ohos.nearlink.ssap.ssap.Property"),
        TH_ANI_FIND_CLASS_METHOD(env, "@ohos.nearlink.ssap.ssap.Property", "<ctor>", nullptr),
        &object, serviceUuid, propertyUuid, value, descriptors, operation);
    return object;
}

ani_object ConvertMethodToAni(ani_env *env, SsapMethod &method)
{
    ani_string serviceUuid = {};
    HILOGI("uuid: %{public}s", GET_ENCRYPT_UUID(method.GetServiceUuid()));
    env->String_NewUTF8(method.GetServiceUuid().ToString().c_str(),
        method.GetServiceUuid().ToString().size(), &serviceUuid);

    ani_string methodUuid = {};
    HILOGI("uuid: %{public}s", GET_ENCRYPT_UUID(method.GetUuid()));
    env->String_NewUTF8(method.GetUuid().ToString().c_str(),
        method.GetUuid().ToString().size(), &methodUuid);

    size_t parameterSize = 0;
    uint8_t* parameterData = method.GetParameter(&parameterSize).get();
    ani_arraybuffer parameter = {};
    {
        void* parameterValueData = {};
        env->CreateArrayBuffer(parameterSize,
            &parameterValueData, &parameter);
        if (memcpy_s(
            reinterpret_cast<uint8_t*>(parameterValueData), parameterSize, parameterData, parameterSize) != EOK) {
            HILOGE("memcpy_s error");
            return {};
        }
    }

    size_t resultSize = 0;
    uint8_t* resultData = method.GetResult(&resultSize).get();
    ani_arraybuffer result = {};
    {
        void* resultValueData = {};
        env->CreateArrayBuffer(resultSize,
            &resultValueData, &result);
        if (memcpy_s(reinterpret_cast<uint8_t*>(resultValueData), resultSize, resultData, resultSize) != EOK) {
            HILOGE("memcpy_s error");
            return {};
        }
    }
    ani_object object = {};
    env->Object_New(TH_ANI_FIND_CLASS(env, "@ohos.nearlink.ssap.ssap.Method"),
        TH_ANI_FIND_CLASS_METHOD(env, "@ohos.nearlink.ssap.ssap.Method", "<ctor>", nullptr),
        &object, serviceUuid, methodUuid, parameter, result);
    return object;
}

ani_enum_item ConvertDescriptorTypeToAni(ani_env* env, int type)
{
    ani_enum_item undef = {};

    ani_enum enumType;
    ani_status ret = env->FindEnum("@ohos.nearlink.ssap.ssap.PropertyDescriptorType", &enumType);
    if (ret != ANI_OK) {
        HILOGE("PropertyDescriptorType not found");
        return undef;
    }
    ani_enum_item enumItem;
    ret = env->Enum_GetEnumItemByIndex(enumType, ani_int(type), &enumItem);
    if (ret != ANI_OK) {
        HILOGE("get enum item failed");
        return undef;
    }

    return enumItem;
}

ani_object ConvertDescriptorToAni(ani_env *env, SsapDescriptor &descriptor)
{
    ani_string serviceUuid = {};
    HILOGI("serviceUuid: %{public}s", GET_ENCRYPT_UUID(descriptor.GetServiceUuid()));
    env->String_NewUTF8(descriptor.GetServiceUuid().ToString().c_str(),
        descriptor.GetServiceUuid().ToString().size(), &serviceUuid);

    ani_string propertyUuid = {};
    HILOGI("uuid: %{public}s", GET_ENCRYPT_UUID(descriptor.GetPropertyUuid()));
    env->String_NewUTF8(descriptor.GetPropertyUuid().ToString().c_str(),
        descriptor.GetPropertyUuid().ToString().size(), &propertyUuid);

    size_t valueSize = 0;
    uint8_t* valueData = descriptor.GetValue(&valueSize).get();
    ani_arraybuffer value = {};
    {
        void* valueValueData = {};
        env->CreateArrayBuffer(valueSize,
            &valueValueData, &value);
        if (memcpy_s(
            reinterpret_cast<uint8_t*>(valueValueData), valueSize, valueData, valueSize) != EOK) {
            HILOGE("memcpy_s error");
            return {};
        }
    }

    HILOGI("descriptorType: %{public}d", descriptor.GetDescriptorType());
    ani_enum_item descriptorType = ConvertDescriptorTypeToAni(env, descriptor.GetDescriptorType());

    ani_object isWriteable = {};
    ani_boolean writeValue = false;
    env->Object_New(TH_ANI_FIND_CLASS(env, "std.core.Boolean"),
        TH_ANI_FIND_CLASS_METHOD(env, "std.core.Boolean", "<ctor>", "z:"), &isWriteable, writeValue);

    ani_object object = {};
    env->Object_New(TH_ANI_FIND_CLASS(env, "@ohos.nearlink.ssap.ssap.PropertyDescriptor"),
        TH_ANI_FIND_CLASS_METHOD(env, "@ohos.nearlink.ssap.ssap.PropertyDescriptor", "<ctor>", nullptr),
        &object, serviceUuid, propertyUuid, value, descriptorType, isWriteable);
    return object;
}

ani_ref TaiheNativeInt::ToTaiheValue(ani_env *env) const
{
    ani_class cls;
    ani_status status = ANI_ERROR;
    if ((status = env->FindClass("std.core.Int", &cls)) != ANI_OK) {
        HILOGE("FindClass status : %{public}d", status);
        return nullptr;
    }
    ani_method ctor;
    if ((status = env->Class_FindMethod(cls, "<ctor>", "i:", &ctor)) != ANI_OK) {
        HILOGE("Class_FindMethod status : %{public}d", status);
        return nullptr;
    }
    ani_object object;
    if ((status = env->Object_New(cls, ctor, &object, value_)) != ANI_OK) {
        HILOGE("Object_New status : %{public}d", status);
        return nullptr;
    }
    return reinterpret_cast<ani_ref>(object);
}

ani_ref TaiheNativeBool::ToTaiheValue(ani_env *env) const
{
    ani_class cls;
    ani_status status = ANI_ERROR;
    if ((status = env->FindClass("std.core.Boolean", &cls)) != ANI_OK) {
        HILOGE("FindClass status : %{public}d", status);
        return nullptr;
    }
    ani_method ctor;
    if ((status = env->Class_FindMethod(cls, "<ctor>", "z:", &ctor)) != ANI_OK) {
        HILOGE("Class_FindMethod status : %{public}d", status);
        return nullptr;
    }
    ani_object object;
    if ((status = env->Object_New(cls, ctor, &object, value_)) != ANI_OK) {
        HILOGE("Object_New status : %{public}d", status);
        return nullptr;
    }
    return reinterpret_cast<ani_ref>(object);
}

ani_ref TaiheNativeSsapProperty::ToTaiheValue(ani_env *env) const
{
    ani_object object = ConvertPropertyToAni(env, const_cast<SsapProperty &>(property_));
    return reinterpret_cast<ani_ref>(object);
}

ani_ref TaiheNativeSsapMethod::ToTaiheValue(ani_env *env) const
{
    ani_object object = ConvertMethodToAni(env, const_cast<SsapMethod &>(method_));
    return reinterpret_cast<ani_ref>(object);
}

ani_ref TaiheNativeSsapDescriptor::ToTaiheValue(ani_env *env) const
{
    ani_object object = ConvertDescriptorToAni(env, const_cast<SsapDescriptor &>(descriptor_));
    return reinterpret_cast<ani_ref>(object);
}
}  // namespace Nearlink
}  // namespace OHOS