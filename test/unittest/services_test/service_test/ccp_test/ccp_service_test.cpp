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
#include <memory>
#include "ThreadUtil.h"
#include "ProfileServiceManager.cpp"
#include "CcpService.h"
#include "CcpDefines.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;
using namespace OHOS::Telephony;

namespace {
/* 清场时遍历的蜂窝 callId 上界，用例内使用的 callId 均不得超过该值 */
constexpr int32_t CCP_SERVICE_UT_CALL_ID_CLEAR_MAX = 8;
/* 蜂窝路取值：不依赖 call_manager 里其它枚举项的具体数值，只保证不等于 TYPE_VOIP */
constexpr int32_t CCP_SERVICE_UT_CALL_TYPE_NOT_VOIP = static_cast<int32_t>(CallType::TYPE_VOIP) + 1;

/* 铺底：造出一路蜂窝通话映射（callId>0），使单路明细入口的挂断在表校验得以放行 */
void SetupPhoneCallMapping(CcpService *ccpService, int32_t callId)
{
    std::vector<Telephony::CallAttributeInfo> callInfoVec;
    CallAttributeInfo callAttributeInfo{};
    callAttributeInfo.callId = callId;
    callAttributeInfo.callState = TelCallState::CALL_STATUS_DIALING;
    callInfoVec.push_back(callAttributeInfo);
    ccpService->ProcessCallStateInfo(callInfoVec);
}

/* 清场：把上界以内的蜂窝 callId 逐条以 DISCONNECTED 归还映射，用例可直接断言回收结果 */
void ClearPhoneCallMappings(CcpService *ccpService)
{
    for (int32_t callId = 1; callId <= CCP_SERVICE_UT_CALL_ID_CLEAR_MAX; callId++) {
        CallAttributeInfo callAttributeInfo{};
        callAttributeInfo.callId = callId;
        callAttributeInfo.callState = TelCallState::CALL_STATUS_DISCONNECTED;
        ccpService->HandleCallDetailChange(callAttributeInfo.callId,
            static_cast<int32_t>(callAttributeInfo.callState), "", { callAttributeInfo });
    }
}
}

class NearlinkCcpServiceTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();

private:
    std::unique_ptr<CcpService> service_;
};

void NearlinkCcpServiceTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase NearlinkCcpServiceTest start");
    // CCP service tests own their service; vendor stack startup is an integration dependency.
    HILOGI("SetUpTestCase NearlinkCcpServiceTest end");
}

void NearlinkCcpServiceTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase NearlinkCcpServiceTest");
}

void NearlinkCcpServiceTest::SetUp()
{
    HILOGI("SetUp NearlinkCcpServiceTest.");
    // CCP tasks execute synchronously in the test thread; no sleep is needed after calls.
    ThreadUtil::GetInstance().threadStateMap_.EnsureInsert(
        THREAD_ID_CCP, ThreadUtil::ThreadState::NOT_SWITCH_THREAD);
    service_ = std::make_unique<CcpService>();
    ProfileServiceManager::GetInstance().pimpl->startedProfiles_.SetProfile(
        SleTransport::ADAPTER_SLE, PROFILE_NAME_CCP, service_.get());
    ASSERT_EQ(service_.get(), CcpService::GetService());
}

void NearlinkCcpServiceTest::TearDown()
{
    HILOGI("TearDown NearlinkCcpServiceTest.");
    ProfileServiceManager::GetInstance().pimpl->startedProfiles_.SetProfile(
        SleTransport::ADAPTER_SLE, PROFILE_NAME_CCP, nullptr);
    service_.reset();
    ThreadUtil::GetInstance().InitThreadStateMap();
}

/**
 * @tc.number: GetService001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkCcpServiceTest, GetService001, TestSize.Level1)
{
    HILOGI("GetService001 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(nullptr, ccpService);
    HILOGI("GetService001 end");
}

/**
 * @tc.number: Disable001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkCcpServiceTest, Disable001, TestSize.Level1)
{
    HILOGI("Disable001 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(nullptr, ccpService);
    /* 预置一路蜂窝映射：Disable 必须把通话缓存清空 */
    SetupPhoneCallMapping(ccpService, 1);
    ASSERT_TRUE(ccpService->HasPhoneCallId());

    ccpService->Disable();
    EXPECT_FALSE(ccpService->HasPhoneCallId());
    EXPECT_FALSE(ccpService->HasVoipCallId());

    /* 重复 Disable 幂等 */
    ccpService->Disable();
    EXPECT_FALSE(ccpService->HasPhoneCallId());
    HILOGI("Disable001 end");
}

