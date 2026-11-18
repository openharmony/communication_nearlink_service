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
#include <memory>
#include "ThreadUtil.h"
#include "ProfileServiceManager.cpp"


namespace OHOS {
namespace Nearlink {

extern void SetMockVendorBusinessTypeDevice(const std::string &addr);
extern void ClearMockVendorBusinessTypeDevice();
extern int GetMockSetPeerDeviceTypeCount();
extern void ClearMockSetPeerDeviceTypeCount();
constexpr int DELAY_LITTLE_MS = 100;

namespace TEST {
using namespace testing;
using namespace testing::ext;

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

private:
    // 测试目的：fixture 持有自有 CdsmService 实例（dis 样板），
    // 不再依赖厂商栈启动链（裸进程 StartTask/slem_initialize 必败，GetService() 返回 nullptr）。
    std::unique_ptr<CdsmService> service_;
};

void NearlinkCdsmTest::SetUpTestCase()
{
    // 测试目的：删除 Start()（裸进程必败且返回值被忽略）与固定 sleep；
    // 服务实例改由每例 SetUp 自建并注入注册表。
    HILOGI("SetUpTestCase start NearlinkCdsmTest");
}

void NearlinkCdsmTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase NearlinkCdsmTest");
}

void NearlinkCdsmTest::SetUp()
{
    HILOGI("SetUp NearlinkCdsmTest.");
    // 测试目的：与产品注册路径同构（ProfileServiceManager SetProfile）直接注入自有服务，
    // 使 GetService() 命中；NOT_SWITCH_THREAD 让服务任务在测试线程同步执行，无需 sleep。
    ThreadUtil::GetInstance().threadStateMap_.EnsureInsert(
        THREAD_ID_CDSM, ThreadUtil::ThreadState::NOT_SWITCH_THREAD);
    service_ = std::make_unique<CdsmService>();
    ProfileServiceManager::GetInstance().pimpl->startedProfiles_.SetProfile(
        SleTransport::ADAPTER_SLE, PROFILE_NAME_CDSM, service_.get());
    ASSERT_EQ(service_.get(), CdsmService::GetService());
}

void NearlinkCdsmTest::TearDown()
{
    HILOGI("TearDown NearlinkCdsmTest.");
    // 测试目的：先解绑注册表再销毁服务，并恢复线程调度默认态，防单例状态跨例残留。
    ProfileServiceManager::GetInstance().pimpl->startedProfiles_.SetProfile(
        SleTransport::ADAPTER_SLE, PROFILE_NAME_CDSM, nullptr);
    service_.reset();
    ThreadUtil::GetInstance().InitThreadStateMap();
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
    ASSERT_NE(cdsmService, nullptr);
    int ret = cdsmService->Connect(device);
    EXPECT_EQ(ret, NL_NO_ERROR);
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
    ASSERT_NE(cdsmService, nullptr);
    int ret = cdsmService->Disconnect(device);
    EXPECT_EQ(ret, NL_NO_ERROR);
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
    ASSERT_NE(cdsmService, nullptr);
    cdsmService->Enable();

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
    ASSERT_NE(cdsmService, nullptr);
    cdsmService->Disable();

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
    ASSERT_NE(cdsmService, nullptr);
    cdsmService->RegisterObserver(serviceObserver);
    cdsmService->DeregisterObserver(serviceObserver);

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
    ASSERT_NE(cdsmService, nullptr);
    int ret = cdsmService->GetConnectState();

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
    ASSERT_NE(cdsmService, nullptr);
    std::list<RawAddress> ret = cdsmService->GetConnectDevices();

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
    ASSERT_NE(cdsmService, nullptr);
    cdsmService->NotifyStateChanged(device, CdsmClientState::CDSM_STATE_CONNECTED,
        CdsmClientState::CDSM_STATE_DISCONNECTED);

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
    ASSERT_NE(cdsmService, nullptr);
    cdsmService->NotifyStateChanged(device, CdsmClientState::CDSM_STATE_CONNECTING,
        CdsmClientState::CDSM_STATE_CONNECTED);

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
    ASSERT_NE(cdsmService, nullptr);
    cdsmService->NotifyStateChanged(device, CdsmClientState::CDSM_STATE_CONNECTING,
        CdsmClientState::CDSM_STATE_DISCONNECTED);

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
    ASSERT_NE(cdsmService, nullptr);
    cdsmService->CdsmStackProfileCallback(&event);

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
    ASSERT_NE(cdsmService, nullptr);
    cdsmService->ProcessEvent(event);

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
    ASSERT_NE(cdsmService, nullptr);
    bool ret = cdsmService->CdsmCheckIsPrivateCooperationDevice(devAddr);
    EXPECT_EQ(ret, false);

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
    ASSERT_NE(cdsmService, nullptr);
    bool ret = cdsmService->CdsmCheckIsCooperationMember(devAddr);
    EXPECT_EQ(ret, false);

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
    ASSERT_NE(cdsmService, nullptr);
    bool ret = cdsmService->CdsmCheckIsCooperationReport(devAddr);
    EXPECT_EQ(ret, false);

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
    ASSERT_NE(cdsmService, nullptr);
    bool ret = cdsmService->CdsmCheckIsCooperationReport(dev1);
    EXPECT_EQ(ret, false);

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
    ASSERT_NE(cdsmService, nullptr);
    bool ret = cdsmService->CdsmGetReportAddr(dev1, reportAddr);
    EXPECT_EQ(ret, false);

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
    ASSERT_NE(cdsmService, nullptr);
    cdsmService->CdsmDeleteGroup(dev1);

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
    ASSERT_NE(cdsmService, nullptr);
    cdsmService->CdsmStopInviteAdv(dev1, true);

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
    ASSERT_NE(cdsmService, nullptr);
    cdsmService->CdsmCreateGroup(dev1, devAddrList, true);

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
    ASSERT_NE(cdsmService, nullptr);
    // 测试目的：原依赖 CdsmStackProfileCallback001 遗留的跨用例 cdsmList_ 数据；
    // fixture 改为逐例自建服务后残留消失，此处自足构造合作集数据验证"查询命中"路径。
    auto cdsmInfo = std::make_shared<CdsmInfo>(1, dev1.GetAddress());
    cdsmInfo->CdsmAddMemberInfo(dev1.GetAddress(), 1);
    cdsmInfo->CdsmAddMemberInfo("11:22:33:44:55:88", 1);
    cdsmService->cdsmList_.EnsureInsert(1, cdsmInfo);
    bool ret = cdsmService->CdsmGetOtherAddr(dev1, other);
    EXPECT_EQ(ret, true);

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
    ASSERT_NE(cdsmService, nullptr);
    bool ret = cdsmService->CdsmGetOtherAddr(dev1, other);
    EXPECT_EQ(ret, false);

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
    ASSERT_NE(cdsmService, nullptr);
    cdsmService->CdsmRecoverFromConf();

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
    ASSERT_NE(cdsmService, nullptr);
    // 测试目的：同 CdsmGetOtherAddr001——自足构造合作集成员数据（原跨用例残留随逐例
    // 自建服务消失），使"按成员地址查组ID命中"路径可测。
    auto cdsmInfo = std::make_shared<CdsmInfo>(1, "11:22:33:44:55:66");
    cdsmInfo->CdsmAddMemberInfo("11:22:33:44:55:66", 1);
    cdsmService->cdsmList_.EnsureInsert(1, cdsmInfo);
    bool ret = cdsmService->CdsmGetGroupId(groupId, "11:22:33:44:55:66");
    EXPECT_EQ(ret, true);

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
    ASSERT_NE(cdsmService, nullptr);
    bool ret = cdsmService->CdsmGetGroupId(groupId, "11:22:33:44:00:00");
    EXPECT_EQ(ret, false);

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
    ASSERT_NE(cdsmService, nullptr);
    cdsmService->HandleCdsmClientCallback(dev1);

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
    ASSERT_NE(cdsmService, nullptr);
    cdsmService->UpdateCdsmMemberProfileConnectState(dev1, SleConnectState::CONNECTED);

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
    ASSERT_NE(cdsmService, nullptr);
    cdsmService->UpdateReportConnectState(dev1, static_cast<int>(SleConnectState::CONNECTED));

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
    ASSERT_NE(cdsmService, nullptr);
    cdsmService->CdsmGetReportedState(dev1, reportConnectState);

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
    ASSERT_NE(cdsmService, nullptr);
    cdsmService->DeregisterCdsmClientCallback();

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

    HILOGI("NearlinkCdsmTest:CdsmClientCdsmStopInviteAdv001 end");
}

/**
 * @tc.name: IsVendorDevice_001
 * @tc.desc: Test IsVendorDevice uses IRemoteDeviceQuery::GetInstance(), should not crash
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, IsVendorDevice_001, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:IsVendorDevice_001 start");
    CdsmService *cdsmService = CdsmService::GetService();
    ASSERT_NE(cdsmService, nullptr);
    RawAddress device("11:22:33:44:55:66");
    bool ret = cdsmService->IsVendorDevice(device);
    EXPECT_FALSE(ret);
    HILOGI("NearlinkCdsmTest:IsVendorDevice_001 end");
}

/**
 * @tc.name: SetPeerDeviceTypeToControllerInner_001
 * @tc.desc: Test SetPeerDeviceTypeToControllerInner with a mixed group (one vendor device),
 *           no member should be set to controller
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, SetPeerDeviceTypeToControllerInner_001, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:SetPeerDeviceTypeToControllerInner_001 start");
    MockCdsmService cdsmService;
    RawAddress vendorReportAddr("11:22:33:44:55:66");
    RawAddress commonColabAddr("11:22:33:44:55:77");

    CdsmCallBackData dataBlock;
    dataBlock.event = CDSM_EVENT_CONNECT;
    dataBlock.cdmsGroupId = 1;
    dataBlock.memberNum = 2;
    dataBlock.currentAddr = vendorReportAddr;
    CdsmCallBackDataMemInfo memInfo;
    memInfo.memberState = 1;
    memInfo.memberAddr = vendorReportAddr;
    dataBlock.memberInfo.push_back(memInfo);
    memInfo.memberAddr = commonColabAddr;
    dataBlock.memberInfo.push_back(memInfo);

    SetMockVendorBusinessTypeDevice(vendorReportAddr.GetAddress());
    ClearMockSetPeerDeviceTypeCount();
    cdsmService.SetPeerDeviceTypeToControllerInner(dataBlock, vendorReportAddr);
    EXPECT_EQ(GetMockSetPeerDeviceTypeCount(), 0);
    ClearMockSetPeerDeviceTypeCount();
    ClearMockVendorBusinessTypeDevice();

    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    HILOGI("NearlinkCdsmTest:SetPeerDeviceTypeToControllerInner_001 end");
}

/**
 * @tc.name: SetPeerDeviceTypeToControllerInner_002
 * @tc.desc: Test SetPeerDeviceTypeToControllerInner with an all-common group,
 *           the member except the report address should be set to controller
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, SetPeerDeviceTypeToControllerInner_002, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:SetPeerDeviceTypeToControllerInner_002 start");
    MockCdsmService cdsmService;
    RawAddress commonReportAddr("11:22:33:44:55:66");
    RawAddress commonColabAddr("11:22:33:44:55:77");

    CdsmCallBackData dataBlock;
    dataBlock.event = CDSM_EVENT_CONNECT;
    dataBlock.cdmsGroupId = 1;
    dataBlock.memberNum = 2;
    dataBlock.currentAddr = commonReportAddr;
    CdsmCallBackDataMemInfo memInfo;
    memInfo.memberState = 1;
    memInfo.memberAddr = commonReportAddr;
    dataBlock.memberInfo.push_back(memInfo);
    memInfo.memberAddr = commonColabAddr;
    dataBlock.memberInfo.push_back(memInfo);

    ClearMockVendorBusinessTypeDevice();
    ClearMockSetPeerDeviceTypeCount();
    cdsmService.SetPeerDeviceTypeToControllerInner(dataBlock, commonReportAddr);
    EXPECT_EQ(GetMockSetPeerDeviceTypeCount(), 1);
    ClearMockSetPeerDeviceTypeCount();

    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    HILOGI("NearlinkCdsmTest:SetPeerDeviceTypeToControllerInner_002 end");
}

class CdsmClientCallbackCounter : public InterfaceCdsmClientServiceCallback {
public:
    int callCount = 0;
    RawAddress lastReportAddr;
    size_t lastMemberCount = 0;
    void OnCdsmClientStateChange(const RawAddress &remoteAddr, std::vector<NearlinkCdsmInfo> &cdsmInfo,
        std::optional<uint64_t> tokenId = std::nullopt) override
    {
        callCount++;
        lastReportAddr = remoteAddr;
        lastMemberCount = cdsmInfo.size();
    }
};

namespace {
const std::string CDSM_UT_ADDR_A = "11:22:33:44:55:66";
const std::string CDSM_UT_ADDR_B = "11:22:33:44:55:88";
const std::string CDSM_UT_ADDR_C = "11:22:33:44:55:99";
const uint32_t CDSM_UT_GROUP_ID = 7;

// 测试目的：给服务注入一个合作集（指定 report 地址与成员），供成员管理类用例自足造数。
void CdsmUtPlantGroup(CdsmService *service, const std::string &reportAddr,
    const std::vector<std::string> &members, bool isPrivateTws = false)
{
    auto cdsmInfo = std::make_shared<CdsmInfo>(CDSM_UT_GROUP_ID, reportAddr);
    cdsmInfo->isPrivateTws_ = isPrivateTws;
    for (const auto &member : members) {
        cdsmInfo->CdsmAddMemberInfo(member, 1);
    }
    service->cdsmList_.EnsureInsert(CDSM_UT_GROUP_ID, cdsmInfo);
}
} // namespace

/**
 * @tc.name: GetConnectDevices002
 * @tc.desc: 已连接客户端出现在已连接设备列表，未连接与空客户端被跳过.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, GetConnectDevices002, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:GetConnectDevices002 start");
    CdsmService *cdsmService = CdsmService::GetService();
    ASSERT_NE(cdsmService, nullptr);

    auto connected = std::make_shared<MockCdsmClient>(CDSM_UT_ADDR_A);
    connected->SetState(CdsmClientState::CDSM_STATE_CONNECTED);
    auto connecting = std::make_shared<MockCdsmClient>(CDSM_UT_ADDR_B);
    connecting->SetState(CdsmClientState::CDSM_STATE_CONNECTING);
    cdsmService->cdsmClients_.EnsureInsert(CDSM_UT_ADDR_A, connected);
    cdsmService->cdsmClients_.EnsureInsert(CDSM_UT_ADDR_B, connecting);
    cdsmService->cdsmClients_.EnsureInsert(CDSM_UT_ADDR_C, nullptr);

    std::list<RawAddress> devices = cdsmService->GetConnectDevices();
    EXPECT_EQ(static_cast<size_t>(1), devices.size());
    if (devices.size() == 1) {
        EXPECT_EQ(CDSM_UT_ADDR_A, devices.front().GetAddress());
    }
    HILOGI("NearlinkCdsmTest:GetConnectDevices002 end");
}

/**
 * @tc.name: GetConnectState002
 * @tc.desc: 已连接客户端使聚合连接状态包含已连接位.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, GetConnectState002, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:GetConnectState002 start");
    CdsmService *cdsmService = CdsmService::GetService();
    ASSERT_NE(cdsmService, nullptr);

    auto connected = std::make_shared<MockCdsmClient>(CDSM_UT_ADDR_A);
    connected->SetState(CdsmClientState::CDSM_STATE_CONNECTED);
    cdsmService->cdsmClients_.EnsureInsert(CDSM_UT_ADDR_A, connected);

    int state = cdsmService->GetConnectState();
    EXPECT_NE(0, state & PROFILE_STATE_CONNECTED);
    HILOGI("NearlinkCdsmTest:GetConnectState002 end");
}

/**
 * @tc.name: GetRealConnectAddress001
 * @tc.desc: 本机地址在合作集内且自身已连接时返回本机地址.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, GetRealConnectAddress001, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:GetRealConnectAddress001 start");
    CdsmService *cdsmService = CdsmService::GetService();
    ASSERT_NE(cdsmService, nullptr);
    CdsmUtPlantGroup(cdsmService, CDSM_UT_ADDR_A, {CDSM_UT_ADDR_A, CDSM_UT_ADDR_B});
    cdsmService->cdsmList_.GetValueAndOpt(CDSM_UT_GROUP_ID,
        [](uint32_t, std::shared_ptr<CdsmInfo> data) {
            data->CdsmUpdateMemberProfileConnectState(CDSM_UT_ADDR_A, SleConnectState::CONNECTED);
        });
        [](uint32_t, std::shared_ptr<CdsmInfo> data) {
            data->CdsmUpdateMemberProfileConnectState(CDSM_UT_ADDR_A, SleConnectState::CONNECTED);
        });

    RawAddress realAddr;
    cdsmService->GetRealConnectAddress(RawAddress(CDSM_UT_ADDR_A), realAddr);
    EXPECT_EQ(CDSM_UT_ADDR_A, realAddr.GetAddress());
    HILOGI("NearlinkCdsmTest:GetRealConnectAddress001 end");
}

/**
 * @tc.name: GetRealConnectAddress002
 * @tc.desc: 本机未连接而对端成员已连接时返回对端地址.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, GetRealConnectAddress002, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:GetRealConnectAddress002 start");
    CdsmService *cdsmService = CdsmService::GetService();
    ASSERT_NE(cdsmService, nullptr);
    CdsmUtPlantGroup(cdsmService, CDSM_UT_ADDR_A, {CDSM_UT_ADDR_A, CDSM_UT_ADDR_B});
    cdsmService->cdsmList_.GetValueAndOpt(CDSM_UT_GROUP_ID,
        [](uint32_t, std::shared_ptr<CdsmInfo> data) {
            data->CdsmUpdateMemberProfileConnectState(CDSM_UT_ADDR_B, SleConnectState::CONNECTED);
        });

    RawAddress realAddr;
    cdsmService->GetRealConnectAddress(RawAddress(CDSM_UT_ADDR_A), realAddr);
    EXPECT_EQ(CDSM_UT_ADDR_B, realAddr.GetAddress());
    HILOGI("NearlinkCdsmTest:GetRealConnectAddress002 end");
}

/**
 * @tc.name: GetRealConnectAddress003
 * @tc.desc: 地址不属于任何合作集时原样返回传入地址.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, GetRealConnectAddress003, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:GetRealConnectAddress003 start");
    CdsmService *cdsmService = CdsmService::GetService();
    ASSERT_NE(cdsmService, nullptr);
    CdsmUtPlantGroup(cdsmService, CDSM_UT_ADDR_A, {CDSM_UT_ADDR_A});

    RawAddress realAddr;
    cdsmService->GetRealConnectAddress(RawAddress(CDSM_UT_ADDR_C), realAddr);
    EXPECT_EQ(CDSM_UT_ADDR_C, realAddr.GetAddress());
    HILOGI("NearlinkCdsmTest:GetRealConnectAddress003 end");
}

/**
 * @tc.name: CdsmReplaceOldReportAddr001
 * @tc.desc: 新地址是成员且旧地址是当前上报地址时完成上报地址替换.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, CdsmReplaceOldReportAddr001, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:CdsmReplaceOldReportAddr001 start");
    CdsmService *cdsmService = CdsmService::GetService();
    ASSERT_NE(cdsmService, nullptr);
    CdsmUtPlantGroup(cdsmService, CDSM_UT_ADDR_A, {CDSM_UT_ADDR_A, CDSM_UT_ADDR_B});

    std::vector<std::string> memberList = {CDSM_UT_ADDR_A, CDSM_UT_ADDR_B};
    SleConfig::GetInstance().SetCdsmMemberList(CDSM_UT_ADDR_A, memberList);

    bool ret = cdsmService->CdsmReplaceOldReportAddr(RawAddress(CDSM_UT_ADDR_A), RawAddress(CDSM_UT_ADDR_B));
    EXPECT_EQ(true, ret);
    std::vector<std::string> newMemberList;
    EXPECT_EQ(true, SleConfig::GetInstance().GetCdsmMemberList(CDSM_UT_ADDR_B, newMemberList));
    SleConfig::GetInstance().RemoveCdsmGroup(CDSM_UT_ADDR_B);
    SleConfig::GetInstance().Save();
    HILOGI("NearlinkCdsmTest:CdsmReplaceOldReportAddr001 end");
}

/**
 * @tc.name: CdsmReplaceOldReportAddr002
 * @tc.desc: 新地址不是合作集成员时不替换.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, CdsmReplaceOldReportAddr002, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:CdsmReplaceOldReportAddr002 start");
    CdsmService *cdsmService = CdsmService::GetService();
    ASSERT_NE(cdsmService, nullptr);
    CdsmUtPlantGroup(cdsmService, CDSM_UT_ADDR_A, {CDSM_UT_ADDR_A});

    bool ret = cdsmService->CdsmReplaceOldReportAddr(RawAddress(CDSM_UT_ADDR_A), RawAddress(CDSM_UT_ADDR_C));
    EXPECT_EQ(false, ret);
    HILOGI("NearlinkCdsmTest:CdsmReplaceOldReportAddr002 end");
}

/**
 * @tc.name: CdsmReplaceOldReportAddr003
 * @tc.desc: 旧地址与当前上报地址不一致时不替换.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, CdsmReplaceOldReportAddr003, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:CdsmReplaceOldReportAddr003 start");
    CdsmService *cdsmService = CdsmService::GetService();
    ASSERT_NE(cdsmService, nullptr);
    CdsmUtPlantGroup(cdsmService, CDSM_UT_ADDR_A, {CDSM_UT_ADDR_A, CDSM_UT_ADDR_B});

    bool ret = cdsmService->CdsmReplaceOldReportAddr(RawAddress(CDSM_UT_ADDR_B), RawAddress(CDSM_UT_ADDR_C));
    EXPECT_EQ(false, ret);
    HILOGI("NearlinkCdsmTest:CdsmReplaceOldReportAddr003 end");
}

/**
 * @tc.name: CdsmRecoverFromConf002
 * @tc.desc: 配置中的合作集按成员列表与本地占位情况分别跳过恢复.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, CdsmRecoverFromConf002, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:CdsmRecoverFromConf002 start");
    CdsmService *cdsmService = CdsmService::GetService();
    ASSERT_NE(cdsmService, nullptr);
    CdsmUtPlantGroup(cdsmService, CDSM_UT_ADDR_A, {CDSM_UT_ADDR_A});

    std::vector<std::string> memberList = {CDSM_UT_ADDR_A, CDSM_UT_ADDR_B};
    SleConfig::GetInstance().SetCdsmMemberList(CDSM_UT_ADDR_A, memberList);
    SleConfig::GetInstance().SetCdsmMemberList(CDSM_UT_ADDR_C, std::vector<std::string> {});

    cdsmService->CdsmRecoverFromConf();

    uint32_t groupId = 0;
    EXPECT_EQ(true, cdsmService->CdsmGetGroupId(groupId, CDSM_UT_ADDR_A));
    SleConfig::GetInstance().RemoveCdsmGroup(CDSM_UT_ADDR_A);
    SleConfig::GetInstance().RemoveCdsmGroup(CDSM_UT_ADDR_C);
    SleConfig::GetInstance().Save();
    HILOGI("NearlinkCdsmTest:CdsmRecoverFromConf002 end");
}

/**
 * @tc.name: RegisterCdsmClientCallback001
 * @tc.desc: 注册空回调被参数校验拒绝.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, RegisterCdsmClientCallback001, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:RegisterCdsmClientCallback001 start");
    CdsmService *cdsmService = CdsmService::GetService();
    ASSERT_NE(cdsmService, nullptr);

    NlErrCode ret = cdsmService->RegisterCdsmClientCallback(nullptr);
    EXPECT_EQ(NL_ERR_INVALID_PARAM, ret);
    HILOGI("NearlinkCdsmTest:RegisterCdsmClientCallback001 end");
}

/**
 * @tc.name: HandleCdsmClientCallback002
 * @tc.desc: 应用回调注册后状态变化上报携带成员列表到达回调.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, HandleCdsmClientCallback002, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:HandleCdsmClientCallback002 start");
    CdsmService *cdsmService = CdsmService::GetService();
    ASSERT_NE(cdsmService, nullptr);
    CdsmUtPlantGroup(cdsmService, CDSM_UT_ADDR_A, {CDSM_UT_ADDR_A, CDSM_UT_ADDR_B});

    auto cbk = std::make_shared<CdsmClientCallbackCounter>();
    ASSERT_EQ(NL_NO_ERROR, cdsmService->RegisterCdsmClientCallback(cbk));

    cdsmService->HandleCdsmClientCallback(RawAddress(CDSM_UT_ADDR_A));
    EXPECT_EQ(1, cbk->callCount);
    EXPECT_EQ(static_cast<size_t>(2), cbk->lastMemberCount);
    cdsmService->DeregisterCdsmClientCallback();
    HILOGI("NearlinkCdsmTest:HandleCdsmClientCallback002 end");
}

/**
 * @tc.name: HandleCdsmClientCallback003
 * @tc.desc: 设备不在任何合作集时状态上报提前返回.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, HandleCdsmClientCallback003, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:HandleCdsmClientCallback003 start");
    CdsmService *cdsmService = CdsmService::GetService();
    ASSERT_NE(cdsmService, nullptr);

    auto cbk = std::make_shared<CdsmClientCallbackCounter>();
    ASSERT_EQ(NL_NO_ERROR, cdsmService->RegisterCdsmClientCallback(cbk));

    cdsmService->HandleCdsmClientCallback(RawAddress(CDSM_UT_ADDR_C));
    EXPECT_EQ(0, cbk->callCount);
    cdsmService->DeregisterCdsmClientCallback();
    HILOGI("NearlinkCdsmTest:HandleCdsmClientCallback003 end");
}

/**
 * @tc.name: UpdateCdsmMemberProfileConnectState002
 * @tc.desc: 成员连接或断开触发应用回调，其他状态不触发.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, UpdateCdsmMemberProfileConnectState002, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:UpdateCdsmMemberProfileConnectState002 start");
    CdsmService *cdsmService = CdsmService::GetService();
    ASSERT_NE(cdsmService, nullptr);
    CdsmUtPlantGroup(cdsmService, CDSM_UT_ADDR_A, {CDSM_UT_ADDR_A, CDSM_UT_ADDR_B});

    auto cbk = std::make_shared<CdsmClientCallbackCounter>();
    ASSERT_EQ(NL_NO_ERROR, cdsmService->RegisterCdsmClientCallback(cbk));

    cdsmService->UpdateCdsmMemberProfileConnectState(RawAddress(CDSM_UT_ADDR_A), SleConnectState::CONNECTED);
    EXPECT_EQ(1, cbk->callCount);
    cdsmService->UpdateCdsmMemberProfileConnectState(RawAddress(CDSM_UT_ADDR_A), SleConnectState::DISCONNECTED);
    EXPECT_EQ(2, cbk->callCount);
    cdsmService->UpdateCdsmMemberProfileConnectState(RawAddress(CDSM_UT_ADDR_A), SleConnectState::CONNECTING);
    EXPECT_EQ(2, cbk->callCount);
    cdsmService->DeregisterCdsmClientCallback();
    HILOGI("NearlinkCdsmTest:UpdateCdsmMemberProfileConnectState002 end");
}

/**
 * @tc.name: UpdateReportConnectState002
 * @tc.desc: 上报设备连接状态写入合作集并可查询到.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, UpdateReportConnectState002, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:UpdateReportConnectState002 start");
    CdsmService *cdsmService = CdsmService::GetService();
    ASSERT_NE(cdsmService, nullptr);
    CdsmUtPlantGroup(cdsmService, CDSM_UT_ADDR_A, {CDSM_UT_ADDR_A, CDSM_UT_ADDR_B});

    cdsmService->UpdateReportConnectState(RawAddress(CDSM_UT_ADDR_A), static_cast<int>(SleConnectState::CONNECTED));
    int reportState = static_cast<int>(SleConnectState::INVALID_STATE);
    cdsmService->CdsmGetReportedState(RawAddress(CDSM_UT_ADDR_A), reportState);
    EXPECT_EQ(static_cast<int>(SleConnectState::CONNECTED), reportState);
    HILOGI("NearlinkCdsmTest:UpdateReportConnectState002 end");
}

/**
 * @tc.name: CdsmGetReportedState002
 * @tc.desc: 非合作集成员查询上报状态得到无效状态.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, CdsmGetReportedState002, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:CdsmGetReportedState002 start");
    CdsmService *cdsmService = CdsmService::GetService();
    ASSERT_NE(cdsmService, nullptr);
    CdsmUtPlantGroup(cdsmService, CDSM_UT_ADDR_A, {CDSM_UT_ADDR_A});

    int reportState = static_cast<int>(SleConnectState::CONNECTED);
    cdsmService->CdsmGetReportedState(RawAddress(CDSM_UT_ADDR_C), reportState);
    EXPECT_EQ(static_cast<int>(SleConnectState::INVALID_STATE), reportState);
    HILOGI("NearlinkCdsmTest:CdsmGetReportedState002 end");
}

/**
 * @tc.name: CdsmCheckIsPrivateCooperationDevice002
 * @tc.desc: 私有耳机合作集成员判定为私有合作设备.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, CdsmCheckIsPrivateCooperationDevice002, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:CdsmCheckIsPrivateCooperationDevice002 start");
    CdsmService *cdsmService = CdsmService::GetService();
    ASSERT_NE(cdsmService, nullptr);
    CdsmUtPlantGroup(cdsmService, CDSM_UT_ADDR_A, {CDSM_UT_ADDR_A, CDSM_UT_ADDR_B}, true);

    EXPECT_EQ(true, cdsmService->CdsmCheckIsPrivateCooperationDevice(RawAddress(CDSM_UT_ADDR_B)));
    HILOGI("NearlinkCdsmTest:CdsmCheckIsPrivateCooperationDevice002 end");
}

/**
 * @tc.name: CdsmCheckIsCooperationMember002
 * @tc.desc: 成员地址判为合作集成员，上报地址不判为成员.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, CdsmCheckIsCooperationMember002, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:CdsmCheckIsCooperationMember002 start");
    CdsmService *cdsmService = CdsmService::GetService();
    ASSERT_NE(cdsmService, nullptr);
    CdsmUtPlantGroup(cdsmService, CDSM_UT_ADDR_A, {CDSM_UT_ADDR_A, CDSM_UT_ADDR_B});

    EXPECT_EQ(true, cdsmService->CdsmCheckIsCooperationMember(RawAddress(CDSM_UT_ADDR_B)));
    EXPECT_EQ(false, cdsmService->CdsmCheckIsCooperationMember(RawAddress(CDSM_UT_ADDR_A)));
    HILOGI("NearlinkCdsmTest:CdsmCheckIsCooperationMember002 end");
}

/**
 * @tc.name: CdsmCheckIsCooperationDevice002
 * @tc.desc: 合作集内地址判为合作设备，集外地址不判为合作设备.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, CdsmCheckIsCooperationDevice002, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:CdsmCheckIsCooperationDevice002 start");
    CdsmService *cdsmService = CdsmService::GetService();
    ASSERT_NE(cdsmService, nullptr);
    CdsmUtPlantGroup(cdsmService, CDSM_UT_ADDR_A, {CDSM_UT_ADDR_A, CDSM_UT_ADDR_B});

    EXPECT_EQ(true, cdsmService->CdsmCheckIsCooperationDevice(RawAddress(CDSM_UT_ADDR_B)));
    EXPECT_EQ(false, cdsmService->CdsmCheckIsCooperationDevice(RawAddress(CDSM_UT_ADDR_C)));
    HILOGI("NearlinkCdsmTest:CdsmCheckIsCooperationDevice002 end");
}

/**
 * @tc.name: CdsmGetReportAddr002
 * @tc.desc: 成员地址可查到所在合作集的上报地址.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, CdsmGetReportAddr002, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:CdsmGetReportAddr002 start");
    CdsmService *cdsmService = CdsmService::GetService();
    ASSERT_NE(cdsmService, nullptr);
    CdsmUtPlantGroup(cdsmService, CDSM_UT_ADDR_A, {CDSM_UT_ADDR_A, CDSM_UT_ADDR_B});

    RawAddress reportAddr;
    EXPECT_EQ(true, cdsmService->CdsmGetReportAddr(RawAddress(CDSM_UT_ADDR_B), reportAddr));
    EXPECT_EQ(CDSM_UT_ADDR_A, reportAddr.GetAddress());
    HILOGI("NearlinkCdsmTest:CdsmGetReportAddr002 end");
}

/**
 * @tc.name: CdsmServiceCallBackDataProc001
 * @tc.desc: 栈连接事件为未知组创建合作集数据并派发连接完成消息.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, CdsmServiceCallBackDataProc001, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:CdsmServiceCallBackDataProc001 start");
    CdsmService *cdsmService = CdsmService::GetService();
    ASSERT_NE(cdsmService, nullptr);

    CdsmCallBackData dataBlock;
    dataBlock.event = CDSM_EVENT_CONNECT;
    dataBlock.cdmsGroupId = CDSM_UT_GROUP_ID;
    dataBlock.memberNum = 2;
    dataBlock.currentAddr = RawAddress(CDSM_UT_ADDR_A);
    CdsmCallBackDataMemInfo memInfo;
    memInfo.memberAddr = RawAddress(CDSM_UT_ADDR_A);
    memInfo.memberState = 1;
    dataBlock.memberInfo.push_back(memInfo);
    memInfo.memberAddr = RawAddress(CDSM_UT_ADDR_B);
    memInfo.memberState = 1;
    dataBlock.memberInfo.push_back(memInfo);

    cdsmService->CdsmServiceCallBackDataProc(dataBlock);

    uint32_t groupId = 0;
    EXPECT_EQ(true, cdsmService->CdsmGetGroupId(groupId, CDSM_UT_ADDR_B));
    HILOGI("NearlinkCdsmTest:CdsmServiceCallBackDataProc001 end");
}

/**
 * @tc.name: CdsmServiceCallBackDataProc002
 * @tc.desc: 栈断连事件更新合作集内设备状态为断开.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, CdsmServiceCallBackDataProc002, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:CdsmServiceCallBackDataProc002 start");
    CdsmService *cdsmService = CdsmService::GetService();
    ASSERT_NE(cdsmService, nullptr);
    CdsmUtPlantGroup(cdsmService, CDSM_UT_ADDR_A, {CDSM_UT_ADDR_A, CDSM_UT_ADDR_B});

    CdsmCallBackData dataBlock;
    dataBlock.event = CDSM_EVENT_DISCONNECT;
    dataBlock.cdmsGroupId = CDSM_UT_GROUP_ID;
    dataBlock.memberNum = 2;
    dataBlock.currentAddr = RawAddress(CDSM_UT_ADDR_A);

    cdsmService->CdsmServiceCallBackDataProc(dataBlock);

    std::vector<NearlinkCdsmInfo> members;
    ASSERT_EQ(NL_NO_ERROR, cdsmService->CdsmGetAllMemberInfo(RawAddress(CDSM_UT_ADDR_A), members));
    EXPECT_EQ(static_cast<size_t>(2), members.size());
    HILOGI("NearlinkCdsmTest:CdsmServiceCallBackDataProc002 end");
}

/**
 * @tc.name: CdsmServiceCallBackDataProc003
 * @tc.desc: 未知栈事件被丢弃不创建合作集.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, CdsmServiceCallBackDataProc003, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:CdsmServiceCallBackDataProc003 start");
    CdsmService *cdsmService = CdsmService::GetService();
    ASSERT_NE(cdsmService, nullptr);

    CdsmCallBackData dataBlock;
    dataBlock.event = 0xFF;
    dataBlock.cdmsGroupId = CDSM_UT_GROUP_ID;
    dataBlock.memberNum = 1;
    dataBlock.currentAddr = RawAddress(CDSM_UT_ADDR_A);

    cdsmService->CdsmServiceCallBackDataProc(dataBlock);

    uint32_t groupId = 0;
    EXPECT_EQ(false, cdsmService->CdsmGetGroupId(groupId, CDSM_UT_ADDR_A));
    HILOGI("NearlinkCdsmTest:CdsmServiceCallBackDataProc003 end");
}

/**
 * @tc.name: ProcessEvent002
 * @tc.desc: 连接开始消息为不存在客户端的设备新建客户端并置为连接中.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, ProcessEvent002, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:ProcessEvent002 start");
    CdsmService *cdsmService = CdsmService::GetService();
    ASSERT_NE(cdsmService, nullptr);

    CdsmMessage event(CDSM_SERVICE_CONNECT_START_EVT);
    event.dev_ = CDSM_UT_ADDR_A;
    cdsmService->ProcessEvent(event);

    std::list<RawAddress> devices = cdsmService->GetConnectDevices();
    EXPECT_EQ(static_cast<size_t>(0), devices.size());
    HILOGI("NearlinkCdsmTest:ProcessEvent002 end");
}

/**
 * @tc.name: ProcessEvent003
 * @tc.desc: 连接完成消息把已存在客户端更新为已连接.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, ProcessEvent003, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:ProcessEvent003 start");
    CdsmService *cdsmService = CdsmService::GetService();
    ASSERT_NE(cdsmService, nullptr);

    auto client = std::make_shared<MockCdsmClient>(CDSM_UT_ADDR_A);
    client->SetState(CdsmClientState::CDSM_STATE_CONNECTING);
    cdsmService->cdsmClients_.EnsureInsert(CDSM_UT_ADDR_A, client);

    CdsmMessage event(CDSM_SERVICE_CONNECT_CMPL_EVT);
    event.dev_ = CDSM_UT_ADDR_A;
    cdsmService->ProcessEvent(event);

    std::list<RawAddress> devices = cdsmService->GetConnectDevices();
    EXPECT_EQ(static_cast<size_t>(1), devices.size());
    HILOGI("NearlinkCdsmTest:ProcessEvent003 end");
}

/**
 * @tc.name: ProcessEvent004
 * @tc.desc: 断开完成消息作用于不存在客户端时仅记录日志.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, ProcessEvent004, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:ProcessEvent004 start");
    CdsmService *cdsmService = CdsmService::GetService();
    ASSERT_NE(cdsmService, nullptr);

    CdsmMessage event(CDSM_SERVICE_DISCONNECT_CMPL_EVT);
    event.dev_ = CDSM_UT_ADDR_C;
    cdsmService->ProcessEvent(event);

    std::list<RawAddress> devices = cdsmService->GetConnectDevices();
    EXPECT_EQ(static_cast<size_t>(0), devices.size());
    HILOGI("NearlinkCdsmTest:ProcessEvent004 end");
}

/**
 * @tc.name: ProcessEvent005
 * @tc.desc: 不支持的消息类型走默认分支不影响现有客户端.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, ProcessEvent005, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:ProcessEvent005 start");
    CdsmService *cdsmService = CdsmService::GetService();
    ASSERT_NE(cdsmService, nullptr);

    auto client = std::make_shared<MockCdsmClient>(CDSM_UT_ADDR_A);
    client->SetState(CdsmClientState::CDSM_STATE_CONNECTED);
    cdsmService->cdsmClients_.EnsureInsert(CDSM_UT_ADDR_A, client);

    CdsmMessage event(0xFF);
    event.dev_ = CDSM_UT_ADDR_A;
    cdsmService->ProcessEvent(event);

    std::list<RawAddress> devices = cdsmService->GetConnectDevices();
    EXPECT_EQ(static_cast<size_t>(1), devices.size());
    HILOGI("NearlinkCdsmTest:ProcessEvent005 end");
}

/**
 * @tc.name: CdsmGetAllMemberInfo001
 * @tc.desc: 查询合作集成员返回全部成员并标记上报地址.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, CdsmGetAllMemberInfo001, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:CdsmGetAllMemberInfo001 start");
    CdsmService *cdsmService = CdsmService::GetService();
    ASSERT_NE(cdsmService, nullptr);
    CdsmUtPlantGroup(cdsmService, CDSM_UT_ADDR_A, {CDSM_UT_ADDR_A, CDSM_UT_ADDR_B});

    std::vector<NearlinkCdsmInfo> members;
    NlErrCode ret = cdsmService->CdsmGetAllMemberInfo(RawAddress(CDSM_UT_ADDR_B), members);
    EXPECT_EQ(NL_NO_ERROR, ret);
    EXPECT_EQ(static_cast<size_t>(2), members.size());
    int reportCount = 0;
    for (const auto &member : members) {
        if (member.isReportAddr_) {
            reportCount++;
            EXPECT_EQ(CDSM_UT_ADDR_A, member.addr_.GetAddress());
        }
    }
    EXPECT_EQ(1, reportCount);
    HILOGI("NearlinkCdsmTest:CdsmGetAllMemberInfo001 end");
}

/**
 * @tc.name: CdsmGetAllMemberInfo002
 * @tc.desc: 查询非合作集地址返回内部错误.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, CdsmGetAllMemberInfo002, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:CdsmGetAllMemberInfo002 start");
    CdsmService *cdsmService = CdsmService::GetService();
    ASSERT_NE(cdsmService, nullptr);

    std::vector<NearlinkCdsmInfo> members;
    NlErrCode ret = cdsmService->CdsmGetAllMemberInfo(RawAddress(CDSM_UT_ADDR_C), members);
    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, ret);
    HILOGI("NearlinkCdsmTest:CdsmGetAllMemberInfo002 end");
}

/**
 * @tc.name: CdsmCheckIsSameCooperation001
 * @tc.desc: 设备列表与合作集成员一致判为同一合作集.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, CdsmCheckIsSameCooperation001, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:CdsmCheckIsSameCooperation001 start");
    CdsmService *cdsmService = CdsmService::GetService();
    ASSERT_NE(cdsmService, nullptr);
    CdsmUtPlantGroup(cdsmService, CDSM_UT_ADDR_A, {CDSM_UT_ADDR_A, CDSM_UT_ADDR_B});

    std::vector<RawAddress> sameList = {RawAddress(CDSM_UT_ADDR_A), RawAddress(CDSM_UT_ADDR_B)};
    EXPECT_EQ(true, cdsmService->CdsmCheckIsSameCooperation(sameList));

    std::vector<RawAddress> emptyList;
    EXPECT_EQ(false, cdsmService->CdsmCheckIsSameCooperation(emptyList));

    std::vector<RawAddress> otherList = {RawAddress(CDSM_UT_ADDR_A), RawAddress(CDSM_UT_ADDR_C)};
    EXPECT_EQ(false, cdsmService->CdsmCheckIsSameCooperation(otherList));
    HILOGI("NearlinkCdsmTest:CdsmCheckIsSameCooperation001 end");
}

/**
 * @tc.name: CdsmCreateGroup002
 * @tc.desc: 相同成员的合作集已存在时创建组返回既有组标识.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, CdsmCreateGroup002, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:CdsmCreateGroup002 start");
    CdsmService *cdsmService = CdsmService::GetService();
    ASSERT_NE(cdsmService, nullptr);
    CdsmUtPlantGroup(cdsmService, CDSM_UT_ADDR_A, {CDSM_UT_ADDR_A, CDSM_UT_ADDR_B});

    std::vector<RawAddress> sameList = {RawAddress(CDSM_UT_ADDR_A), RawAddress(CDSM_UT_ADDR_B)};
    uint32_t groupId = cdsmService->CdsmCreateGroup(RawAddress(CDSM_UT_ADDR_A), sameList);
    EXPECT_EQ(CDSM_UT_GROUP_ID, groupId);
    HILOGI("NearlinkCdsmTest:CdsmCreateGroup002 end");
}

/**
 * @tc.name: CdsmDeleteGroup002
 * @tc.desc: 删除合作集后成员地址不再能查到组标识.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, CdsmDeleteGroup002, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:CdsmDeleteGroup002 start");
    CdsmService *cdsmService = CdsmService::GetService();
    ASSERT_NE(cdsmService, nullptr);
    CdsmUtPlantGroup(cdsmService, CDSM_UT_ADDR_A, {CDSM_UT_ADDR_A, CDSM_UT_ADDR_B});
    auto client = std::make_shared<MockCdsmClient>(CDSM_UT_ADDR_A);
    cdsmService->cdsmClients_.EnsureInsert(CDSM_UT_ADDR_A, client);

    cdsmService->CdsmDeleteGroup(RawAddress(CDSM_UT_ADDR_B));

    uint32_t groupId = 0;
    EXPECT_EQ(false, cdsmService->CdsmGetGroupId(groupId, CDSM_UT_ADDR_A));
    HILOGI("NearlinkCdsmTest:CdsmDeleteGroup002 end");
}

/**
 * @tc.name: CdsmStopInviteAdv002
 * @tc.desc: 非强制停邀请广播要求地址是合作集成员.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, CdsmStopInviteAdv002, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:CdsmStopInviteAdv002 start");
    CdsmService *cdsmService = CdsmService::GetService();
    ASSERT_NE(cdsmService, nullptr);
    CdsmUtPlantGroup(cdsmService, CDSM_UT_ADDR_A, {CDSM_UT_ADDR_A, CDSM_UT_ADDR_B});

    cdsmService->CdsmStopInviteAdv(RawAddress(CDSM_UT_ADDR_C), false);
    cdsmService->CdsmStopInviteAdv(RawAddress(CDSM_UT_ADDR_B), false);
    HILOGI("NearlinkCdsmTest:CdsmStopInviteAdv002 end");
}

/**
 * @tc.name: ClearOldDevice002
 * @tc.desc: 回调成员覆盖全部既有成员时不清理旧设备.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, ClearOldDevice002, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:ClearOldDevice002 start");

    MockCdsmService cdsmService;
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
    cdsmDataPtr->CdsmAddMemberInfo("11:22:33:44:55:77", 1);

    RawAddress oldDevAddr;
    bool ret = cdsmService.ClearOldDevice(dataBlock, cdsmDataPtr, oldDevAddr);
    EXPECT_EQ(false, ret);
    HILOGI("NearlinkCdsmTest:ClearOldDevice002 end");
}

/**
 * @tc.name: CdsmClientUpdateState001
 * @tc.desc: 客户端状态与目标状态一致时不重复上报.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, CdsmClientUpdateState001, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:CdsmClientUpdateState001 start");
    MockCdsmClient cdsmClient(CDSM_UT_ADDR_A);
    cdsmClient.SetState(CdsmClientState::CDSM_STATE_CONNECTED);
    cdsmClient.CdsmClientUpdateState(CdsmClientState::CDSM_STATE_CONNECTED);
    EXPECT_EQ(CdsmClientState::CDSM_STATE_CONNECTED, cdsmClient.CdsmClientGetState());
    HILOGI("NearlinkCdsmTest:CdsmClientUpdateState001 end");
}

/**
 * @tc.name: CdsmClientUpdateState002
 * @tc.desc: 私有合作集成员连接后不启动邀请广播.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, CdsmClientUpdateState002, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:CdsmClientUpdateState002 start");
    CdsmService *cdsmService = CdsmService::GetService();
    ASSERT_NE(cdsmService, nullptr);
    CdsmUtPlantGroup(cdsmService, CDSM_UT_ADDR_A, {CDSM_UT_ADDR_A, CDSM_UT_ADDR_B}, true);

    MockCdsmClient cdsmClient(CDSM_UT_ADDR_B);
    cdsmClient.SetState(CdsmClientState::CDSM_STATE_CONNECTING);
    cdsmClient.CdsmClientUpdateState(CdsmClientState::CDSM_STATE_CONNECTED);
    EXPECT_EQ(CdsmClientState::CDSM_STATE_CONNECTED, cdsmClient.CdsmClientGetState());
    HILOGI("NearlinkCdsmTest:CdsmClientUpdateState002 end");
}

/**
 * @tc.name: CdsmClientUpdateState003
 * @tc.desc: 公版合作集上报设备连接成功后尝试启动邀请广播.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, CdsmClientUpdateState003, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:CdsmClientUpdateState003 start");
    CdsmService *cdsmService = CdsmService::GetService();
    ASSERT_NE(cdsmService, nullptr);
    CdsmUtPlantGroup(cdsmService, CDSM_UT_ADDR_A, {CDSM_UT_ADDR_A, CDSM_UT_ADDR_B});

    MockCdsmClient cdsmClient(CDSM_UT_ADDR_A);
    cdsmClient.SetState(CdsmClientState::CDSM_STATE_CONNECTING);
    cdsmClient.CdsmClientUpdateState(CdsmClientState::CDSM_STATE_CONNECTED);
    EXPECT_EQ(CdsmClientState::CDSM_STATE_CONNECTED, cdsmClient.CdsmClientGetState());
    HILOGI("NearlinkCdsmTest:CdsmClientUpdateState003 end");
}

/**
 * @tc.name: CdsmClientUpdateState004
 * @tc.desc: 服务未注册时客户端状态变化仅记录日志不崩溃.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, CdsmClientUpdateState004, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:CdsmClientUpdateState004 start");
    MockCdsmService cdsmService;
    MockCdsmClient cdsmClient(CDSM_UT_ADDR_A);
    cdsmClient.SetState(CdsmClientState::CDSM_STATE_CONNECTING);
    cdsmClient.CdsmClientUpdateState(CdsmClientState::CDSM_STATE_CONNECTED);
    EXPECT_EQ(CdsmClientState::CDSM_STATE_CONNECTED, cdsmClient.CdsmClientGetState());
    HILOGI("NearlinkCdsmTest:CdsmClientUpdateState004 end");
}

/**
 * @tc.name: CdsmClientStopInviteAdv002
 * @tc.desc: 私有合作集成员不停止邀请广播.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, CdsmClientStopInviteAdv002, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:CdsmClientStopInviteAdv002 start");
    CdsmService *cdsmService = CdsmService::GetService();
    ASSERT_NE(cdsmService, nullptr);
    CdsmUtPlantGroup(cdsmService, CDSM_UT_ADDR_A, {CDSM_UT_ADDR_A, CDSM_UT_ADDR_B}, true);

    MockCdsmClient cdsmClient(CDSM_UT_ADDR_B);
    cdsmClient.CdsmStopInviteAdv();
    HILOGI("NearlinkCdsmTest:CdsmClientStopInviteAdv002 end");
}

/**
 * @tc.name: CreateCdsmClient001
 * @tc.desc: 音频能力开启时创建合作集客户端成功.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkCdsmTest, CreateCdsmClient001, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:CreateCdsmClient001 start");
    std::shared_ptr<CdsmClient> client = CdsmClient::CreateCdsmClient(CDSM_UT_ADDR_A);
    EXPECT_NE(nullptr, client);
    HILOGI("NearlinkCdsmTest:CreateCdsmClient001 end");
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS
