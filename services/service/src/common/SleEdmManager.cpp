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

#include <algorithm>
#include <fstream>
#include <sstream>
#include "cJSON.h"
#include "log.h"
#include "SleEdmManager.h"
#include "nearlink_datashare_helper.h"
#include "nearlink_permission_manager.h"
#include "ThreadUtil.h"

namespace OHOS {
namespace Nearlink {
namespace {
constexpr int MAX_ACCOUNT_SIZE = 1000;
constexpr int MAX_PROFILE_SIZE = 500;
const char *const NEARLINK_ACCOUNT_FORBIDDENLIST_CHANGED_EVENT = "usual.event.EDM_CONFIG_CHANGED";
const char *const NEARLINK_ACCOUNT_FORBIDDEN_CONFIG_FILEPATH =
    "/data/service/el1/public/edm/config/system/all/nearlink/config.json";
}  // namespace

std::shared_ptr<SleEdmManager> SleEdmManager::GetInstance()
{
    static std::shared_ptr<SleEdmManager> instance = std::make_shared<SleEdmManager>();
    return instance;
}

void SleEdmManager::Init()
{
    NL_CHECK_RETURN(!isInit_, "[EDM MODE]:Already Init");
    int ret = UpdateAccountForbiddenlist();
    HILOGI("EDM MODE]:GetForbiddenlist ret: %{public}d", ret);

    EventFwk::MatchingSkills matchingSkills;
    matchingSkills.AddEvent(NEARLINK_ACCOUNT_FORBIDDENLIST_CHANGED_EVENT);
    EventFwk::CommonEventSubscribeInfo subscriberInfo(matchingSkills);
    subscriberInfo.SetThreadMode(EventFwk::CommonEventSubscribeInfo::COMMON);
    subscriberInfo.SetPermission("ohos.permission.MANAGE_EDM_POLICY");

    auto func = [ptr = weak_from_this()](const OHOS::EventFwk::CommonEventData &data) {
        // Work in CommonEvent thread, need post task first.
        DoInServiceManagerThread([ptr]() -> void {
            auto updateEdmListPrt = ptr.lock();
            if (updateEdmListPrt == nullptr) {
                HILOGE("updateEdmListPrt is nullptr");
                return;
            }
            updateEdmListPrt->OnEdmListChanged();
        });
    };
    accountDisallowedEventSubscriber_ =
        std::make_shared<SleCommonEventSubscriber>(subscriberInfo, NEARLINK_ACCOUNT_FORBIDDENLIST_CHANGED_EVENT, func);
    if (!EventFwk::CommonEventManager::SubscribeCommonEvent(accountDisallowedEventSubscriber_)) {
        HILOGE("[EDM MODE]:SubscribeCommonEvent device forbiddenlist fail.");
    }

    isInit_ = true;
}

bool SleEdmManager::IsAllowedConnect(const std::string &profile)
{
    int osAccountId = NearlinkDataShareHelper::GetInstance().GetActiveOsAccountIds();
    HILOGI("[EDM MODE]:GetActiveOsAccountId %{public}d", osAccountId);
    std::vector<std::string> forbiddenlist;
    bool hasKey = accountForbiddenlist_.GetValue(std::to_string(osAccountId), forbiddenlist);
    if (NearLinkPermissionManager::IsNativeCaller() || NearLinkPermissionManager::IsSystemHap()) {
        HILOGI("[EDM MODE]:system app");
        return true;
    } else if (!hasKey) {
        HILOGI("[EDM MODE]:osAccountId not found");
        return true;
    } else if (find(forbiddenlist.begin(), forbiddenlist.end(), profile) == forbiddenlist.end()) {
        HILOGI("[EDM MODE]:profile not in forbiddenlist");
        return false;
    } else {
        HILOGE("[EDM MODE]:profile %{public}s restricted by edm account forbiddenlist.", profile.c_str());
        return false;
    }
}

static void ReadStringFromFile(const std::string &path, std::string &fileStr)
{
    if (path.length() >= PATH_MAX) {
        HILOGE("[EDM MODE]: length invalid");
        return;
    }
    char resolvedPath[PATH_MAX] = {0};
    char *result = realpath(path.c_str(), resolvedPath);
    if (!result) {
        HILOGE("[EDM MODE]: realpath failed");
        return;
    }
    std::ifstream ifs(result);
    if (!ifs.is_open()) {
        HILOGE("[EDM MODE]: ifstream open failed");
        return;
    }
    std::stringstream buffer;
    buffer << ifs.rdbuf();
    fileStr = std::string(buffer.str());
    ifs.close();
}

bool SleEdmManager::UpdateAccountForbiddenlist()
{
    HILOGI("[EDM MODE]:enter");
    accountForbiddenlist_.Clear();
    std::string protocolDenyList;
    ReadStringFromFile(std::string(NEARLINK_ACCOUNT_FORBIDDEN_CONFIG_FILEPATH), protocolDenyList);
    NL_CHECK_RETURN_RET(!protocolDenyList.empty(), false, "[EDM MODE]:get protocolDenyList failed");
    cJSON *root = cJSON_Parse(protocolDenyList.c_str());
    NL_CHECK_RETURN_RET(root, false, "[EDM MODE]:json parse failed");
    cJSON *obj = cJSON_GetObjectItem(root, "ProtocolDenyList");
    if (obj == nullptr) {
        HILOGE("[EDM MODE]:get ProtocolDenyList failed");
        cJSON_Delete(root);
        return false;
    }

    int accountSize = cJSON_GetArraySize(obj);
    if (accountSize > MAX_ACCOUNT_SIZE) {
        HILOGE("[EDM MODE]:accountSize invalid");
        cJSON_Delete(root);
        return false;
    }
    std::vector<std::string> profileForbiddenlist;
    cJSON *childNode = nullptr;
    cJSON_ArrayForEach(childNode, obj) {
        if ((childNode == nullptr) || !cJSON_IsArray(childNode) || (childNode->string == nullptr)) {
            HILOGE("[EDM MODE]: childNode Error!!!");
            continue;
        }
        int profileSize = cJSON_GetArraySize(childNode);
        if (profileSize > MAX_PROFILE_SIZE || profileSize < 0) {
            continue;
        }
        profileForbiddenlist.clear();
        for (int j = 0; j < profileSize; ++j) {
            cJSON *profileNode = cJSON_GetArrayItem(childNode, j);
            if ((profileNode == nullptr) || (!cJSON_IsString(profileNode)) || profileNode->valuestring == nullptr) {
                HILOGE("[EDM MODE]:profile Node error");
                continue;
            }
            std::string profile = cJSON_GetStringValue(profileNode);
            if (profile.size() == 0) {
                continue;
            }
            std::transform(profile.begin(), profile.end(), profile.begin(), ::toupper);
            profileForbiddenlist.push_back(profile);
        }
        accountForbiddenlist_.EnsureInsert(childNode->string, profileForbiddenlist);
    }
    cJSON_Delete(root);
    return true;
}

void SleEdmManager::OnEdmListChanged()
{
    HILOGI("[EDM MODE]:OnReceiveEvent forbiddenlist changed");
    if (!UpdateAccountForbiddenlist()) {
        HILOGE("[EDM MODE]:UpdateAccountForbiddenlist failed");
    }
}
}  // namespace Nearlink
}  // namespace OHOS