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
#ifndef CCP_CCS_SERVER_H
#define CCP_CCS_SERVER_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  通话控制使能
 */
void CcpCcsEnable(void);

/**
 * @brief  通话控制去使能
 */
void CcpCcsDisable(void);

/**
 * @brief  添加通话控制服务
 */
void CcpCreateCcsInstance(void *arg);

/**
 * @brief  通知通话控制服务操作结果
 */
void CcpCallControlResult(void *arg);

/**
 * @brief  通知通话控制服务授权结果
 */
void CcpCcsAuthorizeResult(void *arg);

/**
 * @brief  通话控制服务更新属性
 */
void CcpUpdateCcsProperty(void *arg);

/**
 * @brief  删除通话控制服务
 */
void CcpDeleteCcsInstance(void *arg);

#ifdef __cplusplus
}
#endif

#endif