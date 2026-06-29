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

#include <gtest/gtest.h>
#include "nearlink_dft_utils.h"
#include "nearlink_dft_exception.h"
#include "nearlink_dft_exception_c.h"

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

class NearlinkDftExceptionTest : public testing::Test {
public:
    NearlinkDftExceptionTest() = default;
    ~NearlinkDftExceptionTest() = default;

    static void SetUpTestCase()
    {}
    static void TearDownTestCase()
    {}

    void SetUp(){};
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

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS