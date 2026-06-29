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
 * @file nearlink_ssap_property.h
 *
 * @brief Nearlink ssap property interface.
 *
 * @since 6
 *
 */

#ifndef NEARLINK_SSAP_PROPERTY_H
#define NEARLINK_SSAP_PROPERTY_H

#include <cstddef>
#include <cstdint>
#include <list>
#include "memory"
#include "vector"
#include "nearlink_def.h"
#include "nearlink_ssap_descriptor.h"
#include "nearlink_uuid.h"

namespace OHOS {
namespace Nearlink {
/**
 * @brief Class for SsapService functions.
 *
 * @since 6
 *
 */
class SsapService;
/**
 * @brief SSAP-based Property class
 * A property is a value used in a service along with properties and configuration information about
 * how the value is accessed and information about how the value is displayed or represented.
 */
class NEARLINK_API SsapProperty {
public:
    /** A SSAP property write type.
     *  Define SSAP property write types.
     */
    enum PropertyWriteType {
        PROPERTY_WRITE_CMD = 0x00,
        PROPERTY_WRITE_REQ = 0x01,
        PROPERTY_WRITE_DEFAULT = 0x02
    };

    /** A SSAP property operation Indication.
     *  Define SSAP property operation Indication.
     */
    enum OperationIndication {
        OPERATION_READ = 0x01, /**< readable */
        OPERATION_WRITE_NO_RESPONSE = 0x02,
        OPERATION_WRITE_WITH_RESPONSE = 0x04,
        OPERATION_NOTIFY = 0x08,
        OPERATION_INDICATION = 0x10,
        OPERATION_BROADCAST = 0x20,
        OPERATION_WRITE_CLIENT_CONFIG = 0x200,
        OPERATION_WRITE_SERVER_CONFIG = 0x400,
    };

    /** A SSAP property type.
     *  Define SSAP property types.
     */
    enum PropertyType {
        ENTRY_TYPE_PROPERTY = 0x02,
        ENTRY_TYPE_VENDOR_PROPERTY = 0x0A,
    };

    /**
     * @brief The function to delete constructor of Ssapproperty.
     *
     * @since 6
     *
     */
    SsapProperty() = delete;

    /**
     * @brief A constructor of SsapProperty.
     *
     * @param type Type of Ssap Property.
     * @param uuid Uuid of Ssap Property, uuid is big endian order.
     * @param operIndicate operationIndication of Ssap Property.
     * @param valuePerm permissions of Ssap Property value, For details, see SsapPermission.
     * @since 6
     *
     */
    SsapProperty(const int type, const UUID uuid, uint32_t operIndicate, int valuePerm);

    /**
     * @brief A constructor of SsapEntry.
     *
     * @param handle handle of Ssap Property.
     * @param type Type of Ssap Property.
     * @param uuid Uuid of Ssap Property, uuid is big endian order.
     * @param operIndicate operationIndication of Ssap Property.
     * @param valuePerm permissions of Ssap Property value.
     * @since 6
     *
     */
    SsapProperty(uint16_t handle, const int type, const UUID uuid, uint32_t operIndicate, int valuePerm);
    SsapProperty(uint16_t handle, const UUID uuid, uint32_t operIndicate, int valuePerm);
    SsapProperty(const SsapProperty &);
    SsapProperty &operator=(const SsapProperty &);

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
     * @brief The function to get property type.
     *
     * @return PropertyType.
     * @since 6
     *
     */
    int GetPropertyType() const;

    /**
     * @brief The function to get handle.
     *
     * @return uint16_t  handle.
     * @since 6
     *
     */
    uint16_t GetHandle() const;

    /**
     * @brief The function to get operation Indication.
     *
     * @return operation Indication_.
     * @since 6
     *
     */
    uint32_t GetOperationIndication() const;

    /**
     * @brief The function to set new operationIndication.
     *
     * @return operation Indication_.
     * @since 6
     *
     */
    void SetOperationIndication(uint32_t newOperationIndication);

    /**
     * @brief The function to get service uuid, uuid is big endian order.
     *
     * @return UUID.
     * @since 6
     *
     */
    const UUID &GetServiceUuid() const;

    /**
     * @brief The function to get value permissions.
     *
     * @return value permissions.
     * @since 6
     *
     */
    int GetValuePermissions() const;

    /**
     * @brief The function to get uuid, uuid is big endian order.
     *
     * @return UUID.
     * @since 6
     *
     */
    const UUID &GetUuid() const;

    /**
     * @brief The function to get write type.
     *
     * @return write type.
     * @since 6
     *
     */
    int GetWriteType() const;

    /**
     * @brief The function to set write type.
     *
     * @param type write type.
     * @since 6
     *
     */
    int SetWriteType(int type);

    /**
     * @brief The function to get value.
     *
     * @param size size of get value.
     * @return value pointer.
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
     * @brief The function to set service uuid.
     *
     * @param values values of set SsapService uuid.
     * @since 6
     *
     */
    void SetServiceUuid(const UUID &uuid);
private:
    /**
     * @brief The writeType of property.
     *
     * @since 6
     *
     */
    uint8_t writeType_;
    /**
     * @brief The handle of property.
     *
     * @since 6
     *
     */
    uint16_t handle_;
    /**
     * @brief The type of property.
     *
     * @since 6
     *
     */
    uint8_t propertyType_;
    /**
     * @brief The uuid of entry, uuid is big endian order.
     *
     * @since 6
     *
     */
    UUID uuid_;
    /**
     * @brief The operation Indication of this property.
     *
     * @since 6
     *
     */
    uint32_t operationIndication_;
    /**
     * @brief The value of entry.
     *
     * @since 6
     *
     */
    std::unique_ptr<uint8_t[]> value_;

    /**
     * @brief The length of entry.
     *
     * @since 6
     *
     */
    size_t length_;

    /**
     * @brief The descriptors of entry.
     *
     * @since 6
     *
     */
    std::vector<SsapDescriptor> descriptors_;

    /**
     * @brief The value permissions of property.
     *
     * @since 6
     *
     */
    int valuePermissions_;
    /**
     * @brief The service uuid of property.
     *
     * @since 6
     *
     */
    UUID serviceUuid_;
};

} // namespace Nearlink
} // namespace OHOS

#endif  // NEARLINK_SSAP_PROPERTY_H