/**
 * @tc.number: Enable001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkCcpServiceTest, Enable001, TestSize.Level1)
{
    HILOGI("Enable001 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(nullptr, ccpService);
    ccpService->Enable();
    /* Enable 只起协议栈实例，不得凭空产生通话映射 */
    EXPECT_FALSE(ccpService->HasPhoneCallId());
    EXPECT_FALSE(ccpService->HasVoipCallId());

    /* 重复 Enable 幂等 */
    ccpService->Enable();
    EXPECT_FALSE(ccpService->HasPhoneCallId());
    HILOGI("Enable001 end");
}

/**
 * @tc.number: HandleAnswer001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkCcpServiceTest, HandleAnswer001, TestSize.Level1)
{
    HILOGI("HandleAnswer001 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(nullptr, ccpService);
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);
    int32_t instanceId = 0;
    uint32_t requestId = 0;
    uint8_t callId = 0;

    ccpService->HandleAnswer(*device, instanceId, requestId, callId);
    HILOGI("HandleAnswer001 end");
}

/**
 * @tc.number: HandleHangUp001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkCcpServiceTest, HandleHangUp001, TestSize.Level1)
{
    HILOGI("HandleHangUp001 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(nullptr, ccpService);
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);
    int32_t instanceId = 0;
    uint32_t requestId = 0;
    uint8_t callId = 0;

    ccpService->HandleHangUp(*device, instanceId, requestId, callId);

    instanceId = 1;
    requestId = 1;
    callId = 1;
    ccpService->HandleHangUp(*device, instanceId, requestId, callId);
    /* 空表时挂断入口被在表守卫拦截，不得凭空建映射 */
    EXPECT_FALSE(ccpService->HasPhoneCallId());
    EXPECT_FALSE(ccpService->HasVoipCallId());
    HILOGI("HandleHangUp001 end");
}

/**
 * @tc.number: HandleAuthorize001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkCcpServiceTest, HandleAuthorize001, TestSize.Level1)
{
    HILOGI("HandleAuthorize001 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(nullptr, ccpService);
    int32_t instanceId = 0;
    uint32_t requestId = 0;
    int32_t property = 0;
    ccpService->HandleAuthorize(instanceId, requestId, property);
    HILOGI("HandleAuthorize001 end");
}

/**
 * @tc.number: HandlePhoneStateChange001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkCcpServiceTest, HandlePhoneStateChange001, TestSize.Level1)
{
    HILOGI("HandlePhoneStateChange001 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(nullptr, ccpService);

    /* 无特定通话时传 INVALID_CCP_VOIP_ID/IDLE：入口内部回查 GetCurrentCallList 做全量同步，空列表也要同步给对端 */
    ccpService->HandlePhoneStateChange(
        INVALID_CCP_VOIP_ID, static_cast<int32_t>(Telephony::TelCallState::CALL_STATUS_IDLE), "");
    HILOGI("HandlePhoneStateChange001 end");
}

/**
 * @tc.number: HandleCallDetailChange001
 * @tc.name:
 * @tc.desc: 单路明细入口按状态分流：非挂断只更新明细保留映射，DISCONNECTED 回收映射，UNKNOWN 走默认分支
 */
