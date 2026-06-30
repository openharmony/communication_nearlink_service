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

#ifndef OHOS_NEARLINK_STANDARD_SLE_DATATRANSFER_PROXY_H
#define OHOS_NEARLINK_STANDARD_SLE_DATATRANSFER_PROXY_H

#include "i_nearlink_sle_datatransfer.h"
#include "iremote_proxy.h"

namespace OHOS::Nearlink {
class NearlinkSleDataTransferProxy : public IRemoteProxy<INearlinkSleDataTransfer> {
public:
    NearlinkSleDataTransferProxy() = delete;
    explicit NearlinkSleDataTransferProxy(const sptr<IRemoteObject> &impl);
    ~NearlinkSleDataTransferProxy() override;
    DISALLOW_COPY_AND_MOVE(NearlinkSleDataTransferProxy);

    NlErrCode RegisterSleDataTransferCallback(const sptr<INearlinkSleDataTransferCallback> &callback) override;
    NlErrCode DeregisterSleDataTransferCallback(const sptr<INearlinkSleDataTransferCallback> &callback) override;
    NlErrCode CreatePort(const std::string &uuid, uint16_t &port) override;
    NlErrCode DestroyPort(const std::string &uuid, uint16_t port) override;
    NlErrCode SocketEmptyMsg(uint16_t port, std::string address) override;
    NlErrCode Connect(NearlinkSleDataTransferConnectionParams &params) override;
    NlErrCode Disconnect(NearlinkSleDataTransferConnectionParams &params) override;
    NlErrCode GetConnectionState(NearlinkSleDataTransferConnectionParams &params, int32_t &connState) override;
#ifdef WATCH_STANDARD
    NlErrCode UpdateConnectInterval(std::string device, int32_t intervalType, bool &result) override;
#endif

private:
    ErrCode InnerTransact(uint32_t code, MessageOption &flags, MessageParcel &data, MessageParcel &reply);
    static inline BrokerDelegator<NearlinkSleDataTransferProxy> delegator_;
};
}  // namespace OHOS::Nearlink

#endif  // OHOS_NEARLINK_STANDARD_SLE_DATATRANSFER_PROXY_H