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

#include <random>
#include <vector>
#include "log_util.h"
#include "ffrt.h"
#include "nearlink_errorcode.h"
#include "napi_ha_manager.h"

namespace OHOS {
namespace Nearlink {
static const std::string SDK_NAME = "NearLinkKit";
constexpr const int64_t MAX_RAMDOM_VALUE = 999999;

NapiHaManager::NapiHaManager() {}
 
NapiHaManager::~NapiHaManager() {}

NapiHaManager& NapiHaManager::GetInstance()
{
    static NapiHaManager instance;
    return instance;
}

std::string NapiHaManager::GenerateTransId()
{
    std::random_device randSeed;
    std::mt19937 gen(randSeed());
    std::uniform_int_distribution<> dis(0, MAX_RAMDOM_VALUE);
    return std::string("transId_") + std::to_string(dis(gen));
}

int64_t NapiHaManager::GetCurrentTimestamp()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

void NapiHaManager::ReportEvent(std::string apiName, const int64_t beginTime, const int32_t errCode)
{
    int64_t endTime = GetCurrentTimestamp();

    auto task = [apiName, errCode, beginTime, endTime] {
        int result = (errCode == NL_NO_ERROR ? 0 : 1);
        std::string transId = GenerateTransId();
        OHOS::HiviewDFX::HiAppEvent::Event event("api_diagnostic", "api_exec_end", 
            OHOS::HiviewDFX::HiAppEvent::BEHAVIOR);
        event.AddParam("trans_id", transId);
        event.AddParam("api_name", apiName);
        event.AddParam("sdk_name", SDK_NAME);
        event.AddParam("begin_time", beginTime);
        event.AddParam("end_time", endTime);
        event.AddParam("result", result);
        event.AddParam("error_code", errCode);
        int ret = Write(event);
        HILOGD("transId:%{public}s, apiName:%{public}s, sdkName:%{public}s, startTime:%{public}ld, endTime:%{public}ld,"
            "result:%{public}d, errCode:%{public}d, ret:%{public}d",
            transId.c_str(), apiName.c_str(), SDK_NAME.c_str(), beginTime, endTime, result, errCode, ret);
    };
    ffrt::submit(task, {}, {});
}

void NapiHaManager::AddProcessor()
{
    ffrt::submit([] {
        HILOGI("AddProcessor enter");
        OHOS::HiviewDFX::HiAppEvent::ReportConfig config;
        config.name = "ha_app_event";
        config.configName = "SDK_OCG"; // 固定内容，此配置内容由HA确认规格
        HiviewDFX::HiAppEvent::AppEventProcessorMgr::AddProcessor(config);
        HILOGI("AddProcessor end");
    }, {}, {});
}
}  // namespace Nearlink
}  // namespace OHOS