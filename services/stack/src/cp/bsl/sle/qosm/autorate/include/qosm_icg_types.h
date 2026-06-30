/****************************************************************************
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
****************************************************************************/

/****************************************************************************
 *
 * this file contains icb qos manager: listen controller icb qos event,
 * and output max bitrate by rssi, ack rate and packet interval.
 *
 ***************************************************************************/

#ifndef QOSM_ICG_TYPES_H
#define QOSM_ICG_TYPES_H

#include <stdint.h>
#include "qosm.h"
#include "qosm_autorate_def.h"
#include "qosm_table_mgr.h"
#include "sdf_dlist.h"

#define QOSM_HALF_ROUND_UP(x) ((x) / 2 + 1)

#define RSSI_THRESHOLD_4M_16QAM (-70)
#define RSSI_THRESHOLD_4M_QPSK (-75)

#define ERR_PACKEY_RATE_THRESHOLD 15

#define BITRATE_4M_16QAM_MIN 2300
#define BITRATE_4M_16QAM_MAX 4600
#define BITRATE_THRESHOLD_4M_16QAM 1500

#define BITRATE_4M_QPSK_MIN 960
#define BITRATE_4M_QPSK_MAX 1500
#define BITRATE_THRESHOLD_4M_QPSK 640

#define BITRATE_ENTER_DSP_FLOW_CTRL_MIN 320

#define LOW_ACK_RATE 48
#define HIGH_ACK_RATE 80
#define MAX_ACK_RATE_OVER_CNT 8

#define QOSM_DELAY_SET_PARAM_TIMEOUT_MS 2000
#define QOSM_DELAY_ADD_CONN_TIMEOUT_MS 2000
#define QOSM_INVALID_TIMER_HANDLE (-1)

#define QOSM_ADD_CONNECTION_TIMEOUT_MS 4000
#define QOSM_QOSID_OFFSET 8

#define QOSM_UPGRADE_LEVEL_TIMEOUT_MS 1000
#define QOSM_DOWNGRADE_LEVEL_TIMEOUT_MS 5000

#define QOSM_UPGRADE_CALLBACK_COUNT 2

#define QOSM_INVALID_CONNECT_SEQ 0

#define QOSM_MAX_ESTABLISH_FAIL_CNT 1
#define QOSM_RETRY_CONNECT_TIMEOUT_MS 100

#define QOSM_AUTORATE_MAX_LABEL_CNT 16 // 大小与CM_MAX_LABEL_COUNT相同
#define QOSM_AUTORATE_MAX_LEVEL_CNT 15
#define QOSM_AUTORATE_MAX_CODEC_CONFIG_LEN 128 // Tips：需要和芯片讨论

#define NEARLINK_INVALID_LABEL 255

#define QOSM_IMB_AUTORATE_DOWNWARD_RSSI (-58)
#define QOSM_IMB_AUTORATE_UPWARD_RSSI (-50)

typedef struct {
    QOSM_AutoRateParam autorateParam;
    QOSM_StartParam startParam;
} QOSM_ICGMgrParam;

typedef enum {
    NOTIFY_TYPE_EARPHONE,
    NOTIFY_TYPE_COEXIST,
    NOTIFY_TYPE_5G,
} QOSM_ICGMgrNotifyType;

typedef struct {
    QOSM_ICGMgrNotifyType type;
    uint16_t maxBitrate;              /* 共存模式反馈/5G频段的最大码率限制，全局生效 */
    uint8_t dutyCycle;                /* 共存模式反馈的占空比限制，全局生效 */
    uint16_t supportedBitrate[QOSM_AUTORATE_MAX_SUPPORTED_BITRATE_CNT];
    uint32_t supportedBitrateCnt;
} QOSM_ICGMgrNotifyParam;

/* 码率升降指示枚举值 */
enum {
    LEVEL_NONE = 0,
    LEVEL_UP,
    LEVEL_DOWN,
};

