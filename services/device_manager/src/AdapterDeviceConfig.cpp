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
#include "AdapterDeviceConfig.h"

#include <fstream>
#include <unistd.h>
#include <fcntl.h>
#include "log.h"
#include "xml_parse.h"

namespace OHOS {
namespace Nearlink {
AdapterDeviceConfig *AdapterDeviceConfig::deviceConfigInstance = nullptr;

struct AdapterDeviceConfig::impl {
    utility::XmlParse parse_ {};
    std::string fileName_ {"sle_device_config.xml"};
    std::string filePath_ {SLE_CONFIG_PATH + fileName_};
};

IAdapterDeviceConfig *AdapterDeviceConfig::GetInstance()
{
    if (deviceConfigInstance == nullptr) {
        static AdapterDeviceConfig instance;
        deviceConfigInstance = &instance;
    }

    return static_cast<IAdapterDeviceConfig *>(deviceConfigInstance);
}

AdapterDeviceConfig::AdapterDeviceConfig() : pimpl(std::make_unique<impl>())
{};

AdapterDeviceConfig::~AdapterDeviceConfig()
{}

bool AdapterDeviceConfig::Load()
{
    std::lock_guard<std::mutex> lg(mutex_);
    if (pimpl->parse_.Load(pimpl->filePath_)) {
        return true;
    }
    return false;
}

bool AdapterDeviceConfig::CreateFile()
{
    std::lock_guard<std::mutex> lg(mutex_);
    HILOGI("Create new file(%{public}s)", pimpl->fileName_.c_str());
    if (!pimpl->parse_.Create(pimpl->filePath_)) {
        HILOGE("Create cfg file failed.");
        return false;
    }
    return true;
}

bool AdapterDeviceConfig::Reload()
{
    std::lock_guard<std::mutex> lg(mutex_);
    int ret = remove(pimpl->filePath_.c_str());
    if (ret == -1) {
        HILOGE("[SleConfig] remove fail, errno:%{public}s", strerror(errno));
        return false;
    }
    return true;
}

bool AdapterDeviceConfig::Save()
{
    std::lock_guard<std::mutex> lg(mutex_);
    return pimpl->parse_.Save();
}

bool AdapterDeviceConfig::Fsync()
{
    HILOGI("[SleConfig] fsync begin.");
    std::lock_guard<std::mutex> lg(mutex_);
    int fd = open(pimpl->filePath_.c_str(), O_RDONLY);
    if (fd < 0) {
        HILOGE("[SleConfig] open fail, errno:%{public}s", strerror(errno));
        return false;
    }
    if (fsync(fd) == -1) {
        HILOGE("[SleConfig] fsync fail.");
    }
    close(fd);
    HILOGI("[SleConfig] fsync end.");
    return true;
}

bool AdapterDeviceConfig::SetValue(const std::string &section, const std::string &property, const int &value)
{
    std::lock_guard<std::mutex> lg(mutex_);
    return pimpl->parse_.SetValue(section, property, value);
}

bool AdapterDeviceConfig::SetValue(const std::string &section, const std::string &property, const std::string &value)
{
    std::lock_guard<std::mutex> lg(mutex_);
    return pimpl->parse_.SetValue(section, property, value);
}

bool AdapterDeviceConfig::GetValue(const std::string &section, const std::string &property, int &value)
{
    std::lock_guard<std::mutex> lg(mutex_);
    return pimpl->parse_.GetValue(section, property, value);
}

bool AdapterDeviceConfig::GetValue(const std::string &section, const std::string &property, std::string &value)
{
    std::lock_guard<std::mutex> lg(mutex_);

    return pimpl->parse_.GetValue(section, property, value);
}

bool AdapterDeviceConfig::GetValue(const std::string &section, const std::string &property, bool &value)
{
    std::lock_guard<std::mutex> lg(mutex_);
    return pimpl->parse_.GetValue(section, property, value);
}

bool AdapterDeviceConfig::SetValue(
    const std::string &section, const std::string &subSection, const std::string &property, const int &value)
{
    std::lock_guard<std::mutex> lg(mutex_);
    return pimpl->parse_.SetValue(section, subSection, property, value);
}
bool AdapterDeviceConfig::SetValue(
    const std::string &section, const std::string &subSection, const std::string &property, const std::string &value)
{
    std::lock_guard<std::mutex> lg(mutex_);
    return pimpl->parse_.SetValue(section, subSection, property, value);
}

bool AdapterDeviceConfig::SetValue(
    const std::string &section, const std::string &subSection, const std::string &property, const bool &value)
{
    std::lock_guard<std::mutex> lg(mutex_);
    return pimpl->parse_.SetValue(section, subSection, property, value);
}

bool AdapterDeviceConfig::GetValue(
    const std::string &section, const std::string &subSection, const std::string &property, int &value)
{
    std::lock_guard<std::mutex> lg(mutex_);
    return pimpl->parse_.GetValue(section, subSection, property, value);
}

bool AdapterDeviceConfig::GetValue(
    const std::string &section, const std::string &subSection, const std::string &property, std::string &value)
{
    std::lock_guard<std::mutex> lg(mutex_);
    return pimpl->parse_.GetValue(section, subSection, property, value);
}

bool AdapterDeviceConfig::GetValue(const std::string &section, const std::string &subSection,
                                   const std::string &property, char *value, uint8_t valueLen)
{
    std::lock_guard<std::mutex> lg(mutex_);
    return pimpl->parse_.GetValue(section, subSection, property, value, valueLen);
}

bool AdapterDeviceConfig::GetValue(
    const std::string &section, const std::string &subSection, const std::string &property, bool &value)
{
    std::lock_guard<std::mutex> lg(mutex_);
    return pimpl->parse_.GetValue(section, subSection, property, value);
}

bool AdapterDeviceConfig::GetSubSections(const std::string &section, std::vector<std::string> &subSections)
{
    std::lock_guard<std::mutex> lg(mutex_);
    return pimpl->parse_.GetSubSections(section, subSections);
}

bool AdapterDeviceConfig::RemoveSection(const std::string &section, const std::string &subSection)
{
    std::lock_guard<std::mutex> lg(mutex_);
    return pimpl->parse_.RemoveSection(section, subSection);
}

bool AdapterDeviceConfig::RemoveSection(const std::string &section)
{
    std::lock_guard<std::mutex> lg(mutex_);
    return pimpl->parse_.RemoveSection(section);
}
}  // namespace Sle
}  // namespace OHOS