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
#include "nearlink_access_token_mock.h"
#include "DisClientStackAdapter.h"
#include "DisService.h"
#include "DisServerStackAdapter.h"
#include "SleInterfaceManager.h"
#include "SleServiceManager.h"
#include "ThreadUtil.h"
#include "log.h"
#include "ProfileServiceManager.cpp"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

namespace {
const std::string DIS_UT_ADDR = "00:11:22:33:44:55";
const std::string DIS_UT_MANUFACTURE_DATA = "A-1122-3344-5566";
constexpr uint16_t DIS_UT_MANUFACTURE_LEN = 16;
constexpr uint16_t DIS_UT_EXPECT_VENDOR_ID = 0x3344;
constexpr uint16_t DIS_UT_EXPECT_PRODUCT_ID = 0x1122;
constexpr uint16_t DIS_UT_EXPECT_VERSION = 0x5566;
constexpr uint16_t DIS_UT_MANUFACTURE_MIN_LEN = 15;
constexpr uint16_t DIS_UT_INVALID_ID = 0;
} // namespace

class DisServiceStateObserver : public DisObserver {
public:
    int stateChangeCount = 0;
    int lastState = -1;
    int lastOldState = -1;
    void OnConnectionStateChanged(const RawAddress &deviceAddress, int state, int oldState) override
    {
        stateChangeCount++;
        lastState = state;
        lastOldState = oldState;
    }
};

class NearlinkDisServiceTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();

private:
    // 测试目的：fixture 持有自有 DisService 实例（cdsm_test 样板），
    // 不依赖厂商栈启动链（裸进程协议栈初始化必败，Start 后服务实例为空）。
    std::unique_ptr<DisService> service_;
    DisServiceStateObserver observer_;
};

void NearlinkDisServiceTest::SetUpTestCase()
{
    // 测试目的：删除 SleInterfaceManager::Start()（裸进程必败且返回值被忽略）与固定 sleep；
    // 服务实例改由每例 SetUp 自建并注入注册表。
    NearlinkAccessTokenMock::SetNativeTokenInfo();
    HILOGI("SetUpTestCase NearlinkDisServiceTest start");
}

void NearlinkDisServiceTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase NearlinkDisServiceTest");
}

void NearlinkDisServiceTest::SetUp()
{
    HILOGI("SetUp NearlinkDisServiceTest.");
    // 测试目的：先让 DIS 任务在测试线程同步执行（漏插键会让异步用例假绿、同步等待用例挂死），
    // 再自建服务并注入注册表，使 GetDisService() 命中。
    ThreadUtil::GetInstance().threadStateMap_.EnsureInsert(
        THREAD_ID_DIS, ThreadUtil::ThreadState::NOT_SWITCH_THREAD);
    service_ = std::make_unique<DisService>();
    ProfileServiceManager::GetInstance().pimpl->startedProfiles_.SetProfile(
        SleTransport::ADAPTER_SLE, PROFILE_NAME_DIS, service_.get());
    ASSERT_EQ(service_.get(), DisService::GetDisService());
    DisService::GetDisService()->RegisterObserver(observer_);
}

void NearlinkDisServiceTest::TearDown()
{
    HILOGI("TearDown NearlinkDisServiceTest.");
    // 测试目的：先解绑注册表与观察者再销毁服务，并恢复线程调度默认态，防单例状态跨例残留；
    // SetUp 断言失败时服务可能未注册成功，先判空防解引用空指针。
    DisService *disService = DisService::GetDisService();
    if (disService != nullptr) {
        disService->DeregisterObserver(observer_);
    }
    ProfileServiceManager::GetInstance().pimpl->startedProfiles_.SetProfile(
        SleTransport::ADAPTER_SLE, PROFILE_NAME_DIS, nullptr);
    service_.reset();
    ThreadUtil::GetInstance().InitThreadStateMap();
}

/**
 * @tc.number: GetDisService001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkDisServiceTest, GetDisService001, TestSize.Level1)
{
    HILOGI("GetDisService001 start");
    DisService* disService = DisService::GetDisService();
    EXPECT_NE(nullptr, disService);
    HILOGI("GetDisService001 end");
}

/**
 * @tc.number: Disable001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkDisServiceTest, Disable001, TestSize.Level1)
{
    HILOGI("Disable001 start");
    DisService* disService = DisService::GetDisService();
    disService->Disable();
    EXPECT_NE(nullptr, disService);

    disService->Disable();
    EXPECT_NE(nullptr, disService);
    HILOGI("Disable001 end");
}

/**
 * @tc.number: Enable001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkDisServiceTest, Enable001, TestSize.Level1)
{
    HILOGI("Enable001 start");
    DisService* disService = DisService::GetDisService();
    disService->Enable();
    EXPECT_NE(nullptr, disService);

    disService->Enable();
    EXPECT_NE(nullptr, disService);
    HILOGI("Enable001 end");
}

/**
 * @tc.number: Connect001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkDisServiceTest, Connect001, TestSize.Level1)
{
    HILOGI("Connect001 start");
    DisService* disService = DisService::GetDisService();
    std::string addr = DIS_UT_ADDR;
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);
    int ret = disService->Connect(*device);
    EXPECT_EQ(DIS_SUCCESS, ret);
    HILOGI("Connect001 end");
}

/**
 * @tc.number: Disconnect001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkDisServiceTest, Disconnect001, TestSize.Level1)
{
    HILOGI("Disconnect001 start");
    DisService* disService = DisService::GetDisService();
    std::string addr = DIS_UT_ADDR;
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);
    int ret = disService->Disconnect(*device);
    EXPECT_EQ(DIS_SUCCESS, ret);
    HILOGI("Disconnect001 end");
}

/*
 * @tc.number: NotifyStateChanged001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(NearlinkDisServiceTest, NotifyStateChanged001, TestSize.Level1)
{
    HILOGI("GetPortNotifyStateChanged001001 start");
    DisService* disService = DisService::GetDisService();
    std::string addr = DIS_UT_ADDR;
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);
    disService->NotifyStateChanged(*device, SleConnectState::CONNECTED, SleConnectState::DISCONNECTED);
    EXPECT_NE(nullptr, disService);
    HILOGI("GetPorNotifyStateChanged001t001 end");
}

/*
 * @tc.number: GetConnectDevices001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(NearlinkDisServiceTest, GetConnectDevices001, TestSize.Level1)
{
    HILOGI("GetConnectDevices001 start");
    DisService* disService = DisService::GetDisService();
    std::list<RawAddress> devices = disService->GetConnectDevices();
    EXPECT_EQ(0, devices.size());
    HILOGI("GetConnectDevices001 end");
}

/*
 * @tc.number: GetConnectState001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(NearlinkDisServiceTest, GetConnectState001, TestSize.Level1)
{
    HILOGI("GetConnectState001 start");
    DisService* disService = DisService::GetDisService();
    int ret = disService->GetConnectState();
    EXPECT_EQ(static_cast<int>(SleConnectState::DISCONNECTED), ret);
    HILOGI("GetConnectState001 end");
}

/*
 * @tc.number: GetDeviceVendorId001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(NearlinkDisServiceTest, GetDeviceVendorId001, TestSize.Level1)
{
    HILOGI("GetDeviceVendorId001 start");
    DisService* disService = DisService::GetDisService();
    std::string addr = DIS_UT_ADDR;
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);
    int ret = disService->GetDeviceVendorId(*device);
    EXPECT_NE(nullptr, disService);
    HILOGI("GetDeviceVendorId001 end");
}

/*
 * @tc.number: GetDeviceVersion001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(NearlinkDisServiceTest, GetDeviceVersion001, TestSize.Level1)
{
    HILOGI("GetDeviceVersion001 start");
    DisService* disService = DisService::GetDisService();
    std::string addr = DIS_UT_ADDR;
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);
    int ret = disService->GetDeviceVersion(*device);
    EXPECT_NE(nullptr, disService);
    HILOGI("GetDeviceVersion001 end");
}

/*
 * @tc.number: GetDisServiceData001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(NearlinkDisServiceTest, GetDisServiceData001, TestSize.Level1)
{
    HILOGI("GetDisServiceData001 start");
    DisService* disService = DisService::GetDisService();
    std::string serviceData;
    disService->GetDisServiceData(serviceData);
    EXPECT_NE(nullptr, disService);
    HILOGI("GetDisServiceData001 end");
}

/*
 * @tc.number: GetDisServiceAppearance001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(NearlinkDisServiceTest, GetDisServiceAppearance001, TestSize.Level1)
{
    HILOGI("GetDisServiceAppearance001 start");
    DisService* disService = DisService::GetDisService();
    int ret = disService->GetDisServiceAppearance();
    EXPECT_NE(nullptr, disService);
    HILOGI("GetDisServiceAppearance001 end");
}

/**
 * @tc.number: EnableDisable002
 * @tc.name: 设备信息服务开关幂等
 * @tc.desc: 已使能状态下重复使能、未使能状态下重复去使能均按已处理返回.
 */
HWTEST_F(NearlinkDisServiceTest, EnableDisable002, TestSize.Level1)
{
    HILOGI("EnableDisable002 start");
    DisService* disService = DisService::GetDisService();
    ASSERT_NE(nullptr, disService);

    disService->Enable();
    disService->Enable();
    disService->Disable();
    disService->Disable();
    disService->Enable();
    HILOGI("EnableDisable002 end");
}

/**
 * @tc.number: NotifyStateChanged002
 * @tc.name: 连接建立后保存设备外观与名称并通知观察者
 * @tc.desc: 连接中到已连接的状态迁移触发信息保存链与观察者回调.
 */
HWTEST_F(NearlinkDisServiceTest, NotifyStateChanged002, TestSize.Level1)
{
    HILOGI("NotifyStateChanged002 start");
    DisService* disService = DisService::GetDisService();
    ASSERT_NE(nullptr, disService);
    RawAddress device(DIS_UT_ADDR);
    int countBefore = observer_.stateChangeCount;

    disService->NotifyStateChanged(device, SleConnectState::CONNECTED, SleConnectState::CONNECTING);
    EXPECT_EQ(countBefore + 1, observer_.stateChangeCount);
    EXPECT_EQ(static_cast<int>(SleConnectState::CONNECTED), observer_.lastState);
    EXPECT_EQ(static_cast<int>(SleConnectState::CONNECTING), observer_.lastOldState);
    HILOGI("NotifyStateChanged002 end");
}

/**
 * @tc.number: NotifyStateChanged003
 * @tc.name: 非连接建立的状态迁移只通知观察者
 * @tc.desc: 已断开到已断开迁移不触发信息保存链，仅做观察者分发.
 */
HWTEST_F(NearlinkDisServiceTest, NotifyStateChanged003, TestSize.Level1)
{
    HILOGI("NotifyStateChanged003 start");
    DisService* disService = DisService::GetDisService();
    ASSERT_NE(nullptr, disService);
    RawAddress device(DIS_UT_ADDR);
    int countBefore = observer_.stateChangeCount;

    disService->NotifyStateChanged(device, SleConnectState::DISCONNECTED, SleConnectState::DISCONNECTED);
    EXPECT_EQ(countBefore + 1, observer_.stateChangeCount);
    HILOGI("NotifyStateChanged003 end");
}

/**
 * @tc.number: GetDeviceInformation001
 * @tc.name: 查询远端设备信息在栈读取失败时返回默认信息
 * @tc.desc: 合作设备服务不在席时直接用请求地址查询，读取失败得到默认设备信息.
 */
HWTEST_F(NearlinkDisServiceTest, GetDeviceInformation001, TestSize.Level1)
{
    HILOGI("GetDeviceInformation001 start");
    DisService* disService = DisService::GetDisService();
    ASSERT_NE(nullptr, disService);
    RawAddress device(DIS_UT_ADDR);

    DeviceInformation info = disService->GetDeviceInformation(device);
    EXPECT_EQ(true, info.GetModelData().empty());
    HILOGI("GetDeviceInformation001 end");
}

/**
 * @tc.number: GetDeviceVendorId002
 * @tc.name: 栈读取失败时厂商标识返回无效值
 * @tc.desc: 协议栈读取远端信息失败时厂商标识按无效值返回.
 */
