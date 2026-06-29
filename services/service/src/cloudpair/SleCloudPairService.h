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
#ifndef SLE_CLOUD_PAIR_SERVICE
#define SLE_CLOUD_PAIR_SERVICE

#include <iostream>
#include <string>

#include "BaseDef.h"
#include "ipc_skeleton.h"
#include "interface_cloud_pair_service.h"
#include "nearlink_cloud_pair_def.h"
#include "nearlink_permission_manager.h"
#include "nearlink_safe_map.h"
#include "nearlink_safe_list.h"
#include "nearlink_raw_address.h"
#include "nearlink_utils.h"
#include "raw_address.h"
#include "nearlink_timer.h"
#include "TwsService.h"
#include "nearlink_utils.h"

namespace OHOS {
namespace Nearlink {

class DownCloudPairDevice : public CloudPairDeviceInfo {
public:

    DownCloudPairDevice();
    ~DownCloudPairDevice();

    void SetCloudPairState(int32_t state);
    int32_t GetCloudPairState();

    void SetRole(std::string addr);

    void InitConnectedState();
    bool IsAllMembersDisconnected();
    void SetConnectedState(std::string addr, bool isConnected);

    std::string GetPrimary();
    void SetPrimary(std::string addr);
    std::map<std::string, bool> GetConnectedMaps();
    void SetConnectedMaps(std::map<std::string, bool> connectedMaps);

private:
    std::map<std::string, bool> connectedMaps_;
    std::string primary_ = "";
    int32_t cloudPairState_ = NL_CLOUD_PAIR_STATE::CLOUD_PAIR_INVALID;
};


class SleCloudPairService : public InterfaceCloudPairService {
public:
    static SleCloudPairService &GetInstance();

    SleCloudPairService();
    ~SleCloudPairService();

    void Init() override;
    bool UpdateCloudDeviceInfoList(std::vector<NearlinkCloudPairDevice> &cloudDeviceInfos) override;
    bool GetCloudPairState(const std::string &address, int32_t &cloudPairState) override;

    void ClearCloudDeviceMap(bool isNeedClearConfig) override;
    void AddCloudPairDevices(std::vector<RawAddress> &pairedList) override;
    bool IsCloudDevice(const RawAddress &device) override;

    bool CancelCloudPairing(const RawAddress &device) override;
    bool ChkCloudDeviceAndPermission(const RawAddress &device) override;
    bool CloudDeviceConnectionComplete(const RawAddress &device) override;
    bool CancelCloudPairComplete(const RawAddress &device, int preStatus, int reason, bool isCdsmAcbConnected,
        int acbState) override;
    void SetKeyMissingPairState(const RawAddress &device) override;
    bool ConnectCloudDeviceAllProfile(const RawAddress &device) override;

    std::string GetCloudDeviceAliasName(const RawAddress &device) override;
    bool SetCloudDeviceAliasName(const RawAddress &device, const std::string &name) override;
    std::string GetCloudDeviceIcondId(const RawAddress &device) override;
    std::string GetCloudDeviceSubModelId(const RawAddress &device) override;
    int GetCloudDeviceManufacturerBusinessType(const RawAddress &device) override;

    bool IsCloudDeviceConnecting(const RawAddress &device) override;
    std::string GetBtAddrByReportAddr(std::string sleReportAddr) override;
    std::string GetReportAddrByBtAddr(std::string btAddr) override;

    void SetCrediblePairState(const RawAddress &device) override;
    void HandlePairStatusChanged(const RawAddress &device, int32_t preStatus,
        int32_t status, int32_t reason) override;
    void HandleAcbStateChanged(const RawAddress &device, int32_t state, int reason) override;
    bool IsCloudDeviceCreatePair(const RawAddress &device) override;
    bool IsInRepairing(const RawAddress &device) override;
    bool IsInReplacing(const RawAddress &reportAddr) override;
    bool IsPreparingRepair(const RawAddress &device) override;
    RawAddress GetCloudDeviceRealAddress(const RawAddress &device) override;

private:

    void InitTokenChkTimer(std::string reportAddr);
    void StartTokenChkTimer(std::string reportAddr);
    void StopTokenChkTimer(std::string reportAddr);
    void ReadCloudDeviceInfoFromConf(const std::vector<std::string> &cloudDeviceList);
    void HandleDeviceNameChanged(NearlinkCloudPairDevice &devInfo, const std::string &oldName);
    void HandleTokenChanged(NearlinkCloudPairDevice &devInfo, const std::vector<uint8_t> &oldToken);
    void UpdateCloudState(std::string address, int32_t cloudPairState);
    std::string GetCollabAddrByReportAddr(const RawAddress &reportAddr);
    void GetAllNotPairedCloudDeviceList(std::vector<std::string> &cloudDeviceAddrList);
    void UpdateCloudDev(NearlinkCloudPairDevice &dev, std::shared_ptr<DownCloudPairDevice> downDevice);
    void DelCloudDevFromMap(std::vector<NearlinkCloudPairDevice> &cloudDeviceInfos);
    void EnableVirtualAutoSwitch(const RawAddress &reportAddr);
    void ProcCreateCloudDeviceCdsmGroup(const RawAddress &reportAddr);
    void RemovePairedDeviceFromConf(const RawAddress &reportAddr);
    void RmvSpecificCloudDevice(const RawAddress &reportAddr);
    void RefreshCloudDevice(NearlinkCloudPairDevice devInfo);
    void AddCloudDevice(NearlinkCloudPairDevice devInfo);
    void ReplaceOldCloudDevice(NearlinkCloudPairDevice devInfo);
    void SetNewCloudDevice(std::shared_ptr<DownCloudPairDevice> &oldDev, std::shared_ptr<DownCloudPairDevice> &newDev);
    bool IsAllMembersDisconnected(const RawAddress &device);
    bool SetConnectedState(const RawAddress &device, bool isConnected);
    bool ClearToken(const RawAddress &device);
    RawAddress GetReportAddr(const RawAddress& deivce);
    bool IsValidDownCloudDeviceList(std::vector<NearlinkCloudPairDevice> &cloudDeviceInfos);
    void HandleAcbDisconnectedTask(const RawAddress &reportAddr, int32_t curCloudPairState);
    bool ConvertDecStrToHexStr(const std::string &icon, std::string &iconHexStr);
    NearlinkSafeMap<std::string, std::shared_ptr<DownCloudPairDevice>> cloudDevicesMap_ {};
    NearlinkSafeMap<std::string, std::shared_ptr<NearlinkTimer>> tokenChkTimersMap_ {};
    NearlinkSafeList<std::string> needRepairDevices_ {};
    NearlinkSafeList<std::string> replacedDevices_ {};
};
} // namespace Nearlink
} // namespace OHOS

#endif // SLE_CLOUD_PAIR_SERVICE