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
#include "hibox_hitws.h"
#include "hibox_local.h"
#include "hibox_pairconn.h"
#include "hibox_process.h"
#include "hibox_service.h"
}

using namespace testing::ext;

namespace {
constexpr int FRAME_HEAD_LEN = 3;
constexpr int FRAME_TAIL_LEN = 2;

// 解析出参背板：HiboxParseMsgInd 的 data 是零长数组，必须跟一块真实存储
struct ParseMsgBuf {
    HiboxParseMsgInd ind;
    uint8_t bytes[700];
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

// 整帧构造：[帧头][服务号][命令号][TLV][CRC16低字节在前]，校验码覆盖帧头到TLV
uint16_t MakeFrame(uint8_t *buf, uint8_t header, uint8_t sid, uint8_t cmdId,
    const uint8_t *tlv, uint16_t tlvLen)
{
    buf[0] = header;
    buf[1] = sid;
    buf[2] = cmdId;
    if (tlvLen > 0) {
        (void)memcpy_s(buf + FRAME_HEAD_LEN, tlvLen, tlv, tlvLen);
    }
    uint16_t crc = HiboxL2cuDoCheckMic(buf, FRAME_HEAD_LEN + tlvLen);
    buf[FRAME_HEAD_LEN + tlvLen] = static_cast<uint8_t>(crc & 0xFF);
    buf[FRAME_HEAD_LEN + tlvLen + 1] = static_cast<uint8_t>(crc >> 8);
    return FRAME_HEAD_LEN + tlvLen + FRAME_TAIL_LEN;
}

// 收帧侧桩：把上报给业务层的内容原样留底
struct RecvCapture {
    bool called;
    uint8_t echoType;
    HiboxParseMsgInd ind;
    uint8_t bytes[320];
    uint16_t len;
    uint8_t *args;
};

void ResetCapture(RecvCapture &cap)
{
    (void)memset_s(&cap, sizeof(cap), 0, sizeof(cap));
}

RecvCapture g_indCap;
RecvCapture g_cfmCap;

void IndStub(uint8_t echoType, uint8_t *data, uint16_t len, uint8_t *args)
{
    g_indCap.called = true;
    g_indCap.echoType = echoType;
    g_indCap.len = len;
    g_indCap.args = args;
    (void)memcpy_s(&g_indCap.ind, sizeof(HiboxParseMsgInd), data, sizeof(HiboxParseMsgInd));
    if (len > sizeof(HiboxParseMsgInd)) {
        (void)memcpy_s(g_indCap.bytes, sizeof(g_indCap.bytes), data + sizeof(HiboxParseMsgInd),
            len - sizeof(HiboxParseMsgInd));
    }
}

void CfmStub(uint8_t echoType, uint8_t *data, uint16_t len, uint8_t *args)
{
    g_cfmCap.called = true;
    g_cfmCap.echoType = echoType;
    g_cfmCap.len = len;
    g_cfmCap.args = args;
    (void)memcpy_s(&g_cfmCap.ind, sizeof(HiboxParseMsgInd), data, sizeof(HiboxParseMsgInd));
    if (len > sizeof(HiboxParseMsgInd)) {
        (void)memcpy_s(g_cfmCap.bytes, sizeof(g_cfmCap.bytes), data + sizeof(HiboxParseMsgInd),
            len - sizeof(HiboxParseMsgInd));
    }
}

// 发帧侧桩：把发往对端的整帧留底
struct SendCapture {
    bool called;
    uint8_t frame[TLV_BUF_LEN + FRAME_HEAD_LEN + FRAME_TAIL_LEN];
    uint16_t len;
    void *peer;
};

void ResetCapture(SendCapture &cap)
{
    (void)memset_s(&cap, sizeof(cap), 0, sizeof(cap));
}

SendCapture g_reqCap;
SendCapture g_rspCap;

void SendReqStub(uint8_t *addr, uint8_t *reqData, uint16_t dataLen)
{
    g_reqCap.called = true;
    g_reqCap.len = dataLen;
    g_reqCap.peer = addr;
    (void)memcpy_s(g_reqCap.frame, sizeof(g_reqCap.frame), reqData, dataLen);
}

void SendRspStub(uint8_t *args, uint8_t *rspData, uint16_t dataLen)
{
    g_rspCap.called = true;
    g_rspCap.len = dataLen;
    g_rspCap.peer = args;
    (void)memcpy_s(g_rspCap.frame, sizeof(g_rspCap.frame), rspData, dataLen);
}
} // namespace

class HiboxProcessTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp()
    {
        ResetCapture(g_indCap);
        ResetCapture(g_cfmCap);
        ResetCapture(g_reqCap);
        ResetCapture(g_rspCap);
        // 产品里由适配层注册四个回调；用例注册留底桩对齐同一启动序列
        HiboxRegisterFunc func = { SendReqStub, SendRspStub, IndStub, CfmStub };
        HiboxRegisterProcess(&func);
    }
    void TearDown() {}
};

