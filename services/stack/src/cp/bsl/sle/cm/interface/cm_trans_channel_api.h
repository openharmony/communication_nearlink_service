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
 * this file contains the CM for trans channel sync API definitions
 *
 ***************************************************************************/

#ifndef CM_TRANS_CHANNEL_API_H
#define CM_TRANS_CHANNEL_API_H

#include <stdint.h>
#include "sdf_addr.h"
#include "cm_def.h"
#include "sdf_vector.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CM_TRANS_INVALID_TCID         0x00
#define CM_TRANS_INVALID_AID          3999

/**
 * @brief 星闪传输通道标识定义
 */
typedef enum {
    CM_TCID_SLB_CMTC = 0x01,        /* SLB通用管理传输通道(Common Management Transmission Channel) */
    CM_TCID_SLE_CMTC = 0x02,        /* SLE通用管理传输通道(Common Management Transmission Channel) */

    CM_TCID_SLB_SMTC = 0x09,        /* SLB服务管理传输通道(Service Management Transmission Channel) */
    CM_TCID_SLE_SMTC = 0x0A,        /* SLE服务管理传输通道(Service Management Transmission Channel) */

    CM_TCID_SLB_RSMTC = 0x10,       /* SLB中继服务管理传输通道(Relay Service Management Transmission Channel) */
    CM_TCID_MDCMTC = 0x11,          /* 多域协调与管理传输通道(Multi-Domain Coordination and Managemet Transmission Channel) */
    CM_TCID_5GITC = 0x12,           /* 5G融合传输通道(5G Interworking Transmission Channel) */

    CM_TCID_FTC_RFU_END = 0x1D,     /* 固定传输通道RFU(Reserved for Future Use)结束 */

    CM_TCID_SLB_CUTC = 0x1E,        /* SLB默认单播数据传输通道(Common Unicast Transmission Channel) */
    CM_TCID_SLE_CUTC = 0x1F,        /* SLE默认单播数据传输通道(Common Unicast Transmission Channel) */

    CM_TCID_BC_BEGIN = 0x20,        /* 广播动态分配起始 */
    CM_TCID_BC_END = 0x4F,          /* 广播动态分配结束 */

    CM_TCID_MC_BEGIN = 0x50,        /* 组播动态分配起始 */
    CM_TCID_MC_END = 0x7F,          /* 组播动态分配结束 */

    CM_TCID_UC_BEGIN = 0x80,        /* 单播动态分配起始 */
    CM_TCID_UC_END = 0xDF,          /* 单播动态分配结束 */

    CM_TCID_MAX = 0xFF              /* 最大传输通道标识符 */
} CM_Tcid_E;

/**
 * @brief 星闪传输通道变更状态
 */
typedef enum {
    CM_TRANS_CHANNEL_STATE_INIT = 0x00,    /* 初始化状态 */
    CM_TRANS_CHANNEL_STATE_ACTIVATING,     /* 激活中 */
    CM_TRANS_CHANNEL_STATE_ACTIVATED,      /* 已经激活 */
    CM_TRANS_CHANNEL_STATE_RELEASING,      /* 释放中 */
    CM_TRANS_CHANNEL_STATE_RELEASED,       /* 已经释放或者建立失败 */
} CM_TransChannelState_E;

/**
 * @brief  传输通道信息
 */
typedef struct {
    uint8_t castMode; /* 广播类型，详见CM_AccessTransportMode_E */
    uint16_t lcid;    /* 逻辑链路标识 */
    uint8_t srcTcid;  /* 本设备传输通道标识 */
    uint8_t dstTcid;  /* 对端设备传输通道标识 */
    CM_TransModeConfig_S config;   /* 传输模式配置 */
} CM_TransChan_S;

/**
 * @brief 星闪传输通道状态变化结构体
 */
typedef struct {
    uint8_t  result;               /* 参考链路连接状态CM_TransChannelState_E定义 */
    SDF_Vector_S *channelVector;   /* 元素CM_TransChan_S集合 */
} CM_TransChannelStateList_S;

/**
 * @brief  连接传输通道变化回调函数类型
 */
typedef void (*CM_TransChannelCbk)(CM_TransChannelStateList_S *state);

/**
 * @brief  DTAP模块注册传输通道变化监听回调
 * @return SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_RegTransChannelListener(CM_TransChannelCbk cbk);

/**
 * @brief  DTAP模块取消注册传输通道变化回调
 * @param [in] < void >
 * @return 无
 */
void CM_UnRegTransChannelListener(void);

#ifdef __cplusplus
}
#endif

#endif