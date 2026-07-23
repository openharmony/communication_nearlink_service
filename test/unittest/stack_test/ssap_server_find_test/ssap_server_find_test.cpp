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
#include "gmock/gmock.h"
#include "securec.h"
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <iostream>

#include "ssaps_server.h"
#include "ssapc_client.h"
#include "ssaps_server_api.h"
#include "sdf_buff.h"
#include "sdf_mem.h"
#include "ssaps_service.h"
#include "ssap_manager.h"
#include "ssap_utils.h"
#include "ssaps_service_param.h"
#include "cpfwk_log.h"
#include "sdf_string.h"

#define TEST_SERVICE_SIZE 2

#define TEST_MAX_BUF_CACHE 1024

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

static uint8_t g_buffCache[TEST_MAX_BUF_CACHE] = {0};
static uint8_t g_buffLen = 0;
static bool isSendRsp = false;
static SLE_Addr_S g_addr = {.type = PUBLIC_ADDRESS, .addr = {0x03, 0x03, 0x04, 0x04, 0x05, 0x05}};
static uint16_t g_lcid = 1;
static DTAP_Data_Info_S g_dtapDataInfo = {.lcid = g_lcid};

static NLSTK_SsapUuid_S g_uuid1 = {.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02}};
static NLSTK_SsapUuid_S g_uuid2 = {.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x02, 0x03}};
static NLSTK_SsapUuid_S g_uuid3 = {.uuid = {0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D,
    0x0E, 0x0F, 0x10, 0x11, 0x12}};
static NLSTK_SsapUuid_S g_uuid4 = {.uuid = {0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D,
    0x0E, 0x0F, 0x10, 0x12, 0x13}}; // custome service
static NLSTK_SsapUuid_S g_uuid5 = {.uuid = {0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D,
    0x0E, 0x0F, 0x10, 0x13, 0x14}}; // custome property


// find,op-1,ctrl首要服务-1,handle-2,handle-2
static uint8_t reqPkt1[] = {0x04, 0x01, 0x01, 0x00, 0xFF, 0x00};
// find回复,op-1,ctrl标准模式-1,{开始句柄-2,结束句柄-2,uuid-2,member-1}
static uint8_t rspPkt1[] = {0x05, 0x03, 0x10, 0x00, 0x11, 0x00, 0x02, 0x01, 0x02};

// find,op-1,ctrl属性-1,handle-2,handle-2
static uint8_t reqPkt2[] = {0x04, 0x03, 0x01, 0x00, 0xFF, 0x00};
// find回复,op-1,ctrl标准模式-1,{开始句柄-2,uuid-2,operation-4,count-1,descriptor-x}
static uint8_t rspPkt2[] = {0x05, 0x03, 0x11, 0x00, 0x03, 0x02, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01};

// find,op-1,ctrl首要服务混合模式-1,handle-2,handle-2
static uint8_t reqPkt3[] = {0x04, 0x11, 0x01, 0x00, 0xFF, 0x00};
// find回复,op-1,ctrl混合模式-1,{指示-1,{开始句柄-2,结束句柄-2,uuid-2,member-1},...},
// {指示-1,{开始句柄-2,结束句柄-2,uuid-16,member-1},...}
static uint8_t rspPkt3[] = {0x05, 0x0B, 0x01, 0x10, 0x00, 0x11, 0x00, 0x02, 0x01, 0x02,
    0x81, 0x12, 0x00, 0x12, 0x00, 0x12, 0x11, 0x10, 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A,
    0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x00};

// find,op-1,ctrl属性混合模式-1,handle-2,handle-2,uuid服务-2
static uint8_t reqPkt4[] = {0x06, 0x13, 0x01, 0x00, 0xFF, 0x00, 0x02, 0x01};
// find回复,op-1,ctrl标准模式-1,{指示-1,{开始句柄-2,uuid-2,operation-4,count-1,descriptor-x},...}
// {指示-1,{开始句柄-2,uuid-2,operation-4,count-1,descriptor-x},...}
static uint8_t rspPkt4[] = {0x07, 0x0B, 0x01, 0x11, 0x00, 0x03, 0x02, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x01, 0x80};

class UT_SSAP_SERVER_FIND : public testing::Test {
protected:
    void SetUp() override
    {
        SSAP_ServerInit();
        SSAP_LinkInit();
    }

    void TearDown() override
    {
        SSAP_LinkDeInit();
        SSAP_ServerDeInit();
    }
};

static void AddService()
{
    CP_LOG_INFO("[TEST] enter AddService");
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
    SSAP_CacheProperty(propertyParam);
    SDF_MemFree(propertyParam);
    SSAP_ParamAddDescriptor_S *descParam =
        (SSAP_ParamAddDescriptor_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddDescriptor_S) + 1);
    descParam->type = DESC_TYPE_PROPERTY_INSTRUCTION;
    descParam->val.len = 1;
    descParam->val.value[0] = 0xEE;
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

static void AddCusServiceWithCusProp()
{
    CP_LOG_INFO("[TEST] enter AddCusServiceWithCusProp");
    SSAP_ParamAddService_S *serviceParam = (SSAP_ParamAddService_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddService_S));
    serviceParam->serviceType = ITEM_TYPE_VENDOR_PRIMARY_SERVICE;
    (void)memcpy_s(&serviceParam->uuid, sizeof(NLSTK_SsapUuid_S), &g_uuid4, sizeof(NLSTK_SsapUuid_S));
    SSAP_CacheService(serviceParam);
    SDF_MemFree(serviceParam);
    SSAP_ParamAddProperty_S *propertyParam =
        (SSAP_ParamAddProperty_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddProperty_S) + 1);
    (void)memcpy_s(&propertyParam->uuid, sizeof(NLSTK_SsapUuid_S), &g_uuid5, sizeof(NLSTK_SsapUuid_S));
    propertyParam->val.len = 1;
    propertyParam->val.value[0] = 0xFF;
    SSAP_CacheProperty(propertyParam);
    SDF_MemFree(propertyParam);
    SSAP_ParamAddDescriptor_S *descParam =
        (SSAP_ParamAddDescriptor_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddDescriptor_S) + 1);
    descParam->type = DESC_TYPE_PROPERTY_INSTRUCTION;
    descParam->val.len = 1;
    descParam->val.value[0] = 0xEE;
    SSAP_CacheDescriptor(descParam);
    SDF_MemFree(descParam);
    SSAP_StartService(NULL);
}

static void MockSendCb(SSAP_Link *link, SDF_Buff_S *buff, uint8_t opcode)
{
    CP_LOG_INFO("[TEST] enter MockSendCb");
    isSendRsp = true;
    g_buffLen = SDF_DataLenGet(buff);
    (void)memcpy_s(g_buffCache, TEST_MAX_BUF_CACHE, SDF_DataOffset(buff), SDF_DataLenGet(buff));
    uint8_t type = SSAP_GetOpcodeType(opcode);
    if ((type & SSAP_REPLY_MASK) == 0) {
        SDF_BuffFree(buff);
    } else {
        SSAP_LinkSetTask(link, buff, opcode);
    }
    CP_LOG_INFO("[UT_SSAP_SERVER_FIND] MockSendCb test dtap data send: %s.", SDF_GET_UINT8_STR(g_buffCache, g_buffLen));
}

static SSAP_Link* CreateLink()
{
    return SSAP_CreateSsapLink(&g_addr, g_lcid, MockSendCb);
}

static void DeleteLink()
{
    SSAP_DeleteSsapLinkByAddr(&g_addr);
}

TEST_F(UT_SSAP_SERVER_FIND, ADD_SERVICE)
{
    CP_LOG_INFO("[UT_SSAP_SERVER_FIND] enter TEST_F ADD_SERVICE");
    AddService();
    SDF_Vector_S *services = SSAPS_GetServices();
    EXPECT_EQ(services->size, TEST_SERVICE_SIZE);
}

TEST_F(UT_SSAP_SERVER_FIND, FIND_RSP_PRIMARY_SERVICE)
{
    AddService();

    (void)CreateLink();
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(reqPkt1));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(reqPkt1));
    (void)memcpy_s(tmpBuf, sizeof(reqPkt1), reqPkt1, sizeof(reqPkt1));
    SSAP_Recv(&g_dtapDataInfo, tmp);
    SDF_BuffFree(tmp);
    DeleteLink();

    EXPECT_EQ(g_buffLen, sizeof(rspPkt1));
    EXPECT_EQ(memcmp(g_buffCache, rspPkt1, g_buffLen), 0);
}

