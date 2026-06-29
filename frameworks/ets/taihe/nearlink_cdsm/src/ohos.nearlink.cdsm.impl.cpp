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

#include "ohos.nearlink.cdsm.proj.hpp"
#include "ohos.nearlink.cdsm.impl.hpp"
#include "ani_nearlink_cdsm_callback.h"
#include "ani_nearlink_utils.h"
#include "ani_nearlink_error.h"
#include "taihe/runtime.hpp"
#include "stdexcept"
#include "log.h"
#include "nearlink_errorcode.h"
#include "nearlink_cdsm_client.h"
#include "nearlink_host.h"

namespace OHOS {
namespace Nearlink {

class CdsmClientImpl {
public:
    explicit CdsmClientImpl(taihe::string_view deviceId)
    {
        HILOGI("enter");

        std::string remoteAddr = std::string(deviceId);
        device_ = std::make_shared<NearlinkRemoteDevice>(remoteAddr, ADAPTER_SLE);
        callback_ = std::make_shared<AniCdsmClientCallback>();
        callback_->SetDeviceAddr(remoteAddr);

        auto callback = std::dynamic_pointer_cast<OHOS::Nearlink::NearlinkCdsmClientCallback>(callback_);
        cdsmClient_ = NearlinkCdsmClient::CreateNearlinkCdsmClient(*device_, callback);
    }

    ::ohos::nearlink::cdsm::CdsmInfo GetCdsmInfo()
    {
        HILOGI("enter");
        ::ohos::nearlink::cdsm::CdsmInfo taiheResult = {
            .members = taihe::array<::ohos::nearlink::cdsm::CdsmMemberInfo>{}
        };
        ANI_NL_ASSERT_RETURN((cdsmClient_ != nullptr), NL_ERR_INTERNAL_ERROR, taiheResult);
        NearlinkCdsInfo cdsInfo;
        int errorCode = cdsmClient_->GetCdsmInfo(cdsInfo);
        ANI_NL_ASSERT_RETURN(errorCode == NL_NO_ERROR, errorCode, taiheResult);
        std::vector<::ohos::nearlink::cdsm::CdsmMemberInfo> cdsVec;
        std::vector<Nearlink::NearlinkCdsMemberInfo> memberList = cdsInfo.GetCdsMemberList();
        for (auto member : memberList) {
            ::ohos::nearlink::cdsm::CdsmMemberInfo cdsMember = {
                .address = static_cast<::taihe::string>(member.GetDeviceAddr()),
                .state = static_cast<::ohos::nearlink::cdsm::CdsmConnectionState::key_t>(member.GetState())
            };
            cdsVec.emplace_back(cdsMember);
        }
        taiheResult.members = taihe::array<::ohos::nearlink::cdsm::CdsmMemberInfo>(
                taihe::copy_data_t{}, cdsVec.data(), cdsVec.size());
        return taiheResult;
    }

    void OnCdsmInfoChange(::taihe::callback_view<void(::ohos::nearlink::cdsm::CdsmInfo const& data)> callback)
    {
        if (callback_) {
            callback_->eventSubscribe_.RegisterEvent(callback);
        }
    }

    void OffCdsmInfoChange(::taihe::optional_view<::taihe::callback<void(
        ::ohos::nearlink::cdsm::CdsmInfo const& data)>> callback)
    {
        if (callback_) {
            callback_->eventSubscribe_.DeregisterEvent(callback);
        }
    }

private:
    std::shared_ptr<AniCdsmClientCallback> callback_ = nullptr;

    std::shared_ptr<NearlinkRemoteDevice> device_ = nullptr;
    std::shared_ptr<NearlinkCdsmClient> cdsmClient_ = nullptr;
};

::ohos::nearlink::cdsm::CdsmClient CreateCdsmClient(taihe::string_view deviceId)
{
    HILOGI("enter");
    std::string remoteAddr = std::string(deviceId);
    bool checkRet = CheckDeviceIdParam(remoteAddr);
    ANI_NL_ASSERT_RETURN(checkRet, NL_ERR_INVALID_PARAM,
        (taihe::make_holder<CdsmClientImpl, ::ohos::nearlink::cdsm::CdsmClient>(nullptr)));

    // 提前校验ACCESS权限
    bool isGranted = false;
    NlErrCode checkResult = NearlinkHost::GetInstance().CheckPermissionForNapi(ACCESS_NEARLINK_PERMISSION, isGranted);
    ANI_NL_ASSERT_RETURN(checkResult == NL_NO_ERROR, checkResult,
        (taihe::make_holder<CdsmClientImpl, ::ohos::nearlink::cdsm::CdsmClient>(nullptr)));
    bool checkCdsmSupport = NearlinkHost::GetInstance().IsNearlinkAudioSupport();
    ANI_NL_ASSERT_RETURN(checkCdsmSupport, NL_ERR_CDSM_NOT_SUPPORT,
        (taihe::make_holder<CdsmClientImpl, ::ohos::nearlink::cdsm::CdsmClient>(nullptr)));
    ANI_NL_ASSERT_RETURN(isGranted, NL_ERR_PERMISSION_FAILED,
        (taihe::make_holder<CdsmClientImpl, ::ohos::nearlink::cdsm::CdsmClient>(nullptr)));

    return taihe::make_holder<CdsmClientImpl, ::ohos::nearlink::cdsm::CdsmClient>(remoteAddr);
}
}  // namespace Nearlink
}  // namespace OHOS

// Since these macros are auto-generate, lint will cause false positive.
// NOLINTBEGIN
TH_EXPORT_CPP_API_CreateCdsmClient(OHOS::Nearlink::CreateCdsmClient);
// NOLINTEND
