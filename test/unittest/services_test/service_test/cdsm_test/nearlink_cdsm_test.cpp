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
#include <gmock/gmock.h>
#include <thread>
#include "nearlink_asc_audio_stream_info.h"
#include "SleInterfaceManager.h"
#include "SleServiceManager.h"
#include "TwsService.h"
#include "TwsDefines.h"
#include "CdsmService.h"
#include "SleUtils.h"
#include "log.h"
#include "SleConfig.h"


namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing;
using namespace testing::ext;

constexpr int DELAY_LITTLE_MS = 100;

class CdsmCallbackTest : public CdsmObserver {
public:
    ~CdsmCallbackTest() = default;
    void OnConnectionStateChanged(const RawAddress &deviceAddress, int state, int oldState) override
    {
        HILOGI("enter");
    }
};

class MockCdsmService : public CdsmService {
public:
    void MockClearOldDevice()
    {
        CdsmCallBackData dataBlock;
        dataBlock.event = CDSM_EVENT_CONNECT;
        dataBlock.cdmsGroupId = 1;
        dataBlock.memberNum = 2;
        RawAddress addr1("11:22:33:44:55:66");
        RawAddress addr2("11:22:33:44:55:77");
        dataBlock.currentAddr = addr1;
        CdsmCallBackDataMemInfo memInfo;
        memInfo.memberAddr = addr1;
        memInfo.memberState = 1;
        dataBlock.memberInfo.push_back(memInfo);
        memInfo.memberAddr = addr2;
        memInfo.memberState = 1;
        dataBlock.memberInfo.push_back(memInfo);

        std::shared_ptr<CdsmInfo> cdsmDataPtr = std::make_shared<CdsmInfo>(1, "11:22:33:44:55:66");
        cdsmDataPtr->CdsmAddMemberInfo("11:22:33:44:55:66", 1);
        cdsmDataPtr->CdsmAddMemberInfo("11:22:33:44:55:88", 1);

        RawAddress oldDevAddr("11:22:33:44:55:66");
        bool isNeedClear = ClearOldDevice(dataBlock, cdsmDataPtr, oldDevAddr);
    }

    void MockUpdateCdsmData()
    {
        CdsmCallBackData dataBlock;
        dataBlock.event = CDSM_EVENT_CONNECT;
        dataBlock.cdmsGroupId = 1;
        dataBlock.memberNum = 2;
        RawAddress addr1("11:22:33:44:55:66");
        RawAddress addr2("11:22:33:44:55:77");
        dataBlock.currentAddr = addr1;
        CdsmCallBackDataMemInfo memInfo;
        memInfo.memberAddr = addr1;
        memInfo.memberState = 1;
        dataBlock.memberInfo.push_back(memInfo);
        memInfo.memberAddr = addr2;
        memInfo.memberState = 1;
        dataBlock.memberInfo.push_back(memInfo);

        std::shared_ptr<CdsmInfo> cdsmDataPtr = std::make_shared<CdsmInfo>(1, "11:22:33:44:55:66");\
        cdsmDataPtr->CdsmAddMemberInfo("11:22:33:44:55:66", 1);
        cdsmDataPtr->CdsmAddMemberInfo("11:22:33:44:55:88", 1);

        cdsmList_.EnsureInsert(dataBlock.cdmsGroupId, cdsmDataPtr);

        UpdateCdsmData(dataBlock);
    }

    void MockCdsmDeleteMember()
    {
        CdsmDeleteMember(1, "11:22:33:44:55:66");
    }
};

class MockCdsmClient : public CdsmClient {
public:
    MockCdsmClient(const std::string &address) : CdsmClient(address) {}
    void SetState(CdsmClientState newState) { cdsmClientState_ = newState;}
};

class NearlinkCdsmTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void NearlinkCdsmTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase start NearlinkCdsmTest");
    SleInterfaceManager::GetInstance()->Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
}

void NearlinkCdsmTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase NearlinkCdsmTest");
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
}

void NearlinkCdsmTest::SetUp()
{
    HILOGI("SetUp NearlinkCdsmTest.");
}

void NearlinkCdsmTest::TearDown()
{
    HILOGI("TearDown NearlinkCdsmTest.");
}

/**
 * @tc.name: Connect
 * @tc.desc: Test the Connect function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, Connect001, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:Connect001 start");
    RawAddress device("11:22:33:44:55:66");
    CdsmService *cdsmService = CdsmService::GetService();
    int ret = cdsmService->Connect(device);
    EXPECT_EQ(ret, NL_NO_ERROR);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    HILOGI("NearlinkCdsmTest:Connect001 end");
}

/**
 * @tc.name: Disconnect
 * @tc.desc: Test the Disconnect function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, Disconnect001, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:Disconnect001 start");
    RawAddress device("11:22:33:44:55:66");
    CdsmService *cdsmService = CdsmService::GetService();
    int ret = cdsmService->Disconnect(device);
    EXPECT_EQ(ret, NL_NO_ERROR);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    HILOGI("NearlinkCdsmTest:Disconnect001 end");
}

/**
 * @tc.name: Enable
 * @tc.desc: Test the Enable function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, Enable001, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:Enable001 start");

    CdsmService *cdsmService = CdsmService::GetService();
    cdsmService->Enable();

    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    HILOGI("NearlinkCdsmTest:Enable001 end");
}

/**
 * @tc.name: Disable
 * @tc.desc: Test the Disable function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, Disable001, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:Disable001 start");

    CdsmService *cdsmService = CdsmService::GetService();
    cdsmService->Disable();

    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    HILOGI("NearlinkCdsmTest:Disable001 end");
}

/**
 * @tc.name: RegisterObserver
 * @tc.desc: Test the RegisterObserver function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, RegisterObserver001, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:RegisterObserver001 start");
    CdsmCallbackTest serviceObserver;
    CdsmService *cdsmService = CdsmService::GetService();
    cdsmService->RegisterObserver(serviceObserver);
    cdsmService->DeregisterObserver(serviceObserver);

    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    HILOGI("NearlinkCdsmTest:RegisterObserver001 end");
}

/**
 * @tc.name: GetConnectState
 * @tc.desc: Test the GetConnectState function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, GetConnectState001, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:GetConnectState001 start");
    CdsmService *cdsmService = CdsmService::GetService();
    int ret = cdsmService->GetConnectState();

    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    HILOGI("NearlinkCdsmTest:GetConnectState001 end");
}

/**
 * @tc.name: GetConnectDevices
 * @tc.desc: Test the GetConnectDevices function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, GetConnectDevices001, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:GetConnectDevices001 start");
    CdsmService *cdsmService = CdsmService::GetService();
    std::list<RawAddress> ret = cdsmService->GetConnectDevices();

    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    HILOGI("NearlinkCdsmTest:GetConnectDevices001 end");
}

/**
 * @tc.name: NotifyStateChanged
 * @tc.desc: Test the NotifyStateChanged function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, NotifyStateChanged001, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:NotifyStateChanged001 start");
    RawAddress device("FF:FF:FF:FF:FF:FF");
    CdsmService *cdsmService = CdsmService::GetService();
    cdsmService->NotifyStateChanged(device, CdsmClientState::CDSM_STATE_CONNECTED,
        CdsmClientState::CDSM_STATE_DISCONNECTED);

    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    HILOGI("NearlinkCdsmTest:NotifyStateChanged001 end");
}

/**
 * @tc.name: NotifyStateChanged
 * @tc.desc: Test the NotifyStateChanged function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, NotifyStateChanged002, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:NotifyStateChanged002 start");
    RawAddress device("FF:FF:FF:FF:FF:FF");
    CdsmService *cdsmService = CdsmService::GetService();
    cdsmService->NotifyStateChanged(device, CdsmClientState::CDSM_STATE_CONNECTING,
        CdsmClientState::CDSM_STATE_CONNECTED);

    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    HILOGI("NearlinkCdsmTest:NotifyStateChanged002 end");
}

/**
 * @tc.name: NotifyStateChanged
 * @tc.desc: Test the NotifyStateChanged function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, NotifyStateChanged003, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:NotifyStateChanged003 start");
    RawAddress device("FF:FF:FF:FF:FF:FF");
    CdsmService *cdsmService = CdsmService::GetService();
    cdsmService->NotifyStateChanged(device, CdsmClientState::CDSM_STATE_CONNECTING,
        CdsmClientState::CDSM_STATE_DISCONNECTED);

    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    HILOGI("NearlinkCdsmTest:NotifyStateChanged003 end");
}

/**
 * @tc.name: CdsmStackProfileCallback
 * @tc.desc: Test the CdsmStackProfileCallback function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, CdsmStackProfileCallback001, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:CdsmStackProfileCallback001 start");
    SLE_Addr_S addr = {1, {11,22,33,44,55,66}};
    NLSTK_CdsmEvent_S event{};
    event.addr = addr;
    event.gid = 1;
    event.type = CDSM_EVENT_CONNECT;
    event.num = 1;
    event.memInfo = new (std::nothrow) NLSTK_CdsmMemInfo_S();
    event.memInfo[0].addr = addr;
    event.memInfo[0].state = CDSM_PROFILE_CONNECT;

    CdsmService *cdsmService = CdsmService::GetService();
    cdsmService->CdsmStackProfileCallback(&event);

    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    delete event.memInfo;
    HILOGI("NearlinkCdsmTest:CdsmStackProfileCallback001 end");
}

/**
 * @tc.name: ProcessEvent
 * @tc.desc: Test the ProcessEvent function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, ProcessEvent001, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:ProcessEvent001 start");
    CdsmMessage event(CDSM_SERVICE_DISCONNECT_START_EVT);
    event.cdsmGrpId_ = 1;
    event.dev_ = "11:22:33:44:55:66";

    CdsmService *cdsmService = CdsmService::GetService();
    cdsmService->ProcessEvent(event);

    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    HILOGI("NearlinkCdsmTest:ProcessEvent001 end");
}

/**
 * @tc.name: CdsmCheckIsPrivateCooperationDevice
 * @tc.desc: Test the CdsmCheckIsPrivateCooperationDevice function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, CdsmCheckIsPrivateCooperationDevice001, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:CdsmCheckIsPrivateCooperationDevice001 start");
    RawAddress devAddr("11:22:33:44:55:66");

    CdsmService *cdsmService = CdsmService::GetService();
    bool ret = cdsmService->CdsmCheckIsPrivateCooperationDevice(devAddr);
    EXPECT_EQ(ret, false);

    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    HILOGI("NearlinkCdsmTest:CdsmCheckIsPrivateCooperationDevice001 end");
}

/**
 * @tc.name: CdsmCheckIsCooperationMember
 * @tc.desc: Test the CdsmCheckIsCooperationMember function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, CdsmCheckIsCooperationMember001, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:CdsmCheckIsCooperationMember001 start");
    RawAddress devAddr("11:22:33:44:55:66");

    CdsmService *cdsmService = CdsmService::GetService();
    bool ret = cdsmService->CdsmCheckIsCooperationMember(devAddr);
    EXPECT_EQ(ret, false);

    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    HILOGI("NearlinkCdsmTest:CdsmCheckIsCooperationMember001 end");
}

/**
 * @tc.name: CdsmCheckIsCooperationReport
 * @tc.desc: Test the CdsmCheckIsCooperationReport function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, CdsmCheckIsCooperationReport001, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:CdsmCheckIsCooperationReport001 start");
    RawAddress devAddr("11:22:33:44:55:66");

    CdsmService *cdsmService = CdsmService::GetService();
    bool ret = cdsmService->CdsmCheckIsCooperationReport(devAddr);
    EXPECT_EQ(ret, false);

    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    HILOGI("NearlinkCdsmTest:CdsmCheckIsCooperationReport001 end");
}

/**
 * @tc.name: CdsmCheckIsSameCooperation
 * @tc.desc: Test the CdsmCheckIsSameCooperation function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, CdsmCheckIsSameCooperation001, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:CdsmCheckIsSameCooperation001 start");
    RawAddress dev1("11:22:33:44:55:66");

    CdsmService *cdsmService = CdsmService::GetService();
    bool ret = cdsmService->CdsmCheckIsCooperationReport(dev1);
    EXPECT_EQ(ret, false);

    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    HILOGI("NearlinkCdsmTest:CdsmCheckIsSameCooperation001 end");
}

/**
 * @tc.name: CdsmGetReportAddr
 * @tc.desc: Test the CdsmGetReportAddr function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, CdsmGetReportAddr001, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:CdsmGetReportAddr001 start");
    RawAddress dev1("11:22:33:44:55:66");
    RawAddress reportAddr;

    CdsmService *cdsmService = CdsmService::GetService();
    bool ret = cdsmService->CdsmGetReportAddr(dev1, reportAddr);
    EXPECT_EQ(ret, false);

    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    HILOGI("NearlinkCdsmTest:CdsmGetReportAddr001 end");
}

/**
 * @tc.name: CdsmDeleteGroup
 * @tc.desc: Test the CdsmDeleteGroup function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, CdsmDeleteGroup001, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:CdsmDeleteGroup001 start");
    RawAddress dev1("11:22:33:44:55:66");

    CdsmService *cdsmService = CdsmService::GetService();
    cdsmService->CdsmDeleteGroup(dev1);

    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    HILOGI("NearlinkCdsmTest:CdsmDeleteGroup001 end");
}

/**
 * @tc.name: CdsmStopInviteAdv
 * @tc.desc: Test the CdsmStopInviteAdv function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, CdsmStopInviteAdv001, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:CdsmStopInviteAdv001 start");
    RawAddress dev1("11:22:33:44:55:66");

    CdsmService *cdsmService = CdsmService::GetService();
    cdsmService->CdsmStopInviteAdv(dev1, true);

    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    HILOGI("NearlinkCdsmTest:CdsmStopInviteAdv001 end");
}

/**
 * @tc.name: CdsmCreateGroup
 * @tc.desc: Test the CdsmCreateGroup function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, CdsmCreateGroup001, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:CdsmCreateGroup001 start");
    RawAddress dev1("11:22:33:44:55:66");
    RawAddress dev2("11:22:33:44:55:77");
    std::vector<RawAddress> devAddrList = {dev1, dev2};

    CdsmService *cdsmService = CdsmService::GetService();
    cdsmService->CdsmCreateGroup(dev1, devAddrList, true);

    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    HILOGI("NearlinkCdsmTest:CdsmCreateGroup001 end");
}

/**
 * @tc.name: CdsmGetOtherAddr
 * @tc.desc: Test the CdsmGetOtherAddr function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, CdsmGetOtherAddr001, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:CdsmGetOtherAddr001 start");
    RawAddress dev1("11:22:33:44:55:66");
    RawAddress other;

    CdsmService *cdsmService = CdsmService::GetService();
    bool ret = cdsmService->CdsmGetOtherAddr(dev1, other);
    EXPECT_EQ(ret, true);

    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    HILOGI("NearlinkCdsmTest:CdsmGetOtherAddr001 end");
}

/**
 * @tc.name: CdsmGetOtherAddr
 * @tc.desc: Test the CdsmGetOtherAddr function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, CdsmGetOtherAddr002, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:CdsmGetOtherAddr002 start");
    RawAddress dev1("11:22:33:44:00:00");
    RawAddress other;

    CdsmService *cdsmService = CdsmService::GetService();
    bool ret = cdsmService->CdsmGetOtherAddr(dev1, other);
    EXPECT_EQ(ret, false);

    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    HILOGI("NearlinkCdsmTest:CdsmGetOtherAddr002 end");
}

/**
 * @tc.name: CdsmRecoverFromConf
 * @tc.desc: Test the CdsmRecoverFromConf function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, CdsmRecoverFromConf001, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:CdsmRecoverFromConf001 start");

    CdsmService *cdsmService = CdsmService::GetService();
    cdsmService->CdsmRecoverFromConf();

    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    HILOGI("NearlinkCdsmTest:CdsmRecoverFromConf001 end");
}

/**
 * @tc.name: CdsmGetGroupId
 * @tc.desc: Test the CdsmGetGroupId function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, CdsmGetGroupId001, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:CdsmGetGroupId001 start");
    uint32_t groupId = 0;
    CdsmService *cdsmService = CdsmService::GetService();
    bool ret = cdsmService->CdsmGetGroupId(groupId, "11:22:33:44:55:66");
    EXPECT_EQ(ret, true);

    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    HILOGI("NearlinkCdsmTest:CdsmGetGroupId001 end");
}

/**
 * @tc.name: CdsmGetGroupId
 * @tc.desc: Test the CdsmGetGroupId function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, CdsmGetGroupId002, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:CdsmGetGroupId002 start");
    uint32_t groupId = 0;
    CdsmService *cdsmService = CdsmService::GetService();
    bool ret = cdsmService->CdsmGetGroupId(groupId, "11:22:33:44:00:00");
    EXPECT_EQ(ret, false);

    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    HILOGI("NearlinkCdsmTest:CdsmGetGroupId002 end");
}

/**
 * @tc.name: HandleCdsmClientCallback
 * @tc.desc: Test the HandleCdsmClientCallback function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, HandleCdsmClientCallback001, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:HandleCdsmClientCallback001 start");
    RawAddress dev1("11:22:33:44:55:66");
    CdsmService *cdsmService = CdsmService::GetService();
    cdsmService->HandleCdsmClientCallback(dev1);

    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    HILOGI("NearlinkCdsmTest:HandleCdsmClientCallback001 end");
}

/**
 * @tc.name: UpdateCdsmMemberProfileConnectState
 * @tc.desc: Test the UpdateCdsmMemberProfileConnectState function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, UpdateCdsmMemberProfileConnectState001, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:UpdateCdsmMemberProfileConnectState001 start");
    RawAddress dev1("11:22:33:44:55:66");
    CdsmService *cdsmService = CdsmService::GetService();
    cdsmService->UpdateCdsmMemberProfileConnectState(dev1, SleConnectState::CONNECTED);

    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    HILOGI("NearlinkCdsmTest:UpdateCdsmMemberProfileConnectState001 end");
}

/**
 * @tc.name: UpdateReportConnectState
 * @tc.desc: Test the UpdateReportConnectState function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, UpdateReportConnectState001, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:UpdateReportConnectState001 start");
    RawAddress dev1("11:22:33:44:55:66");
    CdsmService *cdsmService = CdsmService::GetService();
    cdsmService->UpdateReportConnectState(dev1, static_cast<int>(SleConnectState::CONNECTED));

    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    HILOGI("NearlinkCdsmTest:UpdateReportConnectState001 end");
}

/**
 * @tc.name: CdsmGetReportedState
 * @tc.desc: Test the CdsmGetReportedState function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, CdsmGetReportedState001, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:CdsmGetReportedState001 start");
    RawAddress dev1("11:22:33:44:55:66");
    int reportConnectState = 0;
    CdsmService *cdsmService = CdsmService::GetService();
    cdsmService->CdsmGetReportedState(dev1, reportConnectState);

    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    HILOGI("NearlinkCdsmTest:CdsmGetReportedState001 end");
}

/**
 * @tc.name: DeregisterCdsmClientCallback
 * @tc.desc: Test the DeregisterCdsmClientCallback function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, DeregisterCdsmClientCallback001, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:DeregisterCdsmClientCallback001 start");

    CdsmService *cdsmService = CdsmService::GetService();
    cdsmService->DeregisterCdsmClientCallback();

    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    HILOGI("NearlinkCdsmTest:DeregisterCdsmClientCallback001 end");
}

/**
 * @tc.name: ClearOldDevice
 * @tc.desc: Test the ClearOldDevice function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, ClearOldDevice001, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:ClearOldDevice001 start");

    MockCdsmService cdsmService;
    cdsmService.MockClearOldDevice();

    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    HILOGI("NearlinkCdsmTest:ClearOldDevice001 end");
}

/**
 * @tc.name: UpdateCdsmData
 * @tc.desc: Test the UpdateCdsmData function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, UpdateCdsmData001, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:UpdateCdsmData001 start");

    MockCdsmService cdsmService;
    cdsmService.MockUpdateCdsmData();

    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    HILOGI("NearlinkCdsmTest:UpdateCdsmData001 end");
}

/**
 * @tc.name: CdsmDeleteMember
 * @tc.desc: Test the CdsmDeleteMember function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, CdsmDeleteMember001, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:CdsmDeleteMember001 start");

    MockCdsmService cdsmService;
    cdsmService.MockCdsmDeleteMember();

    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    HILOGI("NearlinkCdsmTest:CdsmDeleteMember001 end");
}

/**
 * @tc.name: CdsmClientCdsmStopInviteAdv
 * @tc.desc: Test the CdsmClientCdsmStopInviteAdv function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, CdsmClientCdsmStopInviteAdv001, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:CdsmClientCdsmStopInviteAdv001 start");

    MockCdsmClient cdsmClient("11:22:33:44:55:66");
    cdsmClient.CdsmStopInviteAdv();

    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    HILOGI("NearlinkCdsmTest:CdsmClientCdsmStopInviteAdv001 end");
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS
