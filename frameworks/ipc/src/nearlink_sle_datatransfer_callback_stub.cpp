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

#include "nearlink_sle_datatransfer_callback_stub.h"
#include "log.h"
#include "nearlink_def.h"
#include "ipc_types.h"
#include "string_ex.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace OHOS::Nearlink {
const std::map<uint32_t,
    std::function<ErrCode(NearlinkSleDataTransferCallbackStub *, MessageParcel &, MessageParcel &)>>
    NearlinkSleDataTransferCallbackStub::memberFuncMap_ = {
        {NearlinkSleDataTransferCallbackInterfaceCode::NL_SLE_DATATRANSFER_CALLBACK_CONNECION_STATE_CHANGE_EVENT,
            std::bind(&NearlinkSleDataTransferCallbackStub::OnConnectionStateChangedInner, _1, _2, _3)},
};

NearlinkSleDataTransferCallbackStub::NearlinkSleDataTransferCallbackStub()
{
    HILOGD("start.");
}

NearlinkSleDataTransferCallbackStub::~NearlinkSleDataTransferCallbackStub()
{
    HILOGD("destroy.");
}

int NearlinkSleDataTransferCallbackStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    HILOGD("stub cmd = %{public}d, flags= %{public}d", code, option.GetFlags());
    NL_CHECK_RETURN_RET(NearlinkSleDataTransferCallbackStub::GetDescriptor() == data.ReadInterfaceToken(),
        ERR_INVALID_STATE, "local descriptor is not equal to remote");

    auto itFunc = memberFuncMap_.find(code);
    if (itFunc != memberFuncMap_.end()) {
        auto memberFunc = itFunc->second;
        if (memberFunc != nullptr) {
            return memberFunc(this, data, reply);
        }
    }

    HILOGW("default case, need check.");
    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}

ErrCode NearlinkSleDataTransferCallbackStub::OnConnectionStateChangedInner(MessageParcel &data, MessageParcel &reply)
{
    HILOGD("enter");
    std::shared_ptr<NearlinkSleDataTransferConnectionParams> connectionParams(
        data.ReadParcelable<NearlinkSleDataTransferConnectionParams>());
    NL_CHECK_RETURN_RET(connectionParams, TRANSACTION_ERR, "connectionParams empty");
    int fd = -1;
    if (connectionParams->state_ == static_cast<int32_t>(SleConnectState::CONNECTED)) {
        fd = data.ReadFileDescriptor();
    }
    OnConnectionStateChanged(*connectionParams, fd);
    return NO_ERROR;
}
}  // namespace OHOS::Nearlink
