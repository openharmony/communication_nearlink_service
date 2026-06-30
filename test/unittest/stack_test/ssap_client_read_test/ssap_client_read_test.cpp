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

#include "nlstk_cfgdb_api.h"
#include "nlstk_cfgdb.h"

#define TEST_MAX_BUF_CACHE 1024
static NLSTK_SsapUuid_S g_uuid1 = {.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02}};
static NLSTK_SsapUuid_S g_uuid2 = {.uuid = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
    0x0c, 0x0d, 0x0e, 0x0f, 0x10}};
static SLE_Addr_S g_addr = {.type = PUBLIC_ADDRESS, .addr = {0x03, 0x03, 0x04, 0x04, 0x05, 0x05}};
static uint16_t g_lcid = 1;

static uint8_t g_buffCache[TEST_MAX_BUF_CACHE] = {0};
static uint8_t g_buffLen = 0;

static uint8_t reqPkt1[] = {0x08, 0x03, 0xFF, 0x00, 0x00};
static uint8_t reqPkt2[] = {0x0a, 0x00, 0x01, 0x00, 0xFF, 0x00, 0x00, 0x02, 0x01};
static uint8_t reqPkt3[] = {0x0a, 0x01, 0x01, 0x00, 0xFF, 0x00, 0x00, 0x10, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09,
    0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01};
static uint8_t reqPkt4[] = {0x08, 0x03, 0x01, 0x00, 0x00, 0xFF, 0x00, 0x00};

static uint8_t rspPkt1[] = {0x09, 0x03, 0x01};
static uint8_t rspPkt2[] = {0x09, 0x0b, 0x06, 0x00};
static uint8_t rspPkt3[] = {0x0b, 0x03, 0xFF, 0x00, 0x01};
static uint8_t rspPkt4[] = {0x0b, 0x0b, 0xFF, 0x00, 0x08, 0x00};
static uint8_t rspPkt5[] = {0x0b, 0x07, 0xEE, 0x00, 0x01, 0x80, 0x01, 0xFF, 0x00, 0x02, 0x80, 0x01, 0x02};
static uint8_t rspPkt6[] = {0x0b, 0x0F, 0xEE, 0x00, 0x01, 0x80, 0x01, 0xFF, 0x00, 0x06, 0x00};
static uint8_t rspPkt7[] = {0x0b, 0x0F, 0xFF, 0x00, 0x06, 0x00, 0xEE, 0x00, 0x01, 0x80, 0x01};
static uint8_t rspPkt8[] = {0x09, 0x07, 0x81, 0x01, 0x81, 0x02};

class UT_SSAP_CLIENT_READ : public testing::Test {
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
    SSAP_Link *link = SSAP_CreateSsapLink(&g_addr, g_lcid, MockSendCb);
    return link;
}

static void DeleteLink()
{
    SSAP_DeleteSsapLinkByAddr(&g_addr);
}

TEST_F(UT_SSAP_CLIENT_READ, READ_REQ)
{
    SSAP_Link_S *link = CreateLink();

    SSAP_ReadReqInfo_S *readReqInfo = (SSAP_ReadReqInfo_S *)SDF_MemZalloc(sizeof(SSAP_ReadReqInfo_S));
    readReqInfo->handle = 0xFF;
    readReqInfo->type = 0;
    (void)memcpy_s(&readReqInfo->addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    SSAP_TaskParam_S taskParam = {.appId = 1, .arg = readReqInfo, .freeFunc = SDF_MemFree, .func = SSAPC_ReadReq,
        .timeout = 3000, .valid = true, .appCallback = NULL};
    SSAP_ProcessRequestTask(link, &taskParam, false);
    EXPECT_EQ(g_buffLen, sizeof(reqPkt1));
    EXPECT_EQ(memcmp(g_buffCache, reqPkt1, g_buffLen), 0);

    DeleteLink();
}

TEST_F(UT_SSAP_CLIENT_READ, READ_BY_STANDARD_UUID_REQ)
{
    SSAP_Link_S *link = CreateLink();

    SSAP_ReadByUuidReqInfo_S *readByUuidReqInfo =
        (SSAP_ReadByUuidReqInfo_S *)SDF_MemZalloc(sizeof(SSAP_ReadByUuidReqInfo_S));
    readByUuidReqInfo->handleStart = 0x01;
    readByUuidReqInfo->handleEnd = 0xFF;
    readByUuidReqInfo->dataType = 0;
    (void)memcpy_s(&readByUuidReqInfo->addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    (void)memcpy_s(&readByUuidReqInfo->uuid, sizeof(NLSTK_SsapUuid_S), &g_uuid1, sizeof(NLSTK_SsapUuid_S));
    SSAP_TaskParam_S taskParam = {.appId = 1, .arg = readByUuidReqInfo, .freeFunc = SDF_MemFree, .func = SSAPC_ReadByUuidReq,
        .timeout = 3000, .valid = true, .appCallback = NULL};
    SSAP_ProcessRequestTask(link, &taskParam, false);
    EXPECT_EQ(g_buffLen, sizeof(reqPkt2));
    EXPECT_EQ(memcmp(g_buffCache, reqPkt2, g_buffLen), 0);

    DeleteLink();
}

TEST_F(UT_SSAP_CLIENT_READ, READ_BY_CUSTOM_UUID_REQ)
{
    SSAP_Link_S *link = CreateLink();

    SSAP_ReadByUuidReqInfo_S *readByUuidReqInfo =
        (SSAP_ReadByUuidReqInfo_S *)SDF_MemZalloc(sizeof(SSAP_ReadByUuidReqInfo_S));
    readByUuidReqInfo->handleStart = 0x01;
    readByUuidReqInfo->handleEnd = 0xFF;
    readByUuidReqInfo->dataType = 0;
    (void)memcpy_s(&readByUuidReqInfo->addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    (void)memcpy_s(&readByUuidReqInfo->uuid, sizeof(NLSTK_SsapUuid_S), &g_uuid2, sizeof(NLSTK_SsapUuid_S));
    SSAP_TaskParam_S taskParam = {.appId = 1, .arg = readByUuidReqInfo, .freeFunc = SDF_MemFree, .func = SSAPC_ReadByUuidReq,
        .timeout = 3000, .valid = true, .appCallback = NULL};
    SSAP_ProcessRequestTask(link, &taskParam, false);
    EXPECT_EQ(g_buffLen, sizeof(reqPkt3));
    EXPECT_EQ(memcmp(g_buffCache, reqPkt3, g_buffLen), 0);

    DeleteLink();
}

void PrintValuePkt(SSAP_ValuePkt_S *valuePkt)
{
    printf("Address: ");
    for (int i = 0; i < SLE_ADDR_LEN; i++) {
        printf("%02x", valuePkt->addr.addr[i]);
    }
    printf("\n");
    printf("Op Code: %02x\n", valuePkt->opCode);
    printf("Control Code: %02x\n", valuePkt->controlCode);
    printf("Handle: %04x\n", valuePkt->handle);
    printf("Data Type: %02x\n", valuePkt->dataType);
    printf("Error Code: %02x\n", valuePkt->errorCode);
    printf("Value Len: %02x\n", valuePkt->value.len);
    printf("Value: ");
    for (int i = 0; i < valuePkt->value.len; i++) {
        printf("%02x", valuePkt->value.value[i]);
    }
    printf("\n");
}

TEST_F(UT_SSAP_CLIENT_READ, READ_RSP_SUCCESSPKT)
{
    SSAP_Link_S *link = CreateLink();
    SDF_Buff_S *reqBuf = SDF_BuffNewWithReserve(sizeof(reqPkt1));
    uint8_t *tmpReqBuf = SDF_BuffAppend(reqBuf, sizeof(reqPkt1));
    (void)memcpy_s(tmpReqBuf, sizeof(reqPkt1), reqPkt1, sizeof(reqPkt1));
    link->curTask.buff = reqBuf;
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(rspPkt1));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(rspPkt1));
    (void)memcpy_s(tmpBuf, sizeof(rspPkt1), rspPkt1, sizeof(rspPkt1));
    SSAPC_ReadRspHandle(link, tmp);
    SDF_BuffFree(tmp);

    EXPECT_EQ(link->status, SSAP_LINK_IDLE);
    DeleteLink();
}

TEST_F(UT_SSAP_CLIENT_READ, READ_RSP_ERRORPKT)
{
    SSAP_Link_S *link = CreateLink();
    SDF_Buff_S *reqBuf = SDF_BuffNewWithReserve(sizeof(reqPkt1));
    uint8_t *tmpReqBuf = SDF_BuffAppend(reqBuf, sizeof(reqPkt1));
    (void)memcpy_s(tmpReqBuf, sizeof(reqPkt1), reqPkt1, sizeof(reqPkt1));
    link->curTask.buff = reqBuf;
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(rspPkt2));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(rspPkt2));
    (void)memcpy_s(tmpBuf, sizeof(rspPkt2), rspPkt2, sizeof(rspPkt2));
    SSAPC_ReadRspHandle(link, tmp);
    SDF_BuffFree(tmp);

    EXPECT_EQ(link->status, SSAP_LINK_IDLE);
    DeleteLink();
}

TEST_F(UT_SSAP_CLIENT_READ, READ_BYUUID_RSP_SINGLE_SUCCESS)
{
    SSAP_Link_S *link = CreateLink();
    SDF_Buff_S *reqBuf = SDF_BuffNewWithReserve(sizeof(reqPkt2));
    uint8_t *tmpReqBuf = SDF_BuffAppend(reqBuf, sizeof(reqPkt2));
    (void)memcpy_s(tmpReqBuf, sizeof(reqPkt2), reqPkt2, sizeof(reqPkt2));
    link->curTask.buff = reqBuf;
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(rspPkt3));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(rspPkt3));
    (void)memcpy_s(tmpBuf, sizeof(rspPkt3), rspPkt3, sizeof(rspPkt3));
    SSAPC_ReadByUuidRspHandle(link, tmp);
    SDF_BuffFree(tmp);

    EXPECT_EQ(link->status, SSAP_LINK_IDLE);
    DeleteLink();
}

