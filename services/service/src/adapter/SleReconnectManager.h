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
#ifndef SLE_RECONNECT_MANAGER_H
#define SLE_RECONNECT_MANAGER_H

#include <string>
#include "nearlink_def.h"
#include "nearlink_raw_address.h"
#include "nearlink_utils.h"
#include "log_util.h"
#include <set>

namespace OHOS {
namespace Nearlink {
constexpr uint32_t RECONNECT_ACCEPT_LIST_MAX_SIZE = 16;

struct ReconnectDeviceInfo {
    std::string reportAddr_;
    std::vector<std::string> cdsmMembers_;
    uint32_t cdsmMemberCount_;
    ReconnectDeviceInfo() : cdsmMemberCount_(0) {}
    ReconnectDeviceInfo(const std::string &addr, const std::vector<std::string> &members, uint32_t memberCount)
        : reportAddr_(addr), cdsmMembers_(members), cdsmMemberCount_(memberCount) {}
};

class SleReconnectManager {
public:
    static SleReconnectManager& GetInstance();

    void RemoveAutoConnAudioDevConf(const RawAddress& addr);
    bool IsNeedToIgnore(const std::string& reconnDevAddr);
    std::string GetReconnDevice();
    void SetReconnDeviceParam();
    void SetAutoConnectDevice(const RawAddress& reportAddr, bool isActive, int pairState, std::string btAddr);

    void OnDeviceStartPair(const RawAddress &device);
    void OnDeviceUnpaired(const RawAddress &device);
    void OnDeviceDisConnected(const RawAddress &device, bool isNeedBgConn);
    void OnDeviceConnected(const RawAddress &device, bool isDeviceAvailable);
    void SetBgConnMaxNum(uint32_t size);
    std::vector<std::string> GetReconnectList();

private:
    SleReconnectManager() = default;
    ~SleReconnectManager() = default;

    uint32_t GetWindowDeviceCount() const;

    size_t FindDeviceIndex(const std::string &addr) const;
    bool IsDeviceIndexInWindow(size_t index) const;

    void AddDeviceToPairedQueue(const ReconnectDeviceInfo deviceInfo);
    void RemoveDeviceFromQueue(const std::string &addr);

    void MoveDeviceToTail(const std::string &addr);
    void MoveDeviceFromWindowToHead(const std::string &addr);

    void AdjustWindowForSpace(uint32_t requiredSpace);
    void ExpandWindowLeft();
    uint32_t GetRequiredSpaceForPreviousDevice();

    std::vector<std::string> ParseAddressList(const std::string &addressList);
    std::string BuildAddressListString() const;
    void SaveAddressListToConfig() const;
    void LoadPairedQueueFromConfig();
    void LoadSingleDeviceFromConfig(const std::string &address);
    void LoadDefaultPairedDevices();

    std::vector<ReconnectDeviceInfo> pairedQueue_;
    size_t windowStartIndex_ = 0;
    size_t windowEndIndex_ = 0;
    uint32_t bgConnMaxNum_ = RECONNECT_ACCEPT_LIST_MAX_SIZE;
    std::set<std::string> inBgListDevices_;
};
} // namespace Nearlink
} // namespace OHOS
#endif // SLE_RECONNECT_MANAGER_H