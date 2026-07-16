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

/****************************************************************************
 *
 * this file contains the CM API definitions
 *
 * LCID: Logic Channel ID, 逻辑链路标识
 * TCID: Trans Channel ID, 传输通道标识
 *
 ***************************************************************************/

#ifndef CM_API_H
#define CM_API_H

#include <stdbool.h>
#include <stdint.h>
#include "sdf_addr.h"
#include "cm_def.h"
#include "dli_cmd_struct.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  @brief  节点类型
 */
typedef enum {
    SLE_G_NODE,
    SLE_T_NODE,
} CM_SleNodeType_E;

/**
 * @brief 星闪无线帧类型，bitmap表示
 */
typedef enum {
    CM_RADIO_FRAME_TYPE_1    = 0, /* 无线帧类型1 */
    CM_RADIO_FRAME_TYPE_2    = 1, /* 无线帧类型2 */
    CM_RADIO_FRAME_TYPE_3_M0 = 2, /* 无线帧类型3，m序列0 */
    CM_RADIO_FRAME_TYPE_3_M1 = 3, /* 无线帧类型3，m序列1 */
    CM_RADIO_FRAME_TYPE_3_M2 = 4,
    CM_RADIO_FRAME_TYPE_3_M3 = 5,
    CM_RADIO_FRAME_TYPE_3_M4 = 6,
    CM_RADIO_FRAME_TYPE_3_M5 = 7,
    CM_RADIO_FRAME_TYPE_4_M0 = 8, /* 无线帧类型4，m序列0 */
    CM_RADIO_FRAME_TYPE_4_M1 = 9, /* 无线帧类型4，m序列1 */
    CM_RADIO_FRAME_TYPE_4_M2 = 10,
    CM_RADIO_FRAME_TYPE_4_M3 = 11,
    CM_RADIO_FRAME_TYPE_4_M4 = 12,
    CM_RADIO_FRAME_TYPE_4_M5 = 13,
    CM_RADIO_FRAME_TYPE_END
} CM_RadioFrameType_E;

/**
 * @brief 星闪发送/接收PHY类型，bitmap表示
 */
typedef enum {
    CM_PHY_TYPE_1M = 0x0,    /* 1M PHY */
    CM_PHY_TYPE_2M = 0x1,    /* 2M PHY */
    CM_PHY_TYPE_4M = 0x2,    /* 4M PHY */
    CM_PHY_TYPE_SUPPORT_NUM,
} CM_TxRxPhy_E;

/**
 * @brief 星闪发送/接收导频密度指示，bitmap表示
 */
typedef enum {
    CM_PILOT_DENSITY_4_TO_1  = 0x0,  /* 导频密度为4:1 */
    CM_PILOT_DENSITY_8_TO_1  = 0x1,  /* 导频密度为8:1 */
    CM_PILOT_DENSITY_16_TO_1 = 0x2,  /* 导频密度为16:1 */
    CM_PILOT_DENSITY_NUM,
} CM_TxRxPilotDensity_E;

/**
 * @brief 星闪对端设备类型
 */
typedef enum {
    CM_INVALID_DEV  = 0x00,  /*!< 无效设备 */
    CM_AUDIO_DEV  = 0x01,    /*!< 音频设备 */
} CM_PeerDevType_E;

/**
 * @brief 星闪连接完成类型
 */
typedef enum {
    CM_CONN_COMPLETE_SCAN = 0,   // 通过扫描建立的连接（主动连接）
    CM_CONN_COMPLETE_ADV = 1,    // 通过广播建立的连接（被动连接）
} CM_ConnCompleteType_E;

#pragma pack (1)

/**
 * @brief 星闪逻辑链路对端参数设置请求通知结构体
 */
typedef struct {
    uint16_t lcid;              /* 星闪链路连接handle值 */
    uint16_t intervalMin;          /* 链路调度间隔最小值，单位slot */
    uint16_t intervalMax;          /* 链路调度间隔最大值，单位slot */
    uint16_t maxLatency;           /* 延迟周期，单位为事件组周期 */
    uint16_t supervisionTimeout;   /* 超时时间，单位10ms */
} CM_ConnectRemoteUpdateParamReq_S;

