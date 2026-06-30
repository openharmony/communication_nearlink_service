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
#ifndef INTERFACE_SSAP_SERVER_H
#define INTERFACE_SSAP_SERVER_H

#include <cstdint>
#include <list>
#include "ssap_data.h"
#include "SleInterfaceProfile.h"

namespace OHOS {
namespace Nearlink {

class InterfaceSsapServerCallback {
public:
    virtual ~InterfaceSsapServerCallback() = default;
    virtual void OnMtuChanged(const RawAddress &addr, uint8_t transport, uint16_t mtu) {}
    virtual void OnAddService(Service &service, int ret) {}
    virtual void OnSetPropertyValue(Property &property, int ret) {}
    virtual void OnSetDescriptorValue(Descriptor &descriptor, int ret) {}
    virtual void OnReadPropertyAuthorizeRequest(
        const RawAddress &addr, uint8_t transport, uint16_t requestId, Property &property) {}
    virtual void OnReadDescriptorAuthorizeRequest(
        const RawAddress &addr, uint8_t transport, uint16_t requestId, Descriptor &descriptor) {}
    virtual void OnWritePropertyAuthorizeRequest(
        const RawAddress &addr, uint8_t transport, uint16_t requestId, Property &property) {}
    virtual void OnWriteDescriptorAuthorizeRequest(
        const RawAddress &addr, uint8_t transport, uint16_t requestId, Descriptor &descriptor) {}
    virtual void OnReadProperty(const RawAddress &addr, uint8_t transport, Property &property, int ret) {}
    virtual void OnReadDescriptor(const RawAddress &addr, uint8_t transport, Descriptor &descriptor, int ret) {}
    virtual void OnWriteProperty(const RawAddress &addr, uint8_t transport, Property &property, int ret) {}
    virtual void OnWriteDescriptor(const RawAddress &addr, uint8_t transport, Descriptor &descriptor, int ret) {}
    virtual void OnNotifyProperty(const RawAddress &addr, uint8_t transport, Property &property, int ret) {}
    virtual void OnIndicateProperty(const RawAddress &addr, uint8_t transport, Property &property, int ret) {}
    virtual void OnNotifyEvent(const RawAddress &addr, uint8_t transport, Event &event, int ret) {}
    virtual void OnIndicateEvent(const RawAddress &addr, uint8_t transport, Event &event, int ret) {}
    virtual void OnCallMethod(
        const RawAddress &addr, uint8_t transport, Method &method, std::vector<uint8_t> &value, bool needReturn) {}
    virtual void OnConnectionStateChanged(const RawAddress &addr, uint8_t transport, uint8_t state,
        int ret, int reason) {}
};

class InterfaceProfileSsapServer : public SleInterfaceProfile {
public:
    virtual ~InterfaceProfileSsapServer() {}
    virtual int RegisterApplication(const std::shared_ptr<InterfaceSsapServerCallback> &callback) = 0;
    virtual int DeregisterApplication(int appId) = 0;
    virtual int SetMtu(int appId, uint16_t mtu) = 0;
    virtual int AddService(int appId, const Service &service) = 0;
    virtual int RemoveService(int appId, const Service &service) = 0;
    virtual int ClearServices(int appId) = 0;
    virtual bool CheckServiceExistByUuid(int appId, const Uuid &uuid) = 0;
    virtual int SetPropertyValue(int appId, Property &property) = 0;
    virtual int SetDescriptorValue(int appId, Descriptor &descriptor) = 0;
    virtual std::vector<uint8_t> GetPropertyValue(int appId, const Property &property) = 0;
    virtual std::vector<uint8_t> GetDescriptorValue(int appId, const Descriptor &descriptor) = 0;
    virtual int AuthorizeResponse(int appId, uint16_t requestId, bool allow) = 0;
    virtual int NotifyProperty(int appId, Property &property, const RawAddress &addr, uint8_t transport) = 0;
    virtual int IndicateProperty(int appId, Property &property, const RawAddress &addr, uint8_t transport) = 0;
    virtual int NotifyEvent(
        int appId, const Event &event, std::vector<uint8_t> &value, const RawAddress &addr, uint8_t transport) = 0;
    virtual int IndicateEvent(
        int appId, const Event &event, std::vector<uint8_t> &value, const RawAddress &addr, uint8_t transport) = 0;
    virtual int ReturnMethod(
        int appId, const Method &method, std::vector<uint8_t> &value, const RawAddress &addr, uint8_t transport) = 0;
    virtual int Connect(int appId, const RawAddress &addr, uint8_t transport) = 0;
    virtual int Disconnect(int appId, const RawAddress &addr, uint8_t transport) = 0;
private:
    using SleInterfaceProfile::Connect;
    using SleInterfaceProfile::Disconnect;
};

} // namespace Nearlink
} // namespace OHOS
#endif // INTERFACE_SSAP_SERVER_H