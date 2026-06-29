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

#ifndef ANI_NEARLINK_CDSM_CALLBACK_H_
#define ANI_NEARLINK_CDSM_CALLBACK_H_

#include <shared_mutex>
#include "stdexcept"

#include "ohos.nearlink.cdsm.proj.hpp"
#include "ohos.nearlink.cdsm.impl.hpp"
#include "taihe/runtime.hpp"
#include "ani_event_module.h"
#include "nearlink_cdsm_client.h"

namespace OHOS {
namespace Nearlink {
using namespace Nearlink;

class AniCdsmClientCallback : public NearlinkCdsmClientCallback {
public:
    void OnCdsInfoChanged(const NearlinkCdsInfo& cdsInfo) override;
    std::string GetDeviceAddr(void)
    {
        return deviceAddr_;
    }

    void SetDeviceAddr(std::string deviceAddr)
    {
        deviceAddr_ = deviceAddr;
    }

    AniCdsmClientCallback();
    ~AniCdsmClientCallback() override = default;

private:
    friend class CdsmClientImpl;
    EventModule<void(::ohos::nearlink::cdsm::CdsmInfo const& data)> eventSubscribe_;

    std::string deviceAddr_ = INVALID_MAC_ADDRESS;
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // ANI_NEARLINK_CDSM_CALLBACK_H_