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
 
#ifndef STACK_SSAP_MOCK_H
#define STACK_SSAP_MOCK_H
 
#include <gmock/gmock.h>
#include <stdint.h>

namespace OHOS {
class SsapMockInterface {
public:
    SsapMockInterface() {};
    virtual ~SsapMockInterface() {};
    virtual SDF_Vector_S *SDF_CreateVectorByCapacity(size_t capacity, SDF_Traits traits) = 0;
    virtual SDF_Vector_S *SDF_CreateVector(SDF_Traits traits) = 0;
    virtual void SDF_DestroyVector(SDF_Vector_S *vector) = 0;
    virtual void SDF_CleanVector(SDF_Vector_S *vector) = 0;
    virtual bool SDF_VectorEmplaceBack(SDF_Vector_S *vector, void *ptr) = 0;
    virtual void SDF_VectorRemove(SDF_Vector_S *vector, size_t index) = 0;
    virtual void SDF_VectorRemoveLast(SDF_Vector_S *vector) = 0;
    virtual bool SDF_VectorFindFirstByStartIndex(SDF_Vector_S *vector, SDF_VectorCompFunc func, void *args,
        size_t startIndex, size_t *index) = 0;
    virtual bool SDF_VectorFindFirst(SDF_Vector_S *vector, SDF_VectorCompFunc func, void *args, size_t *index) = 0;
    virtual void *SDF_VectorElementAt(SDF_Vector_S *vector, size_t index) = 0;
    virtual void *SDF_VectorPopElement(SDF_Vector_S *vector, size_t index) = 0;
    virtual void SDF_VectorSort(SDF_Vector_S *vector, SDF_VectorSortFunc sortFunc) = 0;
};

class SsapMock : public SsapMockInterface {
public:
    SsapMock();
    ~SsapMock() override;
    MOCK_METHOD(SDF_Vector_S*, SDF_CreateVectorByCapacity, (size_t capacity, SDF_Traits traits), (override));
    MOCK_METHOD(SDF_Vector_S*, SDF_CreateVector, (SDF_Traits traits), (override));
    MOCK_METHOD(void, SDF_DestroyVector, (SDF_Vector_S *vector), (override));
    MOCK_METHOD(void, SDF_CleanVector, (SDF_Vector_S *vector), (override));
    MOCK_METHOD(bool, SDF_VectorEmplaceBack, (SDF_Vector_S *vector, void *ptr), (override));
    MOCK_METHOD(void, SDF_VectorRemove, (SDF_Vector_S *vector, size_t index), (override));
    MOCK_METHOD(void, SDF_VectorRemoveLast, (SDF_Vector_S *vector), (override));
    MOCK_METHOD(bool, SDF_VectorFindFirstByStartIndex, (SDF_Vector_S *vector, SDF_VectorCompFunc func, void *args,
        size_t startIndex, size_t *index), (override));
    MOCK_METHOD(bool, SDF_VectorFindFirst, (SDF_Vector_S *vector, SDF_VectorCompFunc func, void *args, size_t *index), (override));
    MOCK_METHOD(void *, SDF_VectorElementAt, (SDF_Vector_S *vector, size_t index), (override));
    MOCK_METHOD(void *, SDF_VectorPopElement, (SDF_Vector_S *vector, size_t index), (override));
    MOCK_METHOD(void, SDF_VectorSort, (SDF_Vector_S *vector, SDF_VectorSortFunc sortFunc), (override));
    static SsapMock& GetMock();

private:
    static SsapMock *gMock;
};

}; // namespace OHOS
#endif