/**
 * @tc.name: HiboxProcessRecvIndTest001
 * @tc.desc: 校验收到合法的最小请求帧（佩戴状态、单值TLV）后按请求类型把解析结果原样上报业务层
 * @tc.type: FUNC
 */
HWTEST_F(HiboxProcessTest, HiboxProcessRecvIndTest001, TestSize.Level1)
{
    uint8_t tlv[8];
    const uint8_t inEar = 0x01;
    uint16_t tlvLen = MakeTlv(tlv, LEFT_WEAR_TYPE, LEFT_WEAR_TYPE_LEN, &inEar);
    uint8_t frame[16];
    uint16_t len = MakeFrame(frame, HIBOX_REQ_FIXED_HEADER, SID_DTS_MGMT, CMDID_WEAR_STATUS, tlv, tlvLen);
    uint8_t args = 0;
    ASSERT_EQ(len, 9);
    ASSERT_TRUE(HiboxRecvIndMsg(frame, len, &args));
    ASSERT_TRUE(g_indCap.called);
    ASSERT_EQ(g_indCap.echoType, HIBOX_REQ);
    ASSERT_EQ(g_indCap.ind.msgType, WEAR_STATUS);
    ASSERT_EQ(g_indCap.ind.datalen, sizeof(DtsWearData));
    ASSERT_EQ(g_indCap.bytes[0], inEar);
    ASSERT_EQ(g_indCap.len, sizeof(HiboxParseMsgInd) + sizeof(DtsWearData));
    ASSERT_EQ(g_indCap.args, &args);
}

/**
 * @tc.name: HiboxProcessRecvIndTest002
 * @tc.desc: 校验请求帧入口对坏输入一律拒绝：空指针、零长度、错帧头、应答帧头混入、校验码错、TLV区只剩头
 * @tc.type: FUNC
 */
HWTEST_F(HiboxProcessTest, HiboxProcessRecvIndTest002, TestSize.Level1)
{
    uint8_t tlv[8];
    const uint8_t inEar = 0x01;
    uint16_t tlvLen = MakeTlv(tlv, LEFT_WEAR_TYPE, LEFT_WEAR_TYPE_LEN, &inEar);
    uint8_t frame[16];
    uint16_t len = MakeFrame(frame, HIBOX_REQ_FIXED_HEADER, SID_DTS_MGMT, CMDID_WEAR_STATUS, tlv, tlvLen);
    uint8_t args = 0;

    ASSERT_FALSE(HiboxRecvIndMsg(nullptr, len, &args));
    ASSERT_FALSE(HiboxRecvIndMsg(frame, 0, &args));
    ASSERT_FALSE(HiboxRecvIndMsg(frame, len, nullptr));

    uint8_t badHead[16];
    (void)memcpy_s(badHead, sizeof(badHead), frame, len);
    badHead[0] = HIBOX_REQ_FIXED_HEADER - 1;
    ASSERT_FALSE(HiboxRecvIndMsg(badHead, len, &args));

    // 应答帧头混进请求通道
    uint8_t crossHead[16];
    (void)memcpy_s(crossHead, sizeof(crossHead), frame, len);
    crossHead[0] = HIBOX_RSP_FIXED_HEADER;
    ASSERT_FALSE(HiboxRecvIndMsg(crossHead, len, &args));

    uint8_t badMic[16];
    (void)memcpy_s(badMic, sizeof(badMic), frame, len);
    badMic[len - 1] ^= 0xFF;
    ASSERT_FALSE(HiboxRecvIndMsg(badMic, len, &args));

    // 帧长8：TLV区只剩3字节头，无值可解析
    uint8_t tlvHeadOnly[HIBOX_TLV_HEADER_LEN] = { LEFT_WEAR_TYPE, 0x01, 0x00 };
    uint8_t bare[16];
    uint16_t bareLen = MakeFrame(bare, HIBOX_REQ_FIXED_HEADER, SID_DTS_MGMT, CMDID_WEAR_STATUS,
        tlvHeadOnly, sizeof(tlvHeadOnly));
    ASSERT_EQ(bareLen, 8);
    ASSERT_FALSE(HiboxRecvIndMsg(bare, bareLen, &args));
    ASSERT_FALSE(g_indCap.called);
}

/**
 * @tc.name: HiboxProcessRecvIndTest003
 * @tc.desc: 校验服务号与命令号不在消息表内时请求帧被拒，不上报业务层
 * @tc.type: FUNC
 */
