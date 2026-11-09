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

/*
 * hibox TLV 解析边界测试：产品源码直接编入本测试目标，无 stub。
 * 覆盖异常数据与音频能力两类管理消息的逐类型合法解析、长度错配、未知类型、
 * TLV 截断与入口守卫，另直测 TLV 遍历器的空数据与多 TLV 边界。
 */

#include <gtest/gtest.h>
#include <cstring>
#include <ostream>
#include <vector>

// 产品头为 C 接口（.c 编入），统一以 C 链接引入，避免名字修饰不一致
extern "C" {
#include "hibox_audio.h"
#include "hibox_dts.h"
#include "hibox_local.h"
#include "hibox_process.h"
}

static_assert(sizeof(HiboxParseMsgInd) == 3, "HiboxParseMsgInd is pack(1): msgType(1)+datalen(2)");

namespace {
constexpr size_t MSG_IND_HDR_LEN = sizeof(HiboxParseMsgInd);

// TLV 多字节值为小端序；按宽度命名辅助函数，避免整型字面量重载歧义
std::vector<uint8_t> U8Val(uint8_t v)
{
    return {v};
}
std::vector<uint8_t> U16Val(uint16_t v)
{
    return {static_cast<uint8_t>(v), static_cast<uint8_t>(v >> 8)};
}
std::vector<uint8_t> U32Val(uint32_t v)
{
    return {static_cast<uint8_t>(v), static_cast<uint8_t>(v >> 8), static_cast<uint8_t>(v >> 16),
        static_cast<uint8_t>(v >> 24)};
}
// INT16 与 UINT16 字节序一致，符号由强转恢复
std::vector<uint8_t> I16Val(int16_t v)
{
    return U16Val(static_cast<uint16_t>(v));
}

// 追加一条 TLV：type(1) + length(2, 小端) + value；declaredLen 可与实际数据不符，用于构造截断流
void AppendTlv(std::vector<uint8_t> &msg, uint8_t type, uint16_t declaredLen, const std::vector<uint8_t> &value)
{
    msg.push_back(type);
    msg.push_back(static_cast<uint8_t>(declaredLen & 0xFF));
    msg.push_back(static_cast<uint8_t>((declaredLen >> 8) & 0xFF));
    msg.insert(msg.end(), value.begin(), value.end());
}

std::vector<uint8_t> BuildTlv(uint8_t type, uint16_t declaredLen, const std::vector<uint8_t> &value)
{
    std::vector<uint8_t> msg;
    AppendTlv(msg, type, declaredLen, value);
    return msg;
}

// 结果缓冲：HiboxParseMsgInd 头 + 载荷；每用例整体清零，保证未写入字段可断言
class ParseMsgResult {
public:
    ParseMsgResult()
    {
        std::memset(buf_, 0, sizeof(buf_));
    }
    HiboxParseMsgInd *Ind()
    {
        return reinterpret_cast<HiboxParseMsgInd *>(buf_);
    }
    template <typename T> T *Payload()
    {
        return reinterpret_cast<T *>(buf_ + MSG_IND_HDR_LEN);
    }

private:
    uint8_t buf_[MSG_IND_HDR_LEN + sizeof(DeviceAudioCapsInfo)]; // 取两结构体较大者
};

// 长度错配参数
struct LenMismatchParam {
    uint8_t type;
    uint16_t wrongLen;
};
void PrintTo(const LenMismatchParam &p, std::ostream *os)
{
    *os << "{type=0x" << std::hex << static_cast<int>(p.type) << std::dec << ", wrongLen=" << p.wrongLen << "}";
}

// 供直接调 TLV 遍历器的计数回调（C 链接，经函数指针传入 C 代码）
static int g_tlvCallbackCount = 0;
static uint8_t g_tlvSeenTypes[8] = {0};
} // namespace

extern "C" {
static bool CountingTlvParser(uint8_t type, uint16_t length, uint8_t *value, void *parseResult)
{
    (void)length;
    (void)value;
    if (g_tlvCallbackCount < 8) {
        g_tlvSeenTypes[g_tlvCallbackCount] = type;
    }
    g_tlvCallbackCount++;
    *(reinterpret_cast<uint8_t *>(parseResult)) = type;
    return true;
}
}

class HiboxDtsExceptionParseTest : public ::testing::Test {
protected:
    // 被测源码为纯解析函数，无静态状态，无需初始化
    void SetUp() override {}
    void TearDown() override {}
};

