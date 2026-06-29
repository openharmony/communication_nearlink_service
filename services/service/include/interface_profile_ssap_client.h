/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#ifndef INTERFACE_PROFILE_SSAP_CLIENT_H
#define INTERFACE_PROFILE_SSAP_CLIENT_H

#include <cstdint>
#include <list>
#include "SleInterfaceProfile.h"
#include "ssap_data.h"

namespace OHOS {
namespace Nearlink {

class InterfaceSsapClientCallback {
public:
    virtual ~InterfaceSsapClientCallback() = default;
    virtual void OnMtuChanged(uint16_t mtu, int ret) {}
    virtual void OnDiscoverComplete(int ret) {}
    virtual void OnDiscoverByUuidComplete(const Uuid &uuid, int ret) {}
    virtual void OnServicesRediscovered(std::vector<Service> &services) {}
    virtual void OnServiceChanged(uint16_t handle, const Uuid &uuid) {}
    virtual void OnReadProperty(Property &property, int ret) {}
    virtual void OnCallMethod(Method &method, int ret) {}
    virtual void OnReadDescriptor(Descriptor &descriptor, int ret) {}
    virtual void OnReadPropertiesByUuid(std::list<Property> &list, int ret) {}
    virtual void OnReadDescriptorsByUuid(std::list<Descriptor> &list, int ret) {}
    virtual void OnWriteProperty(Property &property, int ret) {}
    virtual void OnWriteDescriptor(Descriptor &descriptor, int ret) {}
    virtual void OnGetPropertyNotification(const Property &property, bool enable, int ret) {}
    virtual void OnGetPropertyIndication(const Property &property, bool enable, int ret) {}
    virtual void OnSetPropertyNotification(const Property &property, bool enable, int ret) {}
    virtual void OnSetEventNotification(const Event &event, bool enable, int ret) {}
    virtual void OnSetPropertyIndication(const Property &property, bool enable, int ret) {}
    virtual void OnSetEventIndication(const Event &event, bool enable, int ret) {}
    virtual void OnPropertyChanged(const Property &property) {}
    virtual void OnEvent(const Event &event) {}
    virtual void OnConnectionStateChanged(uint8_t state, int ret) {}
};

class InterfaceProfileSsapClient : public SleInterfaceProfile {
public:
    virtual ~InterfaceProfileSsapClient() = default;
    virtual int RegisterApplication(const std::shared_ptr<InterfaceSsapClientCallback> &callback,
                                    const RawAddress &addr, uint8_t transport, uint8_t secureReq) = 0;
    virtual int DeregisterApplication(int appId) = 0;
    virtual int ExchangeMtu(int appId, uint16_t mtu) = 0;
    virtual int DiscoverServices(int appId) = 0;
    virtual int DiscoverServicesByUuid(int appId, const Uuid &uuid, uint16_t startHandle, uint16_t endHandle) = 0;
    virtual std::list<Service> GetServices(int appId) = 0;
    virtual std::list<Service> GetServicesByUuid(int appId, const Uuid &uuid) = 0;
    virtual int ReadProperty(int appId, const Property &property) = 0;
    virtual int ReadDescriptor(int appId, const Descriptor &descriptor) = 0;
    virtual int ReadPropertiesByUuid(int appId, const Uuid &uuid, uint16_t startHandle, uint16_t endHandle) = 0;
    virtual int ReadDescriptorsByUuid(
        int appId, const Uuid &uuid, uint8_t type, uint16_t startHandle, uint16_t endHandle) = 0;
    virtual int WriteProperty(int appId, Property &property, bool withoutRsp) = 0;
    virtual int WriteDescriptor(int appId, Descriptor &descriptor, bool withoutRsp) = 0;
    virtual int GetPropertyNotification(int appId, const Property &property) = 0;
    virtual int SetPropertyNotification(int appId, const Property &property, bool enable) = 0;
    virtual int SetEventNotification(int appId, const Event &event, bool enable) = 0;
    virtual int SetPropertyIndication(int appId, const Property &property, bool enable) = 0;
    virtual int SetEventIndication(int appId, const Event &event, bool enable) = 0;
    virtual int CallMethod(int appId, Method &method, bool withoutRsp) = 0;
    virtual int Connect(int appId, bool autoConnect) = 0;
    virtual int Disconnect(int appId) = 0;
private:
    using SleInterfaceProfile::Connect;
    using SleInterfaceProfile::Disconnect;
};

} // namespace Nearlink
} // namespace OHOS
#endif // INTERFACE_PROFILE_SSAP_CLIENT_H