/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
/*
 * @file HADM_Link_Manager.h
 * @brief HADM链路管理模块，用于管理链路状态信息和核心资源处理模块
 * @details 该模块提供链路控制块的管理功能，包括链路状态的设置与获取、
 *          遥远参数的缓存与管理、用户操作的记录与查询等功能。
 */

#ifndef HADM_LINK_MANAGER_H
#define HADM_LINK_MANAGER_H

#include <stdint.h>
#include "sdf_addr.h"
#include "hadm_api.h"
#include "hadm_parser_iq.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------ */
/*  枚举类型定义                                                           */
/* ------------------------------------------------------------------------ */

/**
 * @brief 声呐状态枚举类型
 * @details 定义了链路管理模块中声呐功能的不同状态
 */
typedef enum {
    HADM_SOUNDING_STATE_IDLE = 0,               /**< 空闲状态 */
    HADM_SOUNDING_STATE_SOUNDING_READY,         /**< 声呐准备就绪状态 */
    HADM_SOUNDING_STATE_CONFIG_CONNECTION,      /**< 连接参数更新状态 */
    HADM_SOUNDING_STATE_CONFIG_SOUNDING,        /**< 声呐配置状态 */
    HADM_SOUNDING_STATE_ENABLE_SOUNDING,        /**< 声呐启用状态 */
    HADM_SOUNDING_STATE_SOUNDING,               /**< 声呐运行状态 */
    HADM_SOUNDING_STATE_DISABLE_SOUNDING,       /**< 声呐禁用状态 */
    HADM_SOUNDING_STATE_INVALID                 /**< 无效状态 */
} HadmSoundingState_E;

typedef enum {
    HADM_PEER_SUPPORT_SOUNDING_DEFALUT = 0,
    HADM_PEER_SUPPORT_SOUNDING_NO,
    HADM_PEER_SUPPORT_SOUNDING_YES
} HadmPeerSupportSounding_E;

/* ------------------------------------------------------------------------ */
/*  结构体定义                                                              */
/* ------------------------------------------------------------------------ */

/**
 * @brief 链路控制块结构体类型定义
 * @details 定义了一个链路控制块的结构体类型，用于管理链路相关资源和状态信息
 */
typedef struct HadmSoundCb {
    SLE_Addr_S addr;
    uint16_t lcid;
    HadmSoundingState_E state;

    uint8_t peerSupportSounding;  // 这个表示CM层是否上报了对端设备的features
    HadmConnectionParam_S connectionParam;
    HadmSoundingParam_S soundingParam;
    HadmRemoteCsParam_S remoteCs;
    HadmUserOperate_E nextOp;

    HadmIqInfoFromDli_S *localIqInfo;
    HadmIqInfoFromDli_S *remoteIqInfo;
} HadmSoundCb_S;

/* ------------------------------------------------------------------------ */
/*  函数声明                                                                */
/* ------------------------------------------------------------------------ */

/**
 * @brief 初始化链路控制块管理模块
 * @return uint32_t 状态码，0表示成功，非0表示失败
 * @details 初始化链路控制块管理模块，包括内存分配、资源初始化等操作
 */
uint32_t HadmInitSoundingCbManager(void);

/**
 * @brief 反初始化链路控制块管理模块
 * @return uint32_t 状态码，0表示成功，非0表示失败
 * @details 反初始化链路控制块管理模块，释放相关资源
 */
void HadmDeInitLinkCbManager(void);

/**
 * @brief 根据地址获取链路控制块
 * @param [in] SLE_Addr_S *addr 链路地址信息指针
 * @return HadmLinkCb_S * 链路控制块指针，NULL表示查找失败
 * @details 根据输入的链路地址信息，查找链路控制块
 */
HadmSoundCb_S *HadmGetLinkCb(SLE_Addr_S *addr);;

/**
 * @brief 根据地址分配链路控制块
 * @param [in] SLE_Addr_S *addr 链路地址信息指针
 * @param [in] uint16_t lcid 链路的逻辑通道标识符
 * @return HadmLinkCb_S * 链路控制块指针，NULL表示分配失败
 * @details 根据输入的链路地址信息，分配并初始化一个链路控制块
 */
HadmSoundCb_S *HadmAllocLinkCb(SLE_Addr_S *addr, uint16_t lcid);

/**
 * @brief 根据地址获取lcid
 * @param [in] SLE_Addr_S *addr 链路地址信息指针
 * @return uint16_t 链路的逻辑通道标识符，NLSTK_INVALID_LCID(0xFFFF)表示当前地址无对应链路
 * @details 根据输入的链路地址信息，获取到链路的逻辑通道标识符
 */
uint16_t HadmGetLcid(SLE_Addr_S *addr);

uint32_t HadmGetAddrsByLcid(uint16_t lcid, SLE_Addr_S *addr);
/**
 * @brief 根据地址释放链路控制块
 * @param [in] SLE_Addr_S *addr 链路地址信息指针
 * @details 根据输入的链路地址信息，释放对应的链路控制块资源
 */
void HadmFreeLinkCb(SLE_Addr_S *addr);

/**
 * @brief 设置链路声呐状态
 * @param [in] SLE_Addr_S *addr 链路地址信息指针
 * @param [in] HadmSoundingState_E state 声呐状态
 * @return uint32_t 状态码，0表示成功，非0表示失败
 * @details 根据输入的链路地址信息，设置链路的声呐状态
 */
uint32_t HadmSetSoundingState(SLE_Addr_S *addr, HadmSoundingState_E state);

