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

#ifndef I_MANUFACTURER_ABILITY_MANAGER_H
#define I_MANUFACTURER_ABILITY_MANAGER_H

#include <array>
#include <cstdint>
#include <string>

#include "raw_address.h"
#include "nearlink_def.h"

namespace OHOS {
namespace Nearlink {

/**
 * @brief Manufacturer Ability Name Constants
 * @note 用于 GetAbilityIndex 接口的能力名称字符串常量
 */
constexpr const char* MANU_ABILITY_FENCE_5G = "FENCE_5G";
constexpr const char* MANU_ABILITY_ADAPTIVE_SWITCH_5G = "ADAPTIVE_SWITCH_5G";
constexpr const char* MANU_ABILITY_QOSID_CONFIG = "QOSID_CONFIG";
constexpr const char* MANU_ABILITY_ASC_START_PLAYING_MERGE = "ASC_START_PLAYING_MERGE";
constexpr const char* MANU_ABILITY_DUAL_EAR_HIGH_QUALITY_RECORDING = "DUAL_EAR_HIGH_QUALITY_RECORDING";
constexpr const char* MANU_ABILITY_DUAL_EAR_KARAOKE = "DUAL_EAR_KARAOKE";
constexpr const char* MANU_ABILITY_VOICE_CALL_AUTORATE = "VOICE_CALL_AUTORATE";
constexpr const char* MANU_ABILITY_VOICE_CALL_FRAME_FOUR = "VOICE_CALL_FRAME_FOUR";

/**
 * @brief 厂商能力管理器接口
 * @note 公开接口定义，不包含枚举定义
 */
class IManufacturerAbilityManager {
public:
    virtual ~IManufacturerAbilityManager() = default;

    /**
     * @brief 初始化管理器
     */
    virtual void Init() = 0;

    /**
     * @brief 反初始化管理器
     */
    virtual void UnInit() = 0;

    /**
     * @brief 获取本端能力位图
     * @return 本端能力位图
     */
    virtual std::array<uint8_t, SLE_MANU_ABILITY_LEN> GetLocalAbility() const = 0;

    /**
     * @brief 过滤对端能力位图
     * @param ability 对端能力位图 (引用，会被修改)
     */
    virtual void FilterAbility(std::array<uint8_t, SLE_MANU_ABILITY_LEN>& ability) const = 0;

    /**
     * @brief 检查本端是否支持特定能力
     * @param index 能力索引
     * @return true/false
     */
    virtual bool CheckAbility(uint8_t index) const = 0;

    /**
     * @brief 获取对端设备的能力值
     * @param device 设备地址
     * @param index 能力索引
     * @return true/false
     */
    virtual bool GetAbilityValue(const RawAddress& device, uint8_t index) = 0;

    /**
     * @brief 获取能力索引值
     * @param abilityName 能力名称字符串
     * @return 能力索引值，-1 表示未找到
     */
    virtual int GetAbilityIndex(const std::string& abilityName) = 0;

    /**
     * @brief 设置本端能力位图的某一位
     * @param index 能力索引
     * @param enabled 是否启用
     * @return true 表示设置成功，false 表示索引无效或设置失败
     */
    virtual bool SetLocalAbility(uint8_t index, bool enabled) = 0;
};

} // namespace Nearlink
} // namespace OHOS

#endif // I_MANUFACTURER_ABILITY_MANAGER_H
