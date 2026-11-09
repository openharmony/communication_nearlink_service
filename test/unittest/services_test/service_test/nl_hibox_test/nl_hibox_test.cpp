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
#include <cstring>

extern "C" {
#include "hibox_audio.h"
#include "hibox_dts.h"
#include "hibox_local.h"
#include "hibox_process.h"
}

using namespace testing::ext;

namespace {
constexpr uint8_t TLV_STUB_TYPE = 1;

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

// 只认 type=1 且长度为 1 的 TLV，其余一律拒绝
bool ParseTlvStub(uint8_t type, uint16_t length, uint8_t *value, void *parseResult)
{
    (void)value;
    if (parseResult == nullptr) {
        return false;
    }
    return type == TLV_STUB_TYPE && length == 1;
}

bool ParseTlvAcceptAll(uint8_t type, uint16_t length, uint8_t *value, void *parseResult)
{
    (void)type;
    (void)length;
    (void)value;
    return parseResult != nullptr;
}
} // namespace

class HiboxLocalTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}
};

/**
 * @tc.name: HiboxTlvParseHandlerTest001
 * @tc.desc: 校验TLV流解析器按[类型1字节+长度2字节+值]逐段累计，返回全部有效值长度之和
 * @tc.type: FUNC
 */
HWTEST_F(HiboxLocalTest, HiboxTlvParseHandlerTest001, TestSize.Level1)
{
    uint8_t buf[16];
    uint16_t off = 0;
    const uint8_t one = 0xA5;
    off += MakeTlv(buf + off, TLV_STUB_TYPE, 1, &one);
    off += MakeTlv(buf + off, TLV_STUB_TYPE, 1, &one);
    int sink = 0;
    ASSERT_EQ(HiboxTlvParseHandler(buf, off, ParseTlvStub, &sink), 2);
}

/**
 * @tc.name: HiboxTlvParseHandlerTest002
 * @tc.desc: 校验TLV流解析器对空指针、零值长、声明长度越过缓冲、回调拒绝四类坏输入一律清零返回
 * @tc.type: FUNC
 */
HWTEST_F(HiboxLocalTest, HiboxTlvParseHandlerTest002, TestSize.Level1)
{
    int sink = 0;
    ASSERT_EQ(HiboxTlvParseHandler(nullptr, 8, ParseTlvStub, &sink), 0);

    uint8_t buf[8];
    const uint8_t one = 1;
    // 值长度为 0 的 TLV
    MakeTlv(buf, TLV_STUB_TYPE, 0, &one);
    ASSERT_EQ(HiboxTlvParseHandler(buf, HIBOX_TLV_HEADER_LEN, ParseTlvAcceptAll, &sink), 0);

    // 声明值长度 5，但缓冲里没有值字节
    MakeTlv(buf, TLV_STUB_TYPE, 5, nullptr);
    ASSERT_EQ(HiboxTlvParseHandler(buf, HIBOX_TLV_HEADER_LEN, ParseTlvAcceptAll, &sink), 0);

    // 回调拒收（出参为空触发桩内拒绝）
    MakeTlv(buf, TLV_STUB_TYPE, 1, &one);
    ASSERT_EQ(HiboxTlvParseHandler(buf, HIBOX_TLV_HEADER_LEN + 1, ParseTlvStub, nullptr), 0);
}

/**
 * @tc.name: HiboxL2cuDoCheckMicTest001
 * @tc.desc: 校验CRC16校验码（多项式0x8005、初值0、高位先行）对已知向量与空输入的取值
 * @tc.type: FUNC
 */
HWTEST_F(HiboxLocalTest, HiboxL2cuDoCheckMicTest001, TestSize.Level1)
{
    const uint8_t data[] = "123456789";
    ASSERT_EQ(HiboxL2cuDoCheckMic(const_cast<uint8_t *>(data), sizeof(data) - 1), 0xFEE8);
    ASSERT_EQ(HiboxL2cuDoCheckMic(const_cast<uint8_t *>(data), 0), 0x0000);
    const uint8_t single[] = {0xAA};
    ASSERT_EQ(HiboxL2cuDoCheckMic(const_cast<uint8_t *>(single), sizeof(single)), 0x03FC);
}

/**
 * @tc.name: HiboxCompareArrayToValueTest001
 * @tc.desc: 校验数组全等判断：全等返回真，任一元素不同返回假，零长度视为全等
 * @tc.type: FUNC
 */
HWTEST_F(HiboxLocalTest, HiboxCompareArrayToValueTest001, TestSize.Level1)
{
    uint8_t allFive[3] = {5, 5, 5};
    uint8_t mixed[3] = {5, 6, 5};
    ASSERT_TRUE(HiboxCompareArrayToValue(allFive, 5, sizeof(allFive)));
    ASSERT_FALSE(HiboxCompareArrayToValue(mixed, 5, sizeof(mixed)));
    ASSERT_TRUE(HiboxCompareArrayToValue(mixed, 5, 0));
}

/**
 * @tc.name: HiboxCombineTlvTest001
 * @tc.desc: 校验TLV组包：正常输入写出[类型+长度+值]并返回总长，空指针、零值长、缓冲不足、全无效值返回0
 * @tc.type: FUNC
 */
HWTEST_F(HiboxLocalTest, HiboxCombineTlvTest001, TestSize.Level1)
{
    uint8_t buf[8] = {0};
    const uint8_t val[2] = {0x11, 0x22};
    uint16_t used = HiboxCombineTlv(buf, sizeof(buf), 0x07, sizeof(val), val);
    ASSERT_EQ(used, HIBOX_TLV_HEADER_LEN + sizeof(val));
    ASSERT_EQ(buf[0], 0x07);
    ASSERT_EQ(buf[1], sizeof(val));
    ASSERT_EQ(buf[2], 0);
    ASSERT_EQ(buf[3], 0x11);
    ASSERT_EQ(buf[4], 0x22);

    ASSERT_EQ(HiboxCombineTlv(nullptr, sizeof(buf), 0x07, 1, val), 0);
    ASSERT_EQ(HiboxCombineTlv(buf, sizeof(buf), 0x07, 0, val), 0);
    ASSERT_EQ(HiboxCombineTlv(buf, HIBOX_TLV_HEADER_LEN - 1, 0x07, 1, val), 0);
    ASSERT_EQ(HiboxCombineTlv(buf, sizeof(buf), 0x07, sizeof(buf), val), 0);
    const uint8_t invalid[1] = {HIBOX_INVALID_DATA};
    ASSERT_EQ(HiboxCombineTlv(buf, sizeof(buf), 0x07, 1, invalid), 0);
}

/**
 * @tc.name: HiboxCommResultParseTest001
 * @tc.desc: 校验通用应答解析：结果TLV(0x7F/长度1)取回应答码；长度不符或累计负载超限则datalen保持0
 * @tc.type: FUNC
 */
HWTEST_F(HiboxLocalTest, HiboxCommResultParseTest001, TestSize.Level1)
{
    uint8_t buf[8];
    ParseMsgBuf msg;
    ClearMsg(msg);
    const uint8_t ok = 0x05;
    uint16_t off = MakeTlv(buf, HIBOX_RSP_TYPE_RESULT, HIBOX_COMMON_MSG_VALUE_LEN, &ok);
    HiboxCommResultParse(NEARLINK_AUDIO_DATAPATH, buf, off, reinterpret_cast<uint8_t *>(&msg));
    ASSERT_EQ(msg.ind.msgType, NEARLINK_AUDIO_DATAPATH);
    ASSERT_EQ(msg.ind.datalen, sizeof(HiboxRspCommonResult));
    ASSERT_EQ(msg.bytes[0], ok);

    // 结果TLV长度不是1：回调拒绝，但datalen仍按满结构体填写（与成功不可区分）
    const uint8_t bad[HIBOX_COMMON_MSG_VALUE_LEN + 1] = {1, 2};
    off = MakeTlv(buf, HIBOX_RSP_TYPE_RESULT, sizeof(bad), bad);
    HiboxCommResultParse(NEARLINK_AUDIO_DATAPATH, buf, off, reinterpret_cast<uint8_t *>(&msg));
    ASSERT_EQ(msg.ind.datalen, sizeof(HiboxRspCommonResult));

    // 未知类型TLV被放行，累计值长度超过结构体上限：datalen保持0
    const uint8_t pad[4] = {0};
    off = MakeTlv(buf, 0x01, sizeof(pad), pad);
    ClearMsg(msg);
    HiboxCommResultParse(NEARLINK_AUDIO_DATAPATH, buf, off, reinterpret_cast<uint8_t *>(&msg));
    ASSERT_EQ(msg.ind.datalen, 0);
}

/**
 * @tc.name: HiboxCommonResultCombTest001
 * @tc.desc: 校验通用应答组包：datalen为1时写出一个结果TLV并返回长度，datalen不符或空指针返回0
 * @tc.type: FUNC
 */
HWTEST_F(HiboxLocalTest, HiboxCommonResultCombTest001, TestSize.Level1)
{
    uint8_t buf[8] = {0};
    ParseMsgBuf arg;
    ClearMsg(arg);
    arg.ind.msgType = NEARLINK_AUDIO_PROFILE;
    arg.ind.datalen = HIBOX_COMMON_MSG_VALUE_LEN;
    arg.bytes[0] = HIBOX_ERR_PARA_ERROR;
    uint16_t used = HiboxCommonResultComb(buf, sizeof(buf), &arg.ind);
    ASSERT_EQ(used, HIBOX_TLV_HEADER_LEN + HIBOX_COMMON_MSG_VALUE_LEN);
    ASSERT_EQ(buf[0], HIBOX_RSP_TYPE_RESULT);
    ASSERT_EQ(buf[1], HIBOX_COMMON_MSG_VALUE_LEN);
    ASSERT_EQ(buf[3], HIBOX_ERR_PARA_ERROR);

    arg.ind.datalen = HIBOX_COMMON_MSG_VALUE_LEN + 1;
    ASSERT_EQ(HiboxCommonResultComb(buf, sizeof(buf), &arg.ind), 0);
    arg.ind.datalen = HIBOX_COMMON_MSG_VALUE_LEN;
    ASSERT_EQ(HiboxCommonResultComb(nullptr, sizeof(buf), &arg.ind), 0);
}

/**
 * @tc.name: HiboxCombileMsgTest001
 * @tc.desc: 校验整帧组包：写出[数据类型+服务号+命令号+TLV+校验码]，缓冲不足或空指针时不写任何字节
 * @tc.type: FUNC
 */
HWTEST_F(HiboxLocalTest, HiboxCombileMsgTest001, TestSize.Level1)
{
    uint8_t buf[16] = {0};
    uint8_t tlv[8] = {0};
    const uint8_t val[2] = {0x31, 0x32};
    uint16_t tlvLen = HiboxCombineTlv(tlv, sizeof(tlv), 0x07, sizeof(val), val);
    HiboxCombileMsg(buf, sizeof(buf), SID_DTS_MGMT, CMDID_WEAR_STATUS, tlv, tlvLen, HIBOX_REQ);
    ASSERT_EQ(buf[0], HIBOX_REQ);
    ASSERT_EQ(buf[1], SID_DTS_MGMT);
    ASSERT_EQ(buf[2], CMDID_WEAR_STATUS);
    ASSERT_EQ(buf[3], 0x07);
    uint16_t crc = HiboxL2cuDoCheckMic(buf, HIBOX_HEAD_FIELD_LEN + tlvLen);
    uint16_t tail = static_cast<uint16_t>(buf[HIBOX_HEAD_FIELD_LEN + tlvLen]) |
        static_cast<uint16_t>(buf[HIBOX_HEAD_FIELD_LEN + tlvLen + 1] << 8);
    ASSERT_EQ(tail, crc);

    uint8_t small[HIBOX_MIN_PAYLOAD_LEN - 1] = {0};
    HiboxCombileMsg(small, sizeof(small), SID_DTS_MGMT, CMDID_WEAR_STATUS, tlv, tlvLen, HIBOX_REQ);
    ASSERT_EQ(small[0], 0);
    HiboxCombileMsg(nullptr, sizeof(buf), SID_DTS_MGMT, CMDID_WEAR_STATUS, tlv, tlvLen, HIBOX_REQ);
}

class HiboxDtsTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}
};

/**
 * @tc.name: HiboxDtsMgmtReqParseTest001
 * @tc.desc: 校验DTS请求解析入口的坏输入门：空TLV、长度不足TLV头、空出参、未知命令一律拒绝
 * @tc.type: FUNC
 */
HWTEST_F(HiboxDtsTest, HiboxDtsMgmtReqParseTest001, TestSize.Level1)
{
    uint8_t buf[8];
    ParseMsgBuf msg;
    ClearMsg(msg);
    const uint8_t one = 1;
    uint16_t off = MakeTlv(buf, DTS_CAP_TYPE, 1, &one);
    ASSERT_FALSE(HiboxDtsMgmtReqParse(CMDID_CAPABILITY, nullptr, off, reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_FALSE(HiboxDtsMgmtReqParse(CMDID_CAPABILITY, buf, HIBOX_TLV_HEADER_LEN,
        reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_FALSE(HiboxDtsMgmtReqParse(CMDID_CAPABILITY, buf, off, nullptr));
    const uint8_t unknownCmd = 0x7E;
    ASSERT_FALSE(HiboxDtsMgmtReqParse(unknownCmd, buf, off, reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_TRUE(HiboxDtsMgmtReqParse(CMDID_CAPABILITY, buf, off, reinterpret_cast<uint8_t *>(&msg)));
}

/**
 * @tc.name: HiboxDtsMgmtReqParseTest002
 * @tc.desc: 校验能力上报解析：能力TLV(类型1)的值复制进结果；单条越界被拒；累计超结构体上限datalen归零
 * @tc.type: FUNC
 */
HWTEST_F(HiboxDtsTest, HiboxDtsMgmtReqParseTest002, TestSize.Level1)
{
    uint8_t buf[24];
    ParseMsgBuf msg;
    ClearMsg(msg);
    const uint8_t caps[DTS_MAX_CAP_LEN] = {DTS_MGR_FEATURE_VID, DTS_MGR_FEATURE_FOTA};
    uint16_t off = MakeTlv(buf, DTS_CAP_TYPE, sizeof(caps), caps);
    ASSERT_TRUE(HiboxDtsMgmtReqParse(CMDID_CAPABILITY, buf, off, reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_EQ(msg.ind.msgType, DTS_CAP);
    ASSERT_EQ(msg.ind.datalen, sizeof(caps));
    ASSERT_EQ(msg.bytes[0], DTS_MGR_FEATURE_VID);
    ASSERT_EQ(msg.bytes[1], DTS_MGR_FEATURE_FOTA);

    // 单条能力TLV长度超过10字节：拷贝保护拒绝
    const uint8_t tooLong[DTS_MAX_CAP_LEN + 1] = {0};
    ClearMsg(msg);
    off = MakeTlv(buf, DTS_CAP_TYPE, sizeof(tooLong), tooLong);
    ASSERT_TRUE(HiboxDtsMgmtReqParse(CMDID_CAPABILITY, buf, off, reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_EQ(msg.ind.datalen, 0);

    // 未知类型TLV被放行累计，累计值长度超过DtsCapData大小：datalen保持0
    ClearMsg(msg);
    off = MakeTlv(buf, 0x55, sizeof(tooLong), tooLong);
    ASSERT_TRUE(HiboxDtsMgmtReqParse(CMDID_CAPABILITY, buf, off, reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_EQ(msg.ind.datalen, 0);
}

/**
 * @tc.name: HiboxDtsMgmtReqParseTest003
 * @tc.desc: 校验佩戴状态解析：左/右/自由佩戴三种TLV各取1字节进对应字段，任一长度不符该轮解析作废
 * @tc.type: FUNC
 */
HWTEST_F(HiboxDtsTest, HiboxDtsMgmtReqParseTest003, TestSize.Level1)
{
    uint8_t buf[24];
    ParseMsgBuf msg;
    ClearMsg(msg);
    uint16_t off = 0;
    const uint8_t inEar = 0x01;
    const uint8_t notWear = 0x00;
    const uint8_t invalid = 0xFF;
    off += MakeTlv(buf + off, LEFT_WEAR_TYPE, LEFT_WEAR_TYPE_LEN, &inEar);
    off += MakeTlv(buf + off, RIGHT_WEAR_TYPE, RIGHT_WEAR_TYPE_LEN, &notWear);
    off += MakeTlv(buf + off, FREEMAN_WEAR_TYPE, FREEMAN_WEAR_TYPE_LEN, &invalid);
    ASSERT_TRUE(HiboxDtsMgmtReqParse(CMDID_WEAR_STATUS, buf, off, reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_EQ(msg.ind.msgType, WEAR_STATUS);
    ASSERT_EQ(msg.ind.datalen, sizeof(DtsWearData));
    DtsWearData *wear = reinterpret_cast<DtsWearData *>(msg.bytes);
    ASSERT_EQ(wear->leftWear, inEar);
    ASSERT_EQ(wear->rightWear, notWear);
    ASSERT_EQ(wear->freemanWear, invalid);

    // 未知佩戴类型：default分支放行，值长度照常累计
    const uint8_t big[5] = {1, 2, 3, 4, 5};
    ClearMsg(msg);
    off = MakeTlv(buf, 0x55, sizeof(big), big);
    ASSERT_TRUE(HiboxDtsMgmtReqParse(CMDID_WEAR_STATUS, buf, off, reinterpret_cast<uint8_t *>(&msg)));
    wear = reinterpret_cast<DtsWearData *>(msg.bytes);
    ASSERT_EQ(wear->leftWear, 0);
    ASSERT_EQ(msg.ind.datalen, 0);

    // 左耳TLV长度不是1：整轮解析作废，字段保持清零，datalen仍按满结构体填写
    const uint8_t two[2] = {0x01, 0x01};
    ClearMsg(msg);
    off = MakeTlv(buf, LEFT_WEAR_TYPE, sizeof(two), two);
    ASSERT_TRUE(HiboxDtsMgmtReqParse(CMDID_WEAR_STATUS, buf, off, reinterpret_cast<uint8_t *>(&msg)));
    wear = reinterpret_cast<DtsWearData *>(msg.bytes);
    ASSERT_EQ(wear->leftWear, 0);
    ASSERT_EQ(msg.ind.datalen, sizeof(DtsWearData));
}

/**
 * @tc.name: HiboxDtsMgmtReqParseTest004
 * @tc.desc: 校验AT串解析：电量/设备信息两类TLV按类型区分并复制串值；短于3字节或两段累计超255都作废
 * @tc.type: FUNC
 */
HWTEST_F(HiboxDtsTest, HiboxDtsMgmtReqParseTest004, TestSize.Level1)
{
    uint8_t buf[512];
    ParseMsgBuf msg;
    ClearMsg(msg);
    const uint8_t battery[] = "BAT";
    uint16_t off = MakeTlv(buf, BATTERY_REPORT_TYPE, sizeof(battery) - 1, battery);
    ASSERT_TRUE(HiboxDtsMgmtReqParse(CMDID_AT_REPORT, buf, off, reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_EQ(msg.ind.msgType, DTS_REPORT);
    ASSERT_EQ(msg.ind.datalen, sizeof(battery) - 1 + 1);
    AtStrData *at = reinterpret_cast<AtStrData *>(msg.bytes);
    ASSERT_EQ(at->dtsType, BATTERY_REPORT);
    ASSERT_EQ(memcmp(at->atString, battery, sizeof(battery) - 1), 0);

    const uint8_t devInfo[] = "INFO";
    ClearMsg(msg);
    off = MakeTlv(buf, DEVICE_INFO_REPORT_TYPE, sizeof(devInfo) - 1, devInfo);
    ASSERT_TRUE(HiboxDtsMgmtReqParse(CMDID_AT_REPORT, buf, off, reinterpret_cast<uint8_t *>(&msg)));
    at = reinterpret_cast<AtStrData *>(msg.bytes);
    ASSERT_EQ(at->dtsType, AT_REPORT);
    ASSERT_EQ(memcmp(at->atString, devInfo, sizeof(devInfo) - 1), 0);

    // 值长度低于下限3：回调拒绝
    const uint8_t shortVal[2] = {0x31, 0x32};
    ClearMsg(msg);
    off = MakeTlv(buf, BATTERY_REPORT_TYPE, sizeof(shortVal), shortVal);
    ASSERT_TRUE(HiboxDtsMgmtReqParse(CMDID_AT_REPORT, buf, off, reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_EQ(msg.ind.datalen, 1);

    // 设备信息TLV值长度低于下限3：作废
    ClearMsg(msg);
    off = MakeTlv(buf, DEVICE_INFO_REPORT_TYPE, sizeof(shortVal), shortVal);
    ASSERT_TRUE(HiboxDtsMgmtReqParse(CMDID_AT_REPORT, buf, off, reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_EQ(msg.ind.datalen, 1);

    // AT串里的未知类型：default放行，累计值长度仍计入
    ClearMsg(msg);
    off = MakeTlv(buf, 0x55, sizeof(shortVal), shortVal);
    ASSERT_TRUE(HiboxDtsMgmtReqParse(CMDID_AT_REPORT, buf, off, reinterpret_cast<uint8_t *>(&msg)));
    at = reinterpret_cast<AtStrData *>(msg.bytes);
    ASSERT_EQ(at->dtsType, 0);

    // 两段各200字节：单段合法但累计超过255，整条作废
    const uint8_t bigVal[200] = {0};
    ClearMsg(msg);
    off = 0;
    off += MakeTlv(buf + off, BATTERY_REPORT_TYPE, sizeof(bigVal), bigVal);
    off += MakeTlv(buf + off, BATTERY_REPORT_TYPE, sizeof(bigVal), bigVal);
    ASSERT_TRUE(HiboxDtsMgmtReqParse(CMDID_AT_REPORT, buf, off, reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_EQ(msg.ind.datalen, 0);
}

/**
 * @tc.name: HiboxDtsMgmtReqParseTest005
 * @tc.desc: 校验音频质量异常解析：异常类型/信号强度/重传/场景/跳频五种TLV按精确长度取值，长度不符整条作废
 * @tc.type: FUNC
 */
HWTEST_F(HiboxDtsTest, HiboxDtsMgmtReqParseTest005, TestSize.Level1)
{
    uint8_t buf[64];
    ParseMsgBuf msg;
    ClearMsg(msg);
    const uint8_t typeVal[4] = {0x78, 0x56, 0x34, 0x12};
    const uint8_t rssiVal[2] = {0xC4, 0xD2};
    const uint8_t retransVal[4] = {0xB8, 0x0B, 0x00, 0x00};
    const uint8_t sceneVal[4] = {5, 0, 0, 0};
    const uint8_t afhVal[4] = {7, 0, 0, 0};
    uint16_t off = 0;
    off += MakeTlv(buf + off, AUDIO_EXCEPTION_TYPE, sizeof(typeVal), typeVal);
    off += MakeTlv(buf + off, AUDIO_EXCEPTION_RSSI, sizeof(rssiVal), rssiVal);
    off += MakeTlv(buf + off, AUDIO_EXCEPTION_RETRANS, sizeof(retransVal), retransVal);
    off += MakeTlv(buf + off, AUDIO_EXCEPTION_SCENE, sizeof(sceneVal), sceneVal);
    off += MakeTlv(buf + off, AUDIO_EXCEPTION_AFH, sizeof(afhVal), afhVal);
    ASSERT_TRUE(HiboxDtsMgmtReqParse(CMDID_AUDIO_EXCEPTION, buf, off, reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_EQ(msg.ind.msgType, AUDIO_EXCEPTION);
    ASSERT_EQ(msg.ind.datalen, sizeof(AudioExceptionData));
    AudioExceptionData *excep = reinterpret_cast<AudioExceptionData *>(msg.bytes);
    ASSERT_EQ(excep->audioExceptionType, 0x12345678);
    ASSERT_EQ(excep->audioQualityRssi, 0xD2C4);
    ASSERT_EQ(excep->audioQualityRetrans, 3000);
    ASSERT_EQ(excep->audioQualityScene, 5);
    ASSERT_EQ(excep->audioQualityAfh, 7);

    // 异常类型TLV长度与4字节不符：整条作废
    const uint8_t wrongLen[5] = {0};
    ClearMsg(msg);
    off = MakeTlv(buf, AUDIO_EXCEPTION_TYPE, sizeof(wrongLen), wrongLen);
    ASSERT_TRUE(HiboxDtsMgmtReqParse(CMDID_AUDIO_EXCEPTION, buf, off, reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_EQ(msg.ind.datalen, 0);

    // 未知类型TLV被放行累计，值长度超过结构体：整条作废守卫
    const uint8_t pad[sizeof(AudioExceptionData) + 2] = {0};
    ClearMsg(msg);
    off = MakeTlv(buf, 0x55, sizeof(pad), pad);
    ASSERT_TRUE(HiboxDtsMgmtReqParse(CMDID_AUDIO_EXCEPTION, buf, off, reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_EQ(msg.ind.datalen, 0);
}

/**
 * @tc.name: HiboxDtsMgmtRspParseTest001
 * @tc.desc: 校验DTS应答解析只认能力命令，其余命令与坏输入一律拒绝
 * @tc.type: FUNC
 */
HWTEST_F(HiboxDtsTest, HiboxDtsMgmtRspParseTest001, TestSize.Level1)
{
    uint8_t buf[8];
    ParseMsgBuf msg;
    ClearMsg(msg);
    const uint8_t caps[2] = {DTS_MGR_FEATURE_CHR, 0};
    uint16_t off = MakeTlv(buf, DTS_CAP_TYPE, sizeof(caps), caps);
    ASSERT_TRUE(HiboxDtsMgmtRspParse(CMDID_CAPABILITY, buf, off, reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_EQ(msg.ind.msgType, DTS_CAP);
    ASSERT_EQ(msg.ind.datalen, sizeof(caps));
    ASSERT_FALSE(HiboxDtsMgmtRspParse(CMDID_WEAR_STATUS, buf, off, reinterpret_cast<uint8_t *>(&msg)));
}

/**
 * @tc.name: HiboxDtsMgmtCombTest001
 * @tc.desc: 校验DTS组包：能力消息写出一个能力TLV；datalen不符、空指针或非能力消息返回0
 * @tc.type: FUNC
 */
HWTEST_F(HiboxDtsTest, HiboxDtsMgmtCombTest001, TestSize.Level1)
{
    uint8_t buf[24] = {0};
    ParseMsgBuf arg;
    ClearMsg(arg);
    arg.ind.msgType = DTS_CAP;
    arg.ind.datalen = sizeof(DtsCapData);
    arg.bytes[0] = DTS_MGR_FEATURE_ICONNECT;
    uint16_t used = HiboxDtsMgmtReqComb(buf, sizeof(buf), &arg.ind);
    ASSERT_EQ(used, HIBOX_TLV_HEADER_LEN + sizeof(DtsCapData));
    ASSERT_EQ(buf[0], DTS_CAP_TYPE);
    ASSERT_EQ(buf[1], sizeof(DtsCapData));
    ASSERT_EQ(buf[3], DTS_MGR_FEATURE_ICONNECT);
    ASSERT_EQ(HiboxDtsMgmtRspComb(buf, sizeof(buf), &arg.ind), used);

    arg.ind.datalen = sizeof(DtsCapData) + 1;
    ASSERT_EQ(HiboxDtsMgmtReqComb(buf, sizeof(buf), &arg.ind), 0);
    arg.ind.datalen = sizeof(DtsCapData);
    ASSERT_EQ(HiboxDtsMgmtReqComb(nullptr, sizeof(buf), &arg.ind), 0);
    arg.ind.msgType = WEAR_STATUS;
    ASSERT_EQ(HiboxDtsMgmtReqComb(buf, sizeof(buf), &arg.ind), 0);
    ASSERT_EQ(HiboxDtsMgmtRspComb(buf, sizeof(buf), &arg.ind), 0);
}

class HiboxAudioTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}
};

/**
 * @tc.name: HiboxAudioMgmtReqParseTest001
 * @tc.desc: 校验音频请求解析入口：空TLV、长度不足TLV头、空出参、未注册命令一律拒绝
 * @tc.type: FUNC
 */
HWTEST_F(HiboxAudioTest, HiboxAudioMgmtReqParseTest001, TestSize.Level1)
{
    uint8_t buf[8];
    ParseMsgBuf msg;
    ClearMsg(msg);
    const uint8_t caps[AUDIO_CAP_LEN] = {0};
    uint16_t off = MakeTlv(buf, AUDIO_CAPS_TYPE, AUDIO_CAP_LEN, caps);
    ASSERT_FALSE(HiboxAudioMgmtReqParse(CMDID_AUDIO_CAP, nullptr, off, reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_FALSE(HiboxAudioMgmtReqParse(CMDID_AUDIO_CAP, buf, HIBOX_TLV_HEADER_LEN,
        reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_FALSE(HiboxAudioMgmtReqParse(CMDID_AUDIO_CAP, buf, off, nullptr));
    const uint8_t unknownCmd = 0x55;
    ASSERT_FALSE(HiboxAudioMgmtReqParse(unknownCmd, buf, off, reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_TRUE(HiboxAudioMgmtReqParse(CMDID_AUDIO_CAP, buf, off, reinterpret_cast<uint8_t *>(&msg)));
}

/**
 * @tc.name: HiboxAudioMgmtReqParseTest002
 * @tc.desc: 校验音频能力解析：五个TLV按精确长度(4/1/2/6/35)全部落位；长度不符作废；部分填写仍报满结构体
 * @tc.type: FUNC
 */
HWTEST_F(HiboxAudioTest, HiboxAudioMgmtReqParseTest002, TestSize.Level1)
{
    uint8_t buf[96];
    ParseMsgBuf msg;
    ClearMsg(msg);
    const uint8_t caps[AUDIO_CAP_LEN] = {0x0F, 0x00, 0x00, 0x00};
    const uint8_t volume[AUDIO_ABSVOLUME_LEN] = {0x64};
    const uint8_t join[AUDIO_INTERVAL_LEN] = {0x20, 0x1E};
    const uint8_t ddm[AUDIO_DDM_LATENCY_LEN] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    uint8_t sle[AUDIO_SLE_INTERVAL_LEN] = {0};
    uint16_t sleOff = 0;
    sle[sleOff++] = 0xF0; // 延迟偏移低字节
    sle[sleOff++] = 0xFF; // 延迟偏移高字节
    sle[sleOff++] = 0x02; // 场景数
    for (uint8_t i = 0; i < MAX_SCENE_NUM; i++) {
        sle[sleOff++] = i;
        sle[sleOff++] = static_cast<uint8_t>(i + 1);
    }
    uint16_t off = 0;
    off += MakeTlv(buf + off, AUDIO_CAPS_TYPE, AUDIO_CAP_LEN, caps);
    off += MakeTlv(buf + off, AUDIO_ABS_VOLUME_TYPE, AUDIO_ABSVOLUME_LEN, volume);
    off += MakeTlv(buf + off, AUDIO_JOIN_INTERVAL_TYPE, AUDIO_INTERVAL_LEN, join);
    off += MakeTlv(buf + off, AUDIO_DDM_INTERVAL_TYPE, AUDIO_DDM_LATENCY_LEN, ddm);
    off += MakeTlv(buf + off, AUDIO_SLE_INTERVAL_TYPE, AUDIO_SLE_INTERVAL_LEN, sle);
    ASSERT_TRUE(HiboxAudioMgmtReqParse(CMDID_AUDIO_CAP, buf, off, reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_EQ(msg.ind.msgType, AUDIO_CAP_QUERY);
    ASSERT_EQ(msg.ind.datalen, sizeof(DeviceAudioCapsInfo));
    DeviceAudioCapsInfo *info = reinterpret_cast<DeviceAudioCapsInfo *>(msg.bytes);
    ASSERT_EQ(info->caps, 0x0F);
    ASSERT_EQ(info->absVolume, 0x64);
    ASSERT_EQ(info->joinIntervalMs, 0x1E20);
    ASSERT_EQ(info->musicIntervalMs, 0x0201);
    ASSERT_EQ(info->hwaIntervalMs, 0x0403);
    ASSERT_EQ(info->otherIntervalMs, 0x0605);
    ASSERT_EQ(info->sleConfig.offset, static_cast<int16_t>(0xFFF0));
    ASSERT_EQ(info->sleConfig.sceneNum, 0x02);
    ASSERT_EQ(info->sleConfig.timesMap[0].scene, 0);
    ASSERT_EQ(info->sleConfig.timesMap[0].times, 1);

    // 未知类型TLV：default放行，累计长度不足以凑满结构体仍报满
    ClearMsg(msg);
    off = MakeTlv(buf, 0x55, 1, caps);
    ASSERT_TRUE(HiboxAudioMgmtReqParse(CMDID_AUDIO_CAP, buf, off, reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_EQ(msg.ind.datalen, sizeof(DeviceAudioCapsInfo));

    // 能力TLV长度错一位：整条作废
    const uint8_t badCaps[AUDIO_CAP_LEN - 1] = {0};
    ClearMsg(msg);
    off = MakeTlv(buf, AUDIO_CAPS_TYPE, sizeof(badCaps), badCaps);
    ASSERT_TRUE(HiboxAudioMgmtReqParse(CMDID_AUDIO_CAP, buf, off, reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_EQ(msg.ind.datalen, 0);
}

/**
 * @tc.name: HiboxAudioMgmtReqParseTest003
 * @tc.desc: 校验星闪音频档案解析：音频流类型/设备名(上限32字节)/业务类型三个TLV取值；超限或长度不符作废
 * @tc.type: FUNC
 */
HWTEST_F(HiboxAudioTest, HiboxAudioMgmtReqParseTest003, TestSize.Level1)
{
    uint8_t buf[128];
    ParseMsgBuf msg;
    ClearMsg(msg);
    const uint8_t audioType[4] = {1, 2, 3, 4};
    const uint8_t devName[NEARLINK_DEVICE_NAME_LEN] = "EARBUD";
    const uint8_t serviceType[1] = {AUDIO_A2DP_BUSINESS_TYPE};
    uint16_t off = 0;
    off += MakeTlv(buf + off, NEARLINK_PROFILE_AUDIO_TYPE, sizeof(audioType), audioType);
    off += MakeTlv(buf + off, NEARLINK_PROFILE_DEVICE_NAME_TYPE, sizeof(devName), devName);
    off += MakeTlv(buf + off, NEARLINK_PROFILE_STREAM_SERVICE_TYPE, sizeof(serviceType), serviceType);
    ASSERT_TRUE(HiboxAudioMgmtReqParse(CMDID_NEARLINK_PROFILE_STATE, buf, off,
        reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_EQ(msg.ind.msgType, NEARLINK_AUDIO_PROFILE);
    ASSERT_EQ(msg.ind.datalen, sizeof(HiBoxNearlinkProfile));
    HiBoxNearlinkProfile *profile = reinterpret_cast<HiBoxNearlinkProfile *>(msg.bytes);
    ASSERT_EQ(memcmp(profile->audioType, audioType, sizeof(audioType)), 0);
    ASSERT_EQ(memcmp(profile->targetDevName, devName, sizeof(devName)), 0);
    ASSERT_EQ(profile->audioServiceType, AUDIO_A2DP_BUSINESS_TYPE);

    // 音频流类型33字节：超上限拒绝
    const uint8_t longAudio[NEARLINK_STREAM_TYPE_NUM + 1] = {0};
    ClearMsg(msg);
    off = MakeTlv(buf, NEARLINK_PROFILE_AUDIO_TYPE, sizeof(longAudio), longAudio);
    ASSERT_TRUE(HiboxAudioMgmtReqParse(CMDID_NEARLINK_PROFILE_STATE, buf, off,
        reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_EQ(msg.ind.datalen, 0);

    // 设备名33字节：超上限拒绝
    const uint8_t longName[NEARLINK_DEVICE_NAME_LEN + 1] = {0};
    ClearMsg(msg);
    off = MakeTlv(buf, NEARLINK_PROFILE_DEVICE_NAME_TYPE, sizeof(longName), longName);
    ASSERT_TRUE(HiboxAudioMgmtReqParse(CMDID_NEARLINK_PROFILE_STATE, buf, off,
        reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_EQ(msg.ind.datalen, 0);

    // 业务类型长度不是1：拒绝
    const uint8_t badService[2] = {1, 2};
    ClearMsg(msg);
    off = MakeTlv(buf, NEARLINK_PROFILE_STREAM_SERVICE_TYPE, sizeof(badService), badService);
    ASSERT_TRUE(HiboxAudioMgmtReqParse(CMDID_NEARLINK_PROFILE_STATE, buf, off,
        reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_EQ(msg.ind.datalen, 0);

    // 未知类型TLV：default放行，解析仍成功
    ClearMsg(msg);
    off = MakeTlv(buf, 0x55, sizeof(audioType), audioType);
    ASSERT_TRUE(HiboxAudioMgmtReqParse(CMDID_NEARLINK_PROFILE_STATE, buf, off,
        reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_EQ(msg.ind.datalen, sizeof(HiBoxNearlinkProfile));
}

/**
 * @tc.name: HiboxAudioMgmtReqParseTest004
 * @tc.desc: 校验双录参数解析走扩展分发：编解码信息TLV(上限89字节)整体复制，超限作废
 * @tc.type: FUNC
 */
HWTEST_F(HiboxAudioTest, HiboxAudioMgmtReqParseTest004, TestSize.Level1)
{
    uint8_t buf[128];
    ParseMsgBuf msg;
    ClearMsg(msg);
    HiBoxNearlinkDualRecCaps caps;
    (void)memset_s(&caps, 0, sizeof(caps));
    caps.codecNum = DEVICE_DUAL_REC_CODEC_NUM;
    caps.param[0].codecId = 0x21;
    caps.param[0].sampleRate = 0xAC44;
    uint16_t off = MakeTlv(buf, NEARLINK_DUAL_REC_REQ_CODEC_INFO_TYPE, sizeof(caps),
        reinterpret_cast<const uint8_t *>(&caps));
    ASSERT_TRUE(HiboxAudioMgmtReqParse(CMDID_DUAL_REC_PARAM_REPORT, buf, off,
        reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_EQ(msg.ind.msgType, NEARLINK_DUAL_REC_PARAM);
    ASSERT_EQ(msg.ind.datalen, sizeof(HiBoxNearlinkDualRecCaps));
    HiBoxNearlinkDualRecCaps *parsed = reinterpret_cast<HiBoxNearlinkDualRecCaps *>(msg.bytes);
    ASSERT_EQ(parsed->codecNum, DEVICE_DUAL_REC_CODEC_NUM);
    ASSERT_EQ(parsed->param[0].codecId, 0x21);
    ASSERT_EQ(parsed->param[0].sampleRate, 0xAC44);

    // 超过结构体上限：作废
    const uint8_t overflow[sizeof(caps) + 1] = {0};
    ClearMsg(msg);
    off = MakeTlv(buf, NEARLINK_DUAL_REC_REQ_CODEC_INFO_TYPE, sizeof(overflow), overflow);
    ASSERT_TRUE(HiboxAudioMgmtReqParse(CMDID_DUAL_REC_PARAM_REPORT, buf, off,
        reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_EQ(msg.ind.datalen, 0);
}

/**
 * @tc.name: HiboxAudioMgmtRspParseTest001
 * @tc.desc: 校验音频应答解析只认出声路径命令，借通用结果TLV取回应答码，其余命令与坏输入拒绝
 * @tc.type: FUNC
 */
HWTEST_F(HiboxAudioTest, HiboxAudioMgmtRspParseTest001, TestSize.Level1)
{
    uint8_t buf[8];
    ParseMsgBuf msg;
    ClearMsg(msg);
    const uint8_t result[HIBOX_COMMON_MSG_VALUE_LEN] = {HIBOX_SUCCESS};
    uint16_t off = MakeTlv(buf, HIBOX_RSP_TYPE_RESULT, HIBOX_COMMON_MSG_VALUE_LEN, result);
    ASSERT_TRUE(HiboxAudioMgmtRspParse(CMDID_NEARLINK_OUTPUT_DATAPATH, buf, off,
        reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_EQ(msg.ind.msgType, NEARLINK_AUDIO_DATAPATH);
    ASSERT_EQ(msg.ind.datalen, sizeof(HiboxRspCommonResult));
    ASSERT_EQ(msg.bytes[0], HIBOX_SUCCESS);
    ASSERT_FALSE(HiboxAudioMgmtRspParse(CMDID_AUDIO_CAP, buf, off, reinterpret_cast<uint8_t *>(&msg)));
}

/**
 * @tc.name: HiboxAudioMgmtReqCombTest001
 * @tc.desc: 校验音频请求组包：出声路径消息写出一个音频类型TLV；datalen超12、空指针或无关消息返回0
 * @tc.type: FUNC
 */
HWTEST_F(HiboxAudioTest, HiboxAudioMgmtReqCombTest001, TestSize.Level1)
{
    uint8_t buf[48] = {0};
    ParseMsgBuf arg;
    ClearMsg(arg);
    arg.ind.msgType = NEARLINK_AUDIO_DATAPATH;
    arg.ind.datalen = sizeof(HiboxNearlinkDataPath);
    arg.bytes[0] = AUDIO_HFP_OUTPUT_DATAPATH_TYPE;
    uint16_t used = HiboxAudioMgmtReqComb(buf, sizeof(buf), &arg.ind);
    ASSERT_EQ(used, HIBOX_TLV_HEADER_LEN + sizeof(HiboxNearlinkDataPath));
    ASSERT_EQ(buf[0], NEARLINK_OUTPUT_DATAPATH_AUDIO_TYPE);
    ASSERT_EQ(buf[1], sizeof(HiboxNearlinkDataPath));
    ASSERT_EQ(buf[3], AUDIO_HFP_OUTPUT_DATAPATH_TYPE);

    arg.ind.datalen = sizeof(HiboxNearlinkDataPath) + 1;
    ASSERT_EQ(HiboxAudioMgmtReqComb(buf, sizeof(buf), &arg.ind), 0);
    arg.ind.datalen = sizeof(HiboxNearlinkDataPath);
    ASSERT_EQ(HiboxAudioMgmtReqComb(nullptr, sizeof(buf), &arg.ind), 0);
    arg.ind.msgType = AUDIO_CAP_QUERY;
    ASSERT_EQ(HiboxAudioMgmtReqComb(buf, sizeof(buf), &arg.ind), 0);
}

/**
 * @tc.name: HiboxAudioMgmtRspCombTest001
 * @tc.desc: 校验音频应答组包三分派：能力消息拼四个TLV、档案消息拼通用结果、双录消息拼编解码TLV
 * @tc.type: FUNC
 */
HWTEST_F(HiboxAudioTest, HiboxAudioMgmtRspCombTest001, TestSize.Level1)
{
    uint8_t buf[64] = {0};
    ParseMsgBuf arg;
    ClearMsg(arg);
    arg.ind.msgType = AUDIO_CAP_QUERY;
    arg.ind.datalen = sizeof(DeviceAudioCapsInfo);
    DeviceAudioCapsInfo *info = reinterpret_cast<DeviceAudioCapsInfo *>(arg.bytes);
    info->caps = 3;
    info->absVolume = 9;
    uint16_t used = HiboxAudioMgmtRspComb(buf, sizeof(buf), &arg.ind);
    // caps(4)+绝对音量(1)+入耳间隔(2)+ddm间隔(6)四个TLV
    ASSERT_EQ(used, 4 * HIBOX_TLV_HEADER_LEN + AUDIO_CAP_LEN + AUDIO_ABSVOLUME_LEN + AUDIO_INTERVAL_LEN +
        AUDIO_DDM_LATENCY_LEN);
    ASSERT_EQ(buf[0], AUDIO_CAPS_TYPE);
    ASSERT_EQ(buf[1], AUDIO_CAP_LEN);
    ASSERT_EQ(buf[3 + AUDIO_CAP_LEN], AUDIO_ABS_VOLUME_TYPE);

    // datalen不符：能力组包返回0
    arg.ind.datalen = sizeof(DeviceAudioCapsInfo) - 1;
    ASSERT_EQ(HiboxAudioMgmtRspComb(buf, sizeof(buf), &arg.ind), 0);

    // 档案消息走通用结果组包
    arg.ind.msgType = NEARLINK_AUDIO_PROFILE;
    arg.ind.datalen = HIBOX_COMMON_MSG_VALUE_LEN;
    arg.bytes[0] = HIBOX_SUCCESS;
    ASSERT_EQ(HiboxAudioMgmtRspComb(buf, sizeof(buf), &arg.ind),
        HIBOX_TLV_HEADER_LEN + HIBOX_COMMON_MSG_VALUE_LEN);

    // 双录消息走扩展组包
    arg.ind.msgType = NEARLINK_DUAL_REC_PARAM;
    arg.ind.datalen = sizeof(DeviceDualRecParmInfo);
    used = HiboxAudioMgmtRspComb(buf, sizeof(buf), &arg.ind);
    ASSERT_EQ(used, HIBOX_TLV_HEADER_LEN + sizeof(DeviceDualRecParmInfo));
    ASSERT_EQ(buf[0], NEARLINK_DUAL_REC_RSP_CODEC_INFO_TYPE);

    arg.ind.datalen = sizeof(DeviceDualRecParmInfo) + 1;
    ASSERT_EQ(HiboxAudioMgmtRspComb(buf, sizeof(buf), &arg.ind), 0);
    arg.ind.datalen = sizeof(DeviceDualRecParmInfo);
    ASSERT_EQ(HiboxAudioMgmtRspComb(nullptr, sizeof(buf), &arg.ind), 0);
}
