/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef NEARLINK_DEF_H
#define NEARLINK_DEF_H

#include <cstdlib>
#include <string>
#include <vector>
#include "nearlink_def_types.h"

namespace OHOS {
namespace Nearlink {
// Defined here are various status codes
/*********************************************
 *
 * Interface Return Value Macro Define
 *
 *********************************************/
enum class ReturnValue : int {
    RET_NO_ERROR = 0,
    RET_NO_SUPPORT = -1,
    RET_BAD_PARAM = -2,
    RET_BAD_STATUS = -3,
    RET_NO_SPACE = -4,
    RET_IS_DISCONNECTED = -5,
    RET_OTHER_PNP_INFO = -6,
    RET_OTHER_PORT_INFO = -7,
};
/* DISPLAY check */
constexpr int SLE_MIN_ADV_DATA_LEN = 12;
constexpr int HILINK_UUID = 0xFDEE;
constexpr int SLE_DISPLAY_TYPE_INDEX = 10;
constexpr int SLE_DISPLAY_VALUE_INDEX = 11;
constexpr int SLE_DISPLAY_TYPE_VALUE = 0x03;
constexpr int SLE_DISPLAY_VALUE = 0x01;

/* uuid def */
constexpr uint16_t SLE_STANDARD_SERVICE_UUID = 0x0600;
constexpr uint16_t UUID_DEVICE_INFORMATION_SERVICE = 0x0609;
constexpr uint16_t UUID_LOCAL_MANAGER_SERVICE = 0x060C;
constexpr uint16_t UUID_DEVICE_INFORMATION_SERVICE_PEN = 0x0906;
constexpr uint16_t UUID_BATTERY_SERVICE = 0x060A;
constexpr uint16_t UUID_BATTERY_SERVICE_PEN = 0x0A06;
constexpr uint16_t SLE_STANDARD_SERVICE_HID_UUID = 0x060B;
constexpr uint16_t SLE_STANDARD_SERVICE_HID_UUID_PEN = 0x0B06;
constexpr uint16_t UUID_PORT_PROFILE_SERVICE = 0x0660;

constexpr int SSAP_INVALID_APPID = -1;
constexpr uint16_t INVALID_LCID = 0xFFFF;
constexpr int INVALID_APPEARANCE = -1;

enum class DeviceClassForService : int {
    DEVICE_SMART_WATCH = 0x000401,              // Smartwatch.
    DEVICE_KEYBOARD = 0x000501,                 // Keyboard.
    DEVICE_MOUSE = 0x000502,                    // Mouse.
    DEVICE_HANDLE = 0x000503,                   // Handle.
    DEVICE_STYLUS = 0x000504,               // Stylus pen.
    DEVICE_TOUCHPAD = 0x000505,                 // Touchpad.
    DEVICE_CLASS_AUDIO_PLAYBACK = 0x000600,  //  General audio playback device.
    DEVICE_CLASS_SMART_SPEAKER = 0x000601,   //  Smart speaker.
    DEVICE_CLASS_ECHO_WALL = 0x000602,       //  Echo Wall.
    DEVICE_WEARABLE_AUDIO = 0x000800,        //  General wearable audio device.
    DEVICE_IN_EAR_EARPHONE = 0x000801,       //  In-ear earphone.
    DEVICE_HEADSET = 0x000802,               //  Headset.
    DEVICE_OVER_EAR_HEADPHONE = 0x000803,    //  Over-ear headphone.
    DEVICE_NECKBAND_EARPHONE = 0x000804,     //  Neck-worn earphone.
    DEVICE_CLASS_VEHICLE_LOCK = 0x001004,    //  Vehicle lock.
    DEVICE_NETWORKING = 0x000F00,            // General network device.
};

/*********************************************
 *
 * SLE UUID Macro Define
 *
 *********************************************/
const std::string SLE_UUID_DIS = "37BEA880-FC70-11EA-B720-000000000609";
const std::string SLE_UUID_LIS = "37BEA880-FC70-11EA-B720-00000000060C";
const std::string SLE_UUID_ICCE = "37BEA880-FC70-11EA-B720-00000000060D";
const std::string SLE_UUID_PORT_PROFILE = "37BEA880-FC70-11EA-B720-000000000660";
const std::string SLE_UUID_CDSM_PROFILE = "37BEA880-FC70-11EA-B720-000000000600";
const std::string SLE_UUID_MCP_PROFILE = "37BEA880-FC70-11EA-B720-000000000614";  /* 通用媒体控制服务 */
const std::string SLE_UUID_TWS_PROFILE  = "00090000-0001-0001-0000-000000000000"; /* 外设私有服务 */
const std::string SLE_UUID_VCP_PROFILE = "37BEA880-FC70-11EA-B720-000000000616";
const std::string SLE_UUID_CCP_PROFILE = "37BEA880-FC70-11EA-B720-000000000612";  /* 通用通话控制服务 */
const std::string SLE_UUID_VAS_PROFILE = "37BEA880-FC70-11EA-B720-000000000613";
const std::string SLE_UUID_BAS_PROFILE = "37BEA880-FC70-11EA-B720-00000000060A";
const std::string SLE_UUID_MIC_PROFILE = "37BEA880-FC70-11EA-B720-000000000619";  /* 音频采集开关控制服务 */
const std::string SLE_UUID_HILINK = "37BEA880-FC70-11EA-B720-00000000FDEE"; /* HiLink */

/*********************************************
 *
 * Adapter Macro Define
 *
 *********************************************/
/**
 * @brief SLE transport define
 * use to GetRemoteDevice(),GetPairedDevices()...
 */
enum class SleFeatureSupported : int32_t {
    SLE_RADIO_FRAME_TYPE_4 = 0,
};

/**
 * @brief policy of auto connecting when nearlink is enabled
 * use to EnableNl(), EnableSle(),...
 */
enum class SleAutoConnectPolicy : int {
    AUTO_CONN_GENERAL = 0, // 默认策略，开星闪后正常自动回连
    AUTO_CONN_EXCEPT_AUDIO_DEVICES, // 不回连音频设备
    AUTO_CONN_EXCEPT_USER_DISCONNECTED_DEVICES, // 不回连用户主动断连的设备
};

/**
 * @brief connect reason define
 */
enum class SleConnectReason : int {
    CONNECT_NONE = -1,
    CONNECT_SUCCESS = 0,
    CONNECT_FAIL,
    CONNECT_LOCAL_DISCONNECT, // 本端主动断连
    CONNECT_REMOTE_DISCONNECT, // 对端主动断连
    CONNECT_FAIL_ACB_CONNECTION, // ACB连接失败
    CONNECT_FAIL_SERVICE_DISCOVERY, // 服务发现失败
    CONNECT_FAIL_NO_AVAILABLE_SERVICE, // 没有可用的服务
    CONNECT_FAIL_CONNECTION_NUM_LIMITED, // 连接数量超过限制
};

struct SleConnectionChangedParam {
    int connState;              // 新的profile连接状态，取值对应SleConnectState
    int connPreState;           // 此前的profile连接状态，取值对应SleConnectState
    int connReason;             // 连接状态变化原因，取值对应SleConnectReason
    std::string connReasonMsg;  // 连接状态变化原因消息，用于维测

