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

#ifndef STACK_DLI_EVENT_MOCK_H
#define STACK_DLI_EVENT_MOCK_H

#include <gmock/gmock.h>
#include <stdint.h>
#include "dli_event.h"

namespace OHOS {
class DliEventMockInterface {
public:
    DliEventMockInterface() {};
    virtual ~DliEventMockInterface() {};
    virtual uint32_t DLI_RegNOCPEventCbk(DLI_RegModuleType module, DLI_NOCPEventCbk cbk) = 0;
    virtual uint32_t DLI_UnregNOCPEventCbk(DLI_RegModuleType module) = 0;
};

class DliEventMock : public DliEventMockInterface {
public:
    DliEventMock();
    ~DliEventMock() override;
    MOCK_METHOD(uint32_t, DLI_RegNOCPEventCbk, (DLI_RegModuleType module, DLI_NOCPEventCbk cbk), (override));
    MOCK_METHOD(uint32_t, DLI_UnregNOCPEventCbk, (DLI_RegModuleType module), (override));
    static DliEventMock& GetMock();

private:
    static DliEventMock *gMock;
};

}; // namespace OHOS
#endif