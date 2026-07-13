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

#include <charconv>
#include "SleCloudPairService.h"
#include "SleInterfaceManager.h"
#include "SleInterfaceAdapterSub.h"
#include "SleRemoteDeviceAdapter.h"
#include "interface_cloud_pair_service.h"
#include "nearlink_def.h"
#include "log_util.h"
#include "CdsmService.h"
#include "SleConfig.h"
#include "nearlink_common_event_helper.h"
#include "nearlink_dft_ue.h"
#include "nearlink_verification_manager.h"

namespace OHOS {
namespace Nearlink {
namespace {
static constexpr uint32_t TOKEN_LEN = 32;
static constexpr uint32_t MEMBER_LIST_LENGTH = 2;
static constexpr uint32_t MAX_ICON_DEC_STR_LENGTH = 6;
constexpr int TOKEN_CHECK_TIMEOUT_MS = 10000;
}
//LCOV_EXCL_START
DownCloudPairDevice::DownCloudPairDevice()
{}

DownCloudPairDevice::~DownCloudPairDevice()
{}

void DownCloudPairDevice::SetCloudPairState(int32_t state)
{
    int preCloudPairState = cloudPairState_;
    cloudPairState_ = state;
    HILOGI("[CLOUD PAIR] device %{public}s switch cloud pair state %{public}d -> %{public}d",
        GetEncryptAddr(reportAddr_).c_str(), preCloudPairState, cloudPairState_);
    if (cloudPairState_ == NL_CLOUD_PAIR_STATE::CLOUD_PAIR_NONE ||
        cloudPairState_ == NL_CLOUD_PAIR_STATE::CLOUD_PAIR_PAIRED) {
        InitConnectedState();
        SetPrimary("");
    }
    RawAddress device(reportAddr_);
    NearlinkDftUe::GetInstance().WriteAudioSourceDeviceUe(device, device, CLOUD_PAIR, state);
}

int32_t DownCloudPairDevice::GetCloudPairState()
{
    return cloudPairState_;
}

void DownCloudPairDevice::InitConnectedState()
{
    NL_CHECK_RETURN(membersAddr_.size() == MEMBER_LIST_LENGTH,
        "[CLOUD PAIR] %{public}s members size = %{public}d invalid", GetEncryptAddr(reportAddr_).c_str(),
        membersAddr_.size());
    for (auto member : membersAddr_) {
        connectedMaps_[member] = false;
    }
}

bool DownCloudPairDevice::IsAllMembersDisconnected()
{
    for (auto &it : connectedMaps_) {
        if (it.second) {
            return false;
        }
    }
    return true;
}

void DownCloudPairDevice::SetConnectedState(std::string addr, bool isConnected)
{
    NL_CHECK_RETURN(connectedMaps_.find(addr) != connectedMaps_.end(),
        "[CLOUD PAIR] %{public}s not in connectedMaps_", GetEncryptAddr(addr).c_str());
    connectedMaps_[addr] = isConnected;
    if (isConnected) {
        SetRole(addr);
    }
}

void DownCloudPairDevice::SetRole(std::string addr)
{
    if (primary_.empty()) {
        SetPrimary(addr);
    }
}

void DownCloudPairDevice::SetPrimary(std::string addr)
{
    primary_ = addr;
    HILOGD("[CLOUD PAIR] reportAddr : %{public}s, role_primary : %{public}s",
        GetEncryptAddr(reportAddr_).c_str(), GetEncryptAddr(addr).c_str());
}

std::string DownCloudPairDevice::GetPrimary()
{
    return primary_;
}

void DownCloudPairDevice::SetConnectedMaps(std::map<std::string, bool> connectedMaps)
{
    connectedMaps_ = connectedMaps;
}

std::map<std::string, bool> DownCloudPairDevice::GetConnectedMaps()
{
    return connectedMaps_;
}

InterfaceCloudPairService &InterfaceCloudPairService::GetInstance()
{
    return SleCloudPairService::GetInstance();
}

SleCloudPairService &SleCloudPairService::GetInstance()
{
    static SleCloudPairService instance;
    return instance;
}

SleCloudPairService::SleCloudPairService()
{}

SleCloudPairService::~SleCloudPairService()
{}

void SleCloudPairService::Init()
{
    std::vector<std::string> cloudDeviceList = SleConfig::GetInstance().GetCloudDeviceAddrList();
    ReadCloudDeviceInfoFromConf(cloudDeviceList);
}

void SleCloudPairService::HandlePairStatusChanged(const RawAddress &device, int32_t preStatus,
    int32_t status, int32_t reason)
{
    if (IsInReplacing(device) && preStatus == static_cast<int32_t>(SlePairState::SLE_PAIR_PAIRED) &&
        status == static_cast<int32_t>(SlePairState::SLE_PAIR_NONE)) {
        replacedDevices_.Erase(device.GetAddress());
        HILOGI("[CLOUD PAIR] replace old cloud device : %{public}s end", GET_ENCRYPT_ADDR(device));
        return;
    }
    if (!IsCloudDevice(device)) {
        HILOGI("[CLOUD PAIR] %{public}s is not cloudDevice", GET_ENCRYPT_ADDR(device));
        return;
    }
    HILOGI("[CLOUD PAIR] device %{public}s, pair status: %{public}d", GET_ENCRYPT_ADDR(device), status);
    RawAddress reportAddr = GetReportAddr(device);
    std::string addr = reportAddr.GetAddress();
    int32_t curCloudPairState = NL_CLOUD_PAIR_STATE::CLOUD_PAIR_INVALID;
    GetCloudPairState(addr, curCloudPairState);
    // 配对成功
    if (preStatus == static_cast<int32_t>(SlePairState::SLE_PAIR_PAIRING) &&
        status == static_cast<int32_t>(SlePairState::SLE_PAIR_PAIRED) &&
        (curCloudPairState == NL_CLOUD_PAIR_STATE::CLOUD_PAIR_CREATE_PAIR ||
         curCloudPairState == NL_CLOUD_PAIR_STATE::CLOUD_PAIR_TOKEN_CHANGING)) {
        UpdateCloudState(addr, NL_CLOUD_PAIR_STATE::CLOUD_PAIR_PAIRED);
        return;
    }

    // 配对失败
    if (preStatus == static_cast<int32_t>(SlePairState::SLE_PAIR_PAIRING) &&
        status == static_cast<int32_t>(SlePairState::SLE_PAIR_NONE) &&
        curCloudPairState == NL_CLOUD_PAIR_STATE::CLOUD_PAIR_CREATE_PAIR) {
        UpdateCloudState(addr, NL_CLOUD_PAIR_STATE::CLOUD_PAIR_NONE);
        return;
    }

    // 取消配对
    if (preStatus == static_cast<int32_t>(SlePairState::SLE_PAIR_CANCELING) &&
        status == static_cast<int32_t>(SlePairState::SLE_PAIR_NONE) &&
        curCloudPairState == NL_CLOUD_PAIR_STATE::CLOUD_PAIR_REMOVING) {
        UpdateCloudState(addr, NL_CLOUD_PAIR_STATE::CLOUD_PAIR_NONE);
        return;
    }
}

void SleCloudPairService::HandleAcbStateChanged(const RawAddress &device, int32_t state, int reason)
{
    if (!IsCloudDevice(device)) {
        HILOGI("[CLOUD PAIR] %{public}s is not cloudDevice", GET_ENCRYPT_ADDR(device));
        return;
    }
    RawAddress reportAddr = GetReportAddr(device);
    int32_t curCloudPairState = NL_CLOUD_PAIR_STATE::CLOUD_PAIR_INVALID;
    GetCloudPairState(reportAddr.GetAddress(), curCloudPairState);
    HILOGI("[CLOUD PAIR] device %{public}s acb state: %{public}d, reason: 0x%{public}x, cloudPairState : %{public}d",
        GET_ENCRYPT_ADDR(device), state, reason, curCloudPairState);
    if (curCloudPairState != NL_CLOUD_PAIR_STATE::CLOUD_PAIR_PAIRING &&
        curCloudPairState != NL_CLOUD_PAIR_STATE::CLOUD_PAIR_TOKEN_CHANGING) {
        return;
    }

    if (state == static_cast<int32_t>(SleConnState::SLE_CONNECTION_STATE_CONNECTED)) {
        SetConnectedState(device, true);
        StartTokenChkTimer(reportAddr.GetAddress());
        HandleAcbConnectedTask(device, curCloudPairState);
    } else if (state == static_cast<int32_t>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED)) {
        SetConnectedState(device, false);
        if (!IsAllMembersDisconnected(device)) {
            return;
        }
        HandleAcbDisconnectedTask(reportAddr, curCloudPairState);
    }
}

void SleCloudPairService::HandleAcbConnectedTask(const RawAddress &device, int32_t curCloudPairState)
{
    if (curCloudPairState != NL_CLOUD_PAIR_STATE::CLOUD_PAIR_PAIRING) {
        return;
    }
    CdsmService *cdsmService = CdsmService::GetService();
    NL_CHECK_RETURN(cdsmService, "cdsmService is nullptr.");
    RawAddress otherDev;
    if (cdsmService->CdsmGetOtherAddr(device, otherDev)) {
        SleRemoteDeviceAdapter::GetInstance()->DisconnectAcbAction(otherDev,
            static_cast<uint8_t>(SleDiscReason::SLE_DISC_REASON_REMOTE_USER_TERMINATED));
    }
}

void SleCloudPairService::HandleAcbDisconnectedTask(const RawAddress &reportAddr, int32_t curCloudPairState)
{
    auto sleAdapter = (SleInterfaceAdapter *)(SleInterfaceManager::GetInstance()->GetAdapter(ADAPTER_SLE));
    // 上层应用已发起重配对，配对失败，PAIRING -> NONE 删配对记录
    if (curCloudPairState == NL_CLOUD_PAIR_STATE::CLOUD_PAIR_PAIRING && IsInRepairing(reportAddr)) {
        HILOGI("[CLOUD PAIR] reportAddr : %{public}s Repair Fail", GET_ENCRYPT_ADDR(reportAddr));
        UpdateCloudState(reportAddr.GetAddress(), NL_CLOUD_PAIR_STATE::CLOUD_PAIR_NONE);
        needRepairDevices_.Erase(reportAddr.GetAddress());
        NL_CHECK_RETURN(sleAdapter, "sleAdapter is null");
        RmvSpecificCloudDevice(reportAddr);
        sleAdapter->RemoveNotPairedCloudDevice(reportAddr);
        return;
    }
    // 首次下云连接失败，PAIRING -> NONE 保留配对记录，等待下次云配
    if (curCloudPairState == NL_CLOUD_PAIR_STATE::CLOUD_PAIR_PAIRING && !IsInRepairing(reportAddr)) {
        UpdateCloudState(reportAddr.GetAddress(), NL_CLOUD_PAIR_STATE::CLOUD_PAIR_NONE);
        NL_CHECK_RETURN(sleAdapter, "sleAdapter is null");
        SleConnectionChangedParam connChangedParam(static_cast<int>(SleConnectState::DISCONNECTED),
            static_cast<int>(SleConnectState::CONNECTING));
        sleAdapter->NotifyConnectionStateChanged(reportAddr, connChangedParam);
        return;
    }
    // keyMissing触发双耳acb已断，TOKEN_CHANGING -> NONE 通知上层应用发起重配对
    if (curCloudPairState == NL_CLOUD_PAIR_STATE::CLOUD_PAIR_TOKEN_CHANGING && IsInRepairing(reportAddr)) {
        UpdateCloudState(reportAddr.GetAddress(), NL_CLOUD_PAIR_STATE::CLOUD_PAIR_NONE);
        EnableVirtualAutoSwitch(reportAddr);
        NearlinkHelper::NearlinkCommonEventHelper::PublishRemovePairEvent(reportAddr.GetAddress(), true);
        return;
    }
}

std::string SleCloudPairService::GetBtAddrByReportAddr(std::string sleReportAddr)
{
    std::string btAddr = "";
    auto func = [&btAddr](std::string key, std::shared_ptr<DownCloudPairDevice> value) -> void {
        btAddr = value->GetBtAddr();
        HILOGD("[CLOUD PAIR] GetBtAddrByReportAddr btAddr %{public}s by sleAddr %{public}s",
            GetEncryptAddr(btAddr).c_str(), GetEncryptAddr(key).c_str());
    };
    cloudDevicesMap_.GetValueAndOpt(sleReportAddr, func);
    return btAddr;
}

std::string SleCloudPairService::GetReportAddrByBtAddr(std::string btAddr)
{
    std::string sleReportAddr = "";
    auto func = [&sleReportAddr, &btAddr](std::string key, std::shared_ptr<DownCloudPairDevice> value) -> bool {
        if (value != nullptr && value->GetBtAddr() == btAddr) {
            sleReportAddr = key;
            HILOGD("[CLOUD PAIR] GetReportAddrByBtAddr sleAddr %{public}s by btAddr %{public}s",
                GetEncryptAddr(sleReportAddr).c_str(), GetEncryptAddr(btAddr).c_str());
            return true;
        }
        return false;
    };
    cloudDevicesMap_.Find(func);
    HILOGD("[CLOUD PAIR]get sleAddr %{public}s by btAddr %{public}s", GetEncryptAddr(sleReportAddr).c_str(),
        GetEncryptAddr(btAddr).c_str());
    return sleReportAddr;
}

void SleCloudPairService::InitTokenChkTimer(std::string reportAddr)
{
    auto timeoutFunc = [this, reportAddr]() -> void {
        int32_t curCloudPairState = NL_CLOUD_PAIR_STATE::CLOUD_PAIR_INVALID;
        GetCloudPairState(reportAddr, curCloudPairState);
        NL_CHECK_RETURN(curCloudPairState == NL_CLOUD_PAIR_STATE::CLOUD_PAIR_PAIRING,
            "[CLOUD PAIR] reportAddr : %{public}s skip for not in pairing.", GetEncryptAddr(reportAddr).c_str());

        auto *sleAdapter = (SleInterfaceAdapter *)(SleInterfaceManager::GetInstance()->GetAdapter(ADAPTER_SLE));
        NL_CHECK_RETURN(sleAdapter, "sleAdapter is null");

        RawAddress reportDev(reportAddr);
        RawAddress memberDev(GetCollabAddrByReportAddr(reportDev));
        sleAdapter->DisconnectAction(reportDev,
            static_cast<uint8_t>(SleDiscReason::SLE_DISC_REASON_REMOTE_USER_TERMINATED));
        sleAdapter->DisconnectAction(memberDev,
            static_cast<uint8_t>(SleDiscReason::SLE_DISC_REASON_REMOTE_USER_TERMINATED));
        HILOGI("[CLOUD PAIR] reportAddr %{public}s, memberAddr %{public}s token check timeout, disconnect acb",
            GET_ENCRYPT_ADDR(reportDev), GET_ENCRYPT_ADDR(memberDev));
        NearlinkDftUe::GetInstance().WriteAudioSourceDeviceUe(reportDev, reportDev, CLOUD_PAIR, TOKENT_CHEAK_TIMEOUT);
    };

    std::shared_ptr<NearlinkTimer> tokenChkTimer = std::make_shared<NearlinkTimer>(timeoutFunc);
    tokenChkTimersMap_.EnsureInsert(reportAddr, tokenChkTimer);
}

void SleCloudPairService::StartTokenChkTimer(std::string reportAddr)
{
    auto func = [](std::string key, std::shared_ptr<NearlinkTimer> value) -> void {
        value->Start(TOKEN_CHECK_TIMEOUT_MS);
    };
    if (tokenChkTimersMap_.GetValueAndOpt(reportAddr, func)) {
        HILOGI("[CLOUD PAIR] addr : %{public}s wait for calling StartCrediblePair or pair success, start timer",
            GetEncryptAddr(reportAddr).c_str());
    }
}

void SleCloudPairService::StopTokenChkTimer(std::string reportAddr)
{
    auto func = [](std::string key, std::shared_ptr<NearlinkTimer> value) -> void {
        value->Stop();
    };
    if (tokenChkTimersMap_.GetValueAndOpt(reportAddr, func)) {
        HILOGI("[CLOUD PAIR] addr : %{public}s has called StartCrediblePair or pair success, stop timer",
            GetEncryptAddr(reportAddr).c_str());
    }
}

void SleCloudPairService::UpdateCloudDev(NearlinkCloudPairDevice &dev,
    std::shared_ptr<DownCloudPairDevice> downDevice)
{
    std::string reportAddr = dev.GetReportAddr();

    downDevice->SetBtAddr(dev.GetBtAddr());
    downDevice->SetDeviceName(dev.GetDeviceName());
    downDevice->SetToken(dev.GetToken());
    downDevice->SetReportAddr(dev.GetReportAddr());
    downDevice->SetMembersAddr(dev.GetMembersAddr());
    downDevice->SetModel(dev.GetModel());
    downDevice->SetSubModelId(dev.GetSubModelId());
    downDevice->SetDeviceIconId(dev.GetDeviceIconId());

    SleConfig::GetInstance().SetCloudDeviceBtAddr(reportAddr, dev.GetBtAddr());
    SleConfig::GetInstance().SetCloudDeviceName(reportAddr, dev.GetDeviceName());
    SleConfig::GetInstance().SetCloudDeviceToken(reportAddr, dev.GetToken());
    SleConfig::GetInstance().SetCloudDeviceReportAddr(reportAddr, dev.GetReportAddr());
    SleConfig::GetInstance().SetCloudDeviceMembersAddrList(reportAddr, dev.GetMembersAddr());
    SleConfig::GetInstance().SetCloudDeviceModel(reportAddr, dev.GetModel());
    SleConfig::GetInstance().SetCloudDeviceSubModelId(reportAddr, dev.GetSubModelId());
    SleConfig::GetInstance().SetCloudDeviceIconId(reportAddr, dev.GetDeviceIconId());

    std::vector<std::string> membersList = dev.GetMembersAddr();
    SleConfig::GetInstance().Save();
}

void SleCloudPairService::UpdateCloudState(std::string address, int32_t cloudPairState)
{
    auto func = [&cloudPairState](std::string key, std::shared_ptr<DownCloudPairDevice> value) -> void {
        value->SetCloudPairState(cloudPairState);
        SleConfig::GetInstance().SetCloudDeviceState(value->GetReportAddr(), cloudPairState);
        SleConfig::GetInstance().Save();
    };
    cloudDevicesMap_.GetValueAndOpt(address, func);
}

void SleCloudPairService::ClearCloudDeviceMap(bool isNeedClearConfig)
{
    cloudDevicesMap_.Clear();
    if (isNeedClearConfig) {
        SleConfig::GetInstance().RemoveAllCloudDevice();
    }
}

void SleCloudPairService::DelCloudDevFromMap(std::vector<NearlinkCloudPairDevice> &cloudDeviceInfos)
{
    std::map<std::string, int32_t> cnt;
    auto func = [&cnt](std::string key, std::shared_ptr<DownCloudPairDevice> value) -> void {
        cnt[key]++;
    };
    cloudDevicesMap_.Iterate(func);
    for (auto &devInfo : cloudDeviceInfos) {
        cnt[devInfo.GetReportAddr()]--;
    }

    auto sleAdapter = (SleInterfaceAdapter *)(SleInterfaceManager::GetInstance()->GetAdapter(ADAPTER_SLE));
    NL_CHECK_RETURN(sleAdapter, "sleAdapter is null");

    for (auto &it : cnt) {
        if (it.second <= 0) {
            continue;
        }
        std::string delReportAddr = it.first;
        RawAddress delDev(delReportAddr);
        // 已在本地真实配对/配对中的设备不删
        int pairState = SleRemoteDeviceAdapter::GetInstance()->GetPairState(delDev);
        if (pairState != static_cast<int>(SlePairState::SLE_PAIR_PAIRING) &&
            pairState != static_cast<int>(SlePairState::SLE_PAIR_PAIRED)) {
            sleAdapter->RemovePair(delDev);
            HILOGI("[CLOUD PAIR] auto remove device %{public}s pairState : %{public}d",
                GET_ENCRYPT_ADDR(delDev), pairState);
            NearlinkDftUe::GetInstance().WriteAudioSourceDeviceUe(delDev, delDev, CLOUD_PAIR, DELETE_DEVICE);
        }
    }
}

bool SleCloudPairService::IsCloudDevice(const RawAddress &device)
{
    RawAddress reportAddr = GetReportAddr(device);
    bool isMatch = cloudDevicesMap_.FindIf(reportAddr.GetAddress());
    return isMatch;
}

void SleCloudPairService::HandleTokenChanged(NearlinkCloudPairDevice &devInfo, const std::vector<uint8_t> &oldToken)
{
    if (oldToken.empty() || devInfo.GetToken() == oldToken) {
        return;
    }
    RawAddress device(devInfo.GetReportAddr());
    HILOGI("[CLOUD PAIR] reportAddr : %{public}s reset and token has changed", GET_ENCRYPT_ADDR(device));
    auto sleAdapter = (SleInterfaceAdapter *)(SleInterfaceManager::GetInstance()->GetAdapter(ADAPTER_SLE));
    NL_CHECK_RETURN(sleAdapter, "sleAdapter is null");
    int pairState = sleAdapter->GetPairState(device);
    NearlinkDftUe::GetInstance().WriteAudioSourceDeviceUe(device, device, CLOUD_PAIR, UPDATE_TOKEN);
    if (pairState != static_cast<int>(SlePairState::SLE_PAIR_PAIRED)) {
        return;
    }

    int32_t curCloudPairState = NL_CLOUD_PAIR_STATE::CLOUD_PAIR_INVALID;
    GetCloudPairState(devInfo.GetReportAddr(), curCloudPairState);
    if (curCloudPairState == NL_CLOUD_PAIR_STATE::CLOUD_PAIR_TOKEN_CHANGING && sleAdapter->IsAcbEncrypted(device)) {
        UpdateCloudState(devInfo.GetReportAddr(), NL_CLOUD_PAIR_STATE::CLOUD_PAIR_PAIRED);
    } else if (curCloudPairState == NL_CLOUD_PAIR_STATE::CLOUD_PAIR_PAIRED && !sleAdapter->IsAcbEncrypted(device)) {
        UpdateCloudState(devInfo.GetReportAddr(), NL_CLOUD_PAIR_STATE::CLOUD_PAIR_REMOVING);
        sleAdapter->CancelPairing(device);
    }
}

void SleCloudPairService::HandleDeviceNameChanged(NearlinkCloudPairDevice &devInfo, const std::string &oldName)
{
    if (oldName == devInfo.GetDeviceName()) {
        return;
    }
    RawAddress device(devInfo.GetReportAddr());
    HILOGI("[CLOUD PAIR] device %{public}s name changed.", GET_ENCRYPT_ADDR(device));
    auto sleAdapter = (SleInterfaceAdapter *)(SleInterfaceManager::GetInstance()->GetAdapter(ADAPTER_SLE));
    NL_CHECK_RETURN(sleAdapter, "sleAdapter is null");
    int pairState = sleAdapter->GetPairState(device);
    if (pairState == static_cast<int>(SlePairState::SLE_PAIR_PAIRED)) {
        sleAdapter->SetAliasName(device, devInfo.GetDeviceName());
    } else {
        NearlinkHelper::NearlinkCommonEventHelper::PublishRemoteNameChangedEvent(devInfo.GetReportAddr(),
            devInfo.GetDeviceName());
    }
    NearlinkDftUe::GetInstance().WriteAudioSourceDeviceUe(device, device, CLOUD_PAIR, UPDATE_NAME);
}

bool SleCloudPairService::UpdateCloudDeviceInfoList(std::vector<NearlinkCloudPairDevice> &cloudDeviceInfos)
{
    NL_CHECK_RETURN_RET(IsValidDownCloudDeviceList(cloudDeviceInfos), false, "Invalid down cloud device list");
    HILOGI("[CLOUD PAIR] downcloud list size : %{public}d, cur local cache size : %{public}d",
        static_cast<int>(cloudDeviceInfos.size()), cloudDevicesMap_.Size());

    auto sleAdapter = (SleInterfaceAdapter *)(SleInterfaceManager::GetInstance()->GetAdapter(ADAPTER_SLE));
    NL_CHECK_RETURN_RET(sleAdapter, false, "sleAdapter is null");

    for (auto &devInfo : cloudDeviceInfos) {
        RawAddress downAddr(devInfo.GetReportAddr());
        if (IsCloudDevice(downAddr)) {
            ReplaceOldCloudDevice(devInfo);
            RefreshCloudDevice(devInfo);
        } else {
            AddCloudDevice(devInfo);
        }
    }
    DelCloudDevFromMap(cloudDeviceInfos);
    return true;
}

void SleCloudPairService::RefreshCloudDevice(NearlinkCloudPairDevice devInfo)
{
    // 更改已有记录
    std::string downCloudDevReportAddr = devInfo.GetReportAddr();
    std::string oldName = "";
    std::vector<uint8_t> oldToken = {};
    auto getInfo = [this, &oldName, &oldToken, &devInfo]
        (std::string key, std::shared_ptr<DownCloudPairDevice> value) -> void {
        oldName = value->GetDeviceName();
        oldToken = value->GetToken();
        UpdateCloudDev(devInfo, value);
    };
    if (cloudDevicesMap_.GetValueAndOpt(downCloudDevReportAddr, getInfo)) {
        HandleTokenChanged(devInfo, oldToken);
        HandleDeviceNameChanged(devInfo, oldName);
    }
}

void SleCloudPairService::AddCloudDevice(NearlinkCloudPairDevice devInfo)
{
    // 新增记录
    auto sleAdapter = (SleInterfaceAdapter *)(SleInterfaceManager::GetInstance()->GetAdapter(ADAPTER_SLE));
    NL_CHECK_RETURN(sleAdapter, "sleAdapter is null");
    std::string downCloudDevReportAddr = devInfo.GetReportAddr();
    std::shared_ptr<DownCloudPairDevice> downDevice = std::make_shared<DownCloudPairDevice>();
    RawAddress newDevice(downCloudDevReportAddr);
    UpdateCloudDev(devInfo, downDevice);
    int pairState = sleAdapter->GetPairState(newDevice);
    cloudDevicesMap_.EnsureInsert(downCloudDevReportAddr, downDevice);
    InitTokenChkTimer(downCloudDevReportAddr);
    HILOGI("[CLOUD PAIR] add device, reportAddr : %{public}s, pairState : %{public}d",
        GetEncryptAddr(downCloudDevReportAddr).c_str(), pairState);
    NearlinkDftUe::GetInstance().WriteAudioSourceDeviceUe(newDevice, newDevice, CLOUD_PAIR, ADD_DEVICE);
    if (pairState == static_cast<int>(SlePairState::SLE_PAIR_PAIRED)) {
        UpdateCloudState(downCloudDevReportAddr, NL_CLOUD_PAIR_STATE::CLOUD_PAIR_PAIRED);
    } else {
        UpdateCloudState(downCloudDevReportAddr, NL_CLOUD_PAIR_STATE::CLOUD_PAIR_NONE);
        sleAdapter->NotifyPairStatusChanged(newDevice, static_cast<int32_t>(SlePairState::SLE_PAIR_NONE),
            static_cast<int32_t>(SlePairState::SLE_PAIR_PAIRED),
            static_cast<uint8_t>(PairingStateChangeReason::PAIRING_SUCCESS));
        EnableVirtualAutoSwitch(newDevice);
    }
    ProcCreateCloudDeviceCdsmGroup(newDevice);
}

void SleCloudPairService::SetNewCloudDevice(std::shared_ptr<DownCloudPairDevice> &oldDev,
    std::shared_ptr<DownCloudPairDevice> &newDev)
{
    newDev->SetDeviceName(oldDev->GetDeviceName());
    newDev->SetToken(oldDev->GetToken());
    newDev->SetModel(oldDev->GetModel());
    newDev->SetSubModelId(oldDev->GetSubModelId());
    newDev->SetDeviceIconId(oldDev->GetDeviceIconId());
    newDev->SetPrimary(oldDev->GetPrimary());
    newDev->SetConnectedMaps(oldDev->GetConnectedMaps());
}

void SleCloudPairService::ReplaceOldCloudDevice(NearlinkCloudPairDevice devInfo)
{
    RawAddress newReportAddr(devInfo.GetReportAddr());
    RawAddress oldReportAddr = GetReportAddr(newReportAddr);
    if (newReportAddr == oldReportAddr) {
        return;
    }
    // 同一副耳机重新恢厂后report地址发生变化
    std::shared_ptr<DownCloudPairDevice> newDev = std::make_shared<DownCloudPairDevice>();
    newDev->SetBtAddr(devInfo.GetBtAddr());
    newDev->SetReportAddr(devInfo.GetReportAddr());
    newDev->SetMembersAddr(devInfo.GetMembersAddr());
    int32_t oldCloudPairState = NL_CLOUD_PAIR_STATE::CLOUD_PAIR_INVALID;
    auto cloudPairInfo = [&newDev, &oldCloudPairState, this]
        (const std::string key, std::shared_ptr<DownCloudPairDevice> &value) -> void {
        SetNewCloudDevice(value, newDev);
        oldCloudPairState = value->GetCloudPairState();
    };
    auto sleAdapter = (SleInterfaceAdapter *)(SleInterfaceManager::GetInstance()->GetAdapter(ADAPTER_SLE));
    NL_CHECK_RETURN(sleAdapter, "sleAdapter is null");

    RawAddress oldCdsmReportAddr;
    CdsmService *cdsmService = CdsmService::GetService();
    NL_CHECK_RETURN(cdsmService, "cdsmService is nullptr.");
    bool isNeedReplaceCdsm = cdsmService->CdsmGetReportAddr(newReportAddr, oldCdsmReportAddr) &&
        oldCdsmReportAddr != newReportAddr && oldCdsmReportAddr == oldReportAddr;
    HILOGI("[CLOUD PAIR] cloud device reportAddr exchanged : %{public}s -> %{public}s, isNeedReplaceCdsm=%{public}d",
        GET_ENCRYPT_ADDR(oldReportAddr), GET_ENCRYPT_ADDR(newReportAddr), isNeedReplaceCdsm);

    cloudDevicesMap_.GetValueAndOpt(oldReportAddr.GetAddress(), cloudPairInfo);

    RmvSpecificCloudDevice(oldReportAddr);
    if (isNeedReplaceCdsm) {
        replacedDevices_.Insert(oldReportAddr.GetAddress());
        sleAdapter->NotifyPairStatusChanged(oldReportAddr, static_cast<int32_t>(SlePairState::SLE_PAIR_PAIRED),
            static_cast<int32_t>(SlePairState::SLE_PAIR_NONE),
            static_cast<uint8_t>(PairingStateChangeReason::PAIRING_LOCAL_CANCELED));
    }

    cloudDevicesMap_.EnsureInsert(newReportAddr.GetAddress(), newDev);
    UpdateCloudState(newReportAddr.GetAddress(), oldCloudPairState);
    InitTokenChkTimer(newReportAddr.GetAddress());
    EnableVirtualAutoSwitch(newReportAddr);
    if (IsInRepairing(oldReportAddr)) {
        needRepairDevices_.Erase(oldReportAddr.GetAddress());
        needRepairDevices_.Insert(newReportAddr.GetAddress());
    }
    if (isNeedReplaceCdsm) {
        cdsmService->CdsmReplaceOldReportAddr(oldReportAddr, newReportAddr);
        sleAdapter->NotifyPairStatusChanged(newReportAddr, static_cast<int32_t>(SlePairState::SLE_PAIR_NONE),
            static_cast<int32_t>(SlePairState::SLE_PAIR_PAIRED),
            static_cast<uint8_t>(PairingStateChangeReason::PAIRING_SUCCESS));
    }
    NearlinkDftUe::GetInstance().WriteAudioSourceDeviceUe(oldReportAddr, newReportAddr,
        CLOUD_PAIR, REPLACE_OLD_DEVICE);
}

void SleCloudPairService::RmvSpecificCloudDevice(const RawAddress &reportAddr)
{
    if (IsInRepairing(reportAddr)) {
        HILOGI("[CLOUD PAIR] device %{public}s in repairing, ignore", GET_ENCRYPT_ADDR(reportAddr));
        return;
    }
    HILOGI("[CLOUD PAIR] remove device %{public}s", GET_ENCRYPT_ADDR(reportAddr));
    NearlinkHelper::NearlinkCommonEventHelper::PublishRemovePairEvent(reportAddr.GetAddress(), false);
    cloudDevicesMap_.Erase(reportAddr.GetAddress());
    tokenChkTimersMap_.Erase(reportAddr.GetAddress());
    SleConfig::GetInstance().RemoveSpecificCloudDevice(reportAddr.GetAddress());
    SleConfig::GetInstance().Save();
    NearlinkDftUe::GetInstance().WriteAudioSourceDeviceUe(reportAddr, reportAddr, CLOUD_PAIR, REMOVE_DEVICE);
}

void SleCloudPairService::SetCrediblePairState(const RawAddress &device)
{
    RawAddress reportAddr = GetReportAddr(device);
    HILOGI("[CLOUD PAIR] device %{public}s start credible pair", GET_ENCRYPT_ADDR(reportAddr));
    int32_t curCloudPairState = NL_CLOUD_PAIR_STATE::CLOUD_PAIR_INVALID;
    GetCloudPairState(reportAddr.GetAddress(), curCloudPairState);

    if (curCloudPairState == NL_CLOUD_PAIR_STATE::CLOUD_PAIR_PAIRING) {
        StopTokenChkTimer(reportAddr.GetAddress());
        UpdateCloudState(reportAddr.GetAddress(), NL_CLOUD_PAIR_STATE::CLOUD_PAIR_CREATE_PAIR);
        needRepairDevices_.Erase(reportAddr.GetAddress());
    }
}

void SleCloudPairService::ProcCreateCloudDeviceCdsmGroup(const RawAddress &reportAddr)
{
    std::string collaAddr = GetCollabAddrByReportAddr(reportAddr);
    CdsmService *cdsmService = CdsmService::GetService();
    NL_CHECK_RETURN(cdsmService, "cdsmService is nullptr.");
    std::vector<RawAddress> devList;
    devList.push_back(reportAddr);
    devList.push_back(RawAddress(collaAddr));
    cdsmService->CdsmCreateGroup(reportAddr, devList, true);

    std::vector<std::string> cdsmDevList = {reportAddr.GetAddress(), collaAddr};
    SleConfig::GetInstance().SetCdsmIsPrivateDevice(reportAddr.GetAddress(), static_cast<int>(true));
    SleConfig::GetInstance().SetCdsmMemberList(reportAddr.GetAddress(), cdsmDevList);
    SleConfig::GetInstance().Save();
}

bool SleCloudPairService::GetCloudPairState(const std::string &address, int32_t &cloudPairState)
{
    RawAddress reportAddr = GetReportAddr(RawAddress(address));
    return cloudDevicesMap_.GetValueAndOpt(reportAddr.GetAddress(), [&cloudPairState](
            std::string key, std::shared_ptr<DownCloudPairDevice> value) -> void {
            cloudPairState = value->GetCloudPairState();
        });
}

bool SleCloudPairService::SetConnectedState(const RawAddress &device, bool isConnected)
{
    RawAddress reportAddr = GetReportAddr(device);
    return cloudDevicesMap_.GetValueAndOpt(reportAddr.GetAddress(), [&device, &isConnected](
        std::string key, std::shared_ptr<DownCloudPairDevice> value) -> void {
        value->SetConnectedState(device.GetAddress(), isConnected);
    });
}

bool SleCloudPairService::IsAllMembersDisconnected(const RawAddress &device)
{
    bool ret = true;
    RawAddress reportAddr = GetReportAddr(device);
    cloudDevicesMap_.GetValueAndOpt(reportAddr.GetAddress(), [&ret](
        std::string key, std::shared_ptr<DownCloudPairDevice> value) -> void {
        ret = value->IsAllMembersDisconnected();
    });
    return ret;
}

void SleCloudPairService::GetAllNotPairedCloudDeviceList(std::vector<std::string> &cloudDeviceAddrList)
{
    cloudDeviceAddrList.clear();
    auto GetDevice = [&cloudDeviceAddrList] (std::string key, std::shared_ptr<DownCloudPairDevice> value) -> void {
        if (value->GetCloudPairState() != NL_CLOUD_PAIR_STATE::CLOUD_PAIR_PAIRED) {
            cloudDeviceAddrList.push_back(value->GetReportAddr());
        }
    };
    cloudDevicesMap_.Iterate(GetDevice);
    HILOGD("[CLOUD PAIR] not paired cloud device num : %{public}d", static_cast<int32_t>(cloudDeviceAddrList.size()));
}

void SleCloudPairService::ReadCloudDeviceInfoFromConf(const std::vector<std::string> &cloudDeviceList)
{
    HILOGI("[CLOUD PAIR] enter, xml cloud pair device num : %{public}d", static_cast<int32_t>(cloudDeviceList.size()));
    for (auto addr : cloudDeviceList) {
        RawAddress rawAddr(addr);
        if ((!INVALID_MAC_ADDRESS.compare(rawAddr.GetAddress())) || (rawAddr.GetAddress().empty())) {
            continue;
        }
        std::vector<uint8_t> token(TOKEN_LEN, 0);
        if (!SleConfig::GetInstance().GetCloudDeviceToken(addr, token)) {
            continue;
        }
        std::shared_ptr<DownCloudPairDevice> downDevice = std::make_shared<DownCloudPairDevice>();
        downDevice->SetBtAddr(SleConfig::GetInstance().GetCloudDeviceBtAddr(addr));
        downDevice->SetDeviceName(SleConfig::GetInstance().GetCloudDeviceName(addr));
        downDevice->SetToken(token);
        downDevice->SetReportAddr(SleConfig::GetInstance().GetCloudDeviceReportAddr(addr));
        downDevice->SetModel(SleConfig::GetInstance().GetCloudDeviceModel(addr));
        downDevice->SetSubModelId(SleConfig::GetInstance().GetCloudDeviceSubModelId(addr));
        downDevice->SetDeviceIconId(SleConfig::GetInstance().GetCloudDeviceIconId(addr));

        std::vector<std::string> membersAddrList;
        SleConfig::GetInstance().GetCloudDeviceMembersAddrList(addr, membersAddrList);
        downDevice->SetMembersAddr(membersAddrList);

        if (SleConfig::GetInstance().GetCloudDeviceState(addr) == NL_CLOUD_PAIR_STATE::CLOUD_PAIR_PAIRED) {
            downDevice->SetCloudPairState(NL_CLOUD_PAIR_STATE::CLOUD_PAIR_PAIRED);
        } else {
            SleConfig::GetInstance().SetCloudDeviceState(addr, NL_CLOUD_PAIR_STATE::CLOUD_PAIR_NONE);
            downDevice->SetCloudPairState(NL_CLOUD_PAIR_STATE::CLOUD_PAIR_NONE);
        }
        cloudDevicesMap_.EnsureInsert(addr, downDevice);
        InitTokenChkTimer(addr);
        EnableVirtualAutoSwitch(rawAddr);
    }
}

std::string SleCloudPairService::GetCollabAddrByReportAddr(const RawAddress &reportAddr) {
    std::string collabAddr = INVALID_MAC_ADDRESS;
    auto GetCollabAddr = [&collabAddr] (std::string key, std::shared_ptr<DownCloudPairDevice> value) -> void {
        std::vector<std::string> membersList = value->GetMembersAddr();
        for (auto &dev : membersList) {
            if (dev != key) {
                collabAddr = dev;
                HILOGI("[CLOUD PAIR] reportAddr : %{public}s, collaAddr : %{public}s", GetEncryptAddr(key).c_str(),
                    GetEncryptAddr(collabAddr).c_str());
                return;
            }
        }
    };
    cloudDevicesMap_.GetValueAndOpt(reportAddr.GetAddress(), GetCollabAddr);
    return collabAddr;
}

void SleCloudPairService::EnableVirtualAutoSwitch(const RawAddress &reportAddr)
{
    cloudDevicesMap_.GetValueAndOpt(reportAddr.GetAddress(),
        [](std::string key, std::shared_ptr<DownCloudPairDevice> value) -> void {
        TwsService *twsService = TwsService::GetService();
        NL_CHECK_RETURN(twsService, "twsService is null");
        std::vector<std::string> membersAddr = value->GetMembersAddr();
        for (auto member : membersAddr) {
            TwsClientData virtualData;
            virtualData.peerAddr_ = RawAddress(member);
            virtualData.autoConnSwitch_ = static_cast<uint8_t>(TwsAutoConnSwitch::ON);
            twsService->UpdateClientData(static_cast<uint8_t>(TwsHiBoxMsgType::HIBOX_AUTO_CONN_SWITCH), virtualData);
            HILOGI("[CLOUD PAIR] device : %{public}s enable auto switch", GetEncryptAddr(member).c_str());
        }
    });
}

RawAddress SleCloudPairService::GetReportAddr(const RawAddress& device)
{
    RawAddress reportAddr(device);
    auto getReportAddr = [&reportAddr, &device] (std::string key, std::shared_ptr<DownCloudPairDevice> value) -> bool {
        std::vector<std::string> memberList = value->GetMembersAddr();
        for (auto &member : memberList) {
            if (member == device.GetAddress()) {
                reportAddr.SetAddress(key);
                HILOGD("[CLOUD PAIR] device : %{public}s,  reportAddr : %{public}s",
                    GET_ENCRYPT_ADDR(device), GET_ENCRYPT_ADDR(reportAddr));
                return true;
            }
        }
        return false;
    };
    cloudDevicesMap_.Find(getReportAddr);
    return reportAddr;
}

bool SleCloudPairService::IsCloudDeviceCreatePair(const RawAddress &device)
{
    int32_t curCloudPairState = NL_CLOUD_PAIR_STATE::CLOUD_PAIR_INVALID;
    GetCloudPairState(device.GetAddress(), curCloudPairState);
    if (curCloudPairState == NL_CLOUD_PAIR_STATE::CLOUD_PAIR_CREATE_PAIR) {
        return true;
    }
    return false;
}

RawAddress SleCloudPairService::GetCloudDeviceRealAddress(const RawAddress &device)
{
    RawAddress realAddr;
    RawAddress reportAddr = GetReportAddr(device);
    auto getRealAddress = [&realAddr] (std::string key, std::shared_ptr<DownCloudPairDevice> value) -> void {
        std::string primary = value->GetPrimary();
        HILOGI("[CLOUD PAIR] reportAddr : %{public}s, cloudPairState : %{public}d, primary : %{public}s",
            GetEncryptAddr(key).c_str(), static_cast<int>(value->GetCloudPairState()), GetEncryptAddr(primary).c_str());
        if (value->GetCloudPairState() == NL_CLOUD_PAIR_STATE::CLOUD_PAIR_CREATE_PAIR && !primary.empty()) {
            realAddr = RawAddress(primary);
        }
    };
    cloudDevicesMap_.GetValueAndOpt(reportAddr.GetAddress(), getRealAddress);
    HILOGI("[CLOUD PAIR] addr:%{public}s, realAddr:%{public}s", GET_ENCRYPT_ADDR(device), GET_ENCRYPT_ADDR(realAddr));
    return realAddr;
}

bool SleCloudPairService::CancelCloudPairing(const RawAddress &device)
{
    RawAddress reportAddr = GetReportAddr(device);
    int32_t curCloudPairState = NL_CLOUD_PAIR_STATE::CLOUD_PAIR_INVALID;
    GetCloudPairState(reportAddr.GetAddress(), curCloudPairState);
    // 无真实配对记录，仅删云配模块缓存记录
    if (curCloudPairState == NL_CLOUD_PAIR_STATE::CLOUD_PAIR_NONE ||
        curCloudPairState == NL_CLOUD_PAIR_STATE::CLOUD_PAIR_PAIRING ||
        curCloudPairState == NL_CLOUD_PAIR_STATE::CLOUD_PAIR_CREATE_PAIR) {
        HILOGI("[CLOUD PAIR] cancel cloud pair device : addr : %{public}s, cloudPairState : %{public}d",
            GET_ENCRYPT_ADDR(reportAddr), curCloudPairState);
        RmvSpecificCloudDevice(reportAddr);
        return true;
    }
    return false;
}

bool SleCloudPairService::CancelCloudPairComplete(const RawAddress &device, int preStatus, int reason,
    bool isCdsmAcbConnected, int acbState)
{
    RawAddress reportAddr = GetReportAddr(device);
    int32_t curCloudPairState = NL_CLOUD_PAIR_STATE::CLOUD_PAIR_INVALID;
    GetCloudPairState(reportAddr.GetAddress(), curCloudPairState);
    HILOGI("[CLOUD PAIR] device : %{public}s cloudPairState : %{public}d", GET_ENCRYPT_ADDR(device), curCloudPairState);
    // 场景1 : paired - 已真实配对，正常删配对记录，已从协议栈回调，跟着清除云配缓存。
    if (curCloudPairState == NL_CLOUD_PAIR_STATE::CLOUD_PAIR_PAIRED) {
        RmvSpecificCloudDevice(reportAddr);
    }
    // 场景2 : 下云识别到token变化，自动删配对记录，等待下一次云配流程。对上层应用不感知配对记录删除，提前return。
    if (curCloudPairState == NL_CLOUD_PAIR_STATE::CLOUD_PAIR_REMOVING ||
        curCloudPairState == NL_CLOUD_PAIR_STATE::CLOUD_PAIR_NONE) {
        UpdateCloudState(reportAddr.GetAddress(), NL_CLOUD_PAIR_STATE::CLOUD_PAIR_NONE);
        EnableVirtualAutoSwitch(reportAddr);
        return true;
    }
    // 场景3 : 耳机恢厂点连接，遇到keyMissing
    if (curCloudPairState == NL_CLOUD_PAIR_STATE::CLOUD_PAIR_TOKEN_CHANGING) {
        bool isNeedRepair = (preStatus == static_cast<int>(SlePairState::SLE_PAIR_PAIRING) ||
                             preStatus == static_cast<int>(SlePairState::SLE_PAIR_PAIRED)) &&
                             reason != static_cast<uint8_t>(PairingStateChangeReason::PAIRING_LOCAL_CANCELED) &&
                             isCdsmAcbConnected;
        if (isNeedRepair) {
            needRepairDevices_.Insert(reportAddr.GetAddress());
        }
        // 场景3.1 : 耳机不在配对模式，主耳已连接acb未加密，副耳acb未连接，保持云配记录，等待双耳acb断连
        if (IsPreparingRepair(device)) {
            HILOGI("[CLOUD PAIR] device : %{public}s need repair", GET_ENCRYPT_ADDR(reportAddr));
            ClearToken(reportAddr);
            if (acbState == static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED)) {
                SetConnectedState(device, true);
            }
            return true;
        // 场景3.2 : 耳机在配对模式，发起重配对请求，用户取消或无响应，同步删除云配记录
        } else {
            HILOGI("[CLOUD PAIR] device : %{public}s don't need repair, removePair", GET_ENCRYPT_ADDR(reportAddr));
            RmvSpecificCloudDevice(reportAddr);
        }
    }
    return false;
}

