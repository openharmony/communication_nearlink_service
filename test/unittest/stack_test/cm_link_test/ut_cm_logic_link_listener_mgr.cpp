/*
 * Copyright (C) 2025-2025 Huawei Device Co., Ltd.
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
#include "cm_log.h"
#include "cm_exter_cbks_mgr.h"
#include "cm_logic_link_listener_mgr.h"
#include "securec.h"
#include "sdf_mem.h"

class UT_CM_LOGIC_LINK_LISTENER_MGR : public testing::Test {
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
        CM_LOGI("SetUpTestCase");
    }

    // TearDownTestCase 在所有 TEST_F 测试完成后执行一次
    static void TearDownTestCase()
    {
        CM_LOGI("TearDownTestCase stub reset");
        // Don't call STUB_Reset
        CM_LOGI("TearDownTestCase");
    }
};

TEST_F(UT_CM_LOGIC_LINK_LISTENER_MGR, CM_NotifyLogicLinkCbks)
{
    SLE_Addr_S *addr = (SLE_Addr_S *)SDF_MemAlloc(sizeof(SLE_Addr_S));
    memset_s(addr, sizeof(SLE_Addr_S), 0x00, sizeof(SLE_Addr_S));
    EXPECT_NE(addr, nullptr);
    CM_LogicLinkState_S *state = (CM_LogicLinkState_S *)SDF_MemAlloc(sizeof(CM_LogicLinkState_S));
    memset_s(state, sizeof(CM_LogicLinkState_S), 0x00, sizeof(CM_LogicLinkState_S));
    state->addr = *addr;
    state->result = CM_LINK_STATE_CONNECTED;
    CM_NotifyLogicLinkCbks(state);
    EXPECT_NE(state, nullptr);
    state->result = CM_LINK_STATE_DISCONNECTED;
    CM_NotifyLogicLinkCbks(state);
    EXPECT_NE(state, nullptr);

    SDF_MemFree(addr);
    SDF_MemFree(state);
}

TEST_F(UT_CM_LOGIC_LINK_LISTENER_MGR, CM_ExecLogicLinkRemoteFeaturesCbks)
{
    CM_LogicLinkRemoteFeatures_S *state =
        (CM_LogicLinkRemoteFeatures_S *)SDF_MemAlloc(sizeof(CM_LogicLinkRemoteFeatures_S));
    memset_s(state, sizeof(CM_LogicLinkRemoteFeatures_S), 0x00, sizeof(CM_LogicLinkRemoteFeatures_S));
    EXPECT_NE(state, nullptr);
    CM_ExecLogicLinkRemoteFeaturesCbks(state);
    EXPECT_NE(state, nullptr);
    SDF_MemFree(state);
}

TEST_F(UT_CM_LOGIC_LINK_LISTENER_MGR, CM_RegExterCbks)
{
    CM_ConnectCbks_S cbk = {0};
    EXPECT_NE(&cbk, nullptr);
    CM_RegExterCbks(&cbk);
    EXPECT_NE(&cbk, nullptr);
}

TEST_F(UT_CM_LOGIC_LINK_LISTENER_MGR, CM_ExecuteEventCbk)
{
    uint8_t *param1 = (uint8_t *)SDF_MemAlloc(sizeof(uint8_t));
    EXPECT_NE(param1, nullptr);
    memset_s(param1, sizeof(uint8_t), 0x00, sizeof(uint8_t));
    CM_ExecuteEventCbk(CM_SLE_CBK_EVENT_CONNECT_CANCEL, param1);

    CM_ConnectRemoteUpdateParamReq_S *param2 =
        (CM_ConnectRemoteUpdateParamReq_S *)SDF_MemAlloc(sizeof(CM_ConnectRemoteUpdateParamReq_S));
    EXPECT_NE(param2, nullptr);
    memset_s(param2, sizeof(CM_ConnectRemoteUpdateParamReq_S), 0x00, sizeof(CM_ConnectRemoteUpdateParamReq_S));
    CM_ExecuteEventCbk(CM_SLE_CBK_EVENT_CONNECT_REMOTE_UPDATE_PARAM_REQ, param2);

    CM_ConnectUpdateParamRsp_S *param3 = (CM_ConnectUpdateParamRsp_S *)SDF_MemAlloc(sizeof(CM_ConnectUpdateParamRsp_S));
    EXPECT_NE(param3, nullptr);
    memset_s(param3, sizeof(CM_ConnectUpdateParamRsp_S), 0x00, sizeof(CM_ConnectUpdateParamRsp_S));
    CM_ExecuteEventCbk(CM_SLE_CBK_EVENT_CONNECT_PARAM_UPDATE, param3);

    CM_ReadRemoteFeatureVersionRsp_S *param4 =
        (CM_ReadRemoteFeatureVersionRsp_S *)SDF_MemAlloc(sizeof(CM_ReadRemoteFeatureVersionRsp_S));
    EXPECT_NE(param4, nullptr);
    memset_s(param4, sizeof(CM_ReadRemoteFeatureVersionRsp_S), 0x00, sizeof(CM_ReadRemoteFeatureVersionRsp_S));
    CM_ExecuteEventCbk(CM_SLE_CBK_EVENT_READ_REMOTE_FEATURE_VERSION, param4);

    CM_SetPhyRsp_S *param5 = (CM_SetPhyRsp_S *)SDF_MemAlloc(sizeof(CM_SetPhyRsp_S));
    EXPECT_NE(param5, nullptr);
    memset_s(param5, sizeof(CM_SetPhyRsp_S), 0x00, sizeof(CM_SetPhyRsp_S));
    CM_ExecuteEventCbk(CM_SLE_CBK_EVENT_SET_PHY, param5);

    CM_ReadLocalFeatureRsp_S *param6 = (CM_ReadLocalFeatureRsp_S *)SDF_MemAlloc(sizeof(CM_ReadLocalFeatureRsp_S));
    EXPECT_NE(param6, nullptr);
    memset_s(param6, sizeof(CM_ReadLocalFeatureRsp_S), 0x00, sizeof(CM_ReadLocalFeatureRsp_S));
    CM_ExecuteEventCbk(CM_SLE_CBK_EVENT_READ_LOCAL_FEATURE, param6);

    CM_EnableConnHighPowerRsp_S *param7 =
        (CM_EnableConnHighPowerRsp_S *)SDF_MemAlloc(sizeof(CM_EnableConnHighPowerRsp_S));
    EXPECT_NE(param7, nullptr);
    memset_s(param7, sizeof(CM_EnableConnHighPowerRsp_S), 0x00, sizeof(CM_EnableConnHighPowerRsp_S));
    CM_ExecuteEventCbk(CM_SLE_CBK_EVENT_ENABLE_CONN_HIGH_POWER, param7);

    CM_SetPeerDevType_S *param8 = (CM_SetPeerDevType_S *)SDF_MemAlloc(sizeof(CM_SetPeerDevType_S));
    EXPECT_NE(param8, nullptr);
    memset_s(param8, sizeof(CM_SetPeerDevType_S), 0x00, sizeof(CM_SetPeerDevType_S));
    CM_ExecuteEventCbk(CM_SLE_CBK_EVENT_SET_PEER_DEV_TYPE, param8);

    CM_SetRxDataFilterRsp_S *param9 = (CM_SetRxDataFilterRsp_S *)SDF_MemAlloc(sizeof(CM_SetRxDataFilterRsp_S));
    EXPECT_NE(param9, nullptr);
    memset_s(param9, sizeof(CM_SetRxDataFilterRsp_S), 0x00, sizeof(CM_SetRxDataFilterRsp_S));
    CM_ExecuteEventCbk(CM_SLE_CBK_EVENT_SET_RX_DATA_FILTER, param9);

    CM_ExecuteEventCbk(CM_SLE_CBK_EVENT_MAX, param9);

    SDF_MemFree(param1);
    SDF_MemFree(param2);
    SDF_MemFree(param3);
    SDF_MemFree(param4);
    SDF_MemFree(param5);
    SDF_MemFree(param6);
    SDF_MemFree(param7);
    SDF_MemFree(param8);
    SDF_MemFree(param9);
}