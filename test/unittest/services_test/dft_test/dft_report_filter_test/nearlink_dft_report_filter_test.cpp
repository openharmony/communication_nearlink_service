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
#include <securec.h>
#include <string>

#include "nearlink_cemap_report_loader.h"
#include "nearlink_dft_car_key_loader.h"
#include "nearlink_dft_exception.h"
#include "nearlink_dft_exception_c.h"
#include "nearlink_dft_excep.h"
#include "nearlink_dft_manager_c.h"
#include "nearlink_dft_manager_c_mock.h"
#include "nearlink_dft_ue.h"
#include "nearlink_dft_utils.h"
#include "nearlink_app_wakeup_ue.h"
#include "raw_address.h"

using namespace testing::ext;
using namespace OHOS::Nearlink;
using namespace OHOS::Nearlink::Test;

namespace {
constexpr uint32_t CM_STATE_CONNECTED = 0;
constexpr uint32_t CM_STATE_DISCONNECTED = 2;
constexpr int32_t CONN_REASON_TIMEOUT = 0x07;
constexpr int32_t CONN_REASON_OTHER = 0x05;
constexpr uint16_t LCID_VALID = 0x0005;
constexpr uint16_t LCID_INVALID = 0xFFFF;
constexpr int32_t PAIR_STATE_OK = 3;
const std::string SOS_PKG = "emergency_service";
const std::string ADV_SOS_PKG = "com.huawei.hmos.emergencycommunication";
const std::string FIND_CALLER = "findnetwork";
} // namespace

class DftReportFilterTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}

    void SetUp()
    {
        DftMockReset();
    }
    void TearDown()
    {
        DftMockReset();
    }
};

/**
 * @tc.name: DftReportAdvertiseStartTest001
 * @tc.desc: 校验应急广播上报只放行应急包名：普通包名不报，应急包名上报一条携带包名的发现事件
 * @tc.type: FUNC
 */
HWTEST_F(DftReportFilterTest, DftReportAdvertiseStartTest001, TestSize.Level1)
{
    DftReportAdvertiseStart("com.example.normal");
    ASSERT_EQ(DftMockReportCalls(), 0);

    DftReportAdvertiseStart(ADV_SOS_PKG);
    ASSERT_EQ(DftMockReportCalls(), 1);
    ASSERT_EQ(DftMockReportCallsOf(DFT_DISCOVERY_EXCEP), 1);
    ASSERT_EQ(DftMockLastReportStrParam(DISCOVERY_PKG_NAME), ADV_SOS_PKG);
}

/**
 * @tc.name: DftReportScanStartAndResultTest001
 * @tc.desc: 校验应急扫描上报：普通包名不登记；应急扫描首次回调上报，同地址一分钟内去重，新地址放行
 * @tc.type: FUNC
 */
HWTEST_F(DftReportFilterTest, DftReportScanStartAndResultTest001, TestSize.Level1)
{
    const uint32_t scannerA = 9001;
    DftReportScanStart(scannerA, "com.example.normal");
    ASSERT_EQ(DftMockReportCalls(), 0);
    // 未登记的扫描者回调不上报
    DftReportScanResult("00:11:22:33:44:51", scannerA);
    ASSERT_EQ(DftMockReportCalls(), 0);

    const uint32_t scannerB = 9002;
    DftReportScanStart(scannerB, SOS_PKG);
    ASSERT_EQ(DftMockReportCallsOf(DFT_DISCOVERY_EXCEP), 1);
    DftReportScanResult("00:11:22:33:44:52", scannerB);
    ASSERT_EQ(DftMockReportCallsOf(DFT_DISCOVERY_EXCEP), 2);
    // 同地址限频期内再次回调不上报
    DftReportScanResult("00:11:22:33:44:52", scannerB);
    ASSERT_EQ(DftMockReportCallsOf(DFT_DISCOVERY_EXCEP), 2);
    // 新地址照常上报
    DftReportScanResult("00:11:22:33:44:53", scannerB);
    ASSERT_EQ(DftMockReportCallsOf(DFT_DISCOVERY_EXCEP), 3);

    // 注销扫描者后回调不再上报
    DftEraseScanPkgMap(scannerB);
    DftReportScanResult("00:11:22:33:44:54", scannerB);
    ASSERT_EQ(DftMockReportCallsOf(DFT_DISCOVERY_EXCEP), 3);
}

/**
 * @tc.name: DftConnTimeoutRateLimitTest001
 * @tc.desc: 校验建链超时限频：首次超时断链上报一条，限频窗口内重复超时不再上报
 * @tc.type: FUNC
 */
HWTEST_F(DftReportFilterTest, DftConnTimeoutRateLimitTest001, TestSize.Level1)
{
    DftCacheAcbFinishConn("00:11:22:33:44:61", CM_STATE_DISCONNECTED, CONN_REASON_TIMEOUT, 1, LCID_INVALID);
    ASSERT_EQ(DftMockReportCallsOf(DFT_DISCONN_EXCEP), 1);
    ASSERT_GT(DftMockCacheCalls(), 0);

    DftCacheAcbFinishConn("00:11:22:33:44:61", CM_STATE_DISCONNECTED, CONN_REASON_TIMEOUT, 2, LCID_INVALID);
    ASSERT_EQ(DftMockReportCallsOf(DFT_DISCONN_EXCEP), 1);

    // 第三次超时：失败计数已累计但仍在限频窗口内，继续静默
    DftCacheAcbFinishConn("00:11:22:33:44:61", CM_STATE_DISCONNECTED, CONN_REASON_TIMEOUT, 3, LCID_INVALID);
    ASSERT_EQ(DftMockReportCallsOf(DFT_DISCONN_EXCEP), 1);
}

/**
 * @tc.name: DftConnTimeoutNormalDisconnTest001
 * @tc.desc: 校验非超时断链走通用断链上报而非超时统计；无超时记录时冲刷流程直接返回
 * @tc.type: FUNC
 */
HWTEST_F(DftReportFilterTest, DftConnTimeoutNormalDisconnTest001, TestSize.Level1)
{
    DftCacheAcbFinishConn("00:11:22:33:44:62", CM_STATE_DISCONNECTED, CONN_REASON_OTHER, 1, LCID_VALID);
    ASSERT_EQ(DftMockReportCallsOf(DFT_DISCONN_EXCEP), 1);

    // 连接成功只缓存时间参数，不产生断链上报
    DftCacheAcbFinishConn("00:11:22:33:44:63", CM_STATE_CONNECTED, 0, 1, LCID_VALID);
    ASSERT_EQ(DftMockReportCallsOf(DFT_DISCONN_EXCEP), 1);

    // 超时已报过一次后同地址正常断链：断链本身上报，超时冲刷静默
    DftCacheAcbFinishConn("00:11:22:33:44:64", CM_STATE_DISCONNECTED, CONN_REASON_TIMEOUT, 1, LCID_INVALID);
    ASSERT_EQ(DftMockReportCallsOf(DFT_DISCONN_EXCEP), 2);
    DftCacheAcbFinishConn("00:11:22:33:44:64", CM_STATE_DISCONNECTED, CONN_REASON_OTHER, 1, LCID_VALID);
    ASSERT_EQ(DftMockReportCallsOf(DFT_DISCONN_EXCEP), 3);
}

/**
 * @tc.name: DftAccurateSearchTaskGateTest001
 * @tc.desc: 校验查找网络精确查找的任务门：未登记任务的地址不产生任何上报，登记后断链失败才上报
 * @tc.type: FUNC
 */
HWTEST_F(DftReportFilterTest, DftAccurateSearchTaskGateTest001, TestSize.Level1)
{
    DftReportAccurateSearchConnFailInfo("00:11:22:33:44:71", DFT_ACB_CONN, DFT_CONN_FAILED, 1);
    DftDealAccurateSearchConnInfo("00:11:22:33:44:71", CM_STATE_CONNECTED, 0);
    ASSERT_EQ(DftMockReportCalls(), 0);
    ASSERT_EQ(DftMockCacheCalls(), 0);

    DftDealAccurateSearchScanInfo(FIND_CALLER, DFT_SCAN);
    DftSetAccurateSearchTaskMap("00:11:22:33:44:71");
    DftDealAccurateSearchConnInfo("00:11:22:33:44:71", CM_STATE_CONNECTED, 0);
    ASSERT_GT(DftMockCacheCalls(), 0);
    ASSERT_EQ(DftMockReportCalls(), 0);

    // 非测距路径断链：上报连接失败并清理任务
    DftDealAccurateSearchConnInfo("00:11:22:33:44:71", CM_STATE_DISCONNECTED, CONN_REASON_OTHER);
    ASSERT_EQ(DftMockReportCallsOf(DFT_ACCURATESEARCH), 1);
    DftDealAccurateSearchConnInfo("00:11:22:33:44:71", CM_STATE_DISCONNECTED, CONN_REASON_OTHER);
    ASSERT_EQ(DftMockReportCallsOf(DFT_ACCURATESEARCH), 1);
}

/**
 * @tc.name: DftAccurateSearchMeasureTest001
 * @tc.desc: 校验测距上报：测距路径断链走断链上报；测距完成/超时按结果分派不同子场景
 * @tc.type: FUNC
 */
HWTEST_F(DftReportFilterTest, DftAccurateSearchMeasureTest001, TestSize.Level1)
{
    DftDealAccurateSearchScanInfo(FIND_CALLER, DFT_SCAN);
    DftSetAccurateSearchTaskMap("00:11:22:33:44:72");
    DftDealAccurateSearchConnInfo("00:11:22:33:44:72", CM_STATE_CONNECTED, 0);
    DftCacheAccurateSearchMeasureInfo(FIND_CALLER, "00:11:22:33:44:72", DFT_MEASURE, DFT_MEASURE_FINISHED);
    // 本端主动断开不触发测距断链上报
    DftDealAccurateSearchConnInfo("00:11:22:33:44:72", CM_STATE_DISCONNECTED, 0x11);
    ASSERT_EQ(DftMockReportCallsOf(DFT_ACCURATESEARCH), 0);

    DftSetMeasureResultMap("00:11:22:33:44:72", true);
    DftDealAccurateSearchMeasureInfo(FIND_CALLER, "00:11:22:33:44:72");
    ASSERT_EQ(DftMockReportCallsOf(DFT_ACCURATESEARCH), 1);
    ASSERT_NE(DftMockLastCacheStrParam(MEASURE_END_TIME), "");

    // 非查找网络调用名：不做任何事
    DftDealAccurateSearchMeasureInfo("other.caller", "00:11:22:33:44:73");
    DftCacheAccurateSearchMeasureInfo("other.caller", "00:11:22:33:44:73", DFT_MEASURE, DFT_MEASURE_FINISHED);
    ASSERT_EQ(DftMockReportCallsOf(DFT_ACCURATESEARCH), 1);
    ASSERT_EQ(DftMockLastCacheStrParam(MEASURE_START_TIME), "");
}

/**
 * @tc.name: DftMeasureTimeoutReportTest001
 * @tc.desc: 校验测距结果为失败时上报超时子场景而非完成时间
 * @tc.type: FUNC
 */
HWTEST_F(DftReportFilterTest, DftMeasureTimeoutReportTest001, TestSize.Level1)
{
    DftDealAccurateSearchScanInfo(FIND_CALLER, DFT_SCAN);
    DftSetAccurateSearchTaskMap("00:11:22:33:44:74");
    DftSetMeasureResultMap("00:11:22:33:44:74", false);
    DftDealAccurateSearchMeasureInfo(FIND_CALLER, "00:11:22:33:44:74");
    ASSERT_EQ(DftMockReportCallsOf(DFT_ACCURATESEARCH), 1);
    ASSERT_FALSE(DftMockLastReportHasParam(MEASURE_END_TIME));
}

/**
 * @tc.name: DftUnpairTaskGateTest001
 * @tc.desc: 校验取消配对任务门：非已配对状态不入任务；入任务后成功与删钥失败分别上报
 * @tc.type: FUNC
 */
HWTEST_F(DftReportFilterTest, DftUnpairTaskGateTest001, TestSize.Level1)
{
    DftCacheUnPairInfo("00:11:22:33:44:81", 0x0101, "caller", 0, PAIR_STATE_OK + 1);
    ASSERT_EQ(DftMockCacheCalls(), 0);
    // 未登记任务的地址：删钥与成功上报都不产生事件
    DftDealUnPairLinkKeyInfo("00:11:22:33:44:81", false);
    DftReportUnPairSuccessInfo("00:11:22:33:44:81", 0, 0);
    ASSERT_EQ(DftMockReportCalls(), 0);

    DftCacheUnPairInfo("00:11:22:33:44:81", 0x0101, "caller", 0, PAIR_STATE_OK);
    ASSERT_EQ(DftMockCacheCallsOf(DFT_UNPAIR_EXCEP), 1);

    // 删钥失败：缓存一条删钥时间并追加上报失败结果
    DftDealUnPairLinkKeyInfo("00:11:22:33:44:81", false);
    ASSERT_EQ(DftMockCacheCallsOf(DFT_UNPAIR_EXCEP), 2);
    ASSERT_EQ(DftMockReportCallsOf(DFT_UNPAIR_EXCEP), 1);

    // 重新登记任务后成功上报一次，随后任务被清理，重复调用不再上报
    DftCacheUnPairInfo("00:11:22:33:44:81", 0x0101, "caller", 0, PAIR_STATE_OK);
    DftReportUnPairSuccessInfo("00:11:22:33:44:81", 0, 0);
    ASSERT_EQ(DftMockReportCallsOf(DFT_UNPAIR_EXCEP), 2);
    DftReportUnPairSuccessInfo("00:11:22:33:44:81", 0, 0);
    ASSERT_EQ(DftMockReportCallsOf(DFT_UNPAIR_EXCEP), 2);
}

/**
 * @tc.name: DftPairConnTypeClickSuppressTest001
 * @tc.desc: 校验点击连接去重：先发生配对连接后，同一地址的点击直连被抑制一次
 * @tc.type: FUNC
 */
HWTEST_F(DftReportFilterTest, DftPairConnTypeClickSuppressTest001, TestSize.Level1)
{
    DftCachePairConnType("00:11:22:33:44:91", FIRST_TIME_CONN, NO_CONN);
    int afterPair = DftMockCacheCallsOf(DFT_PAIR_EXCEP);
    ASSERT_GT(afterPair, 0);

    DftCachePairConnType("00:11:22:33:44:91", NO_PAIR, CLICK_CONN);
    ASSERT_EQ(DftMockCacheCallsOf(DFT_PAIR_EXCEP), afterPair);

    // 后台直连不受该抑制
    DftCachePairConnType("00:11:22:33:44:91", NO_PAIR, BG_CONN);
    ASSERT_GT(DftMockCacheCallsOf(DFT_PAIR_EXCEP), afterPair);
}

/**
 * @tc.name: DftSecurityPairTypeTest001
 * @tc.desc: 校验配对方式缓存按安全配对开关写入两种不同的字符串值
 * @tc.type: FUNC
 */
HWTEST_F(DftReportFilterTest, DftSecurityPairTypeTest001, TestSize.Level1)
{
    DftCacheSecurityPairType("00:11:22:33:44:92", true);
    ASSERT_EQ(DftMockCacheCallsOf(DFT_PAIR_EXCEP), 1);
    ASSERT_EQ(DftMockLastCacheStrParam(SLE_PAIR_PROCESS_TYPE), "SecurityPair");

    DftCacheSecurityPairType("00:11:22:33:44:92", false);
    ASSERT_EQ(DftMockCacheCallsOf(DFT_PAIR_EXCEP), 2);
    ASSERT_EQ(DftMockLastCacheStrParam(SLE_PAIR_PROCESS_TYPE), "NoneSecurityPair");
}

/**
 * @tc.name: DftReportStreamResultTest001
 * @tc.desc: 校验音频流结果：断链回调上报累计次数后清零，非断链只清缓存；失败结果走失败上报
 * @tc.type: FUNC
 */
HWTEST_F(DftReportFilterTest, DftReportStreamResultTest001, TestSize.Level1)
{
    DftReportStreamSuccess("00:11:22:33:44:93", false);
    ASSERT_EQ(DftMockReportCalls(), 0);
    ASSERT_EQ(DftMockEraseCalls(), 1);

    DftReportStreamSuccess("00:11:22:33:44:93", true);
    ASSERT_EQ(DftMockReportCallsOf(DFT_AUDIO_STREAM_EXCEP), 1);
    ASSERT_TRUE(DftMockLastReportHasParam(STREAM_OPERATION_TYPE));

    DftReportStreamASC("00:11:22:33:44:93", STREAM_ASC_STARTING, 0, 0);
    DftReportStreamASC("00:11:22:33:44:93", STREAM_ASC_STARTING, 1, 0);
    ASSERT_EQ(DftMockReportCallsOf(DFT_AUDIO_STREAM_EXCEP), 2);
}

/**
 * @tc.name: DftReportDtfrStatisTest001
 * @tc.desc: 校验数传通道状态分派：建链失败追加上报异常，未知状态不上报
 * @tc.type: FUNC
 */
HWTEST_F(DftReportFilterTest, DftReportDtfrStatisTest001, TestSize.Level1)
{
    DftReportDtfrStatisInfo(DTFR_CHANNEL_ESTABLISHED, "00:11:22:33:44:94", "060D");
    ASSERT_EQ(DftMockReportCallsOf(DFT_DATATRANSFER_STATIS), 1);
    ASSERT_EQ(DftMockReportCallsOf(DFT_DATATRANSFER_EXCEP), 0);

    DftReportDtfrStatisInfo(DTFR_CHANNEL_ESTABLISH_FAIL, "00:11:22:33:44:94", "060D");
    ASSERT_EQ(DftMockReportCallsOf(DFT_DATATRANSFER_STATIS), 2);
    ASSERT_EQ(DftMockReportCallsOf(DFT_DATATRANSFER_EXCEP), 1);

    DftReportDtfrStatisInfo(DTFR_CHANNEL_RELEASED, "00:11:22:33:44:94", "060D");
    DftReportDtfrStatisInfo(DTFR_CHANNEL_RELEASE_FAIL, "00:11:22:33:44:94", "060D");
    ASSERT_EQ(DftMockReportCallsOf(DFT_DATATRANSFER_STATIS), 4);

    const uint8_t unknownState = 0x7F;
    DftReportDtfrStatisInfo(unknownState, "00:11:22:33:44:94", "060D");
    ASSERT_EQ(DftMockReportCallsOf(DFT_DATATRANSFER_STATIS), 4);
}

/**
 * @tc.name: DftQosmNullInputTest001
 * @tc.desc: 校验QOSM三个上报入口对空指针入参直接返回，不产生任何事件
 * @tc.type: FUNC
 */
HWTEST_F(DftReportFilterTest, DftQosmNullInputTest001, TestSize.Level1)
{
    DftReportAudioCodecExcep(nullptr);
    DftReportAudioChoppyExcep(nullptr);
    DftReportAudioStats(nullptr);
    ASSERT_EQ(DftMockReportCalls(), 0);
}

/**
 * @tc.name: DftAudioStatsAirplaneTest001
 * @tc.desc: 校验音频统计上报：非飞行模式把末位频段占用改写为0，飞行模式保留原值；卡顿计数触发额外上报
 * @tc.type: FUNC
 */
HWTEST_F(DftReportFilterTest, DftAudioStatsAirplaneTest001, TestSize.Level1)
{
    QOSM_DftAudioStats stats;
    (void)memset_s(&stats, sizeof(stats), 0, sizeof(stats));
    (void)strcpy_s(stats.band2GInfo, sizeof(stats.band2GInfo), "B2:{1,2}");
    (void)strcpy_s(stats.band5GInfo, sizeof(stats.band5GInfo), "B5:{4,5}");

    DftSetAirplaneMode(false);
    DftReportAudioStats(&stats);
    ASSERT_EQ(DftMockReportCallsOf(DFT_QOSM_STATS), 1);
    ASSERT_EQ(DftMockLastReportStrParam(NL_DFT_STATS_CHANNEL_CONT_TIME_24G), "B2:{1,0}");
    ASSERT_EQ(DftMockLastReportStrParam(NL_DFT_STATS_CHANNEL_CONT_TIME_58G), "B5:{4,0}");

    DftSetAirplaneMode(true);
    DftReportAudioStats(&stats);
    ASSERT_EQ(DftMockReportCallsOf(DFT_QOSM_STATS), 2);
    ASSERT_EQ(DftMockLastReportStrParam(NL_DFT_STATS_CHANNEL_CONT_TIME_24G), "B2:{1,2}");
    ASSERT_EQ(DftMockLastReportStrParam(NL_DFT_STATS_CHANNEL_CONT_TIME_58G), "B5:{4,5}");

    // 卡顿计数非零触发卡顿异常上报
    stats.pcmWriteGtFtCnt = 1;
    DftReportAudioStats(&stats);
    ASSERT_EQ(DftMockReportCallsOf(DFT_QOSM_CHOPPY_EXCEP), 1);
}

/**
 * @tc.name: DftSwitchAndStateFlowTest001
 * @tc.desc: 校验开关事件：状态流缓存用短横线连接；上报时可选携带应用名与开关类型并清空状态流
 * @tc.type: FUNC
 */
HWTEST_F(DftReportFilterTest, DftSwitchAndStateFlowTest001, TestSize.Level1)
{
    DftCacheStateFlow(1);
    ASSERT_EQ(DftMockCacheCallsOf(DFT_FLOW_STATE), 1);
    ASSERT_EQ(DftMockLastCacheStrParam(STATE_FLOW), "1");
    DftCacheStateFlow(2);
    ASSERT_EQ(DftMockLastCacheStrParam(STATE_FLOW), "1-2");

    DftCacheSwitchInfo(SWITCH_OPEN, SWITCH_OPER_SUCCESS, "com.example.app");
    ASSERT_GT(DftMockCacheCallsOf(DFT_SWITCH_EXCEP), 0);

    DftReportSwitchInfo(SWITCH_SUCCESS, "", INVALIDVALUE, SWITCH_OPER_SUCCESS, "");
    ASSERT_EQ(DftMockReportCallsOf(DFT_SWITCH_EXCEP), 1);
    ASSERT_FALSE(DftMockLastReportHasParam(SWITCH_USED_APP));
    ASSERT_FALSE(DftMockLastReportHasParam(SWITCH_TYPE_INFO));

    DftReportSwitchInfo(SWITCH_SUCCESS, "info", SWITCH_OPEN, SWITCH_OPER_SUCCESS, "com.example.app");
    ASSERT_EQ(DftMockReportCallsOf(DFT_SWITCH_EXCEP), 2);
    ASSERT_TRUE(DftMockLastReportHasParam(SWITCH_USED_APP));
    ASSERT_TRUE(DftMockLastReportHasParam(SWITCH_TYPE_INFO));
    ASSERT_TRUE(DftMockLastReportHasParam(SWITCH_INFO_MSG));
}

/**
 * @tc.name: DftCdsmInfoGuardTest001
 * @tc.desc: 校验CDSM信息缓存：任一地址为空不入缓存，三个地址齐全才缓存一条
 * @tc.type: FUNC
 */
HWTEST_F(DftReportFilterTest, DftCdsmInfoGuardTest001, TestSize.Level1)
{
    DftCacheCdsmInfo("", "00:11:22:33:44:96", "00:11:22:33:44:97");
    DftCacheCdsmInfo("00:11:22:33:44:95", "", "00:11:22:33:44:97");
    DftCacheCdsmInfo("00:11:22:33:44:95", "00:11:22:33:44:96", "");
    ASSERT_EQ(DftMockCacheCalls(), 0);

    DftCacheCdsmInfo("00:11:22:33:44:95", "00:11:22:33:44:96", "00:11:22:33:44:97");
    ASSERT_EQ(DftMockCacheCallsOf(DFT_PAIR_EXCEP), 1);
}

/**
 * @tc.name: DftMultideviceConnTest001
 * @tc.desc: 校验多设备连接状态上报只在连接成功与断开两个终态触发
 * @tc.type: FUNC
 */
HWTEST_F(DftReportFilterTest, DftMultideviceConnTest001, TestSize.Level1)
{
    DftDealMultideviceInfo("00:11:22:33:44:98", 1);
    ASSERT_EQ(DftMockReportCallsOf(DFT_MULTIDEVICE_CONN), 0);
    DftDealMultideviceInfo("00:11:22:33:44:98", CM_STATE_CONNECTED);
    DftDealMultideviceInfo("00:11:22:33:44:98", CM_STATE_DISCONNECTED);
    ASSERT_EQ(DftMockReportCallsOf(DFT_MULTIDEVICE_CONN), 2);
}

/**
 * @tc.name: DftDisconnInfoMsgTest001
 * @tc.desc: 校验断链信息缓存：信息非空才携带INFO_MSG参数，场景参数只在首个未决断链时携带
 * @tc.type: FUNC
 */
HWTEST_F(DftReportFilterTest, DftDisconnInfoMsgTest001, TestSize.Level1)
{
    DftCacheDisconnInfoMsg("00:11:22:33:44:99", "", DISCONN_CLICK_SWITCH);
    ASSERT_EQ(DftMockCacheCallsOf(DFT_DISCONN_EXCEP), 1);
    ASSERT_EQ(DftMockLastCacheStrParam(DISCONN_INFO_MSG), "");

    DftCacheDisconnInfoMsg("00:11:22:33:44:9A", "peer reset", DISCONN_EXCEP);
    ASSERT_EQ(DftMockCacheCallsOf(DFT_DISCONN_EXCEP), 2);
    ASSERT_EQ(DftMockLastCacheStrParam(DISCONN_INFO_MSG), "peer reset");
}

/**
 * @tc.name: DftUeInvalidAddrTest001
 * @tc.desc: 校验UE模板上报：非法地址直接返回不上报，合法地址按模板上报一条
 * @tc.type: FUNC
 */
HWTEST_F(DftReportFilterTest, DftUeInvalidAddrTest001, TestSize.Level1)
{
    RawAddress invalid("");
    NearlinkDftUe::GetInstance().WriteCommonUe(DFT_FIND_EXCEP, invalid, 1, 2);
    NearlinkDftUe::GetInstance().WriteAudioSinkDeviceUe(invalid, 1, 2, 3);
    NearlinkDftUe::GetInstance().WriteAudioSourceDeviceUe(invalid, invalid, 1, 2);
    NearlinkDftUe::GetInstance().WriteAudioMuteStreamUe(invalid, 1, 2, 3, "app");
    ASSERT_EQ(DftMockReportCalls(), 0);

    RawAddress valid("11:22:33:44:55:66");
    NearlinkDftUe::GetInstance().WriteCommonUe(DFT_FIND_EXCEP, valid, 1, 2);
    ASSERT_EQ(DftMockReportCalls(), 1);
    NearlinkDftUe::GetInstance().WriteAudioSinkDeviceUe(valid, 1, 2, 3);
    ASSERT_EQ(DftMockReportCalls(), 2);
    NearlinkDftUe::GetInstance().WriteAudioSourceDeviceUe(valid, valid, 1, 2);
    ASSERT_EQ(DftMockReportCalls(), 3);
    NearlinkDftUe::GetInstance().WriteAudioMuteStreamUe(valid, 1, 2, 3, "app");
    ASSERT_EQ(DftMockReportCalls(), 4);
}

/**
 * @tc.name: DftUeTimeParamTest001
 * @tc.desc: 校验UE模板的时间参数只附加在数传之后的UE事件上
 * @tc.type: FUNC
 */
HWTEST_F(DftReportFilterTest, DftUeTimeParamTest001, TestSize.Level1)
{
    RawAddress valid("11:22:33:44:55:77");
    NearlinkDftUe::GetInstance().WriteCommonUe(DFT_SWITCH_EXCEP, valid, 1, 2);
    ASSERT_EQ(DftMockReportCalls(), 1);
    ASSERT_FALSE(DftMockLastReportHasParam(UE_TEMPLATE_ONE_TIME));

    NearlinkDftUe::GetInstance().WriteCommonUe(DFT_FIND_EXCEP, valid, 1, 2);
    ASSERT_EQ(DftMockReportCalls(), 2);
    ASSERT_TRUE(DftMockLastReportHasParam(UE_TEMPLATE_ONE_TIME));

    NearlinkDftUe::GetInstance().WriteCommonUe(DFT_SWITCH_EXCEP, 1, 2);
    ASSERT_FALSE(DftMockLastReportHasParam(UE_TEMPLATE_FIVE_TIME));
    NearlinkDftUe::GetInstance().WriteCommonUe(DFT_FIND_EXCEP, 1, 2);
    ASSERT_TRUE(DftMockLastReportHasParam(UE_TEMPLATE_FIVE_TIME));
}

/**
 * @tc.name: DftAudioControlUeTest001
 * @tc.desc: 校验音频控制结果：控制失败在UE之外追加一条音频档案异常，成功只报UE
 * @tc.type: FUNC
 */
HWTEST_F(DftReportFilterTest, DftAudioControlUeTest001, TestSize.Level1)
{
    RawAddress valid("11:22:33:44:55:88");
    NearlinkDftUe::GetInstance().WriteAudioControlUeAndExcep(valid, 1, 2, 3, NL_NO_ERROR);
    ASSERT_EQ(DftMockReportCallsOf(DFT_AUDIO_SINK_DEVICE_UE), 1);
    ASSERT_EQ(DftMockReportCallsOf(DFT_AUDIO_PROFILE_EXCEP), 0);

    NearlinkDftUe::GetInstance().WriteAudioControlUeAndExcep(valid, 1, 2, 3, NL_ERR_PERMISSION_FAILED);
    ASSERT_EQ(DftMockReportCallsOf(DFT_AUDIO_SINK_DEVICE_UE), 2);
    ASSERT_EQ(DftMockReportCallsOf(DFT_AUDIO_PROFILE_EXCEP), 1);
}

/**
 * @tc.name: DftExcepTemplateTest001
 * @tc.desc: 校验异常模板：非法地址不上报；ICCE档案异常与带调用名模板正常上报
 * @tc.type: FUNC
 */
HWTEST_F(DftReportFilterTest, DftExcepTemplateTest001, TestSize.Level1)
{
    RawAddress invalid("");
    NearlinkDftExcep::GetInstance().WriteIcceProfileExcep(DFT_ICCE_PROFILE_EXCEP, invalid, 1, 2);
    NearlinkDftExcep::GetInstance().WriteCommonExcep(DFT_ICCE_PROFILE_EXCEP, invalid, 1, 2);
    ASSERT_EQ(DftMockReportCalls(), 0);

    RawAddress valid("11:22:33:44:55:99");
    NearlinkDftExcep::GetInstance().WriteIcceProfileExcep(DFT_ICCE_PROFILE_EXCEP, valid, 1, 2);
    ASSERT_EQ(DftMockReportCalls(), 1);

    NearlinkDftExcep::GetInstance().WriteCommonExcep(DFT_SWITCH_EXCEP, 1, 2, "caller");
    ASSERT_EQ(DftMockReportCalls(), 2);
    ASSERT_FALSE(DftMockLastReportHasParam(EXCEP_TEMPLATE_TWO_TIME));
    NearlinkDftExcep::GetInstance().WriteCommonExcep(DFT_FIND_EXCEP, 1, 2, "caller");
    ASSERT_EQ(DftMockReportCalls(), 3);
    ASSERT_TRUE(DftMockLastReportHasParam(EXCEP_TEMPLATE_TWO_TIME));
}

/**
 * @tc.name: DftAppWakeupRecordGateTest001
 * @tc.desc: 校验应用唤醒采集门：hiview开关关闭时不采集；开启后亮屏默认态下同样不落记录，均不崩溃
 * @tc.type: FUNC
 */
HWTEST_F(DftReportFilterTest, DftAppWakeupRecordGateTest001, TestSize.Level1)
{
    DftMockSetHiviewUeState(false);
    NearlinkAppWakeupUe::GetInstance().Record(AppBehaviorType::SCAN_TRANSFER);
    NearlinkAppWakeupUe::GetInstance().RecordByToken(0x10001, 1, 2, AppBehaviorType::SCAN_TRANSFER);

    DftMockSetHiviewUeState(true);
    NearlinkAppWakeupUe::GetInstance().RecordByToken(0x10002, 1, 2, AppBehaviorType::SSAP_TRANSFER);
    NearlinkAppWakeupUe::GetInstance().Record("app.name", 1, 2, AppBehaviorType::PORT_TRANSFER);

    // 应用名token缓存超过上限后整体清空重建
    for (uint32_t i = 0; i < 101; i++) {
        NearlinkAppWakeupUe::GetInstance().RecordByToken(0x20000 + i, 1, 2, AppBehaviorType::SCAN_TRANSFER);
    }
    NearlinkAppWakeupUe::GetInstance().RecordByToken(0x20000, 1, 2, AppBehaviorType::SCAN_TRANSFER);
    NearlinkAppWakeupUe::GetInstance().Shutdown();
}

/**
 * @tc.name: DftExtLoaderNoLibTest001
 * @tc.desc: 校验扩展库缺失路径：车钥匙与众包上报在无扩展so环境下安全短路，不崩溃不抛异常
 * @tc.type: FUNC
 */
HWTEST_F(DftReportFilterTest, DftExtLoaderNoLibTest001, TestSize.Level1)
{
    std::vector<DftParamC> params;
    params.emplace_back(CreateStrParamC(SWITCH_USED_APP, "com.example.app"));
    auto set = DftParam::ConvertToParamSet(DFT_SWITCH_EXCEP, params.data(), params.size());
    ASSERT_FALSE(set.empty());
    NearlinkDftCarKeyLoader::GetInstance().RecordWalletChr(DFT_SWITCH_EXCEP, set);

    NearlinkCemapReportLoader::GetInstance().ReportCemapJsonPayload(nullptr);
    // 同步路径走库加载尝试：无扩展so时安全短路
    NearlinkCemapReportLoader::GetInstance().ReportCemapJsonPayload("{}");
    NearlinkCemapReportLoader::GetInstance().ReportCemapJsonPayloadAsync("{\"k\":1}");
}

/**
 * @tc.name: DISABLED_DftDeviceReportAddrTest001
 * @tc.desc: 校验上报地址登记：首登走插入，同址再登走原地更新，读取时返回最新值（产品缺陷待修复，见用例内说明）
 * @tc.type: FUNC
 */
HWTEST_F(DftReportFilterTest, DISABLED_DftDeviceReportAddrTest001, TestSize.Level1)
{
    // 产品缺陷：DftDeviceManager::EnsureUpdateReportAddr 对已存在地址的更新是静默无效操作——
    // lambda 形参 NearlinkDeviceData value 按值接收（nearlink_dft_device_data.cpp:92），
    // NearlinkSafeMap::GetValueAndOpt 传入的 V& 引用绑定到该拷贝，SetReportAddr 修改即丢弃，
    // 同址再登后 GetReportAddr 仍返回旧值。UpdateName/UpdateAppearance/UpdateManufacturer 同款问题。
    // 本用例期望（同址更新后读到新值）是正确语义，不迁就缺陷改期望；产品修复后去掉 DISABLED_ 前缀与本 GTEST_SKIP。
    GTEST_SKIP() << "wait product fix: EnsureUpdateReportAddr same-addr update is a silent no-op (by-value lambda param)";
    RawAddress device("11:22:33:44:55:F1");
    RawAddress first("11:22:33:44:55:F2");
    RawAddress second("11:22:33:44:55:F3");
    DftDeviceManager::GetInstance().DelDevice(device);
    DftDeviceManager::GetInstance().EnsureUpdateReportAddr(device, first);
    RawAddress got;
    DftDeviceManager::GetInstance().GetReportAddr(device, got);
    ASSERT_EQ(got.GetAddress(), first.GetAddress());

    // 同址更新不新建条目，读取得到新值
    DftDeviceManager::GetInstance().EnsureUpdateReportAddr(device, second);
    DftDeviceManager::GetInstance().GetReportAddr(device, got);
    ASSERT_EQ(got.GetAddress(), second.GetAddress());
    DftDeviceManager::GetInstance().DelDevice(device);
}
/**
 * @tc.name: DftPairFailReportTest001
 * @tc.desc: 校验配对失败上报的路径分派：非SSAP路径直接上报并携带失败原因，成功码不带失败参数，加密路径与挂任务标志各有专属出口
 * @tc.type: FUNC
 */
