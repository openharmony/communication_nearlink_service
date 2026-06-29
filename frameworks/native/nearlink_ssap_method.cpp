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
#include "nearlink_ssap_method.h"
#include "log.h"
#include "type_traits"

namespace OHOS {
namespace Nearlink {

SsapMethod::SsapMethod(int type, const UUID uuid, int permissions)
    : handle_(0),
      methodType_(type),
      uuid_(uuid),
      parameter_(nullptr),
      parameterLength_(0),
      result_(nullptr),
      resultLength_(0),
      permissions_(permissions),
      serviceUuid_()
{}

SsapMethod::SsapMethod(uint16_t handle, const int type, const UUID uuid, const int permissions)
    : handle_(handle),
      methodType_(type),
      uuid_(uuid),
      parameter_(nullptr),
      parameterLength_(0),
      result_(nullptr),
      resultLength_(0),
      permissions_(permissions),
      serviceUuid_()
{}

SsapMethod::SsapMethod(const SsapMethod &src)
    : handle_(src.handle_),
      methodType_(src.methodType_),
      uuid_(src.uuid_),
      parameterLength_(src.parameterLength_),
      resultLength_(src.resultLength_),
      permissions_(src.permissions_),
      serviceUuid_(src.serviceUuid_)
{
    if (nullptr != src.parameter_ && 0 != parameterLength_) {
        parameter_ = std::make_unique<uint8_t[]>(parameterLength_);
        (void)memcpy_s(parameter_.get(), parameterLength_, src.parameter_.get(), parameterLength_);
    } else {
        parameter_.reset(nullptr);
        parameterLength_ = 0;
    }

    if (nullptr != src.result_ && 0 != resultLength_) {
        result_ = std::make_unique<uint8_t[]>(resultLength_);
        (void)memcpy_s(result_.get(), resultLength_, src.result_.get(), resultLength_);
    } else {
        result_.reset(nullptr);
        resultLength_ = 0;
    }
}

SsapMethod &SsapMethod::operator=(const SsapMethod &src)
{
    if (this != &src) {
        handle_ = src.handle_;
        methodType_ = src.methodType_;
        uuid_ = src.uuid_;
        parameterLength_ = src.parameterLength_;
        resultLength_ = src.resultLength_;
        permissions_ = src.permissions_;
        serviceUuid_ = src.serviceUuid_;

        if (nullptr != src.parameter_ && 0 != parameterLength_) {
            parameter_ = std::make_unique<uint8_t[]>(parameterLength_);
            (void)memcpy_s(parameter_.get(), parameterLength_, src.parameter_.get(), parameterLength_);
        } else {
            parameter_.reset(nullptr);
            parameterLength_ = 0;
        }

        if (nullptr != src.result_ && 0 != resultLength_) {
            result_ = std::make_unique<uint8_t[]>(resultLength_);
            (void)memcpy_s(result_.get(), resultLength_, src.result_.get(), resultLength_);
        } else {
            result_.reset(nullptr);
            resultLength_ = 0;
        }
    }
    return *this;
}

int SsapMethod::GetMethodType() const
{
    return methodType_;
}

uint16_t SsapMethod::GetHandle() const
{
    return handle_;
}

const UUID &SsapMethod::GetServiceUuid() const
{
    return serviceUuid_;
}

int SsapMethod::GetPermissions() const
{
    return permissions_;
}

const UUID &SsapMethod::GetUuid() const
{
    return uuid_;
}

const std::unique_ptr<uint8_t[]> &SsapMethod::GetParameter(size_t *size) const
{
    *size = parameterLength_;
    HILOGI("method parameter_ size : %{public}lu", parameterLength_);
    return parameter_;
}

const std::unique_ptr<uint8_t[]> &SsapMethod::GetResult(size_t *size) const
{
    *size = resultLength_;
    HILOGI("method result size : %{public}lu", resultLength_);
    return result_;
}

void SsapMethod::SetParameter(const uint8_t *values, const size_t length)
{
    if (values == nullptr || length == 0) {
        HILOGE("Parameter values is nullptr, or length is 0");
        return;
    }
    parameter_ = std::make_unique<uint8_t[]>(length);
    parameterLength_ = length;
    (void)memcpy_s(parameter_.get(), length, values, length);
}

void SsapMethod::SetResult(const uint8_t *values, const size_t length)
{
    if (values == nullptr || length == 0) {
        HILOGE("values is nullptr, or length is 0");
        return;
    }
    result_ = std::make_unique<uint8_t[]>(length);
    resultLength_ = length;
    HILOGI("method result size : %{public}lu", resultLength_);
    (void)memcpy_s(result_.get(), length, values, length);
}

void SsapMethod::SetServiceUuid(const UUID &uuid)
{
    serviceUuid_ = uuid;
}
}
}