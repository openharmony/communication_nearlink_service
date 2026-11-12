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

#ifndef NEARLINK_REMOTE_DEVICE_H
#define NEARLINK_REMOTE_DEVICE_H

#include <string>
#include <vector>

#include "nearlink_def.h"
#include "nearlink_types.h"
#include "nearlink_errorcode.h"
#include "nearlink_device_model.h"
#include "nearlink_device_information.h"
#include "nearlink_base_remote_device.h"

namespace OHOS {
namespace Nearlink {

/**
 * @brief Represents framework remote device.
 *
 * Embeds NearlinkBaseRemoteDevice in base.so; all implementations are
 * shared from the embedded base device by forwarding without any cast.
 *
 * 说明：仅声明未实现（无任何定义）的接口（如 GetDeviceBatteryLevel）不建立桥接转发
 * （拆分前框架亦未导出）。
 */
class NEARLINK_API NearlinkRemoteDevice {
public:
    /**
     * @brief A structor used to create the <b>NearlinkRemoteDevice</b> instance.
     *
     * @since 6
     */
    NearlinkRemoteDevice();

    /**
     * @brief A structor used to create the <b>NearlinkRemoteDevice</b> instance, addr is little endian order.
     *
     * @since 6
     */
    NearlinkRemoteDevice(const std::string &addr, const int transport);

    /**
     * @brief A structor used to create the <b>NearlinkRemoteDevice</b> instance from a base device.
     *
     * @since 6
     */
    explicit NearlinkRemoteDevice(const NearlinkBaseRemoteDevice &base);

    /**
     * @brief A destructor used to delete the <b>NearlinkRemoteDevice</b> instance.
     *
     * @since 6
     */
    ~NearlinkRemoteDevice();

    /**
     * @brief Get device address, address is little endian order.
     *
     * @return Returns device adress.
     * @since 6
     */
    std::string GetDeviceAddr() const
    {
        return base_.GetDeviceAddr();
    };

    /**
     * @brief Get the embedded base remote device.
     *
     * @return Returns the embedded base remote device reference.
     * @since 6
     */
    const NearlinkBaseRemoteDevice &GetBaseDevice() const
    {
        return base_;
    };

    /**
     * @brief Get device name.
     *
     * @param[out] name Returns device name.
     * @return Returns the status code for this function called.
     */
    NlErrCode GetDeviceName(std::string &name) const;

    /**
     * @brief Get device alias.
     *
     * @param[out] Returns device alias.
     * @return Returns the status code for this function called.
     */
    NlErrCode GetDeviceAlias(std::string &alias) const;

    /**
     * @brief Set device alias.
     *
     * @param aliasName Device alias name.
     * @return Returns the status code for this function called.
     */
    NlErrCode SetDeviceAlias(const std::string &aliasName);

    /**
     * @brief Get device battery levele.
     *
     * @return Returns device battery levele.
     * @since 6
     */
    NlErrCode GetDeviceBatteryLevel() const;

    /**
     * @brief Get device pair state.
     *
     * @param pairState Returns device pair state.
     * @return Returns the status code for this function called.
     */
    NlErrCode GetPairState(int &pairState) const;

    /**
     * @brief Device start pair.
     *
     * @return Returns the status code for this function called.
     */
    NlErrCode StartPair();

    /**
     * @brief Credible Device start pair.
     *
     * @return Returns the status code for this function called.
     */
    NlErrCode StartCrediblePair();

    /**
     * @brief cancel pairing.
     *
     * @return Returns the status code for this function called.
     */
    NlErrCode CancelDevicePairing();

    /**
     * @brief Set confirmation during pairing.
     *
     * @param cfm confirmation.
     * @return Returns the status code for this function called.
     */
    NlErrCode SetPairingConfirmation(bool cfm);

    /**
     * @brief Set passCode during pairing.
     *
     * @param passCode 通行码.
     * @return Returns the status code for this function called.
     */
    NlErrCode SetPairingPassCode(const std::string &passCode);

    /**
     * @brief Check if device was bonded from local.
     *
     * @return Returns the status code for this function called.
     */
    NlErrCode IsBondedFromLocal(bool &isBondedFromLocal) const;

    /**
     * @brief Check if device acb connected.
     *
     * @return Returns the status code for this function called.
     */
    NlErrCode IsAcbConnected(bool &isAcbConnected) const;

    /**
     * @brief Check if device acb Encrypted.
     *
     * @return Returns the status code for this function called.
     */
    NlErrCode IsAcbEncrypted(bool &isAcbEncrypted) const;

    /**
     * @brief Get the product id of a remote device.
     *
     * @param[out] Returns productId.
     * @return Returns the status code for this function called.
     */
    NlErrCode GetDeviceProductId(uint16_t &productId) const;

    /**
     * @brief Get the vendor id of a remote device.
     *
     * @param[out] Returns vendorId.
     * @return Returns the status code for this function called.
     */
    NlErrCode GetDeviceVendorId(uint16_t &vendorId) const;

    NlErrCode GetDeviceModel(DeviceModel &model) const;

    /**
     * @brief Get the device information of a remote device.
     *
     * @param[out] Returns information.
     * @return Returns the status code for this function called.
     */
    NlErrCode GetDeviceInformation(DeviceInformation &information) const;

    /**
     * @brief Get device uuids, uuid is big endian order.
     *
     * @return Returns the status code for this function called.
     */
    NlErrCode GetDeviceUuids(std::vector<std::string> &uuids) const;

    /**
     * @brief Check device pair request reply.
     *
     * @param accept Set gap accept flag.
     * @return Returns the status code for this function called.
     */
    NlErrCode PairRequestReply(bool accept);

    /**
     * @brief Get device transport type.
     *
     * @return Returns device transport type;
     * @since 6
     */
    int GetTransportType() const;

    /**
     * @brief Sets the interval of the connection between a remote device.
     *
     * @param interval type.
     * @return Returns the status code for this function called.
     */
    NlErrCode UpdateConnectInterval(ConnectionInterval interval) const;

    /**
     * @brief Read remote device rssi value.
     *
    * @return Returns the status code for this function called.
     */
    NlErrCode ReadRemoteRssiValue();

    /**
     * @brief Check if nearlink remote device is valid.
     *
     * @return Returns <b>true</b> if nearlink remote device is valid;
     *         returns <b>false</b> if nearlink remote device is not valid.
     * @since 6
     */
    bool IsValidNearlinkRemoteDevice() const;

    /**
     * @brief Get the product type of the device, such as headsets, watchs and car.
     *
     * @return Returns remote device appearance.
     * @since 11
     */
    NlErrCode GetDeviceAppearance(int &appearance) const;

    /**
     * @brief Get ACB connection state. ACB is Asynchronous Connection-Oriented Bidirectional Link.
     *
     * @return Returns remote device acb connection State.
     * @since 6
     */
    NlErrCode GetAcbState(int &acbState) const;

    /**
     * @brief Get battery level of the remote device.
     *
     * @return Returns the battery level of the remote device as a percentage.
     * @note Requires the caller to be registered as a battery observer; otherwise
     * returns NL_ERR_PERMISSION_FAILED.
     * @since 6
     */
    NlErrCode GetBatteryLevel();

private:
    NearlinkBaseRemoteDevice base_;
};
}  // namespace Nearlink
} // namespace OHOS

#endif  // NEARLINK_REMOTE_DEVICE_H