HWTEST_F(HiboxProcessTest, HiboxProcessRecvIndTest003, TestSize.Level1)
{
    uint8_t tlv[8];
    const uint8_t inEar = 0x01;
    uint16_t tlvLen = MakeTlv(tlv, LEFT_WEAR_TYPE, LEFT_WEAR_TYPE_LEN, &inEar);
    uint8_t frame[16];
    uint16_t len = MakeFrame(frame, HIBOX_REQ_FIXED_HEADER, SID_SCENE_MGMT, CMDID_CAPABILITY, tlv, tlvLen);
    uint8_t args = 0;
    ASSERT_FALSE(HiboxRecvIndMsg(frame, len, &args));
    ASSERT_FALSE(g_indCap.called);
}

/**
 * @tc.name: HiboxProcessRecvIndTest004
 * @tc.desc: 校验请求帧解析结果为空（未知TLV累计超结构体上限）时整帧作废，不上报业务层
 * @tc.type: FUNC
 */
HWTEST_F(HiboxProcessTest, HiboxProcessRecvIndTest004, TestSize.Level1)
{
    uint8_t tlv[16];
    const uint8_t pad[4] = { 1, 2, 3, 4 };
    uint16_t tlvLen = MakeTlv(tlv, 0x55, sizeof(pad), pad);
    uint8_t frame[16];
    uint16_t len = MakeFrame(frame, HIBOX_REQ_FIXED_HEADER, SID_DTS_MGMT, CMDID_WEAR_STATUS, tlv, tlvLen);
    uint8_t args = 0;
    ASSERT_FALSE(HiboxRecvIndMsg(frame, len, &args));
    ASSERT_FALSE(g_indCap.called);
}

/**
 * @tc.name: HiboxProcessRecvIndTest005
 * @tc.desc: 校验请求帧按服务号分发到角色切换、耳机特性、音频能力三个解析器并各自落位
 * @tc.type: FUNC
 */
HWTEST_F(HiboxProcessTest, HiboxProcessRecvIndTest005, TestSize.Level1)
{
    uint8_t args = 0;

    // 角色切换服务：消息类型与角色值落位
    uint8_t handoverTlv[8];
    const uint8_t role = 0x02;
    uint16_t tlvLen = MakeTlv(handoverTlv, HITWS_ISO_HANDOVER_TYPE, HIBOX_ISO_COMMON_LEN, &role);
    uint8_t frame[16];
    uint16_t len = MakeFrame(frame, HIBOX_REQ_FIXED_HEADER, SID_HITWS_CTRL,
        CMDID_HITWS_ROLE_HANDOVER_INFORM, handoverTlv, tlvLen);
    ASSERT_TRUE(HiboxRecvIndMsg(frame, len, &args));
    ASSERT_TRUE(g_indCap.called);
    ASSERT_EQ(g_indCap.ind.msgType, HITWS_ISO_HANDOVER);
    ASSERT_EQ(g_indCap.ind.datalen, sizeof(HitwsIsoHandover));
    ASSERT_EQ(g_indCap.bytes[0], HITWS_ISO_HANDOVER_TYPE);
    ASSERT_EQ(g_indCap.bytes[1], role);

    // 配对连接服务：耳机特性值落位
    ResetCapture(g_indCap);
    uint8_t natureTlv[8];
    const uint8_t rightEar = 0x01;
    tlvLen = MakeTlv(natureTlv, EARBUDS_NATURE_TYPE, EARBUDS_NATURE_LEN, &rightEar);
    len = MakeFrame(frame, HIBOX_REQ_FIXED_HEADER, SID_PAIR_CONN, CMDID_EARBUDS_NATURE, natureTlv, tlvLen);
    ASSERT_TRUE(HiboxRecvIndMsg(frame, len, &args));
    ASSERT_TRUE(g_indCap.called);
    ASSERT_EQ(g_indCap.ind.msgType, EARBUDS_NATRUE);
    ASSERT_EQ(g_indCap.ind.datalen, sizeof(EarbudsNature));
    ASSERT_EQ(g_indCap.bytes[0], rightEar);

    // 音频服务：能力位图按小端落进结构体首字段
    ResetCapture(g_indCap);
    uint8_t capTlv[8];
    const uint8_t caps[AUDIO_CAP_LEN] = { 0x0F, 0, 0, 0 };
    tlvLen = MakeTlv(capTlv, AUDIO_CAPS_TYPE, AUDIO_CAP_LEN, caps);
    len = MakeFrame(frame, HIBOX_REQ_FIXED_HEADER, SID_AUDIO_MGMT, CMDID_AUDIO_CAP, capTlv, tlvLen);
    ASSERT_TRUE(HiboxRecvIndMsg(frame, len, &args));
    ASSERT_TRUE(g_indCap.called);
    ASSERT_EQ(g_indCap.ind.msgType, AUDIO_CAP_QUERY);
    ASSERT_EQ(g_indCap.ind.datalen, sizeof(DeviceAudioCapsInfo));
    ASSERT_EQ(g_indCap.bytes[0], 0x0F);
}

