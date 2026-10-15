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

#ifndef NEARLINK_OBSERVER_ADAPTER_H
#define NEARLINK_OBSERVER_ADAPTER_H

#include <memory>

#include "nearlink_host.h"
#include "nearlink_base_host.h"

namespace OHOS {
namespace Nearlink {

std::shared_ptr<NearlinkBaseHostObserver> WrapHostObserver(std::shared_ptr<NearlinkHostObserver> observer);
bool UnwrapHostObserver(
    std::shared_ptr<NearlinkHostObserver> observer, std::shared_ptr<NearlinkBaseHostObserver> &adapter);

std::shared_ptr<NearlinkBaseRemoteDeviceObserver> WrapRemoteDeviceObserver(
    std::shared_ptr<NearlinkRemoteDeviceObserver> observer);
bool UnwrapRemoteDeviceObserver(std::shared_ptr<NearlinkRemoteDeviceObserver> observer,
    std::shared_ptr<NearlinkBaseRemoteDeviceObserver> &adapter);

std::shared_ptr<NearlinkBaseRemoteDeviceBatteryObserver> WrapBatteryObserver(
    std::shared_ptr<NearlinkRemoteDeviceBatteryObserver> observer);
bool UnwrapBatteryObserver(std::shared_ptr<NearlinkRemoteDeviceBatteryObserver> observer,
    std::shared_ptr<NearlinkBaseRemoteDeviceBatteryObserver> &adapter);

std::shared_ptr<NearlinkBaseRemoteDeviceRssiObserver> WrapRssiObserver(
    std::shared_ptr<NearlinkRemoteDeviceRssiObserver> observer);
bool UnwrapRssiObserver(std::shared_ptr<NearlinkRemoteDeviceRssiObserver> observer,
    std::shared_ptr<NearlinkBaseRemoteDeviceRssiObserver> &adapter);

} // namespace Nearlink
} // namespace OHOS

#endif // NEARLINK_OBSERVER_ADAPTER_H
