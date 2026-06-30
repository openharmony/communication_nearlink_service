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
#include "HidHostStackAdapter.h"
#include <map>
#include "log_util.h"
#include "icce_utils.h"
#include "hid_client_api.h"
#include "HidHostService.h"
#include "HidHostDefines.h"

namespace OHOS {
namespace Nearlink {
HidPropertyType_E ConvertToStackPropertyType(uint8_t reportType)
{
    switch (reportType) {
        case HID_HOST_INPUT_REPORT:
            return HID_INPUT_REPORT_INFO;
        case HID_HOST_OUTPUT_REPORT:
            return HID_OUTPUT_REPORT_INFO;
        case HID_HOST_FEATURE_REPORT:
            return HID_FEATURE_REPORT_INFO;
        default:
            return HID_PROPERTY_TYPE_BUFF;
    }
}

HidReportInfo ConvertToServiceReportInfo(SLE_Addr_S *addr, HidReportInfo_S *reportInfo)
{
    HidReportInfo serReportInfo = {};
    NL_CHECK_RETURN_RET(reportInfo != nullptr, serReportInfo, "[HID Adapter] reportInfo is null.");
    serReportInfo.dev_ = ConvertSleAddrToRawAddress(addr);
    serReportInfo.reportId_ = reportInfo->reportIdAndType.reportId;
    serReportInfo.reportType_ = reportInfo->reportIdAndType.reportType;
    uint16_t dataLength = reportInfo->reportInfoValue.len;
    serReportInfo.dataLength_ = dataLength;
    NL_CHECK_RETURN_RET(reportInfo->reportInfoValue.data != nullptr && dataLength != 0,
        serReportInfo, "[HID Adapter] data is invalid");
    serReportInfo.data_ = std::make_unique<uint8_t[]>(dataLength);
    if (memcpy_s(serReportInfo.data_.get(), dataLength, reportInfo->reportInfoValue.data, dataLength) != EOK) {
        LOG_ERROR("[HID Adapter] memcpy error");
    }
    return serReportInfo;
}

int HidHostStackAdapter::RegisterCallbackToStack()
{
    HidClientCallBack_S cb = {};
    cb.connectStateChangeCbk = &HidConnectStateChangeCbk;
    cb.notifyPropertyCbk = &HidNotifyPropertyCbk;
    cb.readPropertyCbk = &HidReadPropertyCbk;
    cb.writePropertyCbk = &HidWritePropertyCbk;

    NLSTK_Errcode_E ret = HidRegClientCbk(&cb);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        HILOGE("[HID Adapter] register callback to stack failed, ret(%{public}d).", ret);
        return HID_HOST_FAILURE;
    }
    return HID_HOST_SUCCESS;
}

int HidHostStackAdapter::Connect(const RawAddress &addr)
{
    SLE_Addr_S stackAddr = ConvertToStackAddr(addr);
    NLSTK_Errcode_E ret = HidConnect(&stackAddr);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        HILOGE("[HID Adapter] Connect failed, ret(%{public}d)", ret);
        return HID_HOST_FAILURE;
    }
    return HID_HOST_SUCCESS;
}

int HidHostStackAdapter::Disconnect(const RawAddress &addr)
{
    SLE_Addr_S stackAddr = ConvertToStackAddr(addr);
    NLSTK_Errcode_E ret = HidDisconnect(&stackAddr);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        HILOGE("[HID Adapter] Disconnect failed, ret(%{public}d)", ret);
        return HID_HOST_FAILURE;
    }
    return HID_HOST_SUCCESS;
}

int HidHostStackAdapter::SendReport(const HidReportInfo &reportInfo)
{
    NL_CHECK_RETURN_RET(reportInfo.dataLength_ > 1, static_cast<int>(ReturnValue::RET_BAD_STATUS),
        "[HID Adapter] data length is error");
    SLE_Addr_S stackAddr = ConvertToStackAddr(reportInfo.dev_);
    HidPropertyType_E stackPropertyType = ConvertToStackPropertyType(reportInfo.reportType_);
    // Delete report id from the data
    HidReportInfo_S stackReportInfo = {
        .reportIdAndType = {reportInfo.reportId_, reportInfo.reportType_},
        .reportInfoValue = {reportInfo.dataLength_ - 1, reportInfo.data_.get() + 1}
    };
    LOG_INFO("[HID Adapter] value_.size() %{public}d", reportInfo.dataLength_ - 1);
    NLSTK_Errcode_E ret = HidWriteProperty(&stackAddr, stackPropertyType, &stackReportInfo);
    NL_CHECK_RETURN_RET(ret == NLSTK_ERRCODE_SUCCESS, static_cast<int>(ReturnValue::RET_BAD_STATUS),
        "[HID Adapter] SendReport failed, ret(%{public}d)", ret);
    return static_cast<int>(ReturnValue::RET_NO_ERROR);
}

int HidHostStackAdapter::SendGetReport(const HidReportInfo &reportInfo)
{
    SLE_Addr_S stackAddr = ConvertToStackAddr(reportInfo.dev_);
    HidPropertyType_E stackPropertyType = ConvertToStackPropertyType(reportInfo.reportType_);
    HidReportIdAndType_S stackRptIdAndType = {reportInfo.reportId_, reportInfo.reportType_};
    NLSTK_Errcode_E ret = HidReadProperty(&stackAddr, stackPropertyType, &stackRptIdAndType);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        HILOGE("[HID Adapter] SendGetReport failed, ret(%{public}d)", ret);
        return HID_HOST_FAILURE;
    }
    return static_cast<int>(ReturnValue::RET_NO_ERROR);
}

HidInformation HidHostStackAdapter::GetRemoteHidInfo(const RawAddress &addr)
{
    HidInformation_S *info = nullptr;
    HidFreeFunc freeFunc = nullptr;
    SLE_Addr_S stackAddr = ConvertToStackAddr(addr);
    uint32_t result = HidGetInformation(&stackAddr, &info, &freeFunc);
    HidInformation hidInfo = {};
    NL_CHECK_RETURN_RET(info != nullptr, hidInfo, "[HID Adapter] info is nullptr.");
    if (result != NLSTK_ERRCODE_SUCCESS) {
        HILOGE("[HID Adapter] GetRemoteHidInfo failed, ret(%{public}d)", result);
        if (freeFunc != nullptr) {
            freeFunc(info);
        }
        return hidInfo;
    }
    if (info->desc.type == 0 && info->desc.desc != nullptr) {
        hidInfo.descLength = info->desc.descLen;
        hidInfo.descInfo = std::make_unique<uint8_t[]>(hidInfo.descLength);
        if (memcpy_s(hidInfo.descInfo.get(), hidInfo.descLength,
            info->desc.desc, hidInfo.descLength) != EOK) {
            LOG_ERROR("[HID Adapter] memcpy error");
        }
    }
    if (freeFunc != nullptr) {
        freeFunc(info);
    }
    return hidInfo;
}

int HidHostStackAdapter::GetDeviceState(const RawAddress &addr)
{
    SLE_Addr_S stackAddr = ConvertToStackAddr(addr);
    uint8_t state = HID_DISCONNECTED;
    NLSTK_Errcode_E ret = HidGetConnectState(&stackAddr, &state);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        HILOGE("[HID Adapter] GetDeviceState failed, ret(%{public}d)", ret);
        return static_cast<int>(SleConnectState::DISCONNECTED);
    }
    return state;
}

std::list<RawAddress> HidHostStackAdapter::GetConnectDevices()
{
    std::list<RawAddress> deviceList;
    SLE_Addr_S* addrs = nullptr;
    size_t num = 0;
    HidFreeFunc freeFunc = nullptr;
    NLSTK_Errcode_E ret = HidGetConnectedDevice(&addrs, &num, &freeFunc);
    NL_CHECK_RETURN_RET(addrs != nullptr, deviceList, "[HID Adapter] addrs is nullptr");
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        HILOGE("[HID Adapter] GetConnectDevices failed, ret(%{public}d)", ret);
        if (freeFunc != nullptr) {
            freeFunc(addrs);
        }
    }
    for (size_t index = 0; index < num; index++) {
        RawAddress device = ConvertSleAddrToRawAddress(&(addrs[index]));
        deviceList.push_back(device);
    }
    if (freeFunc != nullptr) {
        freeFunc(addrs);
    }
    return deviceList;
}

