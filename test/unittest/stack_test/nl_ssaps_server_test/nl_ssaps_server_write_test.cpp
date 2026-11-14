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
#include "sdf_buff.h"
#include "sdf_mem.h"
#include "cpfwk_log.h"

#include "stack_schedule_mock.h"
#include "stack_cm_mock.h"
#include "stack_dtap_mock.h"

using namespace testing;

#define TEST_MAX_BUF_CACHE 2048

static uint8_t g_rspCache[TEST_MAX_BUF_CACHE] = {0};
static uint16_t g_rspLen = 0;

static NLSTK_SsapUuid_S g_svcUuid = {.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02}};
static NLSTK_SsapUuid_S g_propUuid = {.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x02, 0x03}};

static SLE_Addr_S g_localAddr = {.type = PUBLIC_ADDRESS, .addr = {0x03, 0x03, 0x04, 0x04, 0x05, 0x05}};
static uint16_t g_lcid = 1;

// 固定六属性布局的句柄
#define PROP_DATA_W 0x11        // 可读写（带响应与无响应两种写指示）
#define PROP_READ_ONLY 0x12     // 只读
#define PROP_CPCD 0x13          // 带客户端配置描述符
#define PROP_NEED_AUTHZ 0x14    // 需要授权
#define PROP_DATA_W2 0x15       // 第二个可读写属性
#define PROP_WITH_DESCS 0x16    // 带属性说明与属性值格式描述符

class NlSsapsWriteTest : public testing::Test {
public:
    NiceMock<ScheduleMock> scheduleMock;
    NiceMock<CmMock> cmMock;
    NiceMock<DtapMock> dtapMock;
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

static void CacheProperty(uint8_t perm, uint32_t operation, uint8_t v0, uint8_t v1)
{
    SSAP_ParamAddProperty_S *prop =
        (SSAP_ParamAddProperty_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddProperty_S) + 2);
    (void)memcpy_s(&prop->uuid, sizeof(NLSTK_SsapUuid_S), &g_propUuid, sizeof(NLSTK_SsapUuid_S));
    prop->val.len = 2;
    prop->val.value[0] = v0;
    prop->val.value[1] = v1;
    prop->permission.permissionValue = perm;
    prop->operation.operationValue = operation;
    SSAP_CacheProperty(prop);
    SDF_MemFree(prop);
}

static void CacheDescriptor(uint8_t descType, uint32_t operation, uint8_t v0, uint8_t v1)
{
    SSAP_ParamAddDescriptor_S *desc =
        (SSAP_ParamAddDescriptor_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddDescriptor_S) + 2);
    desc->type = descType;
    desc->val.len = 2;
    desc->val.value[0] = v0;
    desc->val.value[1] = v1;
    desc->operation.operationValue = operation;
    SSAP_CacheDescriptor(desc);
    SDF_MemFree(desc);
}

// 一个标准服务挂六个属性：可写、只读、CPCD、需授权、第二可写、双描述符
static void AddWriteService()
{
    SSAP_ParamAddService_S *svc = (SSAP_ParamAddService_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddService_S));
    svc->serviceType = ITEM_TYPE_STD_PRIMARY_SERVICE;
    (void)memcpy_s(&svc->uuid, sizeof(NLSTK_SsapUuid_S), &g_svcUuid, sizeof(NLSTK_SsapUuid_S));
    SSAP_CacheService(svc);
    SDF_MemFree(svc);

    CacheProperty(0, SSAP_OPERATE_INDICATION_WRITE | SSAP_OPERATE_INDICATION_WRITE_NO_RSP, 0x11, 0x22);
    CacheProperty(0, SSAP_OPERATE_INDICATION_READ, 0x00, 0x00);
    CacheProperty(0, SSAP_OPERATE_INDICATION_WRITE, 0x00, 0x00);
    CacheDescriptor(DESC_TYPE_CLIENT_CONFIG, SSAP_OPERATE_INDICATION_DESCRIPTOR_CLIENT_CONFIGURATION_WRITE,
        0x00, 0x00);
    CacheProperty(SSAP_PERMISSION_AUTHORIZATION_NEED, SSAP_OPERATE_INDICATION_WRITE, 0x00, 0x00);
    CacheProperty(0, SSAP_OPERATE_INDICATION_WRITE | SSAP_OPERATE_INDICATION_WRITE_NO_RSP, 0x33, 0x44);
    CacheProperty(0, SSAP_OPERATE_INDICATION_WRITE, 0x55, 0x66);
    CacheDescriptor(DESC_TYPE_PROPERTY_INSTRUCTION, SSAP_OPERATE_INDICATION_DESCRITOR_WRITE, 0xEE, 0xEE);
    CacheDescriptor(DESC_TYPE_PROPERTY_FORMAT, 0, 0x00, 0x00);
    SSAP_StartService(nullptr);
}

