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
 * this file contains cm transmission channel manager
 *
 ***************************************************************************/

#ifndef CM_TRANS_CHANNEL_MGR_H
#define CM_TRANS_CHANNEL_MGR_H

#include <stdbool.h>
#include <stdint.h>
#include "sdf_addr.h"
#include "sdf_dlist.h"
#include "sdf_vector.h"
#include "cm_trans_channel_api.h"
#include "cm_dyn_trans_channel_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  range of fixed and dynamic TCID
 */
#define CM_TRANSMISSION_CHANNEL_FIXED_ID_MIN        CM_TCID_SLB_CMTC
#define CM_TRANSMISSION_CHANNEL_FIXED_ID_MAX        CM_TCID_SLE_CUTC
#define CM_TRANSMISSION_CHANNEL_DYNAMIC_ID_MIN      CM_TCID_BC_BEGIN
#define CM_TRANSMISSION_CHANNEL_DYNAMIC_ID_MAX      CM_TCID_MAX

/**
 * @brief 星闪传输通道支持的AID数目
 */
#define CM_TRANS_CHAN_AID_SURPPORT_NUM 1U

/**
 * @brief 星闪传输通道支持的SLQI数目
 */
#define CM_TRANS_CHAN_SLQI_SURPPORT_NUM 1U

#define CM_TRANS_CHANNEL_IS_FIXED(tcid) \
    (((tcid) >= (CM_TRANSMISSION_CHANNEL_FIXED_ID_MIN)) && (tcid) <= (CM_TRANSMISSION_CHANNEL_FIXED_ID_MAX))

#define CM_TRANS_CHANNEL_IS_DYNAMIC(tcid) \
    (((tcid) >= (CM_TRANSMISSION_CHANNEL_DYNAMIC_ID_MIN)) && (tcid) <= (CM_TRANSMISSION_CHANNEL_DYNAMIC_ID_MAX))

/**
 * @brief 星闪两端设备传输标识位置索引
 */
typedef enum {
    CM_TRANS_LOCAL    = 0,  /* 本端 */
    CM_TRANS_REMOTE   = 1,  /* 对端 */
    CM_TRANS_INDEX_MAX
} CM_TransDeviceIndex_E;

/**
 * @brief  星闪接入层专用参数
 */
typedef struct {
    uint8_t reserv;
} SleTransAccessParam_S;

/**
 * @brief  连接管理传输通道参数
 */
typedef struct SleTransLcidParam {
    uint8_t version;                            /* 原语版本号，当前标准版本设置为0 */
    uint16_t localIndex;                        /* 本地索引 */
    CM_TransModeConfig_S transModeConfig;       /* 传输模式配置 */
    uint8_t castMode;                           /* 期望的星闪接入层数据传输方式，详见CM_AccessTransportMode_E */
    uint8_t channelMode;                        /* 期望的逻辑信道模式，SLE：同步等时，异步 */
    uint8_t direction;                          /* 期望逻辑信道能够支持的传输方向，仅G链路、仅T链路、双向 */
    uint16_t aid;                               /* 业务标识，基础应用层业务标识，详见《星闪无线通信系统 架构》, 无效值为3999*/
    uint8_t exclusive;                          /* 指示希望建立独享的逻辑信道，TRUE / FALSE, 默认值为TRUE */
    uint16_t srcPort;                           /* 本端端口号 */
    uint16_t dstPort;                           /* 对端端口号 */
    uint8_t slqi;                               /* 星闪QoS参数索引 */
    SleTransAccessParam_S *accessParams;        /* 星闪接入层专用参数指示 */
    CM_TransConnFrameType_E frameType;
} SleTransLcidParam_S;

typedef struct SleTransLcid {
    SDF_DListEntry_S entry;                     /* 链表节点 */
    uint16_t lcid;
    SLE_Addr_S addr;
    uint8_t state;                              /* 动态传输通道状态机状态, 参见CM_TransChannelState_E定义 */
    SleTransLcidParam_S *params;                /* optional parameters of logic link create connect request */
    uint8_t lastTransid[CM_TRANS_INDEX_MAX];
} SleTransLcid_S;

/**
 * @brief 连接管理动态传输通道参数
 */
