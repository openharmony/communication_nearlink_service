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

#include "nearlink_hid_host_proxy.h"
#include "log.h"
#include "i_nearlink_hid_host.h"
#include "nearlink_host.h"
#include "nearlink_errorcode.h"
#include "nearlink_def.h"

namespace OHOS {
namespace Nearlink {

NlErrCode NearlinkHidHostProxy::HidHostSetReport(std::string device, uint8_t type,
    std::string &report, int& result)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHidHostProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteString(device), NL_ERR_IPC_TRANS_FAILED, "Write device error.");
    NL_CHECK_RETURN_RET(data.WriteUint8(type), NL_ERR_IPC_TRANS_FAILED, "Write type error.");
    NL_CHECK_RETURN_RET(data.WriteString(report), NL_ERR_IPC_TRANS_FAILED, "Write report error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkHidHostInterfaceCode::NL_SET_REPORT, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_INTERNAL_ERROR, "done fail, ErrCode: %{public}d", error);
    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    result = reply.ReadInt32();
    return exception;
}

ErrCode NearlinkHidHostProxy::InnerTransact(
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
}
}