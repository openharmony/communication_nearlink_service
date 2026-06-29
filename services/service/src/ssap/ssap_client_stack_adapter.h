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
#ifndef SSAP_CLIENT_STACK_ADAPTER_H
#define SSAP_CLIENT_STACK_ADAPTER_H

#include <list>
#include "nearlink_def.h"
#include "nearlink_types.h"
#include "raw_address.h"
#include "sle_uuid.h"
#include "ssap_def.h"
#include "ssap_data.h"
#include "nlstk_ssap_app_client.h"
#include "nlstk_ssap_app_link.h"

// Timeout interval for invoking the protocol stack request and command.
#define REQ_TIMEOUT 30000
#define CMD_TIMEOUT 15000

namespace OHOS {
namespace Nearlink {

class SsapClientStackCallback {
public:
    virtual ~SsapClientStackCallback() = default;

    virtual void OnMtuChanged(int appId, uint16_t mtu, int ret) {}
    virtual void OnDiscoverComplete(int appId, int ret) {}
    virtual void OnDiscoverByUuidComplete(int appId, const Uuid &uuid, int ret) {}
    virtual void OnReadProperty(int appId, Property &property, int ret) {}
    virtual void OnCallMethod(int appId, Method &method, int ret) {}
    virtual void OnReadDescriptor(int appId, Descriptor &descriptor, int ret) {}
    virtual void OnReadPropertiesByUuid(int appId, std::list<Property> &properties, int ret) {}
    virtual void OnWriteProperty(int appId, Property &property, int ret) {}
    virtual void OnWriteDescriptor(int appId, Descriptor &descriptor, int ret) {}
    virtual void OnGetPropertyNotification(int appId, const Property &property, bool enable, int ret) {}
    virtual void OnGetPropertyIndication(int appId, const Property &property, bool enable, int ret) {}
    virtual void OnSetPropertyNotification(int appId, const Property &property, bool enable, int ret) {}
    virtual void OnSetPropertyIndication(int appId, const Property &property, bool enable, int ret) {}
    virtual void OnPropertyChanged(int appId, const Property &property) {}
    virtual void OnEvent(int appId, const Event &event) {}
    virtual void OnServicesRediscovered(int appId, std::vector<Service> &services) {}
    virtual void OnServiceChanged(int appId, uint16_t handle, const Uuid &uuid) {}
    virtual void OnConnectionStateChanged(int appId, int state, int ret, int reason) {}
    virtual void OnDisable(void) {}
};

class SsapClientStackAdapter {
public:
    explicit SsapClientStackAdapter(SsapClientStackCallback &callback);
    ~SsapClientStackAdapter();

    int RegisterApplication(int &appId, const RawAddress &addr, SsapSecureType secReq);
    void DeregisterApplication(int appId);
    void ExchangeMtu(int appId, uint16_t mtu);
    void DiscoverServices(int appId);
    void DiscoverServicesByUuid(int appId, const Uuid &uuid, uint16_t shdl, uint16_t ehdl);
    std::list<Service> GetServices(int appId);
    std::list<Service> GetServicesByUuid(int appId, const Uuid &uuid);
    void ReadProperty(int appId, const Property &property);
    void CallMethod(int appId, Method &method, bool withoutRsp);
    void ReadDescriptor(int appId, const Descriptor &descriptor);
    void ReadPropertiesByUuid(int appId, const Uuid &uuid, uint16_t shdl, uint16_t ehdl);
    void WriteProperty(int appId, const Property &property, bool withoutRsp);
    void WriteDescriptor(int appId, Descriptor &descriptor, bool withoutRsp);
    void GetPropertyNotification(int appId, const Property &property);
    void SetPropertyNotification(int appId, const Property &property, bool enable);
    void GetPropertyIndication(int appId, const Property &property);
    void SetPropertyIndication(int appId, const Property &property, bool enable);
    void Connect(int appId, const RawAddress &addr, bool autoConnect);
    void Disconnect(int appId, const RawAddress &addr);
    void Disable(void);
private:
    static void OnConnectionStateChanged(int appId, uint8_t state, NLSTK_Errcode_E ret, int reason);
    static void OnMtuChanged(int appId, uint16_t mtu, NLSTK_Errcode_E ret);
    static void OnDiscoverComplete(int appId, NLSTK_Errcode_E ret);
    static void OnDiscoverByUuidComplete(int appId, NLSTK_SsapUuid_S *stackUuid, NLSTK_Errcode_E ret);
    static void OnReadProperty(int appId, NLSTK_SsapClientReadPropertyInfo_S *property,
        NLSTK_Errcode_E ret);
    static void OnCallMethod(int appId, NLSTK_SsapClientCallMethodResult_S *method,
        NLSTK_Errcode_E ret);
    static void OnReadDescriptor(int appId, NLSTK_SsapClientReadDescriptorInfo_S *descriptor,
        NLSTK_Errcode_E ret);
    static void OnReadPropertiesByUuid(int appId, NLSTK_SsapClientReadPropertyInfo_S *property,
        uint8_t propertyNum, NLSTK_Errcode_E ret);
    static void OnWriteProperty(int appId, NLSTK_SsapClientReadPropertyInfo_S *property,
        NLSTK_Errcode_E ret);
    static void OnWriteDescriptor(int32_t appId, NLSTK_SsapClientReadDescriptorInfo_S *descriptor, NLSTK_Errcode_E ret);
    static void OnGetPropertyNotification(int appId, NLSTK_SsapUuid_S *stackUuid,
        uint16_t handle, bool enable, NLSTK_Errcode_E ret);
    static void OnGetPropertyIndication(int appId, NLSTK_SsapUuid_S *stackUuid,
        uint16_t handle, bool enable, NLSTK_Errcode_E ret);
    static void OnSetPropertyNotification(int appId, NLSTK_SsapUuid_S *stackUuid,
        uint16_t handle, bool enable, NLSTK_Errcode_E ret);
    static void OnSetPropertyIndication(int appId, NLSTK_SsapUuid_S *stackUuid,
        uint16_t handle, bool enable, NLSTK_Errcode_E ret);
    static void OnPropertyChanged(int appId, NLSTK_SsapClientReadPropertyInfo_S *property);
    static void OnEvent(int appId, NLSTK_SsapClientEventInfo_S *event);
    static void OnServiceRediscover(int32_t appId, NLSTK_SsapServ_S *service, uint16_t serviceNum);
    static void OnServiceChange(int32_t appId, uint16_t handle, NLSTK_SsapUuid_S *uuid);
    static void OnDisable(void);

    void OnConnectionStateChangedTask(int appId, int state, int ret, int reason);
    void OnMtuChangedTask(int appId, uint16_t mtu, int ret);
    void OnDiscoverCompleteTask(int appId, int ret);
    void OnDiscoverByUuidCompleteTask(int appId, const Uuid &uuid, int ret);
    void OnReadPropertyTask(int appId, Property &property, int ret);
    void OnCallMethodTask(int appId, Method &method, int ret);
    void OnReadDescriptorTask(int appId, Descriptor &descriptor, int ret);
    void OnReadPropertiesByUuidTask(int appId, std::list<Property> &properties, int ret);
    void OnWritePropertyTask(int appId, Property &property, int ret);
    void OnWriteDescriptorTask(int appId, Descriptor &descriptor, int ret);
    void OnGetPropertyNotificationTask(int appId, const Property &property, bool enable, int ret);
    void OnGetPropertyIndicationTask(int appId, const Property &property, bool enable, int ret);
    void OnSetPropertyNotificationTask(int appId, const Property &property, bool enable, int ret);
    void OnSetPropertyIndicationTask(int appId, const Property &property, bool enable, int ret);
    void OnPropertyChangedTask(int appId, const Property &property);
    void OnEventTask(int appId, const Event &event);
    void OnServiceRediscoverTask(int appId, std::vector<Service> &services);
    void OnServiceChangeTask(int appId, uint16_t handle, const Uuid &uuid);
    void OnDisableTask(void);

    Property BuildPropertyByStackProperty(NLSTK_SsapClientReadPropertyInfo_S *stackProperty);
    Descriptor BuildDescriptorStructByStackDescriptor(NLSTK_SsapDtor_S *stackDescriptor);
    Property BuildPropertyStructByStackProperty(NLSTK_SsapPrty_S *stackProperty);
    Method BuildMethodStructByStackMethod(NLSTK_SsapPrty_S *stackMethod);
    Event BuildEventStructByStackEvent(NLSTK_SsapPrty_S *stackEvent);
    Service BuildServiceStructByStackService(NLSTK_SsapServ_S *stackService);

    NEARLINK_DECLARE_IMPL();
};


} // namespace Nearlink
} // namespace OHOS

#endif // SSAP_CLIENT_STACK_ADAPTER_H