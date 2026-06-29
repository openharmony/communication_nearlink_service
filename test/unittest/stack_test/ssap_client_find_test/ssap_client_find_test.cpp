/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#include "sdf_mem.h"
#include "ssapc_client.h"
#include "ssap_common.h"
#include "ssap_manager.h"
#include "ssap_link.h"
#include "sdf_addr.h"
#include "ssap_utils.h"

#define TEST_MAX_BUF_CACHE 1024

static uint8_t g_buffCache[TEST_MAX_BUF_CACHE] = {0};
static uint8_t g_buffLen = 0;
static SLE_Addr_S g_addr = {.type = PUBLIC_ADDRESS, .addr = {0x03, 0x03, 0x04, 0x04, 0x05, 0x05}};
static uint16_t g_lcid = 1;
static DTAP_Data_Info_S g_dtapDataInfo = {.lcid = g_lcid};

static NLSTK_SsapUuid_S g_uuid1 = {.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02}};
static NLSTK_SsapUuid_S g_uuid2 = {.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x02, 0x03}};

static uint8_t reqPkt1[] = {0x04, 0x11, 0x01, 0x00, 0xFF, 0x00};
static uint8_t reqPkt2[] = {0x06, 0x11, 0x01, 0x00, 0xFF, 0x00, 0x02, 0x01};
static uint8_t reqPkt3[] = {0x04, 0x13, 0x01, 0x00, 0xFF, 0x00};
static uint8_t reqPkt4[] = {0x06, 0x13, 0x01, 0x00, 0xFF, 0x00, 0x03, 0x02};

// find回复,op-1,ctrl混合模式-1,{指示-1,{开始句柄-2,结束句柄-2,uuid-2,member-1},...},
// {指示-1,{开始句柄-2,结束句柄-2,uuid-16,member-1},...}
static uint8_t rspPkt1[] = {0x05, 0x08, 0x01, 0x10, 0x00, 0x11, 0x00, 0x02, 0x01, 0x02,
    0x81, 0x12, 0x00, 0x12, 0x00, 0x12, 0x11, 0x10, 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A,
    0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x00};

// find回复,op-1,ctrl标准模式-1,{开始句柄-2,结束句柄-2,uuid-2,member-1}
static uint8_t rspPkt2[] = {0x07, 0x00, 0x10, 0x00, 0x11, 0x00, 0x02, 0x01, 0x02};

// find回复,op-1,ctrl标准模式-1,{开始句柄-2,uuid-2,operation-4,count-1,descriptor-x}
static uint8_t rspPkt3[] = {0x05, 0x00, 0x11, 0x00, 0x03, 0x02, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00};

// find回复,op-1,ctrl标准模式-1,{指示-1,{开始句柄-2,uuid-2,operation-4,count-1,descriptor-x},...}
// {指示-1,{开始句柄-2,uuid-2,operation-4,count-1,descriptor-x},...}
static uint8_t rspPkt4[] = {0x07, 0x08, 0x01, 0x11, 0x00, 0x03, 0x02, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x00, 0x80};

