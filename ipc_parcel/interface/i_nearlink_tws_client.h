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

#ifndef I_NEARLINK_TWS_CLIENT_H
#define I_NEARLINK_TWS_CLIENT_H

#include "iremote_broker.h"
#include "i_nearlink_tws_client_observer.h"
#include "nearlink_errorcode.h"
#include "nearlink_service_ipc_interface_code.h"

namespace OHOS {
namespace Nearlink {
namespace {
const std::string NEARLINK_TWS_CLIENT_SERVER = "NearlinkTwsClientServer";
}  // namespace

class INearlinkTwsClient : public OHOS::IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.ipc.INearlinkTwsClient");

    virtual NlErrCode RegisterApplication(const sptr<INearlinkTwsClientObserver> &observer) = 0;
    virtual NlErrCode DeregisterApplication(const sptr<INearlinkTwsClientObserver> &observer) = 0;
    virtual NlErrCode EnableWearDetection(const std::string &address) = 0;
    virtual NlErrCode DisableWearDetection(const std::string &address) = 0;
    virtual NlErrCode GetWearDetectionState(const std::string &address, int32_t &state) = 0;
    virtual NlErrCode IsDeviceWearing(const std::string &address, bool &isWearing) = 0;
    virtual NlErrCode IsWearDetectionSupported(const std::string &address, bool &isSupported) = 0;
    virtual NlErrCode GetTwsRoleInfo(const std::string &address, int32_t &roleInfo) = 0;
    virtual NlErrCode GetTwsAudioDelay(const std::string &address, uint32_t &delayValue) = 0;
    virtual NlErrCode SendUserSelection(const std::string &address,
        const std::vector<struct AudioStreamInfo> &streamData) = 0;
    virtual NlErrCode QueryStreamState(const std::string &address, std::vector<struct AudioStreamInfo> &streamData) = 0;
    virtual NlErrCode IsSupportVirtualAutoConnect(const std::string &address, bool &isSupported) = 0;
    virtual NlErrCode SetVirtualAutoConnectType(const std::string &address, int32_t connType, int32_t businessType) = 0;
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // I_NEARLINK_TWS_CLIENT_H