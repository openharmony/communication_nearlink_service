/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include "SleFeature.h"
#include "nlstk_cfgdb_api.h"
#include <mutex>
#include "log_util.h"

namespace OHOS {
namespace Nearlink {
std::mutex g_featureInstanceMutex;
NLSTK_CfgdbLocalFeatures_S g_localFeature = { 0, { 0 }};

bool SleFeature::IsRangSupported()
{
    std::lock_guard<std::mutex> lock(g_featureInstanceMutex);
    if ((g_localFeature.feats[NEARLINK_FEATURE_INDEX_8] & NEARLINK_FEATURE_RANGE) == NEARLINK_FEATURE_RANGE) {
        HILOGI("[SleFeature] NEARLINK_FEATURE_RANGE support");
        return true;
    }
    return false;
}

void SleFeature::SetLocalFeature(NLSTK_CfgdbLocalFeatures_S *param)
{
    NL_CHECK_RETURN(param, "[SleFeature] param err");
    std::lock_guard<std::mutex> lock(g_featureInstanceMutex);
    if (memcpy_s(&g_localFeature, sizeof(NLSTK_CfgdbLocalFeatures_S),
        param, sizeof(NLSTK_CfgdbLocalFeatures_S)) != EOK) {
        HILOGE("[SleFeature] memcpy_s err");
        return;
    }
}

bool SleFeature::IsFrameFourSupported()
{
    std::lock_guard<std::mutex> lock(g_featureInstanceMutex);
    if ((g_localFeature.feats[NEARLINK_FEATURE_INDEX_0] & NEARLINK_FRAME_4_CAP_BIT) == NEARLINK_FRAME_4_CAP_BIT) {
        HILOGI("[SleFeature] NEARLINK_FRAME_4_CAP support");
        return true;
    }
    return false;
}

} // Nearlink
} // OHOS