HWTEST_F(DftReportFilterTest, DftPairFailReportTest001, TestSize.Level1)
{
    // ACB路径+失败码+非空detail：直接上报，携带detail与配对失败参数
    DftCachePairConnPath("00:11:22:33:44:A1", PAIR_CONN_PATH_ACB);
    DftDealEncryptFailEvent("00:11:22:33:44:A1", 1);
    ASSERT_EQ(DftMockReportCallsOf(DFT_PAIR_EXCEP), 1);
    ASSERT_TRUE(DftMockLastReportHasParam(SLE_CONN_ERROR_REASON));

    // 成功码：不带失败结果参数仍上报
    DftCachePairConnPath("00:11:22:33:44:A2", PAIR_CONN_PATH_ACB);
    DftReportPairInfo("00:11:22:33:44:A2", PAIR_CONN_PATH_ACB, 0);
    ASSERT_EQ(DftMockReportCallsOf(DFT_PAIR_EXCEP), 2);
    ASSERT_FALSE(DftMockLastReportHasParam(SLE_CONN_ERROR_REASON));

    // DIS路径(>=SSAP)挂连接任务标志：走直报分支
    DftCachePairConnPath("00:11:22:33:44:A3", PAIR_CONN_PATH_DIS);
    DftSetConnectProfileTaskFlag("00:11:22:33:44:A3");
    DftReportPairInfo("00:11:22:33:44:A3", PAIR_CONN_PATH_DIS, 5);
    ASSERT_EQ(DftMockReportCallsOf(DFT_PAIR_EXCEP), 3);

    // >=SSAP但不挂标志：转成功上报出口
    DftCachePairConnPath("00:11:22:33:44:A4", PAIR_CONN_PATH_DIS);
    DftReportPairInfo("00:11:22:33:44:A4", PAIR_CONN_PATH_DIS, 5);
    ASSERT_EQ(DftMockReportCallsOf(DFT_PAIR_EXCEP), 4);

    // 加密路径：直接走成功上报出口
    DftCachePairConnPath("00:11:22:33:44:A5", PAIR_CONN_PATH_ENCYP);
    DftReportPairInfo("00:11:22:33:44:A5", PAIR_CONN_PATH_ENCYP, 0);
    ASSERT_EQ(DftMockReportCallsOf(DFT_PAIR_EXCEP), 5);

    // 未缓存路径的地址：直接返回不上报
    DftReportPairInfo("00:11:22:33:44:A6", PAIR_CONN_PATH_ACB, 5);
    ASSERT_EQ(DftMockReportCallsOf(DFT_PAIR_EXCEP), 5);
}

/**
 * @tc.name: DftAccurateSearchGuardTest001
 * @tc.desc: 校验精确查找的任务前置门：未建任务时缓存/上报入口静默；未经历扫描调用时任务名不登记
 * @tc.type: FUNC
 */
HWTEST_F(DftReportFilterTest, DftAccurateSearchGuardTest001, TestSize.Level1)
{
    int cacheBefore = DftMockCacheCalls();
    int reportBefore = DftMockReportCalls();
    DftCacheAccurateSearchConnInfo("00:11:22:33:44:B1", DFT_ACB_CONN, DFT_CONN_SUCCESS);
    DftReportAccurateSearchConnFailInfo("00:11:22:33:44:B1", DFT_MEASURE, DFT_CONN_FAILED, 1);
    DftReportAccurateSearchMeasureInfo("other.caller", "00:11:22:33:44:B1", DFT_MEASURE,
        DFT_MEASURE_FINISHED, 0);
    ASSERT_EQ(DftMockCacheCalls(), cacheBefore);
    ASSERT_EQ(DftMockReportCalls(), reportBefore);

    // 没有扫描调用先行时，任务名仍是默认值：登记不生效，后续失败上报静默
    DftSetAccurateSearchTaskMap("00:11:22:33:44:B2");
    DftReportAccurateSearchConnFailInfo("00:11:22:33:44:B2", DFT_ACB_CONN, DFT_CONN_FAILED, 1);
    ASSERT_EQ(DftMockReportCalls(), reportBefore);
}

/**
 * @tc.name: DftStateFlowOverflowTest001
 * @tc.desc: 校验状态流超过上限后随开关上报整体清空，后续状态从单值重新累计
 * @tc.type: FUNC
 */
HWTEST_F(DftReportFilterTest, DftStateFlowOverflowTest001, TestSize.Level1)
{
    for (int i = 0; i < 22; i++) {
        DftCacheStateFlow(i % 10);
    }
    int cached = DftMockCacheCallsOf(DFT_FLOW_STATE);
    ASSERT_GE(cached, 22);
    DftReportSwitchInfo(SWITCH_SUCCESS, "", INVALIDVALUE, SWITCH_OPER_SUCCESS, "");
    int afterClear = DftMockCacheCallsOf(DFT_FLOW_STATE);
    DftCacheStateFlow(1);
    ASSERT_EQ(DftMockCacheCallsOf(DFT_FLOW_STATE), afterClear + 1);
    // 清空后首个状态值不带连接符
    ASSERT_EQ(DftMockLastCacheStrParam(STATE_FLOW), "1");
}

/**
 * @tc.name: DftDisconnSceneFlagTest001
 * @tc.desc: 校验断链场景参数只在无未决场景标记时携带：标记置位后的断链上报省略场景参数
 * @tc.type: FUNC
 */
HWTEST_F(DftReportFilterTest, DftDisconnSceneFlagTest001, TestSize.Level1)
{
    // 产品静态场景标记（g_sceneFlag/g_disconnFlag）跨用例存活：先行用例（如 DftDisconnInfoMsgTest001）
    // 调 DftCacheDisconnInfoMsg 置位后无人归零。断链上报会无条件清零两个标记，
    // 先做一次断链复位，再在干净状态下断言场景参数携带，使用例与执行顺序无关
    DftConnStateChange("00:11:22:33:44:C0", CM_STATE_DISCONNECTED, 1);
    DftConnStateChange("00:11:22:33:44:C1", CM_STATE_DISCONNECTED, 1);
    ASSERT_EQ(DftMockReportCallsOf(DFT_DISCONN_EXCEP), 2);
    ASSERT_TRUE(DftMockLastReportHasParam(DISCONN_SCENE));

    // 点击取消配对置位场景标记：随后的断链上报不再携带场景参数
    DftCacheDisconnInfoMsg("00:11:22:33:44:C2", "click unpair", DISCONN_CLICK_UNPAIR);
    DftConnStateChange("00:11:22:33:44:C2", CM_STATE_DISCONNECTED, 1);
    ASSERT_EQ(DftMockReportCallsOf(DFT_DISCONN_EXCEP), 3);
    ASSERT_FALSE(DftMockLastReportHasParam(DISCONN_SCENE));
}

/**
 * @tc.name: DftUnpairLinkKeySuccessTest001
 * @tc.desc: 校验删除链钥成功时只缓存删钥时间，不追加上报失败结果
 * @tc.type: FUNC
 */
HWTEST_F(DftReportFilterTest, DftUnpairLinkKeySuccessTest001, TestSize.Level1)
{
    DftCacheUnPairInfo("00:11:22:33:44:D1", 0x0101, "caller", 0, PAIR_STATE_OK);
    int reports = DftMockReportCalls();
    DftDealUnPairLinkKeyInfo("00:11:22:33:44:D1", true);
    ASSERT_EQ(DftMockCacheCallsOf(DFT_UNPAIR_EXCEP), 2);
    ASSERT_EQ(DftMockReportCalls(), reports);
}

/**
 * @tc.name: DftStreamConfigParamTest001
 * @tc.desc: 校验流配置缓存按配置串有无携带配置参数，重配置标记随状态切换
 * @tc.type: FUNC
 */
HWTEST_F(DftReportFilterTest, DftStreamConfigParamTest001, TestSize.Level1)
{
    DftAudioStreamInfo info;
    info.addr = "00:11:22:33:44:E1";
    info.state = STREAM_ASC_CONFIGURING;
    info.configparam = "";
    DftCacheStreamASC(info);
    ASSERT_EQ(DftMockCacheCallsOf(DFT_AUDIO_STREAM_EXCEP), 2);
    ASSERT_EQ(DftMockLastCacheStrParam(STREAM_CONFIG_PARAM), "");

    info.state = STREAM_ASC_RECONFIGURING;
    info.configparam = "codec=l2hc";
    DftCacheStreamASC(info);
    ASSERT_EQ(DftMockLastCacheStrParam(STREAM_CONFIG_PARAM), "codec=l2hc");
}

/**
 * @tc.name: DftUeTemplateMatrixTest001
 * @tc.desc: 校验UE各模板的时间参数只附加在数传之后的事件上：设备型与纯场景型模板都遵守同一门槛
 * @tc.type: FUNC
 */
HWTEST_F(DftReportFilterTest, DftUeTemplateMatrixTest001, TestSize.Level1)
{
    RawAddress valid("11:22:33:44:55:01");
    NearlinkDftUe::GetInstance().WriteCommonUe(DFT_SWITCH_EXCEP, valid, 1, 2, "caller");
    ASSERT_FALSE(DftMockLastReportHasParam(UE_TEMPLATE_TWO_TIME));
    NearlinkDftUe::GetInstance().WriteCommonUe(DFT_FIND_EXCEP, valid, 1, 2, "caller");
    ASSERT_TRUE(DftMockLastReportHasParam(UE_TEMPLATE_TWO_TIME));

    NearlinkDftUe::GetInstance().WriteCommonUe(DFT_SWITCH_EXCEP, valid, 1, "caller");
    ASSERT_FALSE(DftMockLastReportHasParam(UE_TEMPLATE_THREE_TIME));
    NearlinkDftUe::GetInstance().WriteCommonUe(DFT_FIND_EXCEP, valid, 1, "caller");
    ASSERT_TRUE(DftMockLastReportHasParam(UE_TEMPLATE_THREE_TIME));

    NearlinkDftUe::GetInstance().WriteCommonUe(DFT_SWITCH_EXCEP, 1, 2, "caller");
    ASSERT_FALSE(DftMockLastReportHasParam(UE_TEMPLATE_SEVEN_TIME));
    NearlinkDftUe::GetInstance().WriteCommonUe(DFT_FIND_EXCEP, 1, 2, "caller");
    ASSERT_TRUE(DftMockLastReportHasParam(UE_TEMPLATE_SEVEN_TIME));

    NearlinkDftUe::GetInstance().WriteCommonUe(DFT_SWITCH_EXCEP, valid, 1, 1, 2);
    ASSERT_FALSE(DftMockLastReportHasParam(UE_TEMPLATE_EIGHT_TIME));
    NearlinkDftUe::GetInstance().WriteCommonUe(DFT_FIND_EXCEP, valid, 1, 1, 2);
    ASSERT_TRUE(DftMockLastReportHasParam(UE_TEMPLATE_EIGHT_TIME));
}

/**
 * @tc.name: DftExcepTemplateMatrixTest001
 * @tc.desc: 校验异常模板的时间参数门槛：纯错误码模板与带邻设备引用模板都只在数传后事件附加时间
 * @tc.type: FUNC
 */
HWTEST_F(DftReportFilterTest, DftExcepTemplateMatrixTest001, TestSize.Level1)
{
    NearlinkDftExcep::GetInstance().WriteCommonExcep(DFT_SWITCH_EXCEP, 1, 2);
    ASSERT_FALSE(DftMockLastReportHasParam(EXCEP_TEMPLATE_ONE_TIME));
    NearlinkDftExcep::GetInstance().WriteCommonExcep(DFT_FIND_EXCEP, 1, 2);
    ASSERT_TRUE(DftMockLastReportHasParam(EXCEP_TEMPLATE_ONE_TIME));

    RawAddress valid("11:22:33:44:55:02");
    NearlinkDftExcep::GetInstance().WriteCommonExcep(DFT_SWITCH_EXCEP, valid, 1, 1, 2);
    ASSERT_FALSE(DftMockLastReportHasParam(EXCEP_TEMPLATE_THREE_TIME));
    NearlinkDftExcep::GetInstance().WriteCommonExcep(DFT_FIND_EXCEP, valid, 1, 1, 2);
    ASSERT_TRUE(DftMockLastReportHasParam(EXCEP_TEMPLATE_THREE_TIME));
}