bool SleCloudPairService::IsPreparingRepair(const RawAddress &device)
{
    RawAddress reportAddr = GetReportAddr(device);
    int32_t curCloudPairState = NL_CLOUD_PAIR_STATE::CLOUD_PAIR_INVALID;
    GetCloudPairState(reportAddr.GetAddress(), curCloudPairState);
    return IsInRepairing(device) && curCloudPairState == NL_CLOUD_PAIR_STATE::CLOUD_PAIR_TOKEN_CHANGING;
}

bool SleCloudPairService::IsInRepairing(const RawAddress &device)
{
    RawAddress reportAddr = GetReportAddr(device);
    return needRepairDevices_.Find([&reportAddr] (std::string keyMissingDeviceAddress) -> bool {
        return reportAddr.GetAddress() == keyMissingDeviceAddress;
    });
}

bool SleCloudPairService::IsInReplacing(const RawAddress &reportAddr)
{
    return replacedDevices_.Find([&reportAddr] (std::string replacedDeviceAddress) -> bool {
        return reportAddr.GetAddress() == replacedDeviceAddress;
    });
}

bool SleCloudPairService::ClearToken(const RawAddress &device)
{
    RawAddress reportAddr = GetReportAddr(device);
    auto clearToken = [] (std::string key, std::shared_ptr<DownCloudPairDevice> value) -> void {
        std::vector<uint8_t> emptyToken = {};
        value->SetToken(emptyToken);
    };
    return cloudDevicesMap_.GetValueAndOpt(reportAddr.GetAddress(), clearToken);
}

