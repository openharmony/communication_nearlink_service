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

/*
 * 收口 service/common 尾部小文件单测：被测对象与依赖均为真实例。
 * 运行约束：仅在测试机执行——本机性写入（本地名/本地地址）逐用例快照恢复；
 * 涉及协议栈、数据订阅、公共事件的调用在测试环境按受控失败处理。
 * 包含 ProfileServiceManager.cpp 仅为隔离 samgr 注册路径，该源文件已从共享构建源排除。
 */

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "common_event_data.h"
#include "common_event_subscribe_info.h"
#include "matching_skills.h"
#include "want.h"

#include "CdsmService.h"
#include "Compat.h"
#include "ProfileInfo.h"
#include "ProfileServiceManager.cpp"
#include "SleCoexist.h"
#include "SleCoexistManager.h"
#include "SleCollaborationManager.h"
#include "SleCommonEventSubscriber.h"
#include "SleConfig.h"
#include "SleFeature.h"
#include "SleHiviewUe.h"
#include "nearlink_datashare_helper.h"
#include "SleProperties.h"
#include "SleDefs.h"
#include "SleSecurity.h"
#include "SleServiceManager.h"
#include "SysStateMachine.h"
#include "ThreadUtil.h"
#include "UnloadSa.h"
#include "cm.h"
#include "log.h"
#include "nearlink_def.h"
#include "nlstk_sm_api.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {

using namespace testing::ext;

const std::string kTailAddr = "A1:B2:C3:D4:E5:11";

class ServiceCommonTailTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
};

void ServiceCommonTailTest::SetUpTestCase()
{
    // 只创建 adapter 对象，不启动协议栈固件——测试进程没有固件运行环境
    SleServiceManager::GetInstance()->CreateAdapters();
    SleConfig::GetInstance().LoadConfigInfo();
}

void ServiceCommonTailTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase ServiceCommonTailTest");
}

void ServiceCommonTailTest::SetUp()
{
    // 适配器与 CDSM 任务在测试线程内同步完成
    ThreadUtil::GetInstance().threadStateMap_.EnsureInsert(
        THREAD_ID_ADAPTER, ThreadUtil::ThreadState::NOT_SWITCH_THREAD);
    ThreadUtil::GetInstance().threadStateMap_.EnsureInsert(
        THREAD_ID_DEVICE_ADAPTER, ThreadUtil::ThreadState::NOT_SWITCH_THREAD);
    ThreadUtil::GetInstance().threadStateMap_.EnsureInsert(
        THREAD_ID_CDSM, ThreadUtil::ThreadState::NOT_SWITCH_THREAD);
}

void ServiceCommonTailTest::TearDown()
{
    SleCoexistManager::GetInstance()->connInfoList_.Clear();
    auto &collaboration = SleCollaborationManager::GetInstance();
    collaboration.isCollaborationOn_.store(false);
    collaboration.cachedCollaborationEventVec_.clear();
    SleHiviewUe::GetInstance().isHiviewUeOn_.store(true);
    NLSTK_CfgdbLocalFeatures_S zeroFeature = {};
    SleFeature::GetInstance().SetLocalFeature(&zeroFeature);
    ThreadUtil::GetInstance().InitThreadStateMap();
}

/**
 * @tc.name: CoexistConnParam_001
 * @tc.desc: 连接参数变更：非法链路号被拒；多条短间隔连接触发参数刷新定时器；
 *           单连接且无定制参数时不刷新；无连接时参数刷新直接返回
 * @tc.type: FUNC
 */
HWTEST_F(ServiceCommonTailTest, CoexistConnParam_001, TestSize.Level1)
{
    auto coexist = std::make_shared<SleCoexist>();
    coexist->Init();
    coexist->updateDuration_ = 500;

    SLE_Addr_S addr = {};
    RawAddress(kTailAddr).ConvertToUint8(addr.addr);
    coexist->ConnectionStatusChanged(0x21, addr);
    coexist->ConnectionStatusChanged(0x22, addr);
    EXPECT_TRUE(SleCoexistManager::GetInstance()->HasMultipleConnections());

    // 非法链路号：连接信息不更新，定时器不启动
    CM_ConnectUpdateParamRsp_S badParam = {};
    badParam.lcid = INVALID_LCID;
    coexist->ConnectionParamChanged(badParam);
    EXPECT_EQ(nullptr, coexist->updateTimer_);

    // 多连接且间隔低于共存阈值：启动刷新定时器
    CM_ConnectUpdateParamRsp_S param = {};
    param.lcid = 0x21;
    param.extension.interval = 1;
    param.addr = addr;
    coexist->ConnectionParamChanged(param);
    EXPECT_NE(nullptr, coexist->updateTimer_);
    coexist->StopParamUpdateTimer();
    EXPECT_EQ(nullptr, coexist->updateTimer_);

    // 无连接时刷新：直接返回
    SleCoexistManager::GetInstance()->connInfoList_.Clear();
    coexist->UpdateConnParam();

    // 刷新周期非正：不建定时器
    coexist->updateDuration_ = 0;
    coexist->StartParamUpdateTimer();
    EXPECT_EQ(nullptr, coexist->updateTimer_);
}

