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
 
#ifndef STACK_DTAP_MOCK_H
#define STACK_DTAP_MOCK_H
 
#include <gmock/gmock.h>
#include <stdint.h>
#include "dtap.h"

namespace OHOS {
class DtapMockInterface {
public:
    DtapMockInterface() {};
    virtual ~DtapMockInterface() {};
    virtual uint32_t DTAP_RegisterDataRecvCb(uint8_t tcid, DTAP_DataRecvCb cb) = 0;
    virtual uint32_t DTAP_UnregisterDataRecvCb(uint8_t tcid) = 0;
    virtual uint32_t DTAP_DataSend(DTAP_Data_S *data) = 0;
};
 
class DtapMock : public DtapMockInterface {
public:
    DtapMock();
    ~DtapMock() override;
    MOCK_METHOD(uint32_t, DTAP_RegisterDataRecvCb, (uint8_t tcid, DTAP_DataRecvCb cb), (override));
    MOCK_METHOD(uint32_t, DTAP_UnregisterDataRecvCb, (uint8_t tcid), (override));
    MOCK_METHOD(uint32_t, DTAP_DataSend, (DTAP_Data_S *data), (override));
    static DtapMock& GetMock();
 
private:
    static DtapMock *gMock;
};
 
}; // namespace OHOS
#endif