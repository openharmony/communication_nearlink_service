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

#include "dtap_errno.h"
#include "cm_errno.h"
#include "cm_signaling_cap.h"
#include "cm_logic_link_api.h"
#include "cm_signaling_manage.h"
#include "cm_signaling_version.h"
#include "securec.h"
#include "sdf_worker.h"
#include "sdf_map.h"
#include "sle_logic_link_mgr.h"
#include "sdf_evc.h"
#include "sdf_thread.h"
#include "nlstk_schedule.h"

#define DLI_TEST_NUM 10
#define DLI_TEST_LEN 100
#define TEST_ERR 0xffffffff
#define TEST_DEFAULT_NUM 10

extern "C" {
static SLE_Addr_S g_publicAddress = {.type = PUBLIC_ADDRESS, .addr = {0}};
static CM_LogicLinkCbks_S g_logicLinkCbk = {};

SLE_Addr_S *NBC_GetPublicAddress(void)
{
    return &g_publicAddress;
}

uint32_t CM_RegLogicLinkListener(CM_LogicLinkCbks_S *cbks)
{
    g_logicLinkCbk = *cbks;
    return CM_SUCCESS;
}

uint32_t CM_UnRegLogicLinkListener(uint8_t moduleId)
{
    (void)memset_s(&g_logicLinkCbk, sizeof(CM_LogicLinkCbks_S), 0x0, sizeof(CM_LogicLinkCbks_S));
    return CM_SUCCESS;
}
}

class UT_CAP_SIGNALING : public testing::Test {
protected:
    // SetUP 在每一个 TEST_F 测试开始前执行一次
    virtual void SetUp()
    {}

    // TearDown 在每一个 TEST_F 测试完成后执行一次
    virtual void TearDown()
    {}

    // SetUpTestCase 在所有 TEST_F 测试开始前执行一次
    static void SetUpTestCase()
    {
        SDF_ThreadInit(TEST_DEFAULT_NUM);
        SDF_EvcInit();
        ScheduleEnable();
        SleLogicLinkInit();
        // 打桩初始化建立1条链路供测试用例使用
        SLE_Addr_S addr = {1};
        SleLogicLink_S *link = SleLogicLinkAdd(&addr);
        EXPECT_NE(link, nullptr);
        link->lcid = 0;
        link->status = CM_LINK_STATE_CONNECTED;
        EXPECT_EQ(CM_SignalingCacheInit(), CM_SUCCESS);
    }

    // TearDownTestCase 在所有 TEST_F 测试完成后执行一次
    static void TearDownTestCase()
    {
        CM_SignalingCacheDeinit();
        SleLogicLinkDeInit();
        ScheduleDisable();
        SDF_EvcDeinit();
        SDF_ThreadDeinit();
    }
};

static uint32_t CM_SendSignalingDataCbkTest(uint8_t pi, uint8_t tcid, uint16_t lcid, SDF_Buff_S *buff)
{
    (void)pi;
    (void)tcid;
    if (lcid == 0xff || buff == NULL) {
        return TEST_ERR;
    }
    SDF_BuffFree(buff);
    return 0;
}

TEST_F(UT_CAP_SIGNALING, CapSignalingSendReqTest)
{
    CM_SetSendSignalingDataCbk(CM_SendSignalingDataCbkTest);
    CM_CapabilityBitmap_S cap = {0};
    cap.relayCap = CM_CAP_ENABLE;
    cap.transMode = CM_CAP_ENABLE;
    cap.measurementCap = CM_CAP_ENABLE;
    cap.accessSlb = CM_CAP_ENABLE;
    cap.accessSle = CM_CAP_ENABLE;
    cap.version = CM_CAP_ENABLE;
    cap.mps = CM_CAP_ENABLE;
    cap.mtu = CM_CAP_ENABLE;
    EXPECT_EQ(CM_SendReqSignalingCapability(0, &cap), 0);
    EXPECT_EQ(CM_SendReqSignalingCapability(0xff, &cap), TEST_ERR);

    CM_SetSendSignalingDataCbk(NULL);
    EXPECT_EQ(CM_SendBuffToDtap(0, NULL), CM_INVALID_PARAM_ERR);
}

TEST_F(UT_CAP_SIGNALING, CapSignalingProcessRspTest)
{
    uint8_t data[] = {0x02,
        00,
        0x12,
        00,
        0xea,
        00,
        00,
        00,
        0x01,
        00,
        0x12,
        0xcf,
        0x01,
        00,
        0x01,
        00,
        0xa0,
        0x02,
        0xa0,
        0x02,
        0x01,
        0x01};
    SDF_Buff_S *buff = SDF_BuffNewWithReserve(DLI_TEST_LEN);
    ASSERT_TRUE(buff != nullptr);
    uint8_t *tmp = SDF_BuffAppend(buff, sizeof(data));
    (void)memcpy_s(tmp, sizeof(data), data, sizeof(data));
    DTAP_Data_Info_S info = {0};
    CM_RecvSignalingData(&info, NULL);
    EXPECT_EQ(CM_ProcessRspSignalingCapability(0, NULL), CM_INVALID_PARAM_ERR);
    CM_RecvSignalingData(&info, buff);
    SDF_BuffFree(buff);
}

