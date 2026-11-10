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

// 目标: frameworks/native/nearlink_ssap_client.cpp 零覆盖分支(进程内驱动, 不依赖真实 SLE 栈)
// 手法: 编入产品源文件(private=public) 直接构造 impl 层回调桩, 用记录型 Fake SsapClientCallback
//       在进程内驱动 On* 回调与 pimpl 内部函数, 瞄准 106/131/159/183/189/213/219/247/281/322/328/
//       336/414/577/599/931/935/1062/1076 行分支; 网关后(需真实代理/SLE 使能)的分支不在本套用例范围

#include <gtest/gtest.h>
#include <cstring>
#include <vector>

#include "nearlink_def.h"
#include "nearlink_ssap_client.cpp"
#include "nearlink_ssap_property.h"
#include "nearlink_ssap_descriptor.h"
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
const std::string METHOD_UUID_STRING = "11223344-0000-1000-8000-00805F9B3333";
const std::string EVENT_UUID_STRING = "11223344-0000-1000-8000-00805F9B2222";
const std::string OTHER_UUID_STRING = "FF223344-0000-1000-8000-00805F9B9999"; // 首字节即不同, 触发 Equals 首字节比较路径
constexpr uint16_t SVC_HANDLE = 0x0101;
constexpr uint16_t PROP_HANDLE = 0x0011;
constexpr uint16_t MTHD_HANDLE = 0x0021;
constexpr uint16_t EVT_HANDLE = 0x0031;
constexpr uint32_t TEST_OP_IND = 0x0F;
constexpr size_t TEST_VALUE_LEN = 3;
const uint8_t TEST_VALUE[TEST_VALUE_LEN] = {1, 2, 3};

// 工具: 框架 UUID -> 服务侧 Uuid(两者字节序转换链与产品 BuildService 一致)
Uuid ToParcelUuid(const UUID &uuid)
{
    return Uuid::ConvertFrom128Bits(uuid.ConvertTo128Bits());
}
} // namespace

// 记录型 Fake: 实现 SsapClientCallback 全部回调, 记录调用计数与关键参数
class FakeSsapClientCallback : public SsapClientCallback {
public:
    void OnConnectionStateChanged(int connectionState, int ret) override
    {
        connStateCount_++;
        lastConnState_ = connectionState;
        lastConnRet_ = ret;
    }
    void OnPropertyChanged(const SsapProperty &property) override
    {
        propChangedCount_++;
        lastPropUuid_ = property.GetUuid();
        lastPropServiceUuid_ = property.GetServiceUuid();
        lastPropValueSize_ = 0;
        lastPropValue_ = property.GetValue(&lastPropValueSize_).get();
    }
    void OnEventNotified(const SsapEvent &event) override
    {
        eventNotifiedCount_++;
        lastEventUuid_ = event.GetUuid();
        lastEventServiceUuid_ = event.GetServiceUuid();
        lastEventParamSize_ = 0;
        lastEventParam_ = event.GetParameter(&lastEventParamSize_).get();
    }
    void OnPropertyReadResult(const SsapProperty &property, int ret) override
    {
        propReadCount_++;
        lastPropReadRet_ = ret;
        lastPropReadServiceUuid_ = property.GetServiceUuid();
        lastPropReadValueSize_ = 0;
        lastPropReadValue_ = property.GetValue(&lastPropReadValueSize_).get();
        lastPropReadOperInd_ = property.GetOperationIndication();
    }
    void OnMethodCallResult(const SsapMethod &method, int ret) override
    {
        methodCallCount_++;
        lastMethodRet_ = ret;
        lastMethodServiceUuid_ = method.GetServiceUuid();
        lastMethodResultSize_ = 0;
        lastMethodResult_ = method.GetResult(&lastMethodResultSize_).get();
    }
    void OnPropertiesReadResult(const std::vector<SsapProperty> &properties, int ret) override
    {
        propsReadCount_++;
        lastPropsReadRet_ = ret;
        lastPropsReadSize_ = properties.size();
        if (!properties.empty()) {
            lastPropsReadServiceUuid_ = properties[0].GetServiceUuid();
            lastPropsReadValueSize_ = 0;
            lastPropsReadValue_ = properties[0].GetValue(&lastPropsReadValueSize_).get();
        }
    }
    void OnReadRemoteRssiValueResult(int rssi, int status) override
    {
        rssiCount_++;
        lastRssi_ = rssi;
    }
    void OnPropertyWriteResult(const SsapProperty &property, int ret) override
    {
        propWriteCount_++;
        lastPropWriteRet_ = ret;
        lastPropWriteType_ = property.GetWriteType();
        lastPropWriteValueSize_ = 0;
        lastPropWriteValue_ = property.GetValue(&lastPropWriteValueSize_).get();
    }
    void OnDescriptorReadResult(const SsapDescriptor &descriptor, int ret) override
    {
        descReadCount_++;
        lastDescReadRet_ = ret;
        lastDescReadHandle_ = descriptor.GetHandle();
        lastDescReadServiceUuid_ = descriptor.GetServiceUuid();
        lastDescReadPropUuid_ = descriptor.GetPropertyUuid();
        lastDescReadValueSize_ = 0;
        lastDescReadValue_ = descriptor.GetValue(&lastDescReadValueSize_).get();
    }
    void OnDescriptorWriteResult(const SsapDescriptor &descriptor, int ret) override
    {
        descWriteCount_++;
        lastDescWriteHandle_ = descriptor.GetHandle();
        lastDescWriteRet_ = ret;
    }
    void OnMtuUpdate(uint16_t mtu, int ret) override
    {
        mtuCount_++;
        lastMtu_ = mtu;
        lastMtuRet_ = ret;
    }
    void OnServicesDiscovered(int status) override
    {
        discoveredCount_++;
        lastDiscoveredStatus_ = status;
    }
    void OnServicesDiscoveredByUuid(int status, const UUID &uuid) override
    {
        discoveredByUuidCount_++;
        lastDiscoveredStatus_ = status;
        lastDiscoveredUuid_ = uuid;
    }
    void OnConnectionParameterChanged(int interval, int latency, int timeout, int status) override
    {
        connParamCount_++;
        lastInterval_ = interval;
        lastLatency_ = latency;
        lastTimeout_ = timeout;
        lastConnParamStatus_ = status;
    }
    void OnServicesRediscovered(const std::vector<SsapService> &services) override
    {
        rediscoveredCount_++;
        lastRediscoveredSize_ = services.size();
        if (!services.empty()) {
            lastRediscoveredUuid_ = services[0].GetUuid();
        }
    }
    void OnServiceChanged(uint16_t handle, const UUID &uuid) override
    {
        serviceChangedCount_++;
        lastChangedHandle_ = handle;
        lastChangedUuid_ = uuid;
    }
    void OnSetPropertyNotifyResult(const SsapProperty &property, int enable, int ret) override
    {
        notifyResultCount_++;
        lastNotifyEnable_ = enable;
        lastNotifyRet_ = ret;
    }
    void OnSetPropertyIndicateResult(const SsapProperty &property, int enable, int ret) override
    {
        indicateResultCount_++;
        lastIndicateEnable_ = enable;
        lastIndicateRet_ = ret;
    }

