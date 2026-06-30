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

#include "nearlink_asc_callback_stub.h"
#include "log.h"
#include "log_util.h"
#include "raw_address.h"
#include "nearlink_ssap_service_parcel.h"

namespace OHOS {
namespace Nearlink {
NearlinkASCCallbackStub::NearlinkASCCallbackStub()
{
    HILOGD("start.");
    memberFuncMap_[static_cast<uint32_t>(
        NearlinkASCCallbackInterfaceCode::NL_ASC_CALLBACK_CONTROL)] =
        &NearlinkASCCallbackStub::OnAudioControlInner;
    memberFuncMap_[static_cast<uint32_t>(
        NearlinkASCCallbackInterfaceCode::NL_ASC_CALLBACK_ADD_DEVICE)] =
        &NearlinkASCCallbackStub::OnAddSleAudioDeviceInner;
    memberFuncMap_[static_cast<uint32_t>(
        NearlinkASCCallbackInterfaceCode::NL_ASC_CALLBACK_DELETE_DEVICE)] =
        &NearlinkASCCallbackStub::OnDeleteSleAudioDeviceInner;
    memberFuncMap_[static_cast<uint32_t>(
        NearlinkASCCallbackInterfaceCode::NL_ASC_CALLBACK_DEVICE_ACTION_CHANGED)] =
        &NearlinkASCCallbackStub::OnSleAudioDeviceActionChangedInner;
    memberFuncMap_[static_cast<uint32_t>(
        NearlinkASCCallbackInterfaceCode::NL_ASC_CALLBACK_ADD_VIRTUAL_AUDIO_DEVICE)] =
        &NearlinkASCCallbackStub::OnAddSleVirtualAudioDeviceInner;
    memberFuncMap_[static_cast<uint32_t>(
        NearlinkASCCallbackInterfaceCode::NL_ASC_CALLBACK_DELETE_VIRTUAL_AUDIO_DEVICE)] =
        &NearlinkASCCallbackStub::OnDeleteSleVirtualAudioDeviceInner;
}

NearlinkASCCallbackStub::~NearlinkASCCallbackStub()
{
    HILOGD("start.");
    memberFuncMap_.clear();
}

int NearlinkASCCallbackStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    HILOGD("NearlinkASCCallbackStub::OnRemoteRequest, cmd = %{public}d, flags= %{public}d",
        code, option.GetFlags());
    if (NearlinkASCCallbackStub::GetDescriptor() != data.ReadInterfaceToken()) {
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
    HILOGW("NearlinkASCCallbackStub::OnRemoteRequest, default case, need check.");
    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}

ErrCode NearlinkASCCallbackStub::OnAudioControlInner(MessageParcel &data, MessageParcel &reply)
{
    HILOGI("NearlinkASCCallbackStub::OnAudioControlInner Triggered!");
    std::shared_ptr<NearlinkRawAddress> device(data.ReadParcelable<NearlinkRawAddress>());
    if (!device) {
        return TRANSACTION_ERR;
    }
    std::shared_ptr<NearlinkASCAudioControlResult> result(data.ReadParcelable<NearlinkASCAudioControlResult>());
    if (!result) {
        return TRANSACTION_ERR;
    }
    HILOGI("NearlinkASCCallbackStub OnAudioControl ret: %{public}d", result->GetResult());
    OnAudioControl(*device, *result);
    return NO_ERROR;
}

ErrCode NearlinkASCCallbackStub::OnAddSleAudioDeviceInner(MessageParcel &data, MessageParcel &reply)
{
    HILOGD("NearlinkASCCallbackStub::OnAddSleAudioDeviceInner Triggered!");
    std::shared_ptr<NearlinkRawAddress> device(data.ReadParcelable<NearlinkRawAddress>());
    if (!device) {
        return TRANSACTION_ERR;
    }
    uint32_t streamType = data.ReadUint32();
    int32_t mediaVolume = data.ReadInt32();
    int32_t callVolume = data.ReadInt32();
    HILOGI("NearlinkASCCallbackStub OnAddSleAudioDevice %{public}s streamType %{public}d, mediaVolume %{public}d, "
        "callVolume %{public}d", GetEncryptAddr(device->GetAddress()).c_str(), streamType, mediaVolume, callVolume);
    OnAddSleAudioDevice(*device, streamType, mediaVolume, callVolume);
    return NO_ERROR;
}

ErrCode NearlinkASCCallbackStub::OnDeleteSleAudioDeviceInner(MessageParcel &data, MessageParcel &reply)
{
    HILOGD("NearlinkASCCallbackStub::OnDeleteSleAudioDeviceInner Triggered!");
    std::shared_ptr<NearlinkRawAddress> device(data.ReadParcelable<NearlinkRawAddress>());
    if (!device) {
        return TRANSACTION_ERR;
    }
    OnDeleteSleAudioDevice(*device);
    return NO_ERROR;
}

ErrCode NearlinkASCCallbackStub::OnSleAudioDeviceActionChangedInner(MessageParcel &data, MessageParcel &reply)
{
    HILOGI("NearlinkASCCallbackStub::OnSleAudioDeviceActionChangedInner Triggered!");
    std::shared_ptr<NearlinkRawAddress> device(data.ReadParcelable<NearlinkRawAddress>());
    if (!device) {
        return TRANSACTION_ERR;
    }
    std::shared_ptr<NearlinkASCAudioStreamInfo> streamInfo(data.ReadParcelable<NearlinkASCAudioStreamInfo>());
    if (!streamInfo) {
        return TRANSACTION_ERR;
    }
    int32_t action = data.ReadInt32();
    HILOGI("NearlinkASCCallbackStub OnSleAudioDeviceActionChanged %{public}s action %{public}d",
        GetEncryptAddr(device->GetAddress()).c_str(), action);
    OnSleAudioDeviceActionChanged(*device, *streamInfo, action);
    return NO_ERROR;
}

ErrCode NearlinkASCCallbackStub::OnAddSleVirtualAudioDeviceInner(MessageParcel &data, MessageParcel &reply)
{
    HILOGI("NearlinkASCCallbackStub::OnAddSleVirtualAudioDeviceInner Triggered!");
    std::shared_ptr<NearlinkRawAddress> device(data.ReadParcelable<NearlinkRawAddress>());
    if (!device) {
        return TRANSACTION_ERR;
    }
    AudioStreamType streamType = static_cast<AudioStreamType>(data.ReadUint32());
    HILOGI("NearlinkASCCallbackStub OnAddSleVirtualAudioDevice %{public}s, streamType %{public}d",
        GetEncryptAddr(device->GetAddress()).c_str(), streamType);
    OnAddSleVirtualAudioDevice(*device, streamType);
    return NO_ERROR;
}

ErrCode NearlinkASCCallbackStub::OnDeleteSleVirtualAudioDeviceInner(MessageParcel &data, MessageParcel &reply)
{
    HILOGI("NearlinkASCCallbackStub::OnDeleteSleVirtualAudioDeviceInner Triggered!");
    std::shared_ptr<NearlinkRawAddress> device(data.ReadParcelable<NearlinkRawAddress>());
    if (!device) {
        return TRANSACTION_ERR;
    }
    HILOGI("NearlinkASCCallbackStub OnDeleteSleVirtualAudioDevice %{public}s",
        GetEncryptAddr(device->GetAddress()).c_str());
    OnDeleteSleVirtualAudioDevice(*device);
    return NO_ERROR;
}

}  // namespace Nearlink
}  // namespace OHOS
