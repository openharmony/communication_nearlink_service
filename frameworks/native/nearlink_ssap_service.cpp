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

#include "nearlink_ssap_service.h"

#include <functional>
#include "type_traits"
#include "nearlink_uuid.h"
#include "vector"

namespace OHOS {
namespace Nearlink {

SsapService::SsapService(const UUID &uuid, const SsapServiceType type)
    : handle_(0),
      endHandle_(0),
      serviceType_(type),
      includeServices_(),
      properties_(),
      methods_(),
      events_(),
      uuid_(uuid),
      descriptors_()
{}

SsapService::SsapService(const UUID &uuid, uint16_t handle, uint16_t endHandle, const SsapServiceType type)
    : handle_(handle),
      endHandle_(endHandle),
      serviceType_(type),
      includeServices_(),
      properties_(),
      methods_(),
      events_(),
      uuid_(uuid),
      descriptors_()
{}

SsapService::SsapService(const SsapService &src)
    : handle_(src.handle_),
      endHandle_(src.endHandle_),
      serviceType_(src.serviceType_),
      includeServices_(src.includeServices_),
      properties_(src.properties_),
      methods_(src.methods_),
      events_(src.events_),
      uuid_(src.uuid_),
      descriptors_(src.descriptors_)
{}

SsapService::SsapService(SsapService &&src)
    : handle_(src.handle_),
      endHandle_(src.endHandle_),
      serviceType_(src.serviceType_),
      includeServices_(std::move(src.includeServices_)),
      properties_(std::move(src.properties_)),
      methods_(std::move(src.methods_)),
      events_(std::move(src.events_)),
      uuid_(src.uuid_),
      descriptors_(std::move(src.descriptors_))
{}

void SsapService::AddProperty(const SsapProperty &property)
{
    properties_.insert(properties_.end(), std::move(property))->SetServiceUuid(uuid_);
}

void SsapService::AddMethod(const SsapMethod &method)
{
    methods_.insert(methods_.end(), std::move(method))->SetServiceUuid(uuid_);
}

void SsapService::AddEvent(const SsapEvent &event)
{
    events_.insert(events_.end(), std::move(event))->SetServiceUuid(uuid_);
}

void SsapService::AddService(std::shared_ptr<SsapService> service)
{
    includeServices_.push_back(service);
}

std::vector<SsapProperty> &SsapService::GetProperty()
{
    return properties_;
}

std::vector<SsapMethod> &SsapService::GetMethod()
{
    return methods_;
}

std::vector<SsapEvent> &SsapService::GetEvent()
{
    return events_;
}

const std::vector<std::shared_ptr<SsapService>> &SsapService::GetIncludedServices()
{
    return includeServices_;
}

uint16_t SsapService::GetHandle() const
{
    return handle_;
}

void SsapService::AddDescriptor(const SsapDescriptor &descriptor)
{
    descriptors_.insert(descriptors_.end(), std::move(descriptor))->SetServiceUuid(uuid_);
}

SsapDescriptor *SsapService::GetDescriptor(int type)
{
    for (auto &desc : descriptors_) {
        if (desc.GetDescriptorType() == type) {
            return &desc;
        }
    }
    return nullptr;
}

std::vector<SsapDescriptor> &SsapService::GetDescriptors()
{
    return descriptors_;
}

bool SsapService::IsPrimary() const
{
    return (serviceType_ == SsapServiceType::VENDOR_PROMARY);
}

const UUID &SsapService::GetUuid() const
{
    return uuid_;
}

}
}