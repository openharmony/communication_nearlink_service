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

#define LOG_TAG "nearlink_test"  // single definition (target defines clashes with product hibox .c)
#include <gtest/gtest.h>
#include <cstring>

extern "C" {
#include "hibox_audio.h"
#include "hibox_dts.h"
#include "hibox_process.h"
#include "hibox_local.h"
}

using namespace testing::ext;

namespace {
// 解析出参背板：HiboxParseMsgInd 的 data 是零长数组，必须跟一块真实存储
struct ParseMsgBuf {
    HiboxParseMsgInd ind;
    uint8_t bytes[320];
};

void ClearMsg(ParseMsgBuf &buf)
{
    (void)memset_s(&buf, sizeof(buf), 0, sizeof(buf));
}

uint16_t MakeTlv(uint8_t *buf, uint8_t type, uint16_t len, const uint8_t *value)
{
    buf[0] = type;
    buf[1] = static_cast<uint8_t>(len & 0xFF);
    buf[2] = static_cast<uint8_t>(len >> 8);
    if (len > 0 && value != nullptr) {
        (void)memcpy_s(buf + HIBOX_TLV_HEADER_LEN, len, value, len);
    }
    return HIBOX_TLV_HEADER_LEN + len;
}
} // namespace

class HiboxResidualTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}
};

/**
 * @tc.name: HiboxResidualDtsWearTest001
 * @tc.desc: 校验右耳与自由佩戴TLV长度不符时整轮解析作废：字段保持清零，datalen仍按满结构体填写
 * @tc.type: FUNC
 */
HWTEST_F(HiboxResidualTest, HiboxResidualDtsWearTest001, TestSize.Level1)
{
    uint8_t buf[16];
    ParseMsgBuf msg;
    const uint8_t two[2] = { 0x01, 0x01 };

    ClearMsg(msg);
    uint16_t off = MakeTlv(buf, RIGHT_WEAR_TYPE, sizeof(two), two);
    ASSERT_TRUE(HiboxDtsMgmtReqParse(CMDID_WEAR_STATUS, buf, off, reinterpret_cast<uint8_t *>(&msg)));
    DtsWearData *wear = reinterpret_cast<DtsWearData *>(msg.bytes);
    ASSERT_EQ(wear->rightWear, 0);
    ASSERT_EQ(msg.ind.datalen, sizeof(DtsWearData));

    ClearMsg(msg);
    off = MakeTlv(buf, FREEMAN_WEAR_TYPE, sizeof(two), two);
    ASSERT_TRUE(HiboxDtsMgmtReqParse(CMDID_WEAR_STATUS, buf, off, reinterpret_cast<uint8_t *>(&msg)));
    wear = reinterpret_cast<DtsWearData *>(msg.bytes);
    ASSERT_EQ(wear->freemanWear, 0);
    ASSERT_EQ(msg.ind.datalen, sizeof(DtsWearData));
}

/**
 * @tc.name: HiboxResidualDtsAtTest002
 * @tc.desc: 校验AT串单条TLV声明值长超上限（256字节）被拒：datalen按串长下限填写，类型标记仍保留
 * @tc.type: FUNC
 */
HWTEST_F(HiboxResidualTest, HiboxResidualDtsAtTest002, TestSize.Level1)
{
    uint8_t buf[HIBOX_TLV_HEADER_LEN + DTS_DATA_MAX_LEN + 1];
    const uint8_t overlong[DTS_DATA_MAX_LEN + 1] = { 0x31 };

    ParseMsgBuf msg;
    ClearMsg(msg);
    uint16_t off = MakeTlv(buf, BATTERY_REPORT_TYPE, sizeof(overlong), overlong);
    ASSERT_TRUE(HiboxDtsMgmtReqParse(CMDID_AT_REPORT, buf, off, reinterpret_cast<uint8_t *>(&msg)));
    AtStrData *at = reinterpret_cast<AtStrData *>(msg.bytes);
    ASSERT_EQ(at->dtsType, BATTERY_REPORT);
    ASSERT_EQ(msg.ind.datalen, 1);
    ASSERT_EQ(at->atString[0], 0);

    ClearMsg(msg);
    off = MakeTlv(buf, DEVICE_INFO_REPORT_TYPE, sizeof(overlong), overlong);
    ASSERT_TRUE(HiboxDtsMgmtReqParse(CMDID_AT_REPORT, buf, off, reinterpret_cast<uint8_t *>(&msg)));
    at = reinterpret_cast<AtStrData *>(msg.bytes);
    ASSERT_EQ(at->dtsType, AT_REPORT);
    ASSERT_EQ(msg.ind.datalen, 1);
}

/**
 * @tc.name: HiboxResidualAudioDualRecTest003
 * @tc.desc: 校验双录参数解析遇到未知TLV类型放行不拷贝：解析仍成功且长度按满结构体上报
 * @tc.type: FUNC
 */
HWTEST_F(HiboxResidualTest, HiboxResidualAudioDualRecTest003, TestSize.Level1)
{
    uint8_t buf[16];
    ParseMsgBuf msg;
    ClearMsg(msg);
    const uint8_t pad[4] = { 1, 2, 3, 4 };
    uint16_t off = MakeTlv(buf, 0x55, sizeof(pad), pad);
    ASSERT_TRUE(HiboxAudioMgmtReqParse(CMDID_DUAL_REC_PARAM_REPORT, buf, off,
        reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_EQ(msg.ind.msgType, NEARLINK_DUAL_REC_PARAM);
    ASSERT_EQ(msg.ind.datalen, sizeof(HiBoxNearlinkDualRecCaps));
    HiBoxNearlinkDualRecCaps *caps = reinterpret_cast<HiBoxNearlinkDualRecCaps *>(msg.bytes);
    ASSERT_EQ(caps->codecNum, 0);
}

/**
 * @tc.name: HiboxResidualAudioRspCombTest004
 * @tc.desc: 校验音频应答组包扩展通道只认双录消息：其余音频消息类型组不出任何TLV
 * @tc.type: FUNC
 */
HWTEST_F(HiboxResidualTest, HiboxResidualAudioRspCombTest004, TestSize.Level1)
{
    uint8_t buf[64] = {0};
    ParseMsgBuf arg;
    ClearMsg(arg);
    arg.ind.msgType = HDAP_CODEC_REPORT;
    arg.ind.datalen = 4;
    ASSERT_EQ(HiboxAudioMgmtRspComb(buf, sizeof(buf), &arg.ind), 0);
    ASSERT_EQ(buf[0], 0);
}
