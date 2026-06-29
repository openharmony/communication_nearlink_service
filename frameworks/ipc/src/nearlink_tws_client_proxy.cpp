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

#include "nearlink_tws_client_proxy.h"
#include "log.h"
#include "i_nearlink_tws_client.h"
#include "nearlink_errorcode.h"
#include "nearlink_def.h"
#include "nearlink_asc_audio_stream_info.h"

namespace OHOS {
namespace Nearlink {
NlErrCode NearlinkTwsClientProxy::RegisterApplication(const sptr<INearlinkTwsClientObserver> &observer)
{
    HILOGI("start");
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkTwsClientProxy::GetDescriptor()),
        NL_ERR_INTERNAL_ERROR, "Write Token error");
    NL_CHECK_RETURN_RET(data.WriteRemoteObject(observer->AsObject()), NL_ERR_INTERNAL_ERROR,
        "Write RemoteObject error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_SYNC
    };
    ErrCode error = InnerTransact(
        NearlinkTwsClientInterfaceCode::NL_TWS_REGISTER_APP, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkTwsClientProxy::DeregisterApplication(const sptr<INearlinkTwsClientObserver> &observer)
{
    HILOGI("start");
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkTwsClientProxy::GetDescriptor()),
        NL_ERR_INTERNAL_ERROR, "Write Token error");
    NL_CHECK_RETURN_RET(data.WriteRemoteObject(observer->AsObject()), NL_ERR_INTERNAL_ERROR,
        "Write RemoteObject error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_SYNC
    };
    ErrCode error = InnerTransact(
        NearlinkTwsClientInterfaceCode::NL_TWS_DEREGISTER_APP, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkTwsClientProxy::EnableWearDetection(const std::string &address)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkTwsClientProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteString(address), NL_ERR_IPC_TRANS_FAILED, "Write address error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};

    ErrCode error = InnerTransact(NearlinkTwsClientInterfaceCode::NL_ENABLE_WEAR_DETECTION, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkTwsClientProxy::DisableWearDetection(const std::string &address)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkTwsClientProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteString(address), NL_ERR_IPC_TRANS_FAILED, "Write address error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};

    ErrCode error = InnerTransact(NearlinkTwsClientInterfaceCode::NL_DISABLE_WEAR_DETECTION, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkTwsClientProxy::GetWearDetectionState(const std::string &address, int32_t &state)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkTwsClientProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteString(address), NL_ERR_IPC_TRANS_FAILED, "Write address error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};

    ErrCode error = InnerTransact(NearlinkTwsClientInterfaceCode::NL_GET_WEAR_DETECTION_STATE, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);
    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        state = reply.ReadInt32();
    }
    return exception;
}

NlErrCode NearlinkTwsClientProxy::IsDeviceWearing(const std::string &address, bool &isWearing)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkTwsClientProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteString(address), NL_ERR_IPC_TRANS_FAILED, "Write address error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};

    ErrCode error = InnerTransact(NearlinkTwsClientInterfaceCode::NL_IS_DEVICE_WEARING, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);
    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        isWearing = reply.ReadBool();
    }
    return exception;
}

NlErrCode NearlinkTwsClientProxy::IsWearDetectionSupported(const std::string &address, bool &isSupported)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkTwsClientProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteString(address), NL_ERR_IPC_TRANS_FAILED, "Write address error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};

    ErrCode error = InnerTransact(NearlinkTwsClientInterfaceCode::NL_IS_WEAR_DETECTION_SUPPORTED,
                                  option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);
    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        isSupported = reply.ReadBool();
    }
    return exception;
}

NlErrCode NearlinkTwsClientProxy::GetTwsRoleInfo(const std::string &address, int32_t &roleInfo)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkTwsClientProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteString(address), NL_ERR_IPC_TRANS_FAILED, "Write address error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};

    ErrCode error = InnerTransact(NearlinkTwsClientInterfaceCode::NL_GET_TWS_ROLE_INFO,
                                  option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);
    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        roleInfo = reply.ReadInt32();
    }
    return exception;
}

NlErrCode NearlinkTwsClientProxy::GetTwsAudioDelay(const std::string &address, uint32_t &delayValue)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkTwsClientProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteString(address), NL_ERR_IPC_TRANS_FAILED, "Write address error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};

    ErrCode error = InnerTransact(NearlinkTwsClientInterfaceCode::NL_GET_TWS_AUDIO_DELAY, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);
    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        delayValue = reply.ReadUint32();
    }
    return exception;
}

ErrCode NearlinkTwsClientProxy::InnerTransact(
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

NlErrCode NearlinkTwsClientProxy::SendUserSelection(const std::string &address,
    const std::vector<struct AudioStreamInfo> &streamData)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkTwsClientProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteString(address), NL_ERR_IPC_TRANS_FAILED, "Write address error.");
    NearlinkASCAudioStreamInfo streamInfo {};
    streamInfo.SetStreamState(streamData);
    NL_CHECK_RETURN_RET(data.WriteParcelable(&streamInfo), NL_ERR_IPC_TRANS_FAILED, "Write stream info error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};

    ErrCode error = InnerTransact(NearlinkTwsClientInterfaceCode::NL_SEND_USER_SELECTION, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkTwsClientProxy::QueryStreamState(const std::string &address,
    std::vector<struct AudioStreamInfo> &streamData)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkTwsClientProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteString(address), NL_ERR_IPC_TRANS_FAILED, "Write address error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};

    ErrCode error = InnerTransact(NearlinkTwsClientInterfaceCode::NL_QUERY_STREAM_STATE, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);
    NlErrCode ret = static_cast<NlErrCode>(reply.ReadInt32());

    if (ret == NL_NO_ERROR) {
        NearlinkASCAudioStreamInfo streamInfo {};
        const std::shared_ptr<NearlinkASCAudioStreamInfo>
            stream(reply.ReadParcelable<NearlinkASCAudioStreamInfo>());
        NL_CHECK_RETURN_RET(stream, NL_ERR_INTERNAL_ERROR, "read stream info fail");
        stream->GetStreamState(streamData);
    }

    return ret;
}

NlErrCode NearlinkTwsClientProxy::IsSupportVirtualAutoConnect(const std::string &address, bool &isSupported)
{
    isSupported = false;
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkTwsClientProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteString(address), NL_ERR_IPC_TRANS_FAILED, "Write address error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};

    ErrCode error = InnerTransact(NearlinkTwsClientInterfaceCode::NL_IS_SUPPORT_VIRTUAL_AUTO_CONNECT,
        option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);
    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        isSupported = reply.ReadBool();
    }
    return exception;
}

NlErrCode NearlinkTwsClientProxy::SetVirtualAutoConnectType(const std::string &address,
    int32_t connType, int32_t businessType)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkTwsClientProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteString(address), NL_ERR_IPC_TRANS_FAILED, "Write address error.");
    NL_CHECK_RETURN_RET(data.WriteInt32(connType), NL_ERR_IPC_TRANS_FAILED, "Write connect type.");
    NL_CHECK_RETURN_RET(data.WriteInt32(businessType), NL_ERR_IPC_TRANS_FAILED, "Write business type.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};

    ErrCode error = InnerTransact(NearlinkTwsClientInterfaceCode::NL_SET_VIRTUAL_AUTO_CONNECT_TYPE,
        option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

}
}