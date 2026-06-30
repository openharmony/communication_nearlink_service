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
#ifndef MCP_MEDIA_SERVER_H
#define MCP_MEDIA_SERVER_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  添加媒体控制服务
 */
void McpCreateMediaInstance(void *arg);

/**
 * @brief  通知媒体控制服务操作结果
 */
void McpPlayControlResult(void *arg);

/**
 * @brief  通知媒体控制服务授权结果
 */
void McpMediaAuthorizeResult(void *arg);

/**
 * @brief  媒体控制服务更新属性
 */
void McpUpdateMediaProperty(void *arg);

/**
 * @brief  删除媒体控制服务
 */
void McpDeleteMediaInstance(void *arg);

#ifdef __cplusplus
}
#endif

#endif