bool SleCloudPairService::CloudDeviceConnectionComplete(const RawAddress &device)
{
    RawAddress reportAddr = GetReportAddr(device);
    int32_t curCloudPairState = NL_CLOUD_PAIR_STATE::CLOUD_PAIR_INVALID;
    GetCloudPairState(reportAddr.GetAddress(), curCloudPairState);
    // 耳机恢复出厂，token变化之后，启动第二次云配流程，等上层应用校验下发配对。此时耳机已丢失key，不去到协议栈走加密流程。
    if (curCloudPairState == NL_CLOUD_PAIR_STATE::CLOUD_PAIR_NONE ||
        curCloudPairState == NL_CLOUD_PAIR_STATE::CLOUD_PAIR_PAIRING) {
        HILOGI("[CLOUD PAIR] device %{public}s cloud pair state %{public}d, stop sleSecurity_.StartPair",
            GET_ENCRYPT_ADDR(reportAddr), curCloudPairState);
        return true;
    }
    return false;
}

bool SleCloudPairService::ConnectCloudDeviceAllProfile(const RawAddress &device)
{
    auto sleAdapter = (SleInterfaceAdapter *)(SleInterfaceManager::GetInstance()->GetAdapter(ADAPTER_SLE));
    NL_CHECK_RETURN_RET(sleAdapter, false, "sleAdapter is null");
    RawAddress reportAddr = GetReportAddr(device);
    int32_t curCloudPairState = NL_CLOUD_PAIR_STATE::CLOUD_PAIR_INVALID;
    GetCloudPairState(reportAddr.GetAddress(), curCloudPairState);
    // 主耳连接成功，连接副耳，不拦截
    if (curCloudPairState == NL_CLOUD_PAIR_STATE::CLOUD_PAIR_PAIRED) {
        return false;
    }
    // 设备下云后，先调StartPair，后调ConnectAllProfileTask，真实配对流程，不拦截
    if (curCloudPairState != NL_CLOUD_PAIR_STATE::CLOUD_PAIR_INVALID &&
        sleAdapter->GetPairState(device) == static_cast<int>(SlePairState::SLE_PAIR_PAIRED)) {
        HILOGI("[CLOUD PAIR] device : %{public}s is paired, continue real pairing", GET_ENCRYPT_ADDR(device));
        UpdateCloudState(reportAddr.GetAddress(), NL_CLOUD_PAIR_STATE::CLOUD_PAIR_PAIRED);
        return false;
    }
    // 设备下云后，调ConnectAllProfileTask，仅连ACB，拦截后等待上层应用调StartPair
    if (curCloudPairState == NL_CLOUD_PAIR_STATE::CLOUD_PAIR_NONE) {
        ProcCreateCloudDeviceCdsmGroup(reportAddr);
        UpdateCloudState(reportAddr.GetAddress(), NL_CLOUD_PAIR_STATE::CLOUD_PAIR_PAIRING);
        std::string collaAddr = GetCollabAddrByReportAddr(reportAddr);
        sleAdapter->ConnectAcb(reportAddr);
        sleAdapter->ConnectAcb(RawAddress(collaAddr));
        HILOGI("[CLOUD PAIR] reportAddr : %{public}s and collaAddr : %{public}s ConnectAcb",
            GET_ENCRYPT_ADDR(reportAddr), GetEncryptAddr(collaAddr).c_str());
        SleConnectionChangedParam connChangedParam(static_cast<int>(SleConnectState::CONNECTING),
            static_cast<int>(SleConnectState::DISCONNECTED), static_cast<int>(SleConnectReason::CONNECT_SUCCESS));
        sleAdapter->NotifyConnectionStateChanged(reportAddr, connChangedParam);
        return true;
    }
    return false;
}

void SleCloudPairService::SetKeyMissingPairState(const RawAddress &device)
{
    RawAddress reportAddr = GetReportAddr(device);
    int32_t curCloudPairState = NL_CLOUD_PAIR_STATE::CLOUD_PAIR_INVALID;
    GetCloudPairState(reportAddr.GetAddress(), curCloudPairState);
    if (curCloudPairState == NL_CLOUD_PAIR_STATE::CLOUD_PAIR_PAIRED) {
        UpdateCloudState(reportAddr.GetAddress(), NL_CLOUD_PAIR_STATE::CLOUD_PAIR_TOKEN_CHANGING);
        ProcCreateCloudDeviceCdsmGroup(reportAddr);
    }
}

std::string SleCloudPairService::GetCloudDeviceAliasName(const RawAddress &device)
{
    RawAddress reportAddr = GetReportAddr(device);
    std::string name = "";
    auto getAliasName = [&name] (std::string key, std::shared_ptr<DownCloudPairDevice> value) -> void {
        name = value->GetDeviceName();
        HILOGD("[CLOUD PAIR] reportAddr : %{public}s GetAliasName", GetEncryptAddr(key).c_str());
    };
    cloudDevicesMap_.GetValueAndOpt(reportAddr.GetAddress(), getAliasName);
    return name;
}