TEST_F(UT_CAP_SIGNALING, CapSignalingProcessReqTest)
{
    CM_SetSendSignalingDataCbk(CM_SendSignalingDataCbkTest);
    EXPECT_EQ(CM_ProcessReqSignalingCapability(0, NULL), CM_INVALID_PARAM_ERR);
    uint8_t data[] = {0x01, 0x00, 0x04, 0x00, 0xff, 0x00, 0x00, 0x00};
    SDF_Buff_S *buff = SDF_BuffNewWithReserve(DLI_TEST_LEN);
    ASSERT_TRUE(buff != nullptr);
    uint8_t *tmp = SDF_BuffAppend(buff, sizeof(data));
    (void)memcpy_s(tmp, sizeof(data), data, sizeof(data));
    DTAP_Data_Info_S info = {0};
    CM_RecvSignalingData(&info, buff);
    SDF_BuffFree(buff);
}

TEST_F(UT_CAP_SIGNALING, CapSignalingProcessErrBuff)
{
    CM_SetSendSignalingDataCbk(CM_SendSignalingDataCbkTest);
    EXPECT_EQ(CM_ProcessReqSignalingCapability(0, NULL), CM_INVALID_PARAM_ERR);
    uint8_t data[] = {0x01, 0x00};
    SDF_Buff_S *buff = SDF_BuffNewWithReserve(DLI_TEST_LEN);
    ASSERT_TRUE(buff != nullptr);
    uint8_t *tmp = SDF_BuffAppend(buff, sizeof(data));
    (void)memcpy_s(tmp, sizeof(data), data, sizeof(data));
    DTAP_Data_Info_S info = {0};
    CM_RecvSignalingData(&info, buff);
    SDF_BuffFree(buff);
}

TEST_F(UT_CAP_SIGNALING, CapSignalingVersion)
{
    EXPECT_NE(0, CM_GetLogicLinkDeviceType(0));
    CM_SetLinkProtocolVersion(0, 1);
    CM_SetLinkExchangeVersion(0, 0);
    CM_SetDeviceLinkDeviceType(0, true);
    EXPECT_NE(0, CM_GetLogicLinkDeviceType(0));

    SleLogicLink_S *link = SleLogicLinkGetByLcid(0);
    link->devType = 0;
    link->companyId = 0;
    CM_SetDeviceLinkDeviceType(0, true);
    EXPECT_EQ(CM_DEVTYPE_THIRD_PARTY, link->devType);

    link->devType = 0;
    link->companyId = 0x007C;  // haisi companyId
    CM_SetDeviceLinkDeviceType(0, true);
    EXPECT_EQ(CM_DEVTYPE_NEW, link->devType);
}

TEST_F(UT_CAP_SIGNALING, CapSignalinManage)
{
    EXPECT_EQ(0, CM_SignalingCacheInsert(0, 0, 0, NULL, NULL));
    EXPECT_EQ(0, CM_SignalingCacheInsert(1, 1, 0, NULL, NULL));
    EXPECT_EQ(0, CM_SignalingCacheInsert(1, 2, 0, NULL, NULL));
    CM_SignalingCacheRemove(0, 0);
    CM_SignalingCacheRemove(0, 1);
    CM_SignalingCacheClearByLcid(0);
    CM_SignalingCacheClearByLcid(1);
}

TEST_F(UT_CAP_SIGNALING, CM_SignalingInitAndCallbackAndDeinit)
{
    CM_SignalingDeInit();
    EXPECT_EQ(0, CM_SignalingInit());
    EXPECT_EQ(g_logicLinkCbk.moduleId, CM_MODULE_CM_SIGNALING);
    CM_LogicLinkState_S state;
    state.result = CM_LINK_STATE_CONNECTING;
    g_logicLinkCbk.logicLinkCbk(&state);
    state.result = CM_LINK_STATE_CONNECTED;
    g_logicLinkCbk.logicLinkCbk(&state);
    state.result = CM_LINK_STATE_DISCONNECTTING;
    g_logicLinkCbk.logicLinkCbk(&state);
    state.result = CM_LINK_STATE_DISCONNECTED;
    g_logicLinkCbk.logicLinkCbk(&state);
    CM_SignalingDeInit();
}

TEST_F(UT_CAP_SIGNALING, CapSignalinManageOthers)
{
    CM_SignalingCacheDeinit();
    CM_SignalingCacheClearByLcid(0);
    CM_SignalingCacheRemove(0, 1);
    EXPECT_NE(0, CM_SignalingCacheInsert(0, 0, 0, NULL, NULL));
}