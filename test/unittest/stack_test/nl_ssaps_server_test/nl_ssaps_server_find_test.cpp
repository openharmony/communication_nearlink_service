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

#include "ssap_type.h"
#include "ssap_pkt.h"
#include "ssap_utils.h"
#include "ssap_link.h"
#include "ssap_manager.h"
#include "ssaps_service.h"
#include "ssaps_server.h"
#include "ssaps_server_api.h"
#include "nlstk_ssap_app_server.h"
#include "sdf_buff.h"
#include "sdf_mem.h"
#include "cpfwk_log.h"

#include "cm_def.h"
#include "stack_schedule_mock.h"
#include "stack_schedule_stub.h"  // verify-fix: TEST_ScheduleInit
#include "stack_cm_mock.h"
#include "stack_cm_stub.h"
#include "ssaps_server_app.h"
#include "ssap_link_state.h"  // verify-fix: SsapServerAppDeinit
#include "stack_dtap_mock.h"
#include "stack_dtap_stub.h"

using namespace testing;

// 桩里定义的可切换设备类型：UNKNOWN=新版对端，OLD=老版本对端
extern "C" uint8_t g_nlSsapsDevType;

#define TEST_MAX_BUF_CACHE 2048

static uint8_t g_rspCache[TEST_MAX_BUF_CACHE] = {0};
static uint16_t g_rspLen = 0;

// 标准UUID末两字节 0x01,0x02 的首要服务；星闪标准UUID前14字节为固定基值
static NLSTK_SsapUuid_S g_svcStdUuid = {.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02}};
// 标准UUID属性，末两字节 0x02,0x03
static NLSTK_SsapUuid_S g_propStdUuid = {.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x02, 0x03}};
// 标准UUID方法，末两字节 0x02,0x04
static NLSTK_SsapUuid_S g_methodStdUuid = {.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x02, 0x04}};
// 自定义服务UUID（非标准基值）
static NLSTK_SsapUuid_S g_svcCusUuid = {.uuid = {0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
    0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12}};
// 自定义属性UUID
static NLSTK_SsapUuid_S g_propCusUuid = {.uuid = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10}};

// 响应/请求中自定义UUID按数组倒序（大端）出现在报文里
static const uint8_t R_SVC_CUS_UUID[SSAP_UUID128_LEN] = {0x12, 0x11, 0x10, 0x0F, 0x0E, 0x0D, 0x0C, 0x0B,
    0x0A, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03};
static const uint8_t R_PROP_CUS_UUID[SSAP_UUID128_LEN] = {0x10, 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09,
    0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01};

static SLE_Addr_S g_localAddr = {.type = (NLSTK_SsapDescriptorType_E)PUBLIC_ADDRESS, .addr = {0x03, 0x03, 0x04, 0x04, 0x05, 0x05}};
static uint16_t g_lcid = 1;

class NlSsapsFindTest : public testing::Test {
public:
    NiceMock<OHOS::ScheduleMock> scheduleMock;
    NiceMock<OHOS::CmMock> cmMock;
    NiceMock<OHOS::DtapMock> dtapMock;
protected:
    void SetUp() override
    {
        TEST_ScheduleInit();
        EXPECT_CALL(scheduleMock, SchedulePostTask).WillRepeatedly(TEST_SchedulePostTaskStub);
        EXPECT_CALL(scheduleMock, SchedulePostTaskBlocked).WillRepeatedly(TEST_SchedulePostTaskBlockedStub);
        TEST_CM_Init();
        EXPECT_CALL(cmMock, CM_RegLogicLinkListener).WillRepeatedly(TEST_CM_RegLogicLinkListener);
        EXPECT_CALL(cmMock, CM_UnRegLogicLinkListener).WillRepeatedly(TEST_CM_UnRegLogicLinkListener);
        EXPECT_CALL(dtapMock, DTAP_RegisterDataRecvCb).WillRepeatedly(TEST_DTAP_RegisterDataRecvCb);
        EXPECT_CALL(dtapMock, DTAP_UnregisterDataRecvCb).WillRepeatedly(TEST_DTAP_UnregisterDataRecvCb);
        EXPECT_CALL(dtapMock, DTAP_DataSend).WillRepeatedly(TEST_DTAP_DataSend);
        g_nlSsapsDevType = CM_DEVTYPE_UNKNOWN;
        g_rspLen = 0;
        SSAP_LinkInit();
        SSAP_Init();
    }

    void TearDown() override
    {
        SSAP_DeInit();
        SSAP_LinkDeInit();
        SsapServerAppDeinit();
        SsapLinkStateDeinit();
    }
};

static void MockSendCb(SSAP_Link *link, SDF_Buff_S *buff, uint8_t opcode)
{
    g_rspLen = (uint16_t)SDF_DataLenGet(buff);
    if (g_rspLen <= TEST_MAX_BUF_CACHE) {
        (void)memcpy_s(g_rspCache, TEST_MAX_BUF_CACHE, SDF_DataOffset(buff), g_rspLen);
    }
    uint8_t type = SSAP_GetOpcodeType(opcode);
    if ((type & SSAP_REPLY_MASK) == 0) {
        SDF_BuffFree(buff);
    } else {
        SSAP_LinkSetTask(link, buff, opcode);
    }
}

static SSAP_Link *CreateTestLink()
{
    return SSAP_CreateSsapLink(&g_localAddr, g_lcid, MockSendCb);
}

static void DeleteTestLink()
{
    SSAP_DeleteSsapLinkByAddr(&g_localAddr);
}

static void SendFindReq(SSAP_Link *link, const uint8_t *req, uint16_t len)
{
    g_rspLen = 0;
    SDF_Buff_S *buff = SDF_BuffNewWithReserve(len);
    uint8_t *buf = SDF_BuffAppend(buff, len);
    (void)memcpy_s(buf, len, req, len);
    SSAPS_FindReqHandle(link, buff);
    SDF_BuffFree(buff);
}

