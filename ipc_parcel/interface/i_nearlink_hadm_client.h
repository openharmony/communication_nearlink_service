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

#ifndef I_NEARLINK_HADM_CLIENT_H
#define I_NEARLINK_HADM_CLIENT_H

#include "nearlink_errorcode.h"
#include "i_nearlink_hadm_client_callback.h"

namespace OHOS {
namespace Nearlink {
namespace {
const std::string NEARLINK_HADM_CLIENT_SERVER = "NearlinkHadmClientServer";
}  // namespace

class INearlinkHadmClient : public OHOS::IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.ipc.INearlinkHadmClient");

    virtual NlErrCode RegisterNearlinkHadmClientCallback(uint32_t &hadmId,
        const sptr<INearlinkHadmClientCallback> &callback) = 0;
    virtual NlErrCode DeregisterNearlinkHadmClientCallback(uint32_t hadmId,
        const sptr<INearlinkHadmClientCallback> &callback) = 0;
    virtual NlErrCode StartSounding(uint32_t hadmId, const NearlinkRawAddress &addr) = 0;
    virtual NlErrCode StopSounding(uint32_t hadmId, const NearlinkRawAddress &addr) = 0;
    virtual NlErrCode GetHadmFeature(uint8_t &capability) = 0;
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // I_NEARLINK_HADM_CLIENT_H