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
 * this file contains the CM common macro, struct, enum definitions
 *
 ***************************************************************************/


#ifndef CM_DEF_H
#define CM_DEF_H

#include <stdint.h>
#include <stdbool.h>
#include "sdf_addr.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CM_CONNECT_VERSION_1_0            0x10
#define CM_CONNECT_LOCAL_INDEX_0          0
#define CM_INVALID_LCID                   ((uint16_t)0xFFFF)
#define CM_READ_REMOTE_FEATURES_PARAM_LEN 10
#define CM_FEATURES_PARAM_LEN             10
#define CM_PRIVATE_FEATURES_LEN           16
#define CM_PRIVATE_FEATURES_BIT_SUBRATE   16
#define CM_PRIVATE_FEATURES_BIT_AUTORATE  23
#define CM_CHANNEL_MAP_LEN                10
#define CM_NEARLINK_FEATURE_INDEX         8
#define CM_NEARLINK_FEATURE_COMPETENCE    0x4
#define CM_INVALID_VERSION                ((uint16_t)0xFFFF)
#if defined(TV_STANDARD) || defined(SLE_POWER_MANAGER_SUPPORT)
#define SLE_DATA_FILTER_CODE_LEN 8
#endif

typedef enum {
    CM_G_NODE,
    CM_T_NODE,
} CM_NodeType_E;

/**
 * @brief 星闪除CM基础连接外其他模块注册标识(type: uint8_t)
 */
typedef enum {
    CM_MODULE_ADPT = 0x0,          // 服务层适配模块固定分配
    CM_MODULE_CM_SIGNALING = 0x1,
    CM_MODULE_DTAP = 0x2,
    CM_MODULE_SM = 0x3,
    CM_MODULE_SSAP = 0x4,          // 协议栈SSAP模块固定分配
    CM_MODULE_HADM = 0x5,
    CM_MODULE_CM_DYNTRANS = 0x6,
    CM_MODULE_QOSM = 0x7,
    CM_MODULE_BNL = 0x8,
    CM_MODULE_ID_MAX,
} CM_ModuleId_E;

typedef enum {
    CM_DEVTYPE_UNKNOWN              = 0,
    CM_DEVTYPE_OLD                  = 1,
    CM_DEVTYPE_THIRD_PARTY          = 2,
    CM_DEVTYPE_NEW                  = 3,
    CM_DEVTIYPE_MAX,
} CM_DeviceType_E;

/**
 * @brief 星闪逻辑链路连接状态
 */
typedef enum {
    CM_LINK_STATE_CONNECTED                 = 0x00,  /* 已经连接 */
    CM_LINK_STATE_CONNECTING                = 0x01,  /* 连接中 */
    CM_LINK_STATE_DISCONNECTED              = 0x02,  /* 已经断开 */
    CM_LINK_STATE_DISCONNECTTING            = 0x03,  /* 断开中 */
} CM_ConnectLinkState_E;

/**
 * @brief 星闪连接参数帧类型
 */
typedef enum {
    CM_CONN_PARAM_FRAME_TYPE_1 = 0,  // 无线帧类型1
    CM_CONN_PARAM_FRAME_TYPE_2 = 1,  // 无线帧类型2
    CM_CONN_PARAM_FRAME_TYPE_3 = 2,  // 无线帧类型3
    CM_CONN_PARAM_FRAME_TYPE_4 = 3,  // 无线帧类型4
} CM_ConnParamFrameType_E;

/**
 * @brief 断连原因
 */
typedef enum {
    CM_DISC_REASON_COMMAND_TIMEOUT = 0x0D,          /* 命令超时 */
    CM_DISC_REASON_REMOTE_USER_TERMINATED = 0x10,   /* 主动断连 */
    CM_DISC_REASON_CANCEL_PAIR = 0x21,              /* 取消配对 */
} CM_DISC_REASON;

typedef enum  {
    CM_TRANS_MODE_BASIC = 0x00,       /* 基础模式 */
    CM_TRANS_MODE_TRANSPARENT,        /* 透传模式 */
    CM_TRANS_MODE_STREAM,             /* 流模式 */
    CM_TRANS_MODE_RELIABLE,           /* 可靠模式 */
    CM_TRANS_MODE_MAX,                /* 传输模式数目 */
} CM_TransMode_E;

/**
 * @brief 星闪接入层数据传输方式
 */
typedef enum {
    CM_ACCESS_TRANS_MODE_UNICAST = 0x00, /* 单播  */
    CM_ACCESS_TRANS_MODE_DATA_MCST,      /* 数据组播 */
    CM_ACCESS_TRANS_MODE_FEEDBACK_MCST,  /* 反馈组播 */
    CM_ACCESS_TRANS_MODE_BIDI_MCST,      /* 双向组播 */
    CM_ACCESS_TRANS_MODE_SEND_BCST,      /* 广播发送 */
    CM_ACCESS_TRANS_MODE_RECV_BCST,      /* 广播接收 */
    CM_ACCESS_TRANS_MODE_MAX,
} CM_AccessTransportMode_E;

/**
 * @brief 添加背景连接设备地址时，可设置isBypass等标识
 */
typedef struct {
   SLE_Addr_S addr;
   bool isBypass;    // 是否有Bypass自动回连标识
} CM_BgConnAddrParam_S;

/**
 * @brief 添加主动连接设备地址时，设置帧类型等连接参数
 */
typedef struct {
   SLE_Addr_S addr;
   uint8_t frameType;  // 连接帧类型，参见CM_ConnParamFrameType_E定义
} CM_DirectConnAddrParam_S;