/**
 * @tc.name: PropertiesUpdateConfig_001
 * @tc.desc: 本地名/本地地址配置更新：与配置不一致时写入并落盘，一致时不重复写；
 *           未知配置类型不做任何变更
 * @tc.type: FUNC
 */
HWTEST_F(ServiceCommonTailTest, PropertiesUpdateConfig_001, TestSize.Level1)
{
    const std::string savedName = SleConfig::GetInstance().GetLocalName();
    const std::string savedAddr = SleConfig::GetInstance().GetLocalAddress();

    // 内存名未随配置加载（加载入口在适配器启用流程）：配置名与内存名不一致时，
    // 更新把内存名（空串）同步回配置
    ASSERT_TRUE(SleConfig::GetInstance().SetLocalName("TailSuiteName"));
    SleProperties::GetInstance().UpdateConfig(static_cast<int>(SLE_CONFIG::SLE_CONFIG_LOCAL_NAME));
    EXPECT_EQ("", SleConfig::GetInstance().GetLocalName());
    // 名称一致后再次更新：走不重复落盘路径，配置不变
    SleProperties::GetInstance().UpdateConfig(static_cast<int>(SLE_CONFIG::SLE_CONFIG_LOCAL_NAME));
    EXPECT_EQ("", SleConfig::GetInstance().GetLocalName());

    // 地址更新走产品真实路径（从控制器读地址后同步配置）；地址值依赖环境只验证流程可完成
    (void)SleProperties::GetInstance().GetAddrFromController();

    // 未知类型：配置不受影响
    SleProperties::GetInstance().UpdateConfig(0x99);
    EXPECT_EQ("", SleConfig::GetInstance().GetLocalName());

    SleConfig::GetInstance().SetLocalName(savedName);
    SleConfig::GetInstance().SetLocalAddress(savedAddr);
    SleConfig::GetInstance().Save();
}

/**
 * @tc.name: CollaborationEvent_001
 * @tc.desc: 多设备协同状态：状态未变时忽略；缓存队列消化时同值事件只清理不重派；
 *           缓存出现相反状态且星闪处于关断稳定态时按最新状态重派
 * @tc.type: FUNC
 */
HWTEST_F(ServiceCommonTailTest, CollaborationEvent_001, TestSize.Level1)
{
    auto &collaboration = SleCollaborationManager::GetInstance();
    collaboration.isCollaborationOn_.store(true);

    // 状态未变：直接返回，不进事件缓存
    collaboration.OnCollaborationStateChanged(true);
    EXPECT_TRUE(collaboration.cachedCollaborationEventVec_.empty());

    // 空缓存：执行完成直接返回
    collaboration.CurrentCollaborationEventExecuteComplete();
    EXPECT_TRUE(collaboration.cachedCollaborationEventVec_.empty());

    // 缓存同值事件：只清理缓存，不重派
    collaboration.cachedCollaborationEventVec_ = {true, true};
    collaboration.CurrentCollaborationEventExecuteComplete();
    EXPECT_TRUE(collaboration.cachedCollaborationEventVec_.empty());
    EXPECT_TRUE(collaboration.IsCollaborationOn());

    // 缓存相反事件：未被抑制时按最新状态重派并落到关断；飞行模式开启或星闪已开启时重派被抑制
    collaboration.isCollaborationOn_.store(true);
    collaboration.cachedCollaborationEventVec_ = {true, false};
    collaboration.CurrentCollaborationEventExecuteComplete();
    EXPECT_TRUE(collaboration.cachedCollaborationEventVec_.empty());
    bool isSuppressed = NearlinkDataShareHelper::GetInstance().GetAirplaneModeState() ||
        SleServiceManager::GetInstance()->GetState(ADAPTER_SLE) == SleStateID::STATE_TURN_ON;
    EXPECT_EQ(isSuppressed, collaboration.IsCollaborationOn());
}