/**
 * @brief 星闪逻辑链路参数更新响应结构体
 */
typedef struct {
    uint16_t interval;              /* 链路调度间隔，单位slot */
    uint16_t latency;               /* 延迟周期，单位为事件组周期 */
    uint16_t supervisionTimeout;    /* 超时时间，单位10ms */
} CM_ConnectParamUpdateRspExt_S;

/**
 * @brief 星闪逻辑链路参数更新响应结构体
 */
typedef struct {
    uint8_t    version;      /* 原语版本号，当前标准版本设置为0 */
    uint16_t   localIndex;   /* 本地索引 */
    uint8_t    result;       /* 参考链路连接状态定义 */
    uint16_t   lcid;         /* 星闪逻辑链路handle */
    SLE_Addr_S addr;         /* 星闪设备地址 */
    CM_ConnectParamUpdateRspExt_S extension; /* 连接响应扩展参数，协议目前未定位具体结构, 可选 */
} CM_ConnectUpdateParamRsp_S;

/**
 * @brief 星闪逻辑链路更新参数
 */
typedef struct {
    uint8_t    version;            /* 原语版本号，当前标准版本设置为0 */
    uint16_t   localIndex;         /* 本地索引 */
    SLE_Addr_S addr;               /* 星闪链路对端设备地址 */
    uint16_t   intervalMin;        /* 链路调度最小间隔，单位slot */
    uint16_t   intervalMax;        /* 链路调度最大间隔，单位slot */
    uint8_t    txRxInterval;       /* 事件内间隔，单位us */
    uint16_t   eventInterval;      /* 事件间间隔和事件组间间隔，单位us */
    uint16_t   maxLatency;         /* 延迟周期，单位slot */
    uint16_t   supervisionTimeout; /* 超时时间，单位10ms */
    uint8_t    systemTimeUnit;     /* 系统调度时隙，枚举值 */
    uint8_t    txRxFlag;           /* 先发后发指示，指示信令T方在事件中先发送数据还是后发送数据，0为先发，1为后发 */
} CM_ConnectUpdateParamReq_S;

/**
 * @brief  星闪读取对端特性版本结果上报
 */
typedef struct {
    uint16_t lcid;
    uint8_t features[CM_READ_REMOTE_FEATURES_PARAM_LEN];
    uint8_t version;
    uint16_t companyId;
    uint16_t subversion;
    SLE_Addr_S addr;
} CM_ReadRemoteFeatureVersionRsp_S;

/**
 * @brief  星闪读取本端特性上报
 */
typedef struct {
    uint8_t status;
    uint8_t features[CM_FEATURES_PARAM_LEN];
} CM_ReadLocalFeatureRsp_S;

/**
 * @brief  星闪设置对端设备类型上报，与gle_set_peer_dev_type_cmp_t一致
 */
typedef struct {
    uint8_t status;            /* 芯片错误码，详见dli_errno.h */
    SLE_Addr_S addr;
} CM_SetPeerDevType_S;

/**
 * @brief  星闪读取访问过滤列表大小结果上报
 */
typedef struct {
    uint8_t status;     /* 0x00=成功, 0x01-0xFF=失败 */
    uint8_t listSize;       /* 白名单列表支持的最大实体个数 */
} CM_ReadAcceptFilterListSize_S;

/**
 * @brief  星闪设置PHY
 */
typedef struct {
    uint16_t lcid;             /* 连接链路的标识，取值范围[0x0000,0xFFFF] */
    uint8_t txFormat;          /* 发送无线帧类型，参考CM_RadioFrameType_E */
    uint8_t rxFormat;          /* 接收无线帧类型，参考CM_RadioFrameType_E */
    uint8_t txPhy;             /* 发送PHY，参考CM_TxRxPhy_E */
    uint8_t rxPhy;             /* 接收PHY，参考CM_TxRxPhy_E */
    uint8_t txPilotDensity;    /* 发送导频密度指示，参考CM_TxRxPilotDensity_E */
    uint8_t rxPilotDensity;    /* 接收导频密度指示，参考CM_TxRxPilotDensity_E */
    uint8_t gFeedback;         /* 先发链路反馈类型指示，取值范围0-63。
                                     0：指示基于CBG的反馈
                                     1-25：指示不携带数据信息场景组播反馈信息的比特位置, 其中位置信息为指示信息的数值
                                     26：指示基于TB的反馈
                                     27-34：指示携带数据信息场景组播反馈信息的比特位置，其中位置信息为(指示信息的数值-26)
                                     35-63：预留 */
    uint8_t tFeedback;         /* 后发链路反馈类型指示，取值范围0-7
                                     0-5：指示半可靠组播反馈，并指示采用m序列的编号
                                     6：指示基于CBG的反馈
                                     7：指示基于TB的反馈 */
} CM_SetPhyReq_S;

/**
 * @brief  星闪Host侧给Controller侧指定偏好的信道分类
 */
typedef struct {
    uint8_t channelMap[CM_CHANNEL_MAP_LEN];
} CM_SetChannelMapReq_S;

/**
 * @brief  星闪Host侧设置过滤指令规则
 */
typedef struct {
    uint16_t lcid;          /* 连接链路的标识，取值范围[0x0000,0xFFFF] */
    uint8_t enabled;        /* 开始或者关闭filter */
    uint8_t index;          /* 过滤位置，从0开始 */
    uint8_t filterCode;     /* 过滤值 */
} CM_SetRxDataFilterReq_S;

typedef struct {
    uint8_t status;             /* 0x00: PHY参数更新成功, 其他: PHY参数更新失败 */
    uint16_t lcid;              /* 连接链路的标识，取值范围[0x0000,0xFFFF] */
    uint8_t txFormat;           /* 发送无线帧类型，参考CM_RadioFrameType_E */
    uint8_t rxFormat;           /* 接收无线帧类型，参考CM_RadioFrameType_E */
    uint8_t txPhy;              /* 发送PHY，参考CM_TxRxPhy_E */
    uint8_t rxPhy;              /* 接收PHY，参考CM_TxRxPhy_E */
    uint8_t txPilotDensity;     /* 发送导频密度指示，参考CM_TxRxPilotDensity_E */
    uint8_t rxPilotDensity;     /* 接收导频密度指示，参考CM_TxRxPilotDensity_E */
    uint8_t gFeedback;          /* 先发链路反馈类型指示，取值范围0-63。
                                     0：指示基于CBG的反馈
                                     1-25：指示不携带数据信息场景组播反馈信息的比特位置, 其中位置信息为指示信息的数值
                                     26：指示基于TB的反馈
                                     27-34：指示携带数据信息场景组播反馈信息的比特位置，其中位置信息为(指示信息的数值-26)
                                     35-63：预留 */
    uint8_t tFeedback;          /* 后发链路反馈类型指示，取值范围0-7
                                     0-5：指示半可靠组播反馈，并指示采用m序列的编号
                                     6：指示基于CBG的反馈
                                     7：指示基于TB的反馈 */
} CM_SetPhyRsp_S;

typedef struct {
    uint16_t lcid;          /* 连接链路的标识，取值范围[0x0000,0xFFFF] */
    bool enable;            /* false: 不允许使用高功率，true: 允许使用高功率 */
    uint8_t powerLevel;     /* 高功率档位: 7档或者8档等 */
} CM_EnableConnHighPowerReq_S;

typedef struct {
    uint8_t status;
    uint16_t lcid;
} CM_EnableConnHighPowerRsp_S;

typedef struct {
    uint16_t lcid;
} CM_ReadRemoteRssiReq_S;

typedef struct {
    uint16_t lcid;
    uint8_t status;
    int8_t rssi;
} CM_ReadRemoteRssiRsp_S;

