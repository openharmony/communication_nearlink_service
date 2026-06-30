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

#ifndef NEARLINK_STANDARD_NEARLINK_SLE_DATATRANSFER_CALLBACK_STUB_H
#define NEARLINK_STANDARD_NEARLINK_SLE_DATATRANSFER_CALLBACK_STUB_H

#include "i_nearlink_sle_datatransfer_callback.h"
#include "i_nearlink_host.h"
#include "iremote_stub.h"
#include "map"

namespace OHOS::Nearlink {
class NearlinkSleDataTransferCallbackStub : public IRemoteStub<INearlinkSleDataTransferCallback> {
public:
    NearlinkSleDataTransferCallbackStub();
    ~NearlinkSleDataTransferCallbackStub() override;

    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;

private:
    ErrCode OnConnectionStateChangedInner(MessageParcel &data, MessageParcel &reply);
    static const std::map<uint32_t,
        std::function<ErrCode(NearlinkSleDataTransferCallbackStub *, MessageParcel &, MessageParcel &)>>
        memberFuncMap_;
    DISALLOW_COPY_AND_MOVE(NearlinkSleDataTransferCallbackStub);
};
}  // namespace OHOS::Nearlink
#endif  // NEARLINK_STANDARD_NEARLINK_SLE_DATATRANSFER_CALLBACK_STUB_H
