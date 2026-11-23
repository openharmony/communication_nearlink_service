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

#include "stack_schedule_mock.h"
#include "stack_schedule_stub.h"
#include "stack_dli_cmd_mock.h"
#include "stack_dli_cmd_stub.h"
#include "dli_callback.h"
#include "cp_worker.h"

#include "nlstk_init_api.h"
#include "nlstk_cfgdb.h"
#include "nlstk_cfgdb_api.h"
#include "dli_event.h"
#include "dli_event_struct.h"
#include "dli_opcode.h"
#include "dli_errno.h"
#include "nlstk_dft.h"
#include "nlstk_dft_api.h"
#include "nlstk_log.h"

extern "C" {
void DftGetCurTimestampStr(NLSTK_DftParamValueString_S *str);
}

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

static void TEST_DtapRecvAcbHandler(uint16_t lcid, SDF_Buff_S *buf)
{
}

static void TestDliInit(void)
{
    DLI_Callback dliCallback = {0};
    dliCallback.postOtherThread = CP_PostTask;
    dliCallback.postOtherBlockedThread = CP_PostTaskBlocked;
    dliCallback.dftReportKill = NULL;
    dliCallback.recvAcbHandler = TEST_DtapRecvAcbHandler;
    DLI_SetCallback(&dliCallback);
    return;
}

class UT_CFGDB_TEST : public testing::Test {
public:
    NiceMock<ScheduleMock> scheduleMock;
    NiceMock<DliCmdMock> dliCmdMock;
protected:
    void SetUp() override
    {
        // 步骤1：初始化桩函数
        TestDliInit();
        TEST_ScheduleInit();
        EXPECT_CALL(scheduleMock, SchedulePostTask).WillRepeatedly(TEST_SchedulePostTaskStub);
        EXPECT_CALL(scheduleMock, SchedulePostTaskBlocked).WillRepeatedly(TEST_SchedulePostTaskBlockedStub);
    }

    void TearDown() override
    {
        TEST_StackScheduleDeInit();
    }
};

// 调用DLI_ReadLocalVersion返回失败
TEST_F(UT_CFGDB_TEST, UT_CFGDB_001)
{
    EXPECT_CALL(dliCmdMock, DLI_ReadLocalVersion).WillRepeatedly(TEST_DLI_ReadLocalVersionFail);
    uint32_t ret = CfgdbInit();
    ASSERT_EQ(ret, 0);

    ret = CfgdbReadCommConfigValue();
    ASSERT_EQ(ret, 0);

    CfgdbDeinit();
}

// 调用DLI_GetPublicAddress返回失败
TEST_F(UT_CFGDB_TEST, UT_CFGDB_002)
{
    EXPECT_CALL(dliCmdMock, DLI_ReadLocalVersion).WillRepeatedly(TEST_DLI_ReadLocalVersion);
    EXPECT_CALL(dliCmdMock, DLI_GetPublicAddress).WillRepeatedly(TEST_DLI_GetPublicAddressFail);
    uint32_t ret = CfgdbInit();
    ASSERT_EQ(ret, 0);

    ret = CfgdbReadCommConfigValue();
    ASSERT_EQ(ret, 0);

    CfgdbDeinit();
}

TEST_F(UT_CFGDB_TEST, UT_CFGDB_003)
{
    EXPECT_CALL(dliCmdMock, DLI_ReadLocalVersion).WillRepeatedly(TEST_DLI_ReadLocalVersion);
    EXPECT_CALL(dliCmdMock, DLI_GetPublicAddress).WillRepeatedly(TEST_DLI_GetPublicAddress);
    EXPECT_CALL(dliCmdMock, DLI_ReadMaximumAdvDataLen).WillRepeatedly(TEST_DLI_ReadMaximumAdvDataLenFail);
    uint32_t ret = CfgdbInit();
    ASSERT_EQ(ret, 0);

    ret = CfgdbReadCommConfigValue();
    ASSERT_EQ(ret, 0);

    CfgdbDeinit();
}

TEST_F(UT_CFGDB_TEST, UT_CFGDB_004)
{
    EXPECT_CALL(dliCmdMock, DLI_ReadLocalVersion).WillRepeatedly(TEST_DLI_ReadLocalVersion);
    EXPECT_CALL(dliCmdMock, DLI_GetPublicAddress).WillRepeatedly(TEST_DLI_GetPublicAddress);
    EXPECT_CALL(dliCmdMock, DLI_ReadMaximumAdvDataLen).WillRepeatedly(TEST_DLI_ReadMaximumAdvDataLen);
    EXPECT_CALL(dliCmdMock, DLI_ReadAdvSetsNum).WillRepeatedly(TEST_DLI_ReadAdvSetsNumFail);
    uint32_t ret = CfgdbInit();
    ASSERT_EQ(ret, 0);

    ret = CfgdbReadCommConfigValue();
    ASSERT_EQ(ret, 0);

    CfgdbDeinit();
}

TEST_F(UT_CFGDB_TEST, UT_CFGDB_005)
{
    EXPECT_CALL(dliCmdMock, DLI_ReadLocalVersion).WillRepeatedly(TEST_DLI_ReadLocalVersion);
    EXPECT_CALL(dliCmdMock, DLI_GetPublicAddress).WillRepeatedly(TEST_DLI_GetPublicAddress);
    EXPECT_CALL(dliCmdMock, DLI_ReadMaximumAdvDataLen).WillRepeatedly(TEST_DLI_ReadMaximumAdvDataLen);
    EXPECT_CALL(dliCmdMock, DLI_ReadAdvSetsNum).WillRepeatedly(TEST_DLI_ReadAdvSetsNum);
    EXPECT_CALL(dliCmdMock, DLI_ReadSupportCryptoAlgo).WillRepeatedly(TEST_DLI_ReadSupportCryptoAlgoFail);
    uint32_t ret = CfgdbInit();
    ASSERT_EQ(ret, 0);

    ret = CfgdbReadCommConfigValue();
    ASSERT_EQ(ret, 0);

    CfgdbDeinit();
}

TEST_F(UT_CFGDB_TEST, UT_CFGDB_006)
{
    EXPECT_CALL(dliCmdMock, DLI_ReadLocalVersion).WillRepeatedly(TEST_DLI_ReadLocalVersion);
    EXPECT_CALL(dliCmdMock, DLI_GetPublicAddress).WillRepeatedly(TEST_DLI_GetPublicAddress);
    EXPECT_CALL(dliCmdMock, DLI_ReadMaximumAdvDataLen).WillRepeatedly(TEST_DLI_ReadMaximumAdvDataLen);
    EXPECT_CALL(dliCmdMock, DLI_ReadAdvSetsNum).WillRepeatedly(TEST_DLI_ReadAdvSetsNum);
    EXPECT_CALL(dliCmdMock, DLI_ReadSupportCryptoAlgo).WillRepeatedly(TEST_DLI_ReadSupportCryptoAlgo);
    EXPECT_CALL(dliCmdMock, DLI_ReadLocalFeatures).WillRepeatedly(TEST_DLI_ReadLocalFeaturesFail);
    uint32_t ret = CfgdbInit();
    ASSERT_EQ(ret, 0);

    ret = CfgdbReadCommConfigValue();
    ASSERT_EQ(ret, 0);

    CfgdbDeinit();
}
static int g_cfgdbListenerCalls = 0;
static int g_cfgdbAnotherListenerCalls = 0;

static void CountingListener(void *args)
{
    (void)args;
    g_cfgdbListenerCalls++;
}

static void AnotherCountingListener(void *args)
{
    (void)args;
    g_cfgdbAnotherListenerCalls++;
}

// 未初始化时读配置被拒、反初始化安全
TEST_F(UT_CFGDB_TEST, UT_CFGDB_007_UninitedGuards)
{
    CfgdbDeinit();
    EXPECT_EQ(CfgdbReadCommConfigValue(), NLSTK_ERR);
}

// 本地版本读取回调只通知已注册监听的模块，未注册槽位被跳过
TEST_F(UT_CFGDB_TEST, UT_CFGDB_008_VersionNotifyReachesRegisteredModule)
{
    ASSERT_EQ(CfgdbInit(), 0);
    g_cfgdbListenerCalls = 0;
    ASSERT_EQ(NLSTK_CfgdbRegisterConfigListener(CountingListener, NLSTK_CFGDB_MODULE_QOSM,
        NLSTK_CFGDB_CONFIG_LOCAL_VERSION), NLSTK_ERRCODE_SUCCESS);

    uint8_t version[5] = {7, 0, 1, 0, 2}; // 版本、厂商标识、子版本
    DLI_RunRegCbk(DLI_CBK_READ_LOCAL_VERSION, nullptr, version, sizeof(version), 0x0100, DLI_SUCCESS);
    EXPECT_EQ(g_cfgdbListenerCalls, 1);
    EXPECT_EQ(CfgdbGetLocalVersion(), 7);

    CfgdbDeinit();
}

// 本地特性回调按特性位分流：不支持测量能力时通知信号量，支持时续读测量能力
TEST_F(UT_CFGDB_TEST, UT_CFGDB_009_LocalFeatureCsBitRoutesMeasureCaps)
{
    ASSERT_EQ(CfgdbInit(), 0);
    g_cfgdbListenerCalls = 0;
    ASSERT_EQ(NLSTK_CfgdbRegisterConfigListener(CountingListener, NLSTK_CFGDB_MODULE_QOSM,
        NLSTK_CFGDB_CONFIG_LOCAL_FEATURE), NLSTK_ERRCODE_SUCCESS);

    uint8_t feats[NLSTK_CFGDB_LOCAL_FEATURE_LEN] = {0};
    DLI_RunRegCbk(DLI_CBK_READ_LOCAL_FEATURE, nullptr, feats, sizeof(feats), 0x0100, DLI_SUCCESS);
    EXPECT_EQ(g_cfgdbListenerCalls, 1); // 不含测量能力位，直接通知完成

    feats[NLSTK_CFGDB_LOCAL_FEATURE_LEN - 2] = 0x4; // 特性表第 8 字节含测量能力位
    DLI_RunRegCbk(DLI_CBK_READ_LOCAL_FEATURE, nullptr, feats, sizeof(feats), 0x0100, DLI_SUCCESS);
    EXPECT_EQ(g_cfgdbListenerCalls, 2); // 含测量能力位，转读测量能力

    CfgdbDeinit();
}

// 无协同能力时最大广播节点数取本端值，读取回调的协同通知被跳过
TEST_F(UT_CFGDB_TEST, UT_CFGDB_010_MaxAdvNodesWithoutCollab)
{
    ASSERT_EQ(CfgdbInit(), 0);

    uint8_t five = 5;
    DLI_RunRegCbk(DLI_CBK_READ_ADV_SETS_NUM, nullptr, &five, sizeof(five), 0x0100, DLI_SUCCESS);
    EXPECT_EQ(CfgdbGetMaxAdvNodesNum(), 5); // 大于默认值时扣除协同配额（协同为 0）

    uint8_t one = 1;
    DLI_RunRegCbk(DLI_CBK_READ_ADV_SETS_NUM, nullptr, &one, sizeof(one), 0x0100, DLI_SUCCESS);
    EXPECT_EQ(CfgdbGetMaxAdvNodesNum(), 1); // 不大于默认值时原样返回

    CfgdbDeinit();
}

// 部分回调缺省的注册请求只注册已填回调，未填槽位被跳过且不影响其他模块监听
TEST_F(UT_CFGDB_TEST, UT_CFGDB_011_RegisterCbksWithPartialCallbacks)
{
    ASSERT_EQ(CfgdbInit(), 0);
    g_cfgdbListenerCalls = 0;
    g_cfgdbAnotherListenerCalls = 0;
    ASSERT_EQ(NLSTK_CfgdbRegisterConfigListener(AnotherCountingListener, NLSTK_CFGDB_MODULE_QOSM,
        NLSTK_CFGDB_CONFIG_RSSI), NLSTK_ERRCODE_SUCCESS);

    NLSTK_CfgdbCbk_S cbks = {nullptr};
    cbks.rssiCbk = CountingListener; // 其余回调留空
    ASSERT_EQ(NLSTK_CfgdbRegisterCbks(&cbks), NLSTK_ERRCODE_SUCCESS);

    uint8_t rssi[2] = {1, 2};
    DLI_RunRegCbk(DLI_CBK_RSSI_CHANGE, nullptr, rssi, sizeof(rssi), 0x0100, DLI_SUCCESS);
    EXPECT_EQ(g_cfgdbListenerCalls, 1);
    EXPECT_EQ(g_cfgdbAnotherListenerCalls, 1);

    CfgdbDeinit();
}

// 删除不存在的厂商能力地址是空操作，查询仍返回不支持
TEST_F(UT_CFGDB_TEST, UT_CFGDB_012_RemoveUnknownManufacturerAddr)
{
    ASSERT_EQ(CfgdbInit(), 0);
    SLE_Addr_S addr = {};
    ASSERT_EQ(NLSTK_CfgdbRemoveManufacturerAbility(&addr), NLSTK_ERRCODE_SUCCESS);
    EXPECT_FALSE(CfgdbGetManufacturerSupport(&addr, static_cast<CfgdbManufacturerAbility_E>(0)));
    CfgdbDeinit();
}

// 打点入口的空入参与未注册回调路径安全
TEST_F(UT_CFGDB_TEST, UT_CFGDB_013_NlstkDftGuards)
{
    EXPECT_NE(NLSTK_DftRegisterCallback(nullptr), NLSTK_ERRCODE_SUCCESS);

    DftGetCurTimestampStr(nullptr); // 空串缓冲被拒
    SLE_Addr_S addr = {};
    uint16_t value = 3;
    DftCache(&addr, 17, 1, NLSTK_DFT_PARAM_VALUE_TYPE_UINT16, &value); // 未注册缓存回调
    DftReport(&addr, 17, 1, 0); // 未注册上报回调
}