typedef struct {
    SLE_Addr_S addr;
    CM_PeerDevType_E peerDevType;
} CM_SetPeerDevTypeParam;

typedef struct {
    SLE_Addr_S addr;
    uint16_t subrate;                   /* 单位为10ms */
    bool onlySubrate;                   /* true: 只使用subrate，false: 使用subrate和下列参数 */
    uint16_t subrateMax;                /* 基础周期的最大倍数 */
    uint16_t maxLatency;                /* T侧跳过基础周期的个数，默认值0 */
    uint16_t continuationNum;           /* 当前执行周期里面基础周期的执行个数 */
    uint16_t supervisionTimeout;        /* 超时时间，单位10ms */
} CM_SetACBSubrateParam;

typedef struct {
    uint8_t status;         /* 0x00: 设置RxDataFilter成功, 其他: 设置RxDataFilter失败 */
    uint16_t lcid;          /* 连接链路的标识，取值范围[0x0000,0xFFFF] */
} CM_SetRxDataFilterRsp_S;

typedef struct {
    SLE_Addr_S addr;                    /* 星闪链路对端设备地址 */
    uint32_t subrate;                   /* 当前链路的subrate值 */
    uint16_t subrateMax;                /* 基础周期的最大倍数 */
    uint16_t maxLatency;                /* T侧跳过基础周期的个数，默认值0 */
    uint16_t continuationNum;           /* 当前执行周期里面基础周期的执行个数 */
    uint16_t supervisionTimeout;        /* 超时时间，单位10ms */
} CM_AcbSubrateCbParam_S;

#pragma pack ()

/**
 * @brief  连接管理模块对端参数更新请求回调函数类型
 */
typedef void (*CM_ConnectRemoteUpdateParamReqCbk)(CM_ConnectRemoteUpdateParamReq_S *param);

/**
 * @brief  连接管理模块参数更新回调函数类型
 */
typedef void (*CM_ConnectUpdateParamCbk)(CM_ConnectUpdateParamRsp_S *param);

/**
 * @brief  连接管理模块设置PHY参数回调函数类型
 */
typedef void (*CM_SetPhyCbk)(CM_SetPhyRsp_S *param);

/**
 * @brief  连接管理模块读取对端特性版本回调函数类型
 */
typedef void (*CM_ReadRemoteFeatureVersionCbk)(CM_ReadRemoteFeatureVersionRsp_S *feature);

/**
 * @brief  连接管理模块读取本端特性版本回调函数类型
 */
typedef void (*CM_ReadLocalFeatureCbk)(CM_ReadLocalFeatureRsp_S *feature);

/**
 * @brief  连接管理模块使能连接高功率回调函数类型
 */
typedef void (*CM_EnableConnHighPowerCbk)(CM_EnableConnHighPowerRsp_S *rsp);

/**
 * @brief  连接管理模块设置对端设备类型回调函数类型
 */
typedef void (*CM_SetPeerDevTypeCbk)(CM_SetPeerDevType_S *param);

/**
 * @brief  连接管理模块读取白名单列表大小回调函数类型
 */
typedef void (*CM_ReadAcceptFilterListSizeCbk)(CM_ReadAcceptFilterListSize_S *param);

/**
 * @brief  连接管理模块设置RxDataFilter参数回调函数类型
 */
typedef void (*CM_SetRxDataFilterCbk)(CM_SetRxDataFilterRsp_S *param);

typedef void (*CM_SetAcbSubrateCbk)(CM_AcbSubrateCbParam_S *param);

typedef void (*CM_ReqAcbSubrateCbk)(CM_AcbSubrateCbParam_S *param);

typedef void (*CM_ReadRemoteRssiCbk)(CM_ReadRemoteRssiRsp_S *rsp);

/**
 * @brief  连接管理模块回调函数
 */
