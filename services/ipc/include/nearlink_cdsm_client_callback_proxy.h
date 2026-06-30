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

#ifndef OHOS_NEARLINK_CDSM_CLIENT_CALLBACK_PROXY_H
#define OHOS_NEARLINK_CDSM_CLIENT_CALLBACK_PROXY_H

#include "iremote_proxy.h"
#include "i_nearlink_cdsm_client_callback.h"

namespace OHOS {
namespace Nearlink {
class NearlinkCdsmClientCallbackProxy : public IRemoteProxy<INearlinkCdsmClientCallback> {
public:
    NearlinkCdsmClientCallbackProxy() = delete;
    explicit NearlinkCdsmClientCallbackProxy(const sptr<IRemoteObject> &impl);
    ~NearlinkCdsmClientCallbackProxy() override;
    DISALLOW_COPY_AND_MOVE(NearlinkCdsmClientCallbackProxy);

    void OnCdsInfoChanged(const NearlinkCdsInfoParcel &cdsInfo) override;
private:
    ErrCode InnerTransact(uint32_t code, MessageOption &flags, MessageParcel &data, MessageParcel &reply);
    static inline BrokerDelegator<NearlinkCdsmClientCallbackProxy> delegator_;
};
}  // namespace Nearlink
}  // namespace OHOS
#endif // OHOS_NEARLINK_CDSM_CLIENT_CALLBACK_PROXY_H