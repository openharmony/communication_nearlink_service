/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "SleKiaManager.h"
#include "log.h"
#include "nearlink_errorcode.h"
#include "syspara/parameters.h"
#include "cJSON.h"
#include "sg_collect_client.h"
namespace OHOS {
namespace Nearlink {
namespace {
constexpr const char SYS_PARAM_SERVICE_FORCE_ENABLE[] = "const.pc_security.fileguard_force_enable";
constexpr const int64_t EVENT_ID = 0x01C00000A;
}

SleKiaManager &SleKiaManager::GetInstance()
{
    static SleKiaManager ins;
    return ins;
}

static int64_t GetTimestamp()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

int32_t SleKiaManager::UpdateRefusePolicy(
    const int32_t protocolType, const int32_t pid, const int64_t prohibitedSecondsTime)
{
    std::string devicesCheck = system::GetParameter(SYS_PARAM_SERVICE_FORCE_ENABLE, "");
    if (devicesCheck != "true") {
        HILOGE("hasNotCheckedDevice, check vendor devices=%{public}s", devicesCheck.c_str());
        return NL_ERR_SYSTEM_PERMISSION_FAILED;
    }
    HILOGI("add policy-protocolType:%{public}d, pid:%{public}d, prohibitedSecondsTime:%{public}lu",
        protocolType, pid, prohibitedSecondsTime);
    EraseRefuseTimer(protocolType, pid);
    auto timeoutFunc = [protocolType, pid, this]() -> void {
        this->EraseRefuseTimer(protocolType, pid);
    };

    std::shared_ptr<NearlinkTimer> prohibitedTimer = std::make_shared<NearlinkTimer>(timeoutFunc);
    prohibitedTimer->Start(prohibitedSecondsTime, true);

    std::string keyStr = std::to_string(protocolType) + std::to_string(pid);
    refusePolicyMap_.Insert(keyStr, prohibitedTimer);
    HILOGI("refusePolicyMap_ update, map size:%{public}d", refusePolicyMap_.Size());
    return NL_NO_ERROR;
}

void SleKiaManager::EraseRefuseTimer(const int32_t protocolType, const int32_t pid)
{
    auto func = [protocolType, pid](const std::string &key, std::shared_ptr<NearlinkTimer> &value) -> bool {
        if (key == std::to_string(protocolType) + std::to_string(pid)) {
            value->Stop();
            value = nullptr;
            return true;
        }
        return false;
    };
    refusePolicyMap_.IterateAndRmv(func);
}

bool SleKiaManager::ShouldRefuseConnect(const int32_t protocolType, const int32_t pid)
{
    HILOGI("enter check-Type:%{public}d, pid:%{public}d", protocolType, pid);
    if (refusePolicyMap_.FindIf(std::to_string(protocolType) + std::to_string(pid))) {
        HILOGI("ShouldRefuseConnect-Type:%{public}d, pid:%{public}d", protocolType, pid);
        ReportRefuseInfo(pid);
        return true;
    }
    return false;
}

void SleKiaManager::ReportRefuseInfo(int32_t pid)
{
    int64_t refuseTime = GetTimestamp();
    cJSON *outJson = cJSON_CreateObject();
    if (outJson == nullptr) {
        HILOGE("ReportRefuseInfo json object created error");
        return;
    }
    cJSON_AddNumberToObject(outJson, "timestamp", refuseTime);
    cJSON_AddStringToObject(outJson, "type", "nearlink_send");
    cJSON_AddStringToObject(outJson, "ftype", "1");
    cJSON_AddNumberToObject(outJson, "process_pid", pid);
    char *jsonContent = cJSON_PrintUnformatted(outJson);
    if (jsonContent == nullptr) {
        HILOGE("ReportRefuseInfo print json unformatted error");
        cJSON_Delete(outJson);
        outJson = nullptr;
        return;
    }
    std::string content = std::string(jsonContent);
    cJSON_free(jsonContent);
    cJSON_Delete(outJson);
    outJson = nullptr;
    std::shared_ptr<Security::SecurityGuard::EventInfo> eventInfo =
        std::make_shared<Security::SecurityGuard::EventInfo>(EVENT_ID, "1.0", content);
    int ret = Security::SecurityGuard::NativeDataCollectKit::ReportSecurityInfo(eventInfo);
    HILOGI("report pid:%{public}d, refuseTime:%{public}lld, ret:%{public}d", pid, refuseTime, ret);
}
}  // namespace Nearlink
}  // namespace OHOS