bool SleCloudPairService::SetCloudDeviceAliasName(const RawAddress &device, const std::string &name)
{
    RawAddress reportAddr = GetReportAddr(device);
    auto setAliasName = [&name] (std::string key, std::shared_ptr<DownCloudPairDevice> value) -> void {
        value->SetDeviceName(name);
    };
    return cloudDevicesMap_.GetValueAndOpt(reportAddr.GetAddress(), setAliasName);
}

std::string SleCloudPairService::GetCloudDeviceIcondId(const RawAddress &device)
{
    RawAddress reportAddr = GetReportAddr(device);
    std::string iconId = "";
    auto getIconId = [&iconId, this] (std::string key, std::shared_ptr<DownCloudPairDevice> value) -> void {
        if (ConvertDecStrToHexStr(value->GetDeviceIconId(), iconId)) {
            HILOGD("[CLOUD PAIR] reportAddr : %{public}s iconId : %{public}s",
                GetEncryptAddr(key).c_str(), iconId.c_str());
        }
    };
    cloudDevicesMap_.GetValueAndOpt(reportAddr.GetAddress(), getIconId);
    return iconId;
}

std::string SleCloudPairService::GetCloudDeviceSubModelId(const RawAddress &device)
{
    RawAddress reportAddr = GetReportAddr(device);
    std::string subModelId = "";
    auto getSubModelId = [&subModelId](std::string key, std::shared_ptr<DownCloudPairDevice> value) -> void {
        subModelId = value->GetSubModelId();
    };
    cloudDevicesMap_.GetValueAndOpt(reportAddr.GetAddress(), getSubModelId);
    return subModelId;
}

