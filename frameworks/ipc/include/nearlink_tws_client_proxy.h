/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_NEARLINK_TWS_CLIENT_PROXY_H
#define OHOS_NEARLINK_TWS_CLIENT_PROXY_H

#include "iremote_proxy.h"
#include "i_nearlink_tws_client.h"

namespace OHOS {
namespace Nearlink {
class NearlinkTwsClientProxy : public IRemoteProxy<INearlinkTwsClient> {
public:
    explicit NearlinkTwsClientProxy(const sptr<IRemoteObject> &impl) : IRemoteProxy<INearlinkTwsClient>(impl)
    {}
    ~NearlinkTwsClientProxy()
    {}

    NlErrCode RegisterApplication(const sptr<INearlinkTwsClientObserver> &observer) override;
    NlErrCode DeregisterApplication(const sptr<INearlinkTwsClientObserver> &observer) override;
    NlErrCode EnableWearDetection(const std::string &address) override;
    NlErrCode DisableWearDetection(const std::string &address) override;
    NlErrCode GetWearDetectionState(const std::string &address, int32_t &state) override;
    NlErrCode IsDeviceWearing(const std::string &address, bool &isWearing) override;
    NlErrCode IsWearDetectionSupported(const std::string &address, bool &isSupported) override;
    NlErrCode GetTwsRoleInfo(const std::string &address, int32_t &roleInfo) override;
    NlErrCode GetTwsAudioDelay(const std::string &address, uint32_t &delayValue) override;
    NlErrCode SendUserSelection(const std::string &address,
        const std::vector<struct AudioStreamInfo> &streamData) override;
    NlErrCode QueryStreamState(const std::string &address, std::vector<struct AudioStreamInfo> &streamData) override;
    NlErrCode IsSupportVirtualAutoConnect(const std::string &address, bool &isSupported) override;
    NlErrCode SetVirtualAutoConnectType(const std::string &address, int32_t connType, int32_t businessType) override;
private:
    ErrCode InnerTransact(uint32_t code, MessageOption &flags, MessageParcel &data, MessageParcel &reply);
    static inline BrokerDelegator<NearlinkTwsClientProxy> delegator_;
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // OHOS_NEARLINK_TWS_CLIENT_PROXY_H