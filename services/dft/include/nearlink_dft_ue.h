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

#ifndef NEARLINK_DFT_UE_H
#define NEARLINK_DFT_UE_H

#include "raw_address.h"
#include "nearlink_dft_database.h"
#include "nearlink_errorcode.h"

namespace OHOS {
namespace Nearlink {

typedef enum PairType :int {
    PAIRTYPEINVALID,
    CREDIBLEPAIR,
    NOCREDIBLEPAIR,
} PairType;

typedef enum CancelPairType :int {
    CANCELPAIRTYPEINVALID,
    CANCELPAIR,
    REMOVEPAIR,
    KEYMISSING,
} CancelPairType;

typedef enum HasStandardProfile :int {
    NOSTANDARDPROFILE,
    HASSTANDARDPROFILE,
} HasStandardProfile;

typedef enum IsPullDialog :int {
    PULL,
    NOPULL,
} IsPullDialog;

typedef enum PairCfmScenceCode :int {
    PAIRCFMINVALID,
    PAIRINGREQ,
    SETPAIRPASSCODE,
    SETPAIRCONFIRM,
} PairCfmScenceCode;

typedef enum InterfaceCallResult :int {
    CALLSUCCESS,
    CALLFALSE,
} InterfaceCallResult;

typedef enum LocalNameOpera :int {
    CHANGE = 1,
    LOAD,
} LocalNameOpera;

typedef enum DisSceneCode :int {
    LOCALNAME = 1,
} DisSceneCode;

typedef enum AudioSinkDeviceSceneCode :int {
    ADD_DEVICE_SCENE = 1,
    DELETE_DEVICE_SCENE = 2,
    ADD_VIRTUAL_DEVICE_SCENE = 3,
    DELETE_VIRTUAL_DEVICE_SCENE = 4,
    UPDATE_VOICE_STACK_SCENE = 5,
    MEDIA_CONTROL_SCENE = 6,
    CALL_CONTROL_SCENE = 7,
    VOLUME_CONTROL_IN_SINK_SCENE = 8,
    VOICE_ASSISTANT_SCENE_SCENE = 9,
} AudioSinkDeviceSceneCode;

typedef enum AudioSinkDeviceMediaControlSubSceneCode :int {
    MEDIA_CONTROL_SUB_SCENE_PLAY = 1,
    MEDIA_CONTROL_SUB_SCENE_STOP = 2,
    MEDIA_CONTROL_SUB_SCENE_PAUSE = 3,
    MEDIA_CONTROL_SUB_SCENE_FAST_FOR = 4,
    MEDIA_CONTROL_SUB_SCENE_PRE_MEDIA = 5,
    MEDIA_CONTROL_SUB_SCENE_NEXT_MEDIA = 6,
} AudioSinkDeviceMediaControlSubSceneCode;

typedef enum AudioSinkDeviceCallControlSubSceneCode :int {
    CALL_CONTROL_SUB_SCENE_ANSWER = 1,
    CALL_CONTROL_SUB_SCENE_HANG_UP = 2,
    CALL_CONTROL_SUB_SCENE_REJECT = 3,
} AudioSinkDeviceCallControlSubSceneCode;

typedef enum AudioSinkDeviceVolumeControlSubSceneCode :int {
    SINK_VOLUME_CONTROL_MEDIA = 1,
    SINK_VOLUME_CONTROL_CALL = 2,
} AudioSinkDeviceVolumeControlSubSceneCode;

typedef enum AudioSinkDeviceVoiceAssistantSubSceneCode :int {
    VOICE_ASSISTANT_OPEN = 1,
    VOICE_ASSISTANT_CLOSE = 2,
} AudioSinkDeviceVoiceAssistantSubSceneCode;

typedef enum AudioSinkDeviceUpdateVoiceStackReason :int {
    UPDATE_VOICE_STACK_REASON_INVALID = 0,
    UPDATE_VOICE_STACK_REASON_WEAR = 1,
    UPDATE_VOICE_STACK_REASON_UNWEAR = 2,
    UPDATE_VOICE_STACK_REASON_ENABLE_FROM_REMOTE = 3,
    UPDATE_VOICE_STACK_REASON_DISABLE_FROM_REMOTE = 4,
    UPDATE_VOICE_STACK_REASON_ENABLE_WEAR_DETECTION = 5,
    UPDATE_VOICE_STACK_REASON_DISABLE_WEAR_DETECTION = 6,
    UPDATE_VOICE_STACK_REASON_USER_OPERATION_FROM_REMOTE = 7,
} AudioSinkDeviceUpdateVoiceStackReason;

typedef enum AudioSourceDeviceSceneCode :int {
    SET_ACTIVE_DEVICE = 1,
    MEDIA_PLAYER_STATUS_CHANGE = 2,
    VOLUME_CONTROL_IN_SOURCE = 3,
    NEARLINK_CLOUD_PUSH = 4,
    CLOUD_PAIR = 5,
    DUAL_REC_CAP = 6,
    DUAL_REC_START = 7,
    SPATIAL_AUDIO = 8,
} AudioSourceDeviceSceneCode;

typedef enum AudioSourceDeviceSetActiveDeviceSubSceneCode :int {
    CHANGE_ACTIVE_DEVICE = 1,
} AudioSourceDeviceSetActiveDeviceSubSceneCode;

typedef enum AudioSourceDeviceMediaPlayerSubSceneCode :int {
    MEDIA_PLAYER_CHANGE = 1,
    MEDIA_PLAYER_RELEASE = 2,
} AudioSourceDeviceMediaPlayerSubSceneCode;

const int MAX_VOLUME_VALUE = 100;
const int VOLUME_VALUE_10 = 10;
const int VOLUME_VALUE_80 = 80;

typedef enum AudioSourceDeviceVolumeControlSubSceneCode :int {
    VOLUME_BELOW_10 = 1,
    VOLUME_IS_0 = 2,
    VOLUME_ABOVE_80 = 3,
} AudioSourceDeviceVolumeControlSubSceneCode;

typedef enum AudioSourceDeviceCloudPairUpdateSubSceneCode :int {
    // 云配列表刷新
    ADD_DEVICE = 11,
    DELETE_DEVICE = 12,
    REMOVE_DEVICE = 13,
    UPDATE_NAME = 14,
    UPDATE_TOKEN = 15,
    REPLACE_OLD_DEVICE = 16,
    // 异常分支打点
    TOKENT_CHEAK_TIMEOUT = 21,
} AudioSourceDeviceCloudPairUpdateSubSceneCode;

typedef enum AudioSourceDeviceSpatialAudioAdaptiveSwitchSubSceneCode : int {
    SWITCH_ON = 0,
    SWITCH_OFF = 1,
} AudioSourceDeviceSpatialAudioAdaptiveSwitchSubSceneCode;

typedef enum AudioSourceDeviceDualRecSubSceneCode :int {
    DUAL_REC_NOTIFY_PEER = 1,
    DUAL_REC_NOTIFY_DSP = 2,
} AudioSourceDeviceDualRecSubSceneCode;

typedef enum AudioSourceDeviceCloudPushSubSceneCode :int {
    CLOUDPUSH_EVENT_RECEIVED = 1,
    CLOUDPUSH_TRIGGER_UPDATE = 2,
    CLOUDPUSH_5G_RESULT = 3,
} AudioSourceDeviceCloudPushSubSceneCode;

typedef enum AudioMuteStreamSceneCode :int {
    EMPTY_STREAM = 1,
    MUTE_STREAM = 2,
} AudioMuteStreamSceneCode;

typedef enum AudioMuteStreamSubSceneCode :int {
    STREAM_NO_DATA = 1,
    STREAM_DATA_MUTE = 2,
    STREAM_VOLUME_ZERO = 3,
    DATA_MUTE_RESUME = 4,
    VOLUME_ZERO_RESUME = 5,
} AudioMuteStreamSubSceneCode;

class NearlinkDftUe {
public:
    static NearlinkDftUe& GetInstance();
    // used for events similar to g_templateOneUeParam.
    void WriteCommonUe(DftEventEnum eventId, const RawAddress &device, const int sceneCode, const int subSceneCode);
    // used for events similar to g_templateTwoUeParam.
    void WriteCommonUe(DftEventEnum eventId, const RawAddress &device, const int sceneCode, const int subSceneCode,
        std::string callingName);
    // used for events similar to g_templateThreeUeParam.
    void WriteCommonUe(DftEventEnum eventId, const RawAddress &device, const int sceneCode, std::string callingName);
    // used for events similar to g_templateFiveUeParam.
    void WriteCommonUe(DftEventEnum eventId, const int sceneCode, const int subSceneCode);
    // used for events similar to g_templateSevenUeParam.
    void WriteCommonUe(DftEventEnum eventId, const int sceneCode, const int subSceneCode, std::string callingName);
    // used for events similar to g_templateEightUeParam.
    void WriteCommonUe(DftEventEnum eventId, const RawAddress &device, const int accessType, const int sceneCode,
        const int subSceneCode);
    // used for events of UeParamsMeasureStateEnum.
    void WriteMeasureStateUe(DftEventEnum eventId, const int measureIndex, const int measureStateSceneCode,
        int measureStateSubSceneCode);
    // used for events of UeParamsAudioSinkDeviceEnum.
    void WriteAudioSinkDeviceUe(const RawAddress &device, int sceneCode, int subSceneCode, uint16_t changeReason);
    // used for events of UeParamsAudioSinkDeviceEnum and DftAudioProfileExcepParamEnum
    // 控制相关的打点，整合UE与异常的维测在一个逻辑中处理
    void WriteAudioControlUeAndExcep(const RawAddress &device, int sceneCode, int subSceneCode,
        uint16_t changeReason, NlErrCode controlResult);
    // used for events of UeParamsAudioSourceDeviceEnum.
    void WriteAudioSourceDeviceUe(const RawAddress &curDevice, const RawAddress &oldDevice, int sceneCode,
        int subSceneCode);
    void WriteAudioMuteStreamUe(const RawAddress &device, int sceneCode, int subSceneCode,
        int debounceTime, std::string appName);
private:
    NearlinkDftUe();
};

}   // namespace Nearlink
}   // namespace OHOS

#endif // NEARLINK_DFT_UE_H