    int connStateCount_ = 0;
    int propChangedCount_ = 0;
    int eventNotifiedCount_ = 0;
    int propReadCount_ = 0;
    int methodCallCount_ = 0;
    int propsReadCount_ = 0;
    int rssiCount_ = 0;
    int propWriteCount_ = 0;
    int descReadCount_ = 0;
    int descWriteCount_ = 0;
    int mtuCount_ = 0;
    int discoveredCount_ = 0;
    int discoveredByUuidCount_ = 0;
    int connParamCount_ = 0;
    int rediscoveredCount_ = 0;
    int serviceChangedCount_ = 0;
    int notifyResultCount_ = 0;
    int indicateResultCount_ = 0;

    int lastConnState_ = 0;
    int lastConnRet_ = 0;
    UUID lastPropUuid_;
    UUID lastPropServiceUuid_;
    const uint8_t *lastPropValue_ = nullptr;
    size_t lastPropValueSize_ = 0;
    UUID lastEventUuid_;
    UUID lastEventServiceUuid_;
    const uint8_t *lastEventParam_ = nullptr;
    size_t lastEventParamSize_ = 0;
    int lastPropReadRet_ = 0;
    UUID lastPropReadServiceUuid_;
    const uint8_t *lastPropReadValue_ = nullptr;
    size_t lastPropReadValueSize_ = 0;
    uint32_t lastPropReadOperInd_ = 0;
    int lastMethodRet_ = 0;
    UUID lastMethodServiceUuid_;
    const uint8_t *lastMethodResult_ = nullptr;
    size_t lastMethodResultSize_ = 0;
    int lastPropsReadRet_ = 0;
    size_t lastPropsReadSize_ = 0;
    UUID lastPropsReadServiceUuid_;
    const uint8_t *lastPropsReadValue_ = nullptr;
    size_t lastPropsReadValueSize_ = 0;
    int lastRssi_ = 0;
    int lastPropWriteRet_ = 0;
    int lastPropWriteType_ = 0;
    const uint8_t *lastPropWriteValue_ = nullptr;
    size_t lastPropWriteValueSize_ = 0;
    int lastDescReadRet_ = 0;
    uint16_t lastDescReadHandle_ = 0;
    UUID lastDescReadServiceUuid_;
    UUID lastDescReadPropUuid_;
    const uint8_t *lastDescReadValue_ = nullptr;
    size_t lastDescReadValueSize_ = 0;
    uint16_t lastDescWriteHandle_ = 0;
    int lastDescWriteRet_ = 0;
    uint16_t lastMtu_ = 0;
    int lastMtuRet_ = 0;
    int lastDiscoveredStatus_ = 0;
    UUID lastDiscoveredUuid_;
    int lastInterval_ = 0;
    int lastLatency_ = 0;
    int lastTimeout_ = 0;
    int lastConnParamStatus_ = 0;
    size_t lastRediscoveredSize_ = 0;
    UUID lastRediscoveredUuid_;
    uint16_t lastChangedHandle_ = 0;
    UUID lastChangedUuid_;
    int lastNotifyEnable_ = 0;
    int lastNotifyRet_ = 0;
    int lastIndicateEnable_ = 0;
    int lastIndicateRet_ = 0;
};

class SsapClientFrameworkTest : public testing::Test {
public:
    std::shared_ptr<NearlinkRemoteDevice> device_;
    std::shared_ptr<SsapClient> ssapClient_;
    std::shared_ptr<FakeSsapClientCallback> fakeCallback_;
    sptr<SsapClient::impl::NearlinkSsapClientCallbackStubImpl> clientCallback_;
    UUID serviceUuid_;
    UUID propertyUuid_;
    UUID methodUuid_;
    UUID eventUuid_;

    void SetUp() override
    {
        device_ = std::make_shared<NearlinkRemoteDevice>(
            TEST_DEVICE_ADDR, static_cast<int>(NlTransportType::NL_TRANSPORT_SLE));
        ssapClient_ = SsapClient::CreateSsapClient(device_);
        ASSERT_NE(nullptr, ssapClient_);
        fakeCallback_ = std::make_shared<FakeSsapClientCallback>();
        clientCallback_ = new SsapClient::impl::NearlinkSsapClientCallbackStubImpl(ssapClient_);
        ssapClient_->pimpl->clientCallback_ = clientCallback_;
        ssapClient_->pimpl->callback_ = fakeCallback_;
        serviceUuid_ = UUID::FromString(SERVICE_UUID_STRING);
        propertyUuid_ = UUID::FromString(PROPERTY_UUID_STRING);
        methodUuid_ = UUID::FromString(METHOD_UUID_STRING);
        eventUuid_ = UUID::FromString(EVENT_UUID_STRING);
    }

    void TearDown() override
    {
        clientCallback_ = nullptr;
        ssapClient_ = nullptr;
    }

    // 构造含 property/method/event 的服务 parcel, 供 BuildServiceList 缓存进 ssapServices_
    NearlinkSsapServiceParcel MakeServiceParcel() const
    {
        NearlinkSsapServiceParcel svc;
        svc.handle_ = SVC_HANDLE;
        svc.endHandle_ = SVC_HANDLE + 1;
        svc.isPrimary_ = true;
        svc.uuid_ = ToParcelUuid(serviceUuid_);

        Property prop(PROP_HANDLE, ToParcelUuid(propertyUuid_), TEST_OP_IND);
        prop.value_ = std::vector<uint8_t>(TEST_VALUE, TEST_VALUE + TEST_VALUE_LEN);
        svc.properties_.push_back(prop);

        Method mthd(MTHD_HANDLE, ToParcelUuid(methodUuid_));
        mthd.result_ = std::vector<uint8_t>(TEST_VALUE, TEST_VALUE + TEST_VALUE_LEN);
        svc.methods_.push_back(mthd);

        Event evt(EVT_HANDLE, ToParcelUuid(eventUuid_));
        evt.parameter_ = std::vector<uint8_t>(TEST_VALUE, TEST_VALUE + TEST_VALUE_LEN);
        svc.events_.push_back(evt);
        return NearlinkSsapServiceParcel{svc};
    }

    NearlinkSsapPropertyParcel MakePropertyParcel() const
    {
        NearlinkSsapPropertyParcel prop;
        prop.handle_ = PROP_HANDLE;
        prop.uuid_ = ToParcelUuid(propertyUuid_);
        prop.opInd_ = TEST_OP_IND;
        prop.permission_ = 0;
        prop.value_ = std::vector<uint8_t>(TEST_VALUE, TEST_VALUE + TEST_VALUE_LEN);
        return NearlinkSsapPropertyParcel{prop};
    }

