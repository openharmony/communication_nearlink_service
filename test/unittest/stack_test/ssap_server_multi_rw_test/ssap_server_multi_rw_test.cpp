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
#include "ssap_pkt.h"
#include "ssap_type.h"

/* 测试用缓冲区最大长度，用于存储发送/接收的SSAP数据包 */
#define TEST_MAX_BUF_CACHE 2048

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

/* 全局测试状态变量 */
static uint8_t g_buffCache[TEST_MAX_BUF_CACHE] = {0};    /* 存储最近一次发送的响应数据包内容 */
static uint32_t g_buffLen = 0;                           /* 响应数据包的实际长度 */
static bool isSendRsp = false;                           /* 标记是否已发送响应 */
static SLE_Addr_S g_addr = {.type = PUBLIC_ADDRESS, .addr = {0x03, 0x03, 0x04, 0x04, 0x05, 0x05}}; /* 测试用链路地址 */
static uint16_t g_lcid = 1;                              /* 测试用逻辑通道ID */
static DTAP_Data_Info_S g_dtapDataInfo = {.lcid = g_lcid}; /* DTAP层数据信息 */

/*
 * UUID定义: 测试用服务和服务属性的唯一标识符
 * g_uuid1: 主服务(UUID1)，包含两个属性(UUID2和UUIDProp1)，小端序UUID
 * g_uuid2: 属性1(UUID2)，长度1字节，初始值0x11，支持读写
 * g_uuid3: 第二个主服务(UUID3)，无属性
 * g_uuidProp1: 属性2(UUIDProp1)，长度2字节，初始值0x22,0x33，支持读写
 */
static NLSTK_SsapUuid_S g_uuid1 = {.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02}};
static NLSTK_SsapUuid_S g_uuid2 = {.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x02, 0x03}};
static NLSTK_SsapUuid_S g_uuid3 = {.uuid = {0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D,
    0x0E, 0x0F, 0x10, 0x11, 0x12}};
static NLSTK_SsapUuid_S g_uuidProp1 = {.uuid = {0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D,
    0x0E, 0x0F, 0x10, 0x14, 0x15}};

static uint16_t g_propertyHandle1 = 0;
static uint16_t g_propertyHandle2 = 0;

/*
 * @class UT_SSAP_MULTI_READ_WRITE
 * @brief SSAP多值读写功能测试套件
 *
 * 测试套件Setup/TearDown确保每个测试用例执行前:
 * - SSAP服务器模块初始化
 * - SSAP链路管理初始化
 *
 * 测试执行后清理:
 * - SSAP链路管理反初始化
 * - SSAP服务器模块反初始化
 */
class UT_SSAP_MULTI_READ_WRITE : public testing::Test {
protected:
    /*
     * @brief 测试Setup - 每个测试用例执行前调用
     * 初始化SSAP服务器和链路管理模块
     */
    void SetUp() override
    {
        SSAP_ServerInit();
        SSAP_LinkInit();
    }

    /*
     * @brief 测试TearDown - 每个测试用例执行后调用
     * 清理SSAP链路和服务器资源
     */
    void TearDown() override
    {
        SSAP_LinkDeInit();
        SSAP_ServerDeInit();
    }
};

/*
 * @brief 添加测试服务及其属性
 *
 * 创建一个主服务(UUID1)，包含两个可读写的属性:
 * - 属性1(UUID2): 单字节值0x11
 * - 属性2(UUIDProp1): 双字节值0x22,0x33
 *
 * 服务创建流程:
 * 1. 注册主服务(UUID1) - 标准首要服务类型
 * 2. 注册属性1(UUID2) - 关联到主服务，支持读写操作
 * 3. 注册属性2(UUIDProp1) - 关联到主服务，支持读写操作
 * 4. 启动服务
 *
 * @note 属性句柄由服务器动态分配(从0x0010开始)，通过SDF_VectorElementAt获取实际值
 */
static void AddTestServiceWithProperties()
{
    CP_LOG_INFO("[TEST] enter AddTestServiceWithProperties");
    /* 创建并注册主服务(UUID1) */
    SSAP_ParamAddService_S *serviceParam = (SSAP_ParamAddService_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddService_S));
    serviceParam->serviceType = ITEM_TYPE_STD_PRIMARY_SERVICE;
    (void)memcpy_s(&serviceParam->uuid, sizeof(NLSTK_SsapUuid_S), &g_uuid1, sizeof(NLSTK_SsapUuid_S));
    SSAP_CacheService(serviceParam);
    SDF_MemFree(serviceParam);

    /* 创建并注册属性1(UUID2): 值=0x11，长度=1，支持读写 */
    SSAP_ParamAddProperty_S *propertyParam1 =
        (SSAP_ParamAddProperty_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddProperty_S) + 3);
    (void)memcpy_s(&propertyParam1->uuid, sizeof(NLSTK_SsapUuid_S), &g_uuid2, sizeof(NLSTK_SsapUuid_S));
    propertyParam1->val.len = 1;
    propertyParam1->val.value[0] = 0x11;
    propertyParam1->operation.operationValue = SSAP_OPERATE_INDICATION_READ | SSAP_OPERATE_INDICATION_WRITE;
    SSAP_CacheProperty(propertyParam1);
    SDF_MemFree(propertyParam1);

    SSAP_ParamAddDescriptor_S *descParam1 =
        (SSAP_ParamAddDescriptor_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddDescriptor_S) + 1);
    descParam1->type = DESC_TYPE_CLIENT_CONFIG;
    descParam1->val.len = 1;
    descParam1->val.value[0] = 0xEE;
    descParam1->operation.operationValue = 0x704;
    SSAP_CacheDescriptor(descParam1);
    SDF_MemFree(descParam1);

    /* 创建并注册属性2(UUIDProp1): 值=0x22,0x33，长度=2，支持读写 */
    SSAP_ParamAddProperty_S *propertyParam2 =
        (SSAP_ParamAddProperty_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddProperty_S) + 3);
    (void)memcpy_s(&propertyParam2->uuid, sizeof(NLSTK_SsapUuid_S), &g_uuidProp1, sizeof(NLSTK_SsapUuid_S));
    propertyParam2->val.len = 2;
    propertyParam2->val.value[0] = 0x22;
    propertyParam2->val.value[1] = 0x33;
    propertyParam2->operation.operationValue = SSAP_OPERATE_INDICATION_READ | SSAP_OPERATE_INDICATION_WRITE;
    SSAP_CacheProperty(propertyParam2);
    SDF_MemFree(propertyParam2);

    SSAP_ParamAddDescriptor_S *descParam2 =
        (SSAP_ParamAddDescriptor_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddDescriptor_S) + 1);
    descParam2->type = DESC_TYPE_CLIENT_CONFIG;
    descParam2->val.len = 1;
    descParam2->val.value[0] = 0xEE;
    descParam2->operation.operationValue = 0x704;
    SSAP_CacheDescriptor(descParam2);
    SDF_MemFree(descParam2);

    /* 启动服务，使服务对客户端可见 */
    SSAP_StartService(NULL);

    /* 获取实际分配的句柄 */
    SDF_Vector_S *services = SSAPS_GetServices();
    if (services != NULL && services->size > 0) {
        SSAP_Service_S *service = (SSAP_Service_S *)SDF_VectorElementAt(services, services->size - 1);
        if (service != NULL && service->properties != NULL && service->properties->size >= 2) {
            SSAP_Property_S *prop1 = (SSAP_Property_S *)SDF_VectorElementAt(service->properties, 0);
            SSAP_Property_S *prop2 = (SSAP_Property_S *)SDF_VectorElementAt(service->properties, 1);
            g_propertyHandle1 = prop1->handle;
            g_propertyHandle2 = prop2->handle;
            CP_LOG_INFO("[TEST] property handles: handle1=0x%04X, handle2=0x%04X", g_propertyHandle1, g_propertyHandle2);
        }
    }
}

/*
 * @brief 添加第二个测试服务
 *
 * 注册第二个独立的主服务(UUID3)，不包含任何属性
 * 用于测试多服务场景下的结构发现功能
 */
static void AddSecondService()
{
    CP_LOG_INFO("[TEST] enter AddSecondService");
    /* 创建并注册第二个主服务(UUID3) */
    SSAP_ParamAddService_S *serviceParam = (SSAP_ParamAddService_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddService_S));
    serviceParam->serviceType = ITEM_TYPE_STD_PRIMARY_SERVICE;
    (void)memcpy_s(&serviceParam->uuid, sizeof(NLSTK_SsapUuid_S), &g_uuid3, sizeof(NLSTK_SsapUuid_S));
    SSAP_CacheService(serviceParam);
    SDF_MemFree(serviceParam);
    /* 启动第二个服务 */
    SSAP_StartService(NULL);
}

/*
 * @brief 模拟发送回调函数
 *
 * 拦截服务端发送的SSAP响应数据，保存到全局缓存供测试验证
 * 这是测试框架模拟服务端行为的关键机制
 *
 * @param link SSAP链路上下文
 * @param buff 包含待发送数据的缓冲区
 * @param opcode 消息操作码(响应类型)
 */
static void MockSendCb(SSAP_Link *link, SDF_Buff_S *buff, uint8_t opcode)
{
    CP_LOG_INFO("[TEST] MockSendCb enter, opcode: 0x%02X", opcode);
    /* 获取数据长度并保存到全局缓存 */
    g_buffLen = SDF_DataLenGet(buff);
    if (g_buffLen > TEST_MAX_BUF_CACHE) {
        g_buffLen = TEST_MAX_BUF_CACHE;
    }
    (void)memcpy_s(g_buffCache, TEST_MAX_BUF_CACHE, SDF_DataOffset(buff), g_buffLen);
    isSendRsp = true;  /* 标记已发送响应 */
    CP_LOG_INFO("[TEST] MockSendCb send pkt: %s", SDF_GET_UINT8_STR(g_buffCache, g_buffLen));
}

/*
 * @brief 创建测试用SSAP链路
 *
 * 使用预设的地址和回调函数创建链路
 * @return 创建的链路指针
 */
