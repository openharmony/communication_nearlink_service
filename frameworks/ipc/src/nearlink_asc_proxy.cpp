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

#include "nearlink_errorcode.h"
#include "log.h"
#include "nearlink_uuid_parcel.h"
#include "nearlink_asc_proxy.h"

namespace OHOS {
namespace Nearlink {
NlErrCode NearlinkASCProxy::RegisterApplication(const sptr<INearlinkASCCallback> &callback)
{
    HILOGI("start");
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkASCProxy::GetDescriptor()),
        NL_ERR_INTERNAL_ERROR, "Write Token error");
    NL_CHECK_RETURN_RET(data.WriteRemoteObject(callback->AsObject()), NL_ERR_INTERNAL_ERROR,
        "Write RemoteObject error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_SYNC
    };
    ErrCode error = InnerTransact(
        NearlinkASCInterfaceCode::NL_ASC_REGISTER_APP, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkASCProxy::DeregisterApplication(const sptr<INearlinkASCCallback> &callback)
{
    HILOGI("start");
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkASCProxy::GetDescriptor()),
        NL_ERR_INTERNAL_ERROR, "Write Token error");
    NL_CHECK_RETURN_RET(data.WriteRemoteObject(callback->AsObject()), NL_ERR_INTERNAL_ERROR,
        "Write RemoteObject error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_SYNC
    };
    ErrCode error = InnerTransact(
        NearlinkASCInterfaceCode::NL_ASC_DEREGISTER_APP, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkASCProxy::AudioControl(const NearlinkRawAddress &device, AudioStreamType streamType, int cmd)
{
    HILOGI("NearlinkASCProxy::AudioControl start");
    MessageParcel data;
    if (!data.WriteInterfaceToken(NearlinkASCProxy::GetDescriptor())) {
        HILOGE("NearlinkASCProxy::AudioControl WriteInterfaceToken error");
        return NL_ERR_INTERNAL_ERROR;
    }

    NL_CHECK_RETURN_RET(data.WriteParcelable(&device), NL_ERR_INTERNAL_ERROR, "Write device error");
    NL_CHECK_RETURN_RET(data.WriteUint32(streamType), NL_ERR_INTERNAL_ERROR, "Write streamType error");
    NL_CHECK_RETURN_RET(data.WriteInt32(cmd), NL_ERR_INTERNAL_ERROR, "Write cmd error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_SYNC
    };
    ErrCode error = InnerTransact(
        NearlinkASCInterfaceCode::NL_ASC_CONTROL, option, data, reply);
    if (error != NO_ERROR) {
        HILOGE("NearlinkASCProxy::AudioControl done fail, error: %{public}d", error);
        return NL_ERR_IPC_TRANS_FAILED;
    }
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkASCProxy::GetAudioDeviceList(std::vector<NearlinkRawAddress> &devices)
{
    HILOGI("NearlinkASCProxy::GetSleAudioDeviceList start");
    MessageParcel data;
    if (!data.WriteInterfaceToken(NearlinkASCProxy::GetDescriptor())) {
        HILOGE("NearlinkASCProxy::GetSleAudioDeviceList WriteInterfaceToken error");
        return NL_ERR_INTERNAL_ERROR;
    }

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_SYNC
    };
    ErrCode error = InnerTransact(
        NearlinkASCInterfaceCode::NL_ASC_GET_AUDIO_DEVICE_LIST, option, data, reply);
    if (error != NO_ERROR) {
        HILOGE("NearlinkASCProxy::GetSleAudioDeviceList done fail, error: %{public}d", error);
        return NL_ERR_IPC_TRANS_FAILED;
    }

    int result = reply.ReadInt32();
    if (result != NO_ERROR) {
        HILOGE("NearlinkASCProxy::GetSleAudioDeviceList done fail, result: %{public}d", result);
        return NL_ERR_INTERNAL_ERROR;
    }

    uint32_t vecCnt = reply.ReadUint32();
    if ((vecCnt <= 0) || (vecCnt > MAX_AUDIO_DEVICE_COUNT)) {
        HILOGE("vector size is error");
        return NL_ERR_IPC_TRANS_FAILED;
    }
    for (uint32_t i = 0; i < vecCnt; i++) {
        std::shared_ptr<NearlinkRawAddress> device(reply.ReadParcelable<NearlinkRawAddress>());
        if (!device) {
            return NL_ERR_IPC_TRANS_FAILED;
        }
        devices.push_back(*device);
    }

    return static_cast<NlErrCode>(result);
}

NlErrCode NearlinkASCProxy::GetVirtualAudioDeviceList(std::vector<NearlinkRawAddress> &devices)
{
    HILOGI("NearlinkASCProxy::GetVirtualAudioDeviceList start");
    MessageParcel data;
    if (!data.WriteInterfaceToken(NearlinkASCProxy::GetDescriptor())) {
        HILOGE("NearlinkASCProxy::GetVirtualAudioDeviceList WriteInterfaceToken error");
        return NL_ERR_INTERNAL_ERROR;
    }

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_SYNC
    };
    ErrCode error = InnerTransact(
        NearlinkASCInterfaceCode::NL_ASC_GET_VIRTUAL_AUDIO_DEVICE_LIST, option, data, reply);
    if (error != NO_ERROR) {
        HILOGE("NearlinkASCProxy::GetVirtualAudioDeviceList done fail, error: %{public}d", error);
        return NL_ERR_IPC_TRANS_FAILED;
    }

    int result = reply.ReadInt32();
    if (result != NO_ERROR) {
        HILOGE("NearlinkASCProxy::GetVirtualAudioDeviceList done fail, result: %{public}d", result);
        return NL_ERR_INTERNAL_ERROR;
    }

    uint32_t vecCnt = reply.ReadUint32();
    if ((vecCnt <= 0) || (vecCnt > MAX_AUDIO_DEVICE_COUNT)) {
        HILOGE("vector size is error");
        return NL_ERR_IPC_TRANS_FAILED;
    }
    for (uint32_t i = 0; i < vecCnt; i++) {
        std::shared_ptr<NearlinkRawAddress> device(reply.ReadParcelable<NearlinkRawAddress>());
        if (!device) {
            return NL_ERR_IPC_TRANS_FAILED;
        }
        devices.push_back(*device);
    }

    return static_cast<NlErrCode>(result);
}

NlErrCode NearlinkASCProxy::GetSupportStreamType(const NearlinkRawAddress &device, uint32_t& supportStreamType)
{
    HILOGI("NearlinkASCProxy::GetSupportStreamType start");
    MessageParcel data;
    if (!data.WriteInterfaceToken(NearlinkASCProxy::GetDescriptor())) {
        HILOGE("NearlinkASCProxy::GetSupportStreamType WriteInterfaceToken error");
        return NL_ERR_INTERNAL_ERROR;
    }

    NL_CHECK_RETURN_RET(data.WriteParcelable(&device), NL_ERR_INTERNAL_ERROR, "Write device error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_SYNC
    };
    ErrCode error = InnerTransact(
        NearlinkASCInterfaceCode::NL_ASC_GET_SUPPORT_STREAM_TYPE, option, data, reply);
    if (error != NO_ERROR) {
        HILOGE("NearlinkASCProxy::GetSupportStreamType done fail, error: %{public}d", error);
        return NL_ERR_IPC_TRANS_FAILED;
    }

    int result = reply.ReadInt32();
    if (result != NO_ERROR) {
        HILOGE("NearlinkASCProxy::GetSupportStreamType done fail, result: %{public}d", result);
        return NL_ERR_INTERNAL_ERROR;
    }

    supportStreamType = reply.ReadUint32();
    HILOGD("supportStreamType %{public}d", supportStreamType);

    return static_cast<NlErrCode>(result);
}

NlErrCode NearlinkASCProxy::GetAudioDeviceCodecInfo(const NearlinkRawAddress &device,
    std::map<AudioStreamType, AudioStreamCodecInfo> &info)
{
    HILOGI("NearlinkASCProxy::GetAudioDeviceCodecInfo start");
    MessageParcel data;
    if (!data.WriteInterfaceToken(NearlinkASCProxy::GetDescriptor())) {
        HILOGE("NearlinkASCProxy::GetAudioDeviceCodecInfo WriteInterfaceToken error");
        return NL_ERR_INTERNAL_ERROR;
    }

    NL_CHECK_RETURN_RET(data.WriteParcelable(&device), NL_ERR_INTERNAL_ERROR, "Write device error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_SYNC
    };
    ErrCode error = InnerTransact(
        NearlinkASCInterfaceCode::NL_ASC_GET_AUDIO_DEVICE_CODEC_INFO, option, data, reply);
    if (error != NO_ERROR) {
        HILOGE("NearlinkASCProxy::GetAudioDeviceCodecInfo done fail, error: %{public}d", error);
        return NL_ERR_IPC_TRANS_FAILED;
    }

    int result = reply.ReadInt32();
    if (result != NO_ERROR) {
        HILOGE("NearlinkASCProxy::GetAudioDeviceCodecInfo done fail, result: %{public}d", result);
        return NL_ERR_INTERNAL_ERROR;
    }

    uint32_t mapCnt = reply.ReadUint32();
    const uint32_t maxCodecType = 3;
    if ((mapCnt <= 0) || (mapCnt > maxCodecType)) {
        HILOGE("map size is error");
        return NL_ERR_IPC_TRANS_FAILED;
    }
    for (uint32_t i = 0; i < mapCnt; i++) {
        AudioStreamType StreamType = static_cast<AudioStreamType>(reply.ReadUint32());

        AudioStreamCodecInfo codec;
        codec.codecType = reply.ReadUint64();
        codec.sampleRate = reply.ReadUint16();
        codec.bitsPerSample = reply.ReadUint8();
        codec.channelMode = reply.ReadUint8();
        codec.frameDuration = reply.ReadUint16();
        codec.bitRate = reply.ReadUint64();
        codec.frameNumPerSdu = reply.ReadUint8();
        info.insert(std::pair<AudioStreamType, AudioStreamCodecInfo>(StreamType, codec));
    }

    return static_cast<NlErrCode>(result);
}

NlErrCode NearlinkASCProxy::SetActiveSinkDevice(const NearlinkRawAddress &device, uint64_t supportStreamType)
{
    HILOGD("NearlinkASCProxy::SetActiveSinkDevice start");
    MessageParcel data;
    if (!data.WriteInterfaceToken(NearlinkASCProxy::GetDescriptor())) {
        HILOGE("NearlinkASCProxy::SetActiveSinkDevice WriteInterfaceToken error");
        return NL_ERR_INTERNAL_ERROR;
    }

    NL_CHECK_RETURN_RET(data.WriteParcelable(&device), NL_ERR_INTERNAL_ERROR, "Write device error");
    NL_CHECK_RETURN_RET(data.WriteUint64(supportStreamType), NL_ERR_INTERNAL_ERROR, "Write supportStreamType error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_SYNC
    };
    ErrCode error = InnerTransact(
        NearlinkASCInterfaceCode::NL_ASC_SET_ACTIVE_SINK_DEVICE, option, data, reply);
    if (error != NO_ERROR) {
        HILOGE("NearlinkASCProxy::SetActiveSinkDevice done fail, error: %{public}d", error);
        return NL_ERR_IPC_TRANS_FAILED;
    }
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkASCProxy::GetDualRecordAbility(const NearlinkRawAddress &device, bool &isSupport)
{
    MessageParcel data;
    if (!data.WriteInterfaceToken(NearlinkASCProxy::GetDescriptor())) {
        HILOGE("Write Token error");
        return NL_ERR_IPC_TRANS_FAILED;
    }
    NL_CHECK_RETURN_RET(data.WriteParcelable(&device), NL_ERR_INTERNAL_ERROR, "Write device error");
    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_SYNC
    };
    ErrCode result = InnerTransact(NearlinkASCInterfaceCode::NL_ASC_GET_DUAL_RECORD_CAP, option, data, reply);
    NL_CHECK_RETURN_RET(result == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", result);
    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        isSupport = reply.ReadBool();
    }
    return exception;
}

NlErrCode NearlinkASCProxy::GetKaraokeAbility(const NearlinkRawAddress &device, bool &isSupport)
{
    MessageParcel data;
    if (!data.WriteInterfaceToken(NearlinkASCProxy::GetDescriptor())) {
        HILOGE("Write Token error");
        return NL_ERR_IPC_TRANS_FAILED;
    }
    NL_CHECK_RETURN_RET(data.WriteParcelable(&device), NL_ERR_INTERNAL_ERROR, "Write device error");
    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_SYNC
    };
    ErrCode result = InnerTransact(NearlinkASCInterfaceCode::NL_ASC_GET_KARAOKE_ABILITY, option, data, reply);
    NL_CHECK_RETURN_RET(result == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", result);
    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        isSupport = reply.ReadBool();
    }
    return exception;
}

ErrCode NearlinkASCProxy::InnerTransact(
    uint32_t code, MessageOption &flags, MessageParcel &data, MessageParcel &reply)
{
    auto remote = Remote();
    NL_CHECK_RETURN_RET(remote, OBJECT_NULL, "[InnerTransact] fail: get Remote fail code %{public}d", code);
    ErrCode err = remote->SendRequest(code, data, reply, flags);
    if (err != NO_ERROR) {
        HILOGW("[InnerTransact] fail: ipcErr=%{public}d code %{public}d", err, code);
    }
    return err;
}
}  // namespace Nearlink
}  // namespace OHOS
