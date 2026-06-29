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
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KSTACK, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "log.h"
#include "nearlink_cemap_report_loader.h"

namespace OHOS {
namespace Nearlink {
namespace {
const char* const DFT_EXT_LIB_NAME = "libnearlink_dft_manager_ext.z.so";
const char* const REPORT_FUNC_NAME = "ReportCemapJsonPayload";
}  // namespace

NearlinkCemapReportLoader::NearlinkCemapReportLoader()
    : loader_(DFT_EXT_LIB_NAME)
    , queue_(std::make_unique<ffrt::queue>("nearlink_report_cemap", ffrt::queue_attr().qos(ffrt::qos_utility)))
{}

void NearlinkCemapReportLoader::ReportCemapJsonPayload(const char *jsonPayload)
{
    NL_CHECK_RETURN_LOGD(jsonPayload != nullptr, "[NEARLINK_CEMAP]jsonPayload == nullptr");
    if (!loader_.IsLibraryLoaded()) {
        loader_.OpenLib();
        HILOGI("[NEARLINK_CEMAP] load dft_ext lib");
    }
    ReportFunc report = reinterpret_cast<ReportFunc>(loader_.GetSymbol(std::string(REPORT_FUNC_NAME)));
    NL_CHECK_RETURN(report != nullptr, "[NEARLINK_CEMAP] dlsym %{public}s failed", REPORT_FUNC_NAME);
    report(jsonPayload);
}

void NearlinkCemapReportLoader::ReportCemapJsonPayloadAsync(const char *jsonPayload)
{
    NL_CHECK_RETURN_LOGD(jsonPayload != nullptr, "[NEARLINK_CEMAP]jsonPayload == nullptr");
    queue_->submit([jsonPayloadCopy = std::string(jsonPayload), this]() mutable {
        // 原始 char* 指针在执行时可能已释放
        ReportCemapJsonPayload(jsonPayloadCopy.c_str());
    });
    HILOGI("[NEARLINK_CEMAP] submitted to nearlink_report_cemap");
}

NearlinkCemapReportLoader &NearlinkCemapReportLoader::GetInstance()
{
    static NearlinkCemapReportLoader loader;
    return loader;
}

}  // namespace Nearlink
}  // namespace OHOS
