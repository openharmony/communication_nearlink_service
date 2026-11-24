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
#include "hibox_local.h"
#include "hibox_pairconn.h"
#include "hibox_process.h"
}

using namespace testing::ext;

namespace {
// 解析出参背板：HiboxParseMsgInd 的 data 是零长数组，必须跟一块真实存储
struct ParseMsgBuf {
    HiboxParseMsgInd ind;
    uint8_t bytes[64];
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

class HiboxPairConnTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}
};

/**
 * @tc.name: HiboxPairConnReqParseTest001
 * @tc.desc: 校验配对请求解析入口：空TLV、长度不足TLV头、空出参、未注册命令一律拒绝，耳机特性命令放行
 * @tc.type: FUNC
 */
HWTEST_F(HiboxPairConnTest, HiboxPairConnReqParseTest001, TestSize.Level1)
{
    uint8_t buf[8];
    ParseMsgBuf msg;
    ClearMsg(msg);
    const uint8_t rightEar = 0x01;
    uint16_t off = MakeTlv(buf, EARBUDS_NATURE_TYPE, EARBUDS_NATURE_LEN, &rightEar);
    ASSERT_FALSE(HiboxPairConnMgmtReqParse(CMDID_EARBUDS_NATURE, nullptr, off,
        reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_FALSE(HiboxPairConnMgmtReqParse(CMDID_EARBUDS_NATURE, buf, HIBOX_TLV_HEADER_LEN,
        reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_FALSE(HiboxPairConnMgmtReqParse(CMDID_EARBUDS_NATURE, buf, off, nullptr));
    const uint8_t unknownCmd = 0x7E;
    ASSERT_FALSE(HiboxPairConnMgmtReqParse(unknownCmd, buf, off, reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_TRUE(HiboxPairConnMgmtReqParse(CMDID_EARBUDS_NATURE, buf, off,
        reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_EQ(msg.ind.msgType, EARBUDS_NATRUE);
    ASSERT_EQ(msg.ind.datalen, sizeof(EarbudsNature));
    ASSERT_EQ(msg.bytes[0], rightEar);
}

/**
 * @tc.name: HiboxPairConnReqParseTest002
 * @tc.desc: 校验耳机特性解析：值落位；长度不符整轮作废但结果仍按满结构体上报；累计超上限时长度归零
 * @tc.type: FUNC
 */
HWTEST_F(HiboxPairConnTest, HiboxPairConnReqParseTest002, TestSize.Level1)
{
    uint8_t buf[16];
    ParseMsgBuf msg;

    // 特性TLV长度不是1：本轮作废，datalen仍按满结构体填写
    const uint8_t two[2] = { 0x01, 0x01 };
    ClearMsg(msg);
    uint16_t off = MakeTlv(buf, EARBUDS_NATURE_TYPE, sizeof(two), two);
    ASSERT_TRUE(HiboxPairConnMgmtReqParse(CMDID_EARBUDS_NATURE, buf, off,
        reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_EQ(msg.ind.datalen, sizeof(EarbudsNature));
    ASSERT_EQ(msg.bytes[0], 0);

    // 未知类型TLV放行累计，值长度超过结构体上限：datalen保持0
    const uint8_t pad[2] = { 1, 2 };
    ClearMsg(msg);
    off = MakeTlv(buf, 0x55, sizeof(pad), pad);
    ASSERT_TRUE(HiboxPairConnMgmtReqParse(CMDID_EARBUDS_NATURE, buf, off,
        reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_EQ(msg.ind.datalen, 0);
}

/**
 * @tc.name: HiboxPairConnReqParseTest003
 * @tc.desc: 校验自动回连开关与断开通知解析：正确值落位、长度不符作废、累计超上限归零
 * @tc.type: FUNC
 */
HWTEST_F(HiboxPairConnTest, HiboxPairConnReqParseTest003, TestSize.Level1)
{
    uint8_t buf[16];
    ParseMsgBuf msg;

    const uint8_t switchOn = 0x01;
    ClearMsg(msg);
    uint16_t off = MakeTlv(buf, AUTO_CONN_SWITCH_TYPE, AUTO_CONN_SWITCH_LEN, &switchOn);
    ASSERT_TRUE(HiboxPairConnMgmtReqParse(CMDID_AUTO_CONN_SWITCH, buf, off,
        reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_EQ(msg.ind.msgType, AUTO_CONN_SWITCH_CFM);
    ASSERT_EQ(msg.ind.datalen, sizeof(MultConnAutoConnSwitch));
    ASSERT_EQ(msg.bytes[0], switchOn);

    // 开关TLV长度不是1：datalen仍按满结构体填写
    const uint8_t two[2] = { 0x01, 0x01 };
    ClearMsg(msg);
    off = MakeTlv(buf, AUTO_CONN_SWITCH_TYPE, sizeof(two), two);
    ASSERT_TRUE(HiboxPairConnMgmtReqParse(CMDID_AUTO_CONN_SWITCH, buf, off,
        reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_EQ(msg.ind.datalen, sizeof(MultConnAutoConnSwitch));
    ASSERT_EQ(msg.bytes[0], 0);

    // 未知类型累计超开关结构体上限：datalen归零
    ClearMsg(msg);
    off = MakeTlv(buf, 0x55, sizeof(two), two);
    ASSERT_TRUE(HiboxPairConnMgmtReqParse(CMDID_AUTO_CONN_SWITCH, buf, off,
        reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_EQ(msg.ind.datalen, 0);

    const uint8_t notifyValue = 0x01;
    ClearMsg(msg);
    off = MakeTlv(buf, NOTIFY_DISCONNECT_PROFILE, NOTIFY_DISCONNECT_PROFILE_LEN, &notifyValue);
    ASSERT_TRUE(HiboxPairConnMgmtReqParse(CMDID_NOTIFY_SLE_DISCONNECT_PROFILE, buf, off,
        reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_EQ(msg.ind.msgType, NOTIFY_SLE_DISCONNECT_PROFILE);
    ASSERT_EQ(msg.ind.datalen, sizeof(NotifySleDisconnectProfile));
    ASSERT_EQ(msg.bytes[0], notifyValue);

    // 断开通知TLV长度不是1：datalen仍按满结构体填写
    ClearMsg(msg);
    off = MakeTlv(buf, NOTIFY_DISCONNECT_PROFILE, sizeof(two), two);
    ASSERT_TRUE(HiboxPairConnMgmtReqParse(CMDID_NOTIFY_SLE_DISCONNECT_PROFILE, buf, off,
        reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_EQ(msg.ind.datalen, sizeof(NotifySleDisconnectProfile));

    // 未知类型累计超通知结构体上限：datalen归零
    const uint8_t pad[2] = { 1, 2 };
    ClearMsg(msg);
    off = MakeTlv(buf, 0x55, sizeof(pad), pad);
    ASSERT_TRUE(HiboxPairConnMgmtReqParse(CMDID_NOTIFY_SLE_DISCONNECT_PROFILE, buf, off,
        reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_EQ(msg.ind.datalen, 0);
}

/**
 * @tc.name: HiboxPairConnRspParseTest001
 * @tc.desc: 校验配对应答解析：查询结果取回类型与值、两个通用结果命令写对应消息类型、未注册命令与坏输入拒绝
 * @tc.type: FUNC
 */
HWTEST_F(HiboxPairConnTest, HiboxPairConnRspParseTest001, TestSize.Level1)
{
    uint8_t buf[8];
    ParseMsgBuf msg;

    const uint8_t typeValue = PAIR_CONN_ABNORMAL;
    ClearMsg(msg);
    uint16_t off = MakeTlv(buf, QUERY_CONN_RSP_TYPE, QUERY_CONN_LEN, &typeValue);
    ASSERT_TRUE(HiboxPairConnMgmtRspParse(CMDID_QUERY_CONN, buf, off,
        reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_EQ(msg.ind.msgType, QUERY_BUSINESS);
    ASSERT_EQ(msg.ind.datalen, sizeof(QueryPairConnCfmResult));
    QueryPairConnCfmResult *query = reinterpret_cast<QueryPairConnCfmResult *>(msg.bytes);
    ASSERT_EQ(query->msgType, QUERY_CONN_RSP_TYPE);
    ASSERT_EQ(query->typeValue, typeValue);

    // 查询结果TLV长度不是1：datalen仍按满结构体填写
    const uint8_t two[2] = { 0x01, 0x01 };
    ClearMsg(msg);
    off = MakeTlv(buf, QUERY_CONN_RSP_TYPE, sizeof(two), two);
    ASSERT_TRUE(HiboxPairConnMgmtRspParse(CMDID_QUERY_CONN, buf, off,
        reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_EQ(msg.ind.datalen, sizeof(QueryPairConnCfmResult));

    // 未知类型TLV放行累计，值长度超过结构体上限：datalen保持0
    const uint8_t pad[3] = { 1, 2, 3 };
    ClearMsg(msg);
    off = MakeTlv(buf, 0x55, sizeof(pad), pad);
    ASSERT_TRUE(HiboxPairConnMgmtRspParse(CMDID_QUERY_CONN, buf, off,
        reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_EQ(msg.ind.msgType, QUERY_BUSINESS);
    ASSERT_EQ(msg.ind.datalen, 0);

    const uint8_t result = HIBOX_ERR_PARA_ERROR;
    ClearMsg(msg);
    off = MakeTlv(buf, HIBOX_RSP_TYPE_RESULT, HIBOX_COMMON_MSG_VALUE_LEN, &result);
    ASSERT_TRUE(HiboxPairConnMgmtRspParse(CMDID_VENDOR_ACCOUNT_HASH, buf, off,
        reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_EQ(msg.ind.msgType, VENDOR_ACCOUNT_HASH);
    ASSERT_EQ(msg.ind.datalen, sizeof(HiboxRspCommonResult));
    ASSERT_EQ(msg.bytes[0], result);

    ClearMsg(msg);
    ASSERT_TRUE(HiboxPairConnMgmtRspParse(CMDID_SLE_ALL_PROFILE_CONNECTED, buf, off,
        reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_EQ(msg.ind.msgType, SLE_ALL_PROFILE_CONNECTED);

    ASSERT_FALSE(HiboxPairConnMgmtRspParse(CMDID_QUERY_CONN, nullptr, off,
        reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_FALSE(HiboxPairConnMgmtRspParse(CMDID_QUERY_CONN, buf, HIBOX_TLV_HEADER_LEN,
        reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_FALSE(HiboxPairConnMgmtRspParse(CMDID_QUERY_CONN, buf, off, nullptr));
    const uint8_t unknownCmd = 0x7E;
    ASSERT_FALSE(HiboxPairConnMgmtRspParse(unknownCmd, buf, off, reinterpret_cast<uint8_t *>(&msg)));
}

/**
 * @tc.name: HiboxPairConnReqCombTest001
 * @tc.desc: 校验配对组包三分发：业务查询拼三段TLV、账号哈希拼两段、连接完成拼一段；空指针与不符长度返回0
 * @tc.type: FUNC
 */
HWTEST_F(HiboxPairConnTest, HiboxPairConnReqCombTest001, TestSize.Level1)
{
    uint8_t buf[96] = {0};
    ParseMsgBuf arg;
    ClearMsg(arg);

    arg.ind.msgType = QUERY_BUSINESS;
    arg.ind.datalen = sizeof(QueryPairConn);
    QueryPairConn *pair = reinterpret_cast<QueryPairConn *>(arg.bytes);
    pair->connType = PAIR_CONN_ABNORMAL;
    (void)memset_s(pair->accountHash, sizeof(pair->accountHash), 0xAB, sizeof(pair->accountHash));
    pair->mobileBusinessType = 0x02;
    uint16_t used = HiboxPairConnMgmtReqComb(buf, sizeof(buf), &arg.ind);
    // 连接类型(1)+账号哈希(32)+业务类型(1)三段TLV
    ASSERT_EQ(used, 3 * HIBOX_TLV_HEADER_LEN + sizeof(QueryPairConn));
    ASSERT_EQ(buf[0], QUERY_BUSINESS_CONN_TYPE);
    ASSERT_EQ(buf[HIBOX_TLV_HEADER_LEN + 1], QUERY_BUSINESS_ACCOUNT_HASH_TYPE);

    ClearMsg(arg);
    (void)memset_s(buf, sizeof(buf), 0, sizeof(buf));
    arg.ind.msgType = VENDOR_ACCOUNT_HASH;
    arg.ind.datalen = sizeof(VendorAccountHash);
    VendorAccountHash *vendor = reinterpret_cast<VendorAccountHash *>(arg.bytes);
    vendor->deviceOsType = 0x01;
    (void)memset_s(vendor->accountHash, sizeof(vendor->accountHash), 0xCD, sizeof(vendor->accountHash));
    used = HiboxPairConnMgmtReqComb(buf, sizeof(buf), &arg.ind);
    // 系统类型(1)+账号哈希(32)两段TLV
    ASSERT_EQ(used, 2 * HIBOX_TLV_HEADER_LEN + sizeof(VendorAccountHash));
    ASSERT_EQ(buf[0], VENDOR_HONGMENG_OS_TYPE);
    ASSERT_EQ(buf[HIBOX_TLV_HEADER_LEN + 1], VENDOR_ACCOUNT_HASH_TYPE);

    ClearMsg(arg);
    arg.ind.msgType = SLE_ALL_PROFILE_CONNECTED;
    arg.ind.datalen = sizeof(HiboxSleProfileState);
    HiboxSleProfileState *state = reinterpret_cast<HiboxSleProfileState *>(arg.bytes);
    state->state = 0x01;
    used = HiboxPairConnMgmtReqComb(buf, sizeof(buf), &arg.ind);
    ASSERT_EQ(used, HIBOX_TLV_HEADER_LEN + SLE_ALL_PROFILE_CONNECTED_LEN);
    ASSERT_EQ(buf[0], ALL_AUDIO_PROFILE_CONNECTED);

    // 数据长度与结构体不符：组包作废
    arg.ind.datalen = sizeof(QueryPairConn) + 1;
    ASSERT_EQ(HiboxPairConnMgmtReqComb(buf, sizeof(buf), &arg.ind), 0);
    // 无关消息类型：无组包路径
    arg.ind.msgType = WEAR_STATUS;
    arg.ind.datalen = sizeof(QueryPairConn);
    ASSERT_EQ(HiboxPairConnMgmtReqComb(buf, sizeof(buf), &arg.ind), 0);
    ASSERT_EQ(HiboxPairConnMgmtReqComb(nullptr, sizeof(buf), &arg.ind), 0);
    ASSERT_EQ(HiboxPairConnMgmtReqComb(buf, sizeof(buf), nullptr), 0);
}
