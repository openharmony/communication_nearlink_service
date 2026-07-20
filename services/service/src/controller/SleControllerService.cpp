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

#include <chrono>
#include "SleControllerService.h"
#include "qosm_autorate_def.h"
#include "qosm_autorate.h"
#include "qosm_errno.h"
#include "qosm_audio_dfx.h"
#include "log_util.h"
#include "securec.h"
#include "nearlink_def.h"
#include "nearlink_utils.h"
#include "nearlink_sle_controller_def.h"
#include "nlstk_public_define_ext.h"
#include "SleRemoteDeviceAdapter.h"
#include "SleInterfaceManager.h"
#include "SleInterfaceAdapterSub.h"
#include "SleCoexistManager.h"
#include "SleDefs.h"

namespace OHOS {
namespace Nearlink {

namespace {
constexpr uint16_t SLE_CONN_SUPERVISION_TIMEOUT =  1000;     // 超时时间10s
constexpr uint8_t SLE_CONN_INTERVAL_UNIT_DIVISOR_8 =  8;     //  连接interval单位0.125ms，即1/8
constexpr uint8_t SLE_CONN_INTERVAL_SCALE_FACTOR_2 = 2;     //  连接interval计算系数2
constexpr uint8_t SLE_CONN_TIMEOUT_UNIT_10MS = 10;     // 连接链路超时时间单位10ms
constexpr int SLE_HID_COEX_UPDATE_TIMESLOT_10 = 10;     // 海思HID设备interval更新保护间隔10 * interval
constexpr uint16_t SLE_CONN_TIMEOUT_EXTRA_500MS = 500;     // 连接链路超时时间额外增加单位500ms

const std::map<uint16_t, QOSM_AutoBitrate_E> bitRateMap = {
    { static_cast<uint16_t>(SLEBitRate::BITRATE_4600), QOSM_AUTO_BITRATE_4600 },
    { static_cast<uint16_t>(SLEBitRate::BITRATE_2300), QOSM_AUTO_BITRATE_2300 },
    { static_cast<uint16_t>(SLEBitRate::BITRATE_1500), QOSM_AUTO_BITRATE_1500 },
    { static_cast<uint16_t>(SLEBitRate::BITRATE_640),  QOSM_AUTO_BITRATE_640 },
    { static_cast<uint16_t>(SLEBitRate::BITRATE_320),  QOSM_AUTO_BITRATE_320 },
    { static_cast<uint16_t>(SLEBitRate::BITRATE_256),  QOSM_AUTO_BITRATE_256 },
    { static_cast<uint16_t>(SLEBitRate::BITRATE_192),  QOSM_AUTO_BITRATE_192 },
    { static_cast<uint16_t>(SLEBitRate::BITRATE_64),   QOSM_AUTO_BITRATE_64 },
    { static_cast<uint16_t>(SLEBitRate::BITRATE_32),   QOSM_AUTO_BITRATE_32 },
};

// Duty cycle mapping table
const std::map<uint8_t, QOSM_AutoDutyCycle_E> dutyCycleMap = {
    {static_cast<uint8_t>(SledutyCycle::DUTY_CYCLE_100P), QOSM_DUTY_CYCLE_100P},
    {static_cast<uint8_t>(SledutyCycle::DUTY_CYCLE_50P),  QOSM_DUTY_CYCLE_50P},
    {static_cast<uint8_t>(SledutyCycle::DUTY_CYCLE_20P),  QOSM_DUTY_CYCLE_20P},
};
}

InterfaceSleController &InterfaceSleController::GetInstance()
{
    return SleControllerService::GetInstance();
}

SleControllerService &SleControllerService::GetInstance()
{
    static SleControllerService instance;
    return instance;
}

SleControllerService::SleControllerService()
{
    HILOGI("SleControllerService initialized");
}

SleControllerService::~SleControllerService()
{
    HILOGI("SleControllerService destroyed");
}

bool SleControllerService::SetSleCoexParam(uint16_t maxBitRate, uint8_t dutyCycle)
{
    HILOGI("Enter, maxBitRate is %{public}u, dutyCycle is %{public}u", maxBitRate, dutyCycle);
    auto bitRateEnumit = bitRateMap.find(maxBitRate);
    NL_CHECK_RETURN_RET(bitRateEnumit != bitRateMap.end(), false, "Invalid bit rate enum");
    auto dutyCycleEnumit = dutyCycleMap.find(dutyCycle);
    NL_CHECK_RETURN_RET(dutyCycleEnumit != dutyCycleMap.end(), false, "Invalid duty cycle enum");

    QOSM_AutoRateCoexistSuggestionParam param = {};
    param.maxBitrate = static_cast<uint16_t>(bitRateEnumit->second);
    param.dutyCycle = static_cast<uint8_t>(dutyCycleEnumit->second);
    NLSTK_ERRCODE ret = QOSM_AutoRateSetCoexistSuggestion(&param);
    return ret == NLSTK_ERRCODE_SUCCESS;
}

bool SleControllerService::FetchInterval(int32_t intervalType, uint16_t &intervalValue)
{
    // Interval type to value mapping
    static const std::map<int32_t, uint16_t> INTERVAL_MAP = {
        {HIGH_SPEED_INTERVAL_4_5, 0x024},   // HIGH_SPEED_INTERVAL_4_5 (4.5ms)
        {HIGH_SPEED_INTERVAL_4_875, 0x027},   // HIGH_SPEED_INTERVAL_4_875 (4.875ms)
        {MID_SPEED_INTERVAL_11_25, 0x05A},   // MID_SPEED_INTERVAL_11_25 (11.25ms)
        {MID_SPEED_INTERVAL_15, 0x078},   // MID_SPEED_INTERVAL_15 (15ms)
        {MID_SPEED_INTERVAL_50, 0x190},   // MID_SPEED_INTERVAL_50 (50ms)
        {LOW_SPEED_INTERVAL_100, 0x320},   // LOW_SPEED_INTERVAL_100 (100ms)
        {LOW_SPEED_INTERVAL_150, 0x480},   // LOW_SPEED_INTERVAL_150 (150ms)
        {LOW_SPEED_INTERVAL_200, 0x640},   // LOW_SPEED_INTERVAL_200 (200ms)
        {LOW_SPEED_INTERVAL_300, 0x960},   // LOW_SPEED_INTERVAL_300 (300ms)
        {LOW_SPEED_INTERVAL_500, 0xFA0},   // LOW_SPEED_INTERVAL_500 (500ms)
    };

    auto it = INTERVAL_MAP.find(intervalType);
    if (it == INTERVAL_MAP.end()) {
        HILOGE("Invalid interval type: %{public}d", intervalType);
        return false;
    }

    intervalValue = it->second;
    HILOGI("Interval type: %{public}d, value: 0x%{public}x", intervalType, intervalValue);
    return true;
}

bool SleControllerService::GetConnectionParams(
    const std::string &device, uint16_t &intervalValue, CM_ConnectUpdateParamReq_S &updateParam)
{
    RawAddress addr(device);
    uint8_t peerAddrType = SleRemoteDeviceAdapter::GetInstance()->GetPeerDeviceAddrType(addr);
    HILOGI("peerAddrType: 0x%{public}d", peerAddrType);
    updateParam.addr = {peerAddrType, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
    addr.ConvertToUint8(updateParam.addr.addr, SLE_ADDR_LEN);
    updateParam.intervalMin = intervalValue;
    updateParam.intervalMax = intervalValue;
    updateParam.version = 0;
    updateParam.localIndex = 0;
    updateParam.txRxInterval = CM_CONN_EVENT_IFS;
    updateParam.eventInterval = CM_CONN_EVENT_IFS;
    // <超时时间 * 10大于(数据链路时间组周期 * 0.125) * (1 + 最大延迟))>
    updateParam.maxLatency = 0;
    updateParam.supervisionTimeout = SLE_CONN_SUPERVISION_TIMEOUT;
    updateParam.systemTimeUnit = CM_CONN_TIME_UNIT;
    updateParam.txRxFlag = CM_CONN_T_TX_RX_FLAG;

    auto adapter = static_cast<SleInterfaceAdapterSub *>
        (SleInterfaceManager::GetInstance()->GetAdapter(SleTransport::ADAPTER_SLE));
    NL_CHECK_RETURN_RET(adapter, false, "sleAdapter is null");
    adapter->GetConnectionParam(
        device, updateParam.supervisionTimeout, updateParam.maxLatency);

    uint32_t tmpTimeout32 = static_cast<uint32_t>(intervalValue / SLE_CONN_INTERVAL_UNIT_DIVISOR_8 * \
        SLE_CONN_INTERVAL_SCALE_FACTOR_2 * (updateParam.maxLatency + 1));  // 转换到u32进行溢出判断
    if (tmpTimeout32 > UINT16_MAX) { // UINT16_MAX---0xFFFF
        tmpTimeout32 = UINT16_MAX;
    }
    uint16_t tmpTimeout = static_cast<uint16_t>(tmpTimeout32);
    if (updateParam.supervisionTimeout * SLE_CONN_TIMEOUT_UNIT_10MS <= tmpTimeout) {
        updateParam.supervisionTimeout = (tmpTimeout + SLE_CONN_TIMEOUT_EXTRA_500MS) / SLE_CONN_TIMEOUT_UNIT_10MS;
    }
    return true;
}

bool SleControllerService::UpdateConnectInterval(const std::string &device, int32_t intervalType)
{
    uint16_t intervalValue = 0;
    if (!FetchInterval(intervalType, intervalValue)) {
        HILOGE("intervalType not found");
        return false;
    }

    CM_ConnectUpdateParamReq_S updateParam;
    (void)memset_s(&updateParam, sizeof(updateParam), 0x0, sizeof(updateParam));
    if (GetConnectionParams(device, intervalValue, updateParam) == false) {
        return false;
    }

    if (CM_ConnectUpdateParamReq(&updateParam) != NLSTK_ERRCODE_SUCCESS) {
        HILOGE("read remote channel param failed");
        return false;
    }
    HILOGI("address: %{public}s, rel_interval_min: 0x%{public}x, \
        rel_interval_max: 0x%{public}x, rel_timeout: 0x%{public}x, rel_latency: 0x%{public}x", \
        GetEncryptAddr(device).c_str(), updateParam.intervalMin, updateParam.intervalMax, \
        updateParam.supervisionTimeout, updateParam.maxLatency);
    return true;
}

bool SleControllerService::SetSleCoexMode(int32_t mode, const std::vector<std::string> &deviceList,
    const std::vector<ConnectionInterval> &paramList)
{
    if (mode < 0 || mode >= SLE_COEX_MODE_BUTT) {
        HILOGE("invalid sle coex mode: %{public}d", mode);
        return false;
    }
    switch (mode) {
        case static_cast<int32_t>(SLE_HID_COEX_MODE_ENABLE):
            return EnableSleHidCoexMode(deviceList, paramList);
        case static_cast<int32_t>(SLE_HID_COEX_MODE_DISABLE):
            return DisableSleHidCoexMode();
        default:
            HILOGE("Invalid coex mode: %{public}d", mode);
            return false;
    }
    return true;  
}
 
bool SleControllerService::EnableSleHidCoexMode(const std::vector<std::string> &deviceList,
    const std::vector<ConnectionInterval> &paramList)
{
    size_t deviceListSize = deviceList.size();
    NL_CHECK_RETURN_RET(deviceListSize != 0, false, "empty device list");
    NL_CHECK_RETURN_RET(paramList.size() == deviceListSize, false, "deviceList and paramList size mismatch");
    auto adapter = static_cast<SleInterfaceAdapterSub *>
        (SleInterfaceManager::GetInstance()->GetAdapter(SleTransport::ADAPTER_SLE));
    NL_CHECK_RETURN_RET(adapter, false, "sleAdapter is null");
    if (sleHidCoexState_ == SleCoexModeStatus::STARTING || sleHidCoexState_ == SleCoexModeStatus::STOPPING) {
        HILOGE("cannot enable sle hid coex mode, state: %{public}d", static_cast<int>(sleHidCoexState_));
        return false;
    }
 
    SleHidCoexModeParam coexParam = {};
    for (size_t i = 0; i < deviceListSize; i++) {
        uint16_t timeout = 0;
        uint16_t maxLatency = 0;
        uint16_t currentInterval = 0;
        uint16_t coexInterval = 0;
        if (!FetchInterval(paramList[i], coexInterval)) {
            return false;
        }
        adapter->GetConnectionParam(deviceList[i], timeout, maxLatency, currentInterval);
        SleHidCoexDevice coexDevice(deviceList[i], coexInterval, currentInterval);
        coexParam.deviceList.emplace_back(coexDevice);
    }
    HILOGI("change sle hid coex mode state, current state: %{public}d, new state: SleCoexModeStatus::STARTING",
        static_cast<int>(sleHidCoexState_));
    sleHidCoexState_ = SleCoexModeStatus::STARTING;
    coexParam.state = SleCoexModeStatus::STARTING;
    adapter->EnableSleHidCoexMode(coexParam);
    UpdateSleHidCoexIntervalForEach(coexParam.deviceList, 0, SleCoexModeStatus::STARTING);
    return true;
}
 
bool SleControllerService::DisableSleHidCoexMode()
{
    auto adapter = static_cast<SleInterfaceAdapterSub *>
        (SleInterfaceManager::GetInstance()->GetAdapter(SleTransport::ADAPTER_SLE));
    NL_CHECK_RETURN_RET(adapter, false, "sleAdapter is null");
    std::shared_ptr<SleHidCoexModeParam> coexStatus = adapter->GetSleHidCoexModeParam();
    NL_CHECK_RETURN_RET(coexStatus, false, "coexStatus is null");
    if (sleHidCoexState_ == SleCoexModeStatus::STARTING || sleHidCoexState_ == SleCoexModeStatus::STOPPING) {
        HILOGE("cannot enable sle hid coex mode, state: %{public}d", static_cast<int>(sleHidCoexState_));
        return false;
    }
    HILOGI("change sle hid coex mode state, current state: %{public}d, new state: SleCoexModeStatus::STOPPING",
        static_cast<int>(sleHidCoexState_));
    sleHidCoexState_ = SleCoexModeStatus::STOPPING;
    UpdateSleHidCoexIntervalForEach(coexStatus->deviceList, 0, SleCoexModeStatus::STOPPING);
    adapter->DisableSleHidCoexMode();
    return true;
}
 
void SleControllerService::UpdateSleHidCoexIntervalForEach(
    const std::vector<SleHidCoexDevice> &deviceList, size_t index, SleCoexModeStatus state)
{
    NL_CHECK_RETURN(index >= 0 && index <= deviceList.size(), "invalid index: %{public}lu", index);
    NL_CHECK_RETURN(state == SleCoexModeStatus::STARTING || state == SleCoexModeStatus::STOPPING,
        "error, invalid state: %{public}d", static_cast<int>(state));
    auto adapter = static_cast<SleInterfaceAdapterSub *>
        (SleInterfaceManager::GetInstance()->GetAdapter(SleTransport::ADAPTER_SLE));
    NL_CHECK_RETURN(adapter, "sleAdapter is null");
    // 参数更新递归结束，更新共存模式状态，STARTING -> STARTED，STOPPING -> STOPPED
    if (index == deviceList.size()) {
        SleCoexModeStatus currentState = sleHidCoexState_;
        if (state == SleCoexModeStatus::STARTING && currentState == SleCoexModeStatus::STARTING) {
            sleHidCoexState_ = SleCoexModeStatus::STARTED;
            adapter->SetSleHidCoexModeState(SleCoexModeStatus::STARTED);
            sleHidCoexEnableDelayTimer_ = nullptr;
        } else if (state == SleCoexModeStatus::STOPPING && currentState == SleCoexModeStatus::STOPPING) {
            sleHidCoexState_ = SleCoexModeStatus::STOPPED;
            adapter->SetSleHidCoexModeState(SleCoexModeStatus::STOPPED);
            sleHidCoexDisableDelayTimer_ = nullptr;
        }
        HILOGI("update sle hid coex mode finised, current state: %{public}d, next state: %{public}d",
            static_cast<int>(currentState), static_cast<int>(sleHidCoexState_));
        return;
    }
    // 更新当前index下标的设备连接参数，共存使能时更新至共存上限，共存结束时恢复至pending参数
    uint16_t timeout = 0;
    uint16_t maxLatency = 0;
    uint16_t currentInterval = 0;
    uint16_t updateInterval = 0;
    adapter->GetConnectionParam(deviceList[index].addr, timeout, maxLatency, currentInterval);
    if (currentInterval != 0) {
        if (state == SleCoexModeStatus::STARTING && currentInterval < deviceList[index].coexInterval) {
            updateInterval = deviceList[index].coexInterval;
        } else if (state == SleCoexModeStatus::STOPPING && currentInterval != deviceList[index].pendingInterval) {
            updateInterval = deviceList[index].pendingInterval;
        }
    }
    HILOGI("Update sle coex hid interval, addr: %{public}s, index: %{public}lu, curentInt: %{public}d, updateInt: "
        "%{public}d", GetEncryptAddr(deviceList[index].addr).c_str(), index, currentInterval, updateInterval);
    CM_ConnectUpdateParamReq_S updateParam;
    (void)memset_s(&updateParam, sizeof(updateParam), 0x0, sizeof(updateParam));
    if (updateInterval != 0 && !GetConnectionParams(deviceList[index].addr, updateInterval, updateParam)) {
        HILOGW("get connection params failed, addr: %{public}s", GetEncryptAddr(deviceList[index].addr).c_str());
    }
    if (updateInterval != 0 && CM_ConnectUpdateParamReq(&updateParam) != NLSTK_ERRCODE_SUCCESS) {
        HILOGW("update connect param failed, addr: %{public}s", GetEncryptAddr(deviceList[index].addr).c_str());
    }
    // 递归更新下一台设备连接参数，防止参数更新过于频繁，设置参数更新保护时间间隔 = 10 * 当前连接interval + 10 (ms)
    int64_t updateProtectingTime = updateInterval != 0 ? currentInterval * SLE_HID_COEX_UPDATE_TIMESLOT_10 /
        SLE_CONN_INTERVAL_UNIT_DIVISOR_8 + SLE_HID_COEX_UPDATE_TIMESLOT_10 : 0;
    StartSleHidCoexUpdateTimer(deviceList, index + 1, state, updateProtectingTime);
}
 
void SleControllerService::StartSleHidCoexUpdateTimer(const std::vector<SleHidCoexDevice> &deviceList, size_t index,
    SleCoexModeStatus state, int64_t protectTime)
{
    NL_CHECK_RETURN(state == SleCoexModeStatus::STARTING || state == SleCoexModeStatus::STOPPING,
        "error, invalid state: %{public}d", static_cast<int>(state));
    if (protectTime == 0) {
        // 无需设置保护时间间隔，立刻从当前index开始更新剩余设备interval
        UpdateSleHidCoexIntervalForEach(deviceList, index, state);
        return;
    }
    if (state == SleCoexModeStatus::STARTING) {
        if (sleHidCoexEnableDelayTimer_ != nullptr) {
            sleHidCoexEnableDelayTimer_->Stop();
            sleHidCoexEnableDelayTimer_ = nullptr;
        }
        sleHidCoexEnableDelayTimer_ = std::make_shared<NearlinkTimer>([this, deviceList, index, state]() {
            UpdateSleHidCoexIntervalForEach(deviceList, index, state);
        });
        sleHidCoexEnableDelayTimer_->Start(protectTime);
    } else {
        if (sleHidCoexDisableDelayTimer_ != nullptr) {
            sleHidCoexDisableDelayTimer_->Stop();
            sleHidCoexDisableDelayTimer_ = nullptr;
        }
        sleHidCoexDisableDelayTimer_ = std::make_shared<NearlinkTimer>([this, deviceList, index, state]() {
            UpdateSleHidCoexIntervalForEach(deviceList, index, state);
        });
        sleHidCoexDisableDelayTimer_->Start(protectTime);
    }
}

} // namespace Nearlink
} // namespace OHOS
