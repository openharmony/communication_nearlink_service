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
#include "ssaps_server.h"
#include "ssapc_client.h"
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
static NLSTK_SsapUuid_S g_uuid3 = {.uuid = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
    0x0c, 0x0d, 0x0e, 0x0f, 0x10}};
static SLE_Addr_S g_addr = {.type = PUBLIC_ADDRESS, .addr = {0x03, 0x03, 0x04, 0x04, 0x05, 0x05}};
static uint16_t g_lcid = 1;

static uint8_t g_buffCache[TEST_MAX_BUF_CACHE] = {0};
static uint8_t g_buffLen = 0;

static uint8_t reqPkt1[] = {0x08, 0x03, 0x11, 0x00, 0x00};
static uint8_t reqPkt2[] = {0x08, 0x03, 0x11, 0x00, 0x01};
static uint8_t reqPkt3[] = {0x08, 0x03, 0x12, 0x00, 0x00};
static uint8_t reqPkt4[] = {0x0a, 0x00, 0x01, 0x00, 0xFF, 0x00, 0x00, 0x03, 0x02};
static uint8_t reqPkt5[] = {0x0a, 0x00, 0x01, 0x00, 0xFF, 0x00, 0x01, 0x03, 0x02};
static uint8_t reqPkt6[] = {0x0a, 0x00, 0x01, 0x00, 0x05, 0x00, 0x00, 0x03, 0x02};
static uint8_t reqPkt7[] = {0x0a, 0x01, 0x01, 0x00, 0xFF, 0x00, 0x00, 0x10, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09,
    0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01};
static uint8_t reqPkt8[] = {0x0a, 0x00, 0x01, 0x00, 0xFF, 0x00, 0x00, 0x03, 0x02};
static uint8_t reqPkt9[] = {0x08, 0x03, 0x11, 0x00, 0x02};
static uint8_t reqPktLen1[] = {0x08, 0x03};
static uint8_t reqPktLen2[] = {0x08, 0x03, 0x11, 0x00, 0x02, 0x03};
static uint8_t reqPktType[] = {0x08, 0x03, 0x11, 0x00, 0x06};
static uint8_t reqPkt10[] = {0x08, 0x00, 0x11, 0x00, 0x02};
static uint8_t reqPkt11[] = {0x0a, 0x00, 0x01, 0x00, 0xFF, 0x00, 0x00};
static uint8_t reqPkt12[] = {0x0a, 0x00, 0x01, 0x00, 0xFF, 0x00, 0x07, 0x03, 0x02};
static uint8_t reqPkt13[] = {0x0a, 0x01, 0x01, 0x00, 0xFF, 0x00, 0x00, 0x03, 0x02};
static uint8_t reqPkt14[] = {0x0a, 0x00, 0xFF, 0x00, 0x01, 0x00, 0x00, 0x03, 0x02};
static uint8_t reqPkt15[] = {0x0a, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x03, 0x02};

static uint8_t rspPkt1[] = {0x09, 0x03, 0xFF};
static uint8_t rspPkt2[] = {0x09, 0x03, 0xEE};
static uint8_t rspPkt3[] = {0x09, 0x0b, 0x0D, 0x00};
static uint8_t rspPkt4[] = {0x0b, 0x03, 0x11, 0x00, 0xFF};
static uint8_t rspPkt5[] = {0x0b, 0x03, 0x11, 0x00, 0xEE};
static uint8_t rspPkt6[] = {0x0b, 0x0b, 0x01, 0x00, 0x0b, 0x00};
static uint8_t rspPkt7[] = {0x0b, 0x03, 0x11, 0x00, 0xFF};
static uint8_t rspPkt8[] = {0x0b, 0x07, 0x11, 0x00, 0x01, 0x80, 0xFF, 0x12, 0x00, 0x01, 0x80, 0x66};
static uint8_t rspPkt9[] = {0x0b, 0x0F, 0x11, 0x00, 0x01, 0x80, 0xFF, 0x12, 0x00, 0x06, 0x00};
static uint8_t rspPkt10[] = {0x01, 0x00, 0x0A, 0x00, 0x00, 0x01};
static uint8_t rspPkt11[] = {0x01, 0x00, 0x0A, 0x00, 0x00, 0x02};
static uint8_t rspPkt12[] = {0x0B, 0x0B, 0x01, 0x00, 0x01, 0x00};
static uint8_t rspPkt13[] = {0x0B, 0x0B, 0xFF, 0x00, 0x04, 0x00};
static uint8_t rspPkt14[] = {0x0B, 0x0B, 0x00, 0x00, 0x04, 0x00};
static uint8_t rspPktLen1[] = {0x01, 0x00, 0x08, 0x00, 0x00, 0x01};
static uint8_t rspPktLen2[] = {0x01, 0x00, 0x08, 0x00, 0x00, 0x11};
static uint8_t rspPktType[] = {0x01, 0x00, 0x08, 0x00, 0x00, 0x02};
static uint8_t rspPktCtrl[] = {0x09, 0x0B, 0x10, 0x00};

