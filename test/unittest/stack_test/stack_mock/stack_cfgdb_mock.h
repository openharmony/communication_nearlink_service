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
 
#ifndef STACK_CFGDB_MOCK_H
#define STACK_CFGDB_MOCK_H
 
#include <gmock/gmock.h>
#include <stdint.h>
#include "sdf_addr.h"

namespace OHOS {
class CfgdbMockInterface {
public:
    CfgdbMockInterface() {};
    virtual ~CfgdbMockInterface() {};
    virtual uint32_t NLSTK_CfgdbGetPublicAddress(SLE_Addr_S *addr) = 0;
};

class CfgdbMock : public CfgdbMockInterface {
public:
    CfgdbMock();
    ~CfgdbMock() override;
    MOCK_METHOD(uint32_t, NLSTK_CfgdbGetPublicAddress, (SLE_Addr_S *addr), (override));
    static CfgdbMock& GetMock();

private:
    static CfgdbMock *gMock;
};
 
}; // namespace OHOS
#endif