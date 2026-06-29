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
#ifndef SLE_INTERFACE_PROFILE_MANAGER_H
#define SLE_INTERFACE_PROFILE_MANAGER_H

#include <vector>
#include "SleInterfaceProfile.h"

namespace OHOS {
namespace Nearlink {
/**
 * @brief Represents interface profile manager.
 *
 * @since 6
 */
class NEARLINK_API SleInterfaceProfileManager {
public:
    /**
     * @brief A destructor used to delete the <b>SleInterfaceProfileManager</b> instance.
     *
     * @since 6
     */
    virtual ~SleInterfaceProfileManager() = default;

    /**
     * @brief Get profile manager singleton instance pointer.
     *
     * @return Returns the singleton instance pointer.
     * @since 6
     */
    static SleInterfaceProfileManager &GetInstance();

    /**
     * @brief Get profile service pointer.
     *
     * @param name Profile service name.
     * @return Returns the profile service pointer.
     * @since 6
     */
    virtual SleInterfaceProfile *GetProfileService(const std::string &name) const = 0;

    /**
     * @brief Get profile service ID list.
     *
     * @return Returns vector of enabled profile services ID.
     * @since 6
     */
    virtual std::vector<uint32_t> GetProfileServicesList() const = 0;

    /**
     * @brief Get profile service connect state.
     *
     * @param profileID Profile service ID.
     * @return Returns connect state for designated profile service.
     * @since 6
     */
    virtual SleConnectState GetProfileServiceConnectState(const uint32_t profileID) const = 0;

    /**
     * @brief Get local device supported uuids.
     *
     * @param[out] Vector which use to return support uuids.
     * @since 6
     */
    virtual void GetProfileServicesSupportedUuids(std::vector<std::string> &uuids) const = 0;
};
}  // namespace Sle
}  // namespace OHOS

#endif  // SLE_INTERFACE_PROFILE_MANAGER_H