TEST_F(UT_SSAP_SERVER_FIND, FIND_RSP_PROPERTY)
{
    AddService();

    (void)CreateLink();
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(reqPkt2));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(reqPkt2));
   (void)memcpy_s(tmpBuf, sizeof(reqPkt2), reqPkt2, sizeof(reqPkt2));
    SSAP_Recv(&g_dtapDataInfo, tmp);
    SDF_BuffFree(tmp);
    DeleteLink();

    EXPECT_EQ(g_buffLen, sizeof(rspPkt2));
    EXPECT_EQ(memcmp(g_buffCache, rspPkt2, g_buffLen), 0);
}

TEST_F(UT_SSAP_SERVER_FIND, FIND_RSP_PRIMARY_SERVICE_MIX)
{
    AddService();

    (void)CreateLink();
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(reqPkt3));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(reqPkt3));
    (void)memcpy_s(tmpBuf, sizeof(reqPkt3), reqPkt3, sizeof(reqPkt3));
    SSAP_Recv(&g_dtapDataInfo, tmp);
    SDF_BuffFree(tmp);
    DeleteLink();

    EXPECT_EQ(g_buffLen, sizeof(rspPkt3));
    EXPECT_EQ(memcmp(g_buffCache, rspPkt3, g_buffLen), 0);
}

TEST_F(UT_SSAP_SERVER_FIND, FIND_RSP_PROPERTY_MIX_UUID)
{
    AddService();

    (void)CreateLink();
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(reqPkt4));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(reqPkt4));
    (void)memcpy_s(tmpBuf, sizeof(reqPkt4), reqPkt4, sizeof(reqPkt4));
    SSAP_Recv(&g_dtapDataInfo, tmp);
    SDF_BuffFree(tmp);
    DeleteLink();

    EXPECT_EQ(g_buffLen, sizeof(rspPkt4));
    EXPECT_EQ(memcmp(g_buffCache, rspPkt4, g_buffLen), 0);
}

static void Test_SSAP_RecvReq(uint8_t *req, size_t reqLen)
{
    if (req == NULL || reqLen == 0) {
        CP_LOG_ERROR("[UT_SSAP_SERVER_FIND] Test_SSAP_RecvReq input param invalid!");
        return;
    }
    isSendRsp = false;
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(reqLen);
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, reqLen);
    (void)memcpy_s(tmpBuf, reqLen, req, reqLen);
    CP_LOG_INFO("[UT_SSAP_SERVER_FIND] Test_SSAP_RecvReq recv pkt: %s.", SDF_GET_UINT8_STR(tmpBuf, reqLen));
    SSAP_Recv(&g_dtapDataInfo, tmp);
    SDF_BuffFree(tmp);
}

static bool Test_SSAP_CompareLastSendPkt(uint8_t *buf, uint32_t size)
{
    if (isSendRsp == false) {
        CP_LOG_ERROR("[TEST] TEST_DTAP_CompareLastPkt isSendRsp false");
        return false;
    }
    if (g_buffLen == size && memcmp(g_buffCache, buf, g_buffLen) == 0) {
        CP_LOG_INFO("[TEST] TEST_DTAP_CompareLastPkt true, last pkt: %s",
            SDF_GET_UINT8_STR(g_buffCache, g_buffLen));
        return true;
    }
    CP_LOG_ERROR("[TEST] TEST_DTAP_CompareLastPkt false, last pkt: %s, target pkt: %s",
        SDF_GET_UINT8_STR(g_buffCache, g_buffLen), SDF_GET_UINT8_STR(buf, size));
    return false;
}

static uint8_t failReq1[] = {0x04, 0x03};   // len too short
static uint8_t failReq2[] = {0x06, 0x03};   // len too short
static uint8_t failReq3[] = {0x04, 0x03, 0xFF, 0x00, 0x01, 0x00};   // startHandle > endHandle
static uint8_t failReq4[] = {0x04, 0x03, 0x00, 0x00, 0x01, 0x00};   // startHandle == 0
static uint8_t failReq5[] = {0x06, 0x13, 0x01, 0x00, 0xFF, 0x00, 0x03, 0x02, 0x01}; // uuidLen error
static uint8_t failReq6[] = {0x04, 0x05, 0x01, 0x00, 0xFF, 0x00};   // not support find event
static uint8_t failReq7[] = {0x04, 0x06, 0x01, 0x00, 0xFF, 0x00};   // not support find other
static uint8_t failReq8[] = {0x04, 0x19, 0x01, 0x00, 0xFF, 0x00};   // itemType == RFU
static uint8_t failReq9[] = {0x04, 0x21, 0x01, 0x00, 0xFF, 0x00};   // rspMode == FIND_RSP_MODE_MULTI_RSP
static uint8_t errRsp1[] = {0x01, 0x00, 0x04, 0x00, 0x00, 0x01};  // SSAP_ERRCODE_INVALID_PDU
static uint8_t errRsp2[] = {0x01, 0x00, 0x06, 0x00, 0x00, 0x01};  // SSAP_ERRCODE_INVALID_PDU
static uint8_t errRsp3[] = {0x01, 0x00, 0x04, 0x00, 0x00, 0x0B};  // SSAP_ERRCODE_ITEM_INEXIST
static uint8_t errRsp4[] = {0x01, 0x00, 0x04, 0x00, 0x00, 0x02};  // SSAP_ERRCODE_UNSUPPORT_PDU
static uint8_t errRsp5[] = {0x01, 0x00, 0x04, 0xFF, 0x00, 0x04};  // SSAP_ERRCODE_INVALID_HANDLE
static uint8_t errRsp6[] = {0x01, 0x00, 0x04, 0x00, 0x00, 0x04};  // SSAP_ERRCODE_INVALID_HANDLE

// match SSAPS_FindReqHandle and SendFindRsp func
TEST_F(UT_SSAP_SERVER_FIND, FIND_RSP_STRUCTURE_FAIL_001)
{
    AddService();
    CreateLink();
    Test_SSAP_RecvReq(failReq1, sizeof(failReq1));
    EXPECT_TRUE(Test_SSAP_CompareLastSendPkt(errRsp1, sizeof(errRsp1)));

    Test_SSAP_RecvReq(failReq2, sizeof(failReq2));
    EXPECT_TRUE(Test_SSAP_CompareLastSendPkt(errRsp2, sizeof(errRsp2)));

    Test_SSAP_RecvReq(failReq3, sizeof(failReq3));
    EXPECT_TRUE(Test_SSAP_CompareLastSendPkt(errRsp5, sizeof(errRsp5)));

    Test_SSAP_RecvReq(failReq4, sizeof(failReq4));
    EXPECT_TRUE(Test_SSAP_CompareLastSendPkt(errRsp6, sizeof(errRsp6)));

    Test_SSAP_RecvReq(failReq5, sizeof(failReq5));
    EXPECT_TRUE(Test_SSAP_CompareLastSendPkt(errRsp2, sizeof(errRsp2)));

    Test_SSAP_RecvReq(failReq6, sizeof(failReq6));
    EXPECT_TRUE(Test_SSAP_CompareLastSendPkt(errRsp3, sizeof(errRsp3)));

    Test_SSAP_RecvReq(failReq7, sizeof(failReq7));
    EXPECT_TRUE(Test_SSAP_CompareLastSendPkt(errRsp4, sizeof(errRsp4)));

    Test_SSAP_RecvReq(failReq8, sizeof(failReq8));
    EXPECT_TRUE(Test_SSAP_CompareLastSendPkt(errRsp4, sizeof(errRsp4)));

    Test_SSAP_RecvReq(failReq9, sizeof(failReq9));
    EXPECT_TRUE(Test_SSAP_CompareLastSendPkt(errRsp4, sizeof(errRsp4)));

    DeleteLink();
}

static uint8_t failReq10[] = {0x06, 0x04, 0x01, 0x00, 0xFF, 0x00, 0x03, 0x02};   // FIND_STRUCTURE_TYPE_METHOD
static uint8_t failReq11[] = {0x04, 0x04, 0x01, 0x00, 0xFF, 0x00};   // findMethod->size == 0
static uint8_t failReq12[] = {0x04, 0x0C, 0x01, 0x00, 0xFF, 0x00};   // itemType == FIND_ITEM_TYPE_CUSTOMIZE
static uint8_t failReq13[] = {0x06, 0x04, 0x01, 0x00, 0xFF, 0x00, 0x02, 0x01};   // FindMethodListByService
static uint8_t errRsp7[] = {0x01, 0x00, 0x06, 0x01, 0x00, 0x0B};  // SSAP_ERRCODE_ITEM_INEXIST
static uint8_t errRsp8[] = {0x01, 0x00, 0x04, 0x01, 0x00, 0x0B};  // SSAP_ERRCODE_ITEM_INEXIST

