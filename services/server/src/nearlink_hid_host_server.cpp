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

#include "nearlink_hid_host_server.h"
#include "SleInterfaceProfileManager.h"
#include "SleInterfaceProfileHidHost.h"
#include "i_nearlink_hid_host.h"
#include "log_util.h"

namespace OHOS {
namespace Nearlink {

NearlinkHidHostServer::NearlinkHidHostServer()
{
    HILOGI("NearlinkHidHostServer called.");
}

NearlinkHidHostServer::~NearlinkHidHostServer()
{
    HILOGW("~NearlinkHidHostServer called.");
}

NlErrCode NearlinkHidHostServer::HidHostSetReport(std::string device, uint8_t type,
    std::string &report, int& result)
{
    HILOGI("address: %{public}s", GetEncryptAddr(device).c_str());
    ProfileHidHost *hidHostService = static_cast<ProfileHidHost *>
        (SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_HID_HOST));
    NL_CHECK_RETURN_RET(hidHostService, NL_ERR_INTERNAL_ERROR, "hidHostService is nullptr.");
    result = hidHostService->HidHostSendReport(device, type, report.size(), report);
    return NL_NO_ERROR;
}
}
}