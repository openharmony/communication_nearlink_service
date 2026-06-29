/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#ifndef SLE_FEATURE_H
#define SLE_FEATURE_H

#include <vector>
#include "SleDefs.h"
#include "nlstk_cfgdb_api.h"
#include "cm_def.h"

constexpr int NEARLINK_FEATURE_INDEX_8 = 8;
constexpr int NEARLINK_FEATURE_RANGE = 0x4;

constexpr int NEARLINK_FEATURE_INDEX_0 = 0;
constexpr int NEARLINK_FRAME_4_CAP_BIT = 0x80;

/*
 * @brief The nearlink system.
 */
namespace OHOS {
namespace Nearlink {
/*
 * @brief Sle feature.
 */
class SleFeature {
public:
    /**
     * @brief Get sle feature instance.
     *
     * @return @c feature instance.
     */
    static SleFeature &GetInstance()
    {
        static SleFeature instance;
        return instance;
    }

    /**
     * @brief Get sle feature extended scan supported.
     *
     * @return @c true:supported; false:not supported.
     */
    bool IsExtendedScanSupported() const
    {
        return IsFeatureSupported(EXTENDED_SCANNER_FILTER_POLICIES);
    }

    /**
     * @brief Get sle feature 2M phy supported.
     *
     * @return @c true:supported; false:not supported.
     */
    bool IsSle2MPhySupported() const
    {
        return IsFeatureSupported(SLE_2M_PHY);
    }

    /**
     * @brief Get sle feature coded phy supported.
     *
     * @return @c true:supported; false:not supported.
     */
    bool IsSleCodedPhySupported() const
    {
        return IsFeatureSupported(SLE_CODED_PHY);
    }

    /**
     * @brief Get sle feature extended advertising supported.
     *
     * @return @c true:supported; false:not supported.
     */
    bool IsSleExtendedAdvertisingSupported() const
    {
        return IsFeatureSupported(SLE_EXTENDED_ADVERTISING);
    }

    /**
     * @brief Get sle feature periodic advertising supported.
     *
     * @return @c true:supported; false:not supported.
     */
    bool IsSlePeriodicAdvertisingSupported() const
    {
        return IsFeatureSupported(SLE_PERIODIC_ADVERTISING);
    }

    /**
     * @brief Get sle feature privacy supported.
     *
     * @return @c true:supported; false:not supported.
     */
    bool IsPrivacySupported() const
    {
        return IsFeatureSupported(LL_PRIVACY);
    }

    /**
     * @brief Get sle feature maximum advertising data length.
     *
     * @return @c data length.
     */
    uint32_t GetBleMaximumAdvertisingDataLength() const
    {
        return SLE_LEGACY_ADV_DATA_LEN_MAX;
    }

    /**
     * @brief Get sle feature extended advertising max handle number.
     *
     * @return @c handle number.
     */
    static uint8_t GetBleExAdvGetMaxHandleNum()
    {
        return static_cast<uint8_t>(SleAdvertisingHandle::SLE_INVALID_ADVERTISING_HANDLE);
    }

    /**
     * @brief Get sle supported feature.
     *
     * @return @c true:supported; false:not supported.
     */
    static bool IsFeatureSupported(FEATURE_SUPPORTED feature)
    {
        return false;
    }

    /**
     * @brief Set sle local feature.
     *
     * @param param is feature value.
     */
    void SetLocalFeature(NLSTK_CfgdbLocalFeatures_S *param);

    /**
     * @brief Get sle feature range supported.
     *
     * @return @c true:supported; false:not supported.
     */
    bool IsRangSupported();

    /**
     * @brief Get sle feature frame four supported.
     *
     * @return @c true:supported; false:not supported.
     */
    bool IsFrameFourSupported();

private:
    /**
     * @brief Constructor.
     */
    SleFeature()
    {}

    /**
     * @brief Destructor.
     */
    ~SleFeature()
    {}
    /**
     * @brief Constructor.
     */
    SleFeature(SleFeature &) = delete;

    /**
     * @brief Constructor.
     */
    SleFeature &operator=(const SleFeature &) = delete;
};
}  // namespace Sle
}  // namespace OHOS

#endif  // SLE_FEATURE_H