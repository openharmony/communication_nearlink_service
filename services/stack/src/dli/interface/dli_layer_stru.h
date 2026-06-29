/**
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
 * @file         dli_layer_stru.h
 * @brief        dli的layer层的结构体定义
*/


#ifndef DLI_LAYER_STRU_H
#define DLI_LAYER_STRU_H

#include <stdint.h>
#include "sdf_buff.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DLI_HEADER 5 /* data type (1), Handle (2), DLI Payload len (2) */
#define DLI_HEADER_WITHOUT_TYPE_SIZE 4
#define DLI_MAX_TXRX_DATA_LEN 102400 /* 1024 * 100 */

typedef enum {
    DLI_DATA_PRIO_CMD = 0,
    DLI_DATA_PRIO_HIGH,
    DLI_DATA_PRIO_NORMAL,
    DLI_DATA_PRIO_MAX,
} DLI_DataPriority;

/**
 * @brief 命令的dli头部格式定义
 *   0               1               2               3
 *   0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     type      |             cmd               |      len      |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  !     len       |       par
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
typedef struct DLI_CmdStru {
    uint16_t cmd;
    // 在接收cmd对应的ack时，需要event和opcode确定对应的回复
    uint16_t event; // cmd对应的event
    uint16_t parLen;
    void *context; // 上下文的命令数据, layer层不做处理，透传给evt
    /* 超时处理函数,当前入参使用的是uint16_t的数值,强转成了void*指针使用(节省内存) */
    void (*timeoutCallback)(void *param);
    void (*contextFree)(void *context);
    uint8_t par[0]; // 命令行的参数数据
} DLI_CmdStru;

/**
 * @brief 数据的dli头部格式定义
 *   0               1               2               3
 *   0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     type      |    handle             | pb|t|p|     len       |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  !     len       |       data
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
typedef struct DLI_DataStru {
    uint16_t lcid; // 连接的id
    uint8_t type; // 数据类型的type，当支持：DLI_DATATYPE_ACB = 0xA3,DLI_DATATYPE_ICB = 0xA4,
    uint8_t ts; // 上述数据格式定义的t, 是否携带TimeStamp, 1代表携带，0代表不携带
    uint8_t prio; // 上述数据格式定义的p，prio标记(数据包优先级，0表示低优先级，1表示高优先级)
    SDF_Buff_S *buf;
} DLI_DataStru;

enum {
    DLI_DATATYPE_CMD = 0xA1,
    DLI_DATATYPE_EVENT = 0xA2,
    DLI_DATATYPE_ACB = 0xA3,
    DLI_DATATYPE_ICB = 0xA4,
};

typedef enum {
    DLI_FULL_FRAGMENT = 0,
    DLI_FIRST_FRAGMENT,
    DLI_MIDDLE_FRAGMENT,
    DLI_LAST_FRAGMENT,
} DLI_PbFlag;

typedef enum {
    DLI_CMD_TASK = 0,
} DLI_TaskType;

typedef enum {
    ACB_DATA_TYPE = 0,
    ICB_DATA_TYPE,
    UNKNOWN_DATA_TYPE,
} DLI_DataType;
#ifdef __cplusplus
}
#endif
#endif // DLI_LAYER_STRU_H