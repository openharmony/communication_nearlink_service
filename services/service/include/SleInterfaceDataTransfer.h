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

#ifndef SLE_INTERFACE_DATATRANSFER_H
#define SLE_INTERFACE_DATATRANSFER_H

#include "nearlink_types.h"
#include "i_nearlink_sle_datatransfer_callback.h"
#include "nearlink_sle_datatransfer_connection_params.h"
#include "nearlink_errorcode.h"

namespace OHOS::Nearlink {
/**
 * @brief Represents datatransfer service callback.
 *
 * @since 6
 */
class ISleDataTransferServiceCallback {
public:
    virtual ~ISleDataTransferServiceCallback() = default;

    /**
    * @brief Callback function for handling changes in connection state.
    *
    * @param connectionParams Parameters related to the data transfer connection.
    * @param fd File descriptor used by the remote peer for data transfer.
    *        - Valid only when connection state is CONNECTED, otherwise INVALID_FD (-1).
    *        - Passed via IPC: the caller (Service) transfers ownership to callee (Framework).
    *        - Framework takes ownership: managed by PortInfo (RAII) or closed immediately if callback invalid.
    *        - Caller MUST NOT use this fd after this call returns.
    */
    virtual void OnConnectionStateChanged(const DataTransferConnectionParams &connectionParams, int fd) = 0;
    virtual std::string GetRandomAddr(const std::string &adress, uint32_t tokenId) = 0;
};

/**
 * @brief SLE datatransfer interface.
 */
class SleInterfaceDataTransfer {
public:
    virtual ~SleInterfaceDataTransfer() = default;

    static SleInterfaceDataTransfer &GetInstance();

    /**
     * @brief Register datatransfer service callback.
     *
     * @param callback Class ISleDataTransferCallback pointer to register callback.
     * @since 6
     */
    virtual void RegisterSleDataTransferServiceCallback(std::shared_ptr<ISleDataTransferServiceCallback> callback) = 0;

    /**
     * @brief Deregister datatransfer service callback.
     *
     * @since 6
     */
    virtual void DeregisterSleDataTransferServiceCallback() = 0;

    /**
     * @brief Create port.
     *
     * @param uuid application service uuid.
     * @return Returns <b>portId</b> if the operation is successful;
     * @since 6
     */
    virtual uint16_t CreatePort(const std::string &uuid) = 0;

    /**
     * @brief Destroy port.
     *
     * @param uuid application service uuid.
     * @return Returns <b>true</b> if the operation is successful;
     *         returns <b>false</b> if the operation fails.
     * @since 6
     */
    virtual void DestroyPort(const std::string &uuid, uint16_t port) = 0;

    /**
     * @brief Connect port.
     *
     * @param params Connection params.
     * @since 6
     */
    virtual void Connect(DataTransferConnectionParams &params) const = 0;

    /**
     * @brief Disconnect port.
     *
     * @param params Connection params.
     * @since 6
     */
    virtual void Disconnect(DataTransferConnectionParams &params) const = 0;

    /**
     * @brief Get connection state.
     *
     * @param params Connection params.
     * @since 6
     */
    virtual int32_t GetConnectionState(DataTransferConnectionParams &params) const = 0;


    /**
     * @brief Get connection tcid channel state.
     *
     * @param params Connection portId.
     * @param params remote address.
     * @param params tcid state.
     * @param params tcid pre state.
     * @since 6
     */
    virtual int GetConnectionTransferState(uint16_t portId, const std::string &address, int &transState,
        int &preTransState) const = 0;

    /**
     * @brief Write Data.
     *
     * @param params data params.
     * @since 6
     */
    virtual int WriteData(DataTransferDataParams &params) const = 0;

    /**
     * @brief clear cache info.
     *
     * @param tokenId application token id.
     * @return Returns port if the operation is successful;
     *         returns 0 if the operation fails.
     * @since 6
     */
    virtual void ClearCacheByTokenId(uint64_t tokenId) = 0;

    /**
     * @brief change socket channel state.
     *
     * @param params portId.
     * @param params remote adress.
     * @param params socket channel state.
     * @since 6
     */
    virtual void ChangeSocketState(uint16_t portId, std::string address, uint8_t result) = 0;

#ifdef WATCH_STANDARD
    /**
     * @brief The function to update connect interval for Wearlink Proxy.
     * @return Returns <b>true</b> update connect interval success;
     *         returns <b>false</b> update connect interval fail.
     * @since 6
     */
    virtual bool UpdateConnectInterval(std::string device, int32_t intervalType) = 0;
#endif
};
}  // namespace OHOS::Nearlink
#endif  // SLE_INTERFACE_DATATRANSFER_H