// 五种异常字段的合法 TLV 全量回读，逐字段校验解析值
TEST_F(HiboxDtsExceptionParseTest, DtsExceptionAllTypesValidRoundtrip)
{
    std::vector<uint8_t> msg;
    AppendTlv(msg, AUDIO_EXCEPTION_TYPE, sizeof(uint32_t), U32Val(0x11223344u));
    AppendTlv(msg, AUDIO_EXCEPTION_RSSI, sizeof(uint16_t), U16Val(0x5566));
    AppendTlv(msg, AUDIO_EXCEPTION_RETRANS, sizeof(uint32_t), U32Val(0xDEADBEEFu));
    AppendTlv(msg, AUDIO_EXCEPTION_SCENE, sizeof(uint32_t), U32Val(0xCAFEBABEu));
    AppendTlv(msg, AUDIO_EXCEPTION_AFH, sizeof(uint32_t), U32Val(0x0BADC0DEu));

    ParseMsgResult r;
    ASSERT_TRUE(HiboxDtsMgmtReqParse(CMDID_AUDIO_EXCEPTION, msg.data(), msg.size(),
        reinterpret_cast<uint8_t *>(r.Ind())));

    EXPECT_EQ(r.Ind()->msgType, AUDIO_EXCEPTION);
    // 解析载荷长度等于结构体长时 datalen 置为结构体长
    EXPECT_EQ(r.Ind()->datalen, sizeof(AudioExceptionData));
    EXPECT_EQ(r.Payload<AudioExceptionData>()->audioExceptionType, 0x11223344U);
    EXPECT_EQ(r.Payload<AudioExceptionData>()->audioQualityRssi, 0x5566U);
    EXPECT_EQ(r.Payload<AudioExceptionData>()->audioQualityRetrans, 0xDEADBEEFU);
    EXPECT_EQ(r.Payload<AudioExceptionData>()->audioQualityScene, 0xCAFEBABEU);
    EXPECT_EQ(r.Payload<AudioExceptionData>()->audioQualityAfh, 0x0BADC0DEU);
}

// 逐类型长度错配：解析中止，datalen 归零且载荷保持清零
class HiboxDtsExceptionLenMismatchTest : public HiboxDtsExceptionParseTest,
                                         public ::testing::WithParamInterface<LenMismatchParam> {};

TEST_P(HiboxDtsExceptionLenMismatchTest, DtsExceptionTypeLengthMismatch)
{
    const auto &p = GetParam();
    std::vector<uint8_t> value(p.wrongLen, 0xEE); // 字节数与声明一致，仅长度非法
    auto msg = BuildTlv(p.type, p.wrongLen, value);

    ParseMsgResult r;
    ASSERT_TRUE(HiboxDtsMgmtReqParse(CMDID_AUDIO_EXCEPTION, msg.data(), msg.size(),
        reinterpret_cast<uint8_t *>(r.Ind())));

    EXPECT_EQ(r.Ind()->datalen, 0U);
    EXPECT_EQ(r.Payload<AudioExceptionData>()->audioExceptionType, 0U);
    EXPECT_EQ(r.Payload<AudioExceptionData>()->audioQualityRssi, 0U);
    EXPECT_EQ(r.Payload<AudioExceptionData>()->audioQualityRetrans, 0U);
    EXPECT_EQ(r.Payload<AudioExceptionData>()->audioQualityScene, 0U);
    EXPECT_EQ(r.Payload<AudioExceptionData>()->audioQualityAfh, 0U);
}

INSTANTIATE_TEST_SUITE_P(DtsExceptionLenMismatch, HiboxDtsExceptionLenMismatchTest,
    ::testing::Values(
        LenMismatchParam{AUDIO_EXCEPTION_TYPE, 3},    // 合法 4，错配 3
        LenMismatchParam{AUDIO_EXCEPTION_RSSI, 3},    // 合法 2，错配 3
        LenMismatchParam{AUDIO_EXCEPTION_RETRANS, 5}, // 合法 4，错配 5
        LenMismatchParam{AUDIO_EXCEPTION_SCENE, 3},   // 合法 4，错配 3
        LenMismatchParam{AUDIO_EXCEPTION_AFH, 2}));   // 合法 4，错配 2

