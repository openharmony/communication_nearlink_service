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

#ifndef I_NEARLINK_CDSM_CLIENT_H
#define I_NEARLINK_CDSM_CLIENT_H

#include "iremote_broker.h"
#include "nearlink_errorcode.h"
#include "nearlink_raw_address.h"
#include "nearlink_service_ipc_interface_code.h"
#include "nearlink_cdsm_info_parcel.h"
#include "i_nearlink_cdsm_client_callback.h"

namespace OHOS {
namespace Nearlink {
namespace {
const std::string PROFILE_CDSM_CLIENT_SERVER = "NearlinkCdsmClientServer";
}  // namespace
class INearlinkCdsmClient : public OHOS::IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.ipc.INearlinkCdsmClient");

    virtual NlErrCode RegisterCdsmClientCallback(const NearlinkRawAddress &addr,
                                                 const sptr<INearlinkCdsmClientCallback> &callback) = 0;
    virtual NlErrCode DeregisterCdsmClientCallback(const NearlinkRawAddress &addr) = 0;
    virtual NlErrCode GetCdsmInfo(const NearlinkRawAddress &addr, NearlinkCdsInfoParcel &cdsInfo) = 0;
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // I_NEARLINK_CDSM_CLIENT_H