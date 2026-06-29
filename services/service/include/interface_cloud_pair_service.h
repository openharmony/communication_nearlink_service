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

#ifndef INTERFACE_CLOUD_PAIR_SERVICE_H
#define INTERFACE_CLOUD_PAIR_SERVICE_H

#include <cstdint>
#include <string>
#include "nearlink_cloud_pair_device.h"
#include "raw_address.h"

namespace OHOS {
namespace Nearlink {
typedef enum {
    CLOUD_PAIR_INVALID = 0,
    CLOUD_PAIR_NONE = 1,            // 云配对下云了，但是没开始连接
    CLOUD_PAIR_PAIRING = 2,         // 云配对下云了，开始连接，拦截profile连接，等上层应用校验toke
    CLOUD_PAIR_PAIRED = 3,          // 云配对完成，标准配对也完成
    CLOUD_PAIR_REMOVING = 4,        // token变化后，先删配对切回NONE 等下一次可信配对
    CLOUD_PAIR_CREATE_PAIR = 5,      // 校验token完成，开始标准配对
    CLOUD_PAIR_TOKEN_CHANGING = 6,   // token变化中，等待重配对完成
} NL_CLOUD_PAIR_STATE;

class InterfaceCloudPairService {
public:
    virtual ~InterfaceCloudPairService() = default;

    static InterfaceCloudPairService &GetInstance();

    virtual void Init() = 0;
    virtual bool UpdateCloudDeviceInfoList(std::vector<NearlinkCloudPairDevice> &cloudDeviceInfos) = 0;
    virtual bool GetCloudPairState(const std::string &address, int32_t &cloudPairState) = 0;

    virtual void ClearCloudDeviceMap(bool isNeedClearConfig) = 0;
    virtual void AddCloudPairDevices(std::vector<RawAddress> &pairedList) = 0;
    virtual bool IsCloudDevice(const RawAddress &device) = 0;
    virtual bool CancelCloudPairing(const RawAddress &device) = 0;
    virtual bool ChkCloudDeviceAndPermission(const RawAddress &device) = 0;
    virtual bool CloudDeviceConnectionComplete(const RawAddress &device) = 0;
    virtual bool CancelCloudPairComplete(const RawAddress &device, int preStatus, int reason, bool isCdsmAcbConnected,
        int acbState) = 0;
    virtual void SetKeyMissingPairState(const RawAddress &device) = 0;
    virtual bool ConnectCloudDeviceAllProfile(const RawAddress &device) = 0;
    virtual std::string GetCloudDeviceAliasName(const RawAddress &device) = 0;
    virtual bool SetCloudDeviceAliasName(const RawAddress &device, const std::string &name) = 0;
    virtual bool IsCloudDeviceConnecting(const RawAddress &device) = 0;
    virtual std::string GetBtAddrByReportAddr(std::string sleReportAddr) = 0;
    virtual std::string GetReportAddrByBtAddr(std::string btAddr) = 0;
    virtual void SetCrediblePairState(const RawAddress &device) = 0;

    virtual std::string GetCloudDeviceIcondId(const RawAddress &device) = 0;
    virtual std::string GetCloudDeviceSubModelId(const RawAddress &device) = 0;
    virtual int GetCloudDeviceManufacturerBusinessType(const RawAddress &device) = 0;
    virtual void HandlePairStatusChanged(const RawAddress &device, int32_t preStatus,
        int32_t status, int32_t reason) = 0;
    virtual void HandleAcbStateChanged(const RawAddress &device, int32_t state, int reason) = 0;
    virtual bool IsCloudDeviceCreatePair(const RawAddress &device) = 0;
    virtual bool IsInRepairing(const RawAddress &device) = 0;
    virtual bool IsPreparingRepair(const RawAddress &device) = 0;
    virtual bool IsInReplacing(const RawAddress &reportAddr) = 0;
    virtual RawAddress GetCloudDeviceRealAddress(const RawAddress &device) = 0;
};

} // namespace Nearlink
} // namespace OHOS

#endif // INTERFACE_CLOUD_PAIR_SERVICE_H