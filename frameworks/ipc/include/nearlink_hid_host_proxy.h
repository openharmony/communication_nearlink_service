/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_NEARLINK_STANDARD_HID_HOST_PROXY_H
#define OHOS_NEARLINK_STANDARD_HID_HOST_PROXY_H

#include "iremote_proxy.h"
#include "i_nearlink_hid_host.h"

namespace OHOS {
namespace Nearlink {
class NearlinkHidHostProxy : public IRemoteProxy<INearlinkHidHost> {
public:
    explicit NearlinkHidHostProxy(const sptr<IRemoteObject> &impl) : IRemoteProxy<INearlinkHidHost>(impl)
    {}
    ~NearlinkHidHostProxy()
    {}

    NlErrCode HidHostSetReport(std::string device, uint8_t type, std::string &report, int& result) override;

private:
    ErrCode InnerTransact(uint32_t code, MessageOption &flags, MessageParcel &data, MessageParcel &reply);
    static inline BrokerDelegator<NearlinkHidHostProxy> delegator_;
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // OHOS_NEARLINK_STANDARD_HOST_PROXY_H