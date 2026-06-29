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
 * @file nearlink_ssap_server.h
 *
 * @brief ssap server interface.
 *
 * @since 6
 *
 */

#ifndef NEARLINK_SSAP_SERVER_H
#define NEARLINK_SSAP_SERVER_H

#include <memory>
#include "nearlink_def.h"
#include "nearlink_ssap_service.h"
#include "nearlink_remote_device.h"

namespace OHOS {
namespace Nearlink {
/**
 * @brief Class for Ssap Server callback functions.
 *
 * @since 6
 *
 */
class SsapServerCallback {
public:
    /**
     * @brief The callback function to notify connection state update.
     *
     * @param device Remote device object.
     * @param state Connection state.
     * @since 6
     *
     */
    virtual void OnConnectionStateUpdate(const NearlinkRemoteDevice &device, int state, int reason) = 0;

    /**
     * @brief The callback function to notify service add.
     *
     * @param Service Added service object.
     * @param ret Result of service add.
     * @since 6
     *
     */
    virtual void OnServiceAdded(std::shared_ptr<SsapService> service, int ret)
    {}

    /**
     * @brief The callback function to notify entry read request.
     *
     * @param device Remote device object.
     * @param property SsapProperty object.
     * @param requestId Result of request.
     * @since 6
     *
     */
    virtual void OnPropertyReadRequest(
        const NearlinkRemoteDevice &device, const SsapProperty &property, int ret)
    {}

    /**
     * @brief The callback function to notify property write request without authorize.
     *
     * @param device Remote device object.
     * @param property SsapProperty object.
     * @param requestId Result of request.
     * @since 6
     *
     */
    virtual void OnPropertyWriteRequest(
        const NearlinkRemoteDevice &device, const SsapProperty &property, int ret)
    {}

    /**
     * @brief The callback function to notify descriptor read request without authorize.
     *
     * @param device Remote device object.
     * @param descriptor SsapDescriptor object.
     * @param requestId Result of request.
     * @since 6
     *
     */
    virtual void OnDescriptorReadRequest(const NearlinkRemoteDevice &device, const SsapDescriptor &descriptor, int ret)
    {}

    /**
     * @brief The callback function to notify descriptor write request without authorize.
     *
     * @param device Remote device object.
     * @param descriptor SsapDescriptor object.
     * @param requestId Result of request.
     * @since 6
     *
     */
    virtual void OnDescriptorWriteRequest(
        const NearlinkRemoteDevice &device, const SsapDescriptor &descriptor, int ret)
    {}

    /**
     * @brief The callback function to notify mtu update without authorize.
     *
     * @param device Remote device object.
     * @param mtu Current mtu.
     * @since 6
     *
     */
    virtual void OnMtuUpdate(const NearlinkRemoteDevice &device, int mtu)
    {}
    /**
     * @brief The callback function to notify property changed.
     *
     * @param device Remote device object.
     * @param uuid is big endian order.
     * @since 6
     *
     */
    virtual void OnNotifyPropertyChanged(
        const NearlinkRemoteDevice &device, const UUID &uuid, uint16_t handle, int result)
    {}
    /**
     * @brief The callback function to notify event changed.
     *
     * @param device Remote device object.
     * @param uuid is big endian order.
     * @since 6
     *
     */
    virtual void OnNotifyEventChanged(
        const NearlinkRemoteDevice &device, const UUID &uuid, uint16_t handle, int result)
    {}
    /**
     * @brief The callback function to notify connection parameter changed
     *
     * @param device Remote device object.
     * @param interval Interval object.
     * @param latency Latency object.
     * @param timeout Timeout object.
     * @param status Status object.
     * @since 6
     *
     */
    virtual void OnConnectionParameterChanged(
        const NearlinkRemoteDevice &device, int interval, int latency, int timeout, int status)
    {}

    /**
     * @brief A destructor of SsapServerCallback.
     *
     * @since 6
     *
     */
    virtual ~SsapServerCallback()
    {}
};

/**
 * @brief Class for Ssap Server API.
 *
 * @since 6
 *
 */
class NEARLINK_API SsapServer {
public:
    /**
     * @brief A constructor of SsapServerCallback.
     *
     * @param device SsapServerCallback callback object.
     * @since 6
     *
     */
    static std::shared_ptr<SsapServer> CreateSsapServer(std::shared_ptr<SsapServerCallback> callback);
    /**
     * @brief The function to add service.
     *
     * @param service Service object to add.
     * @return Returns the status code for this function called.
     */
    NlErrCode AddService(SsapService &service);
    /**
     * @brief The function to remove service.
     *
     * @param service Service object to remove.
     * @return Returns the status code for this function called.
     */
    NlErrCode RemoveSsapService(const SsapService &service);
    /**
     * @brief The function to clear all services.
     *
     * @return Returns the status code for this function called.
     */
    NlErrCode ClearServices();
    /**
     * @brief The function to clear all services.
     *
     * @return Returns the status code for this function called.
     */
    NlErrCode Close();
    /**
     * @brief The function to get service by UUID.
     *
     * @param uuid UUID of service, uuid is big endian order.
     * @param isPrimary Type of service.
     * @return Returns the shared_ptr of the service.
     */
    std::shared_ptr<SsapService> GetService(const UUID &uuid, bool isPrimary);
    /**
     * @brief The function to notify property change.
     *
     * @param device Remote device object.
     * @param property SsapProperty object.
     * @param confirm Confirm the change.
     * @return Returns the status code for this function called.
     */
    NlErrCode NotifyPropertyChanged(
        const NearlinkRemoteDevice &device, const SsapProperty &property, bool confirm);
    /**
     * @brief The function to notify event change.
     *
     * @param device Remote device object.
     * @param event SsapEvent object.
     * @param value SsapEvent value.
     * @param confirm Confirm the change.
     * @return Returns the status code for this function called.
     */
    NlErrCode NotifyEvent(const NearlinkRemoteDevice &device, const SsapEvent &event, std::vector<uint8_t> &value,
        bool confirm);

    /**
     * @brief The function to set property value.
     *
     * @param property the new property to be set.
     * @return Returns the status code for this function called.
     */
    NlErrCode SetPropertyValue(SsapProperty &property);

    /**
     * @brief The function to set descriptor value.
     *
     * @param property the new descriptor to be set.
     * @return Returns the status code for this function called.
     */
    NlErrCode SetDescriptorValue(SsapDescriptor &descriptor);

    /**
     * @brief The function to send auth responce.
     *
     * @param requestId Id of the request.
     * @param allow whether the auth request is allowed.
     * @return Returns the status code for this function called.
     */
    NlErrCode AuthorizeResponse(uint16_t requestId, bool allow);

    /**
     * @brief The function to Initiate connection.
     *
     * @param device Remote device object.
     * @param secureReq whether security connection is required.
     * @param autoConnect autoConnect or not.
     * @return Returns the status code for this function called.
     */
    NlErrCode Connect(const NearlinkRemoteDevice &device, uint8_t secureReq, bool autoConnect);

    /**
     * @brief The function to cancel connection.
     *
     * @param device Remote device object.
     * @return Returns the status code for this function called.
     */
    NlErrCode CancelConnection(const NearlinkRemoteDevice &device);

    /**
     * @brief A destructor of SsapServer.
     *
     * @since 6
     *
     */
    ~SsapServer();

    NEARLINK_DISALLOW_COPY_AND_ASSIGN(SsapServer);

private:
    bool FindPropertyByUuid(const SsapProperty &property, uint16_t &propertyHandle);

    explicit SsapServer(std::shared_ptr<SsapServerCallback> callback);
    NEARLINK_DECLARE_IMPL();

    // It has no specific meaning, only used to hide the constructor.
    struct Pattern {
        Pattern() {};
    };

public:
    // This constructor is not available, use CreateSsapServer interface to create objects.
    explicit SsapServer(Pattern, std::shared_ptr<SsapServerCallback> callback) : SsapServer(callback) {};
};
} // namespace Nearlink
} // namespace OHOS
#endif  // NEARLINK_SSAP_SERVER_H
