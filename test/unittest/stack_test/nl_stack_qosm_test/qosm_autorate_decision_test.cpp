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
#include "qosm_autorate_report.h"
#include "qosm_icg_mgr.h"
#include "qosm_errno.h"

class UT_QOSM_AUTORATE_DECISION_TEST : public testing::Test {
protected:
    void SetUp()
    {
        TestResetMock();
        QOSM_AutorateSetDspOn(true);
    }

    void TearDown()
    {
        TestParamNotifyCoexist(QOSM_MAX_BITRATE, QOS_DUTY_CYCLE_100P);
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

/*
 * @tc.desc: 质量上报降码率：ACK率低于阈值触发降档，DSP先收到降码率通知，
 *           全链路更新完成后码率切换到低档
 */
TEST_F(UT_QOSM_AUTORATE_DECISION_TEST, QualityReportDowngradeByLowAckRate)
{
    TestConnectIcg(g_qosId);
    EXPECT_EQ(g_bitrateCbkCnt, 0);

    TestQualityCbk(g_connHandle1, 10, -40, 0, 0);
    ASSERT_EQ(g_bitrateCbkCnt, 1);
    EXPECT_EQ(g_bitrateCbk[0].direction, LEVEL_DOWN);
    EXPECT_EQ(g_bitrateCbk[0].downwardBitrate, 96);
    EXPECT_EQ(g_bitrateCbk[0].qosLevel, QOS_LEVEL_1);
    EXPECT_EQ(g_bitrateCbk[0].labelId, 6);
    EXPECT_EQ(g_timerParam[QOSM_TEST_DOWNGRADE_LABEL_TIMER].args, (void *)(uintptr_t)g_qosId);

    TestUpdateParamCbk(g_qosId, g_lcid1, g_connHandle1, 6, CM_ICB_SUCCESS);
    EXPECT_EQ(g_bitrateCbkCnt, 1);

    TestUpdateParamCbk(g_qosId, g_lcid2, g_connHandle2, 6, CM_ICB_SUCCESS);
    ASSERT_EQ(g_bitrateCbkCnt, 2);
    EXPECT_EQ(g_bitrateCbk[1].direction, LEVEL_NONE);
    EXPECT_EQ(g_bitrateCbk[1].downwardBitrate, 96);
}

/*
 * @tc.desc: 目标降档不可用时会继续沿档位链下探，全部不可用则本轮不降档
 */
TEST_F(UT_QOSM_AUTORATE_DECISION_TEST, QualityReportDowngradeWhenTargetUnavailable)
{
    TestConnectIcg(g_qosId);
    const uint16_t highOnly[2] = { 160, 320 };
    TestParamNotifyEarphone(highOnly, 2);

    TestQualityCbk(g_connHandle1, 10, -40, 0, 0);
    EXPECT_EQ(g_bitrateCbkCnt, 0);
    EXPECT_EQ(g_timerParam[QOSM_TEST_DOWNGRADE_LABEL_TIMER].callback, nullptr);
}

/*
 * @tc.desc: 质量上报升码率：单链路连续8次ACK优良且全链路质量优秀后升档，
 *           芯片更新完成后延迟窗口到期，DSP收到升码率通知
 */
TEST_F(UT_QOSM_AUTORATE_DECISION_TEST, QualityReportUpgradeAfterExcellentReports)
{
    TestConnectIcg(g_qosId);
    for (int i = 0; i < 8; i++) {
        TestQualityCbk(g_connHandle1, 90, -40, 0, 0);
        TestQualityCbk(g_connHandle2, 90, -40, 0, 0);
    }
    TestQualityCbk(g_connHandle1, 90, -40, 0, 0);
    EXPECT_EQ(g_updateParam.labelId, 3);
    EXPECT_EQ(g_updateParam.icbCnt, 2);
    EXPECT_EQ(g_updateChannel[0], g_connHandle1);
    EXPECT_EQ(g_updateChannel[1], g_connHandle2);
    EXPECT_EQ(g_timerParam[QOSM_TEST_UPGRADE_LABEL_TIMER].args, (void *)(uintptr_t)g_qosId);

    TestUpdateParamCbk(g_qosId, g_lcid1, g_connHandle1, 3, CM_ICB_SUCCESS);
    TestUpdateParamCbk(g_qosId, g_lcid2, g_connHandle2, 3, CM_ICB_SUCCESS);
    EXPECT_NE(g_timerParam[QOSM_TEST_UPGRADE_LABEL_TIMER].callback, nullptr);

    TestFireTimer(QOSM_TEST_UPGRADE_LABEL_TIMER);
    ASSERT_EQ(g_bitrateCbkCnt, 2);
    EXPECT_EQ(g_bitrateCbk[0].direction, LEVEL_UP);
    EXPECT_EQ(g_bitrateCbk[0].downwardBitrate, 320);
    EXPECT_EQ(g_bitrateCbk[0].qosLevel, QOS_LEVEL_4);
    EXPECT_EQ(g_bitrateCbk[1].direction, LEVEL_NONE);
    EXPECT_EQ(g_bitrateCbk[1].downwardBitrate, 320);
}

/*
 * @tc.desc: 上码率等待窗口内质量恶化：立即终止升码率并转为降码率，切回当前档位
 */
TEST_F(UT_QOSM_AUTORATE_DECISION_TEST, LevelUpWindowTerminatedByLevelDown)
{
    TestConnectIcg(g_qosId);
    for (int i = 0; i < 8; i++) {
        TestQualityCbk(g_connHandle1, 90, -40, 0, 0);
        TestQualityCbk(g_connHandle2, 90, -40, 0, 0);
    }
    TestQualityCbk(g_connHandle1, 90, -40, 0, 0);
    TestUpdateParamCbk(g_qosId, g_lcid1, g_connHandle1, 3, CM_ICB_SUCCESS);
    TestUpdateParamCbk(g_qosId, g_lcid2, g_connHandle2, 3, CM_ICB_SUCCESS);
    EXPECT_NE(g_timerParam[QOSM_TEST_UPGRADE_LABEL_TIMER].callback, nullptr);

    TestQualityCbk(g_connHandle1, 10, -40, 0, 0);
    EXPECT_EQ(g_updateParam.labelId, 4);
    EXPECT_EQ(g_updateParam.icbCnt, 2);
    EXPECT_EQ(g_timerParam[QOSM_TEST_UPGRADE_LABEL_TIMER].callback, nullptr);
    EXPECT_EQ(g_timerParam[QOSM_TEST_DOWNGRADE_LABEL_TIMER].args, (void *)(uintptr_t)g_qosId);
}

/*
 * @tc.desc: 出现txFlushed时ACK率按1处理，仍走降码率决策
 */
TEST_F(UT_QOSM_AUTORATE_DECISION_TEST, QualityReportTxFlushedForcesDowngrade)
{
    TestConnectIcg(g_qosId);
    TestQualityCbk(g_connHandle1, 0, -40, 5, 0);
    ASSERT_EQ(g_bitrateCbkCnt, 1);
    EXPECT_EQ(g_bitrateCbk[0].direction, LEVEL_DOWN);
}

/*
 * @tc.desc: 质量上报过滤：未知链路、DSP关闭、txFlushed与ACK率同时为0时均不做决策
 */
TEST_F(UT_QOSM_AUTORATE_DECISION_TEST, QualityReportIgnoredByGuards)
{
    TestConnectIcg(g_qosId);
    TestQualityCbk(0xB9, 10, -40, 0, 0);
    QOSM_AutorateSetDspOn(false);
    TestQualityCbk(g_connHandle1, 10, -40, 0, 0);
    QOSM_AutorateSetDspOn(true);
    TestQualityCbk(g_connHandle1, 0, -40, 0, 0);
    EXPECT_EQ(g_bitrateCbkCnt, 0);
}

/*
 * @tc.desc: DSP状态与流控回调：DSP重新上线会补一次码率约束通知；
 *           流控只在码率不低于门限且链路存在时按ACK率1触发质量决策
 */
TEST_F(UT_QOSM_AUTORATE_DECISION_TEST, DspStatusAndFlowCtrlCallbacks)
{
    TestConnectIcg(g_qosId);
    QOSM_DspStatusCbk(false);
    TestQualityCbk(g_connHandle1, 10, -40, 0, 0);
    EXPECT_EQ(g_bitrateCbkCnt, 0);

    QOSM_DspStatusCbk(true);
    ASSERT_EQ(g_bitrateCbkCnt, 1);
    EXPECT_EQ(g_bitrateCbk[0].direction, LEVEL_NONE);
    EXPECT_EQ(g_bitrateCbk[0].downwardBitrate, 160);

    QOSM_DspFlowCtrlCbk(g_connHandle1, false);
    QOSM_DspFlowCtrlCbk(0xB9, true);
    EXPECT_EQ(g_bitrateCbkCnt, 1);

    QOSM_DspFlowCtrlCbk(g_connHandle1, true);
    ASSERT_EQ(g_bitrateCbkCnt, 2);
    EXPECT_EQ(g_bitrateCbk[1].direction, LEVEL_DOWN);
}

/*
 * @tc.desc: 低码率档位下收到流控不再触发降码率决策
 */
TEST_F(UT_QOSM_AUTORATE_DECISION_TEST, FlowCtrlIgnoredWhenBitrateBelowThreshold)
{
    TestConnectIcgAtLevel(g_qosId, QOS_LEVEL_1);

    QOSM_DspFlowCtrlCbk(g_connHandle1, true);
    EXPECT_EQ(g_bitrateCbkCnt, 0);
}

/*
 * @tc.desc: 耳机反馈只留低码率可用时，立即触发降码率通知并等待更新完成
 */
TEST_F(UT_QOSM_AUTORATE_DECISION_TEST, EarphoneNotifyConstrainsBitrate)
{
    TestConnectIcg(g_qosId);
    const uint16_t lowOnly[1] = { 96 };
    TestParamNotifyEarphone(lowOnly, 1);
    ASSERT_EQ(g_bitrateCbkCnt, 1);
    EXPECT_EQ(g_bitrateCbk[0].direction, LEVEL_DOWN);
    EXPECT_EQ(g_bitrateCbk[0].downwardBitrate, 96);
    EXPECT_EQ(g_bitrateCbk[0].labelId, 6);

    TestUpdateParamCbk(g_qosId, g_lcid1, g_connHandle1, 6, CM_ICB_SUCCESS);
    TestUpdateParamCbk(g_qosId, g_lcid2, g_connHandle2, 6, CM_ICB_SUCCESS);
    ASSERT_EQ(g_bitrateCbkCnt, 2);
    EXPECT_EQ(g_bitrateCbk[1].direction, LEVEL_NONE);
    EXPECT_EQ(g_bitrateCbk[1].downwardBitrate, 96);
}

/*
 * @tc.desc: 耳机反馈包含当前不支持档位的码率时整条通知被拒绝
 */
TEST_F(UT_QOSM_AUTORATE_DECISION_TEST, EarphoneNotifyRejectedByUnsupportedBitrate)
{
    TestConnectIcg(g_qosId);
    const uint16_t unsupported[1] = { 48 };
    TestParamNotifyEarphone(unsupported, 1);
    EXPECT_EQ(g_bitrateCbkCnt, 0);
}

/*
 * @tc.desc: 共存反馈限制最大码率后触发降码率，限制解除后恢复码率约束通知
 */
TEST_F(UT_QOSM_AUTORATE_DECISION_TEST, CoexistNotifyLimitsBitrate)
{
    TestConnectIcg(g_qosId);
    TestParamNotifyCoexist(200, QOS_DUTY_CYCLE_100P);
    ASSERT_EQ(g_bitrateCbkCnt, 1);
    EXPECT_EQ(g_bitrateCbk[0].direction, LEVEL_DOWN);
    EXPECT_EQ(g_bitrateCbk[0].downwardBitrate, 96);

    TestUpdateParamCbk(g_qosId, g_lcid1, g_connHandle1, 6, CM_ICB_SUCCESS);
    TestUpdateParamCbk(g_qosId, g_lcid2, g_connHandle2, 6, CM_ICB_SUCCESS);

    TestParamNotifyCoexist(QOSM_MAX_BITRATE, QOS_DUTY_CYCLE_100P);
    ASSERT_EQ(g_bitrateCbkCnt, 3);
    EXPECT_EQ(g_bitrateCbk[2].direction, LEVEL_NONE);
    EXPECT_EQ(g_bitrateCbk[2].downwardBitrate, 96);
}

/*
 * @tc.desc: 5G频段通知会重算档位可用性并对已建链ICG补一次约束检查
 */
TEST_F(UT_QOSM_AUTORATE_DECISION_TEST, Notify5GKeepsCurrentBitrate)
{
    TestConnectIcg(g_qosId);
    TestParamNotify5G();
    ASSERT_EQ(g_bitrateCbkCnt, 1);
    EXPECT_EQ(g_bitrateCbk[0].direction, LEVEL_NONE);
    EXPECT_EQ(g_bitrateCbk[0].downwardBitrate, 160);
}

/*
 * @tc.desc: 更新失败事件：无更新在途时忽略；降码率在途时重置状态并回滚到当前档位
 */
TEST_F(UT_QOSM_AUTORATE_DECISION_TEST, UpdateIcgStatusEventFailPaths)
{
    TestConnectIcg(g_qosId);
    TestUpdateParamCbk(g_qosId, g_lcid1, g_connHandle1, 0, CM_ICB_STATUS_EVENT_FAILED);
    EXPECT_EQ(g_bitrateCbkCnt, 0);
    EXPECT_EQ(g_updateParam.labelId, 0);

    TestQualityCbk(g_connHandle1, 10, -40, 0, 0);
    ASSERT_EQ(g_bitrateCbkCnt, 1);
    EXPECT_EQ(g_bitrateCbk[0].direction, LEVEL_DOWN);

    TestUpdateParamCbk(g_qosId, g_lcid1, g_connHandle1, 6, CM_ICB_STATUS_EVENT_FAILED);
    EXPECT_EQ(g_updateParam.labelId, 4);
    EXPECT_EQ(g_updateParam.icbCnt, 2);
    EXPECT_EQ(g_timerParam[QOSM_TEST_UPGRADE_LABEL_TIMER].args, (void *)(uintptr_t)g_qosId);
}

/*
 * @tc.desc: 降码率切档超时：按上报来源重置状态并回滚到当前档位
 */
TEST_F(UT_QOSM_AUTORATE_DECISION_TEST, DowngradeTimeoutRollsBackToCurrentLevel)
{
    TestConnectIcg(g_qosId);
    TestQualityCbk(g_connHandle1, 10, -40, 0, 0);
    ASSERT_EQ(g_bitrateCbkCnt, 1);

    TestFireTimer(QOSM_TEST_DOWNGRADE_LABEL_TIMER);
    EXPECT_EQ(g_updateParam.labelId, 4);
    EXPECT_EQ(g_updateParam.icbCnt, 2);
    EXPECT_EQ(g_timerParam[QOSM_TEST_DOWNGRADE_LABEL_TIMER].callback, nullptr);
}

/*
 * @tc.desc: 通话场景：降码率先通知对端与DSP，更新完成后上报切档成功应答
 */
TEST_F(UT_QOSM_AUTORATE_DECISION_TEST, CallFrame4DowngradeMsgFlow)
{
    TestSetCallParam(g_qosId);
    TestAddParamCbk(g_qosId, CM_ICB_STATE_IMG_CREATED, CM_ICB_SUCCESS, 2);
    QOSM_AutorateSetDspOn(true);
    QOSM_AutoRateRecvMsgParam ability = {};
    ability.qosId = g_qosId;
    ability.qosIndex = QOSM_QOSINDEX_CALL;
    ability.msgType = NTF_VOICE_CALL_AUTORATE_ABILITY;
    QOSM_RecvAutoRateMsg(&ability);
    TestAddConnection(g_qosId);
    TestLabelReportCbk(g_qosId, g_lcid1, g_connHandle1, true, true);
    TestLabelReportCbk(g_qosId, g_lcid2, g_connHandle2, true, true);
    TestEstablishedCbk(g_qosId, g_lcid1, g_connHandle1, true);
    TestEstablishedCbk(g_qosId, g_lcid2, g_connHandle2, true);
    g_bitrateCbkCnt = 0;

    TestQualityCbkEx(g_connHandle1, 90, -85, 0, 0, CM_IMB, 20);
    ASSERT_EQ(g_sendMsgCbkCnt, 1);
    EXPECT_EQ(g_sendMsgCbk[0].msgType, CHANGE_PEER_BITRATE_REQ);
    EXPECT_EQ(g_sendMsgCbk[0].direction, LEVEL_DOWN);
    EXPECT_EQ(g_sendMsgCbk[0].downwardBitrate, 32);
    ASSERT_EQ(g_bitrateCbkCnt, 1);
    EXPECT_EQ(g_bitrateCbk[0].direction, LEVEL_DOWN);

    QOSM_AutoRateRecvMsgParam peerRsp = {};
    peerRsp.qosId = g_qosId;
    peerRsp.qosIndex = QOSM_QOSINDEX_CALL;
    peerRsp.labelId = 1;
    peerRsp.msgType = CHANGE_PEER_BITRATE_RSP;
    peerRsp.result = QOSM_SUCCESS;
    QOSM_RecvAutoRateMsg(&peerRsp);

    TestUpdateParamCbk(g_qosId, g_lcid1, g_connHandle1, 1, CM_ICB_SUCCESS);
    TestUpdateParamCbk(g_qosId, g_lcid2, g_connHandle2, 1, CM_ICB_SUCCESS);
    ASSERT_EQ(g_sendMsgCbkCnt, 2);
    EXPECT_EQ(g_sendMsgCbk[1].msgType, CHANGE_LABEL_ID_RSP);
    EXPECT_EQ(g_sendMsgCbk[1].direction, LEVEL_DOWN);
    EXPECT_EQ(g_sendMsgCbk[1].result, QOSM_SUCCESS);
}

/*
 * @tc.desc: 对外入口的参数防御：空指针、数量越界、取值非法均被拒绝且不触发内部流程
 */
TEST_F(UT_QOSM_AUTORATE_DECISION_TEST, EntryParamGuards)
{
    EXPECT_EQ(QOSM_AutoRateSetParam(nullptr), QOSM_NULL_PTR_ERR);
    QOSM_AutoRateParam param = {};
    EXPECT_EQ(QOSM_AutoRateSetParam(&param), QOSM_INVALID_COUNT_ERR);
    param.linkCnt = 2;
    EXPECT_EQ(QOSM_AutoRateSetParam(&param), QOSM_INVALID_COUNT_ERR);
    param.supportedBitrateCnt = 6;
    param.qosIndex = QOSM_QOSINDEX_MAX;
    EXPECT_EQ(QOSM_AutoRateSetParam(&param), QOSM_NOT_FOUND_ERR);

    EXPECT_EQ(QOSM_AutoRateAddConnection(nullptr), QOSM_NULL_PTR_ERR);
    QOSM_AutoRateConnParam connParam = {};
    EXPECT_EQ(QOSM_AutoRateAddConnection(&connParam), QOSM_NULL_PTR_ERR);
    QOSM_ConnParam connLink = {};
    connParam.link = &connLink;
    connParam.linkCnt = 0;
    EXPECT_EQ(QOSM_AutoRateAddConnection(&connParam), QOSM_INVALID_COUNT_ERR);
    EXPECT_EQ(QOSM_AutoRateDeleteConnection(nullptr), QOSM_NULL_PTR_ERR);

    EXPECT_EQ(QOSM_AutoRateAddDataPath(nullptr), QOSM_NULL_PTR_ERR);
    QOSM_AutoRateDataPath dataPath = {};
    dataPath.codecConfigLen = 4;
    dataPath.codecConfigData = nullptr;
    EXPECT_EQ(QOSM_AutoRateAddDataPath(&dataPath), QOSM_NULL_PTR_ERR);
    EXPECT_EQ(QOSM_AutoRateDeleteDataPath(nullptr), QOSM_NULL_PTR_ERR);

    EXPECT_EQ(QOSM_RecvAutoRateMsg(nullptr), QOSM_NULL_PTR_ERR);
    QOSM_AutoRateRecvMsgParam msg = {};
    msg.qosIndex = QOSM_QOSINDEX_AUDIO;
    EXPECT_EQ(QOSM_RecvAutoRateMsg(&msg), QOSM_INVALID_PARAM_ERR);

    EXPECT_EQ(QOSM_AutoRateSetEarphoneFeedback(nullptr), QOSM_NULL_PTR_ERR);
    QOSM_AutoRateEarphoneFeedbackParam earphone = {};
    EXPECT_EQ(QOSM_AutoRateSetEarphoneFeedback(&earphone), QOSM_INVALID_PARAM_ERR);

    EXPECT_EQ(QOSM_AutoRateSetCoexistSuggestion(nullptr), QOSM_NULL_PTR_ERR);
    QOSM_AutoRateCoexistSuggestionParam coexist = {};
    coexist.maxBitrate = QOSM_MIN_BITRATE - 1;
    EXPECT_EQ(QOSM_AutoRateSetCoexistSuggestion(&coexist), QOSM_INVALID_PARAM_ERR);
    coexist.maxBitrate = QOSM_MAX_BITRATE + 1;
    EXPECT_EQ(QOSM_AutoRateSetCoexistSuggestion(&coexist), QOSM_INVALID_PARAM_ERR);
    coexist.maxBitrate = 320;
    coexist.dutyCycle = QOS_DUTY_CYCLE_ANY;
    EXPECT_EQ(QOSM_AutoRateSetCoexistSuggestion(&coexist), QOSM_INVALID_PARAM_ERR);

    QOSM_ICBParam icbParam = {};
    EXPECT_EQ(QOSM_AutoRateGetICGG2TParam(QOSM_QOSINDEX_AUDIO, nullptr), QOSM_NULL_PTR_ERR);
    EXPECT_EQ(QOSM_AutoRateGetICGT2GParam(QOSM_QOSINDEX_AUDIO, nullptr), QOSM_NULL_PTR_ERR);
    EXPECT_EQ(QOSM_AutoRateGetICGG2TParam(QOSM_QOSINDEX_MAX, &icbParam) == QOSM_SUCCESS, false);
}

/*
 * @tc.desc: DSP侧码率变化决策入口的防御：链路不存在、QOS索引不匹配、
 *           参数更新在途时拒绝请求
 */
TEST_F(UT_QOSM_AUTORATE_DECISION_TEST, ExecuteBitrateChangeDecisionGuards)
{
    EXPECT_EQ(QOSM_ExecuteBitrateChangeDecision(0xB9, QOSM_QOSINDEX_AUDIO, LEVEL_DOWN, QOS_LEVEL_1), false);

    TestConnectIcg(g_qosId);
    EXPECT_EQ(QOSM_ExecuteBitrateChangeDecision(g_connHandle1, QOSM_QOSINDEX_LOW_LATENCY, LEVEL_DOWN,
        QOS_LEVEL_1), false);

    TestQualityCbk(g_connHandle1, 10, -40, 0, 0);
    EXPECT_EQ(QOSM_ExecuteBitrateChangeDecision(g_connHandle1, QOSM_QOSINDEX_AUDIO, LEVEL_DOWN,
        QOS_LEVEL_1), false);
}
