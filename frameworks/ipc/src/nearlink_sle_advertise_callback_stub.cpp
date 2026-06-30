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

#include "nearlink_sle_advertise_callback_stub.h"
#include "log.h"
#include "ipc_types.h"
#include "string_ex.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;
namespace OHOS {
namespace Nearlink {
const std::map<uint32_t, std::function<ErrCode(NearlinkSleAdvertiseCallbackStub *, MessageParcel &, MessageParcel &)>>
    NearlinkSleAdvertiseCallbackStub::memberFuncMap_ = {
        {NearlinkSleAdvertiseCallbackInterfaceCode::NL_SLE_ADVERTISE_CALLBACK_AUTO_STOP_EVENT,
            std::bind(&NearlinkSleAdvertiseCallbackStub::OnAutoStopAdvEventInner, _1, _2, _3)},
        {NearlinkSleAdvertiseCallbackInterfaceCode::NL_SLE_ADVERTISE_CALLBACK_RESULT_EVENT,
            std::bind(&NearlinkSleAdvertiseCallbackStub::OnStartResultEventInner, _1, _2, _3)},
        {NearlinkSleAdvertiseCallbackInterfaceCode::NL_SLE_ADVERTISE_CALLBACK_STOP_RESULT_EVENT,
            std::bind(&NearlinkSleAdvertiseCallbackStub::OnStopResultEventInner, _1, _2, _3)},
        {NearlinkSleAdvertiseCallbackInterfaceCode::NL_SLE_ADVERTISE_CALLBACK_SET_ADV_DATA,
            std::bind(&NearlinkSleAdvertiseCallbackStub::OnSetAdvDataEventInner, _1, _2, _3)},
        {NearlinkSleAdvertiseCallbackInterfaceCode::NL_SLE_ADVERTISE_CALLBACK_ENABLE_RESULT_EVENT,
            std::bind(&NearlinkSleAdvertiseCallbackStub::OnEnableResultEventInner, _1, _2, _3)},
        {NearlinkSleAdvertiseCallbackInterfaceCode::NL_SLE_ADVERTISE_CALLBACK_DISABLE_RESULT_EVENT,
            std::bind(&NearlinkSleAdvertiseCallbackStub::OnDisableResultEventInner, _1, _2, _3)},
};

NearlinkSleAdvertiseCallbackStub::NearlinkSleAdvertiseCallbackStub()
{
    HILOGD("start.");
}

NearlinkSleAdvertiseCallbackStub::~NearlinkSleAdvertiseCallbackStub()
{
    HILOGD("start.");
}

int NearlinkSleAdvertiseCallbackStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    HILOGD("NearlinkSleAdvertiseCallbackStub::OnRemoteRequest, cmd = %{public}d, flags= %{public}d",
        code, option.GetFlags());
    if (NearlinkSleAdvertiseCallbackStub::GetDescriptor() != data.ReadInterfaceToken()) {
        HILOGI("local descriptor is not equal to remote");
        return ERR_INVALID_STATE;
    }

    auto itFunc = memberFuncMap_.find(code);
    if (itFunc != memberFuncMap_.end()) {
        auto memberFunc = itFunc->second;
        if (memberFunc != nullptr) {
            return memberFunc(this, data, reply);
        }
    }

    HILOGW("NearlinkSleAdvertiseCallbackStub::OnRemoteRequest, default case, need check.");
    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}

ErrCode NearlinkSleAdvertiseCallbackStub::OnStartResultEventInner(MessageParcel &data, MessageParcel &reply)
{
    HILOGD("NearlinkSleAdvertiseCallbackStub::OnStartResultEventInner");
    const int32_t result = data.ReadInt32();
    const int32_t advHandle = data.ReadInt32();
    const int32_t opcode = data.ReadInt32();

    OnStartResultEvent(result, advHandle, opcode);
    return NO_ERROR;
}

ErrCode NearlinkSleAdvertiseCallbackStub::OnStopResultEventInner(MessageParcel &data, MessageParcel &reply)
{
    const int32_t result = data.ReadInt32();
    const int32_t advHandle = data.ReadInt32();
    OnStopResultEvent(result, advHandle);
    return NO_ERROR;
}

ErrCode NearlinkSleAdvertiseCallbackStub::OnEnableResultEventInner(MessageParcel &data, MessageParcel &reply)
{
    const int32_t result = data.ReadInt32();
    const int32_t advHandle = data.ReadInt32();
    OnEnableResultEvent(result, advHandle);
    return NO_ERROR;
}

ErrCode NearlinkSleAdvertiseCallbackStub::OnDisableResultEventInner(MessageParcel &data, MessageParcel &reply)
{
    const int32_t result = data.ReadInt32();
    const int32_t advHandle = data.ReadInt32();
    OnDisableResultEvent(result, advHandle);
    return NO_ERROR;
}

ErrCode NearlinkSleAdvertiseCallbackStub::OnAutoStopAdvEventInner(MessageParcel &data, MessageParcel &reply)
{
    const int32_t advHandle = data.ReadInt32();
    OnAutoStopAdvEvent(advHandle);
    return NO_ERROR;
}

ErrCode NearlinkSleAdvertiseCallbackStub::OnSetAdvDataEventInner(MessageParcel &data, MessageParcel &reply)
{
    const int32_t result = data.ReadInt32();
    const int32_t advHandle = data.ReadInt32();
    OnSetAdvDataEvent(result, advHandle);
    return NO_ERROR;
}
}  // namespace Nearlink
}  // namespace OHOS
