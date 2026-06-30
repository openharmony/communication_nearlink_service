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
#include "nearlink_ssap_event.h"
#include "log.h"
#include "type_traits"

namespace OHOS {
namespace Nearlink {

SsapEvent::SsapEvent(int type, const UUID uuid)
    : handle_(0),
      eventType_(type),
      uuid_(uuid),
      parameter_(nullptr),
      length_(0),
      serviceUuid_()
{}

SsapEvent::SsapEvent(uint16_t handle, const int type, const UUID uuid)
    : handle_(handle),
      eventType_(type),
      uuid_(uuid),
      parameter_(nullptr),
      length_(0),
      serviceUuid_()
{}

SsapEvent::SsapEvent(uint16_t handle, const UUID uuid)
    : handle_(handle),
      eventType_(0),
      uuid_(uuid),
      parameter_(nullptr),
      length_(0),
      serviceUuid_()
{}

SsapEvent::SsapEvent(const SsapEvent &src)
    : handle_(src.handle_),
      eventType_(src.eventType_),
      uuid_(src.uuid_),
      length_(src.length_),
      serviceUuid_(src.serviceUuid_)
{
    if (nullptr != src.parameter_ && 0 != length_) {
        parameter_ = std::make_unique<uint8_t[]>(length_);
        (void)memcpy_s(parameter_.get(), length_, src.parameter_.get(), length_);
    } else {
        parameter_.reset(nullptr);
        length_ = 0;
    }
}

SsapEvent &SsapEvent::operator=(const SsapEvent &src)
{
    if (this != &src) {
        handle_ = src.handle_;
        eventType_ = src.eventType_;
        uuid_ = src.uuid_;
        length_ = src.length_,
        serviceUuid_ = src.serviceUuid_;

        if (nullptr != src.parameter_ && 0 != length_) {
            parameter_ = std::make_unique<uint8_t[]>(length_);
            (void)memcpy_s(parameter_.get(), length_, src.parameter_.get(), length_);
        } else {
            parameter_.reset(nullptr);
            length_ = 0;
        }
    }
    return *this;
}

int SsapEvent::GetEventType() const
{
    return eventType_;
}

uint16_t SsapEvent::GetHandle() const
{
    return handle_;
}

const UUID &SsapEvent::GetUuid() const
{
    return uuid_;
}

void SsapEvent::SetUuid(UUID uuidParam)
{
    uuid_ = uuidParam;
}

const std::unique_ptr<uint8_t[]> &SsapEvent::GetParameter(size_t *size) const
{
    *size = length_;
    return parameter_;
}

void SsapEvent::SetParameter(const uint8_t *values, const size_t length)
{
    if (values == nullptr || length == 0) {
        HILOGE("values is nullptr, or length is 0");
        return;
    }
    parameter_ = std::make_unique<uint8_t[]>(length);
    length_ = length;
    (void)memcpy_s(parameter_.get(), length, values, length);
}

const UUID &SsapEvent::GetServiceUuid() const
{
    return serviceUuid_;
}

void SsapEvent::SetServiceUuid(const UUID &uuid)
{
    serviceUuid_ = uuid;
}

}
}