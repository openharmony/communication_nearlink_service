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

#include "nearlink_dft_manager_c_mock.h"

#include <mutex>
#include <unordered_map>

#include "nearlink_dft_manager_c.h"

namespace OHOS {
namespace Nearlink {
namespace Test {
struct MockState {
    std::mutex mtx;
    bool hiviewUeOn = true;
    int reportCalls = 0;
    int cacheCalls = 0;
    int eraseCalls = 0;
    std::unordered_map<int, int> reportCallsByEvent;
    std::unordered_map<int, int> cacheCallsByEvent;
    std::vector<int> lastReportParamIds;
    std::unordered_map<int, std::string> lastReportStrParams;
    std::unordered_map<int, std::string> lastCacheStrParams;

    void Snapshot(const DftParamC params[], size_t size, bool isReport)
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (isReport) {
            lastReportParamIds.clear();
            lastReportStrParams.clear();
        } else {
            lastCacheStrParams.clear();
        }
        for (size_t i = 0; i < size; i++) {
            if (isReport) {
                lastReportParamIds.push_back(params[i].paramId);
            }
            if (params[i].t == DFT_STRING && params[i].u.v.s != nullptr) {
                if (isReport) {
                    lastReportStrParams[params[i].paramId] = std::string(params[i].u.v.s);
                } else {
                    lastCacheStrParams[params[i].paramId] = std::string(params[i].u.v.s);
                }
            }
        }
    }
};

MockState &Mock()
{
    static MockState state;
    return state;
}

void DftMockReset()
{
    auto &mock = Mock();
    std::lock_guard<std::mutex> lock(mock.mtx);
    mock.hiviewUeOn = true;
    mock.reportCalls = 0;
    mock.cacheCalls = 0;
    mock.eraseCalls = 0;
    mock.reportCallsByEvent.clear();
    mock.cacheCallsByEvent.clear();
    mock.lastReportParamIds.clear();
    mock.lastReportStrParams.clear();
    mock.lastCacheStrParams.clear();
}

void DftMockSetHiviewUeState(bool isOn)
{
    auto &mock = Mock();
    std::lock_guard<std::mutex> lock(mock.mtx);
    mock.hiviewUeOn = isOn;
}

int DftMockReportCalls()
{
    auto &mock = Mock();
    std::lock_guard<std::mutex> lock(mock.mtx);
    return mock.reportCalls;
}

int DftMockCacheCalls()
{
    auto &mock = Mock();
    std::lock_guard<std::mutex> lock(mock.mtx);
    return mock.cacheCalls;
}

int DftMockEraseCalls()
{
    auto &mock = Mock();
    std::lock_guard<std::mutex> lock(mock.mtx);
    return mock.eraseCalls;
}

int DftMockReportCallsOf(int eventId)
{
    auto &mock = Mock();
    std::lock_guard<std::mutex> lock(mock.mtx);
    auto it = mock.reportCallsByEvent.find(eventId);
    return it == mock.reportCallsByEvent.end() ? 0 : it->second;
}

bool DftMockLastReportHasParam(int paramId)
{
    auto &mock = Mock();
    std::lock_guard<std::mutex> lock(mock.mtx);
    for (int id : mock.lastReportParamIds) {
        if (id == paramId) {
            return true;
        }
    }
    return false;
}

int DftMockCacheCallsOf(int eventId)
{
    auto &mock = Mock();
    std::lock_guard<std::mutex> lock(mock.mtx);
    auto it = mock.cacheCallsByEvent.find(eventId);
    return it == mock.cacheCallsByEvent.end() ? 0 : it->second;
}

std::string DftMockLastReportStrParam(int paramId)
{
    auto &mock = Mock();
    std::lock_guard<std::mutex> lock(mock.mtx);
    auto it = mock.lastReportStrParams.find(paramId);
    return it == mock.lastReportStrParams.end() ? std::string() : it->second;
}

std::string DftMockLastCacheStrParam(int paramId)
{
    auto &mock = Mock();
    std::lock_guard<std::mutex> lock(mock.mtx);
    auto it = mock.lastCacheStrParams.find(paramId);
    return it == mock.lastCacheStrParams.end() ? std::string() : it->second;
}

} // namespace Test
} // namespace Nearlink
} // namespace OHOS

// 链接期覆写 C 上报边界：被测的上层逻辑依赖计数与快照，不再走 ffrt 队列与落盘
void DftManagerStart(void) {}

void DftManagerStop(void) {}

void DftHiviewUeStateChanged(bool isHiviewUeOn)
{
    OHOS::Nearlink::Test::DftMockSetHiviewUeState(isHiviewUeOn);
}

bool DftHiviewUeStateIsOn(void)
{
    auto &mock = OHOS::Nearlink::Test::Mock();
    std::lock_guard<std::mutex> lock(mock.mtx);
    return mock.hiviewUeOn;
}

void DftManagerReport(DftEventEnum eventId, const DftParamC params[], size_t size)
{
    auto &mock = OHOS::Nearlink::Test::Mock();
    {
        std::lock_guard<std::mutex> lock(mock.mtx);
        mock.reportCalls++;
        mock.reportCallsByEvent[eventId]++;
    }
    if (params != nullptr) {
        mock.Snapshot(params, size, true);
    }
}

void DftManagerCache(DftEventEnum eventId, const DftParamC params[], size_t size)
{
    auto &mock = OHOS::Nearlink::Test::Mock();
    {
        std::lock_guard<std::mutex> lock(mock.mtx);
        mock.cacheCalls++;
        mock.cacheCallsByEvent[eventId]++;
    }
    if (params != nullptr) {
        mock.Snapshot(params, size, false);
    }
}

void DftManagerEraseCache(DftEventEnum eventId, const DftParamC keyParams[], size_t size)
{
    auto &mock = OHOS::Nearlink::Test::Mock();
    {
        std::lock_guard<std::mutex> lock(mock.mtx);
        mock.eraseCalls++;
    }
    if (keyParams != nullptr) {
        mock.Snapshot(keyParams, size, true);
    }
}