class UT_SSAP_CLIENT_FIND : public testing::Test {
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

TEST_F(UT_SSAP_CLIENT_FIND, FIND_PRIMARY_SERVICE)
{
    SSAP_Link_S *link = CreateLink();
    SSAP_ParamFind_S *findParam = (SSAP_ParamFind_S *)SDF_MemZalloc(sizeof(SSAP_ParamFind_S));
    findParam->startHandle = 1;
    findParam->endHandle = 0xFF;
    findParam->type = FIND_STRUCTURE_TYPE_PRIMARY_SERVICE;
    (void)memcpy_s(&findParam->addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    SSAP_TaskParam_S taskParam = {.appId = 1, .arg = findParam, .freeFunc = SDF_MemFree, .func = SSAPC_FindReq,
        .timeout = 3000, .valid = true, .appCallback = NULL};
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

TEST_F(UT_SSAP_CLIENT_FIND, FIND_BY_UUID_PRIMARY_SERVICE)
{
    SSAP_Link_S *link = CreateLink();
    SSAP_ParamFindByUuid_S *findParam = (SSAP_ParamFindByUuid_S *)SDF_MemZalloc(sizeof(SSAP_ParamFindByUuid_S));
    findParam->startHandle = 1;
    findParam->endHandle = 0xFF;
    findParam->type = FIND_STRUCTURE_TYPE_PRIMARY_SERVICE;
    (void)memcpy_s(&findParam->addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    (void)memcpy_s(&findParam->uuid, sizeof(NLSTK_SsapUuid_S), &g_uuid1, sizeof(NLSTK_SsapUuid_S));
    SSAP_TaskParam_S taskParam = {.appId = 1, .arg = findParam, .freeFunc = SDF_MemFree, .func = SSAPC_FindByUuidReq,
        .timeout = 3000, .valid = true, .appCallback = NULL};
    SSAP_ProcessRequestTask(link, &taskParam, false);
    EXPECT_EQ(g_buffLen, sizeof(reqPkt2));
    EXPECT_EQ(memcmp(g_buffCache, reqPkt2, g_buffLen), 0);

    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(rspPkt2));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(rspPkt2));
    (void)memcpy_s(tmpBuf, sizeof(rspPkt2), rspPkt2, sizeof(rspPkt2));
    SSAP_Recv(&g_dtapDataInfo, tmp);
    SDF_BuffFree(tmp);

    DeleteLink();
}

TEST_F(UT_SSAP_CLIENT_FIND, FIND_PROPERTY)
{
    SSAP_Link_S *link = CreateLink();
    SSAP_ParamFind_S *findParam = (SSAP_ParamFind_S *)SDF_MemZalloc(sizeof(SSAP_ParamFind_S));
    findParam->startHandle = 1;
    findParam->endHandle = 0xFF;
    findParam->type = FIND_STRUCTURE_TYPE_PROPERTY;
    (void)memcpy_s(&findParam->addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    SSAP_TaskParam_S taskParam = {.appId = 1, .arg = findParam, .freeFunc = SDF_MemFree, .func = SSAPC_FindReq,
        .timeout = 3000, .valid = true, .appCallback = NULL};
    SSAP_ProcessRequestTask(link, &taskParam, false);
    EXPECT_EQ(g_buffLen, sizeof(reqPkt3));
    EXPECT_EQ(memcmp(g_buffCache, reqPkt3, g_buffLen), 0);

    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(rspPkt3));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(rspPkt3));
    (void)memcpy_s(tmpBuf, sizeof(rspPkt3), rspPkt3, sizeof(rspPkt3));
    SSAP_Recv(&g_dtapDataInfo, tmp);
    SDF_BuffFree(tmp);

    DeleteLink();
}

TEST_F(UT_SSAP_CLIENT_FIND, FIND_BY_UUID_PROPERTY)
{
    SSAP_Link_S *link = CreateLink();
    SSAP_ParamFindByUuid_S *findParam = (SSAP_ParamFindByUuid_S *)SDF_MemZalloc(sizeof(SSAP_ParamFindByUuid_S));
    findParam->startHandle = 1;
    findParam->endHandle = 0xFF;
    findParam->type = FIND_STRUCTURE_TYPE_PROPERTY;
    (void)memcpy_s(&findParam->uuid, sizeof(NLSTK_SsapUuid_S), &g_uuid2, sizeof(NLSTK_SsapUuid_S));
    (void)memcpy_s(&findParam->addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    SSAP_TaskParam_S taskParam = {.appId = 1, .arg = findParam, .freeFunc = SDF_MemFree, .func = SSAPC_FindByUuidReq,
        .timeout = 3000, .valid = true, .appCallback = NULL};
    SSAP_ProcessRequestTask(link, &taskParam, false);
    EXPECT_EQ(g_buffLen, sizeof(reqPkt4));
    EXPECT_EQ(memcmp(g_buffCache, reqPkt4, g_buffLen), 0);

    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(rspPkt4));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(rspPkt4));
    (void)memcpy_s(tmpBuf, sizeof(rspPkt4), rspPkt4, sizeof(rspPkt4));
    SSAP_Recv(&g_dtapDataInfo, tmp);
    SDF_BuffFree(tmp);
    DeleteLink();
}
