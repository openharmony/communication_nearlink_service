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
#ifndef PROFILE_SERVICE_MANAGER_H
#define PROFILE_SERVICE_MANAGER_H

#include <map>
#include <vector>
#include <unordered_set>
#include "SleInterfaceProfile.h"
#include "SleInterfaceProfileManager.h"
#include "sle_uuid.h"

namespace OHOS {
namespace Nearlink {
/**
 * @brief Represents profile service manager.
 *
 * @since 6
 */
class ProfileServiceManager : public SleInterfaceProfileManager {
public:
    /**
     * @brief Get profile service manager singleton instance pointer.
     *
     * @return Returns the singleton instance pointer.
     * @since 6
     */
    static ProfileServiceManager &GetInstance();

    /**
     * @brief Initialize profile service manager.
     *
     * @since 6
     */
    static void Initialize();

    /**
     * @brief Uninitialize profile service manager.
     *
     * @since 6
     */
    static void Uninitialize();

    // framework function
    /**
     * @brief Get profile service pointer.
     *
     * @param name Profile service name.
     * @return Returns the profile service pointer.
     * @since 6
     */
    SleInterfaceProfile *GetProfileService(const std::string &name) const override;

    /**
     * @brief Create profile services according to config.xml.
     *
     * @since 6
     */
    void Start() const;

    /**
     * @brief Delete all profile services when Start() create.
     *
     * @since 6
     */
    void Stop() const;

    /**
     * @brief Enable profile services.
     *
     * @param transport Adapter transport.
     * @return Returns <b>true</b> if the operation is successful;
     *         returns <b>false</b> if the operation fails.
     * @since 6
     */
    bool Enable(const SleTransport transport) const;

    /**
     * @brief Set all profile services states as turn on.
     *
     * @param transport Adapter transport.
     * @since 6
     */
    void OnAllEnabled(const SleTransport transport) const;

    /**
     * @brief Disable profile services.
     *
     * @param transport Adapter transport.
     * @return Returns <b>true</b> if the operation is successful;
     *         returns <b>false</b> if the operation fails.
     * @since 6
     */
    bool Disable(const SleTransport transport) const;

    /**
     * @brief Set all profile services states as turn off.
     *
     * @param transport Adapter transport.
     * @since 6
     */
    void OnAllDisabled(const SleTransport transport) const;

    /**
     * @brief Get profile service ID list.
     *
     * @return Returns vector of enabled profile services ID.
     * @since 6
     */
    std::vector<uint32_t> GetProfileServicesList() const override;

    /**
     * @brief Set system support profile service list.
     */
    void SetSystemSupportProfileServices();

    /**
     * @brief Get system support profile service list.
     *
     * @return Returns set of system support profile service.
     * @since 6
     */
    std::unordered_set<SleUuid> GetSystemSupportProfileServices() const;

    /**
     * @brief Get profile service connect state.
     *
     * @param profileID Profile service ID.
     * @return Returns connect state for designated profile service.
     * @since 6
     */
    SleConnectState GetProfileServiceConnectState(const uint32_t profileID) const override;

    /**
     * @brief Get local device supported uuids.
     *
     * @param[out] Vector which use to return support uuids.
     * @since 6
     */
    void GetProfileServicesSupportedUuids(std::vector<std::string> &uuids) const override;
    /**
     * @brief Get all profile services connect state.
     *
     * @return Returns profile services connect state.
     * @since 6
     */
    SleConnectState GetProfileServicesConnectState() const;

    /**
     * @brief Profile service enable complete notify.
     *
     * @param profileID Profile service ID.
     * @param ret Profile service enable operation result.
     * @since 6
     */
    void OnEnable(const std::string &name, bool ret) const;

    /**
     * @brief Profile service disable complete notify.
     *
     * @param profileID Profile service ID.
     * @param ret Profile service disable operation result.
     * @since 6
     */
    void OnDisable(const std::string &name, bool ret) const;

    /**
     * @brief A constructor used to create an <b>ProfileServiceManager</b> instance.
     *
     * @since 6
     */
    ProfileServiceManager();

    /**
     * @brief A destructor used to delete the <b>ProfileServiceManager</b> instance.
     *
     * @since 6
     */
    ~ProfileServiceManager();

private:
    void CreateConfigSupportProfiles() const;
    void CreateClassicProfileServices() const;
    void CreateSleProfileServices() const;

    void EnableProfiles(const SleTransport transport) const;
    void DisableProfiles(const SleTransport transport) const;
    void EnableCompleteProcess(const std::string &name, bool ret) const;
    void DisableCompleteProcess(const std::string &name, bool ret) const;
    void EnableCompleteNotify(const SleTransport transport) const;
    void DisableCompleteNotify(const SleTransport transport) const;
    bool IsAllEnabled(const SleTransport transport) const;
    bool IsProfilesTurning(const SleTransport transport) const;
    bool IsAllDisabled(const SleTransport transport) const;
    void CheckWaitEnableProfiles(const std::string &name, const SleTransport transport) const;

    SLE_DISALLOW_COPY_AND_ASSIGN(ProfileServiceManager);
    DECLARE_IMPL();
};
}  // namespace Sle
}  // namespace OHOS

#endif  // PROFILE_SERVICE_MANAGER_H