bool SleCloudPairService::IsCloudDeviceConnecting(const RawAddress &device)
{
    RawAddress reportAddr = GetReportAddr(device);
    int32_t curCloudPairState = NL_CLOUD_PAIR_STATE::CLOUD_PAIR_INVALID;
    GetCloudPairState(reportAddr.GetAddress(), curCloudPairState);
    if (curCloudPairState == NL_CLOUD_PAIR_STATE::CLOUD_PAIR_PAIRING ||
        curCloudPairState == NL_CLOUD_PAIR_STATE::CLOUD_PAIR_CREATE_PAIR) {
        return true;
    }
    return false;
}

void SleCloudPairService::AddCloudPairDevices(std::vector<RawAddress> &pairedList)
{
    std::vector<std::string> notPairedDevList = {};
    GetAllNotPairedCloudDeviceList(notPairedDevList);
    uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
    VerificationContext ctx = {.tokenId = tokenId};
    bool isVerificationAllowed = NearlinkVerificationManager::GetInstance().CheckVerification(
        VerificationType::CLOUD_PAIR_CHECK, ctx);
    if (isVerificationAllowed) {
        for (auto it = notPairedDevList.begin(); it != notPairedDevList.end(); ++it) {
            RawAddress addr(*it);
            pairedList.emplace_back(addr);
        }
    }
}

