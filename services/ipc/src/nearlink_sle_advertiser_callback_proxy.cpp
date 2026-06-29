/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "nearlink_sle_advertiser_callback_proxy.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
NearlinkSleAdvertiserCallbackProxy::NearlinkSleAdvertiserCallbackProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<INearlinkSleAdvertiseCallback>(impl)
{}

NearlinkSleAdvertiserCallbackProxy::~NearlinkSleAdvertiserCallbackProxy()
{}

void NearlinkSleAdvertiserCallbackProxy::OnStartResultEvent(int32_t result, int32_t advHandle, int32_t opcode)
{
    HILOGD("Enter");
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkSleAdvertiserCallbackProxy::GetDescriptor()),
        "Write Token error");
    NL_CHECK_RETURN(data.WriteInt32(result), "write result failed");
    NL_CHECK_RETURN(data.WriteInt32(advHandle), "write advHandle failed");
    NL_CHECK_RETURN(data.WriteInt32(opcode), "write opcode failed");
    
    MessageParcel reply;
    MessageOption option = {MessageOption::TF_ASYNC};
    ErrCode error = InnerTransact(NL_SLE_ADVERTISE_CALLBACK_RESULT_EVENT, option, data, reply);
    NL_CHECK_RETURN(error == NO_ERROR, "done fail, error: %{public}d", error);
}

void NearlinkSleAdvertiserCallbackProxy::OnStopResultEvent(int32_t result, int32_t advHandle)
{
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkSleAdvertiserCallbackProxy::GetDescriptor()),
        "write interface token failed.");
    NL_CHECK_RETURN(data.WriteInt32(result), "write result failed");
    NL_CHECK_RETURN(data.WriteInt32(advHandle), "write advHandle failed");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_ASYNC};
    ErrCode error = InnerTransact(NL_SLE_ADVERTISE_CALLBACK_STOP_RESULT_EVENT, option, data, reply);
    NL_CHECK_RETURN(error == NO_ERROR, "done fail, error: %{public}d", error);
}

void NearlinkSleAdvertiserCallbackProxy::OnEnableResultEvent(int32_t result, int32_t advHandle)
{
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkSleAdvertiserCallbackProxy::GetDescriptor()),
        "write interface token failed.");
    NL_CHECK_RETURN(data.WriteInt32(result), "write result failed");
    NL_CHECK_RETURN(data.WriteInt32(advHandle), "write advHandle failed");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_ASYNC};
    ErrCode error = InnerTransact(NL_SLE_ADVERTISE_CALLBACK_ENABLE_RESULT_EVENT, option, data, reply);
    NL_CHECK_RETURN(error == NO_ERROR, "done fail, error: %{public}d", error);
}

void NearlinkSleAdvertiserCallbackProxy::OnDisableResultEvent(int32_t result, int32_t advHandle)
{
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkSleAdvertiserCallbackProxy::GetDescriptor()),
        "write interface token failed.");
    NL_CHECK_RETURN(data.WriteInt32(result), "write result failed");
    NL_CHECK_RETURN(data.WriteInt32(advHandle), "write advHandle failed");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_ASYNC};
    ErrCode error = InnerTransact(NL_SLE_ADVERTISE_CALLBACK_DISABLE_RESULT_EVENT, option, data, reply);
    NL_CHECK_RETURN(error == NO_ERROR, "done fail, error: %{public}d", error);
}

void NearlinkSleAdvertiserCallbackProxy::OnAutoStopAdvEvent(int32_t advHandle)
{
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkSleAdvertiserCallbackProxy::GetDescriptor()),
        "write interfacetoken failed.");
    NL_CHECK_RETURN(data.WriteInt32(advHandle), "write advHandle failed");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_ASYNC};
    ErrCode error = InnerTransact(NL_SLE_ADVERTISE_CALLBACK_AUTO_STOP_EVENT, option, data, reply);
    NL_CHECK_RETURN(error == NO_ERROR, "done fail, error: %{public}d", error);
}

void NearlinkSleAdvertiserCallbackProxy::OnSetAdvDataEvent(int32_t result, int32_t advHandle)
{
    HILOGD("Enter");
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkSleAdvertiserCallbackProxy::GetDescriptor()),
        "write interfacetoken failed.");
    NL_CHECK_RETURN(data.WriteInt32(result), "write result failed");
    NL_CHECK_RETURN(data.WriteInt32(advHandle), "write advHandle failed");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_ASYNC};
    ErrCode error = InnerTransact(NL_SLE_ADVERTISE_CALLBACK_SET_ADV_DATA, option, data, reply);
    NL_CHECK_RETURN(error == NO_ERROR, "done fail, error: %{public}d", error);
}

ErrCode NearlinkSleAdvertiserCallbackProxy::InnerTransact(
    uint32_t code, MessageOption &flags, MessageParcel &data, MessageParcel &reply)
{
    auto remote = Remote();
    NL_CHECK_RETURN_RET(remote, OBJECT_NULL, "[InnerTransact] fail: get Remote fail code %{public}d", code);
    ErrCode err = remote->SendRequest(code, data, reply, flags);
    switch (err) {
        case NO_ERROR: {
            return NO_ERROR;
        }
        case DEAD_OBJECT: {
            HILOGW("[InnerTransact] fail: ipcErr=%{public}d code %{public}d", err, code);
            return DEAD_OBJECT;
        }
        default: {
            HILOGW("[InnerTransact] fail: ipcErr=%{public}d code %{public}d", err, code);
            return TRANSACTION_ERR;
        }
    }
}
}  // namespace Nearlink
}  // namespace OHOS
