/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include "dli_def.h"
#include "dli_errno.h"
#include "dli_layer_stru.h"
#include "sdf_buff.h"
#include "sdf_mem.h"
#include "securec.h"
#include "stack_dli_layer_stub.h"

#define DLI_DATA_MAX_CNT 10000
#define CMD_EVC_TIMEOUT 10000 /* 10000 ms */
#define DLI_CONNECTION_HDL_BIT_LEN 12
#define DLI_PB_BIT_LEN 2
#define DLI_PAYLOAD_BITS_LEN 2
#define DLI_RESET_EVENT_SIZE 5
#define DLI_DATA_LEN_OFFSET 2
#define DLI_TRY_SEND_CNT 3
#define DLI_TRY_SEND_SLEEP_TIME 10 /* 10 ms */
#define DLI_MAX_PRINT_LEN 50 // 50 is max print len
#define DLI_PRINT_BYTE 2

#define DLI_STATUS_EVENT                        0x0001
#define DLI_COMPLETE_EVENT                      0x0002
#define DLI_OPCODE_BYTE_LENGTH                  2

void TEST_DLI_LyaerInit(void)
{
    return;
}

void TEST_DLI_LyaerDeInit(void)
{
    return;
}

uint32_t TEST_DLI_GetDataFragmentNumsStub(SDF_Buff_S *buf)
{
    if (buf == NULL) {
        return 0;
    }
    uint16_t bufferLen = 600;
    return (SDF_DataLenGet(buf) + bufferLen - 1) / bufferLen;
}

uint32_t TEST_DLI_GetFragmentMaxLenStub(void)
{
    return 600 + DLI_HEADER;
}

 static uint8_t g_pdFlag = UINT8_MAX;

static uint8_t DLI_PdFlagGet(uint64_t remainLen, uint16_t oneSendLen)
{
    uint8_t pdFlag = DLI_FIRST_FRAGMENT;
    switch (g_pdFlag) {
        case DLI_FIRST_FRAGMENT:
        case DLI_MIDDLE_FRAGMENT:
            pdFlag = (remainLen > oneSendLen) ? DLI_MIDDLE_FRAGMENT : DLI_LAST_FRAGMENT;
            break;
        case DLI_LAST_FRAGMENT:
        case DLI_FULL_FRAGMENT:
        case UINT8_MAX:
            pdFlag = (remainLen > oneSendLen) ? DLI_FIRST_FRAGMENT : DLI_FULL_FRAGMENT;
            break;
        default:
            break;
    }
    g_pdFlag = pdFlag;
    return pdFlag;
}

uint32_t TEST_DLI_SplitDataStub(DLI_DataStru *data, SDF_Buff_S *fragmentBuf[], uint32_t fragmentCnt)
{
    uint16_t bufferLen = 600;
    uint64_t dataOffset = 0;
    for (uint32_t i = 0; i < fragmentCnt; i++) {
        // 当前节点的全部数据
        uint8_t *info = SDF_DataOffset(data->buf);
        // 当前发送的数据,包含dli的5字节头,且已经保证预留了5字节的dli HEADER
        uint8_t *p = info - DLI_HEADER + dataOffset;
        uint64_t remainLen = SDF_DataLenGet(data->buf) - dataOffset;
        /* 5 bytes header | type (8 bit) | handle (12 bit) | pbFlag (2 bit) |
                        | ts (1 bit) | prio (1 bit) | length (16 bit) | */
        *p = data->type;
        DLI_ENCODE2BYTE(p + 1, data->lcid |
            (DLI_PdFlagGet(remainLen, bufferLen) << DLI_CONNECTION_HDL_BIT_LEN) |
            (data->ts << (DLI_CONNECTION_HDL_BIT_LEN + DLI_PB_BIT_LEN)) |
            (data->prio << (DLI_CONNECTION_HDL_BIT_LEN + DLI_PB_BIT_LEN + 1)));

        uint16_t sendLen = remainLen > bufferLen ? bufferLen : remainLen;
        DLI_ENCODE2BYTE(p + 1 + DLI_PAYLOAD_BITS_LEN, sendLen);

        uint16_t fragmentLen = sendLen + DLI_HEADER;
        uint8_t *fragmentData = SDF_BuffAppend(fragmentBuf[i], fragmentLen);
        if (fragmentData == NULL || memcpy_s(fragmentData, fragmentLen, p, fragmentLen) != EOK) {
            return DLI_STACK_MEM_ERRNO;
        }
        dataOffset += sendLen;
    }
    return DLI_SUCCESS;
}

uint32_t TEST_DLI_DataSendStub(DLI_DataStru *data)
{
    if (data == NULL) {
        return 1;
    }
    uint32_t ret = data->lcid == 0xffff ? 1 : 0;
    if (ret == 0) {
        SDF_MemFree(data);
    }
    return ret;
}

void TEST_DLI_PostNextTaskStub(DLI_TaskType type)
{
    return;
}

uint32_t TEST_DLI_CmdSendStub(DLI_CmdStru *cmd)
{
    return 0;
}