typedef struct CM_ConnectCbks {
    CM_ConnectRemoteUpdateParamReqCbk connRemoteUpdateParamReqCbk; /* 必填 */
    CM_ConnectUpdateParamCbk connUpdateParamCbk;                   /* 必填 */
    CM_ReadRemoteFeatureVersionCbk readRemoteFeatureVersionCbk;    /* 必填 */
    CM_SetPhyCbk setPhyCbk;                                        /* 必填 */
    CM_ReadLocalFeatureCbk readLocalFeatureCbk;                    /* 可选 */
    CM_EnableConnHighPowerCbk enableConnHighPowerCbk;              /* 可选 */
    CM_SetPeerDevTypeCbk setPeerDevTypeCbk;                        /* 可选 */
    CM_SetRxDataFilterCbk setRxDataFilterCbk;                      /* 可选 */
    CM_SetAcbSubrateCbk setAcbSubrateCbk;                          /* 可选 */
    CM_ReqAcbSubrateCbk reqAcbSubrateCbk;                          /* 可选 */
    CM_ReadAcceptFilterListSizeCbk readAcceptFilterListSizeCbk;    /* 可选 */
    CM_ReadRemoteRssiCbk readRemoteRssiCbk;                        /* 可选 */
} CM_ConnectCbks_S;

/**
 * @brief  CM模块初始化
 * @param [in] 无
 * @return SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_Init(void);

/**
 * @brief  CM模块销毁
 * @return void
 */
void CM_DeInit(void);

/**
 * @brief  CM模块使能
 * @param [in] 无
 * @return SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_Enable(void);

/**
 * @brief  CM模块去使能
 * @return void
 */
void CM_Disable(void);

/**
 * @brief  CM模块注册连接相关回调
 * @param [in] < cbks > CM模块外部注册回调指针, 参见CM_ConnectCbks_S定义
 * @return SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_RegConnectCbks(CM_ConnectCbks_S *cbks);

/**
 * @brief  连接链路参数更新
 * @param  [in] < param > 配置参数, 参见CM_ConnectUpdateParamReq_S定义
 * @return SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_ConnectUpdateParamReq(CM_ConnectUpdateParamReq_S *param);

/**
 * @brief  读取访问过滤列表大小
 * @param  [in] 无
 * @return 无
 */
void CM_ReadAcceptFilterListSize(void);

/**
 * @brief  设置PHY参数
 * @param  [in] < param > PHY参数, 参见CM_SetPhyReq_S定义
 * @return SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_SetPhy(CM_SetPhyReq_S *param);

/**
 * @brief  Host侧给Controller侧指定偏好的信道分类
 * @param  [in] param : 可用频点, bit0代表2402 bit1代表2403，依次类推，对应bit为1代表可用
 * @return SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_SetHostChannelClassification(CM_SetChannelMapReq_S *param);

/**
 * @brief  使能星闪高功率连接
 * @param  [in]  < param > 使能参数
 * @return 0: 成功, OTHER: 失败
 */
uint32_t CM_EnableRealConnHighPower(CM_EnableConnHighPowerReq_S *inParam,
    uint32_t (*enableConnHighPower)(DLI_EnableConnHighPowerParam *param));

/**
 * @brief  设置异步链路的subrate
 * @param  [in] param : acb subrate参数
 * @param  [in] setSubrate : 设置acb subrate命令的方法
 * @return SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_SetRealACBSubrate(const CM_SetACBSubrateParam *param,
    uint32_t (*setSubrate)(DLI_ACBSubrateParam *dliParam));

/**
 * @brief  星闪链路获取rssi
 * @param  [in]  < connHandle > 同步链路id
 * @return CM_SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_GetRssi(const CM_ReadRemoteRssiReq_S *param);

/**
 * @brief  查询同步链路对应的异步链路id
 * @param  [in]  < connHandle > 同步链路id
 * @return CM_SUCCESS: 成功, OTHER: 失败
 */
uint16_t CM_GetLcidByConnHandle(uint16_t connHandle);

#ifdef __cplusplus
}
#endif

#endif