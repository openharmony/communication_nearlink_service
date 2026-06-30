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
#ifndef SSAP_CLIENT_SERVICE_H
#define SSAP_CLIENT_SERVICE_H

#include "nearlink_types.h"
#include "interface_profile_ssap_client.h"
#include "context.h"

namespace OHOS {
namespace Nearlink {

class SsapClientService : public InterfaceProfileSsapClient, public utility::Context {
public:
    SsapClientService();
    ~SsapClientService();

    // SleInterfaceProfile interface
    utility::Context *GetContext() override;

    // utility::Context interface
    void Enable() override;
    void Disable() override;

    // InterfaceProfileSsapClient interface
    int RegisterApplication(const std::shared_ptr<InterfaceSsapClientCallback> &callback,
                            const RawAddress &addr, uint8_t transport, uint8_t secureReq) override;
    int DeregisterApplication(int appId) override;
    int ExchangeMtu(int appId, uint16_t mtu) override;
    int DiscoverServices(int appId) override;
    int DiscoverServicesByUuid(int appId, const Uuid &uuid, uint16_t startHandle, uint16_t endHandle) override;
    std::list<Service> GetServices(int appId) override;
    std::list<Service> GetServicesByUuid(int appId, const Uuid &uuid) override;
    int ReadProperty(int appId, const Property &property) override;
    int ReadDescriptor(int appId, const Descriptor &descriptor) override;
    int ReadPropertiesByUuid(int appId, const Uuid &uuid, uint16_t startHandle, uint16_t endHandle) override;
    int ReadDescriptorsByUuid(
        int appId, const Uuid &uuid, uint8_t type, uint16_t startHandle, uint16_t endHandle) override;
    int WriteProperty(int appId, Property &property, bool withoutRsp) override;
    int WriteDescriptor(int appId, Descriptor &descriptor, bool withoutRsp) override;
    int GetPropertyNotification(int appId, const Property &property) override;
    int SetPropertyNotification(int appId, const Property &property, bool enable) override;
    int SetEventNotification(int appId, const Event &event, bool enable) override;
    int SetPropertyIndication(int appId, const Property &property, bool enable) override;
    int SetEventIndication(int appId, const Event &event, bool enable) override;
    int CallMethod(int appId, Method &method, bool withoutRsp) override;
    int Connect(int appId, bool autoConnect) override;
    int Disconnect(int appId) override;
private:
    void ClearApplication();
    bool IsValidAppId(int appId);
    void EnableTask();
    void DisableTask();
    int RegisterApplicationTask(const std::shared_ptr<InterfaceSsapClientCallback> &callback,
        const RawAddress &addr, SsapSecureType secureReq, int32_t pid, int32_t uid);
    int DeregisterApplicationTask(int appId);
    void ExchangeMtuTask(int appId, uint16_t mtu);
    void DiscoverServicesTask(int appId);
    void DiscoverServicesByUuidTask(int appId, const Uuid &uuid, uint16_t startHandle, uint16_t endHandle);
    std::list<Service> GetServicesTask(int appId);
    std::list<Service> GetServicesByUuidTask(int appId, const Uuid &uuid);
    void ReadPropertyTask(int appId, const Property &property);
    void CallMethodTask(int appId, Method &method, bool withoutRsp);
    void ReadDescriptorTask(int appId, const Descriptor &descriptor);
    void ReadPropertiesByUuidTask(int appId, const Uuid &uuid, uint16_t startHandle, uint16_t endHandle);
    void WritePropertyTask(int appId, Property &property, bool withoutRsp);
    void WriteDescriptorTask(int appId, Descriptor &descriptor, bool withoutRsp);
    void GetPropertyNotificationTask(int appId, const Property &property);
    void SetPropertyNotificationTask(int appId, const Property &property, bool enable);
    void SetPropertyIndicationTask(int appId, const Property &property, bool enable);
    void ConnectTask(int appId, bool autoConnect);
    void DisconnectTask(int appId);
    int Connect(const RawAddress &device) override;
    int Disconnect(const RawAddress &device) override;
    std::list<RawAddress> GetConnectDevices() override;
    int GetConnectState() override;

    NEARLINK_DECLARE_IMPL();
};


} // namespace Nearlink
} // namespace OHOS

#endif // SSAP_CLIENT_SERVICE_H