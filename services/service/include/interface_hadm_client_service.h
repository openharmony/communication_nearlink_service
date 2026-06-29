/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef INTERFACE_HADM_CLIENT_H
#define INTERFACE_HADM_CLIENT_H

#include "raw_address.h"
#include "nearlink_hadm_sounding_result.h"

namespace OHOS {
namespace Nearlink {
class InterfaceHadmClientServiceCallback {
public:
    virtual ~InterfaceHadmClientServiceCallback() = default;
    virtual void OnSoundingResult(const RawAddress &addr,
        const NearlinkHadmSoundingResult &result, uint32_t hadmId) = 0;
    virtual void OnSoundingStateChange(const RawAddress &addr, int newState, int errorCode, uint32_t hadmId) = 0;
};

class InterfaceHadmClientService {
public:
    virtual ~InterfaceHadmClientService() = default;
    static InterfaceHadmClientService &GetInstance();
    virtual void RegisterNearlinkHadmClientCallback(std::shared_ptr<InterfaceHadmClientServiceCallback> callback) = 0;
    virtual void DeregisterNearlinkHadmClientCallback() const = 0;
    virtual void StartSounding(uint32_t hadmId, const RawAddress &addr) const = 0;
    virtual void StopSounding(uint32_t hadmId, const RawAddress &addr) const = 0;
    virtual uint32_t AllocHadmId() = 0;
    virtual void RemoveHadmId(uint32_t hadmId);
    virtual void StopSoundingById(uint32_t hadmId) const = 0;
    virtual uint8_t GetHadmFeature() const = 0;
};
} // namespace Nearlink
} // namespace OHOS
#endif // INTERFACE_HADM_CLIENT_H