static bool RspEquals(const uint8_t *expected, uint16_t len)
{
    return g_rspLen == len && memcmp(g_rspCache, expected, len) == 0;
}

static void CacheService(const NLSTK_SsapUuid_S &uuid, NLSTK_SsapItemType_E type)
{
    SSAP_ParamAddService_S *svc = (SSAP_ParamAddService_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddService_S));
    svc->serviceType = type;
    (void)memcpy_s(&svc->uuid, sizeof(NLSTK_SsapUuid_S), &uuid, sizeof(NLSTK_SsapUuid_S));
    SSAP_CacheService(svc);
    SDF_MemFree(svc);
}

static void CacheProperty(const NLSTK_SsapUuid_S &uuid, uint8_t value, uint32_t operation)
{
    SSAP_ParamAddProperty_S *prop =
        (SSAP_ParamAddProperty_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddProperty_S) + 1);
    (void)memcpy_s(&prop->uuid, sizeof(NLSTK_SsapUuid_S), &uuid, sizeof(NLSTK_SsapUuid_S));
    prop->val.len = 1;
    prop->val.value[0] = value;
    prop->operation.operationValue = operation;
    SSAP_CacheProperty(prop);
    SDF_MemFree(prop);
}

static void CacheDescriptor(NLSTK_SsapDescriptorType_E descType, uint32_t operation)
{
    SSAP_ParamAddDescriptor_S *desc =
        (SSAP_ParamAddDescriptor_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddDescriptor_S) + 1);
    desc->type = descType;
    desc->val.len = 1;
    desc->val.value[0] = 0xEE;
    desc->operation.operationValue = operation;
    SSAP_CacheDescriptor(desc);
    SDF_MemFree(desc);
}

// 标准服务(句柄0x10)+标准属性(0x11，带属性说明描述符)
static void AddStdServiceWithPropDesc()
{
    CacheService(g_svcStdUuid, ITEM_TYPE_STD_PRIMARY_SERVICE);
    CacheProperty(g_propStdUuid, 0xFF, 0x01);
    CacheDescriptor(DESC_TYPE_PROPERTY_INSTRUCTION, 0x100);
    SSAP_StartService(nullptr);
}

// 自定义服务(无属性)，句柄0x12
static void AddCusServiceNoProp()
{
    CacheService(g_svcCusUuid, ITEM_TYPE_VENDOR_PRIMARY_SERVICE);
    SSAP_StartService(nullptr);
}

// 标准服务(0x10)+标准属性(0x11,带描述符)+自定义属性(0x12)，须在一次StartService前缓存完
static void AddSvcWithStdAndCusProp()
{
    CacheService(g_svcStdUuid, ITEM_TYPE_STD_PRIMARY_SERVICE);
    CacheProperty(g_propStdUuid, 0xFF, 0x01);
    CacheDescriptor(DESC_TYPE_PROPERTY_INSTRUCTION, 0x100);
    CacheProperty(g_propCusUuid, 0xAB, 0x01);
    SSAP_StartService(nullptr);
}

static int32_t g_regAppId = -1;

static void OnAddServiceStub(int32_t appId, SSAP_Service_S *service, NLSTK_Errcode_E ret)
{
    (void)service;
    (void)ret;
    g_regAppId = appId;
}

// 北向接口注册应用并添加带方法的服务：服务0x10、属性0x11、方法0x12（标准UUID）
static void AddServiceWithMethodViaApi()
{
    NLSTK_SsapAppServerCb_S cb = {0};
    cb.onAddService = OnAddServiceStub;
    int32_t appId = -1;
    ASSERT_EQ(NLSTK_SsapServerRegApp(&cb, &appId), NLSTK_ERRCODE_SUCCESS);

    uint8_t propData[1] = {0x55};
    NLSTK_SsapServiceDescriptorParam_S propDesc = {
        .type = (NLSTK_SsapDescriptorType_E)DESC_TYPE_PROPERTY_INSTRUCTION,
        .val = {.len = 1, .data = propData},
    };
    NLSTK_SsapServicePropertyParam_S prop = {
        .uuid = g_propStdUuid,
        .operation = {.operationValue = 0x01},
        .val = {.len = 1, .data = propData},
        .descriptorNum = 1,
        .descriptors = &propDesc,
    };
    NLSTK_SsapServiceMethodParam_S method = {
        .type = (NLSTK_SsapItemType_E)ITEM_TYPE_STD_METHOD,
        .uuid = g_methodStdUuid,
    };
    NLSTK_ServiceParam_S svcParam = {
        .serviceStatement = {.uuid = g_svcStdUuid, .serviceType = ITEM_TYPE_STD_PRIMARY_SERVICE},
        .servicePropertyNum = 1,
        .property = &prop,
        .serviceMethodNum = 1,
        .method = &method,
    };
    ASSERT_EQ(NLSTK_SsapServerAddService(appId, &svcParam), NLSTK_ERRCODE_SUCCESS);
}

// 北向服务至少要带一个属性：属性数组传空指针会被无条件拒绝，纯方法服务加不进服务库
static uint8_t g_methodSvcPropData[1] = {0x66};
static NLSTK_SsapServicePropertyParam_S g_methodSvcProp = {
    .operation = {.operationValue = 0x01},
    .val = {.len = 1, .data = g_methodSvcPropData},
};

static void MakeMethodServiceParam(const NLSTK_SsapServiceMethodParam_S *method, NLSTK_ServiceParam_S *svcParam)
{
    g_methodSvcProp.uuid = g_propStdUuid;
    svcParam->serviceStatement.uuid = g_svcStdUuid;
    svcParam->serviceStatement.serviceType = ITEM_TYPE_STD_PRIMARY_SERVICE;
    svcParam->servicePropertyNum = 1;
    svcParam->property = &g_methodSvcProp;
    svcParam->serviceMethodNum = 1;
    svcParam->method = method;
}

// 再追加 count 个带属性与一个方法的标准服务（方法UUID末字节区分），句柄紧随其后
static void AddMethodOnlyServicesViaApi(int32_t appId, uint16_t count)
{
    for (uint16_t i = 0; i < count; i++) {
        NLSTK_SsapUuid_S methodUuid = g_methodStdUuid;
        methodUuid.uuid[SSAP_UUID128_LEN - 2] = (uint8_t)(0x30 + i);
        methodUuid.uuid[SSAP_UUID128_LEN - 1] = 0x00;
        NLSTK_SsapServiceMethodParam_S method = {
            .type = (NLSTK_SsapItemType_E)ITEM_TYPE_STD_METHOD,
            .uuid = methodUuid,
        };
        NLSTK_ServiceParam_S svcParam = {};
        MakeMethodServiceParam(&method, &svcParam);
        ASSERT_EQ(NLSTK_SsapServerAddService(appId, &svcParam), NLSTK_ERRCODE_SUCCESS);
    }
}

// 追加一个带属性与自定义UUID方法的标准服务（句柄紧随其后）
static void AddCusMethodServiceViaApi(int32_t appId)
{
    NLSTK_SsapServiceMethodParam_S method = {
        .type = (NLSTK_SsapItemType_E)ITEM_TYPE_STD_METHOD,
        .uuid = g_propCusUuid,
    };
    NLSTK_ServiceParam_S svcParam = {};
    MakeMethodServiceParam(&method, &svcParam);
    ASSERT_EQ(NLSTK_SsapServerAddService(appId, &svcParam), NLSTK_ERRCODE_SUCCESS);
}

// 查首要服务：句柄窗口命中标准服务、窗口只含自定义服务时被条目类型过滤，返回未找到
TEST_F(NlSsapsFindTest, PrimaryFindWindowAndItemTypeFilter)
{
    // 窗口[0x01,0x10]命中标准服务：响应带句柄区间、标准UUID与成员位
    AddStdServiceWithPropDesc();
    SSAP_Link *link = CreateTestLink();
    uint8_t reqHit[] = {0x04, 0x01, 0x01, 0x00, 0x10, 0x00};
    uint8_t rspHit[] = {0x05, 0x03, 0x10, 0x00, 0x11, 0x00, 0x02, 0x01, 0x02};
    SendFindReq(link, reqHit, sizeof(reqHit));
    EXPECT_TRUE(RspEquals(rspHit, sizeof(rspHit)));

    // 窗口[0x12,0x12]只含自定义服务，标准条目类型过滤后无结果
    AddCusServiceNoProp();
    uint8_t reqMiss[] = {0x04, 0x01, 0x12, 0x00, 0x12, 0x00};
    uint8_t rspMiss[] = {0x01, 0x00, 0x04, 0x12, 0x00, 0x0B};
    SendFindReq(link, reqMiss, sizeof(reqMiss));
    EXPECT_TRUE(RspEquals(rspMiss, sizeof(rspMiss)));
    DeleteTestLink();
}

// 按UUID查首要服务：属性UUID与未知UUID都判定类型不符，服务UUID命中
TEST_F(NlSsapsFindTest, PrimaryFindByUuidTypeCheck)
{
    AddStdServiceWithPropDesc();
    SSAP_Link *link = CreateTestLink();

    // 属性UUID不是服务UUID，类型不符返回未找到
    uint8_t reqPropUuid[] = {0x06, 0x01, 0x01, 0x00, 0xFF, 0x00, 0x03, 0x02};
    uint8_t rspErr[] = {0x01, 0x00, 0x06, 0x01, 0x00, 0x0B};
    SendFindReq(link, reqPropUuid, sizeof(reqPropUuid));
    EXPECT_TRUE(RspEquals(rspErr, sizeof(rspErr)));

    // 完全不存在的UUID同样未找到
    uint8_t reqNone[] = {0x06, 0x01, 0x01, 0x00, 0xFF, 0x00, 0x09, 0x09};
    SendFindReq(link, reqNone, sizeof(reqNone));
    EXPECT_TRUE(RspEquals(rspErr, sizeof(rspErr)));

    // 服务UUID命中，响应单条标准首要服务
    uint8_t reqSvcUuid[] = {0x06, 0x01, 0x01, 0x00, 0xFF, 0x00, 0x02, 0x01};
    uint8_t rspHit[] = {0x07, 0x03, 0x10, 0x00, 0x11, 0x00, 0x02, 0x01, 0x02};
    SendFindReq(link, reqSvcUuid, sizeof(reqSvcUuid));
    EXPECT_TRUE(RspEquals(rspHit, sizeof(rspHit)));
    DeleteTestLink();
}

// 混合模式查首要服务：标准与自定义服务分组携带，各自带指示头
TEST_F(NlSsapsFindTest, PrimaryFindMixStdAndCusGrouped)
{
    AddStdServiceWithPropDesc();
    AddCusServiceNoProp();
    SSAP_Link *link = CreateTestLink();
    uint8_t req[] = {0x04, 0x11, 0x01, 0x00, 0xFF, 0x00};
    uint8_t rsp[32] = {0x05, 0x0B,
        0x01, 0x10, 0x00, 0x11, 0x00, 0x02, 0x01, 0x02,
        0x81, 0x12, 0x00, 0x12, 0x00};
    (void)memcpy_s(rsp + 15, SSAP_UUID128_LEN, R_SVC_CUS_UUID, SSAP_UUID128_LEN);
    rsp[31] = 0x00;
    SendFindReq(link, req, sizeof(req));
    EXPECT_TRUE(RspEquals(rsp, sizeof(rsp)));
    DeleteTestLink();
}

