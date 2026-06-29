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

#include <gtest/gtest.h>
#include "securec.h"

#include "qosm_table_mgr.h"
#include "cm_icb_def.h"
#include "dli_layer_config.h"
#include "parameter_wrapper.h"
#include "qosm_log.h"
#include "qosm_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

uint16_t DLI_DataNumGet(DLI_DataType type)
{
    return 0;
}

int32_t PropertyGetInt32(const char* key, int32_t default_value)
{
    return default_value;
}

#ifdef __cplusplus
}
#endif

class UT_QOSM_TABLE_MGR_TEST : public testing::Test {
protected:
    void SetUp()
    {
    }

    virtual void TearDown()
    {
    }

    static void SetUpTestCase()
    {
    }

    static void TearDownTestCase()
    {
    }
};

TEST_F(UT_QOSM_TABLE_MGR_TEST, TestCaseIsAutoratEnabled)
{
    bool ret = QOSM_IsAutorateEnabled();
    EXPECT_EQ(ret, true);
}

TEST_F(UT_QOSM_TABLE_MGR_TEST, TestCaseGetStartParamByIndex)
{
    QOSM_UpdateDutyCycle(QOS_DUTY_CYCLE_100P);
    QOSM_StartParam *param = QOSM_GetStartParamByIndex(QOSM_QOSINDEX_AUDIO, 160, 2);
    EXPECT_NE(param, nullptr);
    EXPECT_EQ(param->startLevel, QOS_LEVEL_3);
    EXPECT_EQ(param->levelCnt, 8);

    // pass illegal bitrate, will return default start param
    param = QOSM_GetStartParamByIndex(QOSM_QOSINDEX_AUDIO, 100, 2);
    EXPECT_NE(param, nullptr);
    EXPECT_EQ(param->startLevel, QOS_LEVEL_2);
    EXPECT_EQ(param->levelCnt, 8);

    // pass illegal qosIndex, will return nullptr
    param = QOSM_GetStartParamByIndex(QOSM_QOSINDEX_MAX, 100, 2);
    EXPECT_EQ(param, nullptr);
}

TEST_F(UT_QOSM_TABLE_MGR_TEST, TestCaseGetQosParamByIndex)
{
    // QOSM_QOSINDEX_AUDIO, 0
    // {
    //     QOS_DUTY_CYCLE_ANY /* dutyCycle */, QOS_LEVEL_7 /* qosLevel */, QOS_LEVEL_7 /* upQosLevel */,
    //     QOS_LEVEL_4 /* downQosLevel */, 4600 /* downwardBitrate */, 0 /* upwardBitrate */,
    //     10000 /* sduIntervalG2T */, 10000 /* sduIntervalT2G */, 0 /* sca */, 1 /* packing */, 0 /* framing */,
    //     300 /* maxLatencyG2T */, 20 /* maxLatencyT2G */, 80 /* icbInterval */, 15 /* ftG2T */, 1 /* ftT2G */,
    //     43 /* rtnG2T */, 1 /* rtnT2G */, QOS_MAKE_SDU_LEN(2876) /* maxSduG2T */, 0 /* maxSduT2G */,
    //     QOS_MAKE_SDU_LEN(2876) /* maxPduG2T */, 0 /* maxPduT2G */,
    //     3 /* nse */, 2 /* bnG2T */, 0 /* bnT2G */, 2 /* phyG2T */, 0 /* phyT2G */,
    //     16 /* mcsG2T */, 0 /* mcsT2G */, 2 /* pilotG2T */, 3 /* pilotT2G */, 1 /* frameG2T */, 0 /* frameT2G */
    // }
    QOSM_LinkParam *param = QOSM_GetQosParamByIndex(QOSM_QOSINDEX_AUDIO, 0);
    EXPECT_NE(param, nullptr);
    EXPECT_EQ(param->dutyCycle, QOS_DUTY_CYCLE_ANY);
    EXPECT_EQ(param->qosLevel, QOS_LEVEL_7);
    EXPECT_EQ(param->upQosLevel, QOS_LEVEL_7);
    EXPECT_EQ(param->downQosLevel, QOS_LEVEL_4);
    EXPECT_EQ(param->downwardBitrate, 4600);

    // pass illegal qosLevelIndex, will return nullptr
    param = QOSM_GetQosParamByIndex(QOSM_QOSINDEX_AUDIO, 8);
    EXPECT_EQ(param, nullptr);

    // pass illegal qosIndex, will return nullptr
    param = QOSM_GetQosParamByIndex(QOSM_QOSINDEX_MAX, 0);
    EXPECT_EQ(param, nullptr);
}

