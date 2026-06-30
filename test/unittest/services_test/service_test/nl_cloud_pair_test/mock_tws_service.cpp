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
#include "TwsService.h"

namespace OHOS {
namespace Nearlink {

TwsService::TwsService() : utility::Context(PROFILE_NAME_TWS, "1.0.0")
{
    HILOGI("%{public}s: service Create", PROFILE_NAME_TWS.c_str());
}

TwsService::~TwsService()
{
    HILOGI("%{public}s: service Destroy", PROFILE_NAME_TWS.c_str());
}

utility::Context *TwsService::GetContext()
{
    return this;
}

void TwsService::Enable()
{}

void TwsService::Disable()
{}

int TwsService::RegisterApplication(const std::shared_ptr<InterfaceTwsClientObserver> &callback)
{
    return NL_NO_ERROR;
}

int TwsService::DeregisterApplication()
{
    return NL_NO_ERROR;
}

void TwsService::RegisterObserver(TwsObserver &serviceObserver)
{}

void TwsService::DeregisterObserver(TwsObserver &serviceObserver)
{}

int TwsService::Connect(const RawAddress &device)
{
    return NL_NO_ERROR;
}

int TwsService::Disconnect(const RawAddress &device)
{
    return NL_NO_ERROR;
}

int TwsService::GetConnectState()
{
    return 0;
}

std::list<RawAddress> TwsService::GetConnectDevices()
{
    std::list<RawAddress> devices;
    return devices;
}

TwsService *TwsService::GetService()
{
    static TwsService instance;
    return &instance;
}

void TwsService::UpdateClientData(uint8_t dataType, TwsClientData &clientData)
{
    HILOGI("[TwsService Mocker] Call UpdateClientData");
}

void TwsService::TwsGetDeviceRole(const RawAddress &devAddr, uint8_t &devRole)
{}

uint8_t TwsService::TwsGetDeviceNature(const RawAddress &devAddr)
{
    return 0;
}

uint8_t TwsService::TwsGetDeviceAudioMusicType(const RawAddress &devAddr)
{
    return 0;
}

void TwsService::GetTwsAudioDelay(const RawAddress &devAddr, uint32_t &audioDelay)
{}

bool TwsService::TwsIsSupportWearDetect(const RawAddress &devAddr)
{
    return true;
}

void TwsService::TwsGetDeviceWearStatus(const RawAddress &devAddr, TwsDevWearStatus &wearStatus)
{}

void TwsService::TwsEnableWearDetection(const RawAddress &devAddr)
{}

void TwsService::TwsDisableWearDetection(const RawAddress &devAddr)
{}

bool TwsService::TwsIsDeviceWearing(const RawAddress &devAddr)
{
    return true;
}

int TwsService::TwsGetWearDetectionState(const RawAddress &devAddr)
{
    return 0;
}

void TwsService::TwsUpdateDeviceDefaultRole(const RawAddress &devAddr, const uint8_t roleType)
{}

void TwsService::TwsSendUserSelection(const RawAddress &device,
    const std::vector<struct AudioStreamInfo> &streamInfo)
{}

bool TwsService::TwsIsSupportVirtualAutoConnect(const RawAddress &devAddr)
{
    return true;
}

void TwsService::SetVirtualAutoConnectType(const RawAddress &devAddr, int32_t connType, int32_t businessType)
{}

void TwsService::QueryStreamState(const RawAddress &devAddr, std::vector<struct AudioStreamInfo> &streamData)
{}

void TwsService::ProcPauseRecordMap()
{}

void TwsService::UpdateHangUpTimeStamp(RawAddress &devAddr)
{}

void TwsService::SendProfileConnected(const RawAddress &devAddr)
{}

void TwsService::SetDeviceManufacturerAbility(
    const RawAddress &device, const std::array<uint8_t, SLE_MANU_ABILITY_LEN> &manuAbility)  const
{}

} // namespace Nearlink
} // namespace OHOS