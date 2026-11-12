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

#include "log.h"
#include "sle_service_data.cpp"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
namespace Nearlink {
namespace TEST {
class SleServiceDataTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        HILOGI("SetUpTestCase start");
    }

    static void TearDownTestCase()
    {
        HILOGI("SetUpTestCase start");
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }
};

/**
 * @tc.name: TestSetTxPower001
 * @tc.desc: 测试设置发射功率函数，覆盖所有分支
 * @tc.type: FUNC
 */
HWTEST_F(SleServiceDataTest, TestSetTxPower001, TestSize.Level1) {
    SleAdvertiserSettingsImpl advertiser;
    // 1. 测试无效参数（返回 RET_BAD_PARAM）
    EXPECT_EQ(advertiser.SetTxPower(static_cast<int>(SleAdvertiserTxPowerLevel::SLE_ADV_TX_POWER_INVAILD)),
              static_cast<int>(ReturnValue::RET_BAD_PARAM));
    EXPECT_EQ(advertiser.SetTxPower(static_cast<int>(SleAdvertiserTxPowerLevel::SLE_ADV_TX_POWER_ULTRA_LOW) - 1),
              static_cast<int>(ReturnValue::RET_BAD_PARAM));
    // 2. 测试各有效功率级别（返回 RET_NO_ERROR）
    // SLE_ADV_TX_POWER_ULTRA_LOW
    EXPECT_EQ(advertiser.SetTxPower(static_cast<int>(SleAdvertiserTxPowerLevel::SLE_ADV_TX_POWER_ULTRA_LOW)),
              static_cast<int>(ReturnValue::RET_NO_ERROR));
    EXPECT_EQ(advertiser.txPower_, static_cast<int8_t>(SleAdvTxPowerValue::SLE_ADV_TX_POWER_ULTRA_LOW_VALUE));
    // SLE_ADV_TX_POWER_LOW
    EXPECT_EQ(advertiser.SetTxPower(static_cast<int>(SleAdvertiserTxPowerLevel::SLE_ADV_TX_POWER_LOW)),
              static_cast<int>(ReturnValue::RET_NO_ERROR));
    EXPECT_EQ(advertiser.txPower_, static_cast<int8_t>(SleAdvTxPowerValue::SLE_ADV_TX_POWER_LOW_VALUE));
    // SLE_ADV_TX_POWER_MEDIUM
    EXPECT_EQ(advertiser.SetTxPower(static_cast<int>(SleAdvertiserTxPowerLevel::SLE_ADV_TX_POWER_MEDIUM)),
              static_cast<int>(ReturnValue::RET_NO_ERROR));
    EXPECT_EQ(advertiser.txPower_, static_cast<int8_t>(SleAdvTxPowerValue::SLE_ADV_TX_POWER_MEDIUM_VALUE));
    // SLE_ADV_TX_POWER_HIGH
    EXPECT_EQ(advertiser.SetTxPower(static_cast<int>(SleAdvertiserTxPowerLevel::SLE_ADV_TX_POWER_HIGH)),
              static_cast<int>(ReturnValue::RET_NO_ERROR));
    EXPECT_EQ(advertiser.txPower_, static_cast<int8_t>(SleAdvTxPowerValue::SLE_ADV_TX_POWER_HIGH_VALUE));
    // SLE_ADV_TX_POWER_ULTRA_HIGH
    EXPECT_EQ(advertiser.SetTxPower(static_cast<int>(SleAdvertiserTxPowerLevel::SLE_ADV_TX_POWER_ULTRA_HIGH)),
              static_cast<int>(ReturnValue::RET_NO_ERROR));
    EXPECT_EQ(advertiser.txPower_, static_cast<int8_t>(SleAdvTxPowerValue::SLE_ADV_TX_POWER_ULTRA_HIGH_VALUE));
    // SLE_ADV_TX_POWER_FIND
    EXPECT_EQ(advertiser.SetTxPower(static_cast<int>(SleAdvertiserTxPowerLevel::SLE_ADV_TX_POWER_FIND)),
              static_cast<int>(ReturnValue::RET_NO_ERROR));
    EXPECT_EQ(advertiser.txPower_, static_cast<int8_t>(SleAdvTxPowerValue::SLE_ADV_TX_POWER_FIND_VALUE));
}

/**
 * @tc.name: SetGetInfo001
 * @tc.desc: Verify SlePeripheralDevice function.
 * @tc.type: FUNC
 */
HWTEST_F(SleServiceDataTest, SetGetInfo001, TestSize.Level1)
{
    HILOGI("SetGetInfo001 start");
    std::shared_ptr<SlePeripheralDevice> dev = std::make_shared<SlePeripheralDevice>();
    RawAddress address = RawAddress("00:00:00:00:00:00");
    int index = 0;
    string subModelId = "12345";
    uint8_t cryptoAlgo = 1;
    uint8_t KeyDerivAlgo = 2;
    uint8_t integrChkInd = 3;
    std::string encryptGroupKeyStr = "123456789";
    uint64_t giv = 4;
    bool isUserDisconnected = true;
    dev->SetCurrentRawAddress(address);
    dev->GetServiceData(index);
    dev->GetServiceDataUUID();
    dev->GetServiceUUID(index);
    EXPECT_EQ(address, dev->GetCurrentRawAddress());
    dev->SetCollaborateAddress(address);
    EXPECT_EQ(address, dev->GetCollaborateAddress());
    dev->SetSubModelId(subModelId);
    EXPECT_EQ(subModelId, dev->GetSubModelId());
    dev->SetCryptoAlgo(cryptoAlgo);
    EXPECT_EQ(cryptoAlgo, dev->GetCryptoAlgo());
    dev->SetKeyDerivAlgo(KeyDerivAlgo);
    EXPECT_EQ(KeyDerivAlgo, dev->GetKeyDerivAlgo());
    dev->SetIntegrChkInd(integrChkInd);
    EXPECT_EQ(integrChkInd, dev->GetIntegrChkInd());
    dev->SetEncryptGroupKeyStr(encryptGroupKeyStr);
    EXPECT_EQ(encryptGroupKeyStr, dev->GetEncryptGroupKeyStr());
    dev->SetGiv(giv);
    EXPECT_EQ(giv, dev->GetGiv());
    dev->SetIsUserDisconnected(isUserDisconnected);
    EXPECT_EQ(isUserDisconnected, dev->GetIsUserDisconnected());
    HILOGI("SetGetInfo001 end");
}

/*
 * 广播数据族补充用例：服务数据/服务 UUID 按类型组装、设备名长短界、
 * 下标越界防护、DIS 与 HiLink 广播解析矩阵、16/128 位服务数据长度边界、
 * 厂商数据/服务数据/服务 UUID 的重复插入去重。
 */
namespace {
/* 追加类型字节与值区：DIS 的值区自带长度字节，HiLink 的值区不带（长度查产品字典） */
void AppendTv(std::string &payload, uint8_t type, const std::string &value)
{
    payload.push_back(static_cast<char>(type));
    payload.append(value);
}
}  // namespace

/**
 * @tc.name: AdvertiserServiceDataByUuidType001
 * @tc.desc: 服务数据与服务UUID按16位/128位UUID类型组装，载荷前缀与长度字段正确。
 * @tc.type: FUNC
 */
HWTEST_F(SleServiceDataTest, AdvertiserServiceDataByUuidType001, TestSize.Level1)
{
    SleAdvertiserDataImpl advertiser;
    const std::string body = "AB";

    advertiser.AddServiceData(Uuid::ConvertFrom16Bits(0x0605), body);
    advertiser.AddServiceData(Uuid::ConvertFromString(SLE_UUID_DIS), body);
    advertiser.AddServiceUuid(Uuid::ConvertFrom16Bits(0x0606));
    advertiser.AddServiceUuid(Uuid::ConvertFromString(SLE_UUID_HILINK));
    const std::string payload = advertiser.GetPayload();

    /* 每段结构为 [类型][长度][取值]，四段依次校验类型与长度字节 */
    EXPECT_EQ(payload[0], static_cast<char>(SLE_ADV_DATA_TYPE_SERVICE_DATA_16BIT_UUID));
    EXPECT_EQ(payload[1], static_cast<char>(body.size() + 2));
    EXPECT_EQ(payload[6], static_cast<char>(SLE_ADV_DATA_TYPE_SERVICE_DATA_128BIT_UUID));
    EXPECT_EQ(payload[7], static_cast<char>(body.size() + 16));
    EXPECT_EQ(payload[26], static_cast<char>(SLE_ADV_DATA_TYPE_COMPLETE_LIST_OF_16BIT_SERVICE_UUIDS));
    EXPECT_EQ(payload[30], static_cast<char>(SLE_ADV_DATA_TYPE_COMPLETE_LIST_OF_128BIT_SERVICE_UUIDS));
}

/**
 * @tc.name: AdvertiserDeviceNameBoundary001
 * @tc.desc: 设备名不超过上限走完整名类型，超限走短名类型。
 * @tc.type: FUNC
 */
HWTEST_F(SleServiceDataTest, AdvertiserDeviceNameBoundary001, TestSize.Level1)
{
    SleAdvertiserDataImpl shortAdv;
    shortAdv.SetDeviceName(std::string(DEVICE_NAME_MAX_LEN, 'a'));
    EXPECT_EQ(shortAdv.GetPayload()[0], static_cast<char>(SLE_ADV_DATA_TYPE_COMPLETE_LOCAL_NAME));

    SleAdvertiserDataImpl longAdv;
    longAdv.SetDeviceName(std::string(DEVICE_NAME_MAX_LEN + 1, 'b'));
    EXPECT_EQ(longAdv.GetPayload()[0], static_cast<char>(SLE_ADV_DATA_TYPE_SHORTENED_LOCAL_NAME));
}

/**
 * @tc.name: AdvertiserDataLengthGuard001
 * @tc.desc: 服务数据与厂商数据超过单段255字节上限时拒绝写入，载荷保持为空。
 * @tc.type: FUNC
 */
HWTEST_F(SleServiceDataTest, AdvertiserDataLengthGuard001, TestSize.Level1)
{
    const std::string tooLong(300, 'x');

    SleAdvertiserDataImpl serviceData;
    serviceData.AddServiceData(Uuid::ConvertFrom16Bits(0x0605), tooLong);
    EXPECT_TRUE(serviceData.GetPayload().empty());

    SleAdvertiserDataImpl manufacturer;
    manufacturer.SetManufacturerData(tooLong);
    EXPECT_TRUE(manufacturer.GetPayload().empty());
}

/**
 * @tc.name: PeripheralServiceUuidBounds001
 * @tc.desc: 按下标取服务UUID：空表、负下标、越界下标返回空值，合法下标返回登记值。
 * @tc.type: FUNC
 */
HWTEST_F(SleServiceDataTest, PeripheralServiceUuidBounds001, TestSize.Level1)
{
    SlePeripheralDevice device;
    const Uuid empty;
    EXPECT_TRUE(device.GetServiceUUID(0) == empty);
    EXPECT_TRUE(device.GetServiceUUID(-1) == empty);

    const Uuid first = Uuid::ConvertFrom16Bits(0x0605);
    device.SetServiceUUID(first);
    device.SetServiceUUID(Uuid::ConvertFrom16Bits(0x0606));
    EXPECT_TRUE(device.GetServiceUUID(2) == empty);
    EXPECT_TRUE(device.GetServiceUUID(0) == first);
    EXPECT_EQ(device.GetServiceUUID().size(), 2U);
}

/**
 * @tc.name: ParseDisServiceData001
 * @tc.desc: DIS广播解析：设备名与外观TLV正常解析，外观长度错配被跳过后解析继续。
 * @tc.type: FUNC
 */
HWTEST_F(SleServiceDataTest, ParseDisServiceData001, TestSize.Level1)
{
    SlePeripheralDevice device;
    std::string payload;
    /* 设备名 TLV */
    AppendTv(payload, SLE_DEVIE_INFO_LOCAL_NAME, std::string("\x05phone", 6));
    /* 外观长度错配（合法为4字节）：该TLV跳过，不影响后续解析 */
    AppendTv(payload, SLE_DEVIE_INFO_APPEARANCE, std::string("\x03\x01\x02\x03", 4));
    device.SetServiceDataUUID(Uuid::ConvertFromString(SLE_UUID_DIS), payload);
    device.ParseSleServiceData();
    EXPECT_EQ(device.GetName(), "phone");
    /* 外观长度错配被跳过，外观保持未设置的无效值 */
    EXPECT_EQ(device.GetAppearance(), SLE_INVALID_APPEARANCE);

    /* 外观长度正确：按小端四字节读入 */
    SlePeripheralDevice appearanceDev;
    std::string appearancePayload;
    AppendTv(appearancePayload, SLE_DEVIE_INFO_APPEARANCE, std::string("\x04\x40\x09\x00\x00", 5));
    appearanceDev.SetServiceDataUUID(Uuid::ConvertFromString(SLE_UUID_DIS), appearancePayload);
    appearanceDev.ParseSleServiceData();
    EXPECT_EQ(appearanceDev.GetAppearance(), 0x0940);
}

/**
 * @tc.name: ParseDisServiceData002
 * @tc.desc: DIS广播解析的畸形输入：非DIS数据不解析、超短载荷不进入循环、未知类型走默认分支、声明长度越界中止。
 * @tc.type: FUNC
 */
HWTEST_F(SleServiceDataTest, ParseDisServiceData002, TestSize.Level1)
{
    /* 服务数据不带DIS UUID：不解析任何字段 */
    SlePeripheralDevice notDis;
    notDis.SetServiceDataUUID(Uuid::ConvertFrom16Bits(0xFDEE), std::string("\x06\x05phone", 7));
    notDis.ParseSleServiceData();
    EXPECT_EQ(notDis.GetName(), "");

    /* 载荷不足最小TLV头：不进入解析循环 */
    SlePeripheralDevice tooShort;
    tooShort.SetServiceDataUUID(Uuid::ConvertFromString(SLE_UUID_DIS), std::string("\x06\x05", 2));
    tooShort.ParseSleServiceData();
    EXPECT_EQ(tooShort.GetName(), "");

    /* 未知类型走默认分支后继续读下一段 */
    SlePeripheralDevice unknownType;
    std::string unknownPayload;
    AppendTv(unknownPayload, 0x09, std::string("\x01\xAA", 2));
    AppendTv(unknownPayload, SLE_DEVIE_INFO_LOCAL_NAME, std::string("\x02ok", 3));
    unknownType.SetServiceDataUUID(Uuid::ConvertFromString(SLE_UUID_DIS), unknownPayload);
    unknownType.ParseSleServiceData();
    EXPECT_EQ(unknownType.GetName(), "ok");

    /* 声明长度超出实际数据：解析中止 */
    SlePeripheralDevice truncated;
    truncated.SetServiceDataUUID(Uuid::ConvertFromString(SLE_UUID_DIS), std::string("\x06\x09a", 3));
    truncated.ParseSleServiceData();
    EXPECT_EQ(truncated.GetName(), "");
}

/**
 * @tc.name: ParseHiLinkNearby001
 * @tc.desc: HiLink靠近发现广播逐类型解析：型号/新型号/子型号/图标端序反转/蓝牙地址格式化。
 * @tc.type: FUNC
 */
HWTEST_F(SleServiceDataTest, ParseHiLinkNearby001, TestSize.Level1)
{
    SlePeripheralDevice device;
    std::string payload;
    /* 版本、主业务=靠近发现、扩展业务 */
    payload.append(std::string("\x01\x01\x00", 3));
    AppendTv(payload, SLE_HILINK_MODEL_ID, std::string("\x01\x02\x03", 3));
    AppendTv(payload, SLE_HILINK_NEW_MODEL_ID, std::string("\x11\x22\x33\x44", 4));
    AppendTv(payload, SLE_HILINK_SUB_MODEL_ID, std::string("\x11", 1));
    /* 图标广播为小端，解析时反转 */
    AppendTv(payload, SLE_HILINK_ICON_ID, std::string("\x34\x12", 2));
    AppendTv(payload, SLE_HILINK_BLE_ADV_ADDR, std::string("\x11\x22\x33\x44\x55\x66", 6));
    device.SetServiceDataUUID(Uuid::ConvertFromString(SLE_UUID_HILINK), payload);
    device.ParseSleServiceData();

    EXPECT_EQ(device.GetModelId(), "010203");
    EXPECT_EQ(device.GetNewModelId(), "11223344");
    EXPECT_EQ(device.GetSubModelId(), "11");
    EXPECT_EQ(device.GetIconId(), "1234");
    EXPECT_EQ(device.GetBtAddr(), "11:22:33:44:55:66");
}

/**
 * @tc.name: ParseHiLinkNearby002
 * @tc.desc: HiLink广播的业务过滤与畸形输入：非HiLink数据不解析、载荷过短直接返回、主业务不匹配返回、未知类型与长度越界中止。
 * @tc.type: FUNC
 */
HWTEST_F(SleServiceDataTest, ParseHiLinkNearby002, TestSize.Level1)
{
    /* 服务数据不是HiLink UUID：不解析 */
    SlePeripheralDevice notHiLink;
    notHiLink.SetServiceDataUUID(Uuid::ConvertFromString(SLE_UUID_DIS), std::string("\x01\x01\x00\x03\x01\x02\x03", 7));
    notHiLink.ParseSleServiceData();
    EXPECT_EQ(notHiLink.GetModelId(), "");

    /* 载荷不足三字节头部：直接返回 */
    SlePeripheralDevice tooShort;
    tooShort.SetServiceDataUUID(Uuid::ConvertFromString(SLE_UUID_HILINK), std::string("\x01\x01", 2));
    tooShort.ParseSleServiceData();
    EXPECT_EQ(tooShort.GetModelId(), "");

    /* 主业务既非靠近发现也非虚连：返回 */
    SlePeripheralDevice otherBusiness;
    otherBusiness.SetServiceDataUUID(Uuid::ConvertFromString(SLE_UUID_HILINK),
        std::string("\x01\x09\x00\x03\x01\x02\x03", 7));
    otherBusiness.ParseSleServiceData();
    EXPECT_EQ(otherBusiness.GetModelId(), "");

    /* 未知扩展类型不在字典：剩余数据中止 */
    SlePeripheralDevice unknownType;
    std::string unknownPayload;
    unknownPayload.append(std::string("\x01\x01\x00", 3));
    AppendTv(unknownPayload, 0x7F, std::string("\x01", 1));
    AppendTv(unknownPayload, SLE_HILINK_MODEL_ID, std::string("\x01\x02\x03", 3));
    unknownType.SetServiceDataUUID(Uuid::ConvertFromString(SLE_UUID_HILINK), unknownPayload);
    unknownType.ParseSleServiceData();
    EXPECT_EQ(unknownType.GetModelId(), "");

    /* 合法类型剩余数据不足字典长度：中止 */
    SlePeripheralDevice overrun;
    std::string overrunPayload;
    overrunPayload.append(std::string("\x01\x01\x00", 3));
    AppendTv(overrunPayload, SLE_HILINK_MODEL_ID, std::string("\x01", 1));
    overrun.SetServiceDataUUID(Uuid::ConvertFromString(SLE_UUID_HILINK), overrunPayload);
    overrun.ParseSleServiceData();
    EXPECT_EQ(overrun.GetModelId(), "");
}

/**
 * @tc.name: ServiceUuid16BitsAudioFlag001
 * @tc.desc: 16位服务UUID含ASC管理/能力标识时设备标记为音频设备；非ASC标识不标记；奇数长度不入解析。
 * @tc.type: FUNC
 */
HWTEST_F(SleServiceDataTest, ServiceUuid16BitsAudioFlag001, TestSize.Level1)
{
    SlePeripheralDeviceParseAdvData parseAdvData;

    /* ASC 管理服务 UUID（小端两字节） */
    uint8_t mgmt[2] = {0x05, 0x06};
    parseAdvData.payload = mgmt;
    parseAdvData.length = sizeof(mgmt);
    SlePeripheralDevice mgmtDev;
    mgmtDev.SetServiceUUID16Bits(parseAdvData);
    EXPECT_TRUE(mgmtDev.GetIsAudioDeviceFlag());

    /* ASC 能力服务 UUID */
    uint8_t ability[2] = {0x06, 0x06};
    parseAdvData.payload = ability;
    parseAdvData.length = sizeof(ability);
    SlePeripheralDevice abilityDev;
    abilityDev.SetServiceUUID16Bits(parseAdvData);
    EXPECT_TRUE(abilityDev.GetIsAudioDeviceFlag());

    /* 非ASC UUID：不标记音频设备 */
    uint8_t normal[2] = {0x11, 0x22};
    parseAdvData.payload = normal;
    parseAdvData.length = sizeof(normal);
    SlePeripheralDevice normalDev;
    normalDev.SetServiceUUID16Bits(parseAdvData);
    EXPECT_FALSE(normalDev.GetIsAudioDeviceFlag());
    EXPECT_EQ(normalDev.GetServiceUUID().size(), 1U);

    /* 长度不是2的倍数：整段不入 */
    uint8_t odd[1] = {0x05};
    parseAdvData.payload = odd;
    parseAdvData.length = sizeof(odd);
    SlePeripheralDevice oddDev;
    oddDev.SetServiceUUID16Bits(parseAdvData);
    EXPECT_EQ(oddDev.GetServiceUUID().size(), 0U);
}

/**
 * @tc.name: ServiceDataUuid16And128001
 * @tc.desc: 16/128位服务数据UUID按长度边界拆分：最小长度只取UUID，超长部分作为数据体。
 * @tc.type: FUNC
 */
HWTEST_F(SleServiceDataTest, ServiceDataUuid16And128001, TestSize.Level1)
{
    SlePeripheralDeviceParseAdvData parseAdvData;

    /* 16 位：长度 2 只有 UUID，数据体为空 */
    uint8_t uuid16Only[2] = {0xEE, 0xFD};
    parseAdvData.payload = uuid16Only;
    parseAdvData.length = sizeof(uuid16Only);
    SlePeripheralDevice dev16;
    dev16.SetServiceDataUUID16Bits(parseAdvData);
    ASSERT_EQ(dev16.GetServiceDataUUID().size(), 1U);
    EXPECT_EQ(dev16.GetServiceData(0), "");

    /* 16 位带 3 字节数据体：换一个新 uuid 的独立设备验证（同 uuid 重登记会被去重） */
    uint8_t uuid16WithBody[5] = {0xEF, 0xFD, 'x', 'y', 'z'};
    parseAdvData.payload = uuid16WithBody;
    parseAdvData.length = sizeof(uuid16WithBody);
    SlePeripheralDevice dev16Body;
    dev16Body.SetServiceDataUUID16Bits(parseAdvData);
    ASSERT_EQ(dev16Body.GetServiceDataUUID().size(), 1U);
    EXPECT_EQ(dev16Body.GetServiceData(0), "xyz");

    /* 128 位：长度 16 只有 UUID，数据体为空 */
    uint8_t uuid128Only[16] = {0};
    uuid128Only[15] = 0x01;
    parseAdvData.payload = uuid128Only;
    parseAdvData.length = sizeof(uuid128Only);
    SlePeripheralDevice dev128;
    dev128.SetServiceDataUUID128Bits(parseAdvData);
    ASSERT_EQ(dev128.GetServiceDataUUID().size(), 1U);
    EXPECT_EQ(dev128.GetServiceData(0), "");

    /* 128 位带 2 字节数据体：独立设备验证 */
    uint8_t uuid128WithBody[18] = {0};
    uuid128WithBody[15] = 0x02;
    uuid128WithBody[16] = 'o';
    uuid128WithBody[17] = 'k';
    parseAdvData.payload = uuid128WithBody;
    parseAdvData.length = sizeof(uuid128WithBody);
    SlePeripheralDevice dev128Body;
    dev128Body.SetServiceDataUUID128Bits(parseAdvData);
    ASSERT_EQ(dev128Body.GetServiceDataUUID().size(), 1U);
    EXPECT_EQ(dev128Body.GetServiceData(0), "ok");

    /* 长度不足UUID宽度：不入 */
    uint8_t tooShort[1] = {0xEE};
    parseAdvData.payload = tooShort;
    parseAdvData.length = sizeof(tooShort);
    SlePeripheralDevice devShort;
    devShort.SetServiceDataUUID16Bits(parseAdvData);
    EXPECT_EQ(devShort.GetServiceDataUUID().size(), 0U);
}

/**
 * @tc.name: DedupInsert001
 * @tc.desc: 厂商数据、服务数据、服务UUID重复登记时只保留首条。
 * @tc.type: FUNC
 */
HWTEST_F(SleServiceDataTest, DedupInsert001, TestSize.Level1)
{
    SlePeripheralDevice device;
    const Uuid uuid = Uuid::ConvertFrom16Bits(0x0605);

    device.SetManufacturerData(0x0117, "first");
    device.SetManufacturerData(0x0117, "second");
    EXPECT_EQ(device.GetManufacturerData().size(), 1U);

    device.SetServiceDataUUID(uuid, "first");
    device.SetServiceDataUUID(uuid, "second");
    EXPECT_EQ(device.GetServiceDataUUID().size(), 1U);
    EXPECT_EQ(device.GetServiceData(0), "first");

    device.SetServiceUUID(uuid);
    device.SetServiceUUID(uuid);
    EXPECT_EQ(device.GetServiceUUID().size(), 1U);
}
}  // namespace TEST
}  // namespace Nearlink
}  // namespace OHOS