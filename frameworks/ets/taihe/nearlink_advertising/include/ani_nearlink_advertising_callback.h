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

#ifndef ANI_NEARLINK_ADVERTISING_CALLBACK_H
#define ANI_NEARLINK_ADVERTISING_CALLBACK_H

#include <atomic>
#include <memory>
#include "ohos.nearlink.advertising.proj.hpp"
#include "ohos.nearlink.advertising.impl.hpp"
#include "taihe/runtime.hpp"
#include "ani_event_subscribe_module.h"
#include "nearlink_sle_advertiser.h"
#include "taihe_async_callback.h"

namespace OHOS {
namespace Nearlink {

enum AdvertiseMode {
    ADV_TX_POWER_LOW = 1,  //  low-power mode
    ADV_TX_POWER_MEDIUM = 2,       // medium-power mode
    ADV_TX_POWER_HIGH = 3       // high-power mode
};

enum AdvertiseType {
    ADV_NOT_CONNECTABLE = 0, //  advertis is not connectable
    ADV_CONNECTABLE = 1,  //  advertis is connectable
};

enum AdvertisingState {
    STARTED = 1, //  advertising state is started
    STOPPED = 2,  //  advertising state is stopped
};

enum FrameType {
    FRAME_TYPE_1 = 1,  // 无线帧类型 1
    FRAME_TYPE_4 = 4   // 无线帧类型 4
};

const char *const  REGISTER_ADVERTISING_STATE_INFO_NAME = "advertisingStateChange";

class AniNearlinkAdvertisingCallback : public SleAdvertiseCallback {
public:
    AniNearlinkAdvertisingCallback();
    ~AniNearlinkAdvertisingCallback() override = default;

    static std::shared_ptr<AniNearlinkAdvertisingCallback> GetInstance();

    void OnStartResultEvent(int result, int advHandle) override;
    void OnStopResultEvent(int result, int advHandle) override;
    void OnSetAdvDataEvent(int result) override {}
    void OnGetAdvHandleEvent(int result, int advHandle) override;

    TaiheAsyncWorkMap asyncPromiseMap_ {};
    AniEventSubscribeModule eventSubscribe_;
};
}  // namespace Nearlink
}  // namespace OHOS

#endif  // ANI_NEARLINK_ADVERTISING_CALLBACK_H
