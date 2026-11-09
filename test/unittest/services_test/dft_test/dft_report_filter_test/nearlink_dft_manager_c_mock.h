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

#ifndef NEARLINK_DFT_MANAGER_C_MOCK_H
#define NEARLINK_DFT_MANAGER_C_MOCK_H

#include <string>
#include <vector>

namespace OHOS {
namespace Nearlink {
namespace Test {

void DftMockReset();
void DftMockSetHiviewUeState(bool isOn);

int DftMockReportCalls();
int DftMockCacheCalls();
int DftMockEraseCalls();
int DftMockReportCallsOf(int eventId);
int DftMockCacheCallsOf(int eventId);

bool DftMockLastReportHasParam(int paramId);
std::string DftMockLastReportStrParam(int paramId);
std::string DftMockLastCacheStrParam(int paramId);

} // namespace Test
} // namespace Nearlink
} // namespace OHOS

#endif // NEARLINK_DFT_MANAGER_C_MOCK_H
