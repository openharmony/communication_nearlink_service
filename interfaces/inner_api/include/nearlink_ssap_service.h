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
 * @file nearlink_ssap_service.h
 *
 * @brief ssap service interface.
 *
 * @since 6
 *
 */

#ifndef NEARLINK_SSAPSERVICE_H
#define NEARLINK_SSAPSERVICE_H

#include "nearlink_ssap_property.h"
#include "nearlink_ssap_event.h"
#include "nearlink_ssap_method.h"
#include "nearlink_ssap_descriptor.h"

namespace OHOS {
namespace Nearlink {
/** A SSAP-based Service type.
 *  Define SSAP-based Service types.
 */
enum class SsapServiceType : int {
    VENDOR_PROMARY = 0x08,   /**< vendor primary service */
    VENDOR_SECONDARY = 0x09, /**< vendor secondary service */
};

/**
 * @brief Class for Ssap Service API.
 *
 * @since 6
 *
 */
class NEARLINK_API SsapService {
public:
    /**
     * @brief The function to delete constructor of SsapService.
     *
     * @since 6
     *
     */
    SsapService() = delete;
    /**
     * @brief A constructor of SsapService.
     *
     * @param uuid UUID of service, uuid is big endian order.
     * @param type Type of service.
     * @since 6
     *
     */
    SsapService(const UUID &uuid, const SsapServiceType type);
    /**
     * @brief A constructor of SsapService.
     *
     * @param uuid UUID of service, uuid is big endian order.
     * @param handle Handle of service.
     * @param endHandle EndHandle of service.
     * @param type Type of service.
     * @since 6
     *
     */
    SsapService(const UUID &uuid, uint16_t handle, uint16_t endHandle, const SsapServiceType type);

    /**
     * @brief The function to add property.
     *
     * @param property Property object to add.
     * @since 6
     *
     */
    void AddProperty(const SsapProperty &property);
    /**
     * @brief The function to add method.
     *
     * @param method Method object to add.
     * @since 6
     *
     */
    void AddMethod(const SsapMethod &method);
    /**
     * @brief The function to add event.
     *
     * @param event Event object to add.
     * @since 6
     *
     */
    void AddEvent(const SsapEvent &event);
    /**
     * @brief The function to add include service.
     *
     * @param entry Service object to add.
     * @since 6
     *
     */
    void AddService(std::shared_ptr<SsapService> service);
    /**
     * @brief The function to get all propertys.
     *
     * @return list of propertys.
     * @since 6
     *
     */
    std::vector<SsapProperty> &GetProperty();
    /**
     * @brief The function to get all methods.
     *
     * @return list of methods.
     * @since 6
     *
     */
    std::vector<SsapMethod> &GetMethod();
    /**
     * @brief The function to get all events.
     *
     * @return list of events.
     * @since 6
     *
     */
    std::vector<SsapEvent> &GetEvent();
    /**
     * @brief The function to get include services.
     *
     * @return list of include services.
     * @since 6
     *
     */
    const std::vector<std::shared_ptr<SsapService>> &GetIncludedServices();
    /**
     * @brief The function to get service's handle.
     *
     * @return handle.
     * @since 6
     *
     */
    uint16_t GetHandle() const;
    /**
     * @brief The function to add descriptor.
     *
     * @param descriptor Descriptor object to add.
     * @since 6
     *
     */
    void AddDescriptor(const SsapDescriptor &descriptor);

    /**
     * @brief The function to get descriptor by UUID.
     *
     * @param type Type of Ssap Descriptor.
     * @return descriptor or nullptr.
     * @since 6
     *
     */
    SsapDescriptor *GetDescriptor(int type);

    /**
     * @brief The function to get descriptors.
     *
     * @return list of descriptors.
     * @since 6
     *
     */
    std::vector<SsapDescriptor> &GetDescriptors();
    /**
     * @brief The function to get service's type.
     *
     * @return bool   primary or not.
     * @since 6
     *
     */
    bool IsPrimary() const;
    /**
     * @brief The function to get service's UUID.
     *
     * @return UUID, uuid is big endian order.
     * @since 6
     *
     */
    const UUID &GetUuid() const;

    SsapService(const SsapService &);
    SsapService &operator=(const SsapService &) = default;

    SsapService(SsapService &&);
    SsapService &operator=(SsapService &&) = default;

private:
    /**
     * @brief The handle of service.
     *
     * @since 6
     *
     */
    uint16_t handle_;
    /**
     * @brief The endHandle of service.
     *
     * @since 6
     *
     */
    uint16_t endHandle_;
    /**
     * @brief The type of service.
     *
     * @since 6
     *
     */
    SsapServiceType serviceType_;
    /**
     * @brief The list of current service's include services.
     *
     * @since 6
     *
     */
    std::vector<std::shared_ptr<SsapService>> includeServices_;
    /**
     * @brief The properties of service.
     *
     * @since 6
     *
     */
    std::vector<SsapProperty> properties_;
    /**
     * @brief The methods of service.
     *
     * @since 6
     *
     */
    std::vector<SsapMethod> methods_;
    /**
     * @brief The events of service.
     *
     * @since 6
     *
     */
    std::vector<SsapEvent> events_;
    /**
     * @brief The UUID of service, uuid is big endian order.
     *
     * @since 6
     *
     */
    UUID uuid_;
    /**
     * @brief The descriptors of entry.
     *
     * @since 6
     *
     */
    std::vector<SsapDescriptor> descriptors_;
};
} // namespace Nearlink
} // namespace OHOS
#endif  // NEARLINK_SSAPSERVICE_H
