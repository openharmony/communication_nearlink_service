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
 * @brief Defines data transfer, including transfer data and callbacks, and data transfer functions.
 *
 * @since 7
 */

/**
 * @file nearlink_sle_datatransfer.h
 *
 * @brief DataTransfer common functions.
 *
 * @since 7
 */

#ifndef NEARLINK_SLE_DATATRANSFER_H
#define NEARLINK_SLE_DATATRANSFER_H

#include "nearlink_def.h"
#include "nearlink_types.h"
#include "nearlink_errorcode.h"

namespace OHOS::Nearlink {
/**
 * @brief Represents connection params.
 *
 * @since 6
 */
class NEARLINK_API ConnectionParams {
public:
    enum class PortTransMode : uint8_t {
        TRANSPORT_MODE_BASIC = 0,    // 基础模式
        TRANSPORT_MODE_TRANSPARENT,  // 透传模式
        TRANSPORT_MODE_STREAM,       // 流模式
        TRANSPORT_MODE_RELIABLE,     // 可靠模式
        TRANSPORT_MODE_MAX,  // 传输模式数目
    };

    /**
     * @brief A constructor used to create a <b>ConnectionParams</b> instance.
     *
     * @since 6
     */
    ConnectionParams();

    ConnectionParams(const std::string &address, const std::string &uuid, int32_t state);

    /**
     * @brief A destructor used to delete the <b>ConnectionParams</b> instance.
     *
     * @since 6
     */
    ~ConnectionParams();

    /**
     * @brief Set address.
     *
     * @param address address.
     * @since 6
     */
    void SetAddress(const std::string &address);

    /**
     * @brief Get address.
     *
     * @return address.
     * @since 6
     */
    std::string GetAddress() const;

    /**
     * @brief Set uuid.
     *
     * @param uuid uuid.
     * @since 6
     */
    void SetUuid(const std::string &uuid);

    /**
     * @brief Get uuid.
     *
     * @return uuid.
     * @since 6
     */
    std::string GetUuid() const;

    void SetState(int32_t state);
    int32_t GetState() const;

    void SetTransMode(uint8_t transMode);
    uint8_t GetTransMode() const;

    void SetMtu(uint16_t mtu);
    uint16_t GetMtu() const;

    void SetFrameType(uint8_t frameType);
    uint8_t GetFrameType() const;

private:
    std::string address_;
    std::string uuid_;
    int32_t state_ = 0;
    uint8_t transMode_ = 0;
    uint16_t mtu_ = 0;
    uint8_t frameType_ = static_cast<uint8_t>(SleConnFrameType::SLE_CONN_FRAME_TYPE_1);
};

/**
 * @brief Represents connection state params.
 *
 * @since 6
 */
class NEARLINK_API ConnStateParams {
public:
    /**
     * @brief A constructor used to create a <b>ConnStateParams</b> instance.
     *
     * @since 6
     */
    ConnStateParams();

    ConnStateParams(const std::string &address, const std::string &uuid);

    /**
     * @brief A destructor used to delete the <b>ConnStateParams</b> instance.
     *
     * @since 6
     */
    ~ConnStateParams();

    /**
     * @brief Set address.
     *
     * @param address address.
     * @since 6
     */
    void SetAddress(const std::string &address);

    /**
     * @brief Get address.
     *
     * @return address.
     * @since 6
     */
    std::string GetAddress() const;

    /**
     * @brief Set uuid.
     *
     * @param uuid uuid.
     * @since 6
     */
    void SetUuid(const std::string &uuid);

    /**
     * @brief Get uuid.
     *
     * @return uuid.
     * @since 6
     */
    std::string GetUuid() const;

private:
    std::string address_;
    std::string uuid_;
};

/**
 * @brief Represents the data params.
 *
 * @since 6
 */
class NEARLINK_API DataParams {
public:
    /**
     * @brief The function to delete constructor of DataParams.
     *
     * @since 6
     *
     */
    DataParams() = delete;

    /**
     * @brief A constructor used to create a <b>DataParams</b> instance.
     *
     * @since 6
     */
    DataParams(const std::string &address, const std::string &uuid);

    DataParams(const DataParams &);
    DataParams &operator=(const DataParams &);

    /**
     * @brief Set address.
     *
     * @param address address.
     * @since 6
     */
    void SetAddress(const std::string &address);

    /**
     * @brief Get address.
     *
     * @return address.
     * @since 6
     */
    std::string GetAddress() const;

    /**
     * @brief Set uuid.
     *
     * @param uuid uuid.
     * @since 6
     */
    void SetUuid(const std::string &uuid);

    /**
     * @brief Get uuid.
     *
     * @return uuid.
     * @since 6
     */
    std::string GetUuid() const;

