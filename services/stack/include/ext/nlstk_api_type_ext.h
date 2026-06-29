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

#ifndef NLSTK_API_TYPE_EXT_H
#define NLSTK_API_TYPE_EXT_H

#include "sdf_struct.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  地址类型
 */
typedef enum {
    SLE_PUBLIC_ADDRESS_TYPE = 0x0,       /*!< 第一类短机构标识 */
    SLE_LONG_MECH_PUBLIC_ADDRESS_TYPE,   /*!< 第二类长机构标识 */
    SLE_LOCAL_MECH_PUBLIC_ADDRESS_TYPE,  /*!< 第三类机构本地标识 */
    SLE_RPA_RESOLV_ADDRESS_TYPE,         /*!< 可解析随机标识 */
    SLE_RPA_UNRESOLV_ADDRESS_TYPE,       /*!< 不可解析随机标识 */
    SLE_RESERVE_ADDRESS_TYPE,            /*!< 第五类联盟预留标识块 */
    SLE_RANDOM_ADDRESS_TYPE,             /*!< 私有标识 */
    SLE_ADDR_TYPE_END
} NLSTK_AddrType_E;

/**
 * @brief 星闪链路subrate类型
 */
typedef enum : uint16_t {
    NLSTK_DEFAULT_SUBRATE = 0x06,               /*!< 音频场景默认subrate */
    NLSTK_MULTI_LINK_SUBRATE = 0x0C,            /*!< 音频场景多连接subrate */
    NLSTK_SUBRATE_2 = 0x02,                     /*!< 语音唤醒提速subrate */
} NLSTK_SubrateType_E;

#pragma pack (1)

typedef struct {
    uint16_t conHandle;
    bool enable;
    uint8_t powerLevel;     /* 高功率档位: 7档或者8档等 */
} NLSTK_EnableConnHighPowerParam_S;

/**
 * @brief  设置异步链路subrate，与CM_SetACBSubrateParam保持一致
 */
typedef struct {
    SLE_Addr_S addr;                     /* 星闪链路对端设备地址 */
    NLSTK_SubrateType_E subrate;
    bool onlySubrate;                   /* true: 只设置subrate，false: 设置subrate和下列参数 */
    uint16_t subrateMax;                /* 基础周期的最大倍数 */
    uint16_t maxLatency;                /* peripheral侧跳过基础周期的个数，默认值0 */
    uint16_t continuationNum;           /* 当前执行周期里面基础周期的执行个数 */
    uint16_t supervisionTimeout;        /* 超时时间，单位10ms */
} NLSTK_SetAcbSubrateParam_S;
#pragma pack ()
#ifndef NLSTK_REMOTE_FEATURE_LEN
#define NLSTK_REMOTE_FEATURE_LEN 16
#endif
typedef struct{
    uint16_t lcid;
    uint8_t features[NLSTK_REMOTE_FEATURE_LEN]; /*!< 对端的feature使能情况 */
} NLSTK_RemoteFeatureInfo_S;
#ifdef __cplusplus
}
#endif

#endif /* NLSTK_API_TYPE_EXT_H */
