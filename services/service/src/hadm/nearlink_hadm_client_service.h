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

#ifndef HADM_CLIENT_SERVICE_H
#define HADM_CLIENT_SERVICE_H

#include <set>
#include <future>
#include <utility>
#include "BaseDef.h"
#include "nearlink_types.h"
#include "interface_hadm_client_service.h"
#include "SleDefs.h"

namespace OHOS {
namespace Nearlink {

class HadmClientService : public InterfaceHadmClientService {
public:
    HadmClientService();
    ~HadmClientService();

    static HadmClientService &GetInstance();
    void RegisterNearlinkHadmClientCallback(
        std::shared_ptr<InterfaceHadmClientServiceCallback> callback);
    void DeregisterNearlinkHadmClientCallback() const;
    void StartSounding(uint32_t hadmId, const RawAddress &addr) const;
    void StopSounding(uint32_t hadmId, const RawAddress &addr) const;
    void StopSoundingById(uint32_t hadmId) const;
     uint8_t GetHadmFeature() const;
     uint32_t AllocHadmId();
     void RemoveHadmId(uint32_t hadmId);

private:

    uint8_t GetSoundingState(const RawAddress &device) const;
    uint32_t GetSoundingAddrInfo(RawAddress &device) const;
    void ReportHadmSoundingState(const RawAddress &addr, int state, int errorcode, uint32_t hadmId) const;
    
    void ReportSoundingIQResult(const RawAddress &addr, const NearlinkHadmSoundingResult &result);
    bool CheckCarkeyState() const;
    bool CheckLowLatencyConnection() const;
    std::pair<bool, bool> GetUserPriority(std::pair<std::string, int> info) const;
    void SaveDutData(NearlinkHadmSoundingResult soundingResult);
    void SaveRtdData(NearlinkHadmSoundingResult soundingResult);

    std::weak_ptr<InterfaceHadmClientServiceCallback> callback_;
    DECLARE_IMPL();
    std::set<uint32_t> hadmIds_;
};
} // namespace Nearlink
} // namespace OHOS
#endif // HID_HOST_SERVICE_H