    NearlinkSsapEventParcel MakeEventParcel() const
    {
        NearlinkSsapEventParcel evt;
        evt.handle_ = EVT_HANDLE;
        evt.uuid_ = ToParcelUuid(eventUuid_);
        evt.parameter_ = std::vector<uint8_t>(TEST_VALUE, TEST_VALUE + TEST_VALUE_LEN);
        return NearlinkSsapEventParcel{evt};
    }

    NearlinkSsapMethodParcel MakeMethodParcel() const
    {
        NearlinkSsapMethodParcel mthd;
        mthd.handle_ = MTHD_HANDLE;
        mthd.uuid_ = ToParcelUuid(methodUuid_);
        mthd.result_ = std::vector<uint8_t>(TEST_VALUE, TEST_VALUE + TEST_VALUE_LEN);
        return NearlinkSsapMethodParcel{mthd};
    }

    void CacheTestService()
    {
        std::vector<NearlinkSsapServiceParcel> parcelVec;
        parcelVec.push_back(MakeServiceParcel());
        ssapClient_->pimpl->BuildServiceList(parcelVec);
    }
};

/**
 * @tc.name: SsapClientFrameworkCreateNullDevice001
 * @tc.desc: CreateSsapClient 空设备指针返回 nullptr
 * @tc.type: FUNC
 */
HWTEST_F(SsapClientFrameworkTest, SsapClientFrameworkCreateNullDevice001, TestSize.Level1)
{
    EXPECT_EQ(nullptr, SsapClient::CreateSsapClient(nullptr));
}

/**
 * @tc.name: SsapClientFrameworkGetDeviceAddr001
 * @tc.desc: GetDeviceAddr 返回构造时传入的设备地址(不依赖连接状态)
 * @tc.type: FUNC
 */
HWTEST_F(SsapClientFrameworkTest, SsapClientFrameworkGetDeviceAddr001, TestSize.Level1)
{
    EXPECT_EQ(TEST_DEVICE_ADDR, ssapClient_->GetDeviceAddr());
}

/**
 * @tc.name: SsapClientFrameworkOnConnStateConnected001
 * @tc.desc: 回调桩 OnConnectionStateChanged(CONNECTED): 状态存储并透传给上层回调
 * @tc.type: FUNC
 */
HWTEST_F(SsapClientFrameworkTest, SsapClientFrameworkOnConnStateConnected001, TestSize.Level1)
{
    constexpr int32_t reason = 7;
    clientCallback_->OnConnectionStateChanged(reason, static_cast<int>(SleConnectState::CONNECTED));
    EXPECT_EQ(1, fakeCallback_->connStateCount_);
    EXPECT_EQ(static_cast<int>(SleConnectState::CONNECTED), fakeCallback_->lastConnState_);
    EXPECT_EQ(reason, fakeCallback_->lastConnRet_);
    EXPECT_EQ(static_cast<int>(SleConnectState::CONNECTED), ssapClient_->pimpl->connectionState_.load());
}

/**
 * @tc.name: SsapClientFrameworkOnConnStateDisconnected002
 * @tc.desc: 回调桩 OnConnectionStateChanged(DISCONNECTED): 触发 CleanConnectionInfo(行106),
 *           isFinding 置位时发出 OnServicesDiscovered(SSAP_FAILURE)
 * @tc.type: FUNC
 */
HWTEST_F(SsapClientFrameworkTest, SsapClientFrameworkOnConnStateDisconnected002, TestSize.Level1)
{
    ssapClient_->pimpl->findInformation_.isFinding = true;
    clientCallback_->OnConnectionStateChanged(0, static_cast<int>(SleConnectState::DISCONNECTED));
    EXPECT_EQ(1, fakeCallback_->connStateCount_);
    EXPECT_EQ(static_cast<int>(SleConnectState::DISCONNECTED), fakeCallback_->lastConnState_);
    EXPECT_EQ(static_cast<int>(SleConnectState::DISCONNECTED), ssapClient_->pimpl->connectionState_.load());
    EXPECT_FALSE(ssapClient_->pimpl->findInformation_.isFinding);
    EXPECT_EQ(1, fakeCallback_->discoveredCount_);
    EXPECT_EQ(static_cast<int>(SsapStatus::SSAP_FAILURE), fakeCallback_->lastDiscoveredStatus_);
}

/**
 * @tc.name: SsapClientFrameworkOnConnStateClientDead001
 * @tc.desc: 客户端已析构时回调桩直接返回, 不崩溃不上抛
 * @tc.type: FUNC
 */
HWTEST_F(SsapClientFrameworkTest, SsapClientFrameworkOnConnStateClientDead001, TestSize.Level1)
{
    std::shared_ptr<NearlinkRemoteDevice> device = std::make_shared<NearlinkRemoteDevice>(
        TEST_DEVICE_ADDR, static_cast<int>(NlTransportType::NL_TRANSPORT_SLE));
    std::shared_ptr<SsapClient> client = SsapClient::CreateSsapClient(device);
    ASSERT_NE(nullptr, client);
    sptr<SsapClient::impl::NearlinkSsapClientCallbackStubImpl> stub =
        new SsapClient::impl::NearlinkSsapClientCallbackStubImpl(client);
    client = nullptr; // 模拟客户端生命周期先于回调桩结束
    stub->OnConnectionStateChanged(0, static_cast<int>(SleConnectState::CONNECTED));
    stub->OnPropertyChanged(MakePropertyParcel());
    stub->OnMtuChanged(0, 512);
    SUCCEED();
}

/**
 * @tc.name: SsapClientFrameworkOnConnStateNoCallback001
 * @tc.desc: 上层回调未注册(weak_ptr 空)时只打日志, 不崩溃
 * @tc.type: FUNC
 */
HWTEST_F(SsapClientFrameworkTest, SsapClientFrameworkOnConnStateNoCallback001, TestSize.Level1)
{
    std::shared_ptr<NearlinkRemoteDevice> device = std::make_shared<NearlinkRemoteDevice>(
        TEST_DEVICE_ADDR, static_cast<int>(NlTransportType::NL_TRANSPORT_SLE));
    std::shared_ptr<SsapClient> client = SsapClient::CreateSsapClient(device);
    ASSERT_NE(nullptr, client);
    sptr<SsapClient::impl::NearlinkSsapClientCallbackStubImpl> stub =
        new SsapClient::impl::NearlinkSsapClientCallbackStubImpl(client);
    stub->OnMtuChanged(0, 256); // 未设置 pimpl->callback_
    client = nullptr;
    SUCCEED();
}

