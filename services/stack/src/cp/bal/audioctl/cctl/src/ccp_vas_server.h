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
#ifndef CCP_VAS_SERVER_H
#define CCP_VAS_SERVER_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  语音助手使能
 */
void CcpVasEnable(void);

/**
 * @brief  语音助手去使能
 */
void CcpVasDisable(void);

/**
 * @brief  添加语音助手服务
 */
void CcpCreateVoiceAssistantService(void *arg);

/**
 * @brief  通知语音助手服务操作结果
 */
void CcpVasControlResult(void *arg);

/**
 * @brief  通知语音助手服务授权结果
 */
void CcpVasStateAuthorizeResult(void *arg);

/**
 * @brief  语音助手服务更新属性
 */
void CcpUpdateVasState(void *arg);

/**
 * @brief  删除语音助手服务
 */
void CcpDeleteVoiceAssistantService(void *arg);

#ifdef __cplusplus
}
#endif

#endif