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
#ifndef INTERFACE_ADVERTISER_SERVICE_H
#define INTERFACE_ADVERTISER_SERVICE_H

#include "SleMultiScanData.h"
#include "nearlink_sle_scan_settings.h"
#include "nearlink_sle_scan_filter.h"

namespace OHOS {
namespace Nearlink {
/**
 * @brief Represents advertise callbacks.
 *
 * @since 6
 */
class ISleAdvertiserCallback {
public:
    virtual ~ISleAdvertiserCallback() = default;
    virtual void OnStartResultEvent(int result, uint8_t advHandle) = 0;
    virtual void OnAutoStopAdvEvent(uint8_t advHandle) = 0;
    virtual void OnEnableResultEvent(int result, uint8_t advHandle) = 0;
    virtual void OnDisableResultEvent(int result, uint8_t advHandle) = 0;
    virtual void OnStopResultEvent(int result, uint8_t advHandle) = 0;
    virtual void OnSetAdvDataEvent(int result, uint8_t advHandle) = 0;
};

class InterfaceAdvertiserService {
public:
    virtual ~InterfaceAdvertiserService() = default;
    static InterfaceAdvertiserService &GetInstance();

    /**
     * @brief Register advertiser callback.
     *
     * @param callback Class IBleAdvertiseCallback pointer to register callback.
     * @since 6
     */
    virtual void RegisterSleAdvertiserCallback(std::weak_ptr<ISleAdvertiserCallback> callback) = 0;
    virtual void RegisterSleConnectableAdvertiserCallback(std::weak_ptr<ISleAdvertiserCallback> callback) = 0;

    /**
     * @brief Deregister advertiser callback.
     *
     * @since 6
     */
    virtual void DeregisterSleAdvertiserCallback() const = 0;

    /**
     * @brief Get advertiser id.
     *
     * @return Returns advertiser handle.
     * @since 6
     */
    virtual uint8_t GetAdvertiserHandle() const = 0;
    virtual uint8_t GetConnectableAdvertiserHandle() const = 0;

    /**
     * @brief Get advertiser status.
     *
     * @return Returns advertiser status.
     * @since 6
     */
    virtual int GetAdvertisingStatus() const = 0;

    /**
     * @brief Start advertising.
     *
     * @param settings Advertise settings.
     * @param advData Advertise data.
     * @param scanResponse Scan response data
     * @param advHandle Advertise handle
     * @since 6
     */
    virtual void StartAdvertising(const SleAdvertiserSettingsImpl &settings, const SleAdvertiserDataImpl &advData,
        const SleAdvertiserDataImpl &scanResponse, uint8_t advHandle) = 0;

    virtual void SetAdvertisingData(const SleAdvertiserDataImpl &advData, const SleAdvertiserDataImpl &scanResponse,
        uint8_t advHandle) const = 0;

    /**
     * @brief Stop advertising.
     *
     * @param advHandle Advertise handle
     * @since 6
     */
    virtual void StopAdvertising(uint8_t advHandle) const = 0;

    virtual void StopAdvertisingAll() = 0;

    virtual void EnableAdvertising(int32_t advHandle) const = 0;

    virtual void DisableAdvertising(int32_t advHandle) const = 0;
};
} // namespace Nearlink
} // namespace OHOS
#endif // INTERFACE_ADVERTISER_SERVICE_H