HWTEST_F(NearlinkDisServiceTest, GetDeviceVendorId002, TestSize.Level1)
{
    HILOGI("GetDeviceVendorId002 start");
    DisService* disService = DisService::GetDisService();
    ASSERT_NE(nullptr, disService);
    RawAddress device(DIS_UT_ADDR);

    uint16_t vendorId = static_cast<uint16_t>(disService->GetDeviceVendorId(device));
    EXPECT_EQ(DIS_UT_INVALID_ID, vendorId);
    HILOGI("GetDeviceVendorId002 end");
}

/**
 * @tc.number: GetDeviceProductId002
 * @tc.name: 栈读取失败时产品标识返回无效值
 * @tc.desc: 协议栈读取远端信息失败时产品标识按无效值返回.
 */
HWTEST_F(NearlinkDisServiceTest, GetDeviceProductId002, TestSize.Level1)
{
    HILOGI("GetDeviceProductId002 start");
    DisService* disService = DisService::GetDisService();
    ASSERT_NE(nullptr, disService);
    RawAddress device(DIS_UT_ADDR);

    uint16_t productId = static_cast<uint16_t>(disService->GetDeviceProductId(device));
    EXPECT_EQ(DIS_UT_INVALID_ID, productId);
    HILOGI("GetDeviceProductId002 end");
}

/**
 * @tc.number: GetDeviceVersion002
 * @tc.name: 栈读取失败时版本号返回无效值
 * @tc.desc: 协议栈读取远端信息失败时版本号按无效值返回.
 */
HWTEST_F(NearlinkDisServiceTest, GetDeviceVersion002, TestSize.Level1)
{
    HILOGI("GetDeviceVersion002 start");
    DisService* disService = DisService::GetDisService();
    ASSERT_NE(nullptr, disService);
    RawAddress device(DIS_UT_ADDR);

    uint16_t version = static_cast<uint16_t>(disService->GetDeviceVersion(device));
    EXPECT_EQ(DIS_UT_INVALID_ID, version);
    HILOGI("GetDeviceVersion002 end");
}

/**
 * @tc.number: DisAdapterOnStateChanged001
 * @tc.name: 栈侧连接中断回调上报配对信息并通知服务
 * @tc.desc: 连接中到已断开的迁移触发配对失败上报与服务状态分发.
 */
HWTEST_F(NearlinkDisServiceTest, DisAdapterOnStateChanged001, TestSize.Level1)
{
    HILOGI("DisAdapterOnStateChanged001 start");
    SLE_Addr_S addr = {};
    RawAddress device(DIS_UT_ADDR);
    device.ConvertToUint8(addr.addr);
    int countBefore = observer_.stateChangeCount;

    DisClientStackAdapter::OnConnectionStateChanged(&addr, DIS_DISCONNECTED, DIS_CONNECTING,
        NLSTK_ERRCODE_FAIL);
    EXPECT_EQ(countBefore + 1, observer_.stateChangeCount);
    HILOGI("DisAdapterOnStateChanged001 end");
}

/**
 * @tc.number: DisAdapterOnStateChanged002
 * @tc.name: 栈侧回调对空地址与非断链迁移的防御
 * @tc.desc: 空地址直接返回，普通状态迁移仅做观察者分发不上报配对失败.
 */
HWTEST_F(NearlinkDisServiceTest, DisAdapterOnStateChanged002, TestSize.Level1)
{
    HILOGI("DisAdapterOnStateChanged002 start");
    SLE_Addr_S addr = {};
    RawAddress device(DIS_UT_ADDR);
    device.ConvertToUint8(addr.addr);
    int countBefore = observer_.stateChangeCount;

    DisClientStackAdapter::OnConnectionStateChanged(nullptr, DIS_DISCONNECTED, DIS_CONNECTING,
        NLSTK_ERRCODE_FAIL);
    EXPECT_EQ(countBefore, observer_.stateChangeCount);

    DisClientStackAdapter::OnConnectionStateChanged(&addr, DIS_CONNECTING, DIS_DISCONNECTED,
        NLSTK_ERRCODE_FAIL);
    EXPECT_EQ(countBefore + 1, observer_.stateChangeCount);
    HILOGI("DisAdapterOnStateChanged002 end");
}

