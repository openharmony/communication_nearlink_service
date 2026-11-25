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
#include "hibox_hitws.h"
#include "hibox_local.h"
#include "hibox_service.h"
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

class HiboxHitwsTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}
};

/**
 * @tc.name: HiboxHitwsReqParseTest001
 * @tc.desc: 校验角色切换请求解析入口：空TLV、长度不足TLV头、空出参、未注册命令一律拒绝，切换命令放行并落位角色
 * @tc.type: FUNC
 */
HWTEST_F(HiboxHitwsTest, HiboxHitwsReqParseTest001, TestSize.Level1)
{
    uint8_t buf[8];
    ParseMsgBuf msg;
    ClearMsg(msg);
    const uint8_t role = 0x02;
    uint16_t off = MakeTlv(buf, HITWS_ISO_HANDOVER_TYPE, HIBOX_ISO_COMMON_LEN, &role);
    ASSERT_FALSE(HiboxHitwsManagerReqParse(CMDID_HITWS_ROLE_HANDOVER_INFORM, nullptr, off,
        reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_FALSE(HiboxHitwsManagerReqParse(CMDID_HITWS_ROLE_HANDOVER_INFORM, buf,
        HIBOX_TLV_HEADER_LEN, reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_FALSE(HiboxHitwsManagerReqParse(CMDID_HITWS_ROLE_HANDOVER_INFORM, buf, off, nullptr));
    const uint8_t unknownCmd = 0x7E;
    ASSERT_FALSE(HiboxHitwsManagerReqParse(unknownCmd, buf, off, reinterpret_cast<uint8_t *>(&msg)));

    ASSERT_TRUE(HiboxHitwsManagerReqParse(CMDID_HITWS_ROLE_HANDOVER_INFORM, buf, off,
        reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_EQ(msg.ind.msgType, HITWS_ISO_HANDOVER);
    ASSERT_EQ(msg.ind.datalen, sizeof(HitwsIsoHandover));
    HitwsIsoHandover *handover = reinterpret_cast<HitwsIsoHandover *>(msg.bytes);
    ASSERT_EQ(handover->type, HITWS_ISO_HANDOVER_TYPE);
    ASSERT_EQ(handover->role, role);
}

/**
 * @tc.name: HiboxHitwsReqParseTest002
 * @tc.desc: 校验角色切换解析：关闭切换类型也落位、两类TLV长度不符作废但按满结构体上报、累计超上限归零
 * @tc.type: FUNC
 */
HWTEST_F(HiboxHitwsTest, HiboxHitwsReqParseTest002, TestSize.Level1)
{
    uint8_t buf[16];
    ParseMsgBuf msg;

    // 关闭切换类型：类型值与角色落位
    const uint8_t role = 0x01;
    ClearMsg(msg);
    uint16_t off = MakeTlv(buf, HITWS_ISO_HANDOVER_CLOSE_TYPE, HIBOX_ISO_COMMON_LEN, &role);
    ASSERT_TRUE(HiboxHitwsManagerReqParse(CMDID_HITWS_ROLE_HANDOVER_INFORM, buf, off,
        reinterpret_cast<uint8_t *>(&msg)));
    HitwsIsoHandover *handover = reinterpret_cast<HitwsIsoHandover *>(msg.bytes);
    ASSERT_EQ(handover->type, HITWS_ISO_HANDOVER_CLOSE_TYPE);
    ASSERT_EQ(handover->role, role);

    // 切换TLV长度不是1：本轮作废，datalen仍按满结构体填写
    const uint8_t two[2] = { 0x01, 0x01 };
    ClearMsg(msg);
    off = MakeTlv(buf, HITWS_ISO_HANDOVER_TYPE, sizeof(two), two);
    ASSERT_TRUE(HiboxHitwsManagerReqParse(CMDID_HITWS_ROLE_HANDOVER_INFORM, buf, off,
        reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_EQ(msg.ind.datalen, sizeof(HitwsIsoHandover));
    ASSERT_EQ(msg.bytes[0], 0);

    // 关闭切换TLV长度不是1：同样按满结构体上报
    ClearMsg(msg);
    off = MakeTlv(buf, HITWS_ISO_HANDOVER_CLOSE_TYPE, sizeof(two), two);
    ASSERT_TRUE(HiboxHitwsManagerReqParse(CMDID_HITWS_ROLE_HANDOVER_INFORM, buf, off,
        reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_EQ(msg.ind.datalen, sizeof(HitwsIsoHandover));

    // 未知类型TLV放行累计，值长度超过结构体上限：datalen保持0
    const uint8_t pad[3] = { 1, 2, 3 };
    ClearMsg(msg);
    off = MakeTlv(buf, 0x55, sizeof(pad), pad);
    ASSERT_TRUE(HiboxHitwsManagerReqParse(CMDID_HITWS_ROLE_HANDOVER_INFORM, buf, off,
        reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_EQ(msg.ind.datalen, 0);
}

/**
 * @tc.name: HiboxHitwsRspCombTest001
 * @tc.desc: 校验角色切换组包：数据齐整时按数据自带类型拼一段角色TLV；长度不符或无关消息返回0
 * @tc.type: FUNC
 */
HWTEST_F(HiboxHitwsTest, HiboxHitwsRspCombTest001, TestSize.Level1)
{
    uint8_t buf[16] = {0};
    ParseMsgBuf arg;
    ClearMsg(arg);
    arg.ind.msgType = HITWS_ISO_HANDOVER;
    arg.ind.datalen = sizeof(HitwsIsoHandover);
    HitwsIsoHandover *handover = reinterpret_cast<HitwsIsoHandover *>(arg.bytes);
    handover->type = HITWS_ISO_HANDOVER_CLOSE_TYPE;
    handover->role = 0x02;
    uint16_t used = HiboxHitwsManagerRspComb(buf, sizeof(buf), &arg.ind);
    // TLV类型取自数据自带类型，值只带角色一字节
    ASSERT_EQ(used, HIBOX_TLV_HEADER_LEN + HIBOX_ISO_COMMON_LEN);
    ASSERT_EQ(buf[0], HITWS_ISO_HANDOVER_CLOSE_TYPE);
    ASSERT_EQ(buf[3], 0x02);

    arg.ind.datalen = sizeof(HitwsIsoHandover) + 1;
    ASSERT_EQ(HiboxHitwsManagerRspComb(buf, sizeof(buf), &arg.ind), 0);
    arg.ind.msgType = DTS_CAP;
    arg.ind.datalen = sizeof(HitwsIsoHandover);
    ASSERT_EQ(HiboxHitwsManagerRspComb(buf, sizeof(buf), &arg.ind), 0);
    ASSERT_EQ(HiboxHitwsManagerRspComb(nullptr, sizeof(buf), &arg.ind), 0);
    ASSERT_EQ(HiboxHitwsManagerRspComb(buf, sizeof(buf), nullptr), 0);
}

class HiboxServiceMgmtTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}
};

/**
 * @tc.name: HiboxServiceMgmtRspParseTest001
 * @tc.desc: 校验厂商能力应答解析：协议版本、系统版本、能力位图三段按上限长度复制；未注册命令与坏输入拒绝
 * @tc.type: FUNC
 */
HWTEST_F(HiboxServiceMgmtTest, HiboxServiceMgmtRspParseTest001, TestSize.Level1)
{
    uint8_t buf[64];
    ParseMsgBuf msg;
    ClearMsg(msg);
    const uint8_t protocol[DEVICE_PROTOCOL_LEN] = { 0x10, 0x01 };
    const uint8_t osType[DEVICE_OS_LEN] = { 0x0C, 0x02 };
    const uint8_t ability[DEVICE_MANUFACTURER_ABILITY_LEN] = { 0x01 };
    uint16_t off = 0;
    off += MakeTlv(buf + off, PROTOCOL_TYPE, DEVICE_PROTOCOL_LEN, protocol);
    off += MakeTlv(buf + off, OS_TYPE, DEVICE_OS_LEN, osType);
    off += MakeTlv(buf + off, MANUFACTURER_ABILITY_BIT_MAP, DEVICE_MANUFACTURER_ABILITY_LEN, ability);
    ASSERT_TRUE(HiboxServiceManagerRspParse(CMDID_DEVICE_ABILITY, buf, off,
        reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_EQ(msg.ind.msgType, DEVICE_MANUFACTURE_ABILITY);
    ASSERT_EQ(msg.ind.datalen, sizeof(ManufacturerAbilityInfo));
    ManufacturerAbilityInfo *info = reinterpret_cast<ManufacturerAbilityInfo *>(msg.bytes);
    ASSERT_EQ(memcmp(info->protocolType, protocol, sizeof(protocol)), 0);
    ASSERT_EQ(memcmp(info->deviceOsType, osType, sizeof(osType)), 0);
    ASSERT_EQ(memcmp(info->manufacturerAbility, ability, sizeof(ability)), 0);

    ASSERT_FALSE(HiboxServiceManagerRspParse(CMDID_DEVICE_ABILITY, nullptr, off,
        reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_FALSE(HiboxServiceManagerRspParse(CMDID_DEVICE_ABILITY, buf, HIBOX_TLV_HEADER_LEN,
        reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_FALSE(HiboxServiceManagerRspParse(CMDID_DEVICE_ABILITY, buf, off, nullptr));
    const uint8_t unknownCmd = 0x7E;
    ASSERT_FALSE(HiboxServiceManagerRspParse(unknownCmd, buf, off, reinterpret_cast<uint8_t *>(&msg)));
}

/**
 * @tc.name: HiboxServiceMgmtRspParseTest002
 * @tc.desc: 校验厂商能力解析的数据长度口径：按实际收到的值长度累计；单段超限或累计超结构体时长度归零
 * @tc.type: FUNC
 */
HWTEST_F(HiboxServiceMgmtTest, HiboxServiceMgmtRspParseTest002, TestSize.Level1)
{
    uint8_t buf[64];
    ParseMsgBuf msg;

    // 只给协议版本一段：datalen就是该段值长度
    const uint8_t protocol[DEVICE_PROTOCOL_LEN] = { 0x10, 0x01 };
    ClearMsg(msg);
    uint16_t off = MakeTlv(buf, PROTOCOL_TYPE, DEVICE_PROTOCOL_LEN, protocol);
    ASSERT_TRUE(HiboxServiceManagerRspParse(CMDID_DEVICE_ABILITY, buf, off,
        reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_EQ(msg.ind.datalen, DEVICE_PROTOCOL_LEN);

    // 协议版本段超上限：本轮解析作废，datalen归零
    const uint8_t tooLong[DEVICE_PROTOCOL_LEN + 1] = { 0 };
    ClearMsg(msg);
    off = MakeTlv(buf, PROTOCOL_TYPE, sizeof(tooLong), tooLong);
    ASSERT_TRUE(HiboxServiceManagerRspParse(CMDID_DEVICE_ABILITY, buf, off,
        reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_EQ(msg.ind.datalen, 0);

    // 各段合法但累计值长度超过结构体上限：datalen归零
    const uint8_t osType[DEVICE_OS_LEN] = { 0 };
    const uint8_t ability[DEVICE_MANUFACTURER_ABILITY_LEN] = { 0 };
    ClearMsg(msg);
    off = 0;
    off += MakeTlv(buf + off, PROTOCOL_TYPE, DEVICE_PROTOCOL_LEN, protocol);
    off += MakeTlv(buf + off, OS_TYPE, DEVICE_OS_LEN, osType);
    off += MakeTlv(buf + off, MANUFACTURER_ABILITY_BIT_MAP, DEVICE_MANUFACTURER_ABILITY_LEN, ability);
    off += MakeTlv(buf + off, PROTOCOL_TYPE, DEVICE_PROTOCOL_LEN, protocol);
    ASSERT_TRUE(HiboxServiceManagerRspParse(CMDID_DEVICE_ABILITY, buf, off,
        reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_EQ(msg.ind.datalen, 0);

    // 未知类型TLV放行：值长度照样累计进datalen
    ClearMsg(msg);
    off = MakeTlv(buf, 0x55, DEVICE_OS_LEN, osType);
    ASSERT_TRUE(HiboxServiceManagerRspParse(CMDID_DEVICE_ABILITY, buf, off,
        reinterpret_cast<uint8_t *>(&msg)));
    ASSERT_EQ(msg.ind.datalen, DEVICE_OS_LEN);
}

/**
 * @tc.name: HiboxServiceMgmtCombTest001
 * @tc.desc: 校验厂商能力组包：三段TLV（协议+系统+位图）共29字节；数据不符或空指针返回0
 * @tc.type: FUNC
 */
HWTEST_F(HiboxServiceMgmtTest, HiboxServiceMgmtCombTest001, TestSize.Level1)
{
    uint8_t buf[64] = {0};
    ParseMsgBuf arg;
    ClearMsg(arg);
    arg.ind.msgType = DEVICE_MANUFACTURE_ABILITY;
    arg.ind.datalen = sizeof(ManufacturerAbilityInfo);
    ManufacturerAbilityInfo *info = reinterpret_cast<ManufacturerAbilityInfo *>(arg.bytes);
    info->protocolType[0] = 0x10;
    info->deviceOsType[0] = 0x0C;
    info->manufacturerAbility[0] = 0x01;
    uint16_t used = HiboxDeviceAbilityReqComb(buf, sizeof(buf), &arg.ind);
    // 协议(2)+系统(2)+位图(16)三段TLV
    ASSERT_EQ(used, 3 * HIBOX_TLV_HEADER_LEN + sizeof(ManufacturerAbilityInfo));
    ASSERT_EQ(buf[0], PROTOCOL_TYPE);
    ASSERT_EQ(buf[3 + DEVICE_PROTOCOL_LEN], OS_TYPE);

    arg.ind.datalen = sizeof(ManufacturerAbilityInfo) + 1;
    ASSERT_EQ(HiboxDeviceAbilityReqComb(buf, sizeof(buf), &arg.ind), 0);
    arg.ind.datalen = sizeof(ManufacturerAbilityInfo);
    ASSERT_EQ(HiboxDeviceAbilityReqComb(nullptr, sizeof(buf), &arg.ind), 0);
    ASSERT_EQ(HiboxDeviceAbilityReqComb(buf, sizeof(buf), nullptr), 0);
}
