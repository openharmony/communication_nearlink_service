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

#ifndef OHOS_NEARLINK_SLE_CONTROLLER_PROXY_H
#define OHOS_NEARLINK_SLE_CONTROLLER_PROXY_H

#include "i_nearlink_sle_controller.h"
#include "iremote_proxy.h"

namespace OHOS::Nearlink {
class NearlinkSleControllerProxy : public IRemoteProxy<INearlinkSleController> {
public:
    NearlinkSleControllerProxy() = delete;
    explicit NearlinkSleControllerProxy(const sptr<IRemoteObject> &impl);
    ~NearlinkSleControllerProxy() override;
    DISALLOW_COPY_AND_MOVE(NearlinkSleControllerProxy);

    NlErrCode SetSleCoexParam(uint16_t maxBitRate, uint8_t dutyCycle) override;
    NlErrCode UpdateConnectInterval(const std::string &device, int32_t intervalType) override;

private:
    ErrCode InnerTransact(uint32_t code, MessageOption &flags, MessageParcel &data, MessageParcel &reply);
    static inline BrokerDelegator<NearlinkSleControllerProxy> delegator_;
};
}  // namespace OHOS::Nearlink

#endif  // OHOS_NEARLINK_SLE_CONTROLLER_PROXY_H
