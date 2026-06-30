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
#ifndef SSAP_SERVER_SERVICE_H
#define SSAP_SERVER_SERVICE_H

#include "context.h"
#include "interface_profile_ssap_server.h"
#include "nearlink_types.h"

namespace OHOS {
namespace Nearlink {

class SsapServerService : public InterfaceProfileSsapServer, public utility::Context {
public:
    SsapServerService();
    ~SsapServerService();

    utility::Context *GetContext() override;

    void Enable() override;
    void Disable() override;

    int RegisterApplication(const std::shared_ptr<InterfaceSsapServerCallback> &callback) override;
    int DeregisterApplication(int appId) override;

    int SetMtu(int appId, uint16_t mtu) override;

    int AddService(int appId, const Service &service) override;
    int RemoveService(int appId, const Service &service) override;
    int ClearServices(int appId) override;
    bool CheckServiceExistByUuid(int appId, const Uuid &uuid) override;

    int SetPropertyValue(int appId, Property &property) override;
    int SetDescriptorValue(int appId, Descriptor &descriptor) override;

    std::vector<uint8_t> GetPropertyValue(int appId, const Property &property) override;
    std::vector<uint8_t> GetDescriptorValue(int appId, const Descriptor &descriptor) override;

    int AuthorizeResponse(int appId, uint16_t requestId, bool allow) override;

    int NotifyProperty(int appId, Property &property, const RawAddress &addr, uint8_t transport) override;
    int IndicateProperty(int appId, Property &property, const RawAddress &addr, uint8_t transport) override;
    int NotifyEvent(int appId, const Event &event,
        std::vector<uint8_t> &value, const RawAddress &addr, uint8_t transport) override;
    int IndicateEvent(int appId, const Event &event,
        std::vector<uint8_t> &value, const RawAddress &addr, uint8_t transport) override;
    int ReturnMethod(int appId, const Method &method,
        std::vector<uint8_t> &value, const RawAddress &addr, uint8_t transport) override;
    int Connect(int appId, const RawAddress &addr, uint8_t transport) override;
    int Disconnect(int appId, const RawAddress &addr, uint8_t transport) override;
private:

    void ClearApplication();
    bool IsValidAppId(int appId);
    void EnableTask();
    void DisableTask();
    int RegisterApplicationTask(
        const std::shared_ptr<InterfaceSsapServerCallback> &callback, int32_t pid, int32_t uid);
    int DeregisterApplicationTask(int appId);
    void SetMtuTask(int appId, uint16_t mtu);
    void AddServiceTask(int appId, Service &service);
    void RemoveServiceTask(int appId, uint16_t handle);
    void ClearServicesTask(int appId);
    bool CheckServiceExistByUuidTask(int32_t appId, const Uuid &uuid);
    void SetPropertyValueTask(int appId, Property &property);
    void SetDescriptorValueTask(int appId, Descriptor &descriptor);
    void AuthorizeResponseTask(int appId, uint16_t requestId, bool allow);
    void NotifyPropertyTask(int appId, Property &property, const RawAddress &addr);

    int Connect(const RawAddress &device) override;
    int Disconnect(const RawAddress &device) override;
    std::list<RawAddress> GetConnectDevices() override;
    int GetConnectState() override;

    NEARLINK_DECLARE_IMPL();
};


} // namespace Nearlink
} // namespace OHOS

#endif // SSAP_SERVER_SERVICE_H