void HidHostStackAdapter::HidConnectStateChangeCbk(SLE_Addr_S *addr, HidConnectState_E state,
    HidConnectState_E preState, NLSTK_Errcode_E ret)
{
    NL_CHECK_RETURN(addr, "[HID Adapter] addr is null.");
    RawAddress device = ConvertSleAddrToRawAddress(addr);
    HILOGI("[HID Adapter] addr(%{public}s), state(%{public}d), preState(%{public}d)",
        GET_ENCRYPT_ADDR(device), state, preState);
    HidHostService *hidHostService = HidHostService::GetService();
    NL_CHECK_RETURN(hidHostService, "[HID Adapter] hidHostService is nullptr.");
    hidHostService->NotifyStateChanged(device, state, preState, static_cast<int32_t>(ret));
}

void HidHostStackAdapter::HidNotifyPropertyCbk(SLE_Addr_S *addr, HidPropertyType_E type, void *value)
{
    NL_CHECK_RETURN(addr, "[HID Adapter] addr is null.");
    RawAddress device = ConvertSleAddrToRawAddress(addr);
    NL_CHECK_RETURN(type == HID_INPUT_REPORT_INFO, "[HID Adapter] addr(%{public}s), type(%{public}d)",
        GET_ENCRYPT_ADDR(device), type);
    NL_CHECK_RETURN(value != nullptr, "[HID Adapter] value is nullptr");
    HidReportInfo_S* stackReportInfo = (HidReportInfo_S*)value;
    HidReportInfo serReportInfo = ConvertToServiceReportInfo(addr, stackReportInfo);
    HidHostService *hidHostService = HidHostService::GetService();
    NL_CHECK_RETURN(hidHostService, "[HID Adapter] hidHostService is nullptr.");
    hidHostService->SendData(serReportInfo);
}

void HidHostStackAdapter::HidReadPropertyCbk(SLE_Addr_S *addr, HidPropertyType_E type, void *value,
    NLSTK_Errcode_E ret)
{
    NL_CHECK_RETURN(addr, "[HID Adapter] addr is null.");
    RawAddress device = ConvertSleAddrToRawAddress(addr);
    HILOGI("[HID Adapter] addr(%{public}s), type(%{public}d), ret(%{public}d)", GET_ENCRYPT_ADDR(device), type, ret);
    if (type != HID_INPUT_REPORT_INFO && type != HID_OUTPUT_REPORT_INFO) {
        return;
    }
    HidHostService *hidHostService = HidHostService::GetService();
    NL_CHECK_RETURN(hidHostService, "[HID Adapter] hidHostService is nullptr.");
    if (ret == NLSTK_ERRCODE_SUCCESS) {
        HidReportInfo_S* stackReportInfo = (HidReportInfo_S*)value;
        HidReportInfo serReportInfo = ConvertToServiceReportInfo(addr, stackReportInfo);
        hidHostService->ReceiveControlData(serReportInfo);
    } else {
        LOG_ERROR("[HID Adapter] get report error");
        hidHostService->ReceiveHandShake(device, HID_HOST_HANDSHAKE_ERROR);
    }
}

void HidHostStackAdapter::HidWritePropertyCbk(SLE_Addr_S *addr, HidPropertyType_E type,
    NLSTK_Errcode_E ret)
{
    NL_CHECK_RETURN(addr, "[HID Adapter] addr is null.");
    RawAddress device = ConvertSleAddrToRawAddress(addr);
    HILOGI("[HID Adapter] addr(%{public}s), type(%{public}d), ret(%{public}d)", GET_ENCRYPT_ADDR(device), type, ret);
    if (type != HID_INPUT_REPORT_INFO && type != HID_OUTPUT_REPORT_INFO) {
        return;
    }
    uint16_t err = (ret == NLSTK_ERRCODE_SUCCESS) ? static_cast<uint16_t>(HID_HOST_SUCCESS) : HID_HOST_HANDSHAKE_ERROR;
    HidHostService *hidHostService = HidHostService::GetService();
    NL_CHECK_RETURN(hidHostService, "[HID Adapter] hidHostService is nullptr.");
    hidHostService->ReceiveHandShake(device, err);
}
}
}