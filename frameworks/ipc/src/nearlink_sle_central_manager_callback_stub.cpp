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

#include "nearlink_sle_central_manager_callback_stub.h"
#include "nearlink_uuid_parcel.h"
#include "log.h"
#include "ipc_types.h"
#include "string_ex.h"

namespace OHOS {
namespace Nearlink {
const uint32_t SLE_CENTRAL_MANAGER_READ_DATA_SIZE_MAX_LEN = 0x100;
const std::map<uint32_t,
    std::function<ErrCode(NearlinkSleCentralManagerCallBackStub *, MessageParcel &, MessageParcel &)>>
    NearlinkSleCentralManagerCallBackStub::memberFuncMap_ = {
        {NearlinkSleCentralManagerCallbackInterfaceCode::NL_SLE_CENTRAL_MANAGER_CALLBACK,
            std::bind(&NearlinkSleCentralManagerCallBackStub::OnScanCallbackInner, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3)},
        {NearlinkSleCentralManagerCallbackInterfaceCode::NL_SLE_CENTRAL_MANAGER_BLE_BATCH_CALLBACK,
            std::bind(&NearlinkSleCentralManagerCallBackStub::OnSleBatchScanResultsEventInner, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3)},
        {NearlinkSleCentralManagerCallbackInterfaceCode::NL_SLE_CENTRAL_MANAGER_CALLBACK_SCAN_FAILED,
            std::bind(&NearlinkSleCentralManagerCallBackStub::OnStartOrStopScanEventInner, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3)},
};

NearlinkSleCentralManagerCallBackStub::NearlinkSleCentralManagerCallBackStub()
{
    HILOGD("start.");
}

NearlinkSleCentralManagerCallBackStub::~NearlinkSleCentralManagerCallBackStub()
{
    HILOGD("start.");
}

int NearlinkSleCentralManagerCallBackStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    HILOGD("NearlinkSleCentralManagerCallBackStub::OnRemoteRequest, cmd = %{public}d, flags= %{public}d",
        code, option.GetFlags());
    if (NearlinkSleCentralManagerCallBackStub::GetDescriptor() != data.ReadInterfaceToken()) {
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
    HILOGW("NearlinkSleCentralManagerCallBackStub::OnRemoteRequest, default case, need check.");
    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}

ErrCode NearlinkSleCentralManagerCallBackStub::OnScanCallbackInner(MessageParcel &data, MessageParcel &reply)
{
    std::shared_ptr<NearlinkSleScanResult> result(data.ReadParcelable<NearlinkSleScanResult>());
    if (!result) {
        return TRANSACTION_ERR;
    }
    OnScanCallback(*result);
    return NO_ERROR;
}

ErrCode NearlinkSleCentralManagerCallBackStub::OnSleBatchScanResultsEventInner(
    MessageParcel &data, MessageParcel &reply)
{
    HILOGI("not implentment");
    return NO_ERROR;
}

ErrCode NearlinkSleCentralManagerCallBackStub::OnStartOrStopScanEventInner(MessageParcel &data, MessageParcel &reply)
{
    int32_t resultCode = data.ReadInt32();
    bool isStartScan = data.ReadBool();
    OnStartOrStopScanEvent(resultCode, isStartScan);
    return NO_ERROR;
}

}  // namespace Nearlink
}  // namespace OHOS
