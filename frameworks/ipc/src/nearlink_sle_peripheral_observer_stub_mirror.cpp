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

#include "nearlink_sle_peripheral_observer_stub.h"
#include "nearlink_sle_peripheral_observer_stub_core.h"

namespace OHOS {
namespace Nearlink {

namespace {
inline NearlinkSlePeripheralObserverStubCore *ToCore(NearlinkSlePeripheralObserverStub *self)
{
    return reinterpret_cast<NearlinkSlePeripheralObserverStubCore *>(self);
}
}

NearlinkSlePeripheralObserverStub::NearlinkSlePeripheralObserverStub()
{
}

NearlinkSlePeripheralObserverStub::~NearlinkSlePeripheralObserverStub()
{
}

int NearlinkSlePeripheralObserverStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    return ToCore(this)->NearlinkSlePeripheralObserverStubCore::OnRemoteRequest(code, data, reply, option);
}

}  // namespace Nearlink
}  // namespace OHOS
