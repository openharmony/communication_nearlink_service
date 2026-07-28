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

#include "napi_ha_manager.h"
#include "log_util.h"
#include "nearlink_errorcode.h"
#include "ffrt_inner.h"
#include "napi_nearlink_utils.h"

#ifdef NEARLINK_HIAPPEVENT_ENABLE
#include "app_api_metric.h"
#include "app_event_processor_mgr.h"
#include "base_type.h"
#endif

namespace OHOS {
namespace Nearlink {
namespace {
const char* SDK_NAME = "ConnectivityKit";
}

#ifdef NEARLINK_HIAPPEVENT_ENABLE
class NapiHaManager::NapiHaManagerImpl {
public:
    NapiHaManagerImpl()
    {
        AddProcessor();
    };
    ~NapiHaManagerImpl() = default;

    int64_t GetCurrentTimestamp()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }

    void ReportEvent(const std::string& apiName, bool success, int64_t beginTime, int32_t errCode)
    {
        int64_t endTime = GetCurrentTimestamp();
        NL_CHECK_RETURN(endTime >= beginTime, "invalid beginTime.");
        int64_t durationUs = std::min(endTime - beginTime, static_cast<int64_t>(INT_MAX));
        ffrt::submit([apiName, success, durationUs, errCode]() {
            OHOS::HiviewDFX::HiAppEvent::ApiInfo apiInfo;
            apiInfo.kit = SDK_NAME;
            apiInfo.api = apiName;

            OHOS::HiviewDFX::HiAppEvent::ApiMetric metric;
            metric.errCode = errCode;
            metric.duration = static_cast<int>(durationUs);
            metric.successful = success;

            int ret = OHOS::HiviewDFX::HiAppEvent::ReportApiMetric(apiInfo, metric);
            if (ret != 0) {
                HILOGE("ReportApiMetric failed, apiName:%{public}s, ret:%{public}d", apiName.c_str(), ret);
            }
        }, {}, {});
    }

private:
    void AddProcessor()
    {
        ffrt::submit([this]() {
            OHOS::HiviewDFX::HiAppEvent::ReportConfig config;
            config.name = "ha_app_event";
            config.configName = "SDK_OCG";
            int64_t result = OHOS::HiviewDFX::HiAppEvent::AppEventProcessorMgr::AddProcessor(config);
            HILOGI("AddProcessor result %{public}lld", static_cast<long long>(result));
        }, {}, {});
    }
};

#else

class NapiHaManager::NapiHaManagerImpl {
public:
    NapiHaManagerImpl()
    {
        HILOGW("Nearlink hiappevent is not enabled, reporting disabled");
    }
    ~NapiHaManagerImpl() = default;

    int64_t GetCurrentTimestamp()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }

    void ReportEvent(const std::string& apiName, bool success, int64_t beginTime, int32_t errCode)
    {}
};
#endif

NapiHaManager& NapiHaManager::GetInstance()
{
    static NapiHaManager instance;
    return instance;
}

NapiHaManager::NapiHaManager() : impl_(std::make_shared<NapiHaManagerImpl>()) {}

NapiHaManager::~NapiHaManager() = default;

int64_t NapiHaManager::GetCurrentTimestamp()
{
    return GetInstance().impl_->GetCurrentTimestamp();
}

void NapiHaManager::ReportEvent(const std::string& apiName, const int64_t beginTime, const int32_t errCode)
{
    bool success = (errCode == NL_NO_ERROR);
    impl_->ReportEvent(apiName, success, beginTime, errCode);
}

}  // namespace Nearlink
}  // namespace OHOS