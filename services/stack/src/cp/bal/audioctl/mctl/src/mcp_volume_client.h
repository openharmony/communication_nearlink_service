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
#ifndef MCP_VOLUME_CLIENT_H
#define MCP_VOLUME_CLIENT_H

#include "nlstk_ssap_app_client.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  音量控制获取SSAP回调
 */
void McpVolumeGetSsapCbk(NLSTK_SsapAppClientCb_S *cb);

/**
 * @brief  音量控制客户端注册回调
 */
void McpRegVolumeClientCbk(void *arg);

/**
 * @brief  音量控制客户端连接profile
 */
void McpVolumeConnect(void *arg);

/**
 * @brief  音量控制客户端断连profile
 */
void McpVolumeDisconnect(void *arg);

/**
 * @brief  音量控制客户端读取属性请求
 */
void McpGetVolume(void *arg);

/**
 * @brief  音量控制发送主音量控制请求
 */
void McpSetVolume(void *arg);

/**
 * @brief  音量控制发送音频流音量控制请求
 */
void McpSetStreamVolume(void *arg);

/**
 * @brief  获取对端音频流音量控制能力
 */
void McpGetStreamVolumeAbility(void *arg);

#ifdef __cplusplus
}
#endif

#endif