// match SendFindMethodRsp func
TEST_F(UT_SSAP_SERVER_FIND, FIND_RSP_STRUCTURE_METHOD_FAIL)
{
    AddService();
    CreateLink();
    Test_SSAP_RecvReq(failReq10, sizeof(failReq10));    // uuidType != UUID_TYPE_SERVICE, uuidType != UUID_TYPE_METHOD
    EXPECT_TRUE(Test_SSAP_CompareLastSendPkt(errRsp7, sizeof(errRsp7)));

    Test_SSAP_RecvReq(failReq11, sizeof(failReq11));
    EXPECT_TRUE(Test_SSAP_CompareLastSendPkt(errRsp8, sizeof(errRsp8)));

    Test_SSAP_RecvReq(failReq12, sizeof(failReq12));
    EXPECT_TRUE(Test_SSAP_CompareLastSendPkt(errRsp8, sizeof(errRsp8)));

    Test_SSAP_RecvReq(failReq13, sizeof(failReq13));
    EXPECT_TRUE(Test_SSAP_CompareLastSendPkt(errRsp7, sizeof(errRsp7)));

    DeleteLink();
}

static uint8_t failReq14[] = {0x06, 0x03, 0xFF, 0x00, 0xFF, 0xFF, 0x03, 0x02};   // FIND_STRUCTURE_TYPE_PROPERTY
static uint8_t failReq15[] = {0x06, 0x0B, 0x01, 0x00, 0xFF, 0xFF, 0x14, 0x13, 0x10, 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A,
    0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03};   // itemType == FIND_ITEM_TYPE_CUSTOMIZE
static uint8_t errRsp9[] = {0x01, 0x00, 0x06, 0xFF, 0x00, 0x0B};  // SSAP_ERRCODE_ITEM_INEXIST
static uint8_t errRsp10[] = {0x07, 0x07};


// match SendFindPropertyRsp func
TEST_F(UT_SSAP_SERVER_FIND, FIND_RSP_STRUCTURE_PROPERTY_FAIL)
{
    CP_LOG_INFO("[UT_SSAP_SERVER_FIND] enter FIND_RSP_STRUCTURE_PROPERTY_FAIL");
    AddService();
    AddCusServiceWithCusProp();
    SSAP_Link *link = CreateLink();
    Test_SSAP_RecvReq(failReq14, sizeof(failReq14));    // findPropertys->size == 0
    EXPECT_TRUE(Test_SSAP_CompareLastSendPkt(errRsp9, sizeof(errRsp9)));

    link->mtu = SSAP_PDU_BASE_LEN + SSAP_FIND_PROPERTY_CUS_LEN - 1; // BuildPropertyPayload error
    Test_SSAP_RecvReq(failReq15, sizeof(failReq15));
    EXPECT_TRUE(Test_SSAP_CompareLastSendPkt(errRsp10, sizeof(errRsp10)));
    link->mtu = SSAP_STACK_MTU_MAX;

    DeleteLink();
}

static uint8_t failReq16[] = {0x06, 0x09, 0x01, 0x00, 0xFF, 0xFF, 0x14, 0x13, 0x10, 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A,
    0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03};
static uint8_t failReq17[] = {0x04, 0x01, 0xFF, 0x00, 0xFF, 0xFF};
static uint8_t failReq18[] = {0x06, 0x09, 0x01, 0x00, 0xFF, 0xFF, 0x13, 0x12, 0x10, 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A,
    0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03};
static uint8_t failReq19[] = {0x04, 0x11, 0x01, 0x00, 0xFF, 0xFF};
static uint8_t errRsp11[] = {0x01, 0x00, 0x04, 0xFF, 0x00, 0x0B};  // SSAP_ERRCODE_ITEM_INEXIST
static uint8_t errRsp12[] = {0x07, 0x07};
static uint8_t errRsp13[] = {0x05, 0x0B, 0x00, 0x80};   // 标准成员0个，自定义成员0个