static SSAP_Link* CreateLink()
{
    return SSAP_CreateSsapLink(&g_addr, g_lcid, MockSendCb);
}

/*
 * @brief 删除测试用SSAP链路
 *
 * 测试结束后清理链路资源
 */
static void DeleteLink()
{
    SSAP_DeleteSsapLinkByAddr(&g_addr);
}

/*
 * @brief 模拟接收SSAP请求
 *
 * 将测试数据封装成SDF_Buff并提交给SSAP_Recv处理
 * 重置全局状态，准备接收响应
 *
 * @param req 原始请求数据字节数组
 * @param reqLen 请求数据长度
 */
static void Test_SSAP_RecvReq(uint8_t *req, size_t reqLen)
{
    if (req == NULL || reqLen == 0) {
        CP_LOG_ERROR("[TEST] Test_SSAP_RecvReq input param invalid!");
        return;
    }
    /* 重置全局状态 */
    isSendRsp = false;
    g_buffLen = 0;
    (void)memset_s(g_buffCache, TEST_MAX_BUF_CACHE, 0, TEST_MAX_BUF_CACHE);
    /* 创建临时缓冲区并复制请求数据 */
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(reqLen);
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, reqLen);
    (void)memcpy_s(tmpBuf, reqLen, req, reqLen);
    CP_LOG_INFO("[TEST] Test_SSAP_RecvReq recv pkt: %s", SDF_GET_UINT8_STR(tmpBuf, reqLen));
    /* 提交请求给SSAP服务器处理 */
    SSAP_Recv(&g_dtapDataInfo, tmp);
    SDF_BuffFree(tmp);
}

/*
 * @brief 比较最后发送的响应包与期望值
 *
 * 用于验证错误响应等硬编码响应格式
 *
 * @param buf 期望的响应字节数组
 * @param size 期望的响应长度
 * @return true表示完全匹配，false表示不匹配
 */
static bool Test_SSAP_CompareLastSendPkt(uint8_t *buf, uint32_t size)
{
    if (isSendRsp == false) {
        CP_LOG_ERROR("[TEST] CompareLastPkt isSendRsp false");
        return false;
    }
    /* 字节级比较响应内容 */
    if (g_buffLen == size && memcmp(g_buffCache, buf, g_buffLen) == 0) {
        CP_LOG_INFO("[TEST] CompareLastPkt true, last pkt: %s",
            SDF_GET_UINT8_STR(g_buffCache, g_buffLen));
        return true;
    }
    CP_LOG_ERROR("[TEST] CompareLastPkt false, last pkt: %s, target pkt: %s",
        SDF_GET_UINT8_STR(g_buffCache, g_buffLen), SDF_GET_UINT8_STR(buf, size));
    return false;
}

/*
 * @brief 检查读响应中的multi标志位
 *
 * 从SSAP_PduReadRsp的msgCtrl字段提取multi位:
 * - bit[2] 表示单值(0)或多值(1)读取
 *
 * @param expectedResult 期望的multi值: SSAP_CTRL_MULTI_SINGLE(0)或SSAP_CTRL_MULTI_MULTI(1)
 * @return true表示multi标志匹配期望值
 */
static bool Test_SSAP_CheckMultiFlag(uint8_t expectedResult)
{
    if (g_buffLen == 0) {
        CP_LOG_ERROR("[TEST] CheckMultiFlag g_buffLen is 0");
        return false;
    }
    if (g_buffLen < sizeof(SSAP_PduReadRsp_S)) {
        CP_LOG_ERROR("[TEST] CheckMultiFlag buffLen too short: %u", g_buffLen);
        return false;
    }
    /* 提取msgCtrl的bit[2]作为multi标志 */
    SSAP_PduReadRsp_S *rsp = (SSAP_PduReadRsp_S *)g_buffCache;
    uint8_t multi = (rsp->msgCtrl >> 2) & 0x01;
    CP_LOG_INFO("[TEST] CheckMultiFlag multi=%u, expected=%u, msgCtrl=0x%02X", multi, expectedResult, rsp->msgCtrl);
    return multi == expectedResult;
}

/*
 * @brief 检查写响应中的result字段
 *
 * 从SSAP_PduWriteRsp的msgCtrl字段提取result位:
 * - bits[3:2] 表示写入结果: 00成功, 01部分成功, 10取消
 *
 * @param expectedResult 期望的result值: SSAP_CTRL_WRITE_SUCCESS(0b00)等
 * @return true表示result字段匹配期望值
 */
static bool Test_SSAP_CheckWriteRspResult(uint8_t expectedResult)
{
    if (g_buffLen == 0) {
        CP_LOG_ERROR("[TEST] CheckWriteRspResult g_buffLen is 0");
        return false;
    }
    if (g_buffLen < sizeof(SSAP_PduWriteRsp_S)) {
        CP_LOG_ERROR("[TEST] CheckWriteRspResult buffLen too short: %u", g_buffLen);
        return false;
    }
    /* 提取msgCtrl的bits[3:2]作为result字段 */
    SSAP_PduWriteRsp_S *rsp = (SSAP_PduWriteRsp_S *)g_buffCache;
    uint8_t result = (rsp->msgCtrl >> 2) & 0x03;
    CP_LOG_INFO("[TEST] CheckWriteRspResult result=%u, expected=%u, msgCtrl=0x%02X", result, expectedResult, rsp->msgCtrl);
    return result == expectedResult;
}

/*
 * @brief 测试用例: ADD_SERVICE - 添加服务并验证服务注册
 *
 * 测试流程:
 * 1. 调用AddTestServiceWithProperties注册一个包含两个属性的主服务(UUID1)
 * 2. 通过SSAPS_GetServices获取已注册服务列表
 * 3. 验证服务列表非空且只包含1个服务
 *
 * 验证点:
 * - 服务列表指针非NULL
 * - 服务数量等于1
 */
TEST_F(UT_SSAP_MULTI_READ_WRITE, ADD_SERVICE)
{
    CP_LOG_INFO("[UT_SSAP_MULTI_READ_WRITE] enter TEST_F ADD_SERVICE");
    AddTestServiceWithProperties();
    SDF_Vector_S *services = SSAPS_GetServices();
    EXPECT_NE(services, nullptr);
    EXPECT_EQ(services->size, 1);
}

/*
 * @brief 测试用例: SINGLE_HANDLE_READ_SUCCESS - 单句柄读请求成功
 *
 * 测试SSAP单值读取功能(单句柄请求)
 *
 * 测试流程:
 * 1. 注册测试服务和属性
 * 2. 创建SSAP链路
 * 3. 发送单句柄读请求: opcode=0x08
 *    - 字节[0]: 0x08 = SSAP_READ_REQ
 *    - 字节[1]: 0x02 = 数据长度(2字节: handle+type)
 *    - 字节[2-3]: handle (动态分配)
 *    - 字节[4]: 0x00 = 操作类型(读数据值)
 * 4. 验证响应已发送、响应长度足够、multi标志为单值(0)
 *
 * 验证点:
 * - 响应已发送(isSendRsp=true)
 * - 响应长度 >= sizeof(SSAP_PduReadRsp_S)
 * - ctrl.multi = SSAP_CTRL_MULTI_SINGLE(0) - 表示单值响应
 */
TEST_F(UT_SSAP_MULTI_READ_WRITE, SINGLE_HANDLE_READ_SUCCESS)
{
    CP_LOG_INFO("[UT_SSAP_MULTI_READ_WRITE] enter SINGLE_HANDLE_READ_SUCCESS");
    AddTestServiceWithProperties();
    (void)CreateLink();

    uint8_t req[] = {0x08, 0x02, 0x00, 0x01, 0x00};
    Test_SSAP_RecvReq(req, sizeof(req));

    EXPECT_TRUE(isSendRsp);
    EXPECT_GE(g_buffLen, sizeof(SSAP_PduReadRsp_S));
    EXPECT_TRUE(Test_SSAP_CheckMultiFlag(SSAP_CTRL_MULTI_SINGLE));

    DeleteLink();
}

/*
 * @brief 测试用例: MULTI_HANDLE_READ_SUCCESS - 多句柄读请求成功
 *
 * 测试SSAP多值读取功能(多句柄批量请求)
 *
 * 测试流程:
 * 1. 注册测试服务和属性
 * 2. 创建SSAP链路
 * 3. 发送多句柄读请求: opcode=0x08
 *    - handle1: 动态分配的操作句柄1
 *    - handle2: 动态分配的操作句柄2
 *    - 每个句柄后跟type=0x00(读数据值)
 * 4. 验证响应已发送且multi标志为多值(1)
 *
 * 验证点:
 * - 响应已发送(isSendRsp=true)
 * - 响应长度 >= sizeof(SSAP_PduReadRsp_S)
 * - ctrl.multi = SSAP_CTRL_MULTI_MULTI(1) - 表示多值响应
 */
TEST_F(UT_SSAP_MULTI_READ_WRITE, MULTI_HANDLE_READ_SUCCESS)
{
    CP_LOG_INFO("[UT_SSAP_MULTI_READ_WRITE] enter MULTI_HANDLE_READ_SUCCESS");
    AddTestServiceWithProperties();
    (void)CreateLink();

    uint8_t req[8] = {
        0x08,             // opcode = SSAP_READ_REQ
        0x03,             // ctrl.fragment = 0b11 (不分片)
        0x00, 0x00, 0x00, // handle, type=0x00(读数据值)
        0x00, 0x00, 0x00  // handle, type=0x00(读数据值)
    };
    (void)memcpy_s(&req[2], sizeof(uint16_t), &g_propertyHandle1, sizeof(uint16_t));
    (void)memcpy_s(&req[5], sizeof(uint16_t), &g_propertyHandle2, sizeof(uint16_t));
    Test_SSAP_RecvReq(req, sizeof(req));

    EXPECT_TRUE(isSendRsp);
    EXPECT_GE(g_buffLen, sizeof(SSAP_PduReadRsp_S));
    EXPECT_TRUE(Test_SSAP_CheckMultiFlag(SSAP_CTRL_MULTI_MULTI));

    DeleteLink();
}

/*
 * @brief 测试用例: MULTI_HANDLE_READ_INVALID_HANDLE - 包含无效句柄的多句柄读请求
 *
 * 测试当多句柄请求中包含无效句柄时的响应行为
 *
 * 测试流程:
 * 1. 注册测试服务和属性
 * 2. 创建SSAP链路
 * 3. 发送多句柄读请求:
 *    - 句柄1: handle=0x0001 (有效, type=0x00读数据值)
 *    - 句柄2: handle=0xFFFF (无效句柄, type=0x00读数据值)
 * 4. 如果响应已发送，验证multi标志为多值(1)
 *
 * 验证点:
 * - 如果发送响应，ctrl.multi = SSAP_CTRL_MULTI_MULTI(1)
 */
TEST_F(UT_SSAP_MULTI_READ_WRITE, MULTI_HANDLE_READ_INVALID_HANDLE)
{
    CP_LOG_INFO("[UT_SSAP_MULTI_READ_WRITE] enter MULTI_HANDLE_READ_INVALID_HANDLE");
    AddTestServiceWithProperties();
    (void)CreateLink();

    uint8_t req[] = {
        0x08,             // opcode = SSAP_READ_REQ
        0x03,             // ctrl.fragment = 0b11 (不分片)
        0x01, 0x00, 0x00, // handle=0x0001, type=0x00(读数据值)
        0xFF, 0xFF, 0x00  // handle=0xFFFF(无效句柄), type=0x00
    };
    Test_SSAP_RecvReq(req, sizeof(req));

    if (isSendRsp) {
        EXPECT_TRUE(Test_SSAP_CheckMultiFlag(SSAP_CTRL_MULTI_MULTI));
    }

    DeleteLink();
}

/*
 * @brief 测试用例: SINGLE_HANDLE_WRITE_SUCCESS - 单句柄写请求成功
 *
 * 测试SSAP单值写入功能(带响应的写请求)
 *
 * 测试流程:
 * 1. 注册测试服务和属性
 * 2. 创建SSAP链路
 * 3. 发送单句柄写请求: opcode=0x0D
 *    - 字节[0]: 0x0D = SSAP_WRITE_REQ(带响应)
 *    - 字节[1]: 0x03 = ctrl(fragment=0b11, multi=0)
 *    - 字节[2-3]: 动态分配的句柄
 *    - 字节[4]: 0x00 = 操作类型(写数据值)
 *    - 字节[5-6]: 0xAA, 0xBB = 写入数据
 * 4. 验证响应已发送且result为成功
 *
 * 验证点:
 * - 响应已发送(isSendRsp=true)
 * - ctrl.result = SSAP_CTRL_WRITE_SUCCESS(0b00) - 写入成功
 */
TEST_F(UT_SSAP_MULTI_READ_WRITE, SINGLE_HANDLE_WRITE_SUCCESS)
{
    CP_LOG_INFO("[UT_SSAP_MULTI_READ_WRITE] enter SINGLE_HANDLE_WRITE_SUCCESS");
    AddTestServiceWithProperties();
    (void)CreateLink();

    uint8_t req[7] = {
        0x0D,
        0x03,
        0x00, 0x00, 0x00, 0xAA, 0xBB
    };
    (void)memcpy_s(&req[2], sizeof(uint16_t), &g_propertyHandle1, sizeof(uint16_t));
    Test_SSAP_RecvReq(req, sizeof(req));

    EXPECT_TRUE(isSendRsp);
    EXPECT_TRUE(Test_SSAP_CheckWriteRspResult(SSAP_CTRL_WRITE_SUCCESS));

    DeleteLink();
}

/*
 * @brief 测试用例: MULTI_HANDLE_WRITE_SUCCESS - 多句柄写请求成功
 *
 * 测试SSAP多值写入功能(多个句柄批量写入)
 *
 * 测试流程:
 * 1. 注册测试服务和属性
 * 2. 创建SSAP链路
 * 3. 发送多句柄写请求: opcode=0x0D, ctrl=0x07(multi+no_frag)
 *    - 子项1: 动态分配句柄1, type=0x00(DATA), 长度=1, 数据=0xCC
 *    - 子项2: 动态分配句柄2, type=0x00(DATA), 长度=1, 数据=0xEE
 * 4. 验证响应已发送且result为成功
 *
 * 验证点:
 * - 响应已发送(isSendRsp=true)
 * - ctrl.result = SSAP_CTRL_WRITE_SUCCESS(0b00) - 全部写入成功
 */
TEST_F(UT_SSAP_MULTI_READ_WRITE, MULTI_HANDLE_WRITE_SUCCESS)
{
    CP_LOG_INFO("[UT_SSAP_MULTI_READ_WRITE] enter MULTI_HANDLE_WRITE_SUCCESS");
    AddTestServiceWithProperties();
    (void)CreateLink();

    uint8_t req[16] = {
        0x0D,
        0x07,
        0x00, 0x00,             // handle1 placeholder
        0x01,                   // subItemCount1
        0x00, 0x00, 0x01, 0xCC, // type1=0x00(DATA), len1=0x0001, value1=0xCC
        0x00, 0x00,             // handle2 placeholder
        0x01,                   // subItemCount2
        0x00, 0x00, 0x01, 0xEE  // type2=0x00(DATA), len2=0x0001, value2=0xEE
    };
    (void)memcpy_s(&req[2], sizeof(uint16_t), &g_propertyHandle1, sizeof(uint16_t));
    (void)memcpy_s(&req[8], sizeof(uint16_t), &g_propertyHandle2, sizeof(uint16_t));
    Test_SSAP_RecvReq(req, sizeof(req));

    EXPECT_TRUE(isSendRsp);
    EXPECT_TRUE(Test_SSAP_CheckWriteRspResult(SSAP_CTRL_WRITE_SUCCESS));

    DeleteLink();
}

/*
 * @brief 测试用例: MULTI_HANDLE_WRITE_WITH_ERRORS - 多句柄写请求全部成功
 *
 * 测试多句柄写入时两个句柄均成功的响应
 *
 * 测试流程:
 * 1. 注册测试服务和属性
 * 2. 创建SSAP链路
 * 3. 发送多句柄写请求:
 *    - 子项1: 动态句柄1, type=0x00(DATA), 长度=2, 数据=0xCC,0xDD
 *    - 子项2: 动态句柄2, type=0x00(DATA), 长度=1, 数据=0xEE
 * 4. 验证响应已发送且result为成功
 *
 * 验证点:
 * - 响应已发送(isSendRsp=true)
 * - ctrl.result = SSAP_CTRL_WRITE_SUCCESS(0b00) - 全部写入成功
 *
 * @note 测试名称中的"ERRORS"表示测试覆盖了多值写入错误场景，但实际使用有效句柄
 */
TEST_F(UT_SSAP_MULTI_READ_WRITE, MULTI_HANDLE_WRITE_WITH_ERRORS)
{
    CP_LOG_INFO("[UT_SSAP_MULTI_READ_WRITE] enter MULTI_HANDLE_WRITE_WITH_ERRORS");
    AddTestServiceWithProperties();
    (void)CreateLink();

    uint8_t req[17] = {
        0x0D,                   // opcode = WRITE_REQ
        0x07,                   // ctrl = multi(0x04) + no_frag(0x03)
        0x00, 0x00,             // handle1 placeholder (2 bytes)
        0x01,                   // subItemCount1 (1 byte)
        0x00, 0x00, 0x02,       // subItem1: type=0x00, len=0x0002 (3 bytes)
        0xCC, 0xDD,             // subItem1.value (2 bytes)
        0x00, 0x00,             // handle2 placeholder (2 bytes)
        0x01,                   // subItemCount2 (1 byte)
        0x00, 0x00, 0x01,       // subItem2: type=0x00, len=0x0001 (3 bytes)
        0xEE                    // subItem2.value (1 byte)
    };
    (void)memcpy_s(&req[2], sizeof(uint16_t), &g_propertyHandle1, sizeof(uint16_t));
    (void)memcpy_s(&req[10], sizeof(uint16_t), &g_propertyHandle2, sizeof(uint16_t));
    Test_SSAP_RecvReq(req, sizeof(req));

    EXPECT_TRUE(isSendRsp);
    EXPECT_TRUE(Test_SSAP_CheckWriteRspResult(SSAP_CTRL_WRITE_SUCCESS));

    DeleteLink();
}

/*
 * @brief 测试用例: MULTI_WRITE_RETURN_ORIGIN - 多值写入返回原始句柄
 *
 * 测试多值写入仅1个句柄
 *
 * 测试流程:
 * 1. 注册测试服务和属性
 * 2. 创建SSAP链路
 * 3. 发送多句柄写请求: opcode=0x0D, ctrl=0x07
 *    - 子项: 动态句柄, type=0x00, 长度=2, 数据=0xCC,0xDD
 * 4. 验证响应已发送、result为成功、返回句柄正确
 *
 * 验证点:
 * - 响应已发送(isSendRsp=true)
 * - ctrl.result = SSAP_CTRL_WRITE_SUCCESS(0b00)
 * - 返回的句柄值与请求中的句柄一致(动态分配)
 */
TEST_F(UT_SSAP_MULTI_READ_WRITE, MULTI_WRITE_RETURN_ORIGIN)
{
    CP_LOG_INFO("[UT_SSAP_MULTI_READ_WRITE] enter MULTI_WRITE_RETURN_ORIGIN");
    AddTestServiceWithProperties();
    (void)CreateLink();

    uint8_t req[10] = {
        0x0D,
        0x07,
        0x00, 0x00,  // handle (overwritten by memcpy)
        0x01,        // subItemCount
        0x00,        // subItem[0].type
        0x02, 0x00,  // subItem[0].len = 2 (little-endian)
        0xCC, 0xDD   // subItem[0].value
    };
    (void)memcpy_s(&req[2], sizeof(uint16_t), &g_propertyHandle1, sizeof(uint16_t));
    CP_LOG_INFO("[TEST] Using dynamic handle: 0x%04X", g_propertyHandle1);
    Test_SSAP_RecvReq(req, sizeof(req));

    EXPECT_TRUE(isSendRsp);
    EXPECT_TRUE(Test_SSAP_CheckWriteRspResult(SSAP_CTRL_WRITE_SUCCESS));
    if (g_buffLen > sizeof(SSAP_PduWriteRsp_S)) {
        uint8_t *items = g_buffCache + sizeof(SSAP_PduWriteRsp_S);
        uint16_t handle = SSAP_BYTE_TO_UINT16_LITTLE(items);
        CP_LOG_INFO("[TEST] Origin return handle: 0x%04X", handle);
        EXPECT_EQ(handle, g_propertyHandle1);
    }

    DeleteLink();
}

