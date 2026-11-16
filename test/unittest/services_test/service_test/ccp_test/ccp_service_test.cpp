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
#include "CcpService.h"
#include "CcpDefines.h"
#include "SleInterfaceManager.h"
#include "SleServiceManager.h"
#include "ThreadUtil.h"
#include "mock_nearlink_call_client.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;
using namespace OHOS::Telephony;
using ::testing::Return;
using ::testing::_;

namespace {
constexpr int CCP_SERVICE_UT_DELAY_50_MS = 50;
constexpr int CCP_SERVICE_UT_DELAY_1000_MS = 1000;
/* 清场时遍历的蜂窝 callId 上界，用例内使用的 callId 均不得超过该值 */
constexpr int32_t CCP_SERVICE_UT_CALL_ID_CLEAR_MAX = 8;
/* 蜂窝路取值：不依赖 call_manager 里其它枚举项的具体数值，只保证不等于 TYPE_VOIP */
constexpr int32_t CCP_SERVICE_UT_CALL_TYPE_NOT_VOIP = static_cast<int32_t>(CallType::TYPE_VOIP) + 1;

/* 铺底：造出一路蜂窝通话映射（callId>0），使 CallDetailsChanged 的守卫得以放行 */
void SetupPhoneCallMapping(CcpService *ccpService, int32_t callId)
{
    std::vector<Telephony::CallAttributeInfo> callInfoVec;
    CallAttributeInfo callAttributeInfo{};
    callAttributeInfo.callId = callId;
    callAttributeInfo.callState = TelCallState::CALL_STATUS_DIALING;
    callInfoVec.push_back(callAttributeInfo);
    ccpService->HandleCallDetailChange(callId, static_cast<int32_t>(TelCallState::CALL_STATUS_DIALING),
        "", callInfoVec);
}

/* 清场：CcpService 为进程级单例且用例按注册顺序执行，蜂窝映射必须逐条用例自行归还，
 * 否则后续用例会因守卫判定依赖全局状态而静默走进非预期分支 */
void ClearPhoneCallMappings(CcpService *ccpService)
{
    std::vector<Telephony::CallAttributeInfo> emptyVec;
    for (int32_t callId = 1; callId <= CCP_SERVICE_UT_CALL_ID_CLEAR_MAX; callId++) {
        ccpService->HandleCallDetailChange(callId,
            static_cast<int32_t>(TelCallState::CALL_STATUS_DISCONNECTED), "", emptyVec);
    }
}
}

class CcpServiceTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void CcpServiceTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase CcpServiceTest start");
    /* 单测未启动 ccp 线程：置为同步执行，保证投递入口的断言即时生效，与仓内其它单测一致 */
    ThreadUtil::GetInstance().threadStateMap_.EnsureInsert(
        THREAD_ID_CCP, ThreadUtil::ThreadState::NOT_SWITCH_THREAD);
    ThreadUtil::GetInstance().InitThreadStateMap();
    SleInterfaceManager::GetInstance()->Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_1000_MS));
    HILOGI("SetUpTestCase CcpServiceTest end");
}

void CcpServiceTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase CcpServiceTest");
}

void CcpServiceTest::SetUp()
{
    HILOGI("SetUp CcpServiceTest.");
    CcpService *ccpService = CcpService::GetService();
    if (ccpService == nullptr) {
        return;
    }
    /* CcpService 为进程级单例：每个用例开始前把通话映射归零，避免断言依赖用例注册顺序。
     * 必须先清自造 VoIP 再清蜂窝：蜂窝挂断会按 voipIdSet_ 恢复 VoIP 状态，顺序颠倒会把 VoIP 状态带回来 */
    if (ccpService->HasVoipCallId()) {
        RawAddress devAddr("00:11:22:33:44:55");
        ccpService->HandleVoipStop(devAddr);
        std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
    }
    if (ccpService->HasPhoneCallId()) {
        ClearPhoneCallMappings(ccpService);
    }
}

void CcpServiceTest::TearDown()
{
    HILOGI("TearDown CcpServiceTest.");
}

/**
 * @tc.number: GetService001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(CcpServiceTest, GetService001, TestSize.Level1)
{
    HILOGI("GetService001 start");
    CcpService* ccpService = CcpService::GetService();
    EXPECT_NE(nullptr, ccpService);
    HILOGI("GetService001 end");
}

/**
 * @tc.number: Disable001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(CcpServiceTest, Disable001, TestSize.Level1)
{
    HILOGI("Disable001 start");
    CcpService* ccpService = CcpService::GetService();
    ccpService->Disable();
    EXPECT_NE(nullptr, ccpService);

    ccpService->Disable();
    EXPECT_NE(nullptr, ccpService);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("Disable001 end");
}

/**
 * @tc.number: Enable001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(CcpServiceTest, Enable001, TestSize.Level1)
{
    HILOGI("Enable001 start");
    CcpService* ccpService = CcpService::GetService();
    ccpService->Enable();
    EXPECT_NE(nullptr, ccpService);

    ccpService->Enable();
    EXPECT_NE(nullptr, ccpService);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("Enable001 end");
}

/**
 * @tc.number: HandleAnswer001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(CcpServiceTest, HandleAnswer001, TestSize.Level1)
{
    HILOGI("HandleAnswer001 start");
    CcpService* ccpService = CcpService::GetService();
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);
    int32_t instanceId = 0;
    uint32_t requestId = 0;
    uint8_t callId = 0;

    ccpService->HandleAnswer(*device, instanceId, requestId, callId);
    EXPECT_NE(nullptr, ccpService);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("HandleAnswer001 end");
}

/**
 * @tc.number: HandleHangUp001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(CcpServiceTest, HandleHangUp001, TestSize.Level1)
{
    HILOGI("HandleHangUp001 start");
    CcpService* ccpService = CcpService::GetService();
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);
    int32_t instanceId = 0;
    uint32_t requestId = 0;
    uint8_t callId = 0;

    ccpService->HandleHangUp(*device, instanceId, requestId, callId);
    EXPECT_NE(nullptr, ccpService);

    instanceId = 1;
    requestId = 1;
    callId = 1;
    ccpService->HandleHangUp(*device, instanceId, requestId, callId);
    EXPECT_NE(nullptr, ccpService);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("HandleHangUp001 end");
}

/**
 * @tc.number: HandleAuthorize001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(CcpServiceTest, HandleAuthorize001, TestSize.Level1)
{
    HILOGI("HandleAuthorize001 start");
    CcpService* ccpService = CcpService::GetService();
    int32_t instanceId = 0;
    uint32_t requestId = 0;
    int32_t property = 0;
    ccpService->HandleAuthorize(instanceId, requestId, property);
    EXPECT_NE(nullptr, ccpService);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("HandleAuthorize001 end");
}

/**
 * @tc.number: HandlePhoneStateChange001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(CcpServiceTest, HandlePhoneStateChange001, TestSize.Level1)
{
    HILOGI("HandlePhoneStateChange001 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(ccpService, nullptr);

    std::vector<Telephony::CallAttributeInfo> callInfoVec;
    CallAttributeInfo callAttributeInfo{};
    callAttributeInfo.callId = 1;
    callAttributeInfo.callState = TelCallState::CALL_STATUS_INCOMING;
    callInfoVec.push_back(callAttributeInfo);
    ccpService->HandlePhoneStateChange(callInfoVec);

    /* 没有通话时也要把空列表同步给对端 */
    callInfoVec.clear();
    ccpService->HandlePhoneStateChange(callInfoVec);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("HandlePhoneStateChange001 end");
}

