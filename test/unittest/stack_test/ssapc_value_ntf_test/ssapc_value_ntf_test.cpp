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
#include "cm_logic_link_api.h"
#include "cm_def.h"

#include "cpfwk_log.h"

#define TEST_MAX_BUF_CACHE 1024

static SLE_Addr_S g_addr = {.type = PUBLIC_ADDRESS, .addr = {0x03, 0x03, 0x04, 0x04, 0x05, 0x05}};
static uint16_t g_lcid = 1;
static DTAP_Data_Info_S g_dtapDataInfo = {.lcid = g_lcid};
static uint8_t g_buffCache[TEST_MAX_BUF_CACHE] = {0};
static uint8_t g_buffLen = 0;

static uint8_t g_recvCbk = -1;
static SSAP_PduErrCode g_errCode = SSAP_ERRCODE_MAX;
static SSAP_ValuePkt_S g_valuePkt = {{0}, 0, 0, 0, 0, 0, {0}};
static SLE_Addr_S g_servChangeAddr = {0};
static uint16_t g_servChangeLength = 0;

class UT_SSAPC_VALUE_NTF : public testing::Test {
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

static void MockSendCb(SSAP_Link_S *link, SDF_Buff_S *buff, uint8_t opcode)
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
    (void)appId;
    g_recvCbk = 1;
}

static void ResetGlobalState()
{
    g_recvCbk = -1;
    g_errCode = SSAP_ERRCODE_MAX;
    (void)memset_s(&g_valuePkt, sizeof(g_valuePkt), 0, sizeof(g_valuePkt));
    (void)memset_s(&g_servChangeAddr, sizeof(g_servChangeAddr), 0, sizeof(g_servChangeAddr));
    g_servChangeLength = 0;
}

TEST_F(UT_SSAPC_VALUE_NTF, VALUE_NTF_TEST_001)
{
    SSAP_Link_S *link = CreateLink();
    EXPECT_NE(link, nullptr);

    SSAP_ExchangeInfoReqInfo_S *exchangeInfoReqInfo =
        (SSAP_ExchangeInfoReqInfo_S *)SDF_MemZalloc(sizeof(SSAP_ExchangeInfoReqInfo_S));
    exchangeInfoReqInfo->mtu = 0xFF;
    exchangeInfoReqInfo->version = 0x0101;
    (void)memcpy_s(&exchangeInfoReqInfo->addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    SSAP_TaskParam_S taskParam = {.appId = 1, .arg = exchangeInfoReqInfo, .freeFunc = SDF_MemFree,
        .func = SSAPC_ExchangeInfoReq, .timeout = 3000, .valid = true, .appCallback = SsapTaskAppCb};
    SSAP_ProcessRequestTask(link, &taskParam, false);

    ResetGlobalState();

    uint8_t valueNtfPkt[] = {
        SSAP_VALUE_NTF, 0x00,
        0x01, 0x00,
        0x03, 0x00,
        0xAA, 0xBB, 0xCC
    };
    size_t pktLen = sizeof(valueNtfPkt);

    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(pktLen);
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, pktLen);
    (void)memcpy_s(tmpBuf, pktLen, valueNtfPkt, pktLen);
    int ret = SSAP_Recv(&g_dtapDataInfo, tmp);
    SDF_BuffFree(tmp);

    EXPECT_EQ(ret, 0);

    DeleteLink();
}

TEST_F(UT_SSAPC_VALUE_NTF, VALUE_NTF_TEST_002)
{
    SSAP_Link_S *link = CreateLink();
    EXPECT_NE(link, nullptr);

    SSAP_ExchangeInfoReqInfo_S *exchangeInfoReqInfo =
        (SSAP_ExchangeInfoReqInfo_S *)SDF_MemZalloc(sizeof(SSAP_ExchangeInfoReqInfo_S));
    exchangeInfoReqInfo->mtu = 0xFF;
    exchangeInfoReqInfo->version = 0x0101;
    (void)memcpy_s(&exchangeInfoReqInfo->addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    SSAP_TaskParam_S taskParam = {.appId = 1, .arg = exchangeInfoReqInfo, .freeFunc = SDF_MemFree,
        .func = SSAPC_ExchangeInfoReq, .timeout = 3000, .valid = true, .appCallback = SsapTaskAppCb};
    SSAP_ProcessRequestTask(link, &taskParam, false);

    ResetGlobalState();

    uint8_t valueNtfPkt[] = {
        SSAP_VALUE_NTF, 0x00,
        0x01, 0x00,
        0x03, 0x00,
        0xAA, 0xBB, 0xCC,
        0x02, 0x00,
        0x02, 0x00,
        0x11, 0x22
    };
    size_t pktLen = sizeof(valueNtfPkt);

    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(pktLen);
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, pktLen);
    (void)memcpy_s(tmpBuf, pktLen, valueNtfPkt, pktLen);
    int ret = SSAP_Recv(&g_dtapDataInfo, tmp);
    SDF_BuffFree(tmp);

    EXPECT_EQ(ret, 0);

    DeleteLink();
}

TEST_F(UT_SSAPC_VALUE_NTF, VALUE_NTF_TEST_003)
{
    SSAP_Link_S *link = CreateLink();
    EXPECT_NE(link, nullptr);

    SSAP_ExchangeInfoReqInfo_S *exchangeInfoReqInfo =
        (SSAP_ExchangeInfoReqInfo_S *)SDF_MemZalloc(sizeof(SSAP_ExchangeInfoReqInfo_S));
    exchangeInfoReqInfo->mtu = 0xFF;
    exchangeInfoReqInfo->version = 0x0101;
    (void)memcpy_s(&exchangeInfoReqInfo->addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    SSAP_TaskParam_S taskParam = {.appId = 1, .arg = exchangeInfoReqInfo, .freeFunc = SDF_MemFree,
        .func = SSAPC_ExchangeInfoReq, .timeout = 3000, .valid = true, .appCallback = SsapTaskAppCb};
    SSAP_ProcessRequestTask(link, &taskParam, false);

    ResetGlobalState();

    uint8_t valueNtfInvalid[] = {
        SSAP_VALUE_NTF, 0x00
    };

    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(valueNtfInvalid));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(valueNtfInvalid));
    (void)memcpy_s(tmpBuf, sizeof(valueNtfInvalid), valueNtfInvalid, sizeof(valueNtfInvalid));
    int ret = SSAP_Recv(&g_dtapDataInfo, tmp);
    SDF_BuffFree(tmp);

    EXPECT_EQ(ret, 0);

    DeleteLink();
}

TEST_F(UT_SSAPC_VALUE_NTF, VALUE_NTF_TEST_004)
{
    SSAP_Link_S *link = CreateLink();
    EXPECT_NE(link, nullptr);

    SSAP_ExchangeInfoReqInfo_S *exchangeInfoReqInfo =
        (SSAP_ExchangeInfoReqInfo_S *)SDF_MemZalloc(sizeof(SSAP_ExchangeInfoReqInfo_S));
    exchangeInfoReqInfo->mtu = 0xFF;
    exchangeInfoReqInfo->version = 0x0101;
    (void)memcpy_s(&exchangeInfoReqInfo->addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    SSAP_TaskParam_S taskParam = {.appId = 1, .arg = exchangeInfoReqInfo, .freeFunc = SDF_MemFree,
        .func = SSAPC_ExchangeInfoReq, .timeout = 3000, .valid = true, .appCallback = SsapTaskAppCb};
    SSAP_ProcessRequestTask(link, &taskParam, false);

    ResetGlobalState();

    uint8_t valueNtfZeroLen[] = {
        SSAP_VALUE_NTF, 0x00,
        0x01, 0x00,
        0x00, 0x00
    };
    size_t pktLen = sizeof(valueNtfZeroLen);

    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(pktLen);
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, pktLen);
    (void)memcpy_s(tmpBuf, pktLen, valueNtfZeroLen, pktLen);
    int ret = SSAP_Recv(&g_dtapDataInfo, tmp);
    SDF_BuffFree(tmp);

    EXPECT_EQ(ret, 0);

    DeleteLink();
}

TEST_F(UT_SSAPC_VALUE_NTF, VALUE_NTF_TEST_005)
{
    SSAP_Link_S *link = CreateLink();
    EXPECT_NE(link, nullptr);

    SSAP_ExchangeInfoReqInfo_S *exchangeInfoReqInfo =
        (SSAP_ExchangeInfoReqInfo_S *)SDF_MemZalloc(sizeof(SSAP_ExchangeInfoReqInfo_S));
    exchangeInfoReqInfo->mtu = 0xFF;
    exchangeInfoReqInfo->version = 0x0101;
    (void)memcpy_s(&exchangeInfoReqInfo->addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    SSAP_TaskParam_S taskParam = {.appId = 1, .arg = exchangeInfoReqInfo, .freeFunc = SDF_MemFree,
        .func = SSAPC_ExchangeInfoReq, .timeout = 3000, .valid = true, .appCallback = SsapTaskAppCb};
    SSAP_ProcessRequestTask(link, &taskParam, false);

    ResetGlobalState();

    uint8_t valueNtfTruncated[] = {
        SSAP_VALUE_NTF, 0x00,
        0x01, 0x00,
        0x05, 0x00,
        0xAA, 0xBB
    };
    size_t pktLen = sizeof(valueNtfTruncated);

    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(pktLen);
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, pktLen);
    (void)memcpy_s(tmpBuf, pktLen, valueNtfTruncated, pktLen);
    int ret = SSAP_Recv(&g_dtapDataInfo, tmp);
    SDF_BuffFree(tmp);

    EXPECT_EQ(ret, 0);

    DeleteLink();
}
