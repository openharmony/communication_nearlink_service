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

#include "base/nearlink_sle_peripheral_observer_stub_core.h"
#include "log.h"
#include "ipc_types.h"
#include "string_ex.h"

namespace OHOS {
namespace Nearlink {
NearlinkSlePeripheralObserverStubCore::NearlinkSlePeripheralObserverStubCore()
{
    memberFuncMap_ = {
        {static_cast<uint32_t>(NearlinkSlePeripheralObserverInterfaceCode::NL_SLE_ON_READ_REMOTE_RSSI_EVENT),
            &NearlinkSlePeripheralObserverStubCore::OnReadRemoteRssiEventInner},
        {static_cast<uint32_t>(NearlinkSlePeripheralObserverInterfaceCode::NL_SLE_PAIRING_REQUEST),
            &NearlinkSlePeripheralObserverStubCore::OnPairRequestInner},
        {static_cast<uint32_t>(NearlinkSlePeripheralObserverInterfaceCode::NL_SLE_PAIR_STATUS_CHANGED),
            &NearlinkSlePeripheralObserverStubCore::OnPairStatusChangedInner},
        {static_cast<uint32_t>(NearlinkSlePeripheralObserverInterfaceCode::NL_SLE_ACB_STATE_CHANGED),
            &NearlinkSlePeripheralObserverStubCore::OnAcbStateChangedInner},
        {static_cast<uint32_t>(NearlinkSlePeripheralObserverInterfaceCode::NL_SLE_CONNECT_STATE_CHANGED),
            &NearlinkSlePeripheralObserverStubCore::OnConnectionStateChangedInner},
        {static_cast<uint32_t>(NearlinkSlePeripheralObserverInterfaceCode::NL_SLE_LINK_FREQ_BAND_CHANGED),
            &NearlinkSlePeripheralObserverStubCore::OnLinkFreqBandChangedInner},
    };
}

NearlinkSlePeripheralObserverStubCore::~NearlinkSlePeripheralObserverStubCore()
{
    memberFuncMap_.clear();
}

int NearlinkSlePeripheralObserverStubCore::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    HILOGD("cmd=%{public}d, flags=%{public}d", code, option.GetFlags());
    if (NearlinkSlePeripheralObserverStubCore::GetDescriptor() != data.ReadInterfaceToken()) {
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

ErrCode NearlinkSlePeripheralObserverStubCore::OnReadRemoteRssiEventInner(MessageParcel &data, MessageParcel &reply)
{
    std::shared_ptr<NearlinkRawAddress> device(data.ReadParcelable<NearlinkRawAddress>());
    if (!device) {
        return TRANSACTION_ERR;
    }
    const int32_t rssi = data.ReadInt32();
    const int32_t status = data.ReadInt32();

    OnReadRemoteRssiEvent(*device, rssi, status);
    return NO_ERROR;
}

ErrCode NearlinkSlePeripheralObserverStubCore::OnPairRequestInner(MessageParcel &data, MessageParcel &reply)
{
    std::shared_ptr<NearlinkRawAddress> device(data.ReadParcelable<NearlinkRawAddress>());
    if (!device) {
        return TRANSACTION_ERR;
    }

    const std::string passkey = data.ReadString();
    const int32_t type = data.ReadInt32();

    OnPairingRequest(*device, passkey, type);
    return NO_ERROR;
}

ErrCode NearlinkSlePeripheralObserverStubCore::OnPairStatusChangedInner(MessageParcel &data, MessageParcel &reply)
{
    std::shared_ptr<NearlinkRawAddress> device(data.ReadParcelable<NearlinkRawAddress>());
    if (!device) {
        return TRANSACTION_ERR;
    }
    const int32_t preState = data.ReadInt32();
    const int32_t state = data.ReadInt32();
    const int32_t reason = data.ReadInt32();

    OnPairStatusChanged(*device, preState, state, reason);
    return NO_ERROR;
}

ErrCode NearlinkSlePeripheralObserverStubCore::OnConnectionStateChangedInner(MessageParcel &data, MessageParcel &reply)
{
    std::shared_ptr<NearlinkRawAddress> device(data.ReadParcelable<NearlinkRawAddress>());
    if (!device) {
        return TRANSACTION_ERR;
    }
    const int32_t preState = data.ReadInt32();
    const int32_t state = data.ReadInt32();
    const int32_t reason = data.ReadInt32();

    OnConnectionStateChanged(*device, preState, state, reason);
    return NO_ERROR;
}

ErrCode NearlinkSlePeripheralObserverStubCore::OnAcbStateChangedInner(MessageParcel &data, MessageParcel &reply)
{
    std::shared_ptr<NearlinkRawAddress> device(data.ReadParcelable<NearlinkRawAddress>());
    if (!device) {
        return TRANSACTION_ERR;
    }
    const int32_t state = data.ReadInt32();
    const int32_t reason = data.ReadInt32();

    OnAcbStateChanged(*device, state, reason);
    return NO_ERROR;
}

ErrCode NearlinkSlePeripheralObserverStubCore::OnLinkFreqBandChangedInner(MessageParcel &data, MessageParcel &reply)
{
    std::shared_ptr<NearlinkRawAddress> device(data.ReadParcelable<NearlinkRawAddress>());
    NL_CHECK_RETURN_RET(device, TRANSACTION_ERR, "device transaction wrong.");

    int32_t freqBand = data.ReadInt32();

    HILOGI("starts");
    OnLinkFreqBandChanged(*device, freqBand);

    return NO_ERROR;
}
}
}  // namespace Nearlink
}  // namespace OHOS