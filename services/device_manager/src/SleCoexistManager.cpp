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

#include "SleCoexistManager.h"
#include "log_util.h"
#include "SleDefs.h"

namespace OHOS {
namespace Nearlink {
SleCoexistManager::SleCoexistManager()
{
    HILOGD("[CoexistMgr] Constructor");
}

SleCoexistManager::~SleCoexistManager()
{
    HILOGD("[CoexistMgr] Destructor");
}

SleCoexistManager* SleCoexistManager::GetInstance()
{
    static SleCoexistManager instance;
    return &instance;
}

void SleCoexistManager::UpdateConnectionInfo(const CoexistConnInfo& info)
{
    connInfoList_.EnsureInsert(info);
    HILOGD("[CoexistMgr] UpdateInfo lcid=0x%{public}x, interval=0x%{public}x, count=%{public}zu",
        info.lcid, info.interval, connInfoList_.Size());
}

void SleCoexistManager::OnConnectionRemoved(uint16_t lcid)
{
    connInfoList_.FindAndRmv([lcid](const CoexistConnInfo& info) {
        return info.lcid == lcid;
    });
    HILOGD("[CoexistMgr] ConnRemoved lcid=0x%{public}x, count=%{public}zu", lcid, connInfoList_.Size());
}

bool SleCoexistManager::GetConnectionParam(const SLE_Addr_S& addr,
    uint16_t& timeout, uint16_t& maxLatency)
{
    bool found = false;
    connInfoList_.Iterate([&addr, &timeout, &maxLatency, &found](const CoexistConnInfo& info) {
        if (memcmp(&info.addr, &addr, sizeof(SLE_Addr_S)) == 0) {
            timeout = info.timeout;
            maxLatency = info.latency;
            found = true;
            HILOGI("[CoexistMgr] GetParam timeout=0x%{public}x, latency=0x%{public}x",
                   timeout, maxLatency);
        }
    });
    return found;
}

void SleCoexistManager::IterateConnInfo(std::function<void(const CoexistConnInfo&)> callback)
{
    connInfoList_.Iterate([callback](const CoexistConnInfo& info) {
        callback(info);
    });
}

bool SleCoexistManager::HasMultipleConnections()
{
    return connInfoList_.Size() > 1;
}

void SleCoexistManager::ClearAll()
{
    connInfoList_.Clear();
    HILOGD("[CoexistMgr] ClearAll");
}

} // namespace Nearlink
} // namespace OHOS
