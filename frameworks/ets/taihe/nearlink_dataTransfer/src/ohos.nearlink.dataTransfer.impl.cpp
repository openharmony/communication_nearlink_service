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

#include <memory>
#include <string>
#include "ohos.nearlink.dataTransfer.proj.hpp"
#include "ohos.nearlink.dataTransfer.impl.hpp"
#include "ani_nearlink_datatransfer_callback.h"
#include "ani_nearlink_error.h"
#include "nearlink_errorcode.h"
#include "nearlink_host.h"
#include "nearlink_sle_datatransfer.h"
#include "nearlink_utils.h"
#include "ani_nearlink_utils.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
namespace {
constexpr size_t UUID_128_BIT_LENGTH = 36;
constexpr size_t BASE_UUID_LENGTH = 32;
const std::string BASE_UUID_PREFIX = "37BEA880-FC70-11EA-B720-00000000";

enum class TransferMode {
    BASIC = 0,         // datatransfer basic mode
    RELIABLE = 1,      // datatransfer reliable mode
};

std::shared_ptr<SleDataTransfer> SleDataTransferGetInstance(void)
{
    static std::shared_ptr<SleDataTransfer> instance = SleDataTransfer::CreateSleDataTransfer();
    return instance;
}

bool CheckBaseUuid(const std::string &uuid)
{
    if (uuid.length() != UUID_128_BIT_LENGTH) {
        return false;
    }
    std::string upperUuid = uuid;
    ConvertUuidToUpperCase(upperUuid);
    return upperUuid.substr(0, BASE_UUID_LENGTH) == BASE_UUID_PREFIX;
}

bool CheckPortUuid(std::string &uuid)
{
    ConvertUuidToUpperCase(uuid);
    if (uuid == "060D") {
        return true;
    }
    if (!IsValidUuid(uuid)) {
        HILOGE("Invalid UUID format");
        HandleSyncErr(NL_ERR_INVALID_UUID);
        return false;
    }
    if (CheckBaseUuid(uuid)) {
        HILOGE("Standard UUID not allowed");
        HandleSyncErr(NL_ERR_STANDARD_UUID_NOT_ALLOWED);
        return false;
    }
    return true;
}

bool CheckAddressAndUuid(std::string &address, std::string &uuid)
{
    if (!IsValidAddress(address)) {
        HILOGE("Invalid address");
        HandleSyncErr(NL_ERR_INVALID_ADDRESS);
        return false;
    }
    if (!CheckPortUuid(uuid)) {
        HILOGE("Invalid uuid");
        return false;
    }
    return true;
}

static TaiheStatus ParseTransMode(const ::ohos::nearlink::dataTransfer::ConnectionParams &params, uint8_t &outTransMode)
{
    int32_t transMode = 0;
    if (params.transferMode.has_value()) {
        transMode = static_cast<int32_t>(params.transferMode.value());
    }
    if (transMode == static_cast<int32_t>(TransferMode::BASIC)) {
        HILOGI("transMode: BASIC");
        outTransMode = static_cast<uint8_t>(ConnectionParams::PortTransMode::TRANSPORT_MODE_BASIC);
    } else if (transMode == static_cast<int32_t>(TransferMode::RELIABLE)) {
        HILOGI("transMode: RELIABLE");
        outTransMode = static_cast<uint8_t>(ConnectionParams::PortTransMode::TRANSPORT_MODE_RELIABLE);
    } else {
        HILOGE("Invalid transMode: %{public}d", transMode);
        // 非 TransferMode 枚举值，属入参类型错误，抛 401
        HandleSyncErr(NL_ERR_INVALID_PARAM);
        return TaiheStatus::TAIHE_INVALID_ARG;
    }
    return TaiheStatus::TAIHE_OK;
}
} // namespace

void CreatePort(const taihe::string_view &uuid)
{
    HILOGI("enter");
    std::string uuidString(uuid);
    ANI_NL_ASSERT_RETURN_VOID(CheckPortUuid(uuidString), NL_ERR_INVALID_PARAM);

    auto sleDataTransfer = SleDataTransferGetInstance();
    ANI_NL_ASSERT_RETURN_VOID(sleDataTransfer != nullptr, NL_ERR_INTERNAL_ERROR);
    NlErrCode err = sleDataTransfer->CreatePort(uuidString, AniNearlinkDataTransferCallback::GetInstance());
    ANI_NL_ASSERT_RETURN_VOID(err == NL_NO_ERROR, err);
}