class UT_SSAP_SERVER_READ : public testing::Test {
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
    propertyParam->operation.operationValue = 1;
    SSAP_CacheProperty(propertyParam);
    SDF_MemFree(propertyParam);
    SSAP_ParamAddDescriptor_S *descParam =
        (SSAP_ParamAddDescriptor_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddDescriptor_S) + 1);
    descParam->type = DESC_TYPE_PROPERTY_INSTRUCTION;
    descParam->val.len = 1;
    descParam->val.value[0] = 0xEE;
    descParam->operation.operationValue = 1;
    SSAP_CacheDescriptor(descParam);
    SDF_MemFree(descParam);
    SSAP_StartService(NULL);

    SSAP_ParamAddService_S *serviceParam2 = (SSAP_ParamAddService_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddService_S));
    serviceParam2->serviceType = ITEM_TYPE_STD_PRIMARY_SERVICE;
    (void)memcpy_s(&serviceParam2->uuid, sizeof(NLSTK_SsapUuid_S), &g_uuid3, sizeof(NLSTK_SsapUuid_S));
    SSAP_CacheService(serviceParam2);
    SDF_MemFree(serviceParam2);
    SSAP_StartService(NULL);
}

static void AddServiceCustom()
{
    SSAP_ParamAddService_S *serviceParam = (SSAP_ParamAddService_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddService_S));
    serviceParam->serviceType = ITEM_TYPE_STD_PRIMARY_SERVICE;
    (void)memcpy_s(&serviceParam->uuid, sizeof(NLSTK_SsapUuid_S), &g_uuid1, sizeof(NLSTK_SsapUuid_S));
    SSAP_CacheService(serviceParam);
    SDF_MemFree(serviceParam);
    SSAP_ParamAddProperty_S *propertyParam =
        (SSAP_ParamAddProperty_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddProperty_S) + 1);
    (void)memcpy_s(&propertyParam->uuid, sizeof(NLSTK_SsapUuid_S), &g_uuid3, sizeof(NLSTK_SsapUuid_S));
    propertyParam->val.len = 1;
    propertyParam->val.value[0] = 0xFF;
    propertyParam->operation.operationValue = 1;
    SSAP_CacheProperty(propertyParam);
    SDF_MemFree(propertyParam);
    SSAP_ParamAddDescriptor_S *descParam =
        (SSAP_ParamAddDescriptor_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddDescriptor_S) + 1);
    descParam->type = DESC_TYPE_PROPERTY_INSTRUCTION;
    descParam->val.len = 1;
    descParam->val.value[0] = 0xEE;
    descParam->operation.operationValue = 1;
    SSAP_CacheDescriptor(descParam);
    SDF_MemFree(descParam);
    SSAP_StartService(NULL);
}

static void AddServiceMulti()
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
    propertyParam->operation.operationValue = 1;
    SSAP_CacheProperty(propertyParam);
    SDF_MemFree(propertyParam);
    SSAP_ParamAddDescriptor_S *descParam =
        (SSAP_ParamAddDescriptor_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddDescriptor_S) + 1);
    descParam->type = DESC_TYPE_PROPERTY_INSTRUCTION;
    descParam->val.len = 1;
    descParam->val.value[0] = 0xEE;
    descParam->operation.operationValue = 1;
    SSAP_CacheDescriptor(descParam);
    SDF_MemFree(descParam);
    SSAP_ParamAddProperty_S *propertyParam1 =
        (SSAP_ParamAddProperty_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddProperty_S) + 1);
    (void)memcpy_s(&propertyParam1->uuid, sizeof(NLSTK_SsapUuid_S), &g_uuid2, sizeof(NLSTK_SsapUuid_S));
    propertyParam1->val.len = 1;
    propertyParam1->val.value[0] = 0x66;
    propertyParam1->operation.operationValue = 1;
    SSAP_CacheProperty(propertyParam1);
    SDF_MemFree(propertyParam1);
    SSAP_StartService(NULL);
}

static void AddServiceMultiError()
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
    propertyParam->operation.operationValue = 1;
    SSAP_CacheProperty(propertyParam);
    SDF_MemFree(propertyParam);
    SSAP_ParamAddDescriptor_S *descParam =
        (SSAP_ParamAddDescriptor_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddDescriptor_S) + 1);
    descParam->type = DESC_TYPE_PROPERTY_INSTRUCTION;
    descParam->val.len = 1;
    descParam->val.value[0] = 0xEE;
    descParam->operation.operationValue = 1;
    SSAP_CacheDescriptor(descParam);
    SDF_MemFree(descParam);
    SSAP_ParamAddProperty_S *propertyParam1 =
        (SSAP_ParamAddProperty_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddProperty_S) + 1);
    (void)memcpy_s(&propertyParam1->uuid, sizeof(NLSTK_SsapUuid_S), &g_uuid2, sizeof(NLSTK_SsapUuid_S));
    propertyParam1->val.len = 1;
    propertyParam1->val.value[0] = 0x66;
    SSAP_CacheProperty(propertyParam1);
    SDF_MemFree(propertyParam1);
    SSAP_StartService(NULL);
}

static void AddServiceClientCfg()
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
    propertyParam->operation.operationValue = 1;
    SSAP_CacheProperty(propertyParam);
    SDF_MemFree(propertyParam);
    SSAP_ParamAddDescriptor_S *descParam =
        (SSAP_ParamAddDescriptor_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddDescriptor_S) + 1);
    descParam->type = DESC_TYPE_CLIENT_CONFIG;
    descParam->val.len = 1;
    descParam->val.value[0] = 0xEE;
    descParam->operation.operationValue = 1;
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

TEST_F(UT_SSAP_SERVER_READ, READ_REQ_PROPERTY_SUCCESS)
{
    AddService();

    SSAP_Link_S *link = CreateLink();
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(reqPkt1));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(reqPkt1));
    (void)memcpy_s(tmpBuf, sizeof(reqPkt1), reqPkt1, sizeof(reqPkt1));
    SSAPS_ReadReqHandle(link, tmp);
    SDF_BuffFree(tmp);

    DeleteLink();
    EXPECT_EQ(memcmp(g_buffCache, rspPkt1, g_buffLen), 0);
}

TEST_F(UT_SSAP_SERVER_READ, READ_REQ_DISCRIPTOR_SUCCESS)
{
    AddService();

    SSAP_Link_S *link = CreateLink();
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(reqPkt2));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(reqPkt2));
    (void)memcpy_s(tmpBuf, sizeof(reqPkt2), reqPkt2, sizeof(reqPkt2));
    SSAPS_ReadReqHandle(link, tmp);
    SDF_BuffFree(tmp);

    DeleteLink();
    EXPECT_EQ(memcmp(g_buffCache, rspPkt2, g_buffLen), 0);
}

TEST_F(UT_SSAP_SERVER_READ, READ_REQ_PROPERTY_FAILED)
{
    AddService();

    SSAP_Link_S *link = CreateLink();
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(reqPkt3));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(reqPkt3));
    (void)memcpy_s(tmpBuf, sizeof(reqPkt3), reqPkt3, sizeof(reqPkt3));
    SSAPS_ReadReqHandle(link, tmp);
    SDF_BuffFree(tmp);

    DeleteLink();
    EXPECT_EQ(memcmp(g_buffCache, rspPkt3, g_buffLen), 0);
}

TEST_F(UT_SSAP_SERVER_READ, READ_BY_UUID_REQ_PROPERTY_SINGLE_SUCCESS)
{
    AddService();

    SSAP_Link_S *link = CreateLink();
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(reqPkt4));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(reqPkt4));
    (void)memcpy_s(tmpBuf, sizeof(reqPkt4), reqPkt4, sizeof(reqPkt4));
    SSAPS_ReadByUuidReqHandle(link, tmp);
    SDF_BuffFree(tmp);

    DeleteLink();
    EXPECT_EQ(g_buffLen, sizeof(rspPkt4));
    EXPECT_EQ(memcmp(g_buffCache, rspPkt4, g_buffLen), 0);
}

TEST_F(UT_SSAP_SERVER_READ, READ_BY_UUID_REQ_DISCRIPTOR_SINGLE_SUCCESS)
{
    AddService();

    SSAP_Link_S *link = CreateLink();
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(reqPkt5));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(reqPkt5));
    (void)memcpy_s(tmpBuf, sizeof(reqPkt5), reqPkt5, sizeof(reqPkt5));
    SSAPS_ReadByUuidReqHandle(link, tmp);
    SDF_BuffFree(tmp);

    DeleteLink();
    EXPECT_EQ(g_buffLen, sizeof(rspPkt5));
    EXPECT_EQ(memcmp(g_buffCache, rspPkt5, g_buffLen), 0);
}

TEST_F(UT_SSAP_SERVER_READ, READ_BY_UUID_REQ_PROPERTY_SINGLE_FAILED)
{
    AddService();

    SSAP_Link_S *link = CreateLink();
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(reqPkt6));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(reqPkt6));
    (void)memcpy_s(tmpBuf, sizeof(reqPkt6), reqPkt6, sizeof(reqPkt6));
    SSAPS_ReadByUuidReqHandle(link, tmp);
    SDF_BuffFree(tmp);

    DeleteLink();
    EXPECT_EQ(g_buffLen, sizeof(rspPkt6));
    EXPECT_EQ(memcmp(g_buffCache, rspPkt6, g_buffLen), 0);
}

TEST_F(UT_SSAP_SERVER_READ, READ_BY_CUSTOM_UUID_REQ_PROPERTY_SINGLE_SUCCESS)
{
    AddServiceCustom();

    SSAP_Link_S *link = CreateLink();
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(reqPkt7));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(reqPkt7));
    (void)memcpy_s(tmpBuf, sizeof(reqPkt7), reqPkt7, sizeof(reqPkt7));
    SSAPS_ReadByUuidReqHandle(link, tmp);
    SDF_BuffFree(tmp);

    DeleteLink();
    EXPECT_EQ(g_buffLen, sizeof(rspPkt7));
    EXPECT_EQ(memcmp(g_buffCache, rspPkt7, g_buffLen), 0);
}

TEST_F(UT_SSAP_SERVER_READ, READ_BY_UUID_REQ_PROPERTY_MULTI_SUCCESS)
{
    AddServiceMulti();

    SSAP_Link_S *link = CreateLink();
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(reqPkt8));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(reqPkt8));
    (void)memcpy_s(tmpBuf, sizeof(reqPkt8), reqPkt8, sizeof(reqPkt8));
    SSAPS_ReadByUuidReqHandle(link, tmp);
    SDF_BuffFree(tmp);

    DeleteLink();
    EXPECT_EQ(g_buffLen, sizeof(rspPkt8));
    EXPECT_EQ(memcmp(g_buffCache, rspPkt8, g_buffLen), 0);
}

TEST_F(UT_SSAP_SERVER_READ, READ_BY_UUID_REQ_PROPERTY_MULTI_FAILED)
{
    AddServiceMultiError();

    SSAP_Link_S *link = CreateLink();
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(reqPkt8));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(reqPkt8));
    (void)memcpy_s(tmpBuf, sizeof(reqPkt8), reqPkt8, sizeof(reqPkt8));
    SSAPS_ReadByUuidReqHandle(link, tmp);
    SDF_BuffFree(tmp);

    DeleteLink();
    EXPECT_EQ(g_buffLen, sizeof(rspPkt9));
    EXPECT_EQ(memcmp(g_buffCache, rspPkt9, g_buffLen), 0);
}

// 读属性配置描述符
TEST_F(UT_SSAP_SERVER_READ, READ_REQ_PROPERTY_CLIENT_CFG_001)
{
    AddServiceClientCfg();

    SSAP_Link_S *link = CreateLink();
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(reqPkt9));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(reqPkt9));
    (void)memcpy_s(tmpBuf, sizeof(reqPkt9), reqPkt9, sizeof(reqPkt9));
    SSAPS_ReadReqHandle(link, tmp);
    SDF_BuffFree(tmp);

    DeleteLink();
    EXPECT_EQ(g_buffLen, sizeof(rspPkt2));
    EXPECT_EQ(memcmp(g_buffCache, rspPkt2, g_buffLen), 0);
}

// 读属性，报文长度短
TEST_F(UT_SSAP_SERVER_READ, READ_REQ_PROPERTY_LEN_001)
{
    AddServiceClientCfg();

    SSAP_Link_S *link = CreateLink();
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(reqPktLen1));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(reqPktLen1));
    (void)memcpy_s(tmpBuf, sizeof(reqPktLen1), reqPktLen1, sizeof(reqPktLen1));
    SSAPS_ReadReqHandle(link, tmp);
    SDF_BuffFree(tmp);

    DeleteLink();
    EXPECT_EQ(g_buffLen, sizeof(rspPktLen1));
    EXPECT_EQ(memcmp(g_buffCache, rspPktLen1, g_buffLen), 0);
}

// 读属性，报文长度短
TEST_F(UT_SSAP_SERVER_READ, READ_REQ_PROPERTY_LEN_002)
{
    AddServiceClientCfg();

    SSAP_Link_S *link = CreateLink();
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(reqPktLen2));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(reqPktLen2));
    (void)memcpy_s(tmpBuf, sizeof(reqPktLen2), reqPktLen2, sizeof(reqPktLen2));
    SSAPS_ReadReqHandle(link, tmp);
    SDF_BuffFree(tmp);

    DeleteLink();
    EXPECT_EQ(g_buffLen, sizeof(rspPktLen2));
    EXPECT_EQ(memcmp(g_buffCache, rspPktLen2, g_buffLen), 0);
}

// 读属性，数据类型错误
TEST_F(UT_SSAP_SERVER_READ, READ_REQ_PROPERTY_TYPE)
{
    AddServiceClientCfg();

    SSAP_Link_S *link = CreateLink();
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(reqPktType));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(reqPktType));
    (void)memcpy_s(tmpBuf, sizeof(reqPktType), reqPktType, sizeof(reqPktType));
    SSAPS_ReadReqHandle(link, tmp);
    SDF_BuffFree(tmp);

    DeleteLink();
    EXPECT_EQ(g_buffLen, sizeof(rspPktType));
    EXPECT_EQ(memcmp(g_buffCache, rspPktType, g_buffLen), 0);
}

