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
#include "SleHuksTool.h"
#include "log.h"
#include "SleInterfaceManager.h"
#include "SleInterfaceAdapterSub.h"
#include "SleSecurity.h"
#include "sm_errcode.h"
#include "nlstk_sm_api.h"
#include "nlstk_public_define_ext.h"
#include "nearlink_def.h"
#include "sle_service_data.h"
#include "SleAdapter.cpp"
#include "SleRemoteDeviceManager.h"

#include "SleInterfaceManager.h"
#include "SleServiceManager.h"
#include "nearlink_sle_datatransfer_server.h"
#include "nearlink_access_token_mock.h"
#include <thread>
#include <chrono>

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

namespace {
constexpr int VCP_SERVICE_UT_DELAY_50_MS = 50;
constexpr int VCP_SERVICE_TDD_DELAY_1000_MS = 1000;
sptr<NearlinkSleDataTransferServer> g_sleDataTransfer = new (std::nothrow) NearlinkSleDataTransferServer();
auto g_adapter = std::make_shared<SleAdapter>();
SleSecurity sleSecurity_(*g_adapter);
}
namespace {

}

class SleAdapterSecurityTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void SleAdapterSecurityTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase SleSecurityTest.");
    NearlinkAccessTokenMock::SetNativeTokenInfo();
    SleInterfaceManager::GetInstance()->Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::VCP_SERVICE_TDD_DELAY_1000_MS));
}

void SleAdapterSecurityTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase SleSecurityTest");
}

void SleAdapterSecurityTest::SetUp()
{
    SleConfig::GetInstance().RemoveAllPairedDevices();
    HILOGI("SetUp SleSecurityTest.");
}

void SleAdapterSecurityTest::TearDown()
{
    HILOGI("TearDown SleSecurityTest.");
}

