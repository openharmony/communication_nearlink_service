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
#ifndef OHOS_NEARLINK_COMMON_EVENT_HELPER_H
#define OHOS_NEARLINK_COMMON_EVENT_HELPER_H

#include <string>
#include "common_event_manager.h"
#include "common_event_subscriber.h"
#include "nearlink_def.h"

namespace OHOS {
namespace NearlinkHelper {


class NearlinkCommonEventHelper {
public:
    /**
     * @brief publish common event with permissions
     *
     * @return Returns <b>true</b> if the operation is successful; returns <b>false</b> if the operation fails.
     * @since 6
     */
    static bool PublishEvent(const OHOS::AAFwk::Want &want, int eventCode, bool isOrdered, bool isSticky,
        const std::vector<std::string> &permissions);

    static void PublishStateChangeEvent(const Nearlink::SleTransport transport, int code);
    static void PublishFullStateChangeEvent(const Nearlink::SleTransport transport, int code);
    static void PublishDisableNlEvent(const Nearlink::SleTransport transport, const std::string &callingName);
    static void PublishScanStartedEvent(int code);
    static void PublishScanFinishedEvent(int code);
    static void PublishDeviceConnectionStateEvent(int32_t state);
    static void PublishHidConnectionStateEvent(const std::string &device, int32_t state);
    static void PublishRemoteNameChangedEvent(const std::string &device, const std::string &remoteName);
    static void PublishDeviceBatteryLevelEvent(const std::string &device, int32_t batteryLevel);
    static void PublishRemovePairEvent(const std::string &device, bool isKeyMissing);
    static void PublishAutoReconnEvent(const std::string &device, bool isActive);
    static void PublishSleAddrToBtAddrMapEvent(const std::string &sleAddr, const std::string &btAddr);
    static void PublishSleDataTransferEvent(int32_t state, const std::string &addr, const std::string &uuid,
        const std::string &callingName);
    static void PublishSleRangingEvent(int32_t state, const std::string& callingName);
    static void PublishChipResetEvent(int32_t targetState);
    static void PublishAudioConnectionStateEvent(int32_t state);
};
}  // namespace NearlinkHelper
}  // namespace OHOS
#endif