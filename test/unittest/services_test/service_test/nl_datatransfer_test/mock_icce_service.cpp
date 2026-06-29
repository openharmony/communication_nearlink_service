/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include <future>
#include "ClassCreator.h"
#include "SleServiceFfrtLog.h"
#include "IcceService.h"
#include "IcceDefines.h"

namespace OHOS {
namespace Nearlink {
constexpr uint8_t ICCE_MAX_CONNECTION_NUM = 6;

struct IcceService::impl {
    impl() = default;
    ~impl() = default;
};

IcceService* IcceService::GetService()
{
    HILOGI("[ICCE Profile Mock] GetService");
    static IcceService service;
    return &service;
}

IcceService::IcceService()
    : utility::Context(PROFILE_NAME_ICCE, "1.0.0")
{
    HILOGD("[ICCE Profile Mock]%{public}s:%{public}s Create", PROFILE_NAME_ICCE.c_str(), Name().c_str());
}

IcceService::~IcceService()
{
    HILOGD("[ICCE Profile Mock]%{public}s:%{public}s Destroy", PROFILE_NAME_ICCE.c_str(), Name().c_str());
}

utility::Context *IcceService::GetContext()
{
    return this;
}

void IcceService::EnableTask()
{
}

void IcceService::Enable()
{
    HILOGI("[ICCE Profile Mock] Enable");
}

void IcceService::DisableTask()
{
}

void IcceService::Disable()
{
}

void IcceService::ConnectTask(const RawAddress &device)
{
}

int IcceService::Connect(const RawAddress &device)
{
    HILOGI("[ICCE Service Mock] Connect addr(%{public}s)", GET_ENCRYPT_ADDR(device));
    return ICCE_SUCCESS;
}

void IcceService::DisconnectTask(const RawAddress &device)
{
}

int IcceService::Disconnect(const RawAddress &device)
{
    HILOGI("[ICCE Service Mock] Disconnect addr(%{public}s)", GET_ENCRYPT_ADDR(device));
    return ICCE_SUCCESS;
}

int32_t IcceService::GetPortTask(const RawAddress &device)
{
    return 0;
}

int32_t IcceService::GetPort(const RawAddress &device)
{
    HILOGI("[ICCE Profile Mock] GetPort");
    return 40960;
}

void IcceService::RegisterObserver(IcceObserver &icceObserver)
{
}

void IcceService::DeregisterObserver(IcceObserver &icceObserver)
{
}

uint8_t IcceService::GetConnectionsDeviceNum()
{
    HILOGI("[ICCE Profile Mock] GetConnectionsDeviceNum");
    return 1;
}

REGISTER_CLASS_CREATOR(IcceService);

} // namespace Nearlink
} // namespace OHOS