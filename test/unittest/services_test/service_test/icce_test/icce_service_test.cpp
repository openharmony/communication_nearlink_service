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
#include "nearlink_access_token_mock.h"
#include "IcceDefines.h"
#include "IcceService.h"
#include "SleInterfaceManager.h"
#include "SleServiceManager.h"
#include "icce_utils.h"

#include "log.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
// 测试目的：mock 栈适配层计数器接口（实现见 mock_icce_stack_adapters.cpp），用于断言服务确实触达适配层。
void ResetIcceStackMock();
int GetIcceStackConnectCalls();
int GetIcceStackDisconnectCalls();
int GetIcceRegisterCallBackCalls();
int GetIcceServerCreateCalls();
int GetIcceServerDestroyCalls();
using namespace testing::ext;

class IcceClientCallbackCommon : public IcceObserver {
public:
    IcceClientCallbackCommon() = default;
    virtual ~IcceClientCallbackCommon() = default;

    void OnConnectionStateChanged(const RawAddress &device, int curState, int prevState){}
};

namespace {
IcceClientCallbackCommon g_icceClientCallback_;
}

class NearlinkIcceServiceTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();

private:
    // 测试目的：用例自持真实 IcceService 实例（不依赖 vendor 栈启动链），随用例生命周期创建/销毁。
    std::unique_ptr<IcceService> service_;
};

void NearlinkIcceServiceTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase NearlinkIcceServiceTest start");
    // ICCE service tests own their service; vendor stack startup is an integration dependency.
    HILOGI("SetUpTestCase NearlinkIcceServiceTest end");
}

void NearlinkIcceServiceTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase NearlinkIcceServiceTest");
}

void NearlinkIcceServiceTest::SetUp()
{
    HILOGI("SetUp NearlinkIcceServiceTest.");
    // 测试目的：ICCE 任务切到当前线程同步执行（去等待）；自建服务并直接写入注册表绕过 Start 链
    //（裸进程 slem_initialize 必败），最后用致命断言锁定"GetService 命中本实例"前提。
    ThreadUtil::GetInstance().threadStateMap_.EnsureInsert(
        THREAD_ID_ICCE, ThreadUtil::ThreadState::NOT_SWITCH_THREAD);
    ResetIcceStackMock();
    service_ = std::make_unique<IcceService>();
    ProfileServiceManager::GetInstance().pimpl->startedProfiles_.SetProfile(
        SleTransport::ADAPTER_SLE, PROFILE_NAME_ICCE, service_.get());
    ASSERT_EQ(service_.get(), IcceService::GetService());
}

void NearlinkIcceServiceTest::TearDown()
{
    HILOGI("TearDown NearlinkIcceServiceTest.");
    // 测试目的：先解绑注册表再销毁服务，避免 GetService 命中悬垂指针；最后恢复线程调度默认态。
    ProfileServiceManager::GetInstance().pimpl->startedProfiles_.SetProfile(
        SleTransport::ADAPTER_SLE, PROFILE_NAME_ICCE, nullptr);
    service_.reset();
    ThreadUtil::GetInstance().InitThreadStateMap();
}

/**
 * @tc.number: GetService001
 * @tc.name:
 * @tc.desc:
 */
// 测试目的：fixture 注册表注入生效——GetService 必须返回非空服务实例（原 EXPECT 升级为致命断言）。
HWTEST_F(NearlinkIcceServiceTest, GetService001, TestSize.Level1)
{
    HILOGI("GetService001 start");
    IcceService* icceService = IcceService::GetService();
    ASSERT_NE(nullptr, icceService);
    HILOGI("GetService001 end");
}

/**
 * @tc.number: Disable001
 * @tc.name:
 * @tc.desc:
 */
// 测试目的：已使能服务 Disable 触发服务端资源销毁一次；重复 Disable 走"已关闭"分支不再触达栈（计数仍为 1）。
HWTEST_F(NearlinkIcceServiceTest, Disable001, TestSize.Level1)
{
    HILOGI("Disable001 start");
    service_->Enable();
    IcceService* icceService = IcceService::GetService();
    ASSERT_NE(nullptr, icceService);
    icceService->Disable();
    icceService->Disable();
    EXPECT_EQ(1, GetIcceServerDestroyCalls());
    HILOGI("Disable001 end");
}

/**
 * @tc.number: Enable001
 * @tc.name:
 * @tc.desc:
 */
// 测试目的：首次 Enable 触发回调注册与服务端创建各一次；重复 Enable 走"已使能"分支不重复建栈（计数仍为 1）。
HWTEST_F(NearlinkIcceServiceTest, Enable001, TestSize.Level1)
{
    HILOGI("Enable001 start");
    IcceService* icceService = IcceService::GetService();
    ASSERT_NE(nullptr, icceService);
    icceService->Enable();
    icceService->Enable();
    EXPECT_EQ(1, GetIcceRegisterCallBackCalls());
    EXPECT_EQ(1, GetIcceServerCreateCalls());
    HILOGI("Enable001 end");
}

/*
 * @tc.number: RegisterObserver001
 * @tc.name:
 * @tc.desc:
*/
// 测试目的：前提断言前置——服务指针先判空再注册观察者，注册不崩溃即通过。
HWTEST_F(NearlinkIcceServiceTest, RegisterObserver001, TestSize.Level1)
{
    HILOGI("RegisterObserver001 start");
    IcceService* icceService = IcceService::GetService();
    ASSERT_NE(nullptr, icceService);
    icceService->RegisterObserver(g_icceClientCallback_);
    HILOGI("RegisterObserver001 end");
}