typedef struct {
    uint8_t version;                            /* 原语版本号，当前标准版本设置为0 */
    uint16_t localIndex;                        /* 本地索引 */
    uint16_t lcid;
    uint8_t reqId;                              /* 信令模块请求标识 */
    uint8_t localTcid;                          /* 本端TCID */
    uint8_t remoteTcid;                         /* 对端TCID */
    uint16_t srcPort;                           /* 本端端口号 */
    uint16_t dstPort;                           /* 对端端口号 */
    CM_TransModeConfig_S transModeConfig;       /* 传输模式配置 */
    uint8_t channelMode;                        /* 期望的逻辑信道模式，SLE：同步等时，异步 */
    uint8_t direction;                          /* 期望逻辑信道能够支持的传输方向，仅G链路、仅T链路、双向 */
    uint16_t aid;                               /* 业务标识 */
    uint8_t slqi;                               /* 星闪QoS参数索引 */
    uint8_t exclusive;                          /* 指示希望建立独享的逻辑信道，TRUE / FALSE, 默认值为TRUE */
    bool measure;                               /* 是否开启传输通道测量 */
    SleTransAccessParam_S *accessParams;        /* 星闪接入层专用参数指示 */
    uint8_t expectedTransportMode;              /* CAST模式, 参见CM_AccessTransportMode_E定义 */
    uint8_t result;                             /* 通道操作失败原因, 参见CM_TransChanEstablishRspResult_E定义 */
    uint8_t state;                              /* 动态传输通道状态机状态, 参见CM_TransChannelState_E定义 */
    CM_TransConnFrameType_E frameType;
} CM_DynTransChanParam_S;

/**
 * @brief 传输通道管理初始化
 */
void CM_TransChannelMgrInit(void);

/**
 * @brief 传输通道管理去初始化
 */
void CM_TransChannelMgrDeInit(void);

/**
 * @brief 注册监听传输通道激活回调
 */
uint32_t CM_RegTransChannelCbk(CM_TransChannelCbk cbk);

/**
 * @brief 取消注册监听传输通道激活回调
 */
void CM_UnRegTransChannelCbk(void);

/**
 * @brief 激活默认传输通道
 */
uint32_t CM_ActivateFixedTransChannel(uint16_t lcid, SleTransLcidParam_S *params);

/*
 * @brief 激活动态传输通道
 */
SleTransLcid_S *CM_ActivateDynamicTransChannel(uint16_t lcid, const CM_DynTransChanParam_S *params);

/*
 * @brief 动态传输通道创建成功后响应设置remoteTcid等参数信息
 */
void CM_DynamicTransChannelSetTcidAndChangeCbk(SleTransLcid_S *node, uint8_t remoteTcid, uint8_t state);

/**
 * @brief 释放默认传输通道
 */
void CM_ReleaseFixedTransChannel(uint16_t lcid);

/**
 * @brief 通过localTcid释放动态传输通道
 */
void CM_ReleaseDynamicTransChannelByLocalTcid(SleTransLcid_S *node);

/**
 * @brief 通过remoteTcid释放动态传输通道
 */
void CM_ReleaseDynamicTransChannelByRemoteTcid(SleTransLcid_S *node);

/**
 * @brief 根据lcid判断是否已经建立固定传输通道
 */
bool CM_IsExistFixedTransChannelByLcid(uint16_t lcid);

/**
 * @brief 根据lcid判断是否已经建立动态传输通道
 */
bool CM_IsExistDynamicTransChannelByLcid(uint16_t lcid);

/**
 * @brief 根据lcid查找固定传输传输通道，参数transVector<SleTransLcid_S *>传输通道信息，注意：保存成员指针，无需手动释放
 */
uint32_t CM_FindFixedTransChannelByLcid(uint16_t lcid, SDF_Vector_S *transVector);

/**
 * @brief 根据lcid查找动态传输传输通道，参数transVector<SleTransLcid_S *>传输通道信息，注意：保存成员指针，无需手动释放
 */
uint32_t CM_FindDynTransChannelByLcid(uint16_t lcid, SDF_Vector_S *transVector);

/**
 * @brief 根据localTcid查找传输传输通道（根据tcid区分固定通道还是动态通道）
 */
SleTransLcid_S *CM_FindTransChannelByLocalTcid(uint16_t lcid, uint8_t tcid);

/**
 * @brief 根据remoteTcid查找传输传输通道（根据tcid区分固定通道还是动态通道）
 */
SleTransLcid_S *CM_FindTransChannelByRemoteTcid(uint16_t lcid, uint8_t tcid);

#ifdef __cplusplus
}
#endif

#endif