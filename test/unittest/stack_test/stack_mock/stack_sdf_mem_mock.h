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
 
#ifndef STACK_SDF_MEM_MOCK_H
#define STACK_SDF_MEM_MOCK_H
 
#include <gmock/gmock.h>
#include <stdint.h>
 
namespace OHOS {
class SdfMemMockInterface {
public:
    SdfMemMockInterface() {};
    virtual ~SdfMemMockInterface() {};
    virtual void *SDF_MemZalloc(size_t size) = 0;
};
 
class SdfMemMock : public SdfMemMockInterface {
public:
    SdfMemMock();
    ~SdfMemMock() override;
    MOCK_METHOD(void *, SDF_MemZalloc, (size_t size), (override));
    static SdfMemMock& GetMock();
private:
    static SdfMemMock *gMock;
};
 
}; // namespace OHOS
#endif