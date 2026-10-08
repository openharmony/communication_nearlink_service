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

#include "nearlink_device_rssi_observer_stub.h"
#include "nearlink_device_rssi_observer_stub_core.h"

namespace OHOS {
namespace Nearlink {

namespace {
inline NearlinkDeviceRssiObserverStubCore *ToCore(NearlinkDeviceRssiObserverStub *self)
{
    return reinterpret_cast<NearlinkDeviceRssiObserverStubCore *>(self);
}
}

NearlinkDeviceRssiObserverStub::NearlinkDeviceRssiObserverStub()
{
}

NearlinkDeviceRssiObserverStub::~NearlinkDeviceRssiObserverStub()
{
}

int32_t NearlinkDeviceRssiObserverStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    return ToCore(this)->NearlinkDeviceRssiObserverStubCore::OnRemoteRequest(code, data, reply, option);
}

}  // namespace Nearlink
}  // namespace OHOS