static void SendPdu(SSAP_Link *link, void (*handler)(SSAP_Link *, SDF_Buff_S *), const uint8_t *pdu,
    uint16_t len)
{
    g_rspLen = 0;
    SDF_Buff_S *buff = SDF_BuffNewWithReserve(len);
    uint8_t *buf = SDF_BuffAppend(buff, len);
    (void)memcpy_s(buf, len, pdu, len);
    handler(link, buff);
    SDF_BuffFree(buff);
}

static bool RspEquals(const uint8_t *expected, uint16_t len)
{
    return g_rspLen == len && memcmp(g_rspCache, expected, len) == 0;
}

static bool PropValueEquals(uint16_t handle, const uint8_t *expected, uint16_t len)
{
    SSAP_Property_S *prop = SSAPS_GetPropertyByHandle(handle);
    if (prop == nullptr || prop->val == nullptr || prop->val->len != len) {
        return false;
    }
    return memcmp(prop->val->value, expected, len) == 0;
}

// 单值写成功：数据域被更新并返回写入成功响应
TEST_F(NlSsapsWriteTest, SingleWriteUpdatesValueAndSuccessRsp)
{
    AddWriteService();
    SSAP_Link *link = CreateTestLink();
    uint8_t req[] = {0x0D, 0x03, 0x11, 0x00, 0x00, 0xAA, 0xBB};
    uint8_t rsp[] = {0x0E, 0x03};
    SendPdu(link, SSAPS_WriteReqHandle, req, sizeof(req));
    EXPECT_TRUE(RspEquals(rsp, sizeof(rsp)));
    uint8_t updated[2] = {0xAA, 0xBB};
    EXPECT_TRUE(PropValueEquals(PROP_DATA_W, updated, sizeof(updated)));
    DeleteTestLink();
}

// 只读属性写入被拒绝：返回禁止写入错误
TEST_F(NlSsapsWriteTest, SingleWriteForbiddenByOperation)
{
    AddWriteService();
    SSAP_Link *link = CreateTestLink();
    uint8_t req[] = {0x0D, 0x03, 0x12, 0x00, 0x00, 0x01, 0x02};
    uint8_t rsp[] = {0x0E, 0x07, 0x01, 0x12, 0x00, 0x07};
    SendPdu(link, SSAPS_WriteReqHandle, req, sizeof(req));
    EXPECT_TRUE(RspEquals(rsp, sizeof(rsp)));
    DeleteTestLink();
}

// 写属性上不存在的描述符类型：返回数据类型错误
TEST_F(NlSsapsWriteTest, SingleWriteDescTypeNotFound)
{
    AddWriteService();
    SSAP_Link *link = CreateTestLink();
    uint8_t req[] = {0x0D, 0x03, 0x11, 0x00, 0x03, 0x01, 0x02};
    uint8_t rsp[] = {0x0E, 0x07, 0x01, 0x11, 0x00, 0x0D};
    SendPdu(link, SSAPS_WriteReqHandle, req, sizeof(req));
    EXPECT_TRUE(RspEquals(rsp, sizeof(rsp)));
    DeleteTestLink();
}

// 超出协议定义的类型：返回PDU不支持
TEST_F(NlSsapsWriteTest, SingleWriteUnsupportedType)
{
    AddWriteService();
    SSAP_Link *link = CreateTestLink();
    uint8_t req[] = {0x0D, 0x03, 0x11, 0x00, 0x06, 0x01, 0x02};
    uint8_t rsp[] = {0x01, 0x00, 0x0D, 0x11, 0x00, 0x02};
    SendPdu(link, SSAPS_WriteReqHandle, req, sizeof(req));
    EXPECT_TRUE(RspEquals(rsp, sizeof(rsp)));
    DeleteTestLink();
}

// 属性值格式描述符没有写指示位：写该描述符落入默认分支返回禁止写入
TEST_F(NlSsapsWriteTest, SingleWriteDescWithoutIndicationForbidden)
{
    AddWriteService();
    SSAP_Link *link = CreateTestLink();
    uint8_t req[] = {0x0D, 0x03, PROP_WITH_DESCS, 0x00, 0x04, 0x01, 0x02};
    uint8_t rsp[] = {0x0E, 0x07, 0x01, PROP_WITH_DESCS, 0x00, 0x07};
    SendPdu(link, SSAPS_WriteReqHandle, req, sizeof(req));
    EXPECT_TRUE(RspEquals(rsp, sizeof(rsp)));
    DeleteTestLink();
}

// 写属性说明描述符（有写指示位）：成功
TEST_F(NlSsapsWriteTest, SingleWritePropertyInstructionDesc)
{
    AddWriteService();
    SSAP_Link *link = CreateTestLink();
    uint8_t req[] = {0x0D, 0x03, PROP_WITH_DESCS, 0x00, 0x01, 0x0D, 0x0E};
    uint8_t rsp[] = {0x0E, 0x03};
    SendPdu(link, SSAPS_WriteReqHandle, req, sizeof(req));
    EXPECT_TRUE(RspEquals(rsp, sizeof(rsp)));
    DeleteTestLink();
}

// 写客户端配置描述符：按对端地址建立副本，再写同地址更新副本值
TEST_F(NlSsapsWriteTest, SingleWriteCpcdCreateAndUpdate)
{
    AddWriteService();
    SSAP_Link *link = CreateTestLink();
    uint8_t req1[] = {0x0D, 0x03, PROP_CPCD, 0x00, 0x02, 0x01, 0x00};
    uint8_t rspOk[] = {0x0E, 0x03};
    SendPdu(link, SSAPS_WriteReqHandle, req1, sizeof(req1));
    EXPECT_TRUE(RspEquals(rspOk, sizeof(rspOk)));

    SSAP_Property_S *prop = SSAPS_GetPropertyByHandle(PROP_CPCD);
    uint8_t err = 0;
    SSAP_LengthValue_S *val = SSAPS_GetPropertyValue(link, prop, DESC_TYPE_CLIENT_CONFIG, &err);
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(val->len, 2);
    EXPECT_EQ(val->value[0], 0x01);

    uint8_t req2[] = {0x0D, 0x03, PROP_CPCD, 0x00, 0x02, 0x02, 0x00};
    SendPdu(link, SSAPS_WriteReqHandle, req2, sizeof(req2));
    EXPECT_TRUE(RspEquals(rspOk, sizeof(rspOk)));
    val = SSAPS_GetPropertyValue(link, prop, DESC_TYPE_CLIENT_CONFIG, &err);
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(val->value[0], 0x02);
    DeleteTestLink();
}

// 原值返回校验：成功响应回显句柄、类型与写入值
TEST_F(NlSsapsWriteTest, SingleWriteVerifyEcho)
{
    AddWriteService();
    SSAP_Link *link = CreateTestLink();
    uint8_t req[] = {0x0D, 0x23, 0x11, 0x00, 0x00, 0xCC, 0xDD};
    uint8_t rsp[] = {0x0E, 0x03, 0x11, 0x00, 0x00, 0xCC, 0xDD};
    SendPdu(link, SSAPS_WriteReqHandle, req, sizeof(req));
    EXPECT_TRUE(RspEquals(rsp, sizeof(rsp)));
    DeleteTestLink();
}

// 原值回显超MTU且对端不支持分包：改报服务端不支持分包错误
TEST_F(NlSsapsWriteTest, SingleWriteVerifyEchoOverMtuNoFrag)
{
    AddWriteService();
    SSAP_Link *link = CreateTestLink();
    link->mtu = 200; // 链路默认MTU为协议最大值，压低后255字节回显才超限
    uint8_t req[2 + 3 + 250] = {0x0D, 0x23, 0x11, 0x00, 0x00};
    for (uint16_t i = 0; i < 250; i++) {
        req[5 + i] = (uint8_t)i;
    }
    SendPdu(link, SSAPS_WriteReqHandle, req, sizeof(req));
    uint8_t rsp[] = {0x0E, 0x07, 0x01, 0x11, 0x00, 0x10};
    EXPECT_TRUE(RspEquals(rsp, sizeof(rsp)));
    DeleteTestLink();
}

// 先MTU交换打开对端分包能力：超MTU回显按完整报文发出
TEST_F(NlSsapsWriteTest, SingleWriteVerifyEchoOverMtuWithFrag)
{
    AddWriteService();
    SSAP_Link *link = CreateTestLink();
    uint8_t exchangeReq[] = {0x02, 0x13, 0xFB, 0x00, 0x01, 0x03};
    SendPdu(link, SSAPS_ExchangeInfoReqHandle, exchangeReq, sizeof(exchangeReq));

    uint8_t req[2 + 3 + 250] = {0x0D, 0x23, 0x11, 0x00, 0x00};
    for (uint16_t i = 0; i < 250; i++) {
        req[5 + i] = (uint8_t)i;
    }
    SendPdu(link, SSAPS_WriteReqHandle, req, sizeof(req));
    uint8_t rsp[2 + 3 + 250] = {0x0E, 0x03, 0x11, 0x00, 0x00};
    for (uint16_t i = 0; i < 250; i++) {
        rsp[5 + i] = (uint8_t)i;
    }
    EXPECT_TRUE(RspEquals(rsp, sizeof(rsp)));
    DeleteTestLink();
}

// 立即取消写入：返回取消结果响应
TEST_F(NlSsapsWriteTest, WriteReqCancelRsp)
{
    AddWriteService();
    SSAP_Link *link = CreateTestLink();
    uint8_t req[] = {0x0D, 0x13, 0x00};
    uint8_t rsp[] = {0x0E, 0x0B};
    SendPdu(link, SSAPS_WriteReqHandle, req, sizeof(req));
    EXPECT_TRUE(RspEquals(rsp, sizeof(rsp)));
    DeleteTestLink();
}

// 分片写中途回单包取消：清掉已缓存分片并返回取消结果
TEST_F(NlSsapsWriteTest, WriteFragBeginThenCancel)
{
    AddWriteService();
    SSAP_Link *link = CreateTestLink();
    uint8_t fragBegin[] = {0x0D, 0x00, 0x11, 0x00, 0x00, 0xAA};
    SendPdu(link, SSAPS_WriteReqHandle, fragBegin, sizeof(fragBegin));
    EXPECT_EQ(g_rspLen, 0); // 分片未组完不产生响应

    uint8_t cancel[] = {0x0D, 0x13, 0x00};
    uint8_t rsp[] = {0x0E, 0x0B};
    SendPdu(link, SSAPS_WriteReqHandle, cancel, sizeof(cancel));
    EXPECT_TRUE(RspEquals(rsp, sizeof(rsp)));
    DeleteTestLink();
}

// 写命令无响应：值被更新但不产生任何报文
TEST_F(NlSsapsWriteTest, WriteCmdUpdatesSilently)
{
    AddWriteService();
    SSAP_Link *link = CreateTestLink();
    uint8_t cmd[] = {0x0C, 0x03, 0x11, 0x00, 0x00, 0x99};
    SendPdu(link, SSAPS_WriteCmdHandle, cmd, sizeof(cmd));
    EXPECT_EQ(g_rspLen, 0);
    uint8_t updated[1] = {0x99};
    EXPECT_TRUE(PropValueEquals(PROP_DATA_W, updated, sizeof(updated)));
    DeleteTestLink();
}

// 写命令控制码不支持（继续传输操作）：静默丢弃
TEST_F(NlSsapsWriteTest, WriteCmdBadOperSilent)
{
    AddWriteService();
    SSAP_Link *link = CreateTestLink();
    uint8_t cmd[] = {0x0C, 0x08, 0x00};
    SendPdu(link, SSAPS_WriteCmdHandle, cmd, sizeof(cmd));
    EXPECT_EQ(g_rspLen, 0);
    DeleteTestLink();
}

// 多值写带原值返回：每个条目按句柄+子项计数+子项（类型/长度/值）回显
TEST_F(NlSsapsWriteTest, MultiWriteVerifyEchoAllItems)
{
    AddWriteService();
    SSAP_Link *link = CreateTestLink();
    uint8_t req[] = {0x0D, 0x27,
        0x11, 0x00, 0x01, 0x00, 0x02, 0x00, 0xD1, 0xD2,
        PROP_DATA_W2, 0x00, 0x01, 0x00, 0x02, 0x00, 0xE1, 0xE2};
    uint8_t rsp[] = {0x0E, 0x03,
        0x11, 0x00, 0x01, 0x00, 0x02, 0x00, 0xD1, 0xD2,
        PROP_DATA_W2, 0x00, 0x01, 0x00, 0x02, 0x00, 0xE1, 0xE2};
    SendPdu(link, SSAPS_WriteReqHandle, req, sizeof(req));
    EXPECT_TRUE(RspEquals(rsp, sizeof(rsp)));
    DeleteTestLink();
}

// 多值写部分失败不带原值返回：错误列表携带失败句柄与错误码
TEST_F(NlSsapsWriteTest, MultiWritePartialErrorWithoutOrigin)
{
    AddWriteService();
    SSAP_Link *link = CreateTestLink();
    uint8_t req[] = {0x0D, 0x07,
        0x99, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x02,
        0x11, 0x00, 0x01, 0x00, 0x02, 0x00, 0x03, 0x04};
    uint8_t rsp[] = {0x0E, 0x07, 0x01, 0x99, 0x00, 0x04};
    SendPdu(link, SSAPS_WriteReqHandle, req, sizeof(req));
    EXPECT_TRUE(RspEquals(rsp, sizeof(rsp)));
    DeleteTestLink();
}

// 多值写部分失败带原值返回：错误列表之后附成功条目的原值
TEST_F(NlSsapsWriteTest, MultiWritePartialErrorWithOrigin)
{
    AddWriteService();
    SSAP_Link *link = CreateTestLink();
    uint8_t req[] = {0x0D, 0x27,
        0x99, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x02,
        0x11, 0x00, 0x01, 0x00, 0x02, 0x00, 0x03, 0x04};
    uint8_t rsp[] = {0x0E, 0x07, 0x01, 0x99, 0x00, 0x04,
        0x11, 0x00, 0x01, 0x00, 0x02, 0x00, 0x03, 0x04};
    SendPdu(link, SSAPS_WriteReqHandle, req, sizeof(req));
    EXPECT_TRUE(RspEquals(rsp, sizeof(rsp)));
    DeleteTestLink();
}

// 多值写零长度子项：跳过不落库，仅处理非零子项
TEST_F(NlSsapsWriteTest, MultiWriteLenZeroSubItemSkipped)
{
    AddWriteService();
    SSAP_Link *link = CreateTestLink();
    uint8_t req[] = {0x0D, 0x27,
        0x11, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0xF1, 0xF2};
    uint8_t rsp[] = {0x0E, 0x03, 0x11, 0x00, 0x01, 0x00, 0x02, 0x00, 0xF1, 0xF2};
    SendPdu(link, SSAPS_WriteReqHandle, req, sizeof(req));
    EXPECT_TRUE(RspEquals(rsp, sizeof(rsp)));
    DeleteTestLink();
}

// 多值写条目子项计数为零：该条目按无效句柄记错误
TEST_F(NlSsapsWriteTest, MultiWriteSubItemCountZero)
{
    AddWriteService();
    SSAP_Link *link = CreateTestLink();
    uint8_t req[] = {0x0D, 0x07, 0x11, 0x00, 0x00};
    uint8_t rsp[] = {0x0E, 0x07, 0x01, 0x11, 0x00, 0x04};
    SendPdu(link, SSAPS_WriteReqHandle, req, sizeof(req));
    EXPECT_TRUE(RspEquals(rsp, sizeof(rsp)));
    DeleteTestLink();
}

// 多值写携带需授权条目：协议不支持，整包按PDU不支持拒绝且不再回写响应
TEST_F(NlSsapsWriteTest, MultiWriteAuthzItemRejected)
{
    AddWriteService();
    SSAP_Link *link = CreateTestLink();
    uint8_t req[] = {0x0D, 0x07,
        PROP_NEED_AUTHZ, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x02};
    uint8_t rsp[] = {0x01, 0x00, 0x0D, 0x00, 0x00, 0x02};
    SendPdu(link, SSAPS_WriteReqHandle, req, sizeof(req));
    EXPECT_TRUE(RspEquals(rsp, sizeof(rsp)));
    DeleteTestLink();
}

// 多值写命令携带需授权条目：静默中止，不更新任何值
TEST_F(NlSsapsWriteTest, MultiWriteCmdAuthzItemStops)
{
    AddWriteService();
    SSAP_Link *link = CreateTestLink();
    uint8_t cmd[] = {0x0C, 0x07,
        PROP_NEED_AUTHZ, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x02};
    SendPdu(link, SSAPS_WriteCmdHandle, cmd, sizeof(cmd));
    EXPECT_EQ(g_rspLen, 0);
    uint8_t untouched[2] = {0x00, 0x00};
    EXPECT_TRUE(PropValueEquals(PROP_NEED_AUTHZ, untouched, sizeof(untouched)));
    DeleteTestLink();
}

