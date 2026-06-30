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

#ifndef OHOS_NEARLINK_SLE_DATA_SHARE_HELPER_H
#define OHOS_NEARLINK_SLE_DATA_SHARE_HELPER_H

#include <singleton.h>
#include "datashare_helper.h"
#include "nearlink_errorcode.h"
#include "uri.h"

namespace OHOS {
namespace Nearlink {

class NearlinkDataShareHelper : public Singleton<NearlinkDataShareHelper> {
public:
    NearlinkDataShareHelper() = default;
    ~NearlinkDataShareHelper() = default;
    int32_t GetSwitchState();
    bool GetAirplaneModeState();
    bool GetCollaborationState();
    bool GetHiviewUeState();
    static std::string GetLocalDeviceName();
    bool RegisterNameChangeObserver() const;
    bool RegisterCollaborationChangeObserver() const;
    bool RegisterHiviewUeChangeObserver() const;
    int32_t GetActiveOsAccountIds() const;
#ifdef WATCH_STANDARD
    void SaveNearlinkSwitchStatus(const std::string &sleState);
    void SaveNearlinkCarkeyName(const std::string &sleCarkeyName);
#endif
private:
    std::shared_ptr<DataShare::DataShareHelper> CreateDataShareHelperInstance() const;
    std::shared_ptr<DataShare::DataShareHelper> CreateDataShareHelper();
    bool GetValue(Uri &uri, const std::string &key, std::string &value);
    void GetTruncationName(std::string &localName);
    int GetUTF8StringLength(const char firstByte);
    int GetValidUTF8StringLength(const std::string &localName);
    bool GetSettingDeviceName(std::string &localName);
};

} // namespace Nearlink
} // namespace OHOS
#endif // OHOS_NEARLINK_SLE_DATA_SHARE_HELPER_H