enum {
    UPDATE_NONE = 0,
    UPDATE_BY_REPORT = 1,
    UPDATE_BY_CALL = 2,
};

/* 链路建链状态 */
typedef enum {
    LINK_NONE = 0,
    LINK_LABEL_SETTING,
    LINK_CONNECTING,
    LINK_CONNECTED,
    LINK_SUCCESS,
} QOSM_LinkConnectStatus;

struct QosICBInfo {
    uint8_t icbId;                    /* 从0开始递增赋值 */
    uint16_t lcid;                    /* 异步链路标识 */
    uint16_t connHandle;              /* 同步链路标识 */
    uint8_t freqBand;                 /* 默认为QOSM_DEFAULT_BAND */
    uint32_t ackRateOverCnt;          /* 记录ackRate低于LOW_ACK_RATE的次数，阈值为MAX_ACK_RATE_OVER_CNT */
    uint8_t connectSeq;               /* 建链序号，用于标识icb是否是同一次建链接口调用发起 */
    bool isInGroupConnecting;         /* true表示正在以组的形式建链 */
    bool isDisconnectedByAcb;         /* true表示同步链路断链是由异步链路断链导致断开 */
    uint8_t establishFailCnt;         /* 建链失败次数 */
    int addConnectionTimerId;         /* 建链定时器 */
    int retryConnectTimerId;          /* 重建链定时器，初始值设为QOSM_INVALID_TIMER_HANDLE */
    bool hasIcbDisconnectEvent;       /* true表示建链完成前收到同步链路断链事件 */
    bool hasCreateICB;                /* true表示已下发创建ICB的命令 */
    QOSM_LinkConnectStatus connectStatus; /* LINK_NONE：没有发起建链，或者建链流程结束
                                             LINK_LABEL_SETTING：正在设置label
                                             LINK_CONNECTING：正在建链
                                             LINK_CONNECTED：成功建链
                                             LINK_SUCCESS：同一批次都建链成功 */
};

struct QosLevelLabel {
    bool isAvailable;                 /* true表示当前level可用，起播和耳机支持的码率交集 */
    bool isSupported;                 /* true表示当前level可用，起播码率集 */
    QOSM_LinkParam *qosParam;         /* 码率自适应参数，包含同步链路参数 */
    uint8_t testLabelId;              /* TestParam的labelId，由调用方生成 */
    uint8_t labelId;                  /* 非TestParam的labelId，由芯片生成 */
};

typedef struct {
    SDF_DListEntry_S entry;
    uint8_t qosId;                        /* 由于全局只有一个调用方，所以qosId充当icgId */
    uint8_t icgId;                        /* ICG唯一标识 */
    CM_ICBType icbType;                   /* CM_IOB：单播，CM_IMB：组播 */
    QOSM_QosIndex qosIndex;               /* 同步链路对应的QOS索引 */
    uint16_t gHandle;                     /* 组播G端Handle，组播时有效，由IMB建链事件上报 */
    bool isTest;                          /* true表示icg是通过SetTestParam设置参数的 */
    bool supportSubrate;                  /* true表示对端设备支持subrate */
    bool supportAutorate;                 /* true表示对端设备支持autorate */
    bool is5G;                            /* true表示当前连接的频段是5G */

    uint8_t testParamCbkCnt;              /* SetTestParam的回调次数统计 */
    bool setTestParamFailed;              /* true表示SetTestParam有一个及以上失败 */
    bool isResetParam;                    /* true表示icg在reset param，此时remove param事件里不做任何处理 */
    bool isRemovingParam;                 /* true表示icg正在remove param */

    QOSM_ICGMgrParam *delaySetParam;      /* 未超时时设置参数需要的参数 */
    int delayParamTaskTimerId;            /* 初始值设为QOSM_INVALID_TIMER_HANDLE，表示未设置定时器 */

    QOSM_AutoRateConnParam *delayConnParam; /* 未超时时建链需要的参数 */
    int delayConnTaskTimerId;             /* 初始值设为QOSM_INVALID_TIMER_HANDLE，表示未设置定时器 */

    uint8_t levelCnt;                     /* 在使用的档位数 */
    uint8_t totalLevelCnt;                /* 总的档位数 */
    struct QosLevelLabel *level;
    QOSM_AutoRateEarphoneFeedbackParam earphoneBitrate;

    uint8_t updateStatus;                 /* 在更新链路参数中，此时不能建链，不再响应更新参数
                                           * 不更新：UPATDE_NONE，芯片上报触发的更新：UPATDE_BY_REPORT，调用方触发的更新：UPATDE_BY_CALL */
    // 芯片约束：icg底下的所有同步链路参数需保持一致。细化约束：
    // 1.更新链路参数期间不能建链，建链期间不能更新参数
    // 2.更新链路参数期间，不再响应链路参数更新，若芯片在此时上报的链路参数档位更低，需缓存起来，等当前链路参数更新完毕再次更新参数
    // 3.更新参数回调里，需要等所有已建链的链路都更新后再回调通知
    // 4.链路参数周期上报里，需要等所有已建链的链路都收到参数上报以后再回调通知
    uint8_t linkCnt;                      /* 总链路数，即已设置参数的链路数 */
    struct QosICBInfo *link;
    uint8_t connectedCnt;                 /* 已建立连接的链路数 */
    uint8_t updatedCnt;                   /* 已更新参数的链路数 */

    QOSM_LinkParam *qosParam;             /* icg在使用的qos参数，首次由startLevel确定，后续随着链路参数更新而更新 */

    uint16_t updatedDirection;            /* 不更新：LEVEL_NONE，升码率：LEVEL_UP，LEVEL_DOWN：需要降level，更新过程中不允许建链 */
    QOSM_LinkParam *updatedQosParam;      /* updatedDirection不为LEVEL_NONE时有效，更新后的Qos参数 */
    uint8_t updatedLabelId;               /* updatedDirection不为LEVEL_NONE时有效，更新后的labelId */

    uint16_t reportedDirection;           /* 不更新：LEVEL_NONE，升码率：LEVEL_UP，LEVEL_DOWN：需要降level，更新过程中不允许建链 */
    QOSM_LinkParam *reportedQosParam;     /* reportedDirection不为LEVEL_NONE时有效，更新后的Qos参数 */
    uint8_t reportedLabelId;              /* reportedDirection不为LEVEL_NONE时有效，更新后的labelId */
    int upLevelDelayTimerId;              /* 升码率需要芯片升完后，等1000ms再升DSP */

    int changeLableTimerId;               /* 升降码率定时器 */
    uint8_t recommendEnter5GTimes;        /* 触发推荐芯片进5G的计数 */
    bool isSupportFrame4;                 /* 是否支持帧4通话 & 通话 Autorate*/
} QOSM_ICGInfo;

typedef enum {
    QOSM_FREQ_BAND_2D4_ENABLE        = 0x01, /* 使能芯片进2.4G频段 */
    QOSM_FREQ_BAND_5D8_RECOMMEND     = 0x04, /* 推荐芯片进5.8G频段 */
    QOSM_FREQ_BAND_2D4_5D8_ADAPTIVE  = 0x05, /* 自适应进退5.8G和2.4G频段 */
} QOSM_EnableFreqBandAbility;

typedef enum {
    QOSM_ENABLE_FREQ_BAND_BY_SERVICE   = 0x00, /* 由Service层触发，使能星闪频段，一般来源共存模块 */
    QOSM_ENABLE_FREQ_BAND_BY_RECOMMEND = 0x01, /* 由Stack层触发，推荐使能星闪5.8G频段 */
    QOSM_ENABLE_FREQ_BAND_BY_RECOVER   = 0x02, /* 在Stack层触发推荐使能5.8G频段后，需要在进5.8G后恢复到触发之前的频段能力
                                                 避免变更芯片退出5.8G的条件 */
} QOSM_EnableFreqBandType;

#endif