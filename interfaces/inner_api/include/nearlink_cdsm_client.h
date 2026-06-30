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

#ifndef NEARLINK_CDSM_CLIENT_H
#define NEARLINK_CDSM_CLIENT_H

#include "nearlink_remote_device.h"
#include "nearlink_cdsm_info.h"

namespace OHOS {
namespace Nearlink {
class NearlinkCdsmClientCallback {
public:
    virtual ~NearlinkCdsmClientCallback() = default;
    virtual void OnCdsInfoChanged(const NearlinkCdsInfo &cdsInfo) = 0;
};

class NEARLINK_API NearlinkCdsmClient {
public:
    NearlinkCdsmClient(NearlinkRemoteDevice &device, std::shared_ptr<NearlinkCdsmClientCallback> &callback);
    ~NearlinkCdsmClient();

    static std::shared_ptr<NearlinkCdsmClient> CreateNearlinkCdsmClient(
        NearlinkRemoteDevice &device, std::shared_ptr<NearlinkCdsmClientCallback> &callback);

    NlErrCode GetCdsmInfo(NearlinkCdsInfo &cdsInfo) const;
private:
    NEARLINK_DISALLOW_COPY_AND_ASSIGN(NearlinkCdsmClient);

    NEARLINK_DECLARE_IMPL();
};

} // namespace Nearlink
} // namespace OHOS

#endif //NEARLINK_CDSM_CLIENT_H