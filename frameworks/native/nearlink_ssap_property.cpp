/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#include "securec.h"
#include "log.h"
#include "type_traits"
#include "nearlink_ssap_descriptor.h"
#include "nearlink_ssap_property.h"

namespace OHOS {
namespace Nearlink {

SsapProperty::SsapProperty(int type, const UUID uuid, uint32_t operIndicate, int valuePerm)
    : writeType_(PROPERTY_WRITE_DEFAULT),
      handle_(0),
      propertyType_(type),
      uuid_(uuid),
      operationIndication_(operIndicate),
      value_(nullptr),
      length_(0),
      descriptors_(),
      valuePermissions_(valuePerm),
      serviceUuid_()
{}

SsapProperty::SsapProperty(uint16_t handle, const int type, const UUID uuid, uint32_t operIndicate, int valuePerm)
    : writeType_(PROPERTY_WRITE_DEFAULT),
      handle_(handle),
      propertyType_(type),
      uuid_(uuid),
      operationIndication_(operIndicate),
      value_(nullptr),
      length_(0),
      descriptors_(),
      valuePermissions_(valuePerm),
      serviceUuid_()
{}

SsapProperty::SsapProperty(uint16_t handle, const UUID uuid, uint32_t operIndicate, int valuePerm)
    : writeType_(PROPERTY_WRITE_DEFAULT),
      handle_(handle),
      propertyType_(0),
      uuid_(uuid),
      operationIndication_(operIndicate),
      value_(nullptr),
      length_(0),
      descriptors_(),
      valuePermissions_(valuePerm),
      serviceUuid_()
{}

SsapProperty::SsapProperty(const SsapProperty &src)
    : writeType_(src.writeType_),
      handle_(src.handle_),
      propertyType_(src.propertyType_),
      uuid_(src.uuid_),
      operationIndication_(src.operationIndication_),
      length_(src.length_),
      descriptors_(src.descriptors_),
      valuePermissions_(src.valuePermissions_),
      serviceUuid_(src.serviceUuid_)
{
    if (nullptr != src.value_ && 0 != length_) {
        value_ = std::make_unique<uint8_t[]>(length_);
        (void)memcpy_s(value_.get(), length_, src.value_.get(), length_);
    } else {
        value_.reset(nullptr);
        length_ = 0;
    }
}

SsapProperty &SsapProperty::operator=(const SsapProperty &src)
{
    if (this != &src) {
        uuid_ = src.uuid_;
        valuePermissions_ = src.valuePermissions_;
        propertyType_ = src.propertyType_;
        handle_ = src.handle_;
        length_ = src.length_;
        writeType_ = src.writeType_;
        operationIndication_ = src.operationIndication_;
        descriptors_ = src.descriptors_;
        serviceUuid_ = src.serviceUuid_;

        if (nullptr != src.value_ && 0 != length_) {
            value_ = std::make_unique<uint8_t[]>(length_);
            (void)memcpy_s(value_.get(), length_, src.value_.get(), length_);
        } else {
            value_.reset(nullptr);
            length_ = 0;
        }
    }
    return *this;
}

void SsapProperty::AddDescriptor(const SsapDescriptor &descriptor)
{
    descriptors_.insert(descriptors_.end(), descriptor)->SetPropertyUuid(uuid_);
}

SsapDescriptor *SsapProperty::GetDescriptor(int type)
{
    for (auto &desc : descriptors_) {
        if (desc.GetDescriptorType() == type) {
            return &desc;
        }
    }
    return nullptr;
}

std::vector<SsapDescriptor> &SsapProperty::GetDescriptors()
{
    return descriptors_;
}

int SsapProperty::GetPropertyType() const
{
    return propertyType_;
}

uint16_t SsapProperty::GetHandle() const
{
    return handle_;
}

uint32_t SsapProperty::GetOperationIndication() const
{
    return operationIndication_;
}

void SsapProperty::SetOperationIndication(uint32_t newOperationIndication)
{
    operationIndication_ = newOperationIndication;
}

const UUID &SsapProperty::GetServiceUuid() const
{
    return serviceUuid_;
}

int SsapProperty::GetValuePermissions() const
{
    return valuePermissions_;
}

const UUID &SsapProperty::GetUuid() const
{
    return uuid_;
}

int SsapProperty::GetWriteType() const
{
    return writeType_;
}

int SsapProperty::SetWriteType(int type)
{
    if (type != PROPERTY_WRITE_CMD && type != PROPERTY_WRITE_REQ &&
        type != PROPERTY_WRITE_DEFAULT) {
        HILOGE("Invalid parameter");
        return static_cast<int>(SsapStatus::SSAP_INVALID_PARAM);
    }

    writeType_ = type;
    return static_cast<int>(SsapStatus::SSAP_SUCCESS);
}

const std::unique_ptr<uint8_t[]> &SsapProperty::GetValue(size_t *size) const
{
    *size = length_;
    return value_;
}

void SsapProperty::SetValue(const uint8_t *values, const size_t length)
{
    if (values == nullptr || length == 0) {
        HILOGE("values is nullptr, or length is 0");
        return;
    }
    value_ = std::make_unique<uint8_t[]>(length);
    length_ = length;
    (void)memcpy_s(value_.get(), length, values, length);
}

void SsapProperty::SetServiceUuid(const UUID &uuid)
{
    serviceUuid_ = uuid;
}
} // Nearlink
} // OHOS