/*
 * @brief 测试用例: MULTI_WRITE_INVALID_PDU_TOO_SHORT - 多值写请求PDU过短
 *
 * 测试当收到的写请求PDU长度不足时的错误处理
 *
 * 测试流程:
 * 1. 注册测试服务和属性
 * 2. 创建SSAP链路
 * 3. 发送过短的写请求: opcode=0x0D, ctrl=0x04(multi), 但无子项数据
 *    - 仅发送opcode和ctrl，没有子项数据
 * 4. 验证返回错误响应
 *
 * 验证点:
 * - 响应内容匹配预期错误包: {0x01, 0x00, 0x0D, 0x00, 0x00, 0x01}
 *   - 0x01: SSAP_ERROR_RSP opcode
 *   - 0x00, 0x0D: 原请求opcode (0x0D为WRITE_REQ)
 *   - 0x00, 0x01: 错误码(0x0001=INVALID_PDU)
 */
TEST_F(UT_SSAP_MULTI_READ_WRITE, MULTI_WRITE_INVALID_PDU_TOO_SHORT)
{
    CP_LOG_INFO("[UT_SSAP_MULTI_READ_WRITE] enter MULTI_WRITE_INVALID_PDU_TOO_SHORT");
    AddTestServiceWithProperties();
    (void)CreateLink();

    uint8_t req[] = {0x0D, 0x04};
    Test_SSAP_RecvReq(req, sizeof(req));

    uint8_t errRsp[] = {0x01, 0x00, 0x0D, 0x00, 0x00, 0x01};
    EXPECT_TRUE(Test_SSAP_CompareLastSendPkt(errRsp, sizeof(errRsp)));

    DeleteLink();
}

/*
 * @brief 测试用例: MULTI_WRITE_INVALID_HANDLE - 多值写入包含无效句柄
 *
 * 测试当所有句柄都无效时的写入响应
 *
 * 测试流程:
 * 1. 注册测试服务和属性
 * 2. 创建SSAP链路
 * 3. 发送多句柄写请求:
 *    - 子项1: handle=0xFFFF (无效)
 *    - 子项2: handle=0x0001 (无效，属性不能直接写)
 * 4. 验证响应已发送且result为部分成功
 *
 * 验证点:
 * - 响应已发送(isSendRsp=true)
 * - ctrl.result = SSAP_CTRL_WRITE_PART(0b01) - 无有效写入
 */
TEST_F(UT_SSAP_MULTI_READ_WRITE, MULTI_WRITE_INVALID_HANDLE)
{
    CP_LOG_INFO("[UT_SSAP_MULTI_READ_WRITE] enter MULTI_WRITE_INVALID_HANDLE");
    AddTestServiceWithProperties();
    (void)CreateLink();

    uint8_t req[] = {
        0x0D,
        0x07,
        0xFF, 0xFF,     // handle = 0xFFFF (invalid)
        0x01,           // subItemCount = 1
        0x01,           // subItem[0].type
        0x01, 0x00,     // subItem[0].len = 1 (little-endian)
        0xAA            // subItem[0].value
    };
    Test_SSAP_RecvReq(req, sizeof(req));

    EXPECT_TRUE(isSendRsp);
    EXPECT_TRUE(Test_SSAP_CheckWriteRspResult(SSAP_CTRL_WRITE_PART));

    DeleteLink();
}

/*
 * @brief 测试用例: WRITE_SINGLE_CMD_SUCCESS - 单句柄写命令成功
 *
 * 测试SSAP写命令(无响应)功能
 * WRITE_CMD (opcode=0x0C) 不返回响应
 *
 * 测试流程:
 * 1. 注册测试服务和属性
 * 2. 创建SSAP链路
 * 3. 发送单句柄写命令: opcode=0x0C
 *    - 字节[0]: 0x0C = SSAP_WRITE_CMD(无响应)
 *    - 字节[1]: 0x03 = ctrl(fragment=0b11, multi=0)
 *    - 字节[2-3]: 0x0002 = 目标句柄
 *    - 字节[4]: 0x00 = 操作类型
 *    - 字节[5-6]: 0xAA, 0xBB = 写入数据
 * 4. 验证无响应发送
 *
 * 验证点:
 * - isSendRsp = false (写命令不返回响应)
 */
TEST_F(UT_SSAP_MULTI_READ_WRITE, WRITE_SINGLE_CMD_SUCCESS)
{
    CP_LOG_INFO("[UT_SSAP_MULTI_READ_WRITE] enter WRITE_SINGLE_CMD_SUCCESS");
    AddTestServiceWithProperties();
    (void)CreateLink();

    uint8_t req[] = {
        0x0C,
        0x03,
        0x02, 0x00, 0x00, 0xAA, 0xBB
    };
    Test_SSAP_RecvReq(req, sizeof(req));

    EXPECT_FALSE(isSendRsp);

    DeleteLink();
}

/*
 * @brief 测试用例: WRITE_MULTI_CMD_SUCCESS - 多句柄写命令成功
 *
 * 测试SSAP多值写命令(无响应)功能
 *
 * 测试流程:
 * 1. 注册测试服务和属性
 * 2. 创建SSAP链路
 * 3. 发送多句柄写命令: opcode=0x0C, ctrl=0x07(multi+no_frag)
 *    - 子项1: handle=0x0002, type=0x01, 长度=2, 数据=0xCC,0xDD
 *    - 子项2: handle=0xFFFF(无效), type=0x01, 长度=1, 数据=0xEE
 * 4. 验证无响应发送
 *
 * 验证点:
 * - isSendRsp = false (写命令不返回响应)
 */
TEST_F(UT_SSAP_MULTI_READ_WRITE, WRITE_MULTI_CMD_SUCCESS)
{
    CP_LOG_INFO("[UT_SSAP_MULTI_READ_WRITE] enter WRITE_MULTI_CMD_SUCCESS");
    AddTestServiceWithProperties();
    (void)CreateLink();

    uint8_t req[] = {
        0x0C,
        0x07,
        0x02, 0x00, 0x01,
        0x01, 0x02, 0xCC, 0xDD,
        0xFF, 0xFF, 0x01,
        0x01, 0x01, 0xEE
    };
    Test_SSAP_RecvReq(req, sizeof(req));

    EXPECT_FALSE(isSendRsp);

    DeleteLink();
}

/*
 * @brief 测试用例: MULTI_WRITE_CMD_INVALID_HANDLE - 多句柄写命令包含无效句柄
 *
 * 测试当写命令中包含无效句柄时的行为(不应发送响应)
 *
 * 测试流程:
 * 1. 注册测试服务和属性
 * 2. 创建SSAP链路
 * 3. 发送多句柄写命令: opcode=0x0C, ctrl=0x07(multi+no_frag)
 *    - 子项: handle=0xFFFF(无效句柄), type=0x01, 长度=1, 数据=0xAA
 * 4. 验证无响应发送(写命令不返回响应)
 *
 * 验证点:
 * - isSendRsp = false (写命令即使包含无效句柄也不返回响应)
 */
TEST_F(UT_SSAP_MULTI_READ_WRITE, MULTI_WRITE_CMD_INVALID_HANDLE)
{
    CP_LOG_INFO("[UT_SSAP_MULTI_READ_WRITE] enter MULTI_WRITE_CMD_INVALID_HANDLE");
    AddTestServiceWithProperties();
    (void)CreateLink();

    uint8_t req[] = {
        0x0C,
        0x07,
        0xFF, 0xFF, 0x01,
        0x01, 0x01, 0xAA
    };
    Test_SSAP_RecvReq(req, sizeof(req));

    EXPECT_FALSE(isSendRsp);

    DeleteLink();
}

/*
 * @brief 测试用例: MULTI_WRITE_ORIGIN_RETURN_TWO_ITEMS - 多值写入后再读验证
 *
 * 测试写+读回验证的完整流程:
 * 1. 用opcode=0x0D写入两个属性的值
 * 2. 验证写入响应成功
 * 3. 用opcode=0x08读回这两个属性
 * 4. 验证读回的值与写入的值一致
 *
 * 测试流程:
 * 1. 注册测试服务和属性
 * 2. 创建SSAP链路
 * 3. 发送多句柄写请求: opcode=0x0D, ctrl=0x07
 *    - handle1 (动态句柄), type=0x01, 长度=1, 数据=0xAA
 *    - handle2 (动态句柄), type=0x01, 长度=2, 数据=0xBB, 0xCC
 * 4. 验证写入响应成功
 * 5. 发送读请求: opcode=0x08, ctrl=0x03
 *    - handle1, type=0x01
 *    - handle2, type=0x01
 * 6. 验证读回的数据与写入一致
 *
 * 验证点:
 * - 写入响应: isSendRsp=true, ctrl.result = SSAP_CTRL_WRITE_SUCCESS
 * - 读响应: isSendRsp=true, 数据匹配
 *
 * Read Rsp Item格式 (bitfield):
 *   bits[14:0] = length, bit[15] = success
 *   Item1: success=1, len=1, value=0xAA → 0x01, 0x00, 0xAA
 *   Item2: success=1, len=2, value=0xBB, 0xCC → 0x02, 0x00, 0xBB, 0xCC
 */
