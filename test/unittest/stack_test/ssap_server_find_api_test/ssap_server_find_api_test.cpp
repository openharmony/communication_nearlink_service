/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "stack_schedule_mock.h"
#include "stack_schedule_stub.h"
#include "stack_cm_mock.h"
#include "stack_cm_stub.h"
#include "stack_dtap_mock.h"
#include "stack_dtap_stub.h"

#include "sdf_addr.h"
#include "sdf_mem.h"

#include "cm_def.h"
#include "dtap_tcid.h"

#include "ssap_link.h"
#include "ssap_manager.h"
#include "nlstk_ssap_app_server.h"
#include "nlstk_ssap_app_link.h"
#include "ssaps_server_api.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

// 静态变量初始化
static SLE_Addr_S g_addr = {.type = PUBLIC_ADDRESS, .addr = {0x03, 0x03, 0x04, 0x04, 0x05, 0x05}};
static uint16_t g_lcid = 1;
static int32_t g_appId = -1;

// 星闪标准UUID为两字节，如0x0102，协议栈会将其扩展为16字节UUID，扩展规则为：0x37BEA880FC7011EAB7200000000000****
static NLSTK_SsapUuid_S g_serviceUuid = {.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02}};
static NLSTK_SsapUuid_S g_serviceCustomUuid = {.uuid = {0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11}};   // 自定义服务UUID
static NLSTK_SsapUuid_S g_propertyUuid = {.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x02, 0x03}};
static NLSTK_SsapUuid_S g_propertyCustomUuid = {.uuid = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10}};   // 自定义属性UUID
static NLSTK_SsapUuid_S g_methodUuid = {.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x02, 0x04}};


static int32_t g_onAddServiceResult = 1;

static SSAP_Service_S *g_serviceInTestCase = NULL;

static uint8_t g_propDescData[2] = {0x00, 0x00};
static NLSTK_SsapServiceDescriptorParam_S g_propDesc = {
    .permission = {{{0}}},
    .type = DESC_TYPE_PROPERTY_INSTRUCTION, // 属性说明描述符
    .operation = {{{0}}},
    .val = {
        .len = 2,
        .data = g_propDescData,
    },
};

static uint8_t g_propertyData[3] = {0x44, 0x55, 0x66};              // 标准UUID属性数据
static uint8_t g_customUuidPropertyData[3] = {0x77, 0x88, 0x99};    // 自定义UUID属性数据
static NLSTK_SsapServicePropertyParam_S g_property = {
    // 属性UUID
    .uuid = g_propertyUuid,
    .permission = {{{0}}},
    .operation = {
        .operationValue = 0x0000,
    },
    .val =
        {
            .len = 3,
            .data = g_propertyData,
        },
    .descriptorNum = 1,
    .descriptors = &g_propDesc,
};
static NLSTK_SsapServiceMethodParam g_method = {
    .type = ITEM_TYPE_STD_METHOD,
    .uuid = g_methodUuid,
    .permission = {{{0}}},
};

static uint8_t g_servDescData[2] = {0x00, 0x00};
static NLSTK_SsapServiceDescriptorParam_S g_servDesc = {
    .permission = {{{0}}},
    .type = DESC_TYPE_PROPERTY_RESERVE,
    .operation = {{{0}}},
    .val = {
        .len = 2,
        .data = g_servDescData,
    },
};
static NLSTK_ServiceParam_S g_serviceParam = {
    .serviceStatement = {.uuid = g_serviceUuid,    // service uuid
                         .serviceType = ITEM_TYPE_STD_PRIMARY_SERVICE,
                         .descriptorNum = 1,
                         .descriptors = &g_servDesc},
    .serviceReferenceNum = 0,
    .serviceReference = NULL,
    .servicePropertyNum = 1,
    .property = &g_property,

    .serviceMethodNum = 1,
    .method = &g_method,
    .serviceEventNum = 0,
    .event = NULL,
};

// 自定义UUID的属性
static NLSTK_SsapServicePropertyParam_S g_customUuidProperty = {
    // 属性UUID
    .uuid = g_propertyCustomUuid,
    .permission = {{{0}}},
    .operation = {
        .operationValue = 0x0000,
    },
    .val =
        {
            .len = 3,
            .data = g_customUuidPropertyData,
        },
    .descriptorNum = 1,
    .descriptors = &g_propDesc,
};

