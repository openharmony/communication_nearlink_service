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
 * @brief Defines sle ranging, including ranging and callbacks, and ranging functions.
 *
 * @since 7
 */

/**
 * @file nearlink_sle_ranging.h
 *
 * @brief ranging common functions.
 *
 * @since 7
 */

#ifndef NEARLINK_SLE_RANGING_H
#define NEARLINK_SLE_RANGING_H

#include "nearlink_remote_device.h"

namespace OHOS {
namespace Nearlink {

/**
 * @brief Represents ranging params.
 *
 * @since 7
 */
class NEARLINK_API RangingConfig {
public:
    /**
     * @brief A constructor used to create a <b>RangingConfig</b> instance.
     *
     * @since 7
     */
    RangingConfig();

    RangingConfig(uint8_t refreshRate);

    /**
     * @brief A destructor used to delete the <b>RangingConfig</b> instance.
     *
     * @since 7
     */
    ~RangingConfig();

    /**
     * @brief Set refresh rate.
     *
     * @param refreshRate refreshRate.
     * @since 7
     */
    void SetRefreshRate(const uint8_t refreshRate);

    /**
     * @brief Get refresh rate.
     *
     * @return refreshRate.
     * @since 7
     */
    uint8_t GetRefreshRate() const;

private:
    uint8_t refreshRate_{0};
};

/**
 * @brief Represents ranging params.
 *
 * @since 7
 */
class NEARLINK_API RangingResult {
public:
    /**
     * @brief A constructor used to create a <b>RangingResult</b> instance.
     *
     * @since 7
     */
    RangingResult();

    RangingResult(const std::string &address, const int result, const float distance);

    /**
     * @brief A destructor used to delete the <b>RangingResult</b> instance.
     *
     * @since 7
     */
    ~RangingResult();

    /**
     * @brief Set address.
     *
     * @param address address.
     * @since 7
     */
    void SetAddress(const std::string &address);

    /**
     * @brief Get address.
     *
     * @return address.
     * @since 7
     */
    std::string GetAddress() const;

    /**
     * @brief Set result.
     *
     * @param result result.
     * @since 7
     */
    void SetResult(const int &result);

    /**
     * @brief Get result.
     *
     * @return result.
     * @since 7
     */
    int GetResult() const;

    /**
     * @brief Set distance.
     *
     * @param distance distance.
     * @since 7
     */
    void SetDistance(const float &distance);

    /**
     * @brief Get distance.
     *
     * @return distance.
     * @since 7
     */
    float GetDistance() const;

    /**
     * @brief Set prob.
     *
     * @param prob prob.
     * @since 7
     */
    void SetProb(const float &prob);

    /**
     * @brief Get prob.
     *
     * @return prob.
     * @since 7
     */
    float GetProb() const;

    /**
     * @brief Set rssi.
     *
     * @param rssi rssi.
     * @since 7
     */
    void SetRssi(const float &rssi);

    /**
     * @brief Get rssi.
     *
     * @return v.
     * @since 7
     */
    float GetRssi() const;


private:
    std::string address_;
    int result_{0};
    float distance_{0.0};
    float prob_{0.0};
    float rssi_{0.0};
};

/**
 * @brief Represents ranging state params.
 *
 * @since 7
 */
class NEARLINK_API RangingState {
public:
    /**
     * @brief A constructor used to create a <b>RangingState</b> instance.
     *
     * @since 7
     */
    RangingState();

    RangingState(const std::string &address, const int newState, const int errorCode);

    /**
     * @brief A destructor used to delete the <b>RangingState</b> instance.
     *
     * @since 7
     */
    ~RangingState();

    /**
     * @brief Set address.
     *
     * @param address address.
     * @since 7
     */
    void SetAddress(const std::string &address);

    /**
     * @brief Get address.
     *
     * @return address.
     * @since 7
     */
    std::string GetAddress() const;

    /**
     * @brief Set new state.
     *
     * @param newState new state.
     * @since 7
     */
    void SetNewState(const int &newState);

    /**
     * @brief Get new state.
     *
     * @return new state.
     * @since 7
     */
    int GetNewState() const;

    /**
     * @brief Set errorCode.
     *
     * @param errorCode errorCode.
     * @since 7
     */
    void SetErrorCode(const int &errorCode);

    /**
     * @brief Get errorCode.
     *
     * @return errorCode.
     * @since 7
     */
    int GetErrorCode() const;

private:
    std::string address_;
    int newState_{0};
    int errorCode_{0};
};

/**
 * @brief Represents sle ranging callback.
 *
 * @since 7
 *
 */
class SleRangingCallback {
public:
    /**
     * @brief A destructor used to delete the <b>SleRangingCallback</b> instance.
     *
     * @since 7
     */
    virtual ~SleRangingCallback() = default;

    /**
     * @brief The function to SleRangingCallback.
     *
     * @param RangingResult callback of sle ranging result.
     * @since 7
     *
     */
    virtual void OnSleRangingResult(const RangingResult &result) = 0;

    /**
     * @brief  The function to OnSleRangingStateChange.
     *
     * @param state callback of sle ranging state.
     * @since 7
     */
    virtual void OnSleRangingStateChange(const RangingState &state) = 0;
};

/**
 * @brief Represents NearlinkSleRanging.
 *
 * @since 7
 */
class NEARLINK_API NearlinkSleRanging {
public:
    /**
     * @brief A constructor of NearlinkSleRanging.
     */
    static std::shared_ptr<NearlinkSleRanging> CreateNearlinkSleRanging(std::shared_ptr<SleRangingCallback> callback);

    /**
     * @brief A destructor used to delete the <b>NearlinkSleRanging</b> instance.
     *
     * @since 7
     */
    ~NearlinkSleRanging();

    /**
     * @brief start sle ranging.
     *
     * @param device the remote device that initiates ranging.
     * @return Returns the status code for this function called.
     */
    NlErrCode StartSleRanging(const NearlinkRemoteDevice &device, const RangingConfig &config);

    /**
     * @brief stop sle ranging.
     *
     * @param device the remote device in ranging.
     * @return Returns the status code for this function called.
     */
    NlErrCode StopSleRanging(const NearlinkRemoteDevice &device);

    /**
     * @brief  Get the ranging capability of the local.
     *
     * @param capability ranging capability.
     * @param[out] capability state for ranging capability.
     *         0 : not support;
     *         1 : support G;
     *         2 : support G/T;
     * @return Returns the status code for this function called.
     */
    static NlErrCode GetRangingSupportedCapability(uint8_t &capability);

private:
    explicit NearlinkSleRanging(std::shared_ptr<SleRangingCallback> callback);

    NEARLINK_DISALLOW_COPY_AND_ASSIGN(NearlinkSleRanging);

    NEARLINK_DECLARE_IMPL();

    struct Pattern {
        Pattern() {};
    };

public:
    // This constructor is not available, use NearlinkSleRanging interface to create objects.
    explicit NearlinkSleRanging(Pattern, std::shared_ptr<SleRangingCallback> callback) :
        NearlinkSleRanging(callback) {};
};

} // namespace Nearlink
} // namespace OHOS
#endif  // NEARLINK_SLE_RANGING_H