TEST_F(UT_QOSM_TABLE_MGR_TEST, TestCaseGetICBTypeByIndex)
{
    CM_ICBType type = QOSM_GetICBTypeByIndex(QOSM_QOSINDEX_CALL);
    EXPECT_EQ(type, CM_IMB);

    type = QOSM_GetICBTypeByIndex(QOSM_QOSINDEX_AUDIO);
    EXPECT_EQ(type, CM_IOB);
}

TEST_F(UT_QOSM_TABLE_MGR_TEST, TestCaseGetICBG2TParamByIndex)
{
    // QOSM_QOSINDEX_AUDIO, 0
    // {
    //     QOS_DUTY_CYCLE_ANY /* dutyCycle */, QOS_LEVEL_7 /* qosLevel */, QOS_LEVEL_7 /* upQosLevel */,
    //     QOS_LEVEL_4 /* downQosLevel */, 4600 /* downwardBitrate */, 0 /* upwardBitrate */,
    //     10000 /* sduIntervalG2T */, 10000 /* sduIntervalT2G */, 0 /* sca */, 1 /* packing */, 0 /* framing */,
    //     300 /* maxLatencyG2T */, 20 /* maxLatencyT2G */, 80 /* icbInterval */, 15 /* ftG2T */, 1 /* ftT2G */,
    //     43 /* rtnG2T */, 1 /* rtnT2G */, QOS_MAKE_SDU_LEN(2876) /* maxSduG2T */, 0 /* maxSduT2G */,
    //     QOS_MAKE_SDU_LEN(2876) /* maxPduG2T */, 0 /* maxPduT2G */,
    //     3 /* nse */, 2 /* bnG2T */, 0 /* bnT2G */, 2 /* phyG2T */, 0 /* phyT2G */,
    //     16 /* mcsG2T */, 0 /* mcsT2G */, 2 /* pilotG2T */, 3 /* pilotT2G */, 1 /* frameG2T */, 0 /* frameT2G */
    // }
    // param->sduInterval = g_qosIndexTable[i].qos[0].sduIntervalG2T;
    // param->sca = g_qosIndexTable[i].qos[0].sca;
    // param->packing = g_qosIndexTable[i].qos[0].packing;
    // param->framing = g_qosIndexTable[i].qos[0].framing;
    // param->maxLatency = g_qosIndexTable[i].qos[0].maxLatencyG2T;
    // param->icbInterval = g_qosIndexTable[i].qos[0].icbInterval;
    // param->ft = g_qosIndexTable[i].qos[0].ftG2T;
    // param->rtn = g_qosIndexTable[i].qos[0].rtnG2T;
    // for (uint32_t j = 0; j < g_qosIndexTable[i].qosCnt; j++) {
    //     param->maxSdu = QOSM_MAX(param->maxSdu, g_qosIndexTable[i].qos[j].maxSduG2T);
    //     param->maxPdu = QOSM_MAX(param->maxPdu, g_qosIndexTable[i].qos[j].maxPduG2T);
    // }
    // param->nse = g_qosIndexTable[i].qos[0].nse;
    // param->bn = g_qosIndexTable[i].qos[0].bnG2T;
    // param->phy = g_qosIndexTable[i].qos[0].phyG2T;
    // param->mcs = g_qosIndexTable[i].qos[0].mcsG2T;
    // param->pilot = g_qosIndexTable[i].qos[0].pilotG2T;
    // param->frame = g_qosIndexTable[i].qos[0].frameG2T;
    // param->icbNum = DLI_DataNumGet(ICB_DATA_TYPE);
    QOSM_ICBParam param = {};
    uint32_t ret = QOSM_GetICBG2TParamByIndex(QOSM_QOSINDEX_AUDIO, &param);
    EXPECT_EQ(ret, QOSM_SUCCESS);
    EXPECT_EQ(param.sduInterval, 10000);
    EXPECT_EQ(param.sca, 0);
    EXPECT_EQ(param.packing, 1);
    EXPECT_EQ(param.framing, 0);
    EXPECT_EQ(param.maxLatency, 300);
    EXPECT_EQ(param.icbInterval, 80);
    EXPECT_EQ(param.ft, 15);
    EXPECT_EQ(param.rtn, 43);
    EXPECT_EQ(param.nse, 3);
    EXPECT_EQ(param.bn, 2);
    EXPECT_EQ(param.phy, 2);
    EXPECT_EQ(param.mcs, 16);
    EXPECT_EQ(param.pilot, 2);
    EXPECT_EQ(param.frame, 1);
    
    // illegal qos index
    ret = QOSM_GetICBG2TParamByIndex(QOSM_QOSINDEX_MAX, &param);
    EXPECT_NE(ret, QOSM_SUCCESS);
}