TEST_F(UT_SSAP_CLIENT_READ, READ_BYUUID_RSP_SINGLE_FAILED)
{
    SSAP_Link_S *link = CreateLink();
    SDF_Buff_S *reqBuf = SDF_BuffNewWithReserve(sizeof(reqPkt2));
    uint8_t *tmpReqBuf = SDF_BuffAppend(reqBuf, sizeof(reqPkt2));
    (void)memcpy_s(tmpReqBuf, sizeof(reqPkt2), reqPkt2, sizeof(reqPkt2));
    link->curTask.buff = reqBuf;
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(rspPkt4));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(rspPkt4));
    (void)memcpy_s(tmpBuf, sizeof(rspPkt4), rspPkt4, sizeof(rspPkt4));
    SSAPC_ReadByUuidRspHandle(link, tmp);
    SDF_BuffFree(tmp);

    EXPECT_EQ(link->status, SSAP_LINK_IDLE);
    DeleteLink();
}

TEST_F(UT_SSAP_CLIENT_READ, READ_BYUUID_RSP_MULTI_SUCCESS)
{
    SSAP_Link_S *link = CreateLink();
    SDF_Buff_S *reqBuf = SDF_BuffNewWithReserve(sizeof(reqPkt2));
    uint8_t *tmpReqBuf = SDF_BuffAppend(reqBuf, sizeof(reqPkt2));
    (void)memcpy_s(tmpReqBuf, sizeof(reqPkt2), reqPkt2, sizeof(reqPkt2));
    link->curTask.buff = reqBuf;
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(rspPkt5));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(rspPkt5));
    (void)memcpy_s(tmpBuf, sizeof(rspPkt5), rspPkt5, sizeof(rspPkt5));
    SSAPC_ReadByUuidRspHandle(link, tmp);
    SDF_BuffFree(tmp);

    EXPECT_EQ(link->status, SSAP_LINK_IDLE);
    DeleteLink();
}

TEST_F(UT_SSAP_CLIENT_READ, READ_BYUUID_RSP_MULTI_FAILED1)
{
    SSAP_Link_S *link = CreateLink();
    SDF_Buff_S *reqBuf = SDF_BuffNewWithReserve(sizeof(reqPkt2));
    uint8_t *tmpReqBuf = SDF_BuffAppend(reqBuf, sizeof(reqPkt2));
    (void)memcpy_s(tmpReqBuf, sizeof(reqPkt2), reqPkt2, sizeof(reqPkt2));
    link->curTask.buff = reqBuf;
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(rspPkt6));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(rspPkt6));
    (void)memcpy_s(tmpBuf, sizeof(rspPkt6), rspPkt6, sizeof(rspPkt6));
    SSAPC_ReadByUuidRspHandle(link, tmp);
    SDF_BuffFree(tmp);

    EXPECT_EQ(link->status, SSAP_LINK_IDLE);
    DeleteLink();
}

TEST_F(UT_SSAP_CLIENT_READ, READ_BYUUID_RSP_MULTI_FAILED2)
{
    SSAP_Link_S *link = CreateLink();
    SDF_Buff_S *reqBuf = SDF_BuffNewWithReserve(sizeof(reqPkt2));
    uint8_t *tmpReqBuf = SDF_BuffAppend(reqBuf, sizeof(reqPkt2));
    (void)memcpy_s(tmpReqBuf, sizeof(reqPkt2), reqPkt2, sizeof(reqPkt2));
    link->curTask.buff = reqBuf;
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(rspPkt7));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(rspPkt7));
    (void)memcpy_s(tmpBuf, sizeof(rspPkt7), rspPkt7, sizeof(rspPkt7));
    SSAPC_ReadByUuidRspHandle(link, tmp);
    SDF_BuffFree(tmp);

    EXPECT_EQ(link->status, SSAP_LINK_IDLE);
    DeleteLink();
}

TEST_F(UT_SSAP_CLIENT_READ, MULTI_ITEMS_READ_RSP_SUCCESSPKT)
{
    SSAP_Link_S *link = CreateLink();
    // 设置设备支持服务结构发现能力位图，CFGDB_READ_MULTI_HANDLES
    NLSTK_ManufacturerAbility_S ablility = {.ability[0] = 2};
    uint32_t ret2 = NLSTK_CfgdbSetManufacturerAbility(&g_addr, &ablility);
    SDF_Buff_S *reqBuf = SDF_BuffNewWithReserve(sizeof(reqPkt4));
    uint8_t *tmpReqBuf = SDF_BuffAppend(reqBuf, sizeof(reqPkt4));
    (void)memcpy_s(tmpReqBuf, sizeof(reqPkt4), reqPkt4, sizeof(reqPkt4));
    link->curTask.buff = reqBuf;
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(rspPkt8));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(rspPkt8));
    (void)memcpy_s(tmpBuf, sizeof(rspPkt8), rspPkt8, sizeof(rspPkt8));
    SSAPC_ReadRspHandle(link, tmp);
    SDF_BuffFree(tmp);

    EXPECT_EQ(link->status, SSAP_LINK_IDLE);
    DeleteLink();
}