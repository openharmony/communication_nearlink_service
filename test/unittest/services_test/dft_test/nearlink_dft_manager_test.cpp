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

#include <vector>
#include "nearlink_dft_utils.h"
#include "nearlink_app_wakeup_ue.h"
#include "nearlink_dft_exception.h"
#include "nearlink_dft_manager_c.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS::Nearlink;

HWTEST(NearlinkDftManagerTest, StartTest_001, TestSize.Level1)
{
    DftManagerStart();
    DftManagerStop();
}

HWTEST(NearlinkDftManagerTest, StopTest_001, TestSize.Level1)
{
    DftManagerStart();
    DftManagerStop();
}

HWTEST(NearlinkDftManagerTest, CacheTest_001, TestSize.Level1)
{
    DftManagerStart();
    std::string app = "com.hxxwxx.app";
    std::string time = "10:36:32 145";
    std::vector<DftParamC> p1;
    p1.emplace_back(CreateStrParamC(SWITCH_USED_APP, app));
    p1.emplace_back(CreateStrParamC(SWITCH_START_TIME, time));
    DftManagerCache(DFT_SWITCH_EXCEP, p1.data(), p1.size());
    DftManagerStop();
}

HWTEST(NearlinkDftManagerTest, ReportTest_001, TestSize.Level1)
{
    DftManagerStart();
    std::string app = "com.hxxwxx.app";
    std::string time = "10:36:32 145";
    std::vector<DftParamC> p1;
    p1.emplace_back(CreateStrParamC(SWITCH_USED_APP, app));
    p1.emplace_back(CreateStrParamC(SWITCH_START_TIME, time));
    DftManagerCache(DFT_SWITCH_EXCEP, p1.data(), p1.size());

    std::vector<DftParamC> p2;
    p2.emplace_back(CreateStrParamC(SWITCH_END_TIME, time));
    DftManagerCache(DFT_SWITCH_EXCEP, p2.data(), p2.size());

    std::string state = "1->2->3->4";
    std::vector<DftParamC> p3;
    p3.emplace_back(CreateStrParamC(STATE_FLOW, state));
    DftManagerCache(DFT_FLOW_STATE, p3.data(), p3.size());

    DftSubEventRefC ref1 = {DFT_FLOW_STATE, nullptr, 0};

    std::vector<DftParamC> p4;
    p4.emplace_back(CreateUi16ParamC(SWITCH_ERR_CODE, 1));
    p4.emplace_back(CreateRefParamC(SUB_STATE_FLOW, ref1));
    DftManagerReport(DFT_SWITCH_EXCEP, p4.data(), p4.size());

    sleep(3);
    DftManagerStop();
}
/**
 * @tc.name: NearlinkDftManagerTest005
 * @tc.desc: 校验带类型地址格式校验：标准20字节地址通过；空串、长度差一、非十六进制字符、冒号位错字符均被拒绝
 * @tc.type: FUNC
 */
HWTEST(NearlinkDftManagerTest, AddressWithTypeTest_001, TestSize.Level1)
{
    ASSERT_TRUE(IsValidAddressWithType("00:11:22:33:44:55_01"));
    ASSERT_FALSE(IsValidAddressWithType(""));
    ASSERT_FALSE(IsValidAddressWithType("00:11:22:33:44:5_01"));
    ASSERT_FALSE(IsValidAddressWithType("g0:11:22:33:44:55_01"));
    ASSERT_FALSE(IsValidAddressWithType("0011:22:33:44:55:66_01"));
}

/**
 * @tc.name: NearlinkDftManagerTest006
 * @tc.desc: 校验地址前缀校验只看前17字节：合法前缀带任意尾缀都通过，空串与前缀内坏字符被拒绝
 * @tc.type: FUNC
 */
HWTEST(NearlinkDftManagerTest, AddressWithSuffixTest_001, TestSize.Level1)
{
    ASSERT_TRUE(IsValidAddressWithSuffix("00:11:22:33:44:55"));
    ASSERT_TRUE(IsValidAddressWithSuffix("00:11:22:33:44:55_anything"));
    ASSERT_FALSE(IsValidAddressWithSuffix(""));
    ASSERT_FALSE(IsValidAddressWithSuffix("00:11:22:33:44:g5"));
    ASSERT_FALSE(IsValidAddressWithSuffix("0:11:22:33:44:55"));
}

