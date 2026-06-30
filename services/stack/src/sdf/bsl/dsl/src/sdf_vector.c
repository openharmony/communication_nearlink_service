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

#include "securec.h"
#include "sdf_mem.h"
#include "sdf_vector.h"

#define SDF_VECTOR_DEFAULT_CAPACITY     8
#define SDF_VECTOR_ELEMENT_SIZE         sizeof(void *)
#define SDF_VECTOR_GROW_FACTOR          2

SDF_Vector_S *SDF_CreateVectorByCapacity(size_t capacity, SDF_Traits traits)
{
    if (capacity == 0) {
        return NULL;
    }
    SDF_Vector_S *vector = (SDF_Vector_S *)SDF_MemZalloc(sizeof(SDF_Vector_S));
    if (vector == NULL) {
        return NULL;
    }
    vector->buf = (void **)SDF_MemZalloc(SDF_VECTOR_ELEMENT_SIZE * capacity);
    if (vector->buf == NULL) {
        SDF_MemFree(vector);
        return NULL;
    }
    vector->capacity = capacity;
    vector->traits = traits;
    return vector;
}

SDF_Vector_S *SDF_CreateVector(SDF_Traits traits)
{
    return SDF_CreateVectorByCapacity(SDF_VECTOR_DEFAULT_CAPACITY, traits);
}

void SDF_DestroyVector(SDF_Vector_S *vector)
{
    if (vector == NULL) {
        return;
    }
    if (vector->traits.dtor != NULL) {
        for (size_t i = 0; i < vector->size; i++) {
            vector->traits.dtor(vector->buf[i]);
        }
    }
    SDF_MemFree(vector->buf);
    SDF_MemFree(vector);
}

void SDF_CleanVector(SDF_Vector_S *vector)
{
    if (vector == NULL) {
        return;
    }
    if (vector->traits.dtor != NULL) {
        for (size_t i = 0; i < vector->size; i++) {
            vector->traits.dtor(vector->buf[i]);
            vector->buf[i] = NULL;
        }
    }
    vector->size = 0;
}

static bool VectorGrow(SDF_Vector_S *vector)
{
    size_t copySize = SDF_VECTOR_ELEMENT_SIZE * vector->capacity;
    void **buf = (void **)SDF_MemZalloc(copySize * SDF_VECTOR_GROW_FACTOR);
    if (buf == NULL) {
        return false;
    }
    (void)memcpy_s(buf, copySize, vector->buf, copySize);
    SDF_MemFree(vector->buf);
    vector->buf = buf;
    vector->capacity = vector->capacity * SDF_VECTOR_GROW_FACTOR;
    return true;
}

bool SDF_VectorEmplaceBack(SDF_Vector_S *vector, void *ptr)
{
    if (vector == NULL || ptr == NULL) {
        return false;
    }
    if (vector->capacity == vector->size) {
        if (!VectorGrow(vector)) {
            return false;
        }
    }
    vector->buf[vector->size] = ptr;
    vector->size++;
    return true;
}

void SDF_VectorRemove(SDF_Vector_S *vector, size_t index)
{
    if (vector == NULL || index >= vector->size) {
        return;
    }
    if (vector->traits.dtor != NULL) {
        vector->traits.dtor(vector->buf[index]);
    }
    if (index != vector->size - 1) {
        size_t moveSize = SDF_VECTOR_ELEMENT_SIZE * (vector->size - index - 1);
        if (memmove_s(&vector->buf[index], moveSize, &vector->buf[index + 1], moveSize) != EOK) {
            return;
        }
    }
    vector->size--;
}

void SDF_VectorRemoveLast(SDF_Vector_S *vector)
{
    if (vector == NULL || vector->size == 0) {
        return;
    }
    SDF_VectorRemove(vector, vector->size - 1);
}

bool SDF_VectorFindFirstByStartIndex(SDF_Vector_S *vector, SDF_VectorCompFunc func, void *args,
    size_t startIndex, size_t *index)
{
    if (vector == NULL || func == NULL || startIndex >= vector->size || index == NULL) {
        return false;
    }
    for (size_t i = startIndex; i < vector->size; i++) {
        if (func(vector->buf[i], args)) {
            *index = i;
            return true;
        }
    }
    return false;
}

bool SDF_VectorFindFirst(SDF_Vector_S *vector, SDF_VectorCompFunc func, void *args, size_t *index)
{
    return SDF_VectorFindFirstByStartIndex(vector, func, args, 0, index);
}

void *SDF_VectorElementAt(SDF_Vector_S *vector, size_t index)
{
    if (vector == NULL || index >= vector->size) {
        return NULL;
    }
    return vector->buf[index];
}

void *SDF_VectorPopElement(SDF_Vector_S *vector, size_t index)
{
    if (vector == NULL || index >= vector->size) {
        return NULL;
    }
    void *element = vector->buf[index];
    if (index != vector->size - 1) {
        size_t moveSize = SDF_VECTOR_ELEMENT_SIZE * (vector->size - index - 1);
        if (memmove_s(&vector->buf[index], moveSize, &vector->buf[index + 1], moveSize) != EOK) {
            return NULL;
        }
    }
    vector->size--;
    return element;
}

void SDF_VectorSort(SDF_Vector_S *vector, SDF_VectorSortFunc sortFunc)
{
    if (vector == NULL || sortFunc == NULL || vector->size == 0) {
        return;
    }
    qsort(vector->buf, vector->size, sizeof(void *), sortFunc);
}