// 多值写命令携带未知类型子项：该条目静默跳过，值不变
TEST_F(NlSsapsWriteTest, MultiWriteCmdBadTypeStops)
{
    AddWriteService();
    SSAP_Link *link = CreateTestLink();
    uint8_t cmd[] = {0x0C, 0x07,
        0x11, 0x00, 0x01, 0x03, 0x02, 0x00, 0x01, 0x02};
    SendPdu(link, SSAPS_WriteCmdHandle, cmd, sizeof(cmd));
    EXPECT_EQ(g_rspLen, 0);
    uint8_t untouched[2] = {0x11, 0x22};
    EXPECT_TRUE(PropValueEquals(PROP_DATA_W, untouched, sizeof(untouched)));
    DeleteTestLink();
}

// 多值写命令零长度子项跳过：仅非零子项生效
TEST_F(NlSsapsWriteTest, MultiWriteCmdLenZeroSubItemSkipped)
{
    AddWriteService();
    SSAP_Link *link = CreateTestLink();
    uint8_t cmd[] = {0x0C, 0x07,
        0x11, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01};
    SendPdu(link, SSAPS_WriteCmdHandle, cmd, sizeof(cmd));
    EXPECT_EQ(g_rspLen, 0);
    uint8_t updated[1] = {0x01};
    EXPECT_TRUE(PropValueEquals(PROP_DATA_W, updated, sizeof(updated)));
    DeleteTestLink();
}

// 多值写命令未知句柄条目：静默跳过，继续后续条目
TEST_F(NlSsapsWriteTest, MultiWriteCmdUnknownHandleSkips)
{
    AddWriteService();
    SSAP_Link *link = CreateTestLink();
    uint8_t cmd[] = {0x0C, 0x07,
        0x99, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x02,
        0x11, 0x00, 0x01, 0x00, 0x02, 0x00, 0x07, 0x08};
    SendPdu(link, SSAPS_WriteCmdHandle, cmd, sizeof(cmd));
    EXPECT_EQ(g_rspLen, 0);
    uint8_t updated[2] = {0x07, 0x08};
    EXPECT_TRUE(PropValueEquals(PROP_DATA_W, updated, sizeof(updated)));
    DeleteTestLink();
}

// 多值写原值回显超MTU且对端不支持分包：整包改报单个不支持分包错误项
TEST_F(NlSsapsWriteTest, MultiWriteVerifyEchoOverMtuNoFrag)
{
    AddWriteService();
    SSAP_Link *link = CreateTestLink();
    link->mtu = 300; // 链路默认MTU为协议最大值，压低后510字节回显才超限
    // 两个条目各带248字节原值：回显总长必然超过MTU
    uint8_t req[2 + 254 + 254] = {0x0D, 0x27,
        0x11, 0x00, 0x01, 0x00, 0xF8, 0x00};
    for (uint16_t i = 0; i < 248; i++) {
        req[8 + i] = (uint8_t)i;
    }
    uint16_t secondOff = 8 + 248;
    req[secondOff] = PROP_DATA_W2;
    req[secondOff + 1] = 0x00;
    req[secondOff + 2] = 0x01;
    req[secondOff + 3] = 0x00;
    req[secondOff + 4] = 0xF8;
    req[secondOff + 5] = 0x00;
    for (uint16_t i = 0; i < 248; i++) {
        req[secondOff + 6 + i] = (uint8_t)i;
    }
    SendPdu(link, SSAPS_WriteReqHandle, req, (uint16_t)sizeof(req));
    uint8_t rsp[] = {0x0E, 0x07, 0x01, 0x11, 0x00, 0x10};
    EXPECT_TRUE(RspEquals(rsp, sizeof(rsp)));
    DeleteTestLink();
}

// 写请求载荷过短：按PDU无效拒绝
TEST_F(NlSsapsWriteTest, WriteReqTooShortRejected)
{
    AddWriteService();
    SSAP_Link *link = CreateTestLink();
    uint8_t req[] = {0x0D, 0x03, 0x00};
    uint8_t rsp[] = {0x01, 0x00, 0x0D, 0x00, 0x00, 0x01};
    SendPdu(link, SSAPS_WriteReqHandle, req, sizeof(req));
    EXPECT_TRUE(RspEquals(rsp, sizeof(rsp)));
    DeleteTestLink();
}
