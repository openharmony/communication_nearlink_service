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

namespace {
/* callId 为蜂窝侧 32 位通话标识（业务层按 int32_t 键映射成本地短编号）：-1 表示无特定通话；
   本层仅拒绝小于 -1 的值，-1 及非负值全部放行 */
constexpr int32_t STUB_INVALID_CALL_ID = -1;
constexpr int32_t STUB_CALL_TYPE_MIN = 0;
constexpr int32_t STUB_CALL_TYPE_MAX = 0xff;
constexpr int32_t STUB_CALL_STATE_MIN = -1;
constexpr int32_t STUB_CALL_STATE_MAX = 0xff;
constexpr int32_t STUB_CALL_NUM_MIN = 0;
constexpr int32_t STUB_CALL_NUM_MAX = 0xff;
}

NearlinkCcpClientStub::NearlinkCcpClientStub()
{
    HILOGI("enter");
    // Note: Permissions need to be configured when the itf to be used. "nullptr" means no permission needed.
    memberFuncMap_ = {
        {STUB_FUNC(NL_CCP_CLIENT_PHONE_STATE_CHANGED, PhoneStateChangedInner, CHECK_PERM(true, {MANAGE_NEARLINK}))},
        {STUB_FUNC(NL_CCP_CLIENT_CALL_DETAILS_CHANGED, CallDetailsChangedInner, CHECK_PERM(true, {MANAGE_NEARLINK}))},
    };
}

NearlinkCcpClientStub::~NearlinkCcpClientStub()
{}

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
    /* 报文完整性预检：4 个 int32 + 号码字符串长度头 */
    constexpr size_t intPartSize = sizeof(int32_t) * 4 + sizeof(int32_t);
    NL_CHECK_RETURN_RET(data.GetReadableBytes() >= intPartSize, ERR_INVALID_VALUE,
        "truncated payload: readable=%{public}d", static_cast<int32_t>(data.GetReadableBytes()));

    int32_t callId = data.ReadInt32();
    NL_CHECK_RETURN_RET(callId >= STUB_INVALID_CALL_ID, ERR_INVALID_VALUE,
        "invalid callId=%{public}d", callId);

    int32_t numActive = data.ReadInt32();
    NL_CHECK_RETURN_RET(numActive >= STUB_CALL_NUM_MIN && numActive <= STUB_CALL_NUM_MAX,
        ERR_INVALID_VALUE, "invalid numActive=%{public}d", numActive);

    int32_t numHeld = data.ReadInt32();
    NL_CHECK_RETURN_RET(numHeld >= STUB_CALL_NUM_MIN && numHeld <= STUB_CALL_NUM_MAX,
        ERR_INVALID_VALUE, "invalid numHeld=%{public}d", numHeld);

    int32_t callState = data.ReadInt32();
    /* CallState 合法域：-1 ~ 0xff，未知状态由业务层白名单兜底 */
    NL_CHECK_RETURN_RET(callState >= STUB_CALL_STATE_MIN && callState <= STUB_CALL_STATE_MAX,
        ERR_INVALID_VALUE, "invalid callState=%{public}d", callState);

    std::string number;
    NL_CHECK_RETURN_RET(data.ReadString(number), ERR_INVALID_VALUE, "read number failed.");
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
    /* 报文完整性预检：3 个 int32 */
    constexpr size_t intPartSize = sizeof(int32_t) * 3;
    NL_CHECK_RETURN_RET(data.GetReadableBytes() >= intPartSize, ERR_INVALID_VALUE,
        "truncated payload: readable=%{public}d", static_cast<int32_t>(data.GetReadableBytes()));

    int32_t callId = data.ReadInt32();
    NL_CHECK_RETURN_RET(callId >= STUB_INVALID_CALL_ID, ERR_INVALID_VALUE,
        "invalid callId=%{public}d", callId);

    int32_t callState = data.ReadInt32();
    /* CallState 合法域：-1 ~ 0xff，未知状态由业务层白名单兜底 */
    NL_CHECK_RETURN_RET(callState >= STUB_CALL_STATE_MIN && callState <= STUB_CALL_STATE_MAX,
        ERR_INVALID_VALUE, "invalid callState=%{public}d", callState);

    int32_t callType = data.ReadInt32();
    /* callType 合法域：0 ~ 0xff，未知类型按非 VoIP 蜂窝路径处理 */
    NL_CHECK_RETURN_RET(callType >= STUB_CALL_TYPE_MIN && callType <= STUB_CALL_TYPE_MAX,
        ERR_INVALID_VALUE, "invalid callType=%{public}d", callType);

    int32_t result = stub->CallDetailsChanged(callId, callState, callType);
    if (!reply.WriteInt32(result)) {
        HILOGE("reply writing failed.");
        return ERR_INVALID_VALUE;
    }
    return NO_ERROR;
}
}  // namespace Nearlink
}  // namespace OHOS