/**
 * @tc.name: HiboxProcessRecvIndTest006
 * @tc.desc: 校验服务号无解析入口时（服务管理号）请求帧虽查表通过仍被拒收
 * @tc.type: FUNC
 */
HWTEST_F(HiboxProcessTest, HiboxProcessRecvIndTest006, TestSize.Level1)
{
    uint8_t tlv[8];
    const uint8_t protocol[DEVICE_PROTOCOL_LEN] = { 0x10, 0x00 };
    uint16_t tlvLen = MakeTlv(tlv, PROTOCOL_TYPE, DEVICE_PROTOCOL_LEN, protocol);
    uint8_t frame[16];
    uint16_t len = MakeFrame(frame, HIBOX_REQ_FIXED_HEADER, SID_SEVICE_MGMT, CMDID_DEVICE_ABILITY, tlv, tlvLen);
    uint8_t args = 0;
    ASSERT_FALSE(HiboxRecvIndMsg(frame, len, &args));
    ASSERT_FALSE(g_indCap.called);
}

/**
 * @tc.name: HiboxProcessRecvCfmTest007
 * @tc.desc: 校验收到合法应答帧后按应答类型把解析结果上报业务层：厂商哈希、厂商能力、能力上报、出声路径四类
 * @tc.type: FUNC
 */
HWTEST_F(HiboxProcessTest, HiboxProcessRecvCfmTest007, TestSize.Level1)
{
    uint8_t tlv[8];
    uint8_t frame[16];
    uint8_t args = 0;

    const uint8_t result = HIBOX_ERR_PARA_ERROR;
    uint16_t tlvLen = MakeTlv(tlv, HIBOX_RSP_TYPE_RESULT, HIBOX_COMMON_MSG_VALUE_LEN, &result);
    uint16_t len = MakeFrame(frame, HIBOX_RSP_FIXED_HEADER, SID_PAIR_CONN, CMDID_VENDOR_ACCOUNT_HASH,
        tlv, tlvLen);
    ASSERT_TRUE(HiboxRecvCfmMsg(frame, len, &args));
    ASSERT_TRUE(g_cfmCap.called);
    ASSERT_EQ(g_cfmCap.echoType, HIBOX_RSP);
    ASSERT_EQ(g_cfmCap.ind.msgType, VENDOR_ACCOUNT_HASH);
    ASSERT_EQ(g_cfmCap.ind.datalen, sizeof(HiboxRspCommonResult));
    ASSERT_EQ(g_cfmCap.bytes[0], result);
    ASSERT_EQ(g_cfmCap.len, sizeof(HiboxParseMsgInd) + sizeof(HiboxRspCommonResult));
    ASSERT_EQ(g_cfmCap.args, &args);

    // 厂商能力应答：数据长度按实际收到的值长度（2字节）填写
    ResetCapture(g_cfmCap);
    const uint8_t protocol[DEVICE_PROTOCOL_LEN] = { 0x10, 0x01 };
    tlvLen = MakeTlv(tlv, PROTOCOL_TYPE, DEVICE_PROTOCOL_LEN, protocol);
    len = MakeFrame(frame, HIBOX_RSP_FIXED_HEADER, SID_SEVICE_MGMT, CMDID_DEVICE_ABILITY, tlv, tlvLen);
    ASSERT_TRUE(HiboxRecvCfmMsg(frame, len, &args));
    ASSERT_TRUE(g_cfmCap.called);
    ASSERT_EQ(g_cfmCap.ind.msgType, DEVICE_MANUFACTURE_ABILITY);
    ASSERT_EQ(g_cfmCap.ind.datalen, DEVICE_PROTOCOL_LEN);
    ASSERT_EQ(g_cfmCap.bytes[0], 0x10);
    ASSERT_EQ(g_cfmCap.len, sizeof(HiboxParseMsgInd) + sizeof(ManufacturerAbilityInfo));

    // 能力上报应答：同样按实际值长度填写，能力字节原样落位
    ResetCapture(g_cfmCap);
    const uint8_t caps[2] = { DTS_MGR_FEATURE_VID, DTS_MGR_FEATURE_FOTA };
    tlvLen = MakeTlv(tlv, DTS_CAP_TYPE, sizeof(caps), caps);
    len = MakeFrame(frame, HIBOX_RSP_FIXED_HEADER, SID_DTS_MGMT, CMDID_CAPABILITY, tlv, tlvLen);
    ASSERT_TRUE(HiboxRecvCfmMsg(frame, len, &args));
    ASSERT_TRUE(g_cfmCap.called);
    ASSERT_EQ(g_cfmCap.ind.msgType, DTS_CAP);
    ASSERT_EQ(g_cfmCap.ind.datalen, sizeof(caps));
    ASSERT_EQ(g_cfmCap.bytes[0], DTS_MGR_FEATURE_VID);
    ASSERT_EQ(g_cfmCap.bytes[1], DTS_MGR_FEATURE_FOTA);
    ASSERT_EQ(g_cfmCap.len, sizeof(HiboxParseMsgInd) + sizeof(DtsCapData));

    // 出声路径应答：借通用结果TLV取回应答码
    ResetCapture(g_cfmCap);
    const uint8_t ok = HIBOX_SUCCESS;
    tlvLen = MakeTlv(tlv, HIBOX_RSP_TYPE_RESULT, HIBOX_COMMON_MSG_VALUE_LEN, &ok);
    len = MakeFrame(frame, HIBOX_RSP_FIXED_HEADER, SID_AUDIO_MGMT, CMDID_NEARLINK_OUTPUT_DATAPATH,
        tlv, tlvLen);
    ASSERT_TRUE(HiboxRecvCfmMsg(frame, len, &args));
    ASSERT_TRUE(g_cfmCap.called);
    ASSERT_EQ(g_cfmCap.ind.msgType, NEARLINK_AUDIO_DATAPATH);
    ASSERT_EQ(g_cfmCap.ind.datalen, sizeof(HiboxRspCommonResult));
    ASSERT_EQ(g_cfmCap.bytes[0], ok);
    ASSERT_EQ(g_cfmCap.len, sizeof(HiboxParseMsgInd) + sizeof(HiboxHdapCommonRsp));
}

/**
 * @tc.name: HiboxProcessRecvCfmTest008
 * @tc.desc: 校验应答帧入口对坏输入一律拒绝：空指针、零长度、错帧头、请求帧头混入、校验码错、TLV区只剩头
 * @tc.type: FUNC
 */
HWTEST_F(HiboxProcessTest, HiboxProcessRecvCfmTest008, TestSize.Level1)
{
    uint8_t tlv[8];
    const uint8_t result = HIBOX_SUCCESS;
    uint16_t tlvLen = MakeTlv(tlv, HIBOX_RSP_TYPE_RESULT, HIBOX_COMMON_MSG_VALUE_LEN, &result);
    uint8_t frame[16];
    uint16_t len = MakeFrame(frame, HIBOX_RSP_FIXED_HEADER, SID_PAIR_CONN, CMDID_VENDOR_ACCOUNT_HASH,
        tlv, tlvLen);
    uint8_t args = 0;

    ASSERT_FALSE(HiboxRecvCfmMsg(nullptr, len, &args));
    ASSERT_FALSE(HiboxRecvCfmMsg(frame, 0, &args));
    ASSERT_FALSE(HiboxRecvCfmMsg(frame, len, nullptr));

    uint8_t badHead[16];
    (void)memcpy_s(badHead, sizeof(badHead), frame, len);
    badHead[0] = HIBOX_RSP_FIXED_HEADER + 1;
    ASSERT_FALSE(HiboxRecvCfmMsg(badHead, len, &args));

    // 请求帧头混进应答通道
    uint8_t crossHead[16];
    (void)memcpy_s(crossHead, sizeof(crossHead), frame, len);
    crossHead[0] = HIBOX_REQ_FIXED_HEADER;
    ASSERT_FALSE(HiboxRecvCfmMsg(crossHead, len, &args));

    uint8_t badMic[16];
    (void)memcpy_s(badMic, sizeof(badMic), frame, len);
    badMic[len - 1] ^= 0xFF;
    ASSERT_FALSE(HiboxRecvCfmMsg(badMic, len, &args));

    uint8_t tlvHeadOnly[HIBOX_TLV_HEADER_LEN] = { HIBOX_RSP_TYPE_RESULT, 0x01, 0x00 };
    uint8_t bare[16];
    uint16_t bareLen = MakeFrame(bare, HIBOX_RSP_FIXED_HEADER, SID_PAIR_CONN, CMDID_VENDOR_ACCOUNT_HASH,
        tlvHeadOnly, sizeof(tlvHeadOnly));
    ASSERT_EQ(bareLen, 8);
    ASSERT_FALSE(HiboxRecvCfmMsg(bare, bareLen, &args));
    ASSERT_FALSE(g_cfmCap.called);
}

/**
 * @tc.name: HiboxProcessRecvCfmTest009
 * @tc.desc: 校验应答帧三类拒收：命令不在表内、解析结果为空、服务号无应答解析入口
 * @tc.type: FUNC
 */
HWTEST_F(HiboxProcessTest, HiboxProcessRecvCfmTest009, TestSize.Level1)
{
    uint8_t tlv[16];
    uint8_t frame[16];
    uint8_t args = 0;

    const uint8_t result = HIBOX_SUCCESS;
    uint16_t tlvLen = MakeTlv(tlv, HIBOX_RSP_TYPE_RESULT, HIBOX_COMMON_MSG_VALUE_LEN, &result);
    uint16_t len = MakeFrame(frame, HIBOX_RSP_FIXED_HEADER, SID_SCENE_MGMT, CMDID_CAPABILITY, tlv, tlvLen);
    ASSERT_FALSE(HiboxRecvCfmMsg(frame, len, &args));

    // 通用结果只容1字节值，未知TLV累计2字节后结果为空，整帧作废
    const uint8_t pad[2] = { 1, 2 };
    tlvLen = MakeTlv(tlv, 0x55, sizeof(pad), pad);
    len = MakeFrame(frame, HIBOX_RSP_FIXED_HEADER, SID_PAIR_CONN, CMDID_VENDOR_ACCOUNT_HASH, tlv, tlvLen);
    ASSERT_FALSE(HiboxRecvCfmMsg(frame, len, &args));

    // 角色切换服务没有应答解析入口
    const uint8_t role = 0x01;
    tlvLen = MakeTlv(tlv, HITWS_ISO_HANDOVER_TYPE, HIBOX_ISO_COMMON_LEN, &role);
    len = MakeFrame(frame, HIBOX_RSP_FIXED_HEADER, SID_HITWS_CTRL, CMDID_HITWS_ROLE_HANDOVER_INFORM,
        tlv, tlvLen);
    ASSERT_FALSE(HiboxRecvCfmMsg(frame, len, &args));
    ASSERT_FALSE(g_cfmCap.called);
}

/**
 * @tc.name: HiboxProcessCombReqTest010
 * @tc.desc: 校验请求组包：能力消息产出完整请求帧并交发送回调；空指针、超短超长、长度不符、未知消息、无组包路径一律拒绝
 * @tc.type: FUNC
 */
HWTEST_F(HiboxProcessTest, HiboxProcessCombReqTest010, TestSize.Level1)
{
    ParseMsgBuf source;
    ClearMsg(source);
    source.ind.msgType = DTS_CAP;
    source.ind.datalen = sizeof(DtsCapData);
    const uint8_t caps[DTS_MAX_CAP_LEN] = { DTS_MGR_FEATURE_VID, DTS_MGR_FEATURE_FOTA };
    (void)memcpy_s(source.bytes, sizeof(caps), caps, sizeof(caps));
    uint16_t dataLen = sizeof(HiboxParseMsgInd) + sizeof(DtsCapData);
    uint8_t addr = 0;

    ASSERT_TRUE(HiboxCombReqMsg(&addr, reinterpret_cast<uint8_t *>(&source), dataLen));
    ASSERT_TRUE(g_reqCap.called);
    ASSERT_EQ(g_reqCap.peer, &addr);
    ASSERT_EQ(g_reqCap.len, FRAME_HEAD_LEN + (HIBOX_TLV_HEADER_LEN + sizeof(DtsCapData)) + FRAME_TAIL_LEN);
    ASSERT_EQ(g_reqCap.frame[0], HIBOX_REQ_FIXED_HEADER);
    ASSERT_EQ(g_reqCap.frame[1], SID_DTS_MGMT);
    ASSERT_EQ(g_reqCap.frame[2], CMDID_CAPABILITY);
    ASSERT_EQ(g_reqCap.frame[3], DTS_CAP_TYPE);
    // TLV载荷逐字节：长度10、能力字节落位
    ASSERT_EQ(g_reqCap.frame[4], sizeof(DtsCapData));
    ASSERT_EQ(g_reqCap.frame[5], 0);
    ASSERT_EQ(g_reqCap.frame[6], DTS_MGR_FEATURE_VID);
    ASSERT_EQ(g_reqCap.frame[7], DTS_MGR_FEATURE_FOTA);
    // 校验码期望用独立手算常量锚定，不用产品函数现算
    const uint16_t expectMic = 0x11AF;
    uint16_t tail = static_cast<uint16_t>(g_reqCap.frame[g_reqCap.len - FRAME_TAIL_LEN]) |
        static_cast<uint16_t>(g_reqCap.frame[g_reqCap.len - 1] << 8);
    ASSERT_EQ(tail, expectMic);

    ASSERT_FALSE(HiboxCombReqMsg(nullptr, reinterpret_cast<uint8_t *>(&source), dataLen));
    ASSERT_FALSE(HiboxCombReqMsg(&addr, nullptr, dataLen));
    ASSERT_FALSE(HiboxCombReqMsg(&addr, reinterpret_cast<uint8_t *>(&source), HIBOX_SOURCE_DATA_HEADER_LEN));
    // 源数据总长超过组包缓冲上限
    ASSERT_FALSE(HiboxCombReqMsg(&addr, reinterpret_cast<uint8_t *>(&source), TLV_BUF_LEN + 1));

    ParseMsgBuf wrong;
    ClearMsg(wrong);
    wrong.ind.msgType = DTS_CAP;
    wrong.ind.datalen = sizeof(DtsCapData);
    // 源数据自带长度与传入总长不符
    ASSERT_FALSE(HiboxCombReqMsg(&addr, reinterpret_cast<uint8_t *>(&wrong), dataLen + 1));
    // 消息类型不在映射表内
    wrong.ind.msgType = 0x7E;
    ASSERT_FALSE(HiboxCombReqMsg(&addr, reinterpret_cast<uint8_t *>(&wrong), dataLen));
    // 佩戴状态消息没有请求组包路径
    wrong.ind.msgType = WEAR_STATUS;
    ASSERT_FALSE(HiboxCombReqMsg(&addr, reinterpret_cast<uint8_t *>(&wrong), dataLen));
}

/**
 * @tc.name: HiboxProcessCombReqTest011
 * @tc.desc: 校验请求组包分发：业务查询、厂商能力、出声路径三类消息各自产出正确服务号命令号与TLV总长
 * @tc.type: FUNC
 */
HWTEST_F(HiboxProcessTest, HiboxProcessCombReqTest011, TestSize.Level1)
{
    uint8_t addr = 0;
    ParseMsgBuf source;
    ClearMsg(source);

    // 业务查询：连接类型+账号哈希+业务类型三段TLV共43字节
    source.ind.msgType = QUERY_BUSINESS;
    source.ind.datalen = sizeof(QueryPairConn);
    QueryPairConn *pair = reinterpret_cast<QueryPairConn *>(source.bytes);
    pair->connType = PAIR_CONN_ABNORMAL;
    (void)memset_s(pair->accountHash, sizeof(pair->accountHash), 0xAB, sizeof(pair->accountHash));
    pair->mobileBusinessType = 0x02;
    ASSERT_TRUE(HiboxCombReqMsg(&addr, reinterpret_cast<uint8_t *>(&source),
        sizeof(HiboxParseMsgInd) + sizeof(QueryPairConn)));
    ASSERT_EQ(g_reqCap.frame[1], SID_PAIR_CONN);
    ASSERT_EQ(g_reqCap.frame[2], CMDID_QUERY_CONN);
    ASSERT_EQ(g_reqCap.len, FRAME_HEAD_LEN + 43 + FRAME_TAIL_LEN);

    // 厂商能力：协议版本+系统版本+能力位图三段TLV共29字节
    ResetCapture(g_reqCap);
    ClearMsg(source);
    source.ind.msgType = DEVICE_MANUFACTURE_ABILITY;
    source.ind.datalen = sizeof(ManufacturerAbilityInfo);
    ManufacturerAbilityInfo *ability = reinterpret_cast<ManufacturerAbilityInfo *>(source.bytes);
    ability->protocolType[0] = 0x10;
    ability->deviceOsType[0] = 0x0C;
    ability->manufacturerAbility[0] = 0x01;
    ASSERT_TRUE(HiboxCombReqMsg(&addr, reinterpret_cast<uint8_t *>(&source),
        sizeof(HiboxParseMsgInd) + sizeof(ManufacturerAbilityInfo)));
    ASSERT_EQ(g_reqCap.frame[1], SID_SEVICE_MGMT);
    ASSERT_EQ(g_reqCap.frame[2], CMDID_DEVICE_ABILITY);
    ASSERT_EQ(g_reqCap.len, FRAME_HEAD_LEN + 29 + FRAME_TAIL_LEN);

    // 出声路径：走音频组包通道，一段32字节音频类型TLV
    ResetCapture(g_reqCap);
    ClearMsg(source);
    source.ind.msgType = NEARLINK_AUDIO_DATAPATH;
    source.ind.datalen = sizeof(HiboxNearlinkDataPath);
    ASSERT_TRUE(HiboxCombReqMsg(&addr, reinterpret_cast<uint8_t *>(&source),
        sizeof(HiboxParseMsgInd) + sizeof(HiboxNearlinkDataPath)));
    ASSERT_EQ(g_reqCap.frame[1], SID_AUDIO_MGMT);
    ASSERT_EQ(g_reqCap.frame[2], CMDID_NEARLINK_OUTPUT_DATAPATH);
    ASSERT_EQ(g_reqCap.len, FRAME_HEAD_LEN + (HIBOX_TLV_HEADER_LEN + sizeof(HiboxNearlinkDataPath)) +
        FRAME_TAIL_LEN);
}

