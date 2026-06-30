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
#ifndef DIS_CLIENT_H
#define DIS_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  注册回调函数
 */
void DisRegClientCbk(void *arg);

/**
 * @brief  DIS建立连接
 */
void DisConnectTask(void *arg);

/**
 * @brief  DIS断开连接
 */
void DisDisconnectTask(void *arg);

/**
 * @brief  DIS计数连接设备
 */
void DisCountConnectedDevicesTask(void *arg);

#ifdef __cplusplus
}
#endif
#endif
