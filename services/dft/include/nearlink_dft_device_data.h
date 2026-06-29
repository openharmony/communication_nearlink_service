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

#ifndef NEARLINK_DFT_DEVICE_DATA_H
#define NEARLINK_DFT_DEVICE_DATA_H

#include <string>
#include "raw_address.h"
#include "nearlink_safe_map.h"

namespace OHOS {
namespace Nearlink {

class NearlinkDeviceData {
public:
    NearlinkDeviceData(){};
    ~NearlinkDeviceData(){};
    std::string GetName();
    int32_t GetAppearance();
    RawAddress GetReportAddr();
    std::string GetAddr();

    void SetName(const std::string &name);
    void SetAppearance(int32_t appearance);
    void SetReportAddr(RawAddress reportAddr);
    void SetAddr(const std::string &address);
    void SetManufacturer(const std::string &manufacturer);
private:
    int32_t appearance_ = 0xFFFF;
    RawAddress reportAddr_;
    std::string name_ {};
    std::string address_ = {};
    std::string manufacturer_ = {};
};

class DftDeviceManager {
public:
    static DftDeviceManager &GetInstance();
    void AddDevice(RawAddress address, const int32_t appearance = 0xFFFF, const std::string &name = "");
    void EnsureUpdateReportAddr(RawAddress address, RawAddress reportAddr);
    void UpdateAppearance(RawAddress address, const int32_t appearance);
    void UpdateName(RawAddress address, const std::string &name);
    void UpdateManufacturer(RawAddress address, const std::string &manufacturer);
    void DelDevice(RawAddress address);
    void DelAllDevice();
    void FillCommonDeviceInfo(RawAddress address, std::string &name, int32_t &appearance);
    void GetReportAddr(RawAddress address, RawAddress &reportAddr);
private:
    NearlinkSafeMap<std::string, NearlinkDeviceData> peerDeviceSafeList_;
    DftDeviceManager(){};
    ~DftDeviceManager(){};
};

}  // namespace Nearlink
}  // namespace OHOS
#endif  /// NEARLINK_DFT_DEVICE_DATA_H