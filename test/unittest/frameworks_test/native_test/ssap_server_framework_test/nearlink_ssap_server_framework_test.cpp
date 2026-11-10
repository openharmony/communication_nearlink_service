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

// 目标: frameworks/native/nearlink_ssap_server.cpp 零覆盖分支(进程内驱动, 不依赖真实 SLE 栈)
// 手法: 编入产品源文件(private=public) 直接取用 impl 层回调桩 serviceCallback_, 用记录型
//       Fake SsapServerCallback 在进程内驱动 On* 回调与 pimpl 内部函数, 瞄准 120/150/299/344/514
//       行分支; 网关后(需真实代理/SLE 使能)的分支不在本套用例范围

#include <gtest/gtest.h>
#include <cstring>
#include <vector>

#include "nearlink_def.h"
#include "nearlink_ssap_server.cpp"
#include "nearlink_ssap_property.h"
#include "nearlink_remote_device.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

namespace {
const std::string TEST_DEVICE_ADDR = "11:22:33:44:55:66";
const std::string SERVICE_UUID_STRING = "11223344-0000-1000-8000-00805F9B34FB";
const std::string PROPERTY_UUID_STRING = "11223344-0000-1000-8000-00805F9B1111";
const std::string OTHER_UUID_STRING = "11223344-0000-1000-8000-00805F9B9999";
constexpr uint16_t SVC_HANDLE = 0x0101;
constexpr uint16_t PROP_HANDLE = 0x0011;
constexpr uint32_t TEST_OP_IND = 0x0F;
constexpr int TEST_PROP_PERM = 17;
constexpr size_t TEST_VALUE_LEN = 3;
const uint8_t TEST_VALUE[TEST_VALUE_LEN] = {1, 2, 3};
// SLE 传输类型(NL_TRANSPORT_SLE = 1), 与设备构造一致
constexpr uint8_t TEST_TRANSPORT = 1;

// 工具: 框架 UUID -> 服务侧 Uuid(与产品 BuildService 的转换链一致)
Uuid ToParcelUuid(const UUID &uuid)
{
    return Uuid::ConvertFrom128Bits(uuid.ConvertTo128Bits());
}
} // namespace

// 记录型 Fake: 实现 SsapServerCallback 全部回调, 记录调用计数与关键参数
class FakeSsapServerCallback : public SsapServerCallback {
public:
    void OnConnectionStateUpdate(const NearlinkRemoteDevice &device, int state, int reason) override
    {
        connUpdateCount_++;
        lastConnAddr_ = device.GetDeviceAddr();
        lastConnState_ = state;
        lastConnReason_ = reason;
    }
    void OnServiceAdded(std::shared_ptr<SsapService> service, int ret) override
    {
        serviceAddedCount_++;
        lastAddedRet_ = ret;
        if (service != nullptr) {
            lastAddedUuid_ = service->GetUuid();
            lastAddedHandle_ = service->GetHandle();
        }
    }
    void OnPropertyReadRequest(const NearlinkRemoteDevice &device, const SsapProperty &property, int ret) override
    {
        propReadReqCount_++;
        lastPropReadServiceUuid_ = property.GetServiceUuid();
        lastPropReadRet_ = ret;
    }
    void OnPropertyWriteRequest(const NearlinkRemoteDevice &device, const SsapProperty &property, int ret) override
    {
        propWriteReqCount_++;
        lastPropWriteServiceUuid_ = property.GetServiceUuid();
        lastPropWriteValueSize_ = 0;
        lastPropWriteValue_ = property.GetValue(&lastPropWriteValueSize_).get();
        lastPropWriteRet_ = ret;
    }
    void OnDescriptorReadRequest(const NearlinkRemoteDevice &device, const SsapDescriptor &descriptor, int ret) override
    {
        descReadReqCount_++;
        lastDescReadHandle_ = descriptor.GetHandle();
        lastDescReadType_ = descriptor.GetDescriptorType();
        lastDescReadRet_ = ret;
    }
    void OnDescriptorWriteRequest(
        const NearlinkRemoteDevice &device, const SsapDescriptor &descriptor, int ret) override
    {
        descWriteReqCount_++;
        lastDescWriteHandle_ = descriptor.GetHandle();
        lastDescWriteRet_ = ret;
    }
    void OnMtuUpdate(const NearlinkRemoteDevice &device, int mtu) override
    {
        mtuCount_++;
        lastMtu_ = mtu;
    }
    void OnNotifyPropertyChanged(
        const NearlinkRemoteDevice &device, const UUID &uuid, uint16_t handle, int result) override
    {
        notifyPropCount_++;
        lastNotifyUuid_ = uuid;
        lastNotifyHandle_ = handle;
        lastNotifyResult_ = result;
    }
    void OnNotifyEventChanged(
        const NearlinkRemoteDevice &device, const UUID &uuid, uint16_t handle, int result) override
    {
        notifyEventCount_++;
        lastEventUuid_ = uuid;
        lastEventHandle_ = handle;
        lastEventResult_ = result;
    }
    void OnConnectionParameterChanged(
        const NearlinkRemoteDevice &device, int interval, int latency, int timeout, int status) override
    {
        connParamCount_++;
        lastInterval_ = interval;
    }

    int connUpdateCount_ = 0;
    int serviceAddedCount_ = 0;
    int propReadReqCount_ = 0;
    int propWriteReqCount_ = 0;
    int descReadReqCount_ = 0;
    int descWriteReqCount_ = 0;
    int mtuCount_ = 0;
    int notifyPropCount_ = 0;
    int notifyEventCount_ = 0;
    int connParamCount_ = 0;

    std::string lastConnAddr_;
    int lastConnState_ = 0;
    int lastConnReason_ = 0;
    int lastAddedRet_ = 0;
    UUID lastAddedUuid_;
    uint16_t lastAddedHandle_ = 0;
    UUID lastPropReadServiceUuid_;
    int lastPropReadRet_ = 0;
    UUID lastPropWriteServiceUuid_;
    const uint8_t *lastPropWriteValue_ = nullptr;
    size_t lastPropWriteValueSize_ = 0;
    int lastPropWriteRet_ = 0;
    uint16_t lastDescReadHandle_ = 0;
    int lastDescReadType_ = 0;
    int lastDescReadRet_ = 0;
    uint16_t lastDescWriteHandle_ = 0;
    int lastDescWriteRet_ = 0;
    int lastMtu_ = 0;
    UUID lastNotifyUuid_;
    uint16_t lastNotifyHandle_ = 0;
    int lastNotifyResult_ = 0;
    UUID lastEventUuid_;
    uint16_t lastEventHandle_ = 0;
    int lastEventResult_ = 0;
    int lastInterval_ = 0;
};

class SsapServerFrameworkTest : public testing::Test {
public:
    std::shared_ptr<FakeSsapServerCallback> fakeCallback_;
    std::shared_ptr<SsapServer> ssapServer_;
    sptr<SsapServer::impl::NearlinkSsapServerCallbackStubImpl> serviceCallback_;
    UUID serviceUuid_;
    UUID propertyUuid_;

    void SetUp() override
    {
        fakeCallback_ = std::make_shared<FakeSsapServerCallback>();
        ssapServer_ = SsapServer::CreateSsapServer(fakeCallback_);
        ASSERT_NE(nullptr, ssapServer_);
        ASSERT_NE(nullptr, ssapServer_->pimpl);
        serviceCallback_ = ssapServer_->pimpl->serviceCallback_;
        ASSERT_NE(nullptr, serviceCallback_);
        serviceUuid_ = UUID::FromString(SERVICE_UUID_STRING);
        propertyUuid_ = UUID::FromString(PROPERTY_UUID_STRING);
    }

    void TearDown() override
    {
        serviceCallback_ = nullptr;
        ssapServer_ = nullptr;
    }

    // 构造含 property(带 descriptor) 的服务 parcel, 经 OnAddService 缓存进 ssapServices
    NearlinkSsapServiceParcel MakeServiceParcel() const
    {
        NearlinkSsapServiceParcel svc;
        svc.handle_ = SVC_HANDLE;
        svc.endHandle_ = SVC_HANDLE + 1;
        svc.isPrimary_ = true;
        svc.uuid_ = ToParcelUuid(serviceUuid_);

        Property prop(PROP_HANDLE, ToParcelUuid(propertyUuid_), TEST_OP_IND);
        prop.permission_ = TEST_PROP_PERM;
        prop.value_ = std::vector<uint8_t>(TEST_VALUE, TEST_VALUE + TEST_VALUE_LEN);
        Descriptor desc(0x0041, 1);
        desc.permission_ = TEST_PROP_PERM;
        desc.value_ = std::vector<uint8_t>(TEST_VALUE, TEST_VALUE + TEST_VALUE_LEN);
        prop.descriptors_.push_back(desc);
        svc.properties_.push_back(prop);
        return NearlinkSsapServiceParcel{svc};
    }

    NearlinkSsapPropertyParcel MakePropertyParcel() const
    {
        NearlinkSsapPropertyParcel prop;
        prop.handle_ = PROP_HANDLE;
        prop.uuid_ = ToParcelUuid(propertyUuid_);
        prop.opInd_ = TEST_OP_IND;
        prop.permission_ = TEST_PROP_PERM;
        prop.value_ = std::vector<uint8_t>(TEST_VALUE, TEST_VALUE + TEST_VALUE_LEN);
        return NearlinkSsapPropertyParcel{prop};
    }

    NearlinkSsapDevice MakeDeviceParcel() const
    {
        NearlinkSsapDevice device;
        device.addr_ = RawAddress(TEST_DEVICE_ADDR);
        device.transport_ = TEST_TRANSPORT;
        return NearlinkSsapDevice{device};
    }

    void AddTestService()
    {
        serviceCallback_->OnAddService(MakeServiceParcel(), static_cast<int32_t>(SsapStatus::SSAP_SUCCESS));
    }
};

/**
 * @tc.name: SsapServerFrameworkCreate001
 * @tc.desc: CreateSsapServer 正常创建且 Init 完成回调桩挂载
 * @tc.type: FUNC
 */
HWTEST_F(SsapServerFrameworkTest, SsapServerFrameworkCreate001, TestSize.Level1)
{
    EXPECT_NE(nullptr, ssapServer_->pimpl->serviceCallback_);
    EXPECT_NE(0, ssapServer_->pimpl->profileRegisterId_); // Init 已分配 profile id
}

/**
 * @tc.name: SsapServerFrameworkInitTwice001
 * @tc.desc: impl::Init 二次调用守卫分支(行299 真腿): 已注册直接返回, 回调桩不重建
 * @tc.type: FUNC
 */
HWTEST_F(SsapServerFrameworkTest, SsapServerFrameworkInitTwice001, TestSize.Level1)
{
    sptr<SsapServer::impl::NearlinkSsapServerCallbackStubImpl> before = ssapServer_->pimpl->serviceCallback_;
    int32_t regIdBefore = ssapServer_->pimpl->profileRegisterId_;
    ssapServer_->pimpl->Init(ssapServer_); // 二次 Init
    EXPECT_EQ(before.GetRefPtr(), ssapServer_->pimpl->serviceCallback_.GetRefPtr());
    EXPECT_EQ(regIdBefore, ssapServer_->pimpl->profileRegisterId_);
}

/**
 * @tc.name: SsapServerFrameworkOnConnStateConnected001
 * @tc.desc: 回调桩 OnConnectionStateChanged(CONNECTED): 设备入已连列表并透传状态
 * @tc.type: FUNC
 */
HWTEST_F(SsapServerFrameworkTest, SsapServerFrameworkOnConnStateConnected001, TestSize.Level1)
{
    constexpr uint8_t state = static_cast<uint8_t>(SleConnectState::CONNECTED);
    constexpr int reason = 6;
    serviceCallback_->OnConnectionStateChanged(MakeDeviceParcel(), state, reason);
    EXPECT_EQ(1, fakeCallback_->connUpdateCount_);
    EXPECT_EQ(TEST_DEVICE_ADDR, fakeCallback_->lastConnAddr_);
    EXPECT_EQ(static_cast<int>(state), fakeCallback_->lastConnState_);
    EXPECT_EQ(reason, fakeCallback_->lastConnReason_);
    // 已连列表可通过 FindConnectedDevice 命中
    NearlinkRemoteDevice device(TEST_DEVICE_ADDR, static_cast<int>(NlTransportType::NL_TRANSPORT_SLE));
    EXPECT_NE(nullptr, ssapServer_->pimpl->FindConnectedDevice(device));
}

/**
 * @tc.name: SsapServerFrameworkOnConnStateDisconnected002
 * @tc.desc: 回调桩 OnConnectionStateChanged(DISCONNECTED): 设备移出已连列表并透传状态
 * @tc.type: FUNC
 */
HWTEST_F(SsapServerFrameworkTest, SsapServerFrameworkOnConnStateDisconnected002, TestSize.Level1)
{
    NearlinkSsapDevice deviceParcel = MakeDeviceParcel();
    constexpr uint8_t connected = static_cast<uint8_t>(SleConnectState::CONNECTED);
    constexpr uint8_t disconnected = static_cast<uint8_t>(SleConnectState::DISCONNECTED);
    serviceCallback_->OnConnectionStateChanged(deviceParcel, connected, 0);
    serviceCallback_->OnConnectionStateChanged(deviceParcel, disconnected, 9);
    EXPECT_EQ(2, fakeCallback_->connUpdateCount_);
    EXPECT_EQ(static_cast<int>(disconnected), fakeCallback_->lastConnState_);
    EXPECT_EQ(9, fakeCallback_->lastConnReason_);
    NearlinkRemoteDevice device(TEST_DEVICE_ADDR, static_cast<int>(NlTransportType::NL_TRANSPORT_SLE));
    EXPECT_EQ(nullptr, ssapServer_->pimpl->FindConnectedDevice(device));
}

/**
 * @tc.name: SsapServerFrameworkOnConnStateServerDead001
 * @tc.desc: 服务端已析构时回调桩直接返回, 不崩溃不上抛
 * @tc.type: FUNC
 */
HWTEST_F(SsapServerFrameworkTest, SsapServerFrameworkOnConnStateServerDead001, TestSize.Level1)
{
    std::shared_ptr<FakeSsapServerCallback> callback = std::make_shared<FakeSsapServerCallback>();
    std::shared_ptr<SsapServer> server = SsapServer::CreateSsapServer(callback);
    ASSERT_NE(nullptr, server);
    sptr<SsapServer::impl::NearlinkSsapServerCallbackStubImpl> stub = server->pimpl->serviceCallback_;
    ASSERT_NE(nullptr, stub);
    server = nullptr; // 模拟服务端生命周期先于回调桩结束
    stub->OnConnectionStateChanged(MakeDeviceParcel(),
        static_cast<uint8_t>(SleConnectState::CONNECTED), 0);
    stub->OnAddService(MakeServiceParcel(), 0);
    stub->OnMtuChanged(MakeDeviceParcel(), 512);
    SUCCEED();
}

/**
 * @tc.name: SsapServerFrameworkOnAddService001
 * @tc.desc: 回调桩 OnAddService: parcel 构建 SsapService 入缓存并透传 OnServiceAdded
 * @tc.type: FUNC
 */
HWTEST_F(SsapServerFrameworkTest, SsapServerFrameworkOnAddService001, TestSize.Level1)
{
    constexpr int32_t addRet = 0;
    serviceCallback_->OnAddService(MakeServiceParcel(), addRet);
    EXPECT_EQ(1, fakeCallback_->serviceAddedCount_);
    EXPECT_EQ(addRet, fakeCallback_->lastAddedRet_);
    EXPECT_TRUE(fakeCallback_->lastAddedUuid_.Equals(serviceUuid_));
    EXPECT_EQ(SVC_HANDLE, fakeCallback_->lastAddedHandle_);
    // 缓存命中: 按服务句柄可查到
    std::shared_ptr<SsapService> svc = nullptr;
    bool found = ssapServer_->pimpl->ssapServices.Find(
        [&svc](const uint16_t handle, std::shared_ptr<SsapService> &val) -> bool {
            if (val->GetHandle() == SVC_HANDLE) {
                svc = val;
                return true;
            }
            return false;
        });
    EXPECT_TRUE(found);
    ASSERT_NE(nullptr, svc);
    EXPECT_EQ(1U, svc->GetProperty().size());
    EXPECT_EQ(1U, svc->GetProperty()[0].GetDescriptors().size());
}

/**
 * @tc.name: SsapServerFrameworkOnPropertyReadRequestHit001
 * @tc.desc: 回调桩 OnPropertyReadRequest uuid 命中分支(行120 真腿): 回调带 serviceUuid
 * @tc.type: FUNC
 */
HWTEST_F(SsapServerFrameworkTest, SsapServerFrameworkOnPropertyReadRequestHit001, TestSize.Level1)
{
    AddTestService();
    constexpr int reqId = 11;
    serviceCallback_->OnPropertyReadRequest(MakeDeviceParcel(), MakePropertyParcel(), reqId);
    EXPECT_EQ(1, fakeCallback_->propReadReqCount_);
    EXPECT_TRUE(fakeCallback_->lastPropReadServiceUuid_.Equals(serviceUuid_));
    EXPECT_EQ(reqId, fakeCallback_->lastPropReadRet_);
}

/**
 * @tc.name: SsapServerFrameworkOnPropertyReadRequestMiss002
 * @tc.desc: 回调桩 OnPropertyReadRequest uuid 未命中分支(行120 假腿): 请求被丢弃, 不回调
 * @tc.type: FUNC
 */
HWTEST_F(SsapServerFrameworkTest, SsapServerFrameworkOnPropertyReadRequestMiss002, TestSize.Level1)
{
    AddTestService();
    NearlinkSsapPropertyParcel unknown = MakePropertyParcel();
    unknown.uuid_ = ToParcelUuid(UUID::FromString(OTHER_UUID_STRING));
    serviceCallback_->OnPropertyReadRequest(MakeDeviceParcel(), unknown, 0);
    EXPECT_EQ(0, fakeCallback_->propReadReqCount_);
}

/**
 * @tc.name: SsapServerFrameworkOnPropertyWriteRequestHit001
 * @tc.desc: 回调桩 OnPropertyWriteRequest uuid 命中分支(行150 真腿): 值与 serviceUuid 透传
 * @tc.type: FUNC
 */
HWTEST_F(SsapServerFrameworkTest, SsapServerFrameworkOnPropertyWriteRequestHit001, TestSize.Level1)
{
    AddTestService();
    constexpr int reqId = 12;
    serviceCallback_->OnPropertyWriteRequest(MakeDeviceParcel(), MakePropertyParcel(), reqId);
    EXPECT_EQ(1, fakeCallback_->propWriteReqCount_);
    EXPECT_TRUE(fakeCallback_->lastPropWriteServiceUuid_.Equals(serviceUuid_));
    EXPECT_EQ(TEST_VALUE_LEN, fakeCallback_->lastPropWriteValueSize_);
    EXPECT_EQ(reqId, fakeCallback_->lastPropWriteRet_);
}

/**
 * @tc.name: SsapServerFrameworkOnPropertyWriteRequestMiss002
 * @tc.desc: 回调桩 OnPropertyWriteRequest uuid 未命中分支(行150 假腿): 请求被丢弃
 * @tc.type: FUNC
 */
HWTEST_F(SsapServerFrameworkTest, SsapServerFrameworkOnPropertyWriteRequestMiss002, TestSize.Level1)
{
    AddTestService();
    NearlinkSsapPropertyParcel unknown = MakePropertyParcel();
    unknown.uuid_ = ToParcelUuid(UUID::FromString(OTHER_UUID_STRING));
    serviceCallback_->OnPropertyWriteRequest(MakeDeviceParcel(), unknown, 0);
    EXPECT_EQ(0, fakeCallback_->propWriteReqCount_);
}

/**
 * @tc.name: SsapServerFrameworkOnDescriptorRequests001
 * @tc.desc: 回调桩 OnDescriptorRead/WriteRequest: 句柄/类型/ret 原样透传
 * @tc.type: FUNC
 */
HWTEST_F(SsapServerFrameworkTest, SsapServerFrameworkOnDescriptorRequests001, TestSize.Level1)
{
    NearlinkSsapDescriptorParcel desc;
    desc.handle_ = 0x0042;
    desc.type_ = 2;
    desc.permission_ = TEST_PROP_PERM;
    constexpr int readRet = 21;
    serviceCallback_->OnDescriptorReadRequest(MakeDeviceParcel(), desc, readRet);
    EXPECT_EQ(1, fakeCallback_->descReadReqCount_);
    EXPECT_EQ(0x0042, fakeCallback_->lastDescReadHandle_);
    EXPECT_EQ(2, fakeCallback_->lastDescReadType_);
    EXPECT_EQ(readRet, fakeCallback_->lastDescReadRet_);

    constexpr int writeRet = 22;
    serviceCallback_->OnDescriptorWriteRequest(MakeDeviceParcel(), desc, writeRet);
    EXPECT_EQ(1, fakeCallback_->descWriteReqCount_);
    EXPECT_EQ(0x0042, fakeCallback_->lastDescWriteHandle_);
    EXPECT_EQ(writeRet, fakeCallback_->lastDescWriteRet_);
}

/**
 * @tc.name: SsapServerFrameworkOnMtuAndNotify001
 * @tc.desc: 回调桩 OnMtuChanged/OnNotifyPropertyChanged/OnNotifyEventChanged: 参数透传
 * @tc.type: FUNC
 */
HWTEST_F(SsapServerFrameworkTest, SsapServerFrameworkOnMtuAndNotify001, TestSize.Level1)
{
    constexpr uint16_t mtu = 256;
    serviceCallback_->OnMtuChanged(MakeDeviceParcel(), mtu);
    EXPECT_EQ(1, fakeCallback_->mtuCount_);
    EXPECT_EQ(mtu, fakeCallback_->lastMtu_);

    constexpr uint16_t notifyHandle = 0x0051;
    constexpr int notifyResult = 0;
    serviceCallback_->OnNotifyPropertyChanged(MakeDeviceParcel(), ToParcelUuid(propertyUuid_),
        notifyHandle, notifyResult);
    EXPECT_EQ(1, fakeCallback_->notifyPropCount_);
    EXPECT_TRUE(fakeCallback_->lastNotifyUuid_.Equals(propertyUuid_));
    EXPECT_EQ(notifyHandle, fakeCallback_->lastNotifyHandle_);
    EXPECT_EQ(notifyResult, fakeCallback_->lastNotifyResult_);

    constexpr uint16_t eventHandle = 0x0061;
    constexpr int eventResult = 1;
    serviceCallback_->OnNotifyEventChanged(MakeDeviceParcel(), ToParcelUuid(serviceUuid_),
        eventHandle, eventResult);
    EXPECT_EQ(1, fakeCallback_->notifyEventCount_);
    EXPECT_TRUE(fakeCallback_->lastEventUuid_.Equals(serviceUuid_));
    EXPECT_EQ(eventHandle, fakeCallback_->lastEventHandle_);
    EXPECT_EQ(eventResult, fakeCallback_->lastEventResult_);
}

/**
 * @tc.name: SsapServerFrameworkFindConnectedDevice001
 * @tc.desc: pimpl FindConnectedDevice 比较分支(行344): 地址+传输命中, 任一不匹配返回空
 * @tc.type: FUNC
 */
HWTEST_F(SsapServerFrameworkTest, SsapServerFrameworkFindConnectedDevice001, TestSize.Level1)
{
    ssapServer_->pimpl->connectedDevices.Insert(SsapDevice(RawAddress(TEST_DEVICE_ADDR), TEST_TRANSPORT));
    ssapServer_->pimpl->connectedDevices.Insert(SsapDevice(RawAddress("AA:BB:CC:DD:EE:FF"), TEST_TRANSPORT + 1));
    NearlinkRemoteDevice sameTransport(TEST_DEVICE_ADDR, static_cast<int>(NlTransportType::NL_TRANSPORT_SLE));
    std::shared_ptr<SsapDevice> hit = ssapServer_->pimpl->FindConnectedDevice(sameTransport);
    ASSERT_NE(nullptr, hit);
    EXPECT_EQ(TEST_DEVICE_ADDR, hit->addr_.GetAddress());
    // 地址不匹配
    NearlinkRemoteDevice wrongAddr("66:55:44:33:22:11", static_cast<int>(NlTransportType::NL_TRANSPORT_SLE));
    EXPECT_EQ(nullptr, ssapServer_->pimpl->FindConnectedDevice(wrongAddr));
    // 传输类型不匹配
    NearlinkRemoteDevice wrongTransport(TEST_DEVICE_ADDR,
        static_cast<int>(NlTransportType::NL_TRANSPORT_SLE) + 1);
    EXPECT_EQ(nullptr, ssapServer_->pimpl->FindConnectedDevice(wrongTransport));
}

/**
 * @tc.name: SsapServerFrameworkFindPropertyByUuid001
 * @tc.desc: FindPropertyByUuid 命中/未命中分支(行514): 命中写出句柄
 * @tc.type: FUNC
 */
HWTEST_F(SsapServerFrameworkTest, SsapServerFrameworkFindPropertyByUuid001, TestSize.Level1)
{
    AddTestService();
    SsapProperty hit(1, propertyUuid_, TEST_OP_IND, TEST_PROP_PERM);
    uint16_t handle = 0;
    EXPECT_TRUE(ssapServer_->FindPropertyByUuid(hit, handle));
    EXPECT_EQ(PROP_HANDLE, handle);
    SsapProperty miss(1, UUID::FromString(OTHER_UUID_STRING), TEST_OP_IND, TEST_PROP_PERM);
    EXPECT_FALSE(ssapServer_->FindPropertyByUuid(miss, handle));
}

/**
 * @tc.name: SsapServerFrameworkBuildServiceUuid16Type001
 * @tc.desc: impl BuildService: 属性 uuid 为 16bit 时映射 ENTRY_TYPE_PROPERTY, 128bit 映射 VENDOR
 * @tc.type: FUNC
 */
HWTEST_F(SsapServerFrameworkTest, SsapServerFrameworkBuildServiceUuid16Type001, TestSize.Level1)
{
    NearlinkSsapServiceParcel svc;
    svc.handle_ = SVC_HANDLE;
    svc.isPrimary_ = true;
    svc.uuid_ = ToParcelUuid(serviceUuid_);

    NearlinkSsapServiceParcel built16{svc};
    Property prop16(PROP_HANDLE, Uuid::ConvertFrom16Bits(0xF010), TEST_OP_IND);
    prop16.permission_ = TEST_PROP_PERM;
    built16.properties_.push_back(prop16);
    std::shared_ptr<SsapService> svc16 = ssapServer_->pimpl->BuildService(built16);
    ASSERT_NE(nullptr, svc16);
    ASSERT_EQ(1U, svc16->GetProperty().size());
    EXPECT_EQ(static_cast<int>(SsapProperty::PropertyType::ENTRY_TYPE_PROPERTY),
        svc16->GetProperty()[0].GetPropertyType());

    NearlinkSsapServiceParcel built128{svc};
    Property prop128(PROP_HANDLE, ToParcelUuid(propertyUuid_), TEST_OP_IND);
    prop128.permission_ = TEST_PROP_PERM;
    built128.properties_.push_back(prop128);
    std::shared_ptr<SsapService> svc128 = ssapServer_->pimpl->BuildService(built128);
    ASSERT_NE(nullptr, svc128);
    ASSERT_EQ(1U, svc128->GetProperty().size());
    EXPECT_EQ(static_cast<int>(SsapProperty::PropertyType::ENTRY_TYPE_VENDOR_PROPERTY),
        svc128->GetProperty()[0].GetPropertyType());
}
} // namespace TEST
} // namespace Nearlink
} // namespace OHOS
