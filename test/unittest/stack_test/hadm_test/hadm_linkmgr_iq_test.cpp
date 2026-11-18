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

#include <cstring>
#include "securec.h"
#include "gtest/gtest.h"
#include "sdf_mem.h"

#include "stack_schedule_mock.h"
#include "stack_schedule_stub.h"
#include "stack_cm_mock.h"
#include "stack_cm_stub.h"
#include "stack_dli_mock.h"
#include "stack_dli_stub.h"

#include "cm_logic_link_api.h"
#include "dli_event_struct.h"
#include "dli_cmd.h"
#include "nlstk_cfgdb.h"

#include "hadm_init.h"
#include "hadm_api.h"
#include "hadm_parser_iq.h"
#include "hadm_link_manager.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

// 复用 hadm_test.cpp 铺好的主链路台架（其地址与链路号对该文件私有，经由这些函数驱动）
void SetHadmSmToIdle();
void SetHadmSmToReady();
void SetHadmSmToConnectionConfig();
void SetHadmSmToConfigSounding();
void SetHadmSmToEnableSounding();
void SetHadmSmToSounding();
void SetHadmSmToDisableSounding();
void TriggerCmConnectEvent();
void TriggerCmDisconnectEvent();
void TriggerCmReportFeatureEvent();
void TriggerDliReportRemoteCsEvent();
void TriggerHadmUserStartEvent();

// 能力数组里星闪测距特性所在字节与取值、能力数组中校准偏移字段位置、IQ 报文中信道图起始位置
#define TEST_FEATURE_BYTE_INDEX 8
#define TEST_FEATURE_SOUNDING_BIT 0x04
#define TEST_CS_CAPS_PHASE_OFFSET 20
#define TEST_CS_CAPS_TOF_OFFSET 22
#define TEST_IQ_CHMAP_OFFSET 10

static SLE_Addr_S g_secondAddr = {.type = PUBLIC_ADDRESS, .addr = {0x0A, 0x0A, 0x0B, 0x0B, 0x0C, 0x0C}};
static uint16_t g_secondLcid = 2;
static const uint16_t g_unknownLcid = 0x7FFF;
static SLE_Addr_S g_unknownAddr = {.type = PUBLIC_ADDRESS, .addr = {0x0F, 0x0F, 0x0E, 0x0E, 0x0D, 0x0D}};

static uint32_t g_iqReportCnt;
static uint16_t g_localNvOffset;
static uint16_t g_remoteNvOffset;
static uint16_t g_localTofOffset;
static uint16_t g_remoteTofOffset;
static uint8_t g_iqMultiTone;
static uint32_t g_iqChnlNum;

static NLSTK_Errcode_E g_userErrorCode = NLSTK_ERRCODE_MAX;

static void STUB_LinkMgrIqCbk(SLE_Addr_S *addr, HadmSoundingIqData_S *args)
{
    g_iqReportCnt++;
    g_localNvOffset = args->localNvOffset;
    g_remoteNvOffset = args->remoteNvOffset;
    g_localTofOffset = args->localTofOffset;
    g_remoteTofOffset = args->remoteTofOffset;
    g_iqMultiTone = args->isMultiTone;
    g_iqChnlNum = args->iqChnlNum;
}

static void STUB_LinkMgrControlCbk(SLE_Addr_S *addr, HadmUserOperate_E ctrlType, NLSTK_Errcode_E errorcode)
{
    g_userErrorCode = errorcode;
}

static void STUB_LinkMgrStateCbk(HadmSoundingStateInfo_S *state)
{
    return;
}

class HADM_LINKMGR_TEST : public testing::Test {
public:
    NiceMock<ScheduleMock> scheduleMock;
    NiceMock<CmMock> cmMock;
    NiceMock<DliMock> dliMock;
protected:
    virtual void SetUp()
    {
        g_iqReportCnt = 0;
        g_userErrorCode = NLSTK_ERRCODE_MAX;
        TEST_ScheduleInit();
        EXPECT_CALL(scheduleMock, SchedulePostTask).WillRepeatedly(TEST_SchedulePostTaskStub);
        EXPECT_CALL(scheduleMock, SchedulePostTaskBlocked).WillRepeatedly(TEST_SchedulePostTaskBlockedStub);

        TEST_CM_Init();
        EXPECT_CALL(cmMock, CM_RegLogicLinkListener).WillRepeatedly(TEST_CM_RegLogicLinkListener);
        EXPECT_CALL(cmMock, CM_UnRegLogicLinkListener).WillRepeatedly(TEST_CM_UnRegLogicLinkListener);

        TEST_DLI_Init();
        EXPECT_CALL(dliMock, DLI_CmdCbkReg).WillRepeatedly(TEST_DLI_CmdCbkReg);
        EXPECT_CALL(dliMock, DLI_CmdCbkUnReg).WillRepeatedly(TEST_DLI_CmdCbkUnReg);
        EXPECT_CALL(dliMock, DLI_SetMeasureParam).WillRepeatedly(TEST_DLI_SetMeasureParam);
        EXPECT_CALL(dliMock, DLI_SetMeasureEnable).WillRepeatedly(TEST_DLI_SetMeasureEnable);
        EXPECT_CALL(dliMock, DLI_ReadRemoteMeasureCaps).WillRepeatedly(TEST_DLI_ReadRemoteMeasureCaps);

        HadmInit();
        HadmSoundingCbk_S cbks = {0};
        cbks.controlResultCbk = STUB_LinkMgrControlCbk;
        cbks.reportIqDataCbk = STUB_LinkMgrIqCbk;
        cbks.soundingStateReportCbk = STUB_LinkMgrStateCbk;
        HadmRegCbk(&cbks);
    }

    virtual void TearDown()
    {
        HadmDeInit();
        TEST_StackScheduleDeInit();
        TEST_CM_DeInit();
        TEST_DLI_DeInit();
    }
};

static void ConnectSecondLink(void)
{
    CM_LogicLinkState_S linkState = {.lcid = g_secondLcid, .result = 0};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_secondAddr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_HADM, &linkState);
}

static void SecondLinkRemoteFeatures(uint8_t supportBit)
{
    CM_LogicLinkRemoteFeatures_S features = {0};
    features.lcid = g_secondLcid;
    features.addr = g_secondAddr;
    features.features[TEST_FEATURE_BYTE_INDEX] = supportBit;
    TEST_CM_RemoteFeatureCbk(CM_MODULE_HADM, &features);
}

static void SecondLinkRemoteCaps(uint16_t nvOffset, uint16_t tofOffset)
{
    DLI_ExecuteCmdRetParam retParam = {0};
    DLI_ReadRemoteCsCapsEvt caps = {.connHandle = g_secondLcid};
    caps.caps[TEST_CS_CAPS_PHASE_OFFSET] = nvOffset & 0xFF;
    caps.caps[TEST_CS_CAPS_PHASE_OFFSET + 1] = (nvOffset >> 8) & 0xFF;
    caps.caps[TEST_CS_CAPS_TOF_OFFSET] = tofOffset & 0xFF;
    caps.caps[TEST_CS_CAPS_TOF_OFFSET + 1] = (tofOffset >> 8) & 0xFF;
    retParam.eventParameter = &caps;
    TEST_DLI_EventCbk(DLI_CBK_READ_REMOTE_MEASURE_CAPS, 0, &retParam);
}

static void SecondLinkToReady(uint16_t nvOffset, uint16_t tofOffset)
{
    ConnectSecondLink();
    SecondLinkRemoteFeatures(TEST_FEATURE_SOUNDING_BIT);
    SecondLinkRemoteCaps(nvOffset, tofOffset);
}

static void SecondLinkToSounding(uint16_t nvOffset, uint16_t tofOffset)
{
    SecondLinkToReady(nvOffset, tofOffset);
    HadmConnectionParam_S connectionParam = {0};
    HadmSoundingParam_S soundParam = {0};
    (void)HadmStartSounding(&g_secondAddr, &connectionParam, &soundParam);

    CM_LogicLinkConnUpdateParam_S connUpdate = {0};
    connUpdate.lcid = g_secondLcid;
    (void)memcpy_s(&connUpdate.addr, sizeof(SLE_Addr_S), &g_secondAddr, sizeof(SLE_Addr_S));
    TEST_CM_ConnUpdateCbk(CM_MODULE_HADM, &connUpdate);

    DLI_ExecuteCmdRetParam configParam = {0};
    TEST_DLI_EventCbk(DLI_CBK_SET_MEASURE_CONFIG_PARAM, 0, &configParam);
    DLI_ExecuteCmdRetParam enableParam = {0};
    TEST_DLI_EventCbk(DLI_CBK_SET_MEASURE_EN, 0, &enableParam);
}

/* 单音 IQ 报文：单通道 chmap，全长 32 字节，尾序 vender/local/remote */
static void MakeSingleToneIq(uint8_t *buff, uint32_t len, uint8_t localId)
{
    (void)memset_s(buff, len, 0, len);
    buff[TEST_IQ_CHMAP_OFFSET] = 0x01; // 单通道
    buff[26] = 0x11; // IQ 数据
    buff[29] = 0xAA; // venderLen
    buff[30] = localId;
    buff[31] = 0xCC; // remoteId
}

static void FeedIqReport(uint8_t *buff, uint32_t len)
{
    DLI_ExecuteCmdRetParam iqParam = {0};
    iqParam.eventParameter = buff;
    iqParam.size = len;
    TEST_DLI_EventCbk(DLI_CBK_MEASURE_IQ_REPORT, 0, &iqParam);
}

/*****************************************************************************************
                                    Test Functions
*****************************************************************************************/

TEST_F(HADM_LINKMGR_TEST, PendingFeaturesInplaceUpdateAndFull)
{
    // 同链路二次上报特性走原位更新；待定表 8 槽占满后再缓存报失败
    SLE_Addr_S addr = g_secondAddr;
    uint32_t ret = HadmCachePendingFeatures(g_secondLcid, &addr, 1);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    ret = HadmCachePendingFeatures(g_secondLcid, &addr, 0); // 原位更新覆盖原值
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    uint8_t support = 1;
    ret = HadmGetPendingFeatures(g_secondLcid, &addr, &support);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(support, 0);

    for (uint16_t lcid = 100; lcid < 108; lcid++) {
        ret = HadmCachePendingFeatures(lcid, &addr, 1);
        EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    }
    ret = HadmCachePendingFeatures(108, &addr, 1);
    EXPECT_EQ(ret, NLSTK_ERRCODE_FAIL);
}

TEST_F(HADM_LINKMGR_TEST, DoubleDeInitKeepsQuiet)
{
    // 反初始化清空链路表与指令队列后，再次反初始化不崩溃
    ConnectSecondLink();
    HadmDeInit();
    EXPECT_EQ(HadmGetSoundingStateByAddr(&g_secondAddr), HADM_SOUNDING_STATE_INVALID);
    HadmDeInit();
    EXPECT_EQ(HadmGetSoundingStateByAddr(&g_secondAddr), HADM_SOUNDING_STATE_INVALID);
}

TEST_F(HADM_LINKMGR_TEST, LinkCbQueryLegs)
{
    // 链路控制块查询：命中返回对象，未命中返回空并报找不到；按链路号反查同理
    ConnectSecondLink();
    EXPECT_NE(HadmGetLinkCb(&g_secondAddr), nullptr);
    EXPECT_EQ(HadmGetLinkCb(&g_unknownAddr), nullptr);
    EXPECT_EQ(HadmGetLinkCb(nullptr), nullptr);

    uint32_t ret = HadmSetSoundingState(&g_unknownAddr, HADM_SOUNDING_STATE_IDLE);
    EXPECT_EQ(ret, NLSTK_HADM_ERRCODE_CAN_NOT_FIND_LINKCB);

    SLE_Addr_S found = {0};
    ret = HadmGetAddrsByLcid(g_secondLcid, &found);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(SDF_CompareSleAddr(&found, &g_secondAddr), 0);
    ret = HadmGetAddrsByLcid(g_unknownLcid, &found);
    EXPECT_EQ(ret, NLSTK_HADM_ERRCODE_CAN_NOT_FIND_LINKCB);
}

TEST_F(HADM_LINKMGR_TEST, SoundingAddrInfoApiLegs)
{
    // 正在测距地址查询：空指针或零容量只返回数量；缓存停止后不再计入数量
    SetHadmSmToConnectionConfig(); // 主链路已缓存启动，计入正在测距
    SLE_Addr_S soundingAddr = {0};
    uint32_t num = HadmGetSoundingAddrInfo(nullptr, 1);
    EXPECT_EQ(num, 1);
    num = HadmGetSoundingAddrInfo(&soundingAddr, 0);
    EXPECT_EQ(num, 1);
    num = HadmGetSoundingAddrInfo(&soundingAddr, 1);
    EXPECT_EQ(num, 1);
    num = HadmGetSoundingNumAndAddr(&soundingAddr, 0); // 只计数不拷贝
    EXPECT_EQ(num, 1);

    uint8_t state = 0;
    NLSTK_Errcode_E ret = HadmGetSoundingState(&soundingAddr, &state);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(state, HADM_SOUNDING_START); // 缓存启动时查询报启动中

    SetHadmSmToEnableSounding();
    HadmStopSounding(&soundingAddr); // 使能流程中请求停止：停止是异步的，查询仍报启动
    ret = HadmGetSoundingState(&soundingAddr, &state);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(state, HADM_SOUNDING_START);

    SetHadmSmToDisableSounding();
    num = HadmGetSoundingAddrInfo(&soundingAddr, 1); // 缓存停止后不再计入
    EXPECT_EQ(num, 0);
}

TEST_F(HADM_LINKMGR_TEST, ParallelLimitRejectsSecondLink)
{
    // 并行规格只有一路：主链路已在启动流程时，另一条就绪链路的启动被拒绝；同链路重复启动放行
    SetHadmSmToConnectionConfig();
    SecondLinkToReady(0, 0);

    HadmConnectionParam_S connectionParam = {0};
    HadmSoundingParam_S soundParam = {0};
    NLSTK_Errcode_E ret = HadmStartSounding(&g_secondAddr, &connectionParam, &soundParam);
    EXPECT_EQ(ret, NLSTK_HADM_ERRCODE_MAX_PARALLEL_SOUNDING_NUM);

    SLE_Addr_S soundingAddr = {0};
    (void)HadmGetSoundingAddrInfo(&soundingAddr, 1);
    ret = HadmStartSounding(&soundingAddr, &connectionParam, &soundParam); // 同链路同参数重复启动
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
}

TEST_F(HADM_LINKMGR_TEST, FeatureReadFailReportsTaskFail)
{
    // 空闲态已缓存启动请求时，读取对端能力命令下发失败要上报启动失败
    SetHadmSmToIdle();
    TriggerHadmUserStartEvent();
    EXPECT_CALL(dliMock, DLI_ReadRemoteMeasureCaps).WillOnce(Return(1)).RetiresOnSaturation();
    TriggerCmReportFeatureEvent();
    EXPECT_EQ(g_userErrorCode, NLSTK_ERRCODE_TASK_FAIL);
}

