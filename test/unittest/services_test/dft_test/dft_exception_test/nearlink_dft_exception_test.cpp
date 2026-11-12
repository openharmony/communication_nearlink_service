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
#include "nearlink_dft_utils.h"
#include "nearlink_dft_exception.h"
#include "nearlink_dft_exception_c.h"

// 调用计数访问口，定义在 nearlink_dft_manager_c_mocker.cpp
extern "C" void DftMockerRecordReset(void);
extern "C" int DftMockerCacheCalls(void);
extern "C" int DftMockerReportCalls(void);

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

const std::string DEVICE_ADDR = "11:22:33:44:55:66";
const std::string DEVICE_NAME = "EARPHONE";
const std::string ACCURATE_DEVICE_NAME = "findnetwork";
constexpr const int32_t DEVICE_INT32_INVALID = 0;
constexpr const uint32_t DEVICE_UINT32_INVALID = 0;
constexpr const uint16_t DEVICE_UINT16_INVALID = 0;
constexpr const int32_t DEVICE_PAIRED_STATE = 3;
constexpr const uint32_t DEVICE_CONN_DISCONNECTED = 2;
constexpr const int32_t DEVICE_CONN_TIMEOUT_REASON = 0x07;
constexpr const uint16_t DEVICE_INVALID_LCID = 0xFFFF;

class NearlinkDftExceptionTest : public testing::Test {
public:
    NearlinkDftExceptionTest() = default;
    ~NearlinkDftExceptionTest() = default;

    static void SetUpTestCase()
    {}
    static void TearDownTestCase()
    {}

    void SetUp()
    {
        // 每例从零计数，隔离前序用例对计数器的污染
        DftMockerRecordReset();
    };
    void TearDown(){};
};

/**
 * @tc.name: NearlinkDftExceptionTest001
 * @tc.desc: DFT_PAIR_EXCEP
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkDftExceptionTest, DFT_PAIR_EXCEP_TEST, TestSize.Level1)
{
    int32_t DEVICE_INT32_INVALID = 0;
    HILOGI("NearlinkDftExceptionTest: DFT_PAIR_EXCEP_TEST start");
    DftSetConnectProfileTaskFlag(DEVICE_ADDR);
    DftCacheBgStartConn(DEVICE_ADDR, DEVICE_INT32_INVALID);
    DftCachePairConnTime(DEVICE_ADDR, PAIR_CONN_PATH_SSAP, SLE_ACB_START_TIME);
    DftCacheAcbFinishConn(DEVICE_ADDR, DEVICE_UINT32_INVALID, DEVICE_INT32_INVALID, DEVICE_UINT32_INVALID,
        DEVICE_UINT16_INVALID);
    DftCachePeerInfoTime(DEVICE_ADDR, PEER_INFO_INVALID);
    DftCachePeerInfo(DEVICE_ADDR, DEVICE_NAME, DEVICE_INT32_INVALID);
    DftCacheDisconChipInfo(DEVICE_ADDR, DEVICE_INT32_INVALID, DEVICE_NAME);
    DftCacheCallingName(DEVICE_ADDR, DEVICE_NAME);
    DftCacheCdsmInfo(DEVICE_ADDR, DEVICE_ADDR, DEVICE_ADDR);
    DftReportPairSuccess(DEVICE_ADDR, DEVICE_UINT16_INVALID);
    DftDealEncryptFailEvent(DEVICE_ADDR, DEVICE_INT32_INVALID);
    HILOGI("NearlinkDftExceptionTest: DFT_PAIR_EXCEP_TEST end");
}

/**
 * @tc.name: NearlinkDftExceptionTest002
 * @tc.desc: DFT_SWITCH_EXCEP
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkDftExceptionTest, DFT_SWITCH_EXCEP_TEST, TestSize.Level1)
{
    HILOGI("NearlinkDftExceptionTest: DFT_SWITCH_EXCEP_TEST start");
    DftCacheSwitchInfo(INVALIDVALUE, SWITCH_OPER_SUCCESS, DEVICE_ADDR);
    DftCacheStateFlow(DEVICE_INT32_INVALID);
    DftReportSwitchInfo(SWITCH_SUCCESS, DEVICE_ADDR, INVALIDVALUE, SWITCH_OPER_SUCCESS, DEVICE_ADDR);
    HILOGI("NearlinkDftExceptionTest: DFT_SWITCH_EXCEP_TEST end");
}

/**
 * @tc.name: NearlinkDftExceptionTest003
 * @tc.desc: DFT_PEER_INFO & DFT_DISCONN_EXCEP
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkDftExceptionTest, DFT_PEER_INFO_TEST, TestSize.Level1)
{
    HILOGI("NearlinkDftExceptionTest: DFT_PEER_INFO_TEST start");
    DftCachePeerManufacturer(DEVICE_ADDR, DEVICE_ADDR);
    DftCacheHidState(DEVICE_ADDR, DEVICE_INT32_INVALID);
    DftCacheDisconnRssi(DEVICE_ADDR, DEVICE_INT32_INVALID);
    DftCacheDisconnInfoMsg(DEVICE_ADDR, DEVICE_ADDR, DEVICE_UINT16_INVALID);
    uint8_t state = 2;
    DftConnStateChange(DEVICE_ADDR, state, DEVICE_UINT32_INVALID);
    HILOGI("NearlinkDftExceptionTest: DFT_PEER_INFO_TEST end");
}

/**
 * @tc.name: NearlinkDftExceptionTest004
 * @tc.desc: DFT_ACCURATESEARCH
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkDftExceptionTest, DFT_ACCURATESEARCH_TEST, TestSize.Level1)
{
    HILOGI("NearlinkDftExceptionTest: DFT_ACCURATESEARCH_TEST start");
    DftDealAccurateSearchScanInfo(ACCURATE_DEVICE_NAME, DFT_SCAN);
    DftSetAccurateSearchTaskMap(DEVICE_ADDR);
    DftSetMeasureResultMap(DEVICE_ADDR, true);
    DftDealAccurateSearchMeasureInfo(DEVICE_ADDR, DEVICE_ADDR);
    DftCacheRssiValueInfo(DEVICE_ADDR, DEVICE_INT32_INVALID);
    DftCacheDisconChipInfo(DEVICE_ADDR, DEVICE_INT32_INVALID, DEVICE_ADDR);
    DftCacheAccurateSearchConnInfo(DEVICE_ADDR, DFT_SCAN, DFT_CONN_SUCCESS);
    DftCacheAccurateSearchMeasureInfo(ACCURATE_DEVICE_NAME, DEVICE_ADDR, DFT_SCAN, DFT_MEASURE_SUCCESS);
    DftDealAccurateSearchMeasureInfo(ACCURATE_DEVICE_NAME, DEVICE_ADDR);
    DftReportAccurateSearchConnFailInfo(DEVICE_ADDR, DFT_SCAN, DFT_CONN_SUCCESS, DEVICE_INT32_INVALID);
    HILOGI("NearlinkDftExceptionTest: DFT_ACCURATESEARCH_TEST end");
}

/**
 * @tc.name: NearlinkDftExceptionTest005
 * @tc.desc: DFT_AUDIO_PROFILE_EXCEP & DFT_UNPAIR_EXCEP
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkDftExceptionTest, DFT_AUDIO_PROFILE_EXCEP_TEST, TestSize.Level1)
{
    HILOGI("NearlinkDftExceptionTest: DFT_AUDIO_PROFILE_EXCEP_TEST start");
    DftReportAudioError(DEVICE_ADDR, DEVICE_UINT16_INVALID, DEVICE_UINT16_INVALID, DEVICE_ADDR);
    DftCacheUnPairInfo(DEVICE_ADDR, DEVICE_INT32_INVALID, DEVICE_ADDR, DEVICE_INT32_INVALID, DEVICE_PAIRED_STATE);
    DftDealUnPairLinkKeyInfo(DEVICE_ADDR, false);
    DftCacheUnPairInfo(DEVICE_ADDR, DEVICE_INT32_INVALID, DEVICE_ADDR, DEVICE_INT32_INVALID, DEVICE_PAIRED_STATE);
    DftReportUnPairSuccessInfo(DEVICE_ADDR, DEVICE_INT32_INVALID, DEVICE_UINT16_INVALID);
    DftCacheMultideviceInfo(DEVICE_ADDR, DEVICE_ADDR, DEVICE_UINT16_INVALID);
    HILOGI("NearlinkDftExceptionTest: DFT_AUDIO_PROFILE_EXCEP_TEST end");
}

/**
 * @tc.name: NearlinkDftExceptionTest006
 * @tc.desc: DFT_DATATRANSFER_STATIS & DFT_DATATRANSFER_EXCEP & DFT_AUDIO_HEADSET_EXCEP
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkDftExceptionTest, DFT_DATATRANSFER_STATIS_TEST, TestSize.Level1)
{
    HILOGI("NearlinkDftExceptionTest: DFT_DATATRANSFER_STATIS_TEST start");
    DftReportDtfrStatisPortInfo(DEVICE_ADDR, DEVICE_INT32_INVALID);
    DftReportDtfrStatisInfo(DTFR_CHANNEL_ESTABLISH_FAIL, DEVICE_ADDR, DEVICE_ADDR);
    DftAudioExceptionData dftAudioExcepData;
    RawAddress addr;
    DftHandleAudioExcep(addr, dftAudioExcepData);
    HILOGI("NearlinkDftExceptionTest: DFT_DATATRANSFER_STATIS_TEST end");
}

/**
 * @tc.name: NearlinkDftExceptionTest007
 * @tc.desc: DFT_AUDIO_STREAM_EXCEP & DFT_QOSM_CODEC_EXCEP
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkDftExceptionTest, DFT_AUDIO_STREAM_EXCEP_TEST, TestSize.Level1)
{
    HILOGI("NearlinkDftExceptionTest: DFT_AUDIO_STREAM_EXCEP_TEST start");
    DftCacheStreamTime(DEVICE_ADDR, DEVICE_INT32_INVALID);
    DftAudioStreamInfo info;
    info.addr = DEVICE_ADDR;
    info.state = STREAM_ASC_RECONFIGURING;
    DftCacheStreamASC(info);
    info.state = STREAM_ASC_STOPPING;
    DftCacheStreamASC(info);
    DftReportStreamASC(DEVICE_ADDR, DEVICE_INT32_INVALID, DTFR_CHANNEL_ESTABLISHED, DEVICE_INT32_INVALID);
    DftReportStreamASC(DEVICE_ADDR, DEVICE_INT32_INVALID, DTFR_CHANNEL_ESTABLISH_FAIL, DEVICE_INT32_INVALID);
    QOSM_DftAudioStats stats;
    stats.pcmWriteGtFtCnt = 1;
    DftReportAudioStats(&stats);
    QOSM_DftAudioCodecExcep excep;
    DftReportAudioCodecExcep(&excep);
    HILOGI("NearlinkDftExceptionTest: DFT_AUDIO_STREAM_EXCEP_TEST end");
}

/**
 * @tc.name: NearlinkDftExceptionTest008
 * @tc.desc: DFT_DISCONN_EXCEP 建链超时失败(0x07)限频上报
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkDftExceptionTest, DFT_DISCONN_EXCEP_TIMEOUT_TEST, TestSize.Level1)
{
    HILOGI("NearlinkDftExceptionTest: DFT_DISCONN_EXCEP_TIMEOUT_TEST start");
    DftCacheAcbFinishConn(DEVICE_ADDR, DEVICE_CONN_DISCONNECTED, DEVICE_CONN_TIMEOUT_REASON, DEVICE_UINT32_INVALID,
        DEVICE_INVALID_LCID);
    DftCacheAcbFinishConn(DEVICE_ADDR, DEVICE_CONN_DISCONNECTED, DEVICE_CONN_TIMEOUT_REASON, DEVICE_UINT32_INVALID,
        DEVICE_INVALID_LCID);
    DftCacheAcbFinishConn(DEVICE_ADDR, DEVICE_CONN_DISCONNECTED, DEVICE_INT32_INVALID, DEVICE_UINT32_INVALID,
        DEVICE_INVALID_LCID);
    HILOGI("NearlinkDftExceptionTest: DFT_DISCONN_EXCEP_TIMEOUT_TEST end");
}

// 复位配对全局状态：先登记使链路可命中，再经加密失败事件的上报链抹除两张缓存表并复位配对标志位
static void ResetPairGlobalsByAddr(const std::string &addr)
{
    DftCachePairConnPath(addr, PAIR_CONN_PATH_ENCYP);
    DftDealEncryptFailEvent(addr, 0);
}

/**
 * @tc.name: NearlinkDftExceptionTest009
 * @tc.desc: 校验 IsValidAddressWithType 接受各类合法地址：全小写、全大写、全数字十六进制，
 *   以及覆盖全部区间端点字符的单串
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkDftExceptionTest, DFT_ADDR_TYPE_VALID_TEST, TestSize.Level1)
{
    HILOGI("NearlinkDftExceptionTest: DFT_ADDR_TYPE_VALID_TEST start");
    // 长度恰为 20；仅校验前 17 位地址，3 位类型后缀不参与
    EXPECT_TRUE(IsValidAddressWithType("aa:bb:cc:dd:ee:ff001")); // 全小写 hex
    EXPECT_TRUE(IsValidAddressWithType("AA:BB:CC:DD:EE:FF001")); // 全大写 hex
    EXPECT_TRUE(IsValidAddressWithType("01:23:45:67:89:56xyz")); // 全数字 + 任意后缀
    // 单串覆盖六个区间端点字符
    EXPECT_TRUE(IsValidAddressWithType("0f:9a:AF:fa:90:F0555"));
    HILOGI("NearlinkDftExceptionTest: DFT_ADDR_TYPE_VALID_TEST end");
}

/**
 * @tc.name: NearlinkDftExceptionTest010
 * @tc.desc: 校验 IsValidAddressWithType 拒绝各类非法地址：空串、长度差一、
 *   十六进制位出现非十六进制字符、冒号分隔位出现其他字符
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkDftExceptionTest, DFT_ADDR_TYPE_INVALID_TEST, TestSize.Level1)
{
    HILOGI("NearlinkDftExceptionTest: DFT_ADDR_TYPE_INVALID_TEST start");
    EXPECT_FALSE(IsValidAddressWithType(""));                      // 空串
    EXPECT_FALSE(IsValidAddressWithType("aa:bb:cc:dd:ee:ff00"));   // 长度 19（差 1）
    EXPECT_FALSE(IsValidAddressWithType("aa:bb:cc:dd:ee:ff0000")); // 长度 21（多 1）
    EXPECT_FALSE(IsValidAddressWithType("gg:bb:cc:dd:ee:ff001"));  // 'g' 超出十六进制区间
    EXPECT_FALSE(IsValidAddressWithType("GG:bb:cc:dd:ee:ff001"));  // 'G' 夹在大写与小写区间之间
    EXPECT_FALSE(IsValidAddressWithType("!!:bb:cc:dd:ee:ff001"));  // '!' 不是十六进制字符
    EXPECT_FALSE(IsValidAddressWithType("::bb:cc:dd:ee:ff001"));   // ':' 不是十六进制字符
    EXPECT_FALSE(IsValidAddressWithType("aaXbb:cc:dd:ee:ff001"));  // 分隔位出现非冒号字符
    HILOGI("NearlinkDftExceptionTest: DFT_ADDR_TYPE_INVALID_TEST end");
}

/**
 * @tc.name: NearlinkDftExceptionTest011
 * @tc.desc: 校验 DftCachePairConnTime 的时长类型取值域：越界类型直接忽略、两个边界类型正常处理；
 *   未登记连接路径的地址不产生缓存写入
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkDftExceptionTest, DFT_PAIR_TIME_RANGE_TEST, TestSize.Level1)
{
    const std::string rangeAddr = "11:22:33:44:55:71";
    const std::string missAddr = "11:22:33:44:55:72";
    HILOGI("NearlinkDftExceptionTest: DFT_PAIR_TIME_RANGE_TEST start");
    // 越界类型直接返回，不写任何缓存
    DftMockerRecordReset();
    DftCachePairConnTime(rangeAddr, PAIR_CONN_PATH_SSAP, PAIR_INVALID);
    DftCachePairConnTime(rangeAddr, PAIR_CONN_PATH_SSAP, SLE_CONN_ERROR_CODE);
    EXPECT_EQ(DftMockerCacheCalls(), 0); // 守卫两次早退：无任何 Cache 副作用
    EXPECT_EQ(DftMockerReportCalls(), 0);
    // 起点边界类型：跳过缓存查询，成功链首条件不成立，落入尾部缓存路径
    DftMockerRecordReset();
    DftCachePairConnTime(rangeAddr, PAIR_CONN_PATH_ACB, SLE_ACB_START_TIME);
    EXPECT_EQ(DftMockerCacheCalls(), 2); // 尾部路径两次写入：路径重登记与配对时长各一次
    EXPECT_EQ(DftMockerReportCalls(), 0);
    // 终点边界类型：触发缓存查询，未登记地址未命中直接返回
    DftMockerRecordReset();
    DftCachePairConnTime(missAddr, PAIR_CONN_PATH_SSAP, SLE_PROFILE_FINISH_TIME);
    EXPECT_EQ(DftMockerCacheCalls(), 0); // 未命中早退，无缓存副作用
    ResetPairGlobalsByAddr(rangeAddr);
    HILOGI("NearlinkDftExceptionTest: DFT_PAIR_TIME_RANGE_TEST end");
}

/**
 * @tc.name: NearlinkDftExceptionTest012
 * @tc.desc: 校验 DftCachePairConnTime 的缓存查询与成功链短路：收尾边界类型跳过查询；
 *   已登记路径命中后，非 ACB 路径不触发成功上报，落入缓存路径
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkDftExceptionTest, DFT_PAIR_CONN_TIME_SHORT_CIRCUIT_TEST, TestSize.Level1)
{
    const std::string acbFinishAddr = "11:22:33:44:55:73";
    const std::string hitAddr = "11:22:33:44:55:74";
    HILOGI("NearlinkDftExceptionTest: DFT_PAIR_CONN_TIME_SHORT_CIRCUIT_TEST start");
    // 收尾边界类型：跳过缓存查询，成功链第二条件不成立
    DftMockerRecordReset();
    DftCachePairConnTime(acbFinishAddr, PAIR_CONN_PATH_SSAP, SLE_ACB_FINISH_TIME);
    EXPECT_EQ(DftMockerCacheCalls(), 2); // 尾部路径两次写入：路径重登记与配对时长各一次
    // 预置路径 AUTH：缓存命中但非 ACB，落入尾部缓存路径
    DftCachePairConnPath(hitAddr, PAIR_CONN_PATH_AUTH);
    DftMockerRecordReset();
    DftCachePairConnTime(hitAddr, PAIR_CONN_PATH_SSAP, SLE_SSAP_START_TIME);
    EXPECT_EQ(DftMockerCacheCalls(), 2); // 命中但路径非 ACB，尾部路径两次写入
    EXPECT_EQ(DftMockerReportCalls(), 0); // 成功链未触发：无 Report
    ResetPairGlobalsByAddr(acbFinishAddr);
    ResetPairGlobalsByAddr(hitAddr);
    HILOGI("NearlinkDftExceptionTest: DFT_PAIR_CONN_TIME_SHORT_CIRCUIT_TEST end");
}

/**
 * @tc.name: NearlinkDftExceptionTest013
 * @tc.desc: 校验 DftCachePairConnTime 的成功链：登记路径为 ACB 且上报 SSAP 起点时上报配对成功、
 *   清除缓存并复位标志位；随后同参数调用因缓存已清空直接返回
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkDftExceptionTest, DFT_PAIR_CONN_TIME_SSAP_SUCCESS_TEST, TestSize.Level1)
{
    const std::string ssapAddr = "11:22:33:44:55:75";
    const std::string pairResultAddr = "11:22:33:44:55:76";
    HILOGI("NearlinkDftExceptionTest: DFT_PAIR_CONN_TIME_SSAP_SUCCESS_TEST start");
    // 以非配对连接类型置位配对标志
    DftCachePairConnType(ssapAddr, NO_PAIR, BG_CONN);
    // 预置路径 ACB：三条件全真，上报成功后清除两张表并复位标志位
    DftCachePairConnPath(ssapAddr, PAIR_CONN_PATH_ACB);
    DftMockerRecordReset();
    DftCachePairConnTime(ssapAddr, PAIR_CONN_PATH_SSAP, SLE_SSAP_START_TIME);
    // 成功链唯一走向：恰一次上报，且不再写入缓存
    EXPECT_EQ(DftMockerReportCalls(), 1);
    EXPECT_EQ(DftMockerCacheCalls(), 0);
    // 缓存已被成功链清除，同参数再调直接返回
    DftMockerRecordReset();
    DftCachePairConnTime(ssapAddr, PAIR_CONN_PATH_SSAP, SLE_SSAP_START_TIME);
    EXPECT_EQ(DftMockerCacheCalls(), 0); // 未命中早退，无副作用
    EXPECT_EQ(DftMockerReportCalls(), 0);
    // 标志位已复位：命中缓存但路径非 ACB，落入尾部缓存路径
    DftCachePairConnPath(pairResultAddr, PAIR_CONN_PATH_DISCOVER);
    DftMockerRecordReset();
    DftCachePairConnTime(pairResultAddr, PAIR_CONN_PATH_SSAP, SLE_SSAP_START_TIME);
    EXPECT_EQ(DftMockerCacheCalls(), 2); // 第三条件不成立，尾部路径两次写入
    EXPECT_EQ(DftMockerReportCalls(), 0);
    ResetPairGlobalsByAddr(ssapAddr);
    ResetPairGlobalsByAddr(pairResultAddr);
    HILOGI("NearlinkDftExceptionTest: DFT_PAIR_CONN_TIME_SSAP_SUCCESS_TEST end");
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS