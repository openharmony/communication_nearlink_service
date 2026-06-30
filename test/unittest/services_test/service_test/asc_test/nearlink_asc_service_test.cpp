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
#include <thread>
#include <string>
#include "log.h"
#include "log_util.h"
#include "ASCService.h"
#include "nearlink_access_token_mock.h"
#include "sle_service_data.h"

#include "SleAudioFrameworkAdapter.h"
#include "SleInterfaceManager.h"
#include "SleServiceManager.h"
#include "SleRemoteDeviceAdapter.h"
#include "ASCUtils.h"
#include "SleDliSnoop.h"
#include "SleInterfaceProfileASC.h"
#include "CdsmService.h"
#include "actm_callback.h"
#include "nlstk_api_type_ext.h"
#include "ASCService.cpp"
#include "ipc_skeleton.h"

namespace OHOS {
namespace Nearlink {
using namespace testing;
using namespace testing::ext;

class ASCObserverCommon : public ASCObserver {
public:
    ASCObserverCommon() = default;
    virtual ~ASCObserverCommon() = default;

    void OnConnectionStateChanged(const RawAddress &deviceAddress, int state, int oldState){}
};

namespace {
constexpr int HOST_FUZZ_DELAY_50_MS = 50;
constexpr int HOST_SERVER_TDD_DELAY_1000_MS = 1000;
constexpr int TDD_DELAY_2000_MS = 2000;
const std::string deviceStr = "AA:BB:CC:DD:EE:FF";
const std::string coDeviceStr = "11:22:33:44:55:66";
ASCObserverCommon g_ascServiceObserver_;
}

class ASCServiceTest : public testing::Test {
public:
    ASCServiceTest() = default;
    ~ASCServiceTest() = default;

    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void ASCServiceTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase ASCServiceTest.");
    OHOS::Nearlink::NearlinkAccessTokenMock::SetNativeTokenInfo();
    SleInterfaceManager::GetInstance()->Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::HOST_SERVER_TDD_DELAY_1000_MS));
}

void ASCServiceTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase ASCServiceTest");
}

void ASCServiceTest::SetUp()
{
    HILOGI("SetUp ASCServiceTest.");
}

void ASCServiceTest::TearDown()
{
    HILOGI("TearDown ASCServiceTest.");
}

/**
 * @tc.name: AddConnectDevices_001
 * @tc.desc: AddConnectDevices
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, AddConnectDevices_001, TestSize.Level1)
{
    HILOGI("AddConnectDevices_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    EXPECT_EQ(true, 0 != asc->GetConnectState());
    asc->DeleteConnectDevices(device);
    EXPECT_EQ(false, 0 != asc->GetConnectState());

    asc->AddConnectDevices(device);
    EXPECT_EQ(true, 0 != asc->GetConnectState());
    asc->IsLeftEarDevice(device);
    RawAddress coSetDevice;
    EXPECT_EQ(false, asc->IsCoSetDeviceExist(device, coSetDevice));
    EXPECT_EQ(false, asc->IsCoSetDeviceStarted(device, coSetDevice));
    std::map<std::string, RawAddress> reportAddrMap {};
    asc->GetAllReportAddr(reportAddrMap);
    EXPECT_EQ(true, 0 != reportAddrMap.size());
    EXPECT_EQ(true, 0 != asc->GetCoSetNum());
    asc->DeleteEarliestDevice();
    EXPECT_EQ(true, 0 != asc->GetConnectState());
    asc->AddConnectDevices(device);
    RawAddress coDevice(coDeviceStr);
    asc->AddConnectDevices(coDevice);
    asc->activeSinkDevice_ = coDevice;
    asc->SetASCStatus(device, NL_SLE_ASC_DISABLED);
    asc->SetASCStatus(coDevice, NL_SLE_ASC_DISABLED);
    asc->DeleteEarliestDevice();
    asc->ShutDown();
    HILOGI("AddConnectDevices_001 end");
}

/**
 * @tc.name: GetAudioProperty_001
 * @tc.desc: GetAudioProperty
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, GetAudioProperty_001, TestSize.Level1)
{
    HILOGI("GetAudioProperty_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    asc->GetAudioProperty(device);
    EXPECT_EQ(true, 0 != asc->GetConnectState());
    HILOGI("GetAudioProperty_001 end");
}

/**
 * @tc.name: Enable_001
 * @tc.desc: Enable
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, Enable_001, TestSize.Level1)
{
    HILOGI("Enable_001 enter");
    ASCService *asc = new ASCService();
    asc->InitDisconnProcTable();
    RawAddress device = RawAddress(deviceStr);
    asc->Enable();
    asc->Disable();
    asc->StartUp();
    asc->Connect(device);
    EXPECT_EQ(false, 0 != asc->GetConnectState());
    ASCMessage event;
    event.dev_ = device.GetAddress();
    asc->ProcessConnectEvent(event);
    asc->AddConnectDevices(device);
    EXPECT_EQ(true, 0 != asc->GetConnectState());

    asc->Disconnect(device);
    asc->ProcessDisConnectEvent(event);
    asc->ShutDown();
    EXPECT_EQ(true, 0 != asc->GetConnectState());
    HILOGI("Enable_001 end");
}

/**
 * @tc.name: DisconnProcAction_001
 * @tc.desc: DisconnProcAction
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, DisconnProcAction_001, TestSize.Level1)
{
    HILOGI("DisconnProcAction_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    asc->DisconnProcAction(device, NL_SLE_ASC_STARTED);
    EXPECT_EQ(true, 0 == asc->DisconnProcAction(device, NL_SLE_ASC_STARTED));
    HILOGI("DisconnProcAction_001 end");
}

/**
 * @tc.name: DisconnProcSetFlag_001
 * @tc.desc: DisconnProcSetFlag
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, DisconnProcSetFlag_001, TestSize.Level1)
{
    HILOGI("DisconnProcSetFlag_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    EXPECT_EQ(true, 0 == asc->DisconnProcSetFlag(device, NL_SLE_ASC_STARTED));
    EXPECT_EQ(false, asc->IsNeedDisconnect(device));
    HILOGI("DisconnProcSetFlag_001 end");
}

/**
 * @tc.name: DisconnProcStatusError_001
 * @tc.desc: DisconnProcStatusError
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, DisconnProcStatusError_001, TestSize.Level1)
{
    HILOGI("DisconnProcStatusError_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    EXPECT_EQ(true, NL_NO_ERROR == asc->DisconnProcStatusError(device, NL_SLE_ASC_ENABLING));
    EXPECT_EQ(false, asc->IsNeedDisconnect(device));
    HILOGI("DisconnProcStatusError_001 end");
}

/**
 * @tc.name: DisconnProcStopPlaying_001
 * @tc.desc: DisconnProcStopPlaying
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, DisconnProcStopPlaying_001, TestSize.Level1)
{
    HILOGI("DisconnProcStopPlaying_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    asc->SetASCStatus(device, NL_SLE_ASC_STARTED);
    asc->SetProcessingStreamType(device, AUDIO_STREAM_MUSIC);
    asc->SetStopDelayingFlag(device, true);
    EXPECT_EQ(false, asc->IsNeedDisconnect(device));
    EXPECT_EQ(true, NL_NO_ERROR == asc->DisconnProcStopPlaying(device, NL_SLE_ASC_STARTED));
    HILOGI("DisconnProcStopPlaying_001 end");
}

#if 0
/**
 * @tc.name: DisconnProcSetStatus_001
 * @tc.desc: DisconnProcSetStatus
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, DisconnProcSetStatus_001, TestSize.Level1)
{
    HILOGI("DisconnProcSetStatus_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    asc->SetProcessingStreamType(device, AUDIO_STREAM_MUSIC);
    EXPECT_EQ(true, NL_NO_ERROR == asc->DisconnProcSetStatus(device, NL_SLE_ASC_STARTED));
    EXPECT_EQ(true, NL_SLE_ASC_DISCONNECTED == asc->GetASCStatus(device));
    HILOGI("DisconnProcSetStatus_001 end");
}
#endif

class ASCCallbackImplTestStub : public InterfaceASCCallback {
public:
    explicit ASCCallbackImplTestStub()
    {
        HILOGI("[NearlinkASCServer]Enter");
    }

    ~ASCCallbackImplTestStub() override = default;

    void OnAudioControlComplete(const NearlinkRawAddress &device, const NearlinkASCAudioControlResult& result) override
    {
        HILOGI("NearlinkASCServer OnAudioControlComplete ret: %{public}d", result.GetResult());
    }

    void OnAddSleAudioDevice(const NearlinkRawAddress &device, uint32_t streamType, int32_t mediaVolume,
        int32_t callVolume) override
    {
        HILOGD("NearlinkASCServer OnAddSleAudioDevice %{public}s streamType %{public}d",
            GetEncryptAddr(device.GetAddress()).c_str(), streamType);
    }

    void OnDeleteSleAudioDevice(const NearlinkRawAddress &device) override
    {
        HILOGD("NearlinkASCServer OnDeleteSleAudioDevice");
    }

    void OnSleAudioDeviceActionChanged(const NearlinkRawAddress &device, const NearlinkASCAudioStreamInfo &streamInfo,
        int action) override
    {
        HILOGD("NearlinkASCServer OnSleAudioDeviceActionChanged");
    }

    void OnAddSleVirtualAudioDevice(const NearlinkRawAddress &device, AudioStreamType streamType) override
    {
        HILOGI("NearlinkASCServer OnAddSleVirtualAudioDevice");
    }

    void OnDeleteSleVirtualAudioDevice(const NearlinkRawAddress &device) override
    {
        HILOGI("NearlinkASCServer OnDeleteSleVirtualAudioDevice");
    }

private:
};

/**
 * @tc.name: ProcessUpdateVirtualDeviceEvent_001
 * @tc.desc: ProcessUpdateVirtualDeviceEvent
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, ProcessUpdateVirtualDeviceEvent_001, TestSize.Level1)
{
    HILOGI("ProcessUpdateVirtualDeviceEvent_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    ASCMessage event;
    event.dev_ = device.GetAddress();
    event.arg1M = UpdateVirtualDeviceCmd::NL_SLE_VIRTUAL_DEVICE_CMD_ADD;
    asc->RegisterApplication(std::make_shared<ASCCallbackImplTestStub>());
    asc->callback_ = std::make_shared<ASCCallbackImplTestStub>();
    asc->SetProcessingStreamType(device, AUDIO_STREAM_MUSIC);
    asc->ProcessUpdateVirtualDeviceEvent(event);
    EXPECT_EQ(false, AUDIO_STREAM_MUSIC == asc->GetProcessingStreamType(device));
    std::set<std::string> virtualAddrSet {};
    asc->GetAllVirtualAudioAddr(virtualAddrSet);
    EXPECT_EQ(true, 0 != virtualAddrSet.size());
    HILOGI("ProcessUpdateVirtualDeviceEvent_001 end");
}

/**
 * @tc.name: ProcessUpdateVirtualDeviceEvent_002
 * @tc.desc: ProcessUpdateVirtualDeviceEvent
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, ProcessUpdateVirtualDeviceEvent_002, TestSize.Level1)
{
    HILOGI("ProcessUpdateVirtualDeviceEvent_002 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    ASCMessage event;
    event.dev_ = device.GetAddress();
    event.arg1M = UpdateVirtualDeviceCmd::NL_SLE_VIRTUAL_DEVICE_CMD_DELETE;
    asc->RegisterApplication(std::make_shared<ASCCallbackImplTestStub>());
    asc->callback_ = std::make_shared<ASCCallbackImplTestStub>();
    asc->SetProcessingStreamType(device, AUDIO_STREAM_MUSIC);
    asc->ProcessUpdateVirtualDeviceEvent(event);
    EXPECT_EQ(false, AUDIO_STREAM_MUSIC == asc->GetProcessingStreamType(device));
    HILOGI("ProcessUpdateVirtualDeviceEvent_002 end");
}

/**
 * @tc.name: ProcessUpdateVirtualDeviceEvent_003
 * @tc.desc: ProcessUpdateVirtualDeviceEvent
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, ProcessUpdateVirtualDeviceEvent_003, TestSize.Level1)
{
    HILOGI("ProcessUpdateVirtualDeviceEvent_003 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    ASCMessage event;
    event.dev_ = device.GetAddress();
    event.arg1M = 0xFF;
    asc->RegisterApplication(std::make_shared<ASCCallbackImplTestStub>());
    asc->callback_ = std::make_shared<ASCCallbackImplTestStub>();
    asc->SetProcessingStreamType(device, AUDIO_STREAM_MUSIC);
    asc->ProcessUpdateVirtualDeviceEvent(event);
    EXPECT_EQ(false, AUDIO_STREAM_MUSIC == asc->GetProcessingStreamType(device));
    HILOGI("ProcessUpdateVirtualDeviceEvent_003 end");
}

/**
 * @tc.name: OnAddDeleteAudioDevice_001
 * @tc.desc: OnAddDeleteAudioDevice
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, OnAddDeleteAudioDevice_001, TestSize.Level1)
{
    HILOGI("OnAddDeleteAudioDevice_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    EXPECT_EQ(0, asc->GetConnectedCnt(device));
    EXPECT_EQ(false, asc->IsNeedReportAudioDevice(device, NL_SLE_ASC_CONN_CMD_CONN));
    EXPECT_EQ(true, asc->IsNeedReportAudioDevice(device, NL_SLE_ASC_CONN_CMD_DISCONN));
    asc->OnAddDeleteAudioDevice(device, NL_SLE_ASC_CONN_CMD_CONN);
    HILOGI("OnAddDeleteAudioDevice_001 end");
}

/**
 * @tc.name: OnAddDeleteAudioDevice_002
 * @tc.desc: OnAddDeleteAudioDevice
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, OnAddDeleteAudioDevice_002, TestSize.Level1)
{
    HILOGI("OnAddDeleteAudioDevice_002 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    EXPECT_EQ(false, asc->IsNeedReportAudioDevice(device, NL_SLE_ASC_CONN_CMD_CONN));
    EXPECT_EQ(true, asc->IsNeedReportAudioDevice(device, NL_SLE_ASC_CONN_CMD_DISCONN));
    asc->OnAddDeleteAudioDevice(device, NL_SLE_ASC_CONN_CMD_CONN);
    HILOGI("OnAddDeleteAudioDevice_002 end");
}

/**
 * @tc.name: OnAddDeleteAudioDevice_003
 * @tc.desc: OnAddDeleteAudioDevice
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, OnAddDeleteAudioDevice_003, TestSize.Level1)
{
    HILOGI("OnAddDeleteAudioDevice_003 enter");
    ASCService *asc = new ASCService();
    RawAddress reportAddr(deviceStr);
    RawAddress memberAddr(coDeviceStr);
    uint32_t testGroupId = 0x12345678;
    CdsmService *cdsmService = CdsmService::GetService();
    auto cdsmInfo = std::make_shared<CdsmInfo>(testGroupId, reportAddr.GetAddress());
    cdsmInfo->CdsmAddMemberInfo(memberAddr.GetAddress(), 1);
    cdsmInfo->CdsmAddMemberInfo(reportAddr.GetAddress(), 1);
    cdsmService->cdsmList_.EnsureInsert(testGroupId, cdsmInfo);
    asc->AddConnectDevices(reportAddr);
    EXPECT_EQ(true, asc->IsConnected(reportAddr));
    asc->SetASCStatus(reportAddr, NL_SLE_ASC_CONNECTED);
    asc->SetASCStatus(memberAddr, NL_SLE_ASC_CONNECTED);
    asc->AddConnectDevices(memberAddr);
    EXPECT_EQ(1, asc->GetConnectedCnt(memberAddr));
    asc->OnAddDeleteAudioDevice(reportAddr, NL_SLE_ASC_CONN_CMD_CONN);
    asc->callback_ = std::make_shared<ASCCallbackImplTestStub>();
    asc->OnAddDeleteAudioDevice(reportAddr, NL_SLE_ASC_CONN_CMD_CONN);
    HILOGI("OnAddDeleteAudioDevice_003 end");
}

/**
 * @tc.name: GetConnectedCnt_001
 * @tc.desc: GetConnectedCnt
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, GetConnectedCnt_001, TestSize.Level1)
{
    HILOGI("GetConnectedCnt_001 enter");
    ASCService *asc = new ASCService();
    RawAddress reportAddr(deviceStr);
    RawAddress memberAddr(coDeviceStr);
    uint32_t testGroupId = 0x12345678;
    CdsmService *cdsmService = CdsmService::GetService();
    auto cdsmInfo = std::make_shared<CdsmInfo>(testGroupId, reportAddr.GetAddress());
    cdsmInfo->CdsmAddMemberInfo(memberAddr.GetAddress(), 1);
    cdsmInfo->CdsmAddMemberInfo(reportAddr.GetAddress(), 1);
    cdsmService->cdsmList_.EnsureInsert(testGroupId, cdsmInfo);
    asc->AddConnectDevices(reportAddr);
    EXPECT_EQ(true, asc->IsConnected(reportAddr));
    asc->SetASCStatus(reportAddr, NL_SLE_ASC_CONNECTED);
    asc->SetASCStatus(memberAddr, NL_SLE_ASC_CONNECTED);
    asc->AddConnectDevices(memberAddr);
    asc->GetConnectedCnt(memberAddr);
    asc->SetASCStatus(reportAddr, NL_SLE_ASC_CREATING);
    asc->SetASCStatus(memberAddr, NL_SLE_ASC_CREATING);
    asc->GetConnectedCnt(memberAddr);
    asc->SetASCStatus(reportAddr, NL_SLE_ASC_CREATED);
    asc->SetASCStatus(memberAddr, NL_SLE_ASC_CREATED);
    asc->GetConnectedCnt(memberAddr);
    asc->SetASCStatus(reportAddr, NL_SLE_ASC_RELEASED);
    asc->SetASCStatus(memberAddr, NL_SLE_ASC_RELEASED);
    asc->GetConnectedCnt(memberAddr);
    asc->SetASCStatus(reportAddr, NL_SLE_ASC_STOPPED);
    asc->SetASCStatus(memberAddr, NL_SLE_ASC_STOPPED);
    asc->GetConnectedCnt(memberAddr);
    asc->SetASCStatus(reportAddr, NL_SLE_ASC_STARTED);
    asc->SetASCStatus(memberAddr, NL_SLE_ASC_STARTED);
    asc->GetConnectedCnt(memberAddr);
    asc->SetASCStatus(reportAddr, NL_SLE_ASC_STARTING);
    asc->SetASCStatus(memberAddr, NL_SLE_ASC_STARTING);
    asc->GetConnectedCnt(memberAddr);
    asc->SetASCStatus(reportAddr, NL_SLE_ASC_RELEASING);
    asc->SetASCStatus(memberAddr, NL_SLE_ASC_RELEASING);
    asc->GetConnectedCnt(memberAddr);
    HILOGI("GetConnectedCnt_001 end");
}

/**
 * @tc.name: OnConnectionStateChanged_001
 * @tc.desc: OnConnectionStateChanged
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, OnConnectionStateChanged_001, TestSize.Level1)
{
    HILOGI("OnConnectionStateChanged_001 enter");
    ASCService *asc = ASCService::GetService();
    asc->RegisterObserver(g_ascServiceObserver_);
    RawAddress device = RawAddress(deviceStr);
    asc->OnConnectionStateChanged(device, NL_SLE_ASC_CONN_CMD_DISCONN, NL_NO_ERROR, 0);
    HILOGI("OnConnectionStateChanged_001 end");
}

/**
 * @tc.name: OnConnectionStateChanged_002
 * @tc.desc: OnConnectionStateChanged
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, OnConnectionStateChanged_002, TestSize.Level1)
{
    HILOGI("OnConnectionStateChanged_002 enter");
    ASCService *asc = ASCService::GetService();
    asc->RegisterObserver(g_ascServiceObserver_);
    RawAddress device = RawAddress(deviceStr);
    asc->OnConnectionStateChanged(device, NL_SLE_ASC_CONN_CMD_DISCONN, NL_SLE_ASC_ERROR_JUDGE_RECFG_STATE_ERROR, 0);
    EXPECT_EQ(false, NL_SLE_ASC_DISCONNECTED == asc->GetASCStatus(device));
    HILOGI("OnConnectionStateChanged_002 end");
}

/**
 * @tc.name: ReportConnectStateChanged_001
 * @tc.desc: ReportConnectStateChanged
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, ReportConnectStateChanged_001, TestSize.Level1)
{
    HILOGI("ReportConnectStateChanged_001 enter");
    ASCService *asc = ASCService::GetService();
    asc->RegisterObserver(g_ascServiceObserver_);
    RawAddress device = RawAddress(deviceStr);
    asc->ReportConnectStateChanged(device, NL_SLE_ASC_CONN_CMD_DISCONN, NL_SLE_ASC_ERROR_JUDGE_RECFG_STATE_ERROR, 0);
    EXPECT_EQ(false, NL_SLE_ASC_DISCONNECTED == asc->GetASCStatus(device));
    HILOGI("ReportConnectStateChanged_001 end");
}

/**
 * @tc.name: GenerateCodecPara_001
 * @tc.desc: GenerateCodecPara
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, GenerateCodecPara_001, TestSize.Level1)
{
    HILOGI("GenerateCodecPara_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    AscQosmInfo info {};
    asc->SaveStreamQosmInfo(device, info);
    asc->SetQosmInfoUpdateFlag(device, true);
    EXPECT_EQ(true, "" != asc->GenerateCodecPara(device, AUDIO_STREAM_MUSIC));
    HILOGI("GenerateCodecPara_001 end");
}

/**
 * @tc.name: AddCoSetChannelInfo_001
 * @tc.desc: AddCoSetChannelInfo
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, AddCoSetChannelInfo_001, TestSize.Level1)
{
    HILOGI("AddCoSetChannelInfo_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    AscQosmInfo info {};
    asc->SaveStreamQosmInfo(device, info);
    asc->SetQosmInfoUpdateFlag(device, true);
    std::string channelInfoStr = "";
    asc->AddCoSetChannelInfo(device, channelInfoStr);
    EXPECT_EQ(true, channelInfoStr != "");
    HILOGI("AddCoSetChannelInfo_001 end");
}

/**
 * @tc.name: GenerateChannelInfo_001
 * @tc.desc: GenerateChannelInfo
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, GenerateChannelInfo_001, TestSize.Level1)
{
    HILOGI("GenerateChannelInfo_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    AscQosmInfo info {};
    asc->SaveStreamQosmInfo(device, info);
    asc->SetQosmInfoUpdateFlag(device, true);
    std::string channelInfoStr = "";
    EXPECT_EQ(true, "" != asc->GenerateChannelInfo(device, NL_SLE_ASC_CONTROL_CMD_START));
    RawAddress coDevice(coDeviceStr);
    asc->AddConnectDevices(coDevice);
    asc->SetASCStatus(device, NL_SLE_ASC_STARTED);
    asc->SetASCStatus(coDevice, NL_SLE_ASC_STARTED);
    asc->SetQosmInfoUpdateFlag(device, true);
    asc->SetQosmInfoUpdateFlag(coDevice, true);
    asc->GenerateChannelInfo(device, NL_SLE_ASC_CONTROL_CMD_START);
    HILOGI("GenerateChannelInfo_001 end");
}

/**
 * @tc.name: GenerateChannelInfo_002
 * @tc.desc: GenerateChannelInfo
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, GenerateChannelInfo_002, TestSize.Level1)
{
    HILOGI("GenerateChannelInfo_002 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    AscQosmInfo info {};
    asc->SaveStreamQosmInfo(device, info);
    asc->SetQosmInfoUpdateFlag(device, true);
    std::string channelInfoStr = "";
    EXPECT_EQ(true, "" != asc->GenerateChannelInfo(device, NL_SLE_ASC_CONTROL_CMD_STOP));
    HILOGI("GenerateChannelInfo_002 end");
}

/**
 * @tc.name: SetAudioDisconnInfo_001
 * @tc.desc: SetAudioDisconnInfo
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, SetAudioDisconnInfo_001, TestSize.Level1)
{
    HILOGI("SetAudioDisconnInfo_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    AscQosmInfo info {};
    asc->SaveStreamQosmInfo(device, info);
    asc->SetQosmInfoUpdateFlag(device, true);
    asc->SetAudioDisconnInfo(device, 123);
    HILOGI("SetAudioDisconnInfo_001 end");
}

/**
 * @tc.name: SetStartPlayingResult_001
 * @tc.desc: SetStartPlayingResult
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, SetStartPlayingResult_001, TestSize.Level1)
{
    HILOGI("SetStartPlayingResult_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    AscQosmInfo info {};
    asc->SaveStreamQosmInfo(device, info);
    asc->SetQosmInfoUpdateFlag(device, true);
    asc->SetStartPlayingResult(device, AUDIO_STREAM_MUSIC, true, 0);

    ASCService::StreamResult streamResult {};
    asc->GetStreamResult(device, AUDIO_STREAM_MUSIC, streamResult);
    EXPECT_EQ(true, true == streamResult.isStartFinished);
    EXPECT_EQ(true, 0 == streamResult.startPlayingResult);
    EXPECT_EQ(true, asc->IsStartPlayingSucc(streamResult));
    EXPECT_EQ(false, asc->IsStartPlayingFail(streamResult));
    EXPECT_EQ(false, asc->IsStartPlayingDoing(streamResult));
    RawAddress coSetDevice {};
    EXPECT_EQ(false, asc->IsNeedReportAudioControlFailed(NL_SLE_ASC_CONTROL_CMD_START, streamResult, coSetDevice,
        NL_SLE_ASC_STARTED));
    EXPECT_EQ(true, asc->IsNeedReportAudioControlComplete(device, NL_SLE_ASC_CONTROL_CMD_START, AUDIO_STREAM_MUSIC,
        NL_NO_ERROR));

    asc->ClearStreamResult(device);
    asc->GetStreamResult(device, AUDIO_STREAM_MUSIC, streamResult);
    EXPECT_EQ(false, false == streamResult.isStartFinished);
    EXPECT_EQ(true, asc->IsStartPlayingSucc(streamResult));
    EXPECT_EQ(false, asc->IsStartPlayingFail(streamResult));
    EXPECT_EQ(false, asc->IsNeedReportAudioControlFailed(NL_SLE_ASC_CONTROL_CMD_START, streamResult, coSetDevice,
        NL_SLE_ASC_STARTED));
    HILOGI("SetStartPlayingResult_001 end");
}

/**
 * @tc.name: SetStopPlayingResult_001
 * @tc.desc: SetStopPlayingResult
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, SetStopPlayingResult_001, TestSize.Level1)
{
    HILOGI("SetStopPlayingResult_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    AscQosmInfo info {};
    asc->SaveStreamQosmInfo(device, info);
    asc->SetQosmInfoUpdateFlag(device, true);
    asc->SetStopPlayingResult(device, AUDIO_STREAM_MUSIC, true, 0);

    ASCService::StreamResult streamResult {};
    asc->GetStreamResult(device, AUDIO_STREAM_MUSIC, streamResult);
    EXPECT_EQ(true, true == streamResult.isStopFinished);
    EXPECT_EQ(true, 0 == streamResult.startPlayingResult);
    EXPECT_EQ(true, asc->IsStopPlayingSucc(streamResult));
    EXPECT_EQ(false, asc->IsStopPlayingFail(streamResult));
    EXPECT_EQ(false, asc->IsStopPlayingDoing(streamResult));
    RawAddress coSetDevice {};
    EXPECT_EQ(false, asc->IsNeedReportAudioControlFailed(NL_SLE_ASC_CONTROL_CMD_STOP, streamResult, coSetDevice,
        NL_SLE_ASC_RELEASED));
    EXPECT_EQ(true, asc->IsNeedReportAudioControlComplete(device, NL_SLE_ASC_CONTROL_CMD_STOP, AUDIO_STREAM_MUSIC,
        NL_NO_ERROR));

    asc->ClearStreamResult(device);
    asc->GetStreamResult(device, AUDIO_STREAM_MUSIC, streamResult);
    EXPECT_EQ(false, false == streamResult.isStopFinished);
    EXPECT_EQ(true, asc->IsStopPlayingSucc(streamResult));
    EXPECT_EQ(false, asc->IsStopPlayingFail(streamResult));
    EXPECT_EQ(false, asc->IsNeedReportAudioControlFailed(NL_SLE_ASC_CONTROL_CMD_STOP, streamResult, coSetDevice,
        NL_SLE_ASC_RELEASED));
    HILOGI("SetStopPlayingResult_001 end");
}

/**
 * @tc.name: ReportAudioControlComplete_001
 * @tc.desc: ReportAudioControlComplete
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, ReportAudioControlComplete_001, TestSize.Level1)
{
    HILOGI("ReportAudioControlComplete_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    AscQosmInfo info {};
    asc->SaveStreamQosmInfo(device, info);
    asc->SetQosmInfoUpdateFlag(device, true);
    asc->SetStopDoingFlag(device, true);
    asc->SetSpatialConfiguringFlag(device, false);
    asc->ReportAudioControlComplete(device, AUDIO_STREAM_MUSIC, NL_SLE_ASC_CONTROL_CMD_START, NL_NO_ERROR, 0);

    ASCService::StreamResult streamResult {};
    asc->GetStreamResult(device, AUDIO_STREAM_MUSIC, streamResult);
    EXPECT_EQ(true, 0 == streamResult.startPlayingResult);
    EXPECT_EQ(true, asc->IsStartPlayingSucc(streamResult));
    EXPECT_EQ(false, asc->IsStartPlayingFail(streamResult));
    HILOGI("ReportAudioControlComplete_001 end");
}

/**
 * @tc.name: ReportAudioControlComplete_002
 * @tc.desc: ReportAudioControlComplete
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, ReportAudioControlComplete_002, TestSize.Level1)
{
    HILOGI("ReportAudioControlComplete_002 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    AscQosmInfo info {};
    asc->SaveStreamQosmInfo(device, info);
    asc->SetQosmInfoUpdateFlag(device, true);
    asc->SetStopDoingFlag(device, true);
    asc->SetSpatialConfiguringFlag(device, false);
    asc->ReportAudioControlComplete(device, AUDIO_STREAM_MUSIC, NL_SLE_ASC_CONTROL_CMD_STOP, NL_NO_ERROR, 0);

    ASCService::StreamResult streamResult {};
    asc->GetStreamResult(device, AUDIO_STREAM_MUSIC, streamResult);
    EXPECT_EQ(true, 0 == streamResult.stopPlayingResult);
    EXPECT_EQ(true, asc->IsStopPlayingSucc(streamResult));
    EXPECT_EQ(false, asc->IsStopPlayingFail(streamResult));
    HILOGI("ReportAudioControlComplete_002 end");
}

/**
 * @tc.name: ReportAudioControlComplete_003
 * @tc.desc: ReportAudioControlComplete
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, ReportAudioControlComplete_003, TestSize.Level1)
{
    HILOGI("ReportAudioControlComplete_003 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    AscQosmInfo info {};
    asc->SaveStreamQosmInfo(device, info);
    asc->SetQosmInfoUpdateFlag(device, true);
    asc->SetStopDoingFlag(device, true);
    asc->SetSpatialConfiguringFlag(device, false);
    asc->ReportAudioControlComplete(device, AUDIO_STREAM_MUSIC, NL_SLE_ASC_CONTROL_CMD_START, 12, 0xC);

    ASCService::StreamResult streamResult {};
    asc->GetStreamResult(device, AUDIO_STREAM_MUSIC, streamResult);
    EXPECT_EQ(false, 0 == streamResult.startPlayingResult);
    EXPECT_EQ(false, asc->IsStartPlayingSucc(streamResult));
    EXPECT_EQ(true, asc->IsStartPlayingFail(streamResult));
    HILOGI("ReportAudioControlComplete_003 end");
}

/**
 * @tc.name: ReportAudioControlComplete_004
 * @tc.desc: ReportAudioControlComplete
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, ReportAudioControlComplete_004, TestSize.Level1)
{
    HILOGI("ReportAudioControlComplete_004 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    AscQosmInfo info {};
    asc->SaveStreamQosmInfo(device, info);
    asc->SetQosmInfoUpdateFlag(device, true);
    asc->SetStopDoingFlag(device, true);
    asc->SetSpatialConfiguringFlag(device, false);
    asc->ReportAudioControlComplete(device, AUDIO_STREAM_MUSIC, NL_SLE_ASC_CONTROL_CMD_STOP, 12, 0xC);

    ASCService::StreamResult streamResult {};
    asc->GetStreamResult(device, AUDIO_STREAM_MUSIC, streamResult);
    EXPECT_EQ(false, 0 == streamResult.stopPlayingResult);
    EXPECT_EQ(false, asc->IsStopPlayingSucc(streamResult));
    EXPECT_EQ(true, asc->IsStopPlayingFail(streamResult));
    HILOGI("ReportAudioControlComplete_004 end");
}

/**
 * @tc.name: GetSupportBps_001
 * @tc.desc: GetSupportBps
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, GetSupportBps_001, TestSize.Level1)
{
    HILOGI("GetSupportBps_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = 2;
    prop.ability.codec[0].param.l2hcParam.bps = 1510;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    uint64_t bps = asc->GetSupportBps(device, NLSTK_ACTM_SOURCE_POINT, 2);

    EXPECT_EQ(1510, bps);
    HILOGI("GetSupportBps_001 end");
}

/**
 * @tc.name: GetSupportBps_002
 * @tc.desc: GetSupportBps
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, GetSupportBps_002, TestSize.Level1)
{
    HILOGI("GetSupportBps_002 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = 2;
    prop.ability.codec[0].param.l2hcParam.bps = 1510;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    uint64_t bps = asc->GetSupportBps(device, NLSTK_ACTM_SOURCE_POINT, 3);

    EXPECT_EQ(0, bps);
    HILOGI("GetSupportBps_002 end");
}

/**
 * @tc.name: IsVidoeExist_001
 * @tc.desc: IsVidoeExist
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, IsVidoeExist_001, TestSize.Level1)
{
    HILOGI("IsVidoeExist_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    EXPECT_EQ(true, AUDIO_STREAM_NONE == asc->GetStopStreamType(device));
    std::list<AudioStreamType>& startedStreamList = asc->GetStartedStreamList(device);
    EXPECT_EQ(true, asc->IsVidoeExist(device, AUDIO_STREAM_VIDEO));
    startedStreamList.emplace_back(AUDIO_STREAM_MUSIC);
    EXPECT_EQ(false, asc->IsVidoeExist(device, AUDIO_STREAM_MUSIC));
    EXPECT_EQ(true, asc->IsStreamExists(device, AUDIO_STREAM_MUSIC));
    EXPECT_EQ(false, asc->IsStreamExists(device, AUDIO_STREAM_VIDEO));
    startedStreamList.emplace_back(AUDIO_STREAM_VIDEO);
    EXPECT_EQ(true, asc->IsVidoeExist(device, AUDIO_STREAM_MUSIC));
    EXPECT_EQ(true, AUDIO_STREAM_MUSIC == asc->GetStopStreamType(device));
    asc->RemoveStartedStream(device, AUDIO_STREAM_MUSIC);
    asc->RemoveStartedStream(device, AUDIO_STREAM_VOIP);
    HILOGI("IsVidoeExist_001 end");
}

/**
 * @tc.name: ChooseBps_001
 * @tc.desc: ChooseBps
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, ChooseBps_001, TestSize.Level1)
{
    HILOGI("ChooseBps_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = 2;
    prop.ability.codec[0].param.l2hcParam.bps = 1510;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    uint64_t bpsRange = 0;
    ASCService::JudgeL2HCParamStru paramIn {};
    paramIn.pointType = NLSTK_ACTM_SOURCE_POINT;
    paramIn.codecId = 2;
    paramIn.qos = NL_SLE_QOS_1;
    EXPECT_EQ(true, 0 != asc->ChooseBps(device, paramIn, bpsRange));

    paramIn.qos = NL_SLE_QOS_2;
    EXPECT_EQ(true, 0 != asc->ChooseBps(device, paramIn, bpsRange));

    paramIn.qos = NL_SLE_QOS_3;
    EXPECT_EQ(true, 0 != asc->ChooseBps(device, paramIn, bpsRange));

    paramIn.qos = NL_SLE_QOS_4;
    EXPECT_EQ(true, 0 != asc->ChooseBps(device, paramIn, bpsRange));

    paramIn.qos = NL_SLE_QOS_5;
    EXPECT_EQ(true, 0 != asc->ChooseBps(device, paramIn, bpsRange));

    paramIn.qos = NL_SLE_QOS_6;
    EXPECT_EQ(true, 0 != asc->ChooseBps(device, paramIn, bpsRange));

    paramIn.qos = NL_SLE_QOS_7;
    EXPECT_EQ(true, 0 != asc->ChooseBps(device, paramIn, bpsRange));

    paramIn.qos = NL_SLE_QOS_8;
    EXPECT_EQ(true, 0 != asc->ChooseBps(device, paramIn, bpsRange));
    HILOGI("ChooseBps_001 end");
}

/**
 * @tc.name: SelectPeerCodecIdInMedia_001
 * @tc.desc: SelectPeerCodecIdInMedia
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, SelectPeerCodecIdInMedia_001, TestSize.Level1)
{
    HILOGI("SelectPeerCodecIdInMedia_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 0;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId ;
    prop.ability.codec[0].companyId = ASC_L2HC_5_0_CODEC.companyId;
    prop.ability.codec[0].vendorId = ASC_L2HC_5_0_CODEC.vendorId;
    prop.ability.codec[0].param.l2hcParam.version = ASC_L2HC_5_0_CODEC.version;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);

    AscCodecIdKey codec;
    // 通信方式, 0x0: 单播, 0x1: 数据组播
    codec = asc->SelectPeerCodecIdInMedia(device, 0);
    EXPECT_EQ(false, ASC_L2HC_5_0_CODEC.codecId == codec.codecId);
    EXPECT_EQ(false, ASC_L2HC_5_0_CODEC.companyId == codec.companyId);
    EXPECT_EQ(false, ASC_L2HC_5_0_CODEC.vendorId == codec.vendorId);
    HILOGI("SelectPeerCodecId_001 end");
}

/**
 * @tc.name: SelectPeerCodecId_001
 * @tc.desc: SelectPeerCodecId
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, SelectPeerCodecId_001, TestSize.Level1)
{
    HILOGI("SelectPeerCodecId_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_VOICE_CODEC.codecId;
    prop.ability.codec[0].companyId = ASC_L2HC_VOICE_CODEC.companyId;
    prop.ability.codec[0].vendorId = ASC_L2HC_VOICE_CODEC.vendorId;
    prop.ability.codec[0].param.l2hcParam.version = ASC_L2HC_VOICE_CODEC.version;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    Qos qos = NL_SLE_QOS_3;

    AscCodecIdKey codec;
    // 通信方式, 0x0: 单播, 0x1: 数据组播
    codec = asc->SelectPeerCodecId(device, qos, 1);
    EXPECT_EQ(true, ASC_L2HC_VOICE_CODEC.codecId == codec.codecId);
    EXPECT_EQ(true, ASC_L2HC_VOICE_CODEC.companyId == codec.companyId);
    EXPECT_EQ(true, ASC_L2HC_VOICE_CODEC.vendorId == codec.vendorId);
    HILOGI("SelectPeerCodecId_001 end");
}

/**
 * @tc.name: SelectPeerCodecId_002
 * @tc.desc: SelectPeerCodecId
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, SelectPeerCodecId_002, TestSize.Level1)
{
    HILOGI("SelectPeerCodecId_002 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = ASC_L2HC_5_0_CODEC.companyId;
    prop.ability.codec[0].vendorId = ASC_L2HC_5_0_CODEC.vendorId;
    prop.ability.codec[0].param.l2hcParam.version = ASC_L2HC_5_0_CODEC.version;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    Qos qos = NL_SLE_QOS_1;

    AscCodecIdKey codec;
    // 通信方式, 0x0: 单播, 0x1: 数据组播
    codec = asc->SelectPeerCodecId(device, qos, 0);
    EXPECT_EQ(true, ASC_L2HC_5_0_CODEC.codecId == codec.codecId);
    EXPECT_EQ(true, ASC_L2HC_5_0_CODEC.companyId == codec.companyId);
    EXPECT_EQ(true, ASC_L2HC_5_0_CODEC.vendorId == codec.vendorId);
    HILOGI("SelectPeerCodecId_002 end");
}

/**
 * @tc.name: SelectPeerCodecId_003
 * @tc.desc: SelectPeerCodecId
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, SelectPeerCodecId_03, TestSize.Level1)
{
    HILOGI("SelectPeerCodecId_003 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_CODEC.codecId;
    prop.ability.codec[0].companyId = ASC_L2HC_CODEC.companyId;
    prop.ability.codec[0].vendorId = ASC_L2HC_CODEC.vendorId;
    prop.ability.codec[0].param.l2hcParam.version = ASC_L2HC_CODEC.version;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    Qos qos = NL_SLE_QOS_8;

    AscCodecIdKey codec;
    // 通信方式, 0x0: 单播, 0x1: 数据组播
    codec = asc->SelectPeerCodecId(device, qos, 0);
    EXPECT_EQ(true, ASC_L2HC_CODEC.codecId == codec.codecId);
    EXPECT_EQ(true, ASC_L2HC_CODEC.companyId == codec.companyId);
    EXPECT_EQ(true, ASC_L2HC_CODEC.vendorId == codec.vendorId);
    HILOGI("SelectPeerCodecId_003 end");
}

/**
 * @tc.name: ConfigStreamToActm_001
 * @tc.desc: ConfigStreamToActm
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, ConfigStreamToActm_001, TestSize.Level1)
{
    HILOGI("ConfigStreamToActm_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().SetCos(device, NL_SLE_QOS_3);
    asc->ConfigStreamToActm(device, AUDIO_STREAM_VOIP);
    QosM::GetInstance().SetCos(device, NL_SLE_QOS_1);
    asc->ConfigStreamToActm(device, AUDIO_STREAM_MUSIC);
    QosM::GetInstance().SetCos(device, NL_SLE_QOS_5);
    asc->ConfigStreamToActm(device, AUDIO_STREAM_MUSIC);
    QosM::GetInstance().SetCos(device, NL_SLE_QOS_NONE);
    asc->ConfigStreamToActm(device, AUDIO_STREAM_MUSIC);
    QosM::GetInstance().SetCos(device, NL_SLE_QOS_BUTT);
    asc->ConfigStreamToActm(device, AUDIO_STREAM_MUSIC);


    HILOGI("ConfigStreamToActm_001 end");
}


/**
 * @tc.name: ConfigStream_001
 * @tc.desc: ConfigStream
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, ConfigStream_001, TestSize.Level1)
{
    HILOGI("ConfigStream_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_BUTT);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    asc->ConfigStream(device, AUDIO_STREAM_MUSIC, NL_SLE_ASC_CONFIGURING);
    EXPECT_EQ(true, AUDIO_STREAM_MUSIC == asc->GetProcessingStreamType(device));
    EXPECT_EQ(true, NL_SLE_QOS_1 == asc->GetStreamQos(device, AUDIO_STREAM_MUSIC));
    HILOGI("ConfigStream_001 end");
}

/**
 * @tc.name: JudgeReConfig_001
 * @tc.desc: JudgeReConfig
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, JudgeReConfig_001, TestSize.Level1)
{
    HILOGI("JudgeReConfig_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    std::list<AudioStreamType>& startedStreamList = asc->GetStartedStreamList(device);
    startedStreamList.emplace_back(AUDIO_STREAM_VIDEO);

    bool isGoOn = false;
    asc->JudgeReConfig(device, AUDIO_STREAM_MUSIC, NL_SLE_ASC_STARTED, isGoOn);
    EXPECT_EQ(false, AUDIO_STREAM_MUSIC == asc->GetProcessingStreamType(device));
    HILOGI("JudgeReConfig_001 end");
}

/**
 * @tc.name: JudgeReConfig_002
 * @tc.desc: JudgeReConfig
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, JudgeReConfig_002, TestSize.Level1)
{
    HILOGI("JudgeReConfig_002 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    std::list<AudioStreamType>& startedStreamList = asc->GetStartedStreamList(device);
    startedStreamList.emplace_back(AUDIO_STREAM_VIDEO);

    bool isGoOn = false;
    asc->JudgeReConfig(device, AUDIO_STREAM_MUSIC, NL_SLE_ASC_CONFIGED, isGoOn);
    EXPECT_EQ(false, AUDIO_STREAM_MUSIC == asc->GetProcessingStreamType(device));
    HILOGI("JudgeReConfig_002 end");
}

/**
 * @tc.name: JudgeReConfig_003
 * @tc.desc: JudgeReConfig
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, JudgeReConfig_003, TestSize.Level1)
{
    HILOGI("JudgeReConfig_003 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    std::list<AudioStreamType>& startedStreamList = asc->GetStartedStreamList(device);
    startedStreamList.emplace_back(AUDIO_STREAM_VIDEO);

    bool isGoOn = false;
    asc->JudgeReConfig(device, AUDIO_STREAM_MUSIC, NL_SLE_ASC_STARTING, isGoOn);
    HILOGI("JudgeReConfig_003 end");
}

/**
 * @tc.name: JudgeReConfig_004
 * @tc.desc: JudgeReConfig
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, JudgeReConfig_004, TestSize.Level1)
{
    HILOGI("JudgeReConfig_004 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);
    std::list<AudioStreamType>& startedStreamList = asc->GetStartedStreamList(device);
    startedStreamList.emplace_back(AUDIO_STREAM_VIDEO);
    asc->PrintStartedStreamList(device);

    bool isGoOn = false;
    asc->JudgeReConfig(device, AUDIO_STREAM_MUSIC, NL_SLE_ASC_STARTED, isGoOn);
    HILOGI("JudgeReConfig_004 end");
}

/**
 * @tc.name: JudgeReConfig_005
 * @tc.desc: JudgeReConfig
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, JudgeReConfig_005, TestSize.Level1)
{
    HILOGI("JudgeReConfig_005 start");
    ASCService *asc = new ASCService();
    RawAddress device(deviceStr);
    asc->AddConnectDevices(device);
    AudioStreamType streamType = static_cast<AudioStreamType>(AUDIO_STREAM_GAME);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_4);
    std::list<AudioStreamType>& startedStreamList = asc->GetStartedStreamList(device);
    startedStreamList.emplace_back(AUDIO_STREAM_VIDEO);
    ASCState state = NL_SLE_ASC_STARTED;
    bool isGoOn = false;
    asc->JudgeReConfig(device, streamType, state, isGoOn);
    state = NL_SLE_ASC_CONFIGED;
    asc->JudgeReConfig(device, streamType, state, isGoOn);
    state = NL_SLE_ASC_OPENED;
    asc->JudgeReConfig(device, streamType, state, isGoOn);
    state = NL_SLE_ASC_DISCONNECTED;
    asc->JudgeReConfig(device, streamType, state, isGoOn);
    HILOGI("JudgeReConfig_005 end");
}

/**
 * @tc.name: StartPlaying_001
 * @tc.desc: StartPlaying
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, StartPlaying_001, TestSize.Level1)
{
    HILOGI("StartPlaying_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);
    std::list<AudioStreamType>& startedStreamList = asc->GetStartedStreamList(device);
    startedStreamList.emplace_back(AUDIO_STREAM_VIDEO);

    bool isGoOn = false;
    AudioStreamType streamType = static_cast<AudioStreamType>(AUDIO_STREAM_GAME);
    AudioStreamType stopDelayingStreamType = static_cast<AudioStreamType>(AUDIO_STREAM_GAME);
    RawAddress reportAddr = asc->GetReportAddr(device);
    asc->SetStopDelayingFlag(reportAddr, true);
    asc->SetASCStatus(device, NL_SLE_ASC_STARTING);
    asc->StartPlaying(device, AUDIO_STREAM_MUSIC, false);

    asc->SetASCStatus(device, NL_SLE_ASC_CREATING);
    asc->StartPlaying(device, AUDIO_STREAM_MUSIC, false);
    asc->StartPlayingExcute(device, streamType, true, stopDelayingStreamType);

    asc->SetASCStatus(device, NL_SLE_ASC_STARTED);
    asc->StartPlaying(device, AUDIO_STREAM_MUSIC, false);
    asc->StartPlayingExcute(device, streamType, true, stopDelayingStreamType);

    asc->SetASCStatus(device, NL_SLE_ASC_CREATED);
    asc->StartPlaying(device, AUDIO_STREAM_MUSIC, false);
    asc->StartPlayingExcute(device, streamType, true, stopDelayingStreamType);

    asc->SetASCStatus(device, NL_SLE_ASC_ENABLED);
    asc->StartPlaying(device, AUDIO_STREAM_MUSIC, false);
    asc->StartPlayingExcute(device, streamType, true, stopDelayingStreamType);
    HILOGI("StartPlaying_001 end");
}


/**
 * @tc.name: OpenStream_001
 * @tc.desc: OpenStream
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, OpenStream_001, TestSize.Level1)
{
    HILOGI("OpenStream_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    asc->OpenStream(device, AUDIO_STREAM_MUSIC);
    EXPECT_EQ(true, AUDIO_STREAM_MUSIC == asc->GetProcessingStreamType(device));
    HILOGI("OpenStream_001 end");
}

/**
 * @tc.name: StartStream_001
 * @tc.desc: StartStream
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, StartStream_001, TestSize.Level1)
{
    HILOGI("StartStream_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    asc->StartStream(device, AUDIO_STREAM_MUSIC);
    EXPECT_EQ(true, AUDIO_STREAM_MUSIC == asc->GetProcessingStreamType(device));
    EXPECT_EQ(false, AUDIO_STREAM_MUSIC == asc->GetReconfigStream(device));
    HILOGI("StartStream_001 end");
}

/**
 * @tc.name: StopStream_001
 * @tc.desc: StopStream
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, StopStream_001, TestSize.Level1)
{
    HILOGI("StopStream_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    asc->StopStream(device, AUDIO_STREAM_MUSIC, NL_SLE_QOS_1, NL_SLE_ASC_STOPPING);
    EXPECT_EQ(true, AUDIO_STREAM_MUSIC == asc->GetProcessingStreamType(device));
    EXPECT_EQ(false, AUDIO_STREAM_MUSIC == asc->GetReconfigStream(device));
    HILOGI("StopStream_001 end");
}

/**
 * @tc.name: StopPlaying_001
 * @tc.desc: StopPlaying
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, StopPlaying_001, TestSize.Level1)
{
    HILOGI("StopPlaying_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    asc->SetASCStatus(device, NL_SLE_ASC_OPENED);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    asc->StopPlaying(device, AUDIO_STREAM_MUSIC);
    EXPECT_EQ(true, AUDIO_STREAM_MUSIC == asc->GetProcessingStreamType(device));
    EXPECT_EQ(false, AUDIO_STREAM_MUSIC == asc->GetReconfigStream(device));
    HILOGI("StopPlaying_001 end");
}

/**
 * @tc.name: StopPlaying_002
 * @tc.desc: StopPlaying
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, StopPlaying_002, TestSize.Level1)
{
    HILOGI("StopPlaying_002 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    asc->SetASCStatus(device, NL_SLE_ASC_CREATED);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    asc->StopPlaying(device, AUDIO_STREAM_MUSIC);
    EXPECT_EQ(true, AUDIO_STREAM_MUSIC == asc->GetProcessingStreamType(device));

    asc->SetStopDelayingFlag(device, true);
    EXPECT_EQ(true, asc->IsStopDelaying(device));

    asc->SetStopDoingFlag(device, true);
    EXPECT_EQ(true, asc->IsStopDoing(device));

    asc->SetSpatialConfiguringFlag(device, true);
    EXPECT_EQ(true, asc->IsSpatialConfiguring(device));

    asc->SetAutoRateBps(device, 123);
    uint16_t autoRateBps;
    EXPECT_EQ(true, asc->GetAutoRateBps(device, autoRateBps));
    EXPECT_EQ(123, autoRateBps);

    asc->SetProcessingStreamType(device, AUDIO_STREAM_MUSIC);
    EXPECT_EQ(true, AUDIO_STREAM_MUSIC == asc->GetProcessingStreamType(device));

    asc->SetBpsRange(device, 12345);
    EXPECT_EQ(true, 12345 == asc->GetBpsRange(device));

    asc->SetASCStatus(device, NL_SLE_ASC_CREATED);
    EXPECT_EQ(true, NL_SLE_ASC_CREATED == asc->GetASCStatus(device));

    asc->ASCToDftCacheTime(device, NL_SLE_ASC_CREATED);
    AscStreamInfo streamInfo {};
    streamInfo.pointType = NLSTK_ACTM_SINK_POINT;
    asc->SaveStreamInfo(device, streamInfo);
    streamInfo.pointType = NLSTK_ACTM_SOURCE_POINT;
    asc->SaveStreamInfo(device, streamInfo);
    asc->GetStreamInfo(device, NLSTK_ACTM_SINK_POINT);
    asc->GetStreamInfo(device, NLSTK_ACTM_SOURCE_POINT);
    EXPECT_EQ(true, asc->IsStreamIdValid(device, NLSTK_ACTM_SINK_POINT));
    EXPECT_EQ(true, asc->IsStreamIdValid(device, NLSTK_ACTM_SOURCE_POINT));

    asc->SetNeedDisconnect(device, true);
    EXPECT_EQ(true, asc->IsNeedDisconnect(device));

    asc->SetSyncFlag(device, true);
    EXPECT_EQ(true, asc->IsSync(device));

    asc->SetConnRptDelayingFlag(device, true);
    EXPECT_EQ(true, asc->IsConnRptDelaying(device));
    HILOGI("StopPlaying_002 end");
}

/**
 * @tc.name: ReleaseStream_001
 * @tc.desc: ReleaseStream
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, ReleaseStream_001, TestSize.Level1)
{
    HILOGI("ReleaseStream_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    RawAddress device2 = RawAddress(coDeviceStr);
    asc->AddConnectDevices(device2);
    EXPECT_EQ(true, asc->IsConnectedMemberExist(device));
    asc->SetASCStatus(device, NL_SLE_ASC_STARTED);
    asc->SetASCStatus(device2, NL_SLE_ASC_STARTED);
    EXPECT_EQ(true, asc->IsStartMemberExist(device));
    EXPECT_EQ(true, asc->IsConnected(device));
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    asc->ReleaseStream(device, AUDIO_STREAM_MUSIC, NL_SLE_QOS_1, NL_SLE_ASC_STOPPING);
    EXPECT_EQ(true, AUDIO_STREAM_MUSIC == asc->GetProcessingStreamType(device));
    EXPECT_EQ(false, AUDIO_STREAM_MUSIC == asc->GetReconfigStream(device));
    HILOGI("ReleaseStream_001 end");
}

/**
 * @tc.name: DeviceReprotDelayCheck_001
 * @tc.desc: DeviceReprotDelayCheck
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, DeviceReprotDelayCheck_001, TestSize.Level1)
{
    HILOGI("DeviceReprotDelayCheck_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);
    std::list<AudioStreamType>& startedStreamList = asc->GetStartedStreamList(device);
    startedStreamList.emplace_back(AUDIO_STREAM_VIDEO);

    asc->DoDeviceConnectedReprot(device);
    asc->DeviceReprotDelayCheck(device);
    RawAddress coSetDevice;
    asc->SyncWithCoSetDevice(device, coSetDevice, AUDIO_STREAM_MUSIC);
    asc->ChangeBpsRange();
    asc->RegisterListener();
    EXPECT_EQ(false, AUDIO_STREAM_MUSIC == asc->GetProcessingStreamType(device));
    EXPECT_EQ(false, AUDIO_STREAM_MUSIC == asc->GetReconfigStream(device));
    HILOGI("DeviceReprotDelayCheck_001 end");
}

/**
 * @tc.name: DeviceReprotDelayCheck_002
 * @tc.desc: DeviceReprotDelayCheck
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, DeviceReprotDelayCheck_002, TestSize.Level1)
{
    HILOGI("DeviceReprotDelayCheck_002 start");
    ASCService *asc = new ASCService();
    RawAddress device(deviceStr);
    RawAddress coDevice(coDeviceStr);
    asc->AddConnectDevices(device);
    asc->AddConnectDevices(coDevice);
    asc->SetASCStatus(device, NL_SLE_ASC_STARTED);
    asc->activeSinkDevice_ = device;
    asc->DeviceReprotDelayCheck(device);
    asc->SetConnRptDelayingFlag(coDevice, true);
    asc->DeviceReprotDelayCheck(device);
    asc->SetConnRptDelayingFlag(coDevice, false);
    asc->DeviceReprotDelayCheck(device);
    HILOGI("DeviceReprotDelayCheck_002 end");
}

/**
 * @tc.name: CbkGetProperty_001
 * @tc.desc: CbkGetProperty
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, CbkGetProperty_001, TestSize.Level1)
{
    HILOGI("CbkGetProperty_001 enter");
    ASCService *asc = ASCService::GetService();
    asc->RegisterObserver(g_ascServiceObserver_);
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    asc->SetASCStatus(device, NL_SLE_ASC_CONNECTING);
    asc->CbkGetProperty(device, properties);
    EXPECT_EQ(false, AUDIO_STREAM_MUSIC == asc->GetProcessingStreamType(device));
    EXPECT_EQ(false, AUDIO_STREAM_MUSIC == asc->GetReconfigStream(device));
    HILOGI("CbkGetProperty_001 end");
}

/**
 * @tc.name: CbkGetProperty_002
 * @tc.desc: CbkGetProperty
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, CbkGetProperty_002, TestSize.Level1)
{
    HILOGI("CbkGetProperty_002 enter");
    ASCService *asc = ASCService::GetService();
    asc->RegisterObserver(g_ascServiceObserver_);
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    asc->SetASCStatus(device, NL_SLE_ASC_CONNECTED);
    asc->CbkGetProperty(device, properties);
    EXPECT_EQ(false, AUDIO_STREAM_MUSIC == asc->GetProcessingStreamType(device));
    EXPECT_EQ(false, AUDIO_STREAM_MUSIC == asc->GetReconfigStream(device));
    HILOGI("CbkGetProperty_002 end");
}

/**
 * @tc.name: CreateStream_001
 * @tc.desc: CreateStream
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, CreateStream_001, TestSize.Level1)
{
    HILOGI("CreateStream_001 enter");
    ASCService *asc = ASCService::GetService();
    asc->RegisterObserver(g_ascServiceObserver_);
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    asc->CreateStream(device);
    EXPECT_EQ(true, NL_SLE_ASC_CREATING == asc->GetASCStatus(device));
    HILOGI("CreateStream_001 end");
}

/**
 * @tc.name: ProcStartBuff_001
 * @tc.desc: ProcStartBuff
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, ProcStartBuff_001, TestSize.Level1)
{
    HILOGI("ProcStartBuff_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device(deviceStr);
    RawAddress coDevice(coDeviceStr);
    asc->AddConnectDevices(device);
    asc->AddConnectDevices(coDevice);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    std::queue<AudioStreamType>& startBuff = asc->GetStartBuff(device);
    startBuff.push(AUDIO_STREAM_MUSIC);
    bool isGoOn = false;
    asc->ProcStartBuff(device, NL_SLE_ASC_STARTED, isGoOn);
    asc->ProcAllMemberStartBuff(device);
    asc->SetStopDelayingFlag(device, true);
    asc->SetStopDelayingFlag(coDevice, true);
    asc->SetASCStatus(device, NL_SLE_ASC_STARTED);
    asc->SetASCStatus(coDevice, NL_SLE_ASC_STARTED);
    asc->CheckAndSetStopDelayingFlag(device);
    asc->SetStopDelayingFlag(device, true);
    asc->SetStopDelayingFlag(coDevice, true);
    asc->SetASCStatus(device, NL_SLE_ASC_RELEASED);
    asc->SetASCStatus(coDevice, NL_SLE_ASC_RELEASED);
    asc->CheckAndSetStopDelayingFlag(device);
    HILOGI("ProcStartBuff_001 end");
}

/**
 * @tc.name: ProcStartBuff_002
 * @tc.desc: ProcStartBuff
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, ProcStartBuff_002, TestSize.Level1)
{
    HILOGI("ProcStartBuff_002 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    std::queue<AudioStreamType>& startBuff = asc->GetStartBuff(device);
    startBuff.push(AUDIO_STREAM_MUSIC);
    bool isGoOn = false;
    asc->ProcBuff(device, NL_SLE_ASC_RELEASED);
    asc->ProcStartBuff(device, NL_SLE_ASC_RELEASED, isGoOn);
    EXPECT_EQ(false, NL_SLE_ASC_STARTED == asc->GetASCStatus(device));
    HILOGI("ProcStartBuff_002 end");
}

/**
 * @tc.name: ProcStopBuff_001
 * @tc.desc: ProcStopBuff
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, ProcStopBuff_001, TestSize.Level1)
{
    HILOGI("ProcStopBuff_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    std::queue<AudioStreamType>& stopBuff = asc->GetStopBuff(device);
    stopBuff.push(AUDIO_STREAM_MUSIC);
    asc->ProcStopBuff(device, NL_SLE_ASC_STARTED);
    EXPECT_EQ(false, NL_SLE_ASC_STARTED == asc->GetASCStatus(device));
    HILOGI("ProcStopBuff_001 end");
}

/**
 * @tc.name: ProcStopBuff_002
 * @tc.desc: ProcStopBuff
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, ProcStopBuff_002, TestSize.Level1)
{
    HILOGI("ProcStopBuff_002 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    std::queue<AudioStreamType>& stopBuff = asc->GetStopBuff(device);
    stopBuff.push(AUDIO_STREAM_MUSIC);
    asc->ProcStopBuff(device, NL_SLE_ASC_RELEASED);
    EXPECT_EQ(false, NL_SLE_ASC_STARTED == asc->GetASCStatus(device));
    HILOGI("ProcStopBuff_002 end");
}

/**
 * @tc.name: CbkCreateStream_001
 * @tc.desc: CbkCreateStream
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, CbkCreateStream_001, TestSize.Level1)
{
    HILOGI("CbkCreateStream_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    AscStreamInfo streamInfo {};
    streamInfo.streamId = 1;
    streamInfo.pointType = NLSTK_ACTM_SINK_POINT;
    asc->CbkCreateStream(device, 0, streamInfo);
    streamInfo.streamId = 2;
    streamInfo.pointType = NLSTK_ACTM_ALL_POINT;
    asc->CbkCreateStream(device, 0, streamInfo);
    EXPECT_EQ(false, NL_SLE_ASC_CREATED == asc->GetASCStatus(device));
    HILOGI("CbkCreateStream_001 end");
}

/**
 * @tc.name: CbkConfigStream_001
 * @tc.desc: CbkConfigStream
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, CbkConfigStream_001, TestSize.Level1)
{
    HILOGI("CbkConfigStream_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    asc->SetASCStatus(device, NL_SLE_ASC_CONFIG_SUBRATE_CHANGED);
    asc->CbkConfigStream(device, 0);
    EXPECT_EQ(false, NL_SLE_ASC_CREATED == asc->GetASCStatus(device));
    HILOGI("CbkConfigStream_001 end");
}

/**
 * @tc.name: CbkReadPropFail_001
 * @tc.desc: CbkReadPropFail
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, CbkReadPropFail_001, TestSize.Level1)
{
    HILOGI("CbkReadPropFail_001 enter");
    ASCService *asc = ASCService::GetService();
    asc->RegisterObserver(g_ascServiceObserver_);
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    asc->SetASCStatus(device, NL_SLE_ASC_CONNECTING);
    asc->CbkReadPropFail(device, 0);
    EXPECT_EQ(false, NL_SLE_ASC_CREATED == asc->GetASCStatus(device));
    asc->SetASCStatus(device, NL_SLE_ASC_CREATED);
    asc->CbkReadPropFail(device, 0); // CbkReadPropFail会执行到异常分支，ReportConnectStateChanged会重新SetASCStatus
    EXPECT_EQ(false, NL_SLE_ASC_CREATED == asc->GetASCStatus(device));
    HILOGI("CbkReadPropFail_001 end");
}

/**
 * @tc.name: ProcWhenIOBCreated_001
 * @tc.desc: ProcWhenIOBCreated
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, ProcWhenIOBCreated_001, TestSize.Level1)
{
    HILOGI("ProcWhenIOBCreated_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    asc->SetASCStatus(device, NL_SLE_ASC_CONNECTING);
    AscQosmInfo qosmInfo {};
    asc->ProcWhenIOBCreated(device, qosmInfo);
    EXPECT_EQ(false, NL_SLE_ASC_CREATED == asc->GetASCStatus(device));
    asc->SetQosmInfoUpdateFlag(device, true);
    AscQosmInfo info {};
    asc->GetStreamQosmInfo(device, info);
    EXPECT_EQ(true, asc->IsQosmInfoUpdated(device));
    HILOGI("ProcWhenIOBCreated_001 end");
}

/**
 * @tc.name: CbkOpenStream_001
 * @tc.desc: CbkOpenStream
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, CbkOpenStream_001, TestSize.Level1)
{
    HILOGI("CbkOpenStream_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    AscQosmInfo qosmInfo {};
    asc->SetASCStatus(device, NL_SLE_ASC_OPENING);
    asc->CbkOpenStream(device, 0, qosmInfo);
    EXPECT_EQ(false, NL_SLE_ASC_CREATED == asc->GetASCStatus(device));
    HILOGI("CbkOpenStream_001 end");
}

/**
 * @tc.name: CbkOpenStream_002
 * @tc.desc: CbkOpenStream
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, CbkOpenStream_002, TestSize.Level1)
{
    HILOGI("CbkOpenStream_002 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    AscQosmInfo qosmInfo {};
    asc->SetASCStatus(device, NL_SLE_ASC_STARTED);
    asc->CbkOpenStream(device, 0, qosmInfo);
    EXPECT_EQ(false, NL_SLE_ASC_CREATED == asc->GetASCStatus(device));
    HILOGI("CbkOpenStream_002 end");
}

/**
 * @tc.name: CbkOpenStream_003
 * @tc.desc: CbkOpenStream
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, CbkOpenStream_003, TestSize.Level1)
{
    HILOGI("CbkOpenStream_003 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    AscQosmInfo qosmInfo {};
    asc->SetASCStatus(device, NL_SLE_ASC_OPENING);
    asc->CbkOpenStream(device, 1, qosmInfo);
    EXPECT_EQ(true, NL_SLE_ASC_CREATED == asc->GetASCStatus(device));
    asc->SetDeviceRole(device);
    HILOGI("CbkOpenStream_003 end");
}

/**
 * @tc.name: SetDeviceRole_001
 * @tc.desc: SetDeviceRole
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, SetDeviceRole_001, TestSize.Level1)
{
    HILOGI("SetDeviceRole_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    RawAddress device1 = RawAddress(INVALID_MAC_ADDRESS);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    asc->SetDeviceRole(device);
    asc->SetDeviceRole(device1);
    HILOGI("SetDeviceRole_001 end");
}

/**
 * @tc.name: ProcSpatialIfNeed_001
 * @tc.desc: ProcSpatialIfNeed
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, ProcSpatialIfNeed_001, TestSize.Level1)
{
    HILOGI("ProcSpatialIfNeed_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);
    asc->ProcSpatialIfNeed(device, AUDIO_STREAM_MUSIC);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_4);
    asc->ProcSpatialIfNeed(device, AUDIO_STREAM_MUSIC);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_8);
    asc->ProcSpatialIfNeed(device, AUDIO_STREAM_MUSIC);
    HILOGI("ProcSpatialIfNeed_001 end");
}

/**
 * @tc.name: CbkStartStream_001
 * @tc.desc: CbkStartStream
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, CbkStartStream_001, TestSize.Level1)
{
    HILOGI("CbkStartStream_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    AscQosmInfo qosmInfo {};
    asc->SetASCStatus(device, NL_SLE_ASC_STARTING);
    asc->CbkStartStream(device, 1, qosmInfo);
    EXPECT_EQ(true, NL_SLE_ASC_CREATED == asc->GetASCStatus(device));
    HILOGI("CbkStartStream_001 end");
}

/**
 * @tc.name: CbkStartStream_002
 * @tc.desc: CbkStartStream
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, CbkStartStream_002, TestSize.Level1)
{
    HILOGI("CbkStartStream_002 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    AscQosmInfo qosmInfo {};
    asc->SetASCStatus(device, NL_SLE_ASC_STARTING);
    asc->CbkStartStream(device, 0, qosmInfo);
    EXPECT_EQ(false, NL_SLE_ASC_CREATED == asc->GetASCStatus(device));
    HILOGI("CbkStartStream_002 end");
}

/**
 * @tc.name: CbkStartStream_003
 * @tc.desc: CbkStartStream
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, CbkStartStream_003, TestSize.Level1)
{
    HILOGI("CbkStartStream_003 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    AscQosmInfo qosmInfo {};
    asc->SetASCStatus(device, NL_SLE_ASC_CONFIG_SUBRATE_CHANGED);
    asc->CbkStartStream(device, 0, qosmInfo);
    asc->SetASCStatus(device, NL_SLE_ASC_RECONFIG_SUBRATE_CHANGED);
    asc->CbkStartStream(device, 0, qosmInfo);
    asc->SetASCStatus(device, NL_SLE_ASC_CREATED);
    RawAddress coSetDevice(coDeviceStr);
    asc->AddConnectDevices(coSetDevice);
    asc->SetSyncFlag(device, true);
    RawAddress reportAddr = asc->GetReportAddr(device);
    asc->SetStopDelayingFlag(reportAddr, true);
    asc->SetNeedDisconnect(device, true);
    asc->CbkStartStream(device, 0, qosmInfo);
    HILOGI("CbkStartStream_003 end");
}

/**
 * @tc.name: ClearWhenDisconnect_001
 * @tc.desc: ClearWhenDisconnect
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, ClearWhenDisconnect_001, TestSize.Level1)
{
    HILOGI("ClearWhenDisconnect_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    AscQosmInfo qosmInfo {};
    asc->SetASCStatus(device, NL_SLE_ASC_STARTING);
    asc->ClearWhenDisconnect(device);
    EXPECT_EQ(true, NL_SLE_ASC_CREATED == asc->GetASCStatus(device));
    HILOGI("ClearWhenDisconnect_001 end");
}

/**
 * @tc.name: StartStartPlayingTimer_001
 * @tc.desc: StartStartPlayingTimer
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, StartStartPlayingTimer_001, TestSize.Level1)
{
    HILOGI("StartStartPlayingTimer_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    AscQosmInfo qosmInfo {};
    asc->SetASCStatus(device, NL_SLE_ASC_STARTING);
    asc->StartStartPlayingTimer(device);
    asc->StopStartPlayingTimer(device);
    asc->activeSinkDevice_ = device;
    asc->StartPlayingTimeout(device);
    EXPECT_EQ(false, NL_SLE_ASC_CREATED == asc->GetASCStatus(device));
    HILOGI("StartStartPlayingTimer_001 end");
}

/**
 * @tc.name: StartStopPlayingTimer_001
 * @tc.desc: StartStopPlayingTimer
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, StartStopPlayingTimer_001, TestSize.Level1)
{
    HILOGI("StartStopPlayingTimer_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    AscQosmInfo qosmInfo {};
    asc->SetASCStatus(device, NL_SLE_ASC_STARTING);
    asc->StartStopPlayingTimer(device);
    asc->StopStopPlayingTimer(device);
    asc->activeSinkDevice_ = device;
    asc->StopPlayingTimeout();
    EXPECT_EQ(false, NL_SLE_ASC_CREATED == asc->GetASCStatus(device));
    HILOGI("StartStopPlayingTimer_001 end");
}

/**
 * @tc.name: StartStopDelayTimer_001
 * @tc.desc: StartStopDelayTimer
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, StartStopDelayTimer_001, TestSize.Level1)
{
    HILOGI("StartStopDelayTimer_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    AscQosmInfo qosmInfo {};
    asc->SetASCStatus(device, NL_SLE_ASC_STARTING);
    asc->StartStopDelayTimer(device);
    asc->StoStopDelayTimer(device);
    asc->StopDelayTimeout();
    ASCMessage event {};
    event.dev_ = device.GetAddress();
    asc->formerSinkDevice_ = device;
    asc->SetStopDelayingFlag(device, true);
    asc->ProcessStopDelayTimeOutEvent(event);
    asc->SetStopDelayingFlag(device, false);
    asc->ProcessStopDelayTimeOutEvent(event);
    asc->ExcuteDelayStop(device);
    EXPECT_EQ(false, NL_SLE_ASC_CREATED == asc->GetASCStatus(device));
    HILOGI("StartStopDelayTimer_001 end");
}

/**
 * @tc.name: StartConnRptDelayTimer_001
 * @tc.desc: StartConnRptDelayTimer
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, StartConnRptDelayTimer_001, TestSize.Level1)
{
    HILOGI("StartConnRptDelayTimer_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    AscQosmInfo qosmInfo {};
    asc->SetASCStatus(device, NL_SLE_ASC_STARTING);
    asc->StartConnRptDelayTimer(device);
    asc->StopConnRptDelayTimer(device);
    asc->ConnRptDelayTimeout();
    ASCMessage event {};
    event.dev_ = device.GetAddress();
    asc->SetConnRptDelayingFlag(device, true);
    asc->ProcessConnRptDelayTimeOutEvent(event);
    asc->SetConnRptDelayingFlag(device, false);
    asc->ProcessConnRptDelayTimeOutEvent(event);
    EXPECT_EQ(false, NL_SLE_ASC_CREATED == asc->GetASCStatus(device));
    HILOGI("StartConnRptDelayTimer_001 end");
}

/**
 * @tc.name: CbkReconfigStopping_001
 * @tc.desc: CbkReconfigStopping
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, CbkReconfigStopping_001, TestSize.Level1)
{
    HILOGI("CbkReconfigStopping_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    asc->SetASCStatus(device, NL_SLE_ASC_STARTING);
    EXPECT_EQ(true, asc->IsInStartProcess(NL_SLE_ASC_STARTING));
    EXPECT_EQ(true, asc->IsInConnectedState(NL_SLE_ASC_STARTING));
    asc->CbkReconfigStopping(device, 1, AUDIO_STREAM_MUSIC);
    asc->CbkReconfigStopping(device, NL_NO_ERROR, AUDIO_STREAM_MUSIC);
    HILOGI("CbkReconfigStopping_001 end");
}

/**
 * @tc.name: CbkReleaseStream_001
 * @tc.desc: CbkReleaseStream
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, CbkReleaseStream_001, TestSize.Level1)
{
    HILOGI("CbkReleaseStream_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    asc->SetASCStatus(device, NL_SLE_ASC_RELEASING);
    asc->CbkReleaseStream(device, 0, 1);
    HILOGI("CbkReleaseStream_001 end");
}

/**
 * @tc.name: CbkReleaseStream_002
 * @tc.desc: CbkReleaseStream
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, CbkReleaseStream_002, TestSize.Level1)
{
    HILOGI("CbkReleaseStream_002 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    asc->SetASCStatus(device, NL_SLE_ASC_RELEASING);
    asc->CbkReleaseStream(device, 1, 1);
    EXPECT_EQ(true, NL_SLE_ASC_CREATED == asc->GetASCStatus(device));
    HILOGI("CbkReleaseStream_002 end");
}

/**
 * @tc.name: CbkReleaseStream_003
 * @tc.desc: CbkReleaseStream
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, CbkReleaseStream_003, TestSize.Level1)
{
    HILOGI("CbkReleaseStream_003 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    asc->SetASCStatus(device, NL_SLE_ASC_RECONFIG_STOPPING);
    asc->CbkReleaseStream(device, 0, 1);
    asc->SetNeedDisconnect(device, true);
    asc->CbkReleaseStream(device, 0, 1);
    HILOGI("CbkReleaseStream_003 end");
}

/**
 * @tc.name: CbkDisconnect_001
 * @tc.desc: CbkDisconnect
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, CbkDisconnect_001, TestSize.Level1)
{
    HILOGI("CbkDisconnect_001 enter");
    ASCService *asc = ASCService::GetService();
    asc->RegisterObserver(g_ascServiceObserver_);
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    asc->SetASCStatus(device, NL_SLE_ASC_RELEASING);
    asc->CbkDisconnect(device, 0);
    EXPECT_EQ(false, NL_SLE_ASC_CREATED == asc->GetASCStatus(device));
    HILOGI("CbkDisconnect_001 end");
}

/**
 * @tc.name: ProcessChangeBitrateEvent_001
 * @tc.desc: ProcessChangeBitrateEvent
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, ProcessChangeBitrateEvent_001, TestSize.Level1)
{
    HILOGI("ProcessChangeBitrateEvent_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    asc->SetASCStatus(device, NL_SLE_ASC_RELEASING);
    ASCMessage event {};
    asc->ProcessChangeBitrateEvent(event);
    EXPECT_EQ(false, NL_SLE_ASC_CREATED == asc->GetASCStatus(device));
    HILOGI("ProcessChangeBitrateEvent_001 end");
}

/**
 * @tc.name: AudioControl_001
 * @tc.desc: AudioControl
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, AudioControl_001, TestSize.Level1)
{
    HILOGI("AudioControl_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    asc->SetASCStatus(device, NL_SLE_ASC_RELEASING);
    asc->AudioControl(device, AUDIO_STREAM_MUSIC, NL_SLE_ASC_CONTROL_CMD_START);
    asc->AudioControl(device, AUDIO_STREAM_MUSIC, NL_SLE_ASC_CONTROL_CMD_STOP);
    EXPECT_EQ(false, NL_SLE_ASC_CREATED == asc->GetASCStatus(device));
    HILOGI("AudioControl_001 end");
}

/**
 * @tc.name: ProcessSendPlayOrPauseEvent_001
 * @tc.desc: ProcessSendPlayOrPauseEvent
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, ProcessSendPlayOrPauseEvent_001, TestSize.Level1)
{
    HILOGI("ProcessSendPlayOrPauseEvent_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    asc->SetASCStatus(device, NL_SLE_ASC_RELEASING);
    ASCMessage event {};
    event.dev_ = device.GetAddress();
    asc->SendPlayOrPauseByWearDetection(device, 1);
    asc->SendEventByWearDetection(device, 1);
    asc->ProcessSendPlayOrPauseEvent(event);
    EXPECT_EQ(false, NL_SLE_ASC_CREATED == asc->GetASCStatus(device));
    HILOGI("ProcessSendPlayOrPauseEvent_001 end");
}

/**
 * @tc.name: ProcessStartPlayingEvent_001
 * @tc.desc: ProcessStartPlayingEvent
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, ProcessStartPlayingEvent_001, TestSize.Level1)
{
    HILOGI("ProcessStartPlayingEvent_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device(deviceStr);
    RawAddress coDevice(coDeviceStr);
    asc->AddConnectDevices(device);
    asc->AddConnectDevices(coDevice);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);


    ASCMessage event {};
    event.dev_ = device.GetAddress();
    asc->SetStopDelayingFlag(device, false);
    asc->SetStopDelayingFlag(coDevice, false);
    asc->SetASCStatus(device, NL_SLE_ASC_RELEASING);
    asc->SetASCStatus(coDevice, NL_SLE_ASC_RELEASING);
    asc->ProcessStartPlayingEvent(event);
    asc->SetStopDelayingFlag(device, true);
    asc->SetStopDelayingFlag(coDevice, true);
    std::list<AudioStreamType>& startedStreamList1 = asc->GetStartedStreamList(device);
    startedStreamList1.emplace_back(AUDIO_STREAM_MUSIC);
    std::list<AudioStreamType>& startedStreamList2 = asc->GetStartedStreamList(coDevice);
    startedStreamList2.emplace_back(AUDIO_STREAM_MUSIC);
    asc->SetASCStatus(device, NL_SLE_ASC_STARTED);
    asc->SetASCStatus(coDevice, NL_SLE_ASC_STARTED);
    asc->ProcessStartPlayingEvent(event);
    asc->SetASCStatus(device, NL_SLE_ASC_RELEASED);
    asc->SetASCStatus(coDevice, NL_SLE_ASC_RELEASED);
    asc->ProcessStartPlayingEvent(event);
    asc->SetASCStatus(device, NL_SLE_ASC_CREATED);
    asc->SetASCStatus(coDevice, NL_SLE_ASC_CREATED);
    asc->ProcessStartPlayingEvent(event);
    HILOGI("ProcessStartPlayingEvent_001 end");
}

/**
 * @tc.name: ProcessStopPlayingEvent_001
 * @tc.desc: ProcessStopPlayingEvent
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, ProcessStopPlayingEvent_001, TestSize.Level1)
{
    HILOGI("ProcessStopPlayingEvent_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    asc->SetASCStatus(device, NL_SLE_ASC_RELEASING);
    ASCMessage event {};
    event.dev_ = device.GetAddress();
    asc->ProcessStopPlayingEvent(event);
    EXPECT_EQ(false, NL_SLE_ASC_CREATED == asc->GetASCStatus(device));
    RawAddress coDevice(coDeviceStr);
    asc->AddConnectDevices(coDevice);
    asc->SetASCStatus(device, NL_SLE_ASC_CREATED);
    asc->SetASCStatus(coDevice, NL_SLE_ASC_CREATED);
    asc->ProcessStopPlayingEvent(event);
    asc->SetASCStatus(device, NL_SLE_ASC_CONNECTED);
    asc->SetASCStatus(coDevice, NL_SLE_ASC_CONNECTED);
    asc->ProcessStopPlayingEvent(event);
    asc->SetASCStatus(device, NL_SLE_ASC_STARTED);
    asc->SetASCStatus(coDevice, NL_SLE_ASC_STARTED);
    asc->ProcessStopPlayingEvent(event);
    asc->SetASCStatus(device, NL_SLE_ASC_STOPPED);
    asc->SetASCStatus(coDevice, NL_SLE_ASC_STOPPED);
    asc->ProcessStopPlayingEvent(event);
    asc->SetASCStatus(device, NL_SLE_ASC_RECONFIGED);
    asc->SetASCStatus(coDevice, NL_SLE_ASC_RECONFIGED);
    asc->ProcessStopPlayingEvent(event);
    HILOGI("ProcessStopPlayingEvent_001 end");
}

/**
 * @tc.name: SetActiveSinkDevice_001
 * @tc.desc: SetActiveSinkDevice
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, SetActiveSinkDevice_001, TestSize.Level1)
{
    HILOGI("SetActiveSinkDevice_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    asc->SetASCStatus(device, NL_SLE_ASC_RELEASING);
    asc->SetActiveSinkDevice(static_cast<NearlinkRawAddress>(device), 123);
    EXPECT_EQ(false, NL_SLE_ASC_CREATED == asc->GetASCStatus(device));
    HILOGI("SetActiveSinkDevice_001 end");
}

/**
 * @tc.name: ProcessSetActiveSinkDeviceEvent_001
 * @tc.desc: ProcessSetActiveSinkDeviceEvent
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, ProcessSetActiveSinkDeviceEvent_001, TestSize.Level1)
{
    HILOGI("ProcessSetActiveSinkDeviceEvent_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    RawAddress device2 = RawAddress(coDeviceStr);
    asc->AddConnectDevices(device2);
    asc->SetASCStatus(device, NL_SLE_ASC_STARTED);
    asc->SetASCStatus(device2, NL_SLE_ASC_STARTED);
    EXPECT_EQ(true, asc->IsStartMemberExist(device));
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    asc->SetASCStatus(device, NL_SLE_ASC_RELEASING);
    asc->SetStopDelayingFlag(device, true);
    EXPECT_EQ(true, asc->IsStopDelaying(device));
    ASCMessage event {};
    event.dev_ = device.GetAddress();
    event.streamTypeBitMap_ = AUDIO_STREAM_NONE;
    asc->ProcessSetActiveSinkDeviceEvent(event);

    asc->SetStopDelayingFlag(device, false);
    EXPECT_EQ(false, asc->IsStopDelaying(device));
    EXPECT_EQ(true, asc->IsConnectedMemberExist(device));
    asc->activeSinkDevice_ = device2;
    asc->SetStopDelayingFlag(device2, true);
    EXPECT_EQ(true, asc->IsStopDelaying(device2));
    event.streamTypeBitMap_ = AUDIO_STREAM_MUSIC;
    asc->SetASCStatus(device, NL_SLE_ASC_RELEASING);
    asc->formerPlayRecord_.addr = device;
    asc->formerSinkDevice_ = device;
    asc->ProcessSetActiveSinkDeviceEvent(event);
    HILOGI("ProcessSetActiveSinkDeviceEvent_001 end");
}

/**
 * @tc.name: ProcessUpdateDeviceRoleEvent_001
 * @tc.desc: ProcessUpdateDeviceRoleEvent
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, ProcessUpdateDeviceRoleEvent_001, TestSize.Level1)
{
    HILOGI("ProcessUpdateDeviceRoleEvent_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device(deviceStr);
    RawAddress coSetDevice(coDeviceStr);
    asc->AddConnectDevices(device);
    asc->AddConnectDevices(coSetDevice);
    EXPECT_EQ(true, asc->IsCoSetDeviceExist(device, coSetDevice));

    asc->SetASCStatus(device, NL_SLE_ASC_STARTED);
    ASCMessage event {};
    event.dev_ = device.GetAddress();
    event.devRole_ = static_cast<uint8_t>(TwsRoleType::ROLE_TYPE_PRIMARY);
    QosM::GetInstance().SetCos(device, NL_SLE_QOS_1);
    asc->ProcessUpdateDeviceRoleEvent(event);
    QosM::GetInstance().SetCos(device, NL_SLE_QOS_2);
    asc->ProcessUpdateDeviceRoleEvent(event);
    QosM::GetInstance().SetCos(device, NL_SLE_QOS_4);
    asc->ProcessUpdateDeviceRoleEvent(event);
    QosM::GetInstance().SetCos(device, NL_SLE_QOS_3);
    asc->ProcessUpdateDeviceRoleEvent(event);
    QosM::GetInstance().SetCos(device, NL_SLE_QOS_5);
    asc->ProcessUpdateDeviceRoleEvent(event);
    event.devRole_ = static_cast<uint8_t>(TwsRoleType::ROLE_TYPE_SECONDARY);
    QosM::GetInstance().SetCos(device, NL_SLE_QOS_6);
    asc->ProcessUpdateDeviceRoleEvent(event);
    QosM::GetInstance().SetCos(device, NL_SLE_QOS_7);
    asc->ProcessUpdateDeviceRoleEvent(event);
    QosM::GetInstance().SetCos(device, NL_SLE_QOS_BUTT);
    asc->ProcessUpdateDeviceRoleEvent(event);
    asc->UpdateDeviceRole(device, 1);
    asc->SetASCStatus(device, NL_SLE_ASC_DISABLED);
    asc->ProcessUpdateDeviceRoleEvent(event);
    HILOGI("ProcessUpdateDeviceRoleEvent_001 end");
}

/**
 * @tc.name: ProcessEvent_001
 * @tc.desc: ProcessEvent ASC_UPDATE_VIRTUAL_DEVICE_EVT ASC_SUBRATE_CHANGE_REQ_EVT
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, ProcessEvent_001, TestSize.Level1)
{
    HILOGI("ProcessEvent_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    asc->SetASCStatus(device, NL_SLE_ASC_RELEASING);
    ASCMessage event {};
    event.dev_ = device.GetAddress();
    event.whatM = ASC_UPDATE_VIRTUAL_DEVICE_EVT;
    asc->ProcessEvent(event);
    event.whatM = ASC_SUBRATE_CHANGE_REQ_EVT;
    asc->ProcessEvent(event);
    EXPECT_EQ(false, NL_SLE_ASC_CREATED == asc->GetASCStatus(device));
    EXPECT_EQ(true, 3 ==
        asc->GetDftUpdateVoiceStackReason(2));
    EXPECT_EQ(true, 0 == asc->GetDftUpdateVoiceStackReason(8));
    HILOGI("ProcessEvent_001 end");
}

/**
 * @tc.name: SleAudioDeviceActionChanged_001
 * @tc.desc: SleAudioDeviceActionChanged
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, SleAudioDeviceActionChanged_001, TestSize.Level1)
{
    HILOGI("SleAudioDeviceActionChanged_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    asc->SetASCStatus(device, NL_SLE_ASC_RELEASING);
    asc->SleAudioDeviceActionChanged(NearlinkRawAddress(device), 0);
    std::vector<struct AudioStreamInfo> streamData;
    asc->SleAudioDeviceActionChanged(NearlinkRawAddress(device), streamData, 0);
    asc->callback_ = std::make_shared<ASCCallbackImplTestStub>();
    asc->SleAudioDeviceActionChanged(NearlinkRawAddress(device), streamData, 0);
    asc->SleAudioDeviceActionChanged(NearlinkRawAddress(device), 0);
    HILOGI("SleAudioDeviceActionChanged_001 end");
}

/**
 * @tc.name: ChangeIsoParamIfNeed_001
 * @tc.desc: ChangeIsoParamIfNeed
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, ChangeIsoParamIfNeed_001, TestSize.Level1)
{
    HILOGI("ChangeIsoParamIfNeed_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    RawAddress coDevice(coDeviceStr);
    asc->AddConnectDevices(device);
    asc->AddConnectDevices(coDevice);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    asc->SetASCStatus(device, NL_SLE_ASC_RELEASING);
    asc->SetIsCallingFlag(true);
    EXPECT_EQ(true, asc->GetIsCallingFlag());
    EXPECT_EQ(true, asc->IsCalling());
    EXPECT_EQ(false, asc->IsPlaying(device));

    asc->RegisterSpatialAudioListener();
    EXPECT_EQ(false, asc->IsSpatialAudioModeEnabled(device.GetAddress()));
    EXPECT_EQ(false, asc->IsSpatialAudioHeadTrackingEnabled(device.GetAddress()));
    asc->SetSpatialAudioModeEnabled(true);
    asc->SetSpatialAudioHeadTrackingEnabled(true);
    EXPECT_EQ(true, asc->IsSpatialAudioModeEnabled());
    EXPECT_EQ(true, asc->IsSpatialAudioHeadTrackingEnabled());
    asc->SetASCStatus(device, NL_SLE_ASC_STARTED);
    std::list<AudioStreamType>& startedStreamList = asc->GetStartedStreamList(device);
    startedStreamList.emplace_back(AUDIO_STREAM_MUSIC);
    asc->ChangeIsoParamIfNeed();
    asc->SetASCStatus(device, NL_SLE_ASC_STARTED);
    asc->SetASCStatus(coDevice, NL_SLE_ASC_STARTED);
    asc->ChangeIsoParamIfNeed();
    HILOGI("ChangeIsoParamIfNeed_001 end");
}

/**
 * @tc.name: RegisterAudioSceneListener_001
 * @tc.desc: RegisterAudioSceneListener
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, RegisterAudioSceneListener_001, TestSize.Level1)
{
    HILOGI("RegisterAudioSceneListener_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    asc->SetASCStatus(device, NL_SLE_ASC_RELEASING);
    asc->RegisterAudioSceneListener();
    HILOGI("RegisterAudioSceneListener_001 end");
}

/**
 * @tc.name: OnAudioSceneChange_001
 * @tc.desc: OnAudioSceneChange
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, OnAudioSceneChange_001, TestSize.Level1)
{
    HILOGI("OnAudioSceneChange_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    asc->SetASCStatus(device, NL_SLE_ASC_RELEASING);
    std::shared_ptr<AudioSceneChangedListener> extAudioSceneListener = std::make_shared<AudioSceneChangedListener>();
    extAudioSceneListener->OnAudioSceneChange(AudioStandard::AudioScene::AUDIO_SCENE_RINGING);
    extAudioSceneListener->OnAudioSceneChange(AudioStandard::AudioScene::AUDIO_SCENE_PHONE_CALL);
    extAudioSceneListener->OnAudioSceneChange(AudioStandard::AudioScene::AUDIO_SCENE_PHONE_CHAT);
    extAudioSceneListener->OnAudioSceneChange(AudioStandard::AudioScene::AUDIO_SCENE_VOICE_RINGING);
    HILOGI("OnAudioSceneChange_001 end");
}

/**
 * @tc.name: ProcessAudioSceneChangeEvent_001
 * @tc.desc: ProcessAudioSceneChangeEvent
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, ProcessAudioSceneChangeEvent_001, TestSize.Level1)
{
    HILOGI("ProcessAudioSceneChangeEvent_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    asc->SetASCStatus(device, NL_SLE_ASC_RELEASING);
    asc->SetIsCallingFlag(false);
    ASCMessage event {};
    event.dev_ = device.GetAddress();
    event.isCalling_ = true;
    asc->ProcessAudioSceneChangeEvent(event);
    HILOGI("ProcessAudioSceneChangeEvent_001 end");
}

/**
 * @tc.name: SwitchAbsVolumeDevice_001
 * @tc.desc: SwitchAbsVolumeDevice
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, SwitchAbsVolumeDevice_001, TestSize.Level1)
{
    HILOGI("SwitchAbsVolumeDevice_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    asc->SetASCStatus(device, NL_SLE_ASC_RELEASING);
    asc->SetIsCallingFlag(false);
    asc->SwitchAbsVolumeDevice(device, true);
    HILOGI("SwitchAbsVolumeDevice_001 end");
}

/**
 * @tc.name: OpenVoiceAssistant_001
 * @tc.desc: OpenVoiceAssistant
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, OpenVoiceAssistant_001, TestSize.Level1)
{
    HILOGI("OpenVoiceAssistant_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    asc->SetASCStatus(device, NL_SLE_ASC_RELEASING);
    asc->OpenVoiceAssistant(device, AUDIO_STREAM_VOICE_ASSISTANT);
    asc->CloseVoiceAssistant(device, AUDIO_STREAM_VOICE_ASSISTANT);
    HILOGI("OpenVoiceAssistant_001 end");
}

/**
 * @tc.name: CheckStreamIsNeedNotifyCcp_001
 * @tc.desc: CheckStreamIsNeedNotifyCcp
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, CheckStreamIsNeedNotifyCcp_001, TestSize.Level1)
{
    HILOGI("CheckStreamIsNeedNotifyCcp_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    asc->SetASCStatus(device, NL_SLE_ASC_RELEASING);
    asc->CheckStreamIsNeedNotifyCcp(device, AUDIO_STREAM_ALARM, NL_SLE_ASC_CONTROL_CMD_START);
    HILOGI("CheckStreamIsNeedNotifyCcp_001 end");
}

/**
 * @tc.name: CheckStreamIsNeedNotifyCcp_002
 * @tc.desc: CheckStreamIsNeedNotifyCcp
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, CheckStreamIsNeedNotifyCcp_002, TestSize.Level1)
{
    HILOGI("CheckStreamIsNeedNotifyCcp_002 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    asc->SetASCStatus(device, NL_SLE_ASC_RELEASING);
    asc->CheckStreamIsNeedNotifyCcp(device, AUDIO_STREAM_ALARM, NL_SLE_ASC_CONTROL_CMD_STOP);
    CcpService *ccp = new CcpService();
    asc->CheckStreamIsNeedNotifyCcp(device, AUDIO_STREAM_VOIP, NL_SLE_ASC_CONTROL_CMD_START);
    asc->CheckStreamIsNeedNotifyCcp(device, AUDIO_STREAM_VOIP, NL_SLE_ASC_CONTROL_CMD_STOP);
    HILOGI("CheckStreamIsNeedNotifyCcp_002 end");
}

/**
 * @tc.name: ProcessMcpInit_001
 * @tc.desc: ProcessMcpInit
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, ProcessMcpInit_001, TestSize.Level1)
{
    HILOGI("ProcessMcpInit_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    asc->SetASCStatus(device, NL_SLE_ASC_RELEASING);
    asc->ProcessMcpInit(device);
    HILOGI("ProcessMcpInit_001 end");
}

/**
 * @tc.name: ProcessCcpInit_001
 * @tc.desc: ProcessCcpInit
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, ProcessCcpInit_001, TestSize.Level1)
{
    HILOGI("ProcessCcpInit_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    asc->SetASCStatus(device, NL_SLE_ASC_RELEASING);
    asc->ProcessCcpInit(device);
    HILOGI("ProcessCcpInit_001 end");
}

/**
 * @tc.name: RegisterAudioPreferredOutPutDeviceChangeListener_001
 * @tc.desc: RegisterAudioPreferredOutPutDeviceChangeListener
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, RegisterAudioPreferredOutPutDeviceChangeListener_001, TestSize.Level1)
{
    HILOGI("RegisterAudioPreferredOutPutDeviceChangeListener_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    asc->SetASCStatus(device, NL_SLE_ASC_RELEASING);
    asc->RegisterAudioPreferredOutPutDeviceChangeListener();
    HILOGI("RegisterAudioPreferredOutPutDeviceChangeListener_001 end");
}

/**
 * @tc.name: UnregisterAudioPreferredOutPutDeviceChangeListener_002
 * @tc.desc: UnregisterAudioPreferredOutPutDeviceChangeListener
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, UnregisterAudioPreferredOutPutDeviceChangeListener_002, TestSize.Level1)
{
    HILOGI("UnregisterAudioPreferredOutPutDeviceChangeListener_002 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    asc->SetASCStatus(device, NL_SLE_ASC_RELEASING);
    asc->UnregisterAudioPreferredOutPutDeviceChangeListener();
    HILOGI("UnregisterAudioPreferredOutPutDeviceChangeListener_002 end");
}

/**
 * @tc.name: IsMusicActive_001
 * @tc.desc: IsMusicActive
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, IsMusicActive_001, TestSize.Level1)
{
    HILOGI("IsMusicActive_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    asc->SetASCStatus(device, NL_SLE_ASC_RELEASING);
    EXPECT_EQ(false, SleAudioFrameworkAdapter::GetInstance().IsMusicActive());
    HILOGI("IsMusicActive_001 end");
}

/**
 * @tc.name: IsAudioOutputToNlAudio_001
 * @tc.desc: IsAudioOutputToNlAudio
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, IsAudioOutputToNlAudio_001, TestSize.Level1)
{
    HILOGI("IsAudioOutputToNlAudio_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    asc->SetASCStatus(device, NL_SLE_ASC_RELEASING);
    asc->IsAudioOutputToNlAudio();
    HILOGI("IsAudioOutputToNlAudio_001 end");
}

/**
 * @tc.name: SetCurrentDeviceMute_001
 * @tc.desc: SetCurrentDeviceMute
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, SetCurrentDeviceMute_001, TestSize.Level1)
{
    HILOGI("SetCurrentDeviceMute_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    ASCService *ascService =
        static_cast<ASCService*>(SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_ASC));
    // 必须这么赋值 asc->activeSinkDevice_ = device 这种赋值不成功, asc对象不同
    if (ascService != nullptr) {
        ascService->activeSinkDevice_ = device;
    }
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);
    asc->activeSinkDevice_ = device;
    asc->timerForRestoreVolumeIfPaused_ = std::make_shared<NearlinkTimer>([this]() -> void {});
    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, asc->SetMusicMuteWhenAudioRelease());
    asc->SetCurrentDeviceMute();
    HILOGI("SetCurrentDeviceMute_001 test");
    RawAddress coDevice = RawAddress(coDeviceStr);
    // 必须这么赋值 asc->activeSinkDevice_ = device 这种赋值不成功, asc对象不同
    if (ascService != nullptr) {
        ascService->activeSinkDevice_ = coDevice;
        std::list<AudioStreamType>& startedList = ascService->GetStartedStreamList(coDevice);
        startedList.emplace_back(AUDIO_STREAM_MUSIC);
        startedList.emplace_back(AUDIO_STREAM_GUID);
    }
    int32_t uid = IPCSkeleton::GetCallingUid();
    AudioStandard::AudioVolumeClientManager::GetInstance().SetVolume(
        AudioStandard::AudioVolumeType::STREAM_MUSIC, 7, uid);
    asc->SetCurrentDeviceMute();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TDD_DELAY_2000_MS));
    HILOGI("SetCurrentDeviceMute_001 end");
}

/**
 * @tc.name: SetMusicMuteWhenAudioRelease_001
 * @tc.desc: SetMusicMuteWhenAudioRelease
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, SetMusicMuteWhenAudioRelease_001, TestSize.Level1)
{
    HILOGI("SetMusicMuteWhenAudioRelease_001 enter");
    ASCService *asc = new ASCService();
    ASCService *ascService =
        static_cast<ASCService*>(SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_ASC));
    if (ascService != nullptr) {
        ascService->activeSinkDevice_ = RawAddress(INVALID_MAC_ADDRESS);
    }
    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, asc->SetMusicMuteWhenAudioRelease());

    if (ascService != nullptr) {
        ascService->activeSinkDevice_ = RawAddress();
    }
    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, asc->SetMusicMuteWhenAudioRelease());
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    // 必须这么赋值 asc->activeSinkDevice_ = device 这种赋值不成功, asc对象不同
    if (ascService != nullptr) {
        ascService->activeSinkDevice_ = device;
    }
    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, asc->SetMusicMuteWhenAudioRelease());
    RawAddress coDevice = RawAddress(coDeviceStr);
    // 必须这么赋值 asc->activeSinkDevice_ = device 这种赋值不成功, asc对象不同
    if (ascService != nullptr) {
        ascService->activeSinkDevice_ = coDevice;
    }
    asc->AddConnectDevices(coDevice);
    int32_t uid = IPCSkeleton::GetCallingUid();
    AudioStandard::AudioVolumeClientManager::GetInstance().SetVolume(
        AudioStandard::AudioVolumeType::STREAM_MUSIC, 0, uid);
    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, asc->SetMusicMuteWhenAudioRelease());
    AudioStandard::AudioVolumeClientManager::GetInstance().SetVolume(
        AudioStandard::AudioVolumeType::STREAM_MUSIC, 7, uid);

    // 设置音频广播业务类型(否则该用例会识别为三方耳机)
    std::shared_ptr<SlePeripheralDevice> reportDev = std::make_shared<SlePeripheralDevice>();
    reportDev->SetAddress(device);
    reportDev->SetManufacturerBusiness(Nearlink::SLE_PRIVATE_AUDIO_BUSINESS_TYPE);
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(deviceStr, reportDev);
    std::shared_ptr<SlePeripheralDevice> memberDev = std::make_shared<SlePeripheralDevice>();
    memberDev->SetAddress(coDevice);
    memberDev->SetManufacturerBusiness(Nearlink::SLE_PRIVATE_AUDIO_BUSINESS_TYPE);
    SleRemoteDeviceAdapter::GetInstance()->AddPeripheralDevice(coDeviceStr, memberDev);

    EXPECT_EQ(NL_NO_ERROR, asc->SetMusicMuteWhenAudioRelease());
    HILOGI("SetMusicMuteWhenAudioRelease_001 end");
}

/**
 * @tc.name: SetMusicUnmute_001
 * @tc.desc: SetMusicUnmute
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, SetMusicUnmute_001, TestSize.Level1)
{
    HILOGI("SetMusicUnmute_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    asc->SetASCStatus(device, NL_SLE_ASC_RELEASING);
    asc->SetMusicUnmute();
    HILOGI("SetMusicUnmute_001 end");
}

/**
 * @tc.name: AcbSubrateChanged_001
 * @tc.desc: AcbSubrateChanged
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, AcbSubrateChanged_001, TestSize.Level1)
{
    HILOGI("AcbSubrateChanged_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    asc->SetASCStatus(device, NL_SLE_ASC_RELEASING);
    asc->AcbSubrateChanged(device, 6);
    HILOGI("AcbSubrateChanged_001 end");
}

/**
 * @tc.name: ProcessSubrateChangedEvent_001
 * @tc.desc: ProcessSubrateChangedEvent
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, ProcessSubrateChangedEvent_001, TestSize.Level1)
{
    HILOGI("ProcessSubrateChangedEvent_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    asc->SetASCStatus(device, NL_SLE_ASC_CONFIGURING);
    ASCMessage event {};
    event.dev_ = device.GetAddress();
    event.subrate_ = NLSTK_DEFAULT_SUBRATE;
    asc->ProcessSubrateChangedEvent(event);
    HILOGI("ProcessSubrateChangedEvent_001 end");
}

/**
 * @tc.name: AcbSubrateChangeReq_001
 * @tc.desc: AcbSubrateChangeReq
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, AcbSubrateChangeReq_001, TestSize.Level1)
{
    HILOGI("AcbSubrateChangeReq_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    asc->SetASCStatus(device, NL_SLE_ASC_CONFIGURING);
    SleAcbSubrateParam subrateParam {};
    asc->AcbSubrateChangeReq(device, subrateParam);
    HILOGI("AcbSubrateChangeReq_001 end");
}

/**
 * @tc.name: ProcessSubrateChangeReq_001
 * @tc.desc: ProcessSubrateChangeReq
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, ProcessSubrateChangeReq_001, TestSize.Level1)
{
    HILOGI("ProcessSubrateChangeReq_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    asc->SetASCStatus(device, NL_SLE_ASC_CONFIGURING);
    ASCMessage event {};
    event.dev_ = device.GetAddress();
    SleAcbSubrateParam subrateParam {};
    event.subratePara_ = subrateParam;
    event.subratePara_.subrate = NLSTK_DEFAULT_SUBRATE - 1;
    asc->ProcessSubrateChangeReq(event);
    HILOGI("ProcessSubrateChangeReq_001 end");
}

/**
 * @tc.name: SetSubrate_001
 * @tc.desc: SetSubrate
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, SetSubrate_001, TestSize.Level1)
{
    HILOGI("SetSubrate_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    asc->SetASCStatus(device, NL_SLE_ASC_CONFIGURING);
    SleAcbSubrateParam subrateParam {};
    asc->SetSubrate(device, subrateParam);
    HILOGI("SetSubrate_001 end");
}

/**
 * @tc.name: SetAutoConnectDevice_001
 * @tc.desc: SetAutoConnectDevice
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, SetAutoConnectDevice_001, TestSize.Level1)
{
    HILOGI("SetAutoConnectDevice_001 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    asc->SetASCStatus(device, NL_SLE_ASC_CONFIGURING);
    asc->SetAutoConnectDevice(device, true);
    HILOGI("SetAutoConnectDevice_001 end");
}

/**
 * @tc.name: SetAutoConnectDevice_002
 * @tc.desc: SetAutoConnectDevice
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, SetAutoConnectDevice_002, TestSize.Level1)
{
    HILOGI("SetAutoConnectDevice_002 enter");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    asc->AddConnectDevices(device);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);

    asc->SetASCStatus(device, NL_SLE_ASC_CONFIGURING);
    asc->SetAutoConnectDevice(device, false);
    HILOGI("SetAutoConnectDevice_002 end");
}

/**
 * @tc.name: GetBpsBitIndexByBps_001
 * @tc.desc: GetBpsBitIndexByBps
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, GetBpsBitIndexByBps_001, TestSize.Level1)
{
    HILOGI("GetBpsBitIndexByBps_001 enter");
    EXPECT_EQ(true, L2HC_BPS_S_48_BIT == ASCUtils::GetBpsBitIndexByBps(BPS_48));
    EXPECT_EQ(true, L2HC_BPS_S_64_BIT == ASCUtils::GetBpsBitIndexByBps(BPS_64));
    EXPECT_EQ(true, L2HC_BPS_S_96_BIT == ASCUtils::GetBpsBitIndexByBps(BPS_96));
    EXPECT_EQ(true, L2HC_BPS_S_128_BIT == ASCUtils::GetBpsBitIndexByBps(BPS_128));
    EXPECT_EQ(true, L2HC_BPS_S_240_BIT == ASCUtils::GetBpsBitIndexByBps(BPS_240));
    EXPECT_EQ(true, L2HC_BPS_S_320_BIT == ASCUtils::GetBpsBitIndexByBps(BPS_320));
    EXPECT_EQ(true, L2HC_BPS_S_480_BIT == ASCUtils::GetBpsBitIndexByBps(BPS_480));
    EXPECT_EQ(true, L2HC_BPS_S_750_BIT == ASCUtils::GetBpsBitIndexByBps(BPS_750));
    EXPECT_EQ(true, L2HC_BPS_S_960_BIT == ASCUtils::GetBpsBitIndexByBps(BPS_960));
    EXPECT_EQ(true, L2HC_BPS_S_1150_BIT == ASCUtils::GetBpsBitIndexByBps(BPS_1150));
    EXPECT_EQ(true, L2HC_BPS_S_2300_BIT == ASCUtils::GetBpsBitIndexByBps(BPS_2300));
    EXPECT_EQ(true, L2HC_BPS_S_64_BIT == ASCUtils::GetBpsBitIndexByBps(0));
    HILOGI("GetBpsBitIndexByBps_001 end");
}

/**
 * @tc.name: GetConnectDevices_001
 * @tc.desc: GetConnectDevices
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, GetConnectDevices_001, TestSize.Level1)
{
    HILOGI("GetConnectDevices_001 start");
    ASCService *asc = new ASCService();
    std::list<RawAddress> connectedDevices = asc->GetConnectDevices();
    EXPECT_EQ(0, connectedDevices.size());
    RawAddress device(coDeviceStr);
    asc->AddConnectDevices(device);
    connectedDevices = asc->GetConnectDevices();
    EXPECT_EQ(1, connectedDevices.size());
    HILOGI("GetConnectDevices_001 end");
}

/**
 * @tc.name: DisconnProcSetStatus_001
 * @tc.desc: DisconnProcSetStatus
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, DisconnProcSetStatus_001, TestSize.Level1)
{
    HILOGI("DisconnProcSetStatus_001 start");
    RawAddress device(coDeviceStr);
    ASCState state = NL_SLE_ASC_DISCONNECTED;
    ASCService *asc = new ASCService();
    EXPECT_EQ(NL_NO_ERROR, asc->DisconnProcSetStatus(device, state));
    HILOGI("DisconnProcSetStatus_001 end");
}

/**
 * @tc.name: IsLeftEarDevice_001
 * @tc.desc: IsLeftEarDevice vendor device
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, IsLeftEarDevice_001, TestSize.Level1)
{
    HILOGI("IsLeftEarDevice_001 start");
    RawAddress targetAddr(INVALID_MAC_ADDRESS);
    RawAddress huaweiAddr(coDeviceStr);
    ASCService *asc = new ASCService();
    asc->AddConnectDevices(targetAddr);
    asc->AddConnectDevices(huaweiAddr);
    asc->IsLeftEarDevice(targetAddr);
    asc->IsLeftEarDevice(huaweiAddr);
    HILOGI("IsLeftEarDevice_001 end");
}

/**
 * @tc.name: IsRolePrimary_001
 * @tc.desc: IsRolePrimary vendor device
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, IsRolePrimary_001, TestSize.Level1)
{
    HILOGI("IsRolePrimary_001 start");
    RawAddress targetAddr(INVALID_MAC_ADDRESS);
    RawAddress huaweiAddr(coDeviceStr);
    ASCService *asc = new ASCService();
    asc->AddConnectDevices(targetAddr);
    asc->AddConnectDevices(huaweiAddr);
    asc->IsRolePrimary(targetAddr);
    asc->IsRolePrimary(huaweiAddr);
    HILOGI("IsRolePrimary_001 end");
}

/**
 * @tc.name: CreateCommLink_001
 * @tc.desc: CreateCommLink vendor device
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, CreateCommLink_001, TestSize.Level1)
{
    HILOGI("CreateCommLink_001 start");
    RawAddress targetAddr(INVALID_MAC_ADDRESS);
    RawAddress huaweiAddr(coDeviceStr);
    AudioStreamType streamType = static_cast<AudioStreamType>(AUDIO_STREAM_GAME);
    ASCService *asc = new ASCService();
    asc->CreateCommLink(targetAddr, streamType);
    asc->CreateCommLink(huaweiAddr, streamType);
    HILOGI("CreateCommLink_001 end");
}

/**
 * @tc.name: DeregisterObserver_001
 * @tc.desc: DeregisterObserver vendor device
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, DeregisterObserver_001, TestSize.Level1)
{
    HILOGI("DeregisterObserver_001 start");
    ASCObserver observer{};
    ASCService *asc = new ASCService();
    asc->DeregisterObserver(observer);
    delete asc;
    HILOGI("DeregisterObserver_001 end");
}

/**
 * @tc.name: DeregisterApplication_001
 * @tc.desc: DeregisterApplication
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, DeregisterApplication_001, TestSize.Level1)
{
    HILOGI("DeregisterApplication_001 start");
    const std::shared_ptr<InterfaceASCCallback> callback = nullptr;
    ASCService *asc = new ASCService();
    asc->DeregisterApplication(callback);
    HILOGI("DeregisterApplication_001 end");
}

/**
 * @tc.name: UpdateSleVirtualDevice_001
 * @tc.desc: UpdateSleVirtualDevice
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, UpdateSleVirtualDevice_001, TestSize.Level1)
{
    HILOGI("UpdateSleVirtualDevice_001 start");
    ASCService *asc = new ASCService();
    RawAddress device(coDeviceStr);
    EXPECT_EQ(true, asc->UpdateSleVirtualDevice(0, device));
    HILOGI("UpdateSleVirtualDevice_001 end");
}

/**
 * @tc.name: SetSubrate_002
 * @tc.desc: SetSubrate
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, SetSubrate_002, TestSize.Level1)
{
    HILOGI("SetSubrate_002 start");
    ASCService *asc = new ASCService();
    RawAddress targetAddr(INVALID_MAC_ADDRESS);
    RawAddress huaweiAddr(coDeviceStr);
    SleAcbSubrateParam subrateParam {};
    subrateParam.onlySubrate = true;
    subrateParam.subrate = static_cast<NLSTK_SubrateType_E>(NLSTK_DEFAULT_SUBRATE);
    asc->SetSubrate(targetAddr, subrateParam);
    asc->SetSubrate(huaweiAddr, subrateParam);
    HILOGI("SetSubrate_002 end");
}

/**
 * @tc.name: StackEventCbk_001
 * @tc.desc: NLSTK_ActmEventCbk StackEventCbk
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, StackEventCbk_001, TestSize.Level1)
{
    HILOGI("StackEventCbk_001 start");
    ASCService *asc = new ASCService();
    SLE_Addr_S addr = {.type = PUBLIC_ADDRESS, .addr = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06}};
    StackEventCbk(&addr, ASC_STACK_CBK_CFG_STREAM, NLSTK_ACTM_SUCCESS, NULL);
    NLSTK_ActmStreamInfo_S info {};
    StackEventCbk(&addr, ASC_STACK_CBK_READ_PROP_FAIL, NLSTK_ACTM_SUCCESS, &info);
    StackEventCbk(&addr, ASC_STACK_CBK_OPEN_STREAM, NLSTK_ACTM_SUCCESS, &info);
    StackEventCbk(&addr, ASC_STACK_CBK_START_STREAM, NLSTK_ACTM_SUCCESS, &info);
    StackEventCbk(&addr, ASC_STACK_CBK_STOP_STREAM, NLSTK_ACTM_SUCCESS, &info);
    StackEventCbk(&addr, ASC_STACK_CBK_RELEASE_STREAM, NLSTK_ACTM_SUCCESS, &info);
    StackEventCbk(&addr, ASC_STACK_CBK_DISCONNECT, NLSTK_ACTM_SUCCESS, &info);
    StackEventCbk(&addr, ASC_STACK_CBK_RELEASE_STREAM, NLSTK_ACTM_SUCCESS, &info);
    StackEventCbk(&addr, ASC_STACK_CBK_RELEASE_STREAM + 100, NLSTK_ACTM_SUCCESS, &info);
    delete asc;
    StackEventCbk(&addr, ASC_STACK_CBK_CREATE_STREAM, NLSTK_ACTM_SUCCESS, &info);
    asc = new ASCService();
    RawAddress device(deviceStr);
    RawAddress coSetDevice(coDeviceStr);
    asc->AddConnectDevices(device);
    asc->AddConnectDevices(coSetDevice);
    ASCMessage event(ASC_SPATIAL_AUDIO_EVT);
    event.dev_ = device.GetAddress();
    event.result_ = true;
    asc->PostEvent(event);
    event.result_ = false;
    asc->PostEvent(event);
    event.whatM = ASC_SPATIAL_AUDIO_HEAD_MOVE_EVT;
    asc->PostEvent(event);
    event.whatM = ASC_UPDATE_VIRTUAL_DEVICE_EVT;
    asc->PostEvent(event);
    asc->UpdateDeviceNature(device);
    event.whatM = ASC_UPDATE_NATURE_EVT;
    asc->PostEvent(event);
    RawAddress reportAddr = asc->GetReportAddr(device);
    asc->activeSinkDevice_ = reportAddr;
    asc->PostEvent(event);
    event.whatM = ASC_UPDATE_NATURE_EVT + 100;
    asc->ProcessAssistEvent(event);
    event.whatM = ASC_UPDATE_VIRTUAL_DEVICE_EVT;
    asc->ProcessAssistEvent(event);
    HILOGI("StackEventCbk_001 end");
}

/**
 * @tc.name: StackPropCbk_001
 * @tc.desc: NLSTK_ActmPropCbk StackPropCbk
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, StackPropCbk_001, TestSize.Level1)
{
    HILOGI("StackPropCbk_001 start");
    ASCService *asc = new ASCService();
    SLE_Addr_S addr = {.type = PUBLIC_ADDRESS, .addr = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06}};
    NLSTK_ActmProp_S props;
    uint8_t pointNum = 1;
    StackPropCbk(&addr, pointNum, &props);
    delete asc;
    StackPropCbk(&addr, pointNum, &props);
    HILOGI("StackPropCbk_001 end");
}

/**
 * @tc.name: StackBitrateCbk_001
 * @tc.desc: StackBitrateCbk
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, StackBitrateCbk_001, TestSize.Level1)
{
    HILOGI("StackBitrateCbk_001 start");
    ASCService *asc = new ASCService();
    NLSTK_ActmBitrateChange_S bitrate {};
    NLSTK_ActmBitrateChange_S *p = nullptr;
    StackBitrateCbk(&bitrate);
    StackBitrateCbk(p);
    delete asc;
    StackBitrateCbk(&bitrate);
    HILOGI("StackBitrateCbk_001 end");
}

/**
 * @tc.name: StackLocationChangeCbk_001
 * @tc.desc:  NLSTK_ActmLocationCbk StackLocationChangeCbk
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, StackLocationChangeCbk_001, TestSize.Level1)
{
    HILOGI("StackLocationChangeCbk_001 start");
    ASCService *asc = new ASCService();
    SLE_Addr_S addrSle = {.type = PUBLIC_ADDRESS, .addr = {0x01, 0x01, 0x01, 0x01, 0x01, 0x01}};
    RawAddress device = RawAddress::ConvertToString(addrSle.addr);
    asc->AddConnectDevices(device);
    ASCMessage event(ASC_STACK_LOCATION_CBK_EVT);
    event.dev_ = device.GetAddress();
    event.isLeft_ = false;
    asc->ProcessStackLocationChangeCbk(event);
    StackLocationChangeCbk(&addrSle, true);
    StackLocationChangeCbk(&addrSle, false);
    delete asc;
    StackLocationChangeCbk(&addrSle, false);
    HILOGI("StackLocationChangeCbk_001 end");
}

/**
 * @tc.name: IsCoSetDeviceStarted_001
 * @tc.desc: IsCoSetDeviceStarted
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, IsCoSetDeviceStarted_001, TestSize.Level1)
{
    HILOGI("IsCoSetDeviceStarted_001 start");
    ASCService *asc = new ASCService();
    RawAddress device(deviceStr);
    RawAddress coSetDevice;
    EXPECT_EQ(false, asc->IsCoSetDeviceStarted(device, coSetDevice));
    coSetDevice = RawAddress(coDeviceStr);
    asc->AddConnectDevices(device);
    asc->AddConnectDevices(coSetDevice);
    asc->SetASCStatus(coSetDevice, NL_SLE_ASC_STARTED);
    EXPECT_EQ(true, asc->IsCoSetDeviceStarted(device, coSetDevice));
    HILOGI("IsCoSetDeviceStarted_001 end");
}

/**
 * @tc.name: GetAscQosmInfo_001
 * @tc.desc: GetAscQosmInfo
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, GetAscQosmInfo_001, TestSize.Level1)
{
    HILOGI("GetAscQosmInfo_001 start");
    ASCService *asc = new ASCService();
    RawAddress device(deviceStr);
    AscQosmInfo qosmInfo {};
    EXPECT_EQ(false, asc->GetAscQosmInfo(device, qosmInfo));
    asc->ProcWhenIOBCreated(device, qosmInfo);
    EXPECT_EQ(true, asc->GetAscQosmInfo(device, qosmInfo));
    HILOGI("GetAscQosmInfo_001 end");
}

/**
 * @tc.name: SyncWhenStartStream_001
 * @tc.desc: SyncWhenStartStream
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, SyncWhenStartStream_001, TestSize.Level1)
{
    HILOGI("SyncWhenStartStream_001 start");
    ASCService *asc = new ASCService();
    RawAddress device(deviceStr);
    RawAddress coSetDevice(coDeviceStr);
    asc->AddConnectDevices(device);
    asc->AddConnectDevices(coSetDevice);
    EXPECT_EQ(true, asc->IsCoSetDeviceExist(device, coSetDevice));
    asc->SetASCStatus(coSetDevice, NL_SLE_ASC_CREATED);
    asc->SetASCStatus(device, NL_SLE_ASC_STARTED);
    asc->SyncWhenStartStream(device, static_cast<AudioStreamType>(1));
    HILOGI("SyncWhenStartStream_001 end");
}

/**
 * @tc.name: ReconfigStream_001
 * @tc.desc: ReconfigStream
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, ReconfigStream_001, TestSize.Level1)
{
    HILOGI("ReconfigStream_001 start");
    ASCService *asc = new ASCService();
    RawAddress device(deviceStr);
    asc->AddConnectDevices(device);
    AudioStreamType streamType = static_cast<AudioStreamType>(1);
    AudioStreamType streamTypeToReconfig = static_cast<AudioStreamType>(128);
    Qos cos = NL_SLE_QOS_1;
    asc->ReconfigStream(device, streamType, streamTypeToReconfig, cos);
    HILOGI("ReconfigStream_001 end");
}

/**
 * @tc.name: StopPlayingExcute_001
 * @tc.desc: StopPlayingExcute
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, StopPlayingExcute_001, TestSize.Level1)
{
    HILOGI("StopPlayingExcute_001 start");
    ASCService *asc = new ASCService();
    RawAddress device(deviceStr);
    asc->AddConnectDevices(device);
    AudioStreamType streamType = static_cast<AudioStreamType>(1);
    asc->StopPlayingExcute(device, streamType);
    HILOGI("StopPlayingExcute_001 end");
}

/**
 * @tc.name: GetAudioDeviceList_001
 * @tc.desc: GetAudioDeviceList
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, GetAudioDeviceList_001, TestSize.Level1)
{
    HILOGI("GetAudioDeviceList_001 start");
    ASCService *asc = new ASCService();
    std::vector<NearlinkRawAddress> devices;
    RawAddress device(deviceStr);
    asc->AddConnectDevices(device);
    asc->GetAudioDeviceList(devices);
    HILOGI("GetAudioDeviceList_001 end");
}

/**
 * @tc.name: GetVirtualAudioDeviceList_001
 * @tc.desc: GetVirtualAudioDeviceList
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, GetVirtualAudioDeviceList_001, TestSize.Level1)
{
    HILOGI("GetVirtualAudioDeviceList_001 start");
    ASCService *asc = new ASCService();
    std::vector<NearlinkRawAddress> devices;
    RawAddress device(deviceStr);
    asc->AddConnectDevices(device);
    asc->GetVirtualAudioDeviceList(devices);
    std::set<std::string> virtualAddrSet;
    asc->connectedVirtualDev_.Insert(deviceStr);
    asc->connectedVirtualDev_.Insert(coDeviceStr);
    asc->GetAllVirtualAudioAddr(virtualAddrSet);
    asc->GetVirtualAudioDeviceList(devices);
    HILOGI("GetVirtualAudioDeviceList_001 end");
}

/**
 * @tc.name: GetAudioDeviceCodecInfo_001
 * @tc.desc: GetAudioDeviceCodecInfo
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, GetAudioDeviceCodecInfo_001, TestSize.Level1)
{
    HILOGI("GetAudioDeviceCodecInfo_001 start");
    ASCService *asc = new ASCService();
    RawAddress device(deviceStr);
    asc->AddConnectDevices(device);
    std::map<AudioStreamType, AudioStreamCodecInfo> info {};
    asc->GetAudioDeviceCodecInfo(static_cast<NearlinkRawAddress>(device), info);
    HILOGI("GetAudioDeviceCodecInfo_001 end");
}

/**
 * @tc.name: StopSink_001
 * @tc.desc: StopSink
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, StopSink_001, TestSize.Level1)
{
    HILOGI("StopSink_001 start");
    ASCService *asc = new ASCService();
    RawAddress device(deviceStr);
    asc->AddConnectDevices(device);
    asc->SetASCStatus(device, NL_SLE_ASC_STARTED);
    std::vector<AscProp> properties {};
    AscProp prop {};
    prop.ability.comm = 1;
    prop.ability.codecNum = 1;
    prop.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    prop.ability.codec[0].companyId = 3;
    prop.ability.codec[0].vendorId = 4;
    prop.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(prop);
    asc->SaveProperty(device, properties);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);
    asc->activeSinkDevice_ = device;
    asc->StopSink();
    HILOGI("StopSink_001 end");
}

/**
 * @tc.name: SetASCStartStreamChangeSubrateFlag_001
 * @tc.desc: SetASCStartStreamChangeSubrateFlag
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, SetASCStartStreamChangeSubrateFlag_001, TestSize.Level1)
{
    HILOGI("SetASCStartStreamChangeSubrateFlag_001 start");
    ASCService *asc = new ASCService();
    RawAddress device(deviceStr);
    asc->AddConnectDevices(device);
    asc->SetASCStartStreamChangeSubrateFlag(device, true);
    asc->SetASCStartStreamChangeSubrateFlag(device, false);
    HILOGI("SetASCStartStreamChangeSubrateFlag_001 end");
}

/**
 * @tc.name: OnPreferredOutputDeviceUpdated_001
 * @tc.desc: OnPreferredOutputDeviceUpdated
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, OnPreferredOutputDeviceUpdated_001, TestSize.Level1)
{
    HILOGI("OnPreferredOutputDeviceUpdated_001 start");
    ASCService *asc = new ASCService();
    auto audioPreferredOutPutDeviceCallback_ =
        std::make_shared<ASCService::SleAudioPreferredOutPutDeviceChangeListener>();
    std::vector<std::shared_ptr<AudioStandard::AudioDeviceDescriptor>> deviceDescriptor;
    auto audioDeviceDescriptor = std::make_shared<AudioStandard::AudioDeviceDescriptor>();
    audioPreferredOutPutDeviceCallback_->OnPreferredOutputDeviceUpdated(deviceDescriptor);
    audioDeviceDescriptor->networkId_ = "test";
    audioDeviceDescriptor->deviceType_ = AudioStandard::DEVICE_TYPE_SPEAKER;
    deviceDescriptor.push_back(audioDeviceDescriptor);
    audioPreferredOutPutDeviceCallback_->OnPreferredOutputDeviceUpdated(deviceDescriptor);
    audioDeviceDescriptor->networkId_ = "LocalDevice";
    audioPreferredOutPutDeviceCallback_->outPutDeviceType_ == AudioStandard::DeviceType::DEVICE_TYPE_NEARLINK;
    audioPreferredOutPutDeviceCallback_->OnPreferredOutputDeviceUpdated(deviceDescriptor);
    audioDeviceDescriptor->deviceType_ =  AudioStandard::DEVICE_TYPE_INVALID;
    audioPreferredOutPutDeviceCallback_->OnPreferredOutputDeviceUpdated(deviceDescriptor);
    HILOGI("OnPreferredOutputDeviceUpdated_001 end");
}

/**
 * @tc.name: OnSpatializationEnabledChangeForAnyDevice_001
 * @tc.desc: OnSpatializationEnabledChangeForAnyDevice
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, OnSpatializationEnabledChangeForAnyDevice_001, TestSize.Level1)
{
    HILOGI("OnSpatializationEnabledChangeForAnyDevice_001 start");
    ASCService *asc = new ASCService();
    ASCService *ascService =
        static_cast<ASCService*>(SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_ASC));
    auto callback_ = std::make_shared<ASCService::SleSpatialAudioModeChangeListener>();
    auto audioDeviceDescriptor = std::make_shared<AudioStandard::AudioDeviceDescriptor>();
    audioDeviceDescriptor->networkId_ = "test";
    audioDeviceDescriptor->deviceType_ = AudioStandard::DEVICE_TYPE_SPEAKER;
    RawAddress device(deviceStr);
    RawAddress coSetDevice(coDeviceStr);
    asc->AddConnectDevices(device);
    asc->AddConnectDevices(coSetDevice);
    audioDeviceDescriptor->macAddress_ = device.GetAddress();
    // 必须这么赋值 asc->activeSinkDevice_ = device 这种赋值不成功, asc对象不同
    if (ascService != nullptr) {
        ascService->activeSinkDevice_ = device;
    }
    asc->IsSpatialAudioModeEnabled(device.GetAddress());
    callback_->OnSpatializationEnabledChangeForAnyDevice(audioDeviceDescriptor, true);
    callback_->OnSpatializationEnabledChangeForAnyDevice(audioDeviceDescriptor, false);
    if (ascService != nullptr) {
        ascService->activeSinkDevice_ = coSetDevice;
    }
    audioDeviceDescriptor->macAddress_ = coSetDevice.GetAddress();
    asc->IsSpatialAudioModeEnabled(coSetDevice.GetAddress());
    callback_->OnSpatializationEnabledChangeForAnyDevice(audioDeviceDescriptor, true);
    callback_->OnSpatializationEnabledChangeForAnyDevice(audioDeviceDescriptor, false);
    audioDeviceDescriptor->macAddress_ = device.GetAddress();
    callback_->OnSpatializationEnabledChangeForAnyDevice(audioDeviceDescriptor, true);
    callback_->OnSpatializationEnabledChangeForAnyDevice(audioDeviceDescriptor, false);
    audioDeviceDescriptor = nullptr;
    callback_->OnSpatializationEnabledChangeForAnyDevice(audioDeviceDescriptor, true);
    HILOGI("OnSpatializationEnabledChangeForAnyDevice_001 end");
}

/**
 * @tc.name: OnHeadTrackingEnabledChangeForAnyDevice_001
 * @tc.desc: OnHeadTrackingEnabledChangeForAnyDevice
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, OnHeadTrackingEnabledChangeForAnyDevice_001, TestSize.Level1)
{
    HILOGI("OnHeadTrackingEnabledChangeForAnyDevice_001 start");
    ASCService *asc = new ASCService();
    ASCService *ascService =
        static_cast<ASCService*>(SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_ASC));
    auto callback_ = std::make_shared<ASCService::SleSpatialAudioHeadTrackingChangeListener>();
    auto audioDeviceDescriptor = std::make_shared<AudioStandard::AudioDeviceDescriptor>();
    audioDeviceDescriptor->networkId_ = "test";
    audioDeviceDescriptor->deviceType_ = AudioStandard::DEVICE_TYPE_SPEAKER;
    RawAddress device(deviceStr);
    RawAddress coSetDevice(coDeviceStr);
    asc->AddConnectDevices(device);
    asc->AddConnectDevices(coSetDevice);
    audioDeviceDescriptor->macAddress_ = device.GetAddress();
    // 必须这么赋值 asc->activeSinkDevice_ = device 这种赋值不成功, asc对象不同
    if (ascService != nullptr) {
        ascService->activeSinkDevice_ = device;
    }
    asc->IsSpatialAudioHeadTrackingEnabled(device.GetAddress());
    callback_->OnHeadTrackingEnabledChangeForAnyDevice(audioDeviceDescriptor, true);
    callback_->OnHeadTrackingEnabledChangeForAnyDevice(audioDeviceDescriptor, false);
    if (ascService != nullptr) {
        ascService->activeSinkDevice_ = coSetDevice;
    }
    audioDeviceDescriptor->macAddress_ = coSetDevice.GetAddress();
    asc->IsSpatialAudioHeadTrackingEnabled(coSetDevice.GetAddress());
    callback_->OnHeadTrackingEnabledChangeForAnyDevice(audioDeviceDescriptor, true);
    callback_->OnHeadTrackingEnabledChangeForAnyDevice(audioDeviceDescriptor, false);
    audioDeviceDescriptor->macAddress_ = device.GetAddress();
    callback_->OnHeadTrackingEnabledChangeForAnyDevice(audioDeviceDescriptor, true);
    callback_->OnHeadTrackingEnabledChangeForAnyDevice(audioDeviceDescriptor, false);
    audioDeviceDescriptor = nullptr;
    callback_->OnHeadTrackingEnabledChangeForAnyDevice(audioDeviceDescriptor, true);
    HILOGI("OnHeadTrackingEnabledChangeForAnyDevice_001 end");
}

/**
 * @tc.name: UnregisterAudioPreferredOutPutDeviceChangeListener_001
 * @tc.desc: UnregisterAudioPreferredOutPutDeviceChangeListener
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, UnregisterAudioPreferredOutPutDeviceChangeListener_001, TestSize.Level1)
{
    HILOGI("UnregisterAudioPreferredOutPutDeviceChangeListener_001 start");
    ASCService *asc = new ASCService();
    asc->RegisterAudioPreferredOutPutDeviceChangeListener();
    asc->UnregisterAudioPreferredOutPutDeviceChangeListener();
    HILOGI("UnregisterAudioPreferredOutPutDeviceChangeListener_001 end");
}

/**
 * @tc.name: SerialManagerSubrate_001
 * @tc.desc: SerialManagerSubrate
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, SerialManagerSubrate_001, TestSize.Level1)
{
    HILOGI("SerialManagerSubrate_001 start");
    ASCService *asc = new ASCService();
    RawAddress device(deviceStr);
    asc->AddConnectDevices(device);
    bool needConfigStream = true;
    asc->SetASCStartStreamChangeSubrateFlag(device, true);
    asc->SerialManagerSubrate(needConfigStream, device, 0);
    HILOGI("SerialManagerSubrate_001 end");
}

/**
 * @tc.name: FillActmImgEncpParam_001
 * @tc.desc: FillActmImgEncpParam
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, FillActmImgEncpParam_001, TestSize.Level1)
{
    HILOGI("FillActmImgEncpParam_001 start");
    ASCService *asc = new ASCService();
    RawAddress device(deviceStr);
    RawAddress coSetDevice(coDeviceStr);
    asc->AddConnectDevices(device);
    asc->AddConnectDevices(coSetDevice);
    uint32_t testGroupId = 0x12345678;
    CdsmService *cdsmService = CdsmService::GetService();
    auto cdsmInfo = std::make_shared<CdsmInfo>(testGroupId, device.GetAddress());
    cdsmInfo->CdsmAddMemberInfo(device.GetAddress(), 1);
    cdsmInfo->CdsmAddMemberInfo(coSetDevice.GetAddress(), 1);
    cdsmService->cdsmList_.EnsureInsert(testGroupId, cdsmInfo);
    Qos cos = NL_SLE_QOS_6;
    NLSTK_ActmConfigParam_S cfgPara {};
    asc->FillActmImgEncpParam(device, cos, cfgPara);
    HILOGI("FillActmImgEncpParam_001 end");
}

/**
 * @tc.name: SelectCodecSampleRatePolicyInDualRec_001
 * @tc.desc: SelectCodecSampleRatePolicyInDualRec
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, SelectCodecSampleRatePolicyInDualRec_001, TestSize.Level1)
{
    HILOGI("SelectCodecSampleRatePolicyInDualRec_001 start");
    ASCService *asc = new ASCService();
    AscCodecIdKey codec = ASC_L2HC_CODEC;
    uint16_t peerSampleRate = ASC_SAMPLE_RATE_48KHZ;
    asc->SelectCodecSampleRatePolicyInDualRec(codec, peerSampleRate);
    peerSampleRate = ASC_SAMPLE_RATE_96KHZ;
    asc->SelectCodecSampleRatePolicyInDualRec(codec, peerSampleRate);
    HILOGI("SelectCodecSampleRatePolicyInDualRec_001 end");
}

/**
 * @tc.name: SelectCodecBitDepthPolicyInDualRec_001
 * @tc.desc: SelectCodecBitDepthPolicyInDualRec
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, SelectCodecBitDepthPolicyInDualRec_001, TestSize.Level1)
{
    HILOGI("SelectCodecBitDepthPolicyInDualRec_001 start");
    ASCService *asc = new ASCService();
    AscCodecIdKey codec = ASC_L2HC_CODEC;
    uint8_t inDepth = ASC_SAMPLE_DEPTH_24BIT;
    asc->SelectCodecBitDepthPolicyInDualRec(codec, inDepth);
    inDepth = ASC_SAMPLE_DEPTH_16BIT;
    asc->SelectCodecBitDepthPolicyInDualRec(codec, inDepth);
    HILOGI("SelectCodecBitDepthPolicyInDualRec_001 end");
}

/**
 * @tc.name: SelectCodecChannelPolicyInDualRec_001
 * @tc.desc: SelectCodecChannelPolicyInDualRec
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, SelectCodecChannelPolicyInDualRec_001, TestSize.Level1)
{
    HILOGI("SelectCodecChannelPolicyInDualRec_001 start");
    ASCService *asc = new ASCService();
    AscCodecIdKey codec = ASC_L2HC_CODEC;
    uint16_t peerChannel = ASC_CHANNEL_SINGLE;
    asc->SelectCodecChannelPolicyInDualRec(codec, peerChannel);
    peerChannel = 0;
    asc->SelectCodecChannelPolicyInDualRec(codec, peerChannel);
    HILOGI("SelectCodecChannelPolicyInDualRec_001 end");
}

/**
 * @tc.name: GetLocalCodecBitDepthCap_001
 * @tc.desc: GetLocalCodecBitDepthCap
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, GetLocalCodecBitDepthCap_001, TestSize.Level1)
{
    HILOGI("GetLocalCodecBitDepthCap_001 start");
    ASCService *asc = new ASCService();
    AscCodecIdKey codec {};
    EXPECT_EQ(0, asc->GetLocalCodecBitDepthCap(codec));
    codec = ASC_L2HC_5_0_CODEC;
    EXPECT_EQ(ASC_L2HC_5_0_BIT_DEPTH_CAP, asc->GetLocalCodecBitDepthCap(codec));
    codec = ASC_L2HC_4_0_CODEC;
    EXPECT_EQ(ASC_L2HC_4_0_BIT_DEPTH_CAP, asc->GetLocalCodecBitDepthCap(codec));
    codec = ASC_L2HC_CODEC;
    EXPECT_EQ(ASC_L2HC_BIT_DEPTH_CAP, asc->GetLocalCodecBitDepthCap(codec));
    codec = ASC_L2HC_VOICE_CODEC;
    EXPECT_EQ(ASC_L2HC_VOICE_BIT_DEPTH_CAP, asc->GetLocalCodecBitDepthCap(codec));
    HILOGI("GetLocalCodecBitDepthCap_001 end");
}

/**
 * @tc.name: GetLocalCodecChannelCap_001
 * @tc.desc: GetLocalCodecChannelCap
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, GetLocalCodecChannelCap_001, TestSize.Level1)
{
    HILOGI("GetLocalCodecChannelCap_001 start");
    ASCService *asc = new ASCService();
    AscCodecIdKey codec {};
    EXPECT_EQ(0, asc->GetLocalCodecChannelCap(codec));
    codec = ASC_L2HC_5_0_CODEC;
    EXPECT_EQ(ASC_L2HC_5_0_CHANNEL_CAP, asc->GetLocalCodecChannelCap(codec));
    codec = ASC_L2HC_4_0_CODEC;
    EXPECT_EQ(ASC_L2HC_4_0_CHANNEL_CAP, asc->GetLocalCodecChannelCap(codec));
    codec = ASC_L2HC_CODEC;
    EXPECT_EQ(ASC_L2HC_CHANNEL_CAP, asc->GetLocalCodecChannelCap(codec));
    codec = ASC_L2HC_VOICE_CODEC;
    EXPECT_EQ(ASC_L2HC_VOICE_CHANNEL_CAP, asc->GetLocalCodecChannelCap(codec));
    HILOGI("GetLocalCodecChannelCap_001 end");
}

/**
 * @tc.name: GetLocalCodecSampleRateCap_001
 * @tc.desc: GetLocalCodecSampleRateCap
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, GetLocalCodecSampleRateCap_001, TestSize.Level1)
{
    HILOGI("GetLocalCodecSampleRateCap_001 start");
    ASCService *asc = new ASCService();
    AscCodecIdKey codec {};
    EXPECT_EQ(0, asc->GetLocalCodecSampleRateCap(codec));
    codec = ASC_L2HC_5_0_CODEC;
    EXPECT_EQ(ASC_L2HC_5_0_SAMPLE_RATE_CAP, asc->GetLocalCodecSampleRateCap(codec));
    codec = ASC_L2HC_4_0_CODEC;
    EXPECT_EQ(ASC_L2HC_4_0_SAMPLE_RATE_CAP, asc->GetLocalCodecSampleRateCap(codec));
    codec = ASC_L2HC_CODEC;
    EXPECT_EQ(ASC_L2HC_SAMPLE_RATE_CAP, asc->GetLocalCodecSampleRateCap(codec));
    codec = ASC_L2HC_VOICE_CODEC;
    EXPECT_EQ(ASC_L2HC_VOICE_SAMPLE_RATE_CAP, asc->GetLocalCodecSampleRateCap(codec));
    HILOGI("GetLocalCodecSampleRateCap_001 end");
}

/**
 * @tc.name: GetDspL2hcVersion_001
 * @tc.desc: GetDspL2hcVersion
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, GetDspL2hcVersion_001, TestSize.Level1)
{
    HILOGI("GetDspL2hcVersion_001 start");
    ASCService *asc = new ASCService();
    AscQosmInfo info;
    (void)memset_s(&info, sizeof(AscQosmInfo), 0x0, sizeof(AscQosmInfo));
    EXPECT_EQ(DSP_L2HC_STD_VERSION, asc->GetDspL2hcVersion(info));
    // ASC_L2HC_CODEC
    info.codecId = ASC_L2HC_CODEC.codecId;
    info.companyId = ASC_L2HC_CODEC.companyId;
    info.vendorId = ASC_L2HC_CODEC.vendorId;
    info.version = ASC_L2HC_VERSION_STANDARD;
    EXPECT_EQ(DSP_L2HC_STD_VERSION, asc->GetDspL2hcVersion(info));
    // ASC_L2HC_5_0_CODEC
    info.codecId = ASC_L2HC_5_0_CODEC.codecId;
    info.companyId = ASC_L2HC_5_0_CODEC.companyId;
    info.vendorId = ASC_L2HC_5_0_CODEC.vendorId;
    info.version = ASC_L2HC_5_VERSION;
    EXPECT_EQ(DSP_L2HC_5_0_VERSION, asc->GetDspL2hcVersion(info));
    // ASC_L2HC_4_0_CODEC
    info.codecId = ASC_L2HC_4_0_CODEC.codecId;
    info.companyId = ASC_L2HC_4_0_CODEC.companyId;
    info.vendorId = ASC_L2HC_4_0_CODEC.vendorId;
    info.version = ASC_L2HC_4_0_HISI_VERSION;
    EXPECT_EQ(DSP_L2HC_4_0_VERSION, asc->GetDspL2hcVersion(info));
    // ASC_L2HC_VOICE_CODEC
    info.codecId = ASC_L2HC_VOICE_CODEC.codecId;
    info.companyId = ASC_L2HC_VOICE_CODEC.companyId;
    info.vendorId = ASC_L2HC_VOICE_CODEC.vendorId;
    info.version = ASC_L2HC_VERSION_STANDARD;
    EXPECT_EQ(DSP_L2HC_VOICE_VERSION, asc->GetDspL2hcVersion(info));
    HILOGI("GetDspL2hcVersion_001 end");
}

/**
 * @tc.name: SyncWhenConnect_001
 * @tc.desc: SyncWhenConnect
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, SyncWhenConnect_001, TestSize.Level1)
{
    HILOGI("SyncWhenConnect_001 start");
    ASCService *asc = new ASCService();
    RawAddress device(deviceStr);
    RawAddress coSetDevice(coDeviceStr);
    asc->AddConnectDevices(device);
    asc->AddConnectDevices(coSetDevice);
    EXPECT_EQ(true, asc->IsCoSetDeviceExist(device, coSetDevice));
    std::list<AudioStreamType>& startedStreamList1 = asc->GetStartedStreamList(device);
    startedStreamList1.emplace_back(AUDIO_STREAM_MUSIC);
    std::list<AudioStreamType>& startedStreamList2 = asc->GetStartedStreamList(coSetDevice);
    startedStreamList2.emplace_back(AUDIO_STREAM_MUSIC);
    asc->SetStopDelayingFlag(device, false);
    EXPECT_EQ(false, asc->IsStopDelaying(device));
    asc->activeSinkDevice_ = device;
    asc->SetASCStatus(coSetDevice, NL_SLE_ASC_STARTED);
    asc->SetASCStatus(device, NL_SLE_ASC_STARTED);
    asc->SyncWhenConnect(device);
    asc->SetStopDelayingFlag(asc->activeSinkDevice_, false);
    HILOGI("SyncWhenConnect_001 end");
}

/**
 * @tc.name: SyncWhenDisconnect_001
 * @tc.desc: SyncWhenDisconnect
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, SyncWhenDisconnect_001, TestSize.Level1)
{
    HILOGI("SyncWhenDisconnect_001 start");
    ASCService *asc = new ASCService();
    RawAddress device(deviceStr);
    RawAddress coSetDevice(coDeviceStr);
    asc->AddConnectDevices(device);
    asc->AddConnectDevices(coSetDevice);
    EXPECT_EQ(true, asc->IsCoSetDeviceExist(device, coSetDevice));
    asc->SetSyncFlag(device, true);
    asc->SetASCStatus(coSetDevice, NL_SLE_ASC_CONNECTED);
    asc->SyncWhenDisconnect(device, NL_SLE_ASC_STARTED);
    HILOGI("SyncWhenDisconnect_001 end");
}

/**
 * @tc.name: ChangeBpsRange_001
 * @tc.desc: ChangeBpsRange
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, ChangeBpsRange_001, TestSize.Level1)
{
    HILOGI("ChangeBpsRange_001 start");
    ASCService *asc = new ASCService();
    RawAddress device(deviceStr);
    RawAddress coSetDevice(coDeviceStr);
    asc->AddConnectDevices(device);
    asc->AddConnectDevices(coSetDevice);
    asc->SetASCStatus(device, NL_SLE_ASC_STARTED);
    asc->SetASCStatus(coSetDevice, NL_SLE_ASC_STARTED);
    asc->activeSinkDevice_ = device;
    QosM::GetInstance().SetCos(device, NL_SLE_QOS_1);
    QosM::GetInstance().SetCos(coSetDevice, NL_SLE_QOS_1);
    asc->ChangeBpsRange();
    QosM::GetInstance().SetCos(device, NL_SLE_QOS_3);
    QosM::GetInstance().SetCos(coSetDevice, NL_SLE_QOS_3);
    asc->SetBpsRange(device, 12345);
    asc->ChangeBpsRange();
    asc->SetBpsRange(device, 0);
    asc->ChangeBpsRange();
    HILOGI("ChangeBpsRange_001 end");
}

/**
 * @tc.name: SetStopDelayingFlag_001
 * @tc.desc: SetStopDelayingFlag
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, SetStopDelayingFlag_001, TestSize.Level1)
{
    HILOGI("SetStopDelayingFlag_001 start");
    ASCService *asc = new ASCService();
    RawAddress device(deviceStr);
    asc->AddConnectDevices(device);
    asc->SetStopDelayingFlag(device, true);
    HILOGI("SetStopDelayingFlag_001 end");
}

/**
 * @tc.name: IsStartingOrStopingMemberExist_001
 * @tc.desc: IsStartingOrStopingMemberExist
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, IsStartingOrStopingMemberExist_001, TestSize.Level1)
{
    HILOGI("IsStartingOrStopingMemberExist_001 start");
    ASCService *asc = new ASCService();
    RawAddress device(deviceStr);
    RawAddress coDevice(coDeviceStr);
    asc->AddConnectDevices(device);
    asc->AddConnectDevices(coDevice);
    asc->SetASCStatus(device, NL_SLE_ASC_STARTED);
    asc->SetASCStatus(coDevice, NL_SLE_ASC_STARTED);
    asc->IsStartingOrStopingMemberExist(device);
    asc->SetASCStatus(device, NL_SLE_ASC_CONFIGURING);
    asc->SetASCStatus(coDevice, NL_SLE_ASC_CONFIGURING);
    asc->IsStartingOrStopingMemberExist(device);
    HILOGI("IsStartingOrStopingMemberExist_001 end");
}

/**
 * @tc.name: ProcessDisConnectEvent_001
 * @tc.desc: ProcessDisConnectEvent
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, ProcessDisConnectEvent_001, TestSize.Level1)
{
    HILOGI("ProcessDisConnectEvent_001 start");
    ASCService *asc = new ASCService();
    RawAddress device(deviceStr);
    asc->InitDisconnProcTable();
    asc->SetASCStatus(device, NL_SLE_ASC_BUTT);
    ASCMessage event(ASC_DISCONNECT_START_EVT);
    event.dev_ = device.GetAddress();
    asc->ProcessDisConnectEvent(event);
    asc->InitDisconnProcTable();
    asc->SetASCStatus(device, NL_SLE_ASC_CONFIGURING);
    asc->ProcessDisConnectEvent(event);
    HILOGI("ProcessDisConnectEvent_001 end");
}

/**
 * @tc.name: IsStartAtLowBps_001
 * @tc.desc: IsStartAtLowBps
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, IsStartAtLowBps_001, TestSize.Level1)
{
    HILOGI("IsStartAtLowBps_001 start");
    ASCService *asc = new ASCService();
    time_t timeStamp = time(nullptr);
    asc->formerPlayRecord_.timeStamp = timeStamp;
    asc->formerPlayRecord_.autoRateBpsBit = L2HC_BPS_S_160_BIT;
    asc->IsStartAtLowBps(asc->formerPlayRecord_, NL_SLE_QOS_1);
    HILOGI("IsStartAtLowBps_001 end");
}

/**
 * @tc.name: StartPlayingExcute_001
 * @tc.desc: StartPlayingExcute
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, StartPlayingExcute_001, TestSize.Level1)
{
    HILOGI("StartPlayingExcute_001 start");
    ASCService *asc = new ASCService();
    RawAddress device = RawAddress(deviceStr);
    AudioStreamType streamType = static_cast<AudioStreamType>(AUDIO_STREAM_GAME);
    AudioStreamType stopDelayingStreamType = static_cast<AudioStreamType>(AUDIO_STREAM_VOIP);
    asc->AddConnectDevices(device);
    asc->formerSinkDevice_ = device;
    asc->SetStopDelayingFlag(asc->formerSinkDevice_, true);
    EXPECT_EQ(NL_NO_ERROR, asc->StartPlaying(device, streamType, true));
    asc->StartPlayingExcute(device, streamType, true, stopDelayingStreamType);
    asc->SetStopDelayingFlag(asc->formerSinkDevice_, false);
    RawAddress reportAddr = asc->GetReportAddr(device);
    asc->SetStopDelayingFlag(reportAddr, true);
    asc->StartPlayingExcute(device, streamType, true, stopDelayingStreamType);
    HILOGI("StartPlayingExcute_001 end");
}

/**
 * @tc.name: SetAutoRateBps_001
 * @tc.desc: SetAutoRateBps
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, SetAutoRateBps_001, TestSize.Level1)
{
    HILOGI("SetAutoRateBps_001 start");
    ASCService *asc = new ASCService();
    RawAddress device(deviceStr);
    RawAddress coDevice(coDeviceStr);
    asc->SetAutoRateBps(device, 123);
    asc->formerPlayRecord_.addr = device;
    asc->AddConnectDevices(device);
    asc->AddConnectDevices(coDevice);
    asc->SetASCStatus(device, NL_SLE_ASC_STARTED);
    asc->SetASCStatus(coDevice, NL_SLE_ASC_STARTED);
    asc->SetAutoRateBps(device, 123);
    HILOGI("SetAutoRateBps_001 end");
}

/**
 * @tc.name: DelayStop_001
 * @tc.desc: DelayStop
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, DelayStop_001, TestSize.Level1)
{
    HILOGI("DelayStop_001 start");
    ASCService *asc = new ASCService();
    RawAddress device(deviceStr);
    RawAddress coDevice(coDeviceStr);
    asc->AddConnectDevices(device);
    asc->AddConnectDevices(coDevice);
    AudioStreamType streamType = static_cast<AudioStreamType>(AUDIO_STREAM_GAME);
    asc->SetStopDelayingFlag(device, true);
    asc->SetStopDelayingFlag(coDevice, true);
    asc->DelayStop(device, streamType);
    asc->SetStopDelayingFlag(device, false);
    asc->SetStopDelayingFlag(coDevice, false);
    asc->DelayStop(device, streamType);
    HILOGI("DelayStop_001 end");
}

/**
 * @tc.name: SendPlayOrPauseIfNeed_001
 * @tc.desc: SendPlayOrPauseIfNeed
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, SendPlayOrPauseIfNeed_001, TestSize.Level1)
{
    HILOGI("SendPlayOrPauseIfNeed_001 start");
    ASCService *asc = new ASCService();
    RawAddress device(deviceStr);
    asc->AddConnectDevices(device);
    asc->SendPlayOrPauseIfNeed(device);
    asc->playOrPauseDevice_.EnsureInsert(device.GetAddress(), 1);
    asc->SendPlayOrPauseIfNeed(device);
    asc->playOrPauseDevice_.EnsureInsert(device.GetAddress(), 0);
    asc->SendPlayOrPauseIfNeed(device);
    HILOGI("SendPlayOrPauseIfNeed_001 end");
}

/**
 * @tc.name: StopConnRptDelayTimer_001
 * @tc.desc: StopConnRptDelayTimer
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, StopConnRptDelayTimer_001, TestSize.Level1)
{
    HILOGI("StopConnRptDelayTimer_001 start");
    ASCService *asc = new ASCService();
    RawAddress device(deviceStr);
    asc->AddConnectDevices(device);
    asc->SetConnRptDelayingFlag(device, false);
    asc->StopConnRptDelayTimer(device);
    asc->SetConnRptDelayingFlag(device, true);
    asc->connRptDelayTimer_[device.GetAddress()] = std::make_shared<NearlinkTimer>([this]() -> void {});
    asc->StopConnRptDelayTimer(device);
    asc->connRptDelayTimer_[device.GetAddress()] = nullptr;
    asc->SetConnRptDelayingFlag(device, true);
    asc->StopConnRptDelayTimer(device);
    HILOGI("StopConnRptDelayTimer_001 end");
}

/**
 * @tc.name: TransferProperty_001
 * @tc.desc: TransferProperty
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, TransferProperty_001, TestSize.Level1)
{
    HILOGI("TransferProperty_001 start");
    std::vector<AscProp> properties {};
    AscProp ascProp {};
    ascProp.ability.comm = 1;
    ascProp.ability.codecNum = 1;
    ascProp.ability.codec[0].codecId = ASC_L2HC_5_0_CODEC.codecId;
    ascProp.ability.codec[0].companyId = 3;
    ascProp.ability.codec[0].vendorId = 4;
    ascProp.ability.codec[0].param.l2hcParam.version = 5;
    properties.emplace_back(ascProp);
    NLSTK_ActmProp_S pro {};
    pro.ability.codecNum = 1;
    pro.ability.comm = 1;
    NLSTK_ActmCodecParam_S codec {};
    codec.codecId = ASC_CODEC_ID_PCM;
    codec.companyId = ASC_COMPANY_ID_STANDARD;
    codec.vendorId = ASC_VENDOR_ID_STANDARD;
    pro.ability.codec = &codec;
    RawAddress device(deviceStr);
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_1);
    uint8_t num = 1;
    ASCUtils::TransferProperty(device, num, &pro, properties);
    codec.codecId= ASC_CODEC_ID_PCM;
    ASCUtils::TransferProperty(device, num, &pro, properties);
    codec.codecId = ASC_CODEC_ID_L2HC;
    ASCUtils::TransferProperty(device, num, &pro, properties);
    codec.codecId = ASC_CODEC_ID_L2HC_PRI;
    ASCUtils::TransferProperty(device, num, &pro, properties);
    codec.codecId = ASC_CODEC_ID_L2HC_VOICE;
    ASCUtils::TransferProperty(device, num, &pro, properties);
    HILOGI("TransferProperty_001 end");
}

/**
 * @tc.name: IsStopImmediately_001
 * @tc.desc: IsStopImmediately
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, IsStopImmediately_001, TestSize.Level1)
{
    HILOGI("IsStopImmediately_001 start");
    ASCService *asc = new ASCService();
    RawAddress device(deviceStr);
    asc->AddConnectDevices(device);
    QosM::GetInstance().IsAudioSceneCall();
    QosM::GetInstance().SetCos(device, NL_SLE_QOS_1);
    QosM::GetInstance().IsStopImmediately(device, NL_SLE_QOS_NONE);
    QosM::GetInstance().SetCos(device, NL_SLE_QOS_1);
    QosM::GetInstance().IsStopImmediately(device, NL_SLE_QOS_1);
    QosM::GetInstance().SetCos(device, NL_SLE_QOS_5);
    QosM::GetInstance().IsStopImmediately(device, NL_SLE_QOS_NONE);
    HILOGI("IsStopImmediately_001 end");
}

/**
 * @tc.name: IsUpExist_001
 * @tc.desc: IsUpExist
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, IsUpExist_001, TestSize.Level1)
{
    HILOGI("IsUpExist_001 start");
    ASCService *asc = new ASCService();
    RawAddress device(deviceStr);
    QosM::GetInstance().deviceQosmMap_.clear();
    EXPECT_EQ(false, QosM::GetInstance().IsUpExist(device));
    // 创建并插入map gosList为空
    DevQosmStru qosm {};
    qosm.cos = NL_SLE_QOS_NONE;
    QosM::GetInstance().deviceQosmMap_[device.GetAddress()] = qosm;
    EXPECT_EQ(false, QosM::GetInstance().IsUpExist(device));
    // 创建并插入map gosList不为空
    std::list<Qos> gosList;
    qosm.cos = NL_SLE_QOS_NONE;
    gosList.emplace_back(NL_SLE_QOS_1);
    qosm.gos = gosList;
    QosM::GetInstance().deviceQosmMap_[device.GetAddress()] = qosm;
    EXPECT_EQ(false, QosM::GetInstance().IsUpExist(device));
    // 创建并插入map gosList不为空
    qosm.cos = NL_SLE_QOS_NONE;
    gosList.emplace_back(NL_SLE_QOS_3);
    qosm.gos = gosList;
    QosM::GetInstance().deviceQosmMap_[device.GetAddress()] = qosm;
    EXPECT_EQ(true, QosM::GetInstance().IsUpExist(device));
    HILOGI("IsUpExist_001 end");
}

/**
 * @tc.name: IsSingleDownExist_001
 * @tc.desc: IsSingleDownExist
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, IsSingleDownExist_001, TestSize.Level1)
{
    HILOGI("IsSingleDownExist_001 start");
    ASCService *asc = new ASCService();
    RawAddress device(deviceStr);
    QosM::GetInstance().deviceQosmMap_.clear();
    EXPECT_EQ(false, QosM::GetInstance().IsSingleDownExist(device));
    // 创建并插入map gosList为空
    DevQosmStru qosm {};
    qosm.cos = NL_SLE_QOS_NONE;
    QosM::GetInstance().deviceQosmMap_[device.GetAddress()] = qosm;
    EXPECT_EQ(false, QosM::GetInstance().IsSingleDownExist(device));
    // 创建并插入map gosList不为空
    qosm.cos = NL_SLE_QOS_NONE;
    std::list<Qos> gosList;
    gosList.emplace_back(NL_SLE_QOS_3);
    qosm.gos = gosList;
    QosM::GetInstance().deviceQosmMap_[device.GetAddress()] = qosm;
    EXPECT_EQ(false, QosM::GetInstance().IsSingleDownExist(device));
    // 创建并插入map gosList不为空
    qosm.cos = NL_SLE_QOS_NONE;
    gosList.emplace_back(NL_SLE_QOS_1);
    qosm.gos = gosList;
    QosM::GetInstance().deviceQosmMap_[device.GetAddress()] = qosm;
    EXPECT_EQ(true, QosM::GetInstance().IsSingleDownExist(device));
    HILOGI("IsSingleDownExist_001 end");
}

/**
 * @tc.name: DeleteQos_001
 * @tc.desc: DeleteQos
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, DeleteQos_001, TestSize.Level1)
{
    HILOGI("DeleteQos_001 start");
    ASCService *asc = new ASCService();
    RawAddress device(deviceStr);
    Qos qos = NL_SLE_QOS_1;
    bool isStopStream = false;
    bool isStopImmediately = false;
    EXPECT_EQ(false, QosM::GetInstance().DeleteQos(device, qos, isStopStream, isStopImmediately));
    qos = NL_SLE_QOS_1;
    QosM::GetInstance().deviceQosmMap_.clear();
    QosM::GetInstance().CheckAndDeleteQos(device, qos);
    QosM::GetInstance().CalculateNos(device);
    QosM::GetInstance().IsQosExist(device, qos);
    QosM::GetInstance().IsQos4Exist(device);
    EXPECT_EQ(false, QosM::GetInstance().DeleteQos(device, qos, isStopStream, isStopImmediately));
    // 创建并插入map gosList为空
    DevQosmStru qosm {};
    qosm.cos = NL_SLE_QOS_NONE;
    QosM::GetInstance().deviceQosmMap_[device.GetAddress()] = qosm;
    QosM::GetInstance().CheckAndDeleteQos(device, qos);
    QosM::GetInstance().CalculateNos(device);
    QosM::GetInstance().IsQosExist(device, qos);
    QosM::GetInstance().IsQos4Exist(device);
    QosM::GetInstance().ClearQosM(device);
    EXPECT_EQ(false, QosM::GetInstance().DeleteQos(device, qos, isStopStream, isStopImmediately));
    HILOGI("DeleteQos_001 end");
}

/**
 * @tc.name: SyncQos_001
 * @tc.desc: SyncQos
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, SyncQos_001, TestSize.Level1)
{
    HILOGI("SyncQos_001 start");
    ASCService *asc = new ASCService();
    RawAddress device(deviceStr);
    RawAddress coDevice(coDeviceStr);
    asc->AddConnectDevices(device);
    asc->AddConnectDevices(coDevice);
    QosM::GetInstance().deviceQosmMap_.clear();
    QosM::GetInstance().SyncQos(device, coDevice);
    // 创建并插入map gosList为空
    DevQosmStru qosm {};
    std::list<Qos> gosList;
    qosm.cos = NL_SLE_QOS_NONE;
    QosM::GetInstance().deviceQosmMap_[coDevice.GetAddress()] = qosm;
    QosM::GetInstance().SyncQos(device, coDevice);
    // 创建并插入map gosList不为空
    qosm.cos = NL_SLE_QOS_NONE;
    gosList.emplace_back(NL_SLE_QOS_1);
    qosm.gos = gosList;
    QosM::GetInstance().deviceQosmMap_[coDevice.GetAddress()] = qosm;
    HILOGI("SyncQos_001 end");
}

/**
 * @tc.name: IsNeedTransferForColAudio_001
 * @tc.desc: IsNeedTransferForColAudio when col audio enabled
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, IsNeedTransferForColAudio_001, TestSize.Level1)
{
    HILOGI("IsNeedTransferForColAudio_001 start");
    ASCService *asc = new ASCService();
    asc->isColAudioEnabled_ = true;
    EXPECT_TRUE(asc->IsNeedTransferForColAudio(AUDIO_STREAM_NONE));
    EXPECT_TRUE(asc->IsNeedTransferForColAudio(AUDIO_STREAM_MUSIC));
    EXPECT_TRUE(asc->IsNeedTransferForColAudio(AUDIO_STREAM_GAME));
    EXPECT_TRUE(asc->IsNeedTransferForColAudio(AUDIO_STREAM_VIDEO));
    EXPECT_TRUE(asc->IsNeedTransferForColAudio(AUDIO_STREAM_ALERT));
    EXPECT_TRUE(asc->IsNeedTransferForColAudio(AUDIO_STREAM_GUID));
    EXPECT_TRUE(asc->IsNeedTransferForColAudio(AUDIO_STREAM_ALARM));
    EXPECT_FALSE(asc->IsNeedTransferForColAudio(AUDIO_STREAM_VOICE_CALL));
    EXPECT_FALSE(asc->IsNeedTransferForColAudio(AUDIO_STREAM_RECORD));
    EXPECT_FALSE(asc->IsNeedTransferForColAudio(AUDIO_STREAM_VOICE_ASSISTANT));
    EXPECT_FALSE(asc->IsNeedTransferForColAudio(AUDIO_STREAM_RING));
    EXPECT_FALSE(asc->IsNeedTransferForColAudio(AUDIO_STREAM_VOIP));
    delete asc;
    HILOGI("IsNeedTransferForColAudio_001 end");
}

/**
 * @tc.name: IsNeedTransferForColAudio_002
 * @tc.desc: IsNeedTransferForColAudio when col audio disabled
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, IsNeedTransferForColAudio_002, TestSize.Level1)
{
    HILOGI("IsNeedTransferForColAudio_002 start");
    ASCService *asc = new ASCService();
    asc->isColAudioEnabled_ = false;
    EXPECT_FALSE(asc->IsNeedTransferForColAudio(AUDIO_STREAM_MUSIC));
    EXPECT_FALSE(asc->IsNeedTransferForColAudio(AUDIO_STREAM_GAME));
    EXPECT_FALSE(asc->IsNeedTransferForColAudio(AUDIO_STREAM_VIDEO));
    delete asc;
    HILOGI("IsNeedTransferForColAudio_002 end");
}

/**
 * @tc.name: IsColAudioStreamExist_001
 * @tc.desc: IsColAudioStreamExist when video stream exists without qos7
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, IsColAudioStreamExist_001, TestSize.Level1)
{
    HILOGI("IsColAudioStreamExist_001 start");
    ASCService *asc = new ASCService();
    RawAddress device(deviceStr);
    asc->AddConnectDevices(device);
    QosM::GetInstance().deviceQosmMap_.clear();
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_10);
    std::list<AudioStreamType>& startedStreamList = asc->GetStartedStreamList(device);
    startedStreamList.emplace_back(AUDIO_STREAM_VIDEO);
    EXPECT_TRUE(asc->IsColAudioStreamExist(device));
    delete asc;
    HILOGI("IsColAudioStreamExist_001 end");
}

/**
 * @tc.name: IsColAudioStreamExist_002
 * @tc.desc: IsColAudioStreamExist when qos7 exists
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, IsColAudioStreamExist_002, TestSize.Level1)
{
    HILOGI("IsColAudioStreamExist_002 start");
    ASCService *asc = new ASCService();
    RawAddress device(deviceStr);
    asc->AddConnectDevices(device);
    QosM::GetInstance().deviceQosmMap_.clear();
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_7);
    std::list<AudioStreamType>& startedStreamList = asc->GetStartedStreamList(device);
    startedStreamList.emplace_back(AUDIO_STREAM_VIDEO);
    EXPECT_FALSE(asc->IsColAudioStreamExist(device));
    delete asc;
    HILOGI("IsColAudioStreamExist_002 end");
}

/**
 * @tc.name: IsColAudioStreamExist_003
 * @tc.desc: IsColAudioStreamExist when no video stream
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, IsColAudioStreamExist_003, TestSize.Level1)
{
    HILOGI("IsColAudioStreamExist_003 start");
    ASCService *asc = new ASCService();
    RawAddress device(deviceStr);
    asc->AddConnectDevices(device);
    QosM::GetInstance().deviceQosmMap_.clear();
    EXPECT_FALSE(asc->IsColAudioStreamExist(device));
    delete asc;
    HILOGI("IsColAudioStreamExist_003 end");
}

/**
 * @tc.name: SetColAudioSwitchEnabled_001
 * @tc.desc: SetColAudioSwitchEnabled from disabled to enabled triggers ChangeIsoParam
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, SetColAudioSwitchEnabled_001, TestSize.Level1)
{
    HILOGI("SetColAudioSwitchEnabled_001 start");
    ASCService *asc = new ASCService();
    RawAddress device(deviceStr);
    asc->AddConnectDevices(device);
    asc->SetASCStatus(device, NL_SLE_ASC_STARTED);
    asc->activeSinkDevice_ = device;
    asc->isColAudioEnabled_ = false;
    asc->isColAudioBeforeEnabled_ = false;
    QosM::GetInstance().deviceQosmMap_.clear();
    std::list<AudioStreamType>& streamList = asc->GetStartedStreamList(device);
    streamList.emplace_back(AUDIO_STREAM_MUSIC);
    asc->SetColAudioSwitchEnabled(true);
    EXPECT_TRUE(asc->isColAudioEnabled_);
    delete asc;
    HILOGI("SetColAudioSwitchEnabled_001 end");
}

/**
 * @tc.name: SetColAudioSwitchEnabled_002
 * @tc.desc: SetColAudioSwitchEnabled from enabled to disabled does not trigger ChangeIsoParam
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, SetColAudioSwitchEnabled_002, TestSize.Level1)
{
    HILOGI("SetColAudioSwitchEnabled_002 start");
    ASCService *asc = new ASCService();
    asc->isColAudioEnabled_ = true;
    asc->isColAudioBeforeEnabled_ = true;
    asc->SetColAudioSwitchEnabled(false);
    EXPECT_FALSE(asc->isColAudioEnabled_);
    delete asc;
    HILOGI("SetColAudioSwitchEnabled_002 end");
}

/**
 * @tc.name: SetColAudioSwitchEnabled_003
 * @tc.desc: SetColAudioSwitchEnabled repeated ON does not trigger (anti-bounce)
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, SetColAudioSwitchEnabled_003, TestSize.Level1)
{
    HILOGI("SetColAudioSwitchEnabled_003 start");
    ASCService *asc = new ASCService();
    asc->isColAudioEnabled_ = true;
    asc->isColAudioBeforeEnabled_ = true;
    asc->SetColAudioSwitchEnabled(true);
    EXPECT_TRUE(asc->isColAudioEnabled_);
    EXPECT_TRUE(asc->isColAudioBeforeEnabled_);
    delete asc;
    HILOGI("SetColAudioSwitchEnabled_003 end");
}

/**
 * @tc.name: ProcColAudioIfNeed_001
 * @tc.desc: ProcColAudioIfNeed adds Qos10 when col audio enabled
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, ProcColAudioIfNeed_001, TestSize.Level1)
{
    HILOGI("ProcColAudioIfNeed_001 start");
    ASCService *asc = new ASCService();
    RawAddress device(deviceStr);
    asc->AddConnectDevices(device);
    asc->isColAudioEnabled_ = true;
    QosM::GetInstance().deviceQosmMap_.clear();
    asc->ProcColAudioIfNeed(device, AUDIO_STREAM_MUSIC);
    EXPECT_TRUE(QosM::GetInstance().IsQosExist(device, NL_SLE_QOS_10));
    delete asc;
    HILOGI("ProcColAudioIfNeed_001 end");
}

/**
 * @tc.name: ProcColAudioIfNeed_002
 * @tc.desc: ProcColAudioIfNeed does not reconfig when Qos10 already exists
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, ProcColAudioIfNeed_002, TestSize.Level1)
{
    HILOGI("ProcColAudioIfNeed_002 start");
    ASCService *asc = new ASCService();
    RawAddress device(deviceStr);
    asc->AddConnectDevices(device);
    asc->isColAudioEnabled_ = true;
    QosM::GetInstance().deviceQosmMap_.clear();
    QosM::GetInstance().AddQos(device, NL_SLE_QOS_10);
    EXPECT_TRUE(QosM::GetInstance().IsQosExist(device, NL_SLE_QOS_10));
    asc->SetASCStatus(device, NL_SLE_ASC_STARTED);
    asc->ProcColAudioIfNeed(device, AUDIO_STREAM_MUSIC);
    EXPECT_TRUE(QosM::GetInstance().IsQosExist(device, NL_SLE_QOS_10));
    delete asc;
    HILOGI("ProcColAudioIfNeed_002 end");
}

/**
 * @tc.name: ProcColAudioIfNeed_003
 * @tc.desc: ProcColAudioIfNeed does nothing when col audio disabled
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, ProcColAudioIfNeed_003, TestSize.Level1)
{
    HILOGI("ProcColAudioIfNeed_003 start");
    ASCService *asc = new ASCService();
    RawAddress device(deviceStr);
    asc->AddConnectDevices(device);
    asc->isColAudioEnabled_ = false;
    QosM::GetInstance().deviceQosmMap_.clear();
    asc->ProcColAudioIfNeed(device, AUDIO_STREAM_MUSIC);
    EXPECT_FALSE(QosM::GetInstance().IsQosExist(device, NL_SLE_QOS_10));
    delete asc;
    HILOGI("ProcColAudioIfNeed_003 end");
}

/**
 * @tc.name: ProcessColAudioSwitchChangeEvent_001
 * @tc.desc: ProcessColAudioSwitchChangeEvent enable
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, ProcessColAudioSwitchChangeEvent_001, TestSize.Level1)
{
    HILOGI("ProcessColAudioSwitchChangeEvent_001 start");
    ASCService *asc = new ASCService();
    asc->isColAudioEnabled_ = false;
    ASCMessage event;
    event.result_ = true;
    asc->ProcessColAudioSwitchChangeEvent(event);
    EXPECT_TRUE(asc->isColAudioEnabled_);
    delete asc;
    HILOGI("ProcessColAudioSwitchChangeEvent_001 end");
}

/**
 * @tc.name: ProcessColAudioSwitchChangeEvent_002
 * @tc.desc: ProcessColAudioSwitchChangeEvent disable
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, ProcessColAudioSwitchChangeEvent_002, TestSize.Level1)
{
    HILOGI("ProcessColAudioSwitchChangeEvent_002 start");
    ASCService *asc = new ASCService();
    asc->isColAudioEnabled_ = true;
    ASCMessage event;
    event.result_ = false;
    asc->ProcessColAudioSwitchChangeEvent(event);
    EXPECT_FALSE(asc->isColAudioEnabled_);
    delete asc;
    HILOGI("ProcessColAudioSwitchChangeEvent_002 end");
}

/**
 * @tc.name: ChangeIsoParamForColAudioIfNeed_001
 * @tc.desc: ChangeIsoParamForColAudioIfNeed with started device
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, ChangeIsoParamForColAudioIfNeed_001, TestSize.Level1)
{
    HILOGI("ChangeIsoParamForColAudioIfNeed_001 start");
    ASCService *asc = new ASCService();
    RawAddress device(deviceStr);
    asc->AddConnectDevices(device);
    asc->SetASCStatus(device, NL_SLE_ASC_STARTED);
    asc->activeSinkDevice_ = device;
    asc->isColAudioEnabled_ = true;
    QosM::GetInstance().deviceQosmMap_.clear();
    std::list<AudioStreamType>& streamList = asc->GetStartedStreamList(device);
    streamList.emplace_back(AUDIO_STREAM_MUSIC);
    asc->ChangeIsoParamForColAudioIfNeed();
    delete asc;
    HILOGI("ChangeIsoParamForColAudioIfNeed_001 end");
}

/**
 * @tc.name: ChangeIsoParamForColAudioIfNeed_002
 * @tc.desc: ChangeIsoParamForColAudioIfNeed with no started device
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, ChangeIsoParamForColAudioIfNeed_002, TestSize.Level1)
{
    HILOGI("ChangeIsoParamForColAudioIfNeed_002 start");
    ASCService *asc = new ASCService();
    RawAddress device(deviceStr);
    asc->AddConnectDevices(device);
    asc->activeSinkDevice_ = device;
    asc->isColAudioEnabled_ = true;
    QosM::GetInstance().deviceQosmMap_.clear();
    asc->ChangeIsoParamForColAudioIfNeed();
    delete asc;
    HILOGI("ChangeIsoParamForColAudioIfNeed_002 end");
}

/**
 * @tc.name: GetStreamQos_ColAudio_001
 * @tc.desc: GetStreamQos returns NL_SLE_QOS_10 when col audio enabled
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, GetStreamQos_ColAudio_001, TestSize.Level1)
{
    HILOGI("GetStreamQos_ColAudio_001 start");
    ASCService *asc = new ASCService();
    RawAddress device(deviceStr);
    asc->AddConnectDevices(device);
    asc->isColAudioEnabled_ = true;
    Qos qos = asc->GetStreamQos(device, AUDIO_STREAM_MUSIC);
    EXPECT_EQ(NL_SLE_QOS_10, qos);
    qos = asc->GetStreamQos(device, AUDIO_STREAM_VOICE_CALL);
    EXPECT_NE(NL_SLE_QOS_10, qos);
    asc->isColAudioEnabled_ = false;
    qos = asc->GetStreamQos(device, AUDIO_STREAM_MUSIC);
    EXPECT_NE(NL_SLE_QOS_10, qos);
    delete asc;
    HILOGI("GetStreamQos_ColAudio_001 end");
}

/**
 * @tc.name: StartPlaying_ColAudio_001
 * @tc.desc: StartPlaying transfers stream type when col audio enabled
 * @tc.type: FUNC
 */
HWTEST_F(ASCServiceTest, StartPlaying_ColAudio_001, TestSize.Level1)
{
    HILOGI("StartPlaying_ColAudio_001 start");
    ASCService *asc = new ASCService();
    RawAddress device(deviceStr);
    asc->AddConnectDevices(device);
    asc->isColAudioEnabled_ = true;
    asc->SetASCStatus(device, NL_SLE_ASC_STARTED);
    asc->StartPlaying(device, AUDIO_STREAM_MUSIC, false);
    delete asc;
    HILOGI("StartPlaying_ColAudio_001 end");
}
}  // namespace Nearlink
}  // namespace OHOS