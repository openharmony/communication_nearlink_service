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
#include "ssapc_client.h"
#include "ssaps_server.h"
#include "ssap_link.h"
#include "ssap_manager.h"
#include "ssap_common.h"
#include "sdf_addr.h"
#include "sdf_mem.h"
#include "ssap_utils.h"
#include "ssaps_server_api.h"
#include "ssaps_service.h"

#define TEST_MAX_BUF_CACHE 1024
static SLE_Addr_S g_addr = {.type = PUBLIC_ADDRESS, .addr = {0x03, 0x03, 0x04, 0x04, 0x05, 0x05}};
static uint16_t g_lcid = 1;

static uint8_t g_buffCache[TEST_MAX_BUF_CACHE] = {0};
static uint8_t g_buffLen = 0;

static uint8_t reqPkt1[] = {0x0C, 0x03, 0x1A, 0x00, 0x00, 0x22};
static uint8_t reqPkt2[] = {0x0C, 0x03, 0x20, 0x00, 0x00, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
static uint8_t reqPkt3[] = {0x0C, 0x03, 0x01, 0x04, 0x01, 0x02, 0x02, 0x02, 0x09};
static uint8_t reqPkt4[] = {0x0C, 0x03, 0x3C, 0x00, 0x04, 0x07, 0x08, 0x05};

class UT_SSAP_WRITE_CMD : public testing::Test {
protected:
    void SetUp() override
    {
        SSAP_ServerInit();
        SSAP_LinkInit();
    }

    void TearDown() override
    {
        SSAP_ServerDeInit();
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

TEST_F(UT_SSAP_WRITE_CMD, WRITE_CMD_DATA_TEST1)
{
    SSAP_Link_S *link = CreateLink();

    uint16_t len = 0x01;
    SSAP_WriteCmdInfo_S *writeCmdInfo = (SSAP_WriteCmdInfo_S *)SDF_MemZalloc(sizeof(SSAP_WriteCmdInfo_S) + len);
    writeCmdInfo->type = 0;
    writeCmdInfo->handle = 0x1A;
    writeCmdInfo->value.len = len;
    uint8_t data[1] = {0x22};
    (void)memcpy_s(writeCmdInfo->value.value, len, data, len);
    (void)memcpy_s(&writeCmdInfo->addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    SSAP_ProcessNormalTask(link, SSAPC_WriteCmd, writeCmdInfo, SDF_MemFree);

    EXPECT_EQ(g_buffLen, sizeof(reqPkt1));
    EXPECT_EQ(memcmp(g_buffCache, reqPkt1, g_buffLen), 0);

    DeleteLink();
}

TEST_F(UT_SSAP_WRITE_CMD, WRITE_CMD_DATA_TEST2)
{
    SSAP_Link_S *link = CreateLink();

    uint16_t len = 0x06;
    SSAP_WriteCmdInfo_S *writeCmdInfo = (SSAP_WriteCmdInfo_S *)SDF_MemZalloc(sizeof(SSAP_WriteCmdInfo_S) + len);
    writeCmdInfo->type = 0;
    writeCmdInfo->handle = 0x20;
    writeCmdInfo->value.len = len;
    uint8_t data[6] = {0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    (void)memcpy_s(writeCmdInfo->value.value, len, data, len);
    (void)memcpy_s(&writeCmdInfo->addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    SSAP_ProcessNormalTask(link, SSAPC_WriteCmd, writeCmdInfo, SDF_MemFree);

    EXPECT_EQ(g_buffLen, sizeof(reqPkt2));
    EXPECT_EQ(memcmp(g_buffCache, reqPkt2, g_buffLen), 0);

    DeleteLink();
}

TEST_F(UT_SSAP_WRITE_CMD, WRITE_CMD_DESCRIPTOR_TEST1)
{
    SSAP_Link_S *link = CreateLink();

    uint16_t len = 0x04;
    SSAP_WriteCmdInfo_S *writeCmdInfo = (SSAP_WriteCmdInfo_S *)SDF_MemZalloc(sizeof(SSAP_WriteCmdInfo_S) + len);
    writeCmdInfo->type = 1;
    writeCmdInfo->handle = 0x0401;
    writeCmdInfo->value.len = len;
    uint8_t data[4] = {0x02, 0x02, 0x02, 0x09};
    (void)memcpy_s(writeCmdInfo->value.value, len, data, len);
    (void)memcpy_s(&writeCmdInfo->addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    SSAP_ProcessNormalTask(link, SSAPC_WriteCmd, writeCmdInfo, SDF_MemFree);

    EXPECT_EQ(g_buffLen, sizeof(reqPkt3));
    EXPECT_EQ(memcmp(g_buffCache, reqPkt3, g_buffLen), 0);

    DeleteLink();
}

TEST_F(UT_SSAP_WRITE_CMD, WRITE_CMD_DESCRIPTOR_TEST2)
{
    SSAP_Link_S *link = CreateLink();

    uint16_t len = 0x03;
    SSAP_WriteCmdInfo_S *writeCmdInfo = (SSAP_WriteCmdInfo_S *)SDF_MemZalloc(sizeof(SSAP_WriteCmdInfo_S) + len);
    writeCmdInfo->type = 0x04;
    writeCmdInfo->handle = 0x3C;
    writeCmdInfo->value.len = len;
    uint8_t data[3] = {0x07, 0x08, 0x05};
    (void)memcpy_s(writeCmdInfo->value.value, len, data, len);
    (void)memcpy_s(&writeCmdInfo->addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    SSAP_ProcessNormalTask(link, SSAPC_WriteCmd, writeCmdInfo, SDF_MemFree);

    EXPECT_EQ(g_buffLen, sizeof(reqPkt4));
    EXPECT_EQ(memcmp(g_buffCache, reqPkt4, g_buffLen), 0);

    DeleteLink();
}