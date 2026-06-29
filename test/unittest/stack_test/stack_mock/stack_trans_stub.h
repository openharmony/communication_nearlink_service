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

#ifndef STACK_TRANS_STUB_H
#define STACK_TRANS_STUB_H

#include <stdint.h>

#include "securec.h"

#include "nai_log.h"
#include "dtap_errno.h"
#include "transport.h"
#include "transport_errno.h"
#include "transport_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

uint32_t TEST_TRANS_RegisterCbks(const TRANS_Cbks_S *cbks);

void TEST_TRANS_UnregisterCbks(void);

uint32_t TEST_TRANS_SendData(TRANS_Addr_S *addr, const uint8_t *data, uint16_t dataLen);

void TEST_TRANS_ChannelSetStatus(SLE_Addr_S *addr, uint8_t tcid, uint8_t result);

TRANS_RecvDataCbk TEST_TRANS_GetRecvDatacbk(void);

TRANS_SendDataCbk TEST_TRANS_GetSendDatacbk(void);

#ifdef __cplusplus
}
#endif

#endif

