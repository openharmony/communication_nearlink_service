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

#ifndef I_NEARLINK_TWS_CLIENT_OBSERVER_H
#define I_NEARLINK_TWS_CLIENT_OBSERVER_H

#include "nearlink_service_ipc_interface_code.h"
#include "iremote_broker.h"

namespace OHOS {
namespace Nearlink {
class INearlinkTwsClientObserver : public OHOS::IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.ipc.INearlinkTwsClientObserver");

    virtual void OnTwsRemoteInfo(const std::string &address, const std::vector<uint8_t> &value) = 0;
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // I_NEARLINK_TWS_CLIENT_OBSERVER_H
