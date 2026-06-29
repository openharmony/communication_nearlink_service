/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef NEARLINK_DFT_DATABASE_UE_H
#define NEARLINK_DFT_DATABASE_UE_H

#include "hisysevent_record_c.h"

#ifdef __cplusplus
extern "C" {
#endif


/*******************************************************************************
 *  UE Event param index define below
 ******************************************************************************/
/*
    Parameter Index, Corresponding to g_templateOneUeParam.
    If you have the same param structure as g_templateOneUeParam(parameter type is consistent),
    you can also use this index.
*/
typedef enum UeParamsTemplateOneEnum : int {
    UE_TEMPLATE_ONE_INVALID = 0,
    // add key param below this
    UE_TEMPLATE_ONE_DEVICE_ADDR,
    // add key param above this
    UE_TEMPLATE_ONE_KEY_BUTT,
    UE_TEMPLATE_ONE_DEVICE_NAME,
    UE_TEMPLATE_ONE_DEVICE_APPEARANCE,
    // add normal param below this
    UE_TEMPLATE_ONE_SCENE_CODE,
    UE_TEMPLATE_ONE_SUB_SCENE_CODE,
    UE_TEMPLATE_ONE_TIME,
    // add normal param above this
    UE_TEMPLATE_ONE_PARAM_BUTT,
} UeParamsTemplateOneEnum;

/*
    Parameter Index, Corresponding to g_templateTwoUeParam.
    If you have the same param structure as g_templateTwoUeParam(parameter type is consistent),
    you can also use this index.
*/
typedef enum UeParamsTemplateTwoEnum : int {
    UE_TEMPLATE_TWO_INVALID = 0,
    // add key param below this
    UE_TEMPLATE_TWO_DEVICE_ADDR,
    // add key param above this
    UE_TEMPLATE_TWO_KEY_BUTT,
    UE_TEMPLATE_TWO_DEVICE_NAME,
    UE_TEMPLATE_TWO_DEVICE_APPEARANCE,
    // add normal param below this
    UE_TEMPLATE_TWO_SCENE_CODE,
    UE_TEMPLATE_TWO_SUB_SCENE_CODE,
    UE_TEMPLATE_TWO_CALLING_NAME,
    UE_TEMPLATE_TWO_TIME,
    // add normal param above this
    UE_TEMPLATE_TWO_PARAM_BUTT,
} UeParamsTemplateTwoEnum;

/*
    Parameter Index, Corresponding to g_templateThreeUeParam.
    If you have the same param structure as g_templateThreeUeParam(parameter type is consistent),
    you can also use this index.
*/
typedef enum UeParamsTemplateThreeEnum : int {
    UE_TEMPLATE_THREE_INVALID = 0,
    // add key param below this
    UE_TEMPLATE_THREE_DEVICE_ADDR,
    // add key param above this
    UE_TEMPLATE_THREE_KEY_BUTT,
    UE_TEMPLATE_THREE_DEVICE_NAME,
    UE_TEMPLATE_THREE_DEVICE_APPEARANCE,
    // add normal param below this
    UE_TEMPLATE_THREE_SCENE_CODE,
    UE_TEMPLATE_THREE_CALLING_NAME,
    UE_TEMPLATE_THREE_TIME,
    // add normal param above this
    UE_TEMPLATE_THREE_PARAM_BUTT,
} UeParamsTemplateThreeEnum;

// COMMON UE param enum, used for UE events with similar parameter structures
typedef enum UeParamsTemplateFourEnum : int {
    UE_TEMPLATE_FOUR_INVALID = 0,
    // add key param below this
    UE_TEMPLATE_FOUR_DEVICE_ADDR,
    // add key param above this
    UE_TEMPLATE_FOUR_KEY_BUTT,
    UE_TEMPLATE_FOUR_DEVICE_NAME,
    UE_TEMPLATE_FOUR_DEVICE_APPEARANCE,
    // add normal param below this
    UE_TEMPLATE_FOUR_SCENE_CODE,
    UE_TEMPLATE_FOUR_SUB_SCENE_CODE,
    UE_TEMPLATE_FOUR_THR_PARAM_TYPE_INT,
    // add normal param above this
    UE_TEMPLATE_FOUR_PARAM_BUTT,
} UeParamsTemplateFourEnum;

/*
    Parameter Index, Corresponding to g_templateFiveUeParam.
    If you have the same param structure as g_templateFiveUeParam(parameter type is consistent),
    you can also use this index.
*/
typedef enum UeParamsTemplateFiveEnum : int {
    UE_TEMPLATE_FIVE_INVALID = 0,
    // add key param below this
    // add key param above this
    UE_TEMPLATE_FIVE_KEY_BUTT,
    // add normal param below this
    UE_TEMPLATE_FIVE_SCENE_CODE,
    UE_TEMPLATE_FIVE_SUB_SCENE_CODE,
    UE_TEMPLATE_FIVE_TIME,
    // add normal param above this
    UE_TEMPLATE_FIVE_PARAM_BUTT,
} UeParamsTemplateFiveEnum;

// COMMON UE param enum, used for UE events with similar parameter structures
typedef enum UeParamsTemplateSixEnum : int {
    UE_TEMPLATE_SIX_INVALID = 0,
    // add key param below this
    // add key param above this
    UE_TEMPLATE_SIX_KEY_BUTT,
    // add normal param below this
    UE_TEMPLATE_SIX_SCENE_CODE,
    UE_TEMPLATE_SIX_SUB_SCENE_CODE,
    UE_TEMPLATE_SIX_THR_PARAM_TYPE_INT,
    // add normal param above this
    UE_TEMPLATE_SIX_PARAM_BUTT,
} UeParamsTemplateSixEnum;

/*
    Parameter Index, Corresponding to g_templateSevenUeParam.
    If you have the same param structure as g_templateSevenUeParam(parameter type is consistent),
    you can also use this index.
*/
typedef enum UeParamsTemplateSevenEnum : int {
    UE_TEMPLATE_SEVEN_INVALID = 0,
    // add key param below this
    // add key param above this
    UE_TEMPLATE_SEVEN_KEY_BUTT,
    // add normal param below this
    UE_TEMPLATE_SEVEN_SCENE_CODE,
    UE_TEMPLATE_SEVEN_SUB_SCENE_CODE,
    UE_TEMPLATE_SEVEN_CALLING_NAME,
    UE_TEMPLATE_SEVEN_TIME,
    // add normal param above this
    UE_TEMPLATE_SEVEN_PARAM_BUTT,
} UeParamsTemplateSevenEnum;

typedef enum UeParamsTemplateEightEnum : int {
    UE_TEMPLATE_EIGHT_INVALID = 0,
    // add key param below this
    // add key param above this
    UE_TEMPLATE_EIGHT_KEY_BUTT,
    // add normal param below this
    UE_TEMPLATE_EIGHT_PEER_INFO,
    UE_TEMPLATE_EIGHT_SCENE_CODE,
    UE_TEMPLATE_EIGHT_SUB_SCENE_CODE,
    UE_TEMPLATE_EIGHT_TIME,
    // add normal param above this
    UE_TEMPLATE_EIGHT_PARAM_BUTT,
} UeParamsTemplateEightEnum;

// NEARLINK_MEASURE_STATE_UE param enum
typedef enum UeParamsMeasureStateEnum : int {
    MEASURE_STATE_INVALID = 0,
    // add key param below this
    // add key param above this
    MEASURE_STATE_KEY_BUTT,
    // add normal param below this
    MEASURE_STATE_INDEX,
    MEASURE_STATE_SCENE_CODE,
    MEASURE_STATE_SUB_SCENE_CODE,
    // add normal param above this
    MEASURE_STATE_PARAM_BUTT,
} UeParamsMeasureStateEnum;

// NEARLINK_AUDIO_SINK_DEVICE_UE param enum
typedef enum UeParamsAudioSinkDeviceEnum : int {
    AUDIO_SINK_DEVICE_INVALID = 0,
    // add key param below this
    // add key param above this
    AUDIO_SINK_DEVICE_KEY_BUTT,
    // add normal param below this
    AUDIO_SINK_DEVICE_ADDR,
    AUDIO_SINK_DEVICE_NAME,
    AUDIO_SINK_DEVICE_APPEARANCE,
    AUDIO_SINK_DEVICE_SCENE_CODE,
    AUDIO_SINK_DEVICE_SUB_SCENE_CODE,
    AUDIO_SINK_DEVICE_CHANGE_REASON,
    AUDIO_SINK_DEVICE_TIME,
    // add normal param above this
    AUDIO_SINK_DEVICE_PARAM_BUTT,
} UeParamsAudioSinkDeviceEnum;

// NEARLINK_AUDIO_SOURCE_DEVICE_UE param enum
typedef enum UeParamsAudioSourceDeviceEnum : int {
    AUDIO_SOURCE_DEVICE_INVALID = 0,
    // add key param below this
    // add key param above this
    AUDIO_SOURCE_DEVICE_KEY_BUTT,
    // add normal param below this
    AUDIO_SOURCE_DEVICE_CURRENT_ADDR,
    AUDIO_SOURCE_DEVICE_CURRENT_NAME,
    AUDIO_SOURCE_DEVICE_CURRENT_APPEARANCE,
    AUDIO_SOURCE_DEVICE_OLD_ADDR,
    AUDIO_SOURCE_DEVICE_OLD_NAME,
    AUDIO_SOURCE_DEVICE_OLD_APPEARANCE,
    AUDIO_SOURCE_DEVICE_SCENE_CODE,
    AUDIO_SOURCE_DEVICE_SUB_SCENE_CODE,
    AUDIO_SOURCE_DEVICE_TIME,
    // add normal param above this
    AUDIO_SOURCE_DEVICE_PARAM_BUTT,
} UeParamsAudioSourceDeviceEnum;

// NEARLINK_AUDIO_MUTE_STREAM_UE param enum
typedef enum UeParamsAudioMuteStreamEnum : int {
    AUDIO_MUTE_STREAM_INVALID = 0,
    // add key param below this
    // add key param above this
    AUDIO_MUTE_STREAM_KEY_BUTT,
    // add normal param below this
    AUDIO_MUTE_STREAM_DEVICE_ADDR,
    AUDIO_MUTE_STREAM_DEVICE_NAME,
    AUDIO_MUTE_STREAM_DEVICE_APPEARANCE,
    AUDIO_MUTE_STREAM_SCENE_CODE,
    AUDIO_MUTE_STREAM_SUB_SCENE_CODE,
    AUDIO_MUTE_STREAM_MUTE_CHECK_TIME,
    AUDIO_MUTE_STREAM_DEBOUNCE_TIME,
    AUDIO_MUTE_APP_NAME,
    // add normal param above this
    AUDIO_MUTE_STREAM_PARAM_BUTT,
} UeParamsAudioMuteStreamEnum;
#ifdef __cplusplus
}
#endif

#endif // NEARLINK_DFT_DATABASE_UE_H