/**
 * @tc.number: HandleCallDetailChange001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(CcpServiceTest, HandleCallDetailChange001, TestSize.Level1)
{
    HILOGI("HandleCallDetailChange001 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(ccpService, nullptr);

    constexpr int32_t callId = 1;
    const std::string number = "11111111111";
    std::vector<Telephony::CallAttributeInfo> callInfoVec;
    CallAttributeInfo callAttributeInfo{};
    callAttributeInfo.callId = callId;
    callAttributeInfo.callState = TelCallState::CALL_STATUS_DIALING;
    callInfoVec.push_back(callAttributeInfo);

    ccpService->HandleCallDetailChange(callId, static_cast<int32_t>(TelCallState::CALL_STATUS_DIALING),
        number, callInfoVec);
    ccpService->HandleCallDetailChange(callId, static_cast<int32_t>(TelCallState::CALL_STATUS_INCOMING),
        number, callInfoVec);
    ccpService->HandleCallDetailChange(callId, static_cast<int32_t>(TelCallState::CALL_STATUS_WAITING),
        number, callInfoVec);
    ccpService->HandleCallDetailChange(callId, static_cast<int32_t>(TelCallState::CALL_STATUS_DISCONNECTED),
        number, callInfoVec);
    ccpService->HandleCallDetailChange(callId, static_cast<int32_t>(TelCallState::CALL_STATUS_UNKNOWN),
        number, callInfoVec);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("HandleCallDetailChange001 end");
}

/**
 * @tc.number: PhoneStateChanged001
 * @tc.name:
 * @tc.desc: callId 为 -1（no specific call）时只刷新全量状态，不得污染 callId 映射
 */
HWTEST_F(CcpServiceTest, PhoneStateChanged001, TestSize.Level1)
{
    HILOGI("PhoneStateChanged001 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(ccpService, nullptr);

    /* CcpService 为进程级单例，记录基线以免受其它用例遗留状态影响 */
    bool voipBefore = ccpService->HasVoipCallId();
    ccpService->PhoneStateChanged(INVALID_CCP_VOIP_ID, 0, 0,
        static_cast<int32_t>(TelCallState::CALL_STATUS_DISCONNECTED), "");
    /* callId=-1 只刷新全量状态，绝不能新增 VoIP 映射：喂入前后 HasVoipCallId() 必须一致 */
    EXPECT_EQ(voipBefore, ccpService->HasVoipCallId());
    /* SetUp 已将单例状态归零，期望干净态为 false */
    EXPECT_FALSE(ccpService->HasVoipCallId());
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("PhoneStateChanged001 end");
}

/**
 * @tc.number: CallDetailsChanged001
 * @tc.name:
 * @tc.desc: 单路明细入口 callId 为 -1（no specific call）时直接跳过，不得污染 callId 映射
 */
HWTEST_F(CcpServiceTest, CallDetailsChanged001, TestSize.Level1)
{
    HILOGI("CallDetailsChanged001 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(ccpService, nullptr);

    /* CcpService 为进程级单例，记录基线以免受其它用例遗留状态影响 */
    bool voipBefore = ccpService->HasVoipCallId();
    ccpService->CallDetailsChanged(INVALID_CCP_VOIP_ID,
        static_cast<int32_t>(TelCallState::CALL_STATUS_DISCONNECTED), CCP_SERVICE_UT_CALL_TYPE_NOT_VOIP);
    /* callId=-1 仅表示无特定通话，绝不能新增 VoIP 映射：喂入前后 HasVoipCallId() 必须一致 */
    EXPECT_EQ(voipBefore, ccpService->HasVoipCallId());
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("CallDetailsChanged001 end");
}

/**
 * @tc.number: CallDetailsChanged002
 * @tc.name:
 * @tc.desc: 存在蜂窝映射时挂断分支放行，DISCONNECTED 清理该 callId 映射
 */
HWTEST_F(CcpServiceTest, CallDetailsChanged002, TestSize.Level1)
{
    HILOGI("CallDetailsChanged002 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(ccpService, nullptr);

    constexpr int32_t callId = 1;
    ClearPhoneCallMappings(ccpService);
    ASSERT_FALSE(ccpService->HasPhoneCallId());
    SetupPhoneCallMapping(ccpService, callId);
    ASSERT_TRUE(ccpService->HasPhoneCallId());

    ccpService->CallDetailsChanged(callId, static_cast<int32_t>(TelCallState::CALL_STATUS_DISCONNECTED),
        CCP_SERVICE_UT_CALL_TYPE_NOT_VOIP);
    /* 挂断明细生效：该路 callId 映射被清理 */
    EXPECT_FALSE(ccpService->HasPhoneCallId());
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("CallDetailsChanged002 end");
}

/**
 * @tc.number: CallDetailsChanged003
 * @tc.name:
 * @tc.desc: 无蜂窝映射时守卫拦截，任何状态都不得新建映射（ccp 自造场景排除）
 */
HWTEST_F(CcpServiceTest, CallDetailsChanged003, TestSize.Level1)
{
    HILOGI("CallDetailsChanged003 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(ccpService, nullptr);

    constexpr int32_t callId = 5;
    ClearPhoneCallMappings(ccpService);
    ASSERT_FALSE(ccpService->HasPhoneCallId());

    ccpService->CallDetailsChanged(callId, static_cast<int32_t>(TelCallState::CALL_STATUS_DISCONNECTED),
        CCP_SERVICE_UT_CALL_TYPE_NOT_VOIP);
    /* 无在表蜂窝通话时直接返回：不得反手分配 index 并写入映射 */
    EXPECT_FALSE(ccpService->HasPhoneCallId());
    EXPECT_FALSE(ccpService->HasVoipCallId());
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("CallDetailsChanged003 end");
}

/**
 * @tc.number: CallDetailsChanged004
 * @tc.name:
 * @tc.desc: 非 DISCONNECTED 状态整条请求被忽略，映射不得误清；结尾自行清场
 */
HWTEST_F(CcpServiceTest, CallDetailsChanged004, TestSize.Level1)
{
    HILOGI("CallDetailsChanged004 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(ccpService, nullptr);

    constexpr int32_t callId = 2;
    ClearPhoneCallMappings(ccpService);
    SetupPhoneCallMapping(ccpService, callId);
    ASSERT_TRUE(ccpService->HasPhoneCallId());

    ccpService->CallDetailsChanged(callId, static_cast<int32_t>(TelCallState::CALL_STATUS_INCOMING),
        CCP_SERVICE_UT_CALL_TYPE_NOT_VOIP);
    /* 非挂断状态的明细由 PhoneStateChanged 承载，本入口不得提前清映射 */
    EXPECT_TRUE(ccpService->HasPhoneCallId());

    /* 清场，避免残留蜂窝映射影响后续用例的守卫判定 */
    ccpService->CallDetailsChanged(callId, static_cast<int32_t>(TelCallState::CALL_STATUS_DISCONNECTED),
        CCP_SERVICE_UT_CALL_TYPE_NOT_VOIP);
    EXPECT_FALSE(ccpService->HasPhoneCallId());
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("CallDetailsChanged004 end");
}

/**
 * @tc.number: CallDetailsChanged005
 * @tc.name:
 * @tc.desc: 守卫放行后 callId 为 -1（no specific call）仍被拦截，不得污染映射
 */
HWTEST_F(CcpServiceTest, CallDetailsChanged005, TestSize.Level1)
{
    HILOGI("CallDetailsChanged005 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(ccpService, nullptr);

    constexpr int32_t callId = 3;
    ClearPhoneCallMappings(ccpService);
    SetupPhoneCallMapping(ccpService, callId);
    ASSERT_TRUE(ccpService->HasPhoneCallId());

    ccpService->CallDetailsChanged(INVALID_CCP_VOIP_ID,
        static_cast<int32_t>(TelCallState::CALL_STATUS_DISCONNECTED), CCP_SERVICE_UT_CALL_TYPE_NOT_VOIP);
    /* -1 仅表示无特定通话：既不能清掉在册的蜂窝映射，也不能自己入表 */
    EXPECT_TRUE(ccpService->HasPhoneCallId());
    EXPECT_FALSE(ccpService->HasVoipCallId());

    ClearPhoneCallMappings(ccpService);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("CallDetailsChanged005 end");
}

/**
 * @tc.number: CallDetailsChanged006
 * @tc.name:
 * @tc.desc: CCP 自造 VoIP（voipId=-2）不被单路明细入口重复处理
 */
HWTEST_F(CcpServiceTest, CallDetailsChanged006, TestSize.Level1)
{
    HILOGI("CallDetailsChanged006 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(ccpService, nullptr);
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);

    constexpr int32_t voipId = NEARLINK_CCP_VOIP_MAX - 1;
    ClearPhoneCallMappings(ccpService);
    /* HandleVoipStart 内部投递到 CCP 线程，需等待完成后取基线 */
    ccpService->HandleVoipStart(*device);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
    ASSERT_TRUE(ccpService->HasVoipCallId());
    ASSERT_FALSE(ccpService->HasPhoneCallId());

    ccpService->CallDetailsChanged(voipId, static_cast<int32_t>(TelCallState::CALL_STATUS_DISCONNECTED),
        static_cast<int32_t>(CallType::TYPE_VOIP));
    /* 自造 VoIP 由 callType 过滤排除：VoIP 状态不得被本入口误清 */
    EXPECT_TRUE(ccpService->HasVoipCallId());

    /* 清场：自造 VoIP 属于全局状态，结尾必须自行释放，不得遗给后续用例 */
    ccpService->HandleVoipStop(*device);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
    EXPECT_FALSE(ccpService->HasVoipCallId());
    HILOGI("CallDetailsChanged006 end");
}

/**
 * @tc.number: CallDetailsChanged007
 * @tc.name:
 * @tc.desc: 蜂窝映射在表时，callType 为 TYPE_VOIP 的挂断不经本通道清理（与 002 唯一差异为 callType）
 */
HWTEST_F(CcpServiceTest, CallDetailsChanged007, TestSize.Level1)
{
    HILOGI("CallDetailsChanged007 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(ccpService, nullptr);

    constexpr int32_t callId = 7;
    ClearPhoneCallMappings(ccpService);
    SetupPhoneCallMapping(ccpService, callId);
    ASSERT_TRUE(ccpService->HasPhoneCallId());

    /* 002 与新条件的对照组：状态、callId 均相同，只把 callType 换成 TYPE_VOIP */
    ccpService->CallDetailsChanged(callId, static_cast<int32_t>(TelCallState::CALL_STATUS_DISCONNECTED),
        static_cast<int32_t>(CallType::TYPE_VOIP));
    /* VoIP 路不经本通道处理，映射必须原样保留 */
    EXPECT_TRUE(ccpService->HasPhoneCallId());

    /* 该路的映射仍由 PhoneStateChanged 通道承载，此处自行归还避免污染后续用例 */
    ClearPhoneCallMappings(ccpService);
    EXPECT_FALSE(ccpService->HasPhoneCallId());
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("CallDetailsChanged007 end");
}

/**
 * @tc.number: HasPhoneCallId001
 * @tc.name:
 * @tc.desc: 蜂窝标识判定：空表为 false，存在 callId>0 映射为 true，清场后回到 false
 */
HWTEST_F(CcpServiceTest, HasPhoneCallId001, TestSize.Level1)
{
    HILOGI("HasPhoneCallId001 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(ccpService, nullptr);

    constexpr int32_t callId = 4;
    ClearPhoneCallMappings(ccpService);
    EXPECT_FALSE(ccpService->HasPhoneCallId());

    SetupPhoneCallMapping(ccpService, callId);
    EXPECT_TRUE(ccpService->HasPhoneCallId());

    ClearPhoneCallMappings(ccpService);
    EXPECT_FALSE(ccpService->HasPhoneCallId());
    /* 仅负数 key（自造 VoIP）不算蜂窝路，该分支由 CallDetailsChanged006 覆盖 */
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("HasPhoneCallId001 end");
}

/**
 * @tc.number: HandleVoipStart002
 * @tc.name:
 * @tc.desc: 存在蜂窝通话时不得自造 VoIP 状态（与 CallDetailsChanged 守卫同一判定）
 */
HWTEST_F(CcpServiceTest, HandleVoipStart002, TestSize.Level1)
{
    HILOGI("HandleVoipStart002 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(ccpService, nullptr);
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);

    constexpr int32_t callId = 6;
    ClearPhoneCallMappings(ccpService);
    SetupPhoneCallMapping(ccpService, callId);
    ASSERT_TRUE(ccpService->HasPhoneCallId());

    ccpService->HandleVoipStart(*device);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
    /* 蜂窝通话在表时守卫拦住，不得创建自造 VoIP */
    EXPECT_FALSE(ccpService->HasVoipCallId());

    ClearPhoneCallMappings(ccpService);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("HandleVoipStart002 end");
}

/**
 * @tc.number: HandleVoipStop001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(CcpServiceTest, HandleVoipStop001, TestSize.Level1)
{
    HILOGI("HandleVoipStop001 start");
    CcpService* ccpService = CcpService::GetService();
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);

    ccpService->HandleVoipStop(*device);
    EXPECT_NE(nullptr, ccpService);

    ccpService->HandleVoipStop(*device);
    EXPECT_NE(nullptr, ccpService);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("HandleVoipStop001 end");
}

/**
 * @tc.number: HandleVoipStart001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(CcpServiceTest, HandleVoipStart001, TestSize.Level1)
{
    HILOGI("HandleVoipStart001 start");
    CcpService* ccpService = CcpService::GetService();
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);

    ccpService->HandleVoipStart(*device);
    EXPECT_NE(nullptr, ccpService);

    ccpService->HandleVoipStart(*device);
    EXPECT_NE(nullptr, ccpService);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("HandleVoipStart001 end");
}

/**
 * @tc.number: TryResumeCurrentCalls001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(CcpServiceTest, TryResumeCurrentCalls001, TestSize.Level1)
{
    HILOGI("TryResumeCurrentCalls001 start");
    CcpService* ccpService = CcpService::GetService();
    ccpService->TryResumeCurrentCalls();
    EXPECT_NE(nullptr, ccpService);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("TryResumeCurrentCalls001 end");
}

/**
 * @tc.number: NotifyAudioDeviceAction001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(CcpServiceTest, NotifyAudioDeviceAction001, TestSize.Level1)
{
    HILOGI("NotifyAudioDeviceAction001 start");
    CcpService* ccpService = CcpService::GetService();
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);
    int action = 0;
    ccpService->NotifyAudioDeviceAction(*device, action);
    EXPECT_NE(nullptr, ccpService);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("NotifyAudioDeviceAction001 end");
}

/**
 * @tc.number: ProcessCallStateInfo001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(CcpServiceTest, ProcessCallStateInfo001, TestSize.Level1)
{
    HILOGI("ProcessCallStateInfo001 start");
    CcpService* ccpService = CcpService::GetService();
    /* 蜂窝 callId 必须为正：喂 0 会在映射表留下 HasPhoneCallId/HasVoipCallId 都清不到的 key=0 条目 */
    constexpr int32_t callId = 1;
    std::vector<Telephony::CallAttributeInfo> callInfoVec;
    CallAttributeInfo callAttributeInfo{};
    callAttributeInfo.callId = callId;
    callAttributeInfo.callState = TelCallState::CALL_STATUS_INCOMING;
    callInfoVec.push_back(callAttributeInfo);

    ccpService->ProcessCallStateInfo(callInfoVec);
    EXPECT_NE(nullptr, ccpService);
    EXPECT_TRUE(ccpService->HasPhoneCallId());

    /* 清场：映射由本用例建立，结尾自行归还，避免残留影响后续用例 */
    ClearPhoneCallMappings(ccpService);
    EXPECT_FALSE(ccpService->HasPhoneCallId());
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("ProcessCallStateInfo001 end");
}

/**
 * @tc.number: ProcessAccountInfo001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(CcpServiceTest, ProcessAccountInfo001, TestSize.Level1)
{
    HILOGI("ProcessAccountInfo001 start");
    CcpService* ccpService = CcpService::GetService();
    std::shared_ptr<CallInOutInfoProp> prop = std::make_shared<CallInOutInfoProp>();
    prop->callId = 1;
    prop->callFlag = 0;
    CallAttributeInfo callAttributeInfo{};
    callAttributeInfo.callState = TelCallState::CALL_STATUS_INCOMING;
    ccpService->ProcessAccountInfo(prop, callAttributeInfo);
    EXPECT_NE(nullptr, ccpService);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("ProcessAccountInfo001 end");
}

/**
 * @tc.number: NotifyAudioDeviceActionViaProfileAsc_001
 * @tc.name: NotifyAudioDeviceAction via ProfileASC interface
 * @tc.desc: Test NotifyAudioDeviceAction uses ProfileASC from profile manager, should not crash
 */
HWTEST_F(CcpServiceTest, NotifyAudioDeviceActionViaProfileAsc_001, TestSize.Level1)
{
    HILOGI("NotifyAudioDeviceActionViaProfileAsc_001 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(ccpService, nullptr);
    RawAddress devAddr("11:22:33:44:55:66");
    ccpService->NotifyAudioDeviceAction(devAddr, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("NotifyAudioDeviceActionViaProfileAsc_001 end");
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS