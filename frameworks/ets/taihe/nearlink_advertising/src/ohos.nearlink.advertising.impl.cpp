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

#include "ohos.nearlink.advertising.proj.hpp"
#include "ohos.nearlink.advertising.impl.hpp"
#include "ani_nearlink_advertising_callback.h"
#include "ani_nearlink_error.h"
#include "ani_nearlink_utils.h"
#include "nearlink_sle_advertiser.h"
#include "nearlink_host.h"
#include "log.h"
#include "nearlink_errorcode.h"
#include "nearlink_def.h"
#include "nearlink_utils.h"

#include <cstdint>
#include <vector>

namespace OHOS {
namespace Nearlink {
constexpr size_t ANI_MANUFACTURER_DATA_MAX_NUM = 0xFFFF;
constexpr size_t ANI_SERVICE_DATA_MAX_NUM = 0xFFFF;
constexpr size_t ANI_SERVICE_UUID_MAX_NUM = 0xFFFF;
std::shared_ptr<SleAdvertiser> SleAdvertiserGetInstance(void)
{
    static std::shared_ptr<SleAdvertiser> instance = SleAdvertiser::CreateSleAdvertiser();
    return instance;
}

uint8_t ConvertPowerMode(int32_t powerParam)
{
    uint8_t txPowerMode = static_cast<uint8_t>(SleAdvertiserTxPowerLevel::SLE_ADV_TX_POWER_LOW);
    switch (powerParam) {
        case static_cast<int32_t>(AdvertiseMode::ADV_TX_POWER_LOW):
            txPowerMode = static_cast<uint8_t>(SleAdvertiserTxPowerLevel::SLE_ADV_TX_POWER_LOW);
            break;
        case static_cast<int32_t>(AdvertiseMode::ADV_TX_POWER_MEDIUM):
            txPowerMode = static_cast<uint8_t>(SleAdvertiserTxPowerLevel::SLE_ADV_TX_POWER_MEDIUM);
            break;
        case static_cast<int32_t>(AdvertiseMode::ADV_TX_POWER_HIGH):
            txPowerMode = static_cast<uint8_t>(SleAdvertiserTxPowerLevel::SLE_ADV_TX_POWER_HIGH);
            break;
        default:
            break;
    }
    return txPowerMode;
}

static TaiheStatus ParseAdvertisingSettingsParameters(const ::ohos::nearlink::advertising::AdvertisingSettings &input,
    SleAdvertiserSettings &outSettings)
{
    uint32_t interval = 0;
    if (input.interval.has_value()) {
        interval = static_cast<uint32_t>(input.interval.value());
        if (interval < static_cast<uint32_t>(AdvInterval::ADV_INTERVAL_MIN) ||
            interval > static_cast<uint32_t>(AdvInterval::ADV_INTERVAL_MAX)) {
            HILOGE("Invalid interval: %{public}d", interval);
            HandleSyncErr(NL_ERR_INVALID_INTERGER);
            return TaiheStatus::TAIHE_INVALID_ARG;
        }
        HILOGI("interval: %{public}d", interval);
        outSettings.SetInterval(interval);
    }
    int32_t power = 0;
    if (input.power.has_value()) {
        power = static_cast<int32_t>(input.power.value());
        HILOGI("power: %{public}d", power);
        if (power < static_cast<int32_t>(AdvertiseMode::ADV_TX_POWER_LOW) ||
            power > static_cast<int32_t>(AdvertiseMode::ADV_TX_POWER_HIGH)) {
            HILOGE("Invalid power: %{public}d", power);
            return TaiheStatus::TAIHE_INVALID_ARG;
        }
        outSettings.SetTxPower(ConvertPowerMode(power));
    }
    if (input.isConnectable.has_value()) {
        bool isConnectable = input.isConnectable.value();
        HILOGI("isConnectable: %{public}d", isConnectable);
        outSettings.SetConnectable(isConnectable);
    }
    return TaiheStatus::TAIHE_OK;
}

static TaiheStatus ParseUuidParams(std::string &uuid, UUID &outUuid)
{
    if (!IsValidUuid(uuid)) {
        HILOGE("Invalid uuid");
        HandleSyncErr(NL_ERR_INVALID_UUID);
        return TaiheStatus::TAIHE_INVALID_ARG;
    }
    ConvertUuidToUpperCase(uuid); // UUID统一解析为大写字母形式
    outUuid = UUID::FromString(uuid);
    return TaiheStatus::TAIHE_OK;
}

static TaiheStatus ParseServiceUuidParameters(const ::ohos::nearlink::advertising::AdvertisingData &input,
    SleAdvertiserData &outData)
{
    if (input.serviceUuids.has_value()) {
        TAIHE_NEARLINK_RETURN_IF(input.serviceUuids.value().size() > ANI_SERVICE_UUID_MAX_NUM,
            "Too many service uuids", TaiheStatus::TAIHE_INVALID_ARG);
        for (auto &serviceUuid : input.serviceUuids.value()) {
            std::string id = std::string(serviceUuid);
            UUID uuid {};
            TAIHE_NEARLINK_CALL_RETURN(ParseUuidParams(id, uuid));
            outData.AddServiceUuid(uuid);
        }
    }
    return TaiheStatus::TAIHE_OK;
}

static TaiheStatus ParseManufacturerDataParameters(const ::ohos::nearlink::advertising::AdvertisingData &input,
    SleAdvertiserData &outData)
{
    if (input.manufacturerData.has_value()) {
        TAIHE_NEARLINK_RETURN_IF(input.manufacturerData.value().size() > ANI_MANUFACTURER_DATA_MAX_NUM,
            "Too much manufacturer data", TaiheStatus::TAIHE_INVALID_ARG);
        for (auto &manufacturer : input.manufacturerData.value()) {
            TAIHE_NEARLINK_RETURN_IF(manufacturer.manufacturerId > 0xFFFF,
                "Invalid manufacturerId", TaiheStatus::TAIHE_INVALID_ARG);
            outData.AddManufacturerData(manufacturer.manufacturerId,
                std::string(manufacturer.manufacturerData.begin(), manufacturer.manufacturerData.end()));
        }
    }
    return TaiheStatus::TAIHE_OK;
}

static TaiheStatus ParseServiceDataParameters(const ::ohos::nearlink::advertising::AdvertisingData &input,
    SleAdvertiserData &outData)
{
    if (input.serviceData.has_value()) {
        TAIHE_NEARLINK_RETURN_IF(input.serviceData.value().size() > ANI_SERVICE_DATA_MAX_NUM,
            "Too much service data", TaiheStatus::TAIHE_INVALID_ARG);
        for (auto &service : input.serviceData.value()) {
            std::string id = std::string(service.serviceUuid);
            UUID dataUuid {};
            TAIHE_NEARLINK_CALL_RETURN(ParseUuidParams(id, dataUuid));
            outData.AddServiceData(dataUuid,
                std::string(service.serviceData.begin(), service.serviceData.end()));
        }
    }
    return TaiheStatus::TAIHE_OK;
}

static TaiheStatus ParseAdvertisDataParameters(const ::ohos::nearlink::advertising::AdvertisingData &input,
    SleAdvertiserData &outData)
{
    TAIHE_NEARLINK_CALL_RETURN(ParseServiceUuidParameters(input, outData));
    TAIHE_NEARLINK_CALL_RETURN(ParseManufacturerDataParameters(input, outData));
    TAIHE_NEARLINK_CALL_RETURN(ParseServiceDataParameters(input, outData));

    bool includeDeviceName = false;
    if (input.includeDeviceName.has_value()) {
        includeDeviceName = input.includeDeviceName.value();
    }
    HILOGI("includeDeviceName: %{public}d", includeDeviceName);
    outData.SetIncludeDeviceName(includeDeviceName);
    return TaiheStatus::TAIHE_OK;
}

static TaiheStatus CheckAdvertisingParamWith(const ::ohos::nearlink::advertising::AdvertisingParams &params,
    SleAdvertiserSettings &outSettings, SleAdvertiserData &outAdvData, SleAdvertiserData &outRspData)
{
    SleAdvertiserSettings settings;
    TAIHE_NEARLINK_CALL_RETURN(ParseAdvertisingSettingsParameters(params.advertisingSettings, settings));

    SleAdvertiserData advData;
    TAIHE_NEARLINK_CALL_RETURN(ParseAdvertisDataParameters(params.advertisingData, advData));

    outSettings = std::move(settings);
    outAdvData = std::move(advData);
    return TaiheStatus::TAIHE_OK;
}

uintptr_t StartAdvertising(const ::ohos::nearlink::advertising::AdvertisingParams &params)
{
    HILOGI("enter");
    SleAdvertiserSettings settings;
    SleAdvertiserData advData;
    SleAdvertiserData rspData;
    uint16_t duration = 0;

    TaiheStatus status = CheckAdvertisingParamWith(params, settings, advData, rspData);
    ANI_NL_ASSERT_RETURN(status == TAIHE_OK, NL_ERR_INVALID_PARAM, reinterpret_cast<uintptr_t>(nullptr));
    // 提前校验ACCESS权限
    bool isGranted = false;
    NlErrCode checkResult = NearlinkHost::GetInstance().CheckPermissionForNapi(ACCESS_NEARLINK_PERMISSION, isGranted);
    ANI_NL_ASSERT_RETURN(checkResult == NL_NO_ERROR, checkResult, reinterpret_cast<uintptr_t>(nullptr));
    ANI_NL_ASSERT_RETURN(isGranted, NL_ERR_PERMISSION_FAILED, reinterpret_cast<uintptr_t>(nullptr));

    auto func = [settings, advData, rspData, duration]() {
        auto sleAdvertiser = SleAdvertiserGetInstance();
        if (sleAdvertiser == nullptr) {
            HILOGE("sleAdvertiser is nullptr");
            return TaiheAsyncWorkRet(NL_ERR_INTERNAL_ERROR);
        }
        int ret = sleAdvertiser->StartAdvertising(
            settings, advData, rspData, duration, AniNearlinkAdvertisingCallback::GetInstance());
        return TaiheAsyncWorkRet(ret);
    };
    ani_env *env = taihe::get_env();
    ANI_NL_ASSERT_RETURN(env, NL_ERR_INTERNAL_ERROR, reinterpret_cast<uintptr_t>(nullptr));
    auto asyncWork = TaiheAsyncWorkFactory::CreateAsyncWork(env, nullptr, func);
    ANI_NL_ASSERT_RETURN(asyncWork, NL_ERR_INTERNAL_ERROR, reinterpret_cast<uintptr_t>(nullptr));
    bool success = AniNearlinkAdvertisingCallback::GetInstance()
        ->asyncPromiseMap_.TryPush(TaiheAsyncType::GET_ADVERTISING_HANDLE, asyncWork);
    ANI_NL_ASSERT_RETURN(success, NL_ERR_INTERNAL_ERROR, reinterpret_cast<uintptr_t>(nullptr));
    asyncWork->Run();
    return reinterpret_cast<uintptr_t>(asyncWork->GetRet());
}

void StopAdvertising(int32_t advertisingId)
{
    HILOGI("enter");
    auto sleAdvertiser = SleAdvertiserGetInstance();
    ANI_NL_ASSERT_RETURN_VOID(sleAdvertiser != nullptr, NL_ERR_INTERNAL_ERROR);
    uint8_t advHandle = 0;
    sleAdvertiser->GetAdvHandle(AniNearlinkAdvertisingCallback::GetInstance(), advHandle);
    ANI_NL_ASSERT_RETURN_VOID(advertisingId == advHandle, NL_ERR_INVALID_ADV_ID);

    // 提前校验ACCESS权限
    bool isGranted = false;
    NlErrCode checkResult = NearlinkHost::GetInstance().CheckPermissionForNapi(ACCESS_NEARLINK_PERMISSION, isGranted);
    ANI_NL_ASSERT_RETURN_VOID(checkResult == NL_NO_ERROR, checkResult);
    ANI_NL_ASSERT_RETURN_VOID(isGranted, NL_ERR_PERMISSION_FAILED);

    int ret = sleAdvertiser->StopAdvertising(AniNearlinkAdvertisingCallback::GetInstance());
    ANI_NL_ASSERT_RETURN_VOID(ret == NL_NO_ERROR, ret);
}

void OnAdvertisingStateChange(
    ::taihe::callback_view<void(::ohos::nearlink::advertising::AdvertisingStateChangeInfo const& data)> callback)
{
    HILOGI("enter");
    ANI_NL_ASSERT_RETURN_VOID(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    AniNearlinkAdvertisingCallback::GetInstance()->eventSubscribe_.RegisterEvent<
        void(::ohos::nearlink::advertising::AdvertisingStateChangeInfo const&)>(
            REGISTER_ADVERTISING_STATE_INFO_NAME, callback);
}

void OffAdvertisingStateChange(
    ::taihe::optional_view<
        ::taihe::callback<void(::ohos::nearlink::advertising::AdvertisingStateChangeInfo const& data)>> callback)
{
    HILOGI("enter");
    ANI_NL_ASSERT_RETURN_VOID(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    AniNearlinkAdvertisingCallback::GetInstance()->eventSubscribe_.DeregisterEvent<
        void(::ohos::nearlink::advertising::AdvertisingStateChangeInfo const&)>(
            REGISTER_ADVERTISING_STATE_INFO_NAME, callback);
}
}  // namespace Nearlink
}  // namespace OHOS

// Since these macros are auto-generate, lint will cause false positive.
// NOLINTBEGIN
TH_EXPORT_CPP_API_StartAdvertising(OHOS::Nearlink::StartAdvertising);
TH_EXPORT_CPP_API_StopAdvertising(OHOS::Nearlink::StopAdvertising);
TH_EXPORT_CPP_API_OnAdvertisingStateChange(OHOS::Nearlink::OnAdvertisingStateChange);
TH_EXPORT_CPP_API_OffAdvertisingStateChange(OHOS::Nearlink::OffAdvertisingStateChange);
// NOLINTEND
