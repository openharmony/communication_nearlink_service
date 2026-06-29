/****************************************************************************
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
 * this file contains the CM for dynmaic trans channel state management definitions
 *
 ***************************************************************************/

#ifndef CM_DYNC_TRANS_CHAN_STATE_MGR_H
#define CM_DYNC_TRANS_CHAN_STATE_MGR_H

#include "cm_def.h"
#include "cm_trans_channel_api.h"
#include "cm_trans_channel_mgr.h"
#include "cm_dyn_trans_channel_api.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CM_DYN_TRANS_CHAN_EVENT_ACTIVE_ESTABLISHING_REQ = 0,       /* 传输通道主动建立请求事件 */
    CM_DYN_TRANS_CHAN_EVENT_ACTIVE_ESTABLISHED_RSP = 1,        /* 收到传输通道主动建立成功响应事件 */
    CM_DYN_TRANS_CHAN_EVENT_ACTIVE_ESTABLISHING_FAIL_RSP = 2,  /* 收到传输通道主动建立失败响应事件，比如超时 */
    CM_DYN_TRANS_CHAN_EVENT_ACTIVE_RELEASING_REQ = 3,          /* 传输通道主动释放请求事件 */
    CM_DYN_TRANS_CHAN_EVENT_ACTIVE_RELEASED_RSP = 4,           /* 收到传输通道主动释放成功响应事件 */
    CM_DYN_TRANS_CHAN_EVENT_ACTIVE_RELEASING_FAIL_RSP = 5,     /* 传输通道主动释放信令失败事件，比如超时 */
    CM_DYN_TRANS_CHAN_EVENT_PASSIVE_RELEASED_REQ = 6,          /* 收到传输通道被动释放请求事件 */
    CM_DYN_TRANS_CHAN_EVENT_PASSIVE_ESTABLISHED_REQ = 7,       /* 收到传输通道建立请求事件 */
    CM_DYN_TRANS_CHAN_EVENT_MAX                                /* 事件不支持 */
} CM_DynTransChanEventType_E;

typedef struct {
    uint8_t currentState;                                     /* 参见 CM_DynTransChannelState_E 定义 */
    uint8_t event;                                            /* 状态机事件，参见CM_DynTransChanEventType_E定义 */
    uint8_t nextState;                                        /* 参见 CM_DynTransChannelState_E 定义 */
    uint32_t (*func)(uint8_t curState, const CM_DynTransChanParam_S *msg, uint8_t nextState);
} CM_DynTransChanHandlerEvent;

/**
 * @brief 将事件post到dyn trans channel的状态机，并根据定义好的流程处理
 * @return 成功: CM_SUCCESS, 其他：失败
 */
uint32_t CM_DynTransChanPostEventAndHandler(uint8_t event, const CM_DynTransChanParam_S *param);

/**
 * @brief 注册回调
 * @return 无
 */
void CM_DynTransChannStateMgrRegCbks(const CM_DynTransChannelCbks_S *cbks);

/**
 * @brief 取消注册回调
 * @return 无
 */
void CM_DynTransChannStateMgrUnRegCbks(void);

/**
 * @brief 链路断开后，根据lcid释放所有节点
 * @return 无
 */
void CM_DynTransChannReleaseAllNode(uint16_t lcid);

#ifdef __cplusplus
}
#endif

#endif