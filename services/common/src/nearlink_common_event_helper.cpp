/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include "nearlink_common_event_helper.h"
#include "common_event.h"
#include "common_event_data.h"
#include "SleDefs.h"
#include "log_util.h"

using namespace OHOS::EventFwk;

namespace OHOS {
namespace NearlinkHelper {
/* COMMON EVENTS */
const char *COMMON_EVENT_NEARLINK_HOST_STATE_UPDATE = "usual.event.nearlink.host.STATE_UPDATE";
const char *COMMON_EVENT_NEARLINK_HOST_FULL_STATE_UPDATE = "usual.event.nearlink.host.FULL_STATE_UPDATE";
const char *COMMON_EVENT_NEARLINK_HOST_DISABLE_NEARLINK = "usual.event.nearlink.host.DISABLE_NEARLINK";
const char *COMMON_EVENT_NEARLINK_HOST_SCAN_STARTED = "usual.event.nearlink.host.SCAN_STARTED";
const char *COMMON_EVENT_NEARLINK_HOST_SCAN_FINISHED = "usual.event.nearlink.host.SCAN_FINISHED";
const char *COMMON_EVENT_NEARLINK_HOST_DATA_TRANSFER_UPDATE =
    "usual.event.nearlink.host.DATA_TRANSFER_UPDATE";
const char *COMMON_EVENT_NEARLINK_HOST_RANGING_UPDATE =
    "usual.event.nearlink.host.RANGING_UPDATE";
const char *COMMON_EVENT_NEARLINK_CHIP_RESET = "usual.event.nearlink.host.CHIP_RESET";
const char *COMMON_EVENT_NEARLINK_REMOTEDEVICE_CONNECT_STATUS_VALUE =
    "usual.event.nearlink.remotedevice.CONNECT_STATUS_VALUE";
const char *COMMON_EVENT_NEARLINK_HID_CONNECTION_STATE_CHANGED =
    "usual.event.nearlink.hid.CONNECTION_STATE_CHANGED";
const char *COMMON_EVENT_NEARLINK_REMOTEDEVICE_NAME_UPDATE = "usual.event.nearlink.remotedevice.NAME_UPDATE";
const char *COMMON_EVENT_NEARLINK_REMOTEDEVICE_BATTERY_VALUE_UPDATE =
    "usual.event.nearlink.remotedevice.BATTERY_VALUE_UPDATE";
const char *COMMON_EVENT_NEARLINK_REMOVE_PAIR  = "usual.event.nearlink.remotedevice.REMOVE_PAIR";

const char *COMMON_EVENT_NEARLINK_AUTO_CONNECT_DEVICE = "usual.event.nearlink.remotedevice.AUTO_CONNECT_DEVICE";
const char *COMMON_EVENT_NEARLINK_SLE_BT_ADDR_MAP = "usual.event.nearlink.remotedevice.SLE_BT_ADDR_MAP";

const char *COMMON_EVENT_NEARLINK_AUDIO_CONNECTION_STATE_CHANGED =
    "usual.event.nearlink.audio.CONNECTION_STATE_CHANGED";

/* COMMON EVENT PARAM KEYS */
const char *PARAM_KEY_DEVICE_ADDR = "deviceAddr";
const char *PARAM_KEY_STATE = "state";
const char *PARAM_KEY_TARGET_STATE = "targetState";
const char *PARAM_KEY_REMOTE_NAME = "remoteName";
const char *PARAM_KEY_BATTERY_LEVEL = "batteryLevel";
const char *PARAM_KEY_UUID = "uuid";
const char *PARAM_KEY_DEVICE_BT_ADDR = "deviceBtAddr";
const char *PARAM_KEY_ACTIVATED_DEVICE = "lastActiveDevice";
const char *PARAM_KEY_CONNECTED_DEVICE = "lastConnectedDevice";
const char *PARAM_KEY_CALLING_NAME = "callingName";
const char *PARAM_KEY_CLOUD_PAIR_KEY_MISSING_FALG = "cloudPairKeymissingFlag";

// permissions
const std::vector<std::string> ACCESS_PERMISSIONS {"ohos.permission.ACCESS_NEARLINK"};
const std::vector<std::string> EMPTY_PERMISSION {};
const std::vector<std::string> MANAGE_PERMISSIONS {"ohos.permission.MANAGE_NEARLINK"};

const int INVALID_EVENT_CODE = -1;

bool NearlinkCommonEventHelper::PublishEvent(const OHOS::AAFwk::Want &want, int eventCode,
    bool isOrdered, bool isSticky, const std::vector<std::string> &permissions)
{
    HILOGD("enter");
    HILOGI("eventAction: %{public}s", want.GetAction().c_str());
    OHOS::EventFwk::CommonEventData data;
    data.SetWant(want);
    if (eventCode != INVALID_EVENT_CODE) {
        data.SetCode(eventCode);
    }
    OHOS::EventFwk::CommonEventPublishInfo publishInfo;
    publishInfo.SetOrdered(isOrdered);
    // sticky tag: EventFwk would keep last event for later subscriber.
    publishInfo.SetSticky(isSticky);
    if (permissions.size() > 0) {
        publishInfo.SetSubscriberPermissions(permissions);
    }
    bool publishResult = OHOS::EventFwk::CommonEventManager::PublishCommonEvent(data, publishInfo);
    HILOGI("publishResult = %{public}d", publishResult);
    return publishResult;
}

void NearlinkCommonEventHelper::PublishStateChangeEvent(const Nearlink::SleTransport transport, int code)
{
    if (transport == Nearlink::SleTransport::ADAPTER_SLE) {
        if (code == Nearlink::SleStateID::STATE_TURN_ON || code == Nearlink::SleStateID::STATE_TURN_OFF) {
            HILOGI("PublishStateChangeEvent with code = %{public}d", code);
            OHOS::AAFwk::Want want;
            want.SetAction(COMMON_EVENT_NEARLINK_HOST_STATE_UPDATE);
            (void)PublishEvent(want, code, false, false, EMPTY_PERMISSION);
        } else {
            HILOGD("non-public state change");
        }
    } else {
        HILOGE("unsupported transport");
    }
}

void NearlinkCommonEventHelper::PublishFullStateChangeEvent(const Nearlink::SleTransport transport, int code)
{
    if (transport == Nearlink::SleTransport::ADAPTER_SLE) {
        if (code == Nearlink::SleStateID::STATE_TURN_ON || code == Nearlink::SleStateID::STATE_TURN_OFF ||
            code == Nearlink::SleStateID::STATE_TURN_HALF) {
            HILOGI("PublishFullStateChangeEvent with code = %{public}d", code);
            OHOS::AAFwk::Want want;
            want.SetAction(COMMON_EVENT_NEARLINK_HOST_FULL_STATE_UPDATE);
            (void)PublishEvent(want, code, false, false, EMPTY_PERMISSION);
        } else {
            HILOGD("non-public state change");
        }
    } else {
        HILOGE("unsupported transport");
    }
}

void NearlinkCommonEventHelper::PublishDisableNlEvent(const Nearlink::SleTransport transport,
    const std::string &callingName)
{
    NL_CHECK_RETURN(transport == Nearlink::SleTransport::ADAPTER_SLE, "unsupported transport");
    HILOGI("PublishDisableNlEvent with callingName = %{public}s", callingName.c_str());
    OHOS::AAFwk::Want want;
    want.SetAction(COMMON_EVENT_NEARLINK_HOST_DISABLE_NEARLINK);
    want.SetParam("callingName", callingName);
    (void)PublishEvent(want, INVALID_EVENT_CODE, false, false, MANAGE_PERMISSIONS);
}

void NearlinkCommonEventHelper::PublishScanStartedEvent(int code)
{
    if (code == Nearlink::SCAN_STARTED) {
        HILOGI("PublishScanStartedEvent with code = %{public}d", code);
        OHOS::AAFwk::Want want;
        want.SetAction(COMMON_EVENT_NEARLINK_HOST_SCAN_STARTED);
        std::vector<std::string> permissions {};
        (void)PublishEvent(want, code, false, false, permissions);
    }
}

void NearlinkCommonEventHelper::PublishScanFinishedEvent(int code)
{
    if (code == Nearlink::SCAN_STOPED) {
        HILOGI("PublishScanFinishedEvent with code = %{public}d", code);
        OHOS::AAFwk::Want want;
        want.SetAction(COMMON_EVENT_NEARLINK_HOST_SCAN_FINISHED);
        (void)PublishEvent(want, code, false, false, ACCESS_PERMISSIONS);
    }
}

void NearlinkCommonEventHelper::PublishDeviceConnectionStateEvent(int32_t state)
{
    HILOGI("PublishDeviceConnectionStateEvent with state = %{public}d", state);
    OHOS::AAFwk::Want want;
    want.SetAction(COMMON_EVENT_NEARLINK_REMOTEDEVICE_CONNECT_STATUS_VALUE);
    want.SetParam(PARAM_KEY_STATE, state);
    (void)PublishEvent(want, INVALID_EVENT_CODE, false, true, ACCESS_PERMISSIONS);
}

void NearlinkCommonEventHelper::PublishHidConnectionStateEvent(const std::string &device,
    int32_t state)
{
    OHOS::AAFwk::Want want;
    want.SetAction(COMMON_EVENT_NEARLINK_HID_CONNECTION_STATE_CHANGED);
    want.SetParam(PARAM_KEY_DEVICE_ADDR, device);
    want.SetParam(PARAM_KEY_STATE, state);
    (void)PublishEvent(want, INVALID_EVENT_CODE, false, true, MANAGE_PERMISSIONS);
}

void NearlinkCommonEventHelper::PublishRemoteNameChangedEvent(const std::string &device, const std::string &remoteName)
{
    OHOS::AAFwk::Want want;
    want.SetAction(COMMON_EVENT_NEARLINK_REMOTEDEVICE_NAME_UPDATE);
    want.SetParam(PARAM_KEY_DEVICE_ADDR, device);
    want.SetParam(PARAM_KEY_REMOTE_NAME, remoteName);
    std::vector<std::string> permissions {"ohos.permission.ACCESS_NEARLINK", "ohos.permission.MANAGE_NEARLINK"};
    (void)PublishEvent(want, INVALID_EVENT_CODE, false, false, permissions);
}

void NearlinkCommonEventHelper::PublishDeviceBatteryLevelEvent(const std::string &device, int32_t batteryLevel)
{
    OHOS::AAFwk::Want want;
    want.SetAction(COMMON_EVENT_NEARLINK_REMOTEDEVICE_BATTERY_VALUE_UPDATE);
    want.SetParam(PARAM_KEY_DEVICE_ADDR, device);
    want.SetParam(PARAM_KEY_BATTERY_LEVEL, batteryLevel);
    std::vector<std::string> permissions {"ohos.permission.ACCESS_NEARLINK", "ohos.permission.MANAGE_NEARLINK"};
    (void)PublishEvent(want, INVALID_EVENT_CODE, false, true, permissions);
}

void NearlinkCommonEventHelper::PublishRemovePairEvent(const std::string &device, bool isKeyMissing)
{
    OHOS::AAFwk::Want want;
    want.SetAction(COMMON_EVENT_NEARLINK_REMOVE_PAIR);
    want.SetParam(PARAM_KEY_DEVICE_ADDR, device);
    want.SetParam(PARAM_KEY_CLOUD_PAIR_KEY_MISSING_FALG, isKeyMissing);
    std::vector<std::string> permissions {};
    (void)PublishEvent(want, INVALID_EVENT_CODE, false, false, permissions);
}

void NearlinkCommonEventHelper::PublishSleAddrToBtAddrMapEvent(const std::string &sleAddr, const std::string &btAddr)
{
    OHOS::AAFwk::Want want;
    want.SetAction(COMMON_EVENT_NEARLINK_SLE_BT_ADDR_MAP);
    want.SetParam(PARAM_KEY_DEVICE_ADDR, sleAddr);
    want.SetParam(PARAM_KEY_DEVICE_BT_ADDR, btAddr);
    (void)PublishEvent(want, INVALID_EVENT_CODE, false, false, MANAGE_PERMISSIONS);
}

void NearlinkCommonEventHelper::PublishAutoReconnEvent(const std::string &device, bool isActive)
{
    OHOS::AAFwk::Want want;
    want.SetAction(COMMON_EVENT_NEARLINK_AUTO_CONNECT_DEVICE);
    if (isActive) {
        want.SetParam(PARAM_KEY_ACTIVATED_DEVICE, device);
    } else {
        want.SetParam(PARAM_KEY_CONNECTED_DEVICE, device);
    }
    (void)PublishEvent(want, INVALID_EVENT_CODE, false, false, MANAGE_PERMISSIONS);
}

void NearlinkCommonEventHelper::PublishSleDataTransferEvent(int32_t state, const std::string &addr,
    const std::string &uuid, const std::string &callingName)
{
    HILOGD("enter");
    OHOS::AAFwk::Want want;
    want.SetAction(COMMON_EVENT_NEARLINK_HOST_DATA_TRANSFER_UPDATE);
    want.SetParam(PARAM_KEY_STATE, state);
    want.SetParam(PARAM_KEY_DEVICE_ADDR, addr);
    want.SetParam(PARAM_KEY_UUID, uuid);
    want.SetParam(PARAM_KEY_CALLING_NAME, callingName);
    std::vector<std::string> permissions {"ohos.permission.MANAGE_NEARLINK"};
    (void)PublishEvent(want, INVALID_EVENT_CODE, false, false, permissions);
}

void NearlinkCommonEventHelper::PublishSleRangingEvent(int32_t state, const std::string& callingName)
{
    HILOGD("enter");
    OHOS::AAFwk::Want want;
    want.SetAction(COMMON_EVENT_NEARLINK_HOST_RANGING_UPDATE);
    want.SetParam(PARAM_KEY_STATE, state);
    want.SetParam(PARAM_KEY_CALLING_NAME, callingName);
    std::vector<std::string> permissions {"ohos.permission.MANAGE_NEARLINK"};
    (void)PublishEvent(want, INVALID_EVENT_CODE, false, false, permissions);
}

void NearlinkCommonEventHelper::PublishChipResetEvent(int32_t targetState)
{
    OHOS::AAFwk::Want want;
    want.SetAction(COMMON_EVENT_NEARLINK_CHIP_RESET);
    want.SetParam(PARAM_KEY_TARGET_STATE, targetState);
    std::vector<std::string> permissions {"ohos.permission.ACCESS_NEARLINK", "ohos.permission.MANAGE_NEARLINK"};
    (void)PublishEvent(want, INVALID_EVENT_CODE, false, false, permissions);
}

void NearlinkCommonEventHelper::PublishAudioConnectionStateEvent(int32_t state)
{
    OHOS::AAFwk::Want want;
    want.SetAction(COMMON_EVENT_NEARLINK_AUDIO_CONNECTION_STATE_CHANGED);
    want.SetParam(PARAM_KEY_STATE, state);
    (void)PublishEvent(want, INVALID_EVENT_CODE, false, true, MANAGE_PERMISSIONS);
}
} // namespace NearlinkHelper
} // namespace OHOS
