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
#ifndef HIBOX_COMMON
#define HIBOX_COMMON

#include <stdio.h>
#include <stdbool.h>
#include "securec.h"
#include <stdlib.h>
#include <log.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _INT8_T_DECLARED
typedef unsigned char       uint8_t;
#endif
#ifndef _INT16_T_DECLARED
typedef unsigned short      uint16_t;
#endif
#ifndef _INT32_T_DECLARED
typedef unsigned int        uint32_t;
typedef int        int32_t;
#endif

#define HIBOX_ADDR_LEN 6
#define DTS_DATA_MAX_LEN 255
#define DTS_DATA_MIN_LEN 3
#define ICARRY_NAM_MAX_TLV_LEN 64
#define DEVICE_FIND_SN_ADDR_LEN 8
#define DEVICE_FIND_PRODUCT_ID_LEN 4
#define DTS_MAX_CAP_LEN 10
#define DTS_NAME_MAX_LEN 32
#define SERVICE_CAP_MAX_LEN 10
#define VENDOR_ACCOUNT_HASH_LEN 32
#define MAX_SCENE_NUM 16
#define DEVICE_PROTOCOL_LEN 2
#define DEVICE_OS_LEN 2
#define DEVICE_MANUFACTURER_ABILITY_LEN 16
#define DEVICE_MANUFACTURER_ABILITY_BIT_LEN 128
#define DEVICE_DUAL_REC_CODEC_MAX_NUM 4
#define DEVICE_DUAL_REC_CODEC_NUM 1 // 当前版本填写的编解码数量
#define DUAL_REC_CODEC_UP_BITRATE_BIT_LEN 5
#define NEARLINK_DEVICE_NAME_LEN    32
#define NEARLINK_STREAM_TYPE_NUM    32

/**
* @brief parse msgid and comb msgid
*/
enum HiboxParseMsgType {
    SERVICE_CAP = 0x01, /**< echo1,1 */
    AUDIO_SCENE = 0x02, /**< echo2,1 */
    SPORT_SCENE = 0x03, /**< echo2,2 */
    AUDIO_DELAY = 0x04, /**< echo2,3 */
    DTS_CAP = 0x05, /**< echo4,1 */
    WEAR_STATUS = 0x06, /**< echo4,2 */
    DTS_REPORT = 0x07, /**< echo4,3 */
    AUDIO_EXCEPTION = 0x08, /**< echo4,4 */
    SDP_QUERY = 0x09, /**< echo5,1 */
    TWS_MOBILE_CONN = 0x0A, /**< echo5,2 */
    CANCEL_PAIR = 0x0D, /**< echo5,5 */
    PAIR_DEVICE_TYPE = 0x0E, /**< echo5,6 */
    EARBUDS_NATRUE = 0x0F, /**< echo5,8 */
    QUERY_BUSINESS = 0x10, /**< echo5,A */
    SECU_ADV = 0x11, /**< echo5,B */
    ICARRY_ROLE = 0x12, /**< echo6,1 */
    ICARRY_DEVICE = 0x13, /**< echo6,2 */
    ICARRY_NAME = 0x14, /**< echo6,3 */
    ICARRY_STATE = 0x15, /**< echo6,4 */
    ICARRY_CAP = 0x16, /**< echo6,5 */
    AUTO_PAIR_STATE = 0x17, /**< echo7,1 */
    AUTO_PAIR_PWD = 0x18, /**< echo7,2 */
    AUTO_PAIR_PWD_UPDATE = 0x19, /**< echo7,3 */
    HDAP_CODEC_REPORT = 0x1A, /**< echo 8,1 */
    HDAP_SET_CODEC = 0x1B, /**< echo 8,2 */
    HDAP_START_BUSINESS = 0x1C, /**< echo 8,3 */
    HDAP_STOP_BUSINESS = 0x1D, /**< echo 8,4 */
    HDAP_SEND_KARAOKE_PARAM = 0x1E, /**< echo 8,5 */
    HDAP_VERSION_PARAM = 0x1F, /**< echo 8,6 */
    AUDIO_CODEC_RATE = 0x20, /**< echo 8,7 */
    NOTIFY_PROFILE_STATE = 0x21, /**< echo 8,8 */
    NOTIFY_OUTPUT_PATH = 0x22, /**< echo 8,9 */
    DEVICE_DESCRIP = 0x23, /**< echo9,1 */
    DEVICE_CONN = 0x24, /**< echo9,2 */
    DEVICE_CTRL = 0x25, /**< echo9,3 */
    DEVICE_BELL_STATE = 0x26, /**< echo9,4 */
    UHD_NEGOTIATION = 0x27, /**< echoA,1 */
    UHD_AUTO_RATE = 0x28, /**< echoA,2 */
    HITWS_ISO_CONFIG = 0x29, /**< echo3,1 */
    HITWS_ISO_HANDOVER = 0x2A, /**< echo3,2 */
    HITWS_ISO_RESTART = 0x2B, /**< echo3,5 */
    VENDOR_ACCOUNT_HASH = 0x2C, /* echo 5,D */
    AUDIO_CAP_QUERY = 0x2D, /* *< echo 8,A */
    AUTO_CONN_SWITCH_CFM = 0x2E, /* *< echo 5,E */
    PERIPHERAL_MAC_UPDATE = 0x2F, /* *< echo 5,10 */
    NEARLINK_AUDIO_PROFILE = 0x30, /* *< echo 8,B */
    NEARLINK_AUDIO_DATAPATH = 0x31, /* *< echo 8,C */
    NOTIFY_SLE_DISCONNECT_PROFILE = 0x32, /* *< echo 5,11 */
    SLE_ALL_PROFILE_CONNECTED = 0x34, /* *< echo 5,12 */
    DEVICE_MANUFACTURE_ABILITY = 0x35, /* *< echo 1,2 */
    NEARLINK_DUAL_REC_PARAM = 0x36, /* *< echo 8,E */
};

