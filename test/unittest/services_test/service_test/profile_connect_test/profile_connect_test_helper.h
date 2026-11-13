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

#ifndef PROFILE_CONNECT_TEST_HELPER_H
#define PROFILE_CONNECT_TEST_HELPER_H

#include <list>
#include <string>
#include <utility>
#include <vector>

#include "IAdapterStateObserver.h"
#include "SleInterfaceManager.h"
#include "SleInterfaceProfile.h"
#include "context.h"
#include "i_verification_strategy.h"
#include "interface_profile_ssap_client.h"
#include "nearlink_def.h"
#include "raw_address.h"
#include "sle_uuid.h"
#include "ssap_data.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {

// 全部为合成地址，不与真实设备冲突
const std::string kAddrL = "A1:B2:C3:D4:E5:01";
const std::string kAddrR = "A1:B2:C3:D4:E5:02";
const std::string kAddrM = "B2:C3:D4:E5:F6:03";
const std::string kAddrU = "CC:DD:EE:FF:00:99";
const std::string kBtAddr = "00:11:22:33:44:55";

class TestProfileContext : public utility::Context {
public:
    TestProfileContext() : utility::Context("TestProfile", "1.0.0") {}

    void Enable() override { enableCount++; }
    void Disable() override { disableCount++; }

    int enableCount = 0;
    int disableCount = 0;
};

// 通用业务假件：连接状态掩码可配，连接/断连调用被统计
class FakeProfile : public SleInterfaceProfile {
public:
    explicit FakeProfile(uint8_t stateMask = 0) : stateMask_(stateMask) {}

    int Connect(const RawAddress &device) override
    {
        connectCount++;
        return 0;
    }

    int Disconnect(const RawAddress &device) override
    {
        disconnectCount++;
        return 0;
    }

    std::list<RawAddress> GetConnectDevices() override { return {}; }

    int GetConnectState() override
    {
        return stateMask_;
    }

    utility::Context *GetContext() override
    {
        return &context_;
    }

    TestProfileContext context_;
    int connectCount = 0;
    int disconnectCount = 0;
    uint8_t stateMask_ = 0;
};

// SSAP 客户端假件：注册/连接/发现返回值可配，发现结果服务列表可注入
class FakeSsapClientService : public InterfaceProfileSsapClient {
public:
    int Connect(const RawAddress &device) override { return 0; }
    int Disconnect(const RawAddress &device) override { return 0; }
    std::list<RawAddress> GetConnectDevices() override { return {}; }
    int GetConnectState() override { return 0; }
    utility::Context *GetContext() override { return &context_; }

    int RegisterApplication(const std::shared_ptr<InterfaceSsapClientCallback> &callback, const RawAddress &addr,
        uint8_t transport, uint8_t secureReq) override
    {
        registerCount++;
        return registerAppId;
    }

    int DeregisterApplication(int appId) override
    {
        deregisterCount++;
        return static_cast<int>(SsapStatus::SSAP_SUCCESS);
    }

    int ExchangeMtu(int appId, uint16_t mtu) override { return 0; }

    int DiscoverServices(int appId) override
    {
        discoverCount++;
        return discoverRet;
    }

    int DiscoverServicesByUuid(int appId, const Uuid &uuid, uint16_t startHandle, uint16_t endHandle) override { return 0; }

    std::list<Service> GetServices(int appId) override
    {
        return services;
    }

    std::list<Service> GetServicesByUuid(int appId, const Uuid &uuid) override { return {}; }

    int ReadProperty(int appId, const Property &property) override { return 0; }

    int ReadDescriptor(int appId, const Descriptor &descriptor) override { return 0; }

    int ReadPropertiesByUuid(int appId, const Uuid &uuid, uint16_t startHandle, uint16_t endHandle) override { return 0; }

    int ReadDescriptorsByUuid(int appId, const Uuid &uuid, uint8_t type, uint16_t startHandle, uint16_t endHandle) override { return 0; }

    int WriteProperty(int appId, Property &property, bool withoutRsp) override { return 0; }

    int WriteDescriptor(int appId, Descriptor &descriptor, bool withoutRsp) override { return 0; }

    int GetPropertyNotification(int appId, const Property &property) override { return 0; }

    int SetPropertyNotification(int appId, const Property &property, bool enable) override { return 0; }

    int SetEventNotification(int appId, const Event &event, bool enable) override { return 0; }

    int SetPropertyIndication(int appId, const Property &property, bool enable) override { return 0; }

    int SetEventIndication(int appId, const Event &event, bool enable) override { return 0; }

    int CallMethod(int appId, Method &method, bool withoutRsp) override { return 0; }

    int Connect(int appId, bool autoConnect) override
    {
        connectCount++;
        return connectRet;
    }

    int Disconnect(int appId) override
    {
        disconnectCount++;
        return 0;
    }

    TestProfileContext context_;
    int registerAppId = 10;
    int connectRet = static_cast<int>(ReturnValue::RET_NO_ERROR);
    int discoverRet = static_cast<int>(ReturnValue::RET_NO_ERROR);
    std::list<Service> services;
    int registerCount = 0;
    int deregisterCount = 0;
    int connectCount = 0;
    int disconnectCount = 0;
    int discoverCount = 0;
};

// 适配器状态观察者假件：状态通知与关星闪回执被记录
class FakeAdapterStateObserver : public IAdapterStateObserver {
public:
    void OnStateChange(const SleTransport transport, const SleStateID state) override
    {
        stateChanges.push_back(state);
    }

    void OnDisableResponse(bool isHalfDisable, SwitchCallerInfo callerInfo) override
    {
        disableResponses.push_back({isHalfDisable, callerInfo});
    }

    std::vector<SleStateID> stateChanges;
    std::vector<std::pair<bool, SwitchCallerInfo>> disableResponses;
};

// 系统状态观察者假件
class FakeSystemStateObserver : public ISystemStateObserver {
public:
    void OnSystemStateChange(const SleSystemState state) override
    {
        changes.push_back(state);
    }

    std::vector<SleSystemState> changes;
};

// 开关控制验证策略桩：按剧本返回验证结论
class SwitchControlStubStrategy : public IVerificationStrategy {
public:
    explicit SwitchControlStubStrategy(bool result) : result_(result) {}

    bool Check(const VerificationContext &ctx) override { return result_; }
    VerificationType GetType() override { return VerificationType::SWITCH_CONTROL; }

    bool result_ = false;
};
}  // namespace TEST
}  // namespace Nearlink
}  // namespace OHOS
#endif  // PROFILE_CONNECT_TEST_HELPER_H
