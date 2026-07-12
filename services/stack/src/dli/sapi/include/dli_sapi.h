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

#ifndef DLI_SAPI_H
#define DLI_SAPI_H

#include <stdint.h>
// 适配hardware层，联调不用改源码，后续删除此宏相关的桩函数
#ifdef NEARLINK_SERVICE_STACK_LOCAL_TEST
#include "dli_data_stub.h"
#else
#include "SleDliLayerAdapter.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*DLI_SapiPacketReceived)(SlePacketType type, const SlePacket *packet);

/**
 * @brief  初始化sapi模块,阻塞式的接口 最多3s返回结果
 * @param  [in] < cb > 注册到驱动的接收回调函数
 * @return 0: 成功, OTHER: 失败
 */
uint32_t DLI_SapiInit(DLI_SapiPacketReceived cb);

/**
 * @brief  反初始化sapi模块
 * @return 无
 */
void DLI_SapiDeinit(void);

/**
 * @brief  数据发送
 * @param  [in] < data > 要发送的数据流, 包含命令及数据
 * @param  [in] < len >  要发送的数据流长度
 * @return 无
 */
int DLI_SapiSend(const uint8_t *data, uint32_t len);

/**
 * @brief  获取芯片版本
 * @return 芯片版本
 */
int DLI_GetDliVersion(void);

#ifdef __cplusplus
}
#endif
#endif