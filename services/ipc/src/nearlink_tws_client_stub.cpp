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

#include "nearlink_tws_client_stub.h"
#include "log.h"
#include "nearlink_errorcode.h"
#include "ipc_types.h"
#include "nearlink_def.h"
#include "nearlink_permission_manager.h"
#include "nearlink_ASC_source.h"
#include "nearlink_asc_audio_stream_info.h"
#include "nearlink_utils.h"
#ifdef HICOLLIE_ENABLE
#include "nearlink_hicollie_adapter.h"
#endif

#ifdef STUB_FUNC
#undef STUB_FUNC
#endif
#define STUB_FUNC(code, func, perm) NearlinkTwsClientInterfaceCode::code, {NearlinkTwsClientStub::func, perm}

namespace OHOS {
namespace Nearlink {
namespace {
constexpr int WEAR_DETECTION_STATE_INVALID = -1;
constexpr int TWS_ROLE_INFO_TYPE_INVALID = -1;
}

NearlinkTwsClientStub::NearlinkTwsClientStub()
{
    HILOGI("enter");
    // Note: Permissions need to be configured when the itf to be used. "nullptr" means no permission needed.
    memberFuncMap_ = {
        {STUB_FUNC(NL_TWS_REGISTER_APP, RegisterApplicationInner, CHECK_PERM(true, {MANAGE_NEARLINK}))},
        {STUB_FUNC(NL_TWS_DEREGISTER_APP, DeregisterApplicationInner, CHECK_PERM(true, {MANAGE_NEARLINK}))},
        {STUB_FUNC(NL_ENABLE_WEAR_DETECTION, EnableWearDetectionInner, CHECK_PERM(true, {MANAGE_NEARLINK}))},
        {STUB_FUNC(NL_DISABLE_WEAR_DETECTION, DisableWearDetectionInner, CHECK_PERM(true, {MANAGE_NEARLINK}))},
        {STUB_FUNC(NL_GET_WEAR_DETECTION_STATE, GetWearDetectionStateInner, CHECK_PERM(true, {MANAGE_NEARLINK}))},
        {STUB_FUNC(NL_IS_DEVICE_WEARING, IsDeviceWearingInner, CHECK_PERM(true, {MANAGE_NEARLINK}))},
        {STUB_FUNC(NL_IS_WEAR_DETECTION_SUPPORTED, IsWearDetectionSupportedInner,
                   CHECK_PERM(true, {MANAGE_NEARLINK}))},
        {STUB_FUNC(NL_GET_TWS_ROLE_INFO, GetTwsRoleInfoInner, CHECK_PERM(true, {MANAGE_NEARLINK}))},
        {STUB_FUNC(NL_GET_TWS_AUDIO_DELAY, GetTwsAudioDelayInner, CHECK_PERM(true, {MANAGE_NEARLINK}))},
        {STUB_FUNC(NL_SEND_USER_SELECTION, SendUserSelectionInner, CHECK_PERM(true, {MANAGE_NEARLINK}))},
        { STUB_FUNC(NL_IS_SUPPORT_VIRTUAL_AUTO_CONNECT, IsSupportVirtualAutoConnectInner,
            CHECK_PERM(true, {MANAGE_NEARLINK})) },
        { STUB_FUNC(NL_SET_VIRTUAL_AUTO_CONNECT_TYPE, SetVirtualAutoConnectTypeInner,
            CHECK_PERM(true, {MANAGE_NEARLINK})) },
        {STUB_FUNC(NL_QUERY_STREAM_STATE, QueryStreamStateInner, CHECK_PERM(true, {MANAGE_NEARLINK}))},
    };
}

NearlinkTwsClientStub::~NearlinkTwsClientStub()
{}

int32_t NearlinkTwsClientStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
#ifdef HICOLLIE_ENABLE
    NearlinkHicollieAdapter hicollieAdapter("NearlinkTwsClient", code);
#endif
    CHECK_PERMISSION_AND_EXECUTE(NearlinkTwsClientStub);
}

int32_t NearlinkTwsClientStub::RegisterApplicationInner(NearlinkTwsClientStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    HILOGD("enter");
    sptr<IRemoteObject> remote = data.ReadRemoteObject();
    const sptr<INearlinkTwsClientObserver> callback = OHOS::iface_cast<INearlinkTwsClientObserver>(remote);
    NL_CHECK_RETURN_RET(callback, TRANSACTION_ERR, "callback is nullptr");
    NlErrCode result = stub->RegisterApplication(callback);
    bool resultRet = reply.WriteInt32(result);
    if (!resultRet) {
        HILOGE("NearlinkTwsClientStub: reply writing failed.");
        return ERR_INVALID_VALUE;
    }
    HILOGD("NearlinkTwsClientStub::RegisterApplicationInner end");
    return NO_ERROR;
}

int32_t NearlinkTwsClientStub::DeregisterApplicationInner(NearlinkTwsClientStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    HILOGD("enter");
    sptr<IRemoteObject> remote = data.ReadRemoteObject();
    const sptr<INearlinkTwsClientObserver> callback = OHOS::iface_cast<INearlinkTwsClientObserver>(remote);
    NL_CHECK_RETURN_RET(callback, TRANSACTION_ERR, "callback is nullptr");
    NlErrCode result = stub->DeregisterApplication(callback);
    bool ret = reply.WriteInt32(result);
    if (!ret) {
        HILOGE("NearlinkTwsClientStub: reply writing failed.");
        return ERR_INVALID_VALUE;
    }
    return NO_ERROR;
}

int32_t NearlinkTwsClientStub::EnableWearDetectionInner(NearlinkTwsClientStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    HILOGD("enter");
    std::string address;
    NL_CHECK_RETURN_RET(data.ReadString(address), TRANSACTION_ERR, "ReadString failed.");
    NL_CHECK_RETURN_RET(IsValidAddress(address), INVALID_DATA, "Invalid Address");

    ErrCode status = stub->EnableWearDetection(address);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    return NO_ERROR;
}

int32_t NearlinkTwsClientStub::DisableWearDetectionInner(NearlinkTwsClientStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    HILOGD("enter");
    std::string address;
    NL_CHECK_RETURN_RET(data.ReadString(address), TRANSACTION_ERR, "ReadString failed.");
    NL_CHECK_RETURN_RET(IsValidAddress(address), INVALID_DATA, "Invalid Address");

    ErrCode status = stub->DisableWearDetection(address);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    return NO_ERROR;
}

int32_t NearlinkTwsClientStub::GetWearDetectionStateInner(NearlinkTwsClientStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    HILOGD("enter");
    std::string address;
    NL_CHECK_RETURN_RET(data.ReadString(address), TRANSACTION_ERR, "ReadString failed.");
    NL_CHECK_RETURN_RET(IsValidAddress(address), INVALID_DATA, "Invalid Address");

    int32_t state = WEAR_DETECTION_STATE_INVALID;
    ErrCode status = stub->GetWearDetectionState(address, state);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    NL_CHECK_RETURN_RET(reply.WriteInt32(state), TRANSACTION_ERR, "WriteInt32 failed.");
    return NO_ERROR;
}

int32_t NearlinkTwsClientStub::IsDeviceWearingInner(NearlinkTwsClientStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    HILOGD("enter");
    std::string address;
    NL_CHECK_RETURN_RET(data.ReadString(address), TRANSACTION_ERR, "ReadString failed.");
    NL_CHECK_RETURN_RET(IsValidAddress(address), INVALID_DATA, "Invalid Address");

    bool isWearing = false;
    ErrCode status = stub->IsDeviceWearing(address, isWearing);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    NL_CHECK_RETURN_RET(reply.WriteBool(isWearing), TRANSACTION_ERR, "WriteBool failed.");
    return NO_ERROR;
}

int32_t NearlinkTwsClientStub::IsWearDetectionSupportedInner(NearlinkTwsClientStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    HILOGD("enter");
    std::string address;
    NL_CHECK_RETURN_RET(data.ReadString(address), TRANSACTION_ERR, "ReadString failed.");
    NL_CHECK_RETURN_RET(IsValidAddress(address), INVALID_DATA, "Invalid Address");

    bool isSupported = false;
    ErrCode status = stub->IsWearDetectionSupported(address, isSupported);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    NL_CHECK_RETURN_RET(reply.WriteBool(isSupported), TRANSACTION_ERR, "WriteBool failed.");
    return NO_ERROR;
}

int32_t NearlinkTwsClientStub::GetTwsRoleInfoInner(NearlinkTwsClientStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    HILOGD("enter");
    std::string address;
    NL_CHECK_RETURN_RET(data.ReadString(address), TRANSACTION_ERR, "ReadString failed.");
    NL_CHECK_RETURN_RET(IsValidAddress(address), INVALID_DATA, "Invalid Address");

    int32_t roleInfo = TWS_ROLE_INFO_TYPE_INVALID;
    ErrCode status = stub->GetTwsRoleInfo(address, roleInfo);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    NL_CHECK_RETURN_RET(reply.WriteInt32(roleInfo), TRANSACTION_ERR, "WriteInt32 failed.");
    return NO_ERROR;
}

int32_t NearlinkTwsClientStub::GetTwsAudioDelayInner(NearlinkTwsClientStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    HILOGD("enter");
    std::string address;
    NL_CHECK_RETURN_RET(data.ReadString(address), TRANSACTION_ERR, "ReadString failed.");
    NL_CHECK_RETURN_RET(IsValidAddress(address), INVALID_DATA, "Invalid Address");

    uint32_t delayValue = 0;
    ErrCode status = stub->GetTwsAudioDelay(address, delayValue);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    if (status == NL_NO_ERROR) {
        NL_CHECK_RETURN_RET(reply.WriteUint32(delayValue), TRANSACTION_ERR, "WriteUint32 failed.");
    }
    return NO_ERROR;
}

int32_t NearlinkTwsClientStub::SendUserSelectionInner(NearlinkTwsClientStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    HILOGD("enter");
    std::string address;
    NL_CHECK_RETURN_RET(data.ReadString(address), TRANSACTION_ERR, "ReadString failed.");
    NL_CHECK_RETURN_RET(IsValidAddress(address), INVALID_DATA, "Invalid Address");
    std::shared_ptr<NearlinkASCAudioStreamInfo> streamInfo(data.ReadParcelable<NearlinkASCAudioStreamInfo>());
    if (!streamInfo) {
        return TRANSACTION_ERR;
    }

    std::vector<struct AudioStreamInfo> streamData {};
    streamInfo->GetStreamState(streamData);
    ErrCode status = stub->SendUserSelection(address, streamData);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    return NO_ERROR;
}

int32_t NearlinkTwsClientStub::QueryStreamStateInner(NearlinkTwsClientStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    HILOGD("enter");
    std::string address;
    NL_CHECK_RETURN_RET(data.ReadString(address), TRANSACTION_ERR, "ReadString failed.");
    NL_CHECK_RETURN_RET(IsValidAddress(address), INVALID_DATA, "Invalid Address");

    std::vector<struct AudioStreamInfo> streamData;
    ErrCode status = stub->QueryStreamState(address, streamData);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");

    if (status == NO_ERROR) {
        NearlinkASCAudioStreamInfo streamInfo {};
        streamInfo.SetStreamState(streamData);
        NL_CHECK_RETURN_RET(reply.WriteParcelable(&streamInfo), TRANSACTION_ERR, "Write streamInfo failed.");
    }

    return NO_ERROR;
}

int32_t NearlinkTwsClientStub::IsSupportVirtualAutoConnectInner(NearlinkTwsClientStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    HILOGD("enter");
    std::string address;
    NL_CHECK_RETURN_RET(data.ReadString(address), TRANSACTION_ERR, "ReadString failed.");
    NL_CHECK_RETURN_RET(IsValidAddress(address), INVALID_DATA, "Invalid Address");

    bool isSupported = false;
    ErrCode status = stub->IsSupportVirtualAutoConnect(address, isSupported);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    if (status == NO_ERROR) {
        NL_CHECK_RETURN_RET(reply.WriteBool(isSupported), TRANSACTION_ERR, "WriteBool failed.");
    }
    return NO_ERROR;
}

int32_t NearlinkTwsClientStub::SetVirtualAutoConnectTypeInner(NearlinkTwsClientStub *stub,
    MessageParcel &data, MessageParcel &reply)
{
    HILOGD("enter");
    std::string address;
    int32_t connType = 0;
    int32_t businessType = 0;
    NL_CHECK_RETURN_RET(data.ReadString(address), TRANSACTION_ERR, "Read address failed");
    NL_CHECK_RETURN_RET(data.ReadInt32(connType), TRANSACTION_ERR, "Read connType failed");
    NL_CHECK_RETURN_RET(data.ReadInt32(businessType), TRANSACTION_ERR, "Read businessType failed");
    NL_CHECK_RETURN_RET(IsValidAddress(address), INVALID_DATA, "Invalid Address");

    ErrCode status = stub->SetVirtualAutoConnectType(address, connType, businessType);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");

    return NO_ERROR;
}

}  // namespace Nearlink
}  // namespace OHOS