// 混合模式组包预算：MTU只够装下标准服务时，自定义服务被截断且指示头计数如实反映
TEST_F(NlSsapsFindTest, PrimaryFindMixTruncatedByMtu)
{
    AddStdServiceWithPropDesc();
    AddCusServiceNoProp();
    SSAP_Link *link = CreateTestLink();
    link->mtu = 11; // 头2+双指示头2+标准项7，自定义项装不下
    uint8_t req[] = {0x04, 0x11, 0x01, 0x00, 0xFF, 0x00};
    uint8_t rsp[] = {0x05, 0x0B, 0x01, 0x10, 0x00, 0x11, 0x00, 0x02, 0x01, 0x02, 0x80};
    SendFindReq(link, req, sizeof(req));
    EXPECT_TRUE(RspEquals(rsp, sizeof(rsp)));
    DeleteTestLink();
}

// 查属性：标准/自定义/混合三种条目类型各自过滤出对应属性，描述符类型列表随行
TEST_F(NlSsapsFindTest, PropertyFindItemTypeFilter)
{
    AddSvcWithStdAndCusProp();
    SSAP_Link *link = CreateTestLink();

    // 标准类型：仅标准属性，带描述符计数1与类型字节
    uint8_t reqStd[] = {0x04, 0x03, 0x01, 0x00, 0xFF, 0x00};
    uint8_t rspStd[] = {0x05, 0x03, 0x11, 0x00, 0x03, 0x02, 0x01, 0x00, 0x00, 0x00, 0x01, 0x01};
    SendFindReq(link, reqStd, sizeof(reqStd));
    EXPECT_TRUE(RspEquals(rspStd, sizeof(rspStd)));

    // 自定义类型：仅自定义属性，无描述符
    uint8_t reqCus[] = {0x04, 0x0B, 0x01, 0x00, 0xFF, 0x00};
    uint8_t rspCus[25] = {0x05, 0x07, 0x12, 0x00};
    (void)memcpy_s(rspCus + 4, SSAP_UUID128_LEN, R_PROP_CUS_UUID, SSAP_UUID128_LEN);
    rspCus[20] = 0x01;
    rspCus[24] = 0x00;
    SendFindReq(link, reqCus, sizeof(reqCus));
    EXPECT_TRUE(RspEquals(rspCus, sizeof(rspCus)));

    // 混合类型：两组指示头分别携带
    uint8_t reqMix[] = {0x04, 0x13, 0x01, 0x00, 0xFF, 0x00};
    uint8_t rspMix[37] = {0x05, 0x0B, 0x01};
    (void)memcpy_s(rspMix + 3, 10, rspStd + 2, 10);
    rspMix[13] = 0x81;
    (void)memcpy_s(rspMix + 14, 23, rspCus + 2, 23);
    SendFindReq(link, reqMix, sizeof(reqMix));
    EXPECT_TRUE(RspEquals(rspMix, sizeof(rspMix)));

    // 自定义条目预算不足：MTU装不下任何条目时仅剩响应头
    link->mtu = 10;
    SendFindReq(link, reqCus, sizeof(reqCus));
    uint8_t rspCusTrunc[] = {0x05, 0x07};
    EXPECT_TRUE(RspEquals(rspCusTrunc, sizeof(rspCusTrunc)));
    DeleteTestLink();
}

// 按服务UUID查属性：命中第二个服务前先跳过不匹配的第一个；命中第一个服务返回其全部属性
TEST_F(NlSsapsFindTest, PropertyFindByServiceUuidTraverse)
{
    AddSvcWithStdAndCusProp();
    AddCusServiceNoProp();
    SSAP_Link *link = CreateTestLink();

    // 命中第二个（自定义、无属性）服务：跳过第一个后无属性可返回
    uint8_t req2[6 + SSAP_UUID128_LEN] = {0x06, 0x13, 0x01, 0x00, 0xFF, 0x00};
    for (int i = 0; i < SSAP_UUID128_LEN; i++) {
        req2[6 + i] = g_svcCusUuid.uuid[SSAP_UUID128_LEN - 1 - i];
    }
    uint8_t rspEmpty[] = {0x01, 0x00, 0x06, 0x01, 0x00, 0x0B};
    SendFindReq(link, req2, sizeof(req2));
    EXPECT_TRUE(RspEquals(rspEmpty, sizeof(rspEmpty)));

    // 命中第一个服务：其标准与自定义属性按混合模式成组返回
    uint8_t req1[] = {0x06, 0x13, 0x01, 0x00, 0xFF, 0x00, 0x02, 0x01};
    uint8_t rsp1[37] = {0x07, 0x0B, 0x01};
    uint8_t stdItem[10] = {0x11, 0x00, 0x03, 0x02, 0x01, 0x00, 0x00, 0x00, 0x01, 0x01};
    (void)memcpy_s(rsp1 + 3, 10, stdItem, 10);
    rsp1[13] = 0x81;
    uint8_t cusItem[23] = {0x12, 0x00};
    (void)memcpy_s(cusItem + 2, SSAP_UUID128_LEN, R_PROP_CUS_UUID, SSAP_UUID128_LEN);
    cusItem[18] = 0x01;
    (void)memcpy_s(rsp1 + 14, 23, cusItem, 23);
    SendFindReq(link, req1, sizeof(req1));
    EXPECT_TRUE(RspEquals(rsp1, sizeof(rsp1)));
    DeleteTestLink();
}