/**
* @brief send rsp errorcode
*/
enum HiboxErrorCode {
    HIBOX_SUCCESS = 0x00,
    HIBOX_ERR_UNKOWN_ERROR_TYPE,
    HIBOX_ERR_UNSUPPORTED_SERVICE,
    HIBOX_ERR_UNSUPPORTED_COMMAND,
    HIBOX_ERR_NO_PERMISSION,
    HIBOX_ERR_SYSTEM_BUSY,
    HIBOX_ERR_REQUEST_FORMAT_ERROR,
    HIBOX_ERR_PARA_ERROR,
    HIBOX_ERR_ALLOC_MEMORY_FAIL,
    HIBOX_ERR_RSP_TIMEOUT
};

/**
* @brief send link type
*/
enum HiboxTransport {
    HIBOX_TRANSPORT_BR_EDR = 0x01,
    HIBOX_TRANSPORT_LE = 0x02,
};

/**
* @brief hibox type
*/
enum HiboxType {
    HIBOX_REQ = 0x01,
    HIBOX_RSP = 0x02,
};

#pragma pack(1)
/**
* @brief args for parse interface or send interface
*/
typedef struct {
    uint8_t bd[HIBOX_ADDR_LEN]; /**< peer addr */
    uint8_t linkType; /**< linkType, define in enum HiboxTransport */
    uint8_t id; /**< L2CAP Transport Identify */
} HiboxRecvMsgArg;

/**
* @brief parse data struct, or send source data struct
*/
typedef struct {
    uint8_t msgType;  /**< msgType,define in struct HiboxParseMsgType */
    uint16_t datalen; /**< datalen */
    uint8_t data[0]; /**< data,define in the following struct */
} HiboxParseMsgInd;

/**
* @brief parse common data
*/
typedef struct {
    uint8_t rspResult;
} HiboxRspCommonResult;

/**
* @brief echo1,1 req and rsp data struct
*/
typedef struct {
    uint8_t serivceCap[SERVICE_CAP_MAX_LEN];/**< support service id,0xFF is invalid */
} ServiceManangeData;

/**
* @brief echo2,1 req data struct, rsp data define in HiboxRspCommonResult
*/
typedef struct {
    uint8_t audioScene; /**< define in enum AudioSceneValue */
} AudioSceneData;

/**
* @brief echo2,2 req data struct, rsp data define in HiboxRspCommonResult
*/
typedef struct {
    uint8_t sportScene;
} SportSceneData;

/**
* @brief echo2,3 req data struct, rsp data define in HiboxRspCommonResult
*/
typedef struct {
    uint8_t audioDelay;
} AudioDelayData;