// 自定义UUID的服务
static NLSTK_ServiceParam_S g_customUuidServiceParam = {
    .serviceStatement = {.uuid = g_serviceCustomUuid,    // service uuid
                         .serviceType = ITEM_TYPE_STD_PRIMARY_SERVICE,
                         .descriptorNum = 1,
                         .descriptors = &g_servDesc},
    .serviceReferenceNum = 0,
    .serviceReference = NULL,
    .servicePropertyNum = 1,
    .property = &g_customUuidProperty,

    .serviceMethodNum = 0,
    .method = NULL,
    .serviceEventNum = 0,
    .event = NULL,
};

/* 服务的句柄可用范围为0x0010-0xFFFF，这里服务中只有一个属性，最终分配的属性句柄为0x0011 */
// {SSAP_FIND_STRUCTURE_REQ(1B), RFU(2b) 服务端响应一次(0)，标准条目(00), 首要服务(001),起始句柄(2B),结束句柄(2B)}
static uint8_t reqPkt1[] = {0x04, 0x01, 0x01, 0x00, 0xFF, 0x00};
// {SSAP_FIND_STRUCTURE_RSP(1B), RFU(4bits) 标准条目(00)，单个完整数据包(11),{开始句柄-2,结束句柄-2,uuid-2,member-1}}
static uint8_t rspPkt1[] = {0x05, 0x03, 0x10, 0x00, 0x12, 0x00, 0x02, 0x01, 0x06};

// {SSAP_FIND_STRUCTURE_REQ(1B),RFU(2b) 服务端响应一次(0)，标准条目(00), 属性(011),起始句柄(2B),结束句柄(2B)}
static uint8_t reqPkt2[] = {0x04, 0x03, 0x01, 0x00, 0xFF, 0x00};
// {SSAP_FIND_STRUCTURE_RSP(1B), RFU(4bits) 标准条目(00)，单个完整数据包(11),
//     {开始句柄-2,uuid-2,operation-4,count-1,descriptor-x}}
static uint8_t rspPkt2[] = {0x05, 0x03, 0x11, 0x00, 0x03, 0x02, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01};

// find,op-1,ctrl属性混合模式-1,handle-2,handle-2,属性uuid-2
static uint8_t reqPkt3[] = {0x06, 0x13, 0x01, 0x00, 0xFF, 0x00, 0x03, 0x02};
// SSAP_FIND_BY_UUID_RSP(1B),RFU(4b),混合(10),单个完整数据包(11),
//     {信息指示-1,{开始句柄-2,uuid-2,operation-4,count-1,descriptor-x},...}
static uint8_t rspPkt3[] = {0x07, 0x0B, 0x01, 0x11, 0x00, 0x03, 0x02, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x01, 0x80};
// SSAP_FIND_BY_UUID_RSP_OLD(1B),RFU(4b),标准(00),单个完整数据包(11),
//     {属性句柄-2,(老协议没有uuid-2,)operation-4,count-1,descriptor-x}
static uint8_t rspPkt3Old[] = {0x07, 0x03, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01};

// {SSAP_FIND_BY_UUID_REQ(1B),RFU(4b) 服务端响应一次(0)，混合(10), 属性(011),起始句柄(2B),结束句柄(2B),属性uuid-16}
static uint8_t reqPkt4[] = {0x06, 0x13, 0x01, 0x00, 0xFF, 0x00, 0x10, 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A,
    0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0X01};
// SSAP_FIND_BY_UUID_RSP(1B),RFU(4b),混合(10),单个完整数据包(11),{指示-1(标准属性(0),0个(000 0000))}
//     {指示-1(自定义属性(1),1个(000 0001)),{自定义属性句柄-2,uuid-2,operation-4,count-1,descriptor-x},...}
static uint8_t rspPkt4[] = {0x07, 0x0B, 0X00, 0X81, 0X11, 0X00, 0x10, 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A,
    0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0X01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01};
// SSAP_FIND_BY_UUID_RSP(1B)-HISI_OLD,RFU(4b),混合(10),单个完整数据包(11),{指示-1(标准属性(0),0个(000 0000))}
//     {指示-1(自定义属性(1),1个(000 0001)),{自定义属性句柄-2,(!没有UUID),operation-4,desc_count-1,descriptor-x},...}
static uint8_t rspPkt4Old[] = {0x07, 0x0B, 0X00, 0X81, 0X11, 0X00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01};

// {SSAP_FIND_BY_UUID_REQ(1B),RFU(4b) 服务端响应一次(0)，混合(10), 首要服务(001),起始句柄(2B),结束句柄(2B),属性uuid-16}
static uint8_t reqPkt5[] = {0x06, 0x11, 0x01, 0x00, 0xFF, 0x00, 0x11, 0x10, 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A,
    0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02};
// SSAP_FIND_BY_UUID_RSP(1B)-HISI_OLD,RFU(4b),混合(10),单个完整数据包(11),{指示-1(标准服务(0),0个(000 0000))}
//     {指示-1(自定义服务(1),1个(000 0001)),起始句柄-2，结束句柄-2，存在属性}
static uint8_t rspPkt5Old[] = {0x07, 0x0B, 0X00, 0X81, 0X10, 0X00, 0x11, 0x00, 0x02};

// {SSAP_FIND_BY_UUID_REQ(1B),RFU(4b) 服务端响应一次(0)，标准(00), 首要服务(001),起始句柄(2B),结束句柄(2B),服务的uuid-2}
static uint8_t reqPkt6[] = {0x06, 0x01, 0x01, 0x00, 0xFF, 0x00, 0x02, 0x01};
// SSAP_FIND_BY_UUID_RSP-OLD(1B),RFU(4b),标准(00),单个完整数据包(11),
//     {起始句柄-2，结束句柄-2，(没有UUID-2，)member-1(存在属性)}
static uint8_t rspPkt6Old[] = {0x07, 0x03, 0x10, 0x00, 0x12, 0x00, 0x06};

static uint8_t findMethodStandard[] = {0x04, 0x04, 0x01, 0x00, 0xFF, 0x00};
static uint8_t findMethodMix[] = {0x04, 0x14, 0x01, 0x00, 0xFF, 0x00};
static uint8_t findMethodStandardByUuid[] = {0x06, 0x04, 0x01, 0x00, 0xFF, 0x00, 0x04, 0x02};
static uint8_t findMethodMixByUuid[] = {0x06, 0x14, 0x01, 0x00, 0xFF, 0x00, 0x04, 0x02};
static uint8_t findMethodStandardByServiceUuid[] = {0x06, 0x04, 0x01, 0x00, 0xFF, 0x00, 0x02, 0x01};
static uint8_t findMethodMixByServiceUuid[] = {0x06, 0x14, 0x01, 0x00, 0xFF, 0x00, 0x02, 0x01};

static uint8_t findMethodStandardRsp[] = {0x05, 0x03, 0x12, 0x00, 0x04, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00};
static uint8_t findMethodMixRsp[] = {0x05, 0x0B, 0x01, 0x12, 0x00, 0x04, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80};
static uint8_t findMethodStandardByUuidRsp[] = {0x07, 0x03, 0x12, 0x00, 0x04, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00};
static uint8_t findMethodMixByUuidRsp[] = {0x07, 0x0B, 0x01, 0x12, 0x00, 0x04, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80};
static uint8_t findMethodStandardByServiceUuidRsp[] = {0x07, 0x03, 0x12, 0x00, 0x04, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00};
static uint8_t findMethodMixByServiceUuidRsp[] = {0x07, 0x0B, 0x01, 0x12, 0x00, 0x04, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80};

class IT_SSAP_SERVER_FIND : public testing::Test {
public:
    NiceMock<ScheduleMock> scheduleMock;
    NiceMock<CmMock> cmMock;
    NiceMock<DtapMock> dtapMock;
protected:
    void SetUp() override
    {
        // 步骤1：初始化桩函数
        TEST_ScheduleInit();
        EXPECT_CALL(scheduleMock, SchedulePostTask).WillRepeatedly(TEST_SchedulePostTaskStub);
        EXPECT_CALL(scheduleMock, SchedulePostTaskBlocked).WillRepeatedly(TEST_SchedulePostTaskBlockedStub);

        TEST_CM_Init();
        EXPECT_CALL(cmMock, CM_RegLogicLinkListener).WillRepeatedly(TEST_CM_RegLogicLinkListener);
        EXPECT_CALL(cmMock, CM_UnRegLogicLinkListener).WillRepeatedly(TEST_CM_UnRegLogicLinkListener);

        EXPECT_CALL(dtapMock, DTAP_RegisterDataRecvCb).WillRepeatedly(TEST_DTAP_RegisterDataRecvCb);
        EXPECT_CALL(dtapMock, DTAP_UnregisterDataRecvCb).WillRepeatedly(TEST_DTAP_UnregisterDataRecvCb);
        EXPECT_CALL(dtapMock, DTAP_DataSend).WillRepeatedly(TEST_DTAP_DataSend);

        // 步骤2：初始化SSAP模块
        SSAP_LinkInit();
        SSAP_Init();
    }

