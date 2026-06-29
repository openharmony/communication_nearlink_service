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

#ifndef NEARLINK_DEVICE_INFORMATION_H
#define NEARLINK_DEVICE_INFORMATION_H

#include <string>

namespace OHOS {
namespace Nearlink {
class DeviceInformation {
public:
    /**
     * @brief A constructor used to create an <b>DeviceInformation</b> instance.
     *
     * @since 6.1.1(24)
     */
    DeviceInformation() = default;

    /**
     * @brief A destructor used to delete the <b>DeviceInformation</b> instance.
     *
     * @since 6.1.1(24)
     */
    ~DeviceInformation() = default;

    /**
     * @brief A constructor used to create an <b>DeviceInformation</b> instance and set parameters value.
     *
     * @param manufacture   The manufacture of the remote device.
     * @param model         The model of the remote device.
     * @since 6.1.1(24)
     */
    explicit DeviceInformation(const std::string &manufacture, const std::string &model)
        : manufactureData_(manufacture), modelData_(model) {};

    /**
     * @brief A constructor used to create an <b>DeviceInformation</b> instance by other instance.
     *
     * @param other Other DeviceInformation instance.
     * @since 6.1.1(24)
     */
    DeviceInformation(const DeviceInformation &other) = default;

    /**
     * @brief The operator to compare current instance with others.
     *
     * @param other Other DeviceInformation instance.
     * @since 6.1.1(24)
     */
    DeviceInformation &operator=(const DeviceInformation &) = default;

    /**
     * @brief Get DeviceInformation manufacture string.
     *
     * @return Returns manufacture string.
     * @since 6.1.1(24)
     */
    const std::string &GetManufacturerData() const
    {
        return manufactureData_;
    }

    /**
     * @brief Set DeviceInformation manufacture string.
     *
     * @since 6.1.1(24)
     */
    void SetManufacturerData(const std::string &manufactureData)
    {
        manufactureData_ = manufactureData;
    }

    /**
     * @brief Get DeviceInformation model string.
     *
     * @return Returns model string.
     * @since 6.1.1(24)
     */
    const std::string &GetModelData() const
    {
        return modelData_;
    }

    /**
     * @brief Set DeviceInformation model string.
     *
     * @since 6.1.1(24)
     */
    void SetModelData(const std::string &modelData)
    {
        modelData_ = modelData;
    }

protected:
    /**
     * @brief The manufacturerData of the remote device.
     *
     * @since 6.1.1(24)
     */
    std::string manufactureData_ {""};

    /**
     * @brief The modelData of the remote device.
     *
     * @since 6.1.1(24)
     */
    std::string modelData_ {""};
};
} // namespace Nearlink
} // namespace OHOS
#endif // NEARLINK_DEVICE_INFORMATION_H