/**
 * @tc.name: NearlinkDftManagerTest007
 * @tc.desc: 校验带类型地址的打码输出：高两字节与低字节保留、中间三字节打码、类型后缀原样拼接；非法输入返回空串
 * @tc.type: FUNC
 */
HWTEST(NearlinkDftManagerTest, EncryptAddrWithTypeTest_001, TestSize.Level1)
{
    ASSERT_EQ(GetEncryptAddrWithType("00:11:22:33:44:55_01"), "55:44:**:**:**:00_01");
    ASSERT_EQ(GetEncryptAddrWithType(""), "");
    ASSERT_EQ(GetEncryptAddrWithType("00:11:22:33:44:55"), "");
}

/**
 * @tc.name: NearlinkDftManagerTest008
 * @tc.desc: 校验毫秒时间串格式（带日期23位、不带日期12位）与字符串包裹、地址转小写工具
 * @tc.type: FUNC
 */
HWTEST(NearlinkDftManagerTest, TimeAndStringHelperTest_001, TestSize.Level1)
{
    ASSERT_EQ(GetMillTime(true).length(), 23);
    ASSERT_EQ(GetMillTime(false).length(), 12);
    ASSERT_GT(GetTimestamp(), 0);
    ASSERT_EQ(JsonStringWrap("abc"), "\"abc\"");
    ASSERT_EQ(GetLowercaseAddr("AA:BB:CC"), "aa:bb:cc");
}

/**
 * @tc.name: NearlinkDftManagerTest009
 * @tc.desc: 校验C参数构造器按声明的类型携带值：布尔/多宽度整型/字符串均原样取回
 * @tc.type: FUNC
 */
HWTEST(NearlinkDftManagerTest, CreateParamTest_001, TestSize.Level1)
{
    ASSERT_EQ(CreateBoolParamC(1, true).t, DFT_BOOL);
    ASSERT_EQ(CreateBoolParamC(1, true).u.v.b, true);
    ASSERT_EQ(CreateUi8ParamC(1, 0x7B).u.v.ui8, 0x7B);
    ASSERT_EQ(CreateI8ParamC(1, -3).u.v.i8, -3);
    ASSERT_EQ(CreateUi16ParamC(1, 0x1234).u.v.ui16, 0x1234);
    ASSERT_EQ(CreateI16ParamC(1, -300).u.v.i16, -300);
    ASSERT_EQ(CreateUi32ParamC(1, 0x12345678).u.v.ui32, 0x12345678);
    ASSERT_EQ(CreateI32ParamC(1, -70000).u.v.i32, -70000);
    std::string app = "com.example.app";
    DftParamC strParam = CreateStrParamC(1, app);
    ASSERT_EQ(strParam.t, DFT_STRING);
    ASSERT_EQ(std::string(strParam.u.v.s), app);
}

/**
 * @tc.name: NearlinkDftManagerTest010
 * @tc.desc: 校验参数集合转换的过滤规则：空入参返回空集，类型与事件不匹配的参数被剔除，合法参数进集合
 * @tc.type: FUNC
 */
HWTEST(NearlinkDftManagerTest, ConvertToParamSetTest_001, TestSize.Level1)
{
    ASSERT_TRUE(DftParam::ConvertToParamSet(DFT_SWITCH_EXCEP, nullptr, 2).empty());
    ASSERT_TRUE(DftParam::ConvertToParamSet(DFT_SWITCH_EXCEP, nullptr, 0).empty());

    // 开关事件的应用名必须是字符串：整型形式被剔除
    std::vector<DftParamC> wrong;
    wrong.emplace_back(CreateUi16ParamC(SWITCH_USED_APP, 1));
    ASSERT_TRUE(DftParam::ConvertToParamSet(DFT_SWITCH_EXCEP, wrong.data(), wrong.size()).empty());

    std::string app = "com.example.app";
    std::vector<DftParamC> right;
    right.emplace_back(CreateStrParamC(SWITCH_USED_APP, app));
    right.emplace_back(CreateUi16ParamC(SWITCH_ERR_CODE, 1));
    auto set = DftParam::ConvertToParamSet(DFT_SWITCH_EXCEP, right.data(), right.size());
    ASSERT_EQ(set.size(), 2);
}

/**
 * @tc.name: NearlinkDftManagerTest011
 * @tc.desc: 校验子事件引用参数在子事件位置被剔除、在普通位置可进集合；集合按参数号排序；空指针比较返回假
 * @tc.type: FUNC
 */
HWTEST(NearlinkDftManagerTest, ConvertSubRefAndOrderTest_001, TestSize.Level1)
{
    std::vector<DftParamC> keys;
    keys.emplace_back(CreateUi16ParamC(SWITCH_ERR_CODE, 1));
    DftSubEventRefC ref = {DFT_FLOW_STATE, keys.data(), keys.size()};
    std::vector<DftParamC> params;
    params.emplace_back(CreateRefParamC(SUB_STATE_FLOW, ref));

    ASSERT_TRUE(DftParam::ConvertToParamSet(DFT_SWITCH_EXCEP, params.data(), params.size(), true).empty());
    ASSERT_EQ(DftParam::ConvertToParamSet(DFT_SWITCH_EXCEP, params.data(), params.size()).size(), 1);

    std::string app = "com.example.app";
    std::vector<DftParamC> two;
    two.emplace_back(CreateUi16ParamC(SWITCH_ERR_CODE, 1));
    two.emplace_back(CreateStrParamC(SWITCH_USED_APP, app));
    auto set = DftParam::ConvertToParamSet(DFT_SWITCH_EXCEP, two.data(), two.size());
    ASSERT_EQ((*set.begin())->GetParamId(), SWITCH_ERR_CODE);

    DftParam::TypeLessCmp cmp;
    ASSERT_FALSE(cmp(nullptr, nullptr));
    ASSERT_FALSE(cmp(*set.begin(), nullptr));
    ASSERT_FALSE(cmp(nullptr, *set.begin()));
}

/**
 * @tc.name: NearlinkDftManagerTest012
 * @tc.desc: 校验事件/参数元数据查表：非法事件号返回空名与默认域名，行为事件归UE域，故障事件归SERVICE域
 * @tc.type: FUNC
 */
HWTEST(NearlinkDftManagerTest, EventMetaLookupTest_001, TestSize.Level1)
{
    ASSERT_EQ(ConvertToHiSysType(DFT_PARAM_TYPE_INVALID), HISYSEVENT_INVALID);
    ASSERT_EQ(ConvertToHiSysType(DFT_TYPE_BUTT), HISYSEVENT_INVALID);
    ASSERT_EQ(ConvertToHiSysType(DFT_BOOL), HISYSEVENT_BOOL);

    ASSERT_STREQ(GetEventName(DFT_EVENT_INVALID), "");
    ASSERT_STRNE(GetEventName(DFT_SWITCH_EXCEP), "");
    ASSERT_STREQ(GetParamName(DFT_SWITCH_EXCEP, SWITCH_USED_APP, DFT_UINT16), "");
    ASSERT_STRNE(GetParamName(DFT_SWITCH_EXCEP, SWITCH_USED_APP, DFT_STRING), "");

    ASSERT_EQ(GetEventType(DFT_EVENT_INVALID), HISYSEVENT_FAULT);
    ASSERT_STREQ(GetDomainName(DFT_EVENT_INVALID), "NEARLINK_SERVICE");
    ASSERT_STREQ(GetDomainName(DFT_SWITCH_EXCEP), "NEARLINK_SERVICE");
    ASSERT_STREQ(GetDomainName(DFT_PAIRING_START_UE), "NEARLINK_UE");
}

/**
 * @tc.name: NearlinkDftManagerTest013
 * @tc.desc: 校验hiview开关状态读写：关闭后上报直接返回，重新打开恢复上报通路，全程不崩溃
 * @tc.type: FUNC
 */
HWTEST(NearlinkDftManagerTest, HiviewUeGateTest_001, TestSize.Level1)
{
    std::string app = "com.example.app";
    std::vector<DftParamC> params;
    params.emplace_back(CreateStrParamC(SWITCH_USED_APP, app));

    DftHiviewUeStateChanged(false);
    ASSERT_FALSE(DftHiviewUeStateIsOn());
    DftManagerReport(DFT_SWITCH_EXCEP, params.data(), params.size());
    DftManagerEraseCache(DFT_SWITCH_EXCEP, params.data(), params.size());
    // 开关关闭时应用唤醒采集直接返回，不进入订阅初始化
    NearlinkAppWakeupUe::GetInstance().Record(AppBehaviorType::SCAN_TRANSFER);
    ASSERT_FALSE(DftHiviewUeStateIsOn());

    DftHiviewUeStateChanged(true);
    // 开关恢复后采集走到订阅初始化与亮屏默认态早退
    NearlinkAppWakeupUe::GetInstance().Record(AppBehaviorType::SCAN_TRANSFER);
    NearlinkAppWakeupUe::GetInstance().Shutdown();
    ASSERT_TRUE(DftHiviewUeStateIsOn());
    DftManagerStart();
    DftManagerReport(DFT_SWITCH_EXCEP, params.data(), params.size());
    sleep(1);
    DftManagerStop();
}

/**
 * @tc.name: NearlinkDftManagerTest014
 * @tc.desc: 校验C入口参数门：非法事件号、空参数、零长度参数一律早退，不进入管理器
 * @tc.type: FUNC
 */
HWTEST(NearlinkDftManagerTest, ManagerEntryGuardTest_001, TestSize.Level1)
{
    std::string app = "com.example.app";
    std::vector<DftParamC> params;
    params.emplace_back(CreateStrParamC(SWITCH_USED_APP, app));

    DftManagerReport(DFT_EVENT_INVALID, params.data(), params.size());
    DftManagerReport(DFT_EVENT_BUTT, params.data(), params.size());
    DftManagerCache(DFT_SWITCH_EXCEP, nullptr, 2);
    DftManagerCache(DFT_SWITCH_EXCEP, params.data(), 0);
    DftManagerCache(DFT_EVENT_INVALID, params.data(), params.size());
    DftManagerEraseCache(DFT_SWITCH_EXCEP, nullptr, 2);
    DftManagerEraseCache(DFT_EVENT_INVALID, params.data(), params.size());
}

/**
 * @tc.name: NearlinkDftManagerTest015
 * @tc.desc: 校验管理器生命周期幂等：未启动直接停、重复启动、重复停止均安全；缓存合并与LRU逐出不出错
 * @tc.type: FUNC
 */
HWTEST(NearlinkDftManagerTest, ManagerLifecycleTest_001, TestSize.Level1)
{
    DftManagerStop();
    DftManagerStart();
    DftManagerStart();

    std::string app = "com.example.app";
    std::string time = "10:36:32 145";
    std::vector<DftParamC> keys;
    keys.emplace_back(CreateStrParamC(SWITCH_USED_APP, app));
    // 同键两轮缓存走合并路径
    DftManagerCache(DFT_SWITCH_EXCEP, keys.data(), keys.size());
    std::vector<DftParamC> more;
    more.emplace_back(CreateStrParamC(SWITCH_USED_APP, app));
    more.emplace_back(CreateStrParamC(SWITCH_START_TIME, time));
    DftManagerCache(DFT_SWITCH_EXCEP, more.data(), more.size());
    // 超过LRU容量的不同键触发逐出
    for (int i = 0; i < 12; i++) {
        std::vector<DftParamC> addrKeys;
        addrKeys.emplace_back(CreateStrParamC(SLE_DEVICE_ADDR, "11:22:33:44:55:" + std::to_string(10 + i)));
        DftManagerCache(DFT_PAIR_EXCEP, addrKeys.data(), addrKeys.size());
    }
    DftManagerReport(DFT_SWITCH_EXCEP, more.data(), more.size());
    DftManagerEraseCache(DFT_SWITCH_EXCEP, keys.data(), keys.size());
    sleep(2);
    DftManagerStop();
    DftManagerStop();
    // 停止后再缓存/上报：早退不崩溃
    DftManagerCache(DFT_SWITCH_EXCEP, keys.data(), keys.size());
    DftManagerReport(DFT_SWITCH_EXCEP, keys.data(), keys.size());
}

/**
 * @tc.name: NearlinkDftManagerTest017
 * @tc.desc: 校验未启动管理器时缓存与上报直接丢弃，启动后恢复入队，全程不崩溃
 * @tc.type: FUNC
 */
