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
#include "ssaps_server.h"
#include "ssap_link.h"
#include "sdf_addr.h"
#include "sdf_mem.h"
#include "ssaps_service.h"
#include "ssap_manager.h"
#include "ssap_utils.h"
#include "ssaps_server_api.h"
#include "ssap_common.h"

#include "sdf_string.h"
#include "cpfwk_log.h"

#define TEST_MAX_BUF_CACHE 1024
static NLSTK_SsapUuid_S g_uuid1 = {.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02}};
static NLSTK_SsapUuid_S g_uuid2 = {.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x02, 0x03}};
static SLE_Addr_S g_addr = {.type = PUBLIC_ADDRESS, .addr = {0x03, 0x03, 0x04, 0x04, 0x05, 0x05}};
static uint16_t g_lcid = 1;

static uint8_t g_buffCache[TEST_MAX_BUF_CACHE] = {0};
static uint8_t g_buffLen = 0;

static uint8_t reqPkt1[] = {0x0d, 0x03, 0x11, 0x00, 0x00, 0x06};
static uint8_t reqPkt2[] = {0x0d, 0x03, 0x11, 0x00, 0x01, 0x07};
static uint8_t reqPkt3[] = {0x0d, 0x03, 0x12, 0x00, 0x00, 0x06};
static uint8_t reqPkt4[] = {0x0d, 0x03, 0x12, 0x00, 0x00};
static uint8_t reqPkt5[] = {0x0d, 0x07, 0x11, 0x00, 0x00, 0x06};
static uint8_t reqPkt6[] = {0x0C, 0x07, 0x11, 0x00, 0x00, 0x06};
static uint8_t reqPkt7[] = {0x0C, 0x13, 0x11, 0x00, 0x00, 0x06};
static uint8_t reqPkt8[] = {0x0C, 0x00, 0x11, 0x00, 0x00, 0x06};
static uint8_t reqPkt9[] = {0x0C, 0x03, 0x11, 0x00, 0x00, 0x06};

static uint8_t rspPkt1[] = {0x0e, 0x03, 0x11, 0x00, 0x00, 0x06};
static uint8_t rspPkt2[] = {0x0e, 0x03, 0x11, 0x00, 0x01, 0x07};
static uint8_t rspPkt3[] = {0x0e, 0x07, 0x01, 0x12, 0x00, 0x04};
static uint8_t rspPkt4[] = {0x01, 0x00, 0x0D, 0x00, 0x00, 0x01};
static uint8_t rspPkt5[] = {0x01, 0x00, 0x0D, 0x00, 0x00, 0x01};

class UT_SSAP_SERVER_WRITE : public testing::Test {
protected:
    void SetUp() override
    {
        SSAP_ServerInit();
        SSAP_LinkInit();
    }

    void TearDown() override
    {
        SSAP_ServerDeInit();
    }
};

static void AddService()
{
    SSAP_ParamAddService_S *serviceParam = (SSAP_ParamAddService_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddService_S));
    serviceParam->serviceType = ITEM_TYPE_STD_PRIMARY_SERVICE;
    (void)memcpy_s(&serviceParam->uuid, sizeof(NLSTK_SsapUuid_S), &g_uuid1, sizeof(NLSTK_SsapUuid_S));
    SSAP_CacheService(serviceParam);
    SDF_MemFree(serviceParam);
    SSAP_ParamAddProperty_S *propertyParam =
        (SSAP_ParamAddProperty_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddProperty_S) + 1);
    (void)memcpy_s(&propertyParam->uuid, sizeof(NLSTK_SsapUuid_S), &g_uuid2, sizeof(NLSTK_SsapUuid_S));
    propertyParam->val.len = 1;
    propertyParam->val.value[0] = 0xFF;
    propertyParam->operation.operationValue = 0x104;
    SSAP_CacheProperty(propertyParam);
    SDF_MemFree(propertyParam);
    SSAP_ParamAddDescriptor_S *descParam =
        (SSAP_ParamAddDescriptor_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddDescriptor_S) + 1);
    descParam->type = DESC_TYPE_PROPERTY_INSTRUCTION;
    descParam->val.len = 1;
    descParam->val.value[0] = 0xEE;
    descParam->operation.operationValue = 0x104;
    SSAP_CacheDescriptor(descParam);
    SDF_MemFree(descParam);
    SSAP_StartService(NULL);
}

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

TEST_F(UT_SSAP_SERVER_WRITE, WRITE_REQ_PROPERTY_SUCCESS)
{
    AddService();

    SSAP_Link_S *link = CreateLink();
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(reqPkt1));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(reqPkt1));
    (void)memcpy_s(tmpBuf, sizeof(reqPkt1), reqPkt1, sizeof(reqPkt1));
    SSAPS_WriteReqHandle(link, tmp);
    SDF_BuffFree(tmp);

    DeleteLink();
    EXPECT_EQ(memcmp(g_buffCache, rspPkt1, g_buffLen), 0);
}

