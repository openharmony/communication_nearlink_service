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

#include "nearlink_sle_controller_proxy.h"
#include "nearlink_sle_controller_proxy_core.h"

namespace OHOS::Nearlink {

namespace {
inline NearlinkSleControllerProxyCore *ToCore(NearlinkSleControllerProxy *self)
{
    return reinterpret_cast<NearlinkSleControllerProxyCore *>(self);
}
}
BrokerDelegator<NearlinkSleControllerProxy> NearlinkSleControllerProxy::delegator_;

NearlinkSleControllerProxy::NearlinkSleControllerProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<INearlinkSleController>(impl)
{}

NearlinkSleControllerProxy::~NearlinkSleControllerProxy()
{}

NlErrCode NearlinkSleControllerProxy::SetSleCoexParam(uint16_t maxBitRate, uint8_t dutyCycle)
{
    return ToCore(this)->NearlinkSleControllerProxyCore::SetSleCoexParam(maxBitRate, dutyCycle);
}

NlErrCode NearlinkSleControllerProxy::UpdateConnectInterval(const std::string &device, int32_t intervalType)
{
    return ToCore(this)->NearlinkSleControllerProxyCore::UpdateConnectInterval(device, intervalType);
}

NlErrCode NearlinkSleControllerProxy::SetSleCoexMode(int32_t mode, const std::vector<std::string> &deviceList,
    const std::vector<ConnectionInterval> &paramList)
{
    return ToCore(this)->NearlinkSleControllerProxyCore::SetSleCoexMode(mode, deviceList, paramList);
}

}  // namespace OHOS::Nearlink
