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
#ifndef OHOS_NEARLINK_STANDARD_HID_HOST_INTERFACE_H
#define OHOS_NEARLINK_STANDARD_HID_HOST_INTERFACE_H

#include <vector>
#include <string_ex.h>
#include <iremote_broker.h>
#include "nearlink_errorcode.h"
#include "nearlink_service_ipc_interface_code.h"

namespace OHOS {
namespace Nearlink {
namespace {
const std::string PROFILE_HID_HOST_SERVER = "NearlinkHidHostServer";
}  // namespace

class INearlinkHidHost : public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.ipc.INearlinkHidHost");

    virtual NlErrCode HidHostSetReport(std::string device, uint8_t type,
        std::string &report, int& result) = 0;
};
}  // namespace Nearlink
}  // namespace OHOS

#endif // OHOS_NEARLINK_STANDARD_HID_HOST_INTERFACE_H