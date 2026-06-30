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

#ifndef OHOS_NEARLINK_DATATRANSFER_CACHE_H
#define OHOS_NEARLINK_DATATRANSFER_CACHE_H

#include <cstddef>
#include <cstdint>
#include <vector>
#include "string"
#include "nearlink_safe_list.h"
#include "nearlink_def.h"

namespace OHOS::Nearlink {
enum TransferState {
    TRANSFER_FAIL = -1,
    TRANSFER_AVAILABLE = 0,
    TRANSFER_BUSY = 1,
};

struct AppConnectParamMapping {
    std::string address;
    std::string randomAddress;
    uint16_t dstPort{0};
    uint8_t tcid{0};
    uint8_t transMode{0};
    uint8_t frameType{0};
    int32_t state{0};
    int transState{TransferState::TRANSFER_AVAILABLE};
    int preTransState{TransferState::TRANSFER_AVAILABLE};

    // 定义==运算符，用于调用NearlinkSafeList的Insert方法
    bool operator==(const AppConnectParamMapping &other) const
    {
        return address == other.address;
    }
};

class SleDataTransferCache {
public:
    /**
     * @brief A constructor used to create a <b>SleDataTransferCache</b> instance.
     *
     * @since 6
     */
    SleDataTransferCache();

    /**
     * @brief A destructor used to delete the <b>SleDataTransferCache</b> instance.
     *
     * @since 6
     */
    ~SleDataTransferCache();

    /**
     * @brief Set uuid.
     *
     * @param string uuid.
     * @since 6
     */
    void SetUuid(const std::string &uuid);

    /**
     * @brief Get uuid.
     *
     * @return Returns uuid.
     * @since 6
     */
    std::string GetUuid() const;

    /**
     * @brief Set tokenId.
     *
     * @param uint64_t tokenId.
     * @since 6
     */
    void SetTokenId(uint64_t tokenId);

    /**
     * @brief Get tokenId.
     *
     * @return Returns tokenId.
     * @since 6
     */
    uint64_t GetTokenId() const;

    /**
     * @brief Set Uid.
     *
     * @param int32_t Uid.
     * @since 6
     */
    void SetUid(int32_t uid);

    /**
     * @brief Get Uid.
     *
     * @return Returns Uid.
     * @since 6
     */
    int32_t GetUid() const;

    /**
     * @brief Set Pid.
     *
     * @param int32_t Pid.
     * @since 6
     */
    void SetPid(int32_t pid);

    /**
     * @brief Get Pid.
     *
     * @return Returns Pid.
     * @since 6
     */
    int32_t GetPid() const;

    /**
     * @brief Get port Connects.
     *
     * @return Returns portConnects_.
     * @since 6
     */
    std::vector<AppConnectParamMapping> GetPortConnects();

    /**
     * @brief Set port Connects.
     *
     * @param appConnectParamMapping cache.
     * @since 6
     */
    void SetPortConnects(const AppConnectParamMapping &temp);

    /**
     * @brief Update state.
     *
     * @param int32_t newState.
     * @since 6
     */
    void UpdateState(int32_t newState);
    void UpdateTransferState(const std::string &addr, uint8_t tcid, int32_t newState);
    std::string GetRandomAddrByAddr(const std::string &addr);
    void UpdateStateByAddr(const std::string &addr, int32_t newState);
    uint8_t UpdateStateAndGetTcidByAddr(const std::string &addr, int32_t newState);
    bool GetAppConnectParamByAddr(const std::string &addr, AppConnectParamMapping &param);
    bool HasAppConnect();
    bool HasRemoteAddressConnect(const std::string &address);
    bool StopAppConnect();
    bool HasTargetAddr(const std::string &addr);
#ifdef RES_SCHED_SUPPORT
    bool needRssReportDisconnect();
#endif
private:
    std::string uuid_;
    uint64_t tokenId_{0};
    int32_t uid_{0};
    int32_t pid_{0};
    NearlinkSafeList<AppConnectParamMapping> portConnectionList_;
};
}  // namespace OHOS::Nearlink
#endif  // OHOS_NEARLINK_DATATRANSFER_CACHE_H