TEST_F(UT_QOSM_TABLE_MGR_TEST, TestCaseGetICBT2GParamByIndex)
{
    // QOSM_QOSINDEX_AUDIO, 0
    // {
    //     QOS_DUTY_CYCLE_ANY /* dutyCycle */, QOS_LEVEL_7 /* qosLevel */, QOS_LEVEL_7 /* upQosLevel */,
    //     QOS_LEVEL_4 /* downQosLevel */, 4600 /* downwardBitrate */, 0 /* upwardBitrate */,
    //     10000 /* sduIntervalG2T */, 10000 /* sduIntervalT2G */, 0 /* sca */, 1 /* packing */, 0 /* framing */,
    //     300 /* maxLatencyG2T */, 20 /* maxLatencyT2G */, 80 /* icbInterval */, 15 /* ftG2T */, 1 /* ftT2G */,
    //     43 /* rtnG2T */, 1 /* rtnT2G */, QOS_MAKE_SDU_LEN(2876) /* maxSduG2T */, 0 /* maxSduT2G */,
    //     QOS_MAKE_SDU_LEN(2876) /* maxPduG2T */, 0 /* maxPduT2G */,
    //     3 /* nse */, 2 /* bnG2T */, 0 /* bnT2G */, 2 /* phyG2T */, 0 /* phyT2G */,
    //     16 /* mcsG2T */, 0 /* mcsT2G */, 2 /* pilotG2T */, 3 /* pilotT2G */, 1 /* frameG2T */, 0 /* frameT2G */
    // }
    // param->sduInterval = g_qosIndexTable[i].qos[0].sduIntervalT2G;
    // param->sca = g_qosIndexTable[i].qos[0].sca;
    // param->packing = g_qosIndexTable[i].qos[0].packing;
    // param->framing = g_qosIndexTable[i].qos[0].framing;
    // param->maxLatency = g_qosIndexTable[i].qos[0].maxLatencyT2G;
    // param->icbInterval = g_qosIndexTable[i].qos[0].icbInterval;
    // param->ft = g_qosIndexTable[i].qos[0].ftT2G;
    // param->rtn = g_qosIndexTable[i].qos[0].rtnT2G;
    // for (uint32_t j = 0; j < g_qosIndexTable[i].qosCnt; j++) {
    //     param->maxSdu = QOSM_MAX(param->maxSdu, g_qosIndexTable[i].qos[j].maxSduT2G);
    //     param->maxPdu = QOSM_MAX(param->maxPdu, g_qosIndexTable[i].qos[j].maxPduT2G);
    // }
    // param->nse = g_qosIndexTable[i].qos[0].nse;
    // param->bn = g_qosIndexTable[i].qos[0].bnT2G;
    // param->phy = g_qosIndexTable[i].qos[0].phyT2G;
    // param->mcs = g_qosIndexTable[i].qos[0].mcsT2G;
    // param->pilot = g_qosIndexTable[i].qos[0].pilotT2G;
    // param->frame = g_qosIndexTable[i].qos[0].frameT2G;
    // param->icbNum = DLI_DataNumGet(ICB_DATA_TYPE);
    QOSM_ICBParam param = {};
    uint32_t ret = QOSM_GetICBT2GParamByIndex(QOSM_QOSINDEX_AUDIO, &param);
    EXPECT_EQ(ret, QOSM_SUCCESS);
    EXPECT_EQ(param.sduInterval, 10000);
    EXPECT_EQ(param.sca, 0);
    EXPECT_EQ(param.packing, 1);
    EXPECT_EQ(param.framing, 0);
    EXPECT_EQ(param.maxLatency, 20);
    EXPECT_EQ(param.icbInterval, 80);
    EXPECT_EQ(param.ft, 1);
    EXPECT_EQ(param.rtn, 1);
    EXPECT_EQ(param.nse, 3);
    EXPECT_EQ(param.bn, 0);
    EXPECT_EQ(param.phy, 0);
    EXPECT_EQ(param.mcs, 0);
    EXPECT_EQ(param.pilot, 3);
    EXPECT_EQ(param.frame, 0);
    
    // illegal qos index
    ret = QOSM_GetICBT2GParamByIndex(QOSM_QOSINDEX_MAX, &param);
    EXPECT_NE(ret, QOSM_SUCCESS);
}