/**
 * @tc.name: Sle_Encryption_Test001
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterSecurityTest, Sle_Encryption_Test001, TestSize.Level1)
{
    HILOGI("SleHuksToolEncryption_Test001 start");
    LinkKey linkKey = {0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x20, 0x21,
                       0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29};
    EncryptedLinkKey encryptedLinkKey;
    EXPECT_EQ(HKS_SUCCESS, SleHksTool::GetInstance().SleLinkKeyEncrypt(linkKey, encryptedLinkKey));
    EXPECT_EQ(OCTET128_LEN, encryptedLinkKey.size());
    LinkKey linkKeyDencrypt;
    EXPECT_EQ(HKS_SUCCESS, SleHksTool::GetInstance().SleLinkKeyDecrypt(encryptedLinkKey, linkKeyDencrypt));
    EXPECT_EQ(true, linkKey == linkKeyDencrypt);
    HILOGI("SleHuksToolEncryption_Test001 end");
}

/**
 * @tc.name: Sle_Encryption_Test002
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterSecurityTest, Sle_Encryption_Test002, TestSize.Level1)
{
    HILOGI("SleHuksToolEncryption_Test002 start");
    EXPECT_EQ(HKS_SUCCESS, SleHksTool::GetInstance().SleDeleteHksKey());
    EXPECT_EQ(HKS_ERROR_NOT_EXIST, SleHksTool::GetInstance().SleDeleteHksKey());
    HILOGI("SleHuksToolEncryption_Test002 end");
}

/**
 * @tc.name: SleLinkKeyDecrypt001
 * @tc.desc: 验证 SleLinkKeyDecrypt
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterSecurityTest, SleLinkKeyDecrypt001, TestSize.Level1)
{
    HILOGI("SleAdapterSecurityTest: SleLinkKeyDecrypt001 start");
    std::string linkKeyStr = "key"; 
    LinkKey linkkey = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
    uint8_t ret = SleRemoteDeviceAdapter::GetInstance()->SleLinkKeyDecrypt(linkKeyStr, linkkey);
    EXPECT_EQ(false, ret);
    HILOGI("SleAdapterSecurityTest: SleLinkKeyDecrypt001 end");
}

/**
 * @tc.name: GetLinkRole001
 * @tc.desc: 验证 GetLinkRole 函数返回正确的连接角色
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterSecurityTest, GetLinkRole001, TestSize.Level1)
{
    HILOGI("SleAdapterSecurityTest: GetLinkRole001 start");
    RawAddress device("00:11:22:33:44:55");
    uint8_t role = g_adapter->GetLinkRole(device);
    EXPECT_EQ(0xFF, role);
    HILOGI("SleAdapterSecurityTest: GetLinkRole001 end");
}
 
/**
 * @tc.name: SetBtAddrBySleAddr001
 * @tc.desc: 验证 SetBtAddrBySleAddr 函数设置蓝牙地址
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterSecurityTest, SetBtAddrBySleAddr001, TestSize.Level1)
{
    HILOGI("SleAdapterSecurityTest: SetBtAddrBySleAddr001 start");
    std::string sleAddr = "00:11:22:33:44:55";
    std::string btAddr = "66:77:88:99:AA:BB";
    uint8_t ret = g_adapter->SetBtAddrBySleAddr(sleAddr, btAddr);
    EXPECT_EQ(0, ret);
    HILOGI("SleAdapterSecurityTest: SetBtAddrBySleAddr001 end");
}

/**
 * @tc.name: SetBtAddrBySleAddrTask001
 * @tc.desc: 验证 SetBtAddrBySleAddrTask - 成功更新设备的BT地址
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, SetBtAddrBySleAddrTask001)
{
    HILOGI("SleAdapterSecurityTest: SetBtAddrBySleAddrTask001 start");
    std::string sleAddr = "AA:BB:CC:DD:EE:FF";
    std::string newBtAddr = "11:22:33:44:55:66";
    std::string oldBtAddr = "00:00:00:00:00:00";
    std::shared_ptr<SlePeripheralDevice> peerDevice = std::make_shared<SlePeripheralDevice>();
    peerDevice->SetAddress(RawAddress(sleAddr));
    peerDevice->SetBtAddr(oldBtAddr);
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(sleAddr, peerDevice);
    bool result = g_adapter->SetBtAddrBySleAddrTask(sleAddr, newBtAddr);
    EXPECT_TRUE(result);
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(sleAddr);
    HILOGI("SleAdapterSecurityTest: SetBtAddrBySleAddrTask001 end");
}

/**
 * @tc.name: SetPairingPassCode001
 * @tc.desc: 验证 SetPairingPassCode
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterSecurityTest, SetPairingPassCode001, TestSize.Level1)
{
    HILOGI("SleAdapterSecurityTest: SetPairingPassCode001 start");
    RawAddress device("00:11:22:33:44:55");
    std::string passCode = "123456";

    std::shared_ptr<SlePeripheralDevice> peerDevice = std::make_shared<SlePeripheralDevice>();
    peerDevice->SetAcbConnectState(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED));
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(device.GetAddress(), peerDevice);

    uint8_t ret = g_adapter->SetPairingPassCode(device, passCode);
    EXPECT_EQ(true, ret);
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(device.GetAddress());
    HILOGI("SleAdapterSecurityTest: SetPairingPassCode001 end");
}

/**
 * @tc.name: SetPairingConfirmation001
 * @tc.desc: 验证 SetPairingConfirmation
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterSecurityTest, SetPairingConfirmation001, TestSize.Level1)
{
    HILOGI("SleAdapterSecurityTest: SetPairingConfirmation001 start");
    RawAddress device("00:11:22:33:44:55");

    std::shared_ptr<SlePeripheralDevice> peerDevice = std::make_shared<SlePeripheralDevice>();
    peerDevice->SetAcbConnectState(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED));
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(device.GetAddress(), peerDevice);

    uint8_t ret = g_adapter->SetPairingConfirmation(device);
    EXPECT_EQ(true, ret);
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(device.GetAddress());
    HILOGI("SleAdapterSecurityTest: SetPairingConfirmation001 end");
}

/**
 * @tc.name: StartPairTask001
 * @tc.desc: 验证 StartPairTask
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, StartPairTask001)
{
    HILOGI("SleAdapterSecurityTest: StartPairTask001 start");
    RawAddress device("00:11:22:33:44:55");

    std::shared_ptr<SlePeripheralDevice> peerDevice = std::make_shared<SlePeripheralDevice>();
    peerDevice->SetAcbConnectState(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED));
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(device.GetAddress(), peerDevice);

    bool ret = g_adapter->StartPairTask(device);
    EXPECT_EQ(true, ret);
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(device.GetAddress());
    HILOGI("SleAdapterSecurityTest: StartPairTask001 end");
}

/**
 * @tc.name: ConnectAcb001
 * @tc.desc: 验证 ConnectAcb
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, ConnectAcb001)
{
    HILOGI("SleAdapterSecurityTest: ConnectAcb001 start");
    RawAddress device("00:11:22:33:44:55");
    g_adapter->ConnectAcb(device);
    HILOGI("SleAdapterSecurityTest: ConnectAcb001 end");
}

/**
 * @tc.name: Disconnect001
 * @tc.desc: 验证 Disconnect
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, Disconnect001)
{
    HILOGI("SleAdapterSecurityTest: Disconnect001 start");
    RawAddress device("00:11:22:33:44:55");
    g_adapter->pimpl->needReconnectDevices_.Emplace(device.GetAddress());
    bool ret = g_adapter->Disconnect(device);
    EXPECT_EQ(true, ret);
    HILOGI("SleAdapterSecurityTest: Disconnect001 end");
}

/**
 * @tc.name: RemoveAllPairsTask001
 * @tc.desc: 验证 RemoveAllPairsTask
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, RemoveAllPairsTask001)
{
    HILOGI("SleAdapterSecurityTest: RemoveAllPairsTask001 start");

    RawAddress device("00:11:22:33:44:55");
    std::shared_ptr<SlePeripheralDevice> peerDevice = std::make_shared<SlePeripheralDevice>();
    peerDevice->SetPairedStatus(static_cast<int>(SlePairState::SLE_PAIR_PAIRED));
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(device.GetAddress(), peerDevice);
    bool ret = g_adapter->RemoveAllPairsTask();
    EXPECT_EQ(true, ret);
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(device.GetAddress());
    HILOGI("SleAdapterSecurityTest: RemoveAllPairsTask001 end");
}

/**
 * @tc.name: Get_Set_SleProperties001
 * @tc.desc: 验证 Get_Set_SleProperties
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, Get_Set_SleProperties001)
{
    HILOGI("SleAdapterSecurityTest: Get_Set_SleProperties001 start");
    int mode = g_adapter->GetBondableMode();
    EXPECT_EQ(0, mode);
    int ret = g_adapter->SetBondableMode(0);
    EXPECT_EQ(true, ret);
    uint32_t length = g_adapter->GetSleMaxAdvertisingDataLength();
    EXPECT_EQ(0xFF, length);
    int ioBility = g_adapter->GetIoCapability();
    EXPECT_EQ(0, ioBility);
    bool result = g_adapter->SetIoCapability(1);
    EXPECT_EQ(true, result);
    bool enable = g_adapter->IsSleEnabled();
    EXPECT_EQ(true, enable);
    HILOGI("SleAdapterSecurityTest: Get_Set_SleProperties001 end");
}

/**
 * @tc.name: SaveDeviceModelInfoToConf001
 * @tc.desc: 验证 SaveDeviceModelInfoToConf
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, SaveDeviceModelInfoToConf001)
{
    HILOGI("SleAdapterSecurityTest: SaveDeviceModelInfoToConf001 start");
    RawAddress device("00:11:22:33:44:55");
    auto value = std::make_shared<SlePeripheralDevice>();
    std::string modelId = "NewModelId";
    value->SetModelId(modelId);
    value->SetNewModelId(modelId);
    value->SetSubModelId(modelId);
    value->SetIconId(modelId);
    bool result = SleRemoteDeviceManager::GetInstance()->SaveDeviceModelInfoToConf(device, value);
    EXPECT_EQ(false, result);
    HILOGI("SleAdapterSecurityTest: SaveDeviceModelInfoToConf001 end");
}

/**
 * @tc.name: SavePeerDeviceInfoToConf001
 * @tc.desc: 验证 SavePeerDeviceInfoToConf
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, SavePeerDeviceInfoToConf001)
{
    HILOGI("SleAdapterSecurityTest: SavePeerDeviceInfoToConf001 start");
    NearlinkSafeMap<std::string, std::shared_ptr<SlePeripheralDevice>> peerConnDeviceSafeList;
    
    auto device = std::make_shared<SlePeripheralDevice>();
    device->SetPairedStatus(static_cast<int>(SlePairState::SLE_PAIR_PAIRED));
    RawAddress address("00:11:22:33:44:55");
    device->SetAddress(address);
    device->SetAddressType(0x01);
    device->SetName("Device1");
    device->SetAliasName("Alias1");
    device->SetAppearance(0x0001);
    device->SetCdsmAddrType(0x02);
    device->SetIsAudioDeviceFlag(true);
    int business = 1;
    device->SetManufacturerBusiness(business);
    device->SetManufacturerAbility({0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08});
    device->SetModelId("modelId1");
    device->SetNewModelId("newModelId1");
    device->SetSubModelId("subModelId1");
    device->SetIconId("iconId1");
    peerConnDeviceSafeList.EnsureInsert(address.GetAddress(), device);

    bool result = SleRemoteDeviceAdapter::GetInstance()->SavePeerDeviceInfoToConf();
    EXPECT_EQ(false, result);
    g_adapter->ClearDeviceManufacturerAbility(address);
    peerConnDeviceSafeList.Erase("00:11:22:33:44:55");
    HILOGI("SleAdapterSecurityTest: SavePeerDeviceInfoToConf001 end");
}

/**
 * @tc.name: SaveDeviceManufacturerAbility001
 * @tc.desc: 验证 SaveDeviceManufacturerAbility
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, SaveDeviceManufacturerAbility001)
{
    HILOGI("SleAdapterSecurityTest: SaveDeviceManufacturerAbility001 start");
    RawAddress device("00:11:22:33:44:55");
    std::shared_ptr<SlePeripheralDevice> peerDevice = std::make_shared<SlePeripheralDevice>();
    peerDevice->SetAddress(device);
    peerDevice->SetManufacturerAbility({0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08});
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(device.GetAddress(), peerDevice);
    SleRemoteDeviceAdapter::GetInstance()->SaveDeviceManufacturerAbility(device);
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(device.GetAddress());
    HILOGI("SleAdapterSecurityTest: SaveDeviceManufacturerAbility001 end");
}

/**
 * @tc.name: SavePeerDevices2Smp001
 * @tc.desc: 验证 SavePeerDevices2Smp
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, SavePeerDevices2Smp001)
{
    HILOGI("SleAdapterSecurityTest: SavePeerDevices2Smp001 start");
    NearlinkSafeMap<std::string, std::shared_ptr<SlePeripheralDevice>> peerConnDeviceSafeList;
    auto device = std::make_shared<SlePeripheralDevice>();
    RawAddress addr("00:11:22:33:44:55");
    device->SetAddress(addr);
    device->SetAddressType(0x01);
    device->SetCryptoAlgo(0x02);
    device->SetKeyDerivAlgo(0x03);
    device->SetIntegrChkInd(0x04);
    peerConnDeviceSafeList.EnsureInsert(addr.GetAddress(), device);
    SleRemoteDeviceAdapter::GetInstance()->SavePeerDevices2Smp();
    peerConnDeviceSafeList.Erase(addr.GetAddress());
    HILOGI("SleAdapterSecurityTest: SavePeerDevices2Smp001 end");
}

/**
 * @tc.name: GetConnectionParam001
 * @tc.desc: 验证 GetConnectionParam
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, GetConnectionParam001)
{
    HILOGI("SleAdapterSecurityTest: GetConnectionParam001 start");
    std::string device = "00:11:22:33:44:55";
    uint16_t timeout = 0;
    uint16_t maxLatency = 0;
    bool result = g_adapter->GetConnectionParam(device, timeout, maxLatency);
    EXPECT_EQ(false, result);
    HILOGI("SleAdapterSecurityTest: GetConnectionParam001 end");
}

/**
 * @tc.name: GetRealAddress001
 * @tc.desc: 验证 GetRealAddress
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, GetRealAddress001)
{
    HILOGI("SleAdapterSecurityTest: GetRealAddress001 start");
    RawAddress reportAddr("00:11:22:33:44:55");
    RawAddress otherAddr("AA:BB:CC:DD:EE:FF");
    uint32_t testGroupId = 0x12345678;
    CdsmService *cdsmService = CdsmService::GetService();
    auto cdsmInfo = std::make_shared<CdsmInfo>(testGroupId, otherAddr.GetAddress());
    cdsmInfo->CdsmAddMemberInfo(reportAddr.GetAddress(), 1);
    cdsmInfo->CdsmAddMemberInfo(otherAddr.GetAddress(), 1);
    cdsmService->cdsmList_.EnsureInsert(testGroupId, cdsmInfo);

    std::shared_ptr<SlePeripheralDevice> peerDevice = std::make_shared<SlePeripheralDevice>();
    peerDevice->SetAcbConnectState(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED));
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(otherAddr.GetAddress(), peerDevice);

    RawAddress result = SleRemoteDeviceAdapter::GetInstance()->GetRealAddress(reportAddr);
    EXPECT_EQ(otherAddr, result);
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(otherAddr.GetAddress());
    cdsmService->cdsmList_.Erase(testGroupId);
    HILOGI("SleAdapterSecurityTest: GetRealAddress001 end");
}

/**
 * @tc.name: GetPairState001
 * @tc.desc: 验证 GetPairState - CDSM组内成员状态检查(部分配对)
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, GetPairState001)
{
    HILOGI("SleAdapterSecurityTest: GetPairState001 start");
    RawAddress inputAddr("11:22:33:44:55:66");
    RawAddress otherMemberAddr("AA:BB:CC:DD:EE:FF");
    uint32_t testGroupId = 0x12345678;
    CdsmService *cdsmService = CdsmService::GetService();
    auto cdsmInfo = std::make_shared<CdsmInfo>(testGroupId, otherMemberAddr.GetAddress());
    cdsmInfo->CdsmAddMemberInfo(inputAddr.GetAddress(), 1);
    cdsmInfo->CdsmAddMemberInfo(otherMemberAddr.GetAddress(), 1);
    cdsmService->cdsmList_.EnsureInsert(testGroupId, cdsmInfo);

    std::shared_ptr<SlePeripheralDevice> dev1 = std::make_shared<SlePeripheralDevice>();
    dev1->SetAddress(inputAddr);
    dev1->SetPairedStatus(static_cast<int>(SlePairState::SLE_PAIR_NONE));
    std::shared_ptr<SlePeripheralDevice> dev2 = std::make_shared<SlePeripheralDevice>();
    dev2->SetAddress(otherMemberAddr);
    dev2->SetPairedStatus(static_cast<int>(SlePairState::SLE_PAIR_PAIRED));
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(inputAddr.GetAddress(), dev1);
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(otherMemberAddr.GetAddress(), dev2);

    int result = g_adapter->GetPairState(inputAddr);
    EXPECT_EQ(static_cast<int>(SlePairState::SLE_PAIR_PAIRED), result);
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(inputAddr.GetAddress());
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(otherMemberAddr.GetAddress());
    cdsmService->cdsmList_.Erase(testGroupId);
    HILOGI("SleAdapterSecurityTest: GetPairState001 end");
}

/**
 * @tc.name: GetPairedDevices001
 * @tc.desc: 验证 GetPairedDevices
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, GetPairedDevices001)
{
    HILOGI("SleAdapterSecurityTest: GetPairedDevices001 start");
    RawAddress memberAddr("11:22:33:44:55:66");
    RawAddress reportAddr("AA:BB:CC:DD:EE:FF");
    uint32_t testGroupId = 0x12345678;
    CdsmService *cdsmService = CdsmService::GetService();
    auto cdsmInfo = std::make_shared<CdsmInfo>(testGroupId, reportAddr.GetAddress());
    cdsmInfo->CdsmAddMemberInfo(memberAddr.GetAddress(), 1);
    cdsmService->cdsmList_.EnsureInsert(testGroupId, cdsmInfo);
    std::shared_ptr<SlePeripheralDevice> peerDevice = std::make_shared<SlePeripheralDevice>();
    peerDevice->SetAddress(memberAddr);
    peerDevice->SetPairedStatus(static_cast<int>(SlePairState::SLE_PAIR_PAIRED));
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(memberAddr.GetAddress(), peerDevice);
    std::vector<RawAddress> result = g_adapter->GetPairedDevices();
    bool foundReport = (std::find(result.begin(), result.end(), reportAddr) != result.end());
    bool foundMember = (std::find(result.begin(), result.end(), memberAddr) != result.end());
    EXPECT_TRUE(foundReport);
    EXPECT_FALSE(foundMember);
    EXPECT_EQ(1, result.size());
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(memberAddr.GetAddress());
    cdsmService->cdsmList_.Erase(testGroupId);
    HILOGI("SleAdapterSecurityTest: GetPairedDevices001 end");
}

/**
 * @tc.name: IsAcbEncryptedTask001
 * @tc.desc: 验证 IsAcbEncryptedTask - CDSM组内设备加密状态检查
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, IsAcbEncryptedTask001)
{
    HILOGI("SleAdapterSecurityTest: IsAcbEncryptedTask001 start");
    RawAddress inputAddr("11:22:33:44:55:66");
    RawAddress member1Addr("AA:BB:CC:DD:EE:FF");
    RawAddress member2Addr("00:11:22:33:44:55");
    uint32_t testGroupId = 0x12345678;
    CdsmService *cdsmService = CdsmService::GetService();
    auto cdsmInfo = std::make_shared<CdsmInfo>(testGroupId, member1Addr.GetAddress());
    cdsmInfo->CdsmAddMemberInfo(inputAddr.GetAddress(), 1);
    cdsmInfo->CdsmAddMemberInfo(member1Addr.GetAddress(), 1);
    cdsmInfo->CdsmAddMemberInfo(member2Addr.GetAddress(), 1);
    cdsmService->cdsmList_.EnsureInsert(testGroupId, cdsmInfo);

    std::shared_ptr<SlePeripheralDevice> dev1 = std::make_shared<SlePeripheralDevice>();
    dev1->SetAddress(inputAddr);
    dev1->SetAcbConnectState(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_ENCRYPTED));
    std::shared_ptr<SlePeripheralDevice> dev2 = std::make_shared<SlePeripheralDevice>();
    dev2->SetAddress(member1Addr);
    dev2->SetAcbConnectState(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED));
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(inputAddr.GetAddress(), dev1);
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(member1Addr.GetAddress(), dev2);

    bool result = g_adapter->IsAcbEncryptedTask(inputAddr);
    EXPECT_TRUE(result);

    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(inputAddr.GetAddress());
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(member1Addr.GetAddress());
    cdsmService->cdsmList_.Erase(testGroupId);
    HILOGI("SleAdapterSecurityTest: IsAcbEncryptedTask001 end");
}

/**
 * @tc.name: IsAcbConnectedTask001
 * @tc.desc: 验证 IsAcbConnectedTask - CDSM组内设备连接状态检查
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, IsAcbConnectedTask001)
{
    HILOGI("SleAdapterSecurityTest: IsAcbConnectedTask001 start");
    RawAddress inputAddr("11:22:33:44:55:66");
    RawAddress member1Addr("AA:BB:CC:DD:EE:FF");
    RawAddress member2Addr("00:11:22:33:44:55");
    uint32_t testGroupId = 0x12345678;
    CdsmService *cdsmService = CdsmService::GetService();
    auto cdsmInfo = std::make_shared<CdsmInfo>(testGroupId, member1Addr.GetAddress());
    cdsmInfo->CdsmAddMemberInfo(inputAddr.GetAddress(), 1);
    cdsmInfo->CdsmAddMemberInfo(member1Addr.GetAddress(), 1);
    cdsmInfo->CdsmAddMemberInfo(member2Addr.GetAddress(), 1);
    cdsmService->cdsmList_.EnsureInsert(testGroupId, cdsmInfo);
    std::shared_ptr<SlePeripheralDevice> dev1 = std::make_shared<SlePeripheralDevice>();
    dev1->SetAddress(inputAddr);
    dev1->SetAcbConnectState(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED));
    std::shared_ptr<SlePeripheralDevice> dev2 = std::make_shared<SlePeripheralDevice>();
    dev2->SetAddress(member1Addr);
    dev2->SetAcbConnectState(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTING));
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(inputAddr.GetAddress(), dev1);
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(member1Addr.GetAddress(), dev2);
    bool result = g_adapter->IsAcbConnectedTask(inputAddr);
    EXPECT_TRUE(result);
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(inputAddr.GetAddress());
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(member1Addr.GetAddress());
    cdsmService->cdsmList_.Erase(testGroupId);
    HILOGI("SleAdapterSecurityTest: IsAcbConnectedTask001 end");
}

/**
 * @tc.name: GetConnectedDevices001
 * @tc.desc: 验证 GetConnectedDevices
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, GetConnectedDevices001)
{
    HILOGI("SleAdapterSecurityTest: GetConnectedDevices001 start");
    RawAddress reportAddr("AA:BB:CC:DD:EE:FF");
    RawAddress memberAddr1("11:22:33:44:55:66");
    RawAddress memberAddr2("22:33:44:55:66:77");

    uint32_t testGroupId = 0x12345678;
    CdsmService *cdsmService = CdsmService::GetService();
    auto cdsmInfo = std::make_shared<CdsmInfo>(testGroupId, reportAddr.GetAddress());
    cdsmInfo->CdsmAddMemberInfo(memberAddr1.GetAddress(), 1);
    cdsmInfo->CdsmAddMemberInfo(memberAddr2.GetAddress(), 1);
    cdsmService->cdsmList_.EnsureInsert(testGroupId, cdsmInfo);

    std::shared_ptr<SlePeripheralDevice> peerDevice1 = std::make_shared<SlePeripheralDevice>();
    peerDevice1->SetAddress(memberAddr1);
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(memberAddr1.GetAddress(), peerDevice1);

    std::shared_ptr<SlePeripheralDevice> peerDevice2 = std::make_shared<SlePeripheralDevice>();
    peerDevice2->SetAddress(memberAddr2);
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(memberAddr2.GetAddress(), peerDevice2);
    std::vector<RawAddress> result = g_adapter->GetConnectedDevices();

    EXPECT_EQ(1, result.size());
    EXPECT_EQ(reportAddr, result[0]);
    EXPECT_FALSE(std::find(result.begin(), result.end(), memberAddr1) != result.end());
    EXPECT_FALSE(std::find(result.begin(), result.end(), memberAddr2) != result.end());

    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(memberAddr1.GetAddress());
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(memberAddr2.GetAddress());
    cdsmService->cdsmList_.Erase(testGroupId);
    HILOGI("SleAdapterSecurityTest: GetConnectedDevices001 end");
}

/**
 * @tc.name: HasConnectedDevice001
 * @tc.desc: 验证 HasConnectedDevice
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, HasConnectedDevice001)
{
    HILOGI("SleAdapterSecurityTest: HasConnectedDevice001 start");
    RawAddress device("00:11:22:33:44:55");
    std::shared_ptr<SlePeripheralDevice> peerDevice = std::make_shared<SlePeripheralDevice>();
    peerDevice->SetAcbConnectState(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED));
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(device.GetAddress(), peerDevice);
    bool result = g_adapter->HasConnectedDevice();
    EXPECT_EQ(true, result);
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(device.GetAddress());
    HILOGI("SleAdapterSecurityTest: HasConnectedDevice001 end");
}

/**
 * @tc.name: IsBondedFromLocal001
 * @tc.desc: 验证 IsBondedFromLocal
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, IsBondedFromLocal001)
{
    HILOGI("SleAdapterSecurityTest: IsBondedFromLocalTask001 start");
    RawAddress device("00:11:22:33:44:55");
    std::shared_ptr<SlePeripheralDevice> peerDevice = std::make_shared<SlePeripheralDevice>();
    peerDevice->SetAcbConnectState(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_ENCRYPTED));
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(device.GetAddress(), peerDevice);
    bool result = SleRemoteDeviceAdapter::GetInstance()->IsBondedFromLocal(device);
    EXPECT_EQ(true, result);
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(device.GetAddress());
    HILOGI("SleAdapterSecurityTest: IsBondedFromLocalTask001 end");
}

/**
 * @tc.name: GetLcidByAddress001
 * @tc.desc: 验证 GetLcidByAddress
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, GetLcidByAddress001)
{
    HILOGI("SleAdapterSecurityTest: GetLcidByAddress001 start");
    RawAddress device("00:11:22:33:44:55");
    std::shared_ptr<SlePeripheralDevice> peerDevice = std::make_shared<SlePeripheralDevice>();
    peerDevice->SetLcid(1);
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(device.GetAddress(), peerDevice);
    bool result = SleRemoteDeviceAdapter::GetInstance()->GetLcidByAddress(device);
    EXPECT_EQ(1, result);
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(device.GetAddress());
    HILOGI("SleAdapterSecurityTest: GetLcidByAddress001 end");
}

/**
 * @tc.name: GetAddressByLcid001
 * @tc.desc: 验证 GetAddressByLcid
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, GetAddressByLcid001)
{
    HILOGI("SleAdapterSecurityTest: GetAddressByLcid001 start");
    uint16_t lcid = 0x0040;
    std::string device = "AA:BB:CC:DD:EE:FF";
    std::shared_ptr<SlePeripheralDevice> peerDevice = std::make_shared<SlePeripheralDevice>();
    peerDevice->SetLcid(lcid); 
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(device, peerDevice);
    std::string result = SleRemoteDeviceAdapter::GetInstance()->GetAddressByLcid(lcid);
    EXPECT_EQ(device, result);
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(device);
    HILOGI("SleAdapterSecurityTest: GetAddressByLcid001 end");
}

/**
 * @tc.name: CdsmSaveData001
 * @tc.desc: 验证 CdsmSaveData - 保存CDSM设备信息及更新地址类型
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, CdsmSaveData001)
{
    HILOGI("SleAdapterSecurityTest: CdsmSaveData001 start");
    RawAddress reportAddr("AA:BB:CC:DD:EE:FF"); // 主设备地址
    RawAddress memberAddr("11:22:33:44:55:66"); // 成员设备地址
    uint32_t testGroupId = 0x12345678;
    CdsmService *cdsmService = CdsmService::GetService();

    SleRemoteDeviceAdapter::GetInstance()->CdsmSaveData(memberAddr);

    auto cdsmInfo = std::make_shared<CdsmInfo>(testGroupId, reportAddr.GetAddress());
    cdsmInfo->CdsmAddMemberInfo(memberAddr.GetAddress(), 1);
    cdsmInfo->CdsmAddMemberInfo(reportAddr.GetAddress(), 1);
    cdsmService->cdsmList_.EnsureInsert(testGroupId, cdsmInfo);

    std::shared_ptr<SlePeripheralDevice> memberDev = std::make_shared<SlePeripheralDevice>();
    memberDev->SetAddress(memberAddr);
    memberDev->SetManufacturerBusiness(0);
    std::shared_ptr<SlePeripheralDevice> reportDev = std::make_shared<SlePeripheralDevice>();
    reportDev->SetAddress(reportAddr);
    reportDev->SetManufacturerBusiness(Nearlink::SLE_PRIVATE_AUDIO_BUSINESS_TYPE);
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(memberAddr.GetAddress(), memberDev);
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(reportAddr.GetAddress(), reportDev);

    SleRemoteDeviceAdapter::GetInstance()->CdsmSaveData(memberAddr);

    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(memberAddr.GetAddress());
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(reportAddr.GetAddress());
    cdsmService->cdsmList_.Erase(testGroupId);
    HILOGI("SleAdapterSecurityTest: CdsmSaveData001 end");
}

/**
 * @tc.name: RemoveNotPairedCloudDevice001
 * @tc.desc: 验证 RemoveNotPairedCloudDevice - 移除CDSM组内所有成员的设备信息
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, RemoveNotPairedCloudDevice001)
{
    HILOGI("SleAdapterSecurityTest: RemoveNotPairedCloudDevice001 start");
    RawAddress reportAddr("AA:BB:CC:DD:EE:FF"); // 主设备地址
    RawAddress memberAddr("11:22:33:44:55:66"); // 成员设备地址
    uint32_t testGroupId = 0x12345678;
    CdsmService *cdsmService = CdsmService::GetService();

    auto cdsmInfo = std::make_shared<CdsmInfo>(testGroupId, reportAddr.GetAddress());
    cdsmInfo->CdsmAddMemberInfo(reportAddr.GetAddress(), 1);
    cdsmInfo->CdsmAddMemberInfo(memberAddr.GetAddress(), 1);
    cdsmService->cdsmList_.EnsureInsert(testGroupId, cdsmInfo);
    std::shared_ptr<SlePeripheralDevice> reportDev = std::make_shared<SlePeripheralDevice>();
    reportDev->SetAddress(reportAddr);
    std::shared_ptr<SlePeripheralDevice> memberDev = std::make_shared<SlePeripheralDevice>();
    memberDev->SetAddress(memberAddr);
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(reportAddr.GetAddress(), reportDev);
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(memberAddr.GetAddress(), memberDev);
    g_adapter->pimpl->needReconnectDevices_.Emplace(reportAddr.GetAddress());
    g_adapter->pimpl->needReconnectDevices_.Emplace(memberAddr.GetAddress());
    g_adapter->pimpl->credibleDevice_.Insert(reportAddr.GetAddress());

    g_adapter->RemoveNotPairedCloudDevice(memberAddr);

    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(reportAddr.GetAddress());
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(memberAddr.GetAddress());
    cdsmService->cdsmList_.Erase(testGroupId);
    g_adapter->pimpl->needReconnectDevices_.Erase(reportAddr.GetAddress());
    g_adapter->pimpl->needReconnectDevices_.Erase(memberAddr.GetAddress());
    g_adapter->pimpl->credibleDevice_.Erase(reportAddr.GetAddress());
    HILOGI("SleAdapterSecurityTest: RemoveNotPairedCloudDevice001 end");
}

/**
 * @tc.name: ProcClearOldCdsmGroup001
 * @tc.desc: 验证 ProcClearOldCdsmGroup - 主设备丢失场景
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, ProcClearOldCdsmGroup001)
{
    HILOGI("SleAdapterSecurityTest: ProcClearOldCdsmGroup001 start");
    RawAddress oldReportAddr("AA:BB:CC:DD:EE:FF");
    RawAddress oldMemberAddr("00:11:22:33:44:55");
    RawAddress newCollabAddr("66:55:44:33:22:11");
    uint32_t testGroupId = 0x12345678;

    CdsmService *cdsmService = CdsmService::GetService();
    auto cdsmInfo = std::make_shared<CdsmInfo>(testGroupId, oldReportAddr.GetAddress());
    cdsmInfo->CdsmAddMemberInfo(oldReportAddr.GetAddress(), 1);
    cdsmInfo->CdsmAddMemberInfo(oldMemberAddr.GetAddress(), 1);
    cdsmService->cdsmList_.EnsureInsert(testGroupId, cdsmInfo);

    std::shared_ptr<SlePeripheralDevice> reportDev = std::make_shared<SlePeripheralDevice>();
    reportDev->SetAddress(oldReportAddr);
    reportDev->SetPairedStatus(static_cast<int>(SlePairState::SLE_PAIR_PAIRED));
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(oldReportAddr.GetAddress(), reportDev);

    g_adapter->ProcClearOldCdsmGroup(oldReportAddr, newCollabAddr, true);
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(oldReportAddr.GetAddress());
    cdsmService->cdsmList_.Erase(testGroupId);
    HILOGI("SleAdapterSecurityTest: ProcClearOldCdsmGroup001 end");
}

/**
 * @tc.name: ProcClearOldCdsmGroup002
 * @tc.desc: 验证 ProcClearOldCdsmGroup - 对端设备(原主设备)丢失场景
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, ProcClearOldCdsmGroup002)
{
    HILOGI("SleAdapterSecurityTest: ProcClearOldCdsmGroup002 start");
    RawAddress oldReportAddr("AA:BB:CC:DD:EE:FF");
    RawAddress oldMemberAddr("00:11:22:33:44:55");
    RawAddress newReportAddr("66:55:44:33:22:11");
    
    uint32_t testGroupId = 0x12345678;
    CdsmService *cdsmService = CdsmService::GetService();
    auto cdsmInfo = std::make_shared<CdsmInfo>(testGroupId, oldReportAddr.GetAddress());
    cdsmInfo->CdsmAddMemberInfo(oldReportAddr.GetAddress(), 1);
    cdsmInfo->CdsmAddMemberInfo(oldMemberAddr.GetAddress(), 1);
    cdsmService->cdsmList_.EnsureInsert(testGroupId, cdsmInfo);

    std::shared_ptr<SlePeripheralDevice> collabDev = std::make_shared<SlePeripheralDevice>();
    collabDev->SetAddress(oldReportAddr);
    collabDev->SetPairedStatus(static_cast<int>(SlePairState::SLE_PAIR_PAIRED));
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(oldReportAddr.GetAddress(), collabDev);

    g_adapter->ProcClearOldCdsmGroup(newReportAddr, oldReportAddr, true);
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(oldReportAddr.GetAddress());
    cdsmService->cdsmList_.Erase(testGroupId);
    HILOGI("SleAdapterSecurityTest: ProcClearOldCdsmGroup002 end");
}

/**
 * @tc.name: ProcClearOldCdsmGroup003
 * @tc.desc: 验证 ProcClearOldCdsmGroup - 稳定状态，无需清理
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, ProcClearOldCdsmGroup003)
{
    HILOGI("SleAdapterSecurityTest: ProcClearOldCdsmGroup003 start");
    RawAddress oldReportAddr("AA:BB:CC:DD:EE:FF"); // 旧的主设备
    RawAddress oldMemberAddr("00:11:22:33:44:55"); // 旧的成员设备
    uint32_t testGroupId = 0x12345678;
    CdsmService *cdsmService = CdsmService::GetService();
    auto cdsmInfo = std::make_shared<CdsmInfo>(testGroupId, oldReportAddr.GetAddress());
    cdsmInfo->CdsmAddMemberInfo(oldReportAddr.GetAddress(), 1);
    cdsmInfo->CdsmAddMemberInfo(oldMemberAddr.GetAddress(), 1);
    cdsmService->cdsmList_.EnsureInsert(testGroupId, cdsmInfo);

    std::shared_ptr<SlePeripheralDevice> dev1 = std::make_shared<SlePeripheralDevice>();
    dev1->SetAddress(oldReportAddr);
    dev1->SetPairedStatus(static_cast<int>(SlePairState::SLE_PAIR_PAIRED));
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(oldReportAddr.GetAddress(), dev1);
    std::shared_ptr<SlePeripheralDevice> dev2 = std::make_shared<SlePeripheralDevice>();
    dev2->SetAddress(oldMemberAddr);
    dev2->SetPairedStatus(static_cast<int>(SlePairState::SLE_PAIR_PAIRED));
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(oldMemberAddr.GetAddress(), dev2);

    g_adapter->ProcClearOldCdsmGroup(oldMemberAddr, oldReportAddr, true);

    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(oldReportAddr.GetAddress());
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(oldMemberAddr.GetAddress());
    cdsmService->cdsmList_.Erase(testGroupId);
    HILOGI("SleAdapterSecurityTest: ProcClearOldCdsmGroup003 end");
}

/**
 * @tc.name: IsVendorDevice001
 * @tc.desc: 验证 IsVendorDevice
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, IsVendorDevice001)
{
    HILOGI("SleAdapterSecurityTest: IsVendorDevice001 start");
    RawAddress targetAddr("AA:BB:CC:DD:EE:FF");
    uint32_t testGroupId = 0x12345678;
    CdsmService *cdsmService = CdsmService::GetService();
    auto cdsmInfo = std::make_shared<CdsmInfo>(testGroupId, "00:00:00:00:00:00");
    cdsmInfo->CdsmAddMemberInfo(targetAddr.GetAddress(), 1);
    cdsmService->cdsmList_.EnsureInsert(testGroupId, cdsmInfo);
    std::shared_ptr<SlePeripheralDevice> peerDevice = std::make_shared<SlePeripheralDevice>();
    peerDevice->SetAddress(targetAddr);
    peerDevice->SetManufacturerBusiness(Nearlink::SLE_PRIVATE_AUDIO_BUSINESS_TYPE);
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(targetAddr.GetAddress(), peerDevice);
    bool result = SleRemoteDeviceAdapter::GetInstance()->IsVendorDevice(targetAddr);
    EXPECT_TRUE(result);
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(targetAddr.GetAddress());
    cdsmService->cdsmList_.Erase(testGroupId);
    HILOGI("SleAdapterSecurityTest: IsVendorDevice001 end");
}

/**
 * @tc.name: IsServiceSupportedConn001
 * @tc.desc: 验证 IsServiceSupportedConn
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, IsServiceSupportedConn001)
{
    HILOGI("SleAdapterSecurityTest: IsServiceSupportedConn001 start");
    RawAddress device("AA:BB:CC:DD:EE:FF");
    std::shared_ptr<SlePeripheralDevice> peerDevice = std::make_shared<SlePeripheralDevice>();
    peerDevice->SetIsDeviceAvailable(false);
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(device.GetAddress(), peerDevice);
    bool result = SleRemoteDeviceAdapter::GetInstance()->IsServiceSupportedConn(device);
    EXPECT_FALSE(result);
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(device.GetAddress());
    HILOGI("SleAdapterSecurityTest: IsServiceSupportedConn001 end");
}

/**
* @tc.name: IsNeedToIgnore001
* @tc.desc: 验证 IsNeedToIgnore 函数在 autoConnPolicy 为 AUTO_CONN_EXCEPT_AUDIO_DEVICES 时返回 true
* @tc.type: FUNC
*/
TEST_F(SleAdapterSecurityTest, IsNeedToIgnore001)
{
    HILOGI("SleAdapterSecurityTest: IsNeedToIgnore001 start");
    SleServiceManager::GetInstance()->autoConnPolicy_ = SleAutoConnectPolicy::AUTO_CONN_EXCEPT_AUDIO_DEVICES;
    std::string reconnDevAddr = "AA:BB:CC:DD:EE:FF";
    bool result = SleReconnectManager::GetInstance().IsNeedToIgnore(reconnDevAddr);
    EXPECT_TRUE(result);
    SleServiceManager::GetInstance()->autoConnPolicy_ = SleAutoConnectPolicy::AUTO_CONN_GENERAL;
    HILOGI("SleAdapterSecurityTest: IsNeedToIgnore001 end");
}

