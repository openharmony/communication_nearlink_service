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
static DTAP_Data_Info_S g_dtapDataInfo = {.lcid = g_lcid};

static uint8_t g_buffCache[TEST_MAX_BUF_CACHE] = {0};
static uint8_t g_buffLen = 0;

static uint8_t reqPkt1[] = {0x02, 0x03, 0xFF, 0x00, 0x01, 0x01};
static uint8_t rspPkt1[] = {0x03, 0x03, 0x00, 0x02, 0x01, 0x01};
static uint8_t errRsp1[] = {0x01, 0x00, 0x02, 0x00, 0x00, 0x01};

class UT_SSAP_CLIENT_EXCHANGEINFO : public testing::Test {
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

static SSAP_Link_S *CreateLink()
{
    return SSAP_CreateSsapLink(&g_addr, g_lcid, MockSendCb);
}

static void DeleteLink()
{
    SSAP_DeleteSsapLinkByAddr(&g_addr);
}

TEST_F(UT_SSAP_CLIENT_EXCHANGEINFO, EXCHANGEINFO_REQ)
{
    SSAP_Link_S *link = CreateLink();

    SSAP_ExchangeInfoReqInfo_S *exchangeInfoReqInfo =
        (SSAP_ExchangeInfoReqInfo_S *)SDF_MemZalloc(sizeof(SSAP_ExchangeInfoReqInfo_S));
    exchangeInfoReqInfo->mtu = 0xFF;
    exchangeInfoReqInfo->version = 0x0101;
    (void)memcpy_s(&exchangeInfoReqInfo->addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    SSAP_TaskParam_S taskParam = {.appId = 1, .arg = exchangeInfoReqInfo, .freeFunc = SDF_MemFree,
        .func = SSAPC_ExchangeInfoReq, .timeout = 3000, .valid = true, .appCallback = NULL};
    SSAP_ProcessRequestTask(link, &taskParam, false);
    EXPECT_EQ(g_buffLen, sizeof(reqPkt1));
    EXPECT_EQ(memcmp(g_buffCache, reqPkt1, g_buffLen), 0);

    DeleteLink();
}

TEST_F(UT_SSAP_CLIENT_EXCHANGEINFO, EXCHANGEINFO_RSP)
{
    SSAP_Link_S *link = CreateLink();

    SSAP_ExchangeInfoReqInfo_S *exchangeInfoReqInfo =
        (SSAP_ExchangeInfoReqInfo_S *)SDF_MemZalloc(sizeof(SSAP_ExchangeInfoReqInfo_S));
    exchangeInfoReqInfo->mtu = 0xFF;
    exchangeInfoReqInfo->version = 0x0101;
    (void)memcpy_s(&exchangeInfoReqInfo->addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    SSAP_TaskParam_S taskParam = {.appId = 1, .arg = exchangeInfoReqInfo, .freeFunc = SDF_MemFree,
        .func = SSAPC_ExchangeInfoReq, .timeout = 3000, .valid = true, .appCallback = NULL};
    SSAP_ProcessRequestTask(link, &taskParam, false);
    EXPECT_EQ(g_buffLen, sizeof(reqPkt1));
    EXPECT_EQ(memcmp(g_buffCache, reqPkt1, g_buffLen), 0);

    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(rspPkt1));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(rspPkt1));
    (void)memcpy_s(tmpBuf, sizeof(rspPkt1), rspPkt1, sizeof(rspPkt1));
    SSAP_Recv(&g_dtapDataInfo, tmp);
    SDF_BuffFree(tmp);

    DeleteLink();
}

TEST_F(UT_SSAP_CLIENT_EXCHANGEINFO, EXCHANGEINFO_ERR)
{
    SSAP_Link_S *link = CreateLink();

    SSAP_ExchangeInfoReqInfo_S *exchangeInfoReqInfo =
        (SSAP_ExchangeInfoReqInfo_S *)SDF_MemZalloc(sizeof(SSAP_ExchangeInfoReqInfo_S));
    exchangeInfoReqInfo->mtu = 0xFF;
    exchangeInfoReqInfo->version = 0x0101;
    (void)memcpy_s(&exchangeInfoReqInfo->addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    SSAP_TaskParam_S taskParam = {.appId = 1, .arg = exchangeInfoReqInfo, .freeFunc = SDF_MemFree,
        .func = SSAPC_ExchangeInfoReq, .timeout = 3000, .valid = true, .appCallback = NULL};
    SSAP_ProcessRequestTask(link, &taskParam, false);
    EXPECT_EQ(g_buffLen, sizeof(reqPkt1));
    EXPECT_EQ(memcmp(g_buffCache, reqPkt1, g_buffLen), 0);

    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(errRsp1));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(errRsp1));
    (void)memcpy_s(tmpBuf, sizeof(errRsp1), errRsp1, sizeof(errRsp1));
    SSAP_Recv(&g_dtapDataInfo, tmp);
    SDF_BuffFree(tmp);

    DeleteLink();
}