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

#include "ani_nearlink_cdsm_callback.h"
#include "ohos.nearlink.cdsm.proj.hpp"
#include "ohos.nearlink.cdsm.impl.hpp"
#include "log_util.h"

namespace OHOS {
namespace Nearlink {

AniCdsmClientCallback::AniCdsmClientCallback()
{}

void AniCdsmClientCallback::OnCdsInfoChanged(const NearlinkCdsInfo& cdsInfo)
{
    std::shared_lock<std::shared_mutex> guard(eventSubscribe_.lock_);
    HILOGI("enter");

    std::vector<::ohos::nearlink::cdsm::CdsmMemberInfo> cdsVec;
    std::vector<Nearlink::NearlinkCdsMemberInfo> memberList = cdsInfo.GetCdsMemberList();
    for (auto member : memberList) {
        ::ohos::nearlink::cdsm::CdsmMemberInfo cdsMember = {
            .address = static_cast<::taihe::string>(member.GetDeviceAddr()),
            .state = static_cast<::ohos::nearlink::cdsm::CdsmConnectionState::key_t>(member.GetState())
        };
        cdsVec.emplace_back(cdsMember);
    }
    ::ohos::nearlink::cdsm::CdsmInfo taiheResult = {
        .members = taihe::array<::ohos::nearlink::cdsm::CdsmMemberInfo>(
            taihe::copy_data_t{}, cdsVec.data(), cdsVec.size())
    };

    for (auto callback : eventSubscribe_.callbackVec_) {
        if (callback) {
            (*callback)(taiheResult);
        }
    }
}
}  // namespace Nearlink
}  // namespace OHOS