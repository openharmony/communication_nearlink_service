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
#include <assert.h>
#include <stdio.h>
#include "securec.h"

#include "dtap_tcid.h"
#include "stack_dtap_stub.h"

#include "cpfwk_log.h"
#include "sdf_string.h"

#define DTAP_MAX_TCID_NUM 32

#define DTAP_CTRL_BLOCK_USED 1
#define DTAP_CTRL_BLOCK_UNUSED 0
#define TEST_MAX_BUF_CACHE 1024

typedef struct {
    uint8_t tcid;
    uint8_t usedFlag;
    uint8_t reserved[2];
    DTAP_DataRecvCb cb;
} DTAP_DataRecvCtrlBlock_S;

DTAP_DataRecvCtrlBlock_S g_dtapDataRecvCtrlBlock[DTAP_MAX_TCID_NUM];

uint32_t TEST_DTAP_RegisterDataRecvCb(uint8_t tcid, DTAP_DataRecvCb cb)
{
    for (int i = 0; i < DTAP_MAX_TCID_NUM; i++) {
        if (g_dtapDataRecvCtrlBlock[i].usedFlag == DTAP_CTRL_BLOCK_UNUSED) {
            g_dtapDataRecvCtrlBlock[i].usedFlag = DTAP_CTRL_BLOCK_USED;
            g_dtapDataRecvCtrlBlock[i].tcid = tcid;
            g_dtapDataRecvCtrlBlock[i].cb = cb;
            return 0;
        }
    }
    return 1;
}

uint32_t TEST_DTAP_UnregisterDataRecvCb(uint8_t tcid)
{
    for (int i = 0; i < DTAP_MAX_TCID_NUM; i++) {
        g_dtapDataRecvCtrlBlock[i].usedFlag = DTAP_CTRL_BLOCK_UNUSED;
        g_dtapDataRecvCtrlBlock[i].tcid = 0;
        g_dtapDataRecvCtrlBlock[i].cb = NULL;
    }
    return 0;
}

void TEST_DTAP_RecData(uint8_t tcid, DTAP_Data_Info_S *info, SDF_Buff_S *buff)
{
    for (int i = 0; i < DTAP_MAX_TCID_NUM; i++) {
        if (g_dtapDataRecvCtrlBlock[i].usedFlag == DTAP_CTRL_BLOCK_USED && g_dtapDataRecvCtrlBlock[i].tcid == tcid) {
            g_dtapDataRecvCtrlBlock[i].cb(info, buff);
        }
    }
}

void TEST_DTAP_RecDataWithPkt(uint8_t lcid, uint8_t tcid, uint8_t *buf, uint32_t size)
{
    DTAP_Data_Info_S dtapInfo = {.lcid = lcid};
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(size);
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, size);
    (void)memcpy_s(tmpBuf, size, buf, size);
    CP_LOG_INFO("[TEST] test dtap data recv: %s.", SDF_GET_UINT8_STR(buf, size));
    TEST_DTAP_RecData(tcid, &dtapInfo, tmp);
    SDF_BuffFree(tmp);
}

static uint8_t g_buffCache[TEST_MAX_BUF_CACHE] = {0};
static uint32_t g_buffLen = 0;

uint32_t TEST_DTAP_DataSend(DTAP_Data_S *data)
{
    g_buffLen = SDF_DataLenGet(data->buff);
    (void)memcpy_s(g_buffCache, TEST_MAX_BUF_CACHE, SDF_DataOffset(data->buff), SDF_DataLenGet(data->buff));
    CP_LOG_INFO("[TEST] test dtap data send: %s.", SDF_GET_UINT8_STR(g_buffCache, g_buffLen));
    SDF_BuffFree(data->buff);
    return 0;
}

bool TEST_DTAP_CompareLastPkt(uint8_t *buf, uint32_t size)
{
    if (g_buffLen == size && memcmp(g_buffCache, buf, g_buffLen) == 0) {
        CP_LOG_INFO("[TEST] TEST_DTAP_CompareLastPkt true, last pkt: %s",
            SDF_GET_UINT8_STR(g_buffCache, g_buffLen));
        return true;
    }
    CP_LOG_ERROR("[TEST] TEST_DTAP_CompareLastPkt false, last pkt: %s, target pkt: %s",
        SDF_GET_UINT8_STR(g_buffCache, g_buffLen), SDF_GET_UINT8_STR(buf, size));
    return false;
}

static uint8_t g_initFindRspBuf[] = {0x01, 0x00, 0x04, 0x00, 0x00, 0x0B};

void TEST_DTAP_SSAP_RevcInitPkt(uint16_t lcid)
{
    DTAP_Data_Info_S dtapInfo = {.lcid = lcid};
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(g_initFindRspBuf));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(g_initFindRspBuf));
    (void)memcpy_s(tmpBuf, sizeof(g_initFindRspBuf), g_initFindRspBuf, sizeof(g_initFindRspBuf));
    TEST_DTAP_RecData(TCID_SLE_SMTC, &dtapInfo, tmp);
    SDF_BuffFree(tmp);
}
