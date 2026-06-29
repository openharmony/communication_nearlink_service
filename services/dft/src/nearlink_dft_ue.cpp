/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "nearlink_dft_ue.h"
#include <string>
#include "log.h"
#include "nearlink_dft_manager_c.h"
#include "nearlink_dft_utils.h"
#include "nearlink_dft_manager.h"
#include "nearlink_utils.h"
#include "nearlink_dft_device_data.h"
#include "nearlink_dft_exception.h"

namespace OHOS {
namespace Nearlink {

NearlinkDftUe::NearlinkDftUe()
{}

NearlinkDftUe& NearlinkDftUe::GetInstance()
{
    static NearlinkDftUe instance;
    return instance;
}

void NearlinkDftUe::WriteCommonUe(DftEventEnum eventId, const RawAddress &device, const int sceneCode,
    const int subSceneCode)
{
    std::vector<DftParamC> params;
    std::string peerDeviceName = DEFAULT_DEVICE_NAME;
    int appearance = DEFAULT_APPEARANCE;
    NL_CHECK_RETURN(IsValidAddress(device.GetAddress()), "invalid addr");
    DftDeviceManager::GetInstance().FillCommonDeviceInfo(device, peerDeviceName, appearance);
    std::string time = std::to_string(GetTimestamp());
    params.emplace_back(CreateStrParamC(UE_TEMPLATE_ONE_DEVICE_ADDR, device.GetAddress()));
    params.emplace_back(CreateStrParamC(UE_TEMPLATE_ONE_DEVICE_NAME, peerDeviceName));
    params.emplace_back(CreateI32ParamC(UE_TEMPLATE_ONE_DEVICE_APPEARANCE, appearance));
    params.emplace_back(CreateI32ParamC(UE_TEMPLATE_ONE_SCENE_CODE, sceneCode));
    params.emplace_back(CreateI32ParamC(UE_TEMPLATE_ONE_SUB_SCENE_CODE, subSceneCode));
    if (eventId > DFT_DATATRANSFER_EXCEP) {
        params.emplace_back(CreateStrParamC(UE_TEMPLATE_ONE_TIME, time));
    }
    DftManagerReport(eventId, params.data(), params.size());
}

void NearlinkDftUe::WriteCommonUe(DftEventEnum eventId, const RawAddress &device, const int sceneCode,
    const int subSceneCode, std::string callingName)
{
    std::vector<DftParamC> params;
    std::string peerDeviceName = DEFAULT_DEVICE_NAME;
    int appearance = DEFAULT_APPEARANCE;
    NL_CHECK_RETURN(IsValidAddress(device.GetAddress()), "invalid addr");
    DftDeviceManager::GetInstance().FillCommonDeviceInfo(device, peerDeviceName, appearance);
    std::string time = std::to_string(GetTimestamp());
    params.emplace_back(CreateStrParamC(UE_TEMPLATE_TWO_DEVICE_ADDR, device.GetAddress()));
    params.emplace_back(CreateStrParamC(UE_TEMPLATE_TWO_DEVICE_NAME, peerDeviceName));
    params.emplace_back(CreateI32ParamC(UE_TEMPLATE_TWO_DEVICE_APPEARANCE, appearance));
    params.emplace_back(CreateI32ParamC(UE_TEMPLATE_TWO_SCENE_CODE, sceneCode));
    params.emplace_back(CreateI32ParamC(UE_TEMPLATE_TWO_SUB_SCENE_CODE, subSceneCode));
    params.emplace_back(CreateStrParamC(UE_TEMPLATE_TWO_CALLING_NAME, callingName));
    if (eventId > DFT_DATATRANSFER_EXCEP) {
        params.emplace_back(CreateStrParamC(UE_TEMPLATE_TWO_TIME, time));
    }
    DftManagerReport(eventId, params.data(), params.size());
}

void NearlinkDftUe::WriteCommonUe(DftEventEnum eventId, const RawAddress &device, const int sceneCode,
    std::string callingName)
{
    std::vector<DftParamC> params;
    std::string peerDeviceName = DEFAULT_DEVICE_NAME;
    int appearance = DEFAULT_APPEARANCE;
    NL_CHECK_RETURN(IsValidAddress(device.GetAddress()), "invalid addr");
    DftDeviceManager::GetInstance().FillCommonDeviceInfo(device, peerDeviceName, appearance);
    std::string time = std::to_string(GetTimestamp());
    params.emplace_back(CreateStrParamC(UE_TEMPLATE_THREE_DEVICE_ADDR, device.GetAddress()));
    params.emplace_back(CreateStrParamC(UE_TEMPLATE_THREE_DEVICE_NAME, peerDeviceName));
    params.emplace_back(CreateI32ParamC(UE_TEMPLATE_THREE_DEVICE_APPEARANCE, appearance));
    params.emplace_back(CreateI32ParamC(UE_TEMPLATE_THREE_SCENE_CODE, sceneCode));
    params.emplace_back(CreateStrParamC(UE_TEMPLATE_THREE_CALLING_NAME, callingName));
    if (eventId > DFT_DATATRANSFER_EXCEP) {
        params.emplace_back(CreateStrParamC(UE_TEMPLATE_THREE_TIME, time));
    }
    DftManagerReport(eventId, params.data(), params.size());
}

void NearlinkDftUe::WriteCommonUe(DftEventEnum eventId, const int sceneCode, const int subSceneCode)
{
    std::vector<DftParamC> params;
    params.emplace_back(CreateI32ParamC(UE_TEMPLATE_FIVE_SCENE_CODE, sceneCode));
    params.emplace_back(CreateI32ParamC(UE_TEMPLATE_FIVE_SUB_SCENE_CODE, subSceneCode));
    std::string time = std::to_string(GetTimestamp());
    if (eventId > DFT_DATATRANSFER_EXCEP) {
        params.emplace_back(CreateStrParamC(UE_TEMPLATE_FIVE_TIME, time));
    }
    DftManagerReport(eventId, params.data(), params.size());
}

void NearlinkDftUe::WriteCommonUe(DftEventEnum eventId, const int sceneCode, const int subSceneCode,
    std::string callingName)
{
    std::vector<DftParamC> params;
    std::string time = std::to_string(GetTimestamp());
    params.emplace_back(CreateI32ParamC(UE_TEMPLATE_SEVEN_SCENE_CODE, sceneCode));
    params.emplace_back(CreateI32ParamC(UE_TEMPLATE_SEVEN_SUB_SCENE_CODE, subSceneCode));
    params.emplace_back(CreateStrParamC(UE_TEMPLATE_SEVEN_CALLING_NAME, callingName));
    if (eventId > DFT_DATATRANSFER_EXCEP) {
        params.emplace_back(CreateStrParamC(UE_TEMPLATE_SEVEN_TIME, time));
    }
    DftManagerReport(eventId, params.data(), params.size());
}

void NearlinkDftUe::WriteCommonUe(DftEventEnum eventId, const RawAddress &device, const int accessType,
    const int sceneCode, const int subSceneCode)
{
    std::vector<DftParamC> refKeys;
    refKeys.emplace_back(CreateStrParamC(PEER_INFO_ADDR, device.GetAddress()));
    refKeys.emplace_back(CreateUi16ParamC(PEER_INFO_TYPE, accessType));
    DftSubEventRefC peerInfoRef = {DFT_PEER_INFO, refKeys.data(), refKeys.size()};
    std::vector<DftParamC> params;
    std::string time = std::to_string(GetTimestamp());
    params.emplace_back(CreateI32ParamC(UE_TEMPLATE_EIGHT_SCENE_CODE, sceneCode));
    params.emplace_back(CreateI32ParamC(UE_TEMPLATE_EIGHT_SUB_SCENE_CODE, subSceneCode));
    params.emplace_back(CreateRefParamC(UE_TEMPLATE_EIGHT_PEER_INFO, peerInfoRef));
    if (eventId > DFT_DATATRANSFER_EXCEP) {
        params.emplace_back(CreateStrParamC(UE_TEMPLATE_EIGHT_TIME, time));
    }
    DftManagerReport(eventId, params.data(), params.size());
}

void NearlinkDftUe::WriteMeasureStateUe(DftEventEnum eventId, const int measureIndex, const int measureStateSceneCode,
    int measureStateSubSceneCode)
{
    std::vector<DftParamC> params;
    params.emplace_back(CreateI32ParamC(MEASURE_STATE_INDEX, measureIndex));
    params.emplace_back(CreateI32ParamC(MEASURE_STATE_SCENE_CODE, measureStateSceneCode));
    params.emplace_back(CreateI32ParamC(MEASURE_STATE_SUB_SCENE_CODE, measureStateSubSceneCode));
    DftManagerReport(eventId, params.data(), params.size());
}

void NearlinkDftUe::WriteAudioSinkDeviceUe(const RawAddress &device, int sceneCode, int subSceneCode,
    uint16_t changeReason)
{
    std::vector<DftParamC> params;
    std::string peerDeviceName = DEFAULT_DEVICE_NAME;
    int appearance = DEFAULT_APPEARANCE;
    NL_CHECK_RETURN(IsValidAddress(device.GetAddress()), "invalid addr");
    DftDeviceManager::GetInstance().FillCommonDeviceInfo(device, peerDeviceName, appearance);
    std::string time = std::to_string(GetTimestamp());
    params.emplace_back(CreateStrParamC(AUDIO_SINK_DEVICE_ADDR, device.GetAddress()));
    params.emplace_back(CreateStrParamC(AUDIO_SINK_DEVICE_NAME, peerDeviceName));
    params.emplace_back(CreateI32ParamC(AUDIO_SINK_DEVICE_APPEARANCE, appearance));
    params.emplace_back(CreateI32ParamC(AUDIO_SINK_DEVICE_SCENE_CODE, sceneCode));
    params.emplace_back(CreateI32ParamC(AUDIO_SINK_DEVICE_SUB_SCENE_CODE, subSceneCode));
    params.emplace_back(CreateUi16ParamC(AUDIO_SINK_DEVICE_CHANGE_REASON, changeReason));
    params.emplace_back(CreateStrParamC(AUDIO_SINK_DEVICE_TIME, time));
    DftManagerReport(DFT_AUDIO_SINK_DEVICE_UE, params.data(), params.size());
}

void NearlinkDftUe::WriteAudioControlUeAndExcep(const RawAddress &device, int sceneCode, int subSceneCode,
    uint16_t changeReason, NlErrCode controlResult)
{
    WriteAudioSinkDeviceUe(device, sceneCode, subSceneCode, changeReason);
    if (controlResult != NL_NO_ERROR) {
        DftReportAudioError(device.GetAddress(), sceneCode, subSceneCode);
    }
}

void NearlinkDftUe::WriteAudioSourceDeviceUe(const RawAddress &curDevice, const RawAddress &oldDevice, int sceneCode,
    int subSceneCode)
{
    std::vector<DftParamC> params;

    std::string currentDevName = DEFAULT_DEVICE_NAME;
    int currentAppearance = DEFAULT_APPEARANCE;
    NL_CHECK_RETURN(IsValidAddress(curDevice.GetAddress()), "invalid addr");
    DftDeviceManager::GetInstance().FillCommonDeviceInfo(curDevice, currentDevName, currentAppearance);

    std::string oldDevName = DEFAULT_DEVICE_NAME;
    int oldAppearance = DEFAULT_APPEARANCE;
    NL_CHECK_RETURN(IsValidAddress(oldDevice.GetAddress()), "invalid addr");
    DftDeviceManager::GetInstance().FillCommonDeviceInfo(oldDevice, oldDevName, oldAppearance);

    std::string time = std::to_string(GetTimestamp());

    params.emplace_back(CreateStrParamC(AUDIO_SOURCE_DEVICE_CURRENT_ADDR, curDevice.GetAddress()));
    params.emplace_back(CreateStrParamC(AUDIO_SOURCE_DEVICE_CURRENT_NAME, currentDevName));
    params.emplace_back(CreateI32ParamC(AUDIO_SOURCE_DEVICE_CURRENT_APPEARANCE, currentAppearance));
    params.emplace_back(CreateStrParamC(AUDIO_SOURCE_DEVICE_OLD_ADDR, oldDevice.GetAddress()));
    params.emplace_back(CreateStrParamC(AUDIO_SOURCE_DEVICE_OLD_NAME, oldDevName));
    params.emplace_back(CreateI32ParamC(AUDIO_SOURCE_DEVICE_OLD_APPEARANCE, oldAppearance));
    params.emplace_back(CreateI32ParamC(AUDIO_SOURCE_DEVICE_SCENE_CODE, sceneCode));
    params.emplace_back(CreateI32ParamC(AUDIO_SOURCE_DEVICE_SUB_SCENE_CODE, subSceneCode));
    params.emplace_back(CreateStrParamC(AUDIO_SOURCE_DEVICE_TIME, time));
    DftManagerReport(DFT_AUDIO_SOURCE_DEVICE_UE, params.data(), params.size());
}

void NearlinkDftUe::WriteAudioMuteStreamUe(const RawAddress &device, int sceneCode, int subSceneCode,
    int debounceTime, std::string appName)
{
    std::vector<DftParamC> params;
    std::string peerDeviceName = DEFAULT_DEVICE_NAME;
    int appearance = DEFAULT_APPEARANCE;
    NL_CHECK_RETURN(IsValidAddress(device.GetAddress()), "invalid addr");
    DftDeviceManager::GetInstance().FillCommonDeviceInfo(device, peerDeviceName, appearance);
    params.emplace_back(CreateStrParamC(AUDIO_MUTE_STREAM_DEVICE_ADDR, device.GetAddress()));
    params.emplace_back(CreateStrParamC(AUDIO_MUTE_STREAM_DEVICE_NAME, peerDeviceName));
    params.emplace_back(CreateI32ParamC(AUDIO_MUTE_STREAM_DEVICE_APPEARANCE, appearance));
    params.emplace_back(CreateI32ParamC(AUDIO_MUTE_STREAM_SCENE_CODE, sceneCode));
    params.emplace_back(CreateI32ParamC(AUDIO_MUTE_STREAM_SUB_SCENE_CODE, subSceneCode));
    params.emplace_back(CreateI32ParamC(AUDIO_MUTE_STREAM_MUTE_CHECK_TIME, CHECK_TIME_INTERVAL_MS));
    params.emplace_back(CreateI32ParamC(AUDIO_MUTE_STREAM_DEBOUNCE_TIME, debounceTime));
    params.emplace_back(CreateStrParamC(AUDIO_MUTE_APP_NAME, appName));
    DftManagerReport(DFT_AUDIO_MUTE_STREAM_UE, params.data(), params.size());
}

}  // namespace Nearlink
}  // namespace OHOS