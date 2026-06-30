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

#include "cpfwk_log.h"
#include "sdf_string.h"
#define TEST_MAX_BUF_CACHE 1024
static NLSTK_SsapUuid_S g_uuid1 = {.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02}};
static NLSTK_SsapUuid_S g_uuid2 = {.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x02, 0x03}};
static SLE_Addr_S g_addr = {.type = PUBLIC_ADDRESS, .addr = {0x03, 0x03, 0x04, 0x04, 0x05, 0x05}};
static uint16_t g_lcid = 1;

static uint8_t g_buffCache[TEST_MAX_BUF_CACHE] = {0};
static uint8_t g_buffLen = 0;

static uint8_t reqPktMethod[] = {0x13, 0x03, 0x11};
static uint8_t reqPktMethod2[] = {0x13, 0x02, 0x11, 0x00};
static uint8_t reqPktMethod3[] = {0x13, 0x03, 0x11, 0x00};

static uint8_t rspPktErr[] = {0x01, 0x00, 0x13, 0x00, 0x00, 0x01};
static uint8_t rspPktErr2[] = {0x01, 0x00, 0x13, 0x11, 0x00, 0x10};
static uint8_t rspPktErr3[] = {0x01, 0x00, 0x13, 0x11, 0x00, 0x0C};
static uint8_t rspPktErr4[] = {0x01, 0x00, 0x13, 0x11, 0x00, 0x04};

class UT_SSAPS_SERVER : public testing::Test {
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

// 调用方法，报文长度错误
TEST_F(UT_SSAPS_SERVER, CALL_METHOD_REQ_001)
{
    AddServiceClientCfg();

    SSAP_Link_S *link = CreateLink();
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(reqPktMethod));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(reqPktMethod));
    (void)memcpy_s(tmpBuf, sizeof(reqPktMethod), reqPktMethod, sizeof(reqPktMethod));
    SSAPS_MethodReqHandle(link, tmp);
    SDF_BuffFree(tmp);

    DeleteLink();
    EXPECT_EQ(g_buffLen, sizeof(rspPktErr));
    EXPECT_EQ(memcmp(g_buffCache, rspPktErr, g_buffLen), 0);
}

// 调用方法，非完整包
TEST_F(UT_SSAPS_SERVER, CALL_METHOD_REQ_002)
{
    AddServiceClientCfg();

    SSAP_Link_S *link = CreateLink();
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(reqPktMethod2));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(reqPktMethod2));
    (void)memcpy_s(tmpBuf, sizeof(reqPktMethod2), reqPktMethod2, sizeof(reqPktMethod2));
    SSAPS_MethodReqHandle(link, tmp);
    SDF_BuffFree(tmp);

    DeleteLink();
    EXPECT_EQ(g_buffLen, sizeof(rspPktErr2));
    EXPECT_EQ(memcmp(g_buffCache, rspPktErr2, g_buffLen), 0);
}

// 调用方法，不存在该条目
TEST_F(UT_SSAPS_SERVER, CALL_METHOD_REQ_003)
{
    AddServiceClientCfg();

    SSAP_Link_S *link = CreateLink();
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(reqPktMethod3));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(reqPktMethod3));
    (void)memcpy_s(tmpBuf, sizeof(reqPktMethod3), reqPktMethod3, sizeof(reqPktMethod3));
    SSAPS_MethodReqHandle(link, tmp);
    SDF_BuffFree(tmp);

    DeleteLink();
    EXPECT_EQ(g_buffLen, sizeof(rspPktErr3));
    EXPECT_EQ(memcmp(g_buffCache, rspPktErr3, g_buffLen), 0);
}

// 调用方法，不存在该条目
TEST_F(UT_SSAPS_SERVER, CALL_METHOD_REQ_004)
{
    SSAP_Link_S *link = CreateLink();
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(reqPktMethod3));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(reqPktMethod3));
    (void)memcpy_s(tmpBuf, sizeof(reqPktMethod3), reqPktMethod3, sizeof(reqPktMethod3));
    SSAPS_MethodReqHandle(link, tmp);
    SDF_BuffFree(tmp);

    DeleteLink();
    EXPECT_EQ(g_buffLen, sizeof(rspPktErr4));
    EXPECT_EQ(memcmp(g_buffCache, rspPktErr4, g_buffLen), 0);
}