/**
 * @tc.name: SsapClientFrameworkOnPropertyChangedHit001
 * @tc.desc: 回调桩 OnPropertyChanged 命中分支(行131): 更新缓存属性值并带 serviceUuid 透传
 * @tc.type: FUNC
 */
HWTEST_F(SsapClientFrameworkTest, SsapClientFrameworkOnPropertyChangedHit001, TestSize.Level1)
{
    CacheTestService();
    clientCallback_->OnPropertyChanged(MakePropertyParcel());
    EXPECT_EQ(1, fakeCallback_->propChangedCount_);
    EXPECT_TRUE(fakeCallback_->lastPropServiceUuid_.Equals(serviceUuid_));
    EXPECT_EQ(TEST_VALUE_LEN, fakeCallback_->lastPropValueSize_);
    // 缓存中的属性值被同步更新
    std::shared_ptr<SsapService> svc = ssapClient_->pimpl->FindService(SVC_HANDLE);
    ASSERT_NE(nullptr, svc);
    ASSERT_EQ(1U, svc->GetProperty().size());
    size_t cachedSize = 0;
    const auto &cachedValue = svc->GetProperty()[0].GetValue(&cachedSize);
    ASSERT_NE(nullptr, cachedValue.get());
    EXPECT_EQ(TEST_VALUE_LEN, cachedSize);
}

/**
 * @tc.name: SsapClientFrameworkOnPropertyChangedMiss002
 * @tc.desc: 回调桩 OnPropertyChanged 未命中分支(行131 假腿): 无缓存也透传, serviceUuid 为空
 * @tc.type: FUNC
 */
HWTEST_F(SsapClientFrameworkTest, SsapClientFrameworkOnPropertyChangedMiss002, TestSize.Level1)
{
    CacheTestService();
    NearlinkSsapPropertyParcel unknown = MakePropertyParcel();
    unknown.handle_ = 0x9999; // 缓存中不存在的句柄
    clientCallback_->OnPropertyChanged(unknown);
    EXPECT_EQ(1, fakeCallback_->propChangedCount_);
    EXPECT_FALSE(fakeCallback_->lastPropServiceUuid_.Equals(serviceUuid_));
}

/**
 * @tc.name: SsapClientFrameworkOnEventNotifiedHit001
 * @tc.desc: 回调桩 OnEventNotified 命中分支(行159): 以缓存事件 uuid/serviceUuid 透传
 * @tc.type: FUNC
 */
HWTEST_F(SsapClientFrameworkTest, SsapClientFrameworkOnEventNotifiedHit001, TestSize.Level1)
{
    CacheTestService();
    clientCallback_->OnEventNotified(MakeEventParcel());
    EXPECT_EQ(1, fakeCallback_->eventNotifiedCount_);
    EXPECT_TRUE(fakeCallback_->lastEventUuid_.Equals(eventUuid_));
    EXPECT_TRUE(fakeCallback_->lastEventServiceUuid_.Equals(serviceUuid_));
    EXPECT_EQ(TEST_VALUE_LEN, fakeCallback_->lastEventParamSize_);
}

/**
 * @tc.name: SsapClientFrameworkOnEventNotifiedMiss002
 * @tc.desc: 回调桩 OnEventNotified 未命中分支: 透传时 serviceUuid 为空
 * @tc.type: FUNC
 */
HWTEST_F(SsapClientFrameworkTest, SsapClientFrameworkOnEventNotifiedMiss002, TestSize.Level1)
{
    CacheTestService();
    NearlinkSsapEventParcel unknown = MakeEventParcel();
    unknown.handle_ = 0x9999;
    clientCallback_->OnEventNotified(unknown);
    EXPECT_EQ(1, fakeCallback_->eventNotifiedCount_);
    EXPECT_FALSE(fakeCallback_->lastEventServiceUuid_.Equals(serviceUuid_));
}

/**
 * @tc.name: SsapClientFrameworkOnReadPropertySuccessHit001
 * @tc.desc: 回调桩 OnReadProperty 成功+uuid 命中(行183/189 真腿): 值与 serviceUuid 透传
 * @tc.type: FUNC
 */
HWTEST_F(SsapClientFrameworkTest, SsapClientFrameworkOnReadPropertySuccessHit001, TestSize.Level1)
{
    CacheTestService();
    clientCallback_->OnReadProperty(static_cast<int32_t>(SsapStatus::SSAP_SUCCESS), MakePropertyParcel());
    EXPECT_EQ(1, fakeCallback_->propReadCount_);
    EXPECT_EQ(static_cast<int>(SsapStatus::SSAP_SUCCESS), fakeCallback_->lastPropReadRet_);
    EXPECT_TRUE(fakeCallback_->lastPropReadServiceUuid_.Equals(serviceUuid_));
    EXPECT_EQ(TEST_VALUE_LEN, fakeCallback_->lastPropReadValueSize_);
    EXPECT_EQ(TEST_OP_IND, fakeCallback_->lastPropReadOperInd_);
}

/**
 * @tc.name: SsapClientFrameworkOnReadPropertyFailKeepsValue002
 * @tc.desc: 回调桩 OnReadProperty 失败(行183 假腿): 不回填值仍透传结果
 * @tc.type: FUNC
 */
HWTEST_F(SsapClientFrameworkTest, SsapClientFrameworkOnReadPropertyFailKeepsValue002, TestSize.Level1)
{
    CacheTestService();
    clientCallback_->OnReadProperty(static_cast<int32_t>(SsapStatus::SSAP_FAILURE), MakePropertyParcel());
    EXPECT_EQ(1, fakeCallback_->propReadCount_);
    EXPECT_EQ(static_cast<int>(SsapStatus::SSAP_FAILURE), fakeCallback_->lastPropReadRet_);
    EXPECT_EQ(nullptr, fakeCallback_->lastPropReadValue_);
}

/**
 * @tc.name: SsapClientFrameworkOnReadPropertyUuidMiss003
 * @tc.desc: 回调桩 OnReadProperty uuid 未命中(行189 假腿): 结果被丢弃, 不透传
 * @tc.type: FUNC
 */
HWTEST_F(SsapClientFrameworkTest, SsapClientFrameworkOnReadPropertyUuidMiss003, TestSize.Level1)
{
    CacheTestService();
    NearlinkSsapPropertyParcel unknown = MakePropertyParcel();
    unknown.uuid_ = ToParcelUuid(UUID::FromString(OTHER_UUID_STRING));
    clientCallback_->OnReadProperty(static_cast<int32_t>(SsapStatus::SSAP_SUCCESS), unknown);
    EXPECT_EQ(0, fakeCallback_->propReadCount_);
}

/**
 * @tc.name: SsapClientFrameworkOnCallMethodSuccessHit001
 * @tc.desc: 回调桩 OnCallMethod 成功+句柄命中(行213/219 真腿): result 与 serviceUuid 透传
 * @tc.type: FUNC
 */