    SleConnectionChangedParam() : connState(static_cast<int>(SleConnectState::INVALID_STATE)),
        connPreState(static_cast<int>(SleConnectState::INVALID_STATE)), connReason(-1), connReasonMsg("") {}

    SleConnectionChangedParam(int state, int preState) :
        connState(state), connPreState(preState),  connReason(-1), connReasonMsg("") {}

    SleConnectionChangedParam(int state, int preState, int reason) :
        connState(state), connPreState(preState),  connReason(reason), connReasonMsg("") {}
};

/**
 * @brief pair reason define
 */
enum class SlePairReason : int {
    PAIR_SUCCESS = 0,
    PAIR_FAIL,
};

/**
 * @brief access permission define
 * use to
 * Get/SetPhonebookPermission(),Get/SetMessagePermission()
 */
enum class BTPermissionType : int {
    ACCESS_UNKNOWN,
    ACCESS_ALLOWED,
    ACCESS_FORBIDDEN,
};
/**
 * @brief connection strategy define
 * use to
 * SetConnectionStrategy(),GetConnectionStrategy()
 */
enum class BTStrategyType : int {
    CONNECTION_UNKNOWN,
    CONNECTION_ALLOWED,
    CONNECTION_FORBIDDEN,
};

/**
 * @brief connection mode
 */
enum class SLEConnectionMode : int {
    CONNECTION_MODE_UNCONNECTABLE = 0,
    CONNECTION_MODE_CONNECTABLE,
};

/**
 * @brief profile id define
 */
const uint32_t PROFILE_ID_HOST = 0x00100000;

/**
 * @brief default value
 * use to
 * initialize mac address, name or other values.
 */
/**
 * @brief remote device's transport type
 * use to
 * GetTransportType()
 */
enum class NlTransportType : int {
    NL_TRANSPORT_SLB = 0,
    NL_TRANSPORT_SLE = 1
};


// Scan mode
typedef enum {
    SCAN_MODE_LOW_POWER = 0x00,
    SCAN_MODE_BALANCED = 0x01,
    SCAN_MODE_LOW_LATENCY = 0x02,
    SCAN_MODE_OP_P2_60_3000 = 0x03,
    SCAN_MODE_OP_P10_60_600 = 0x04,
    SCAN_MODE_OP_P25_60_240 = 0x05,
    SCAN_MODE_OP_P100_1000_1000 = 0x06,
    SCAN_MODE_OP_P50_100_200 = 0x07,
    SCAN_MODE_OP_P50_240_480 = 0x08,
    SCAN_MODE_OP_P50_480_960 = 0x09,
    SCAN_MODE_OP_P100_240_240 = 0x0A,
    SCAN_MODE_FULL_SCAN = 0x0B,
    SCAN_MODE_INVALID
} SCAN_MODE;

/*********************************************
 *
 * Public Ssap Macro Define
 *
 *********************************************/

/*
 * ssap service error code
 */
enum SsapStatus : int {
    SSAP_DESC_FAIL = -19,
    SSAP_SLE_ERR_BASE = SSAP_DESC_FAIL - 0x800, // -0x813
    SSAP_PDU_ERR_BASE = SSAP_SLE_ERR_BASE - 0xFF, // -0x912
    SSAP_PDU_ERR_DISCONNECTED = SSAP_PDU_ERR_BASE + 0x10, // -0x902, SSAP_ERR_DISCONNECTED
    SSAP_CHANGED = -18,
    SSAP_DISCONNECT,
    SSAP_NO_ENTRY,
    SSAP_TIMEOUT,
    SSAP_MAX_OBSERVER,
    SSAP_BUSY,
    SSAP_NOT_CONN,
    SSAP_DISCONN_TIMEOUT,
    SSAP_CONN_TIMEOUT,
    SSAP_ENCRYPT_TIMEOUT,
    SSAP_NO_APP,
    SSAP_MAX_APP,
    SSAP_INVALID_PARAM,
    SSAP_NOT_ENABLE,
    SSAP_NOT_SUPPORT,
    SSAP_MAX_TRANSPORT,
    SSAP_ALREADY_ENABLE,
    SSAP_FAILURE,
    SSAP_SUCCESS,
};

/*
 * ssap transport secure require
 */
enum SsapSecureType : uint8_t {
    SSAP_SEC_ENCRYPT,
    SSAP_SEC_NONE,
    SSAP_SEC_MAX,
    SSAP_SEC_INVALID = 0xFF
};

/*
 * ssap disconnect reason
 */
enum AcbConnectionReason : uint8_t {
    ACB_CONNECT_SUCCESS = 0,
    ACB_CONNECTION_FAILED_TO_BE_ESTABLISHED = 0x1C,
};

/*********************************************
 *
 * SLE Macro Define
 *
 *********************************************/
// adv result
enum AdvResult : uint8_t {
    ADV_RESULT_SUCCESS = 0,
    ADV_RESULT_FAILED_CHECK_PARA_FAIL,
    ADV_RESULT_FAILED_TOO_MANY_ADVERTISERS,
    ADV_RESULT_FAILED_ALREADY_STARTED,
    ADV_RESULT_FAILED_INTERNAL_ERROR,
    ADV_RESULT_FAILED_FEATURE_UNSUPPORTED,
    ADV_RESULT_FAILED_NOT_STARTED,
    ADV_RESULT_FAILED_INVALID_HANDLE,
};

enum class SleAdvertisingHandle : uint8_t {
    // Special advertising set handle used for the legacy advertising set.
    SLE_LEGACY_ADVERTISING_HANDLE = 0x00,
    // Special advertising set handle used as return or parameter to signify an invalid handle.
    SLE_INVALID_ADVERTISING_HANDLE = 0xFF
};

// Report delay millis default value
const int SLE_REPORT_DELAY_MILLIS = 5000;

// Definitions for UUID length constants.
enum class UuidLength : int {
    SLE_UUID_LEN_16 = 2,
    SLE_UUID_LEN_128 = 16
};
const int DEVICE_NAME_MAX_LEN = 26;

// Limit the container size to prevent memory roll-up problems.
const int MAX_OBSERVER_SIZE = 200;

const uint32_t SLE_ADV_MAX_LEGACY_ADVERTISING_DATA_BYTES = 255;

// SLE disconnect reason
enum class SleDiscReason : uint8_t {
    SLE_DISC_REASON_COMMAND_TIMEOUT = 0x0D,          /* profile连接超时 */
    SLE_DISC_REASON_REMOTE_USER_TERMINATED = 0x10,   /* 本端主动断连 */
    SLE_DISC_REASON_CANCEL_PAIR = 0x21,              /* 取消配对 */
};

// SLE pair direct
enum class SlePairDirect : int {
    SLE_PAIR_DEFAULT = 0x00,
    SLE_PAIR_ACTIVE = 0x01,
    SLE_PAIR_PASSIVE = 0x02
};

enum class SlePairState : int {
    SLE_PAIR_NONE = 0x01,
    SLE_PAIR_PAIRING = 0x02,
    SLE_PAIR_PAIRED = 0x03,
    SLE_PAIR_CANCELING = 0x04
};

const uint8_t SLE_ADV_DATA_FIELD_TYPE_AND_LEN = 2;

// Phy type
enum class SlePhyType : uint8_t {
    PHY_LE_1M = 1,
    PHY_LE_2M = 2,
    PHY_LE_CODED = 3,
    PHY_LE_ALL_SUPPORTED = 255
};

// SLE advertiser TX power level.
enum class SleAdvertiserTxPowerLevel : uint8_t {
    SLE_ADV_TX_POWER_ULTRA_LOW = 0x00,
    SLE_ADV_TX_POWER_LOW = 0x01,
    SLE_ADV_TX_POWER_MEDIUM = 0x02,
    SLE_ADV_TX_POWER_HIGH = 0x03,
    SLE_ADV_TX_POWER_ULTRA_HIGH = 0x04,
    SLE_ADV_TX_POWER_FIND = 0x05,
    SLE_ADV_TX_POWER_INVAILD,
};

enum class SleHighPowerLevel : uint8_t {
    SLE_HIGH_POWER_LEVEL_7 = 0x07,  // 高功率7档
    SLE_HIGH_POWER_LEVEL_8 = 0x08,  // 高功率8档
};

// SLE advertiser role
constexpr uint8_t SLE_FREQ_HOPPING_LEN = 10;

// SLE HADM STATE CHANGE REASON
enum class HadmStateChangeReason : int {
    NO_ERROR = 0,                          /*!< 无异常 */
    INNER_ERROR,                           /*!< 内部错误 */
    TASK_BUSY,                             /*!< 任务繁忙 */
    HIGH_DUTY_CYCLE,                       /*!< 高占空比业务 */
    HIGH_PRIORITY_INTERRUPT,               /*!< 高优先级业务 */
    CAR_KEY_INTERRUPT,                     /*!< 车钥匙业务 */
    FULL_SCENARIO_UNSUPPORTED,             /*!< 不支持全场景测距业务 */
};

// SLE HADM STATE CHANGE REASON
enum class HadmSupportCapability : uint8_t {
    NOT_SUPPORT = 0,                           /*!< 不支持测距 */
    SUPPORT_G_INITIATOR,                       /*!< 支持G发起测距 */
    SUPPORT_G_T_INITIATOR,                     /*!< 支持G/T任意发起测距 */
};

// 音频流控制命令：打开/关闭
enum ASCControlCmd : uint8_t {
    NL_SLE_ASC_CONTROL_CMD_START        = 1,
    NL_SLE_ASC_CONTROL_CMD_STOP         = 2,
};

// 闪链回调结果
enum class SlHostResult : int32_t {
    SL_RESULT_SUCCESS           = 0,
    SL_RESULT_FAIL              = 1,
    SL_RESULT_TIMEOUT           = 2,

    SL_RESULT_LIMIT             = 3,
};

// 闪链回调状态
enum class SlHostStatus : int32_t {
    SL_NODE_STATUS_ONLINE         = 0,
    SL_NODE_STATUS_OFFLINE        = 1,
    SL_NODE_STATUS_JOIN_SUCCESS   = 2,
    SL_NODE_STATUS_JOIN_FAIL      = 3,
    SL_NODE_STATUS_QUIT_SUCCESS   = 4,
    SL_NODE_STATUS_QUIT_FAIL      = 5,
    SL_NODE_STATUS_LOST_SYNC      = 6,
    SL_NODE_STATUS_ABNORMAL       = 7,
    SL_NODE_STATUS_GROUP_DEINITED = 8,
    SL_NODE_STATUS_ADD_SUCCESS    = 9,
    SL_NODE_STATUS_ADD_FAIL       = 10,
    SL_NODE_STATUS_NODE_REMOVE    = 11,

    SL_NODE_STATUS_LIMIT          = 12,
};

// 闪链组网模式
enum class SlGroupJoinMode : uint8_t {
    SL_JOIN_MODE_G_INVITE = 0,
    SL_JOIN_MODE_T_APPLY  = 1,
    SL_JOIN_MODE_BOTH     = 2,

    SL_JOIN_MODE_LIMIT    = 3,
};

// 闪链sniff模式
enum class SlGroupSniffMode : uint8_t {
    SL_SNIFF_MODE_ACTIVE      = 0,
    SL_SNIFF_MODE_ONE_QUARTER = 1,
    SL_SNIFF_MODE_ONE_EIGHTH  = 2,
    SL_SNIFF_MODE_LIMIT       = 3,
};

// 闪链节点角色
enum class SlGroupMemberRole : uint8_t {
    SL_GROUP_MEMBER_ROLE_G     = 1,
    SL_GROUP_MEMBER_ROLE_T     = 2,