TEST_F(HADM_LINKMGR_TEST, IqParseSingleAndMultiTone)
{
    // IQ 报文解析：bit10 置位为多音（多一字节信息、通道数乘四）；尾序由 bit14 决定，
    // 置位时本端/对端/厂商在前，未置位时厂商在前——两个标志位互相独立
    uint8_t single[32];
    MakeSingleToneIq(single, sizeof(single), 0);
    HadmIqInfoFromDli_S *singleInfo = HadmPaserIqInfoFromDli(single, sizeof(single));
    ASSERT_NE(singleInfo, nullptr);
    EXPECT_EQ(singleInfo->isMultiTone, 0);
    EXPECT_EQ(singleInfo->iqChnlNum, 1);
    EXPECT_EQ(singleInfo->venderLen, 0xAA);
    EXPECT_EQ(singleInfo->localId, 0);
    EXPECT_EQ(singleInfo->remoteId, 0xCC);
    HadmFreeIqInfo(singleInfo);

    uint8_t multi[42];
    (void)memset_s(multi, sizeof(multi), 0, sizeof(multi));
    multi[4] = 0x00;
    multi[5] = 0x44; // 信息类型 bit10 多音 + bit14 标识在前
    multi[TEST_IQ_CHMAP_OFFSET] = 0x01; // 单通道，多音下通道数乘四
    multi[26] = 0x99; // 多出的 1 字节多音信息
    multi[39] = 0xBB; // 标识在前尾序：本端/对端/厂商
    multi[40] = 0xCC;
    multi[41] = 0xAA;
    HadmIqInfoFromDli_S *multiInfo = HadmPaserIqInfoFromDli(multi, sizeof(multi));
    ASSERT_NE(multiInfo, nullptr);
    EXPECT_EQ(multiInfo->isMultiTone, 1);
    EXPECT_EQ(multiInfo->iqChnlNum, 4);
    EXPECT_EQ(multiInfo->localId, 0xBB);
    EXPECT_EQ(multiInfo->remoteId, 0xCC);
    EXPECT_EQ(multiInfo->venderLen, 0xAA);
    HadmFreeIqInfo(multiInfo);

    multi[5] = 0x04; // 只置多音位不置标识位：多音生效但尾序回退厂商在前
    HadmIqInfoFromDli_S *multiNoIdInfo = HadmPaserIqInfoFromDli(multi, sizeof(multi));
    ASSERT_NE(multiNoIdInfo, nullptr);
    EXPECT_EQ(multiNoIdInfo->isMultiTone, 1);
    EXPECT_EQ(multiNoIdInfo->iqChnlNum, 4);
    EXPECT_EQ(multiNoIdInfo->venderLen, 0xBB);
    EXPECT_EQ(multiNoIdInfo->localId, 0xCC);
    EXPECT_EQ(multiNoIdInfo->remoteId, 0xAA);
    HadmFreeIqInfo(multiNoIdInfo);
    HadmFreeIqInfo(nullptr);
}

TEST_F(HADM_LINKMGR_TEST, IqReportOffsetsLegs)
{
    // 校准偏移边界：对端在范围内保留、越界回退默认；本端脏值经配置库写入后同样回退
    SecondLinkToSounding(0, 0);
    EXPECT_EQ(HadmGetSoundingStateByAddr(&g_secondAddr), HADM_SOUNDING_STATE_SOUNDING);

    uint8_t local[32];
    uint8_t remote[32];
    MakeSingleToneIq(local, sizeof(local), 0);
    MakeSingleToneIq(remote, sizeof(remote), 1);
    FeedIqReport(local, sizeof(local));
    EXPECT_EQ(g_iqReportCnt, 0); // 单边 IQ 不上报

    FeedIqReport(remote, sizeof(remote));
    EXPECT_EQ(g_iqReportCnt, 1);
    EXPECT_EQ(g_remoteNvOffset, 300); // 零值越界回默认
    EXPECT_EQ(g_remoteTofOffset, 0); // 零值不超上限，保留
    EXPECT_EQ(g_localNvOffset, 300);
    EXPECT_EQ(g_localTofOffset, 0);

    HadmRemoteCsParam_S remoteCs = {0};
    remoteCs.phaseCaliOffsetCm = 500; // 范围内保留
    remoteCs.tofCaliOffsetM = 200;
    (void)HadmCacheLinkRemoteCs(&g_secondAddr, &remoteCs);
    FeedIqReport(local, sizeof(local));
    FeedIqReport(remote, sizeof(remote));
    EXPECT_EQ(g_iqReportCnt, 2);
    EXPECT_EQ(g_remoteNvOffset, 500);
    EXPECT_EQ(g_remoteTofOffset, 200);

    remoteCs.phaseCaliOffsetCm = 901; // 超上限回默认
    (void)HadmCacheLinkRemoteCs(&g_secondAddr, &remoteCs);
    FeedIqReport(local, sizeof(local));
    FeedIqReport(remote, sizeof(remote));
    EXPECT_EQ(g_iqReportCnt, 3);
    EXPECT_EQ(g_remoteNvOffset, 300);

    remoteCs.phaseCaliOffsetCm = 100;
    remoteCs.tofCaliOffsetM = 3001; // 超上限回默认
    (void)HadmCacheLinkRemoteCs(&g_secondAddr, &remoteCs);
    FeedIqReport(local, sizeof(local));
    FeedIqReport(remote, sizeof(remote));
    EXPECT_EQ(g_iqReportCnt, 4);
    EXPECT_EQ(g_remoteNvOffset, 100);
    EXPECT_EQ(g_remoteTofOffset, 1385);

    ASSERT_EQ(CfgdbInit(), NLSTK_ERRCODE_SUCCESS);
    DLI_ExecuteCmdRetParam localCapsParam = {0};
    DLI_ReadLocalCsCapsEvt localCaps = {0};
    localCaps.caps[TEST_CS_CAPS_PHASE_OFFSET] = 700 & 0xFF; // 本端范围内保留
    localCaps.caps[TEST_CS_CAPS_PHASE_OFFSET + 1] = (700 >> 8) & 0xFF;
    localCaps.caps[TEST_CS_CAPS_TOF_OFFSET] = 0xA1; // 4001 超上限回默认
    localCaps.caps[TEST_CS_CAPS_TOF_OFFSET + 1] = 0x0F;
    localCapsParam.eventParameter = &localCaps;
    TEST_DLI_EventCbk(DLI_CBK_READ_LOCAL_MEASURE_CAPS, 0, &localCapsParam);
    FeedIqReport(local, sizeof(local));
    FeedIqReport(remote, sizeof(remote));
    EXPECT_EQ(g_iqReportCnt, 5);
    EXPECT_EQ(g_localNvOffset, 700);
    EXPECT_EQ(g_localTofOffset, 1385);
    CfgdbDeinit();
}

TEST_F(HADM_LINKMGR_TEST, DisconnectReleasesCachedIq)
{
    // 断链释放链路控制块时，已缓存的单边 IQ 数据随析构释放，链路状态失效
    SetHadmSmToSounding();
    SLE_Addr_S soundingAddr = {0};
    (void)HadmGetSoundingAddrInfo(&soundingAddr, 1); // 取主链路地址用于事后查询
    uint8_t local[32];
    MakeSingleToneIq(local, sizeof(local), 0);
    FeedIqReport(local, sizeof(local));
    EXPECT_EQ(g_iqReportCnt, 0); // 只有本端数据，等待对端

    TriggerCmDisconnectEvent();
    EXPECT_EQ(HadmGetSoundingStateByAddr(&soundingAddr), HADM_SOUNDING_STATE_INVALID);
    FeedIqReport(local, sizeof(local)); // 链路已释放，孤报不崩溃不上报
    EXPECT_EQ(g_iqReportCnt, 0);
}
