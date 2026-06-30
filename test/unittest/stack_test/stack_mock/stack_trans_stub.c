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
#include <stdint.h>
#include "stack_trans_stub.h"

static TRANS_Cbks_S g_transportCbks = {NULL};

uint32_t TEST_TRANS_RegisterCbks(const TRANS_Cbks_S *cbks)
{
    g_transportCbks = *cbks;
    return 0;
}

void TEST_TRANS_UnregisterCbks(void)
{
    TRANS_Cbks_S cbks = { NULL };
    g_transportCbks = cbks;
}

uint32_t TEST_TRANS_SendData(TRANS_Addr_S *addr, const uint8_t *data, uint16_t dataLen)
{
    uint16_t succ = 5;
    uint16_t txFull = 6;
    uint16_t failed = 1;
    if (dataLen == succ) {
        return TRANS_SUCCESS;
    } else if(dataLen == txFull) {
        return TRANS_RESULT_TX_CACHE_FULL;
    }
    return 1;
}

void TEST_TRANS_ChannelSetStatus(SLE_Addr_S *addr, uint8_t tcid, uint8_t result) {};

TRANS_RecvDataCbk TEST_TRANS_GetRecvDatacbk(void)
{
    return g_transportCbks.recvDataCbk;
}

TRANS_SendDataCbk TEST_TRANS_GetSendDatacbk(void)
{
    return g_transportCbks.sendDataCbk;
}