// 按属性UUID查属性：自定义UUID命中走自定义条目；服务UUID按新路径聚合返回该服务属性
TEST_F(NlSsapsFindTest, PropertyFindByPropUuid)
{
    AddSvcWithStdAndCusProp();
    SSAP_Link *link = CreateTestLink();

    // 自定义属性UUID+自定义条目类型：命中该属性
    uint8_t reqProp[6 + SSAP_UUID128_LEN] = {0x06, 0x0B, 0x01, 0x00, 0xFF, 0x00};
    for (int i = 0; i < SSAP_UUID128_LEN; i++) {
        reqProp[6 + i] = g_propCusUuid.uuid[SSAP_UUID128_LEN - 1 - i];
    }
    uint8_t rsp[25] = {0x07, 0x07, 0x12, 0x00};
    (void)memcpy_s(rsp + 4, SSAP_UUID128_LEN, R_PROP_CUS_UUID, SSAP_UUID128_LEN);
    rsp[20] = 0x01;
    rsp[24] = 0x00;
    SendFindReq(link, reqProp, sizeof(reqProp));
    EXPECT_TRUE(RspEquals(rsp, sizeof(rsp)));

    // 服务UUID：新路径按服务聚合，返回该服务下通过条目类型过滤的属性
    uint8_t reqSvc[] = {0x06, 0x03, 0x01, 0x00, 0xFF, 0x00, 0x02, 0x01};
    uint8_t rspSvc[] = {0x07, 0x03, 0x11, 0x00, 0x03, 0x02, 0x01, 0x00, 0x00, 0x00, 0x01, 0x01};
    SendFindReq(link, reqSvc, sizeof(reqSvc));
    EXPECT_TRUE(RspEquals(rspSvc, sizeof(rspSvc)));
    DeleteTestLink();
}

// 属性混合组包预算：MTU只够一个标准属性时截断第二个
TEST_F(NlSsapsFindTest, PropertyFindMixTruncatedByMtu)
{
    AddSvcWithStdAndCusProp();
    SSAP_Link *link = CreateTestLink();
    link->mtu = 14; // 头2+双指示头2+标准属性项10（基长7+UUID2+描述符1）
    uint8_t req[] = {0x04, 0x13, 0x01, 0x00, 0xFF, 0x00};
    uint8_t rsp[] = {0x05, 0x0B, 0x01, 0x11, 0x00, 0x03, 0x02, 0x01, 0x00, 0x00, 0x00, 0x01, 0x01, 0x80};
    SendFindReq(link, req, sizeof(req));
    EXPECT_TRUE(RspEquals(rsp, sizeof(rsp)));
    DeleteTestLink();
}

// 查服务结构：窗口只覆盖属性句柄时，服务因区间交叠被纳入但其条目按句柄过滤，仅返回属性
TEST_F(NlSsapsFindTest, StructureFindWindowOverlapPropertyOnly)
{
    AddStdServiceWithPropDesc();
    SSAP_Link *link = CreateTestLink();
    uint8_t req[] = {0x04, 0x00, 0x11, 0x00, 0x11, 0x00};
    uint8_t rsp[] = {0x05, 0x03, 0x11, 0x00, 0x02, 0x03, 0x02, 0x01, 0x00, 0x00, 0x00, 0x01, 0x01};
    SendFindReq(link, req, sizeof(req));
    EXPECT_TRUE(RspEquals(rsp, sizeof(rsp)));
    DeleteTestLink();
}

// 查服务结构：全窗口返回服务条目与属性条目，服务条目带描述符计数
TEST_F(NlSsapsFindTest, StructureFindServiceAndPropertyItems)
{
    AddStdServiceWithPropDesc();
    SSAP_Link *link = CreateTestLink();
    uint8_t req[] = {0x04, 0x00, 0x01, 0x00, 0xFF, 0x00};
    uint8_t rsp[] = {0x05, 0x03,
        0x10, 0x00, 0x00, 0x02, 0x01, 0x00,
        0x11, 0x00, 0x02, 0x03, 0x02, 0x01, 0x00, 0x00, 0x00, 0x01, 0x01};
    SendFindReq(link, req, sizeof(req));
    EXPECT_TRUE(RspEquals(rsp, sizeof(rsp)));
    DeleteTestLink();
}

// 按UUID查服务结构：非服务UUID类型不符返回PDU无效；自定义条目类型过滤掉标准服务
TEST_F(NlSsapsFindTest, StructureFindByUuidAndItemTypeReject)
{
    AddStdServiceWithPropDesc();
    SSAP_Link *link = CreateTestLink();

    // 属性UUID不是服务UUID：结构查找类型不符，返回PDU无效
    uint8_t reqPropUuid[] = {0x06, 0x00, 0x01, 0x00, 0xFF, 0x00, 0x03, 0x02};
    uint8_t rspInvalid[] = {0x01, 0x00, 0x06, 0x01, 0x00, 0x01};
    SendFindReq(link, reqPropUuid, sizeof(reqPropUuid));
    EXPECT_TRUE(RspEquals(rspInvalid, sizeof(rspInvalid)));

    // 自定义条目类型查标准服务结构：条目被过滤，未找到
    uint8_t reqCusType[] = {0x04, 0x08, 0x01, 0x00, 0xFF, 0x00};
    uint8_t rspEmpty[] = {0x01, 0x00, 0x04, 0x01, 0x00, 0x0B};
    SendFindReq(link, reqCusType, sizeof(reqCusType));
    EXPECT_TRUE(RspEquals(rspEmpty, sizeof(rspEmpty)));
    DeleteTestLink();
}

// 服务结构组包预算：MTU只够一个服务条目时截断后续条目
TEST_F(NlSsapsFindTest, StructureFindTruncatedByMtu)
{
    AddStdServiceWithPropDesc();
    AddCusServiceNoProp();
    SSAP_Link *link = CreateTestLink();
    link->mtu = 8; // 头2+服务条目6（句柄2+类型1+标准UUID2+描述符计数1）
    uint8_t req[] = {0x04, 0x00, 0x01, 0x00, 0xFF, 0x00};
    uint8_t rsp[] = {0x05, 0x03, 0x10, 0x00, 0x00, 0x02, 0x01, 0x00};
    SendFindReq(link, req, sizeof(req));
    EXPECT_TRUE(RspEquals(rsp, sizeof(rsp)));
    DeleteTestLink();
}

