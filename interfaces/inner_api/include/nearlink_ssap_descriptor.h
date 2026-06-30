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
 * @file nearlink_ssap_descriptor.h
 *
 * @brief Nearlink ssap descriptor interface.
 *
 * @since 6
 *
 */

#ifndef NEARLINK_SSAP_DESCRIPTOR_H
#define NEARLINK_SSAP_DESCRIPTOR_H

#include <memory>
#include "nearlink_def.h"
#include "nearlink_types.h"
#include "nearlink_uuid.h"

namespace OHOS {
namespace Nearlink {
/**
 * @brief Class for SsapProperty functions.
 *
 * @since 6
 *
 */
class SsapProperty;
class SsapService;
/**
 * @brief SSAP-based Descriptor class
 * Descriptors describe the value or permit configuration of the server with respect to the entry.
 */
class NEARLINK_API SsapDescriptor {
public:
    /** A SSAP Descriptor type.
     *  Define SSAP Descriptor type.
     */
    enum PropertyDescriptorType {
        DESCRIPTOR_TYPE_PROPERTY = 0x01,
        DESCRIPTOR_TYPE_CLIENT_PROPERTY_CONFIG = 0x02,
        DESCRIPTOR_TYPE_SERVER_PROPERTY_CONFIG = 0x03,
        DESCRIPTOR_TYPE_PROPERTY_FORMAT = 0x04,
        DESCRIPTOR_TYPE_PROPERT_RFU = 0x05,
        DESCRIPTOR_TYPE_VENDOR = 0xFF,
    };

    /**
     * @brief The function to delete constructor of SsapDescriptor.
     *
     * @since 6
     *
     */
    SsapDescriptor() = delete;

    /**
     * @brief A constructor of SsapDescriptor.
     *
     * @param type type of Ssap Descriptor.
     * @param permission permission of Ssap Descriptor.
     * @since 6
     *
     */
    SsapDescriptor(int type, int permission);

    /**
     * @brief A constructor of SsapDescriptor.
     *
     * @param handle handle of Ssap Descriptor.
     * @param type type of Ssap Descriptor.
     * @param permission permission of Ssap Descriptor.
     * @since 6
     *
     */
    SsapDescriptor(uint16_t handle, int type, int permission);
    explicit SsapDescriptor(const SsapDescriptor &);
    SsapDescriptor &operator=(const SsapDescriptor &);
    SsapDescriptor(SsapDescriptor &&) = default;
    SsapDescriptor &operator=(SsapDescriptor &&) = default;

    /**
     * @brief The function to get handle.
     *
     * @return handle.
     * @since 6
     *
     */
    uint16_t GetHandle() const;

    /**
     * @brief The function to get descriptor type.
     *
     * @return DescriptorType.
     * @since 6
     *
     */
    int GetDescriptorType() const;

    /**
     * @brief The function to get value.
     *
     * @param size size of get value.
     * @return value.
     * @since 6
     *
     */
    const std::unique_ptr<uint8_t[]> &GetValue(size_t *size) const;

    /**
     * @brief The function to set value.
     *
     * @param values values of set value.
     * @param length length of set value.
     * @since 6
     *
     */
    void SetValue(const uint8_t *values, const size_t length);

    /**
     * @brief The function to get descriptor permission.
     *
     * @return SsapPermission.
     * @since 6
     *
     */
    int GetDescriptorPermission() const;
    /**
     * @brief The function to get property uuid, uuid is big endian order.
     *
     * @return UUID.
     * @since 6
     *
     */
    const UUID &GetPropertyUuid() const;
    /**
     * @brief The function to set property uuid.
     *
     * @param values values of set SsapService uuid.
     * @since 6
     *
     */
    void SetPropertyUuid(const UUID &uuid);

    /**
     * @brief The function to get service uuid, uuid is big endian order.
     *
     * @return UUID.
     * @since 6
     *
     */
    const UUID &GetServiceUuid() const;
    /**
     * @brief The function to set service uuid.
     *
     * @param values values of set SsapService uuid.
     * @since 6
     *
     */
    void SetServiceUuid(const UUID &uuid);

private:
    /**
     * @brief The handle of descriptor.
     *
     * @since 6
     *
     */
    uint16_t handle_;

    /**
     * @brief The type of descriptor.
     *
     * @since 6
     *
     */
    uint16_t descriptorType_;

    /**
     * @brief The value of descriptor.
     *
     * @since 6
     *
     */
    std::unique_ptr<uint8_t[]> value_;

    /**
     * @brief The length of descriptor.
     *
     * @since 6
     *
     */
    size_t length_;

    /**
     * @brief The value descriptor permissions of property.
     *
     * @since 6
     *
     */
    int descriptorPermissions_;
    /**
     * @brief The property uuid of descriptor.
     *
     * @since 6
     *
     */
    UUID propertyUuid_;
    /**
     * @brief The service uuid of descriptor.
     *
     * @since 6
     *
     */
    UUID serviceUuid_;
};
} // namespace Nearlink
} // namespace OHOS

#endif  // NEARLINK_SSAP_DESCRIPTOR_H
