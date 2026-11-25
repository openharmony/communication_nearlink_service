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

#ifndef NEARLINK_CCS_MOCK_RECORD_H
#define NEARLINK_CCS_MOCK_RECORD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct MockCcsRecord {
    uint32_t createInstanceCount;
    uint32_t deleteInstanceCount;
    uint32_t callStateUpdateCount;
    uint32_t inOutUpdateCount;
    uint32_t terminateUpdateCount;
    uint32_t callControlResultCount;
    uint32_t authorizeResultCount;
    uint32_t lastCallControlRequestId;
    uint32_t lastCallCount;
    uint32_t lastInOutUserInfoLen;
    uint32_t lastInOutUserAliasLen;
    uint8_t lastInOutCallFlag;
    uint8_t lastInOutCallId;
    uint8_t lastCallFirstId;
};

struct MockCcsRecord MockCcsGetRecord(void);
void MockCcsResetRecord(void);

#ifdef __cplusplus
}
#endif

#endif // NEARLINK_CCS_MOCK_RECORD_H
