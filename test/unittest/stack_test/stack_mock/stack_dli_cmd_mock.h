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

#ifndef STACK_DLI_CMD_MOCK_H
#define STACK_DLI_CMD_MOCK_H

#include <gmock/gmock.h>
#include <stdint.h>

namespace OHOS {
class DliCmdMockInterface {
public:
    DliCmdMockInterface() {};
    virtual ~DliCmdMockInterface() {};
    virtual uint32_t DLI_GetPublicAddress(void) = 0;
    virtual uint32_t DLI_ReadLocalFeatures(void) = 0;
    virtual uint32_t DLI_ReadLocalVersion(void) = 0;
    virtual uint32_t DLI_ReadMaximumAdvDataLen(void) = 0;
    virtual uint32_t DLI_ReadAdvSetsNum(void) = 0;
    virtual uint32_t DLI_ReadSupportCryptoAlgo(void) = 0;
    virtual bool DLI_IsSupportNewDisMeasure(void) = 0;
};

class DliCmdMock : public DliCmdMockInterface {
public:
    DliCmdMock();
    ~DliCmdMock() override;
    MOCK_METHOD(uint32_t, DLI_GetPublicAddress, (), (override));
    MOCK_METHOD(uint32_t, DLI_ReadLocalFeatures, (), (override));
    MOCK_METHOD(uint32_t, DLI_ReadLocalVersion, (), (override));
    MOCK_METHOD(uint32_t, DLI_ReadMaximumAdvDataLen, (), (override));
    MOCK_METHOD(uint32_t, DLI_ReadAdvSetsNum, (), (override));
    MOCK_METHOD(uint32_t, DLI_ReadSupportCryptoAlgo, (), (override));
    MOCK_METHOD(bool, DLI_IsSupportNewDisMeasure, (), (override));
    static DliCmdMock& GetMock();
private:
    static DliCmdMock *gMock;
};
}
#endif