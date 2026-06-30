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
#include "BasClientStackAdapter.h"
#include "BasService.h"
#include "nearlink_dft_device_data.h"
#include "nearlink_dft_exception.h"
#include "icce_utils.h"
#include "DeviceBatteryManager.h"
#include "nlstk_public_define.h"
#include "bas_def.h"

namespace OHOS {
namespace Nearlink {
namespace{
constexpr int BAS_SUCCESS = 0;
constexpr int BAS_FAILURE = 1;
constexpr uint8_t BAS_MAX_CONNECTION_NUM = 8;
}
BasClientStackAdapter::BasClientStackAdapter() = default;

BasClientStackAdapter::~BasClientStackAdapter() = default;

int BasClientStackAdapter::Connect(const RawAddress &addr)
{
    HILOGI("[BAS Adapter] Connect device addr(%{public}s)", GET_ENCRYPT_ADDR(addr));
    SLE_Addr_S stackAddr = ConvertToStackAddr(addr);
    uint8_t num = 0;
    NLSTK_Errcode_E res = NLSTK_GetConnectedBasDeviceNum(&num);
    if (res != NLSTK_ERRCODE_SUCCESS || num >= BAS_MAX_CONNECTION_NUM) {
        DftReportPairInfo(addr.GetAddress(), PAIR_CONN_PATH_BAS, BAS_FAILURE, "BAS Connect reach max");
        HILOGE("[BAS Adapter] Get connected device num failed, res(%{public}d), num(%{public}d)", res, num);
        return BAS_FAILURE;
    }
    NLSTK_Errcode_E ret = NLSTK_BasProfileConnect(&stackAddr);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        HILOGE("[BAS Adapter] Connect failed, ret(%{public}d)", ret);
        return BAS_FAILURE;
    }
    return BAS_SUCCESS;
}

int BasClientStackAdapter::Disconnect(const RawAddress &addr)
{
    SLE_Addr_S stackAddr = ConvertToStackAddr(addr);
    NLSTK_Errcode_E ret = NLSTK_BasProfileDisconnect(&stackAddr);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        HILOGE("[BAS Adapter] Disconnect failed, ret(%{public}d)", ret);
        return BAS_FAILURE;
    }
    return BAS_SUCCESS;
}

void BasClientStackAdapter::GetDeviceBatteryLevel(const RawAddress &addr)
{
    SLE_Addr_S stackAddr = ConvertToStackAddr(addr);
    NLSTK_Errcode_E ret = NLSTK_GetBatteryLevel(&stackAddr);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        HILOGE("[BAS Adapter] Get Device Battery Level failed, ret(%{public}d)", ret);
    }
}

void BasClientStackAdapter::OnBasGetPropertyCbk(
    SLE_Addr_S *addr, BasPropertyType_E type, void *value, NLSTK_Errcode_E ret)
{
    NL_CHECK_RETURN(addr, "[BAS Adapter] addr is null.");
    int8_t batteryLevel = -1;
    RawAddress device = ConvertSleAddrToRawAddress(addr);
    BasService *basService = BasService::GetInstance();
    NL_CHECK_RETURN(basService, "[BAS Adapter] basService is nullptr.");
    if (!value) {
        HILOGW("[BAS Adapter] battery is null.");
        basService->NotifyBatteryLevelEvent(device, batteryLevel);
        return;
    }
    if (type == BAS_BATTERY_PERCENTAGE && ret == NLSTK_ERRCODE_SUCCESS) {
        NLSTK_VariableData_S *stackReportInfo = (NLSTK_VariableData_S *)value;
        if (stackReportInfo->data) {
            batteryLevel = *stackReportInfo->data;
        }
        HILOGI("[BAS Adapter] addr(%{public}s), type(%{public}d), ret(%{public}d), batteryLevel(%{public}d)",
            GET_ENCRYPT_ADDR(device), type, ret, batteryLevel);
        NL_CHECK_RETURN(basService, "[BAS Adapter] basService is nullptr.");
        basService->NotifyBatteryLevelEvent(device, batteryLevel);
    }
}

void BasClientStackAdapter::OnBasPropertyChangedCbk(SLE_Addr_S *addr, BasPropertyType_E type, void *value)
{
    NL_CHECK_RETURN(addr, "[BAS Adapter] addr is null.");
    NL_CHECK_RETURN(value, "[BAS Adapter] value is null.");
    if (type == BAS_BATTERY_PERCENTAGE) {
        RawAddress device = ConvertSleAddrToRawAddress(addr);
        NLSTK_VariableData_S *stackReportInfo = (NLSTK_VariableData_S *)value;
        NL_CHECK_RETURN(stackReportInfo->data, "[BAS Adapter] batteryData is null.");
        int8_t batteryLevel = *stackReportInfo->data;
        HILOGI("[BAS Adapter] addr(%{public}s), type(%{public}d), batteryLevel(%{public}d)",
            GET_ENCRYPT_ADDR(device), type, batteryLevel);
        BasService *basService = BasService::GetInstance();
        NL_CHECK_RETURN(basService, "[BAS Adapter] basService is nullptr.");
        basService->NotifyBatteryLevelChanged(device, batteryLevel);
    }
}

void BasClientStackAdapter::OnBasConnectStateChangeCbk(
    SLE_Addr_S *addr, NLSTK_BasConnectState_E curState, NLSTK_BasConnectState_E preState, NLSTK_Errcode_E ret)
{
    NL_CHECK_RETURN(addr, "addr is null.");
    RawAddress device = RawAddress::ConvertToString(addr->addr);
    HILOGI("[BAS Adapter] addr(%{public}s), curState(%{public}d), preState(%{public}d), ret(%{public}d)",
        GET_ENCRYPT_ADDR(device), curState, preState, ret);
    if (preState == BAS_CONNECTING && curState == BAS_DISCONNECTED) {
        DftReportPairInfo(device.GetAddress(), PAIR_CONN_PATH_BAS, ret);
    }
    BasService *basService = BasService::GetInstance();
    NL_CHECK_RETURN(basService, "basService is null.");
    basService->NotifyStateChanged(
        device, static_cast<SleConnectState>(curState), static_cast<SleConnectState>(preState));
}

int BasClientStackAdapter::RegisterCallBackToStack()
{
    BasClientCallBack_S cb = {};
    cb.readPropertyCbk = &OnBasGetPropertyCbk;
    cb.connectStateChangeCbk = &OnBasConnectStateChangeCbk;
    cb.propertyChangedCbk = &OnBasPropertyChangedCbk;
    NLSTK_Errcode_E ret = NLSTK_BasRegisterCallBack(&cb);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        HILOGE("[BAS Adapter] register callback to stack failed, ret(%{public}d).", ret);
        return BAS_FAILURE;
    }
    return BAS_SUCCESS;
}

}  // namespace Nearlink
}  // namespace OHOS