/**
 * @tc.name: SecurityCallbacks_001
 * @tc.desc: 安全管理回调直驱适配器：配对开始/完成/加密完成/取消完成/配对请求各回调
 *           在真实适配器上安全返回；配对答复接受时按产品约定返回失败；
 *           协议栈未初始化时发起配对按失败返回
 * @tc.type: FUNC
 */
HWTEST_F(ServiceCommonTailTest, SecurityCallbacks_001, TestSize.Level1)
{
    auto *adapterSub = static_cast<SleInterfaceAdapterSub *>(
        SleInterfaceManager::GetInstance()->GetAdapter(SleTransport::ADAPTER_SLE));
    ASSERT_NE(nullptr, adapterSub);
    SleSecurity security(*adapterSub);

    SLE_Addr_S addr = {};
    RawAddress(kTailAddr).ConvertToUint8(addr.addr);

    NLSTK_SmPairingStart_S startParam = {};
    startParam.addr = addr;
    EXPECT_TRUE(security.PairStartStatusChange(startParam));

    NLSTK_SmEncComplete_S encParam = {};
    encParam.addr = addr;
    EXPECT_TRUE(security.SmpEncComplete(encParam));

    NLSTK_SmPairingRemove_S removeParam = {};
    removeParam.addr = addr;
    EXPECT_TRUE(security.SmpPairCancelComplete(removeParam));

    NLSTK_SmPairingRequest_S reqParam = {};
    reqParam.addr = addr;
    EXPECT_TRUE(security.SmpPairingReq(reqParam));

    // 非成功也非主动取消的认证结果：按配对完成回调适配器
    NLSTK_SmAuthComplete_S authParam = {};
    authParam.addr = addr;
    authParam.authStatus = SM_PAIR_ERROR;
    EXPECT_TRUE(security.SmpAuthComplete(authParam));

    // 主动取消且无 CDSM 服务：返回失败
    authParam.authStatus = SM_PAIR_ACTIVE_CANCEL;
    EXPECT_FALSE(security.SmpAuthComplete(authParam));

    EXPECT_FALSE(security.PairRequestReply(RawAddress(kTailAddr), 0, true));
    EXPECT_FALSE(security.StartPair(RawAddress(kTailAddr), 0));
}

/**
 * @tc.name: SecuritySavePairKey_001
 * @tc.desc: 配对成功回调保存链路密钥：密钥经加密后写入配置；
 *           非私有音频设备的合作集共享密钥路径直接跳过
 * @tc.type: FUNC
 */
HWTEST_F(ServiceCommonTailTest, SecuritySavePairKey_001, TestSize.Level1)
{
    auto *adapterSub = static_cast<SleInterfaceAdapterSub *>(
        SleInterfaceManager::GetInstance()->GetAdapter(SleTransport::ADAPTER_SLE));
    ASSERT_NE(nullptr, adapterSub);
    SleSecurity security(*adapterSub);

    SLE_Addr_S addr = {};
    RawAddress(kTailAddr).ConvertToUint8(addr.addr);
    NLSTK_SmAuthComplete_S authParam = {};
    authParam.addr = addr;
    authParam.authStatus = SM_PAIR_OK;
    for (int i = 0; i < SM_LINK_KEY_LEN; ++i) {
        authParam.linkKey[i] = static_cast<uint8_t>(i);
    }
    EXPECT_TRUE(security.SmpAuthComplete(authParam));

    // 普通设备（非私有音频业务）：共享密钥路径不改动已保存的链路密钥
    const std::string savedLinkKey = SleConfig::GetInstance().GetLinkKey(kTailAddr);
    EncryptedLinkKey sharedKey = {};
    EncryptedLinkKey sharedGroupKey = {};
    security.SaveCdsmOtherAddressKey(RawAddress(kTailAddr), sharedKey, sharedGroupKey, authParam);
    EXPECT_EQ(savedLinkKey, SleConfig::GetInstance().GetLinkKey(kTailAddr));

    SleConfig::GetInstance().RemovePairedDevice(kTailAddr);
    SleConfig::GetInstance().Save();
}

/**
 * @tc.name: SysStateMachineDispatch_001
 * @tc.desc: 系统状态机各状态消息路由：受支持消息返回成功，未知消息一律拒绝；
 *           适配器状态变更按迁移中态处理不触发停止/重启副作用
 * @tc.type: FUNC
 */
