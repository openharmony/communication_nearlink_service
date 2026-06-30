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
#ifndef TWS_DEFINES_H
#define TWS_DEFINES_H

#include <cstdint>
#include <string>
#include "ssap_data.h"
#include "hibox_common.h"
#include "sdf_struct.h"

/* 定义函数指针类型 */
using LibCallbackReg = void (*)(HiboxRegisterFunc *callBackFunc);
using LibFuncDecodeMsg = bool (*)(uint8_t *echoData, uint16_t len, uint8_t *args);
using LibFuncEncodeMsg = bool (*)(uint8_t *args, uint8_t *rspData, uint16_t dataLen);

namespace OHOS {
namespace Nearlink {

const std::string SLE_UUID_TWS_PROPERTY_READ   = "00090000-0001-0001-0001-000000000000"; /* 私有属性，用途：外设->手机 */
const std::string SLE_UUID_TWS_PROPERTY_WRITE  = "00090000-0001-0001-0002-000000000000"; /* 私有属性，用途：手机->外设 */

/* 私有服务：编/解码库信息 */
const std::string TWS_ENCODE_DECODE_LIB_NAME = "libnearlink_hibox.z.so";   /* 私有命令编码、解码库 */
const std::string TWS_LIB_INTERFACE_RECV_REQ = "HiboxRecvIndMsg";          /* 外设->手机 请求消息 */
const std::string TWS_LIB_INTERFACE_RECV_RSP = "HiboxRecvCfmMsg";          /* 外设->手机 响应消息 */
const std::string TWS_LIB_INTERFACE_SEND_REQ = "HiboxCombReqMsg";          /* 手机->外设 请求消息 */
const std::string TWS_LIB_INTERFACE_SEND_RSP = "HiboxCombRspMsg";          /* 手机->外设 响应消息 */
const std::string TWS_LIB_INTERFACE_CALLBACK_REG = "HiboxRegisterProcess"; /* 注册编码库回调 */

constexpr uint8_t TWS_MSG_DIRECT_DATA_INDEX = 0;             /* 取第一个字节判断消息方向 */
constexpr uint8_t TWS_MSG_DIRECT_REQ = 0x01;                  /* 请求类消息 */
constexpr uint8_t TWS_MSG_DIRECT_RSP = 0x02;                  /* 响应类消息 */
constexpr uint8_t TWS_DYM_LIB_REQ_FIXED_HEADER = 0x3F;        /* 请求消息固定Flag头 */
constexpr uint8_t TWS_DYM_LIB_RSP_FIXED_HEADER = 0xF3;        /* 响应消息固定Flag头 */

enum class TwsHiBoxMsgType : uint8_t {
    HIBOX_ROLE_TYPE = 0x2A,            /* Nearlink echo 3,2   Only Request & No Response */
    HIBOX_DATA_TRANSPORT_CAP = 0x05,   /* Nearlink echo 4,1   Request & Response */
    HIBOX_WEAR_STATUS = 0x06,          /* Nearlink echo 4,2   Only Request & No Response */
    HIBOX_AT_CMD = 0x07,               /* Nearlink echo 4,3   Only Request & No Response */
    HIBOX_AUDIO_HEADSET_EXCEPTION = 0x08,       /* Nearlink echo 4,4   Only Request & No Response */

    HIBOX_EARBUDS_NATRUE = 0x0F,       /* Nearlink echo 5,8 */
    HIBOX_QUERY_BUSINESS = 0x10,       /* Nearlink echo 5,A */
    HIBOX_VENDOR_ACCOUNT_HASH = 0x2C,      /* Nearlink echo 5,D */
    HIBOX_AUTO_CONN_SWITCH = 0x2E,     /* Nearlink echo 5,E */

    HIBOX_AUDIO_CAP_QUERY = 0x2D,      /* Nearlink echo 8,A */

    HIBOX_AUDIO_PROFILE_STATE = 0x30,  /* echo 8,B */
    HIBOX_AUDIO_OUT_DATAPATH = 0x31,   /* echo 8,C */
    HIBOX_NOTIFY_SLE_DISCONNECT_PROFILE = 0x32, /* Nearlink echo 5,11 */
    HIBOX_SLE_PROFILE_STATE = 0x34, /* Nearlink echo 5,12 */
    HIBOX_MANUFACTURER_ABILITY = 0x35, /* Nearlink echo 1,2 */
    HIBOX_AUDIO_DUAL_REC_PARAM = 0x36,     /* *< echo 8,E */

