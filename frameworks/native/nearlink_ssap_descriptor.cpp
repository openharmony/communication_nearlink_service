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
#include <cstddef>
#include <cstdint>
#include "securec.h"
#include "log.h"
#include "cstdint"
#include "memory"
#include "nearlink_ssap_descriptor.h"

namespace OHOS {
namespace Nearlink {

SsapDescriptor::SsapDescriptor(int type, int permission)
    : handle_(0),
      descriptorType_(type),
      value_(nullptr),
      length_(0),
      descriptorPermissions_(permission),
      propertyUuid_(),
      serviceUuid_()
{}

SsapDescriptor::SsapDescriptor(uint16_t handle, int type, int permission)
    : handle_(handle),
      descriptorType_(type),
      value_(nullptr),
      length_(0),
      descriptorPermissions_(permission),
      propertyUuid_(),
      serviceUuid_()
{}

SsapDescriptor::SsapDescriptor(const SsapDescriptor &src)
    : handle_(src.handle_),
      descriptorType_(src.descriptorType_),
      length_(src.length_),
      descriptorPermissions_(src.descriptorPermissions_),
      propertyUuid_(src.propertyUuid_),
      serviceUuid_(src.serviceUuid_)
{
    if (length_ != 0 && src.value_ != nullptr) {
        value_ = std::make_unique<uint8_t[]>(length_);
        (void)memcpy_s(value_.get(), length_, src.value_.get(), length_);
    } else {
        value_.reset(nullptr);
        length_ = 0;
    }
}

SsapDescriptor &SsapDescriptor::operator=(const SsapDescriptor &src)
{
    if (this != &src) {
        handle_ = src.handle_;
        descriptorType_ = src.descriptorType_;
        length_ = src.length_;
        descriptorPermissions_ = src.descriptorPermissions_;
        propertyUuid_ = src.propertyUuid_;
        serviceUuid_ = src.serviceUuid_;

        if (length_ != 0 && src.value_ != nullptr) {
            value_ = std::make_unique<uint8_t[]>(length_);
            (void)memcpy_s(value_.get(), length_, src.value_.get(), length_);
        } else {
            value_.reset(nullptr);
            length_ = 0;
        }
    }
    return *this;
}

uint16_t SsapDescriptor::GetHandle() const
{
    return handle_;
}

int SsapDescriptor::GetDescriptorType() const
{
    return descriptorType_;
}

const std::unique_ptr<uint8_t[]> &SsapDescriptor::GetValue(size_t *size) const
{
    *size = length_;
    return value_;
}

void SsapDescriptor::SetValue(const uint8_t *values, const size_t length)
{
    if (length == 0 || values == nullptr) {
        HILOGI("value is nullptr, or length is 0");
        return;
    }
    value_ = std::make_unique<uint8_t[]>(length);
    length_ = length;
    (void)memcpy_s(value_.get(), length, values, length);
}

int SsapDescriptor::GetDescriptorPermission() const
{
    return descriptorPermissions_;
}

const UUID &SsapDescriptor::GetPropertyUuid() const
{
    return propertyUuid_;
}

void SsapDescriptor::SetPropertyUuid(const UUID &uuid)
{
    propertyUuid_ = uuid;
}

const UUID &SsapDescriptor::GetServiceUuid() const
{
    return serviceUuid_;
}

void SsapDescriptor::SetServiceUuid(const UUID &uuid)
{
    serviceUuid_ = uuid;
}

}
}