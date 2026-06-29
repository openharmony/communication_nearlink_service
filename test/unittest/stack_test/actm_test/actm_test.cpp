/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "sdf_addr.h"
#include "sdf_mem.h"

#include "stack_schedule_mock.h"
#include "stack_schedule_stub.h"
#include "stack_cm_mock.h"
#include "stack_cm_stub.h"
#include "stack_dtap_mock.h"
#include "stack_dtap_stub.h"
#include "stack_qosm_mock.h"
#include "stack_qosm_stub.h"
#include "stack_dli_mock.h"
#include "stack_dli_stub.h"

#include "cm_def.h"
#include "dtap_tcid.h"
#include "ssap_link.h"
#include "ssap_manager.h"
#include "ssaps_service.h"
#include "ssapc_cache.h"
#include "dli_event_struct.h"
#include "nlstk_sm.h"

#include "actm_api.h"
#include "actm.h"
#include "cdsm.h"
#include "cpfwk_log.h"

#define QOSM_DOWN 0
#define QOSM_UP   1

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

static SLE_Addr_S g_addr1 = {.type = PUBLIC_ADDRESS, .addr = {0x01, 0x01, 0x02, 0x02, 0x03, 0x03}};
static bool g_profileReadState = false;
static uint8_t g_event = 0xFF;
static uint8_t g_result = 0xFF;
static uint8_t g_streamId1 = 0xFF;
static uint8_t g_streamId2 = 0xFF;
static NLSTK_SsapUuid_S g_streamManagementServiceUuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x06, 0x05};
static NLSTK_SsapUuid_S g_sourcePointUuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x10, 0x15};
static NLSTK_SsapUuid_S g_sinkPointUuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x10, 0x16};
static NLSTK_SsapUuid_S g_controlPointUuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x10, 0x17};
static NLSTK_SsapUuid_S g_streamStateChangeUuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x10, 0x18};

static NLSTK_SsapUuid_S g_audioPublicPropServiceUuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x06, 0x06};
static NLSTK_SsapUuid_S g_sourcePropertyUuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x10, 0x19};
static NLSTK_SsapUuid_S g_sourceAbilityUuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x10, 0x1A};
static NLSTK_SsapUuid_S g_sinkPropertyUuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x10, 0x1F};
static NLSTK_SsapUuid_S g_sinkAbilityUuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x10, 0x20};
static NLSTK_SsapUuid_S g_sourceStreamUuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x10, 0x77};
static NLSTK_SsapUuid_S g_sinkStreamUuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x10, 0x78};

static void Stub_ActmEventCbk(SLE_Addr_S *addr, uint8_t eventType, uint8_t result, void *param);

static void Stub_ActmPropCbk(SLE_Addr_S *addr, uint8_t num, NLSTK_ActmProp_S *prop);

static void Stub_ActmBitrateCbk(NLSTK_ActmBitrateChange_S *bitrate);

class IT_ACTM_TEST : public testing::Test {
public:
    NiceMock<ScheduleMock> scheduleMock;
    NiceMock<CmMock> cmMock;
    NiceMock<DtapMock> dtapMock;
    NiceMock<QosmMock> qosmMock;
    NiceMock<DliMock> dliMock;
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

        EXPECT_CALL(qosmMock, QOSM_AutoRateRegisterCallback).WillRepeatedly(TEST_QOSM_AutoRateRegisterCallback);
        EXPECT_CALL(qosmMock, QOSM_AutoRateUnregisterCallback).WillRepeatedly(TEST_QOSM_AutoRateUnregisterCallback);
        EXPECT_CALL(qosmMock, QOSM_AutoRateSetTestParam).WillRepeatedly(TEST_QOSM_AutoRateSetTestParam);
        EXPECT_CALL(qosmMock, QOSM_AutoRateAddConnection).WillRepeatedly(TEST_QOSM_AutoRateAddConnection);
        EXPECT_CALL(qosmMock, QOSM_AutoRateAddDataPath).WillRepeatedly(TEST_QOSM_AutoRateAddDataPath);
        EXPECT_CALL(qosmMock, QOSM_AutoRateDeleteDataPath).WillRepeatedly(TEST_QOSM_AutoRateDeleteDataPath);
        EXPECT_CALL(qosmMock, QOSM_AutoRateDeleteConnection).WillRepeatedly(TEST_QOSM_AutoRateDeleteConnection);
        EXPECT_CALL(qosmMock, QOSM_AutoRateSetEarphoneFeedback).WillRepeatedly(TEST_QOSM_AutoRateSetEarphoneFeedback);

        TEST_DLI_Init();
        EXPECT_CALL(dliMock, DLI_CmdCbkReg).WillRepeatedly(TEST_DLI_CmdCbkReg);
        EXPECT_CALL(dliMock, DLI_CmdCbkUnReg).WillRepeatedly(TEST_DLI_CmdCbkUnReg);
        EXPECT_CALL(dliMock, DLI_SetMeasureParam).WillRepeatedly(TEST_DLI_SetMeasureParam);
        EXPECT_CALL(dliMock, DLI_SetMeasureEnable).WillRepeatedly(TEST_DLI_SetMeasureEnable);
        EXPECT_CALL(dliMock, DLI_ReadRemoteMeasureCaps).WillRepeatedly(TEST_DLI_ReadRemoteMeasureCaps);
        EXPECT_CALL(dliMock, DLI_EnableIMGEncryption).WillRepeatedly(TEST_DLI_EnableIMGEncryption);

        // 步骤2：初始化SSAP模块
        SSAP_LinkInit();
        SSAP_Init();

        CdsmInit();
        SmInit();
        ActmInit();
        ActmEnable();
        NLSTK_ActmCbk_S cbk = {0};
        cbk.bitCbk = Stub_ActmBitrateCbk;
        cbk.eventCbk = Stub_ActmEventCbk;
        cbk.propCbk = Stub_ActmPropCbk;
        NLSTK_ActmRegisterCallback(&cbk);
    }

    void TearDown() override
    {
        ActmDisable();
        ActmDeinit();
        SmDeInit();
        CdsmDeInit();
        // 步骤5：去初始化SSAP模块
        SSAP_DeInit();
        SSAP_LinkDeInit();
    }
};

static void Stub_ActmEventCbk(SLE_Addr_S *addr, uint8_t eventType, uint8_t result, void *param)
{
    CP_LOG_INFO("Enter Stub_ActmEventCbk, g_event = %d", g_event);
    g_event = eventType;
    g_result = result;
    if (eventType == NLSTK_ACTM_EVENT_CREATE_STREAM && param != NULL) {
        NLSTK_ActmStreamInfo_S *info = (NLSTK_ActmStreamInfo_S *)param;
        if (info->commType == NLSTK_ACTM_UNICAST) {
            g_streamId1 = info->streamId;
        } else if (info->commType == NLSTK_ACTM_MULTICAST) {
            g_streamId2 = info->streamId;
        }
    } else if (eventType == NLSTK_ACTM_EVENT_DELETE_STREAM && param != NULL) {
        NLSTK_ActmStreamInfo_S *info = (NLSTK_ActmStreamInfo_S *)param;
        if (info->commType == NLSTK_ACTM_UNICAST) {
            g_streamId1 = 0xFF;
        } else if (info->commType == NLSTK_ACTM_MULTICAST) {
            g_streamId2 = 0xFF;
        }
    }
}

static void Stub_ActmPropCbk(SLE_Addr_S *addr, uint8_t num, NLSTK_ActmProp_S *prop)
{
    g_profileReadState = true;
}

static void Stub_ActmBitrateCbk(NLSTK_ActmBitrateChange_S *bitrate)
{
}