void DestroyPort(const taihe::string_view &uuid)
{
    HILOGI("enter");
    std::string uuidString(uuid);
    ANI_NL_ASSERT_RETURN_VOID(CheckPortUuid(uuidString), NL_ERR_INVALID_PARAM);

    auto sleDataTransfer = SleDataTransferGetInstance();
    ANI_NL_ASSERT_RETURN_VOID(sleDataTransfer != nullptr, NL_ERR_INTERNAL_ERROR);
    NlErrCode err = sleDataTransfer->DestroyPort(uuidString);
    ANI_NL_ASSERT_RETURN_VOID(err == NL_NO_ERROR, err);
}

void Connect(const ::ohos::nearlink::dataTransfer::ConnectionParams &params)
{
    HILOGI("enter");
    std::string address = std::string(params.address);
    std::string uuid = std::string(params.uuid);
    ANI_NL_ASSERT_RETURN_VOID(CheckAddressAndUuid(address, uuid), NL_ERR_INVALID_PARAM);

    auto sleDataTransfer = SleDataTransferGetInstance();
    ANI_NL_ASSERT_RETURN_VOID(sleDataTransfer != nullptr, NL_ERR_INTERNAL_ERROR);

    ConnectionParams nativeParams;
    nativeParams.SetAddress(address);
    nativeParams.SetUuid(uuid);
    uint8_t transMode = 0;
    TaiheStatus status = ParseTransMode(params, transMode);
    ANI_NL_ASSERT_RETURN_VOID(status == TAIHE_OK, NL_ERR_INVALID_PARAM);
    nativeParams.SetTransMode(transMode);

    NlErrCode err = sleDataTransfer->Connect(nativeParams);
    ANI_NL_ASSERT_RETURN_VOID(err == NL_NO_ERROR, err);
}

void Disconnect(const ::ohos::nearlink::dataTransfer::ConnectionParams &params)
{
    HILOGI("enter");
    std::string address = std::string(params.address);
    std::string uuid = std::string(params.uuid);
    ANI_NL_ASSERT_RETURN_VOID(CheckAddressAndUuid(address, uuid), NL_ERR_INVALID_PARAM);

    auto sleDataTransfer = SleDataTransferGetInstance();
    ANI_NL_ASSERT_RETURN_VOID(sleDataTransfer != nullptr, NL_ERR_INTERNAL_ERROR);

    ConnectionParams nativeParams;
    nativeParams.SetAddress(address);
    nativeParams.SetUuid(uuid);
    uint8_t transMode = 0;
    TaiheStatus status = ParseTransMode(params, transMode);
    ANI_NL_ASSERT_RETURN_VOID(status == TAIHE_OK, NL_ERR_INVALID_PARAM);
    nativeParams.SetTransMode(transMode);

    NlErrCode err = sleDataTransfer->Disconnect(nativeParams);
    ANI_NL_ASSERT_RETURN_VOID(err == NL_NO_ERROR, err);
}

::ohos::nearlink::constant::ConnectionState GetConnectionState(
    const ::ohos::nearlink::dataTransfer::ConnectionStateParams &params)
{
    HILOGI("enter");
    std::string address = std::string(params.address);
    std::string uuid = std::string(params.uuid);
    ANI_NL_ASSERT_RETURN(CheckAddressAndUuid(address, uuid), NL_ERR_INVALID_PARAM, 
        ::ohos::nearlink::constant::ConnectionState::key_t::STATE_DISCONNECTED);

    auto sleDataTransfer = SleDataTransferGetInstance();
    ANI_NL_ASSERT_RETURN(sleDataTransfer != nullptr, NL_ERR_INTERNAL_ERROR, 
        ::ohos::nearlink::constant::ConnectionState::key_t::STATE_DISCONNECTED);

    ConnStateParams nativeParams;
    nativeParams.SetAddress(address);
    nativeParams.SetUuid(uuid);

    int32_t connState = 0;
    NlErrCode err = sleDataTransfer->GetConnectionState(nativeParams, connState);
    ANI_NL_ASSERT_RETURN(err == NL_NO_ERROR, err,
        ::ohos::nearlink::constant::ConnectionState::key_t::STATE_DISCONNECTED);
    return ohos::nearlink::constant::ConnectionState::from_value(connState);
}

