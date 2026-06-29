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
#ifndef SLE_PROPERTIES_H
#define SLE_PROPERTIES_H

#include <mutex>
#include <string>
#include <vector>

#include "BaseObserverList.h"
#include "SleConfig.h"
#include "SleDefs.h"
#include "SleInterfaceAdapterSub.h"
#include "sdf_struct.h"

namespace OHOS {
namespace Nearlink {
/**
 * @brief bondable mode define
 * use to
 * GetBondableMode(), SetBondableMode()
 */
enum class BondableMode : int {
    BONDABLE_MODE_OFF = 0x00,
    BONDABLE_MODE_ON = 0x01
};

/**
 * @SleProperties to save and get classic properties.
 */
class SleProperties {
public:
    /**
     * @brief Constructor.
     */
    SleProperties(SleProperties &) = delete;

    /**
     * @brief Constructor.
     */
    SleProperties &operator=(const SleProperties &) = delete;

    /**
     * @brief Get sle Properties instance.
     *
     * @return @c advertiser instance.
     */
    static SleProperties &GetInstance();

    bool SetLocalAddress(const std::string &addr) const;
    SLE_Addr_S GetLocalSleAddress() const;
    bool SetLocalName(const std::string &name) const;
    int SetBondableMode(const int mode) const;
    bool SetIoCapability(const int ioCapability) const;

    int GetBondableMode() const;
    int GetIoCapability() const;
    std::string GetLocalAddress() const;
    std::string GetLocalName() const;

    bool LoadSleConfigInfo() const;

    bool SaveDefaultValues() const;
    bool GetAddrFromController() const;

    void RegisterSleAdapterObserver(IAdapterSleObserver &observer) const;
    void DeregisterSleAdapterObserver(IAdapterSleObserver &observer) const;

private:
    SleProperties();
    ~SleProperties();

    void ReadSleHostInfo() const;
    bool UpdateConfig(int type) const;
    struct impl;
    std::shared_ptr<impl> pimpl = nullptr;
};
}  // namespace Nearlink
}  // namespace OHOS

#endif  // SLE_PROPERTIES_H