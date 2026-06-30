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
 * @brief Defines nearlink tws client, including wear detection functions.
 *
 * @since 6
 */

/**
 * @file nearlink_tws_client.h
 *
 * @brief Framework nearlink tws client interface.
 *
 * @since 6
 */

#ifndef NEARLINK_TWS_CLIENT_H
#define NEARLINK_TWS_CLIENT_H

#include "nearlink_def.h"
#include "nearlink_errorcode.h"
#include "nearlink_types.h"

namespace OHOS {
namespace Nearlink {

/**
 * @brief Tws client API observer function.
 *
 * @since 6.0
 */
class NearlinkTwsClientObserver {
public:
    /**
     * @brief A destructor used to delete the Tws client observer instance.
     *
     * @since 6.0
     */
    virtual ~NearlinkTwsClientObserver() = default;

    /**
     * @brief The callback function Tws device remote info.
     *
     * @param address the Tws device address.
     * @param value the Tws device remote info.
     * @since 6.0
     */
    virtual void OnTwsRemoteInfo(const std::string &address, const std::vector<uint8_t> &value)
    {}
};

/**
 * @brief TWS client API.
 *
 * @since 6.0
 */
class NEARLINK_API NearlinkTwsClient {
public:
    /**
     * @brief Get tws client.
     *
     * @return Returns the singleton instance.
     * @since 6
     */
    static NearlinkTwsClient &GetInstance();

    NlErrCode RegisterTwsClientObserver(std::shared_ptr<NearlinkTwsClientObserver> observer);
    NlErrCode DeregisterTwsClientObserver(std::shared_ptr<NearlinkTwsClientObserver> observer);

    NlErrCode EnableWearDetection(const std::string &address);
    NlErrCode DisableWearDetection(const std::string &address);
    NlErrCode GetWearDetectionState(const std::string &address, int32_t &state);
    NlErrCode IsDeviceWearing(const std::string &address, bool &isWearing);
    NlErrCode IsWearDetectionSupported(const std::string &address, bool &isSupported);
    NlErrCode GetTwsRoleInfo(const std::string &address, int32_t &roleInfo);
    NlErrCode GetTwsAudioDelay(const std::string &address, uint32_t &delayValue);
    NlErrCode SendUserSelection(const std::string &address, const std::vector<struct AudioStreamInfo> &streamData);
    NlErrCode QueryStreamState(const std::string &address, std::vector<struct AudioStreamInfo> &streamData);
    NlErrCode IsSupportVirtualAutoConnect(const std::string &address, bool &isSupported);
    NlErrCode SetVirtualAutoConnectType(const std::string &address, int32_t connType, int32_t businessType);
    ~NearlinkTwsClient();

private:
    NearlinkTwsClient();
    NEARLINK_DISALLOW_COPY_AND_ASSIGN(NearlinkTwsClient);
    NEARLINK_DECLARE_IMPL();
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // NEARLINK_TWS_CLIENT_H
