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
 * 长尾小域收尾测试：测距服务启停幂等与事件默认分支、测距服务端注册的
 * 特性门与已注册早退、端口服务停用门与端口适配层状态回调、对话断连的
 * 未命中路径、设备名 UTF-8 截断矩阵、DLI 回调注入与线程工具生命周期、
 * DLI 收发错误路径与 ICCE 适配层状态回调分支。
 */

#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <cstring>
#include <thread>

#include "DialogGenerate.h"
#include "IcceClientStackAdapter.h"
#include "LisServer.h"
#include "LisService.h"
#include "PortClientStackAdapter.h"
#include "PortService.h"
#include "SleDliCallbacks.h"
#include "SleDliLayerAdapter.h"
#include "SleDliThreadUtil.h"
#include "SleInterfaceManager.h"
#include "message.h"
#include "nearlink_access_token_mock.h"
#include "nearlink_datashare_helper.h"
#include "nearlink_def_types.h"
#include "securec.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS::Nearlink;

namespace {
constexpr int DELAY_1000_MS = 1000;
constexpr int DELAY_300_MS = 300;
constexpr const char *SMALL_ADDR = "11:22:33:44:55:66";

std::atomic<int> g_dliInitCalls(0);
std::atomic<int> g_dliPacketCalls(0);
std::atomic<int> g_dliTaskRan(0);
} // namespace

class SmallDomainTailTest : public testing::Test {
public:
    SmallDomainTailTest() {}
    ~SmallDomainTailTest() {}

    static void SetUpTestCase(void)
    {
        NearlinkAccessTokenMock::SetNativeTokenInfo();
        SleInterfaceManager::GetInstance()->Start();
        std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_1000_MS));
    }

    static void TearDownTestCase(void) {}

    void SetUp(void)
    {
        g_dliInitCalls = 0;
        g_dliPacketCalls = 0;
        g_dliTaskRan = 0;
    }

    void TearDown(void) {}
};

/* 测距服务：重复启停幂等，未知事件安全落空 */
HWTEST_F(SmallDomainTailTest, LisServiceLifecycleTest_001, TestSize.Level1)
{
    LisService service;
    service.Enable();
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_300_MS));
    // 已启动后再启用：幂等早退
    service.Enable();
    service.Disable();
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_300_MS));
    // 已停用后再停用：幂等早退
    service.Disable();
    // 未知事件号：事件处理默认分支安全落空
    utility::Message unknown(999);
    service.ProcessEvent(unknown);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_300_MS));
}

/* 测距服务端注册门：特性关闭直接早退，已登记应用号直接早退，
 *   未登记应用号在无服务管理器时安全返回 */
HWTEST_F(SmallDomainTailTest, LisServerRegisterGatesTest_001, TestSize.Level1)
{
    LisServer server;
    // 测距特性默认关闭：注册接口早退
    server.RegisterLisServerApplicationInterface();
    // 未登记应用号：去注册早退
    server.DeregisterLisServerApplicationInterface();
    // 已登记应用号：在测距特性已开启的环境下走重复注册早退，默认特性关闭时被前置特性门拦截
    server.appId_ = 0;
    server.RegisterLisServerApplicationInterface();
    EXPECT_EQ(server.appId_, 0);
}

/* 端口服务：未启用先停用走幂等早退，启用后停用走完整停用路径；
 *   端口适配层状态回调在无注册观察者时安全返回 */
HWTEST_F(SmallDomainTailTest, PortServiceAndAdapterStatesTest_001, TestSize.Level1)
{
    PortService service;
    // 未启用：停用幂等早退
    service.Disable();
    service.Enable();
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_300_MS));
    service.Disable();
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_300_MS));

    // 适配层状态回调：断开自连接中与已连接两种状态输入都安全返回
    SLE_Addr_S addr = {};
    addr.type = 0;
    (void)memset_s(addr.addr, SLE_ADDR_LEN, 0x11, 1);
    PortClientStackAdapter::OnConnectionStateChanged(&addr, PORT_DISCONNECTED, PORT_CONNECTING,
        NLSTK_ERRCODE_SUCCESS);
    PortClientStackAdapter::OnConnectionStateChanged(&addr, PORT_CONNECTED, PORT_CONNECTING,
        NLSTK_ERRCODE_SUCCESS);
    SUCCEED();
}

/* 对话能力断连：未连接过的能力名断连安全无副作用 */
HWTEST_F(SmallDomainTailTest, DialogDisconnectMissTest_001, TestSize.Level1)
{
    DialogGenerate::GetInstance().DialogDisconnectExtensionAbility("absent.ability");
    SUCCEED();
}

/* 设备名 UTF-8 首字节分类：单双三四字节各归其位，非法首字节按单字节处理 */
HWTEST_F(SmallDomainTailTest, DataShareUtf8FirstByteTest_001, TestSize.Level1)
{
    auto &helper = NearlinkDataShareHelper::GetInstance();
    EXPECT_EQ(helper.GetUTF8StringLength('\x01'), 1);
    EXPECT_EQ(helper.GetUTF8StringLength('\x7F'), 1);
    EXPECT_EQ(helper.GetUTF8StringLength('\xC0'), 2);
    EXPECT_EQ(helper.GetUTF8StringLength('\xDF'), 2);
    EXPECT_EQ(helper.GetUTF8StringLength('\xE0'), 3);
    EXPECT_EQ(helper.GetUTF8StringLength('\xEF'), 3);
    EXPECT_EQ(helper.GetUTF8StringLength('\xF0'), 4);
    EXPECT_EQ(helper.GetUTF8StringLength('\xF7'), 4);
    // 0xF8 起不属于四字节头：回落单字节
    EXPECT_EQ(helper.GetUTF8StringLength('\xF8'), 1);
}

/* 设备名有效长度：纯 ASCII 截到上限、多字节序列不拆字符、恰好三十直接返回、
 *   不足三十全取 */
HWTEST_F(SmallDomainTailTest, DataShareValidLengthTest_001, TestSize.Level1)
{
    auto &helper = NearlinkDataShareHelper::GetInstance();
    // 三十一个单字节：取三十
    EXPECT_EQ(helper.GetValidUTF8StringLength(std::string(31, 'a')), 30);
    // 十个三字节序列恰三十字节：直接返回三十
    std::string triplets;
    for (int i = 0; i < 10; i++) {
        triplets += "\xE0\x80\x80";
    }
    EXPECT_EQ(helper.GetValidUTF8StringLength(triplets), 30);
    // 二十九个单字节加一个四字节头：不拆字符取二十九
    EXPECT_EQ(helper.GetValidUTF8StringLength(std::string(29, 'a') + "\xF0\x80\x80\x80"), 29);
    // 短串全取
    EXPECT_EQ(helper.GetValidUTF8StringLength("abc"), 3);
}

/* 设备名截断：三十字节内原样返回，超长纯 ASCII 截到三十，
 *   多字节尾部不拆字符并用点补齐到三十字节 */
HWTEST_F(SmallDomainTailTest, DataShareTruncationTest_001, TestSize.Level1)
{
    auto &helper = NearlinkDataShareHelper::GetInstance();
    std::string shortName = "abc";
    helper.GetTruncationName(shortName);
    EXPECT_EQ(shortName, "abc");

    std::string ascii31(31, 'a');
    helper.GetTruncationName(ascii31);
    EXPECT_EQ(ascii31.size(), 30U);
    EXPECT_EQ(ascii31, std::string(30, 'a'));

    // 二十九个单字节加四字节字符：截断到二十九并补一个点
    std::string mixed = std::string(29, 'a') + "\xF0\x80\x80\x80";
    helper.GetTruncationName(mixed);
    EXPECT_EQ(mixed.size(), 30U);
    EXPECT_EQ(mixed.substr(29), std::string("."));
}

/* DLI 回调注入：空回调安全静默，注入函数指针后初始化完成与数据到达
 *   都送达回调，长数据走截断打印分支 */
HWTEST_F(SmallDomainTailTest, DliCallbacksInjectTest_001, TestSize.Level1)
{
    // 空回调：两条入口都安全静默
    SleDliCallbacks nullCb(nullptr);
    nullCb.initializationComplete(static_cast<SleStatus>(0));
    std::vector<uint8_t> small(4, 0x01);
    nullCb.hciPacketReceived(1, small);

    // 注入回调：初始化完成与数据到达送达，超长数据触发截断打印
    SleDliCallbackFunc funcs = {};
    funcs.initializationComplete = [](SleInitStatus status) { g_dliInitCalls++; };
    funcs.dliPacketReceived = [](SlePacketType type, const SlePacket *packet) {
        g_dliPacketCalls++;
    };
    SleDliCallbacks cb(&funcs);
    cb.initializationComplete(static_cast<SleStatus>(0));
    EXPECT_EQ(g_dliInitCalls.load(), 1);
    cb.hciPacketReceived(1, small);
    EXPECT_EQ(g_dliPacketCalls.load(), 1);
    std::vector<uint8_t> large(60, 0x02);
    cb.hciPacketReceived(1, large);
    EXPECT_EQ(g_dliPacketCalls.load(), 2);
}

/* DLI 线程工具：任务在线程执行，销毁队列后再投递被丢弃；
 *   收包空指针与未初始化收发、关闭都走错误路径安全返回 */
HWTEST_F(SmallDomainTailTest, DliThreadAndAdapterErrorsTest_001, TestSize.Level1)
{
    auto &util = SleDliThreadUtil::GetInstance();
    util.InitThread();
    util.PostTask([]() { g_dliTaskRan++; });
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_300_MS));
    EXPECT_GE(g_dliTaskRan.load(), 1);

    // 销毁队列后再投递：任务被丢弃不再执行
    int before = g_dliTaskRan.load();
    util.DestroyQueue();
    util.PostTask([]() { g_dliTaskRan++; });
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_300_MS));
    EXPECT_EQ(g_dliTaskRan.load(), before);

    // 收包空指针：传输错误返回
    EXPECT_NE(SleSendDliPacket(nullptr), 0);
    // 未初始化关闭：安全返回
    SleHalClose();
}

/* ICCE 适配层状态回调：断开自连接中走建链失败上报分支，
 *   已连接自连接中与其它状态安全透传 */
HWTEST_F(SmallDomainTailTest, IcceAdapterStateChangeTest_001, TestSize.Level1)
{
    SLE_Addr_S addr = {};
    addr.type = 0;
    (void)memset_s(addr.addr, SLE_ADDR_LEN, 0x22, 1);
    IcceClientStackAdapter::OnConnectionStateChange(&addr, ICCE_DISCONNECTED, ICCE_CONNECTING,
        NLSTK_ERRCODE_SUCCESS);
    IcceClientStackAdapter::OnConnectionStateChange(&addr, ICCE_CONNECTED, ICCE_CONNECTING,
        NLSTK_ERRCODE_SUCCESS);
    SUCCEED();
}
