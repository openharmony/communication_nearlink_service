/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#ifndef LIS_SERVER_H
#define LIS_SERVER_H

#include <string>
#include "interface_profile_ssap_server.h"
#include "ssap_server_service.h"
#include "nearlink_def.h"
#include "ssap_data.h"
#include "ssap_inner_def.h"
#include "LisDefines.h"

namespace OHOS {
namespace Nearlink {
class LisServer {
public:
    /**
     * @brief Get device info server singleton instance reference.
     *
     * @return Returns the singleton instance reference.
     */
    static LisServer &GetInstance();
    LisServer();
    ~LisServer();
    int RegisterLisServerApplication();
    int DeregisterLisServerApplication();

private:
    class LisSsapServerCallback;
    std::unique_ptr<Service> BuildService();
    InterfaceProfileSsapServer *GetSsapServerService();
    void RegisterLisServerApplicationInterface();
    void DeregisterLisServerApplicationInterface();
    bool LoadDeviceTypeInd();
    std::shared_ptr<LisSsapServerCallback> serviceCallback_ {nullptr};
    std::unique_ptr<Service> instance_ = {nullptr};
    int appId_ = -1;
    std::string deviceTypeInd_ = LIS_DEVICE_TYPE_IND;
};
} // namespace Sle
} // namespace OHOS
#endif // LIS_SERVER_H