/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

/**
 * @addtogroup Nearlink
 * @{
 *
 * @brief Defines a nearlink system that provides basic nearlink connection and profile functions,
 *        including SLE, SSAP, etc.
 *
 * @since 6
 *
 */

/**
 * @file nearlink_ssap_client.h
 *
 * @brief Nearlink ssap client interface.
 *
 * @since 6
 *
 */

#ifndef NEARLINK_SSAPCLIENT_H
#define NEARLINK_SSAPCLIENT_H

#include <memory>
#include "nearlink_def.h"
#include "nearlink_ssap_service.h"
#include "nearlink_remote_device.h"

namespace OHOS {
namespace Nearlink {

/**
 * @brief Class for SsapClientCallback functions.
 *
 * @since 6
 *
 */
class SsapClientCallback {
public:
    /**
     * @brief The function to OnConnectionStateChanged.
     *
     * @param connectionState callback of ssapClientCallback.
     * @param ret ret of SsapClientCallback.
     * @since 6
     *
     */
    virtual void OnConnectionStateChanged(int connectionState, int ret) = 0;

    /**
     * @brief The function to OnPropertyChanged.
     *
     * @param property SsapProperty object to changed.
     * @since 6
     *
     */
    virtual void OnPropertyChanged(const SsapProperty &property)
    {}

    /**
     * @brief The function to OnEventNotified.
     *
     * @param property SsapProperty object to changed.
     * @since 6
     *
     */
    virtual void OnEventNotified(const SsapEvent &event)
    {}

    /**
     * @brief The function to OnPropertyReadResult.
     *
     * @param property SsapProperty object.
     * @param ret ret of SsapClientCallback.
     * @since 6
     *
     */
    virtual void OnPropertyReadResult(const SsapProperty &property, int ret)
    {}

    /**
     * @brief The function to OnMethodCallResult.
     *
     * @param method SsapMethod object.
     * @param ret ret of SsapClientCallback.
     * @since 6
     *
     */
    virtual void OnMethodCallResult(const SsapMethod &method, int ret)
    {}

    /**
     * @brief The function to OnPropertiesReadResult.
     *
     * @param properties vector of SsapProperty object.
     * @param ret ret of SsapClientCallback.
     * @since 6
     *
     */
    virtual void OnPropertiesReadResult(const std::vector<SsapProperty> &properties, int ret)
    {}

    virtual void OnReadRemoteRssiValueResult(int rssi, int status)
    {}
    /**
     * @brief The function to OnPropertyWriteResult.
     *
     * @param property SsapProperty object.
     * @param ret ret of SsapClientCallback.
     * @since 6
     *
     */
    virtual void OnPropertyWriteResult(const SsapProperty &property, int ret)
    {}

    /**
     * @brief The function to OnDescriptorReadResult.
     *
     * @param descriptor descriptor object.
     * @param ret ret of SsapClientCallback.
     * @since 6
     *
     */
    virtual void OnDescriptorReadResult(const SsapDescriptor &descriptor, int ret)
    {}

    /**
     * @brief The function to OnDescriptorWriteResult.
     *
     * @param descriptor descriptor object.
     * @param ret ret of SsapClientCallback.
     * @since 6
     *
     */
    virtual void OnDescriptorWriteResult(const SsapDescriptor &descriptor, int ret)
    {}

    /**
     * @brief The function to OnMtuUpdate.
     *
     * @param mtu mtu to update.
     * @param ret ret of SsapClientCallback.
     * @since 6
     *
     */
    virtual void OnMtuUpdate(uint16_t mtu, int ret)
    {}

    /**
     * @brief The function to OnServicesDiscovered.
     *
     * @param status Status object.
     * @since 6
     *
     */
    virtual void OnServicesDiscovered(int status)
    {}

    /**
     * @brief The function to OnServicesDiscovered.
     *
     * @param status Status object.
     * @param uuid is big endian order.
     * @since 6
     *
     */
    virtual void OnServicesDiscoveredByUuid(int status, const UUID &uuid)
    {}

    /**
     * @brief The function to OnConnectionParameterChanged.
     *
     * @param interval interval object.
     * @param latency latency object.
     * @param timeout timeout object.
     * @param status status object.
     * @since 6
     *
     */
    virtual void OnConnectionParameterChanged(int interval, int latency, int timeout, int status)
    {}

    /**
     * @brief The function to OnServicesRediscovered.
     *
     * @param services vector of SsapService object.
     * @since 7.0
     *
     */
    virtual void OnServicesRediscovered(const std::vector<SsapService> &services)
    {}

    /**
     * @brief The function to OnServiceChanged.
     *
     * @param handle handle of changed service.
     * @param uuid uuid of changed service.
     * @since 7.0
     *
     */
    virtual void OnServiceChanged(uint16_t handle, const UUID &uuid)
    {}

    /**
     * @brief The function to OnSetPropertyNotifyResult.
     *
     * @param property SsapProperty object.
     * @param enable enable or disable SsapProperty Notify.
     * @param ret ret of SsapClientCallback.
     * @since 6
     *
     */
    virtual void OnSetPropertyNotifyResult(const SsapProperty &property, int enable, int ret)
    {}

    /**
     * @brief The function to OnSetPropertyIndicateResult.
     *
     * @param property SsapProperty object.
     * @param enable enable or disable SsapProperty Indicate.
     * @param ret ret of SsapClientCallback.
     * @since 6.1
     *
     */
    virtual void OnSetPropertyIndicateResult(const SsapProperty &property, int enable, int ret)
    {}

    /**
     * @brief A destructor of SsapClientCallback.
     *
     * @since 6
     *
     */
    virtual ~SsapClientCallback()
    {}
};

/**
 * @brief Class for SsapClient functions.
 *
 * @since 6
 *
 */
class NEARLINK_API SsapClient {
public:
    /**
     * @brief A constructor of SsapClient.
     *
     * @param device remote device.
     * @since 6
     *
     */
    static std::shared_ptr<SsapClient> CreateSsapClient(std::shared_ptr<NearlinkRemoteDevice> device);

    /**
     * @brief The function to Connect.
     *
     * @param callback callback of ssapClientCallback.
     * @return Returns the status code for this function called.
     * @since 6
     *
     */
    NlErrCode Connect(std::shared_ptr<SsapClientCallback> callback);

    /**
     * @brief The function to request connection priority.
     *
     * @param connPriority connPriority of SsapClient.
     * @return Returns the status code for this function called.
     */
    NlErrCode RequestConnectionPriority(int connPriority); // need add develop

    /**
     * @brief The function to request connection for set scan frame format Ind.
     *
     * @param scanFrameFormatInd scanFrameFormatInd of SsapClient.
     * @return Returns the status code for this function called.
     */
    NlErrCode RequestConnScanFrameFormatInd(int scanFrameFormatInd);

    /**
     * @brief The function to get device address.
     *
     * @return Returns the device address.
     */
    std::string GetDeviceAddr();

    /**
     * @brief The function to request fastest connection.
     *
     * @return Returns the status code for this function called.
     */
    NlErrCode RequestFastestConn(); // need add develop

    /**
     * @brief The function to disconnect.
     *
     * @return Returns the status code for this function called.
     */
    NlErrCode Disconnect();

    /**
     * @brief The function to close.
     *
     * @return Returns the status code for this function called.
     */
    NlErrCode Close();

    /**
     * @brief The function to discover services.
     *
     * @return Returns the status code for this function called.
     */
    NlErrCode FindStructure();

    /**
     * @brief The function to discover services.
     *
     * @param uuid uuid of SsapClient, uuid is big endian order.
     * @return Returns the status code for this function called.
     */
    NlErrCode FindStructureByUuid(const UUID &uuid);

    /**
     * @brief The function to get service.
     *
     * @param uuid uuid of ssap service, uuid is big endian order.
     * @return Returns the ssap service.
     */
    std::shared_ptr<SsapService> GetService(const UUID &uuid);

    /**
     * @brief The function to get service.
     *
     * @return Returns the status code for this function called.
     */
    NlErrCode GetService(std::vector<SsapService> &services);

    /**
     * @brief The function to get handle.
     *
     * @return Returns the result of this function called.
     */
    bool GetHandle(const UUID &serviceUuid, const UUID &propertyUuid, uint16_t &handle);

