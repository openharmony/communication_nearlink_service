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

#include "ohos.nearlink.ssap.proj.hpp"
#include "ohos.nearlink.ssap.impl.hpp"
#include "ani_nearlink_ssap_client_callback.h"
#include "ani_nearlink_ssap_server_callback.h"
#include "ani_nearlink_utils.h"
#include "ani_nearlink_error.h"
#include "taihe/runtime.hpp"
#include "stdexcept"
#include "log.h"
#include "nearlink_errorcode.h"
#include "nearlink_host.h"
#include "nearlink_remote_device.h"
#include "nearlink_ssap_client.h"
#include "nearlink_ssap_server.h"
#include "nearlink_ssap_service.h"
#include "nearlink_ssap_property.h"
#include "nearlink_ssap_method.h"
#include "nearlink_ssap_event.h"
#include "nearlink_ssap_descriptor.h"
#include "nearlink_uuid.h"
#include "nearlink_utils.h"
#include "ani_nearlink_ssap_client.h"
#include "ani_nearlink_ssap_server.h"

namespace OHOS {
namespace Nearlink {
::ohos::nearlink::ssap::Client CreateClient(taihe::string_view address)
{
    HILOGI("enter");
    std::string remoteAddr = std::string(address);
    std::string invalidAddr = "";
    bool checkRet = IsValidAddress(remoteAddr);
    ANI_NL_ASSERT_RETURN(checkRet, NL_ERR_INVALID_ADDRESS,
        (taihe::make_holder<SsapClientImpl, ::ohos::nearlink::ssap::Client>(invalidAddr)));

    // 提前校验ACCESS权限
    bool isGranted = false;
    NlErrCode checkResult = NearlinkHost::GetInstance().CheckPermissionForNapi(ACCESS_NEARLINK_PERMISSION, isGranted);
    ANI_NL_ASSERT_RETURN(checkResult == NL_NO_ERROR, checkResult,
        (taihe::make_holder<SsapClientImpl, ::ohos::nearlink::ssap::Client>(invalidAddr)));
    ANI_NL_ASSERT_RETURN(isGranted, NL_ERR_PERMISSION_FAILED,
        (taihe::make_holder<SsapClientImpl, ::ohos::nearlink::ssap::Client>(invalidAddr)));

    return taihe::make_holder<SsapClientImpl, ::ohos::nearlink::ssap::Client>(remoteAddr);
}

::ohos::nearlink::ssap::Server CreateServer()
{
    HILOGI("enter");
    bool isGranted = false;
    NlErrCode checkResult = NearlinkHost::GetInstance().CheckPermissionForNapi(ACCESS_NEARLINK_PERMISSION, isGranted);
    ANI_NL_ASSERT_RETURN(checkResult == NL_NO_ERROR, checkResult,
        (taihe::make_holder<SsapServerImpl, ::ohos::nearlink::ssap::Server>()));
    ANI_NL_ASSERT_RETURN(isGranted, NL_ERR_PERMISSION_FAILED,
        (taihe::make_holder<SsapServerImpl, ::ohos::nearlink::ssap::Server>()));

    return taihe::make_holder<SsapServerImpl, ::ohos::nearlink::ssap::Server>();
}
}  // namespace Nearlink
}  // namespace OHOS

// Since these macros are auto-generate, lint will cause false positive.
// NOLINTBEGIN
TH_EXPORT_CPP_API_CreateClient(OHOS::Nearlink::CreateClient);
TH_EXPORT_CPP_API_CreateServer(OHOS::Nearlink::CreateServer);
// NOLINTEND