/**
 * @tc.name: SetAliasNameTask001
 * @tc.desc: 验证 SetAliasNameTask - CDSM设备设置别名，遍历更新组内所有成员
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, SetAliasNameTask001)
{
    HILOGI("SleAdapterSecurityTest: SetAliasNameTask001 start");
    std::string newAlias = "MyTwsHeadset";
    RawAddress memberAddr("11:22:33:44:55:66");
    RawAddress reportAddr("AA:BB:CC:DD:EE:FF");
    uint32_t testGroupId = 0x12345678;
    CdsmService *cdsmService = CdsmService::GetService();
    auto cdsmInfo = std::make_shared<CdsmInfo>(testGroupId, reportAddr.GetAddress());
    cdsmInfo->CdsmAddMemberInfo(memberAddr.GetAddress(), 1);
    cdsmInfo->CdsmAddMemberInfo(reportAddr.GetAddress(), 1);
    cdsmService->cdsmList_.EnsureInsert(testGroupId, cdsmInfo);
    std::shared_ptr<SlePeripheralDevice> memberDev = std::make_shared<SlePeripheralDevice>();
    memberDev->SetAddress(memberAddr);
    memberDev->SetAliasName("OldAlias"); 
    std::shared_ptr<SlePeripheralDevice> reportDev = std::make_shared<SlePeripheralDevice>();
    reportDev->SetAddress(reportAddr);
    reportDev->SetAliasName("OldAlias");
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(memberAddr.GetAddress(), memberDev);
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(reportAddr.GetAddress(), reportDev);

    bool result = g_adapter->SetAliasNameTask(memberAddr, newAlias);
    EXPECT_TRUE(result);
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(memberAddr.GetAddress());
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(reportAddr.GetAddress());
    cdsmService->cdsmList_.Erase(testGroupId);
    HILOGI("SleAdapterSecurityTest: SetAliasNameTask001 end");
}

/**
 * @tc.name: ProcCdsmDisconnectAllProfile001
 * @tc.desc: 验证 ProcCdsmDisconnectAllProfile
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, ProcCdsmDisconnectAllProfile001)
{
    HILOGI("SleAdapterSecurityTest: ProcCdsmDisconnectAllProfile001 start");
    RawAddress memberAddr("11:22:33:44:55:66");
    RawAddress reportAddr("AA:BB:CC:DD:EE:FF");
    uint32_t testGroupId = 0x12345678;
    CdsmService *cdsmService = CdsmService::GetService();
    auto cdsmInfo = std::make_shared<CdsmInfo>(testGroupId, reportAddr.GetAddress());
    cdsmInfo->CdsmAddMemberInfo(memberAddr.GetAddress(), 1);
    cdsmInfo->CdsmAddMemberInfo(reportAddr.GetAddress(), 1);
    cdsmService->cdsmList_.EnsureInsert(testGroupId, cdsmInfo);
    bool res = true;
    bool result = g_adapter->ProcCdsmDisconnectAllProfile(memberAddr, res, 1);
    EXPECT_TRUE(result);
    cdsmService->cdsmList_.Erase(testGroupId);
    HILOGI("SleAdapterSecurityTest: ProcCdsmDisconnectAllProfile001 end");
}

/**
 * @tc.name: CancelPairingTask001
 * @tc.desc: 验证 CancelPairingTask 配对状态为 SLE_PAIR_NONE or PAIR_CANCELING
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, CancelPairingTask001)
{
    HILOGI("SleAdapterSecurityTest: CancelPairingTask001 start");
    RawAddress device("00:11:22:33:44:55");
    std::shared_ptr<SlePeripheralDevice> peerDevice = std::make_shared<SlePeripheralDevice>();
    peerDevice->SetPairedStatus(static_cast<int>(SlePairState::SLE_PAIR_CANCELING)); 
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(device.GetAddress(), peerDevice);
    bool result1 = g_adapter->CancelPairingTask(device);
    EXPECT_EQ(false, result1);
    peerDevice->SetPairedStatus(static_cast<int>(SlePairState::SLE_PAIR_NONE)); 
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(device.GetAddress(), peerDevice);
    bool result2 = g_adapter->CancelPairingTask(device);
    EXPECT_EQ(false, result2);
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(device.GetAddress());
    HILOGI("SleAdapterSecurityTest: CancelPairingTask001 end");
}

/**
 * @tc.name: SetSleConnectionModeTask001
 * @tc.desc: 验证 设置可发现广播SetSleConnectionModeTask
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, SetSleConnectionModeTask001)
{
    HILOGI("SleAdapterSecurityTest: SetSleConnectionModeTask001 start");
    RawAddress device("00:11:22:33:44:55");
    int32_t connectionMode1 = static_cast<int>(SLEConnectionMode::CONNECTION_MODE_UNCONNECTABLE);
    int32_t duration = 10;
    int32_t connectionMode2 = 123;
    int32_t connectionMode3 = static_cast<int>(SLEConnectionMode::CONNECTION_MODE_CONNECTABLE);
    g_adapter->SetSleConnectionModeTask(connectionMode1, duration);
    g_adapter->SetSleConnectionModeTask(connectionMode2, duration);
    g_adapter->SetSleConnectionModeTask(connectionMode3, 0);
    g_adapter->SetSleConnectable();
    g_adapter->SetSleUnconnectable();
    g_adapter->pimpl->sleConnectableHandle_.store(
        static_cast<uint8_t>(SleAdvertisingHandle::SLE_INVALID_ADVERTISING_HANDLE));
    g_adapter->SetSleUnconnectable();
    HILOGI("SleAdapterSecurityTest: SetSleConnectionModeTask001 end");
}

/**
 * @tc.name: UpdateDeviceModelInfo001
 * @tc.desc: 验证 UpdateDeviceModelInfo - CDSM设备批量更新模型信息
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, UpdateDeviceModelInfo001)
{
    HILOGI("SleAdapterSecurityTest: UpdateDeviceModelInfo001 start");
    std::string address = "11:22:33:44:55:66";
    std::string modelId = "ModelX";
    std::string newModelId = "ModelY";
    std::string iconId = "IconZ";
    std::string devType = "3";
    CdsmService *cdsmService = CdsmService::GetService();
    uint32_t testGroupId = 0x12345678;
    RawAddress reportAddr("AA:BB:CC:DD:EE:FF");
    auto cdsmInfo = std::make_shared<CdsmInfo>(testGroupId, reportAddr.GetAddress());
    cdsmInfo->CdsmAddMemberInfo(address, 1);
    cdsmInfo->CdsmAddMemberInfo(reportAddr.GetAddress(), 1);
    cdsmService->cdsmList_.EnsureInsert(testGroupId, cdsmInfo);
    
    DeviceModel model;
    model.SetModelId(modelId);
    model.SetIconId(iconId);
    model.SetDevType(devType);
    g_adapter->UpdateDeviceModelInfo(address, model, newModelId);
    HILOGI("SleAdapterSecurityTest: UpdateDeviceModelInfo001 end");
}

/**
 * @tc.name: SetAudioDeviceFlag001
 * @tc.desc: 验证 SetAudioDeviceFlag
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, SetAudioDeviceFlag001)
{
    HILOGI("SleAdapterSecurityTest: SetAudioDeviceFlag001 start");
    RawAddress device("00:11:22:33:44:55");
    SleRemoteDeviceAdapter::GetInstance()->SetAudioDeviceFlag(device);
    HILOGI("SleAdapterSecurityTest: SetAudioDeviceFlag001 end");
}

/**
 * @tc.name: SetDeviceIsAvailable001
 * @tc.desc: 验证 SetDeviceIsAvailable
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, SetDeviceIsAvailable001)
{
    HILOGI("SleAdapterSecurityTest: SetDeviceIsAvailable001 start");
    bool enable = true;
    RawAddress device("00:11:22:33:44:55");
    SleRemoteDeviceAdapter::GetInstance()->SetDeviceIsAvailable(device, enable);
    HILOGI("SleAdapterSecurityTest: SetDeviceIsAvailable001 end");
}

/**
 * @tc.name: FormatModelIdInfo001
 * @tc.desc: 验证 FormatModelIdInfo - 输入长度大于指定长度，截断处理
 * @tc.type: FUNC
 */
