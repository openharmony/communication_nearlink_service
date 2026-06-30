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
#ifndef CDSM_CLIENT_H
#define CDSM_CLIENT_H

#include <map>
#include <mutex>

#include "ssap_data.h"
#include "ssap_def.h"
#include "log_util.h"
#include "interface_profile_ssap_client.h"
#include "SleInterfaceProfileManager.h"
#include "CdsmMessage.h"
#include "SleInterfaceProfileCdsm.h"
#include "BaseObserverList.h"
#include "nearlink_safe_map.h"

namespace OHOS {
namespace Nearlink {

class CdsmClient {
public:
    explicit CdsmClient(const std::string &address) : address_(address) {}

    /*  Destroy the CdsmClient object */
    ~CdsmClient() = default;

    /*  Construct a new CdsmClient object */
    static std::shared_ptr<CdsmClient> CreateCdsmClient(const std::string &address);

    /* 连接合作集服务 */
    bool CdsmClientStartConnect();
    bool CdsmClientStartDisconnect();
    void CdsmClientUpdateState(CdsmClientState toState);
    CdsmClientState CdsmClientGetState();

    /* 停止合作集广播 */
    void CdsmStopInviteAdv();
private:
    std::string address_ = "";
    std::atomic<CdsmClientState> cdsmClientState_ = CdsmClientState::CDSM_STATE_DISCONNECTED; /* 成员Profile连接状态 */

    void CdsmClientSetState(CdsmClientState newState);
}; // CdsmClient
} // namespace Sle
} // namespace OHOS

#endif