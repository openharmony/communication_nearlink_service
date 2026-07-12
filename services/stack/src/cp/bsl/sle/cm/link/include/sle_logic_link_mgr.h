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
 * this file contains device logic link manager defines
 *
 ***************************************************************************/


#ifndef SLE_LOGIC_LINK_MG_H
#define SLE_LOGIC_LINK_MG_H

#include <stdint.h>
#include "sdf_dlist.h"
#include "sdf_addr.h"
#include "cm_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 逻辑链路中心保存的链路参数
 */
typedef struct {
    SDF_DListEntry_S entry;                     /* 链表节点 */
    uint8_t role;                               /* 链路角色：G或者T */
    uint8_t status;                             /* 链路状态, 参见 CM_ConnectLinkState_E */
    uint16_t timeout;                           /* 链路超时断链时间 */
    SLE_Addr_S locAddr;                         /* 连接本地链路地址 */
    SLE_Addr_S rmtAddr;                         /* 连接对端链路地址 */
    uint16_t lcid;                              /* 链路句柄 */
    uint16_t connInterval;                      /* 链路调度间隔 */
    bool isSubrateUpdating;                     /* true表示正在更新subrate */
    uint16_t subrate;                           /* subrate，单位10ms */
    uint16_t subrateMax;                        /* 基础周期的最大倍数 */
    uint16_t maxLatency;                        /* T侧跳过基础周期的个数，默认值0 */
    uint16_t continuationNum;                   /* 当前执行周期里面基础周期的执行个数 */
    uint16_t supervisionTimeout;                /* 超时时间，单位10ms */
    uint16_t centralClockAccuracy;              /* 逻辑链路AAC */
    uint8_t channelMode;                        /* 数据传输方式，单播、数据组播、反馈组播、双向组播、广播 */
    uint16_t maxTxOctets;                       /* 最大tx大小 */
    uint16_t maxRxOctets;                       /* 最大rx大小 */
    uint8_t features[CM_FEATURES_PARAM_LEN];    /* 对端设备的features */
    uint8_t remotePrivateFeature[CM_PRIVATE_FEATURES_LEN]; /* 对端设备的私有features */
    uint8_t version;                            /* 版本号 */
    uint16_t companyId;                         /* 厂商编号 */
    uint16_t subversion;                        /* 子版本号 */
    uint16_t devType;                           /* 设备类型 */
    uint16_t mtu;                               /* 最大MTU */
    uint16_t mps;                               /* 最大MPS */
    uint8_t rxWnd;                              /* 对端可靠模式传输通道接收队列长度 */
    uint16_t supportTransMode;                  /* 对端支持的传输模式 */
    uint16_t protocolVersion;                   /* 协议版本 */
    uint16_t exchangeVersion;                   /* 记录exchange消息中的version */
    uint8_t discReason;                         /* 链路断链原因 */
    uint8_t connCompleteType;                   /* 连接完成类型，0：通过本端扫描建立的连接，1：通过本端广播建立的连接, 参见
                                                   CM_ConnCompleteType_E定义 */
    uint8_t advHandle;                          /* 广播标识，若是通过本端广播建立的连接，则为对应启动广播标识，
                                                   否则默认为0，无效值 */
} SleLogicLink_S;

/**
 * @brief 链路管理初始化
 */
void SleLogicLinkInit(void);

/**
 * @brief 链路管理去初始化
 */
void SleLogicLinkDeInit(void);

/**
 * @brief 新增对端设备链路
 * @param [in] addr:设备address
 */
SleLogicLink_S *SleLogicLinkAdd(SLE_Addr_S *addr);

/**
 * @brief 删除对端设备链路
 * @param [in] node:节点
 */
void SleLogicLinkRemove(SleLogicLink_S *node);

/**
 * @brief 通过地址查询对端设备链路
 * @param [in] addr:设备address
 */
SleLogicLink_S *SleLogicLinkGetByAddr(const SLE_Addr_S *addr);

/**
 * @brief 根据lcid查询对端设备链路
 * @param [in] lcid:链路handle
 */
SleLogicLink_S *SleLogicLinkGetByLcid(uint16_t lcid);

/**
 * @brief 根据status查询对端设备链路
 * @param [in] status:链路状态
 */
SleLogicLink_S *SleLogicLinkGetByStatus(uint16_t status);

/**
 * @brief 获取已完成连接的连接数
 */
uint32_t SleLogicLinkGetConnectedSize(void);

#ifdef __cplusplus
}
#endif

#endif // SLE_LOGIC_LINK_MG_H