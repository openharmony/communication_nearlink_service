/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "nearlink_asc_callback_proxy.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
void NearlinkASCCallbackProxy::OnAudioControl(const NearlinkRawAddress &device,
    const NearlinkASCAudioControlResult& result)
{
    HILOGI("Triggered!");
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkASCCallbackProxy::GetDescriptor()),
        "Write Token error");
    NL_CHECK_RETURN(data.WriteParcelable(&device), "Write device error");
    NL_CHECK_RETURN(data.WriteParcelable(&result), "Write result error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_ASYNC
    };
    ErrCode error = InnerTransact(
        NearlinkASCCallbackInterfaceCode::NL_ASC_CALLBACK_CONTROL,
        option, data, reply);
    NL_CHECK_RETURN(error == NO_ERROR, "done fail, error: %{public}d", error);
}

void NearlinkASCCallbackProxy::OnAddSleAudioDevice(const NearlinkRawAddress &device, uint32_t streamType,
    int32_t mediaVolume, int32_t callVolume)
{
    HILOGD("Triggered!");
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkASCCallbackProxy::GetDescriptor()),
        "Write Token error");
    NL_CHECK_RETURN(data.WriteParcelable(&device), "Write device error");
    NL_CHECK_RETURN(data.WriteUint32(streamType), "Write streamType error");
    NL_CHECK_RETURN(data.WriteInt32(mediaVolume), "Write mediaVolume error");
    NL_CHECK_RETURN(data.WriteInt32(callVolume), "Write callVolume error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_ASYNC
    };
    ErrCode error = InnerTransact(
        NearlinkASCCallbackInterfaceCode::NL_ASC_CALLBACK_ADD_DEVICE,
        option, data, reply);
    NL_CHECK_RETURN(error == NO_ERROR, "done fail, error: %{public}d", error);
}

void NearlinkASCCallbackProxy::OnDeleteSleAudioDevice(const NearlinkRawAddress &device)
{
    HILOGD("Triggered!");
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkASCCallbackProxy::GetDescriptor()),
        "Write Token error");
    NL_CHECK_RETURN(data.WriteParcelable(&device), "Write device error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_ASYNC
    };
    ErrCode error = InnerTransact(
        NearlinkASCCallbackInterfaceCode::NL_ASC_CALLBACK_DELETE_DEVICE,
        option, data, reply);
    NL_CHECK_RETURN(error == NO_ERROR, "done fail, error: %{public}d", error);
}

void NearlinkASCCallbackProxy::OnSleAudioDeviceActionChanged(const NearlinkRawAddress &device,
    const NearlinkASCAudioStreamInfo &streamInfo, int action)
{
    HILOGD("Triggered!");
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkASCCallbackProxy::GetDescriptor()),
        "Write Token error");
    NL_CHECK_RETURN(data.WriteParcelable(&device), "Write device error");
    NL_CHECK_RETURN(data.WriteParcelable(&streamInfo), "Write stream info error");
    NL_CHECK_RETURN(data.WriteInt32(action), "Write action error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_ASYNC
    };
    ErrCode error = InnerTransact(
        NearlinkASCCallbackInterfaceCode::NL_ASC_CALLBACK_DEVICE_ACTION_CHANGED,
        option, data, reply);
    NL_CHECK_RETURN(error == NO_ERROR, "done fail, error: %{public}d", error);
}

void NearlinkASCCallbackProxy::OnAddSleVirtualAudioDevice(const NearlinkRawAddress &device, AudioStreamType streamType)
{
    HILOGI("Triggered!");
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkASCCallbackProxy::GetDescriptor()),
        "Write Token error");
    NL_CHECK_RETURN(data.WriteParcelable(&device), "Write device error");
    NL_CHECK_RETURN(data.WriteUint32(streamType), "Write streamType error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_ASYNC
    };
    ErrCode error = InnerTransact(
        NearlinkASCCallbackInterfaceCode::NL_ASC_CALLBACK_ADD_VIRTUAL_AUDIO_DEVICE,
        option, data, reply);
    NL_CHECK_RETURN(error == NO_ERROR, "done fail, error: %{public}d", error);
}

void NearlinkASCCallbackProxy::OnDeleteSleVirtualAudioDevice(const NearlinkRawAddress &device)
{
    HILOGI("Triggered!");
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkASCCallbackProxy::GetDescriptor()),
        "Write Token error");
    NL_CHECK_RETURN(data.WriteParcelable(&device), "Write device error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_ASYNC
    };
    ErrCode error = InnerTransact(
        NearlinkASCCallbackInterfaceCode::NL_ASC_CALLBACK_DELETE_VIRTUAL_AUDIO_DEVICE,
        option, data, reply);
    NL_CHECK_RETURN(error == NO_ERROR, "done fail, error: %{public}d", error);
}

uid_t NearlinkASCCallbackProxy::GetUid()
{
    return uid_;
}

ErrCode NearlinkASCCallbackProxy::InnerTransact(uint32_t code, MessageOption &flags, MessageParcel &data,
    MessageParcel &reply)
{
    auto remote = Remote();
    NL_CHECK_RETURN_RET(remote, OBJECT_NULL, "[InnerTransact] fail: get Remote fail code %{public}d", code);
    ErrCode err = remote->SendRequest(code, data, reply, flags);
    switch (err) {
        case NO_ERROR: {
            return NO_ERROR;
        }
        case DEAD_OBJECT: {
            HILOGW("fail: ipcErr=%{public}d code %{public}d", err, code);
            return DEAD_OBJECT;
        }
        default: {
            HILOGW("fail: ipcErr=%{public}d code %{public}d", err, code);
            return TRANSACTION_ERR;
        }
    }
}
}  // namespace Nearlink
}  // namespace OHOS