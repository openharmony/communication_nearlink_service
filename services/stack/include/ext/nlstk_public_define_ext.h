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

#ifndef NLSTK_PUBLIC_DEFINE_EXT_H
#define NLSTK_PUBLIC_DEFINE_EXT_H

#include <stdint.h>
#include "sdf_addr.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NLSTK_OK 0
#define NLSTK_ERR 1

// 此宏定义用于NLSTK接口的超时，因为NLSTK的独立线程运行，在用户调用NLSTK的同步接口的时候，需要定义最长等待时间，避免挂死
#define NLSTK_API_TIME_OUT 3000

#define NLSTK_ERRCODE_COMMON_INTER_BEGIN               1
#define NLSTK_ERRCODE_SSAP_INTER_BEGIN                 0x100
#define NLSTK_ERRCODE_COMMON_INTER_END                 0x800

// 无效的LCID
#define NLSTK_INVALID_LCID                   ((uint16_t)0xFFFF)

#ifndef NLSTK_ERRCODE
typedef uint32_t NLSTK_ERRCODE;
#endif

/* 协议栈方法调用逻辑error code，非协议相关, 与对外error code 保持一致 */
typedef enum {
    NLSTK_ERRCODE_SUCCESS = 0,
    NLSTK_ERRCODE_BASE = NLSTK_ERRCODE_COMMON_INTER_BEGIN,      /* base */
    NLSTK_ERRCODE_DIRECT_RETURN,                                /* direct return */
    NLSTK_ERRCODE_NO_ATTATION,                                  /* no attation */
    NLSTK_ERRCODE_PARAM_ERR,                                    /* parameter error */
    NLSTK_ERRCODE_FAIL,                                         /* configure fail */
    NLSTK_ERRCODE_TIMEOUT,                                      /* configure timeout */
    NLSTK_ERRCODE_UNSUPPORTED,                                  /* unsupported parameter */
    NLSTK_ERRCODE_GETRECORD_FAIL,                               /* get current record fail */
    NLSTK_ERRCODE_POINTER_NULL,                                 /* pointer is NULL */
    NLSTK_ERRCODE_NO_RECORD,                                    /* no record return */
    NLSTK_ERRCODE_STATUS_ERR,                                   /* state error */
    NLSTK_ERRCODE_NOMEM,                                        /* no memory */
    NLSTK_ERRCODE_AUTH_FAIL,                                    /* authentication fail */
    NLSTK_ERRCODE_AUTH_PKEY_MISS,                               /* authentication fail due to pin code or key lost */
    NLSTK_ERRCODE_RMT_DEV_DOWN,                                 /* remote device down */
    NLSTK_ERRCODE_PAIRING_REJECT,                               /* pair reject */
    NLSTK_ERRCODE_BUSY,                                         /* system busy */
    NLSTK_ERRCODE_NOT_READY,                                    /* system not ready */
    NLSTK_ERRCODE_CONN_FAIL,                                    /* connect fail */
    NLSTK_ERRCODE_OUT_OF_RANGE,                                 /* out of range */
    NLSTK_ERRCODE_MEMCPY_FAIL,
    NLSTK_ERRCODE_MALLOC_FAIL,
    NLSTK_ERRCODE_NOT_IMPLEMENT,                                /* uapi not implement */
    NLSTK_ERRCODE_TASK_FAIL,                                    /* post task to stack thread fail */
    NLSTK_ERRCODE_SYS_ERROR,                                    /* system error */
    NLSTK_ERRCODE_TASK_TIMEOUT,                                 /* task timeout */
    NLSTK_ERRCODE_BUF_LEN_ERR,                                  /* buf len not enough */

    NLSTK_ERRCODE_SSAP_BASE = NLSTK_ERRCODE_SSAP_INTER_BEGIN,
    NLSTK_ERRCODE_SSAP_NO_INVALIDE_HANLDE,                      /* no invalide handle */

    NLSTK_HADM_ERRCODE_BASE = 0x2000,
    NLSTK_HADM_ERRCODE_CAN_NOT_FIND_LINKCB,                     /* can not find linkcb */
    NLSTK_HADM_ERRCODE_LINK_EXCEPTION,                          /* link exception */
    NLSTK_HADM_ERRCODE_ADDR_ALREADY_IN_SOUNDING,                /* 改地址正在测距中 */
    NLSTK_HADM_ERRCODE_MAX_PARALLEL_SOUNDING_NUM,               /* 并发测距任务达到最大值 */
    NLSTK_HADM_ERRCODE_CONFIG_CM_FAIL,                          /* 调用CM接口配置链路参数失败 */
    NLSTK_HADM_ERRCODE_CONFIG_DLI_FAIL,                         /* 调用DLI接口配置链路参数失败 */
    NLSTK_HADM_ERRCODE_CALL_CM_FAIL,                            /* 调用CM接口失败 */
    NLSTK_HADM_ERRCODE_CALL_DLI_FAIL,                           /* 调用DLI接口失败 */
    NLSTK_HADM_ERRCODE_PEER_NOT_SUPPORT_SOUNDING,               /* 对端不支持测距 */
    NLSTK_ERRCODE_MAX = NLSTK_ERRCODE_COMMON_INTER_END          /* 最大错误码边界 */
} NLSTK_Errcode_E;

typedef struct {
    uint16_t len;
    uint8_t *data;
} NLSTK_VariableData_S;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