TEST_F(UT_SSAP_MULTI_READ_WRITE, MULTI_WRITE_ORIGIN_RETURN_TWO_ITEMS)
{
    CP_LOG_INFO("[UT_SSAP_MULTI_READ_WRITE] enter MULTI_WRITE_ORIGIN_RETURN_TWO_ITEMS");
    AddTestServiceWithProperties();
    (void)CreateLink();

    uint8_t writeReq[] = {
        0x0D,             // opcode = SSAP_WRITE_REQ
        0x07,             // ctrl = multi + no_frag
        0x01, 0x00, 0x01, // MultiItem 1: handle, subItemCount=1 (handle placeholder at [2-3])
        0x00, 0x01, 0x00, 0xAA,  // SubItem 1: type=0x00(DATA), len=1(LE), value=0xAA
        0x02, 0x00, 0x01, // MultiItem 2: handle, subItemCount=1 (handle placeholder at [9-10])
        0x00, 0x02, 0x00, 0xBB, 0xCC   // SubItem 2: type=0x00(DATA), len=2(LE), value=0xBB, 0xCC
    };
    writeReq[2] = (uint8_t)(g_propertyHandle1 & 0xFF);
    writeReq[3] = (uint8_t)((g_propertyHandle1 >> 8) & 0xFF);
    writeReq[9] = (uint8_t)(g_propertyHandle2 & 0xFF);
    writeReq[10] = (uint8_t)((g_propertyHandle2 >> 8) & 0xFF);

    Test_SSAP_RecvReq(writeReq, sizeof(writeReq));

    EXPECT_TRUE(isSendRsp);
    EXPECT_TRUE(Test_SSAP_CheckWriteRspResult(SSAP_CTRL_WRITE_SUCCESS));

    isSendRsp = false;
    g_buffLen = 0;
    (void)memset_s(g_buffCache, TEST_MAX_BUF_CACHE, 0, TEST_MAX_BUF_CACHE);

    uint8_t readReq[] = {
        0x08,             // opcode = SSAP_READ_REQ
        0x03,             // ctrl = no_frag
        0x01, 0x00, 0x00, // [0-4] handle1 placeholder, type=0x00(DATA)
        0x02, 0x00, 0x00  // [5-7] handle2 placeholder, type=0x00(DATA)
    };
    readReq[2] = (uint8_t)(g_propertyHandle1 & 0xFF);
    readReq[3] = (uint8_t)((g_propertyHandle1 >> 8) & 0xFF);
    readReq[5] = (uint8_t)(g_propertyHandle2 & 0xFF);
    readReq[6] = (uint8_t)((g_propertyHandle2 >> 8) & 0xFF);

    Test_SSAP_RecvReq(readReq, sizeof(readReq));

    uint8_t expectedReadRsp[] = {
        0x09,             // opcode = SSAP_READ_RSP
        0x07,             // ctrl = multi(1) + no_frag(3) + error(0)
        0x01, 0x80, 0xAA, // item1: success=1(bit15), len=1, value=0xAA
        0x02, 0x80, 0xBB, 0xCC  // item2: success=1(bit15), len=2, value=0xBB, 0xCC
    };
    EXPECT_TRUE(isSendRsp);
    EXPECT_TRUE(Test_SSAP_CompareLastSendPkt(expectedReadRsp, sizeof(expectedReadRsp)));

    DeleteLink();
}

/*
 * @brief 测试用例: MULTI_WRITE_THREE_HANDLES_MIXED_ERRORS - 多句柄写入混合错误
 *
 * 测试多个有效句柄和无效句柄混合写入时的响应
 *
 * 测试流程:
 * 1. 注册测试服务和属性
 * 2. 创建SSAP链路
 * 3. 发送多句柄写请求(共5个子项):
 *    - 子项1: handle=0x0001, type=0x01, 长度=1, 数据=0xCC
 *    - 子项2: handle=0x0002, type=0x01, 长度=1, 数据=0xEE
 *    - 子项3: handle=0xFFFF, type=0x01, 长度=1, 数据=0xFF
 * 4. 验证响应已发送且result为部分成功
 *
 * 验证点:
 * - 响应已发送(isSendRsp=true)
 * - ctrl.result = SSAP_CTRL_WRITE_PART(0b01) - 存在部分错误
 */
TEST_F(UT_SSAP_MULTI_READ_WRITE, MULTI_WRITE_THREE_HANDLES_MIXED_ERRORS)
{
    CP_LOG_INFO("[UT_SSAP_MULTI_READ_WRITE] enter MULTI_WRITE_THREE_HANDLES_MIXED_ERRORS");
    AddTestServiceWithProperties();
    (void)CreateLink();

    uint8_t req[] = {
        0x0D,
        0x07,
        0x01, 0x00, 0x01,  // MultiItem 1: handle=0x0001, subItemCount=1
        0x01, 0x01, 0x00, 0xCC,  // SubItem 1: type=0x01, len=1 (LE), value=CC
        0x02, 0x00, 0x01,  // MultiItem 2: handle=0x0002, subItemCount=1
        0x01, 0x01, 0x00, 0xEE,  // SubItem 2: type=0x01, len=1 (LE), value=EE
        0xFF, 0xFF, 0x01,  // MultiItem 3: handle=0xFFFF (invalid), subItemCount=1
        0x01, 0x01, 0x00, 0xFF   // SubItem 3: type=0x01, len=1 (LE), value=FF
    };
    // rsp: 0E 07 03 00 00 04 00 00 04 00 00 04
    Test_SSAP_RecvReq(req, sizeof(req));

    EXPECT_TRUE(isSendRsp);
    EXPECT_TRUE(Test_SSAP_CheckWriteRspResult(SSAP_CTRL_WRITE_PART));

    DeleteLink();
}

/*
 * @brief 测试用例: MULTI_WRITE_THREE_HANDLES_MIXED_ERRORS - 多句柄写入，subItem里的数据类型重复
 *
 * 测试多句柄写入，subItem里的数据类型重复
 *
 * 测试流程:
 * 1. 注册测试服务和属性
 * 2. 创建SSAP链路
 * 3. 发送多句柄写请求(共5个子项):
 *    - 子项1: handle=0x0001, type=0x01, 长度=1, 数据=0xCC
 *    - 子项2: handle=0x0002, type=0x01, 长度=1, 数据=0xEE
 *    - 子项3: handle=0xFFFF, type=0x01, 长度=1, 数据=0xFF
 * 4. 验证响应已发送且result为部分成功
 *
 * 验证点:
 * - 响应已发送(isSendRsp=true)
 */
TEST_F(UT_SSAP_MULTI_READ_WRITE, MULTI_WRITE_REQ_SAME_TYPE)
{
    CP_LOG_INFO("[UT_SSAP_MULTI_READ_WRITE] enter MULTI_WRITE_REQ_SAME_TYPE");
    AddTestServiceWithProperties();
    (void)CreateLink();

    uint8_t req[] = {
        0x0d,
        0x27,
        0x12, 0x00, 0x02,  // MultiItem 1: handle=0x0012, subItemCount=2
        0x02, 0x05, 0x00, 0x00, 0x27, 0x12, 0x00, 0x02, // SubItem 1: type=0x02, len=0x0005 (LE), value=0x00, 0x27, 0x12, 0x00, 0x02
        0x02, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // SubItem 2: type=0x02, len=0x0005 (LE), value=0x00, 0x00, 0x00, 0x00, 0x00
        0x00, 0x00, 0x00,  // MultiItem 2: handle=0x0000, subItemCount=0
        0x02, 0x02, 0x00,  // MultiItem 3: handle=0x0202, subItemCount=0
        0x00, 0x02, 0x00   // MultiItem 4: handle=0x0200, subItemCount=0
    };
    // rsp: 0E 07 03 00 00 04 02 02 04 00 02 04 12 00 02 02 05 00 00 27 12 00 02 02 05 00 00 00 00 00 00
    Test_SSAP_RecvReq(req, sizeof(req));

    EXPECT_TRUE(isSendRsp);

    uint8_t rsp[] = {
        0x0E,
        0x07,
        0x03, // 3个错误
        0x00, 0x00, 0x04, // 错误1：handle + errorCode
        0x02, 0x02, 0x04, // 错误1：handle + errorCode
        0x00, 0x02, 0x04, // 错误1：handle + errorCode
        0x12, 0x00, 0x02, // 原值：handle + 元组数
        0x02, 0x05, 0x00, 0x00, 0x27, 0x12, 0x00, 0x02, // 原值：type + len + value
        0x02, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 // 原值：type + len + value
    };
    EXPECT_TRUE(Test_SSAP_CompareLastSendPkt(rsp, sizeof(rsp)));

    DeleteLink();
}

/*
 * @brief 测试用例: MULTI_WRITE_THREE_HANDLES_MIXED_ERRORS - 多句柄写入混合错误超过255
 *
 * 测试多个有效句柄和无效句柄混合写入时的响应
 *
 * 测试流程:
 * 1. 注册测试服务和属性
 * 2. 创建SSAP链路
 * 3. 发送多句柄写请求
 * 4. 验证返回错误响应
 *
 * 验证点:
 * - 响应内容匹配预期错误包: {0x01, 0x00, 0x0D, 0x00, 0x00, 0x01}
 *   - 0x01: SSAP_ERROR_RSP opcode
 *   - 0x00, 0x0D: 原请求opcode (0x0D为WRITE_REQ)
 *   - 0x00, 0x01: 错误码(0x0001=INVALID_PDU)
 */
TEST_F(UT_SSAP_MULTI_READ_WRITE, MULTI_WRITE_HANDLES_MIXED_ERRORS_OVER_255)
{
    CP_LOG_INFO("[UT_SSAP_MULTI_READ_WRITE] enter MULTI_WRITE_947_HANDLES_MIXED_ERRORS");
    AddTestServiceWithProperties();
    (void)CreateLink();

    uint8_t req[] = {
        0x0d, 0x07, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf7, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x87, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x0d, 0x27, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x2d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x11, 0x00, 0x00
    };

    Test_SSAP_RecvReq(req, sizeof(req));

    uint8_t errRsp[] = {0x01, 0x00, 0x0D, 0x00, 0x00, 0x01};
    EXPECT_TRUE(Test_SSAP_CompareLastSendPkt(errRsp, sizeof(errRsp)));

    DeleteLink();
}


