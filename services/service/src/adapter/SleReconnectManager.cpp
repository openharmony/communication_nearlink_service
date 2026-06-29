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

#include "SleReconnectManager.h"
#include "SleConfig.h"
#include "SleServiceManager.h"
#include "SleInterfaceAdapterSub.h"
#include "CdsmService.h"
#include "cm_api.h"
#include "cm_def.h"
#include "cm_errno.h"
#include "log.h"
#include "SleUtils.h"
#include "parameters.h"
#include "nearlink_common_event_helper.h"
#include "SleRemoteDeviceAdapter.h"

namespace OHOS {
namespace Nearlink {
namespace {
constexpr const char* NEARLINK_RECONN_DEVICE_BT_ADDR = "persist.nearlink.reconn_device_bluetooth_address";
}
SleReconnectManager& SleReconnectManager::GetInstance()
{
    static SleReconnectManager instance;
    return instance;
}

void SleReconnectManager::SetBgConnMaxNum(uint32_t size)
{
    bgConnMaxNum_ = size;
    AdjustWindowForSpace(0);
    ExpandWindowLeft();
    HILOGI("Set bgConnMaxNum_ = %{public}d", size);
}

void SleReconnectManager::RemoveAutoConnAudioDevConf(const RawAddress& addr)
{
    CdsmService *cdsmService = CdsmService::GetService();
    NL_CHECK_RETURN(cdsmService, "cdsmService nullptr.");
    RawAddress reportAddr;
    RawAddress memberAddr;
    std::string lastASCActiveDevice = SleConfig::GetInstance().GetLastASCActiveDevice();
    std::string lastASCConnectedDevice = SleConfig::GetInstance().GetLastASCConnectedDevice();

    if (IsValidAddress(lastASCActiveDevice)) {
        memberAddr.SetAddress(lastASCActiveDevice);
        if (cdsmService->CdsmGetReportAddr(memberAddr, reportAddr) &&
            reportAddr == addr && SleConfig::GetInstance().RemoveLastASCActiveDevice()) {
            NearlinkHelper::NearlinkCommonEventHelper::PublishAutoReconnEvent(INVALID_MAC_ADDRESS, true);
        }
    }
    if (IsValidAddress(lastASCConnectedDevice)) {
        memberAddr.SetAddress(lastASCConnectedDevice);
        if (cdsmService->CdsmGetReportAddr(memberAddr, reportAddr) &&
            reportAddr == addr && SleConfig::GetInstance().RemoveLastASCConnectedDevice()) {
            NearlinkHelper::NearlinkCommonEventHelper::PublishAutoReconnEvent(INVALID_MAC_ADDRESS, false);
        }
    }
    if (cdsmService->CdsmGetReportAddr(addr, reportAddr)) {
        NearlinkHelper::NearlinkCommonEventHelper::PublishSleAddrToBtAddrMapEvent(reportAddr.GetAddress(),
            INVALID_MAC_ADDRESS);
        SetReconnDeviceParam();
    }
}
bool SleReconnectManager::IsNeedToIgnore(const std::string& reconnDevAddr)
{
    bool enableAutoConnAudioDevice = SleServiceManager::GetInstance()->IsEnableAutoConnectAudioDevices();
    if (!enableAutoConnAudioDevice) {
        return true;
    }
    std::string addr = GetReconnDevice();
    RawAddress reportAddr(addr);
    RawAddress memberAddr;

    CdsmService *cdsmService = CdsmService::GetService();
    if (cdsmService != nullptr && !reportAddr.GetAddress().empty()) {
        cdsmService->CdsmGetOtherAddr(reportAddr, memberAddr);
        if (reportAddr.GetAddress() != reconnDevAddr && memberAddr.GetAddress() != reconnDevAddr) {
            return true;
        }
    }
    return false;
}
std::string SleReconnectManager::GetReconnDevice()
{
    std::string lastASCActiveDevice = SleConfig::GetInstance().GetLastASCActiveDevice();
    std::string lastASCConnectedDevice = SleConfig::GetInstance().GetLastASCConnectedDevice();

    if (IsValidAddress(lastASCActiveDevice)) {
        return lastASCActiveDevice;
    } else if (IsValidAddress(lastASCConnectedDevice)) {
        return lastASCConnectedDevice;
    }
    return "";
}
void SleReconnectManager::SetReconnDeviceParam()
{
    std::string sleAddr = GetReconnDevice();
    if (sleAddr.empty()) {
        OHOS::system::SetParameter(NEARLINK_RECONN_DEVICE_BT_ADDR, INVALID_MAC_ADDRESS);
    } else {
        std::string btAddr = SleConfig::GetInstance().GetBtAddrBySleAddr(sleAddr);
        if (!btAddr.empty()) {
            OHOS::system::SetParameter(NEARLINK_RECONN_DEVICE_BT_ADDR, btAddr);
        }
    }
}
void SleReconnectManager::SetAutoConnectDevice(const RawAddress& reportAddr, bool isActive,
    int pairState, std::string btAddr)
{
    if (pairState != static_cast<int>(SlePairState::SLE_PAIR_NONE)) {
        if (!btAddr.empty()) {
            NearlinkHelper::NearlinkCommonEventHelper::PublishSleAddrToBtAddrMapEvent(reportAddr.GetAddress(), btAddr);
        }
        NearlinkHelper::NearlinkCommonEventHelper::PublishAutoReconnEvent(reportAddr.GetAddress(), isActive);

        if (isActive) {
            SleConfig::GetInstance().SetLastASCActiveDevice(reportAddr.GetAddress());
        } else {
            SleConfig::GetInstance().SetLastASCConnectedDevice(reportAddr.GetAddress());
        }
        SleConfig::GetInstance().Save();
        SetReconnDeviceParam();
    }
}

uint32_t SleReconnectManager::GetWindowDeviceCount() const
{
    uint32_t deviceCount = 0;
    if (windowStartIndex_ > windowEndIndex_ || windowEndIndex_ > pairedQueue_.size()) {
        return 0;
    }
    for (size_t i = windowStartIndex_; i < windowEndIndex_; ++i) {
        deviceCount += pairedQueue_[i].cdsmMemberCount_;
    }
    return deviceCount;
}

bool SleReconnectManager::IsDeviceIndexInWindow(size_t index) const
{
    return windowStartIndex_ <= windowEndIndex_ && windowEndIndex_ <= pairedQueue_.size() &&
        index >= windowStartIndex_ && index < windowEndIndex_;
}

size_t SleReconnectManager::FindDeviceIndex(const std::string &addr) const
{
    for (size_t i = 0; i < pairedQueue_.size(); ++i) {
        if (pairedQueue_[i].reportAddr_ == addr) {
            return i;
        }
    }
    return pairedQueue_.size();
}

void SleReconnectManager::AddDeviceToPairedQueue(const ReconnectDeviceInfo deviceInfo)
{
    uint32_t requiredSpace = deviceInfo.cdsmMemberCount_;

    AdjustWindowForSpace(requiredSpace);
    pairedQueue_.push_back(deviceInfo);
    windowEndIndex_ = pairedQueue_.size();
    HILOGI("Add Device %{public}s to tail, windowStartIndex_ = %{public}d, windowEndIndex_ = %{public}d",
        GetEncryptAddr(deviceInfo.reportAddr_).c_str(), windowStartIndex_, windowEndIndex_);
}

void SleReconnectManager::RemoveDeviceFromQueue(const std::string &addr)
{
    size_t deviceIndex = FindDeviceIndex(addr);
    if (deviceIndex == pairedQueue_.size()) {
        return;
    }

    bool isInWin = IsDeviceIndexInWindow(deviceIndex);
    ReconnectDeviceInfo deviceInfo = pairedQueue_[deviceIndex];
    pairedQueue_.erase(pairedQueue_.begin() + deviceIndex);
    if (!isInWin) {
        windowStartIndex_ = windowStartIndex_ > 0 ? windowStartIndex_ - 1 : 0;
    }
    windowEndIndex_ = pairedQueue_.size();
    ExpandWindowLeft();
    HILOGI("Remove Device %{public}s, windowStartIndex_ = %{public}d, windowEndIndex_ = %{public}d",
        GetEncryptAddr(deviceInfo.reportAddr_).c_str(), windowStartIndex_, windowEndIndex_);
}

void SleReconnectManager::MoveDeviceToTail(const std::string &addr)
{
    size_t deviceIndex = FindDeviceIndex(addr);
    if (deviceIndex == pairedQueue_.size()) {
        return;
    }
    ReconnectDeviceInfo deviceInfo = pairedQueue_[deviceIndex];
    if (IsDeviceIndexInWindow(deviceIndex)) {
        pairedQueue_.erase(pairedQueue_.begin() + deviceIndex);
        pairedQueue_.push_back(deviceInfo);
        return;
    }

    pairedQueue_.erase(pairedQueue_.begin() + deviceIndex);
    windowStartIndex_ = windowStartIndex_ > 0 ? windowStartIndex_ - 1 : 0;
    windowEndIndex_--;

    uint32_t requiredSpace = deviceInfo.cdsmMemberCount_ > 0 ? deviceInfo.cdsmMemberCount_ : 1;
    uint32_t currentWindowCount = GetWindowDeviceCount();

    AdjustWindowForSpace(requiredSpace);
    pairedQueue_.push_back(deviceInfo);
    windowEndIndex_ = pairedQueue_.size();
    HILOGI("Move Device %{public}s to tail, windowStartIndex_ = %{public}d, windowEndIndex_ = %{public}d",
        GetEncryptAddr(deviceInfo.reportAddr_).c_str(), windowStartIndex_, windowEndIndex_);
}

void SleReconnectManager::MoveDeviceFromWindowToHead(const std::string &addr)
{
    size_t deviceIndex = FindDeviceIndex(addr);
    if (!IsDeviceIndexInWindow(deviceIndex)) {
        return;
    }
    ReconnectDeviceInfo deviceInfo = pairedQueue_[deviceIndex];
    pairedQueue_.erase(pairedQueue_.begin() + deviceIndex);
    pairedQueue_.insert(pairedQueue_.begin(), deviceInfo);
    windowStartIndex_ = windowStartIndex_ < pairedQueue_.size() - 1 ? windowStartIndex_ + 1 : 0;
    ExpandWindowLeft();
}

void SleReconnectManager::AdjustWindowForSpace(uint32_t requiredSpace)
{
    auto adapter = static_cast<SleInterfaceAdapterSub*>(
    SleInterfaceManager::GetInstance()->GetAdapter(SleTransport::ADAPTER_SLE));
    NL_CHECK_RETURN(adapter, "[SleReconnectManager] sleAdapter is null.");

    uint32_t currentCount = GetWindowDeviceCount();
    HILOGI("bgConnMax Size = %{public}d, currentWindows Size=%{public}d, requiredSpace=%{public}d",
        bgConnMaxNum_, currentCount, requiredSpace);
    while (currentCount + requiredSpace > bgConnMaxNum_ && windowStartIndex_ < windowEndIndex_) {
        ReconnectDeviceInfo deviceInfo = pairedQueue_[windowStartIndex_];
        bool recoverBgList = inBgListDevices_.find(deviceInfo.reportAddr_) != inBgListDevices_.end();
        HILOGI("reportAddr %{public}s out of window, recoverBgList = %{public}d",
            GetEncryptAddr(deviceInfo.reportAddr_).c_str(), recoverBgList);
        for (const auto &memberAddr : deviceInfo.cdsmMembers_) {
            RawAddress memberDevice(memberAddr);
            if (recoverBgList) {
                adapter->RemoveBgConnDevice(memberAddr);
            }
            SleRemoteDeviceAdapter::GetInstance()->RemovePeerDeviceTypeToController(memberDevice);
        }
        currentCount -= deviceInfo.cdsmMemberCount_;
        if (windowStartIndex_ >= pairedQueue_.size() - 1) {
            break;
        }
        windowStartIndex_ = windowStartIndex_ + 1;
    }
}

void SleReconnectManager::ExpandWindowLeft()
{
    if (windowStartIndex_ == 0) {
        return;
    }
    auto adapter = static_cast<SleInterfaceAdapterSub*>(
    SleInterfaceManager::GetInstance()->GetAdapter(SleTransport::ADAPTER_SLE));
    NL_CHECK_RETURN(adapter, "[SleReconnectManager] sleAdapter is null.");
    uint32_t requiredSpace = GetRequiredSpaceForPreviousDevice();
    HILOGI("bgConnMax Size = %{public}d, currentWindows Size=%{public}d, requiredSpace=%{public}d",
        bgConnMaxNum_, GetWindowDeviceCount(), requiredSpace);
    while (windowStartIndex_ > 0 && GetWindowDeviceCount() + requiredSpace <= bgConnMaxNum_) {
        windowStartIndex_--;
        ReconnectDeviceInfo deviceInfo = pairedQueue_[windowStartIndex_];
        bool recoverBgList = inBgListDevices_.find(deviceInfo.reportAddr_) != inBgListDevices_.end();
        HILOGI("reportAddr %{public}s add in window, recoverBgList = %{public}d",
            GetEncryptAddr(deviceInfo.reportAddr_).c_str(), recoverBgList);
        for (const auto &memberAddr : deviceInfo.cdsmMembers_) {
            RawAddress memberDevice(memberAddr);
            if (recoverBgList) {
                SleRemoteDeviceAdapter::GetInstance()->AddBgConnDevice(memberAddr);
            }
            SleRemoteDeviceAdapter::GetInstance()->SetPeerDeviceTypeToController(memberDevice);
        }
        requiredSpace = GetRequiredSpaceForPreviousDevice();
    }
}

uint32_t SleReconnectManager::GetRequiredSpaceForPreviousDevice()
{
    if (windowStartIndex_ > 0) {
        return pairedQueue_[windowStartIndex_ - 1].cdsmMemberCount_;
    }
    return 0;
}

void SleReconnectManager::OnDeviceDisConnected(const RawAddress &device, bool isNeedBgConn)
{
    CdsmService *cdsmService = CdsmService::GetService();
    NL_CHECK_RETURN(cdsmService, "cdsmService nullptr.");
    RawAddress reportAddr(device);
    cdsmService->CdsmGetReportAddr(device, reportAddr);
    size_t deviceIndex = FindDeviceIndex(reportAddr.GetAddress());
    if (deviceIndex == pairedQueue_.size()) {
        return;
    }
    if (isNeedBgConn) {
        inBgListDevices_.insert(reportAddr.GetAddress());
    }
}

void SleReconnectManager::OnDeviceConnected(const RawAddress &device, bool isDeviceAvailable)
{
    CdsmService *cdsmService = CdsmService::GetService();
    NL_CHECK_RETURN(cdsmService, "cdsmService nullptr.");
    RawAddress reportAddr(device);
    cdsmService->CdsmGetReportAddr(device, reportAddr);
    size_t deviceIndex = FindDeviceIndex(reportAddr.GetAddress());
    if (deviceIndex == pairedQueue_.size()) {
        return;
    }
    if (!isDeviceAvailable) {                               // 连接成功，但为不可用设备，不需要回连
        RemoveDeviceFromQueue(reportAddr.GetAddress());
        return;
    }
    if (inBgListDevices_.find(reportAddr.GetAddress()) != inBgListDevices_.end()) {
        inBgListDevices_.erase(reportAddr.GetAddress());    // 连接成功后，不需要再加背景连白名单
    }
    MoveDeviceToTail(reportAddr.GetAddress());
    SaveAddressListToConfig();
}

void SleReconnectManager::OnDeviceStartPair(const RawAddress &device)
{
    std::string deviceAddr = device.GetAddress();
    CdsmService *cdsmService = CdsmService::GetService();
    NL_CHECK_RETURN(cdsmService, "cdsmService nullptr.");
    std::vector<std::string> cdsmMembers;
    uint32_t cdsmMemberCount = 1;
    if (cdsmService->CdsmCheckIsCooperationDevice(device)) {
        std::vector<NearlinkCdsmInfo> cdsmInfoList;
        if (cdsmService->CdsmGetAllMemberInfo(device, cdsmInfoList) == NL_NO_ERROR) {
            cdsmMemberCount = cdsmInfoList.size();
            for (const auto &info : cdsmInfoList) {
                cdsmMembers.push_back(info.addr_.GetAddress());
            }
        }
    } else {
        cdsmMembers.push_back(device.GetAddress());
    }
    RawAddress reportAddr(device);
    cdsmService->CdsmGetReportAddr(device, reportAddr);
    size_t deviceIndex = FindDeviceIndex(reportAddr.GetAddress());
    if (deviceIndex == pairedQueue_.size()) {
        ReconnectDeviceInfo deviceInfo(reportAddr.GetAddress(), cdsmMembers, cdsmMemberCount);
        AddDeviceToPairedQueue(deviceInfo);
    } else {
        MoveDeviceToTail(reportAddr.GetAddress());
    }
    SaveAddressListToConfig();
}

void SleReconnectManager::OnDeviceUnpaired(const RawAddress &device)
{
    std::string deviceAddr = device.GetAddress();
    CdsmService *cdsmService = CdsmService::GetService();
    NL_CHECK_RETURN(cdsmService, "cdsmService nullptr.");
    RawAddress reportAddr(device);
    cdsmService->CdsmGetReportAddr(device, reportAddr);
    RemoveDeviceFromQueue(reportAddr.GetAddress());
    SaveAddressListToConfig();
}

std::vector<std::string> SleReconnectManager::GetReconnectList()
{
    LoadPairedQueueFromConfig();
    std::vector<std::string> reconnectList;
    if (pairedQueue_.empty()) {
        return reconnectList;
    }

    if (windowStartIndex_ > windowEndIndex_ || windowEndIndex_ > pairedQueue_.size()) {
        HILOGW("[SleReconnectManager] Invalid window range, returning empty list");
        return reconnectList;
    }

    for (size_t i = windowStartIndex_; i < windowEndIndex_; ++i) {
        for (const auto &addr : pairedQueue_[i].cdsmMembers_) {
            reconnectList.emplace_back(addr);
            HILOGI("Add device %{public}s", GetEncryptAddr(addr).c_str());
        }
    }
    return reconnectList;
}

void SleReconnectManager::LoadPairedQueueFromConfig()
{
    pairedQueue_.clear();
    std::string addressList = SleConfig::GetInstance().GetReconnectDeviceAddressList();
    if (addressList.empty()) {
        HILOGI("[SleReconnectManager] No paired device address list in config pairedQueue");
        LoadDefaultPairedDevices();
        SaveAddressListToConfig();
        return;
    }
    std::vector<std::string> deviceAddresses = ParseAddressList(addressList);
    for (const auto &address : deviceAddresses) {
        LoadSingleDeviceFromConfig(address);
    }
    SaveAddressListToConfig();
    HILOGI("[SleReconnectManager] Loaded paired queue from config, size: %{public}zu", pairedQueue_.size());
}

std::vector<std::string> SleReconnectManager::ParseAddressList(const std::string &addressList)
{
    std::vector<std::string> deviceAddresses;
    size_t pos = 0;
    size_t prevPos = 0;
    while ((pos = addressList.find('|', prevPos)) != std::string::npos) {
        if (pos > prevPos) {
            deviceAddresses.push_back(addressList.substr(prevPos, pos - prevPos));
        }
        prevPos = pos + 1;
    }
    if (prevPos < addressList.length()) {
        deviceAddresses.push_back(addressList.substr(prevPos));
    }
    return deviceAddresses;
}

std::string SleReconnectManager::BuildAddressListString() const
{
    std::string addressList;
    for (const auto &deviceInfo : pairedQueue_) {
        if (!addressList.empty()) {
            addressList += "|";
        }
        addressList += deviceInfo.reportAddr_;
    }
    return addressList;
}

void SleReconnectManager::SaveAddressListToConfig() const
{
    std::string addressList = BuildAddressListString();
    SleConfig::GetInstance().SetReconnectDeviceAddressList(addressList);
    SleConfig::GetInstance().Save();
}

void SleReconnectManager::LoadSingleDeviceFromConfig(const std::string &address)
{
    std::vector<std::string> cdsmMembers = {};
    SleConfig::GetInstance().GetCdsmMemberList(address, cdsmMembers);
    if (cdsmMembers.empty()) {
        cdsmMembers.push_back(address);
    }
    size_t cdsmMemberCount = cdsmMembers.size();
    ReconnectDeviceInfo deviceInfo(address, cdsmMembers, cdsmMemberCount);
    inBgListDevices_.insert(address);
    AddDeviceToPairedQueue(deviceInfo);
    bool isUserDisconnected = false;
    for (auto &addr : cdsmMembers) {
        isUserDisconnected = isUserDisconnected || SleConfig::GetInstance().GetUserDisconnectedFlag(addr);
    }
    if (isUserDisconnected && !SleServiceManager::GetInstance()->IsEnableAutoConnectUserDisconnectedDevices()) {
        HILOGI("chip reset and  %{public}s is disconnted by user, low priority", GetEncryptAddr(address).c_str());
        MoveDeviceFromWindowToHead(address);
    }
}

void SleReconnectManager::LoadDefaultPairedDevices()
{
    std::vector<std::string> pairedAddrList = SleConfig::GetInstance().GetPairedAddrList();
    CdsmService *cdsmService = CdsmService::GetService();
    NL_CHECK_RETURN(cdsmService, "cdsmService is null");

    for (auto &addrStr : pairedAddrList) {
        HILOGI("addr : %{public}s", GetEncryptAddr(addrStr).c_str());
        RawAddress addr(addrStr);
        RawAddress reportAddr(addr);
        cdsmService->CdsmGetReportAddr(addr, reportAddr);
        size_t deviceIndex = FindDeviceIndex(reportAddr.GetAddress());
        if (deviceIndex != pairedQueue_.size()) {
            continue;
        }
        std::vector<std::string> cdsmMembers = {};
        SleConfig::GetInstance().GetCdsmMemberList(reportAddr.GetAddress(), cdsmMembers);
        if (cdsmMembers.empty()) {
            cdsmMembers.push_back(reportAddr.GetAddress());
        }
        size_t cdsmMemberCount = cdsmMembers.size();
        ReconnectDeviceInfo deviceInfo(reportAddr.GetAddress(), cdsmMembers, cdsmMemberCount);
        inBgListDevices_.insert(reportAddr.GetAddress());
        AddDeviceToPairedQueue(deviceInfo);
        bool isUserDisconnected = false;
        for (auto &addr : cdsmMembers) {
            isUserDisconnected = isUserDisconnected || SleConfig::GetInstance().GetUserDisconnectedFlag(addr);
        }
        if (isUserDisconnected && !SleServiceManager::GetInstance()->IsEnableAutoConnectUserDisconnectedDevices()) {
            HILOGI("chip reset and  %{public}s is disconnted by user, low priority", GET_ENCRYPT_ADDR(reportAddr));
            MoveDeviceFromWindowToHead(reportAddr.GetAddress());
        }
    }
}

} // namespace Nearlink
} // namespace OHOS