// match SendFindPrimaryServiceRsp func
TEST_F(UT_SSAP_SERVER_FIND, FIND_RSP_STRUCTURE_PRIMARY_SERVICE_FAIL)
{
    CP_LOG_INFO("[UT_SSAP_SERVER_FIND] enter FIND_RSP_STRUCTURE_PRIMARY_SERVICE_FAIL");
    AddService();
    AddCusServiceWithCusProp();
    SSAP_Link *link = CreateLink();
    Test_SSAP_RecvReq(failReq16, sizeof(failReq16));    // isByUuid && GetUuidType(uuid) != UUID_TYPE_SERVICE
    EXPECT_TRUE(Test_SSAP_CompareLastSendPkt(errRsp7, sizeof(errRsp7)));

    Test_SSAP_RecvReq(failReq17, sizeof(failReq17));
    EXPECT_TRUE(Test_SSAP_CompareLastSendPkt(errRsp11, sizeof(errRsp11)));

    link->mtu = SSAP_PDU_BASE_LEN + SSAP_FIND_PRIMARY_SERVICE_CUS_LEN - 1;
    Test_SSAP_RecvReq(failReq18, sizeof(failReq18));    // mtu err
    EXPECT_TRUE(Test_SSAP_CompareLastSendPkt(errRsp12, sizeof(errRsp12)));
    link->mtu = SSAP_STACK_MTU_MAX;

    // BuildMixPrimaryServicePayload err
    link->mtu = SSAP_PDU_BASE_LEN + SSAP_FIND_INFO_INDICATION_LEN * 2 + SSAP_FIND_PRIMARY_SERVICE_BASE_LEN;
    Test_SSAP_RecvReq(failReq19, sizeof(failReq19));
    EXPECT_TRUE(Test_SSAP_CompareLastSendPkt(errRsp13, sizeof(errRsp13)));
    link->mtu = SSAP_STACK_MTU_MAX;

    DeleteLink();
}

// match ssaps_service.c
TEST_F(UT_SSAP_SERVER_FIND, SSAPS_SERVICE_FAIL)
{
    CP_LOG_INFO("[UT_SSAP_SERVER_FIND] enter SSAPS_SERVICE_FAIL");

    SSAP_ParamAddService_S *serviceParam = (SSAP_ParamAddService_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddService_S));
    serviceParam->serviceType = ITEM_TYPE_STD_PRIMARY_SERVICE;
    (void)memcpy_s(&serviceParam->uuid, sizeof(NLSTK_SsapUuid_S), &g_uuid1, sizeof(NLSTK_SsapUuid_S));
    SSAP_CacheService(serviceParam);
    SSAP_CacheService(serviceParam);    // g_tempService != NULL
    SDF_MemFree(serviceParam);

    SSAP_ParamAddProperty_S *propertyParam =
        (SSAP_ParamAddProperty_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddProperty_S) + 0);
    (void)memcpy_s(&propertyParam->uuid, sizeof(NLSTK_SsapUuid_S), &g_uuid2, sizeof(NLSTK_SsapUuid_S));
    propertyParam->val.len = 0;
    SSAP_CacheProperty(propertyParam);  // addProperty->val.len != 0
    SDF_MemFree(propertyParam);

    SSAP_ParamAddDescriptor_S *descParam =
        (SSAP_ParamAddDescriptor_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddDescriptor_S));
    descParam->type = DESC_TYPE_PROPERTY_INSTRUCTION;
    descParam->val.len = 0;
    SSAP_CacheDescriptor(descParam);    // addDescriptor->val.len != 0
    SDF_MemFree(descParam);

    SSAP_StartService(NULL);

    SSAP_ServerDeInit();
    SSAPS_CleanServiceCpcd(&g_addr);   // g_services == NULL
    SSAP_ServerInit();

    SSAP_Link *link = CreateLink();

    DeleteLink();
}