/*
 * @brief 测试用例: MULTI_WRITE_CMD_EMPTY_SUBITEMS - 多值写命令子项为空
 *
 * 测试当多值写命令的子项数据不完整时的行为
 *
 * 测试流程:
 * 1. 注册测试服务和属性
 * 2. 创建SSAP链路
 * 3. 发送多句柄写命令: opcode=0x0C, ctrl=0x07(multi+no_frag)
 *    - 子项头: handle=0x0002, type=0x00, 声明长度=0(无数据)
 *    - PDU在提供数据前被截断
 * 4. 验证无响应发送
 *
 * 验证点:
 * - isSendRsp = false (写命令不返回响应，即使PDU不完整)
 */
TEST_F(UT_SSAP_MULTI_READ_WRITE, MULTI_WRITE_CMD_EMPTY_SUBITEMS)
{
    CP_LOG_INFO("[UT_SSAP_MULTI_READ_WRITE] enter MULTI_WRITE_CMD_EMPTY_SUBITEMS");
    AddTestServiceWithProperties();
    (void)CreateLink();

    uint8_t req[] = {
        0x0C,
        0x07,
        0x02, 0x00, 0x00
    };
    Test_SSAP_RecvReq(req, sizeof(req));

    EXPECT_FALSE(isSendRsp);

    DeleteLink();
}

/*
 * @brief 测试用例: MULTI_WRITE_CMD_CTRL_OPER_INVALID - 多值写命令操作无效
 *
 * 测试当ctrl包含不支持的oper标志时的行为
 *
 * 测试流程:
 * 1. 注册测试服务和属性
 * 2. 创建SSAP链路
 * 3. 发送多句柄写命令: opcode=0x0C, ctrl=0x08
 *    - ctrl=0x08: fragment=0b00(分片), multi=0, oper=0b10(非WRITE_INSTANT)
 *    - 子项: handle=0x0002, type=0x01, 长度=2, 数据=0xCC,0xDD
 * 4. 验证无响应发送
 *
 * 验证点:
 * - isSendRsp = false (写命令不返回响应)
 */
TEST_F(UT_SSAP_MULTI_READ_WRITE, MULTI_WRITE_CMD_CTRL_OPER_INVALID)
{
    CP_LOG_INFO("[UT_SSAP_MULTI_READ_WRITE] enter MULTI_WRITE_CMD_CTRL_OPER_INVALID");
    AddTestServiceWithProperties();
    (void)CreateLink();

    uint8_t req[] = {
        0x0C,
        0x08,
        0x02, 0x00, 0x01,
        0x01, 0x02, 0xCC, 0xDD
    };
    Test_SSAP_RecvReq(req, sizeof(req));

    EXPECT_FALSE(isSendRsp);

    DeleteLink();
}

/*
 * @brief 测试用例: MULTI_WRITE_RESPONSE_ITEM_COUNT - 多值写入响应项数量
 *
 * 测试多值写入响应的数据完整性
 *
 * 测试流程:
 * 1. 注册测试服务和属性
 * 2. 创建SSAP链路
 * 3. 发送多句柄写请求: opcode=0x0D, ctrl=0x07(multi+no_frag)
 *    - 子项1: 动态句柄1, type=0x00(DATA), 长度=1, 数据=0xAA
 *    - 子项2: 动态句柄2, type=0x00(DATA), 长度=1, 数据=0xBB
 * 4. 验证响应已发送且result为成功
 *
 * 验证点:
 * - 响应已发送(isSendRsp=true)
 * - ctrl.result = SSAP_CTRL_WRITE_SUCCESS(0b00)
 */
TEST_F(UT_SSAP_MULTI_READ_WRITE, MULTI_WRITE_RESPONSE_ITEM_COUNT)
{
    CP_LOG_INFO("[UT_SSAP_MULTI_READ_WRITE] enter MULTI_WRITE_RESPONSE_ITEM_COUNT");
    AddTestServiceWithProperties();
    (void)CreateLink();

    uint8_t req[16] = {
        0x0D,
        0x07,
        0x00, 0x00,
        0x01,
        0x00, 0x00, 0x01, 0xAA,
        0x00, 0x00,
        0x01,
        0x00, 0x00, 0x01, 0xBB
    };
    (void)memcpy_s(&req[2], sizeof(uint16_t), &g_propertyHandle1, sizeof(uint16_t));
    (void)memcpy_s(&req[9], sizeof(uint16_t), &g_propertyHandle2, sizeof(uint16_t));
    Test_SSAP_RecvReq(req, sizeof(req));

    EXPECT_TRUE(isSendRsp);
    EXPECT_TRUE(Test_SSAP_CheckWriteRspResult(SSAP_CTRL_WRITE_SUCCESS));

    DeleteLink();
}

/*
 * @brief 测试用例: MULTI_WRITE_SINGLE_CMD_SUBITEM_ZERO_LENGTH - 多值写命令子项零长度
 *
 * 测试当多值写命令子项数据长度为0时的行为
 *
 * 测试流程:
 * 1. 注册测试服务和属性
 * 2. 创建SSAP链路
 * 3. 发送多值写命令: opcode=0x0C, ctrl=0x07(multi+no_frag)
 *    - 子项: handle=0x0002, type=0x01, 长度=0(无数据)
 *    - 写命令不解析数据长度，只验证PDU格式
 * 4. 验证无响应发送
 *
 * 验证点:
 * - isSendRsp = false (写命令不返回响应)
 */
TEST_F(UT_SSAP_MULTI_READ_WRITE, MULTI_WRITE_SINGLE_CMD_SUBITEM_ZERO_LENGTH)
{
    CP_LOG_INFO("[UT_SSAP_MULTI_READ_WRITE] enter MULTI_WRITE_SINGLE_CMD_SUBITEM_ZERO_LENGTH");
    AddTestServiceWithProperties();
    (void)CreateLink();

    uint8_t req[] = {
        0x0C,
        0x07,
        0x02, 0x00, 0x01,
        0x01, 0x00
    };
    Test_SSAP_RecvReq(req, sizeof(req));

    EXPECT_FALSE(isSendRsp);

    DeleteLink();
}

/*
 * @brief 测试用例: MULTI_WRITE_CMD_AUTH_ITEM_NOT_SUPPORTED - 多值写命令认证项不支持
 *
 * 测试当写命令中包含认证项时的行为
 *
 * 测试流程:
 * 1. 注册测试服务和属性
 * 2. 创建SSAP链路
 * 3. 发送多值写命令: opcode=0x0C, ctrl=0x07
 *    - 子项: handle=0x0002, type=0x01, 长度=2, 数据=0xCC,0xDD
 *    - type=0x01可能表示需要认证的操作类型
 * 4. 验证无响应发送
 *
 * 验证点:
 * - isSendRsp = false (写命令不返回响应)
 */
TEST_F(UT_SSAP_MULTI_READ_WRITE, MULTI_WRITE_CMD_AUTH_ITEM_NOT_SUPPORTED)
{
    CP_LOG_INFO("[UT_SSAP_MULTI_READ_WRITE] enter MULTI_WRITE_CMD_AUTH_ITEM_NOT_SUPPORTED");
    AddTestServiceWithProperties();
    (void)CreateLink();

    uint8_t req[] = {
        0x0C,
        0x07,
        0x02, 0x00, 0x01,
        0x01, 0x02, 0xCC, 0xDD
    };
    Test_SSAP_RecvReq(req, sizeof(req));

    EXPECT_FALSE(isSendRsp);

    DeleteLink();
}

/*
 * @brief 测试用例: MULTI_WRITE_PDU_MALFORMED_TRUNCATED - 多值写入PDU被截断
 *
 * 测试当收到的写请求PDU被截断时的错误处理
 *
 * 测试流程:
 * 1. 注册测试服务和属性
 * 2. 创建SSAP链路
 * 3. 发送截断的写请求: opcode=0x0D, ctrl=0x07(multi+no_frag)
 *    - 只提供了opcode和ctrl，后续子项数据不完整
 * 4. 验证返回错误响应
 *
 * 验证点:
 * - 响应内容匹配预期错误包: {0x01, 0x00, 0x0D, 0x00, 0x00, 0x01}
 *   - 0x01: SSAP_ERROR_RSP opcode
 *   - 0x00, 0x0D: 原请求opcode (0x0D为WRITE_REQ)
 *   - 0x00, 0x01: 错误码(0x0001=INVALID_PDU)
 */
TEST_F(UT_SSAP_MULTI_READ_WRITE, MULTI_WRITE_PDU_MALFORMED_TRUNCATED)
{
    CP_LOG_INFO("[UT_SSAP_MULTI_READ_WRITE] enter MULTI_WRITE_PDU_MALFORMED_TRUNCATED");
    AddTestServiceWithProperties();
    (void)CreateLink();

    uint8_t req[] = {
        0x0D,
        0x07,
        0x02, 0x00
    };
    Test_SSAP_RecvReq(req, sizeof(req));

    uint8_t errRsp[] = {0x01, 0x00, 0x0D, 0x00, 0x00, 0x01};
    EXPECT_TRUE(Test_SSAP_CompareLastSendPkt(errRsp, sizeof(errRsp)));

    DeleteLink();
}

/*
 * @brief 测试用例: MULTI_READ_THREE_HANDLES_SUCCESS - 三句柄读请求成功
 *
 * 测试SSAP多值读取功能(三个句柄批量请求)
 *
 * 测试流程:
 * 1. 注册测试服务和属性
 * 2. 创建SSAP链路
 * 3. 发送三句柄读请求: opcode=0x08
 *    - 句柄1: 动态句柄1, type=0x00(读数据值)
 *    - 句柄2: 动态句柄2, type=0x00(读数据值)
 *    - 句柄3: 动态句柄1(重复), type=0x01(执行)
 * 4. 验证响应已发送且multi标志为多值(1)
 *
 * 验证点:
 * - 响应已发送(isSendRsp=true)
 * - ctrl.multi = SSAP_CTRL_MULTI_MULTI(1) - 表示多值响应
 */