    /**
     * @brief The function to read property.
     *
     * @param property Property object.
     * @return Returns the status code for this function called.
     */
    NlErrCode ReadProperty(SsapProperty &property);

    /**
     * @brief The function to call method.
     *
     * @param method Method object.
     * @return Returns the status code for this function called.
     */
    NlErrCode CallMethod(SsapMethod &method);

    /**
     * @brief The function to write entry.
     *
     * @param entry entry object.
     * @param value entry value.
     * @return Returns the status code for this function called.
     */
    NlErrCode CallMethod(SsapMethod &method, std::vector<uint8_t> value);

    /**
     * @brief The function to read property by uuid.
     *
     * @param property Property object.
     * @return Returns the status code for this function called.
     */
    NlErrCode ReadPropertyByUuid(SsapProperty &property);

    /**
     * @brief The function to read descriptor.
     *
     * @param descriptor descriptor object.
     * @return Returns the status code for this function called.
     */
    NlErrCode ReadDescriptor(SsapDescriptor &descriptor);

    /**
     * @brief The function to RequestSleMtuSize.
     *
     * @param mtu mtu of SsapClient.
     * @return Returns the status code for this function called.
     */
    NlErrCode RequestSleMtuSize(int mtu);

    /**
     * @brief The function to SetNotifyProperty.
     *
     * @param entry property object.
     * @param enable enable of SsapClient.
     * @return Returns the status code for this function called.
     */
    NlErrCode SetNotifyProperty(SsapProperty &property, bool enable);

    /**
     * @brief The function to SetIndicateProperty.
     *
     * @param property property object.
     * @param enable enable of SsapClient.
     * @return Returns the status code for this function called.
     */
    NlErrCode SetIndicateProperty(SsapProperty &property, bool enable);

    /**
     * @brief The function to write property.
     *
     * @param property property object.
     * @return Returns the status code for this function called.
     */
    NlErrCode WriteProperty(SsapProperty &property);

    /**
     * @brief The function to write entry.
     *
     * @param entry entry object.
     * @param value entry value.
     * @return Returns the status code for this function called.
     */
    NlErrCode WriteProperty(SsapProperty &property, std::vector<uint8_t> value);

    /**
     * @brief The function to write entry.
     *
     * @param descriptor descriptor object.
     * @return Returns the status code for this function called.
     */
    NlErrCode WriteDescriptor(SsapDescriptor &descriptor);

    /**
     * @brief The function to SetNotifyEvent.
     *  It is a local operation.if you want server notify, need write descriptor.
     *
     * @param event SsapEvent object.
     * @param enable enable of SsapClient.
     * @return Returns the status code for this function called.
     */
    NlErrCode SetNotifyEvent(SsapEvent &event, bool enable);

    /**
     * @brief The function to SetIndicateEvent.
     *  It is a local operation.if you want server Indicate, need write descriptor.
     *
     * @param event SsapEvent object.
     * @param enable enable of SsapClient.
     * @return Returns the status code for this function called.
     */
    NlErrCode SetIndicateEvent(SsapEvent &event, bool enable);

    /**
     * @brief Read remote rssi value
     *
     * @param remoteRssiValue The remote rssi value.
     * @return Returns the status code for this function called.
     */
    NlErrCode ReadRemoteRssiValue(int &remoteRssiValue);

    ~SsapClient();

    NEARLINK_DISALLOW_COPY_AND_ASSIGN(SsapClient);

private:

    NlErrCode SetNotifyPropertyInner(SsapProperty &property, bool enable, uint8_t notifyOption);

    bool FindPropertyByUuid(SsapProperty &property, uint16_t &propertyHandle);

    bool FindMethodByUuid(SsapMethod &method, uint16_t &methodHandle);

    explicit SsapClient(std::shared_ptr<NearlinkRemoteDevice> device);
    NEARLINK_DECLARE_IMPL();

    // It has no specific meaning, only used to hide the constructor.
    struct Pattern {
        Pattern() {};
    };

public:
    // This constructor is not available, use CreateSsapClient interface to create objects.
    explicit SsapClient(Pattern, std::shared_ptr<NearlinkRemoteDevice> device) : SsapClient(device) {};
};
} // namespace Nearlink
} // namespace OHOS
#endif  // NEARLINK_SSAPCLIENT_H
