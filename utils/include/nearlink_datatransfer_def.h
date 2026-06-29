/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef NEARLINK_DATATRANSFER_DEF_H
#define NEARLINK_DATATRANSFER_DEF_H

#include <vector>

#include "sle_uuid.h"
#include "raw_address.h"
#include "nearlink_def_types.h"

namespace OHOS {
namespace Nearlink {

/**
 * @brief Represents datatransfer connection params.
 *
 * @since 6
 */
class DataTransferConnectionParams {
public:
    /**
     * @brief A constructor used to create a DataTransferConnectionParams instance.
     *
     * @since 6
     */
    DataTransferConnectionParams(){};

    /**
     * @brief A destructor used to delete the DataTransferConnectionParams instance.
     *
     * @since 6
     */
    ~DataTransferConnectionParams(){};

    /**
     * @brief Get datatransfer connection address.
     *
     * @since 6
     */
    std::string GetAddress() const
    {
        return address_;
    }

    /**
     * @brief Set datatransfer connection address.
     *
     * @param address datatransfer connection address.
     * @since 6
     */
    void SetAddress(const std::string &address)
    {
        address_ = address;
    }

    /**
     * @brief Get datatransfer connection uuid.
     *
     * @since 6
     */
    std::string GetUuid() const
    {
        return uuid_;
    }

    /**
     * @brief Set datatransfer connection uuid.
     *
     * @param uuid datatransfer connection uuid.
     * @since 6
     */
    void SetUuid(const std::string &uuid)
    {
        uuid_ = uuid;
    }

    /**
     * @brief Get datatransfer port.
     *
     * @since 6
     */
    uint16_t GetPort() const
    {
        return port_;
    }

    /**
     * @brief Set datatransfer port.
     *
     * @param port datatransfer port.
     * @since 6
     */
    void SetPort(uint16_t port)
    {
        port_ = port;
    }

    int32_t GetState() const
    {
        return state_;
    }

    void SetState(int32_t state)
    {
        state_ = state;
    }

    uint8_t GetTransMode() const
    {
        return transMode_;
    }

    void SetTransMode(uint8_t transMode)
    {
        transMode_ = transMode;
    }

    uint8_t GetFrameType() const
    {
        return frameType_;
    }

    void SetFrameType(uint8_t frameType)
    {
        frameType_ = frameType;
    }

    uint16_t GetMtu() const
    {
        return mtu_;
    }

    void SetMtu(uint16_t mtu)
    {
        mtu_ = mtu;
    }

public:
    std::string address_;
    std::string uuid_;
    uint16_t port_ = 0;
    int32_t state_ = -1;
    uint8_t transMode_ = 0;
    uint8_t frameType_ = static_cast<uint8_t>(SleConnFrameType::SLE_CONN_FRAME_TYPE_1);
    uint16_t mtu_ = 0;
};

struct DataTransferDataParams {
    DataTransferDataParams() {}

    DataTransferDataParams(const std::string &address, const std::string &uuid, uint16_t port,
        const std::vector<uint8_t> &data)
        : address_(address), uuid_(uuid), port_(port), data_(data) {}

    std::string address_;
    std::string uuid_;
    uint16_t port_ { 0 };
    std::vector<uint8_t> data_;
};
}  // namespace Nearlink
}  // namespace OHOS

#endif  /// NEARLINK_DATATRANSFER_DEF_H