// 北向注册的服务结构：服务自带描述符时，结构条目携带描述符类型列表
TEST_F(NlSsapsFindTest, StructureFindServiceDescriptorFromApi)
{
    uint8_t servDescData[1] = {0x00};
    NLSTK_SsapServiceDescriptorParam_S servDesc = {
        .type = (NLSTK_SsapDescriptorType_E)DESC_TYPE_PROPERTY_RESERVE,
        .val = {.len = 1, .data = servDescData},
    };
    NLSTK_SsapAppServerCb_S cb = {0};
    cb.onAddService = OnAddServiceStub;
    int32_t appId = -1;
    ASSERT_EQ(NLSTK_SsapServerRegApp(&cb, &appId), NLSTK_ERRCODE_SUCCESS);
    uint8_t propData[1] = {0x77};
    NLSTK_SsapServicePropertyParam_S prop = {
        .uuid = g_propStdUuid,
        .operation = {.operationValue = 0x01},
        .val = {.len = 1, .data = propData},
    };
    NLSTK_ServiceParam_S svcParam = {
        .serviceStatement = {.uuid = g_svcStdUuid, .serviceType = ITEM_TYPE_STD_PRIMARY_SERVICE,
                             .descriptorNum = 1, .descriptors = &servDesc},
        .servicePropertyNum = 1,
        .property = &prop,
    };
    ASSERT_EQ(NLSTK_SsapServerAddService(appId, &svcParam), NLSTK_ERRCODE_SUCCESS);

    SSAP_Link *link = CreateTestLink();
    uint8_t req[] = {0x04, 0x00, 0x01, 0x00, 0x10, 0x10};
    uint8_t rsp[] = {0x05, 0x03, 0x10, 0x00, 0x00, 0x02, 0x01, 0x01, 0x00};
    SendFindReq(link, req, sizeof(req));
    EXPECT_TRUE(RspEquals(rsp, sizeof(rsp)));
    DeleteTestLink();
}

// 查方法：北向注册带方法的服务后，标准方法条目按句柄+UUID+保留位返回
TEST_F(NlSsapsFindTest, MethodFindStandardItem)
{
    AddServiceWithMethodViaApi();
    SSAP_Link *link = CreateTestLink();
    uint8_t req[] = {0x04, 0x04, 0x01, 0x00, 0xFF, 0x00};
    uint8_t rsp[] = {0x05, 0x03, 0x12, 0x00, 0x04, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00};
    SendFindReq(link, req, sizeof(req));
    EXPECT_TRUE(RspEquals(rsp, sizeof(rsp)));
    DeleteTestLink();
}

// 查方法混合模式：两个方法只装得下第一个时按MTU截断，指示头计数为1
TEST_F(NlSsapsFindTest, MethodFindMixTruncatedByMtu)
{
    AddServiceWithMethodViaApi();
    AddMethodOnlyServicesViaApi(g_regAppId, 1);
    SSAP_Link *link = CreateTestLink();
    link->mtu = 15; // 头2+双指示头2+标准方法项9，第二个方法装不下
    uint8_t req[] = {0x04, 0x14, 0x01, 0x00, 0xFF, 0x00};
    uint8_t rsp[] = {0x05, 0x0B, 0x01, 0x12, 0x00, 0x04, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80};
    SendFindReq(link, req, sizeof(req));
    EXPECT_TRUE(RspEquals(rsp, sizeof(rsp)));
    DeleteTestLink();
}

// 方法标准组包预算：两个方法只装得下一个时按MTU截断
TEST_F(NlSsapsFindTest, MethodFindStdTruncatedByMtu)
{
    AddServiceWithMethodViaApi();
    AddMethodOnlyServicesViaApi(g_regAppId, 1);
    SSAP_Link *link = CreateTestLink();
    link->mtu = 11; // 头2+方法项9
    uint8_t req[] = {0x04, 0x04, 0x01, 0x00, 0xFF, 0x00};
    uint8_t rsp[] = {0x05, 0x03, 0x12, 0x00, 0x04, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00};
    SendFindReq(link, req, sizeof(req));
    EXPECT_TRUE(RspEquals(rsp, sizeof(rsp)));
    DeleteTestLink();
}

// 按UUID查方法：方法UUID命中；服务UUID返回该服务全部方法；属性UUID类型不符
TEST_F(NlSsapsFindTest, MethodFindByUuidVariants)
{
    AddServiceWithMethodViaApi();
    SSAP_Link *link = CreateTestLink();

    // 方法UUID命中
    uint8_t reqMethodUuid[] = {0x06, 0x04, 0x01, 0x00, 0xFF, 0x00, 0x04, 0x02};
    uint8_t rspHit[] = {0x07, 0x03, 0x12, 0x00, 0x04, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00};
    SendFindReq(link, reqMethodUuid, sizeof(reqMethodUuid));
    EXPECT_TRUE(RspEquals(rspHit, sizeof(rspHit)));

    // 属性UUID类型不符
    uint8_t reqPropUuid[] = {0x06, 0x04, 0x01, 0x00, 0xFF, 0x00, 0x03, 0x02};
    uint8_t rspErr[] = {0x01, 0x00, 0x06, 0x01, 0x00, 0x0B};
    SendFindReq(link, reqPropUuid, sizeof(reqPropUuid));
    EXPECT_TRUE(RspEquals(rspErr, sizeof(rspErr)));

    // 服务UUID返回其方法
    uint8_t reqSvcUuid[] = {0x06, 0x04, 0x01, 0x00, 0xFF, 0x00, 0x02, 0x01};
    SendFindReq(link, reqSvcUuid, sizeof(reqSvcUuid));
    EXPECT_TRUE(RspEquals(rspHit, sizeof(rspHit)));
    DeleteTestLink();
}

