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
#ifndef ADAPTER_DEVICE_CONFIG_H
#define ADAPTER_DEVICE_CONFIG_H

#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include "BaseDef.h"
/*
 * @brief The Sle subsystem.
 */
namespace OHOS {
namespace Nearlink {
/**
 * @brief SLE config.
 */
class IAdapterDeviceConfig {
public:
    virtual ~IAdapterDeviceConfig() = default;

    virtual bool CreateFile() = 0;

    /**
     * @brief Load XML Document from specified path.
     * @return true Success Load XML Document.
     * @return false Failed Load XML Document.
     */
    virtual bool Load() = 0;

    /**
     * @brief Reload XML Document from specified path.
     * @return true Success reload XML Document.
     * @return false Failed reload XML Document.
     */
    virtual bool Reload() = 0;

    /**
     * @brief Load XML Document from specified path.
     * @param[in] path XML Document path.
     * @return true Success Load XML Document.
     * @return false Failed Load XML Document.
     */
    virtual bool Save() = 0;

    /**
     * @brief Fsync XML Document from specified path.
     * @return true Success Fsync XML Document.
     * @return false Failed Fsync XML Document.
     */
    virtual bool Fsync() = 0;

    /**
     * @brief Get specified property value.
     *        Value type is int.
     * @param[in] section
     * @param[in] subSection
     * @param[in] property
     * @param[out] value Value type is int.
     * @return true Success get specified property's value.
     * @return false Failed get specified property's value.
     */
    virtual bool GetValue(
        const std::string &section, const std::string &subSection, const std::string &property, int &value) = 0;

    /**
     * @brief Get specified property value.
     *        Value type is string.
     * @param[in] section
     * @param[in] subSection
     * @param[in] property
     * @param[out] value Value type is string.
     * @return true Success get specified property's value.
     * @return false Failed get specified property's value.
     */
    virtual bool GetValue(
        const std::string &section, const std::string &subSection, const std::string &property, std::string &value) = 0;

    /**
     * @brief Get specified property value.
     *        Value type is char*.
     * @param[in] section
     * @param[in] subSection
     * @param[in] property
     * @param[out] value Value type is char*.
     * @param[in] valueLen Value length to check value length.
     * @return true Success get specified property's value.
     * @return false Failed get specified property's value.
     */
    virtual bool GetValue(const std::string &section, const std::string &subSection, const std::string &property, 
        char* value, uint8_t valueLen) = 0;

    /**
     * @brief Get specified property value.
     *        Value type is bool.
     * @param[in] section
     * @param[in] subSection
     * @param[in] property
     * @param[out] value Value type is bool.
     * @return true Success get specified property's value.
     * @return false Failed get specified property's value.
     */
    virtual bool GetValue(
        const std::string &section, const std::string &subSection, const std::string &property, bool &value) = 0;

    /**
     * @brief Set specified property value.
     *        Value type is int.
     * @param[in] section
     * @param[in] subSection
     * @param[in] property
     * @param[in] value Value type is const int.
     * @return true Success set specified property's value.
     * @return false Failed set specified property's value.
     */
    virtual bool SetValue(
        const std::string &section, const std::string &subSection, const std::string &property, const int &value) = 0;

    /**
     * @brief Set specified property value.
     *        Value type is string.
     * @param[in] section
     * @param[in] subSection
     * @param[in] property
     * @param[in] value Value type is const string.
     * @return true Success set specified property's value.
     * @return false Failed set specified property's value.
     */
    virtual bool SetValue(const std::string &section, const std::string &subSection, const std::string &property,
        const std::string &value) = 0;

    /**
     * @brief Set specified property value.
     *        Value type is bool.
     * @param[in] section
     * @param[in] subSection
     * @param[in] property
     * @param[in] value Value type is const bool.
     * @return true Success set specified property's value.
     * @return false Failed set specified property's value.
     */
    virtual bool SetValue(
        const std::string &section, const std::string &subSection, const std::string &property, const bool &value) = 0;

    /**
     * @brief Get Address
     * @param[in] section
     * @param[out] subSections
     * @return true Specified section has one or Multiple subSections.
     * @return false Specified section do not has any subSection.
     */
    virtual bool GetSubSections(const std::string &section, std::vector<std::string> &subSections) = 0;

    /**
     * @brief Remove XML document specified section.
     * @param[in] section
     * @param[in] subSection
     * @return true Success remove XML document specified section.
     * @return false Failed remove XML document specified section.
     */
    virtual bool RemoveSection(const std::string &section, const std::string &subSection) = 0;

    /**
     * @brief Remove XML document specified section.
     * @param[in] section
     * @return true Success remove XML document specified section.
     * @return false Failed remove XML document specified section.
     */
    virtual bool RemoveSection(const std::string &section) = 0;

    /**
     * @brief Get specified property value.
     * @param[in] section
     * @param[in] property
     * @param[out] value Int type value.
     * @return true Success get specified property's value.
     * @return false Failed get specified property's value.
     */
    virtual bool GetValue(const std::string &section, const std::string &property, int &value) = 0;

    /**
     * @brief Get specified property value.
     * @param[in] section
     * @param[in] property
     * @param[out] value String type value.
     * @return true Success get specified property's value.
     * @return false Failed get specified property's value.
     */
    virtual bool GetValue(const std::string &section, const std::string &property, std::string &value) = 0;

    /**
     * @brief Get specified property value.
     * @param[in] section
     * @param[in] property
     * @param[out] value Bool type value.
     * @return true Success get specified property's value.
     * @return false Failed get specified property's value.
     */
    virtual bool GetValue(const std::string &section, const std::string &property, bool &value) = 0;

    /**
     * @brief Set specified property value.
     *        Value type is int.
     * @param[in] section
     * @param[in] property
     * @param[in] value Value type is const int.
     * @return true Success set specified property's value.
     * @return false Failed set specified property's value.
     */
    virtual bool SetValue(const std::string &section, const std::string &property, const int &value) = 0;

    /**
     * @brief Set specified property value.
     *        Value type is string.
     * @param[in] section
     * @param[in] property
     * @param[in] value Value type is const string.
     * @return true Success set specified property's value.
     * @return false Failed set specified property's value.
     */
    virtual bool SetValue(const std::string &section, const std::string &property, const std::string &value) = 0;
};

class AdapterDeviceConfig : public IAdapterDeviceConfig {
public:
    /**
     * @brief Get the Instance object
     * @return IAdapterConfig*
     */
    static IAdapterDeviceConfig *GetInstance();

    bool CreateFile() override;

    /**
     * @brief Load XML Document from specified path.
     * @return true Success Load XML Document.
     * @return false Failed Load XML Document.
     */
    bool Load() override;

    /**
     * @brief Reload XML Document from specified path.
     * @return true Success reload XML Document.
     * @return false Failed reload XML Document.
     */
    bool Reload() override;

    /**
     * @brief Load XML Document from specified path.
     * @param[in] path XML Document path.
     * @return true Success Load XML Document.
     * @return false Failed Load XML Document.
     */
    bool Save() override;

    /**
     * @brief Fsync XML Document from specified path.
     * @return true Success Fsync XML Document.
     * @return false Failed Fsync XML Document.
     */
    bool Fsync() override;
    
    /**
     * @brief Get specified property value.
     *        Value type is int.
     * @param[in] section
     * @param[in] subSection
     * @param[in] property
     * @param[out] value Value type is int.
     * @return true Success get specified property's value.
     * @return false Failed get specified property's value.
     */
    bool GetValue(
        const std::string &section, const std::string &subSection, const std::string &property, int &value) override;

    /**
     * @brief Get specified property value.
     *        Value type is string.
     * @param[in] section
     * @param[in] subSection
     * @param[in] property
     * @param[out] value Value type is string.
     * @return true Success get specified property's value.
     * @return false Failed get specified property's value.
     */
    bool GetValue(const std::string &section, const std::string &subSection, const std::string &property,
        std::string &value) override;

    /**
     * @brief Get specified property value.
     *        Value type is char*.
     * @param[in] section
     * @param[in] subSection
     * @param[in] property
     * @param[out] value Value type is char*.
     * @param[in] valueLen Value length to check value length.
     * @return Success get specified property's value return true, else return false.
     */
    bool GetValue(const std::string &section, const std::string &subSection, const std::string &property,
        char* value, uint8_t valueLen) override;

