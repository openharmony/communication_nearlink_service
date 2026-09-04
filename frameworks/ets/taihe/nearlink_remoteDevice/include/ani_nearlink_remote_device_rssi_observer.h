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
#ifndef ANI_NEARLINK_REMOTE_DEVICE_RSSI_OBSERVER_H
#define ANI_NEARLINK_REMOTE_DEVICE_RSSI_OBSERVER_H

#include <shared_mutex>
#include "nearlink_host.h"
#include "taihe_async_work.h"

namespace OHOS {
namespace Nearlink {
class AniRemoteDeviceRssiObserver : public NearlinkRemoteDeviceRssiObserver {
public:
    AniRemoteDeviceRssiObserver();
    ~AniRemoteDeviceRssiObserver() override = default;
    static std::shared_ptr<AniRemoteDeviceRssiObserver> GetInstance(void);

    void OnReadRemoteRssiEvent(const NearlinkRemoteDevice &device, int rssi, int status) override;

    TaiheAsyncWorkMap asyncPromiseMap_ {};
};

}  // namespace Nearlink
}  // namespace OHOS
#endif /* ANI_NEARLINK_REMOTE_DEVICE_RSSI_OBSERVER_H */