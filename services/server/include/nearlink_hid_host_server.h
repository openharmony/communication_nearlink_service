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

#ifndef OHOS_NEARLINK_STANDARD_HID_HOST_SERVER_H
#define OHOS_NEARLINK_STANDARD_HID_HOST_SERVER_H

#include <mutex>
#include "nearlink_def.h"
#include "nearlink_hid_host_stub.h"
#include "nearlink_types.h"
#include "iservice_registry.h"
#include "system_ability.h"

namespace OHOS {
namespace Nearlink {

class NearlinkHidHostServer : public NearlinkHidHostStub {
public:
    explicit NearlinkHidHostServer();
    ~NearlinkHidHostServer() override;

    NlErrCode HidHostSetReport(std::string device, uint8_t type, std::string &report, int& result) override;
private:
    NEARLINK_DISALLOW_COPY_AND_ASSIGN(NearlinkHidHostServer);
};

}  // namespace NearLink
}  // namespace OHOS
#endif // OHOS_NEARLINK_STANDARD_HID_HOST_SERVER_H