TEST_F(UT_SSAP_SERVER_WRITE, WRITE_REQ_DESCRIPTOR_SUCCESS)
{
    AddService();

    SSAP_Link_S *link = CreateLink();
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(reqPkt2));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(reqPkt2));
    (void)memcpy_s(tmpBuf, sizeof(reqPkt2), reqPkt2, sizeof(reqPkt2));
    SSAPS_WriteReqHandle(link, tmp);
    SDF_BuffFree(tmp);

    DeleteLink();
    EXPECT_EQ(memcmp(g_buffCache, rspPkt2, g_buffLen), 0);
}

TEST_F(UT_SSAP_SERVER_WRITE, WRITE_REQ_PROPERTY_FAILED)
{
    AddService();

    SSAP_Link_S *link = CreateLink();
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(reqPkt3));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(reqPkt3));
    (void)memcpy_s(tmpBuf, sizeof(reqPkt3), reqPkt3, sizeof(reqPkt3));
    SSAPS_WriteReqHandle(link, tmp);
    SDF_BuffFree(tmp);

    DeleteLink();
    EXPECT_EQ(memcmp(g_buffCache, rspPkt3, g_buffLen), 0);
}

// 报文长度错误
TEST_F(UT_SSAP_SERVER_WRITE, WRITE_REQ_PROPERTY_FAILED_001)
{
    AddService();

    SSAP_Link_S *link = CreateLink();
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(reqPkt4));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(reqPkt4));
    (void)memcpy_s(tmpBuf, sizeof(reqPkt4), reqPkt4, sizeof(reqPkt4));
    SSAPS_WriteReqHandle(link, tmp);
    SDF_BuffFree(tmp);

    DeleteLink();
    EXPECT_EQ(g_buffLen, sizeof(rspPkt4));
    EXPECT_EQ(memcmp(g_buffCache, rspPkt4, g_buffLen), 0);
}

// 写入多个值
TEST_F(UT_SSAP_SERVER_WRITE, WRITE_REQ_PROPERTY_FAILED_002)
{
    AddService();

    SSAP_Link_S *link = CreateLink();
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(reqPkt5));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(reqPkt5));
    (void)memcpy_s(tmpBuf, sizeof(reqPkt5), reqPkt5, sizeof(reqPkt5));
    SSAPS_WriteReqHandle(link, tmp);
    SDF_BuffFree(tmp);

    DeleteLink();
    EXPECT_EQ(g_buffLen, sizeof(rspPkt5));
    EXPECT_EQ(memcmp(g_buffCache, rspPkt5, g_buffLen), 0);
}

// 写入多个值
TEST_F(UT_SSAP_SERVER_WRITE, WRITE_CMD_PROPERTY_FAILED_001)
{
    AddService();

    SSAP_Link_S *link = CreateLink();
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(reqPkt6));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(reqPkt6));
    (void)memcpy_s(tmpBuf, sizeof(reqPkt6), reqPkt6, sizeof(reqPkt6));
    SSAPS_WriteCmdHandle(link, tmp);
    SDF_BuffFree(tmp);
}

// 写入操作指示错误
TEST_F(UT_SSAP_SERVER_WRITE, WRITE_CMD_PROPERTY_FAILED_002)
{
    AddService();

    SSAP_Link_S *link = CreateLink();
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(reqPkt7));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(reqPkt7));
    (void)memcpy_s(tmpBuf, sizeof(reqPkt7), reqPkt7, sizeof(reqPkt7));
    SSAPS_WriteCmdHandle(link, tmp);
    SDF_BuffFree(tmp);
}

// 分片标记错误
TEST_F(UT_SSAP_SERVER_WRITE, WRITE_CMD_PROPERTY_FAILED_003)
{
    AddService();

    SSAP_Link_S *link = CreateLink();
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(reqPkt8));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(reqPkt8));
    (void)memcpy_s(tmpBuf, sizeof(reqPkt8), reqPkt8, sizeof(reqPkt8));
    SSAPS_WriteCmdHandle(link, tmp);
    SDF_BuffFree(tmp);
}

// 未找到属性
TEST_F(UT_SSAP_SERVER_WRITE, WRITE_CMD_PROPERTY_FAILED_004)
{
    SSAP_Link_S *link = CreateLink();
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(reqPkt9));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(reqPkt9));
    (void)memcpy_s(tmpBuf, sizeof(reqPkt9), reqPkt9, sizeof(reqPkt9));
    SSAPS_WriteCmdHandle(link, tmp);
    SDF_BuffFree(tmp);
}


TEST_F(UT_SSAP_SERVER_WRITE, WRITE_CMD_PROPERTY_FAILED_005)
{
    AddService();

    SSAP_Link_S *link = CreateLink();
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(reqPkt9));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(reqPkt9));
    (void)memcpy_s(tmpBuf, sizeof(reqPkt9), reqPkt9, sizeof(reqPkt9));
    SSAPS_WriteCmdHandle(link, tmp);
    SDF_BuffFree(tmp);
}