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

#ifndef OHOS_NEARLINK_STANDARD_SLE_DATATRANSFER_INTERFACE_H
#define OHOS_NEARLINK_STANDARD_SLE_DATATRANSFER_INTERFACE_H

#include "nearlink_service_ipc_interface_code.h"
#include "i_nearlink_sle_datatransfer_callback.h"
#include "nearlink_sle_datatransfer_connection_params.h"
#include "iremote_broker.h"
#include "nearlink_errorcode.h"

namespace OHOS::Nearlink {
namespace {
const std::string SLE_DATATRANSFER_SERVER = "SleDataTransferServer";
}  // namespace

class INearlinkSleDataTransfer : public OHOS::IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.ipc.INearlinkSleDataTransfer");

    virtual NlErrCode RegisterSleDataTransferCallback(const sptr<INearlinkSleDataTransferCallback> &callback) = 0;
    virtual NlErrCode DeregisterSleDataTransferCallback(const sptr<INearlinkSleDataTransferCallback> &callback) = 0;
    virtual NlErrCode CreatePort(const std::string &uuid, uint16_t &port) = 0;
    virtual NlErrCode SocketEmptyMsg(uint16_t port, std::string address) = 0;
    virtual NlErrCode DestroyPort(const std::string &uuid, uint16_t port) = 0;
    virtual NlErrCode Connect(NearlinkSleDataTransferConnectionParams &params) = 0;
    virtual NlErrCode Disconnect(NearlinkSleDataTransferConnectionParams &params) = 0;
    virtual NlErrCode GetConnectionState(NearlinkSleDataTransferConnectionParams &params, int32_t &connState) = 0;
};
}  // namespace OHOS::Nearlink
#endif  // OHOS_NEARLINK_STANDARD_SLE_DATATRANSFER_INTERFACE_H