void WriteData(const ::ohos::nearlink::dataTransfer::DataParams &params)
{
    HILOGI("enter");
    std::string address = std::string(params.address);
    std::string uuid = std::string(params.uuid);
    ANI_NL_ASSERT_RETURN_VOID(CheckAddressAndUuid(address, uuid), NL_ERR_INVALID_PARAM);

    auto sleDataTransfer = SleDataTransferGetInstance();
    ANI_NL_ASSERT_RETURN_VOID(sleDataTransfer != nullptr, NL_ERR_INTERNAL_ERROR);

    DataParams nativeParams(address, uuid);
    nativeParams.SetAddress(address);
    nativeParams.SetUuid(uuid);
    nativeParams.SetData(params.data.data(), params.data.size());

    NlErrCode err = sleDataTransfer->WriteData(nativeParams);
    ANI_NL_ASSERT_RETURN_VOID(err == NL_NO_ERROR, err);
}

void OnConnectionStateChanged(
    ::taihe::callback_view<void(::ohos::nearlink::dataTransfer::ConnectionResult const&)> callback)
{
    HILOGI("enter");
    ANI_NL_ASSERT_RETURN_VOID(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    AniNearlinkDataTransferCallback::GetInstance()->connectionStateChangedEvent_.RegisterEvent(callback);
}

void OffConnectionStateChanged(
    ::taihe::optional_view<::taihe::callback<void(::ohos::nearlink::dataTransfer::ConnectionResult const&)>> callback)
{
    HILOGI("enter");
    ANI_NL_ASSERT_RETURN_VOID(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    AniNearlinkDataTransferCallback::GetInstance()->connectionStateChangedEvent_.DeregisterEvent(callback);
}

void OnReadData(::taihe::callback_view<void(::ohos::nearlink::dataTransfer::DataParams const&)> callback)
{
    HILOGI("enter");
    ANI_NL_ASSERT_RETURN_VOID(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    AniNearlinkDataTransferCallback::GetInstance()->readDataEvent_.RegisterEvent(callback);
}

void OffReadData(
    ::taihe::optional_view<::taihe::callback<void(::ohos::nearlink::dataTransfer::DataParams const&)>> callback)
{
    HILOGI("enter");
    ANI_NL_ASSERT_RETURN_VOID(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    AniNearlinkDataTransferCallback::GetInstance()->readDataEvent_.DeregisterEvent(callback);
}
}  // namespace Nearlink
}  // namespace OHOS

// Since these macros are auto-generate, lint will cause false positive.
// NOLINTBEGIN
TH_EXPORT_CPP_API_CreatePort(OHOS::Nearlink::CreatePort);
TH_EXPORT_CPP_API_DestroyPort(OHOS::Nearlink::DestroyPort);
TH_EXPORT_CPP_API_Connect(OHOS::Nearlink::Connect);
TH_EXPORT_CPP_API_Disconnect(OHOS::Nearlink::Disconnect);
TH_EXPORT_CPP_API_WriteData(OHOS::Nearlink::WriteData);
TH_EXPORT_CPP_API_OnConnectionStateChanged(OHOS::Nearlink::OnConnectionStateChanged);
TH_EXPORT_CPP_API_OffConnectionStateChanged(OHOS::Nearlink::OffConnectionStateChanged);
TH_EXPORT_CPP_API_OnReadData(OHOS::Nearlink::OnReadData);
TH_EXPORT_CPP_API_OffReadData(OHOS::Nearlink::OffReadData);
TH_EXPORT_CPP_API_GetConnectionState(OHOS::Nearlink::GetConnectionState);
// NOLINTEND
