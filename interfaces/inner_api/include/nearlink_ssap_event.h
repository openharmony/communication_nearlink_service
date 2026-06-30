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
 * @file nearlink_ssap_method.h
 *
 * @brief Nearlink ssap method interface.
 *
 * @since 6
 *
 */

#ifndef NEARLINK_SSAP_EVENT_H
#define NEARLINK_SSAP_EVENT_H

#include <cstddef>
#include <cstdint>
#include "nearlink_def.h"
#include "cstdint"
#include <list>
#include "memory"
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
 * @brief SSAP-based Event class
 * A event is a value used in a service along with properties and configuration information about
 * how the value is accessed and information about how the value is displayed or represented.
 */
class NEARLINK_API SsapEvent {
public:
    /** A SSAP event type.
     *  Define SSAP event types.
     */
    enum MethodType {
        ENTRY_TYPE_EVENT = 0x04,
        ENTRY_TYPE_VENDOR_EVENT = 0x0C,
    };

    /**
     * @brief The function to delete constructor of SsapEvent.
     *
     * @since 6
     *
     */
    SsapEvent() = delete;

    /**
     * @brief A constructor of SsapEvent.
     *
     * @param type Type of Ssap Event.
     * @param uuid Uuid of Ssap Event, uuid is big endian order.
     * @since 6
     *
     */
    SsapEvent(int type, const UUID uuid);

    /**
     * @brief A constructor of SsapEntry.
     *
     * @param handle handle of Ssap Event.
     * @param type Type of Ssap Event.
     * @param uuid Uuid of Ssap Event, uuid is big endian order.
     * @since 6
     *
     */
    SsapEvent(uint16_t handle, const int type, const UUID uuid);

    /**
     * @brief A constructor of SsapEntry.
     *
     * @param handle handle of Ssap Event.
     * @param type Type of Ssap Event.
     * @param uuid Uuid of Ssap Event, uuid is big endian order.
     * @since 6
     *
     */
    SsapEvent(uint16_t handle, const UUID uuid);
    SsapEvent(const SsapEvent &);
    SsapEvent &operator=(const SsapEvent &);

    /**
     * @brief The function to get event type.
     *
     * @return EventType.
     * @since 6
     *
     */
    int GetEventType() const;

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
     * @brief The function to get uuid.
     *
     * @return UUID, uuid is big endian order.
     * @since 6
     *
     */
    const UUID &GetUuid() const;

    /**
     * @brief The function to set uuid.
     *
     * @param values values of set uuid.
     * @param length length of set uuid.
     * @since 6
     *
     */
    void SetUuid(UUID uuidParam);

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
     * @brief The function to set result.
     *
     * @param values values of set result.
     * @param length length of set result.
     * @since 6
     *
     */
    void SetParameter(const uint8_t *values, const size_t length);

    /**
     * @brief The function to set service uuid.
     *
     * @param uuid values of set SsapService uuid.
     * @since 6
     *
     */
    void SetServiceUuid(const UUID &uuid);


private:
    /**
     * @brief The handle of event.
     *
     * @since 6
     *
     */
    uint16_t handle_;
    /**
     * @brief The type of event.
     *
     * @since 6
     *
     */
    uint8_t eventType_;
    /**
     * @brief The uuid of event, uuid is big endian order.
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
     * @brief The length of entry.
     *
     * @since 6
     *
     */
    size_t length_;
    /**
     * @brief The service uuid of event.
     *
     * @since 6
     *
     */
    UUID serviceUuid_;
};

} // namespace Nearlink
} // namespace OHOS

#endif  // NEARLINK_SSAP_EVENT_H
