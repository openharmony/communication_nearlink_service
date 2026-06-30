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

#include "nearlink_hid_host.h"
#include "nearlink_sa_manager.h"
#include "nearlink_host.h"
#include "i_nearlink_hid_host.h"
#include "nearlink_utils.h"

namespace OHOS {
namespace Nearlink {

struct NearlinkHidHost::impl {
    impl();
    ~impl();

    void HidHostSetReport(std::string device, uint8_t type, std::string &report)
    {
        HILOGI("Enter!");
        int result;
        sptr<INearlinkHidHost> proxy = GetProxy<INearlinkHidHost>(PROFILE_HID_HOST_SERVER);
        NL_CHECK_RETURN(proxy, "proxy is nullptr");
        proxy->HidHostSetReport(device, type, report, result);
    }

    int32_t profileRegisterId_ = 0;
};

NearlinkHidHost::impl::impl()
{
    HILOGI("start");
    std::shared_ptr<NearlinkRegisterInfo> info = std::make_shared<NearlinkRegisterInfo>(PROFILE_HID_HOST_SERVER);
    info->serviceStartedFunc_ = [this](sptr<IRemoteObject> remote) -> void {
        sptr<INearlinkHidHost> proxy = iface_cast<INearlinkHidHost>(remote);
        NL_CHECK_RETURN(proxy, "proxy is nullptr");
    };

    profileRegisterId_ = NearlinkSaManager::GetInstance().RegisterFunc(info);
    if (profileRegisterId_ == INVALID_PROFILE_ID) {
        HILOGE("profileRegisterId_ is invalid");
    }
}

NearlinkHidHost::impl::~impl()
{
    HILOGI("starts");
    NearlinkSaManager::GetInstance().DeregisterFunc(profileRegisterId_);
}

NearlinkHidHost::NearlinkHidHost()
{
    pimpl = std::make_unique<impl>();
}

NearlinkHidHost::~NearlinkHidHost() {}

NearlinkHidHost *NearlinkHidHost::GetProfile()
{
    static NearlinkHidHost instance;
    return &instance;
}

void NearlinkHidHost::HidHostSetReport(std::string device, uint8_t type, std::string &report)
{
    NL_CHECK_RETURN(NearlinkHost::GetInstance().IsNearlinkSupport(), "nearlink is not support.");
    NL_CHECK_RETURN(IS_SLE_ENABLED(), "nearlink is off.");
    sptr<INearlinkHidHost> proxy = GetProxy<INearlinkHidHost>(PROFILE_HID_HOST_SERVER);
    NL_CHECK_RETURN(proxy != nullptr, "proxy is nullptr");

    NL_CHECK_RETURN(IsValidAddress(device),  "device address is invalid !");

    return pimpl->HidHostSetReport(device, type, report);
}

} // namespace Nearlink
} // namespace OHOS