    void TearDown() override
    {
        // 步骤5：去初始化SSAP模块
        NLSTK_SsapServerDeregisterApplication(g_appId);
        SSAP_DeInit();
        SSAP_LinkDeInit();
    }
};

// 模拟用户创建AppId的时候传入的回调函数，用于接收服务注册的结果
static void Stub_SsapServerAddServiceResult(int32_t appId, SSAP_Service_S *service, NLSTK_Errcode_E ret)
{
    g_appId = appId;
    g_onAddServiceResult = ret;
    g_serviceInTestCase = service;
}

// 完成注册AppId和添加服务并建链的初始化操作
static NLSTK_Errcode_E TestEnvInit(int32_t *appId, NLSTK_ServiceParam_S *serviceParam)
{
    // 步骤1：注册AppId
    NLSTK_SsapAppServerCb_S cb = {0};
    cb.onAddService = Stub_SsapServerAddServiceResult;
    NLSTK_Errcode_E ret = NLSTK_SsapServerRegApp(&cb, appId);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        return NLSTK_ERRCODE_MAX;
    }

    // step2: 添加服务
    ret = NLSTK_SsapServerAddService(*appId, serviceParam);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        return NLSTK_ERRCODE_MAX;
    }

    // step3: 建链
    CM_LogicLinkState_S linkState = {.lcid = g_lcid, .result = 0, };
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);
    return NLSTK_ERRCODE_SUCCESS;
}

/**
 * @test FIND_PRIMARY_SERVICE_RSP
 * @brief 此测试用例用于验证服务端接收FIND_STRUCTURE_REQ报文并解析处理报文的正确性。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 调用NLSTK_SsapServerRegApp注册应用，获取appId。
 * 2. 调用NLSTK_SsapServerAddService添加服务。
 * 3. 模拟服务端接收到FIND_STRUCTURE_REQ报文，并解析报文，构造响应报文并发送。
 * 4. 校验发送报文是否符合预期。
 *
 * @pre
 * - SSAP服务端已初始化。
 *
 * @post
 * - 预期结果：
 * 1. 成功注册应用，获取到有效的appId。
 * 2. 成功添加服务。
 * 3. 服务端成功处理报文，并将查找到的值发送给客户端
 * 4. 发送的报文符合预期。
 */
TEST_F(IT_SSAP_SERVER_FIND, FIND_PRIMARY_SERVICE_RSP)
{
    // 步骤1：注册AppId并建链
    int32_t appId = -1;
    NLSTK_Errcode_E ret = TestEnvInit(&appId, &g_serviceParam);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);

    // step2: 模拟服务端接收到FIND_STRUCTURE_REQ报文，服务端会根据操作码(0x04)进入对应处理函数SSAPS_FindReqHandle
    // 这里查找的是首要服务
    DTAP_Data_Info_S dtapInfo = {.lcid = g_lcid};
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(reqPkt1));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(reqPkt1));
    (void)memcpy_s(tmpBuf, sizeof(reqPkt1), reqPkt1, sizeof(reqPkt1));
    TEST_DTAP_RecData(TCID_SLE_SMTC, &dtapInfo, tmp);

    // step3: 校验发送报文是否符合预期
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(rspPkt1, sizeof(rspPkt1)));

    ret = NLSTK_SsapServerRemoveService(appId, g_serviceInTestCase->handle);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);

    SDF_BuffFree(tmp);
}

TEST_F(IT_SSAP_SERVER_FIND, FIND_PROPERTY_RSP)
{
    // step1：注册AppId
    int32_t appId = -1;
    NLSTK_Errcode_E ret = TestEnvInit(&appId, &g_serviceParam);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);

    // step2: 模拟服务端接收到FIND_STRUCTURE_REQ报文，服务端会根据操作码(0x04)进入对应处理函数SSAPS_FindReqHandle
    // 这里查找的是属性
    DTAP_Data_Info_S dtapInfo = {.lcid = g_lcid};
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(reqPkt2));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(reqPkt2));
    (void)memcpy_s(tmpBuf, sizeof(reqPkt2), reqPkt2, sizeof(reqPkt2));
    TEST_DTAP_RecData(TCID_SLE_SMTC, &dtapInfo, tmp);

    // step3: 校验发送报文是否符合预期
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(rspPkt2, sizeof(rspPkt2)));

    ret = NLSTK_SsapServerRemoveService(appId, g_serviceInTestCase->handle);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);

    SDF_BuffFree(tmp);
}