// 读属性，控制码错误
TEST_F(UT_SSAP_SERVER_READ, READ_REQ_PROPERTY_CTRL_FAILED)
{
    AddServiceClientCfg();

    SSAP_Link_S *link = CreateLink();
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(reqPkt10));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(reqPkt10));
    (void)memcpy_s(tmpBuf, sizeof(reqPkt10), reqPkt10, sizeof(reqPkt10));
    SSAPS_ReadReqHandle(link, tmp);
    SDF_BuffFree(tmp);

    DeleteLink();
    EXPECT_EQ(g_buffLen, sizeof(rspPktCtrl));
    EXPECT_EQ(memcmp(g_buffCache, rspPktCtrl, g_buffLen), 0);
}

// 读属性，报文长度错误
TEST_F(UT_SSAP_SERVER_READ, READ_BY_UUID_REQ_PROPERTY_FAILED_001)
{

    SSAP_Link_S *link = CreateLink();
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(reqPkt11));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(reqPkt11));
    (void)memcpy_s(tmpBuf, sizeof(reqPkt11), reqPkt11, sizeof(reqPkt11));
    SSAPS_ReadByUuidReqHandle(link, tmp);
    SDF_BuffFree(tmp);

    DeleteLink();
    EXPECT_EQ(g_buffLen, sizeof(rspPkt10));
    EXPECT_EQ(memcmp(g_buffCache, rspPkt10, g_buffLen), 0);
}

// 读属性，读取数据类型错误
TEST_F(UT_SSAP_SERVER_READ, READ_BY_UUID_REQ_PROPERTY_FAILED_002)
{

    SSAP_Link_S *link = CreateLink();
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(reqPkt12));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(reqPkt12));
    (void)memcpy_s(tmpBuf, sizeof(reqPkt12), reqPkt12, sizeof(reqPkt12));
    SSAPS_ReadByUuidReqHandle(link, tmp);
    SDF_BuffFree(tmp);

    DeleteLink();
    EXPECT_EQ(g_buffLen, sizeof(rspPkt11));
    EXPECT_EQ(memcmp(g_buffCache, rspPkt11, g_buffLen), 0);
}

// 读属性，数据长度非法
TEST_F(UT_SSAP_SERVER_READ, READ_BY_UUID_REQ_PROPERTY_FAILED_003)
{

    SSAP_Link_S *link = CreateLink();
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(reqPkt13));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(reqPkt13));
    (void)memcpy_s(tmpBuf, sizeof(reqPkt13), reqPkt13, sizeof(reqPkt13));
    SSAPS_ReadByUuidReqHandle(link, tmp);
    SDF_BuffFree(tmp);

    DeleteLink();
    EXPECT_EQ(g_buffLen, sizeof(rspPkt12));
    EXPECT_EQ(memcmp(g_buffCache, rspPkt12, g_buffLen), 0);
}

// 读属性，开始句柄大于结束句柄
TEST_F(UT_SSAP_SERVER_READ, READ_BY_UUID_REQ_PROPERTY_FAILED_004)
{

    SSAP_Link_S *link = CreateLink();
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(reqPkt14));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(reqPkt14));
    (void)memcpy_s(tmpBuf, sizeof(reqPkt14), reqPkt14, sizeof(reqPkt14));
    SSAPS_ReadByUuidReqHandle(link, tmp);
    SDF_BuffFree(tmp);

    DeleteLink();
    EXPECT_EQ(g_buffLen, sizeof(rspPkt13));
    EXPECT_EQ(memcmp(g_buffCache, rspPkt13, g_buffLen), 0);
}

// 读属性，开始句柄为0
TEST_F(UT_SSAP_SERVER_READ, READ_BY_UUID_REQ_PROPERTY_FAILED_005)
{

    SSAP_Link_S *link = CreateLink();
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(reqPkt15));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(reqPkt15));
    (void)memcpy_s(tmpBuf, sizeof(reqPkt15), reqPkt15, sizeof(reqPkt15));
    SSAPS_ReadByUuidReqHandle(link, tmp);
    SDF_BuffFree(tmp);

    DeleteLink();
    EXPECT_EQ(g_buffLen, sizeof(rspPkt14));
    EXPECT_EQ(memcmp(g_buffCache, rspPkt14, g_buffLen), 0);
}