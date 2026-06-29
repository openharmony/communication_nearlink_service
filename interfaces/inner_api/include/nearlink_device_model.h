/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef NEARLINK_DEVICE_MODEL_H
#define NEARLINK_DEVICE_MODEL_H

#include <string>

namespace OHOS {
namespace Nearlink {
class DeviceModel {
public:
    /**
     * @brief A constructor used to create an <b>DeviceModel</b> instance.
     *
     * @since 6
     */
    DeviceModel() = default;

    /**
     * @brief A destructor used to delete the <b>DeviceModel</b> instance.
     *
     * @since 6
     */
    ~DeviceModel() = default;

    /**
     * @brief A constructor used to create an <b>DeviceModel</b> instance and set parameters value.
     *
     * @param modelId       The modelId of the remote device.
     * @param subModelId    The subModelId of the remote device.
     * @param iconId        The iconId of the remote device.
     * @since 6
     */
    explicit DeviceModel(const std::string &modelId, const std::string &subModelId, const std::string &iconId)
        : modelId_(modelId), subModelId_(subModelId), iconId_(iconId) {};

    /**
     * @brief A constructor used to create an <b>DeviceModel</b> instance by other instance.
     *
     * @param other Other DeviceModel instance.
     * @since 6
     */
    DeviceModel(const DeviceModel &other) = default;

    /**
     * @brief The operator to compare current instance with others.
     *
     * @param other Other DeviceModel instance.
     * @since 6
     */
    DeviceModel &operator=(const DeviceModel &) = default;

    /**
     * @brief Get DeviceModel modelId string.
     *
     * @return Returns modelId string.
     * @since 6
     */
    const std::string &GetModelId() const
    {
        return modelId_;
    };

    /**
     * @brief Set DeviceModel modelId string.
     *
     * @since 6
     */
    void SetModelId(const std::string &modelId)
    {
        modelId_ = modelId;
    };

    /**
     * @brief Get DeviceModel subModelId string.
     *
     * @return Returns subModelId string.
     * @since 6
     */
    const std::string &GetSubModelId() const
    {
        return subModelId_;
    };

    /**
     * @brief Set DeviceModel subModelId string.
     *
     * @since 6
     */
    void SetSubModelId(const std::string &subModelId)
    {
        subModelId_ = subModelId;
    };

    /**
     * @brief Get DeviceModel iconId string.
     *
     * @return Returns iconId string.
     * @since 6
     */
    const std::string &GetIconId() const
    {
        return iconId_;
    };

    /**
     * @brief Set DeviceModel iconId string.
     *
     * @since 6
     */
    void SetIconId(const std::string &iconId)
    {
        iconId_ = iconId;
    };

    /**
     * @brief Get DeviceModel dev type string.
     *
     * @return Returns dev type string.
     */
    const std::string &GetDevType() const
    {
        return devType_;
    };
 
    /**
     * @brief Set DeviceModel dev type string.
     */
    void SetDevType(const std::string &devType)
    {
        devType_ = devType;
    };

protected:
    /**
     * @brief The modelId of the remote device.
     *
     * @since 6.0.0(19)
     */
    std::string modelId_;

    /**
     * @brief The subModelId of the remote device.
     *
     * @since 6.0.0(19)
     */
    std::string subModelId_;

    /**
     * @brief The iconId of the remote device.
     *
     * @since 6.0.0(19)
     */
    std::string iconId_;

    /**
     * @brief The dev type of the remote device.
     */
    std::string devType_;
};
} // namespace Nearlink
} // namespace OHOS
#endif // NEARLINK_DEVICE_MODEL_H