    SL_GROUP_MEMBER_ROLE_LIMIT = 3,
};

// 虚拟链接设备更新命令：增加/删除
enum UpdateVirtualDeviceCmd : uint8_t {
    NL_SLE_VIRTUAL_DEVICE_CMD_ADD = 1,
    NL_SLE_VIRTUAL_DEVICE_CMD_DELETE = 2,
};

enum class PairingStateChangeReason : uint8_t {
    PAIRING_SUCCESS = 0,
    PAIRING_FAILURE,
    PAIRING_ACB_CONNECTION_FAILED,
    PAIRING_EXCEED_ACB_MAX,
    PAIRING_REMOTE_CANCELED,
    PAIRING_LOCAL_CANCELED,
    PAIRING_AUTH_FAILED,
    PAIRING_INVALID_REASON = 0xFF
};

// Nearlink scannerId
constexpr uint32_t SLE_SCAN_INVALID_ID = 0;

constexpr uint32_t SLE_HADM_INVALID_ID = 0;

constexpr uint8_t SLE_INVALID_LINK_ROLE = 0xFF;

#define IS_SLE_ENABLED() (NearlinkHost::GetInstance().IsSleEnabled())

const char* const ACCESS_NEARLINK_PERMISSION = "ohos.permission.ACCESS_NEARLINK";

enum class SsapConnPowerLevel : uint8_t {
    POWER_LEVEL_7,
    POWER_LEVEL_8,
    POWER_LEVEL_MAX,
};

/**
 * @brief subrate参数
 */
typedef struct {
    bool onlySubrate;                   /* true: 只设置subrate，false: 设置subrate和下列参数 */
    uint16_t subrate;
    uint16_t subrateMax;                /* 基础周期的最大倍数 */
    uint16_t maxLatency;                /* T侧跳过基础周期的个数，默认值0 */
    uint16_t continuationNum;           /* 当前执行周期里面基础周期的执行个数 */
    uint16_t supervisionTimeout;        /* 超时时间，单位10ms */
} SleAcbSubrateParam;

enum class NearLinkFreqBand : uint8_t {
    FREQ_BAND_2D4  = 0x01,  /*!< 仅支持2.4G频段 */
    FREQ_BAND_5D8_FENCE  = 0x04,  /*!< 支持5G地理围栏场景 */
    FREQ_BAND_2D4_5D8_ADAPTIVE  = 0x05,  /*!< 支持5G自适应场景 */
};

/**
 * @brief Connection interval type
 *
 * @since 6
 */
enum ConnectionInterval : int {
    HIGH_SPEED_INTERVAL_4_5 = 0,
    HIGH_SPEED_INTERVAL_4_875 = 0x1,
    MID_SPEED_INTERVAL_11_25 = 0x2,
    MID_SPEED_INTERVAL_15 = 0x3,
    MID_SPEED_INTERVAL_50 = 0x4,
    LOW_SPEED_INTERVAL_100 = 0x5,
    LOW_SPEED_INTERVAL_150 = 0x6,
    LOW_SPEED_INTERVAL_200 = 0x7,
    LOW_SPEED_INTERVAL_300 = 0x8,
    LOW_SPEED_INTERVAL_500 = 0x9
};

constexpr int SLE_PRIVATE_AUDIO_BUSINESS_TYPE = 0x01;

#ifdef NEARLINK_EXPORT
#define NEARLINK_API __attribute__((visibility("default")))
#else
#define NEARLINK_API
#endif

const int MAX_TX_POWER = 20;
const int MIN_TX_POWER = 2;
} // namespace Nearlink
} // namespace OHOS

#endif