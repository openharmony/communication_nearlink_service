/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "SleControllerService.h"
#include "qosm_autorate_def.h"
#include "qosm_autorate.h"
#include "qosm_errno.h"
#include "qosm_audio_dfx.h"
#include "log_util.h"
#include "securec.h"
#include "nearlink_def.h"
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
constexpr uint16_t SLE_CONN_TIMEOUT_EXTRA_500MS = 500;     // 连接链路超时时间额外增加单位500ms

const std::map<uint16_t, QOSM_AutoBitrate_T> bitRateMap = {
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
const std::map<uint8_t, QOSM_AutoDutyCycle_T> dutyCycleMap = {
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

} // namespace Nearlink
} // namespace OHOS