/**
 * @tc.number: DisAdapterParseManufacturer001
 * @tc.name: 厂商信息解析的防御与三种字段提取
 * @tc.desc: 空指针、超短数据、非法格式返回无效值，合法数据按位解析厂商、产品与版本.
 */
HWTEST_F(NearlinkDisServiceTest, DisAdapterParseManufacturer001, TestSize.Level1)
{
    HILOGI("DisAdapterParseManufacturer001 start");
    DisClientStackAdapter adapter;

    uint8_t validData[DIS_UT_MANUFACTURE_LEN] = {};
    for (size_t i = 0; i < DIS_UT_MANUFACTURE_DATA.size(); i++) {
        validData[i] = static_cast<uint8_t>(DIS_UT_MANUFACTURE_DATA[i]);
    }

    EXPECT_EQ(DIS_UT_INVALID_ID,
        adapter.ParseManufacturerInfo(nullptr, DIS_UT_MANUFACTURE_LEN,
            ManufacturerInfoType::MANUFACTURER_INFO_VENDOR_ID));
    EXPECT_EQ(DIS_UT_INVALID_ID,
        adapter.ParseManufacturerInfo(validData, DIS_UT_MANUFACTURE_MIN_LEN - 1,
            ManufacturerInfoType::MANUFACTURER_INFO_VENDOR_ID));
    EXPECT_EQ(DIS_UT_EXPECT_VENDOR_ID,
        adapter.ParseManufacturerInfo(validData, DIS_UT_MANUFACTURE_LEN,
            ManufacturerInfoType::MANUFACTURER_INFO_VENDOR_ID));
    EXPECT_EQ(DIS_UT_EXPECT_PRODUCT_ID,
        adapter.ParseManufacturerInfo(validData, DIS_UT_MANUFACTURE_LEN,
            ManufacturerInfoType::MANUFACTURER_INFO_PRODUCT_ID));
    EXPECT_EQ(DIS_UT_EXPECT_VERSION,
        adapter.ParseManufacturerInfo(validData, DIS_UT_MANUFACTURE_LEN,
            ManufacturerInfoType::MANUFACTURER_INFO_VERSION));
    HILOGI("DisAdapterParseManufacturer001 end");
}

/**
 * @tc.number: DisServerAdapterCreateServerInfo001
 * @tc.name: 服务端设备信息组装与资源释放
 * @tc.desc: 带完整信息与空信息各组装一次，两种情况下资源释放路径均不崩溃.
 */
HWTEST_F(NearlinkDisServiceTest, DisServerAdapterCreateServerInfo001, TestSize.Level1)
{
    HILOGI("DisServerAdapterCreateServerInfo001 start");
    DisServerStackAdapter adapter;

    DisServerInfo fullInfo;
    fullInfo.versionInfo_ = "1.0.1";
    fullInfo.nameInfo_ = "test-device";
    fullInfo.manufactureInfo_ = "0";
    fullInfo.appearanceId_ = DIS_DEVICE_SMARTPHONE;
    adapter.CreateServerInfo(fullInfo);
    adapter.DestroyServerInfo();

    DisServerInfo emptyInfo;
    adapter.CreateServerInfo(emptyInfo);
    adapter.DestroyServerInfo();
    HILOGI("DisServerAdapterCreateServerInfo001 end");
}

/**
 * @tc.number: DisServerAdapterUpdateLocalDeviceName001
 * @tc.name: 本端名称更新在空名称时被拒绝
 * @tc.desc: 空名称不触发栈接口，合法名称走栈更新失败路径仅记录日志.
 */
HWTEST_F(NearlinkDisServiceTest, DisServerAdapterUpdateLocalDeviceName001, TestSize.Level1)
{
    HILOGI("DisServerAdapterUpdateLocalDeviceName001 start");
    DisServerStackAdapter adapter;

    adapter.UpdateLocalDeviceName("");
    adapter.UpdateLocalDeviceName("new-name");
    HILOGI("DisServerAdapterUpdateLocalDeviceName001 end");
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS
