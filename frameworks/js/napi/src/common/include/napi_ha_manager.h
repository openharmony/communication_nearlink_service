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

#ifndef NAPI_HA_MANAGER_H
#define NAPI_HA_MANAGER_H

#include <cstring>
#include <cstdint>
#include <memory>

namespace OHOS {
namespace Nearlink {

#define NAPI_HA_EXCEP_REPORT(cond, apiName, beginTime, errCode)                           \
do {                                                                                      \
   if (!(cond)) {                                                                         \
      NapiHaManager::GetInstance().ReportEvent(apiName, beginTime, errCode);              \
   }                                                                                      \
} while (0)

class NapiHaManager {
public:
   static NapiHaManager& GetInstance();
   static int64_t GetCurrentTimestamp();
   void ReportEvent(const std::string& apiName, const int64_t beginTime, const int32_t errCode);

   NapiHaManager(const NapiHaManager &) = delete;
   NapiHaManager& operator=(const NapiHaManager&) = delete;

private:
   NapiHaManager();
   ~NapiHaManager();

   class NapiHaManagerImpl;
   std::shared_ptr<NapiHaManagerImpl> m_impl;
};

}  // namespace Nearlink
}  // namespace OHOS
#endif  // NAPI_HA_MANAGER_H