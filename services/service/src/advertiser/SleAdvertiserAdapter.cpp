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
#include "SleAdvertiserAdapter.h"
#include "ThreadUtil.h"
#include "SleServiceFfrtLog.h"
#include "ipc_skeleton.h"

namespace OHOS {
namespace Nearlink {
namespace {
constexpr int STOP_ALL_ADV_WAIT_TIMEOUT_MS = 2100;  // 2100ms
}

struct SleAdvertiserAdapter::impl {
    impl();
    impl(const impl &);
    impl &operator=(const impl &);
    ~impl();

    std::unique_ptr<SleAdvertiserImpl> sleAdvertiser_ = nullptr;
};

SleAdvertiserAdapter::impl::impl()
{
    sleAdvertiser_ = std::make_unique<SleAdvertiserImpl>();
}

SleAdvertiserAdapter::impl::~impl()
{}

SleAdvertiserAdapter::SleAdvertiserAdapter(): pimpl(std::make_unique<SleAdvertiserAdapter::impl>())
{
    HILOGI("Create SleAdvertiserAdapter");
}

SleAdvertiserAdapter::~SleAdvertiserAdapter()
{
    HILOGI("~SleAdvertiserAdapter");
    pimpl->sleAdvertiser_ = nullptr;
}

InterfaceAdvertiserService &InterfaceAdvertiserService::GetInstance()
{
    return SleAdvertiserAdapter::GetInstance();
}

SleAdvertiserAdapter &SleAdvertiserAdapter::GetInstance()
{
    // C++11 static local variable initialization is thread-safe.
    static SleAdvertiserAdapter sleAdvertiserAdapter;
    return sleAdvertiserAdapter;
}

void SleAdvertiserAdapter::RegisterSleAdvertiserCallback(std::weak_ptr<ISleAdvertiserCallback> callback)
{
    LOG_DEBUG("enter");
    if (pimpl->sleAdvertiser_ != nullptr) {
        pimpl->sleAdvertiser_->RegisterSleAdvertiserCallback(callback);
    }
}

void SleAdvertiserAdapter::RegisterSleConnectableAdvertiserCallback(std::weak_ptr<ISleAdvertiserCallback> callback)
{
    LOG_DEBUG("enter");
    if (pimpl->sleAdvertiser_ != nullptr) {
        pimpl->sleAdvertiser_->RegisterSleConnectableAdvertiserCallback(callback);
    }
}

void SleAdvertiserAdapter::DeregisterSleAdvertiserCallback() const
{
    LOG_DEBUG("enter");
    DoInAdvThread([this]() -> void {
        if (pimpl->sleAdvertiser_ != nullptr) {
            pimpl->sleAdvertiser_->DeregisterCallbackToDd();
        }
    });
}

uint8_t SleAdvertiserAdapter::GetAdvertiserHandle() const
{
    LOG_DEBUG("enter");
    std::promise<uint8_t> promise;
    std::future<uint8_t> future = promise.get_future();
    DoInAdvThread([this, &promise]() -> void {
        if (pimpl->sleAdvertiser_ != nullptr) {
            uint8_t handle = pimpl->sleAdvertiser_->CreateAdvertiserSetHandle();
            promise.set_value(handle);
            return;
        }
        promise.set_value(static_cast<uint8_t>(SleAdvertisingHandle::SLE_INVALID_ADVERTISING_HANDLE));
    });
    return future.get();
}

uint8_t SleAdvertiserAdapter::GetConnectableAdvertiserHandle() const
{
    LOG_DEBUG("enter");
    std::promise<int> promise;
    std::future<int> future = promise.get_future();
    DoInAdvThread([this, &promise]() -> void {
        if (pimpl->sleAdvertiser_ != nullptr) {
            int status = pimpl->sleAdvertiser_->CreateConnectableAdvertiserSetHandle();
            promise.set_value(status);
            return;
        }
        promise.set_value(static_cast<uint8_t>(SleAdvertisingHandle::SLE_INVALID_ADVERTISING_HANDLE));
    });
    return future.get();
}

int SleAdvertiserAdapter::GetAdvertisingStatus() const
{
    LOG_DEBUG("enter");
    std::promise<int> promise;
    std::future<int> future = promise.get_future();
    DoInAdvThread([this, &promise]() -> void {
        if (pimpl->sleAdvertiser_ != nullptr) {
            int status = pimpl->sleAdvertiser_->GetAdvertisingStatus();
            promise.set_value(status);
            return;
        }
        promise.set_value(static_cast<int>(SleAdvState::SLE_ADV_STATE_IDLE));
    });
    return future.get();
}

void SleAdvertiserAdapter::StartAdvertising(const SleAdvertiserSettingsImpl &settings,
    const SleAdvertiserDataImpl &advData, const SleAdvertiserDataImpl &scanResponse, uint8_t advHandle)
{
    DoInAdvThread([this, settings, advData, scanResponse, advHandle]() -> void {
        if (pimpl->sleAdvertiser_ != nullptr) {
            pimpl->sleAdvertiser_->StartAdvertising(settings, advData, scanResponse, advHandle);
        }
    });
}

void SleAdvertiserAdapter::SetAdvertisingData(const SleAdvertiserDataImpl &advData,
    const SleAdvertiserDataImpl &scanResponse, uint8_t advHandle) const
{
    DoInAdvThread([this, advData, scanResponse, advHandle]() -> void {
        if (pimpl->sleAdvertiser_ != nullptr) {
            pimpl->sleAdvertiser_->SetAdvertisingData(advData, scanResponse, advHandle);
        }
    });
}

void SleAdvertiserAdapter::StopAdvertising(uint8_t advHandle) const
{
    DoInAdvThread([this, advHandle]() -> void {
        if (pimpl->sleAdvertiser_ != nullptr) {
            pimpl->sleAdvertiser_->StopAdvertising(advHandle);
        }
    });
}

void SleAdvertiserAdapter::StopAdvertisingAll()
{
    std::shared_ptr<std::promise<void>> stopAllAdvPromise = std::make_shared<std::promise<void>>();
    std::future<void> future = stopAllAdvPromise->get_future();
    DoInAdvThread([this, &stopAllAdvPromise]() -> void {
        if (!pimpl->sleAdvertiser_) {
            HILOGE("sleAdvertiser_ is null.");
            stopAllAdvPromise->set_value();
            return;
        }
        pimpl->sleAdvertiser_->StopAdvertisingAll(stopAllAdvPromise);
    });
    HILOGI("waiting for stop all adv....");
    auto status = future.wait_for(std::chrono::milliseconds(STOP_ALL_ADV_WAIT_TIMEOUT_MS));
    if (status != std::future_status::ready) {
        HILOGE("wait stop all adv finished timeout");
        return;
    }
    HILOGI("all adv stopped");
    future.get();
}

void SleAdvertiserAdapter::EnableAdvertising(int32_t advHandle) const
{
    DoInAdvThread([this, advHandle]() -> void {
        if (pimpl->sleAdvertiser_ != nullptr) {
            pimpl->sleAdvertiser_->EnableAdvertising(advHandle);
        }
    });
}

void SleAdvertiserAdapter::DisableAdvertising(int32_t advHandle) const
{
    DoInAdvThread([this, advHandle]() -> void {
        if (pimpl->sleAdvertiser_ != nullptr) {
            pimpl->sleAdvertiser_->DisableAdvertising(advHandle);
        }
    });
}
}  // namespace Nearlink
}  // namespace OHOS