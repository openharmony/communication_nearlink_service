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

#include "ssap_link.h"
#include "ssap_type.h"
#include "ssapc_client.h"
#include "ssap_pkt.h"
#include "ssap_utils.h"
#include "ssap_manager.h"
#include "ssap_common.h"
#include "sdf_addr.h"
#include "sdf_buff.h"
#include "sdf_mem.h"
#include "dtap.h"

#include "cpfwk_log.h"

#define TEST_MAX_BUF_CACHE 1024

static SLE_Addr_S g_addr = {.type = PUBLIC_ADDRESS, .addr = {0x03, 0x03, 0x04, 0x04, 0x05, 0x05}};
static uint16_t g_lcid = 1;
static DTAP_Data_Info_S g_dtapDataInfo = {.lcid = g_lcid};
static uint8_t g_buffCache[TEST_MAX_BUF_CACHE] = {0};
static uint8_t g_buffLen = 0;

static NLSTK_SsapUuid_S g_uuid = {.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02}};

static uint8_t reqPktExchangeInfo[] = {0x02, 0x03, 0xFF, 0x00, 0x01, 0x01};
static uint8_t rspPktExchangeInfoError[] = {0x03, 0x03, 0x00, 0x02, 0x01};
static uint8_t rspPktExchangeInfo[] = {0x03, 0x00, 0x00, 0x00, 0x01, 0x01};

static uint8_t g_recvCbk = -1;
static SSAP_PduErrCode g_errCode = SSAP_ERRCODE_MAX;

class UT_SSAPC_CLIENT : public testing::Test {
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

static void SsapTaskAppCb(int32_t appId, void *arg)
{
    CP_LOG_ERROR("Enter SsapTaskAppCb");
    (void)appId;
    g_recvCbk = 1;
    SSAP_ExchangeComplete_S *complete = (SSAP_ExchangeComplete_S*)arg;
    g_errCode = (SSAP_PduErrCode)complete->errCode;
}

static void StubResetCache()
{
    g_recvCbk = -1;
    g_errCode = SSAP_ERRCODE_MAX;
}
// 检验SSAPC_ExchangeInfoRspHandle收包长度错误
TEST_F(UT_SSAPC_CLIENT, CLIENT_TEST_001)
{
    SSAP_Link_S *link = CreateLink();

    SSAP_ExchangeInfoReqInfo_S *exchangeInfoReqInfo =
        (SSAP_ExchangeInfoReqInfo_S *)SDF_MemZalloc(sizeof(SSAP_ExchangeInfoReqInfo_S));
    exchangeInfoReqInfo->mtu = 0xFF;
    exchangeInfoReqInfo->version = 0x0101;
    (void)memcpy_s(&exchangeInfoReqInfo->addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    SSAP_TaskParam_S taskParam = {.appId = 1, .arg = exchangeInfoReqInfo, .freeFunc = SDF_MemFree,
        .func = SSAPC_ExchangeInfoReq, .timeout = 3000, .valid = true, .appCallback = SsapTaskAppCb};
    SSAP_ProcessRequestTask(link, &taskParam, false);
    EXPECT_EQ(g_buffLen, sizeof(reqPktExchangeInfo));
    EXPECT_EQ(memcmp(g_buffCache, reqPktExchangeInfo, g_buffLen), 0);

    StubResetCache();
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(rspPktExchangeInfoError));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(rspPktExchangeInfoError));
    (void)memcpy_s(tmpBuf, sizeof(rspPktExchangeInfoError), rspPktExchangeInfoError, sizeof(rspPktExchangeInfoError));
    SSAP_Recv(&g_dtapDataInfo, tmp);
    SDF_BuffFree(tmp);
    EXPECT_EQ(g_recvCbk, 1);
    EXPECT_EQ(g_errCode, SSAP_ERRCODE_INVALID_PDU);

    DeleteLink();
}

TEST_F(UT_SSAPC_CLIENT, CLIENT_TEST_002)
{
    SSAP_Link_S *link = CreateLink();

    SSAP_ExchangeInfoReqInfo_S *exchangeInfoReqInfo =
        (SSAP_ExchangeInfoReqInfo_S *)SDF_MemZalloc(sizeof(SSAP_ExchangeInfoReqInfo_S));
    exchangeInfoReqInfo->mtu = 0xFF;
    exchangeInfoReqInfo->version = 0x0101;
    (void)memcpy_s(&exchangeInfoReqInfo->addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    SSAP_TaskParam_S taskParam = {.appId = 1, .arg = exchangeInfoReqInfo, .freeFunc = SDF_MemFree,
        .func = SSAPC_ExchangeInfoReq, .timeout = 3000, .valid = true, .appCallback = SsapTaskAppCb};
    SSAP_ProcessRequestTask(link, &taskParam, false);
    EXPECT_EQ(g_buffLen, sizeof(reqPktExchangeInfo));
    EXPECT_EQ(memcmp(g_buffCache, reqPktExchangeInfo, g_buffLen), 0);

    StubResetCache();
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(rspPktExchangeInfo));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(rspPktExchangeInfo));
    (void)memcpy_s(tmpBuf, sizeof(rspPktExchangeInfo), rspPktExchangeInfo, sizeof(rspPktExchangeInfo));
    SSAP_Recv(&g_dtapDataInfo, tmp);
    SDF_BuffFree(tmp);
    EXPECT_EQ(g_recvCbk, 1);
    EXPECT_EQ(g_errCode, SSAP_ERRCODE_SUCCESS);

    DeleteLink();
}
