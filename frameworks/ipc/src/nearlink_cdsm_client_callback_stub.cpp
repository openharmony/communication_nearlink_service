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
#include "nearlink_cdsm_client_callback_stub.h"

#include <memory>

#include "log.h"
#include "nearlink_service_ipc_interface_code.h"

namespace OHOS {
namespace Nearlink {

NearlinkCdsmClientCallbackStub::NearlinkCdsmClientCallbackStub()
{
    HILOGD("[Cdsm]Enter");
    memberFuncMap_[
        static_cast<uint32_t>(NearlinkCdsmClientCallbackInterfaceCode::NL_CDSM_CLIENT_CALLBACK_RESULT_EVENT)
    ] = &NearlinkCdsmClientCallbackStub::OnCdsInfoChangedInner;
}

NearlinkCdsmClientCallbackStub::~NearlinkCdsmClientCallbackStub()
{
    HILOGD("[Cdsm]Enter");
    memberFuncMap_.clear();
}

int NearlinkCdsmClientCallbackStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    HILOGI("[Cdsm]cmd = %{public}d, flags= %{public}d", code, option.GetFlags());
    if (NearlinkCdsmClientCallbackStub::GetDescriptor() != data.ReadInterfaceToken()) {
        HILOGI("[Cdsm]local descriptor is not equal to remote");
        return ERR_INVALID_STATE;
    }

    auto itFunc = memberFuncMap_.find(code);
    if (itFunc != memberFuncMap_.end()) {
        auto memberFunc = itFunc->second;
        if (memberFunc != nullptr) {
            return (this->*memberFunc)(data, reply);
        }
    }

    HILOGI("[Cdsm]default case, need check.");
    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}

ErrCode NearlinkCdsmClientCallbackStub::OnCdsInfoChangedInner(MessageParcel &data, MessageParcel &reply)
{
    HILOGI("[Cdsm]enter");
    std::shared_ptr<NearlinkCdsInfoParcel> result(data.ReadParcelable<NearlinkCdsInfoParcel>());
    if (!result) {
        HILOGE("[Cdsm]TRANSACTION_ERR");
        return TRANSACTION_ERR;
    }
    OnCdsInfoChanged(*result);
    return NO_ERROR;
}
}  // namespace Nearlink
}  // namespace OHOS