HWTEST_F(NearlinkCcpServiceTest, HandleCallDetailChange001, TestSize.Level1)
{
    HILOGI("HandleCallDetailChange001 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(nullptr, ccpService);

    constexpr int32_t callId = 1;
    /* 每个用例独占服务实例，喂入前必须是干净态 */
    ASSERT_FALSE(ccpService->HasPhoneCallId());
    ASSERT_FALSE(ccpService->HasVoipCallId());
    /* 非挂断首报不要求在表，但挂断依赖在表记录：先用全量入口铺底 */
    SetupPhoneCallMapping(ccpService, callId);
    ASSERT_TRUE(ccpService->HasPhoneCallId());

    CallAttributeInfo callAttributeInfo{};
    callAttributeInfo.callId = callId;
    callAttributeInfo.callState = TelCallState::CALL_STATUS_DIALING;
    ccpService->HandleCallDetailChange(callAttributeInfo.callId,
        static_cast<int32_t>(callAttributeInfo.callState), "", { callAttributeInfo });
    callAttributeInfo.callState = TelCallState::CALL_STATUS_INCOMING;
    ccpService->HandleCallDetailChange(callAttributeInfo.callId,
        static_cast<int32_t>(callAttributeInfo.callState), "", { callAttributeInfo });
    callAttributeInfo.callState = TelCallState::CALL_STATUS_WAITING;
    ccpService->HandleCallDetailChange(callAttributeInfo.callId,
        static_cast<int32_t>(callAttributeInfo.callState), "", { callAttributeInfo });
    /* 非挂断状态只更新明细，映射不得误清 */
    EXPECT_TRUE(ccpService->HasPhoneCallId());
    callAttributeInfo.callState = TelCallState::CALL_STATUS_DISCONNECTED;
    ccpService->HandleCallDetailChange(callAttributeInfo.callId,
        static_cast<int32_t>(callAttributeInfo.callState), "", { callAttributeInfo });
    /* 挂断明细生效：该路映射被回收 */
    EXPECT_FALSE(ccpService->HasPhoneCallId());
    callAttributeInfo.callState = TelCallState::CALL_STATUS_UNKNOWN;
    ccpService->HandleCallDetailChange(callAttributeInfo.callId,
        static_cast<int32_t>(callAttributeInfo.callState), "", { callAttributeInfo });
    /* UNKNOWN 走默认分支，不得新建任何映射 */
    EXPECT_FALSE(ccpService->HasPhoneCallId());
    EXPECT_FALSE(ccpService->HasVoipCallId());
    HILOGI("HandleCallDetailChange001 end");
}

/**
 * @tc.number: PhoneStateChanged001
 * @tc.name:
 * @tc.desc: 全量通话状态刷新不带 callId，不得新建 callId 映射（不污染 VoIP 判定）
 */
HWTEST_F(NearlinkCcpServiceTest, PhoneStateChanged001, TestSize.Level1)
{
    HILOGI("PhoneStateChanged001 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(nullptr, ccpService);

    /* 每个用例独占服务实例，喂入前必须是干净态 */
    ASSERT_FALSE(ccpService->HasVoipCallId());
    /* 无特定通话时传 INVALID_CCP_VOIP_ID/IDLE：仅做全量刷新 */
    ccpService->HandlePhoneStateChange(
        INVALID_CCP_VOIP_ID, static_cast<int32_t>(Telephony::TelCallState::CALL_STATUS_IDLE), "");
    /* 全量刷新不做 callId 入表，绝不能新增 VoIP 映射 */
    EXPECT_FALSE(ccpService->HasVoipCallId());
    HILOGI("PhoneStateChanged001 end");
}

/**
 * @tc.number: CallDetailsChanged001
 * @tc.name:
 * @tc.desc: 单路明细入口 callId 为 -1（no specific call）时不得入表，由在表校验兜底拦截，不污染 callId 映射
 */
HWTEST_F(NearlinkCcpServiceTest, CallDetailsChanged001, TestSize.Level1)
{
    HILOGI("CallDetailsChanged001 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(nullptr, ccpService);

    /* 每个用例独占服务实例，喂入前必须是干净态 */
    ASSERT_FALSE(ccpService->HasVoipCallId());
    CallAttributeInfo callAttributeInfo{};
    callAttributeInfo.callId = INVALID_CCP_VOIP_ID;
    callAttributeInfo.callState = TelCallState::CALL_STATUS_DISCONNECTED;
    callAttributeInfo.callType = static_cast<CallType>(CCP_SERVICE_UT_CALL_TYPE_NOT_VOIP);
    ccpService->HandleCallDetailChange(callAttributeInfo.callId,
        static_cast<int32_t>(callAttributeInfo.callState), "", { callAttributeInfo });
    /* callId=-1 仅表示无特定通话，绝不能新增 VoIP 映射 */
    EXPECT_FALSE(ccpService->HasVoipCallId());
    HILOGI("CallDetailsChanged001 end");
}

/**
 * @tc.number: CallDetailsChanged002
 * @tc.name:
 * @tc.desc: 存在蜂窝映射时挂断分支放行，DISCONNECTED 清理该 callId 映射
 */
HWTEST_F(NearlinkCcpServiceTest, CallDetailsChanged002, TestSize.Level1)
{
    HILOGI("CallDetailsChanged002 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(nullptr, ccpService);

    constexpr int32_t callId = 1;
    ClearPhoneCallMappings(ccpService);
    ASSERT_FALSE(ccpService->HasPhoneCallId());
    SetupPhoneCallMapping(ccpService, callId);
    ASSERT_TRUE(ccpService->HasPhoneCallId());

    CallAttributeInfo callAttributeInfo{};
    callAttributeInfo.callId = callId;
    callAttributeInfo.callState = TelCallState::CALL_STATUS_DISCONNECTED;
    callAttributeInfo.callType = static_cast<CallType>(CCP_SERVICE_UT_CALL_TYPE_NOT_VOIP);
    ccpService->HandleCallDetailChange(callAttributeInfo.callId,
        static_cast<int32_t>(callAttributeInfo.callState), "", { callAttributeInfo });
    /* 挂断明细生效：该路 callId 映射被清理 */
    EXPECT_FALSE(ccpService->HasPhoneCallId());
    HILOGI("CallDetailsChanged002 end");
}

/**
 * @tc.number: CallDetailsChanged003
 * @tc.name:
 * @tc.desc: 无蜂窝映射时守卫拦截，任何状态都不得新建映射（ccp 自造场景排除）
 */
HWTEST_F(NearlinkCcpServiceTest, CallDetailsChanged003, TestSize.Level1)
{
    HILOGI("CallDetailsChanged003 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(nullptr, ccpService);

    constexpr int32_t callId = 5;
    ClearPhoneCallMappings(ccpService);
    ASSERT_FALSE(ccpService->HasPhoneCallId());

    CallAttributeInfo callAttributeInfo{};
    callAttributeInfo.callId = callId;
    callAttributeInfo.callState = TelCallState::CALL_STATUS_DISCONNECTED;
    callAttributeInfo.callType = static_cast<CallType>(CCP_SERVICE_UT_CALL_TYPE_NOT_VOIP);
    ccpService->HandleCallDetailChange(callAttributeInfo.callId,
        static_cast<int32_t>(callAttributeInfo.callState), "", { callAttributeInfo });
    /* 无在表蜂窝通话时直接返回：不得反手分配 index 并写入映射 */
    EXPECT_FALSE(ccpService->HasPhoneCallId());
    EXPECT_FALSE(ccpService->HasVoipCallId());
    HILOGI("CallDetailsChanged003 end");
}

/**
 * @tc.number: CallDetailsChanged004
 * @tc.name:
 * @tc.desc: 非 DISCONNECTED 状态只更新明细，映射不得误清；结尾自行清场
 */
HWTEST_F(NearlinkCcpServiceTest, CallDetailsChanged004, TestSize.Level1)
{
    HILOGI("CallDetailsChanged004 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(nullptr, ccpService);

    constexpr int32_t callId = 2;
    ClearPhoneCallMappings(ccpService);
    SetupPhoneCallMapping(ccpService, callId);
    ASSERT_TRUE(ccpService->HasPhoneCallId());

    CallAttributeInfo callAttributeInfo{};
    callAttributeInfo.callId = callId;
    callAttributeInfo.callState = TelCallState::CALL_STATUS_INCOMING;
    callAttributeInfo.callType = static_cast<CallType>(CCP_SERVICE_UT_CALL_TYPE_NOT_VOIP);
    ccpService->HandleCallDetailChange(callAttributeInfo.callId,
        static_cast<int32_t>(callAttributeInfo.callState), "", { callAttributeInfo });
    /* 非挂断状态只更新明细，不得提前清映射 */
    EXPECT_TRUE(ccpService->HasPhoneCallId());

    /* 挂断明细生效：该路映射必须被回收 */
    callAttributeInfo.callState = TelCallState::CALL_STATUS_DISCONNECTED;
    ccpService->HandleCallDetailChange(callAttributeInfo.callId,
        static_cast<int32_t>(callAttributeInfo.callState), "", { callAttributeInfo });
    EXPECT_FALSE(ccpService->HasPhoneCallId());
    HILOGI("CallDetailsChanged004 end");
}

/**
 * @tc.number: CallDetailsChanged005
 * @tc.name:
 * @tc.desc: 守卫放行后 callId 为 -1（no specific call）仍被拦截，不得污染映射
 */
HWTEST_F(NearlinkCcpServiceTest, CallDetailsChanged005, TestSize.Level1)
{
    HILOGI("CallDetailsChanged005 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(nullptr, ccpService);

    constexpr int32_t callId = 3;
    ClearPhoneCallMappings(ccpService);
    SetupPhoneCallMapping(ccpService, callId);
    ASSERT_TRUE(ccpService->HasPhoneCallId());

    CallAttributeInfo callAttributeInfo{};
    callAttributeInfo.callId = INVALID_CCP_VOIP_ID;
    callAttributeInfo.callState = TelCallState::CALL_STATUS_DISCONNECTED;
    callAttributeInfo.callType = static_cast<CallType>(CCP_SERVICE_UT_CALL_TYPE_NOT_VOIP);
    ccpService->HandleCallDetailChange(callAttributeInfo.callId,
        static_cast<int32_t>(callAttributeInfo.callState), "", { callAttributeInfo });
    /* -1 仅表示无特定通话：既不能清掉在册的蜂窝映射，也不能自己入表 */
    EXPECT_TRUE(ccpService->HasPhoneCallId());
    EXPECT_FALSE(ccpService->HasVoipCallId());

    ClearPhoneCallMappings(ccpService);
    HILOGI("CallDetailsChanged005 end");
}

/**
 * @tc.number: CallDetailsChanged006
 * @tc.name:
 * @tc.desc: 在表的 VoIP callId（自造 VoIP，voipId=-2）挂断经单路明细入口清理
 */
HWTEST_F(NearlinkCcpServiceTest, CallDetailsChanged006, TestSize.Level1)
{
    HILOGI("CallDetailsChanged006 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(nullptr, ccpService);
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);

    constexpr int32_t voipId = NEARLINK_CCP_VOIP_MAX - 1;
    ClearPhoneCallMappings(ccpService);
    /* CCP 任务在测试线程内联执行，HandleVoipStart 返回即可取基线 */
    ccpService->HandleVoipStart(*device);
    ASSERT_TRUE(ccpService->HasVoipCallId());
    ASSERT_FALSE(ccpService->HasPhoneCallId());

    CallAttributeInfo callAttributeInfo{};
    callAttributeInfo.callId = voipId;
    callAttributeInfo.callState = TelCallState::CALL_STATUS_DISCONNECTED;
    callAttributeInfo.callType = CallType::TYPE_VOIP;
    ccpService->HandleCallDetailChange(callAttributeInfo.callId,
        static_cast<int32_t>(callAttributeInfo.callState), "", { callAttributeInfo });
    /* 在表的 VoIP callId 同样经受在表守卫，挂断后映射被清理 */
    EXPECT_FALSE(ccpService->HasVoipCallId());

    /* 收尾：显式停止自造 VoIP，验证 stop 入口能把映射清干净 */
    ccpService->HandleVoipStop(*device);
    EXPECT_FALSE(ccpService->HasVoipCallId());
    HILOGI("CallDetailsChanged006 end");
}

/**
 * @tc.number: CallDetailsChanged007
 * @tc.name:
 * @tc.desc: callType 不参与挂断判定：TYPE_VOIP 挂断与 002 同行为，映射被清理
 */
HWTEST_F(NearlinkCcpServiceTest, CallDetailsChanged007, TestSize.Level1)
{
    HILOGI("CallDetailsChanged007 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(nullptr, ccpService);

    constexpr int32_t callId = 7;
    ClearPhoneCallMappings(ccpService);
    SetupPhoneCallMapping(ccpService, callId);
    ASSERT_TRUE(ccpService->HasPhoneCallId());

    /* 002 的对照组：状态、callId 均相同，只把 callType 换成 TYPE_VOIP */
    CallAttributeInfo callAttributeInfo{};
    callAttributeInfo.callId = callId;
    callAttributeInfo.callState = TelCallState::CALL_STATUS_DISCONNECTED;
    callAttributeInfo.callType = CallType::TYPE_VOIP;
    ccpService->HandleCallDetailChange(callAttributeInfo.callId,
        static_cast<int32_t>(callAttributeInfo.callState), "", { callAttributeInfo });
    /* callType 不参与挂断判定，映射与 002 一样被清理 */
    EXPECT_FALSE(ccpService->HasPhoneCallId());

    /* 把剩余 callId 一并归还，确认清场后回到空表 */
    ClearPhoneCallMappings(ccpService);
    EXPECT_FALSE(ccpService->HasPhoneCallId());
    HILOGI("CallDetailsChanged007 end");
}

/**
 * @tc.number: HasPhoneCallId001
 * @tc.name:
 * @tc.desc: 蜂窝标识判定：空表为 false，存在 callId>0 映射为 true，清场后回到 false
 */
HWTEST_F(NearlinkCcpServiceTest, HasPhoneCallId001, TestSize.Level1)
{
    HILOGI("HasPhoneCallId001 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(nullptr, ccpService);

    constexpr int32_t callId = 4;
    ClearPhoneCallMappings(ccpService);
    EXPECT_FALSE(ccpService->HasPhoneCallId());

    SetupPhoneCallMapping(ccpService, callId);
    EXPECT_TRUE(ccpService->HasPhoneCallId());

    ClearPhoneCallMappings(ccpService);
    EXPECT_FALSE(ccpService->HasPhoneCallId());
    /* 仅负数 key（自造 VoIP）不算蜂窝路，该分支由 CallDetailsChanged006 覆盖 */
    HILOGI("HasPhoneCallId001 end");
}

/**
 * @tc.number: HandleVoipStart002
 * @tc.name:
 * @tc.desc: 存在蜂窝通话时不得自造 VoIP 状态（与 CallDetailsChanged 守卫同一判定）
 */
HWTEST_F(NearlinkCcpServiceTest, HandleVoipStart002, TestSize.Level1)
{
    HILOGI("HandleVoipStart002 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(nullptr, ccpService);
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);

    constexpr int32_t callId = 6;
    ClearPhoneCallMappings(ccpService);
    SetupPhoneCallMapping(ccpService, callId);
    ASSERT_TRUE(ccpService->HasPhoneCallId());

    ccpService->HandleVoipStart(*device);
    /* 蜂窝通话在表时守卫拦住，不得创建自造 VoIP */
    EXPECT_FALSE(ccpService->HasVoipCallId());

    /* 清场：HandleVoipStart 已登记 voipIdSet_/currentVoipCallId_，
     * 若只清蜂窝映射，挂断时会触发 VoIP 补建，故先显式停止 VoIP */
    ccpService->HandleVoipStop(*device);
    ClearPhoneCallMappings(ccpService);
    EXPECT_FALSE(ccpService->HasPhoneCallId());
    EXPECT_FALSE(ccpService->HasVoipCallId());
    HILOGI("HandleVoipStart002 end");
}

/**
 * @tc.number: HandleVoipStop001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkCcpServiceTest, HandleVoipStop001, TestSize.Level1)
{
    HILOGI("HandleVoipStop001 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(nullptr, ccpService);
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);

    /* 先起一路自造 VoIP：否则 currentVoipCallId_ 为无效值，stop 入口首句即返回、清理分支零覆盖 */
    ccpService->HandleVoipStart(*device);
    ASSERT_TRUE(ccpService->HasVoipCallId());

    ccpService->HandleVoipStop(*device);
    EXPECT_FALSE(ccpService->HasVoipCallId());

    /* 重复停止必须幂等：无在途 VoIP 时再停一次不得产生新映射 */
    ccpService->HandleVoipStop(*device);
    EXPECT_FALSE(ccpService->HasVoipCallId());
    HILOGI("HandleVoipStop001 end");
}

/**
 * @tc.number: HandleVoipStart001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkCcpServiceTest, HandleVoipStart001, TestSize.Level1)
{
    HILOGI("HandleVoipStart001 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(nullptr, ccpService);
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);

    ccpService->HandleVoipStart(*device);
    EXPECT_TRUE(ccpService->HasVoipCallId());

    /* 重复启动幂等：仍只登记同一路自造 VoIP */
    ccpService->HandleVoipStart(*device);
    EXPECT_TRUE(ccpService->HasVoipCallId());
    /* 收尾：显式停止，验证 stop 入口能把映射清干净 */
    ccpService->HandleVoipStop(*device);
    EXPECT_FALSE(ccpService->HasVoipCallId());
    HILOGI("HandleVoipStart001 end");
}

/**
 * @tc.number: TryResumeCurrentCalls001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkCcpServiceTest, TryResumeCurrentCalls001, TestSize.Level1)
{
    HILOGI("TryResumeCurrentCalls001 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(nullptr, ccpService);
    ccpService->TryResumeCurrentCalls();
    HILOGI("TryResumeCurrentCalls001 end");
}

/**
 * @tc.number: NotifyAudioDeviceAction001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkCcpServiceTest, NotifyAudioDeviceAction001, TestSize.Level1)
{
    HILOGI("NotifyAudioDeviceAction001 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(nullptr, ccpService);
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);
    int action = 0;
    ccpService->NotifyAudioDeviceAction(*device, action);
    HILOGI("NotifyAudioDeviceAction001 end");
}

/**
 * @tc.number: ProcessCallStateInfo001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkCcpServiceTest, ProcessCallStateInfo001, TestSize.Level1)
{
    HILOGI("ProcessCallStateInfo001 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(nullptr, ccpService);
    /* 蜂窝 callId 必须为正：喂 0 会在映射表留下 HasPhoneCallId/HasVoipCallId 都清不到的 key=0 条目 */
    constexpr int32_t callId = 1;
    std::vector<Telephony::CallAttributeInfo> callInfoVec;
    CallAttributeInfo callAttributeInfo{};
    callAttributeInfo.callId = callId;
    callAttributeInfo.callState = TelCallState::CALL_STATUS_INCOMING;
    callInfoVec.push_back(callAttributeInfo);

    ccpService->ProcessCallStateInfo(callInfoVec);
    EXPECT_TRUE(ccpService->HasPhoneCallId());

    /* 挂断后映射必须回收：结尾清场同时作为回收断言 */
    ClearPhoneCallMappings(ccpService);
    EXPECT_FALSE(ccpService->HasPhoneCallId());
    HILOGI("ProcessCallStateInfo001 end");
}

/**
 * @tc.number: ProcessAccountInfo001
 * @tc.name:
 * @tc.desc: 号码为空时仍写入协议前缀，别名保持为空，返回成功
 */
HWTEST_F(NearlinkCcpServiceTest, ProcessAccountInfo001, TestSize.Level1)
{
    HILOGI("ProcessAccountInfo001 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(nullptr, ccpService);
    std::shared_ptr<CallInOutInfoProp> prop = std::make_shared<CallInOutInfoProp>();
    prop->callId = 1;
    prop->callFlag = 0;
    CallAttributeInfo callAttributeInfo{};
    callAttributeInfo.callState = TelCallState::CALL_STATUS_INCOMING;
    EXPECT_TRUE(ccpService->ProcessAccountInfo(prop, callAttributeInfo));
    /* 号码为空：userInfo 只保留协议前缀，userAlias 不写入 */
    EXPECT_STREQ(CCP_SUPPORT_PROTOCOLS, prop->userInfo);
    EXPECT_STREQ("", prop->userAlias);
    HILOGI("ProcessAccountInfo001 end");
}

/**
 * @tc.number: NotifyAudioDeviceActionViaProfileAsc_001
 * @tc.name: NotifyAudioDeviceAction via ProfileASC interface
 * @tc.desc: Test NotifyAudioDeviceAction uses ProfileASC from profile manager, should not crash
 */
HWTEST_F(NearlinkCcpServiceTest, NotifyAudioDeviceActionViaProfileAsc_001, TestSize.Level1)
{
    HILOGI("NotifyAudioDeviceActionViaProfileAsc_001 start");
    CcpService* ccpService = CcpService::GetService();
    ASSERT_NE(nullptr, ccpService);
    RawAddress devAddr("11:22:33:44:55:66");
    ccpService->NotifyAudioDeviceAction(devAddr, 1);
    HILOGI("NotifyAudioDeviceActionViaProfileAsc_001 end");
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS