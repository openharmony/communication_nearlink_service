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

#ifndef OHOS_NEARLINK_STANDARD_TWS_CLIENT_SERVER_H
#define OHOS_NEARLINK_STANDARD_TWS_CLIENT_SERVER_H

#include <mutex>
#include "nearlink_def.h"
#include "nearlink_tws_client_stub.h"
#include "nearlink_types.h"
#include "iservice_registry.h"
#include "system_ability.h"

namespace OHOS {
namespace Nearlink {

class NearlinkTwsClientServer : public NearlinkTwsClientStub {
public:
    explicit NearlinkTwsClientServer();
    ~NearlinkTwsClientServer() override;

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
        const std::vector<struct AudioStreamInfo> &streamInfo) override;
    NlErrCode QueryStreamState(const std::string &address, std::vector<struct AudioStreamInfo> &streamData) override;
    NlErrCode IsSupportVirtualAutoConnect(const std::string &address, bool &isSupported) override;
    NlErrCode SetVirtualAutoConnectType(const std::string &address, int32_t connType, int32_t businessType) override;
private:
    NEARLINK_DISALLOW_COPY_AND_ASSIGN(NearlinkTwsClientServer);
    NEARLINK_DECLARE_IMPL();
};

}  // namespace NearLink
}  // namespace OHOS
#endif // OHOS_NEARLINK_STANDARD_TWS_CLIENT_SERVER_H