typedef struct {
    uint8_t streamId;
    uint8_t channelId;
    uint8_t m2sPolarParam; /**< en (bit0) polar_r (bit1~bit2) pilot_ratio (bit3~bit5) */
    uint8_t s2mPolarParam; /**< en (bit0) polar_r (bit1~bit2) pilot_ratio (bit3~bit5) */
    uint16_t reserve;
} HitwsPolarConfig;

#define HITWS_MAX_SCENE_NUM 16

/**
* @brief echo3,5 req and rsp data struct
*/
typedef struct {
    uint8_t recovery;
} HitwsIsoRecovery;

enum DtsMgrFeature {
    DTS_MGR_FEATURE_VID = 0x01,
    DTS_MGR_FEATURE_WEAR_DETECT = 0x02,
    DTS_MGR_FEATURE_FOTA = 0x03,
    DTS_MGR_FEATURE_CHR = 0x04,
    DTS_MGR_FEATURE_ICONNECT = 0x05,
    DTS_MGR_FEATURE_PRIVATE_VR = 0x06,
    DTS_MGR_FEATURE_VENDOR_BATTERY = 0x07,
    DTS_MGR_FEATURE_VRTUAL_CALL = 0x08,
};
/**
* @brief echo4,1 req data struct, rsp data define in HiboxRspCommonResult
*/
typedef struct {
    uint8_t dtsCap[DTS_MAX_CAP_LEN]; /**< define in enum DtsMgrFeature */
} DtsCapData;

/**
* @brief echo4,4 req data struct, rsp data define in HiboxRspCommonResult
*/
typedef struct {
    uint32_t audioExceptionType;
    uint16_t audioQualityRssi;
    uint32_t audioQualityRetrans;
    uint32_t audioQualityScene;
    uint32_t audioQualityAfh;
} AudioExceptionData;

enum PairType {
    PAIR_CONN_NORMAL = 0x00,
    PAIR_CONN_ABNORMAL = 0x01,
};


/**
* @brief echo5,11 req data struct
* @brief echo5,11 rsq data struct
*/
typedef struct {
    uint8_t notifyValue;
} NotifySleDisconnectProfile;

/**
 * @brief echo5,8 请求消息结构体
 * @note 外设->手机
 */
typedef struct {
    uint8_t nature; /**< 0x00:left ;0x01:right */
} EarbudsNature;

/**
 * @brief echo5,A req data struct
 */
typedef struct {
    uint8_t connType;
    uint8_t accountHash[VENDOR_ACCOUNT_HASH_LEN];
    uint8_t mobileBusinessType;
} QueryPairConn;

typedef struct {
    uint8_t msgType;
    uint8_t typeValue;
} QueryPairConnCfmResult;

/* echo 5,E req data */
typedef struct {
    uint8_t autoConnSwitch;
} MultConnAutoConnSwitch;

/* echo 1,2 req data */
typedef struct {
    uint8_t protocolType[DEVICE_PROTOCOL_LEN];                    // 星闪私有协议栈版本
    uint8_t deviceOsType[DEVICE_OS_LEN];                          // 鸿蒙系统版本
    uint8_t manufacturerAbility[DEVICE_MANUFACTURER_ABILITY_LEN]; // 能力位图
} ManufacturerAbilityInfo;

/* echo 5,D req data */
typedef struct {
    uint8_t deviceOsType;
    uint8_t accountHash[VENDOR_ACCOUNT_HASH_LEN];
} VendorAccountHash;



enum IcarryAction {
    REMOVE_ICARRY = 0x00,
    PREEMPT_ICARRY = 0x01,
    CREATE_ICARRY = 0x02,
    PREEMPT_EARBUDS = 0x03,
};

enum IcarryOper {
    ICARRY_CONNECT_STATE = 0x01,
    ICARRY_WEAR_STATE = 0x02,
    ICARRY_ANSWER_STATE = 0x03,
    ICARRY_BUSINESS_STATE = 0x04,
};

/* echo 8, a req data,caps:
Bit0 : autorate
Bit1 : dropPlay
Bit2 : lowLatency
Bit3 : changeName
Bit4 : a2dpNoGroup
Bit5~Bit31 : Reserved
*/
// 根据场景，设置sduIntervalTimes
typedef struct {
    uint8_t scene;
    uint8_t times;
} IntervalTimesConfig;