HWTEST(NearlinkDftManagerTest, ManagerPreStartTest_001, TestSize.Level1)
{
    std::string app = "com.example.app";
    std::vector<DftParamC> params;
    params.emplace_back(CreateStrParamC(SWITCH_USED_APP, app));
    DftManagerStop();
    DftManagerReport(DFT_SWITCH_EXCEP, params.data(), params.size());
    DftManagerCache(DFT_SWITCH_EXCEP, params.data(), params.size());
    DftManagerEraseCache(DFT_SWITCH_EXCEP, params.data(), params.size());

    DftManagerStart();
    DftManagerCache(DFT_SWITCH_EXCEP, params.data(), params.size());
    DftManagerReport(DFT_SWITCH_EXCEP, params.data(), params.size());
    sleep(2);
    DftManagerStop();
}

/**
 * @tc.name: NearlinkDftManagerTest018
 * @tc.desc: 校验地址格式矩阵的剩余边：大写十六进制合法，十进制位与冒号位各自独立判错
 * @tc.type: FUNC
 */
HWTEST(NearlinkDftManagerTest, AddressMatrixMoreTest_001, TestSize.Level1)
{
    ASSERT_TRUE(IsValidAddressWithType("9A:Bc:De:F0:12:34_01"));
    ASSERT_FALSE(IsValidAddressWithType("9A:Bc:De:F0:12;34_01"));
    ASSERT_FALSE(IsValidAddressWithType("9A:Bc:De:F0:12g34_01"));
    ASSERT_FALSE(IsValidAddressWithSuffix("9A:Bc:De:F0:12;34"));
    ASSERT_FALSE(IsValidAddressWithSuffix("9A:Bc:De:F0:12g34"));
}

/**
 * @tc.name: NearlinkDftManagerTest016
 * @tc.desc: 校验子事件引用在真链路上报时能拼出缓存JSON：状态流与带掩码地址的邻设备信息均不崩溃
 * @tc.type: FUNC
 */
HWTEST(NearlinkDftManagerTest, SubEventJsonSerialTest_001, TestSize.Level1)
{
    DftManagerStart();
    // 状态流未缓存时引用先走未命中路径：拼出空串不崩溃
    std::string appMiss = "com.example.miss";
    DftSubEventRefC missRef = {DFT_FLOW_STATE, nullptr, 0};
    std::vector<DftParamC> missParams;
    missParams.emplace_back(CreateStrParamC(SWITCH_USED_APP, appMiss));
    missParams.emplace_back(CreateRefParamC(SUB_STATE_FLOW, missRef));
    DftManagerReport(DFT_SWITCH_EXCEP, missParams.data(), missParams.size());

    DftCacheStateFlow(1);
    DftCacheStateFlow(2);
    std::string app = "com.example.app";
    DftSubEventRefC flowRef = {DFT_FLOW_STATE, nullptr, 0};
    std::vector<DftParamC> params;
    params.emplace_back(CreateStrParamC(SWITCH_USED_APP, app));
    params.emplace_back(CreateRefParamC(SUB_STATE_FLOW, flowRef));
    DftManagerReport(DFT_SWITCH_EXCEP, params.data(), params.size());

    // 邻设备信息缓存后经引用上报：普通地址/带类型地址/带尾缀地址三种掩码路径
    DftCachePeerName("11:22:33:44:55:66", "peer");
    DftCachePeerName("11:22:33:44:55:66_01", "peer");
    DftCachePeerName("11:22:33:44:55:66_extra", "peer");
    for (const auto &addr : {"11:22:33:44:55:66", "11:22:33:44:55:66_01", "11:22:33:44:55:66_extra"}) {
        std::vector<DftParamC> peerKeys;
        peerKeys.emplace_back(CreateStrParamC(PEER_INFO_ADDR, addr));
        peerKeys.emplace_back(CreateUi16ParamC(PEER_INFO_TYPE, 0));
        DftSubEventRefC peerRef = {DFT_PEER_INFO, peerKeys.data(), peerKeys.size()};
        std::vector<DftParamC> disconn;
        disconn.emplace_back(CreateStrParamC(DISCONN_PEER_ADDR, addr));
        disconn.emplace_back(CreateUi16ParamC(DISCONN_PEER_TYPE, 0));
        disconn.emplace_back(CreateI32ParamC(DISCONN_ERRORCODE, 1));
        disconn.emplace_back(CreateRefParamC(SUB_PEER_INFO, peerRef));
        DftManagerReport(DFT_DISCONN_EXCEP, disconn.data(), disconn.size());
    }
    sleep(2);
    DftManagerStop();
}
