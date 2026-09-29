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

#include "nearlink_device_battery_observer_stub_core.h"
#include "log.h"
#include "ipc_types.h"
#include "string_ex.h"

namespace OHOS {
namespace Nearlink {
NearlinkDeviceBatteryObserverStubCore::NearlinkDeviceBatteryObserverStubCore()
{
    memberFuncMap_ = {
        {static_cast<uint32_t>(NearlinkDeviceBatteryObserverInterfaceCode::NL_SLE_GET_BATTERYLEVEL_EVENT),
            &NearlinkDeviceBatteryObserverStubCore::OnGetBatteryLevelEventInner},
        {static_cast<uint32_t>(NearlinkDeviceBatteryObserverInterfaceCode::NL_SLE_BATTERYLEVEL_CHANGED),
            &NearlinkDeviceBatteryObserverStubCore::OnBatteryLevelChangedInner},
    };
}

NearlinkDeviceBatteryObserverStubCore::~NearlinkDeviceBatteryObserverStubCore()
{
    memberFuncMap_.clear();
}

int NearlinkDeviceBatteryObserverStubCore::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    HILOGD("cmd=%{public}d, flags=%{public}d", code, option.GetFlags());
    if (NearlinkDeviceBatteryObserverStubCore::GetDescriptor() != data.ReadInterfaceToken()) {
        HILOGI("local descriptor is not equal to remote");
        return ERR_INVALID_STATE;
    }

    auto itFunc = memberFuncMap_.find(code);
    if (itFunc != memberFuncMap_.end()) {
        auto memberFunc = itFunc->second;
        if (memberFunc != nullptr) {
            return (this->*memberFunc)(data, reply);
        }
    }
    HILOGW("default case, need check.");
    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}

ErrCode NearlinkDeviceBatteryObserverStubCore::OnGetBatteryLevelEventInner(MessageParcel &data, MessageParcel &reply)
{
    std::shared_ptr<NearlinkRawAddress> device(data.ReadParcelable<NearlinkRawAddress>());
    NL_CHECK_RETURN_RET(device, TRANSACTION_ERR, "device transaction wrong.");

    int32_t batteryLevel = data.ReadInt32();

    HILOGI("starts");
    OnGetBatteryLevelEvent(*device, batteryLevel);

    return NO_ERROR;
}

ErrCode NearlinkDeviceBatteryObserverStubCore::OnBatteryLevelChangedInner(MessageParcel &data, MessageParcel &reply)
{
    std::shared_ptr<NearlinkRawAddress> device(data.ReadParcelable<NearlinkRawAddress>());
    NL_CHECK_RETURN_RET(device, TRANSACTION_ERR, "device transaction wrong.");

    int32_t batteryLevel = data.ReadInt32();

    HILOGI("starts");
    OnBatteryLevelChanged(*device, batteryLevel);

    return NO_ERROR;
}
}  // namespace Nearlink
}  // namespace OHOS