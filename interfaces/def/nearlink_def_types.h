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

#ifndef NEARLINK_DEF_TYPES_H
#define NEARLINK_DEF_TYPES_H

#include <cstdint>
#include <string>

namespace OHOS {
namespace Nearlink {

constexpr uint8_t SLE_MANU_ABILITY_LEN = 16;

constexpr uint8_t SLE_ADV_DEVICE_APPEARANCE_LEN = 3;

enum class AdvInterval : int {
    ADV_INTERVAL_MIN = 0x0000A0,
    ADV_SLE_CONNECTABLE_ADV_INTERBAL = 0x0004B0,
    ADV_INTERVAL_MAX = 0xFFFFFF,
    ADV_INTERVAL_DEFAULT = 0x001388
};

enum class SleAdvertiserPrimaryFrameType : uint8_t {
    SLE_ADV_PRI_FRAME_TYPE_1 = 0x00,
    SLE_ADV_PRI_FRAME_TYPE_4 = 0x01,
};

enum class SleAdvPhy : uint8_t {
    SLE_ADVERTISEMENT_PHY_NONE = 0x00,
    SLE_ADVERTISEMENT_PHY_1M = 0x01,
    SLE_ADVERTISEMENT_PHY_2M = 0x02,
    SLE_ADVERTISEMENT_PHY_CODED = 0x03
};

enum class SleLinkRole : uint8_t {
    T_CAN_NEGO = 0,
    G_CAN_NEGO,
    T_NO_NEGO,
    G_NO_NEGO
};

enum class SleAdvertiserFlag : uint8_t {
    SLE_ADV_FLAG_NON_DISC = 0x00,
    SLE_ADV_FLAG_GENERAL_DISC = 0x01,
    SLE_ADV_FLAG_PRIOR_DISC = 0x02,
    SLE_ADV_FLAG_PAIRED_DISC = 0x03,
    SLE_ADV_FLAG_LIMITED_DISC = 0x04
};

enum class SLE_ADDR_TYPE : uint8_t {
    SLE_PUBLIC_ADDRESS_TYPE = 0x0,
    SLE_LONG_MECH_PUBLIC_ADDRESS_TYPE,
    SLE_LOCAL_MECH_PUBLIC_ADDRESS_TYPE,
    SLE_RPA_RESOLV_ADDRESS_TYPE,
    SLE_RPA_UNRESOLV_ADDRESS_TYPE,
    SLE_RESERVE_ADDRESS_TYPE,
    SLE_RANDOM_ADDRESS_TYPE,
    SLE_ADDR_TYPE_END
};

enum class SleConnDirect : int {
    SLE_CONNECTION_PASSIVE = 0x00,
    SLE_CONNECTION_ACTIVE = 0x01
};

enum class SleConnState : int {
    SLE_CONNECTION_STATE_DISCONNECTED = 0x00,
    SLE_CONNECTION_STATE_CONNECTING = 0x01,
    SLE_CONNECTION_STATE_DISCONNECTING = 0x02,
    SLE_CONNECTION_STATE_CONNECTED = 0x03,
    SLE_CONNECTION_STATE_ENCRYPTED = 0x04
};

enum class SleScanFrameType : uint8_t {
    SLE_SCAN_FRAME_TYPE_1 = 0x01,
    SLE_SCAN_FRAME_TYPE_4 = 0x02,
};

enum class SleConnFrameType : uint8_t {
    SLE_CONN_FRAME_TYPE_1 = 0,
    SLE_CONN_FRAME_TYPE_2 = 1,
    SLE_CONN_FRAME_TYPE_3 = 2,
    SLE_CONN_FRAME_TYPE_4 = 3,
    SLE_CONN_FRAME_INVALID,
};

constexpr int8_t RSSI_DEFAULT_THRESHOLD = -128;

enum SsapTransportType : uint8_t {
    SSAP_TRANSPORT_SLB,
    SSAP_TRANSPORT_SLE,
    SSAP_TRANSPORT_MAX,
    SSAP_TRANSPORT_INVALID = 0xFF
};

const std::string INVALID_MAC_ADDRESS = "00:00:00:00:00:00";

enum SleTransport {
    ADAPTER_SLB = 0,
    ADAPTER_SLE,
};

enum SleStateID {
    STATE_TURNING_ON,
    STATE_TURN_ON,
    STATE_TURNING_OFF,
    STATE_TURN_OFF,
    STATE_TURNING_HALF_TO_OFF,
    STATE_TURNING_OFF_TO_HALF,
    STATE_TURNING_HALF_TO_ON,
    STATE_TURNING_ON_TO_HALF,
    STATE_TURN_HALF,
};

enum class SleEventType : int {
    INTERFACE_TRIGGERED = 0,
    AIRPLANE_TRIGGERED,
    COLLABORATION_TRIGGERED,
    RESTORE_TRIGGERED,
    CHIP_RESET_TRIGGERED,
    SELF_HEALING_RETRY,
    SVC_TRIGGERED,
    SYS_STOP_TRIGGERED,
    SYS_RESET_TRIGGERED,
    SYS_FACTORY_RESET_TRIGGERED,
};

enum class SleConnectState : int {
    CONNECTING,
    CONNECTED,
    DISCONNECTING,
    DISCONNECTED,
    INVALID_STATE
};

} // namespace Nearlink
} // namespace OHOS

#endif
