/****************************************************************************
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
****************************************************************************/

/****************************************************************************
 *
 * this file contains the CM common signaling
 *
 ***************************************************************************/


#ifndef CM_SIGNALING_INTERNAL_H
#define CM_SIGNALING_INTERNAL_H

#include "dtap.h"
#include "sdf_buff.h"
#include "cm_def.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CM_CAP_WND 200    // 可靠模式传输通道默认接收队列的长度，单位PDU个数
#define CM_CAP_MTU 4096   // 可靠模式传输通道默认MTU
#define CM_CAP_MPS 1500   // 可靠模式传输通道默认MPS

#define RELIABLE_MODE_MASK      0x0001
#define STREAM_MODE_MASK        0x0002
#define TRANSPARENT_MODE_MASK   0x0004

typedef struct {
    uint8_t rxWnd;  // 可靠模式接收窗口大小
    uint16_t supportTransMode;  // 支持的传输模式
} CM_CapInfo_S;

/**
 * @brief  连接管理模块信令模块初始化
 * @param [in] 无
 * @return 0 成功，其他失败
 */
uint32_t CM_SignalingInit(void);

/**
 * @brief  连接管理模块信令模块去初始化
 * @param [in] 无
 */
void CM_SignalingDeInit(void);

/**
 * @brief  连接管理模块信令数据发送回调函数类型
 * @param [in] pi 连接管理信令的发送的上层协议指示
 * @param [in] tcid 连接管理信令的发送的传输通道
 * @param [in] lcid 连接管理信令的发送的逻辑链路通道
 * @param [in] buff 连接管理信令的发送的buff
 */
typedef uint32_t (*CM_SendSignalingDataCbk)(uint8_t pi, uint8_t tcid,
    uint16_t lcid, SDF_Buff_S *buff);

/**
 * @brief  CM模块设置信令数据发送的回调方法
 * @param [in] cbk 连接管理信令的发送函数类型
 * @return 无
 */
void CM_SetSendSignalingDataCbk(CM_SendSignalingDataCbk cbk);

/**
 * @brief  CM模块处理信令数据的方法
 * @param  [in] < info > 数据信息指针
 * @param  [in] < buff > 数据buff指针
 * @return [in] 返回值
 */
int CM_RecvSignalingData(DTAP_Data_Info_S *info, SDF_Buff_S *buff);

uint32_t CM_GetLogicLinkCapInfo(CM_CapInfo_S *capInfo, const SLE_Addr_S *addr);

#ifdef __cplusplus
}
#endif

#endif