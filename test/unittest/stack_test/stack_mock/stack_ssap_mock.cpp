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

#include <gtest/gtest.h>
#include <securec.h>
#include "stack_ssap_stub.h"
#include "stack_ssap_mock.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
void *g_stackSsapMock;

SsapMock::SsapMock()
{
    g_stackSsapMock = reinterpret_cast<void *>(this);
}
 
SsapMock::~SsapMock()
{
    g_stackSsapMock = nullptr;
}
 
static SsapMockInterface *SsapMock()
{
    return reinterpret_cast<SsapMockInterface *>(g_stackSsapMock);
}
 
extern "C" {

SDF_Vector_S *SDF_CreateVectorByCapacity(size_t capacity, SDF_Traits traits)
{
    return SsapMock()->SDF_CreateVectorByCapacity(capacity, traits);
}

SDF_Vector_S *SDF_CreateVector(SDF_Traits traits)
{
    return SsapMock()->SDF_CreateVector(traits);
}

void SDF_DestroyVector(SDF_Vector_S *vector)
{
    return SsapMock()->SDF_DestroyVector(vector);
}

void SDF_CleanVector(SDF_Vector_S *vector)
{
    return SsapMock()->SDF_CleanVector(vector);
}

bool SDF_VectorEmplaceBack(SDF_Vector_S *vector, void *ptr)
{
    return SsapMock()->SDF_VectorEmplaceBack(vector, ptr);
}

void SDF_VectorRemove(SDF_Vector_S *vector, size_t index)
{
    return SsapMock()->SDF_VectorRemove(vector, index);
}

void SDF_VectorRemoveLast(SDF_Vector_S *vector)
{
    return SsapMock()->SDF_VectorRemoveLast(vector);
}

bool SDF_VectorFindFirstByStartIndex(SDF_Vector_S *vector, SDF_VectorCompFunc func, void *args,
    size_t startIndex, size_t *index)
{
    return SsapMock()->SDF_VectorFindFirstByStartIndex(vector, func, args, startIndex, index);
}

bool SDF_VectorFindFirst(SDF_Vector_S *vector, SDF_VectorCompFunc func, void *args, size_t *index)
{
    return SsapMock()->SDF_VectorFindFirst(vector, func, args, index);
}

void *SDF_VectorElementAt(SDF_Vector_S *vector, size_t index)
{
    return SsapMock()->SDF_VectorElementAt(vector, index);
}

void *SDF_VectorPopElement(SDF_Vector_S *vector, size_t index)
{
    return SsapMock()->SDF_VectorPopElement(vector, index);
}

void SDF_VectorSort(SDF_Vector_S *vector, SDF_VectorSortFunc sortFunc)
{
    return SsapMock()->SDF_VectorSort(vector, sortFunc);
}

}
} // namespace OHOS