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

#include <gtest/gtest.h>

#include "nl_stack_qosm_test_mock.h"
#include "qosm_autorate_common.h"
#include "qosm_errno.h"
#include "qosm_icg_mgr.h"

class UT_QOSM_ICG_STATE_TEST : public testing::Test {
protected:
    void SetUp()
    {
        TestResetMock();
        QOSM_AutorateSetDspOn(true);
    }

    void TearDown()
    {
        QOSM_AutorateSetDspOn(false);
        g_useDelayParamTimer = false;
        QOSM_ICGMgrDisable();
    }

    static void SetUpTestCase()
    {
        QOSM_ICGMgrEnable();
        TestRegisterCallback();
    }

    static void TearDownTestCase()
    {
        TestUnregisterCallback();
        QOSM_ICGMgrDisable();
    }
};

static void ConnectToLabelStage(uint8_t qosId)
{
    TestSetAudioAutorateParam(qosId, QOS_LEVEL_3);
    TestAddParamCbk(qosId, CM_ICB_STATE_IOG_CREATED, CM_ICB_SUCCESS, 2);
    TestAddConnection(qosId);
    TestLabelReportCbk(qosId, g_lcid1, g_connHandle1, true, false);
    TestLabelReportCbk(qosId, g_lcid2, g_connHandle2, true, false);
}

/*
 * @tc.desc: 音频ICG从设参到建链的完整链路：逐档下发自动调参、按链路数保存句柄、
 *           组内建链成功后回调调用方
 */
TEST_F(UT_QOSM_ICG_STATE_TEST, SetAutorateParamAndConnectFullFlow)
{
    TestSetAudioAutorateParam(g_qosId, QOS_LEVEL_3);
    EXPECT_EQ(g_settedAutorateParam.id, g_qosId);
    EXPECT_EQ(g_settedAutorateParam.type, CM_IOB);

    TestAddParamCbk(g_qosId, CM_ICB_STATE_IOG_CREATED, CM_ICB_SUCCESS, 2);
    EXPECT_EQ(g_paramCbk[0].qosId, g_qosId);
    EXPECT_EQ(g_paramCbk[0].state, QOSM_PARAM_SETTED);
    EXPECT_EQ(g_paramCbk[0].result, 0);

    TestAddConnection(g_qosId);
    EXPECT_EQ(g_labelParam.icbCnt, 2);
    EXPECT_EQ(g_labelIcb[0].connHandle, g_connHandle1);
    EXPECT_EQ(g_labelIcb[1].connHandle, g_connHandle2);
    EXPECT_EQ(g_createParam.channelCnt, 2);

    TestEstablishedCbk(g_qosId, g_lcid1, g_connHandle1, true);
    TestEstablishedCbk(g_qosId, g_lcid2, g_connHandle2, true);
    EXPECT_EQ(g_connCbk[0].qosId, g_qosId);
    EXPECT_EQ(g_connCbk[0].state, QOSM_CONNECTION_ADDED);
    EXPECT_EQ(g_connCbk[0].result, 0);
    EXPECT_EQ(g_connCbk[0].linkCnt, 2);
    EXPECT_EQ(g_connCbk[0].bitrate, 160);
    EXPECT_EQ(g_connCbkLink[0].connHandle, g_connHandle1);
    EXPECT_EQ(g_connCbkLink[1].connHandle, g_connHandle2);
}

/*
 * @tc.desc: 建档失败时设参整体失败并回调调用方，不残留ICG
 */
TEST_F(UT_QOSM_ICG_STATE_TEST, SetAutorateParamCreateFail)
{
    TestSetOtherParam(g_qosId, QOSM_QOSINDEX_MAX, 8, true);
    EXPECT_EQ(g_paramCbk[0].qosId, g_qosId);
    EXPECT_EQ(g_paramCbk[0].state, QOSM_PARAM_SETTED);
    EXPECT_NE(g_paramCbk[0].result, 0);

    TestAddConnection(g_qosId);
    EXPECT_EQ(g_connCbk[0].state, QOSM_CONNECTION_ADDED);
    EXPECT_NE(g_connCbk[0].result, 0);
}

/*
 * @tc.desc: 芯片设参回调失败或链路数不匹配时销毁ICG并回调失败
 */
TEST_F(UT_QOSM_ICG_STATE_TEST, ConfigIcgFailDestroysIcg)
{
    TestSetAudioAutorateParam(g_qosId, QOS_LEVEL_3);
    TestAddParamCbk(g_qosId, CM_ICB_STATE_IOG_CREATED, CM_ICB_FAILED, 2);
    EXPECT_NE(g_paramCbk[0].result, 0);

    TestSetAudioAutorateParam(g_qosId, QOS_LEVEL_3);
    TestAddParamCbk(g_qosId, CM_ICB_STATE_IOG_CREATED, CM_ICB_SUCCESS, 1);
    EXPECT_NE(g_paramCbk[0].result, 0);
}

/*
 * @tc.desc: 非自动调参媒体流设参按链路组下发固定参数
 */
TEST_F(UT_QOSM_ICG_STATE_TEST, SetParamMediaFlow)
{
    TestSetOtherParam(g_qosId, QOSM_QOSINDEX_OTHERS, 8, false);
    EXPECT_EQ(g_settedParam.id, g_qosId);
    EXPECT_EQ(g_settedParam.icbCnt, 2);
    EXPECT_GT(g_settedParam.paramCnt, 0);

    TestAddParamCbk(g_qosId, CM_ICB_STATE_IOG_CREATED, CM_ICB_SUCCESS, 2);
    EXPECT_EQ(g_paramCbk[0].qosId, g_qosId);
    EXPECT_EQ(g_paramCbk[0].result, 0);
}

/*
 * @tc.desc: 建链入口防御：未知QOS、无效句柄、删参在途均回调失败
 */
TEST_F(UT_QOSM_ICG_STATE_TEST, AddConnectionInvalidPaths)
{
    TestAddConnection(9);
    EXPECT_EQ(g_connCbk[0].qosId, 9);
    EXPECT_NE(g_connCbk[0].result, 0);

    TestSetAudioAutorateParam(g_qosId, QOS_LEVEL_3);
    TestAddParamCbk(g_qosId, CM_ICB_STATE_IOG_CREATED, CM_ICB_SUCCESS, 2);
    QOSM_AutoRateConnParam connParam = {};
    QOSM_ConnParam link = {};
    connParam.qosId = g_qosId;
    connParam.linkCnt = 1;
    link.connHandle = 0xB9;
    link.lcid = g_lcid1;
    connParam.link = &link;
    QOSM_ICGMgrAddConnection(&connParam);
    EXPECT_EQ(g_connCbk[0].qosId, g_qosId);
    EXPECT_NE(g_connCbk[0].result, 0);

    TestAddConnection(g_qosId);
    TestLabelReportCbk(g_qosId, g_lcid1, g_connHandle1, true, false);
    TestLabelReportCbk(g_qosId, g_lcid2, g_connHandle2, true, false);
    TestEstablishedCbk(g_qosId, g_lcid1, g_connHandle1, true);
    TestEstablishedCbk(g_qosId, g_lcid2, g_connHandle2, true);
    QOSM_ICGMgrRemoveParam((void *)(uintptr_t)g_qosId);
    TestAddConnection(g_qosId);
    EXPECT_NE(g_connCbk[0].result, 0);
    TestRemoveParamCbk(g_qosId, CM_ICB_STATE_IOG_REMOVED);
    EXPECT_EQ(g_paramCbk[1].state, QOSM_PARAM_REMOVED);
    EXPECT_EQ(g_paramCbk[1].result, 0);
}

/*
 * @tc.desc: 参数更新期间收到建链请求会被暂存，超时后重新执行并按已建链成功回调
 */
TEST_F(UT_QOSM_ICG_STATE_TEST, AddConnectionDelayedByUpdate)
{
    TestConnectIcg(g_qosId);
    TestQualityCbk(g_connHandle1, 10, -40, 0, 0);
    EXPECT_EQ(g_bitrateCbk[0].direction, LEVEL_DOWN);

    TestAddConnection(g_qosId);
    EXPECT_EQ(g_labelParam.icbCnt, 0);
    EXPECT_EQ(g_timerParam[QOSM_TEST_DELAY_CONN_TIMER].args, (void *)(uintptr_t)g_qosId);

    TestFireTimer(QOSM_TEST_DELAY_CONN_TIMER);
    EXPECT_EQ(g_connCbk[0].state, QOSM_CONNECTION_ADDED);
    EXPECT_EQ(g_connCbk[0].result, 0);
}

/*
 * @tc.desc: 暂存建链被删除时按成功回调并清理暂存任务
 */
TEST_F(UT_QOSM_ICG_STATE_TEST, DelayedConnectionDeleted)
{
    TestConnectIcg(g_qosId);
    TestQualityCbk(g_connHandle1, 10, -40, 0, 0);
    TestAddConnection(g_qosId);

    TestDeleteConnection(g_qosId, g_connHandle1);
    EXPECT_EQ(g_connCbk[1].state, QOSM_CONNECTION_DELETED);
    EXPECT_EQ(g_connCbk[1].result, 0);
    EXPECT_EQ(g_timerParam[QOSM_TEST_DELAY_CONN_TIMER].callback, nullptr);
}

/*
 * @tc.desc: 芯片设label失败时整组建链失败并复位链路状态
 */
TEST_F(UT_QOSM_ICG_STATE_TEST, SetLabelFailFailsWholeGroup)
{
    g_setLabelRet = CM_FAIL;
    TestSetAudioAutorateParam(g_qosId, QOS_LEVEL_3);
    TestAddParamCbk(g_qosId, CM_ICB_STATE_IOG_CREATED, CM_ICB_SUCCESS, 2);
    TestAddConnection(g_qosId);
    EXPECT_EQ(g_connCbk[0].state, QOSM_CONNECTION_ADDED);
    EXPECT_NE(g_connCbk[0].result, 0);
    EXPECT_EQ(g_connCbk[0].linkCnt, 2);
}

/*
 * @tc.desc: label上报失败进入建链失败处理：未知句柄直接失败回调，
 *           建链中失败先挂重试定时器，超时后重新下发建链
 */
TEST_F(UT_QOSM_ICG_STATE_TEST, LabelReportFailTriggersRetry)
{
    ConnectToLabelStage(g_qosId);
    TestLabelReportCbk(g_qosId, g_lcid1, 0xB9, false, false);
    EXPECT_EQ(g_connCbk[0].state, QOSM_CONNECTION_ADDED);
    EXPECT_NE(g_connCbk[0].result, 0);

    TestLabelReportCbk(g_qosId, g_lcid1, g_connHandle1, false, false);
    EXPECT_NE(g_timerParam[QOSM_TEST_RETRY_CONN_TIMER].callback, nullptr);

    TestFireTimer(QOSM_TEST_RETRY_CONN_TIMER);
    EXPECT_EQ(g_createParam.channelCnt, 1);
    EXPECT_EQ(g_createChannel[0].connHandle, g_connHandle1);
}

/*
 * @tc.desc: 组内两条链路各建链失败两次后整组失败，回调携带全部链路
 */
TEST_F(UT_QOSM_ICG_STATE_TEST, EstablishFailAllInGroup)
{
    ConnectToLabelStage(g_qosId);
    TestEstablishedCbk(g_qosId, g_lcid1, g_connHandle1, false);
    TestEstablishedCbk(g_qosId, g_lcid2, g_connHandle2, false);
    TestEstablishedCbk(g_qosId, g_lcid1, g_connHandle1, false);
    TestEstablishedCbk(g_qosId, g_lcid2, g_connHandle2, false);
    EXPECT_EQ(g_connCbk[0].state, QOSM_CONNECTION_ADDED);
    EXPECT_NE(g_connCbk[0].result, 0);
    EXPECT_EQ(g_connCbk[0].linkCnt, 2);
}

/*
 * @tc.desc: 断链事件按链路清理：逐条回调删除成功，全部断开后发起删参并回调完成
 */
TEST_F(UT_QOSM_ICG_STATE_TEST, DestroyConnectedLinksThenRemoveParam)
{
    TestConnectIcg(g_qosId);
    TestDisconnectCbk(g_qosId, g_lcid1, g_connHandle1, CM_LINK_ICB, CM_ICB_SUCCESS);
    EXPECT_EQ(g_connCbk[1].state, QOSM_CONNECTION_DELETED);
    EXPECT_EQ(g_connCbk[1].result, 0);
    EXPECT_EQ(g_connCbk[1].linkCnt, 1);
    EXPECT_EQ(g_dfxUpdateConnCnt, 1);

    TestDisconnectCbk(g_qosId, g_lcid2, g_connHandle2, CM_LINK_ICB, CM_ICB_SUCCESS);
    EXPECT_EQ(g_removedParam.id, g_qosId);
    TestRemoveParamCbk(g_qosId, CM_ICB_STATE_IOG_REMOVED);
    EXPECT_EQ(g_paramCbk[1].state, QOSM_PARAM_REMOVED);
    EXPECT_EQ(g_paramCbk[1].result, 0);
}

/*
 * @tc.desc: 参数更新在途时断开剩余链路会提前收敛更新流程
 */
TEST_F(UT_QOSM_ICG_STATE_TEST, DestroyDuringUpdateCompletesEarly)
{
    TestConnectIcg(g_qosId);
    TestQualityCbk(g_connHandle1, 10, -40, 0, 0);
    TestUpdateParamCbk(g_qosId, g_lcid1, g_connHandle1, 6, CM_ICB_SUCCESS);
    EXPECT_EQ(g_bitrateCbkCnt, 1);

    TestDisconnectCbk(g_qosId, g_lcid2, g_connHandle2, CM_LINK_ICB, CM_ICB_SUCCESS);
    ASSERT_EQ(g_bitrateCbkCnt, 2);
    EXPECT_EQ(g_bitrateCbk[1].direction, LEVEL_NONE);
    EXPECT_EQ(g_connCbk[1].state, QOSM_CONNECTION_DELETED);
    EXPECT_EQ(g_connCbk[1].result, 0);
}

/*
 * @tc.desc: 组建链中收到异步链路断链：该链路标记断开并回调失败，
 *           其余链路继续走完建链并成功回调
 */
TEST_F(UT_QOSM_ICG_STATE_TEST, AcbDisconnectDuringGroupConnect)
{
    ConnectToLabelStage(g_qosId);
    TestDisconnectCbk(g_qosId, g_lcid1, g_connHandle1, CM_LINK_ACB, CM_ICB_SUCCESS);
    EXPECT_EQ(g_connCbk[0].state, QOSM_CONNECTION_ADDED);
    EXPECT_NE(g_connCbk[0].result, 0);
    EXPECT_EQ(g_connCbk[0].linkCnt, 1);

    TestEstablishedCbk(g_qosId, g_lcid2, g_connHandle2, true);
    EXPECT_EQ(g_connCbk[0].result, 0);
    EXPECT_EQ(g_connCbk[0].linkCnt, 1);
    EXPECT_EQ(g_connCbkLink[0].connHandle, g_connHandle2);
}

/*
 * @tc.desc: 已建链ICG重新设参：先断开既有链路，删参完成后执行暂存的新参数
 */
TEST_F(UT_QOSM_ICG_STATE_TEST, SetParamOnExistingIcgDelaysUntilRemoved)
{
    TestConnectIcg(g_qosId);
    g_useDelayParamTimer = true;
    TestSetAudioAutorateParam(g_qosId, QOS_LEVEL_3);
    EXPECT_EQ(g_timerParam[QOSM_TEST_DELAY_PARAM_TIMER].args, (void *)(uintptr_t)g_qosId);
    EXPECT_EQ(g_disconnectParam.channelCnt, 1);
    EXPECT_EQ(g_disconnectChannel[0].connHandle, g_connHandle2);

    TestDisconnectCbk(g_qosId, g_lcid1, g_connHandle1, CM_LINK_ICB, CM_ICB_SUCCESS);
    TestDisconnectCbk(g_qosId, g_lcid2, g_connHandle2, CM_LINK_ICB, CM_ICB_SUCCESS);
    EXPECT_EQ(g_removedParam.id, g_qosId);

    TestFireTimer(QOSM_TEST_DELAY_PARAM_TIMER);
    TestRemoveParamCbk(g_qosId, CM_ICB_STATE_IOG_REMOVED);
    EXPECT_EQ(g_paramCbk[1].state, QOSM_PARAM_REMOVED);
    EXPECT_EQ(g_paramCbk[1].result, 0);
    EXPECT_EQ(g_settedAutorateParam.id, g_qosId);
}

/*
 * @tc.desc: 删链入口：芯片删除失败回调失败，成功则下发单链路断链命令
 */
TEST_F(UT_QOSM_ICG_STATE_TEST, DeleteConnectionPaths)
{
    TestConnectIcg(g_qosId);
    g_deleteConnectionRet = CM_FAIL;
    TestDeleteConnection(g_qosId, g_connHandle1);
    EXPECT_EQ(g_connCbk[1].state, QOSM_CONNECTION_DELETED);
    EXPECT_NE(g_connCbk[1].result, 0);

    g_deleteConnectionRet = CM_SUCCESS;
    TestDeleteConnection(g_qosId, g_connHandle1);
    EXPECT_EQ(g_disconnectParam.channelCnt, 1);
    EXPECT_EQ(g_disconnectChannel[0].connHandle, g_connHandle1);
}

/*
 * @tc.desc: 频段切换事件更新链路频段并按5G重算码率约束，同时通知DFX变更
 */
TEST_F(UT_QOSM_ICG_STATE_TEST, FreqBandSwitchUpdatesBandAndNotify)
{
    TestConnectIcg(g_qosId);
    CM_FreqBandSwitchParam bandParam = {};
    bandParam.connHandle = g_connHandle1;
    bandParam.oldFreqBand = QOS_BAND_2D4;
    bandParam.newFreqBand = QOS_BAND_5D8;
    g_freqBandListener(&bandParam);
    EXPECT_EQ(g_dfxBand, QOS_BAND_5D8);
    ASSERT_EQ(g_bitrateCbkCnt, 1);
    EXPECT_EQ(g_bitrateCbk[0].direction, LEVEL_NONE);

    bandParam.newFreqBand = QOS_BAND_2D4;
    g_freqBandListener(&bandParam);
    EXPECT_EQ(g_dfxBand, QOS_BAND_2D4);
    EXPECT_EQ(g_bitrateCbkCnt, 2);
}

/*
 * @tc.desc: 数据通路的建立与删除按结果回调；失败时携带失败原因
 */
TEST_F(UT_QOSM_ICG_STATE_TEST, DataPathSetupAndRemove)
{
    TestConnectIcg(g_qosId);
    TestDataPathCbk(g_qosId, CM_ICB_STATE_ICB_DATA_PATH_SETUP, 0);
    EXPECT_EQ(g_dataPathCbk[0].state, QOSM_DATAPATH_ADDED);
    EXPECT_EQ(g_dataPathCbk[0].result, 0);

    g_cmSetDataPathRet = CM_FAIL;
    QOSM_AutoRateDataPath dataPath = {};
    uint8_t codecConfig[2] = { 1, 2 };
    dataPath.qosId = g_qosId;
    dataPath.connHandle = g_connHandle1;
    dataPath.codecConfigLen = 2;
    dataPath.codecConfigData = codecConfig;
    QOSM_ICGMgrAddDataPath(&dataPath);
    EXPECT_NE(g_dataPathCbk[0].result, 0);
    EXPECT_EQ(g_dataPathCbk[0].connHandle, g_connHandle1);

    QOSM_AutoRateDeletedDataPath deletedPath = {};
    deletedPath.qosId = 9;
    deletedPath.connHandle = g_connHandle1;
    QOSM_ICGMgrDeleteDataPath(&deletedPath);
    EXPECT_EQ(g_dataPathCbk[1].state, QOSM_DATAPATH_DELETED);
    EXPECT_NE(g_dataPathCbk[1].result, 0);

    g_cmRemoveDataPathRet = CM_FAIL;
    deletedPath.qosId = g_qosId;
    QOSM_ICGMgrDeleteDataPath(&deletedPath);
    EXPECT_NE(g_dataPathCbk[1].result, 0);

    g_cmRemoveDataPathRet = CM_SUCCESS;
    QOSM_ICGMgrDeleteDataPath(&deletedPath);
    TestDataPathCbk(g_qosId, CM_ICB_STATE_ICB_DATA_PATH_REMOVED, 0);
    EXPECT_EQ(g_dataPathCbk[1].result, 0);
}