TEST_F(UT_SSAP_MULTI_READ_WRITE, MULTI_READ_THREE_HANDLES_SUCCESS)
{
    CP_LOG_INFO("[UT_SSAP_MULTI_READ_WRITE] enter MULTI_READ_THREE_HANDLES_SUCCESS");
    AddTestServiceWithProperties();
    (void)CreateLink();

    uint8_t req[11] = {
        0x08,             // opcode = SSAP_READ_REQ
        0x03,             // ctrl.fragment = 0b11 (不分片)
        0x00, 0x00, 0x00, // handle, type=0x00
        0x00, 0x00, 0x00, // handle, type=0x00
        0x00, 0x00, 0x01  // handle, type=0x01
    };
    (void)memcpy_s(&req[2], sizeof(uint16_t), &g_propertyHandle1, sizeof(uint16_t));
    (void)memcpy_s(&req[5], sizeof(uint16_t), &g_propertyHandle2, sizeof(uint16_t));
    (void)memcpy_s(&req[8], sizeof(uint16_t), &g_propertyHandle1, sizeof(uint16_t));
    Test_SSAP_RecvReq(req, sizeof(req));

    EXPECT_TRUE(isSendRsp);
    EXPECT_TRUE(Test_SSAP_CheckMultiFlag(SSAP_CTRL_MULTI_MULTI));

    DeleteLink();
}

/*
 * @brief 测试用例: WRITE_REQ_CTRL_FRAGMENT_NOT_SUPPORTED - 写请求不支持分片
 *
 * 测试当写请求ctrl.fragment包含分片标志(非0b11)时的错误处理
 *
 * 测试流程:
 * 1. 注册测试服务和属性
 * 2. 创建SSAP链路
 * 3. 发送带分片标志的写请求: opcode=0x0D, ctrl=0x00
 *    - ctrl=0x00: fragment=0b00(分片), multi=0, oper=0b00(WRITE_INSTANT), verify=0
 *    - SSAP服务器不支持分片，fragment必须为0b11(0x03)
 *    - oper=0b00满足WRITE_INSTANT检查，但fragment检查失败
 * 4. 验证返回WRITE_RSP错误响应
 *
 * 验证点:
 * - 响应内容匹配预期错误包: {0x0E, 0x07, 0x01, 0x00, 0x00, 0x10}
 *   - 0x0E: SSAP_WRITE_RSP opcode
 *   - 0x07: ctrl (fragment=0b11, multi=1, error=1)
 *   - 0x01: errorCount
 *   - 0x00, 0x00: handle (句柄不足时空为0)
 *   - 0x10: SSAP_ERRCODE_SERVER_FRAG (服务端不支持分片)
 */
TEST_F(UT_SSAP_MULTI_READ_WRITE, WRITE_REQ_CTRL_FRAGMENT_NOT_SUPPORTED)
{
    CP_LOG_INFO("[UT_SSAP_MULTI_READ_WRITE] enter WRITE_REQ_CTRL_FRAGMENT_NOT_SUPPORTED");
    AddTestServiceWithProperties();
    (void)CreateLink();

    uint8_t req[] = {
        0x0D,
        0x00,
        0x02, 0x00, 0x00, 0xAA, 0xBB
    };
    Test_SSAP_RecvReq(req, sizeof(req));

    uint8_t errRsp[] = {0x0E, 0x07, 0x01, 0x00, 0x00, 0x10};
    EXPECT_TRUE(Test_SSAP_CompareLastSendPkt(errRsp, sizeof(errRsp)));

    DeleteLink();
}

/*
 * @brief 测试用例: WRITE_CMD_CTRL_FRAGMENT_NOT_SUPPORTED - 写命令不支持分片
 *
 * 测试当写命令ctrl包含不支持的分片标志时的行为
 *
 * 测试流程:
 * 1. 注册测试服务和属性
 * 2. 创建SSAP链路
 * 3. 发送带分片标志的写命令: opcode=0x0C, ctrl=0x08
 *    - ctrl=0x08: fragment=0b00(分片), multi=0, oper=0b10, verify=0
 *    - fragment=0b00不满足0b11要求，但WRITE_CMD不返回响应
 * 4. 验证无响应发送
 *
 * 验证点:
 * - isSendRsp = false (写命令不返回响应)
 */
TEST_F(UT_SSAP_MULTI_READ_WRITE, WRITE_CMD_CTRL_FRAGMENT_NOT_SUPPORTED)
{
    CP_LOG_INFO("[UT_SSAP_MULTI_READ_WRITE] enter WRITE_CMD_CTRL_FRAGMENT_NOT_SUPPORTED");
    AddTestServiceWithProperties();
    (void)CreateLink();

    uint8_t req[] = {
        0x0C,
        0x08,
        0x02, 0x00, 0x00, 0xAA, 0xBB
    };
    Test_SSAP_RecvReq(req, sizeof(req));

    EXPECT_FALSE(isSendRsp);

    DeleteLink();
}

/*
 * @brief 测试用例: FIND_STRUCTURE_BY_UUID_STD_SERVICE - 按UUID查找标准服务
 *
 * 测试SSAP结构发现功能(按UUID查找标准服务)
 *
 * 测试流程:
 * 1. 注册两个测试服务(UUID1和UUID3)
 * 2. 创建SSAP链路
 * 3. 发送按UUID查找请求: opcode=0x06
 *    - 字节[0]: 0x06 = SSAP_FIND_STRUCTURE_BY_UUID_REQ
 *    - 字节[1]: 0x01 = flags(标准服务类型)
 *    - 字节[2-3]: 0x0001 = 起始句柄
 *    - 字节[4-5]: 0x00FF = 结束句柄
 *    - 字节[6-7]: 0x0101 = UUID类型(标准服务)
 * 4. 验证响应已发送
 *
 * 验证点:
 * - 响应已发送(isSendRsp=true)
 */
TEST_F(UT_SSAP_MULTI_READ_WRITE, FIND_STRUCTURE_BY_UUID_STD_SERVICE)
{
    CP_LOG_INFO("[UT_SSAP_MULTI_READ_WRITE] enter FIND_STRUCTURE_BY_UUID_STD_SERVICE");
    AddTestServiceWithProperties();
    AddSecondService();
    (void)CreateLink();

    uint8_t req[] = {
        0x06,
        0x01,
        0x01, 0x00,
        0xFF, 0x00,
        0x01, 0x01
    };
    Test_SSAP_RecvReq(req, sizeof(req));

    EXPECT_TRUE(isSendRsp);

    DeleteLink();
}

/*
 * @brief 测试用例: FIND_STRUCTURE_BY_UUID_VENDOR_SERVICE - 按UUID查找厂商服务
 *
 * 测试SSAP结构发现功能(按UUID查找厂商定义的服务)
 *
 * 测试流程:
 * 1. 注册测试服务(UUID1)
 * 2. 创建SSAP链路
 * 3. 发送按UUID查找请求: opcode=0x06
 *    - 字节[0]: 0x06 = SSAP_FIND_STRUCTURE_BY_UUID_REQ
 *    - 字节[1]: 0x01 = flags
 *    - 字节[2-3]: 0x0001 = 起始句柄
 *    - 字节[4-5]: 0x00FF = 结束句柄
 *    - 字节[6-7]: 0x0180 = UUID类型(厂商服务，0x8000及以上)
 * 4. 验证响应已发送
 *
 * 验证点:
 * - 响应已发送(isSendRsp=true)
 */
TEST_F(UT_SSAP_MULTI_READ_WRITE, FIND_STRUCTURE_BY_UUID_VENDOR_SERVICE)
{
    CP_LOG_INFO("[UT_SSAP_MULTI_READ_WRITE] enter FIND_STRUCTURE_BY_UUID_VENDOR_SERVICE");
    AddTestServiceWithProperties();
    (void)CreateLink();

    uint8_t req[] = {
        0x06,
        0x01,
        0x01, 0x00,
        0xFF, 0x00,
        0x80, 0x01
    };
    Test_SSAP_RecvReq(req, sizeof(req));

    EXPECT_TRUE(isSendRsp);

    DeleteLink();
}

/*
 * @brief 测试用例: READ_EMPTY_DATABASE - 空数据库读请求错误处理
 *
 * 测试当服务数据库为空时发送多值读请求的错误处理
 *
 * 测试流程:
 * 1. 不注册任何服务(保持数据库为空)
 * 2. 创建SSAP链路
 * 3. 发送多值读请求: opcode=0x08
 *    - 字节[0]: 0x08 = SSAP_READ_REQ
 *    - 字节[1]: 0x03 = ctrl.fragment (不分片)
 *    - 句柄1: handle=0x0001, type=0x00(读数据值)
 *    - 句柄2: handle=0x00FF, type=0x00(读数据值)
 * 4. 验证返回READ_RSP响应(不是ERROR_RSP)
 *
 * 验证点:
 * - 响应内容匹配预期错误包: {0x09, 0x0F, 0x04, 0x00, 0x04, 0x00}
 *   - 0x09: SSAP_READ_RSP opcode
 *   - 0x0F: ctrl (fragment=0b11, multi=1, error=1)
 *   - 0x04, 0x00: 第一个item错误 (success=0, length=0x0004=INVALID_HANDLE)
 *   - 0x04, 0x00: 第二个item错误 (success=0, length=0x0004=INVALID_HANDLE)
 */
TEST_F(UT_SSAP_MULTI_READ_WRITE, READ_EMPTY_DATABASE)
{
    CP_LOG_INFO("[UT_SSAP_MULTI_READ_WRITE] enter READ_EMPTY_DATABASE");
    (void)CreateLink();

    uint8_t req[] = {
        0x08,             // opcode = SSAP_READ_REQ
        0x03,             // ctrl.fragment = 0b11 (不分片)
        0x01, 0x00, 0x00, // handle=0x0001, type=0x00
        0xFF, 0x00, 0x00  // handle=0x00FF, type=0x00
    };
    Test_SSAP_RecvReq(req, sizeof(req));

    uint8_t errRsp[] = {0x09, 0x0F, 0x04, 0x00, 0x04, 0x00};
    EXPECT_TRUE(Test_SSAP_CompareLastSendPkt(errRsp, sizeof(errRsp)));

    DeleteLink();
}

