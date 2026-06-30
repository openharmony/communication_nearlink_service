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

#include "sdf_addr.h"
#include "sdf_mem.h"

#include "cm_def.h"
#include "dtap_tcid.h"
#include "ssap_link.h"
#include "ssap_manager.h"
#include "ssaps_service.h"
#include "ssapc_cache.h"

#include "nlstk_dis_server.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

static uint8_t g_manufactureVal[] = {0x01};
static uint8_t g_deviceModelVal[] = {0x02};
static uint8_t g_serialNumberVal[] = {0x03};
static uint8_t g_hardwareVersionVal[] = {0x04};
static uint8_t g_firmwareVersionVal[] = {0x05};
static uint8_t g_softwareVersionVal[] = {0x06};
static uint8_t g_deviceLocalAliasVal[] = {0x07};
static uint8_t g_updateLocalAliasVal[] = {0x07, 0x07};
static uint8_t g_deviceAppearance = 8;

class IT_DIS_SERVER_TEST : public testing::Test {
public:
    NiceMock<ScheduleMock> scheduleMock;
protected:
    void SetUp() override
    {
        // 步骤1：初始化桩函数
        TEST_ScheduleInit();
        EXPECT_CALL(scheduleMock, SchedulePostTask).WillRepeatedly(TEST_SchedulePostTaskQueueStub);
        EXPECT_CALL(scheduleMock, SchedulePostTaskBlocked).WillRepeatedly(TEST_SchedulePostTaskBlockedStub);
        EXPECT_CALL(scheduleMock, ScheduleTimerAdd).WillRepeatedly(TEST_ScheduleTimerAddStub);
        EXPECT_CALL(scheduleMock, ScheduleTimerDel).WillRepeatedly(TEST_ScheduleTimerDelStub);

        // 步骤2：初始化SSAP模块
        SSAP_LinkInit();
        SSAP_Init();
    }

    void TearDown() override
    {
        SSAP_DeInit();

        TEST_StackScheduleDeInit();
    }
};

static void TEST_SetDevInfo(NLSTK_DeviceInfo_S *devInfo)
{
    devInfo->deviceAppearance = g_deviceAppearance;
    devInfo->manufacturerInfo.data = g_manufactureVal;
    devInfo->manufacturerInfo.len = sizeof(g_manufactureVal);
    devInfo->deviceModel.data = g_deviceModelVal;
    devInfo->deviceModel.len = sizeof(g_deviceModelVal);
    devInfo->deviceSerialNumber.data = g_serialNumberVal;
    devInfo->deviceSerialNumber.len = sizeof(g_serialNumberVal);
    devInfo->hardwareVersion.data = g_hardwareVersionVal;
    devInfo->hardwareVersion.len = sizeof(g_hardwareVersionVal);
    devInfo->firmwareVersion.data = g_firmwareVersionVal;
    devInfo->firmwareVersion.len = sizeof(g_firmwareVersionVal);
    devInfo->softwareVersion.data = g_softwareVersionVal;
    devInfo->softwareVersion.len = sizeof(g_softwareVersionVal);
    devInfo->deviceLocalAlias.data = g_deviceLocalAliasVal;
    devInfo->deviceLocalAlias.len = sizeof(g_deviceLocalAliasVal);
}

/**
 * @test DIS_SERVER_CREATE_001
 * @brief 此测试用例用于验证DIS服务端创建实例。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 创建服务端实例
 * 2. 更新服务端别名
 * 3. 删除服务端实例
 *
 * @pre
 * - SSAP模块已初始化。
 * - DIS子模块已初始化。
 *
 * @post
 * - 预期结果：DIS服务端创建实例成功
 */
TEST_F(IT_DIS_SERVER_TEST, DIS_SERVER_CREATE_001)
{
    NLSTK_DeviceInfo_S devInfo = {{0}};
    TEST_SetDevInfo(&devInfo);
    NLSTK_Errcode_E ret = NLSTK_DisCreateInstance(&devInfo);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);

    NLSTK_VariableData_S updateName = {0};
    updateName.data = g_updateLocalAliasVal;
    updateName.len = sizeof(g_updateLocalAliasVal);
    ret = NLSTK_DisUpdateLocalDeviceName(&updateName);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);

    ret = NLSTK_DisDestroyInstance();
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
}

// 添加销毁实例任务失败
TEST_F(IT_DIS_SERVER_TEST, DIS_SERVER_CREATE_FAIL_001)
{
    EXPECT_CALL(scheduleMock, SchedulePostTask).WillRepeatedly(TEST_SchedulePostTaskStubFail);

    NLSTK_Errcode_E ret = NLSTK_DisDestroyInstance();
    EXPECT_EQ(ret, NLSTK_ERRCODE_TASK_FAIL);
}