    /**
     * @brief The function to get value.
     *
     * @param size size of get value.
     * @return value pointer.
     * @since 6
     *
     */
    const std::unique_ptr<uint8_t[]> &GetData(size_t *size) const;

    /**
     * @brief The function to set value.
     *
     * @param values values of set value.
     * @param length length of set value.
     * @since 6
     *
     */
    void SetData(const uint8_t *values, const size_t length);

private:
    std::string address_;
    std::string uuid_;
    /**
     * @brief The value of entry.
     *
     * @since 6
     *
     */
    std::unique_ptr<uint8_t[]> data_;
    size_t length_;
};

/**
 * @brief Represents datatransfer callback.
 *
 * @since 6
 *
 */
class SleDataTransferCallback {
public:
    /**
     * @brief A destructor used to delete the <b>SleDataTransferCallback</b> instance.
     *
     * @since 6
     */
    virtual ~SleDataTransferCallback() = default;

    /**
     * @brief The function to OnConnectionStateChanged.
     *
     * @param connectionState callback of SleDataTransferCallback.
     * @param ret ret of SleDataTransferCallback.
     * @since 6
     *
     */
    virtual void OnConnectionStateChanged(const ConnectionParams &result) = 0;

    /**
     * @brief  The function to OnReceiveData.
     *
     * @param result read data result.
     * @since 6
     */
    virtual void OnReceiveData(const DataParams &result) = 0;
};

/**
 * @brief Represents datatransfer.
 *
 * @since 7
 */
class NEARLINK_API SleDataTransfer : public std::enable_shared_from_this<SleDataTransfer> {
public:
    /**
     * @brief A constructor of SleAdvertiser.
     */
    static std::shared_ptr<SleDataTransfer> CreateSleDataTransfer(void);

    /**
     * @brief Create port.
     *
     * @param uuid application service uuid.
     * @return Returns the status code for this function called.
     */
    NlErrCode CreatePort(const std::string &uuid, std::shared_ptr<SleDataTransferCallback> callback);

    /**
     * @brief Destroy port.
     *
     * @param uuid application service uuid.
     * @return Returns the status code for this function called.
     */
    NlErrCode DestroyPort(const std::string &uuid);

    /**
     * @brief Connect port.
     *
     * @param params Connection params.
     * @return Returns the status code for this function called.
     */
    NlErrCode Connect(const ConnectionParams &params);

    /**
     * @brief Disconnect port.
     *
     * @param params Connection params.
     * @return Returns the status code for this function called.
     */
    NlErrCode Disconnect(const ConnectionParams &params);

    /**
     * @brief Get data transfer connect state.
     *
     * @param params remoteAddr and uuid.
     * @param[out] connect state for data transfer.
     *         ConnectionState::CONNECTING;
     *         ConnectionState::CONNECTED;
     *         ConnectionState::DISCONNECTING;
     *         ConnectionState::DISCONNECTED.
     * @return Returns the status code for this function called.
     */
    NlErrCode GetConnectionState(const ConnStateParams &params, int32_t &connState);

    /**
     * @brief The function to write data.
     *
     * @param params data object.
     * @return Returns the status code for this function called.
     */
    NlErrCode WriteData(DataParams &params);

    ~SleDataTransfer();

    /**
     * @brief The function to judge if support HighSpeedDataTransfer for Wearlink Proxy.
     *
     * @return Returns <b>true</b> if HighSpeedDataTransfer support;
     *         returns <b>false</b> if HighSpeedDataTransfer doesn't support.
     */
    bool IsSupportHighSpeedDataTransfer() const;

     /**
     * @brief The function to update connect interval for Wearlink Proxy.
     * @param[in] intervalType for data transfer.
     *         HIGH_SPEED_INTERVAL---0;
     *         MID_SPEED_INTERVAL---1;
     *         LOW_SPEED_INTERVAL---2;
     * @return Returns <b>true</b> update connect interval success;
     *         returns <b>false</b> update connect interval fail.
     */
    bool UpdateConnectInterval(std::string device, int32_t intervalType) const;

private:
    SleDataTransfer();

    NEARLINK_DISALLOW_COPY_AND_ASSIGN(SleDataTransfer);

    struct impl;
    std::shared_ptr<impl> pimpl;

    // It has no specific meaning, only used to hide the constructor.
    struct Pattern {
        Pattern(){};
    };

public:
    // This constructor is not available, use CreateSleAdvertiser interface to create objects.
    explicit SleDataTransfer(Pattern) : SleDataTransfer(){};
};
}  // namespace OHOS::Nearlink
#endif  // NEARLINK_SLE_DATATRANSFER_H