bool SleCloudPairService::ChkCloudDeviceAndPermission(const RawAddress &device)
{
    uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
    VerificationContext ctx = {.tokenId = tokenId};
    bool isVerificationAllowed = NearlinkVerificationManager::GetInstance().CheckVerification(
        VerificationType::CLOUD_PAIR_CHECK, ctx);
    if (IsCloudDevice(device) && isVerificationAllowed) {
        return true;
    }
    return false;
}

int SleCloudPairService::GetCloudDeviceManufacturerBusinessType(const RawAddress &device)
{
    return IsCloudDevice(device) ? Nearlink::SLE_PRIVATE_AUDIO_BUSINESS_TYPE : 0;
}

bool SleCloudPairService::IsValidDownCloudDeviceList(std::vector<NearlinkCloudPairDevice> &cloudDeviceInfos)
{
    for (auto &devInfo : cloudDeviceInfos) {
        NL_CHECK_RETURN_RET(IsValidAddress(devInfo.GetReportAddr()), false, "Invalid sle reportAddr");
        NL_CHECK_RETURN_RET(IsValidAddress(devInfo.GetBtAddr()), false, "Invalid bt addr");
        NL_CHECK_RETURN_RET(devInfo.GetToken().size() == TOKEN_LEN, false, "Invalid token length");
        NL_CHECK_RETURN_RET(!devInfo.GetDeviceName().empty(), false, "Empty alias name");
        NL_CHECK_RETURN_RET(devInfo.GetDeviceIconId().size() <= MAX_ICON_DEC_STR_LENGTH, false, "Invalid icon length");
        std::vector<std::string> memberList = devInfo.GetMembersAddr();
        NL_CHECK_RETURN_RET(memberList.size() == MEMBER_LIST_LENGTH, false, "Invalid memberList length : %{public}d",
            memberList.size());
        for (auto &member : memberList) {
            HILOGI("sle member addr : %{public}s", GetEncryptAddr(member).c_str());
            NL_CHECK_RETURN_RET(IsValidAddress(member) && INVALID_MAC_ADDRESS != member,
                false, "Invalid sle member addr");
        }
    }
    return true;
}

bool SleCloudPairService::ConvertDecStrToHexStr(const std::string &icon, std::string &iconHexStr)
{
    uint32_t iconCode = 0;
    auto [ptr, ec] = std::from_chars(icon.data(), icon.data() + icon.size(), iconCode);
    if (!(ec == std::errc{} && ptr == icon.data() + icon.size())) {
        HILOGE("[CLOUD PAIR]: Convert dec to hex failed");
        return false;
    }
    std::stringstream ss;
    ss << std::hex << std::uppercase << iconCode;
    iconHexStr = ss.str();
    return true;
}
//LCOV_EXCL_STOP
} // namespace Nearlink
} // namespace OHOS