// 按UUID查方法：目标服务在前时跳过不匹配服务；句柄窗口过滤掉区外方法
TEST_F(NlSsapsFindTest, MethodFindByServiceUuidTraverseAndWindow)
{
    AddServiceWithMethodViaApi();
    AddMethodOnlyServicesViaApi(g_regAppId, 1); // 第二个服务（0x13-0x15）的方法句柄0x15、UUID末两字节0x30,0x00
    SSAP_Link *link = CreateTestLink();

    // 第二个服务的方法UUID：遍历跳过第一个服务后命中
    uint8_t reqSecond[] = {0x06, 0x04, 0x01, 0x00, 0xFF, 0x00, 0x00, 0x30};
    uint8_t rspSecond[] = {0x07, 0x03, 0x15, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00};
    SendFindReq(link, reqSecond, sizeof(reqSecond));
    EXPECT_TRUE(RspEquals(rspSecond, sizeof(rspSecond)));

    // 窗口只含第一个服务区间：第二个服务的方法被句柄过滤
    uint8_t reqWindow[] = {0x06, 0x04, 0x01, 0x00, 0x13, 0x00, 0x04, 0x02};
    uint8_t rspFirst[] = {0x07, 0x03, 0x12, 0x00, 0x04, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00};
    SendFindReq(link, reqWindow, sizeof(reqWindow));
    EXPECT_TRUE(RspEquals(rspFirst, sizeof(rspFirst)));
    DeleteTestLink();
}

// 混合模式查方法：标准与自定义方法分组携带，各自带指示头
TEST_F(NlSsapsFindTest, MethodFindMixStdAndCusGrouped)
{
    AddServiceWithMethodViaApi();           // 方法0x12（标准UUID）
    AddCusMethodServiceViaApi(g_regAppId);  // 方法0x15（自定义UUID）
    SSAP_Link *link = CreateTestLink();
    uint8_t req[] = {0x04, 0x14, 0x01, 0x00, 0xFF, 0x00};
    uint8_t rsp[36] = {0x05, 0x0B,
        0x01, 0x12, 0x00, 0x04, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x81, 0x15, 0x00};
    (void)memcpy_s(rsp + 15, SSAP_UUID128_LEN, R_PROP_CUS_UUID, SSAP_UUID128_LEN);
    rsp[31] = 0x00;
    rsp[32] = 0x00;
    rsp[33] = 0x00;
    rsp[34] = 0x00;
    rsp[35] = 0x00;
    SendFindReq(link, req, sizeof(req));
    EXPECT_TRUE(RspEquals(rsp, sizeof(rsp)));
    DeleteTestLink();
}

// 查服务结构：带方法的服务返回服务+属性+方法三类条目，方法条目带类型与保留位
TEST_F(NlSsapsFindTest, StructureFindWithMethodItem)
{
    AddServiceWithMethodViaApi();
    SSAP_Link *link = CreateTestLink();
    uint8_t req[] = {0x04, 0x00, 0x01, 0x00, 0xFF, 0x00};
    uint8_t rsp[] = {0x05, 0x03,
        0x10, 0x00, 0x00, 0x02, 0x01, 0x00,
        0x11, 0x00, 0x02, 0x03, 0x02, 0x01, 0x00, 0x00, 0x00, 0x01, 0x01,
        0x12, 0x00, 0x03, 0x04, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00};
    SendFindReq(link, req, sizeof(req));
    EXPECT_TRUE(RspEquals(rsp, sizeof(rsp)));
    DeleteTestLink();
}

// 老版本对端按UUID查首要服务：命中标准服务返回老格式条目（无UUID）；类型不符与空窗口报未找到
TEST_F(NlSsapsFindTest, V10PrimaryFindByUuidOldFormat)
{
    AddStdServiceWithPropDesc();
    SSAP_Link *link = CreateTestLink();
    g_nlSsapsDevType = CM_DEVTYPE_OLD;

    // 按服务UUID命中：老格式条目为句柄区间+成员位
    uint8_t reqSvcUuid[] = {0x06, 0x01, 0x01, 0x00, 0xFF, 0x00, 0x02, 0x01};
    uint8_t rspOld[] = {0x07, 0x03, 0x10, 0x00, 0x11, 0x00, 0x02};
    SendFindReq(link, reqSvcUuid, sizeof(reqSvcUuid));
    EXPECT_TRUE(RspEquals(rspOld, sizeof(rspOld)));

    // 属性UUID类型不符：老版本路径同样拒绝
    uint8_t reqPropUuid[] = {0x06, 0x01, 0x01, 0x00, 0xFF, 0x00, 0x03, 0x02};
    uint8_t rspErr[] = {0x01, 0x00, 0x06, 0x01, 0x00, 0x0B};
    SendFindReq(link, reqPropUuid, sizeof(reqPropUuid));
    EXPECT_TRUE(RspEquals(rspErr, sizeof(rspErr)));

    // 窗口越过全部服务：老版本列表为空报未找到
    uint8_t reqMiss[] = {0x04, 0x01, 0x13, 0x00, 0x14, 0x00};
    uint8_t rspMiss[] = {0x01, 0x00, 0x04, 0x13, 0x00, 0x0B};
    SendFindReq(link, reqMiss, sizeof(reqMiss));
    EXPECT_TRUE(RspEquals(rspMiss, sizeof(rspMiss)));
    DeleteTestLink();
}

// 老版本对端按自定义服务UUID混合查：仅自定义服务命中，老格式混合分组（标准计数为零）
TEST_F(NlSsapsFindTest, V10PrimaryMixOldFormatByCusUuid)
{
    AddStdServiceWithPropDesc();
    AddCusServiceNoProp();
    SSAP_Link *link = CreateTestLink();
    g_nlSsapsDevType = CM_DEVTYPE_OLD;
    uint8_t req[6 + SSAP_UUID128_LEN] = {0x06, 0x11, 0x01, 0x00, 0xFF, 0x00};
    for (int i = 0; i < SSAP_UUID128_LEN; i++) {
        req[6 + i] = g_svcCusUuid.uuid[SSAP_UUID128_LEN - 1 - i];
    }
    uint8_t rsp[] = {0x07, 0x0B, 0x00, 0x81, 0x12, 0x00, 0x12, 0x00, 0x00};
    SendFindReq(link, req, sizeof(req));
    EXPECT_TRUE(RspEquals(rsp, sizeof(rsp)));
    DeleteTestLink();
}

