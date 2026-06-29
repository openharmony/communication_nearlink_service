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

#ifndef STACK_SSAP_STUB_H
#define STACK_SSAP_STUB_H

#include "sdf_vector.h"

#ifdef __cplusplus
extern "C" {
#endif

SDF_Vector_S *TEST_SDF_CreateVectorByCapacity(size_t capacity, SDF_Traits traits);
SDF_Vector_S *TEST_SDF_CreateVector(SDF_Traits traits);
void TEST_SDF_DestroyVector(SDF_Vector_S *vector);
void TEST_SDF_CleanVector(SDF_Vector_S *vector);
bool TEST_SDF_VectorEmplaceBack(SDF_Vector_S *vector, void *ptr);
void TEST_SDF_VectorRemove(SDF_Vector_S *vector, size_t index);
void TEST_SDF_VectorRemoveLast(SDF_Vector_S *vector);
bool TEST_SDF_VectorFindFirstByStartIndex(SDF_Vector_S *vector, SDF_VectorCompFunc func, void *args,
    size_t startIndex, size_t *index);
bool TEST_SDF_VectorFindFirst(SDF_Vector_S *vector, SDF_VectorCompFunc func, void *args, size_t *index);
void *TEST_SDF_VectorElementAt(SDF_Vector_S *vector, size_t index);
void *TEST_SDF_VectorPopElement(SDF_Vector_S *vector, size_t index);
void TEST_SDF_VectorSort(SDF_Vector_S *vector, SDF_VectorSortFunc sortFunc);

#ifdef __cplusplus
}
#endif

#endif