HWTEST_F(SsapClientFrameworkTest, SsapClientFrameworkOnCallMethodSuccessHit001, TestSize.Level1)
{
    CacheTestService();
    clientCallback_->OnCallMethod(static_cast<int32_t>(SsapStatus::SSAP_SUCCESS), MakeMethodParcel());
    EXPECT_EQ(1, fakeCallback_->methodCallCount_);
    EXPECT_EQ(static_cast<int>(SsapStatus::SSAP_SUCCESS), fakeCallback_->lastMethodRet_);
    EXPECT_TRUE(fakeCallback_->lastMethodServiceUuid_.Equals(serviceUuid_));
    EXPECT_EQ(TEST_VALUE_LEN, fakeCallback_->lastMethodResultSize_);
}

/**
 * @tc.name: SsapClientFrameworkOnCallMethodFail002
 * @tc.desc: 回调桩 OnCallMethod 失败(行213 假腿): result 不回填仍透传
 * @tc.type: FUNC
 */
HWTEST_F(SsapClientFrameworkTest, SsapClientFrameworkOnCallMethodFail002, TestSize.Level1)
{
    CacheTestService();
    clientCallback_->OnCallMethod(static_cast<int32_t>(SsapStatus::SSAP_FAILURE), MakeMethodParcel());
    EXPECT_EQ(1, fakeCallback_->methodCallCount_);
    EXPECT_EQ(nullptr, fakeCallback_->lastMethodResult_);
}

/**
 * @tc.name: SsapClientFrameworkOnCallMethodHandleMiss003
 * @tc.desc: 回调桩 OnCallMethod 句柄未命中(行219 假腿): 结果被丢弃, 不透传
 * @tc.type: FUNC
 */
HWTEST_F(SsapClientFrameworkTest, SsapClientFrameworkOnCallMethodHandleMiss003, TestSize.Level1)
{
    CacheTestService();
    NearlinkSsapMethodParcel unknown = MakeMethodParcel();
    unknown.handle_ = 0x9999;
    clientCallback_->OnCallMethod(static_cast<int32_t>(SsapStatus::SSAP_SUCCESS), unknown);
    EXPECT_EQ(0, fakeCallback_->methodCallCount_);
}

/**
 * @tc.name: SsapClientFrameworkOnReadPropertiesByUuidError001
 * @tc.desc: 回调桩 OnReadPropertiesByUuid 失败腿: 空列表+错误码透传, 避免 promise 挂起
 * @tc.type: FUNC
 */
HWTEST_F(SsapClientFrameworkTest, SsapClientFrameworkOnReadPropertiesByUuidError001, TestSize.Level1)
{
    std::vector<NearlinkSsapPropertyParcel> props;
    props.push_back(MakePropertyParcel());
    clientCallback_->OnReadPropertiesByUuid(static_cast<int32_t>(SsapStatus::SSAP_FAILURE), props);
    EXPECT_EQ(1, fakeCallback_->propsReadCount_);
    EXPECT_EQ(0U, fakeCallback_->lastPropsReadSize_);
    EXPECT_EQ(static_cast<int>(SsapStatus::SSAP_FAILURE), fakeCallback_->lastPropsReadRet_);
}

/**
 * @tc.name: SsapClientFrameworkOnReadPropertiesByUuidEmpty002
 * @tc.desc: 回调桩 OnReadPropertiesByUuid 空结果腿: 空列表+ret 透传
 * @tc.type: FUNC
 */
HWTEST_F(SsapClientFrameworkTest, SsapClientFrameworkOnReadPropertiesByUuidEmpty002, TestSize.Level1)
{
    std::vector<NearlinkSsapPropertyParcel> empty;
    clientCallback_->OnReadPropertiesByUuid(static_cast<int32_t>(SsapStatus::SSAP_SUCCESS), empty);
    EXPECT_EQ(1, fakeCallback_->propsReadCount_);
    EXPECT_EQ(0U, fakeCallback_->lastPropsReadSize_);
    EXPECT_EQ(static_cast<int>(SsapStatus::SSAP_SUCCESS), fakeCallback_->lastPropsReadRet_);
}

/**
 * @tc.name: SsapClientFrameworkOnReadPropertiesByUuidHit003
 * @tc.desc: 回调桩 OnReadPropertiesByUuid 成功+uuid 命中(行247 真腿): 结果列表带 serviceUuid
 * @tc.type: FUNC
 */
HWTEST_F(SsapClientFrameworkTest, SsapClientFrameworkOnReadPropertiesByUuidHit003, TestSize.Level1)
{
    CacheTestService();
    std::vector<NearlinkSsapPropertyParcel> props;
    props.push_back(MakePropertyParcel());
    NearlinkSsapPropertyParcel second = MakePropertyParcel();
    second.handle_ = PROP_HANDLE + 1;
    props.push_back(second);
    clientCallback_->OnReadPropertiesByUuid(static_cast<int32_t>(SsapStatus::SSAP_SUCCESS), props);
    EXPECT_EQ(1, fakeCallback_->propsReadCount_);
    EXPECT_EQ(2U, fakeCallback_->lastPropsReadSize_);
    EXPECT_TRUE(fakeCallback_->lastPropsReadServiceUuid_.Equals(serviceUuid_));
    EXPECT_EQ(TEST_VALUE_LEN, fakeCallback_->lastPropsReadValueSize_);
}

/**
 * @tc.name: SsapClientFrameworkOnReadPropertiesByUuidMiss004
 * @tc.desc: 回调桩 OnReadPropertiesByUuid uuid 未命中(行247 假腿): 数据不丢弃, serviceUuid 留空
 * @tc.type: FUNC
 */
HWTEST_F(SsapClientFrameworkTest, SsapClientFrameworkOnReadPropertiesByUuidMiss004, TestSize.Level1)
{
    CacheTestService();
    NearlinkSsapPropertyParcel unknown = MakePropertyParcel();
    unknown.uuid_ = ToParcelUuid(UUID::FromString(OTHER_UUID_STRING));
    std::vector<NearlinkSsapPropertyParcel> props;
    props.push_back(unknown);
    clientCallback_->OnReadPropertiesByUuid(static_cast<int32_t>(SsapStatus::SSAP_SUCCESS), props);
    EXPECT_EQ(1, fakeCallback_->propsReadCount_);
    EXPECT_EQ(1U, fakeCallback_->lastPropsReadSize_);
    EXPECT_FALSE(fakeCallback_->lastPropsReadServiceUuid_.Equals(serviceUuid_));
    EXPECT_EQ(TEST_VALUE_LEN, fakeCallback_->lastPropsReadValueSize_);
}

/**
 * @tc.name: SsapClientFrameworkOnWritePropertySuccess001
 * @tc.desc: 回调桩 OnWriteProperty 成功(行281 真腿): 值回填且写类型为 PROPERTY_WRITE_REQ
 * @tc.type: FUNC
 */
HWTEST_F(SsapClientFrameworkTest, SsapClientFrameworkOnWritePropertySuccess001, TestSize.Level1)
{
    clientCallback_->OnWriteProperty(static_cast<int32_t>(SsapStatus::SSAP_SUCCESS), MakePropertyParcel());
    EXPECT_EQ(1, fakeCallback_->propWriteCount_);
    EXPECT_EQ(static_cast<int>(SsapStatus::SSAP_SUCCESS), fakeCallback_->lastPropWriteRet_);
    EXPECT_EQ(TEST_VALUE_LEN, fakeCallback_->lastPropWriteValueSize_);
    EXPECT_EQ(SsapProperty::PROPERTY_WRITE_REQ, fakeCallback_->lastPropWriteType_);
}

/**
 * @tc.name: SsapClientFrameworkOnWritePropertyFail002
 * @tc.desc: 回调桩 OnWriteProperty 失败(行281 假腿): 值不回填仍透传
 * @tc.type: FUNC
 */
HWTEST_F(SsapClientFrameworkTest, SsapClientFrameworkOnWritePropertyFail002, TestSize.Level1)
{
    clientCallback_->OnWriteProperty(static_cast<int32_t>(SsapStatus::SSAP_FAILURE), MakePropertyParcel());
    EXPECT_EQ(1, fakeCallback_->propWriteCount_);
    EXPECT_EQ(nullptr, fakeCallback_->lastPropWriteValue_);
}

/**
 * @tc.name: SsapClientFrameworkOnSetPropertyNotify001
 * @tc.desc: 回调桩 OnSetPropertyNotification/Indication: enable 与 ret 原样透传
 * @tc.type: FUNC
 */
HWTEST_F(SsapClientFrameworkTest, SsapClientFrameworkOnSetPropertyNotify001, TestSize.Level1)
{
    clientCallback_->OnSetPropertyNotification(static_cast<int32_t>(SsapStatus::SSAP_SUCCESS), true,
        MakePropertyParcel());
    EXPECT_EQ(1, fakeCallback_->notifyResultCount_);
    EXPECT_EQ(1, fakeCallback_->lastNotifyEnable_);
    clientCallback_->OnSetPropertyIndication(static_cast<int32_t>(SsapStatus::SSAP_FAILURE), false,
        MakePropertyParcel());
    EXPECT_EQ(1, fakeCallback_->indicateResultCount_);
    EXPECT_EQ(0, fakeCallback_->lastIndicateEnable_);
    EXPECT_EQ(static_cast<int>(SsapStatus::SSAP_FAILURE), fakeCallback_->lastIndicateRet_);
}

/**
 * @tc.name: SsapClientFrameworkOnReadDescriptorHit001
 * @tc.desc: 回调桩 OnReadDescriptor 成功+句柄命中(行322/328 真腿): serviceUuid/propertyUuid/值透传
 * @tc.type: FUNC
 */
HWTEST_F(SsapClientFrameworkTest, SsapClientFrameworkOnReadDescriptorHit001, TestSize.Level1)
{
    CacheTestService();
    NearlinkSsapDescriptorParcel desc;
    desc.handle_ = PROP_HANDLE; // OnReadDescriptor 以属性句柄在缓存中定位
    desc.type_ = 1;
    desc.permission_ = 0;
    desc.value_ = std::vector<uint8_t>(TEST_VALUE, TEST_VALUE + TEST_VALUE_LEN);
    clientCallback_->OnReadDescriptor(static_cast<int32_t>(SsapStatus::SSAP_SUCCESS), desc);
    EXPECT_EQ(1, fakeCallback_->descReadCount_);
    EXPECT_EQ(static_cast<int>(SsapStatus::SSAP_SUCCESS), fakeCallback_->lastDescReadRet_);
    EXPECT_EQ(PROP_HANDLE, fakeCallback_->lastDescReadHandle_);
    EXPECT_TRUE(fakeCallback_->lastDescReadServiceUuid_.Equals(serviceUuid_));
    EXPECT_TRUE(fakeCallback_->lastDescReadPropUuid_.Equals(propertyUuid_));
    EXPECT_EQ(TEST_VALUE_LEN, fakeCallback_->lastDescReadValueSize_);
}

/**
 * @tc.name: SsapClientFrameworkOnReadDescriptorFail002
 * @tc.desc: 回调桩 OnReadDescriptor 失败(行322 假腿): 值不回填仍透传
 * @tc.type: FUNC
 */
HWTEST_F(SsapClientFrameworkTest, SsapClientFrameworkOnReadDescriptorFail002, TestSize.Level1)
{
    CacheTestService();
    NearlinkSsapDescriptorParcel desc;
    desc.handle_ = PROP_HANDLE;
    desc.type_ = 1;
    desc.value_ = std::vector<uint8_t>(TEST_VALUE, TEST_VALUE + TEST_VALUE_LEN);
    clientCallback_->OnReadDescriptor(static_cast<int32_t>(SsapStatus::SSAP_FAILURE), desc);
    EXPECT_EQ(1, fakeCallback_->descReadCount_);
    EXPECT_EQ(nullptr, fakeCallback_->lastDescReadValue_);
}

/**
 * @tc.name: SsapClientFrameworkOnReadDescriptorMiss003
 * @tc.desc: 回调桩 OnReadDescriptor 句柄未命中(行336): 仅告警仍透传, uuid 留空
 * @tc.type: FUNC
 */
HWTEST_F(SsapClientFrameworkTest, SsapClientFrameworkOnReadDescriptorMiss003, TestSize.Level1)
{
    CacheTestService();
    NearlinkSsapDescriptorParcel desc;
    desc.handle_ = 0x9999; // 缓存中不存在的句柄
    desc.type_ = 1;
    clientCallback_->OnReadDescriptor(static_cast<int32_t>(SsapStatus::SSAP_SUCCESS), desc);
    EXPECT_EQ(1, fakeCallback_->descReadCount_);
    EXPECT_FALSE(fakeCallback_->lastDescReadServiceUuid_.Equals(serviceUuid_));
}

/**
 * @tc.name: SsapClientFrameworkOnWriteDescriptor001
 * @tc.desc: 回调桩 OnWriteDescriptor: 句柄/类型/ret 原样透传
 * @tc.type: FUNC
 */
HWTEST_F(SsapClientFrameworkTest, SsapClientFrameworkOnWriteDescriptor001, TestSize.Level1)
{
    NearlinkSsapDescriptorParcel desc;
    desc.handle_ = 0x0042;
    desc.type_ = 2;
    clientCallback_->OnWriteDescriptor(static_cast<int32_t>(SsapStatus::SSAP_SUCCESS), desc);
    EXPECT_EQ(1, fakeCallback_->descWriteCount_);
    EXPECT_EQ(0x0042, fakeCallback_->lastDescWriteHandle_);
    EXPECT_EQ(static_cast<int>(SsapStatus::SSAP_SUCCESS), fakeCallback_->lastDescWriteRet_);
}

/**
 * @tc.name: SsapClientFrameworkOnMtuAndConnParam001
 * @tc.desc: 回调桩 OnMtuChanged/OnConnectionParameterChanged: 参数原样透传
 * @tc.type: FUNC
 */
HWTEST_F(SsapClientFrameworkTest, SsapClientFrameworkOnMtuAndConnParam001, TestSize.Level1)
{
    constexpr int32_t state = 3;
    constexpr uint16_t mtu = 512;
    clientCallback_->OnMtuChanged(state, mtu);
    EXPECT_EQ(1, fakeCallback_->mtuCount_);
    EXPECT_EQ(mtu, fakeCallback_->lastMtu_);
    EXPECT_EQ(state, fakeCallback_->lastMtuRet_);

    constexpr int32_t interval = 24;
    constexpr int32_t latency = 4;
    constexpr int32_t timeout = 300;
    constexpr int32_t status = 0;
    clientCallback_->OnConnectionParameterChanged(interval, latency, timeout, status);
    EXPECT_EQ(1, fakeCallback_->connParamCount_);
    EXPECT_EQ(interval, fakeCallback_->lastInterval_);
    EXPECT_EQ(latency, fakeCallback_->lastLatency_);
    EXPECT_EQ(timeout, fakeCallback_->lastTimeout_);
    EXPECT_EQ(status, fakeCallback_->lastConnParamStatus_);
}

/**
 * @tc.name: SsapClientFrameworkOnServicesDiscovered001
 * @tc.desc: 回调桩 OnServicesDiscovered: isFinding 置位时(行577 真腿)回调并复位, 未置位不回调
 * @tc.type: FUNC
 */
HWTEST_F(SsapClientFrameworkTest, SsapClientFrameworkOnServicesDiscovered001, TestSize.Level1)
{
    constexpr int32_t status = 4;
    clientCallback_->OnServicesDiscovered(status); // isFinding 为假: 不回调
    EXPECT_EQ(0, fakeCallback_->discoveredCount_);
    ssapClient_->pimpl->findInformation_.isFinding = true;
    clientCallback_->OnServicesDiscovered(status);
    EXPECT_EQ(1, fakeCallback_->discoveredCount_);
    EXPECT_EQ(status, fakeCallback_->lastDiscoveredStatus_);
    EXPECT_FALSE(ssapClient_->pimpl->findInformation_.isFinding);
}

/**
 * @tc.name: SsapClientFrameworkOnServicesDiscoveredByUuid001
 * @tc.desc: 回调桩 OnServicesDiscoveredByUuid: isFinding 置位时(行599 真腿)回调并复位
 * @tc.type: FUNC
 */
HWTEST_F(SsapClientFrameworkTest, SsapClientFrameworkOnServicesDiscoveredByUuid001, TestSize.Level1)
{
    constexpr int32_t status = 5;
    clientCallback_->OnServicesDiscoveredByUuid(status, ToParcelUuid(serviceUuid_));
    EXPECT_EQ(0, fakeCallback_->discoveredByUuidCount_);
    ssapClient_->pimpl->findInformation_.isFinding = true;
    clientCallback_->OnServicesDiscoveredByUuid(status, ToParcelUuid(serviceUuid_));
    EXPECT_EQ(1, fakeCallback_->discoveredByUuidCount_);
    EXPECT_EQ(status, fakeCallback_->lastDiscoveredStatus_);
    EXPECT_TRUE(fakeCallback_->lastDiscoveredUuid_.Equals(serviceUuid_));
    EXPECT_FALSE(ssapClient_->pimpl->findInformation_.isFinding);
}

/**
 * @tc.name: SsapClientFrameworkOnServicesRediscovered001
 * @tc.desc: 回调桩 OnServicesRediscovered: parcel 列表构建为 SsapService 后整体透传(行414)
 * @tc.type: FUNC
 */
HWTEST_F(SsapClientFrameworkTest, SsapClientFrameworkOnServicesRediscovered001, TestSize.Level1)
{
    std::vector<NearlinkSsapServiceParcel> parcelVec;
    parcelVec.push_back(MakeServiceParcel());
    clientCallback_->OnServicesRediscovered(parcelVec);
    EXPECT_EQ(1, fakeCallback_->rediscoveredCount_);
    EXPECT_EQ(1U, fakeCallback_->lastRediscoveredSize_);
    EXPECT_TRUE(fakeCallback_->lastRediscoveredUuid_.Equals(serviceUuid_));
}

/**
 * @tc.name: SsapClientFrameworkOnServiceChanged001
 * @tc.desc: 回调桩 OnServiceChanged: 句柄与 uuid 转换后透传
 * @tc.type: FUNC
 */
HWTEST_F(SsapClientFrameworkTest, SsapClientFrameworkOnServiceChanged001, TestSize.Level1)
{
    constexpr uint16_t changedHandle = 0x0505;
    NearlinkUuidParcel uuidParcel(ToParcelUuid(serviceUuid_));
    clientCallback_->OnServiceChanged(changedHandle, uuidParcel);
    EXPECT_EQ(1, fakeCallback_->serviceChangedCount_);
    EXPECT_EQ(changedHandle, fakeCallback_->lastChangedHandle_);
    EXPECT_TRUE(fakeCallback_->lastChangedUuid_.Equals(serviceUuid_));
}

/**
 * @tc.name: SsapClientFrameworkCleanConnectionInfo001
 * @tc.desc: pimpl CleanConnectionInfo: isFinding 置位时以 SSAP_FAILURE 结束发现流程
 * @tc.type: FUNC
 */
HWTEST_F(SsapClientFrameworkTest, SsapClientFrameworkCleanConnectionInfo001, TestSize.Level1)
{
    ssapClient_->pimpl->findInformation_.isFinding = true;
    ssapClient_->pimpl->CleanConnectionInfo();
    EXPECT_EQ(1, fakeCallback_->discoveredCount_);
    EXPECT_EQ(static_cast<int>(SsapStatus::SSAP_FAILURE), fakeCallback_->lastDiscoveredStatus_);
    EXPECT_FALSE(ssapClient_->pimpl->findInformation_.isFinding);
}

/**
 * @tc.name: SsapClientFrameworkBuildServiceListInclude001
 * @tc.desc: BuildServiceList: includeServices 二阶段回链, 被包含服务按 startHandle_ 挂回
 * @tc.type: FUNC
 */
HWTEST_F(SsapClientFrameworkTest, SsapClientFrameworkBuildServiceListInclude001, TestSize.Level1)
{
    constexpr uint16_t includeHandle = 0x0202;
    UUID includeUuid = UUID::FromString(OTHER_UUID_STRING);
    NearlinkSsapServiceParcel included;
    included.handle_ = includeHandle;
    included.endHandle_ = includeHandle + 1;
    included.isPrimary_ = false;
    included.uuid_ = ToParcelUuid(includeUuid);

    NearlinkSsapServiceParcel main = MakeServiceParcel();
    // startHandle_ 指向被包含服务句柄, 与产品第二阶段 FindService(iSvc.startHandle_) 对齐
    Service includeLink(0, includeHandle, includeHandle + 1, false, ToParcelUuid(includeUuid));
    main.includeServices_.push_back(includeLink);

    std::vector<NearlinkSsapServiceParcel> parcelVec;
    parcelVec.push_back(included);
    parcelVec.push_back(main);
    ssapClient_->pimpl->BuildServiceList(parcelVec);

    std::shared_ptr<SsapService> mainSvc = ssapClient_->pimpl->FindService(SVC_HANDLE);
    ASSERT_NE(nullptr, mainSvc);
    EXPECT_FALSE(mainSvc->GetIncludedServices().empty());
    std::shared_ptr<SsapService> includedSvc = ssapClient_->pimpl->FindService(includeHandle);
    ASSERT_NE(nullptr, includedSvc);
    EXPECT_TRUE(includedSvc->GetUuid().Equals(includeUuid));
}

/**
 * @tc.name: SsapClientFrameworkBuildServiceListEmpty001
 * @tc.desc: BuildServiceList 空列表: 不崩溃, 缓存保持为空
 * @tc.type: FUNC
 */
HWTEST_F(SsapClientFrameworkTest, SsapClientFrameworkBuildServiceListEmpty001, TestSize.Level1)
{
    std::vector<NearlinkSsapServiceParcel> empty;
    ssapClient_->pimpl->BuildServiceList(empty);
    EXPECT_EQ(nullptr, ssapClient_->pimpl->FindService(SVC_HANDLE));
}

/**
 * @tc.name: SsapClientFrameworkFindService001
 * @tc.desc: pimpl FindService: 命中返回缓存服务, 未命中返回 nullptr
 * @tc.type: FUNC
 */
HWTEST_F(SsapClientFrameworkTest, SsapClientFrameworkFindService001, TestSize.Level1)
{
    CacheTestService();
    std::shared_ptr<SsapService> hit = ssapClient_->pimpl->FindService(SVC_HANDLE);
    ASSERT_NE(nullptr, hit);
    EXPECT_TRUE(hit->GetUuid().Equals(serviceUuid_));
    EXPECT_EQ(nullptr, ssapClient_->pimpl->FindService(0x9999));
}

/**
 * @tc.name: SsapClientFrameworkGetHandleHit001
 * @tc.desc: GetHandle 全命中(行931/935 真腿): 返回 true 且写出属性句柄
 * @tc.type: FUNC
 */
HWTEST_F(SsapClientFrameworkTest, SsapClientFrameworkGetHandleHit001, TestSize.Level1)
{
    CacheTestService();
    uint16_t handle = 0;
    EXPECT_TRUE(ssapClient_->GetHandle(serviceUuid_, propertyUuid_, handle));
    EXPECT_EQ(PROP_HANDLE, handle);
}

/**
 * @tc.name: SsapClientFrameworkGetHandleServiceMiss002
 * @tc.desc: GetHandle 服务 uuid 未命中(行931 真腿提前返回): 返回 false
 * @tc.type: FUNC
 */
HWTEST_F(SsapClientFrameworkTest, SsapClientFrameworkGetHandleServiceMiss002, TestSize.Level1)
{
    CacheTestService();
    uint16_t handle = 0;
    EXPECT_FALSE(ssapClient_->GetHandle(UUID::FromString(OTHER_UUID_STRING), propertyUuid_, handle));
}

/**
 * @tc.name: SsapClientFrameworkGetHandlePropertyMiss003
 * @tc.desc: GetHandle 服务命中但属性 uuid 未命中(行935 假腿): 返回 false
 * @tc.type: FUNC
 */
HWTEST_F(SsapClientFrameworkTest, SsapClientFrameworkGetHandlePropertyMiss003, TestSize.Level1)
{
    CacheTestService();
    uint16_t handle = 0;
    EXPECT_FALSE(ssapClient_->GetHandle(serviceUuid_, UUID::FromString(OTHER_UUID_STRING), handle));
}

/**
 * @tc.name: SsapClientFrameworkFindPropertyByUuid001
 * @tc.desc: FindPropertyByUuid(行1062): 命中写出句柄返回 true, 未命中返回 false
 * @tc.type: FUNC
 */
HWTEST_F(SsapClientFrameworkTest, SsapClientFrameworkFindPropertyByUuid001, TestSize.Level1)
{
    CacheTestService();
    SsapProperty hit(1, propertyUuid_, TEST_OP_IND, 0);
    uint16_t handle = 0;
    EXPECT_TRUE(ssapClient_->FindPropertyByUuid(hit, handle));
    EXPECT_EQ(PROP_HANDLE, handle);
    SsapProperty miss(1, UUID::FromString(OTHER_UUID_STRING), TEST_OP_IND, 0);
    EXPECT_FALSE(ssapClient_->FindPropertyByUuid(miss, handle));
}

/**
 * @tc.name: SsapClientFrameworkFindMethodByUuid001
 * @tc.desc: FindMethodByUuid(行1076): 命中写出句柄返回 true, 未命中返回 false
 * @tc.type: FUNC
 */
HWTEST_F(SsapClientFrameworkTest, SsapClientFrameworkFindMethodByUuid001, TestSize.Level1)
{
    CacheTestService();
    SsapMethod hit(0, 0, methodUuid_, 0);
    uint16_t handle = 0;
    EXPECT_TRUE(ssapClient_->FindMethodByUuid(hit, handle));
    EXPECT_EQ(MTHD_HANDLE, handle);
    SsapMethod miss(0, 0, UUID::FromString(OTHER_UUID_STRING), 0);
    EXPECT_FALSE(ssapClient_->FindMethodByUuid(miss, handle));
}

/**
 * @tc.name: SsapClientFrameworkWritePropertyNoValue001
 * @tc.desc: WriteProperty 无值入口(不经栈网关): 空值直接返回 NL_ERR_INTERNAL_ERROR
 * @tc.type: FUNC
 */
HWTEST_F(SsapClientFrameworkTest, SsapClientFrameworkWritePropertyNoValue001, TestSize.Level1)
{
    SsapProperty empty(1, propertyUuid_, TEST_OP_IND, 0);
    EXPECT_EQ(NL_ERR_INTERNAL_ERROR, ssapClient_->WriteProperty(empty));
}
} // namespace TEST
} // namespace Nearlink
} // namespace OHOS
