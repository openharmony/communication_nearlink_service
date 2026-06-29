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
#include "napi_nearlink_cdsm_callback.h"
#include "log.h"
#include "napi_async_work.h"
#include "napi_native_object.h"
#include "napi_nearlink_cdsm.h"

namespace OHOS {
namespace Nearlink {

NapiNearlinkCdsmClientCallback::NapiNearlinkCdsmClientCallback()
    : eventSubscribe(NEARLINK_CALLBACK_CDSM_INFO_CHANGE, NL_MODULE_NAME)
{}

void NapiNearlinkCdsmClientCallback::OnCdsInfoChanged(const NearlinkCdsInfo& cdsInfo)
{
    HILOGI("cdsm member size=%{public}zu", cdsInfo.GetCdsMemberList().size());
    NL_CHECK_RETURN(cdsInfo.GetCdsMemberList().size() != 0, "cds member invalid");
    const auto napiNative = std::make_shared<NapiNativeCdsmInfo>(cdsInfo);
    eventSubscribe.PublishEvent(NEARLINK_CALLBACK_CDSM_INFO_CHANGE, napiNative);
}

} // namespace Nearlink
} // namespace OHOS
