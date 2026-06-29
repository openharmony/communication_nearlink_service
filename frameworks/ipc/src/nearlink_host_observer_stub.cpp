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

#include "nearlink_host_observer_stub.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
NearlinkHostObserverStub::NearlinkHostObserverStub()
{
    HILOGD("enter");
    memberFuncMap_[static_cast<uint32_t>(NearlinkHostObserverInterfaceCode::NL_HOST_OBSERVER_STATE_CHANGE)] =
        &NearlinkHostObserverStub::OnStateChangedInner;
    memberFuncMap_[static_cast<uint32_t>(NearlinkHostObserverInterfaceCode::NL_HOST_OBSERVER_PAIR_CONFIRMED)] =
        &NearlinkHostObserverStub::OnPairConfirmedInner;
    memberFuncMap_[static_cast<uint32_t>(NearlinkHostObserverInterfaceCode::NL_HOST_OBSERVER_DEVICE_NAME_CHANGED)] =
        &NearlinkHostObserverStub::OnDeviceNameChangedInner;
    memberFuncMap_[static_cast<uint32_t>(NearlinkHostObserverInterfaceCode::NL_HOST_OBSERVER_DEVICE_ADDR_CHANGED)] =
        &NearlinkHostObserverStub::OnDeviceAddrChangedInner;
    memberFuncMap_[static_cast<uint32_t>(NearlinkHostObserverInterfaceCode::NL_HOST_OBSERVER_FULL_STATE_CHANGE)] =
        &NearlinkHostObserverStub::OnFullStateChangedInner;
    memberFuncMap_[static_cast<uint32_t>(NearlinkHostObserverInterfaceCode::NL_HOST_OBSERVER_SWITCH_STATE_CHANGED)] =
        &NearlinkHostObserverStub::OnSwitchStateChangedInner;
    memberFuncMap_[static_cast<uint32_t>(NearlinkHostObserverInterfaceCode::NL_HOST_OBSERVER_DISABLE_RESPONSE)] =
        &NearlinkHostObserverStub::OnDisableResponseInner;
}

NearlinkHostObserverStub::~NearlinkHostObserverStub()
{
    HILOGD("enter");
    memberFuncMap_.clear();
}

int32_t NearlinkHostObserverStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    HILOGD("transaction of code: %{public}d is received", code);
    if (NearlinkHostObserverStub::GetDescriptor() != data.ReadInterfaceToken()) {
        HILOGE("local descriptor is not equal to remote");
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

ErrCode NearlinkHostObserverStub::OnStateChangedInner(MessageParcel &data, MessageParcel &reply)
{
    int32_t transport = data.ReadInt32();
    int32_t state = data.ReadInt32();

    HILOGD("starts");
    OnStateChanged(transport, state);

    return NO_ERROR;
}

ErrCode NearlinkHostObserverStub::OnFullStateChangedInner(MessageParcel &data, MessageParcel &reply)
{
    int32_t transport = data.ReadInt32();
    int32_t state = data.ReadInt32();

    HILOGD("starts");
    OnFullStateChanged(transport, state);

    return NO_ERROR;
}

ErrCode NearlinkHostObserverStub::OnSwitchStateChangedInner(MessageParcel &data, MessageParcel &reply)
{
    int32_t state = data.ReadInt32();

    HILOGD("starts");
    OnSwitchStateChanged(state);

    return NO_ERROR;
}

ErrCode NearlinkHostObserverStub::OnDisableResponseInner(MessageParcel &data, MessageParcel &reply)
{
    bool isHalfDisable = data.ReadBool();

    HILOGD("starts");
    OnDisableResponse(isHalfDisable);

    return NO_ERROR;
}

ErrCode NearlinkHostObserverStub::OnPairConfirmedInner(MessageParcel &data, MessageParcel &reply)
{
    int32_t transport = data.ReadInt32();
    std::shared_ptr<NearlinkRawAddress> device(data.ReadParcelable<NearlinkRawAddress>());
    if (!device) {
        return TRANSACTION_ERR;
    }
    int32_t reqType = data.ReadInt32();
    int32_t number = data.ReadInt32();

    HILOGI("starts");
    OnPairConfirmed(transport, *device, reqType, number);

    return NO_ERROR;
}

ErrCode NearlinkHostObserverStub::OnDeviceNameChangedInner(MessageParcel &data, MessageParcel &reply)
{
    std::string deviceName = data.ReadString();

    HILOGI("starts");
    OnDeviceNameChanged(deviceName);

    return NO_ERROR;
}

ErrCode NearlinkHostObserverStub::OnDeviceAddrChangedInner(MessageParcel &data, MessageParcel &reply)
{
    std::string address = data.ReadString();

    HILOGI("OnDeviceAddrChangedInner starts");
    OnDeviceAddrChanged(address);

    return NO_ERROR;
}
}  // namespace Nearlink
}  // namespace OHOS