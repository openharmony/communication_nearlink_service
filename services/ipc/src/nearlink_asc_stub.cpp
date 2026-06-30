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

#include "nearlink_permission_manager.h"
#include "nearlink_asc_stub.h"
#include "log.h"
#include "ipc_types.h"
#include "string_ex.h"
#ifdef HICOLLIE_ENABLE
#include "nearlink_hicollie_adapter.h"
#endif
#ifdef STUB_FUNC
#undef STUB_FUNC
#endif
#define STUB_FUNC(code, func, perm) NearlinkASCInterfaceCode::code, {NearlinkASCStub::func, perm}

namespace OHOS {
namespace Nearlink {

NearlinkASCStub::NearlinkASCStub()
{
    HILOGI("enter");
    // Note: Permissions need to be configured when the itf to be used. "nullptr" means no permission needed.
    memberFuncMap_ = {
        {STUB_FUNC(NL_ASC_REGISTER_APP, RegisterApplicationInner, CHECK_PERM(false, {MANAGE_NEARLINK}))},
        {STUB_FUNC(NL_ASC_DEREGISTER_APP, DeregisterApplicationInner, CHECK_PERM(false, {MANAGE_NEARLINK}))},
        {STUB_FUNC(NL_ASC_CONTROL, AudioControlInner, CHECK_PERM(false, {MANAGE_NEARLINK}))},
        {STUB_FUNC(NL_ASC_GET_AUDIO_DEVICE_LIST, GetAudioDeviceListInner, CHECK_PERM(false, {MANAGE_NEARLINK}))},
        {STUB_FUNC(NL_ASC_GET_VIRTUAL_AUDIO_DEVICE_LIST,
            GetVirtualAudioDeviceListInner, CHECK_PERM(false, {MANAGE_NEARLINK}))},
        {STUB_FUNC(NL_ASC_GET_SUPPORT_STREAM_TYPE, GetSupportStreamTypeInner, CHECK_PERM(false, {MANAGE_NEARLINK}))},
        {STUB_FUNC(NL_ASC_GET_AUDIO_DEVICE_CODEC_INFO, GetAudioDeviceCodecInfoInner,
            CHECK_PERM(false, {MANAGE_NEARLINK}))},
        {STUB_FUNC(NL_ASC_SET_ACTIVE_SINK_DEVICE, SetActiveSinkDeviceInner, CHECK_PERM(false, {MANAGE_NEARLINK}))},
        {STUB_FUNC(NL_ASC_GET_DUAL_RECORD_CAP, GetDualRecordAbilityInner, CHECK_PERM(false, {MANAGE_NEARLINK}))},
        {STUB_FUNC(NL_ASC_GET_KARAOKE_ABILITY, GetKaraokeAbilityInner, CHECK_PERM(false, {MANAGE_NEARLINK}))},
    };
}

NearlinkASCStub::~NearlinkASCStub()
{}


int32_t NearlinkASCStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
#ifdef HICOLLIE_ENABLE
    NearlinkHicollieAdapter hicollieAdapter("NearlinkASC", code);
#endif
    CHECK_PERMISSION_AND_EXECUTE(NearlinkASCStub);
}

int32_t NearlinkASCStub::RegisterApplicationInner(NearlinkASCStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    HILOGD("NearlinkASCStub::RegisterApplicationInner starts");
    sptr<IRemoteObject> remote = data.ReadRemoteObject();
    const sptr<INearlinkASCCallback> callback = OHOS::iface_cast<INearlinkASCCallback>(remote);
    NL_CHECK_RETURN_RET(callback, TRANSACTION_ERR, "callback is nullptr");
    NlErrCode result = stub->RegisterApplication(callback);
    bool resultRet = reply.WriteInt32(result);
    if (!resultRet) {
        HILOGE("NearlinkASCStub: reply writing failed.");
        return ERR_INVALID_VALUE;
    }
    HILOGD("NearlinkASCStub::RegisterApplicationInner end");
    return NO_ERROR;
}

int32_t NearlinkASCStub::DeregisterApplicationInner(NearlinkASCStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    HILOGI("NearlinkASCStub::DeregisterApplicationInner starts");
    sptr<IRemoteObject> remote = data.ReadRemoteObject();
    const sptr<INearlinkASCCallback> callback = OHOS::iface_cast<INearlinkASCCallback>(remote);
    NL_CHECK_RETURN_RET(callback, TRANSACTION_ERR, "callback is nullptr");
    NlErrCode result = stub->DeregisterApplication(callback);
    bool ret = reply.WriteInt32(result);
    if (!ret) {
        HILOGE("NearlinkASCStub: reply writing failed.");
        return ERR_INVALID_VALUE;
    }
    return NO_ERROR;
}

int32_t NearlinkASCStub::AudioControlInner(NearlinkASCStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    HILOGI("NearlinkASCStub::AudioControlInner starts");
    std::shared_ptr<NearlinkRawAddress> device(data.ReadParcelable<NearlinkRawAddress>());
    if (!device) {
        return TRANSACTION_ERR;
    }
    AudioStreamType streamType = static_cast<AudioStreamType>(data.ReadUint32());
    int32_t cmd = data.ReadInt32();
    NlErrCode result = stub->AudioControl(*device, streamType, cmd);
    bool ret = reply.WriteInt32(result);
    if (!ret) {
        HILOGE("NearlinkASCStub: reply writing failed.");
        return ERR_INVALID_VALUE;
    }
    return NO_ERROR;
}

int32_t NearlinkASCStub::GetAudioDeviceListInner(NearlinkASCStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    HILOGI("NearlinkASCStub::GetAudioDeviceListInner starts");

    std::vector<NearlinkRawAddress> devices;

    NlErrCode result = stub->GetAudioDeviceList(devices);
    bool ret = reply.WriteInt32(result);
    if (!ret) {
        HILOGE("NearlinkASCStub: reply writing failed.");
        return ERR_INVALID_VALUE;
    }

    uint32_t vecCnt = devices.size();
    if (vecCnt > MAX_AUDIO_DEVICE_COUNT) {
        HILOGI("vector size exceed limit.");
        vecCnt = MAX_AUDIO_DEVICE_COUNT;
    }
    NL_CHECK_RETURN_RET(reply.WriteUint32(vecCnt), NL_ERR_INTERNAL_ERROR, "Write vecCnt error");
    for (const NearlinkRawAddress& dev : devices) {
        NL_CHECK_RETURN_RET(reply.WriteParcelable(&dev), NL_ERR_INTERNAL_ERROR, "Write device error");
    }

    return NO_ERROR;
}

int32_t NearlinkASCStub::GetVirtualAudioDeviceListInner(NearlinkASCStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    HILOGI("NearlinkASCStub::GetVirtualAudioDeviceListInner starts");

    std::vector<NearlinkRawAddress> devices;

    NlErrCode result = stub->GetVirtualAudioDeviceList(devices);
    bool ret = reply.WriteInt32(result);
    if (!ret) {
        HILOGE("NearlinkASCStub: reply writing failed.");
        return ERR_INVALID_VALUE;
    }

    uint32_t vecCnt = devices.size();
    if (vecCnt > MAX_VIRTUAL_AUDIO_DEVICE_COUNT) {
        HILOGI("vector size exceed limit.");
        vecCnt = MAX_VIRTUAL_AUDIO_DEVICE_COUNT;
    }
    NL_CHECK_RETURN_RET(reply.WriteUint32(vecCnt), NL_ERR_INTERNAL_ERROR, "Write vecCnt error");
    for (const NearlinkRawAddress& dev : devices) {
        NL_CHECK_RETURN_RET(reply.WriteParcelable(&dev), NL_ERR_INTERNAL_ERROR, "Write device error");
    }

    return NO_ERROR;
}

int32_t NearlinkASCStub::GetSupportStreamTypeInner(NearlinkASCStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    HILOGI("NearlinkASCStub::GetSupportStreamTypeInner starts");
    std::shared_ptr<NearlinkRawAddress> device(data.ReadParcelable<NearlinkRawAddress>());
    if (!device) {
        return TRANSACTION_ERR;
    }
    uint32_t supportStreamType = AUDIO_STREAM_NONE;

    NlErrCode result = stub->GetSupportStreamType(*device, supportStreamType);
    bool ret = reply.WriteInt32(result);
    if (!ret) {
        HILOGE("NearlinkASCStub: reply writing failed.");
        return ERR_INVALID_VALUE;
    }

    NL_CHECK_RETURN_RET(reply.WriteUint32(supportStreamType), NL_ERR_INTERNAL_ERROR, "Write supportStreamType error");
    return NO_ERROR;
}

int32_t NearlinkASCStub::GetAudioDeviceCodecInfoInner(NearlinkASCStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    HILOGI("NearlinkASCStub::GetAudioDeviceCodecInfoInner starts");

    std::shared_ptr<NearlinkRawAddress> device(data.ReadParcelable<NearlinkRawAddress>());
    if (!device) {
        return TRANSACTION_ERR;
    }
    std::map<AudioStreamType, AudioStreamCodecInfo> info;
    NlErrCode result = stub->GetAudioDeviceCodecInfo(*device, info);
    bool ret = reply.WriteInt32(result);
    if (!ret) {
        HILOGE("NearlinkASCStub: reply writing failed.");
        return ERR_INVALID_VALUE;
    }

    NL_CHECK_RETURN_RET(reply.WriteUint32(info.size()), NL_ERR_INTERNAL_ERROR, "Write mapCnt error");
    for (auto &codec : info) {
        NL_CHECK_RETURN_RET(reply.WriteUint32(codec.first), NL_ERR_INTERNAL_ERROR, "Write streamType error");
        NL_CHECK_RETURN_RET(reply.WriteUint64(codec.second.codecType), NL_ERR_INTERNAL_ERROR,
            "Write codecType error");
        NL_CHECK_RETURN_RET(reply.WriteUint16(codec.second.sampleRate), NL_ERR_INTERNAL_ERROR,
            "Write sampleRate error");
        NL_CHECK_RETURN_RET(reply.WriteUint8(codec.second.bitsPerSample), NL_ERR_INTERNAL_ERROR,
            "Write bitsPerSample error");
        NL_CHECK_RETURN_RET(reply.WriteUint8(codec.second.channelMode), NL_ERR_INTERNAL_ERROR,
            "Write channelMode error");
        NL_CHECK_RETURN_RET(reply.WriteUint16(codec.second.frameDuration), NL_ERR_INTERNAL_ERROR,
            "Write frameDuration error");
        NL_CHECK_RETURN_RET(reply.WriteUint64(codec.second.bitRate), NL_ERR_INTERNAL_ERROR,
            "Write bitRate error");
        NL_CHECK_RETURN_RET(reply.WriteUint8(codec.second.frameNumPerSdu), NL_ERR_INTERNAL_ERROR,
            "Write frameNumPerSdu error");
    }

    return NO_ERROR;
}

int32_t NearlinkASCStub::SetActiveSinkDeviceInner(NearlinkASCStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    HILOGI("NearlinkASCStub::SetActiveSinkDeviceInner starts");
    std::shared_ptr<NearlinkRawAddress> device(data.ReadParcelable<NearlinkRawAddress>());
    if (!device) {
        return TRANSACTION_ERR;
    }
    uint64_t supportStreamType = data.ReadUint64();
    NlErrCode result = stub->SetActiveSinkDevice(*device, supportStreamType);
    bool ret = reply.WriteInt32(result);
    if (!ret) {
        HILOGE("NearlinkASCStub: reply writing failed.");
        return ERR_INVALID_VALUE;
    }
    return NO_ERROR;
}

int32_t NearlinkASCStub::GetDualRecordAbilityInner(NearlinkASCStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    HILOGI("NearlinkASCStub::GetDualRecordAbilityInner starts");
    std::shared_ptr<NearlinkRawAddress> device(data.ReadParcelable<NearlinkRawAddress>());
    if (!device) {
        return TRANSACTION_ERR;
    }
    bool isSupport = false;
    NlErrCode result = stub->GetDualRecordAbility(*device, isSupport);
    NL_CHECK_RETURN_RET(reply.WriteInt32(result), TRANSACTION_ERR, "WriteInt32 failed.");
    NL_CHECK_RETURN_RET(reply.WriteBool(isSupport), TRANSACTION_ERR, "WriteBool failed.");
    return result;
}

int32_t NearlinkASCStub::GetKaraokeAbilityInner(NearlinkASCStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    HILOGI("NearlinkASCStub::GetKaraokeAbilityInner starts");
    std::shared_ptr<NearlinkRawAddress> device(data.ReadParcelable<NearlinkRawAddress>());
    if (!device) {
        return TRANSACTION_ERR;
    }
    bool isSupport = false;
    NlErrCode result = stub->GetKaraokeAbility(*device, isSupport);
    NL_CHECK_RETURN_RET(reply.WriteInt32(result), TRANSACTION_ERR, "WriteInt32 failed.");
    NL_CHECK_RETURN_RET(reply.WriteBool(isSupport), TRANSACTION_ERR, "WriteBool failed.");
    return result;
}

}  // namespace Nearlink
}  // namespace OHOS