/*
 * @tc.number: DeregisterObserver001
 * @tc.name:
 * @tc.desc:
*/
// 测试目的：前提断言前置——服务指针先判空再解注册观察者，操作不崩溃即通过。
HWTEST_F(NearlinkIcceServiceTest, DeregisterObserver001, TestSize.Level1)
{
    HILOGI("DeregisterObserver001 start");
    IcceService* icceService = IcceService::GetService();
    ASSERT_NE(nullptr, icceService);
    icceService->DeregisterObserver(g_icceClientCallback_);
    HILOGI("DeregisterObserver001 end");
}

/**
 * @tc.number: Connect001
 * @tc.name:
 * @tc.desc:
 */
// 测试目的：使能前提下 Connect 返回成功且同步触达客户端适配层一次（NOT_SWITCH_THREAD 下任务同步执行，
// 原 sleep(50ms) 等待由适配层计数断言替代）。
HWTEST_F(NearlinkIcceServiceTest, Connect001, TestSize.Level1)
{
    HILOGI("Connect001 start");
    service_->Enable();
    IcceService* icceService = IcceService::GetService();
    ASSERT_NE(nullptr, icceService);
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);
    int ret = icceService->Connect(*device);
    EXPECT_EQ(ICCE_SUCCESS, ret);
    EXPECT_EQ(1, GetIcceStackConnectCalls());
    HILOGI("Connect001 end");
}

/**
 * @tc.number: Disconnect001
 * @tc.name:
 * @tc.desc:
 */
// 测试目的：Disconnect 返回成功且同步触达客户端适配层一次，计数断言替代原固定 sleep 等待。
HWTEST_F(NearlinkIcceServiceTest, Disconnect001, TestSize.Level1)
{
    HILOGI("Disconnect001 start");
    IcceService* icceService = IcceService::GetService();
    ASSERT_NE(nullptr, icceService);
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);
    int ret = icceService->Disconnect(*device);
    EXPECT_EQ(ICCE_SUCCESS, ret);
    EXPECT_EQ(1, GetIcceStackDisconnectCalls());
    HILOGI("Disconnect001 end");
}

/*
 * @tc.number: GetPort001
 * @tc.name:
 * @tc.desc:
*/
// 测试目的：前提断言前置——服务指针先判空再查端口；mock 栈返回固定端口 1（原用例仅判服务非空，
// 现增加对端口透传值的断言）。
HWTEST_F(NearlinkIcceServiceTest, GetPort001, TestSize.Level1)
{
    HILOGI("GetPort001 start");
    IcceService* icceService = IcceService::GetService();
    ASSERT_NE(nullptr, icceService);
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);
    int32_t ret = icceService->GetPort(*device);
    EXPECT_EQ(1, ret);
    HILOGI("GetPort001 end");
}

/*
 * @tc.number: GetConnectionsDeviceNum001
 * @tc.name:
 * @tc.desc:
*/
// 测试目的：前提断言前置——服务指针先判空再查连接设备数；mock 栈无连接返回 0。
HWTEST_F(NearlinkIcceServiceTest, GetConnectionsDeviceNum001, TestSize.Level1)
{
    HILOGI("GetConnectionsDeviceNum001 start");
    IcceService* icceService = IcceService::GetService();
    ASSERT_NE(nullptr, icceService);
    uint8_t ret = icceService->GetConnectionsDeviceNum();
    EXPECT_EQ(0, ret);
    HILOGI("GetConnectionsDeviceNum001 end");
}

/*
 * @tc.number: ConvertToStackAddr_001
 * @tc.name: ConvertToStackAddr with valid address
 * @tc.desc: Verify ConvertToStackAddr uses IRemoteDeviceQuery to get addr type and returns correct SLE_Addr_S
 */
HWTEST_F(NearlinkIcceServiceTest, ConvertToStackAddr_001, TestSize.Level1)
{
    HILOGI("ConvertToStackAddr_001 start");
    std::string addr = "00:11:22:33:44:55";
    RawAddress device(addr);
    SLE_Addr_S sleAddr = ConvertToStackAddr(device);
    EXPECT_EQ(0, sleAddr.type);
    EXPECT_EQ(0x00, sleAddr.addr[0]);
    EXPECT_EQ(0x11, sleAddr.addr[1]);
    EXPECT_EQ(0x22, sleAddr.addr[2]);
    EXPECT_EQ(0x33, sleAddr.addr[3]);
    EXPECT_EQ(0x44, sleAddr.addr[4]);
    EXPECT_EQ(0x55, sleAddr.addr[5]);
    HILOGI("ConvertToStackAddr_001 end");
}

/*
 * @tc.number: ConvertSleAddrToRawAddress_001
 * @tc.name: ConvertSleAddrToRawAddress with valid SLE_Addr_S
 * @tc.desc: Verify ConvertSleAddrToRawAddress returns correct RawAddress
 */
HWTEST_F(NearlinkIcceServiceTest, ConvertSleAddrToRawAddress_001, TestSize.Level1)
{
    HILOGI("ConvertSleAddrToRawAddress_001 start");
    SLE_Addr_S sleAddr = {0};
    sleAddr.addr[0] = 0xAA;
    sleAddr.addr[1] = 0xBB;
    sleAddr.addr[2] = 0xCC;
    sleAddr.addr[3] = 0xDD;
    sleAddr.addr[4] = 0xEE;
    sleAddr.addr[5] = 0xFF;
    RawAddress rawAddr = ConvertSleAddrToRawAddress(&sleAddr);
    EXPECT_EQ(std::string("AA:BB:CC:DD:EE:FF"), rawAddr.GetAddress());
    HILOGI("ConvertSleAddrToRawAddress_001 end");
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS
