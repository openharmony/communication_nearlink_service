/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#ifndef SLE_ADVERTISER_ADAPTER_H
#define SLE_ADVERTISER_ADAPTER_H

#include "SleAdvertiserImpl.h"

namespace OHOS {
namespace Nearlink {

/**
 *  @brief SLE scan adpter implementation class
 */
class SleAdvertiserAdapter : public InterfaceAdvertiserService {
public:
    SleAdvertiserAdapter();
    ~SleAdvertiserAdapter();

    static SleAdvertiserAdapter &GetInstance();
    void RegisterSleAdvertiserCallback(std::weak_ptr<ISleAdvertiserCallback> callback) override;
    void RegisterSleConnectableAdvertiserCallback(std::weak_ptr<ISleAdvertiserCallback> callback) override;
    void DeregisterSleAdvertiserCallback() const override;
    uint8_t GetAdvertiserHandle() const override;
    uint8_t GetConnectableAdvertiserHandle() const override;
    int GetAdvertisingStatus() const override;
    void StartAdvertising(const SleAdvertiserSettingsImpl &settings, const SleAdvertiserDataImpl &advData,
        const SleAdvertiserDataImpl &scanResponse, uint8_t advHandle) override;
    void SetAdvertisingData(const SleAdvertiserDataImpl &advData, const SleAdvertiserDataImpl &scanResponse,
        uint8_t advHandle) const override;
    void StopAdvertising(uint8_t advHandle) const override;
    void StopAdvertisingAll() override;
    void EnableAdvertising(int32_t advHandle) const override;
    void DisableAdvertising(int32_t advHandle) const override;

private:
    SLE_DISALLOW_COPY_AND_ASSIGN(SleAdvertiserAdapter);
    DECLARE_IMPL();
};
}  // namespace Nearlink
}  // namespace OHOS

#endif  // SLE_ADVERTISER_ADAPTER_H