TEST_F(IT_SSAP_SERVER_FIND, FIND_METHOD_RSP)
{
    // step1：注册AppId
    int32_t appId = -1;
    NLSTK_Errcode_E ret = TestEnvInit(&appId, &g_serviceParam);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);

    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, findMethodStandard, sizeof(findMethodStandard));
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(findMethodStandardRsp, sizeof(findMethodStandardRsp)));

    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, findMethodMix, sizeof(findMethodMix));
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(findMethodMixRsp, sizeof(findMethodMixRsp)));

    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, findMethodStandardByUuid, sizeof(findMethodStandardByUuid));
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(findMethodStandardByUuidRsp, sizeof(findMethodStandardByUuidRsp)));

    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, findMethodMixByUuid, sizeof(findMethodMixByUuid));
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(findMethodMixByUuidRsp, sizeof(findMethodMixByUuidRsp)));

    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, findMethodStandardByServiceUuid, sizeof(findMethodStandardByServiceUuid));
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(findMethodStandardByServiceUuidRsp, sizeof(findMethodStandardByServiceUuidRsp)));

    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, findMethodMixByServiceUuid, sizeof(findMethodMixByServiceUuid));
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(findMethodMixByServiceUuidRsp, sizeof(findMethodMixByServiceUuidRsp)));

    ret = NLSTK_SsapServerRemoveService(appId, g_serviceInTestCase->handle);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
}

TEST_F(IT_SSAP_SERVER_FIND, FIND_PROPERTY_BY_STD_UUID_RSP_MIX)
{
    // step1：注册AppId
    int32_t appId = -1;
    NLSTK_Errcode_E ret = TestEnvInit(&appId, &g_serviceParam);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);

    // step2: 模拟服务端接收到FIND_STRUCTURE_REQ报文，服务端会根据操作码(0x06)进入对应处理函数SSAPS_FindReqHandle
    // 这里是在混合模式下查找标准两字节UUID的属性
    DTAP_Data_Info_S dtapInfo = {.lcid = g_lcid};
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(reqPkt3));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(reqPkt3));
    (void)memcpy_s(tmpBuf, sizeof(reqPkt3), reqPkt3, sizeof(reqPkt3));
    TEST_DTAP_RecData(TCID_SLE_SMTC, &dtapInfo, tmp);

    // step3: 校验发送报文是否符合预期
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(rspPkt3, sizeof(rspPkt3)));

    ret = NLSTK_SsapServerRemoveService(appId, g_serviceInTestCase->handle);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);

    SDF_BuffFree(tmp);
}

TEST_F(IT_SSAP_SERVER_FIND, FIND_PROPERTY_BY_CUSTOM_UUID_RSP_MIX)
{
    // step1：注册AppId
    int32_t appId = -1;
    NLSTK_Errcode_E ret = TestEnvInit(&appId, &g_customUuidServiceParam);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);

    // step2: 模拟服务端接收到FIND_STRUCTURE_REQ报文，服务端会根据操作码(0x06)进入对应处理函数SSAPS_FindReqHandle
    // 这里是在混合模式下查找自定义十六字节UUID的属性
    DTAP_Data_Info_S dtapInfo = {.lcid = g_lcid};
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(reqPkt4));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(reqPkt4));
    (void)memcpy_s(tmpBuf, sizeof(reqPkt4), reqPkt4, sizeof(reqPkt4));
    TEST_DTAP_RecData(TCID_SLE_SMTC, &dtapInfo, tmp);

    // step3: 校验发送报文是否符合预期
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(rspPkt4, sizeof(rspPkt4)));

    ret = NLSTK_SsapServerRemoveService(appId, g_serviceInTestCase->handle);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);

    SDF_BuffFree(tmp);
}

TEST_F(IT_SSAP_SERVER_FIND, SERVER_CLEAN_APP)
{
    // 步骤1：注册AppId并建链
    int32_t appId = -1;
    NLSTK_Errcode_E ret = TestEnvInit(&appId, &g_serviceParam);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);

    NLSTK_SsapServerCleanAppResultCb serverCleanAppResultCb = {0};
    ret = NLSTK_SsapCleanServerApp(serverCleanAppResultCb);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);

    // 断链
    CM_LogicLinkState_S linkState = {.lcid = g_lcid, .result = 2, };
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);

    ret = NLSTK_SsapCleanServerApp(serverCleanAppResultCb);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
}