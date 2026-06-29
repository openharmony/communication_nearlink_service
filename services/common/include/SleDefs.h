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
#ifndef SLE_DEFS_H
#define SLE_DEFS_H

#include <string>

#include "nearlink_def.h"
#include "sle_uuid.h"
#include "sdf_struct.h"

/*
 * @brief The sle system.
 */
namespace OHOS {
namespace Nearlink {
/// const
constexpr uint8_t SLE_IRK_HEX_ELN = 0x10;
constexpr uint8_t SLE_IRK_RAND_HEX_LEN = 0x03;
constexpr uint8_t SLE_IRK_RAND_ELN = 0x1F;
constexpr uint8_t SLE_IRK_RAND_LEFT_SHIFT = 0x04;
constexpr uint8_t SHORTENED_LOCAL_NAME = 0X08;
constexpr uint8_t COMPLETE_LOCAL_NAME = 0X09;
constexpr uint8_t MANUFACTURER_SPECIFIC_DATA = 0XFF;
constexpr uint8_t TX_POWER_LEVEL = 0x0A;
constexpr uint8_t SERVICE_DATA_16_BIT_UUID = 0X16;
constexpr uint8_t SERVICE_DATA_32_BIT_UUID = 0X20;
constexpr uint8_t SERVICE_DATA_128_BIT_UUID = 0X21;
constexpr uint8_t SLE_DIV_RESULT_TWO = 0x02;

/// Maximum advertising data length that can fit in a legacy PDU.
constexpr uint32_t SLE_LEGACY_ADV_DATA_LEN_MAX = 0xFF;
constexpr uint8_t SLE_LEGACY_SCAN_RSP_DATA_LEN_MAX = 0x1F;
constexpr uint8_t SLE_EX_ADV_PAYLOAD_DATA_LEN = 0XBF;

/// Regular scan params, Unit slot
constexpr uint16_t SLE_SCAN_MODE_LOW_POWER_WINDOW = 900; // 900 * 0.125 = 112.5ms
constexpr uint16_t SLE_SCAN_MODE_LOW_POWER_INTERVAL = 9000; // 9000 * 0.125 = 1125ms
constexpr uint16_t SLE_SCAN_MODE_BALANCED_WINDOW = 1024; // 1024 * 0.125 = 128ms
constexpr uint16_t SLE_SCAN_MODE_BALANCED_INTERVAL = 4096; // 4096 * 0.125 = 512ms
constexpr uint16_t SLE_SCAN_MODE_LOW_LATENCY_WINDOW = 4096; // 4096 * 0.125 = 512ms
constexpr uint16_t SLE_SCAN_MODE_LOW_LATENCY_INTERVAL = 4096; // 4096 * 0.125 = 512ms
constexpr uint16_t SLE_SCAN_MODE_FULL_SCAN_WINDOW = 2048; // 2048 * 0.125 = 256ms
constexpr uint16_t SLE_SCAN_MODE_FULL_SCAN_INTERVAL = 4096; // 4096 * 0.125 = 512ms
constexpr uint16_t SLE_SCAN_MODE_OP_P2_60_3000_WINDOW = 60; // 60 * 0.125 = 7.5ms
constexpr uint16_t SLE_SCAN_MODE_OP_P2_60_3000_INTERVAL = 3000; // 3000 * 0.125 = 375ms
constexpr uint16_t SLE_SCAN_MODE_OP_P10_60_600_WINDOW = 60; // 60 * 0.125 = 7.5ms
constexpr uint16_t SLE_SCAN_MODE_OP_P10_60_600_INTERVAL = 600; // 600 * 0.125 = 75ms
constexpr uint16_t SLE_SCAN_MODE_OP_P25_60_240_WINDOW = 60; // 60 * 0.125 = 7.5ms
constexpr uint16_t SLE_SCAN_MODE_OP_P25_60_240_INTERVAL = 240; // 240 * 0.125 = 30ms
constexpr uint16_t SLE_SCAN_MODE_OP_P50_100_200_WINDOW = 100; // 100 * 0.125 = 12.5ms
constexpr uint16_t SLE_SCAN_MODE_OP_P50_100_200_INTERVAL = 200; // 200 * 0.125 = 25ms
constexpr uint16_t SLE_SCAN_MODE_OP_P50_240_480_WINDOW = 240; // 240 * 0.125 = 30ms
constexpr uint16_t SLE_SCAN_MODE_OP_P50_240_480_INTERVAL = 480; // 480 * 0.125 = 60ms
constexpr uint16_t SLE_SCAN_MODE_OP_P50_480_960_WINDOW = 480; // 480 * 0.125 = 60ms
constexpr uint16_t SLE_SCAN_MODE_OP_P50_480_960_INTERVAL = 960; // 960 * 0.125 = 120ms
constexpr uint16_t SLE_SCAN_MODE_OP_P100_240_240_WINDOW = 240; // 240 * 0.125 = 30ms
constexpr uint16_t SLE_SCAN_MODE_OP_P100_240_240_INTERVAL = 240; // 240 * 0.125 = 30ms
constexpr uint16_t SLE_SCAN_MODE_OP_P100_1000_1000_WINDOW = 1000; // 1000 * 0.125 = 125ms
constexpr uint16_t SLE_SCAN_MODE_OP_P100_1000_1000_INTERVAL = 1000; // 1000 * 0.125 = 125ms

/// Batch scan params, Unit slot
constexpr uint16_t SLE_SCAN_MODE_BATCH_LOW_POWER_WINDOW = 500; // 500 * 0.125 = 62.5ms
constexpr uint16_t SLE_SCAN_MODE_BATCH_LOW_POWER_INTERVAL = 45000; // 45000 * 0.125 = 5625ms
constexpr uint16_t SLE_SCAN_MODE_BATCH_BALANCED_WINDOW = 1000; // 1000 * 0.125 =125ms
constexpr uint16_t SLE_SCAN_MODE_BATCH_BALANCED_INTERVAL = 45000; // 45000 * 0.125 = 5625ms
constexpr uint16_t SLE_SCAN_MODE_BATCH_LOW_LATENCY_WINDOW = 1500; // 1500 * 0.125 = 187.5ms
constexpr uint16_t SLE_SCAN_MODE_BATCH_LOW_LATENCY_INTERVAL = 45000; // 45000 * 0.125 = 5625ms
constexpr uint16_t SLE_SCAN_MODE_BATCH_FULL_SCAN_WINDOW = 1248; // 1248 * 0.125 = 156ms
constexpr uint16_t SLE_SCAN_MODE_BATCH_FULL_SCAN_INTERVAL = 45000; // 45000 * 0.125 = 5625ms
constexpr uint16_t SLE_SCAN_MODE_BATCH_OP_P2_60_3000_WINDOW = 60; // 60 * 0.125 = 7.5ms
constexpr uint16_t SLE_SCAN_MODE_BATCH_OP_P2_60_3000_INTERVAL = 3000; // 3000 * 0.125 = 375ms
constexpr uint16_t SLE_SCAN_MODE_BATCH_OP_P10_60_600_WINDOW = 60; // 60 * 0.125 = 7.5ms
constexpr uint16_t SLE_SCAN_MODE_BATCH_OP_P10_60_600_INTERVAL = 600; // 600 * 0.125 = 75ms
constexpr uint16_t SLE_SCAN_MODE_BATCH_OP_P25_60_240_WINDOW = 60; // 60 * 0.125 = 7.5ms
constexpr uint16_t SLE_SCAN_MODE_BATCH_OP_P25_60_240_INTERVAL = 240; // 240 * 0.125 = 30ms
constexpr uint16_t SLE_SCAN_MODE_BATCH_OP_P50_100_200_WINDOW = 100; // 100 * 0.125 = 12.5ms
constexpr uint16_t SLE_SCAN_MODE_BATCH_OP_P50_100_200_INTERVAL = 200; // 200 * 0.125 = 25ms
constexpr uint16_t SLE_SCAN_MODE_BATCH_OP_P50_240_480_WINDOW = 240; // 240 * 0.125 = 30ms
constexpr uint16_t SLE_SCAN_MODE_BATCH_OP_P50_240_480_INTERVAL = 480; // 480 * 0.125 = 60ms
constexpr uint16_t SLE_SCAN_MODE_BATCH_OP_P50_480_960_WINDOW = 480; // 480 * 0.125 = 60ms
constexpr uint16_t SLE_SCAN_MODE_BATCH_OP_P50_480_960_INTERVAL = 960; // 960 * 0.125 = 120ms
constexpr uint16_t SLE_SCAN_MODE_BATCH_OP_P100_240_240_WINDOW = 240; // 240 * 0.125 = 30ms
constexpr uint16_t SLE_SCAN_MODE_BATCH_OP_P100_240_240_INTERVAL = 240; // 240 * 0.125 = 30ms
constexpr uint16_t SLE_SCAN_MODE_BATCH_OP_P100_1000_1000_WINDOW = 1000; // 1000 * 0.125 = 125ms
constexpr uint16_t SLE_SCAN_MODE_BATCH_OP_P100_1000_1000_INTERVAL = 1000; // 1000 * 0.125 = 125ms

// Sle Scan params
constexpr float SLE_SCAN_UNIT_TIME = 0.125;
constexpr uint16_t SLE_SCAN_MODE_DEFAULT_WINDOW_MS = 0x960;
constexpr uint16_t SLE_SCAN_MODE_DEFAULT_INTERVAL_MS = 0x12C0;

/// Invalid MAC Address
const std::string SLE_INVALID_MAC_ADDRESS = "00:00:00:00:00:00";
const std::string SLE_DEFAULT_DEVICE_NAME = "SleDevice";
constexpr uint8_t SLE_DEFAULT_IO = 0x01;

constexpr uint8_t SLE_NON_RES_PRI_ADDR = 0x3F;
constexpr uint8_t SLE_STATIC_PRI_ADDR = 0xC0;

constexpr int BTM_ACB_DISCONNECT_REASON = 0x13;
constexpr int SLE_THREAD_WAIT_TIMEOUT = 5;


const std::string SLE_PENCIL_ADVERTSTING_DATA1 = "0319C7030F16EEFD";
const std::string SLE_PENCIL_ADVERTSTING_DATA2 = "1209485541574549204D2D50656E63696C2033";

constexpr int SLE_CHANGE_RPA_ADDRESS_INTERVAL = 15 * 60 * 1000;
constexpr uint16_t GATT_UUID_GAP_DEVICE_NAME = 0x2A00;

const Uuid::UUID128Bit DEFAULT_UUID_MASK = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

constexpr int CM_CONN_INITIATE_PHYS = 1;
constexpr int CM_CONN_SCAN_PRIVATE_WINDOW = 0x1E0; // 0x1E0 * 0.125 = 60ms
constexpr int CM_CONN_SCAN_PRIVATE_INTERVAL = 0x960; // 0x960 * 0.125 = 300ms
constexpr int CM_CONN_PRIVATE_MIN_INTERVAL = 0x64; // 0x64 * 0.125 = 12.5ms
constexpr int CM_CONN_PRIVATE_MAX_INTERVAL = 0x64;
constexpr int CM_CONN_PRIVATE_TIMEOUT = 0xC8;
constexpr int CM_CONN_MAX_LATENCY = 0x1F3;
constexpr int CM_CONN_LATENCY = 0x0;
constexpr int CM_CONN_DEFAULT_LATENCY = 7;

#ifdef TV_STANDARD
constexpr int CM_CONN_COEXIST_INTERAL_THRED = 0x10;
#else
constexpr int CM_CONN_COEXIST_INTERAL_THRED = 0x20;
#endif
constexpr int CM_CONN_COEXIST_INTERAL = 0x20;
constexpr int CM_CONN_EVENT_IFS = 125;
constexpr int CM_CONN_TIME_UNIT = 4;

constexpr int CM_CONN_T_TX_RX_FLAG = 1;

constexpr int CM_CONN_UPDATE_DURATION = 3000; // ms

constexpr uint16_t CM_CONN_ASC_INTERVAL = 160;
constexpr uint8_t CM_CONN_ASC_EVENT_IFS = 125;
constexpr uint16_t CM_CONN_ASC_EVENT_EFS = 100;
constexpr uint16_t CM_CONN_ASC_SUPERVISION_TIMEOUT = 500; // 超时时间5000ms
constexpr uint8_t CM_CONN_ASC_TIME_UNIT = 4;

// asc connected state
constexpr int SLE_ASC_STATE_CONNECTED = 1;

// max hadm register
constexpr uint32_t SLE_HADM_REGISTER_MAX_NUM = 5;

/// Filter status
enum class FILTER_STATUS : int {
    SLE_SCAN_FILTER_STATUS_IDLE = 0,
    SLE_SCAN_FILTER_STATUS_WORKING,
    SLE_SCAN_FILTER_STATUS_BAD,
};

/// Advertising status
enum class ADVERTISE_STATUS : int {
    NOT_STARTED = 0,
    STARTTING,
    STARTED,
    DISABLING,
    DISABLED,
    ENABLING,
    ENABLED,
    REMOVING,
    UPDATING,
};

/// Local config
enum class SLE_CONFIG : int {
    SLE_CONFIG_LOCAL_NAME = 0x00,
    SLE_CONFIG_LOCAL_ADDRESS = 0x01,
    SLE_CONFIG_ADAPTER_BONDED_MODE = 0x02,
};

const int SLE_DD_ADV_PARAM_SET_COMPLETE_EVT = 0x00;
const int SLE_DD_ADV_DATA_SET_COMPLETE_EVT = 0x01;
const int SLE_DD_ADV_SCAN_RSP_DATA_SET_COMPLETE_EVT = 0x02;
const int SLE_DD_ADV_ENABLE_COMPLETE_EVT = 0x03;
const int SLE_DD_ADV_DISABLE_COMPLETE_EVT = 0x04;
const int SLE_DD_ADV_REMOVE_COMPLETE_EVT = 0x05;
const int SLE_DD_ADV_CLEAR_COMPLETE_EVT = 0x06;
const int SLE_DD_ADV_TERMINATED_COMPLETE_EVT = 0x07;

const int SLE_GAP_SCAN_PARAM_SET_COMPLETE_EVT = 0x00;
const int SLE_GAP_SCAN_START_COMPLETE_EVT = 0x01;
const int SLE_GAP_SCAN_STOP_COMPLETE_EVT = 0x02;

constexpr int SLE_ADAPTER_PROF_CONN_STATE_UNUSED = 0;
constexpr int SLE_ADAPTER_PROF_CONN_STATE_ACB_CONNECTING = 1;
constexpr int SLE_ADAPTER_PROF_CONN_STATE_SSAP_CONNECTING = 2;
constexpr int SLE_ADAPTER_PROF_CONN_STATE_DISCOVERING = 3;
constexpr int SLE_ADAPTER_PROF_CONN_STATE_PROFILE_CONNECTING = 4;
constexpr int SLE_ADAPTER_PROF_CONN_STATE_CONNECTED = 5;
constexpr int SLE_ADAPTER_PROF_CONN_STATE_DISCONNECTING = 6;
constexpr int SLE_ADAPTER_PROF_CONN_STATE_WAIT_DISCONNECTED = 7;
constexpr int SLE_RSSI_CHANGE_FLAG = 256;

/// Scan callback type
using CALLBACK_TYPE = enum { CALLBACK_TYPE_ALL_MATCHES = 0x00, CALLBACK_TYPE_FIRST_MATCH = 0x01 };

using STOP_SCAN_TYPE = enum { STOP_SCAN_TYPE_NOR = 0x00, STOP_SCAN_TYPE_RESOLVING_LIST = 0x01 };

/// Scan status
using SCAN_STATUS = enum {
    SCAN_NOT_STARTED = -1,
    SCAN_SUCCESS = 0,
    SCAN_FAILED_ALREADY_STARTED,
    SCAN_FAILED_APPLICATION_REGISTRATION_FAILED,
    SCAN_FAILED_INTERNAL_ERROR,
    SCAN_FAILED_FEATURE_UNSUPPORTED,
    SCAN_FAILED_OUT_OF_HARDWARE_RESOURCES
};

/// Scan state
using SCAN_STATE = enum {
    SCAN_STOPED = 0,
    SCAN_STARTED
};

/// Features supported by the controller.
/// NEARLINK SPECIFICATION Version 5.0 | Vol 6, Part B - 4.6
using FEATURE_SUPPORTED = enum {
    SLE_ENCRYPTION = 0,
    CONNECTION_PARAMETERS_REQUEST_PROCEDURE,
    EXTENDED_REJECT_INDICATION,
    TERMINAL_INITIATED_FEATURES_EXCHANGE,
    SLE_PING,
    SLE_DATA_PACKET_LENGTH_EXTENSION,
    LL_PRIVACY,
    EXTENDED_SCANNER_FILTER_POLICIES,
    SLE_2M_PHY,
    STABLE_MODULATION_INDEX_TRANSMITTER,
    STABLE_MODULATION_INDEX_RECEIVER,
    SLE_CODED_PHY,
    SLE_EXTENDED_ADVERTISING,
    SLE_PERIODIC_ADVERTISING,
    CHANNEL_SELECTION_ALGORITHM_2,
    SLE_POWER_CLASS
};

/// sle scan mode
using SLE_SCAN_MODE = enum {
    SLE_SCAN_MODE_NON_DISC = 0x00,  /// Non Discoverable
    SLE_SCAN_MODE_GENERAL = 0x01,   /// General
    SLE_SCAN_MODE_PRIOR = 0x02,   /// Prior
    SLE_SCAN_MODE_PAIRED = 0x03,    /// Paired
    SLE_SCAN_MODE_LIMITED = 0x04,    /// Limited
};

/**
 * @brief 扫描过滤策略
 */
enum class SLE_SCAN_FILTER : int {
    SLE_SCAN_FLT_BASIC_NONE          = 0x00,     /*!< 基本不过滤的扫描过滤策略 */
    SLE_SCAN_FLT_BASIC               = 0x01,     /*!< 基本过滤的扫描过滤策略 */
    SLE_SCAN_FLT_EXTEND_NONE         = 0x02,     /*!< 扩展不过滤的扫描过滤策略 */
    SLE_SCAN_FLT_EXTEND              = 0x03,     /*!< 扩展过滤的扫描过滤策略 */
};

/**
 * @brief Sle scan duplicate type
 */
using SLE_SCAN_DUPLICATE = enum {
    SLE_SCAN_DUPLICATE_DISABLE =
        0x00,  // the Link Layer should generate advertising reports to the host for each packet received
    SLE_SCAN_DUPLICATE_ENABLE = 0x01,  /// the Link Layer should filter out duplicate advertising reports to the Host
    SLE_SCAN_DUPLICATE_MAX = 0x02,     /// 0x02 – 0xFF, Reserved for future use
};

/**
 * @brief  扫描带宽
 */
using SLE_SCAN_PHY = enum {
    SLE_SCAN_PHY_1M             = 0x01,
    SLE_SCAN_PHY_2M             = 0x02,
    SLE_SCAN_PHY_4M             = 0x04,
};

/**
 * @brief  扫描类型
 */
using SLE_SCAN_TYPE = enum {
    SLE_SCAN_TYPE_PASSIVE       = 0x00,
    SLE_SCAN_TYPE_ACTIVE        = 0x01,
};

/**
 * @brief 单PHY扫描参数
 */
using SleScanCoreParams = struct {
    SLE_SCAN_PHY scanPhy;
    SLE_SCAN_TYPE scanType;
    uint16_t scanInterval;
    uint16_t scanWindow;
};

const int SCAN_PHY_MAX_NUM = 3;
const int SCAN_PHY_NUM_DEFAULT = 1;
const int SCAN_DURATION_TIME = 10000; // ms
/**
 * @brief 扫描参数
 */
using SleScanParams = struct {
    SLE_ADDR_TYPE ownAddrType;
    SLE_SCAN_FILTER scanFilterPolicy;
    uint8_t frameType;
    uint8_t phyCount;
    SleScanCoreParams param[SCAN_PHY_MAX_NUM];
};

using SLE_ADV_FILTER = enum {
    ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY = 0x00,  /// Allow both scan and connection requests from anyone
    ADV_FILTER_ALLOW_SCAN_WLST_CON_ANY,   /// Allow both scan req from WH List devices and connection req fro anyone
    ADV_FILTER_ALLOW_SCAN_ANY_CON_WLST,   /// Allow both scan req from anyone and connection req from WH List devices
    ADV_FILTER_ALLOW_SCAN_WLST_CON_WLST,  /// Allow scan and connection requests from WH List devices
};

/// Advertising channel mask
using SLE_ADV_CHANNEL = enum {
    ADV_CHNL_37 = 0x01,
    ADV_CHNL_38 = 0x02,
    ADV_CHNL_39 = 0x04,
    ADV_CHNL_ALL = 0x07,
};

/// Advertising mode
using SLE_ADV_TYPE = enum {
    ADV_TYPE_NONCONN_NONSCAN = 0x00,       /*!< 不可连接不可扫描 */
    ADV_TYPE_CONNECTABLE_NONSCAN = 0x01,   /*!< 可连接不可扫描 */
    ADV_TYPE_NONCONN_SCANABLE = 0x02,      /*!< 不可连接可扫描 */
    ADV_TYPE_CONNECTABLE_SCANABLE = 0x03,  /*!< 可连接可扫描 */
};

/// Scan Event Type
using SLE_SCAN_EVENT_TYPE = enum {
    SCAN_CONNECTABLE_ADV_IND = 0x00,
    SCAN_SCANABLE_ADV_IND = 0x01,
    SCAN_DIRECT_ADV_IND = 0x02,
    SCAN_SCAN_RSP_ADV_IND = 0x03,
    SCAN_FIRST_CHANNEL_ADV_IND = 0x04,
};

using SLE_ADV_LEVEL_TYPE = enum {
    ADV_TYPE_NON_DISC = 0x00, // 不可见发现
    ADV_TYPE_GENERAL_DISC = 0x01, // 一般可发现
    ADV_TYPE_PRIOR_DISC = 0x02, // 优先可发现
    ADV_TYPE_PAIRED_DISC = 0x03, // 被配对过设备发现
    ADV_TYPE_LIMITED_DISC = 0x04, // 被指定设备发现
};

/// Advertising parameters
using SleAdvParams = struct {
    uint32_t advMinInterval;  /// Minimum advertising interval for
    /// undirected and low duty cycle directed advertising.
    /// Range: 0x0000A0 to 0xFFFFFF Default: N = 0x001388 (625ms)
    /// ime = N * 0.625 msec Time, Min: 20 ms, Default: 625ms
    uint32_t advMaxInterval;  /// Maximum advertising interval for
    uint8_t advMode;            /// Advertising type
    SLE_Addr_S ownAddr;             /// Owner nearlink device address
    SLE_ADDR_TYPE ownAddrType;       /// Owner nearlink device address type
    SLE_Addr_S peerAddr;                 /// Peer device nearlink device address
    SLE_ADDR_TYPE peerAddrType;      /// Peer device nearlink device address type
    SLE_ADV_CHANNEL channelMap;      /// Advertising channel map
    SLE_ADV_FILTER advFilterPolicy;  /// Advertising filter policy
    uint8_t linkRole;   /// link GT role
    uint8_t primaryFrameType;
};
}  // namespace Sle
}  // namespace OHOS
#endif // SLE_DEFS_H
