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

#include "nearlink_ccp_client_stub.h"
#include "log.h"
#include "nearlink_errorcode.h"
#include "nearlink_permission_manager.h"
#include "ipc_types.h"
#include "string_ex.h"
#ifdef HICOLLIE_ENABLE
#include "nearlink_hicollie_adapter.h"
#endif

#ifdef STUB_FUNC
#undef STUB_FUNC
#endif
#define STUB_FUNC(code, func, perm) NearlinkCcpInterfaceCode::code, {NearlinkCcpClientStub::func, perm}

namespace OHOS {
namespace Nearlink {

NearlinkCcpClientStub::NearlinkCcpClientStub()
{
    HILOGI("enter");
    // Note: Permissions need to be configured when the itf to be used. "nullptr" means no permission needed.
    memberFuncMap_ = {
        {STUB_FUNC(NL_CCP_CLIENT_PHONE_STATE_CHANGED, PhoneStateChangedInner,
           CHECK_PERM(true, {MANAGE_NEARLINK}))},
        {STUB_FUNC(NL_CCP_CLIENT_CALL_DETAILS_CHANGED, CallDetailsChangedInner,
           CHECK_PERM(true, {MANAGE_NEARLINK}))},
    };
}

NearlinkCcpClientStub::~NearlinkCcpClientStub()
{
    memberFuncMap_.clear();
}

int32_t NearlinkCcpClientStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
#ifdef HICOLLIE_ENABLE
    NearlinkHicollieAdapter hicollieAdapter("NearlinkCcpClientStub", code);
#endif
    CHECK_PERMISSION_AND_EXECUTE(NearlinkCcpClientStub);
}

int32_t NearlinkCcpClientStub::PhoneStateChangedInner(NearlinkCcpClientStub *stub, MessageParcel &data,
                                                      MessageParcel &reply)
{
    int32_t callId = data.ReadInt32();
    int32_t numActive = data.ReadInt32();
    int32_t numHeld = data.ReadInt32();
    int32_t callState = data.ReadInt32();
    std::string number = data.ReadString();

    int32_t result = stub->PhoneStateChanged(callId, numActive, numHeld, callState, number);
    if (!reply.WriteInt32(result)) {
        HILOGE("reply writing failed.");
        return ERR_INVALID_VALUE;
    }
    return NO_ERROR;
}

int32_t NearlinkCcpClientStub::CallDetailsChangedInner(NearlinkCcpClientStub *stub, MessageParcel &data,
                                                       MessageParcel &reply)
{
    int32_t callId = data.ReadInt32();
    int32_t callState = data.ReadInt32();
    int32_t callType = data.ReadInt32();

    int32_t result = stub->CallDetailsChanged(callId, callState, callType);
    if (!reply.WriteInt32(result)) {
        HILOGE("reply writing failed.");
        return ERR_INVALID_VALUE;
    }
    return NO_ERROR;
}
}  // namespace Nearlink
}  // namespace OHOS