TEST(FormatModelIdInfoTest, FormatModelIdInfo001)
{
    std::string input = "12345678";
    uint32_t len = 4;
    std::string result = FormatModelIdInfo(input, len);
    EXPECT_EQ(result, "1234");
}

/**
 * @tc.name: FormatModelIdInfo002
 * @tc.desc: 验证 FormatModelIdInfo - 输入长度小于指定长度，左侧补0
 * @tc.type: FUNC
 */
TEST(FormatModelIdInfoTest, FormatModelIdInfo002)
{
    std::string input = "12";
    uint32_t len = 5;
    std::string result = FormatModelIdInfo(input, len);
    EXPECT_EQ(result, "00012");
}

/**
 * @tc.name: FormatModelIdInfo003
 * @tc.desc: 验证 FormatModelIdInfo - 输入长度等于指定长度，原样返回
 * @tc.type: FUNC
 */
TEST(FormatModelIdInfoTest, FormatModelIdInfo003)
{
    std::string input = "abc";
    uint32_t len = 3;
    std::string result = FormatModelIdInfo(input, len);
    EXPECT_EQ(result, "abc");
}

/**
 * @tc.name: CancelAllConnection001
 * @tc.desc: 验证 CancelAllConnection
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, CancelAllConnection001)
{
    HILOGI("SleAdapterSecurityTest: CancelAllConnection001 start");
    RawAddress device("00:11:22:33:44:55");
    std::shared_ptr<SlePeripheralDevice> peerDevice = std::make_shared<SlePeripheralDevice>();
    peerDevice->SetAcbConnectState(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTING));
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(device.GetAddress(), peerDevice);
    g_adapter->CancelAllConnection();
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(device.GetAddress());
    HILOGI("SleAdapterSecurityTest: CancelAllConnection001 end");
}

/**
 * @tc.name: DisconnectAllProfileInner001
 * @tc.desc: 验证 DisconnectAllProfileInner
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, DisconnectAllProfileInner001)
{
    HILOGI("SleAdapterSecurityTest: DisconnectAllProfileInner001 start");
    RawAddress device("00:11:22:33:44:55");
    std::shared_ptr<SlePeripheralDevice> peerDevice = std::make_shared<SlePeripheralDevice>();
    peerDevice->SetAcbConnectState(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED));
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(device.GetAddress(), peerDevice);
    g_adapter->pimpl->needReconnectDevices_.Emplace(device.GetAddress());
    bool res = true;
    g_adapter->DisconnectAllProfileInner(device, res, 1);
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(device.GetAddress());
    HILOGI("SleAdapterSecurityTest: DisconnectAllProfileInner001 end");
}

/**
 * @tc.name: ConnectAllProfileTask001
 * @tc.desc: 验证 ConnectAllProfileTask
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, ConnectAllProfileTask001)
{
    HILOGI("SleAdapterSecurityTest: ConnectAllProfileTask001 start");
    RawAddress device("00:11:22:33:44:55");
    std::shared_ptr<SlePeripheralDevice> peerDevice = std::make_shared<SlePeripheralDevice>();
    peerDevice->SetPairedStatus(static_cast<int>(SlePairState::SLE_PAIR_PAIRED));
    peerDevice->SetAcbConnectState(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED));
    peerDevice->SetManufacturerBusiness(Nearlink::SLE_PRIVATE_AUDIO_BUSINESS_TYPE);
    peerDevice->SetAppearance(static_cast<uint32_t>(DeviceClassForService::DEVICE_CLASS_VEHICLE_LOCK));
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(device.GetAddress(), peerDevice);
    g_adapter->ConnectAllProfileTask(device);
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(device.GetAddress());
    HILOGI("SleAdapterSecurityTest: ConnectAllProfileTask001 end");
}

/**
 * @tc.name: ConnectAllProfileTask002
 * @tc.desc: 验证 ConnectAllProfileTask
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, ConnectAllProfileTask002)
{
    HILOGI("SleAdapterSecurityTest: ConnectAllProfileTask002 start");
    RawAddress reportAddr("AA:BB:CC:DD:EE:FF");
    RawAddress memberAddr("11:22:33:44:55:66");
    std::shared_ptr<SlePeripheralDevice> peerDevice = std::make_shared<SlePeripheralDevice>();
    peerDevice->SetPairedStatus(static_cast<int>(SlePairState::SLE_PAIR_PAIRED));
    peerDevice->SetAcbConnectState(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_ENCRYPTED));
    peerDevice->SetManufacturerBusiness(Nearlink::SLE_PRIVATE_AUDIO_BUSINESS_TYPE);
    peerDevice->SetAppearance(static_cast<uint32_t>(DeviceClassForService::DEVICE_CLASS_VEHICLE_LOCK));
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(reportAddr.GetAddress(), peerDevice);
    
    uint32_t testGroupId = 0x12345678;
    CdsmService *cdsmService = CdsmService::GetService();
    auto cdsmInfo = std::make_shared<CdsmInfo>(testGroupId, reportAddr.GetAddress());
    cdsmInfo->CdsmAddMemberInfo(reportAddr.GetAddress(), 1);
    cdsmInfo->CdsmAddMemberInfo(memberAddr.GetAddress(), 1);
    cdsmService->cdsmList_.EnsureInsert(testGroupId, cdsmInfo);

    g_adapter->ConnectAllProfileTask(reportAddr);
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(reportAddr.GetAddress());
    cdsmService->cdsmList_.Erase(testGroupId);
    HILOGI("SleAdapterSecurityTest: ConnectAllProfileTask002 end");
}

/**
 * @tc.name: ConnectAllProfileTask003
 * @tc.desc: 验证 ConnectAllProfileTask
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, ConnectAllProfileTask003)
{
    HILOGI("SleAdapterSecurityTest: ConnectAllProfileTask003 start");
    RawAddress device("00:11:22:33:44:55");
    std::shared_ptr<SlePeripheralDevice> peerDevice = std::make_shared<SlePeripheralDevice>();
    peerDevice->SetPairedStatus(static_cast<int>(SlePairState::SLE_PAIR_PAIRED));
    peerDevice->SetAcbConnectState(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTING));
    peerDevice->SetManufacturerBusiness(Nearlink::SLE_PRIVATE_AUDIO_BUSINESS_TYPE);
    peerDevice->SetAppearance(static_cast<uint32_t>(DeviceClassForService::DEVICE_CLASS_VEHICLE_LOCK));
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(device.GetAddress(), peerDevice);
    g_adapter->ConnectAllProfileTask(device);
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(device.GetAddress());
    HILOGI("SleAdapterSecurityTest: ConnectAllProfileTask003 end");
}

/**
 * @tc.name: Get_Pair_Connect_Cnt001
 * @tc.desc: 验证 GetNotPairNoneCnt和GetPairedCnt，GetUnDisconnectedCnt和GetConnectedCnt
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, Get_Pair_Connect_Cnt001)
{
    HILOGI("SleAdapterSecurityTest: Get_Pair_Connect_Cnt001 start");
    RawAddress device("00:11:22:33:44:55");
    int notPairNoneNum = SleRemoteDeviceAdapter::GetInstance()->GetNotPairNoneCnt(device);
    EXPECT_EQ(0, notPairNoneNum);
    int pairedNum = SleRemoteDeviceAdapter::GetInstance()->GetPairedCnt(device);
    EXPECT_EQ(0, pairedNum);

    int unDisconnectedNum = g_adapter->GetUnDisconnectedCnt(device);
    EXPECT_EQ(0, unDisconnectedNum);
    int connectedNum = g_adapter->GetConnectedCnt(device);
    EXPECT_EQ(0, connectedNum);
    HILOGI("SleAdapterSecurityTest: Get_Pair_Connect_Cnt001 end");
}

/**
 * @tc.name: CreateNewPeripheralDevice001
 * @tc.desc: 验证 CreateNewPeripheralDevice - 合作集成员首次配对
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, CreateNewPeripheralDevice001)
{
    HILOGI("SleAdapterSecurityTest: CreateNewPeripheralDevice001 start");
    SLE_Addr_S addr = {
        .type = 0x01,
        .addr = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55}
    };
    uint16_t lcid = 0x0010;
    uint8_t role = 1;
    uint8_t connCompleteType = 0;
    SleConfig::GetInstance().SetSleBusiness(RawAddress::ConvertToString(addr.addr).GetAddress(), 0x02);
    CdsmService *cdsmService = CdsmService::GetService();
    uint32_t testGroupId = 0x12345678;
    RawAddress reportAddr("11:22:33:44:55:66");
    auto cdsmInfo = std::make_shared<CdsmInfo>(testGroupId, reportAddr.GetAddress());
    cdsmInfo->CdsmAddMemberInfo(RawAddress::ConvertToString(addr.addr).GetAddress(), 1);
    cdsmService->cdsmList_.EnsureInsert(testGroupId, cdsmInfo);

    g_adapter->CreateNewPeripheralDevice(addr, lcid, role, connCompleteType);

    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(RawAddress::ConvertToString(addr.addr).GetAddress());
    cdsmService->cdsmList_.Erase(testGroupId);
    HILOGI("SleAdapterSecurityTest: CreateNewPeripheralDevice001 end");
}

TEST_F(SleAdapterSecurityTest, SaveCdsmOtherAddressKey001)
{
    HILOGI("SleAdapterSecurityTest: SaveCdsmOtherAddressKey001 start");
    RawAddress memberAddr("11:22:33:44:55:66");
    RawAddress otherAddr("AA:BB:CC:DD:EE:FF");
    uint32_t testGroupId = 0x12345678;
    EncryptedLinkKey sharedKeys = {0};
    EncryptedLinkKey sharedGroupkeys = {0};
    NLSTK_SmAuthComplete_S param = {};
    param.cryptoAlgo = 1;
    param.keyDerivAlgo = 1;
    param.intgChkInd = 1;
    param.giv = 0x12345678;
    for (int i = 0; i < SM_LINK_KEY_LEN; ++i) {
        param.linkKey[i] = static_cast<uint8_t>(i + 1);
    }
    std::shared_ptr<SlePeripheralDevice> peerDevice = std::make_shared<SlePeripheralDevice>();
    peerDevice->SetManufacturerBusiness(0x01);
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(memberAddr.GetAddress(), peerDevice);

    CdsmService *cdsmService = CdsmService::GetService();
    auto cdsmInfo = std::make_shared<CdsmInfo>(testGroupId, memberAddr.GetAddress());
    cdsmInfo->CdsmAddMemberInfo(memberAddr.GetAddress(), 1);
    cdsmInfo->CdsmAddMemberInfo(otherAddr.GetAddress(), 1);
    cdsmService->cdsmList_.EnsureInsert(testGroupId, cdsmInfo);
 
    sleSecurity_.SaveCdsmOtherAddressKey(memberAddr, sharedKeys, sharedGroupkeys, param);
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(memberAddr.GetAddress());
    cdsmService->cdsmList_.Erase(testGroupId);
    HILOGI("SleAdapterSecurityTest: SaveCdsmOtherAddressKey001 end");
}

/**
 * @tc.name: CreateNewPeripheralDevice002
 * @tc.desc: 验证 CreateNewPeripheralDevice - 私有音频设备
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, CreateNewPeripheralDevice002)
{
    HILOGI("SleAdapterSecurityTest: CreateNewPeripheralDevice002 start");
    SLE_Addr_S addr = {
        .type = 0x01,
        .addr = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55}
    };
    uint16_t lcid = 0x0011;
    uint8_t role = 1;
    uint8_t connCompleteType = 0;
    SleConfig::GetInstance().SetSleBusiness(RawAddress::ConvertToString(addr.addr).GetAddress(), 
        Nearlink::SLE_PRIVATE_AUDIO_BUSINESS_TYPE);
    CdsmService *cdsmService = CdsmService::GetService();
    uint32_t testGroupId = 0x12345679;
    auto cdsmInfo = std::make_shared<CdsmInfo>(testGroupId, "11:22:33:44:55:66");
    cdsmInfo->CdsmAddMemberInfo(RawAddress::ConvertToString(addr.addr).GetAddress(), 1);
    cdsmService->cdsmList_.EnsureInsert(testGroupId, cdsmInfo);
    g_adapter->CreateNewPeripheralDevice(addr, lcid, role, connCompleteType);
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(RawAddress::ConvertToString(addr.addr).GetAddress());
    cdsmService->cdsmList_.Erase(testGroupId);
    HILOGI("SleAdapterSecurityTest: CreateNewPeripheralDevice002 end");
}

/**
 * @tc.name: CreateNewPeripheralDevice003
 * @tc.desc: 验证 CreateNewPeripheralDevice - 非CDSM成员
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, CreateNewPeripheralDevice003)
{
    HILOGI("SleAdapterSecurityTest: CreateNewPeripheralDevice003 start");
    SLE_Addr_S addr = {
        .type = 0x01,
        .addr = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55}
    }; 
    uint16_t lcid = 0x0012;
    uint8_t role = 1;
    uint8_t connCompleteType = 0;
    SleConfig::GetInstance().SetSleBusiness(RawAddress::ConvertToString(addr.addr).GetAddress(), 0);
    CdsmService *cdsmService = CdsmService::GetService();
    g_adapter->CreateNewPeripheralDevice(addr, lcid, role, connCompleteType);
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(RawAddress::ConvertToString(addr.addr).GetAddress());
    HILOGI("SleAdapterSecurityTest: CreateNewPeripheralDevice003 end");
}

/**
 * @tc.name: GetNotPairNoneCnt001
 * @tc.desc: 验证 GetNotPairNoneCnt - 统计非NONE状态的成员数量
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, GetNotPairNoneCnt001)
{
    HILOGI("SleAdapterSecurityTest: GetNotPairNoneCnt001 start");
    RawAddress reportAddr("AA:BB:CC:DD:EE:FF");
    RawAddress memberAddr1("11:22:33:44:55:66");
    RawAddress memberAddr2("22:33:44:55:66:77");
    uint32_t testGroupId = 0x12345678;

    CdsmService *cdsmService = CdsmService::GetService();
    auto cdsmInfo = std::make_shared<CdsmInfo>(testGroupId, reportAddr.GetAddress());
    cdsmInfo->CdsmAddMemberInfo(reportAddr.GetAddress(), 1);
    cdsmInfo->CdsmAddMemberInfo(memberAddr1.GetAddress(), 1);
    cdsmInfo->CdsmAddMemberInfo(memberAddr2.GetAddress(), 1);
    cdsmService->cdsmList_.EnsureInsert(testGroupId, cdsmInfo);

    std::shared_ptr<SlePeripheralDevice> dev1 = std::make_shared<SlePeripheralDevice>();
    dev1->SetAddress(reportAddr);
    dev1->SetPairedStatus(static_cast<int>(SlePairState::SLE_PAIR_PAIRED));
    std::shared_ptr<SlePeripheralDevice> dev2 = std::make_shared<SlePeripheralDevice>();
    dev2->SetAddress(memberAddr1);
    dev2->SetPairedStatus(static_cast<int>(SlePairState::SLE_PAIR_NONE));
    std::shared_ptr<SlePeripheralDevice> dev3 = std::make_shared<SlePeripheralDevice>();
    dev3->SetAddress(memberAddr2);
    dev3->SetPairedStatus(static_cast<int>(SlePairState::SLE_PAIR_PAIRING));
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(reportAddr.GetAddress(), dev1);
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(memberAddr1.GetAddress(), dev2);
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(memberAddr2.GetAddress(), dev3);

    int result = SleRemoteDeviceAdapter::GetInstance()->GetNotPairNoneCnt(reportAddr);
    EXPECT_EQ(2, result);

    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(reportAddr.GetAddress());
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(memberAddr1.GetAddress());
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(memberAddr2.GetAddress());
    cdsmService->cdsmList_.Erase(testGroupId);
    HILOGI("SleAdapterSecurityTest: GetNotPairNoneCnt001 end");
}

/**
 * @tc.name: GetPairedCnt001
 * @tc.desc: 验证 GetPairedCnt - 统计已配对(PAIRED)状态的成员数量
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, GetPairedCnt001)
{
    HILOGI("SleAdapterSecurityTest: GetPairedCnt001 start");
    RawAddress reportAddr("AA:BB:CC:DD:EE:FF");
    RawAddress memberAddr1("11:22:33:44:55:66");
    RawAddress memberAddr2("22:33:44:55:66:77");
    uint32_t testGroupId = 0x12345678;
    CdsmService *cdsmService = CdsmService::GetService();
    auto cdsmInfo = std::make_shared<CdsmInfo>(testGroupId, reportAddr.GetAddress());
    cdsmInfo->CdsmAddMemberInfo(reportAddr.GetAddress(), 1);
    cdsmInfo->CdsmAddMemberInfo(memberAddr1.GetAddress(), 1);
    cdsmInfo->CdsmAddMemberInfo(memberAddr2.GetAddress(), 1);
    cdsmService->cdsmList_.EnsureInsert(testGroupId, cdsmInfo);

    std::shared_ptr<SlePeripheralDevice> dev1 = std::make_shared<SlePeripheralDevice>();
    dev1->SetAddress(reportAddr);
    dev1->SetPairedStatus(static_cast<int>(SlePairState::SLE_PAIR_PAIRED));
    std::shared_ptr<SlePeripheralDevice> dev2 = std::make_shared<SlePeripheralDevice>();
    dev2->SetAddress(memberAddr1);
    dev2->SetPairedStatus(static_cast<int>(SlePairState::SLE_PAIR_NONE));
    std::shared_ptr<SlePeripheralDevice> dev3 = std::make_shared<SlePeripheralDevice>();
    dev3->SetAddress(memberAddr2);
    dev3->SetPairedStatus(static_cast<int>(SlePairState::SLE_PAIR_PAIRING));
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(reportAddr.GetAddress(), dev1);
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(memberAddr1.GetAddress(), dev2);
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(memberAddr2.GetAddress(), dev3);

    int result = SleRemoteDeviceAdapter::GetInstance()->GetPairedCnt(reportAddr);
    EXPECT_EQ(1, result);

    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(reportAddr.GetAddress());
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(memberAddr1.GetAddress());
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(memberAddr2.GetAddress());
    cdsmService->cdsmList_.Erase(testGroupId);
    HILOGI("SleAdapterSecurityTest: GetPairedCnt001 end");
}

/**
 * @tc.name: GetUnDisconnectedCnt001
 * @tc.desc: 验证 GetUnDisconnectedCnt - 统计CDSM组内未断开设备数量
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, GetUnDisconnectedCnt001)
{
    HILOGI("SleAdapterSecurityTest: GetUnDisconnectedCnt001 start");
    RawAddress reportAddr("AA:BB:CC:DD:EE:FF");
    RawAddress member1Addr("11:22:33:44:55:66");  
    RawAddress member2Addr("22:33:44:55:66:77");
    uint32_t testGroupId = 0x12345678;
    CdsmService *cdsmService = CdsmService::GetService();

    auto cdsmInfo = std::make_shared<CdsmInfo>(testGroupId, reportAddr.GetAddress());
    cdsmInfo->CdsmAddMemberInfo(reportAddr.GetAddress(), 1);
    cdsmInfo->CdsmAddMemberInfo(member1Addr.GetAddress(), 1);
    cdsmInfo->CdsmAddMemberInfo(member2Addr.GetAddress(), 1);
    cdsmService->cdsmList_.EnsureInsert(testGroupId, cdsmInfo);

    std::shared_ptr<SlePeripheralDevice> dev1 = std::make_shared<SlePeripheralDevice>();
    dev1->SetAddress(reportAddr);
    dev1->SetAcbConnectState(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTING));
    std::shared_ptr<SlePeripheralDevice> dev2 = std::make_shared<SlePeripheralDevice>();
    dev2->SetAddress(member1Addr);
    dev2->SetAcbConnectState(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED));
    std::shared_ptr<SlePeripheralDevice> dev3 = std::make_shared<SlePeripheralDevice>();
    dev3->SetAddress(member2Addr);
    dev3->SetAcbConnectState(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED));
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(reportAddr.GetAddress(), dev1);
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(member1Addr.GetAddress(), dev2);
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(member2Addr.GetAddress(), dev3);

    int result = g_adapter->GetUnDisconnectedCnt(reportAddr);

    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(reportAddr.GetAddress());
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(member1Addr.GetAddress());
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(member2Addr.GetAddress());
    cdsmService->cdsmList_.Erase(testGroupId);
    HILOGI("SleAdapterSecurityTest: GetUnDisconnectedCnt001 end");
}

/**
 * @tc.name: GetConnectedCnt001
 * @tc.desc: 验证 GetConnectedCnt - 统计CDSM组内Profile全连接的成员数量
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, GetConnectedCnt001)
{
    HILOGI("SleAdapterSecurityTest: GetConnectedCnt001 start");
    RawAddress reportAddr("AA:BB:CC:DD:EE:FF");
    RawAddress member1Addr("11:22:33:44:55:66");
    RawAddress member2Addr("22:33:44:55:66:77");
    uint32_t testGroupId = 0x12345678;
    CdsmService *cdsmService = CdsmService::GetService();
    auto cdsmInfo = std::make_shared<CdsmInfo>(testGroupId, reportAddr.GetAddress());
    cdsmInfo->CdsmAddMemberInfo(reportAddr.GetAddress(), 1);
    cdsmInfo->CdsmAddMemberInfo(member1Addr.GetAddress(), 1);
    cdsmInfo->CdsmAddMemberInfo(member2Addr.GetAddress(), 1);
    cdsmService->cdsmList_.EnsureInsert(testGroupId, cdsmInfo);
    std::shared_ptr<SlePeripheralDevice> dev1 = std::make_shared<SlePeripheralDevice>();
    dev1->SetAddress(reportAddr);
    std::shared_ptr<SlePeripheralDevice> dev2 = std::make_shared<SlePeripheralDevice>();
    dev2->SetAddress(member1Addr);
    std::shared_ptr<SlePeripheralDevice> dev3 = std::make_shared<SlePeripheralDevice>();
    dev3->SetAddress(member2Addr);
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(reportAddr.GetAddress(), dev1);
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(member1Addr.GetAddress(), dev2);
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(member2Addr.GetAddress(), dev3);
    int result = g_adapter->GetConnectedCnt(reportAddr);
    EXPECT_EQ(0, result);
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(reportAddr.GetAddress());
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(member1Addr.GetAddress());
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(member2Addr.GetAddress());
    cdsmService->cdsmList_.Erase(testGroupId);
    HILOGI("SleAdapterSecurityTest: GetConnectedCnt001 end");
}

/**
 * @tc.name: ProcCdsmDeviceConnect001
 * @tc.desc: 验证 ProcCdsmDeviceConnect
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, ProcCdsmDeviceConnect001)
{
    HILOGI("SleAdapterSecurityTest: ProcCdsmDeviceConnect001 start");
    RawAddress addr("12:34:56:78:9A:BC");
    bool result1 = g_adapter->ProcCdsmDeviceConnect(addr);
    EXPECT_EQ(false, result1);

    RawAddress reportAddr("AA:BB:CC:DD:EE:FF");
    RawAddress memberAddr("11:22:33:44:55:66");
    uint32_t testGroupId = 0x12345678;
    CdsmService *cdsmService = CdsmService::GetService();
    auto cdsmInfo = std::make_shared<CdsmInfo>(testGroupId, reportAddr.GetAddress());
    cdsmInfo->CdsmAddMemberInfo(memberAddr.GetAddress(), 1);
    cdsmInfo->CdsmAddMemberInfo(reportAddr.GetAddress(), 1);
    cdsmService->cdsmList_.EnsureInsert(testGroupId, cdsmInfo);
    std::shared_ptr<SlePeripheralDevice> dev1 = std::make_shared<SlePeripheralDevice>();
    dev1->SetAddress(memberAddr);
    std::shared_ptr<SlePeripheralDevice> dev2 = std::make_shared<SlePeripheralDevice>();
    dev2->SetAddress(reportAddr);
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(memberAddr.GetAddress(), dev1);
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(reportAddr.GetAddress(), dev2);
    bool result = g_adapter->ProcCdsmDeviceConnect(memberAddr);
    EXPECT_TRUE(result);
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(memberAddr.GetAddress());
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(reportAddr.GetAddress());
    cdsmService->cdsmList_.Erase(testGroupId);
    HILOGI("SleAdapterSecurityTest: ProcCdsmDeviceConnect001 end");
}

/**
 * @tc.name: GetProfileConnStateTask001
 * @tc.desc: 验证 GetProfileConnStateTask
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, GetProfileConnStateTask001)
{
    HILOGI("SleAdapterSecurityTest: GetProfileConnStateTask001 start");
    RawAddress reportAddr("AA:BB:CC:DD:EE:FF");
    RawAddress memberAddr("11:22:33:44:55:66");
    uint32_t testGroupId = 0x12345678;
    CdsmService *cdsmService = CdsmService::GetService();
    auto cdsmInfo = std::make_shared<CdsmInfo>(testGroupId, reportAddr.GetAddress());
    cdsmInfo->CdsmAddMemberInfo(memberAddr.GetAddress(), 1);
    cdsmInfo->CdsmAddMemberInfo(reportAddr.GetAddress(), 1);
    cdsmService->cdsmList_.EnsureInsert(testGroupId, cdsmInfo);
    int result = g_adapter->GetProfileConnStateTask(reportAddr);
    EXPECT_EQ(static_cast<int>(SleConnectState::DISCONNECTED), result);
    cdsmService->cdsmList_.Erase(testGroupId);
    HILOGI("SleAdapterSecurityTest: GetProfileConnStateTask001 end");
}

/**
 * @tc.name: SetOtherDeviceInfo001
 * @tc.desc: 验证 SetOtherDeviceInfo - 复制设备属性
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, SetOtherDeviceInfo001)
{
    HILOGI("SleAdapterSecurityTest: SetOtherDeviceInfo001 start");
    std::string testName = "TestDevice";
    int testAppearance = 192;
    std::string testModelId = "ModelX";
    std::shared_ptr<SlePeripheralDevice> dev = std::make_shared<SlePeripheralDevice>();
    dev->SetName(testName);
    dev->SetAppearance(testAppearance);
    dev->SetManufacturerBusiness(0);
    dev->SetModelId(testModelId);
    dev->SetAddressType(1);
    dev->SetIconId("Icon1");
    std::shared_ptr<const SlePeripheralDevice> srcDev = dev;
    std::shared_ptr<SlePeripheralDevice> otherDev = std::make_shared<SlePeripheralDevice>();
    SleRemoteDeviceAdapter::GetInstance()->SetOtherDeviceInfo(srcDev, otherDev);
    HILOGI("SleAdapterSecurityTest: SetOtherDeviceInfo001 end");
}

/**
 * @tc.name: CdsmAddOtherRecord001
 * @tc.desc: 验证 CdsmAddOtherRecord - 添加CDSM对端设备记录(Report角色)
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, CdsmAddOtherRecord001)
{
    HILOGI("SleAdapterSecurityTest: CdsmAddOtherRecord001 start");
    RawAddress srcAddr("11:22:33:44:55:66");
    RawAddress otherAddr("AA:BB:CC:DD:EE:FF");
    uint32_t testGroupId = 0x12345678;
    std::shared_ptr<SlePeripheralDevice> srcDev = std::make_shared<SlePeripheralDevice>();
    srcDev->SetAddress(srcAddr);
    srcDev->SetName("SourceDevice");
    srcDev->SetAppearance(100);
    srcDev->SetManufacturerBusiness(0);
    srcDev->SetModelId("ModelS");
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(srcAddr.GetAddress(), srcDev);
    CdsmService *cdsmService = CdsmService::GetService();
    auto cdsmInfo = std::make_shared<CdsmInfo>(testGroupId, otherAddr.GetAddress());
    cdsmInfo->CdsmAddMemberInfo(otherAddr.GetAddress(), 1);
    cdsmService->cdsmList_.EnsureInsert(testGroupId, cdsmInfo);
    SleRemoteDeviceAdapter::GetInstance()->CdsmAddOtherRecord(srcAddr, otherAddr);
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(srcAddr.GetAddress());
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(otherAddr.GetAddress());
    cdsmService->cdsmList_.Erase(testGroupId);
    HILOGI("SleAdapterSecurityTest: CdsmAddOtherRecord001 end");
}

/**
 * @tc.name: UpdateDefaultRole001
 * @tc.desc: 验证 UpdateDefaultRole - 成功路径(仅1个设备加密)
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, UpdateDefaultRole001)
{
    HILOGI("SleAdapterSecurityTest: UpdateDefaultRole001 start");
    RawAddress reportAddr("AA:BB:CC:DD:EE:FF");
    RawAddress memberAddr("11:22:33:44:55:66");
    uint32_t testGroupId = 0x12345678;
    CdsmService *cdsmService = CdsmService::GetService();
    auto cdsmInfo = std::make_shared<CdsmInfo>(testGroupId, reportAddr.GetAddress());
    cdsmInfo->CdsmAddMemberInfo(reportAddr.GetAddress(), 1);
    cdsmInfo->CdsmAddMemberInfo(memberAddr.GetAddress(), 1);
    cdsmService->cdsmList_.EnsureInsert(testGroupId, cdsmInfo);

    std::shared_ptr<SlePeripheralDevice> dev1 = std::make_shared<SlePeripheralDevice>();
    dev1->SetAddress(reportAddr);
    dev1->SetAcbConnectState(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_ENCRYPTED));
    std::shared_ptr<SlePeripheralDevice> dev2 = std::make_shared<SlePeripheralDevice>();
    dev2->SetAddress(memberAddr);
    dev2->SetAcbConnectState(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED));
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(reportAddr.GetAddress(), dev1);
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(memberAddr.GetAddress(), dev2);

    SleRemoteDeviceAdapter::GetInstance()->UpdateDefaultRole(memberAddr);

    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(reportAddr.GetAddress());
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(memberAddr.GetAddress());
    cdsmService->cdsmList_.Erase(testGroupId);
    HILOGI("SleAdapterSecurityTest: UpdateDefaultRole001 end");
}

/**
 * @tc.name: UpdateDefaultRole002
 * @tc.desc: 验证 UpdateDefaultRole - 提前返回(无设备加密)
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, UpdateDefaultRole002)
{
    HILOGI("SleAdapterSecurityTest: UpdateDefaultRole002 start");
    RawAddress reportAddr("AA:BB:CC:DD:EE:FF");
    RawAddress memberAddr("11:22:33:44:55:66");
    uint32_t testGroupId = 0x12345678;
    CdsmService *cdsmService = CdsmService::GetService();
    auto cdsmInfo = std::make_shared<CdsmInfo>(testGroupId, reportAddr.GetAddress());
    cdsmInfo->CdsmAddMemberInfo(reportAddr.GetAddress(), 1);
    cdsmInfo->CdsmAddMemberInfo(memberAddr.GetAddress(), 1);
    cdsmService->cdsmList_.EnsureInsert(testGroupId, cdsmInfo);

    std::shared_ptr<SlePeripheralDevice> dev1 = std::make_shared<SlePeripheralDevice>();
    dev1->SetAddress(reportAddr);
    dev1->SetAcbConnectState(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED));
    std::shared_ptr<SlePeripheralDevice> dev2 = std::make_shared<SlePeripheralDevice>();
    dev2->SetAddress(memberAddr);
    dev2->SetAcbConnectState(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED));
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(reportAddr.GetAddress(), dev1);
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(memberAddr.GetAddress(), dev2);

    SleRemoteDeviceAdapter::GetInstance()->UpdateDefaultRole(memberAddr);

    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(reportAddr.GetAddress());
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(memberAddr.GetAddress());
    cdsmService->cdsmList_.Erase(testGroupId);
    HILOGI("SleAdapterSecurityTest: UpdateDefaultRole002 end");
}

/**
 * @tc.name: UpdateDefaultRole003
 * @tc.desc: 验证 UpdateDefaultRole - 提前返回(多个设备加密)
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, UpdateDefaultRole003)
{
    HILOGI("SleAdapterSecurityTest: UpdateDefaultRole003 start");
    RawAddress reportAddr("AA:BB:CC:DD:EE:FF");
    RawAddress memberAddr("11:22:33:44:55:66");
    uint32_t testGroupId = 0x12345678;
    CdsmService *cdsmService = CdsmService::GetService();
    auto cdsmInfo = std::make_shared<CdsmInfo>(testGroupId, reportAddr.GetAddress());
    cdsmInfo->CdsmAddMemberInfo(reportAddr.GetAddress(), 1);
    cdsmInfo->CdsmAddMemberInfo(memberAddr.GetAddress(), 1);
    cdsmService->cdsmList_.EnsureInsert(testGroupId, cdsmInfo);

    std::shared_ptr<SlePeripheralDevice> dev1 = std::make_shared<SlePeripheralDevice>();
    dev1->SetAddress(reportAddr);
    dev1->SetAcbConnectState(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_ENCRYPTED));
    std::shared_ptr<SlePeripheralDevice> dev2 = std::make_shared<SlePeripheralDevice>();
    dev2->SetAddress(memberAddr);
    dev2->SetAcbConnectState(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_ENCRYPTED));
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(reportAddr.GetAddress(), dev1);
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(memberAddr.GetAddress(), dev2);

    SleRemoteDeviceAdapter::GetInstance()->UpdateDefaultRole(memberAddr);

    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(reportAddr.GetAddress());
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(memberAddr.GetAddress());
    cdsmService->cdsmList_.Erase(testGroupId);
    HILOGI("SleAdapterSecurityTest: UpdateDefaultRole003 end");
}

/**
 * @tc.name: IsProfileStateReport001
 * @tc.desc: 验证 IsProfileStateReport
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, IsProfileStateReport001)
{
    HILOGI("SleAdapterSecurityTest: IsProfileStateReport001 start");
    RawAddress device("00:11:22:33:44:55");
    RawAddress reportAddr("00:11:22:33:44:55");
    CdsmService *cdsmService = CdsmService::GetService();
    int newConnState = 0x01;
    bool ret = g_adapter->IsProfileStateReport(device, reportAddr, cdsmService, newConnState);
    EXPECT_EQ(false, ret);
    HILOGI("SleAdapterSecurityTest: IsProfileStateReport001 end");
}

/**
 * @tc.name: GetGroupAndGiv001
 * @tc.desc: 验证 GetGroupAndGiv
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, GetGroupAndGiv001)
{
    HILOGI("SleAdapterSecurityTest: GetGroupAndGiv001 start");
    RawAddress device("00:11:22:33:44:55");
    std::shared_ptr<SlePeripheralDevice> peerDevice = std::make_shared<SlePeripheralDevice>();
    std::string keyStr = "123456";
    uint64_t g = 1;
    peerDevice->SetEncryptGroupKeyStr(keyStr);
    peerDevice->SetGiv(g);
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(device.GetAddress(), peerDevice);
    std::string encryptGroupKeyStr = "";
    uint64_t giv = 0;
    bool ret = SleRemoteDeviceAdapter::GetInstance()->GetGroupAndGiv(device, encryptGroupKeyStr, giv);
    EXPECT_EQ(true, ret);
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(device.GetAddress());
    HILOGI("SleAdapterSecurityTest: GetGroupAndGiv001 end");
}

/**
 * @tc.name: GetPairAlgoInfo001
 * @tc.desc: 验证 GetPairAlgoInfo
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, GetPairAlgoInfo001)
{
    HILOGI("SleAdapterSecurityTest: GetPairAlgoInfo001 start");
    RawAddress device("00:11:22:33:44:55");
    std::shared_ptr<SlePeripheralDevice> peerDevice = std::make_shared<SlePeripheralDevice>();
    uint8_t param1 = 1;
    uint8_t param2 = 2; 
    uint8_t param3 = 3;
    peerDevice->SetCryptoAlgo(param1);
    peerDevice->SetKeyDerivAlgo(param2);
    peerDevice->SetIntegrChkInd(param3);
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(device.GetAddress(), peerDevice);
    uint8_t cryptoAlgo = 0;
    uint8_t keyDerivAlgo = 0;
    uint8_t integrChkInd = 0;
    bool ret = SleRemoteDeviceAdapter::GetInstance()->GetPairAlgoInfo(device, cryptoAlgo, keyDerivAlgo, integrChkInd);
    EXPECT_EQ(true, ret);
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(device.GetAddress());
    HILOGI("SleAdapterSecurityTest: GetPairAlgoInfo001 end");
}

/**
 * @tc.name: SendImgSecuConfig001
 * @tc.desc: 验证 SendImgSecuConfig
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, SendImgSecuConfig001)
{
    HILOGI("SleAdapterSecurityTest: SendImgSecuConfig001 start");
    RawAddress device("00:11:22:33:44:55");
    g_adapter->SendImgSecuConfig(device);
    HILOGI("SleAdapterSecurityTest: SendImgSecuConfig001 end");
}

/**
 * @tc.name: IsDisconnectedByUser001
 * @tc.desc: 验证 IsDisconnectedByUser
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, IsDisconnectedByUser001)
{
    HILOGI("SleAdapterSecurityTest: IsDisconnectedByUser001 start");
    int acbConnState1 = 0x02; 
    int pairState1 = 0x03;
    int reason1 = 0x11;
    bool ret1 = g_adapter->IsDisconnectedByUser(acbConnState1, pairState1, reason1);
    EXPECT_EQ(true, ret1);
    int acbConnState2 = 0x01; 
    int pairState2 = 0x03;
    int reason2 = 0x11;
    bool ret2 = g_adapter->IsDisconnectedByUser(acbConnState2, pairState2, reason2);
    EXPECT_EQ(false, ret2);
    HILOGI("SleAdapterSecurityTest: IsDisconnectedByUser001 end");
}

/**
 * @tc.name: NeedBgConn001
 * @tc.desc: 验证 NeedBgConn
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, NeedBgConn001)
{
    HILOGI("SleAdapterSecurityTest: NeedBgConn001 start");
    int acbConnState = 0x03;
    int pairState = 0x03;
    RawAddress peerAddr("00:11:22:33:44:55");
    int reason = 1;
    bool ret = g_adapter->NeedBgConn(acbConnState, pairState, peerAddr, reason);
    EXPECT_EQ(false, ret);
    HILOGI("SleAdapterSecurityTest: NeedBgConn001 end");
}

/**
 * @tc.name: SavePairDirect001
 * @tc.desc: 验证 SavePairDirect
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, SavePairDirect001)
{
    HILOGI("SleAdapterSecurityTest: SavePairDirect001 start");
    int connDirect1 = 1;
    RawAddress peerAddr("00:11:22:33:44:55");
    SleRemoteDeviceAdapter::GetInstance()->SavePairDirect(connDirect1, peerAddr);
    int connDirect2 = 0;
    SleRemoteDeviceAdapter::GetInstance()->SavePairDirect(connDirect2, peerAddr);
    HILOGI("SleAdapterSecurityTest: SavePairDirect001 end");
}

/**
 * @tc.name: AddBgConnDevice001
 * @tc.desc: 验证 AddBgConnDevice
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, AddBgConnDevice001)
{
    HILOGI("SleAdapterSecurityTest: AddBgConnDevice001 start");
    RawAddress device("00:11:22:33:44:55");
    std::shared_ptr<SlePeripheralDevice> peerDevice = std::make_shared<SlePeripheralDevice>();
    peerDevice->SetPairDirection(static_cast<int>(SlePairDirect::SLE_PAIR_PASSIVE));
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(device.GetAddress(), peerDevice);
    SleRemoteDeviceAdapter::GetInstance()->AddBgConnDevice(device.GetAddress());
    peerDevice->SetPairDirection(static_cast<int>(SlePairDirect::SLE_PAIR_ACTIVE));
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(device.GetAddress(), peerDevice);
    SleRemoteDeviceAdapter::GetInstance()->AddBgConnDevice(device.GetAddress());
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(device.GetAddress());
    HILOGI("SleAdapterSecurityTest: AddBgConnDevice001 end");
}

/**
 * @tc.name: RemoveBgConnDevice001
 * @tc.desc: 验证 RemoveBgConnDevice
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, RemoveBgConnDevice001)
{
    HILOGI("SleAdapterSecurityTest: RemoveBgConnDevice001 start");
    RawAddress device("00:11:22:33:44:55");
    g_adapter->RemoveBgConnDevice(device.GetAddress());
    HILOGI("SleAdapterSecurityTest: RemoveBgConnDevice001 end");
}

/**
 * @tc.name: IsPairStateReport001
 * @tc.desc: 验证 IsPairStateReport
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, IsPairStateReport001)
{
    HILOGI("SleAdapterSecurityTest: IsPairStateReport001 start");
    RawAddress device("00:11:22:33:44:55");
    RawAddress reportAddr("00:11:22:33:44:55");
    CdsmService *cdsmService = CdsmService::GetService();
    int pairStatus = 1;
    bool ret = g_adapter->IsPairStateReport(device, reportAddr, cdsmService, pairStatus);
    EXPECT_EQ(true, ret);
    HILOGI("SleAdapterSecurityTest: IsPairStateReport001 end");
}

/**
 * @tc.name: IsLlPrivacySupported001
 * @tc.desc: 验证 IsLlPrivacySupported
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, IsLlPrivacySupported001)
{
    HILOGI("SleAdapterSecurityTest: IsLlPrivacySupported001 start");
    bool ret = g_adapter->IsLlPrivacySupported();
    EXPECT_EQ(false, ret);
    HILOGI("SleAdapterSecurityTest: IsLlPrivacySupported001 end");
}

/**
 * @tc.name: SetBleRoles001
 * @tc.desc: 验证 SetBleRoles
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, SetBleRoles001)
{
    HILOGI("SleAdapterSecurityTest: SetBleRoles001 start");
    bool ret = g_adapter->SetBleRoles();
    EXPECT_EQ(0, ret);
    HILOGI("SleAdapterSecurityTest: SetBleRoles001 end");
}

/**
 * @tc.name: Set_Name_and_Appearance001
 * @tc.desc: 验证 Set_Name_and_Appearance
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, Set_Name_and_Appearance001)
{
    HILOGI("SleAdapterSecurityTest: Set_Name_and_Appearance001 start");
    RawAddress device("00:11:22:33:44:55");
    std::string name = "testname";
    bool ret1 = SleRemoteDeviceAdapter::GetInstance()->SetName(device, name);
    EXPECT_EQ(false, ret1);
    int appearance = 1;
    bool ret2 = SleRemoteDeviceAdapter::GetInstance()->SetAppearance(device, appearance);
    EXPECT_EQ(false, ret2);
    HILOGI("SleAdapterSecurityTest: Set_Name_and_Appearance001 end");
}

/**
 * @tc.name: CdsmCancelPairingProcess001
 * @tc.desc: 验证 CdsmCancelPairingProcess - 取消已配对CDSM成员的配对
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, CdsmCancelPairingProcess001)
{
    HILOGI("SleAdapterSecurityTest: CdsmCancelPairingProcess001 start");
    RawAddress reportAddr("AA:BB:CC:DD:EE:FF"); // 主设备地址
    RawAddress memberAddr("11:22:33:44:55:66"); // 成员设备地址
    uint32_t testGroupId = 0x12345678;
    CdsmService *cdsmService = CdsmService::GetService();
    auto cdsmInfo = std::make_shared<CdsmInfo>(testGroupId, reportAddr.GetAddress());
    cdsmInfo->CdsmAddMemberInfo(reportAddr.GetAddress(), 1);
    cdsmInfo->CdsmAddMemberInfo(memberAddr.GetAddress(), 1);
    cdsmService->cdsmList_.EnsureInsert(testGroupId, cdsmInfo);

    std::shared_ptr<SlePeripheralDevice> reportDev = std::make_shared<SlePeripheralDevice>();
    reportDev->SetAddress(reportAddr);
    reportDev->SetPairedStatus(static_cast<int>(SlePairState::SLE_PAIR_PAIRED));
    std::shared_ptr<SlePeripheralDevice> memberDev = std::make_shared<SlePeripheralDevice>();
    memberDev->SetAddress(memberAddr);
    memberDev->SetPairedStatus(static_cast<int>(SlePairState::SLE_PAIR_PAIRED));
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(reportAddr.GetAddress(), reportDev);
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(memberAddr.GetAddress(), memberDev);

    g_adapter->CdsmCancelPairingProcess(reportAddr);

    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(reportAddr.GetAddress());
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(memberAddr.GetAddress());
    cdsmService->cdsmList_.Erase(testGroupId);
    HILOGI("SleAdapterSecurityTest: CdsmCancelPairingProcess001 end");
}

/**
 * @tc.name: SetName001
 * @tc.desc: 验证 SetName - CDSM设备批量更新组内成员名称
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, SetName001)
{
    HILOGI("SleAdapterSecurityTest: SetName001 start");
    RawAddress reportAddr("AA:BB:CC:DD:EE:FF"); // 主设备地址
    RawAddress memberAddr("11:22:33:44:55:66"); // 成员设备地址
    std::string newName = "MyNewHeadset";
    std::string oldName = "OldHeadsetName";
    uint32_t testGroupId = 0x12345678;
    CdsmService *cdsmService = CdsmService::GetService();
    auto cdsmInfo = std::make_shared<CdsmInfo>(testGroupId, reportAddr.GetAddress());
    cdsmInfo->CdsmAddMemberInfo(memberAddr.GetAddress(), 1);
    cdsmInfo->CdsmAddMemberInfo(reportAddr.GetAddress(), 1);
    cdsmService->cdsmList_.EnsureInsert(testGroupId, cdsmInfo);
    std::shared_ptr<SlePeripheralDevice> memberDev = std::make_shared<SlePeripheralDevice>();
    memberDev->SetAddress(memberAddr);
    memberDev->SetName(oldName);
    std::shared_ptr<SlePeripheralDevice> reportDev = std::make_shared<SlePeripheralDevice>();
    reportDev->SetAddress(reportAddr);
    reportDev->SetName(oldName);
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(memberAddr.GetAddress(), memberDev);
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(reportAddr.GetAddress(), reportDev);
    bool result = SleRemoteDeviceAdapter::GetInstance()->SetName(memberAddr, newName);
    EXPECT_TRUE(result);
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(memberAddr.GetAddress());
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(reportAddr.GetAddress());
    cdsmService->cdsmList_.Erase(testGroupId);
    HILOGI("SleAdapterSecurityTest: SetName001 end");
}

/**
 * @tc.name: SetAppearance001
 * @tc.desc: 验证 SetAppearance - CDSM设备批量更新组内成员外观
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, SetAppearance001)
{
    HILOGI("SleAdapterSecurityTest: SetAppearance001 start");
    int oldAppearance = 0;
    int newAppearance = 192;
    RawAddress reportAddr("AA:BB:CC:DD:EE:FF");
    RawAddress memberAddr("11:22:33:44:55:66");
    uint32_t testGroupId = 0x12345678;

    CdsmService *cdsmService = CdsmService::GetService();
    auto cdsmInfo = std::make_shared<CdsmInfo>(testGroupId, reportAddr.GetAddress());
    cdsmInfo->CdsmAddMemberInfo(memberAddr.GetAddress(), 1);
    cdsmInfo->CdsmAddMemberInfo(reportAddr.GetAddress(), 1);
    cdsmService->cdsmList_.EnsureInsert(testGroupId, cdsmInfo);

    std::shared_ptr<SlePeripheralDevice> memberDev = std::make_shared<SlePeripheralDevice>();
    memberDev->SetAddress(memberAddr);
    memberDev->SetAppearance(oldAppearance);
    std::shared_ptr<SlePeripheralDevice> reportDev = std::make_shared<SlePeripheralDevice>();
    reportDev->SetAddress(reportAddr);
    reportDev->SetAppearance(oldAppearance);
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(memberAddr.GetAddress(), memberDev);
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(reportAddr.GetAddress(), reportDev);
    bool result = SleRemoteDeviceAdapter::GetInstance()->SetAppearance(memberAddr, newAppearance);
    EXPECT_TRUE(result);
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(memberAddr.GetAddress());
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(reportAddr.GetAddress());
    cdsmService->cdsmList_.Erase(testGroupId);
    HILOGI("SleAdapterSecurityTest: SetAppearance001 end");
}

/**
 * @tc.name: Set_and_Get_FrameType001
 * @tc.desc: 验证 Set_and_Get_FrameType
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, Set_and_Get_FrameType001)
{
    HILOGI("SleAdapterSecurityTest: Set_and_Get_FrameType001 start");
    RawAddress device("00:11:22:33:44:55");
    uint8_t frameType = 0x01;
    g_adapter->SetConnFrameType(device.GetAddress(), frameType);
    bool ret = g_adapter->GetConnFrameType(device.GetAddress(), frameType);
    EXPECT_EQ(true, ret);
    g_adapter->DelConnFrameType(device.GetAddress());
    HILOGI("SleAdapterSecurityTest: Set_and_Get_FrameType001 end");
}

/**
 * @tc.name: Register_Deregister_SleAdapterObserver001
 * @tc.desc: 验证 RegisterSleAdapterObserver和DeregisterSleAdapterObserver
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, Register_Deregister_SleAdapterObserver001)
{
    HILOGI("SleAdapterSecurityTest: Register_Deregister_SleAdapterObserver001 start");
    std::unique_ptr<IAdapterSleObserver> sleObserver = std::make_unique<IAdapterSleObserver>();
    bool ret1 = g_adapter->RegisterSleAdapterObserver(*sleObserver);
    EXPECT_EQ(true, ret1);
    bool ret2 = g_adapter->DeregisterSleAdapterObserver(*sleObserver);
    EXPECT_EQ(true, ret2);
    HILOGI("SleAdapterSecurityTest: Register_Deregister_SleAdapterObserver001 end");
}

/**
 * @tc.name: GetConnDirect001
 * @tc.desc: 验证 GetConnDirect
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, GetConnDirect001)
{
    HILOGI("SleAdapterSecurityTest: GetConnDirect001 start");
    RawAddress device("00:11:22:33:44:55");
    std::shared_ptr<SlePeripheralDevice> peerDevice = std::make_shared<SlePeripheralDevice>();
    peerDevice->SetConnDirect(static_cast<int>(SleConnDirect::SLE_CONNECTION_ACTIVE));
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(device.GetAddress(), peerDevice);

    uint8_t ret = SleRemoteDeviceAdapter::GetInstance()->GetConnDirect(device);
    EXPECT_EQ(0x01, ret);
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(device.GetAddress());
    HILOGI("SleAdapterSecurityTest: GetConnDirect001 end");
}

/**
 * @tc.name: SleFreqHopping001
 * @tc.desc: 验证 SleFreqHopping 频率映射表长度不合法
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, SleFreqHopping001)
{
    HILOGI("SleAdapterSecurityTest: SleFreqHopping001 start");
    std::vector<uint8_t> freq = {0x01, 0x02, 0x03};
    uint8_t ret = g_adapter->SleFreqHopping(freq);
    EXPECT_EQ(false, ret);
    HILOGI("SleAdapterSecurityTest: SleFreqHopping001 end");
}

/**
 * @tc.name: Update_and_Clear_SleConnectableTimer001
 * @tc.desc: 验证 UpdateSleConnectableTimer和ClearSleConnectableTimer
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, Update_and_Clear_SleConnectableTimer001)
{
    HILOGI("SleAdapterSecurityTest: Update_and_Clear_SleConnectableTimer001 start");
    g_adapter->pimpl->duration_ = 1;
    g_adapter->UpdateSleConnectableTimer();
    g_adapter->ClearSleConnectableTimer();
    g_adapter->pimpl->duration_ = 0;
    HILOGI("SleAdapterSecurityTest: Update_and_Clear_SleConnectableTimer001 end");
}

/**
 * @tc.name: DisconnectionCompleteTask001
 * @tc.desc: 验证 DisconnectionCompleteTask
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, DisconnectionCompleteTask001)
{
    HILOGI("SleAdapterSecurityTest: DisconnectionCompleteTask001 start");
    RawAddress device("AA:BB:CC:DD:EE:FF");
    uint16_t lcid = 0x01;
    int reason = 0;
    std::shared_ptr<SlePeripheralDevice> peerDevice = std::make_shared<SlePeripheralDevice>();
    peerDevice->SetAddress(device);
    peerDevice->SetPairedStatus(static_cast<int>(SlePairState::SLE_PAIR_PAIRING));
    peerDevice->SetPrePairedStatus(static_cast<int>(SlePairState::SLE_PAIR_PAIRED));
    peerDevice->SetAcbConnectState(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED));
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(device.GetAddress(), peerDevice);
    g_adapter->DisconnectionCompleteTask(lcid, device, reason);
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(device.GetAddress());
    HILOGI("SleAdapterSecurityTest: DisconnectionCompleteTask001 end");
}

/**
 * @tc.name: OnAcbStateChanged001
 * @tc.desc: 验证 OnAcbStateChanged
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, OnAcbStateChanged001)
{
    HILOGI("SleAdapterSecurityTest: OnAcbStateChanged001 start");
    RawAddress device("AA:BB:CC:DD:EE:FF");
    int connectState = 0x01;
    int reason = static_cast<int>(SleDiscReason::SLE_DISC_REASON_CANCEL_PAIR);
    g_adapter->OnAcbStateChanged(device, connectState, reason);
    HILOGI("SleAdapterSecurityTest: OnAcbStateChanged001 end");
}

/**
 * @tc.name: ConnectionStateTask001
 * @tc.desc: 验证 ConnectionStateTask
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, ConnectionStateTask001)
{
    HILOGI("SleAdapterSecurityTest: ConnectionStateTask001 start");
    CM_LogicLinkState_S param1 = {
        .lcid = 0,
        .role = 0,
        .addr = {
            .type = 0x01,
            .addr = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55},
        },
        .result = 0x00,
        .discReason = 1,
        .connCompleteType = 0,
        .advHandle = 0
    };

    CM_LogicLinkState_S param2 = {
        .lcid = 0,
        .role = 0,
        .addr = {
            .type = 0x01,
            .addr = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55},
        },
        .result = 0x02,
        .discReason = 1,
        .connCompleteType = 0,
        .advHandle = 0
    };
    RawAddress device("00:11:22:33:44:55");
    std::shared_ptr<SlePeripheralDevice> peerDevice = std::make_shared<SlePeripheralDevice>();
    peerDevice->SetAcbConnectState(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED));
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(device.GetAddress(), peerDevice);
    g_adapter->ConnectionStateTask(param1);
    g_adapter->ConnectionStateTask(param2);
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(device.GetAddress());
    HILOGI("SleAdapterSecurityTest: ConnectionStateTask001 end");
}

/**
 * @tc.name: PairComplete001
 * @tc.desc: 验证 PairComplete - 成功状态，路由到 PairCmpSuccess
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, PairComplete001)
{
    HILOGI("SleAdapterSecurityTest: PairComplete001 start");
    RawAddress device("AA:BB:CC:DD:EE:FF");
    std::shared_ptr<SlePeripheralDevice> peerDevice = std::make_shared<SlePeripheralDevice>();
    peerDevice->SetAddress(device);
    peerDevice->SetPairedStatus(static_cast<int>(SlePairState::SLE_PAIR_PAIRING));
    peerDevice->SetConnDirect(static_cast<int>(SleConnDirect::SLE_CONNECTION_ACTIVE));
    peerDevice->SetPairDirection(static_cast<int>(SlePairDirect::SLE_PAIR_DEFAULT));
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(device.GetAddress(), peerDevice);
    g_adapter->PairComplete(device, SM_PAIR_OK);
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(device.GetAddress());
    HILOGI("SleAdapterSecurityTest: PairComplete001 end");
}

/**
 * @tc.name: PairComplete002
 * @tc.desc: 验证 PairComplete - 失败状态，路由到 PairCmpFail
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, PairComplete002)
{
    HILOGI("SleAdapterSecurityTest: PairComplete002 start");
    RawAddress device("AA:BB:CC:DD:EE:FF");
    std::shared_ptr<SlePeripheralDevice> peerDevice = std::make_shared<SlePeripheralDevice>();
    peerDevice->SetAddress(device);
    peerDevice->SetPairedStatus(static_cast<int>(SlePairState::SLE_PAIR_PAIRING));
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(device.GetAddress(), peerDevice);
    g_adapter->PairComplete(device, SM_PAIR_ERROR);
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(device.GetAddress());
    HILOGI("SleAdapterSecurityTest: PairComplete002 end");
}

/**
 * @tc.name: adapter_callback001
 * @tc.desc: 验证 SleAdapter中的回调函数
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, adapter_callback001)
{
    HILOGI("SleAdapterSecurityTest: adapter_callback001 start");
    CM_FreqBandSwitchParam param1 = {
        .status = 0,
        .lcid = 0x0102,
        .oldFreqBand = 0,
        .newFreqBand = 1
    };
    g_adapter->FreqBandChanged(&param1);
    CM_ReadRemoteFeatureVersionRsp_S param2 = {
        .lcid = 0x1234,
        .features = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        .version = 0x01,
        .companyId = 0x1234,
        .subversion = 0x5678,
        .addr = {
            .type = 0x01,
            .addr = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55}
        }
    };
    g_adapter->ReadFeatureVersionCallback(&param2);
    HILOGI("SleAdapterSecurityTest: adapter_callback001 end");
}

/**
 * @tc.name: adapter_callback002
 * @tc.desc: 验证 SleAdapter中的回调函数
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, adapter_callback002)
{
    HILOGI("SleAdapterSecurityTest: adapter_callback002 start");
    CM_SetPhyRsp_S param5 = {
        .status = 0x01,
        .lcid = 0x01,
        .txFormat = 0x01,
        .rxFormat = 0x01,
        .txPhy = 0x01,
        .rxPhy = 0x01,
        .txPilotDensity = 0x01,
        .rxPilotDensity = 0x01,
        .gFeedback = 0x01,
        .tFeedback = 0x01
    };
    g_adapter->SetPhyCallback(&param5);

    NLSTK_CfgdbLocalFeatures_S param7 = {
        .status = 0x01,
        .feats = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
    };
    g_adapter->ReadLocalFeatureCallback(&param7);

    CM_AcbSubrateCbParam_S param8 = {
        .addr = {
            .type = 0x01,
            .addr = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55}
        },
        .subrate = 0x01,
        .subrateMax = 0x01,
        .maxLatency = 0x01,
        .continuationNum = 0x01,
        .supervisionTimeout = 0x01
    };
    g_adapter->AcbSubrateChanged(&param8);
    HILOGI("SleAdapterSecurityTest: adapter_callback002 end");
}

/**
 * @tc.name: adapter_callback003
 * @tc.desc: 验证 SleAdapter中的回调函数
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, adapter_callback003)
{
    HILOGI("SleAdapterSecurityTest: adapter_callback003 start");
    DisconChipInfo disconChipInfo = {
        .connHandle = 1234,
        .signalStrength = -60,
        .actualRssiTs = 123456789,
        .rssiIdx = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13},
        .actualRssiValue = {-50, -51, -52, -53, -54, -55, -56, -57, -58, -59, -60, -61, -62, -63}
    };
    nbc_callback_param_t param1 = {
        .data = &disconChipInfo,
        .dataLen = sizeof(DisconChipInfo)
    };
    g_adapter->RssiChangedCallback(&param1);

    PowerLevelInfo powerLevelInfo = {
        .connHandle = 1234,
        .powerLevel = 50
    };
    nbc_callback_param_t param2 = {
        .data = &powerLevelInfo,
        .dataLen = sizeof(PowerLevelInfo)
    };
    g_adapter->PowerLevelChangedCallback(&param2);

    SLE_Addr_S deviceAddr = {
        .addr = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55}
    };
    CM_ConnectParamUpdateRspExt_S extension = {
        .interval = 100,
        .latency = 50,
        .supervisionTimeout = 1000
    };
    CM_ConnectUpdateParamRsp_S param3 = {
        .version = 0,
        .localIndex = 1,
        .result = 0,
        .lcid = 1234,
        .addr = deviceAddr,
        .extension = extension
    };
    g_adapter->ConnectionUpdateCallback(&param3);
    HILOGI("SleAdapterSecurityTest: adapter_callback003 end");
}

/**
 * @tc.name: adapter_callback004
 * @tc.desc: 验证 SleAdapter中的回调函数
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, adapter_callback004)
{
    HILOGI("SleAdapterSecurityTest: adapter_callback004 start");
    CM_ConnectRemoteUpdateParamReq_S param1 = {
        .lcid = 0x01,
        .intervalMin = 0x01,
        .intervalMax = 0x01,
        .maxLatency = 0x01,
        .supervisionTimeout = 0x01
    };
    g_adapter->ConnectionUpdateRequestCallback(&param1);

    CM_AcbSubrateCbParam_S param2 = {
        .addr = {
            .type = 0x01,
            .addr = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55}
        },
        .subrate = 0x01,
        .subrateMax = 0x01,
        .maxLatency = 0x01,
        .continuationNum = 0x01,
        .supervisionTimeout = 0x01
    };
    g_adapter->AcbSubrateChangeReq(&param2);
    HILOGI("SleAdapterSecurityTest: adapter_callback004 end");
}

/**
 * @tc.name: adv_callback001
 * @tc.desc: 验证 广播的回调函数
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, adv_callback001)
{
    HILOGI("SleAdapterSecurityTest: adv_callback001 start");
    RawAddress device("00:11:22:33:44:55");
    g_adapter->pimpl->sleConnectableHandle_.store(
        static_cast<uint8_t>(SleAdvertisingHandle::SLE_LEGACY_ADVERTISING_HANDLE));
    int result = 0;
    uint8_t advHandle = 0;
    g_adapter->pimpl->sleConnectableAdvertiserCallback_->OnStartResultEvent(result, advHandle);
    g_adapter->pimpl->sleConnectableAdvertiserCallback_->OnStartResultEvent(1, advHandle);
    g_adapter->pimpl->sleConnectableHandle_.store(
        static_cast<uint8_t>(SleAdvertisingHandle::SLE_LEGACY_ADVERTISING_HANDLE));
    g_adapter->pimpl->sleConnectableAdvertiserCallback_->OnAutoStopAdvEvent(advHandle);
    g_adapter->pimpl->sleConnectableHandle_.store(
        static_cast<uint8_t>(SleAdvertisingHandle::SLE_LEGACY_ADVERTISING_HANDLE));
    g_adapter->pimpl->sleConnectableAdvertiserCallback_->OnStopResultEvent(result, advHandle);
    g_adapter->pimpl->sleConnectableHandle_.store(
        static_cast<uint8_t>(SleAdvertisingHandle::SLE_LEGACY_ADVERTISING_HANDLE));
    g_adapter->pimpl->sleConnectableAdvertiserCallback_->OnEnableResultEvent(result, advHandle);
    g_adapter->pimpl->sleConnectableHandle_.store(
        static_cast<uint8_t>(SleAdvertisingHandle::SLE_LEGACY_ADVERTISING_HANDLE));
    g_adapter->pimpl->sleConnectableAdvertiserCallback_->OnDisableResultEvent(result, advHandle);
    g_adapter->pimpl->sleConnectableAdvertiserCallback_->OnSetAdvDataEvent(result, advHandle);
    g_adapter->pimpl->sleConnectableHandle_.store(
        static_cast<uint8_t>(SleAdvertisingHandle::SLE_INVALID_ADVERTISING_HANDLE));
    HILOGI("SleAdapterSecurityTest: adv_callback001 end");
}

/**
 * @tc.name: ConnectionCompleteTask001
 * @tc.desc: 验证 ConnectionCompleteTask
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, ConnectionCompleteTask001)
{
    HILOGI("SleAdapterSecurityTest: ConnectionCompleteTask001 start");
    RawAddress device("00:11:22:33:44:55");
    std::shared_ptr<SlePeripheralDevice> peerDevice = std::make_shared<SlePeripheralDevice>();
    peerDevice->SetPairedStatus(0x01);
    peerDevice->SetLcid(0);
    peerDevice->SetRoles(1);
    peerDevice->SetAddressType(0);
    peerDevice->SetAcbConnectState(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED));
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(device.GetAddress(), peerDevice);
    SLE_Addr_S addr = {
        .type = 0x01,
        .addr = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55}
    };
    uint16_t lcid = 0;
    uint8_t role = 0;
    uint8_t connCompleteType = 0;
    g_adapter->ConnectionCompleteTask(addr, lcid, role, connCompleteType);
    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(device.GetAddress());
    HILOGI("SleAdapterSecurityTest: ConnectionCompleteTask001 end");
}

/**
 * @tc.name: ConnectionCompleteTaskInner001
 * @tc.desc: 验证 ConnectionCompleteTaskInner
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, ConnectionCompleteTaskInner001)
{
    HILOGI("SleAdapterSecurityTest: ConnectionCompleteTaskInner001 start");
    RawAddress device("00:11:22:33:44:55");
    SLE_Addr_S addr = {
        .type = 0x01,
        .addr = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55}
    };
    uint16_t lcid = 0;
    uint8_t role = 1;
    g_adapter->ConnectionCompleteTaskInner(static_cast<int>(SlePairState::SLE_PAIR_PAIRED), lcid, device, addr, role);
    g_adapter->ConnectionCompleteTaskInner(static_cast<int>(SlePairState::SLE_PAIR_PAIRING), lcid, device, addr, role);
    HILOGI("SleAdapterSecurityTest: ConnectionCompleteTaskInner001 end");
}

/**
 * @tc.name: SendBgConnList001
 * @tc.desc: 验证 SendBgConnList
 * @tc.type: FUNC
 */