// 未知类型走 default：解析继续且返回 true。注：仅含未知 TLV 时 datalen 仍为结构体长而载荷未被写入（产品侧现状）
TEST_F(HiboxDtsExceptionParseTest, DtsExceptionUnknownTypeDefault)
{
    auto msg = BuildTlv(0x7F, 2, {0xAA, 0xBB});

    ParseMsgResult r;
    ASSERT_TRUE(HiboxDtsMgmtReqParse(CMDID_AUDIO_EXCEPTION, msg.data(), msg.size(),
        reinterpret_cast<uint8_t *>(r.Ind())));

    EXPECT_EQ(r.Ind()->datalen, sizeof(AudioExceptionData));
    EXPECT_EQ(r.Payload<AudioExceptionData>()->audioExceptionType, 0U);
    EXPECT_EQ(r.Payload<AudioExceptionData>()->audioQualityRssi, 0U);
}

// TLV 截断：声明长度超出剩余数据，解析中止
TEST_F(HiboxDtsExceptionParseTest, DtsExceptionTlvLengthExceedsRemaining)
{
    // 头声明 5 字节，实际只跟 1 字节，总长 4 大于一条 TLV 头，可过入口守卫
    auto msg = BuildTlv(AUDIO_EXCEPTION_TYPE, 5, {0x11});

    ParseMsgResult r;
    ASSERT_TRUE(HiboxDtsMgmtReqParse(CMDID_AUDIO_EXCEPTION, msg.data(), msg.size(),
        reinterpret_cast<uint8_t *>(r.Ind())));

    EXPECT_EQ(r.Ind()->datalen, 0U);
    EXPECT_EQ(r.Payload<AudioExceptionData>()->audioExceptionType, 0U);
}

// 长度为 0 的 TLV：解析中止，字段不写入
TEST_F(HiboxDtsExceptionParseTest, DtsExceptionZeroLengthTlvValue)
{
    std::vector<uint8_t> msg;
    AppendTlv(msg, AUDIO_EXCEPTION_TYPE, 0, {});               // 非法：空 value
    AppendTlv(msg, AUDIO_EXCEPTION_RSSI, 2, U16Val(0x5566));   // 使总长大于一条 TLV 头，可过入口守卫

    ParseMsgResult r;
    ASSERT_TRUE(HiboxDtsMgmtReqParse(CMDID_AUDIO_EXCEPTION, msg.data(), msg.size(),
        reinterpret_cast<uint8_t *>(r.Ind())));

    EXPECT_EQ(r.Ind()->datalen, 0U);
    EXPECT_EQ(r.Payload<AudioExceptionData>()->audioQualityRssi, 0U); // 未被解析
}

// 入口守卫：空指针与不足一条 TLV 头的输入返回 false（回调内空结果防御分支无法从导出接口触达）
TEST_F(HiboxDtsExceptionParseTest, DtsExceptionEntryGuard)
{
    ParseMsgResult r;
    uint8_t *res = reinterpret_cast<uint8_t *>(r.Ind());
    std::vector<uint8_t> guardMsg = BuildTlv(AUDIO_EXCEPTION_TYPE, 4, U32Val(0x11223344u));

    EXPECT_FALSE(HiboxDtsMgmtReqParse(CMDID_AUDIO_EXCEPTION, nullptr, guardMsg.size(), res));
    EXPECT_FALSE(HiboxDtsMgmtReqParse(CMDID_AUDIO_EXCEPTION, guardMsg.data(), guardMsg.size(), nullptr));
    EXPECT_FALSE(HiboxDtsMgmtReqParse(CMDID_AUDIO_EXCEPTION, guardMsg.data(), HIBOX_TLV_HEADER_LEN, res));
    EXPECT_FALSE(HiboxDtsMgmtReqParse(CMDID_AUDIO_EXCEPTION, guardMsg.data(), 0, res));
}

class HiboxAudioCapParseTest : public ::testing::Test {
protected:
    // 被测源码为纯解析函数，无静态状态，无需初始化
    void SetUp() override {}
    void TearDown() override {}
};

