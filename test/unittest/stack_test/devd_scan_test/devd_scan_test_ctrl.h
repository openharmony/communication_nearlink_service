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

#ifndef DEVD_SCAN_TEST_CTRL_H
#define DEVD_SCAN_TEST_CTRL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TEST_DLI_STUB_MAX_PAYLOAD 64

/* 扫描桩开关：非零时对应命令入口直接返回失败 */
extern uint8_t g_testDevdStartScanFail;
extern uint8_t g_testDevdStopScanFail;
/* 静默开关：命令成功但不下发完成事件，用于把状态机停在过渡态 */
extern uint8_t g_testDevdStartScanSilent;
extern uint8_t g_testDevdStopScanSilent;
/* 以芯片事件身份注入一次扫描事件回调 */
void TEST_DevdInjectScanEvent(uint8_t event, uint8_t result);

void TEST_DevdDliStubReset(void);
/* 回放开关：非零时命令经任务队列异步回放产品回调，status 为回放状态 */
extern uint8_t g_dliStubSetAdvParamReply;
extern uint16_t g_dliStubSetAdvParamStatus;
extern uint8_t g_dliStubSetAdvDataReply;
extern uint16_t g_dliStubSetAdvDataStatus;
extern uint8_t g_dliStubSetScanRspReply;
extern uint16_t g_dliStubSetScanRspStatus;
extern uint8_t g_dliStubEnableAdvReply;
extern uint16_t g_dliStubEnableAdvStatus;
extern uint8_t g_dliStubRemoveAdvReply;
extern uint16_t g_dliStubRemoveAdvStatus;
extern uint8_t g_dliStubSetScanParamReply;
extern uint16_t g_dliStubSetScanParamStatus;
extern uint8_t g_dliStubEnableScanReply;
extern uint16_t g_dliStubEnableScanStatus;
/* 命令入口失败开关：非零时命令不捕获不回放，直接返回失败 */
extern uint8_t g_dliStubEnableAdvFail;
/* 捕获：最近一次各命令的参数与调用计数 */
extern uint8_t g_dliStubAdvParamSecondFormat;
extern uint16_t g_dliStubAdvParamTimeout;
extern uint16_t g_dliStubAdvParamConnMin;
extern uint16_t g_dliStubAdvParamConnMax;
extern uint8_t g_dliStubAdvDataHandle;
extern uint8_t g_dliStubAdvDataOp;
extern uint16_t g_dliStubAdvDataLen;
extern uint8_t g_dliStubAdvDataPayload[TEST_DLI_STUB_MAX_PAYLOAD];
extern uint32_t g_dliStubSetAdvDataCnt;
extern uint8_t g_dliStubScanRspHandle;
extern uint8_t g_dliStubScanRspOp;
extern uint8_t g_dliStubEnableAdvHandle;
extern uint8_t g_dliStubEnableAdvEnable;
extern uint32_t g_dliStubEnableAdvCnt;
extern uint8_t g_dliStubRemoveAdvHandle;
extern uint8_t g_dliStubScanParamFrameType;
extern uint16_t g_dliStubScanParamWindow[2];
extern uint16_t g_dliStubScanParamInterval[2];
extern uint8_t g_dliStubEnableScanEnable;
extern uint32_t g_dliStubSetScanParamCnt;
extern uint32_t g_dliStubEnableScanCnt;

#ifdef __cplusplus
}
#endif

#endif /* DEVD_SCAN_TEST_CTRL_H */