HWTEST_F(ServiceCommonTailTest, SysStateMachineDispatch_001, TestSize.Level1)
{
    SysStateMachine stateMachine;
    SleServiceManager &am = *SleServiceManager::GetInstance();

    SysStoppedState stopped(stateMachine, am);
    utility::Message startMsg(static_cast<int>(SysStateMachine::MSG_SYS_START_CMP));
    EXPECT_TRUE(stopped.Dispatch(startMsg));
    EXPECT_FALSE(stopped.Dispatch(utility::Message(0x99)));

    SysStartedState started(stateMachine, am);
    EXPECT_TRUE(started.Dispatch(utility::Message(
        static_cast<int>(SysStateMachine::MSG_SYS_RESET_REQ))));
    EXPECT_TRUE(started.Dispatch(utility::Message(
        static_cast<int>(SysStateMachine::MSG_SYS_FACTORY_RESET_REQ))));
    EXPECT_TRUE(started.Dispatch(utility::Message(
        static_cast<int>(SysStateMachine::MSG_SYS_STOP_REQ))));
    EXPECT_FALSE(started.Dispatch(utility::Message(0x99)));

    SysStoppingState stopping(stateMachine, am);
    EXPECT_TRUE(stopping.Dispatch(utility::Message(
        static_cast<int>(SysStateMachine::MSG_SYS_STOP_CMP))));
    utility::Message adapterMsg(static_cast<int>(SysStateMachine::MSG_SYS_ADAPTER_STATE_CHANGE_REQ));
    adapterMsg.arg1M = STATE_TURNING_ON;
    EXPECT_TRUE(stopping.Dispatch(adapterMsg));
    EXPECT_FALSE(stopping.Dispatch(utility::Message(0x99)));

    SysResettingState resetting(stateMachine, am);
    EXPECT_TRUE(resetting.Dispatch(adapterMsg));
    EXPECT_TRUE(resetting.Dispatch(utility::Message(
        static_cast<int>(SysStateMachine::MSG_SYS_STOP_REQ))));
    EXPECT_FALSE(resetting.Dispatch(utility::Message(0x99)));

    SysFactoryResettingState factoryResetting(stateMachine, am);
    utility::Message adapterOnMsg(static_cast<int>(SysStateMachine::MSG_SYS_ADAPTER_STATE_CHANGE_REQ));
    adapterOnMsg.arg1M = STATE_TURNING_ON;
    EXPECT_TRUE(factoryResetting.Dispatch(adapterOnMsg));
    EXPECT_TRUE(factoryResetting.Dispatch(utility::Message(
        static_cast<int>(SysStateMachine::MSG_SYS_STOP_CMP))));
    EXPECT_TRUE(factoryResetting.Dispatch(utility::Message(
        static_cast<int>(SysStateMachine::MSG_SYS_STOP_REQ))));
    EXPECT_TRUE(factoryResetting.Dispatch(utility::Message(
        static_cast<int>(SysStateMachine::MSG_SYS_CLEAR_ALL_STORAGE_CMP))));
    EXPECT_FALSE(factoryResetting.Dispatch(utility::Message(0x99)));
}

/**
 * @tc.name: UnloadSaTimer_001
 * @tc.desc: 卸载定时器只在星闪关断态启动且不重复启动，停止可重复；
 *           空闲插件缺失时按受控失败返回
 * @tc.type: FUNC
 */
HWTEST_F(ServiceCommonTailTest, UnloadSaTimer_001, TestSize.Level1)
{
    auto &unloadSa = UnloadSa::GetInstance();

    unloadSa.StartUnloadNearlinkSaTimer(STATE_TURN_ON);
    EXPECT_FALSE(unloadSa.isTimerStarted_.load());

    unloadSa.StartUnloadNearlinkSaTimer(STATE_TURN_OFF);
    EXPECT_TRUE(unloadSa.isTimerStarted_.load());
    unloadSa.StartUnloadNearlinkSaTimer(STATE_TURN_OFF);
    EXPECT_TRUE(unloadSa.isTimerStarted_.load());

    unloadSa.StopUnloadNearlinkSaTimer();
    EXPECT_FALSE(unloadSa.isTimerStarted_.load());
    unloadSa.StopUnloadNearlinkSaTimer();
    EXPECT_FALSE(unloadSa.isTimerStarted_.load());

    unloadSa.NearlinkHostExtOnIdleInner();
}

/**
 * @tc.name: HiviewUeState_001
 * @tc.desc: 维测开关状态：未变时忽略，变化时更新并记录
 * @tc.type: FUNC
 */
HWTEST_F(ServiceCommonTailTest, HiviewUeState_001, TestSize.Level1)
{
    auto &hiviewUe = SleHiviewUe::GetInstance();
    hiviewUe.isHiviewUeOn_.store(false);

    hiviewUe.OnHiviewUeStateChanged(false);
    EXPECT_FALSE(hiviewUe.isHiviewUeOn_.load());

    hiviewUe.OnHiviewUeStateChanged(true);
    EXPECT_TRUE(hiviewUe.isHiviewUeOn_.load());
}

/**
 * @tc.name: CompatCheck_001
 * @tc.desc: 兼容清单按前缀匹配：已知前缀命中，未知前缀与未知类型都不命中
 * @tc.type: FUNC
 */
HWTEST_F(ServiceCommonTailTest, CompatCheck_001, TestSize.Level1)
{
    EXPECT_TRUE(Compat::CompatCheck(CompatType::COMPAT_DISABLE_BLE_SECURE_CONNECTIONS, "08:62:66:11:22:33"));
    EXPECT_TRUE(Compat::CompatCheck(CompatType::COMPAT_REJECT_AUTO_PAIRING, "BMW 320i"));
    EXPECT_FALSE(Compat::CompatCheck(CompatType::COMPAT_DISABLE_BLE_SECURE_CONNECTIONS, "00:11:22:33:44:55"));
    EXPECT_FALSE(Compat::CompatCheck(CompatType::COMPAT_KEYBOARD_REQUIRES_FIXED_PIN, "BMW 320i"));
}

/**
 * @tc.name: ProfileIdToName_001
 * @tc.desc: profile 编号转名称：已支持编号返回注册名，未知编号返回空
 * @tc.type: FUNC
 */
HWTEST_F(ServiceCommonTailTest, ProfileIdToName_001, TestSize.Level1)
{
    EXPECT_EQ(PROFILE_NAME_CDSM, SupportProfilesInfo::IdToName(0x00004006));
    EXPECT_EQ("", SupportProfilesInfo::IdToName(0x0000DEAD));
}

/**
 * @tc.name: CommonEventSubscriber_001
 * @tc.desc: 公共事件订阅者只回调订阅动作匹配的事件，其他动作被忽略
 * @tc.type: FUNC
 */
HWTEST_F(ServiceCommonTailTest, CommonEventSubscriber_001, TestSize.Level1)
{
    int callbackCount = 0;
    EventFwk::MatchingSkills skills;
    skills.AddEvent("com.nearlink.test.tail");
    EventFwk::CommonEventSubscribeInfo info(skills);
    SleCommonEventSubscriber subscriber(info, "com.nearlink.test.tail",
        [&callbackCount](const EventFwk::CommonEventData &data) -> void { ++callbackCount; });

    AAFwk::Want matched;
    matched.SetAction("com.nearlink.test.tail");
    subscriber.OnReceiveEvent(EventFwk::CommonEventData(matched));
    EXPECT_EQ(1, callbackCount);

    AAFwk::Want other;
    other.SetAction("com.nearlink.test.other");
    subscriber.OnReceiveEvent(EventFwk::CommonEventData(other));
    EXPECT_EQ(1, callbackCount);
}

/**
 * @tc.name: SleFeatureRange_001
 * @tc.desc: 测距特性按本机能力位判定：能力位置位后支持，清零后不支持
 * @tc.type: FUNC
 */
HWTEST_F(ServiceCommonTailTest, SleFeatureRange_001, TestSize.Level1)
{
    EXPECT_FALSE(SleFeature::GetInstance().IsRangSupported());

    NLSTK_CfgdbLocalFeatures_S param = {};
    param.feats[NEARLINK_FEATURE_INDEX_8] = NEARLINK_FEATURE_RANGE;
    SleFeature::GetInstance().SetLocalFeature(&param);
    EXPECT_TRUE(SleFeature::GetInstance().IsRangSupported());

    NLSTK_CfgdbLocalFeatures_S zero = {};
    SleFeature::GetInstance().SetLocalFeature(&zero);
    EXPECT_FALSE(SleFeature::GetInstance().IsRangSupported());
}
} // namespace TEST
} // namespace Nearlink
} // namespace OHOS
