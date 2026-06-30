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
#include "nearlink_dft_common_event_helper.h"
#include "common_event.h"
#include "common_event_data.h"
#include "log_util.h"

using namespace OHOS::EventFwk;

namespace OHOS {
namespace NearlinkHelper {
namespace {
const int INVALID_EVENT_CODE = -1;
const char *COMMON_EVENT_NEARLINK_HOST_AUDIO_EXCEP = "usual.event.nearlink.host.AUDIO_EXCEP";
const char *PARAM_KEY_CODEC_TYPE = "codecType";
const char *PARAM_KEY_BITRATE_INFO = "bitrateInfo";
const char *PARAM_KEY_DEVICE_ADDR = "deviceAddr";
const char *PARAM_KEY_FREQ_BAND = "freqBand";
}
bool NearlinkDftCommonEventHelper::PublishEvent(const OHOS::AAFwk::Want &want, int eventCode,
    bool isOrdered, bool isSticky, const std::vector<std::string> &permissions)
{
    HILOGI("enter, eventAction: %{public}s", want.GetAction().c_str());
    OHOS::EventFwk::CommonEventData data;
    if (eventCode != INVALID_EVENT_CODE) {
        data.SetCode(eventCode);
    }
    data.SetWant(want);
    OHOS::EventFwk::CommonEventPublishInfo publishInfo;
    publishInfo.SetOrdered(isOrdered);
    // sticky tag: EventFwk would keep last event for later subscriber.
    publishInfo.SetSticky(isSticky);
    publishInfo.SetSubscriberPermissions(permissions);
    bool res = OHOS::EventFwk::CommonEventManager::PublishCommonEvent(data, publishInfo);
    HILOGI("publishResult = %{public}d", res);
    return res;
}

void NearlinkDftCommonEventHelper::PublishSleAudioExcepEvent(const std::string &addr, uint16_t codecType,
    uint16_t bitrateInfo, uint16_t freqBand)
{
    HILOGI("enter, addr: %{public}s, codecType: %{public}u, bitrateInfo: %{public}u, freqBand: %{public}u",
        Nearlink::GetEncryptAddr(addr).c_str(), codecType, bitrateInfo, freqBand);
    OHOS::AAFwk::Want want;
    want.SetAction(COMMON_EVENT_NEARLINK_HOST_AUDIO_EXCEP);
    want.SetParam(PARAM_KEY_DEVICE_ADDR, addr);
    want.SetParam(PARAM_KEY_CODEC_TYPE, codecType);
    want.SetParam(PARAM_KEY_BITRATE_INFO, bitrateInfo);
    want.SetParam(PARAM_KEY_FREQ_BAND, freqBand);
    std::vector<std::string> permissions {"ohos.permission.MANAGE_NEARLINK"};
    (void)PublishEvent(want, INVALID_EVENT_CODE, false, false, permissions);
}
}
}