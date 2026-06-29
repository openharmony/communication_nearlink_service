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
#ifndef NLSTK_MCP_VOLUME_CLINET_H
#define NLSTK_MCP_VOLUME_CLINET_H

#include "sdf_addr.h"
#include "nlstk_public_define.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#define NLSTK_MCP_MUTE_ON 0x01 // 静音状态
#define NLSTK_MCP_MUTE_OFF 0x00 // 正常状态

/**
 * @brief 音量输出端连接状态
 */
typedef enum {
    NLSTK_MCP_VOLUME_CONNECTING,
    NLSTK_MCP_VOLUME_CONNECTED,
    NLSTK_MCP_VOLUME_DISCONNECTING,
    NLSTK_MCP_VOLUME_DISCONNECTED,
} NLSTK_McpVolumeState_E;

/**
 * @brief 音量属性类型枚举
 */
typedef enum {
    NLSTK_MCP_VOLUME_STATUS = 0x00,          ///< 音量状态
    NLSTK_MCP_CHANNEL_VOLUME_STATUS = 0x01,  ///< 通道音量状态
    NLSTK_MCP_STREAM_VOLUME_STATUS = 0x02,   ///< 音频流音量状态
    NLSTK_MCP_MAX_PROPERTY,                  ///< 最大属性数
} NLSTK_McpVolumePropertyType_E;

/**
 * @brief 音量状态附加信息
 */
typedef enum {
    NLSTK_MCP_INIT_VOLUME,
    NLSTK_MCP_SERVER_CHANGE_VOLUME,
    NLSTK_MCP_CLIENT_CHANGE_VOLUME,
} NLSTK_McpVolumeAdditionalInfo_E;

/**
 * @brief 音频流音量类型
 */
typedef enum {
    NLSTK_MCP_MEDIA_STREAM,
    NLSTK_MCP_CALL_STREAM,
} NLSTK_McpSetStreamVolume_E;

/**
 * @brief 音量状态属性
 */
typedef struct {
    uint8_t volume;         ///< 音量
    uint8_t mute;           ///< 静音
    uint8_t additionalInfo; ///< 附加信息
} NLSTK_McpVolumeStatus_S;

/**
 * @brief 音频流音量状态属性
 */
typedef struct {
    uint8_t mediaVolume;        ///< 媒体音量
    uint8_t mediaInfo;          ///< 媒体附加信息
    uint8_t callVolume;         ///< 通话音量
    uint8_t callInfo;           ///< 通话附加信息
} NLSTK_McpStreamVolumeStatus_S;

/**
 * @brief 设置音频流音量参数
 */
typedef struct {
    uint8_t volume;                         ///< 音量
    NLSTK_McpSetStreamVolume_E streamType;    ///< 流类型
} NLSTK_McpSetStreamVolume_S;

// 音量控制服务端发送过来的音量变化事件
typedef void (*NLSTK_McpVolumeChangeEvent)(SLE_Addr_S *addr, uint8_t volume);

// 音量控制服务端发送过来的音量状态事件
typedef void (*NLSTK_McpMuteStatusChangeEvent)(SLE_Addr_S *addr, uint8_t muteStatus);

// 客户端收到服务端的音量状态属性变更时，会通过此回调上报属性变化
typedef void (*NLSTK_McpNotifyVolumeChange)(SLE_Addr_S *addr, NLSTK_McpVolumePropertyType_E property, void *value);

typedef void (*NLSTK_McpGetVolumeRsp)(SLE_Addr_S *addr, NLSTK_McpVolumePropertyType_E property,
    uint8_t errorCode, void *value);

// 客户端设置音量通用响应回调，当前音量设置和音频流音量设置都使用此回调
typedef void (*NLSTK_McpSetVolumeRsp)(SLE_Addr_S *addr, uint8_t errorCode);

typedef void (*NLSTK_McpVolumeConnectStateChange)(SLE_Addr_S *addr, uint8_t state, uint8_t preState);