// 星闪音画同步参数传递总结构体
typedef struct {
    int16_t offset;
    uint8_t sceneNum;
    IntervalTimesConfig timesMap[MAX_SCENE_NUM];
} SleLatencyConfig;

typedef struct {
    uint32_t caps;
    uint8_t absVolume;
    uint16_t joinIntervalMs;  /* join mode interval */
    uint16_t musicIntervalMs; /* ddm mode music interval */
    uint16_t hwaIntervalMs;   /* ddm mode HWA interval */
    uint16_t otherIntervalMs; /* ddm mode other interval */
    SleLatencyConfig sleConfig;
} DeviceAudioCapsInfo;
/* echo 8,B req data */
typedef struct {
    uint8_t audioType[NEARLINK_STREAM_TYPE_NUM]; /* 音频流类型数组 */
    uint8_t targetDevName[NEARLINK_DEVICE_NAME_LEN + 1]; /* 耳机音频切换至的目标设备名，全0为无效值 */
    uint8_t audioServiceType; /* 音频流业务类型 */
    bool isUserSelected; /* 是否用户强选 */
} HiBoxNearlinkProfile;

/* echo 8,E rsp data 20bytes */
typedef struct {
    uint8_t version;
    uint8_t codecId;
    uint16_t companyId; // 厂商标识
    uint16_t vendorId;  // 厂商自定义编解码类型
    uint8_t upBitrate[DUAL_REC_CODEC_UP_BITRATE_BIT_LEN]; // 上行比特率
    uint8_t sampleRate;
    uint8_t depth;
    uint8_t channel;
    uint8_t frame;
    uint8_t reserved[5]; // 预留 5 Bytes
} DeviceDualRecParmInfo;

/* echo 8,E req data 22bytes */
typedef struct {
    uint8_t version;
    uint8_t codecId;
    uint16_t companyId; // 厂商标识
    uint16_t vendorId; // 厂商自定义编解码类型
    uint8_t upBitrate[DUAL_REC_CODEC_UP_BITRATE_BIT_LEN]; // 上行比特率
    uint16_t sampleRate;
    uint8_t depth;
    uint16_t channel;
    uint8_t frame;
    uint8_t reserved[5]; // 预留 5 Bytes
} DeviceDualRecCapInfo;

/* echo 8,E req data 88bytes */
typedef struct {
    uint8_t codecNum; // 编解码器数量
    DeviceDualRecCapInfo param[DEVICE_DUAL_REC_CODEC_MAX_NUM];
} HiBoxNearlinkDualRecCaps;

#pragma pack()

typedef void (*HiboxSendReqMsgHandler)(uint8_t *addr, uint8_t *reqData, uint16_t dataLen);
typedef void (*HiboxSendRspMsgHandler)(uint8_t *args, uint8_t *rspData, uint16_t dataLen);
typedef void (*HiboxMsgIndHandler)(uint8_t echoType, uint8_t *data, uint16_t len, uint8_t *args);
typedef void (*HiboxMsgCfmHandler)(uint8_t echoType, uint8_t *data, uint16_t len, uint8_t *args);

/* hibox register func stru */
typedef struct {
    HiboxSendReqMsgHandler sendReqFunc;
    HiboxSendRspMsgHandler sendRspFunc;
    HiboxMsgIndHandler msgIndFunc;
    HiboxMsgCfmHandler msgCfmFunc;
} HiboxRegisterFunc;

/* hibox register func */
void HiboxRegisterProcess(HiboxRegisterFunc *func);
/* hibox process echo L2CAP Ind msg */
bool HiboxRecvIndMsg(uint8_t *echoReq, uint16_t len, uint8_t *args);
/* hibox process echo L2CAP Cfm msg */
bool HiboxRecvCfmMsg(uint8_t *echoRsp, uint16_t len, uint8_t *args);
/* hibox comb req msg */
bool HiboxCombReqMsg(uint8_t *addr, uint8_t *source, uint16_t datalen);
/* hibox comb rsp msg */
bool HiboxCombRspMsg(uint8_t *args, uint8_t *rspData, uint16_t dataLen);

#ifdef __cplusplus
}
#endif

#endif
