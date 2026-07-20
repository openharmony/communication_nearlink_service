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
#include "ASCService.h"

namespace OHOS {
namespace Nearlink {
ASCService::ASCService() : utility::Context(PROFILE_NAME_ASC, "1.0.0")
{
}

ASCService::~ASCService()
{
}

utility::Context *ASCService::GetContext()
{
    return this;
}

ASCService *ASCService::GetService()
{
    return static_cast<ASCService *>(SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_ASC));
}

void ASCService::Init()
{
}

int ASCService::AudioControl(const RawAddress &device, AudioStreamType streamType, int cmd)
{
    return NL_NO_ERROR;
}

void ASCService::StopSink()
{
}

int ASCService::GetAudioDeviceList(std::vector<NearlinkRawAddress>& devices)
{
    return NL_NO_ERROR;
}

int ASCService::GetVirtualAudioDeviceList(std::vector<NearlinkRawAddress>& devices)
{
    return NL_NO_ERROR;
}

int ASCService::GetSupportStreamType(const NearlinkRawAddress &device, uint32_t& supportStreamType)
{
    return NL_NO_ERROR;
}

int ASCService::GetAudioDeviceCodecInfo(const NearlinkRawAddress &device, std::map<AudioStreamType,
    AudioStreamCodecInfo> &info)
{
    return NL_NO_ERROR;
}

int ASCService::SetActiveSinkDevice(const NearlinkRawAddress &device, uint32_t supportStreamType)
{
    HILOGI("[AscService Mocker] SetActiveSinkDevice enter");
    activeSinkDevice_ = device;
    return NL_NO_ERROR;
}

int ASCService::UpdateDeviceRole(const RawAddress &device, uint8_t devRole)
{
    return NL_NO_ERROR;
}

int ASCService::RegisterApplication(const std::shared_ptr<InterfaceASCCallback> &callback)
{
    return NL_NO_ERROR;
}

int ASCService::DeregisterApplication()
{
    return NL_NO_ERROR;
}

int ASCService::DeregisterApplication(const std::shared_ptr<InterfaceASCCallback> &callback)
{
    return NL_NO_ERROR;
}

void ASCService::RegisterAudioPreferredOutPutDeviceChangeListener()
{
}

void ASCService::UnregisterAudioPreferredOutPutDeviceChangeListener()
{
}

void ASCService::RegisterObserver(ASCObserver &disObserver)
{
}

void ASCService::DeregisterObserver(ASCObserver &disObserver)
{
}

void ASCService::Enable()
{
}

void ASCService::Disable()
{
}

int ASCService::Connect(const RawAddress &device)
{
    return NL_NO_ERROR;
}

int ASCService::Disconnect(const RawAddress &device)
{
    return NL_NO_ERROR;
}

std::list<RawAddress> ASCService::GetConnectDevices()
{
    std::list<RawAddress> devices {};
    std::string addr = "00:11:22:33:44:55";
    RawAddress device(addr);
    devices.push_back(device);
    activeSinkDevice_ = device;
    return devices;
}

int ASCService::GetConnectState()
{
    return NL_NO_ERROR;
}

bool ASCService::UpdateSleVirtualDevice(int32_t cmd, const RawAddress &device)
{
    return true;
}

int32_t ASCService::SetMusicMuteWhenAudioRelease()
{
    return NL_NO_ERROR;
}

int32_t ASCService::SetMusicUnmute()
{
    return NL_NO_ERROR;
}

bool ASCService::IsAudioOutputToNlAudio()
{
    return true;
}

void ASCService::SetCurrentDeviceMute()
{
}

void ASCService::RegisterListener()
{
}

void ASCService::CbkGetProperty(const RawAddress& device, const std::vector<AscProp>& properties)
{
}

void ASCService::CbkReadPropFail(const RawAddress& device, uint8_t result)
{
}

void ASCService::CbkCreateStream(const RawAddress& device, uint8_t result, AscStreamInfo streamInfo)
{
}

void ASCService::CbkConfigStream(const RawAddress& device, uint8_t result)
{
}

void ASCService::CbkOpenStream(const RawAddress& device, uint8_t result, const AscQosmInfo& qosmInfo)
{
}

void ASCService::CbkStartStream(const RawAddress& device, uint8_t result, const AscQosmInfo& qosmInfo)
{
}

void ASCService::CbkStopStream(const RawAddress& device, uint8_t result, uint16_t connHandle)
{
}

void ASCService::CbkReleaseStream(const RawAddress& device, uint8_t result, uint16_t connHandle)
{
}

void ASCService::CbkDisconnect(const RawAddress& device, uint8_t result)
{
}

void ASCService::ClearWhenDisconnect(const RawAddress& device)
{
}

bool ASCService::IsStreamExists(const RawAddress& device, AudioStreamType streamType)
{
    return true;
}

const NearlinkRawAddress ASCService::GetActiveSinkDevice() const
{
    HILOGI("[AscService Mocker] GetActiveSinkDevice enter");
    return NearlinkRawAddress(activeSinkDevice_);
}

void ASCService::SleAudioDeviceActionChanged(const NearlinkRawAddress &device, int action)
{
}

void ASCService::SleAudioDeviceActionChanged(const NearlinkRawAddress &device,
    const std::vector<struct AudioStreamInfo> &streamData, int action)
{
}

bool ASCService::IsCalling()
{
    return false;
}

bool ASCService::IsPlaying(const RawAddress &device)
{
    return true;
}

bool ASCService::GetAscQosmInfo(const RawAddress& device, AscQosmInfo& qosmInfo)
{
    return true;
}

void ASCService::SendPlayOrPauseByWearDetection(const RawAddress &devAddr, uint8_t playOrPauseKey)
{
}

void ASCService::AcbSubrateChanged(const RawAddress &device, uint32_t subrate)
{
}

void ASCService::AcbSubrateChangeReq(const RawAddress &device, const SleAcbSubrateParam &subrateParam)
{
}

void ASCService::ProcessBaseEvent(const ASCMessage &event)
{
}

void ASCService::ProcessAssistEvent(const ASCMessage &event)
{
}

void ASCService::ProcessEvent(const ASCMessage &event)
{
}

void ASCService::PostEvent(const ASCMessage &event)
{
}

void ASCService::SetIsCallingFlag(bool isCalling)
{
    HILOGI("[AscService Mocker] SetIsCallingFlag enter");
    isCalling_ = isCalling;
}

bool ASCService::GetIsCallingFlag()
{
    HILOGI("[AscService Mocker] GetIsCallingFlag enter");
    return isCalling_;
}

}  // namespace Nearlink
}  // namespace OHOS