static void STUB_CacheActmService()
{
    SsapCacheServInfo_S serv = {0};
    int ret = 255;
    serv.handle = 0x10;
    serv.endHandle = 0x14;
    serv.serviceType = ITEM_TYPE_STD_PRIMARY_SERVICE;
    memcpy_s(&serv.uuid, sizeof(NLSTK_SsapUuid_S), &g_streamManagementServiceUuid, sizeof(NLSTK_SsapUuid_S));
    ret = SsapcCacheServ(&g_addr1, &serv);

    NLSTK_SsapPrty_S controlPoint = {0};
    controlPoint.handle = 0x11;
    memcpy_s(&controlPoint.uuid, sizeof(NLSTK_SsapUuid_S), &g_controlPointUuid, sizeof(NLSTK_SsapUuid_S));
    ret = SsapcCacheMethod(&g_addr1, &controlPoint);

    NLSTK_SsapPrty_S streamStateChangeEvent = {0};
    streamStateChangeEvent.handle = 0x12;
    memcpy_s(&streamStateChangeEvent.uuid, sizeof(NLSTK_SsapUuid_S), &g_streamStateChangeUuid, sizeof(NLSTK_SsapUuid_S));
    ret = SsapcCacheEvent(&g_addr1, &streamStateChangeEvent);

    NLSTK_SsapPrty_S sourcePointProp = {0};
    sourcePointProp.handle = 0x13;
    memcpy_s(&sourcePointProp.uuid, sizeof(NLSTK_SsapUuid_S), &g_sourcePointUuid, sizeof(NLSTK_SsapUuid_S));
    ret = SsapcCachePrty(&g_addr1, &sourcePointProp);

    NLSTK_SsapPrty_S sinkPointProp = {0};
    sinkPointProp.handle = 0x14;
    memcpy_s(&sinkPointProp.uuid, sizeof(NLSTK_SsapUuid_S), &g_sinkPointUuid, sizeof(NLSTK_SsapUuid_S));
    ret = SsapcCachePrty(&g_addr1, &sinkPointProp);

    SsapCacheServInfo_S propServ = {0};
    propServ.handle = 0x15;
    propServ.endHandle = 0x1B;
    propServ.serviceType = ITEM_TYPE_STD_PRIMARY_SERVICE;
    memcpy_s(&propServ.uuid, sizeof(NLSTK_SsapUuid_S), &g_audioPublicPropServiceUuid, sizeof(NLSTK_SsapUuid_S));
    ret = SsapcCacheServ(&g_addr1, &propServ);

    NLSTK_SsapPrty_S sourceProp = {0};
    sourceProp.handle = 0x16;
    memcpy_s(&sourceProp.uuid, sizeof(NLSTK_SsapUuid_S), &g_sourcePropertyUuid, sizeof(NLSTK_SsapUuid_S));
    SsapcCachePrty(&g_addr1, &sourceProp);

    NLSTK_SsapPrty_S sourceAbilityProp = {0};
    sourceAbilityProp.handle = 0x17;
    memcpy_s(&sourceAbilityProp.uuid, sizeof(NLSTK_SsapUuid_S), &g_sourceAbilityUuid, sizeof(NLSTK_SsapUuid_S));
    SsapcCachePrty(&g_addr1, &sourceAbilityProp);

    NLSTK_SsapPrty_S sinkPropertyProp = {0};
    sinkPropertyProp.handle = 0x18;
    memcpy_s(&sinkPropertyProp.uuid, sizeof(NLSTK_SsapUuid_S), &g_sinkPropertyUuid, sizeof(NLSTK_SsapUuid_S));
    SsapcCachePrty(&g_addr1, &sinkPropertyProp);

    NLSTK_SsapPrty_S sinkAbilityProp = {0};
    sinkAbilityProp.handle = 0x19;
    memcpy_s(&sinkAbilityProp.uuid, sizeof(NLSTK_SsapUuid_S), &g_sinkAbilityUuid, sizeof(NLSTK_SsapUuid_S));
    SsapcCachePrty(&g_addr1, &sinkAbilityProp);

    NLSTK_SsapPrty_S sourceStreamProp = {0};
    sourceStreamProp.handle = 0x1A;
    memcpy_s(&sourceStreamProp.uuid, sizeof(NLSTK_SsapUuid_S), &g_sourceStreamUuid, sizeof(NLSTK_SsapUuid_S));
    SsapcCachePrty(&g_addr1, &sourceStreamProp);

    NLSTK_SsapPrty_S sinkStreamProp = {0};
    sinkStreamProp.handle = 0x1B;
    memcpy_s(&sinkStreamProp.uuid, sizeof(NLSTK_SsapUuid_S), &g_sinkStreamUuid, sizeof(NLSTK_SsapUuid_S));
    SsapcCachePrty(&g_addr1, &sinkStreamProp);

    SsapcCacheServDiscFinish(&g_addr1);
}

static uint8_t g_readSourcePointRsp[] = {0x09, 0x03, 0x01, 0x01, 0x00, 0x00};
static uint8_t g_readSinkPointRsp[] = {0x09, 0x03, 0x03, 0x03, 0x00, 0x00};
static uint8_t g_readSourcePropRsp[] = {0x09, 0x03, 0x01, 0x21, 0x17, 0x00, 0x1A, 0x00};
static uint8_t g_readSinkPropRsp[] = {0x09, 0x03, 0x03, 0x21, 0x19, 0x00, 0x1B, 0x00};
static uint8_t g_readSourceAbilityRsp[] = {0x09, 0x03, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x18, 0x01, 0x01, 0x00, 0x02, 0x02, 0x20,
    0x00, 0x03, 0x01, 0x02, 0x04, 0x02, 0x01, 0x02, 0x05, 0x01, 0x02, 0x06, 0x05, 0x2C, 0x00, 0x00, 0x00, 0x00, 0x02, 0xFE, 0x0F, 0x00, 0x00};
static uint8_t g_readSinkAbilityRsp[] = {0x09, 0x03, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x18, 0x01, 0x01, 0x01, 0x02, 0x02, 0x80, 0x02,
    0x03, 0x01, 0x06, 0x04, 0x02, 0x01, 0x02, 0x05, 0x01, 0x02, 0x06, 0x05, 0xD8, 0x03, 0x00, 0x00, 0x00, 0xFF, 0x09, 0x00, 0x01,
    0x00, 0x18, 0x01, 0x01, 0x04, 0x02, 0x02, 0x80, 0x02, 0x03, 0x01, 0x06, 0x04, 0x02, 0x01, 0x02, 0x05, 0x01, 0x02, 0x06, 0x05,
    0x5C, 0x37, 0x00, 0x00, 0x00, 0x01, 0xFE, 0x0F, 0x00, 0x00};
static uint8_t g_sourceStreamTypeRsp[] = {0x09, 0x03, 0xFE, 0x0F, 0x00, 0x00};
static uint8_t g_sinkStreamTypeRsp[] = {0x09, 0x03, 0xFE, 0x0F, 0x00, 0x00};
static uint8_t g_readErrorRsp[] = {0x09, 0x0B, 0x04, 0x00};
static uint8_t g_writeStreamTypeRsp[] = {0x0E, 0x03, 0x1B, 0x00, 0x02, 0x01, 0x00};

static uint8_t g_configRsp[] = {0x14, 0x03, 0x01, 0x01, 0x03, 0x00};
static uint8_t g_openRsp[] = {0x14, 0x03, 0x02, 0x01, 0x03, 0x00};
static uint8_t g_startRsp[] = {0x14, 0x03, 0x03, 0x01, 0x03, 0x00};
static uint8_t g_stopRsp[] = {0x14, 0x03, 0x04, 0x01, 0x03, 0x00};
static uint8_t g_releaseRsp[] = {0x14, 0x03, 0x05, 0x01, 0x03, 0x00};
static uint8_t g_bitrateRsp[] = {0x14, 0x03, 0x10, 0x01, 0x03, 0x00};

static uint8_t g_configError[] = {0x14, 0x03, 0x01, 0x01, 0x03, 0x0A};

static uint8_t g_startEvent[] = {0x0F, 0x07, 0x12, 0x00, 0x03, 0x00, 0x01, 0x01, 0x03};
static uint8_t g_stopEvent[] = {0x0F, 0x07, 0x12, 0x00, 0x03, 0x00, 0x02, 0x01, 0x03};
static uint8_t g_releaseEvent[] = {0x0F, 0x07, 0x12, 0x00, 0x03, 0x00, 0x03, 0x01, 0x03};
static uint8_t g_bitrateEvent[] = {0x0F, 0x07, 0x12, 0x00, 0x09, 0x00, 0x10, 0x01, 0x03, 0x05, 0x50, 0x07, 0x00, 0x00, 0x00};

static void STUB_ProfileConnect(void)
{
    TEST_DTAP_SSAP_RevcInitPkt(0);
    TEST_DTAP_RecDataWithPkt(0, TCID_SLE_SMTC, g_readSourcePointRsp, sizeof(g_readSourcePointRsp));
    TEST_DTAP_RecDataWithPkt(0, TCID_SLE_SMTC, g_readSinkPointRsp, sizeof(g_readSinkPointRsp));
    TEST_DTAP_RecDataWithPkt(0, TCID_SLE_SMTC, g_writeStreamTypeRsp, sizeof(g_writeStreamTypeRsp));
    TEST_DTAP_RecDataWithPkt(0, TCID_SLE_SMTC, g_readSourcePropRsp, sizeof(g_readSourcePropRsp));
    TEST_DTAP_RecDataWithPkt(0, TCID_SLE_SMTC, g_readSinkPropRsp, sizeof(g_readSinkPropRsp));
    TEST_DTAP_RecDataWithPkt(0, TCID_SLE_SMTC, g_readSourceAbilityRsp, sizeof(g_readSourceAbilityRsp));
    TEST_DTAP_RecDataWithPkt(0, TCID_SLE_SMTC, g_readSinkAbilityRsp, sizeof(g_readSinkAbilityRsp));
    TEST_DTAP_RecDataWithPkt(0, TCID_SLE_SMTC, g_sourceStreamTypeRsp, sizeof(g_sourceStreamTypeRsp));
    TEST_DTAP_RecDataWithPkt(0, TCID_SLE_SMTC, g_sinkStreamTypeRsp, sizeof(g_sinkStreamTypeRsp));
    TEST_DTAP_RecDataWithPkt(0, TCID_SLE_SMTC, g_readErrorRsp, sizeof(g_readErrorRsp));
}

TEST_F(IT_ACTM_TEST, ACTM_IT_001)
{
    // 完成建链
    CM_LogicLinkState_S linkState = {.lcid = 0, .result = 0,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr1, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);
    EXPECT_CALL(cmMock, CM_GetLogicLinkByAddr).WillRepeatedly(TEST_CM_GetLogicLinkByAddr);
    STUB_CacheActmService();

    NLSTK_ActmReadRemoteProp(&g_addr1);

    STUB_ProfileConnect();

    EXPECT_TRUE(g_profileReadState);
    // 创建两条音频流
    NLSTK_ActmStreamParam_S streamParam = {0};
    streamParam.pointType = NLSTK_ACTM_SINK_POINT;
    streamParam.commType = NLSTK_ACTM_UNICAST;
    NLSTK_ActmCreateStream(&g_addr1, &streamParam);
    streamParam.pointType = NLSTK_ACTM_SOURCE_POINT;
    streamParam.commType = NLSTK_ACTM_MULTICAST;
    NLSTK_ActmCreateStream(&g_addr1, &streamParam);
    EXPECT_EQ(g_streamId1, 0);
    EXPECT_EQ(g_streamId2, 1);

    // 发送两条音频流配置请求
    NLSTK_ActmConfig_S sinkConfig = {0};
    sinkConfig.codec.codecId = L2HC_CODEC_NORMAL;
    sinkConfig.codec.l2hc.frameConf = 0x3;
    sinkConfig.codec.l2hc.bpsConf = 0x6;
    sinkConfig.codec.l2hc.bpsRange = 0x40;
    sinkConfig.channel.qosId = 0x1;
    sinkConfig.channel.trans = 0x1;
    NLSTK_ActmConfig_S sourceConfig = {0};
    sourceConfig.codec.codecId = L2HC_CODEC_VOICE;
    sourceConfig.codec.l2hc.bpsConf = 0x5;
    sourceConfig.codec.l2hc.bpsRange = 0x20;
    sourceConfig.channel.qosId = 0x5;
    sourceConfig.channel.trans = 0x1;
    NLSTK_ActmConfigParam_S configParam = {0};
    configParam.isImg = true;
    configParam.streamId = g_streamId2;
    configParam.srcConfig = &sourceConfig;
    NLSTK_ActmConfigAudioStream(&g_addr1, &configParam);
    TEST_DTAP_RecDataWithPkt(0, TCID_SLE_SMTC, g_configRsp, sizeof(g_configRsp));
    EXPECT_EQ(g_event, NLSTK_ACTM_EVENT_CONFIG);
    configParam.streamId = g_streamId1;
    configParam.sinkConfig = &sinkConfig;
    configParam.srcConfig = NULL;
    NLSTK_ActmConfigAudioStream(&g_addr1, &configParam);
    TEST_DTAP_RecDataWithPkt(0, TCID_SLE_SMTC, g_configRsp, sizeof(g_configRsp));
    EXPECT_EQ(g_event, NLSTK_ACTM_EVENT_CONFIG);

    // 开启第一条音频流通道
    NLSTK_ActmOpenParam_S openParam = {0};
    openParam.streamId = g_streamId1;
    NLSTK_ActmOpenAudioStream(&g_addr1, &openParam);
    TEST_DTAP_RecDataWithPkt(0, TCID_SLE_SMTC, g_openRsp, sizeof(g_openRsp));
    EXPECT_EQ(g_event, NLSTK_ACTM_EVENT_OPEN);

    // 改变第一条音频流状态
    NLSTK_ActmChangeParam_S startParam = {0};
    startParam.streamId = g_streamId1;
    startParam.op = NLSTK_ACTM_STREAM_TRANS;
    NLSTK_ActmChangeAudioStream(&g_addr1, &startParam);
    TEST_QOSM_NotifyParam(0, QOSM_PARAM_SETTED, false, QOSM_SUCCESS);
    TEST_QOSM_NotifyConnection(0, QOSM_CONNECTION_ADDED, QOSM_SUCCESS);
    TEST_DTAP_RecDataWithPkt(0, TCID_SLE_SMTC, g_startRsp, sizeof(g_startRsp));
    TEST_DTAP_RecDataWithPkt(0, TCID_SLE_SMTC, g_startEvent, sizeof(g_startEvent));
    EXPECT_EQ(g_event, NLSTK_ACTM_EVENT_TRANS);

    // 设置上下行datapath
    NLSTK_ActmSetDirection(&g_addr1, NLSTK_ACTM_DIRECTION_BOTH);
    TEST_QOSM_NotifyDataPath(0, QOSM_DATAPATH_ADDED, QOSM_SUCCESS, QOSM_DOWN);
    TEST_QOSM_NotifyDataPath(0, QOSM_DATAPATH_ADDED, QOSM_SUCCESS, QOSM_UP);

    NLSTK_ActmBitrateParam_S bitrateParam = {0};
    bitrateParam.streamId = g_streamId1;
    NLSTK_ActmUpdateBitrate(&g_addr1, &bitrateParam);
    TEST_DTAP_RecDataWithPkt(0, TCID_SLE_SMTC, g_bitrateRsp, sizeof(g_bitrateRsp));
    EXPECT_EQ(g_event, NLSTK_ACTM_EVENT_BITRATE);

    TEST_QOSM_NotifyBitrate(0);
    TEST_DTAP_RecDataWithPkt(0, TCID_SLE_SMTC, g_bitrateEvent, sizeof(g_bitrateEvent));

    NLSTK_ActmChangeParam_S stopParam = {0};
    stopParam.streamId = g_streamId1;
    stopParam.op = NLSTK_ACTM_STREAM_STOP;
    NLSTK_ActmChangeAudioStream(&g_addr1, &stopParam);
    TEST_DTAP_RecDataWithPkt(0, TCID_SLE_SMTC, g_stopRsp, sizeof(g_stopRsp));
    TEST_DTAP_RecDataWithPkt(0, TCID_SLE_SMTC, g_stopEvent, sizeof(g_stopEvent));
    EXPECT_EQ(g_event, NLSTK_ACTM_EVENT_STOP);

    NLSTK_ActmReleaseParam_S releaseParam = {0};
    releaseParam.streamId = g_streamId1;
    NLSTK_ActmReleaseAudioStream(&g_addr1, &releaseParam);
    TEST_QOSM_NotifyDataPath(0, QOSM_DATAPATH_DELETED, QOSM_SUCCESS, QOSM_DOWN);
    TEST_QOSM_NotifyDataPath(0, QOSM_DATAPATH_DELETED, QOSM_SUCCESS, QOSM_UP);
    TEST_QOSM_NotifyConnection(0, QOSM_CONNECTION_DELETED, QOSM_SUCCESS);
    TEST_QOSM_NotifyParam(0, QOSM_PARAM_REMOVED, false, QOSM_SUCCESS);
    TEST_DTAP_RecDataWithPkt(0, TCID_SLE_SMTC, g_releaseRsp, sizeof(g_releaseRsp));
    TEST_DTAP_RecDataWithPkt(0, TCID_SLE_SMTC, g_releaseEvent, sizeof(g_releaseEvent));
    EXPECT_EQ(g_event, NLSTK_ACTM_EVENT_RELEASE);

    releaseParam.streamId = g_streamId2;
    NLSTK_ActmReleaseAudioStream(&g_addr1, &releaseParam);
    TEST_QOSM_NotifyDataPath(0, QOSM_DATAPATH_DELETED, QOSM_SUCCESS, QOSM_DOWN);
    TEST_QOSM_NotifyDataPath(0, QOSM_DATAPATH_DELETED, QOSM_SUCCESS, QOSM_UP);
    TEST_QOSM_NotifyConnection(0, QOSM_CONNECTION_DELETED, QOSM_SUCCESS);
    TEST_QOSM_NotifyParam(0, QOSM_PARAM_REMOVED, true, QOSM_SUCCESS);
    TEST_DTAP_RecDataWithPkt(0, TCID_SLE_SMTC, g_releaseRsp, sizeof(g_releaseRsp));
    EXPECT_EQ(g_event, NLSTK_ACTM_EVENT_RELEASE);

    NLSTK_ActmDeleteStream(&g_addr1, g_streamId1);
    NLSTK_ActmDeleteStream(&g_addr1, g_streamId2);
    EXPECT_EQ(g_streamId1, 0xFF);
    EXPECT_EQ(g_streamId2, 0xFF);

    NLSTK_ActmDisconnect(&g_addr1);
    CM_LogicLinkState_S linkState2 = {.lcid = 0, .result = 2,};
    (void)memcpy_s(&linkState2.addr, sizeof(SLE_Addr_S), &g_addr1, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState2);
    g_profileReadState = false;
    g_event = 0xFF;
}

TEST_F(IT_ACTM_TEST, ACTM_IT_002)
{
    CM_LogicLinkState_S linkState = {.lcid = 0, .result = 0,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr1, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);
    STUB_CacheActmService();
    NLSTK_ActmReadRemoteProp(&g_addr1);
    STUB_ProfileConnect();

    NLSTK_ActmStreamParam_S streamParam = {0};
    streamParam.pointType = NLSTK_ACTM_SINK_POINT;
    streamParam.commType = NLSTK_ACTM_UNICAST;
    NLSTK_ActmCreateStream(&g_addr1, &streamParam);
    NLSTK_ActmConfig_S sinkConfig = {0};
    sinkConfig.channel.qosId = 0x4;
    NLSTK_ActmConfigParam_S configParam = {0};
    configParam.streamId = g_streamId1;
    configParam.sinkConfig = &sinkConfig;
    NLSTK_ActmConfigAudioStream(&g_addr1, &configParam);
    TEST_DTAP_RecDataWithPkt(0, TCID_SLE_SMTC, g_configError, sizeof(g_configError));
    EXPECT_EQ(g_event, NLSTK_ACTM_EVENT_CONFIG);
    EXPECT_EQ(g_result, 0xA);

    NLSTK_ActmConfigAudioStream(&g_addr1, &configParam);
    TEST_DTAP_RecDataWithPkt(0, TCID_SLE_SMTC, g_configRsp, sizeof(g_configRsp));
    NLSTK_ActmChangeParam_S startParam = {0};
    startParam.streamId = g_streamId1;
    startParam.op = NLSTK_ACTM_STREAM_TRANS;
    NLSTK_ActmChangeAudioStream(&g_addr1, &startParam);
    TEST_DTAP_RecDataWithPkt(0, TCID_SLE_SMTC, g_startRsp, sizeof(g_startRsp));
    TEST_QOSM_NotifyParam(0, QOSM_PARAM_SETTED, false, QOSM_FAIL);
    EXPECT_EQ(g_event, NLSTK_ACTM_EVENT_TRANS);
    EXPECT_EQ(g_result, NLSTK_ACTM_QOSM_ERROR);

    NLSTK_ActmChangeAudioStream(&g_addr1, &startParam);
    TEST_DTAP_RecDataWithPkt(0, TCID_SLE_SMTC, g_startRsp, sizeof(g_startRsp));
    TEST_QOSM_NotifyParam(0, QOSM_PARAM_SETTED, false, QOSM_SUCCESS);
    TEST_QOSM_NotifyConnection(0, QOSM_CONNECTION_ADDED, QOSM_FAIL);
    EXPECT_EQ(g_event, NLSTK_ACTM_EVENT_TRANS);
    EXPECT_EQ(g_result, NLSTK_ACTM_QOSM_ERROR);

    NLSTK_ActmChangeAudioStream(&g_addr1, &startParam);
    TEST_DTAP_RecDataWithPkt(0, TCID_SLE_SMTC, g_startRsp, sizeof(g_startRsp));
    TEST_QOSM_NotifyParam(0, QOSM_PARAM_SETTED, false, QOSM_SUCCESS);
    TEST_QOSM_NotifyConnection(0, QOSM_CONNECTION_ADDED, QOSM_SUCCESS);
    NLSTK_ActmSetDirection(&g_addr1, NLSTK_ACTM_DIRECTION_BOTH);
    TEST_QOSM_NotifyDataPath(0, QOSM_DATAPATH_ADDED, QOSM_SUCCESS, QOSM_DOWN);
    TEST_QOSM_NotifyDataPath(0, QOSM_DATAPATH_ADDED, QOSM_SUCCESS, QOSM_UP);
    TEST_QOSM_NotifyConnection(0, QOSM_CONNECTION_DELETED, QOSM_SUCCESS);
    TEST_QOSM_NotifyParam(0, QOSM_PARAM_REMOVED, false, QOSM_SUCCESS);
    EXPECT_EQ(g_event, NLSTK_ACTM_EVENT_RELEASE);

    NLSTK_ActmDeleteStream(&g_addr1, g_streamId1);
    NLSTK_ActmDisconnect(&g_addr1);
    CM_LogicLinkState_S linkState2 = {.lcid = 0, .result = 2,};
    (void)memcpy_s(&linkState2.addr, sizeof(SLE_Addr_S), &g_addr1, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState2);
    g_profileReadState = false;
    g_event = 0xFF;
}