TEST_F(UT_QOSM_TABLE_MGR_TEST, TestCaseGetAutorateThreshold)
{
    /* bitrate, up ackrate, down ackrate, up diff max, up rssi */
    // { 320, 85, 60, 255, -128 },
    QOSM_UpdateDutyCycle(QOS_DUTY_CYCLE_100P);
    const struct QOSM_AutoRateThreshold *threshold = QOSM_GetAutorateThreshold(QOSM_QOSINDEX_AUDIO, 320);
    EXPECT_NE(threshold, nullptr);
    EXPECT_EQ(threshold->bitrate, 320);
    EXPECT_EQ(threshold->upAckRate, 85);
    EXPECT_EQ(threshold->downAckRate, 60);
    EXPECT_EQ(threshold->upDiffMax, 255);
    EXPECT_EQ(threshold->upRssi, -128);
    
    /* bitrate, up ackrate, down ackrate, up diff max, up rssi */
    // { 320, 85, 60, 255, -128 },
    QOSM_UpdateDutyCycle(QOS_DUTY_CYCLE_50P);
    threshold = QOSM_GetAutorateThreshold(QOSM_QOSINDEX_AUDIO, 320);
    EXPECT_NE(threshold, nullptr);
    EXPECT_EQ(threshold->bitrate, 320);
    EXPECT_EQ(threshold->upAckRate, 85);
    EXPECT_EQ(threshold->downAckRate, 60);
    EXPECT_EQ(threshold->upDiffMax, 255);
    EXPECT_EQ(threshold->upRssi, -128);

    // illegal qos index
    QOSM_UpdateDutyCycle(QOS_DUTY_CYCLE_100P);
    threshold = QOSM_GetAutorateThreshold(QOSM_QOSINDEX_MAX, 320);
    EXPECT_EQ(threshold, nullptr);
    threshold = QOSM_GetAutorateThreshold(0, 320);
    EXPECT_EQ(threshold, nullptr);

    // illegal bitrate
    threshold = QOSM_GetAutorateThreshold(QOSM_QOSINDEX_AUDIO, 100);
    EXPECT_EQ(threshold, nullptr);
}