// 五种能力字段（含 SLE）的合法 TLV 全量回读，逐字段校验解析值
TEST_F(HiboxAudioCapParseTest, AudioCapAllTypesValidRoundtrip)
{
    std::vector<uint8_t> ddm; // DDM：music/hwa/other 三个 UINT16，共 6 字节
    for (uint16_t v : {0x0506, 0x0708, 0x090A}) {
        auto b = U16Val(v);
        ddm.insert(ddm.end(), b.begin(), b.end());
    }

    std::vector<uint8_t> sle;
    auto off = I16Val(-2);
    sle.insert(sle.end(), off.begin(), off.end());
    sle.push_back(3); // sceneNum
    for (uint8_t i = 0; i < MAX_SCENE_NUM; i++) { // timesMap 全 16 项，解析器不裁剪 sceneNum
        sle.push_back(i);
        sle.push_back(static_cast<uint8_t>(i * 2));
    }

    std::vector<uint8_t> msg;
    AppendTlv(msg, AUDIO_CAPS_TYPE, AUDIO_CAP_LEN, U32Val(0xAABBCCDDu));
    AppendTlv(msg, AUDIO_ABS_VOLUME_TYPE, AUDIO_ABSVOLUME_LEN, U8Val(0x5A));
    AppendTlv(msg, AUDIO_JOIN_INTERVAL_TYPE, AUDIO_INTERVAL_LEN, U16Val(0x0304));
    AppendTlv(msg, AUDIO_DDM_INTERVAL_TYPE, AUDIO_DDM_LATENCY_LEN, ddm);
    AppendTlv(msg, AUDIO_SLE_INTERVAL_TYPE, AUDIO_SLE_INTERVAL_LEN, sle);

    ASSERT_EQ(msg.size(), 3 + AUDIO_CAP_LEN + 3 + AUDIO_ABSVOLUME_LEN + 3 + AUDIO_INTERVAL_LEN +
        3 + AUDIO_DDM_LATENCY_LEN + 3 + AUDIO_SLE_INTERVAL_LEN);

    ParseMsgResult r;
    ASSERT_TRUE(HiboxAudioMgmtReqParse(CMDID_AUDIO_CAP, msg.data(), msg.size(),
        reinterpret_cast<uint8_t *>(r.Ind())));

    EXPECT_EQ(r.Ind()->msgType, AUDIO_CAP_QUERY);
    EXPECT_EQ(r.Ind()->datalen, sizeof(DeviceAudioCapsInfo));
    auto *d = r.Payload<DeviceAudioCapsInfo>();
    EXPECT_EQ(d->caps, 0xAABBCCDDU);
    EXPECT_EQ(d->absVolume, 0x5AU);
    EXPECT_EQ(d->joinIntervalMs, 0x0304U);
    EXPECT_EQ(d->musicIntervalMs, 0x0506U);
    EXPECT_EQ(d->hwaIntervalMs, 0x0708U);
    EXPECT_EQ(d->otherIntervalMs, 0x090AU);
    EXPECT_EQ(d->sleConfig.offset, -2);
    EXPECT_EQ(d->sleConfig.sceneNum, 3);
    EXPECT_EQ(d->sleConfig.timesMap[3].scene, 3);
    EXPECT_EQ(d->sleConfig.timesMap[3].times, 6);
    EXPECT_EQ(d->sleConfig.timesMap[MAX_SCENE_NUM - 1].times,
        static_cast<uint8_t>((MAX_SCENE_NUM - 1) * 2));
}

// 逐类型长度错配：解析中止，datalen 归零且载荷保持清零
class HiboxAudioCapLenMismatchTest : public HiboxAudioCapParseTest,
                                     public ::testing::WithParamInterface<LenMismatchParam> {};

TEST_P(HiboxAudioCapLenMismatchTest, AudioCapTypeLengthMismatch)
{
    const auto &p = GetParam();
    std::vector<uint8_t> value(p.wrongLen, 0xEE);
    auto msg = BuildTlv(p.type, p.wrongLen, value);

    ParseMsgResult r;
    ASSERT_TRUE(HiboxAudioMgmtReqParse(CMDID_AUDIO_CAP, msg.data(), msg.size(),
        reinterpret_cast<uint8_t *>(r.Ind())));

    EXPECT_EQ(r.Ind()->datalen, 0U);
    auto *d = r.Payload<DeviceAudioCapsInfo>();
    EXPECT_EQ(d->caps, 0U);
    EXPECT_EQ(d->absVolume, 0U);
    EXPECT_EQ(d->joinIntervalMs, 0U);
    EXPECT_EQ(d->musicIntervalMs, 0U);
    EXPECT_EQ(d->sleConfig.offset, 0);
}

INSTANTIATE_TEST_SUITE_P(AudioCapLenMismatch, HiboxAudioCapLenMismatchTest,
    ::testing::Values(
        LenMismatchParam{AUDIO_CAPS_TYPE, 3},          // 合法 4，错配 3
        LenMismatchParam{AUDIO_ABS_VOLUME_TYPE, 2},    // 合法 1，错配 2
        LenMismatchParam{AUDIO_JOIN_INTERVAL_TYPE, 1}, // 合法 2，错配 1
        LenMismatchParam{AUDIO_DDM_INTERVAL_TYPE, 5},  // 合法 6，错配 5
        LenMismatchParam{AUDIO_SLE_INTERVAL_TYPE, AUDIO_SLE_INTERVAL_LEN - 1})); // 合法 35，错配 34

// 未知类型走 default：解析继续且返回 true
TEST_F(HiboxAudioCapParseTest, AudioCapUnknownTypeDefault)
{
    auto msg = BuildTlv(0x7F, 4, {0x11, 0x22, 0x33, 0x44});

    ParseMsgResult r;
    ASSERT_TRUE(HiboxAudioMgmtReqParse(CMDID_AUDIO_CAP, msg.data(), msg.size(),
        reinterpret_cast<uint8_t *>(r.Ind())));

    EXPECT_EQ(r.Ind()->datalen, sizeof(DeviceAudioCapsInfo)); // 解析到载荷即置满长
    EXPECT_EQ(r.Payload<DeviceAudioCapsInfo>()->caps, 0U);
}

// TLV 截断：声明长度超出剩余数据，解析中止
TEST_F(HiboxAudioCapParseTest, AudioCapTlvLengthExceedsRemaining)
{
    auto msg = BuildTlv(AUDIO_CAPS_TYPE, 0x00FF, {0x11}); // 声明 255，仅 1 字节

    ParseMsgResult r;
    ASSERT_TRUE(HiboxAudioMgmtReqParse(CMDID_AUDIO_CAP, msg.data(), msg.size(),
        reinterpret_cast<uint8_t *>(r.Ind())));

    EXPECT_EQ(r.Ind()->datalen, 0U);
    EXPECT_EQ(r.Payload<DeviceAudioCapsInfo>()->caps, 0U);
}

// 入口守卫与未知 cmdId 均返回 false
TEST_F(HiboxAudioCapParseTest, AudioCapEntryGuardAndUnknownCmd)
{
    ParseMsgResult r;
    uint8_t *res = reinterpret_cast<uint8_t *>(r.Ind());
    std::vector<uint8_t> guardMsg = BuildTlv(AUDIO_CAPS_TYPE, AUDIO_CAP_LEN, U32Val(0xAABBCCDDu));

    EXPECT_FALSE(HiboxAudioMgmtReqParse(CMDID_AUDIO_CAP, nullptr, guardMsg.size(), res));
    EXPECT_FALSE(HiboxAudioMgmtReqParse(CMDID_AUDIO_CAP, guardMsg.data(), guardMsg.size(), nullptr));
    EXPECT_FALSE(HiboxAudioMgmtReqParse(CMDID_AUDIO_CAP, guardMsg.data(), HIBOX_TLV_HEADER_LEN, res));
    EXPECT_FALSE(HiboxAudioMgmtReqParse(0x63, guardMsg.data(), guardMsg.size(), res)); // 未定义 cmdId
}

class HiboxTlvParseHandlerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        g_tlvCallbackCount = 0;
        std::memset(g_tlvSeenTypes, 0, sizeof(g_tlvSeenTypes));
    }
    void TearDown() override { SetUp(); } // 对称复位测试自身辅助计数
};

// 空指针与不足一条 TLV 头的输入：不回调，返回 0
TEST_F(HiboxTlvParseHandlerTest, TlvHandlerNullAndEmptyData)
{
    uint8_t ctx = 0;
    uint8_t two[2] = {0x01, 0x02};

    EXPECT_EQ(HiboxTlvParseHandler(nullptr, 10, CountingTlvParser, &ctx), 0U);
    EXPECT_EQ(HiboxTlvParseHandler(two, 0, CountingTlvParser, &ctx), 0U);
    EXPECT_EQ(HiboxTlvParseHandler(two, HIBOX_TLV_HEADER_LEN - 1, CountingTlvParser, &ctx), 0U);
    EXPECT_EQ(g_tlvCallbackCount, 0);
}

// 多 TLV 遍历：回调按序收到各类型，返回值为载荷长度之和
TEST_F(HiboxTlvParseHandlerTest, TlvHandlerWalkthrough)
{
    std::vector<uint8_t> msg;
    AppendTlv(msg, 0x11, 2, {0xAA, 0xBB});
    AppendTlv(msg, 0x22, 1, {0xCC});

    uint8_t ctx = 0;
    EXPECT_EQ(HiboxTlvParseHandler(msg.data(), msg.size(), CountingTlvParser, &ctx), 3U);
    EXPECT_EQ(g_tlvCallbackCount, 2);
    EXPECT_EQ(g_tlvSeenTypes[0], 0x11);
    EXPECT_EQ(g_tlvSeenTypes[1], 0x22);
    EXPECT_EQ(ctx, 0x22);
}