    /* 私有服务内部枚举 */
    TWS_PROFILE_WEAR_DETECTION_STATE = 0xFE, /* 内部命令：佩戴检测使能 */
};

constexpr int TWS_PROFILE_SSAP_THREAD_WAIT_TIMEOUT = 5;
constexpr uint32_t TWS_PROFILE_PROPERTY_MAX_LEN = 256;

/* 私有服务，状态机状态值定义，单设备连接状态复用（取值范围：0-9） */
enum class TwsClientState : uint8_t {
    TWS_STATE_DISCONNECTED = 0,
    TWS_STATE_CONNECTING,
    TWS_STATE_DISCONNECTING,
    TWS_STATE_CONNECTED,
};

/* 私有服务，Profile管理模块启动、关闭服务处理（取值范围：10-19） */
constexpr int TWS_SERVICE_STARTUP_EVT = 10;
constexpr int TWS_SERVICE_SHUTDOWN_EVT = 11;
constexpr int TWS_SERVICE_GET_DEVICES_EVT = 12;

/* 私有服务，单设备状态变更消息值（取值范围：20-29） */
constexpr int TWS_SERVCICE_CONNECT_START_EVT = 20;
constexpr int TWS_SERVICE_DISCONNECT_START_EVT = 21;
constexpr int TWS_SERVICE_CONNECT_CMPL_EVT = 22;
constexpr int TWS_SERIVCE_DISCONNECT_CMPL_EVT = 23;

/* 私有服务，单设备超时消息值（取值范围：30-39） */
constexpr int TWS_SERVICE_CONNECTION_TIMEOUT_EVT = 30;
constexpr int TWS_SERVICE_DISCONNECTION_TIMEOUT_EVT = 31;

/* 私有服务，注册ssap client回调消息值（取值范围：50-69） */
constexpr int TWS_SERVICE_SSAP_CONNECT_STATE_CHANGE = 50;
constexpr int TWS_SERVICE_SSAP_SEND_DATA_EVENT = 51;
constexpr int TWS_SERVICE_SSAP_RECV_DATA_EVENT = 52;
constexpr int TWS_SERVICE_RECV_REQ_EVENT = 53;
constexpr int TWS_SERVICE_RECV_RSP_EVENT = 54;
constexpr int TWS_SERVICE_SEND_REQ_EVENT = 55;
constexpr int TWS_SERVICE_SEND_RSP_EVENT = 56;
constexpr int TWS_SERVICE_SEND_REMOTE_INFO_EVENT = 57;

constexpr int TWS_SERVICE_SSAP_STATE_DISCONNECTED = 70;
constexpr int TWS_SERVICE_SSAP_STATE_CONNECTED = 71;

/* 佩戴检测，单耳摘下暂停或复播逻辑乒乓时长 */
constexpr int DOUBLE_REMOVE_CHECK_TIME = 2000; // 2s
constexpr int MAX_RESUME_PLAY_TIME_SIMPLE = 180000L; // 180s

constexpr int NL_SLE_ASC_STATE_STARTED = 1; // 1 is started, 0 is not started

/* 消息交互错误码 */
enum class TwsErrCode : uint8_t {
    SUCCESS = 0,         /* 0x00: 成功 */
    UNKNOWN,             /* 0x01: 未知error类型 */
    SERVICE_NOT_SUPPORT, /* 0x02: 不支持该Service的请求 */
    CMD_NOT_SUPPORT,     /* 0x03: 不支持该Command的请求 */
    PERMISSION_DENIED,   /* 0x04: 无权限 */
    SYS_BUSY,            /* 0x05: 系统忙 */
    REQ_FORAMT_INVALID,  /* 0x06: 请求格式错误 */
    PARAM_INVALID,       /* 0x07: 参数错误 */
    NO_MEM,              /* 0x08: 申请内存失败 */
    TIMEOUT,             /* 0x09: 响应超时 */
};

/* 数据通信能力类型 */
enum class TwsDtsCap : uint8_t {
    DTS_ABILITY_INVALID = 0x00,            /* echo 4,1  数通能力: 非法值 */
    DTS_ABILITY_VID = 0x01,                /* echo 4,1  数通能力: 骨声纹 */
    DTS_ABILITY_WEAR_DETECT = 0x02,        /* echo 4,1  数通能力: 佩戴检测 */
    DTS_ABILITY_FOTA = 0x03,               /* echo 4,1  数通能力: FOTA */
    DTS_ABILITY_CHR = 0x04,                /* echo 4,1  数通能力: CHR */
    DTS_ABILITY_ICONNECT = 0x05,           /* echo 4,1  数通能力: ICONNECT */
    DTS_ABILITY_PRIVATE_VR = 0x06,         /* echo 4,1  数通能力: 私有语音助手 */
    DTS_ABILITY_VENDOR_BATTERY = 0x07,     /* echo 4,1  数通能力: vendor电量 */
    DTS_ABILITY_VRTUAL_CALL = 0x08,        /* echo 4,1  数通能力: 虚拟通话 */
};

enum class TwsAtCmdType :uint8_t {
    VENDOR_BATTERY_DIALOG = 0,                 /* echo 4,3 AT指令：AT+VENDORBATTERY，电量弹框 */
    UPDATE_VENDOR_BATTERY,                     /* echo 4,3 AT指令：AT+UPDATEVENDORBATTERY，Vendor电量 */
    CLOSE_VENDOR_BATTERY,                      /* echo 4,3 AT指令：AT+CLOSEVENDORBATTERY，关闭电量弹框 */
    DEVICE_INFO,                           /* echo 4,3 AT指令：AT+XSHUAWEIF，设备信息 */
};

enum class TwsCloseBatteryReason :uint8_t {
    CLOSE_VENDOR_BATTERY_INVALID = 0,          /* 非关闭电量弹窗默认值，电量弹窗和更新电量时使用 */
    CLOSE_VENDOR_BATTERY_TIMEOUT = 2,          /* AT+CLOSEVENDORBATTERY=2，上报电量5秒后，超时关闭弹框 */
    CLOSE_VENDOR_BATTERY_BOX_CLOSE = 3,        /* AT+CLOSEVENDORBATTERY=3，耳机关盒关闭弹框 */
};

/* 耳机佩戴变化 */
enum class TwsWearAction : uint8_t {
    ACTION_NONE = 0,
    ACTION_ADD_DOUBLE,
    ACTION_ADD_LEFT,
    ACTION_ADD_RIGHT,
    ACTION_REMOVE_DOUBLE,
    ACTION_REMOVE_LEFT,
    ACTION_REMOVE_RIGHT,
    ACTION_SWITCH,
};

enum class TwsWearPauseReason : uint8_t {
    PAUSE_NO_NEED = 0,
    PAUSE_REASON_LEFT_REMOVE_LEFT,
    PAUSE_REASON_RIGHT_REMOVE_RIGHT,
    PAUSE_REASON_DOUBLE_REMOVE_LEFT,
    PAUSE_REASON_DOUBLE_REMOVE_RIGHT,
    PAUSE_REASON_DOUBLE_REMOVE_DOUBLE,
};

/* 系统默认版本 */
constexpr uint8_t MOBILE_HMOS_VERSION = 0x07;

/* echo 5,d 系统版本 */
constexpr uint8_t TWS_MOBILE_OS_TYPE_HMOS_VERSION = MOBILE_HMOS_VERSION;

/* echo 1,2 系统版本 */
constexpr uint8_t FIRST_MANUFACTURER_OS_TYPE_HMOS_VERSION = MOBILE_HMOS_VERSION;
constexpr uint8_t SECOND_MANUFACTURER_OS_TYPE_HMOS_VERSION = 0x00; /* 系统子版本 */
constexpr uint8_t FIRST_NEARLINK_PROTOCOL_VERSION = 0x01;
constexpr uint8_t SECOND_NEARLINK_PROTOCOL_VERSION = 0x00;

/* echo 8,B 8,C 相关枚举 */
enum class TwsStreamUsability : uint8_t {
    INVALID = 0, /* 0: 无效值 */
    AVAILABLE,   /* 1: 可用 */
    DISABLE,     /* 2: 禁用 */
};

enum class TwsStreamType : uint8_t {
    NOT_DEFINE = 0,  /* byte0 未定义 */
    MUSIC,           /* byte1 音乐 */
    CELL_CALL,       /* byte2 蜂窝通话 */
    VOICE_ASSISTANT, /* byte3 语音助手 */
    RING_TONE,       /* byte4 铃声 */
    VOIP_CALL,       /* byte5 VOIP通话 */
    GAME,            /* byte6 游戏 */
    RECORDING,       /* byte7 录音 */
    REMINDERS,       /* byte8 提醒 */
    VIDEO,           /* byte9 视频 */
    NAVIFATION,      /* byte10 导航 */
    ALERTS,          /* byte11 告警 */

    // STREAM_TYPE在上面定义
    STREAM_TYPE_MAX
};

enum class TwsMediaState : uint8_t {
    ENABLE = 1,  /* 媒体可用 */
    DISABLE = 2, /* 媒体不可用 */
};

enum class TwsAudioServiceType : uint8_t {
    INVALID = 0,     /* byte0 无效值 */
    IDLE,            /* byte1 空闲 */
    NOT_DEFINE,      /* byte2 未定义 */
    EMPTY_MUTE,      /* byte3 空流/静音流 */
};

// 定义音频场景枚举类型
enum class AudioScene : uint8_t{
    SLE_AUDIO_SCENE_MUSIC = 0x00,          // 音乐场景
    SLE_AUDIO_SCENE_GAME = 0x01,           // 游戏场景
    SLE_AUDIO_SCENE_SPATIAL_AUDIO = 0x02,  // 空间音频场景
    SLE_AUDIO_SCENE_CALL = 0x03,           // 通话场景
    SLE_AUDIO_SCENE_OTHERS = 0x04          // 其他场景
};

enum class TwsAutoConnType : uint8_t {
    AUTO_PREEMPTION = 0x02,                /* echo 5,a 手机自动抢占耳机（星闪只支持该类型） */
};

enum class TwsBusinessType : uint8_t {
    NOT_CONNECT = 0,                       /* 0:未连接 */
    IDLE,                                  /* 1:空闲 */
    MUSIC,                                 /* 2:音乐 */
    PHONE_CALL,                            /* 3:接通电话、去电 */
    INCOMING_CALL_RING,                    /* 4:来电振铃 */
};

/* echo 5,5 抢占通道查询结果 */
enum class TwsCaptureResult : uint8_t {
    INVALID = 0,                            /* 0x00:非法值 */
    ACCEPT = 1,                             /* 0x01:接受 */
    REFUSE_WITH_PRIORITY,                   /* 0x02:拒绝，因为耳机当前有业务或者业务优先级比较高 */
    REFUSE_WITH_ACCOUNT,                    /* 0x03:拒绝，因为手机的vendor账号与耳机虚拟连接广播中广播不等。 */
    REFUSE_WITH_VIRTUAL_STATE,              /* 0x04:拒绝，因为手机的虚拟自动连接开关已经关闭 */
    REFUSE_WITH_AUTO,                       /* 0x05:拒绝，耳机当前拒绝虚拟自动抢占。 */
    REFUSE_WITH_OTHER,                      /* 0x06:其他拒绝原因 */
};

/* echo 5,e 自动连接开关 */
enum class TwsAutoConnSwitch : uint8_t {
    OFF = 0,                                /* 自动连接开关：关 */
    ON,                                     /* 自动连接开关：开 */
};

/* echo 5,11 静默数传是否断开服务 */
enum class TwsProfileAction : uint8_t {
    INVALID = 0x00,                         /* 默认值 */
    DISCONNECT = 0x01,                      /* 通知手机断开服务 */
};

#pragma pack(1)

/**
 * @brief echo4,3 AT指令透传数据结构
 * @note 外设<->手机，手机不需要回复
 */
typedef struct {
    uint8_t atCmdType;   /* AT命令类型 */
    uint8_t atCmdStr[0]; /* AT命令字符串，max len:230 */
} TwsAtCmdData;

/**
 * @brief echo4,2 请求消息结构体
 * @note 外设->手机，手机不需回复
 */
typedef struct {
    uint8_t leftWear;    /* 左耳佩戴状态 */
    uint8_t rightWear;   /* 右耳佩戴状态 */
    uint8_t freemanWear; /* 助听器佩戴状态 */
} TwsWearStateData;

/**
 * @brief echo3,2 请求消息结构
 * @note 无需回复响应消息
 */
typedef struct {
    uint8_t type;
    uint8_t role;
} TwsIsoHandover;

/* 私有特性类型4：设备信息上报，数据TLV字段排布 */
struct TwsDeviceDatas {                 /* Type: 数据含义 */
    std::string deviceName;             /* 1: 用户设置的设备名称，UTF-8编码 */
    std::string timeStamp;              /* 2: 名称设置时的时间戳 */
    std::string vendorIdSrc;            /* 3: 厂商ID源 */
    std::string vendorId;               /* 4: 厂商ID */
    std::string productId;              /* 5: 产品ID */
    std::string version;                /* 6: 版本号 */
    std::string newModelId;             /* 7: 全场景productid,传原始字符 */
    std::string modelId;                /* 8: 靠近发现Model ID，手机侧动画展示 */
    std::string devClass;               /* 9: 配件COD，设备类别，如：手机、耳机等大类 */
    std::string devType;                /* a: 配件设备类型 */
    std::string iconId;                 /* b: 配件图标类型 */
};

/**
 * @brief echo5,11 请求消息结构体
 * @note 外设->手机
 */
typedef struct {
    uint8_t notifyValue; /**< 0x01:通知断开profile */
} NotifyDisconnectProfile;

typedef struct {
    uint32_t aac64KBitRate : 1;
    uint32_t scoNotDropPlay : 1;
    uint32_t aac48KSampleRate : 1;
    uint32_t deviceRename : 1;
    uint32_t disableA2dpPktGroup : 1;
    uint32_t voiceCombineAbility : 1;
    uint32_t resv : 26; // 保留
} AudioCapBitmap;

/**
 * @brief echo8,A 请求消息结构体
 * @note 外设->手机
 */
typedef struct {
    AudioCapBitmap caps;
    uint8_t absVolume;
    uint16_t joinIntervalMs;  /* join mode interval */
    uint16_t musicIntervalMs; /* ddm mode music interval */
    uint16_t hwaIntervalMs;   /* ddm mode HWA interval */
    uint16_t otherIntervalMs; /* ddm mode other interval */
    SleLatencyConfig sleConfig;
} TwsAudioCapsData;

/* 编解码库交互参数传递 */
typedef struct {
    SLE_Addr_S peerAddr;
} TwsSharedLibMsgArgs;

/* 编解码库消息交互 */
typedef struct {
    uint8_t msgType;
    uint16_t dataLen;
    uint8_t data[0];
} TwsEncodeMsg;

typedef struct {
    uint8_t msgType;
    uint16_t dataLen;
    uint8_t data[0];
} TwsTlvMsg;

typedef struct {
    uint8_t fixHead;
    uint8_t serviceId;
    uint8_t cmdId;
    uint8_t payload[0];
} TwsMsgStreamHead;

/* 佩戴检测单耳摘下pause */
typedef struct {
    int pauseReason;
    int64_t pauseTime;
} TwsPauseRecord;

typedef struct {
    std::vector<TwsHiBoxMsgType> msgQueue; /* 缓存消息队列 */
} TwsCacheMessage;

typedef struct {
    uint8_t codecId;        // 编解码器标识, 0x0: PCM(暂不支持), 0x1: L2HC
    uint16_t companyId;     // 厂商标识
    uint16_t vendorId;      // 厂商编解码器标识
    uint8_t version;        // version
    uint8_t rateConf;       // 采样率Hz
    uint8_t depthConf;      // 位深
    uint8_t channelConf;    // 通道类型
    uint8_t frameConf;      // 帧类型
    uint8_t bpsConf;        // 码率索引
    uint64_t bpsRange;      // 码率位图
} TwsDualRecordCodecInfo;

#pragma pack()

} // namespace Sle
} // namespace OHOS

#endif
