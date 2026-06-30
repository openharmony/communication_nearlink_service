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

#ifndef MANUFACTURER_ABILITY_LOADER_H
#define MANUFACTURER_ABILITY_LOADER_H

#include <array>
#include <cstdint>
#include <string>
#include "IManufacturerAbilityManager.h"
#include "DynamicLibraryLoader.h"
#include "raw_address.h"
#include "nearlink_def.h"

namespace OHOS {
namespace Nearlink {
/**
 * @brief 厂商能力加载器
 * @note 基于 CxxDynamicLibraryLoader 实现，支持状态机管理和延迟卸载
 */
class ManufacturerAbilityLoader : public CxxDynamicLibraryLoader<IManufacturerAbilityManager> {
public:
    static constexpr const char* DEFAULT_LIB_NAME = "libnearlink_manu_ability.z.so";
    static constexpr const char* DEFAULT_LIB_CREATE_FUNC_NAME = "CreateManufacturerAbilityManager";
    static constexpr const char* DEFAULT_LIB_DESTROY_FUNC_NAME = "DestroyManufacturerAbilityManager";
    // 延迟卸载时间：5 分钟
    static constexpr uint32_t DEFAULT_UNLOAD_TIMER_MS = 300000;

    /**
     * @brief 获取单例实例
     * @return 单例引用
     */
    static ManufacturerAbilityLoader& GetInstance();

    /**
     * @brief 加载动态库
     */
    void Load();

    /**
     * @brief 卸载动态库
     */
    void Unload();

    /**
     * @brief 获取本端能力位图
     * @return 能力位图
     */
    std::array<uint8_t, SLE_MANU_ABILITY_LEN> GetLocalAbility();

    /**
     * @brief 过滤对端能力位图
     * @param ability 对端能力位图
     */
    void FilterAbility(std::array<uint8_t, SLE_MANU_ABILITY_LEN>& ability);

    /**
     * @brief 检查本端是否支持特定能力
     * @param index 能力索引
     * @return true/false
     */
    bool CheckAbility(uint8_t index);

    /**
     * @brief 获取对端设备的能力值
     * @param device 设备地址
     * @param index 能力索引
     * @return true/false
     */
    bool GetAbilityValue(const RawAddress& device, uint8_t index);

    /**
     * @brief 获取能力索引值
     * @param abilityName 能力名称字符串
     * @return 能力索引值，-1 表示未找到
     */
    int GetAbilityIndex(const std::string& abilityName);

    /**
     * @brief 设置本端能力位图的某一位
     * @param index 能力索引
     * @param enabled 是否启用
     * @return true 表示设置成功，false 表示索引无效或设置失败
     */
    bool SetLocalAbility(uint8_t index, bool enabled);

private:
    ManufacturerAbilityLoader();
    ~ManufacturerAbilityLoader() override = default;
    ManufacturerAbilityLoader(const ManufacturerAbilityLoader&) = delete;
    ManufacturerAbilityLoader& operator=(const ManufacturerAbilityLoader&) = delete;
};

} // namespace Nearlink
} // namespace OHOS

#endif // MANUFACTURER_ABILITY_LOADER_H
