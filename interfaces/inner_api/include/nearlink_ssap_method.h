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
 * @file nearlink_ssap_method.h
 *
 * @brief Nearlink ssap method interface.
 *
 * @since 6
 *
 */

#ifndef NEARLINK_SSAP_METHOD_H
#define NEARLINK_SSAP_METHOD_H

#include <cstddef>
#include <cstdint>
#include <list>
#include "memory"
#include "nearlink_def.h"
#include "nearlink_uuid.h"
#include "vector"

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
 * @brief SSAP-based Method class
 * A method is a value used in a service along with properties and configuration information about
 * how the value is accessed and information about how the value is displayed or represented.
 */
class NEARLINK_API SsapMethod {
public:
    /** A SSAP method call type.
     *  Define SSAP method call types.
     */
    enum CallMethodType {
        CALL_METHOD_CMD = 0x00,
        CALL_METHOD_REQ = 0x01,
        CALL_METHOD_DEFAULT = 0x02
    };

    /** A SSAP property type.
     *  Define SSAP property types.
     */
    enum MethodType {
        ENTRY_TYPE_METHOD = 0x03,
        ENTRY_TYPE_VENDOR_METHOD = 0x0B,
    };

    /**
     * @brief The function to delete constructor of SsapMethod.
     *
     * @since 6
     *
     */
    SsapMethod() = delete;

    /**
     * @brief A constructor of SsapMethod.
     *
     * @param type Type of Ssap Method.
     * @param uuid Uuid of Ssap Method, uuid is big endian order.
     * @param permissions of Ssap method value, For details, see SsapPermission.
     * @since 6
     *
     */
    SsapMethod(int type, const UUID uuid, int permissions);

    /**
     * @brief A constructor of SsapEntry.
     *
     * @param handle handle of Ssap Method.
     * @param type Type of Ssap Method.
     * @param uuid Uuid of Ssap Method, uuid is big endian order.
     * @param permissions of Ssap Method.
     * @since 6
     *
     */
    SsapMethod(uint16_t handle, const int type, const UUID uuid, const int permissions);
    SsapMethod(const SsapMethod &);
    SsapMethod &operator=(const SsapMethod &);

    /**
     * @brief The function to get method type.
     *
     * @return MethodType.
     * @since 6
     *
     */
    int GetMethodType() const;

    /**
     * @brief The function to get handle.
     *
     * @return uint16_t  handle.
     * @since 6
     *
     */
    uint16_t GetHandle() const;

    /**
     * @brief The function to get service uuid.
     *
     * @return UUID service uuid which entry belong to.
     * @since 6
     *
     */
    const UUID &GetServiceUuid() const;

    /**
     * @brief The function to get permissions.
     *
     * @return permissions.
     * @since 6
     *
     */
    int GetPermissions() const;

    /**
     * @brief The function to get uuid.
     *
     * @return UUID, uuid is big endian order.
     * @since 6
     *
     */
    const UUID &GetUuid() const;

    /**
     * @brief The function to get parameter.
     *
     * @param size size of get parameter.
     * @return parameter pointer.
     * @since 6
     *
     */
    const std::unique_ptr<uint8_t[]> &GetParameter(size_t *size) const;

    /**
     * @brief The function to get result.
     *
     * @param size size of get result.
     * @return result pointer.
     * @since 6
     *
     */
    const std::unique_ptr<uint8_t[]> &GetResult(size_t *size) const;

    /**
     * @brief The function to set parameter.
     *
     * @param values values of set parameter.
     * @param length length of set parameter.
     * @since 6
     *
     */
    void SetParameter(const uint8_t *values, const size_t length);

    /**
     * @brief The function to set result.
     *
     * @param values values of set result.
     * @param length length of set result.
     * @since 6
     *
     */
    void SetResult(const uint8_t *values, const size_t length);

    /**
     * @brief The function to set service uuid.
     *
     * @param uuid values of set uuid.
     * @since 6
     *
     */
    void SetServiceUuid(const UUID &uuid);

private:
    /**
     * @brief The handle of method.
     *
     * @since 6
     *
     */
    uint16_t handle_;
    /**
     * @brief The type of method.
     *
     * @since 6
     *
     */
    uint8_t methodType_;
    /**
     * @brief The uuid of method, uuid is big endian order.
     *
     * @since 6
     *
     */
    UUID uuid_;
    /**
     * @brief The value of parameter.
     *
     * @since 6
     *
     */
    std::unique_ptr<uint8_t[]> parameter_;
    /**
     * @brief The length of parameter.
     *
     * @since 6
     *
     */
    size_t parameterLength_;
    /**
     * @brief The value of result.
     *
     * @since 6
     *
     */
    std::unique_ptr<uint8_t[]> result_;
    /**
     * @brief The length of result.
     *
     * @since 6
     *
     */
    size_t resultLength_;
    /**
     * @brief The value permissions of method.
     *
     * @since 6
     *
     */
    int permissions_;
    /**
     * @brief The service uuid of method.
     *
     * @since 6
     *
     */
    UUID serviceUuid_;
};

} // namespace Nearlink
} // namespace OHOS

#endif  // NEARLINK_SSAP_METHOD_H