TEST_F(SleAdapterSecurityTest, SendBgConnList001)
{
    HILOGI("SleAdapterSecurityTest: SendBgConnList001 start");
    RawAddress device("00:11:22:33:44:55");
    std::shared_ptr<SlePeripheralDevice> peerDevice = std::make_shared<SlePeripheralDevice>();
    peerDevice->SetPairDirection(static_cast<int>(SlePairDirect::SLE_PAIR_PASSIVE));
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(device.GetAddress(), peerDevice);
    g_adapter->SendBgConnList();

    peerDevice->SetPairDirection(static_cast<int>(SlePairDirect::SLE_PAIR_ACTIVE));
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(device.GetAddress(), peerDevice);
    g_adapter->SendBgConnList();

    SleRemoteDeviceAdapter::GetInstance()->RemovePeripheralDevice(device.GetAddress());
    HILOGI("SleAdapterSecurityTest: SendBgConnList001 end");
}

/**
 * @tc.name: AuthComplete001
 * @tc.desc: SM_ERR_OK，验证保存连接信息和密钥，返回true
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterSecurityTest, AuthComplete001, TestSize.Level1)
{
    HILOGI("SleAdapterSecurityTest: AuthComplete001 start");
    NLSTK_SmAuthComplete_S param = {};
    param.authStatus = SM_ERR_OK;
    uint8_t addr[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    (void)memcpy_s(param.addr.addr, sizeof(param.addr.addr), addr, sizeof(addr));
    sleSecurity_.AuthComplete(&param);
    EXPECT_EQ(true, sleSecurity_.SmpAuthComplete(param));
    HILOGI("SleAdapterSecurityTest: AuthComplete001 end");
}

/**
 * @tc.name: AuthComplete002
 * @tc.desc: SM_ERR_ACTIVE_CANCEL，验证取消配对并返回false
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterSecurityTest, AuthComplete002, TestSize.Level1)
{
    HILOGI("SleAdapterSecurityTest: AuthComplete002 start");
    NLSTK_SmAuthComplete_S param = {};
    param.authStatus = SM_ERR_ACTIVE_CANCEL;
    uint8_t addr[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    (void)memcpy_s(param.addr.addr, sizeof(param.addr.addr), addr, sizeof(addr));
    param.addr.type = 0x0;
    EXPECT_EQ(false, sleSecurity_.SmpAuthComplete(param));
    HILOGI("SleAdapterSecurityTest: AuthComplete002 end");
}

/**
 * @tc.name: AuthComplete003
 * @tc.desc: 其他错误码，调用 LePairComplete 并返回true
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterSecurityTest, AuthComplete003, TestSize.Level1)
{
    HILOGI("SleAdapterSecurityTest: AuthComplete003 start");
    NLSTK_SmAuthComplete_S param = {};
    param.authStatus = SM_PAIR_OK;
    uint8_t addr[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    (void)memcpy_s(param.addr.addr, sizeof(param.addr.addr), addr, sizeof(addr));
    EXPECT_EQ(true, sleSecurity_.SmpAuthComplete(param));
    param.authStatus = SM_PAIR_ERROR;
    EXPECT_EQ(true, sleSecurity_.SmpAuthComplete(param));
    HILOGI("SleAdapterSecurityTest: AuthComplete003 end");
}

/**
 * @tc.name: PairStartChanged001
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterSecurityTest, PairStartChanged001, TestSize.Level1)
{
    HILOGI("SleAdapterSecurityTest: PairStartChanged001 start");
    NLSTK_SmPairingStart_S param = {};
    uint8_t addr[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    (void)memcpy_s(param.addr.addr, sizeof(param.addr.addr), addr, sizeof(addr));
    param.bondStatus = 0x00;
    sleSecurity_.PairStartChanged(&param);
    EXPECT_EQ(true, sleSecurity_.PairStartStatusChange(param));
    HILOGI("SleAdapterSecurityTest: PairStartChanged001 end");
}

/**
 * @tc.name: EncryptionComplete001
 * @tc.desc: 测试不同加密状态
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterSecurityTest, EncryptionComplete001, TestSize.Level1)
{
    HILOGI("SleAdapterSecurityTest: EncryptionComplete001 start");
    NLSTK_SmEncComplete_S param = {};
    uint8_t addr[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    (void)memcpy_s(param.addr.addr, sizeof(param.addr.addr), addr, sizeof(addr));

    param.encStatus = SM_PAIR_OK;
    sleSecurity_.EncryptionComplete(&param);

    param.encStatus = SM_PAIR_ERROR;
    EXPECT_EQ(true, sleSecurity_.SmpEncComplete(param));

    param.encStatus = SM_KEY_MISSING;
    EXPECT_EQ(true, sleSecurity_.SmpEncComplete(param));

    param.encStatus = SM_LINK_DISCONNCTED;
    EXPECT_EQ(true, sleSecurity_.SmpEncComplete(param));
    HILOGI("SleAdapterSecurityTest: EncryptionComplete001 end");
}

/**
 * @tc.name: PairCancelComplete001
 * @tc.desc: 测试不同配对取消状态
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterSecurityTest, PairCancelComplete001, TestSize.Level1)
{
    HILOGI("SleAdapterSecurityTest: PairCancelComplete001 start");
    NLSTK_SmPairingRemove_S param = {};
    uint8_t addr[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    (void)memcpy_s(param.addr.addr, sizeof(param.addr.addr), addr, sizeof(addr));
    param.removeStatus = NLSTK_ERRCODE_SUCCESS;
    EXPECT_EQ(true, sleSecurity_.SmpPairCancelComplete(param));
    param.removeStatus = NLSTK_ERRCODE_FAIL;
    sleSecurity_.PairCancelComplete(&param);
    HILOGI("SleAdapterSecurityTest: PairCancelComplete001 end");
}

/**
 * @tc.name: PairingReq001
 * @tc.desc: 测试不同配对取消状态
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterSecurityTest, PairingReq001, TestSize.Level1)
{
    HILOGI("SleAdapterSecurityTest: PairingReq001 start");
    NLSTK_SmPairingRequest_S param = {};
    uint8_t addr[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    (void)memcpy_s(param.addr.addr, sizeof(param.addr.addr), addr, sizeof(addr));
    (void)memcpy_s(param.sixDigits, sizeof(param.sixDigits), "123456", 7);
    param.requestType = 0x03; // AUTH_PASSWORD_ENTRY
    sleSecurity_.PairingReq(&param);
    param.requestType = 0x02; // AUTH_PASSCODE
    EXPECT_EQ(true, sleSecurity_.SmpPairingReq(param));
    HILOGI("SleAdapterSecurityTest: PairingReq001 end");
}

/**
 * @tc.name: SaveSlePairKey001
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(SleAdapterSecurityTest, SaveSlePairKey001, TestSize.Level1)
{
    HILOGI("SleAdapterSecurityTest: SaveSlePairKey001 start");
    RawAddress device("00:11:22:33:44:55");
    NLSTK_SmAuthComplete_S param = {};
    param.authStatus = SM_PAIR_OK;
    uint8_t addr[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    (void)memcpy_s(param.addr.addr, sizeof(param.addr.addr), addr, sizeof(addr));
    uint8_t key[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};
    (void)memcpy_s(param.linkKey, sizeof(param.linkKey), key, sizeof(key));
    EXPECT_EQ(false, sleSecurity_.SaveSlePairKey(device, param));
    HILOGI("SleAdapterSecurityTest: SaveSlePairKey001 end");
}
} // namespace TEST
} // namespace Nearlink
} // namespace OHOS