/**
 * @tc.name: HiboxProcessCombRspTest012
 * @tc.desc: 校验应答组包分发：能力、音频、角色切换三类消息产出应答帧；无应答组包路径与坏输入一律拒绝
 * @tc.type: FUNC
 */
HWTEST_F(HiboxProcessTest, HiboxProcessCombRspTest012, TestSize.Level1)
{
    uint8_t args = 0;
    ParseMsgBuf source;
    ClearMsg(source);

    source.ind.msgType = DTS_CAP;
    source.ind.datalen = sizeof(DtsCapData);
    uint16_t dataLen = sizeof(HiboxParseMsgInd) + sizeof(DtsCapData);
    ASSERT_TRUE(HiboxCombRspMsg(&args, reinterpret_cast<uint8_t *>(&source), dataLen));
    ASSERT_TRUE(g_rspCap.called);
    ASSERT_EQ(g_rspCap.frame[0], HIBOX_RSP_FIXED_HEADER);
    ASSERT_EQ(g_rspCap.frame[1], SID_DTS_MGMT);
    ASSERT_EQ(g_rspCap.frame[2], CMDID_CAPABILITY);

    // 音频能力：四段TLV（能力位图+绝对音量+入耳间隔+ddm间隔）共25字节
    ResetCapture(g_rspCap);
    ClearMsg(source);
    source.ind.msgType = AUDIO_CAP_QUERY;
    source.ind.datalen = sizeof(DeviceAudioCapsInfo);
    DeviceAudioCapsInfo *info = reinterpret_cast<DeviceAudioCapsInfo *>(source.bytes);
    info->caps = 3;
    info->absVolume = 9;
    dataLen = sizeof(HiboxParseMsgInd) + sizeof(DeviceAudioCapsInfo);
    ASSERT_TRUE(HiboxCombRspMsg(&args, reinterpret_cast<uint8_t *>(&source), dataLen));
    ASSERT_EQ(g_rspCap.frame[1], SID_AUDIO_MGMT);
    ASSERT_EQ(g_rspCap.frame[2], CMDID_AUDIO_CAP);
    ASSERT_EQ(g_rspCap.len, FRAME_HEAD_LEN + 25 + FRAME_TAIL_LEN);

    // 角色切换：一段角色TLV
    ResetCapture(g_rspCap);
    ClearMsg(source);
    source.ind.msgType = HITWS_ISO_HANDOVER;
    source.ind.datalen = sizeof(HitwsIsoHandover);
    HitwsIsoHandover *handover = reinterpret_cast<HitwsIsoHandover *>(source.bytes);
    handover->type = HITWS_ISO_HANDOVER_TYPE;
    handover->role = 0x02;
    dataLen = sizeof(HiboxParseMsgInd) + sizeof(HitwsIsoHandover);
    ASSERT_TRUE(HiboxCombRspMsg(&args, reinterpret_cast<uint8_t *>(&source), dataLen));
    ASSERT_EQ(g_rspCap.frame[1], SID_HITWS_CTRL);
    ASSERT_EQ(g_rspCap.frame[2], CMDID_HITWS_ROLE_HANDOVER_INFORM);
    ASSERT_EQ(g_rspCap.len, FRAME_HEAD_LEN + (HIBOX_TLV_HEADER_LEN + HIBOX_ISO_COMMON_LEN) + FRAME_TAIL_LEN);

    ASSERT_FALSE(HiboxCombRspMsg(nullptr, reinterpret_cast<uint8_t *>(&source), dataLen));
    ASSERT_FALSE(HiboxCombRspMsg(&args, nullptr, dataLen));
    ASSERT_FALSE(HiboxCombRspMsg(&args, reinterpret_cast<uint8_t *>(&source), HIBOX_SOURCE_DATA_HEADER_LEN));
    ASSERT_FALSE(HiboxCombRspMsg(&args, reinterpret_cast<uint8_t *>(&source), dataLen + 1));

    // 业务查询消息没有应答组包路径
    ClearMsg(source);
    source.ind.msgType = QUERY_BUSINESS;
    source.ind.datalen = sizeof(QueryPairConn);
    dataLen = sizeof(HiboxParseMsgInd) + sizeof(QueryPairConn);
    ASSERT_FALSE(HiboxCombRspMsg(&args, reinterpret_cast<uint8_t *>(&source), dataLen));

    source.ind.msgType = 0x7E;
    ASSERT_FALSE(HiboxCombRspMsg(&args, reinterpret_cast<uint8_t *>(&source), dataLen));
}
