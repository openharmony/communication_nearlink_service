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
#ifndef DIS_CLIENT_STACK_ADAPTER_H
#define DIS_CLIENT_STACK_ADAPTER_H

#include "raw_address.h"
#include "nearlink_safe_map.h"
#include "nlstk_dis_client.h"
#include "nearlink_device_information.h"

namespace OHOS {
namespace Nearlink {

constexpr int DIS_SUCCESS = 0;
constexpr int DIS_FAILURE = 1;
constexpr int DIS_INVALID = -1;
constexpr uint32_t DIS_INVALID_APPEARANCE_INFO = 0xFFFFFFFF;

const std::string PAIR_DIS_REACH_MAX_NUM = "DIS Connect reach max";

enum class ManufacturerInfoType : uint8_t {
    MANUFACTURER_INFO_VENDOR_ID = 0,
    MANUFACTURER_INFO_PRODUCT_ID,
    MANUFACTURER_INFO_VERSION
};

enum class DisInfoType {
    DIS_MANUFACTURER_INFO = 0x00,
    DIS_MODEL_INFO,
    DIS_SERIAL_INFO,
    DIS_HARDWARE_VERSION_INFO,
    DIS_FIRMWARE_VERSION_INFO,
    DIS_SOFTWARE_VERSION_INFO,
    DIS_LOCAL_ALIAS_INFO,
    DIS_APPEARANCE_INFO,
};

class DisClientStackAdapter {
public:
    DisClientStackAdapter();
    ~DisClientStackAdapter();
    int RegisteCallBackToStack();
    int Connect(const RawAddress &addr);
    int Disconnect(const RawAddress &addr);
    uint16_t GetDeviceVendorId(const RawAddress &device);
    uint16_t GetDeviceProductId(const RawAddress &device);
    uint16_t GetDeviceVersion(const RawAddress &device);
    uint32_t GetAppearanceInfo(const RawAddress &device);
    std::string GetNameInfo(const RawAddress &device);
    void GetDeviceInformation(const RawAddress &device, DeviceInformation &information);

private:
    static void OnConnectionStateChanged(
        SLE_Addr_S *addr, NLSTK_DisConnectState_E curState, NLSTK_DisConnectState_E preState, NLSTK_Errcode_E ret);
    uint16_t ParseManufacturerInfo(uint8_t *inputData, uint16_t len, ManufacturerInfoType type);
    std::string DeviceInformationToString(const DeviceInformation &info);

};

} // namespace Nearlink
} // namespace OHOS

#endif // DIS_CLIENT_STACK_ADAPTER_H