// match ssaps_service_param.c
TEST_F(UT_SSAP_SERVER_FIND, SSAPS_SERVICE_PARAM_FAIL)
{
    FreeService(NULL);
    FreeProperty(NULL);
    FreeDescriptor(NULL);
    FreeClientConfig(NULL);

    SSAP_ClientPropertyConfigDescriptor_S  *cpcd = (SSAP_ClientPropertyConfigDescriptor_S *)SDF_MemZalloc(
            sizeof(SSAP_ClientPropertyConfigDescriptor_S));
    ASSERT_NE(cpcd, NULL);
    uint16_t len = 1;
    cpcd->val = (SSAP_LengthValue_S *)SDF_MemZalloc(sizeof(SSAP_LengthValue_S) + len);
    if (cpcd->val != NULL) {
        cpcd->val->len = len;
    }
    FreeClientConfig(cpcd);

    SSAP_Service_S *ssapService = NULL;
    ssapService = SsapAllocServiceParam(NULL);
    EXPECT_EQ(ssapService, NULL);

    NLSTK_ServiceParam_S service = {};
    service.serviceStatement.descriptorNum = 0;
    ssapService = SsapAllocServiceParam(&service);
    EXPECT_EQ(ssapService, NULL);

    service.serviceStatement.descriptorNum = 1;
    service.serviceStatement.descriptors = NULL;
    ssapService = SsapAllocServiceParam(&service);  // serviceStatement->descriptors != NULL
    EXPECT_EQ(ssapService, NULL);

    service.serviceStatement.descriptorNum = 0;
    service.servicePropertyNum = 1;
    NLSTK_SsapServicePropertyParam_S property = {};
    service.property = &property;
    property.val.len = 0;
    ssapService = SsapAllocServiceParam(&service);  // propertyParm->val.len != 0
    EXPECT_NE(ssapService, NULL);
    FreeService(ssapService);
    ssapService = NULL;

    property.val.len = 0;
    NLSTK_SsapServiceDescriptorParam_S descriptors = {};
     NLSTK_SsapServiceMethodParam_S method = {};
    property.descriptorNum = 0;
    property.descriptors = &descriptors;
    service.serviceMethodNum = 0;
    service.method = &method;
    // propertyParm->descriptorNum > 0 && propertyParm->descriptors != NULL
    ssapService = SsapAllocServiceParam(&service);
    EXPECT_NE(ssapService, NULL);
    FreeService(ssapService);
    ssapService = NULL;

    property.descriptorNum = 1;
    property.descriptors = NULL;
    service.serviceMethodNum = 1;
    service.method = NULL;
    // propertyParm->descriptorNum > 0 && propertyParm->descriptors != NULL
    ssapService = SsapAllocServiceParam(&service);
    EXPECT_NE(ssapService, NULL);
    FreeService(ssapService);
    ssapService = NULL;

    property.descriptorNum = 0;
    property.descriptors = NULL;
    service.serviceMethodNum = 0;
    service.method = NULL;
    // propertyParm->descriptorNum > 0 && propertyParm->descriptors != NULL
    ssapService = SsapAllocServiceParam(&service);
    EXPECT_NE(ssapService, NULL);
    FreeService(ssapService);
    ssapService = NULL;
}

// ============================================================================
// FIND_STRUCTURE_TYPE_SERVICE_STRUCTURE 分支测试
// match SendFindStructureRsp func
// ============================================================================
TEST_F(UT_SSAP_SERVER_FIND, FIND_RSP_SERVICE_STRUCT_STD)
{
    CP_LOG_INFO("[UT_SSAP_SERVER_FIND] enter FIND_RSP_SERVICE_STRUCT_STD");
    AddService();
    SSAP_Link *link = CreateLink();

    static uint8_t reqStructStd[] = {0x04, 0x00, 0x01, 0x00, 0xFF, 0x00};
    static uint8_t rspStructStd[] = {
        0x05, 0x03,
        // 服务
        0x10, 0x00, 0x00, 0x02, 0x01, // handle(2) + type(1) + uuid(2) + 描述符类型列表
        0x00, // 描述符类型列表
        // 属性
        0x11, 0x00, 0x02, 0x03, 0x02, // handle(2) + type(1) + uuid(2)
        0x00, 0x00, 0x00, 0x00, // 操作指示
        0x01, 0x01 // 描述符类型列表
    };

    Test_SSAP_RecvReq(reqStructStd, sizeof(reqStructStd));

    EXPECT_TRUE(Test_SSAP_CompareLastSendPkt(rspStructStd, sizeof(rspStructStd)));

    DeleteLink();
}
