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

#ifndef OHOS_NEARLINK_VCP_CLIENT_PROXY_H
#define OHOS_NEARLINK_VCP_CLIENT_PROXY_H

#include "iremote_proxy.h"
#include "i_nearlink_vcp_client.h"

namespace OHOS {
namespace Nearlink {
class NearlinkVcpClientProxy : public IRemoteProxy<INearlinkVcpClient> {
public:
    explicit NearlinkVcpClientProxy(const sptr<IRemoteObject> &impl) : IRemoteProxy<INearlinkVcpClient>(impl) {}
    ~NearlinkVcpClientProxy() {}

    NlErrCode SetDeviceAbsoluteVolume(const NearlinkRawAddress &addr, int32_t volumeLevel, uint8_t streamType) override;
    NlErrCode GetDeviceMediaVolume(const NearlinkRawAddress &addr, int32_t &mediaVolume) override;
    NlErrCode GetDeviceCallVolume(const NearlinkRawAddress &addr, int32_t &callVolume) override;

private:
    ErrCode InnerTransact(uint32_t code, MessageOption &flags, MessageParcel &data, MessageParcel &reply);
    static inline BrokerDelegator<NearlinkVcpClientProxy> delegator_;
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // OHOS_NEARLINK_VCP_CLIENT_PROXY_H