/**
 * @brief  流模式配置参数
 */
typedef struct {
    uint16_t reorderTimeout;   /* 重排序定时器超时时间 */
    uint16_t crcInit;          /* crc种子 */
    uint16_t flushTimeout;     /* 丢弃定时器超时时间 */
} CM_TransStreamModeConfig_S;

/**
 * @brief  可靠模式配置参数
 */
typedef struct  {
    uint16_t reorderTimeout;           /* 重排序定时器超时时间 */
    uint16_t crcInit;                  /* crc种子 */
    uint16_t retransTimeout;           /* 重传定时器超时时间 */
    uint16_t rspTimeout;               /* 应答定时器超时时间 */
    uint8_t txWindow;                  /* 发送队列的长度 */
    uint8_t maxTxThreshold;            /* 最大重传次数 */
} CM_TransReliableModeConfig_S;

/**
 * @brief  传输管理传输模式配置参数
 */
typedef struct {
    uint8_t transMode;                                      /* 传输模式，参考枚举CM_TransMode_E */
    uint16_t mtu;                                           /* 最大传输单元 */
    uint16_t mps;                                           /* 最大载荷大小 */
    union {
        CM_TransReliableModeConfig_S reliableMode;          /* 可靠模式参数配置 */
        CM_TransStreamModeConfig_S streamMode;              /* 流模式参数配置 */
    };
} CM_TransModeConfig_S;

/**
 * @brief  逻辑链路信息
 */
typedef struct {
    uint16_t lcid;                             /* 逻辑链路标识 */
    uint8_t role;                              /* 链路角色：G或者T，参见CM_NodeType_E定义 */
    SLE_Addr_S addr;                           /* 星闪设备地址 */
} CM_LogicLink_S;

/**
 * @brief 星闪逻辑链路状态变化响应结构体
 */
typedef struct {
    uint16_t  lcid;         /* 星闪逻辑链路handle */
    uint8_t   role;         /* 链路角色：G或者T */
    SLE_Addr_S addr;        /* 星闪设备地址 */
    uint8_t   result;       /* 参考链路连接状态CM_ConnectLinkState_E定义 */
    uint8_t discReason;     /* 链路断链原因，参见DLI_ERRNO头文件定义 */
    uint8_t connCompleteType; /* 连接完成类型，0：通过本端扫描建立的连接，1：通过本端广播建立的连接 */
    uint8_t advHandle;        /* 广播标识，若是通过本端广播建立的连接，则为对应启动广播标识，否则默认为0，无效值 */
} CM_LogicLinkState_S;

/**
 * @brief 星闪逻辑链路对端特性响应结构体
 */
typedef struct {
    uint16_t  lcid;                            /* 星闪逻辑链路handle */
    uint8_t features[CM_FEATURES_PARAM_LEN];   /* 对端设备的features */
    SLE_Addr_S addr;
} CM_LogicLinkRemoteFeatures_S;

/**
 * @brief 星闪逻辑链路主动连接参数更新响应结构体
 */
typedef struct {
    uint8_t    result;       /* 芯片错误码，详见dli_errno.h */
    uint16_t   lcid;         /* 星闪逻辑链路handle */
    SLE_Addr_S addr;         /* 星闪设备地址 */
} CM_LogicLinkConnUpdateParam_S;

/**
 * @brief  连接逻辑链路模块回调函数类型
 */
typedef void (*CM_LogicLinkCbk)(CM_LogicLinkState_S *state);

/**
 * @brief  连接逻辑链路模块对端特性回调函数类型
 */
typedef void (*CM_LogicLinkRemoteFeaturesCbk)(CM_LogicLinkRemoteFeatures_S *param);

/**
 * @brief  连接逻辑链路模块主动连接参数更新回调函数类型
 */
typedef void (*CM_LogicLinkConnUpdateParamCbk)(CM_LogicLinkConnUpdateParam_S *param);

/**
 * @brief  逻辑链路变化相关监听回调函数
 */
typedef struct CM_LogicLinkCbks {
    uint8_t moduleId;               /* 模块标识，参见CM_ModuleId_E定义 */
    CM_LogicLinkCbk logicLinkCbk;   /* 可空 */
    CM_LogicLinkRemoteFeaturesCbk remoteFeaturesCbk; /* 可空 */
    CM_LogicLinkConnUpdateParamCbk connUpdateParamCbk;   /* 可空 */
} CM_LogicLinkCbks_S;

/**
 * @brief 星闪逻辑链路连接状态
 */
typedef enum {
    CM_STATE_CONNECTED = 0x00, /*!< 已经连接 */
    CM_STATE_CONNECTING,       /*!< 连接中 */
    CM_STATE_DISCONNECTED,     /*!< 已经断开 */
    CM_STATE_DISCONNECTTING,   /*!< 断开中 */
    CM_STATE_CONNECT_UPDATED,  /*!< 连接更新 */
} CM_ConnectState_E;

/**
 * @brief 星闪原语版本号
 */
typedef enum {
    BASE_VERSION = 0x00,
    BASE_VERSION_1_0 = 0x10,
    BASE_VERSION_1_1 = 0x11,
    BASE_VERSION_1_2 = 0x12,
    BASE_VERSION_MAX = 0xFF,
} CM_BaseVersion_E;

/**
 * @brief 星闪调度低时延使能
 */
typedef enum {
    CM_LOW_LATENCY_EN,      /*!< 低时延调度使能 */
    CM_LOW_LATENCY_DISABLE, /*!< 低时延调度关闭 */
} CM_LowLatencyType_E;

#ifdef __cplusplus
}
#endif

#endif