    /**
     * @brief Get specified property value.
     *        Value type is bool.
     * @param[in] section
     * @param[in] subSection
     * @param[in] property
     * @param[out] value Value type is bool.
     * @return true Success get specified property's value.
     * @return false Failed get specified property's value.
     */
    bool GetValue(
        const std::string &section, const std::string &subSection, const std::string &property, bool &value) override;

    /**
     * @brief Set specified property value.
     *        Value type is int.
     * @param[in] section
     * @param[in] subSection
     * @param[in] property
     * @param[in] value Value type is const int.
     * @return true Success set specified property's value.
     * @return false Failed set specified property's value.
     */
    bool SetValue(const std::string &section, const std::string &subSection, const std::string &property,
        const int &value) override;

    /**
     * @brief Set specified property value.
     *        Value type is string.
     * @param[in] section
     * @param[in] subSection
     * @param[in] property
     * @param[in] value Value type is const string.
     * @return true Success set specified property's value.
     * @return false Failed set specified property's value.
     */
    bool SetValue(const std::string &section, const std::string &subSection, const std::string &property,
        const std::string &value) override;

    /**
     * @brief Set specified property value.
     *        Value type is bool.
     * @param[in] section
     * @param[in] subSection
     * @param[in] property
     * @param[in] value Value type is const bool.
     * @return true Success set specified property's value.
     * @return false Failed set specified property's value.
     */
    bool SetValue(const std::string &section, const std::string &subSection, const std::string &property,
        const bool &value) override;

    /**
     * @brief Get Address
     * @param[in] section
     * @param[out] subSections
     * @return true Specified section has one or multiple subSections.
     * @return false Specified section do not has any subSection.
     */
    bool GetSubSections(const std::string &section, std::vector<std::string> &subSections) override;

    /**
     * @brief Remove XML document specified section.
     * @param[in] section
     * @param[in] subSection
     * @return true Success remove XML document specified section.
     * @return false Failed remove XML document specified section.
     */
    bool RemoveSection(const std::string &section, const std::string &subSection) override;

    /**
     * @brief Remove XML document specified section.
     * @param[in] section
     * @return true Success remove XML document specified section.
     * @return false Failed remove XML document specified section.
     */
    bool RemoveSection(const std::string &section) override;

    /**
     * @brief Get specified property value.
     * @param[in] section
     * @param[in] property
     * @param[out] value Int type value.
     * @return true Success get specified property's value.
     * @return false Failed get specified property's value.
     */
    bool GetValue(const std::string &section, const std::string &property, int &value) override;

    /**
     * @brief Get specified property value.
     * @param[in] section
     * @param[in] property
     * @param[out] value String type value.
     * @return true Success get specified property's value.
     * @return false Failed get specified property's value.
     */
    bool GetValue(const std::string &section, const std::string &property, std::string &value) override;

    /**
     * @brief Get specified property value.
     * @param[in] section
     * @param[in] property
     * @param[out] value Bool type value.
     * @return true Success get specified property's value.
     * @return false Failed get specified property's value.
     */
    bool GetValue(const std::string &section, const std::string &property, bool &value) override;

    /**
     * @brief Set specified property value.
     *        Value type is int.
     * @param[in] section
     * @param[in] property
     * @param[in] value Value type is const int.
     * @return true Success set specified property's value.
     * @return false Failed set specified property's value.
     */
    bool SetValue(const std::string &section, const std::string &property, const int &value) override;

    /**
     * @brief Set specified property value.
     *        Value type is string.
     * @param[in] section
     * @param[in] property
     * @param[in] value Value type is const string.
     * @return true Success set specified property's value.
     * @return false Failed set specified property's value.
     */
    bool SetValue(const std::string &section, const std::string &property, const std::string &value) override;

private:
    /**
     * @brief Construct a new Adapter Config object
     */
    AdapterDeviceConfig();

    /**
     * @brief Destroy the Adapter Config object
     */
    ~AdapterDeviceConfig() override;

    std::mutex mutex_ {};
    static AdapterDeviceConfig *deviceConfigInstance;
    DECLARE_IMPL();
};
}  // namespace Sle
}  // namespace OHOS

#endif // ADAPTER_DEVICE_CONFIG_H