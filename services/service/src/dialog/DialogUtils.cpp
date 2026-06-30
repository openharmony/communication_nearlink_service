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
#include <algorithm>
#include <string>
#include "DialogUtils.h"
#include "log.h"
#include "log_util.h"
constexpr int32_t PARAM_NUM = 3;
namespace OHOS {
namespace Nearlink {
void NearlinkAbilityConnection::OnAbilityConnectDone(const AppExecFwk::ElementName &element,
    const sptr<IRemoteObject> &remoteObject, int32_t resultCode)
{
    HILOGI("on ability connected");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteInt32(PARAM_NUM);
    data.WriteString16(u"bundleName");
    data.WriteString16(Str8ToStr16(bundleName_));
    data.WriteString16(u"abilityName");
    data.WriteString16(Str8ToStr16(abilityName_));
    data.WriteString16(u"parameters");
    data.WriteString16(Str8ToStr16(commandStr_));

    NL_CHECK_RETURN(data.WriteParcelable(&element), "Connect done element error.");
    NL_CHECK_RETURN(data.WriteRemoteObject(remoteObject), "Connect done remote object error.");
    NL_CHECK_RETURN(data.WriteInt32(resultCode), "Connect done result code error.");

    int32_t errCode = remoteObject->SendRequest(
        AAFwk::IAbilityConnection::ON_ABILITY_CONNECT_DONE, data, reply, option);
    HILOGI("AbilityConnectionWrapperProxy::OnAbilityConnectDone result %{public}d", errCode);
}

void NearlinkAbilityConnection::OnAbilityDisconnectDone(const AppExecFwk::ElementName &element,
    int32_t resultCode)
{
    HILOGI("on ability disconnected");
}
} // namespace Nearlink
} // namespace OHOS