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
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <memory>

#ifdef NEARLINK_HIAPPEVENT_ENABLE
#include "app_api_metric.h"
#include "app_event_processor_mgr.h"
#include "base_type.h"
#endif

namespace OHOS {
namespace Nearlink {
namespace {
const char* SDK_NAME = "ConnectivityKit";
constexpr int64_t REPORT_NOT_SUPPORT_CODE = -200;
constexpr int ADD_PROCESSOR_TIMEOUT_SECONDS = 5;
}

#ifdef NEARLINK_HIAPPEVENT_ENABLE
class NapiHaManager::NapiHaManagerImpl {
public:
    NapiHaManagerImpl() = default;
    ~NapiHaManagerImpl() = default;

    int64_t GetCurrentTimestamp()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }

    void ReportEvent(const std::string& apiName, bool success, int64_t beginTime, int32_t errCode)
    {
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (m_processorId == -1) {
                AddProcessor();
                auto ret = m_cv.wait_for(lock,
                    std::chrono::seconds(ADD_PROCESSOR_TIMEOUT_SECONDS), [this]() { return m_processorId != -1; });
                if (!ret) {
                    HILOGE("AddProcessor timeout, m_processorId:%{public}lld", (long long)m_processorId);
                    return;
                }
            }
            if (m_processorId == REPORT_NOT_SUPPORT_CODE) {
                return;
            }
        }

        auto endTime = GetCurrentTimestamp();
        auto durationUs = std::max<int64_t>(endTime - beginTime, 0);
        if (durationUs < 0) {
            HILOGE("ReportEvent error params, duration = %{public}lld", (long long)durationUs);
            return;
        }

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
    }

private:
    void AddProcessor()
    {
        ffrt::submit([this]() {
            OHOS::HiviewDFX::HiAppEvent::ReportConfig config;
            config.name = "ha_app_event";
            config.configName = "SDK_OCG";
            int64_t result = OHOS::HiviewDFX::HiAppEvent::AppEventProcessorMgr::AddProcessor(config);
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_processorId = result;
            }
            m_cv.notify_one();

            HILOGI("AddProcessor result %{public}lld", (long long)result);
        }, {}, {});
    }

    std::mutex m_mutex;
    std::condition_variable m_cv;
    int64_t m_processorId{-1};
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

NapiHaManager::NapiHaManager() : m_impl(std::make_shared<NapiHaManagerImpl>()) {}

NapiHaManager::~NapiHaManager() = default;

int64_t NapiHaManager::GetCurrentTimestamp()
{
    return GetInstance().m_impl->GetCurrentTimestamp();
}

void NapiHaManager::ReportEvent(const std::string& apiName, const int64_t beginTime, const int32_t errCode)
{
    bool success = (errCode == NL_NO_ERROR);
    m_impl->ReportEvent(apiName, success, beginTime, errCode);
}

}  // namespace Nearlink
}  // namespace OHOS