/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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
#include "audio_spatialization_manager.h"
#include "audio_errors.h"
#include "audio_policy_manager.h"
#include "log_util.h"

namespace OHOS {
namespace AudioStandard {
using namespace std;
const std::string deviceStr = "AA:BB:CC:DD:EE:FF";
const std::string coDeviceStr = "11:22:33:44:55:66";
AudioSpatializationManager::AudioSpatializationManager()
{
    HILOGI("AudioSpatializationManager start");
}

AudioSpatializationManager::~AudioSpatializationManager()
{
    HILOGI("AudioSpatializationManager::~AudioSpatializationManager");
}

AudioSpatializationManager *AudioSpatializationManager::GetInstance()
{
    static AudioSpatializationManager audioSpatializationManager;
    return &audioSpatializationManager;
}

bool AudioSpatializationManager::IsSpatializationEnabled()
{
    return true;
}

bool AudioSpatializationManager::IsSpatializationEnabled(
    const std::shared_ptr<AudioDeviceDescriptor> &selectedAudioDevice)
{
    return true;
}

bool AudioSpatializationManager::IsSpatializationEnabledForCurrentDevice()
{
    return true;
}

int32_t AudioSpatializationManager::SetSpatializationEnabled(const bool enable)
{
    return 1;
}

int32_t AudioSpatializationManager::SetSpatializationEnabled(
    const std::shared_ptr<AudioDeviceDescriptor> &selectedAudioDevice, const bool enable)
{
    return 1;
}

bool AudioSpatializationManager::IsHeadTrackingEnabled()
{
    return true;
}

bool AudioSpatializationManager::IsHeadTrackingEnabled(
    const std::shared_ptr<AudioDeviceDescriptor> &selectedAudioDevice)
{
    return true;
}

int32_t AudioSpatializationManager::SetHeadTrackingEnabled(const bool enable)
{
    return 1;
}

int32_t AudioSpatializationManager::SetHeadTrackingEnabled(
    const std::shared_ptr<AudioDeviceDescriptor> &selectedAudioDevice, const bool enable)
{
    return 1;
}

int32_t AudioSpatializationManager::RegisterSpatializationEnabledEventListener(
    const std::shared_ptr<AudioSpatializationEnabledChangeCallback> &callback)
{
    return 1;
}

int32_t AudioSpatializationManager::RegisterSpatializationEnabledForCurrentDeviceEventListener(
    const std::shared_ptr<AudioSpatializationEnabledChangeForCurrentDeviceCallback> &callback)
{
    return 1;
}

int32_t AudioSpatializationManager::RegisterHeadTrackingEnabledEventListener(
    const std::shared_ptr<AudioHeadTrackingEnabledChangeCallback> &callback)
{
    return 1;
}

int32_t AudioSpatializationManager::UnregisterSpatializationEnabledEventListener()
{
    return 1;
}

int32_t AudioSpatializationManager::UnregisterSpatializationEnabledForCurrentDeviceEventListener()
{
    return 1;
}

int32_t AudioSpatializationManager::UnregisterHeadTrackingEnabledEventListener()
{
    return 1;
}

bool AudioSpatializationManager::IsSpatializationSupported()
{
    return 1;
}

bool AudioSpatializationManager::IsSpatializationSupportedForDevice(
    const std::shared_ptr<AudioDeviceDescriptor> &selectedAudioDevice)
{
    CHECK_AND_RETURN_RET_LOG(selectedAudioDevice != nullptr, false, "selectedAudioDevice is nullptr");
    if (selectedAudioDevice->macAddress_ == deviceStr) {
        return false;
    }
    return true;
}

bool AudioSpatializationManager::IsHeadTrackingSupported()
{
    return true;
}

bool AudioSpatializationManager::IsHeadTrackingSupportedForDevice(
    const std::shared_ptr<AudioDeviceDescriptor> &selectedAudioDevice)
{
    CHECK_AND_RETURN_RET_LOG(selectedAudioDevice != nullptr, false, "selectedAudioDevice is nullptr");
    if (selectedAudioDevice->macAddress_ == deviceStr) {
        return false;
    }
    return true;
}

int32_t AudioSpatializationManager::UpdateSpatialDeviceState(const AudioSpatialDeviceState audioSpatialDeviceState)
{
    return 1;
}

AudioSpatializationSceneType AudioSpatializationManager::GetSpatializationSceneType()
{
    return SPATIALIZATION_SCENE_TYPE_MUSIC;
}

int32_t AudioSpatializationManager::SetSpatializationSceneType(
    const AudioSpatializationSceneType spatializationSceneType)
{
    return 1;
}

bool AudioSpatializationManager::IsHeadTrackingDataRequested(const std::string &macAddress)
{
    return true;
}

int32_t AudioSpatializationManager::RegisterHeadTrackingDataRequestedEventListener(const std::string &macAddress,
    const std::shared_ptr<HeadTrackingDataRequestedChangeCallback> &callback)
{
    return 1;
}

int32_t AudioSpatializationManager::UnregisterHeadTrackingDataRequestedEventListener(const std::string &macAddress)
{
    return 1;
}
} // namespace AudioStandard
} // namespace OHOS
