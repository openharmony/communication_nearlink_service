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
 * this file contains the CM Inner API definitions
 *
 ***************************************************************************/

#ifndef CM_INNER_API_H
#define CM_INNER_API_H

#include <stdint.h>
#include "cm_icb_mgr.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 星闪逻辑链路创建和释放响应扩展参数结构体
 */
typedef struct {
    void *extension; /* 预留结构体指针 */
} CM_ConnectParamRspExt_S;

/**
 * @brief 星闪逻辑链路创建响应结构体
 */
typedef struct {
    uint8_t   role;         /* 链路角色：G或者T, 参考CM_NodeType_E定义*/
    uint8_t   version;      /* 原语版本号，当前标准版本设置为0 */
    uint16_t  localIndex;   /* 本地索引 */
    uint8_t   result;       /* 参考链路连接状态CM_ConnectLinkState_E定义 */
    uint8_t   discReason;   /* 链路断链原因 */
    uint16_t  lcid;         /* 星闪逻辑链路handle */
    SLE_Addr_S addr;        /* 星闪设备地址 */
    uint8_t connCompleteType; /* 连接完成类型，0：通过本端扫描建立的连接，1：通过本端广播建立的连接 */
    uint8_t advHandle;        /* 广播标识，若是通过本端广播建立的连接，则为对应启动广播标识，否则默认为0，无效值 */
    CM_ConnectParamRspExt_S *extension; /* 连接响应扩展参数，协议目前未定位具体结构, 可选 */
} CM_ConnectParamRsp_S, CM_DisconnectParamRsp_S;

/**
 * @brief 星闪逻辑链路创建请求结构体
 */
typedef struct {
    uint8_t   version;     /* 原语版本号，当前标准版本设置为0 */
    uint16_t  localIndex;  /* 本地索引 */
    SLE_Addr_S addr;       /* 星闪设备地址 */
} CM_ConnectParamReq_S;

typedef struct {
    SLE_Addr_S addr;       /* 星闪设备地址 */
    uint8_t discReason;    /* 断连原因 */
} CM_DisconnectParamReq_S;

/**
 * @brief 星闪逻辑链路建立默认参数
 */
typedef struct {
    uint8_t  bitFrameType;
    uint8_t  enableFilterPolicy;  /* 链路是否打开过滤功能 */
    uint8_t  initiatingPhys;      /* 链路扫描通信带宽： 1:1M, 2:2M */
    uint8_t  gtNegotiate;         /* 链路建立时是否进行G和T交互 */
    uint16_t scanInterval;        /* 链路建立时扫描对端设备的interval */
    uint16_t scanWindow;          /* 链路建立时扫描对端设备的windows */
    uint16_t minInterval;         /* 链路调度最小interval */
    uint16_t maxInterval;         /* 链路调度最大interval */
    uint16_t timeout;             /* 链路超时时间 */
} CM_ConnectSetParamReq_S;

/**
 * @brief  连接管理模块回调函数类型
 */
typedef void (*CM_ConnectStateCbk)(CM_ConnectParamRsp_S *param);

/**
 * @brief  连接管理模块取消连接建立回调函数类型
 * param 0：成功，其他：失败
 */
typedef void (*CM_ConnectCancelCbk)(uint8_t *param);

/**
 * @brief  CM模块取消注册连接相关回调
 * @param [in] < void > 无
 * @return void
 */
void CM_UnRegConnectCbks(void);

/**
 * @brief 创建连接链路请求
 * @param  [in] < param > 连接参数, 参见CM_ConnectParamReq_S定义
 * @return SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_ConnectEstablishReq(CM_ConnectParamReq_S *param);

/**
 * @brief  释放连接链路请求
 * @param  [in] < param > 连接参数, 参见CM_DisconnectParamReq_S定义
 * @return SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_ConnectReleaseReq(CM_DisconnectParamReq_S *param);

/**
 * @brief  取消已发起连接建立，但未建链完成的请求
 * @return SUCCESS: 成功, OTHER: 失败
*/
uint32_t CM_ConnectCancelReq(void);

/**
 * @brief  连接链路配置参数请求
 * @param  [in] < param > 配置参数, 参见CM_ConnectSetParamReq_S定义
 * @return SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_ConnectSetParamReq(CM_ConnectSetParamReq_S *param);

/**
 * @brief  清除访问过滤列表
 * @param  [in] 无
 * @return 无
 */
void CM_ClearAcceptFilterList(void);

/**
 * @brief  将设备添加到访问过滤列表
 * @param  [in] < addr > 对端设备地址
 * @return SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_AddDeviceToAcceptFilterList(SLE_Addr_S *addr, bool isBypass);

/**
 * @brief  将设备从访问过滤列表移除
 * @param  [in] < addr > 对端设备地址
 * @return SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_RemoveDeviceFromAcceptFilterList(SLE_Addr_S *addr);

/**
 * @brief  设置异步链路的subrate，内部接口，运行在CP线程
 * @param  [in] param : acb subrate参数
 * @return SUCCESS: 成功, OTHER: 失败
 */
uint32_t CM_InnerSetACBSubrate(const CM_SetACBSubrateInnerParam *param);

/**
 * @brief  获取对端设备的私有特性，内部接口，运行在CP线程
 * @param  [in] lcid : 异步链路唯一标识
 * @param  [in] featureBit : 特性位图
 * @return true：特性支持，false : 特性不支持
 */
bool CM_InnerGetRemotePrivateFeature(uint16_t lcid, uint16_t featureBit);

#ifdef __cplusplus
}
#endif

#endif