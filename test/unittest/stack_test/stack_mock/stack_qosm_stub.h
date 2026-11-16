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
#ifndef STUB_QOSM_H
#define STUB_QOSM_H

#include <stdbool.h>

#include "qosm_autorate_def.h"
#include "qosm_errno.h"

#ifdef __cplusplus
extern "C" {
#endif
uint32_t TEST_QOSM_AutoRateRegisterCallback(const QOSM_AutoRateCallback *callback);

uint32_t TEST_QOSM_AutoRateUnregisterCallback(void);

uint32_t AUTORATE_QOSM_AutoRateSetAutorateParam(const QOSM_AutoRateParam *param);

uint32_t TEST_QOSM_AutoRateAddConnection(const QOSM_AutoRateConnParam *callback);

uint32_t TEST_QOSM_AutoRateAddDataPath(const QOSM_AutoRateDataPath *callback);

uint32_t TEST_QOSM_AutoRateDeleteDataPath(const QOSM_AutoRateDeletedDataPath *callback);

uint32_t TEST_QOSM_AutoRateDeleteConnection(const QOSM_AutoRateConnParam *callback);

uint32_t TEST_QOSM_AutoRateSetEarphoneFeedback(const QOSM_AutoRateEarphoneFeedbackParam *callback);

void TEST_QOSM_NotifyParam(uint8_t icgId, uint8_t state, bool isImg, uint32_t result);

void TEST_QOSM_NotifyParamEx(uint8_t icgId, uint8_t state, bool isImg, uint32_t result, uint8_t linkCnt,
    uint16_t *connHandle, uint16_t gHandle);

void TEST_QOSM_NotifyConnection(uint8_t icgId, uint8_t state, uint32_t result);

void TEST_QOSM_NotifyDataPath(uint8_t icgId, uint8_t state, uint32_t result, uint8_t direction);

void TEST_QOSM_NotifyDataPathByHandle(uint16_t handle, uint8_t state, uint32_t result, uint8_t direction);

void TEST_QOSM_NotifyBitrate(uint8_t icgId);

void TEST_QOSM_NotifyAutoRateMsg(const QOSM_AutoRateSendMsgCb *msg);

void TEST_QOSM_SetRecvAutoRateMsgResult(uint32_t result);

uint32_t TEST_QOSM_GetRecvAutoRateMsgCount(void);

void TEST_QOSM_SetRemoveParamResult(uint32_t result);

#ifdef __cplusplus
}
#endif
#endif