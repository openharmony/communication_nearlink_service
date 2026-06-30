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

#ifndef OHOS_NEARLINK_HADM_CLIENT_PROXY_H
#define OHOS_NEARLINK_HADM_CLIENT_PROXY_H

#include "iremote_proxy.h"
#include "i_nearlink_hadm_client.h"
#include "i_nearlink_hadm_client_callback.h"

namespace OHOS {
namespace Nearlink {
class NearlinkHadmClientProxy : public IRemoteProxy<INearlinkHadmClient> {
public:
    NearlinkHadmClientProxy() = delete;
    explicit NearlinkHadmClientProxy(const sptr<IRemoteObject> &impl);
    ~NearlinkHadmClientProxy() override;
    DISALLOW_COPY_AND_MOVE(NearlinkHadmClientProxy);

    NlErrCode RegisterNearlinkHadmClientCallback(uint32_t &hadmId,
        const sptr<INearlinkHadmClientCallback> &callback) override;
    NlErrCode DeregisterNearlinkHadmClientCallback(uint32_t hadmId,
        const sptr<INearlinkHadmClientCallback> &callback) override;
    NlErrCode StartSounding(uint32_t hadmId, const NearlinkRawAddress &addr) override;
    NlErrCode StopSounding(uint32_t hadmId, const NearlinkRawAddress &addr) override;
    NlErrCode GetHadmFeature(uint8_t &capability) override;

private:
    ErrCode InnerTransact(uint32_t code, MessageOption &flags, MessageParcel &data, MessageParcel &reply);
    static inline BrokerDelegator<NearlinkHadmClientProxy> delegator_;
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // OHOS_NEARLINK_HADM_CLIENT_PROXY_H