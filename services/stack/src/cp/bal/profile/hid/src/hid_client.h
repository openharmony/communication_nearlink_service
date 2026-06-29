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
#ifndef HID_CLIENT_H
#define HID_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 注册人机交互服务profile客户端回调
 */
void HidRegClientCbkInner(void *arg);

/**
 * @brief 对人机交互设备进行HID profile连接
 */
void HidConnectInner(void *arg);

/**
 * @brief 对人机交互设备进行HID profile断连
 */
void HidDisconnectInner(void *arg);

/**
 * @brief 获取连接获得的对端HID属性信息
 */
void HidGetInformationInner(void *arg);

/**
 * @brief 读取人机交互设备的指定属性值
 */
void HidReadPropertyInner(void *arg);

/**
 * @brief 向人机交互设备写入指定属性值
 */
void HidWritePropertyInner(void *arg);

/**
 * @brief 获取人机交互设备连接状态
 */
void HidGetConnectStateInner(void *arg);

/**
 * @brief 获取已连接人机交互设备数量
 */
void HidGetConnectedDeviceNumInner(void *arg);


void HidGetConnectedDeviceInner(void *arg);

#ifdef __cplusplus
}
#endif

#endif