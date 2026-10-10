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

#ifndef NEARLINK_REMOTE_DEVICE_BASE_H
#define NEARLINK_REMOTE_DEVICE_BASE_H

#include <cstdint>
#include <string>
#include <vector>

#include "nearlink_def.h"
#include "nearlink_types.h"
#include "nearlink_errorcode.h"
#include "nearlink_device_model.h"
#include "nearlink_device_information.h"

namespace OHOS {
namespace Nearlink {

class NearlinkRemoteDevice;

/**
 * @brief base.so implementation class of framework remote device.
 *
 * NearlinkRemoteDevice derives from this class, so framework-side mirror methods
 * share this implementation without any cast.
 */
class NEARLINK_API NearlinkBaseRemoteDevice {
public:
    NearlinkBaseRemoteDevice(){};
    NearlinkBaseRemoteDevice(const std::string &addr, const int transport);
    virtual ~NearlinkBaseRemoteDevice(){};

    std::string GetDeviceAddr() const
    {
        return address_;
    };
    NlErrCode GetDeviceName(std::string &name) const;
    NlErrCode GetDeviceAlias(std::string &alias) const;
    NlErrCode SetDeviceAlias(const std::string &aliasName);
    NlErrCode GetDeviceBatteryLevel() const;
    NlErrCode GetPairState(int &pairState) const;
    NlErrCode StartPair();
    NlErrCode StartCrediblePair();
    NlErrCode CancelDevicePairing();
    NlErrCode SetPairingConfirmation(bool cfm);
    NlErrCode SetPairingPassCode(const std::string &passCode);
    NlErrCode IsBondedFromLocal(bool &isBondedFromLocal) const;
    NlErrCode IsAcbConnected(bool &isAcbConnected) const;
    NlErrCode IsAcbEncrypted(bool &isAcbEncrypted) const;
    NlErrCode GetDeviceProductId(uint16_t &productId) const;
    NlErrCode GetDeviceVendorId(uint16_t &vendorId) const;
    NlErrCode GetDeviceModel(DeviceModel &model) const;
    NlErrCode GetDeviceInformation(DeviceInformation &information) const;
    NlErrCode GetDeviceUuids(std::vector<std::string> &uuids) const;
    NlErrCode PairRequestReply(bool accept);
    int GetTransportType() const;
    NlErrCode UpdateConnectInterval(ConnectionInterval interval) const;
    NlErrCode ReadRemoteRssiValue();
    bool IsValidNearlinkRemoteDevice() const;
    NlErrCode GetDeviceAppearance(int &appearance) const;
    NlErrCode GetAcbState(int &acbState) const;
    NlErrCode GetBatteryLevel();

private:
    bool IsValidPassCode(const std::string& passCode);

    std::string address_ = "00:00:00:00:00:00"; // address is little endian order
    int transport_ = 0;
};

/**
 * @brief Convert base device to framework device reference for observer callbacks.
 *
 * The two classes share identical memory layout; the cast is the inverse of the
 * ToBase cast.
 */
inline const NearlinkRemoteDevice &ToFrameworkDevice(const NearlinkBaseRemoteDevice &device)
{
    return reinterpret_cast<const NearlinkRemoteDevice &>(device);
}
}  // namespace Nearlink
} // namespace OHOS

#endif  // NEARLINK_REMOTE_DEVICE_BASE_H