// 老版本对端混合组包预算：两个同UUID自定义服务只装得下一个时截断
TEST_F(NlSsapsFindTest, V10PrimaryMixOldTruncatedByMtu)
{
    AddStdServiceWithPropDesc();
    AddCusServiceNoProp(); // 句柄0x12
    AddCusServiceNoProp(); // 句柄0x13，与服务0x12同UUID，按UUID查同时命中
    SSAP_Link *link = CreateTestLink();
    g_nlSsapsDevType = CM_DEVTYPE_OLD;
    link->mtu = 9; // 头2+双指示头2+老格式条目5，第二个条目装不下
    uint8_t req[6 + SSAP_UUID128_LEN] = {0x06, 0x11, 0x01, 0x00, 0xFF, 0x00};
    for (int i = 0; i < SSAP_UUID128_LEN; i++) {
        req[6 + i] = g_svcCusUuid.uuid[SSAP_UUID128_LEN - 1 - i];
    }
    uint8_t rsp[] = {0x07, 0x0B, 0x00, 0x81, 0x12, 0x00, 0x12, 0x00, 0x00};
    SendFindReq(link, req, sizeof(req));
    EXPECT_TRUE(RspEquals(rsp, sizeof(rsp)));
    DeleteTestLink();
}

// 老版本对端按属性UUID查属性：老格式条目无UUID（句柄+操作指示+描述符计数+类型）
TEST_F(NlSsapsFindTest, V10PropertyOldFormatByPropUuid)
{
    AddStdServiceWithPropDesc();
    SSAP_Link *link = CreateTestLink();
    g_nlSsapsDevType = CM_DEVTYPE_OLD;
    uint8_t req[] = {0x06, 0x03, 0x01, 0x00, 0xFF, 0x00, 0x03, 0x02};
    uint8_t rsp[] = {0x07, 0x03, 0x11, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x01};
    SendFindReq(link, req, sizeof(req));
    EXPECT_TRUE(RspEquals(rsp, sizeof(rsp)));
    DeleteTestLink();
}

// 老版本对端按服务UUID查属性：老协议不支持，报未找到
TEST_F(NlSsapsFindTest, V10PropertyServiceUuidReject)
{
    AddStdServiceWithPropDesc();
    SSAP_Link *link = CreateTestLink();
    g_nlSsapsDevType = CM_DEVTYPE_OLD;
    uint8_t reqSvcUuid[] = {0x06, 0x13, 0x01, 0x00, 0xFF, 0x00, 0x02, 0x01};
    uint8_t rspErr[] = {0x01, 0x00, 0x06, 0x01, 0x00, 0x0B};
    SendFindReq(link, reqSvcUuid, sizeof(reqSvcUuid));
    EXPECT_TRUE(RspEquals(rspErr, sizeof(rspErr)));

    // 不带UUID查属性：老版本入口仍按新格式组包（仅按UUID查才走老格式）
    uint8_t reqNoUuid[] = {0x04, 0x03, 0x01, 0x00, 0xFF, 0x00};
    uint8_t rspNewFmt[] = {0x05, 0x03, 0x11, 0x00, 0x03, 0x02, 0x01, 0x00, 0x00, 0x00, 0x01, 0x01};
    SendFindReq(link, reqNoUuid, sizeof(reqNoUuid));
    EXPECT_TRUE(RspEquals(rspNewFmt, sizeof(rspNewFmt)));
    DeleteTestLink();
}

// 老版本对端属性混合组包预算：两个同UUID自定义属性只装得下一个时截断
TEST_F(NlSsapsFindTest, V10PropertyMixOldTruncatedByMtu)
{
    CacheService(g_svcStdUuid, ITEM_TYPE_STD_PRIMARY_SERVICE);
    CacheProperty(g_propCusUuid, 0xAB, 0x01); // 属性0x11
    CacheProperty(g_propCusUuid, 0xCD, 0x01); // 属性0x12，与0x11同UUID
    SSAP_StartService(nullptr);
    SSAP_Link *link = CreateTestLink();
    g_nlSsapsDevType = CM_DEVTYPE_OLD;
    link->mtu = 12; // 头2+双指示头2+老格式属性项7（无描述符），第二项装不下
    uint8_t req[6 + SSAP_UUID128_LEN] = {0x06, 0x13, 0x01, 0x00, 0xFF, 0x00};
    for (int i = 0; i < SSAP_UUID128_LEN; i++) {
        req[6 + i] = g_propCusUuid.uuid[SSAP_UUID128_LEN - 1 - i];
    }
    uint8_t rsp[] = {0x07, 0x0B, 0x00, 0x81, 0x11, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00};
    SendFindReq(link, req, sizeof(req));
    EXPECT_TRUE(RspEquals(rsp, sizeof(rsp)));
    DeleteTestLink();
}

// 老版本对端不按条目类型过滤：自定义条目类型请求仍返回标准+自定义两组，响应类型恒为混合
TEST_F(NlSsapsFindTest, V10PrimaryIgnoresItemTypeFilter)
{
    AddStdServiceWithPropDesc();
    AddCusServiceNoProp();
    SSAP_Link *link = CreateTestLink();
    g_nlSsapsDevType = CM_DEVTYPE_OLD;
    uint8_t req[] = {0x04, 0x09, 0x01, 0x00, 0xFF, 0x00}; // 首要服务+自定义条目类型
    uint8_t rsp[32] = {0x05, 0x0B,
        0x01, 0x10, 0x00, 0x11, 0x00, 0x02, 0x01, 0x02,
        0x81, 0x12, 0x00, 0x12, 0x00};
    (void)memcpy_s(rsp + 15, SSAP_UUID128_LEN, R_SVC_CUS_UUID, SSAP_UUID128_LEN);
    rsp[31] = 0x00;
    SendFindReq(link, req, sizeof(req));
    EXPECT_TRUE(RspEquals(rsp, sizeof(rsp)));
    DeleteTestLink();
}