/**
 * @brief 获取链路声呐状态
 * @param [in] SLE_Addr_S *addr 链路地址信息指针
 * @return HadmSoundingState_E 链路声呐状态
 * @details 根据输入的链路地址信息，获取链路的声呐状态
 */
HadmSoundingState_E HadmGetSoundingStateByAddr(SLE_Addr_S *addr);

/**
 * @brief 设置链路远程参数
 * @param [in] SLE_Addr_S *addr 链路地址信息指针
 * @param [in] void *remoteCs 远程参数指针
 * @return uint32_t 状态码，0表示成功，非0表示失败
 * @details 根据输入的链路地址信息，设置链路的远程参数
 */
uint32_t HadmCacheLinkRemoteCs(SLE_Addr_S *addr, HadmRemoteCsParam_S *remoteCs);

/**
 * @brief 获取链路远程参数
 * @param [in] SLE_Addr_S *addr 链路地址信息指针
 * @param [out]  char* remoteCs 远程参数指针
 * @return uint32_t 状态码，0表示成功，非0表示失败
 * @details 根据输入的链路地址信息，获取链路的远程参数
 */
uint32_t HadmGetLinkRemoteCs(SLE_Addr_S *addr, HadmRemoteCsParam_S *remoteCs);

/**
 * @brief 设置链路连接参数
 * @param [in] SLE_Addr_S *addr 链路地址信息指针
 * @param [in] HadmConnectionParam_S *param 连接参数指针
 * @return uint32_t 状态码，0表示成功，非0表示失败
 * @details 根据输入的链路地址信息，设置链路的连接参数
 */
uint32_t HadmCacheConnectionParam(SLE_Addr_S *addr, HadmConnectionParam_S *param);

/**
 * @brief 获取链路连接参数
 * @param [in] SLE_Addr_S *addr 链路地址信息指针，不能为空
 * @param [out] HadmConnectionParam_S *param 用于存储连接参数的结构体指针，需预先分配内存
 * @return uint32_t 返回连接参数的长度，0表示获取失败
 * @attention
 * - addr参数不能为空，否则可能导致未定义行为
 * - param参数需要预先分配足够的内存空间，否则可能导致内存溢出
 * - 该函数需要链路已经建立连接，否则可能返回错误
 * @details
 * 该函数根据输入的链路地址信息，查询并获取链路的连接参数。
 * 内部实现会根据addr中的地址信息，查找对应的链路配置，
 * 并将参数填充到param指向的结构体中。
 * @return uint32_t 状态码，0表示成功，非0表示失败
 */
uint32_t HadmGetConnectionParam(SLE_Addr_S *addr, HadmConnectionParam_S *param);

/**
 * @brief 缓存声音参数
 * @param [in] SLE_Addr_S *addr 链路地址信息指针，不能为空
 * @param [in] HadmSoundingParam_S *param 测距参数指针
 * @return uint32_t 状态码，0表示成功，非0表示失败
 */
uint32_t HadmCacheSoundingParam(SLE_Addr_S *addr, HadmSoundingParam_S *param);

/**
 * @brief 获取声音参数
 * @param [in] SLE_Addr_S *addr 链路地址信息指针，不能为空
 * @param [out] HadmSoundingParam_S *param 用于存储声音参数的结构体指针，需预先分配内存
 * @return uint32_t 状态码，0表示成功，非0表示失败
 * @attention
 * - addr参数不能为空，否则可能导致未定义行为
 * - param参数需要预先分配足够的内存空间，否则可能导致内存溢出
 * - 该函数需要链路已经建立连接，否则可能返回错误
 * @details
 * 该函数根据输入的链路地址信息，查询并获取声音参数。
 * 内部实现会根据addr中的地址信息，查找对应的声音参数配置，
 * 并将参数填充到param指向的结构体中。
 */
uint32_t HadmGetSoundingParam(SLE_Addr_S *addr, HadmSoundingParam_S *param);

/**
 * @brief 缓存用户操作
 * @param [in] SLE_Addr_S *addr 链路地址信息指针
 * @param [in] HadmUserOperate_E operate 用户操作类型
 * @return uint32_t 状态码，0表示成功，非0表示失败
 * @details 根据输入的链路地址信息，缓存用户的操作类型
 */
uint32_t HadmCacheUserOperate(SLE_Addr_S *addr, HadmUserOperate_E operate);

/**
 * @brief 获取用户操作
 * @param [in] SLE_Addr_S *addr 链路地址信息指针
 * @return HadmUserOperate_E 用户操作类型
 * @details 根据输入的链路地址信息，获取用户的操作类型
 */
HadmUserOperate_E HadmGetUserOperate(SLE_Addr_S *addr);

uint32_t HadmCacheRemoteFeatures(SLE_Addr_S *addr, uint8_t peerSupportSounding);

uint32_t HadmGetRemoteFeatures(SLE_Addr_S *addr, HadmPeerSupportSounding_E *peerSupportSounding);

uint32_t HadmCacheLocalIqInfo(SLE_Addr_S *addr, HadmIqInfoFromDli_S *iqInfo);
uint32_t HadmCacheRemoteIqInfo(SLE_Addr_S *addr, HadmIqInfoFromDli_S *iqInfo);
HadmIqInfoFromDli_S *HadmGetLocalIqInfo(SLE_Addr_S *addr);
HadmIqInfoFromDli_S *HadmGetRemoteIqInfo(SLE_Addr_S *addr);

uint32_t HadmGetSoundingNumAndAddr(SLE_Addr_S *addr, uint32_t soundingAddrNum);

#ifdef __cplusplus
}
#endif
#endif /* HADM_LINK_MANAGER_H */