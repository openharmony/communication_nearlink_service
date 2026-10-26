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

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS
