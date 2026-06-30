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
#include "gtest/gtest.h"
#include "securec.h"
#include "ssapc_client.h"
#include "ssap_link.h"
#include "ssap_manager.h"
#include "ssap_common.h"
#include "sdf_addr.h"
#include "sdf_mem.h"
#include "ssap_utils.h"

#define TEST_MAX_BUF_CACHE 1024

static SLE_Addr_S g_addr = {.type = PUBLIC_ADDRESS, .addr = {0x03, 0x03, 0x04, 0x04, 0x05, 0x05}};
static uint16_t g_lcid = 1;

static uint8_t g_buffCache[TEST_MAX_BUF_CACHE] = {0};
static uint8_t g_buffLen = 0;

static uint8_t reqPkt1[] = {0x0d, 0x23, 0xFF, 0x00, 0x00, 0x06};

static uint8_t rspPkt1[] = {0x0e, 0x03, 0xFF, 0x00, 0x01, 0x01};
static uint8_t rspPkt2[] = {0x0e, 0x0b, 0x01, 0xFF, 0x00, 0x07};

static uint8_t reqPkt3[] = {0x0d, 0x23, 0xFF, 0x00, 0x00};
// SSAP_ERRCODE_INVALID_PDU
static uint8_t rspPkt3Error[] = {0x01, 0x00, 0x0d, 0x00, 0x00, 0x01};

static uint8_t ackPkt1[] = {0x11, 0x03, 0x00};

class UT_SSAP_CLIENT_WRITE : public testing::Test {
protected:
    void SetUp() override
    {
        SSAP_LinkInit();
    }

    void TearDown() override
    {
        SSAP_LinkDeInit();
    }
};

static void MockSendCb(SSAP_Link *link, SDF_Buff_S *buff, uint8_t opcode)
{
    g_buffLen = SDF_DataLenGet(buff);
    (void)memcpy_s(g_buffCache, TEST_MAX_BUF_CACHE, SDF_DataOffset(buff), SDF_DataLenGet(buff));
    uint8_t type = SSAP_GetOpcodeType(opcode);
    if ((type & SSAP_REPLY_MASK) == 0) {
        SDF_BuffFree(buff);
    } else {
        SSAP_LinkSetTask(link, buff, opcode);
    }
}

static SSAP_Link* CreateLink()
{
    return SSAP_CreateSsapLink(&g_addr, g_lcid, MockSendCb);
}

static void DeleteLink()
{
    SSAP_DeleteSsapLinkByAddr(&g_addr);
}

TEST_F(UT_SSAP_CLIENT_WRITE, WRITE_REQ)
{
    SSAP_Link_S *link = CreateLink();

    SSAP_WriteReqInfo_S *writeReqInfo = (SSAP_WriteReqInfo_S *)SDF_MemZalloc(sizeof(SSAP_WriteReqInfo_S) + 1);
    writeReqInfo->handle = 0xFF;
    writeReqInfo->type = 0;
    writeReqInfo->value.len = 1;
    uint8_t value[1] = {6};
    (void)memcpy_s(writeReqInfo->value.value, 1, value, 1);
    (void)memcpy_s(&writeReqInfo->addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    SSAP_TaskParam_S taskParam = {.appId = 1, .arg = writeReqInfo, .freeFunc = SDF_MemFree, .func = SSAPC_WriteReq,
        .timeout = 3000, .valid = true, .appCallback = NULL};
    SSAP_ProcessRequestTask(link, &taskParam, false);
    EXPECT_EQ(g_buffLen, sizeof(reqPkt1));
    EXPECT_EQ(memcmp(g_buffCache, reqPkt1, g_buffLen), 0);

    DeleteLink();
}


TEST_F(UT_SSAP_CLIENT_WRITE, WRITE_RSP_SUCCESS)
{
    SSAP_Link_S *link = CreateLink();
    SDF_Buff_S *reqBuf = SDF_BuffNewWithReserve(sizeof(reqPkt1));
    uint8_t *tmpReqBuf = SDF_BuffAppend(reqBuf, sizeof(reqPkt1));
    (void)memcpy_s(tmpReqBuf, sizeof(reqPkt1), reqPkt1, sizeof(reqPkt1));
    link->curTask.buff = reqBuf;
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(rspPkt1));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(rspPkt1));
    (void)memcpy_s(tmpBuf, sizeof(rspPkt1), rspPkt1, sizeof(rspPkt1));
    SSAPC_WriteRspHandle(link, tmp);
    SDF_BuffFree(tmp);

    EXPECT_EQ(link->status, SSAP_LINK_IDLE);
    DeleteLink();
}

TEST_F(UT_SSAP_CLIENT_WRITE, WRITE_RSP_FAILED)
{
    SSAP_Link_S *link = CreateLink();
    SDF_Buff_S *reqBuf = SDF_BuffNewWithReserve(sizeof(reqPkt1));
    uint8_t *tmpReqBuf = SDF_BuffAppend(reqBuf, sizeof(reqPkt1));
    (void)memcpy_s(tmpReqBuf, sizeof(reqPkt1), reqPkt1, sizeof(reqPkt1));
    link->curTask.buff = reqBuf;
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(rspPkt2));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(rspPkt2));
    (void)memcpy_s(tmpBuf, sizeof(rspPkt2), rspPkt2, sizeof(rspPkt2));
    SSAPC_WriteRspHandle(link, tmp);
    SDF_BuffFree(tmp);

    EXPECT_EQ(link->status, SSAP_LINK_IDLE);
    DeleteLink();
}

TEST_F(UT_SSAP_CLIENT_WRITE, WRITE_RSP_ERROR)
{
    SSAP_Link_S *link = CreateLink();
    SDF_Buff_S *reqBuf = SDF_BuffNewWithReserve(sizeof(reqPkt3));
    uint8_t *tmpReqBuf = SDF_BuffAppend(reqBuf, sizeof(reqPkt3));
    (void)memcpy_s(tmpReqBuf, sizeof(reqPkt3), reqPkt3, sizeof(reqPkt3));
    link->curTask.buff = reqBuf;
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(rspPkt3Error));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(rspPkt3Error));
    (void)memcpy_s(tmpBuf, sizeof(rspPkt3Error), rspPkt3Error, sizeof(rspPkt3Error));
    SSAPC_WriteRspHandle(link, tmp);
    SDF_BuffFree(tmp);

    EXPECT_EQ(link->status, SSAP_LINK_IDLE);
    DeleteLink();
}