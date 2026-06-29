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

#include "nearlink_vcp_client_stub.h"
#include "log.h"
#include "nearlink_permission_manager.h"
#include "ipc_types.h"
#include "string_ex.h"
#ifdef HICOLLIE_ENABLE
#include "nearlink_hicollie_adapter.h"
#endif

#ifdef STUB_FUNC
#undef STUB_FUNC
#endif
#define STUB_FUNC(code, func, perm) NearlinkVcpInterfaceCode::code, {NearlinkVcpClientStub::func, perm}

namespace OHOS {
namespace Nearlink {
namespace {
constexpr uint8_t VCP_DEFAULT_VOLUME_LEVEL = 7;
}

NearlinkVcpClientStub::NearlinkVcpClientStub()
{
    HILOGI("enter");
    // Note: Permissions need to be configured when the itf to be used. "nullptr" means no permission needed.
    memberFuncMap_ = {
        {STUB_FUNC(NL_VCP_CLIENT_SET_DEVICE_ABSOLUTE_VOLUME, SetDeviceAbsoluteVolumeInner,
           CHECK_PERM(true, {MANAGE_NEARLINK}))},
        {STUB_FUNC(NL_VCP_CLIENT_GET_MEDIA_VOLUME, GetDeviceMediaVolumeInner,
           CHECK_PERM(true, {MANAGE_NEARLINK}))},
        {STUB_FUNC(NL_VCP_CLIENT_GET_CALL_VOLUME, GetDeviceCallVolumeInner,
           CHECK_PERM(true, {MANAGE_NEARLINK}))},
    };
}

NearlinkVcpClientStub::~NearlinkVcpClientStub()
{
    memberFuncMap_.clear();
}

int32_t NearlinkVcpClientStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
#ifdef HICOLLIE_ENABLE
    NearlinkHicollieAdapter hicollieAdapter("NearlinkVcpClient", code);
#endif
    CHECK_PERMISSION_AND_EXECUTE(NearlinkVcpClientStub);
}

int32_t NearlinkVcpClientStub::SetDeviceAbsoluteVolumeInner(NearlinkVcpClientStub *stub, MessageParcel &data,
                                                            MessageParcel &reply)
{
    std::shared_ptr<NearlinkRawAddress> addr(data.ReadParcelable<NearlinkRawAddress>());
    if (!addr) {
        return TRANSACTION_ERR;
    }
    int32_t volumeLevel = data.ReadInt32();
    uint8_t streamType = data.ReadUint8();

    int result = stub->SetDeviceAbsoluteVolume(*addr, volumeLevel, streamType);
    if (!reply.WriteInt32(result)) {
        HILOGE("reply writing failed.");
        return ERR_INVALID_VALUE;
    }
    return NO_ERROR;
}

int32_t NearlinkVcpClientStub::GetDeviceMediaVolumeInner(NearlinkVcpClientStub *stub, MessageParcel &data,
                                                         MessageParcel &reply)
{
    std::shared_ptr<NearlinkRawAddress> addr(data.ReadParcelable<NearlinkRawAddress>());
    if (!addr) {
        return TRANSACTION_ERR;
    }
    int32_t mediaVolume = VCP_DEFAULT_VOLUME_LEVEL;
    NlErrCode status = stub->GetDeviceMediaVolume(*addr, mediaVolume);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    NL_CHECK_RETURN_RET(status == NL_NO_ERROR, TRANSACTION_ERR, "stop write.");
    NL_CHECK_RETURN_RET(reply.WriteInt32(mediaVolume), TRANSACTION_ERR, "WriteInt32 failed.");
    return NO_ERROR;
}

int32_t NearlinkVcpClientStub::GetDeviceCallVolumeInner(NearlinkVcpClientStub *stub, MessageParcel &data,
                                                        MessageParcel &reply)
{
    std::shared_ptr<NearlinkRawAddress> addr(data.ReadParcelable<NearlinkRawAddress>());
    if (!addr) {
        return TRANSACTION_ERR;
    }
    int32_t callVolume = VCP_DEFAULT_VOLUME_LEVEL;
    NlErrCode status = stub->GetDeviceCallVolume(*addr, callVolume);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    NL_CHECK_RETURN_RET(status == NL_NO_ERROR, TRANSACTION_ERR, "stop write.");
    NL_CHECK_RETURN_RET(reply.WriteInt32(callVolume), TRANSACTION_ERR, "WriteInt32 failed.");
    return NO_ERROR;
}
}
}