typedef struct {
    NLSTK_McpVolumeChangeEvent volumeChangeEvent;                 // 音量变化同步事件
    NLSTK_McpMuteStatusChangeEvent muteStatusChangeEvent;         // 静音状态变化同步事件
    NLSTK_McpNotifyVolumeChange notifyVolumeChange;               // 音量变化触发的通知
    NLSTK_McpGetVolumeRsp getVolumeRsp;                           // 本端发送了读取音量请求后，服务端返回的结果
    NLSTK_McpSetVolumeRsp setVolumeRsp;                           // 本端发送了设置音量请求后，服务端返回的结果
    NLSTK_McpVolumeConnectStateChange stateChange;                // 连接状态变化回调
} NLSTK_McpVolumeClientCallBack_S;

/**
 * @brief 注册音量控制端回调
 *
 * 该函数用于注册音量控制端回调，包含连接状态回调、异步调用回调和上报客户端事件回调。
 *
 * @param[in] clientCallback 回调函数结构体
 *
 * @return uint32_t 返回调用结果
 *
 */
uint32_t NLSTK_McpRegVolumeClientCbk(NLSTK_McpVolumeClientCallBack_S *clientCallback);

/**
 * @brief 特性连接音量输出端
 *
 * 该函数用于特性连接音量输出端，需要对音量输出端依次执行建链、获取服务结构、读取和订阅音量状态；
 * 当连接状态发生变化时，将通过 NLSTK_McpVolumeConnectStateChange 回调给用户。
 *
 * @param[in] addr 对端地址
 *
 * @return uint32_t 返回调用结果
 *
 */
uint32_t NLSTK_McpVolumeConnect(SLE_Addr_S *addr);

/**
 * @brief 特性断连音量输出端
 *
 * 该函数用于特性断连音量输出端，将对音量输出端执行逻辑上的断链（具体断链逻辑由SSAP控制）并销毁相应资源；
 * 当连接状态发生变化时，将通过 NLSTK_McpVolumeConnectStateChange 回调给用户。
 *
 * @param[in] addr 对端地址
 *
 * @return uint32_t 返回调用结果
 *
 */
uint32_t NLSTK_McpVolumeDisconnect(SLE_Addr_S *addr);

/**
 * @brief 获取当前音量状态
 *
 * 该函数用于获取音量输出端的当前音量状态。
 *
 * @param[in] addr 对端地址
 * @param[in] property 音量属性类型
 *
 * @return uint32_t 返回调用结果
 *
 */
uint32_t NLSTK_McpGetVolume(SLE_Addr_S *addr, NLSTK_McpVolumePropertyType_E property);

/**
 * @brief 发送设置主音量控制请求
 *
 * 该函数用于向音量输出端发送设置主音量控制请求，音量输出端的响应结果会通过 NLSTK_McpSetVolumeRsp 回调函数上报。
 *
 * @param[in] addr 对端地址
 * @param[in] volume 音量值
 *
 * @return uint32_t 返回调用结果
 *
 */
uint32_t NLSTK_McpSetVolume(SLE_Addr_S *addr, uint8_t volume);

/**
 * @brief 发送设置音频流音量控制请求
 *
 * 该函数用于向音量输出端发送设置音频流音量控制请求，音量输出端的响应结果会通过 NLSTK_McpSetVolumeRsp 回调函数上报。
 *
 * @param[in] addr 对端地址
 * @param[in] volumeArray 设置音频流音量数组指针
 * @param[in] num 设置音频流音量数组大小
 *
 * @return uint32_t 返回调用结果
 *
 */
uint32_t NLSTK_McpSetStreamVolume(SLE_Addr_S *addr, NLSTK_McpSetStreamVolume_S *volumeArray, uint8_t num);

/**
 * @brief 获取对端音频流音量控制能力
 *
 * 该函数用于获取对端音频流音量控制能力，同步接口，通过返回值返回是否支持音频流音量控制。
 *
 * @param[in] addr 对端地址
 *
 * @return bool 是否支持音频流音量控制
 *
 */
bool NLSTK_McpGetStreamVolumeAbility(SLE_Addr_S *addr);

#ifdef __cplusplus
}
#endif

#endif