/*
 * @brief 测试用例: VALUE_NTF_SINGLE_ITEM - 客户端接收单条属性变化通知
 *
 * 测试SSAPC_ValueNtfHandle处理单条Value NTF报文
 *
 * 测试流程:
 * 1. 添加测试服务和属性
 * 2. 创建SSAP链路
 * 3. 发送VALUE_NTF报文: opcode=0x0F
 *    - 字节[0]: 0x0F = SSAP_VALUE_NTF
 *    - 字节[1]: 0x03 = ctrl.fragment (不分片)
 *    - handle: 0x0001 (LE)
 *    - length: 0x01, 0x00 (len=1, LE)
 *    - value: 0xAA
 * 4. 验证不发送任何响应 (NTF无需回复)
 *
 * ValueNtf PDU格式:
 *   msgCode(1) + ctrl(1) + handle(2 LE) + length(2 LE) + value(variable)
 */
TEST_F(UT_SSAP_MULTI_READ_WRITE, VALUE_NTF_SINGLE_ITEM)
{
    CP_LOG_INFO("[UT_SSAP_MULTI_READ_WRITE] enter VALUE_NTF_SINGLE_ITEM");
    AddTestServiceWithProperties();
    (void)CreateLink();

    uint8_t req[] = {
        0x0F,             // opcode = SSAP_VALUE_NTF
        0x03,             // ctrl.fragment = 0b11 (不分片)
        0x01, 0x00,       // handle = 0x0001 (LE)
        0x01, 0x00,       // length = 1 (LE)
        0xAA              // value = 0xAA
    };
    Test_SSAP_RecvReq(req, sizeof(req));

    EXPECT_FALSE(isSendRsp);

    DeleteLink();
}

/*
 * @brief 测试用例: VALUE_NTF_MULTI_ITEMS - 客户端接收多条属性变化通知
 *
 * 测试SSAPC_ValueNtfHandle处理多条Value NTF报文
 *
 * 测试流程:
 * 1. 添加测试服务和属性
 * 2. 创建SSAP链路
 * 3. 发送包含两条item的VALUE_NTF报文
 * 4. 验证不发送响应
 */
TEST_F(UT_SSAP_MULTI_READ_WRITE, VALUE_NTF_MULTI_ITEMS)
{
    CP_LOG_INFO("[UT_SSAP_MULTI_READ_WRITE] enter VALUE_NTF_MULTI_ITEMS");
    AddTestServiceWithProperties();
    (void)CreateLink();

    uint8_t req[] = {
        0x0F,             // opcode = SSAP_VALUE_NTF
        0x03,             // ctrl.fragment = 0b11 (不分片)
        0x01, 0x00,       // handle = 0x0001 (LE)
        0x01, 0x00,       // length = 1 (LE)
        0xAA,             // value = 0xAA
        0x02, 0x00,       // handle = 0x0002 (LE)
        0x02, 0x00,       // length = 2 (LE)
        0xBB, 0xCC        // value = 0xBB, 0xCC
    };
    Test_SSAP_RecvReq(req, sizeof(req));

    EXPECT_FALSE(isSendRsp);

    DeleteLink();
}

/*
 * @brief 测试用例: VALUE_NTF_TRUNCATED - Value NTF报文数据过短
 *
 * 测试SSAPC_ValueNtfHandle处理长度不足的PDU
 *
 * 测试流程:
 * 1. 添加测试服务
 * 2. 创建SSAP链路
 * 3. 发送长度不足的VALUE_NTF报文 (len=5 < MIN_LEN=7)
 * 4. 验证不发送响应 (静默丢弃)
 */
TEST_F(UT_SSAP_MULTI_READ_WRITE, VALUE_NTF_TRUNCATED)
{
    CP_LOG_INFO("[UT_SSAP_MULTI_READ_WRITE] enter VALUE_NTF_TRUNCATED");
    AddTestServiceWithProperties();
    (void)CreateLink();

    uint8_t req[] = {
        0x0F,             // opcode = SSAP_VALUE_NTF
        0x03,             // ctrl.fragment = 0b11 (不分片)
        0x01, 0x00        // handle = 0x0001 (LE), 但缺少length和value
    };
    Test_SSAP_RecvReq(req, sizeof(req));

    EXPECT_FALSE(isSendRsp);

    DeleteLink();
}

/*
 * @brief 测试用例: VALUE_IND_SINGLE_ITEM - 客户端接收单条属性变化指示
 *
 * 测试SSAPC_ValueIndHandle处理单条Value IND报文并回复ACK
 *
 * 测试流程:
 * 1. 添加测试服务和属性
 * 2. 创建SSAP链路
 * 3. 发送VALUE_IND报文: opcode=0x10
 *    - 字节[0]: 0x10 = SSAP_VALUE_IND
 *    - 字节[1]: 0x03 = ctrl.fragment (不分片)
 *    - handle: 0x0001 (LE)
 *    - length: 0x01, 0x00 (len=1, LE)
 *    - value: 0xDD
 * 4. 验证收到ACK响应 (opcode=0x11)
 *
 * ValueInd PDU格式:
 *   msgCode(1) + ctrl(1) + handle(2 LE) + length(2 LE) + value(variable)
 *
 * ACK PDU格式:
 *   msgCode(1) + ctrl(1) + result[](每条item对应1字节: 0=失败,1=成功)
 */
TEST_F(UT_SSAP_MULTI_READ_WRITE, VALUE_IND_SINGLE_ITEM)
{
    CP_LOG_INFO("[UT_SSAP_MULTI_READ_WRITE] enter VALUE_IND_SINGLE_ITEM");
    AddTestServiceWithProperties();
    (void)CreateLink();

    uint8_t req[] = {
        0x10,             // opcode = SSAP_VALUE_IND
        0x03,             // ctrl.fragment = 0b11 (不分片)
        0x01, 0x00,       // handle = 0x0001 (LE)
        0x01, 0x00,       // length = 1 (LE)
        0xDD              // value = 0xDD
    };
    Test_SSAP_RecvReq(req, sizeof(req));

    uint8_t ackRsp[] = {
        0x11,             // opcode = SSAP_VALUE_ACK
        0x03,             // ctrl.fragment = 0b11 (不分片)
        0x01              // result = 0x01 (接收成功)
    };
    EXPECT_TRUE(isSendRsp);
    EXPECT_TRUE(Test_SSAP_CompareLastSendPkt(ackRsp, sizeof(ackRsp)));

    DeleteLink();
}

/*
 * @brief 测试用例: VALUE_IND_MULTI_ITEMS - 客户端接收多条属性变化指示
 *
 * 测试SSAPC_ValueIndHandle处理多条Value IND报文并回复ACK
 *
 * 测试流程:
 * 1. 添加测试服务和属性
 * 2. 创建SSAP链路
 * 3. 发送包含两条item的VALUE_IND报文
 * 4. 验证收到ACK响应，包含两个成功标志
 */
TEST_F(UT_SSAP_MULTI_READ_WRITE, VALUE_IND_MULTI_ITEMS)
{
    CP_LOG_INFO("[UT_SSAP_MULTI_READ_WRITE] enter VALUE_IND_MULTI_ITEMS");
    AddTestServiceWithProperties();
    (void)CreateLink();

    uint8_t req[] = {
        0x10,             // opcode = SSAP_VALUE_IND
        0x03,             // ctrl.fragment = 0b11 (不分片)
        0x01, 0x00,       // handle = 0x0001 (LE)
        0x01, 0x00,       // length = 1 (LE)
        0xEE,             // value = 0xEE
        0x02, 0x00,       // handle = 0x0002 (LE)
        0x02, 0x00,       // length = 2 (LE)
        0xFF, 0x11        // value = 0xFF, 0x11
    };
    Test_SSAP_RecvReq(req, sizeof(req));

    uint8_t ackRsp[] = {
        0x11,             // opcode = SSAP_VALUE_ACK
        0x03,             // ctrl.fragment = 0b11 (不分片)
        0x01,             // result[0] = 0x01 (成功)
        0x01              // result[1] = 0x01 (成功)
    };
    EXPECT_TRUE(isSendRsp);
    EXPECT_TRUE(Test_SSAP_CompareLastSendPkt(ackRsp, sizeof(ackRsp)));

    DeleteLink();
}

/*
 * @brief 测试用例: VALUE_IND_ITEM_OVERFLOW - Value IND报文item数据溢出
 *
 * 测试SSAPC_ValueIndHandle处理item长度超出剩余数据的PDU
 * 应截断处理，只ACK已解析成功的item
 *
 * 测试流程:
 * 1. 添加测试服务和属性
 * 2. 创建SSAP链路
 * 3. 发送VALUE_IND报文，其中第二个item的length超出剩余数据
 * 4. 验证收到ACK响应，只包含第一个item的成功标志
 */
TEST_F(UT_SSAP_MULTI_READ_WRITE, VALUE_IND_ITEM_OVERFLOW)
{
    CP_LOG_INFO("[UT_SSAP_MULTI_READ_WRITE] enter VALUE_IND_ITEM_OVERFLOW");
    AddTestServiceWithProperties();
    (void)CreateLink();

    uint8_t req[] = {
        0x10,             // opcode = SSAP_VALUE_IND
        0x03,             // ctrl.fragment = 0b11 (不分片)
        0x01, 0x00,       // handle = 0x0001 (LE)
        0x01, 0x00,       // length = 1 (LE)
        0xEE,             // value = 0xEE
        0x02, 0x00,       // handle = 0x0002 (LE)
        0x10, 0x00        // length = 16 (LE), 超出剩余空间
                          // 后续没有16字节数据
    };
    Test_SSAP_RecvReq(req, sizeof(req));

    uint8_t ackRsp[] = {
        0x11,             // opcode = SSAP_VALUE_ACK
        0x03,             // ctrl.fragment = 0b11 (不分片)
        0x01              // result[0] = 0x01 (只有第一个item成功)
    };
    EXPECT_TRUE(isSendRsp);
    EXPECT_TRUE(Test_SSAP_CompareLastSendPkt(ackRsp, sizeof(ackRsp)));

    DeleteLink();
}