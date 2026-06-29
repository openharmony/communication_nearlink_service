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

#ifndef NEARLINK_CEMAP_REPORT_LOADER_H
#define NEARLINK_CEMAP_REPORT_LOADER_H

#include "ffrt.h"
#include "DynamicLibraryLoader.h"

namespace OHOS {
namespace Nearlink {

class NearlinkCemapReportLoader {
public:
    using ReportFunc = void(*)(const char *);

    NearlinkCemapReportLoader();
    ~NearlinkCemapReportLoader() = default;

    void ReportCemapJsonPayload(const char *jsonPayload);
    void ReportCemapJsonPayloadAsync(const char *jsonPayload);

    static NearlinkCemapReportLoader &GetInstance();

private:
    CDynamicLibraryLoader loader_;
    std::unique_ptr<ffrt::queue> queue_;
};

}  // namespace Nearlink
}  // namespace OHOS
#endif // NEARLINK_CEMAP_REPORT_LOADER_H
