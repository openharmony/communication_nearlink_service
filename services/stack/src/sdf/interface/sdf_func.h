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
#ifndef SDF_BUFF_H
#define SDF_BUFF_H

#include <stdint.h>
#include "sdf_struct.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************************** BUFF PART *****************************************/
typedef SDF_Buff_S* (*SDF_BuffNewPtr)(uint32_t size);
typedef SDF_Buff_S* (*SDF_BuffNewWithReservePtr)(uint32_t size);
typedef SDF_Buff_S* (*SDF_BuffNewWithExtraReservePtr)(uint32_t size, uint16_t extraSize);
typedef void (*SDF_BuffFreePtr)(SDF_Buff_S *buff);
typedef uint8_t* (*SDF_BuffAppendPtr)(SDF_Buff_S *buff, uint32_t size);
typedef uint8_t* (*SDF_BuffPrependPtr)(SDF_Buff_S *buff, uint32_t size);
typedef uint8_t* (*SDF_BuffTrimPrefixPtr)(SDF_Buff_S *buff, uint32_t size);
typedef int32_t (*SDF_BuffTrimSuffixPtr)(SDF_Buff_S *buff, uint32_t size);
typedef SDF_Buff_S* (*SDF_BuffCopyPtr)(SDF_Buff_S *buff);

/***************************************** DLIST PART *****************************************/
typedef uint32_t (*SDF_DListCountPtr)(SDF_DListHead_S *head);
typedef void (*SDF_DListMovePtr)(SDF_DListHead_S *dst, SDF_DListHead_S *src);
typedef void (*SDF_DListAddPtr)(SDF_DListHead_S *head, SDF_DListEntry_S *node,
    SDF_DListEntry_S *prev, SDF_DListEntry_S *next);
typedef SDF_DListEntry_S* (*SDF_DListDelPtr)(SDF_DListHead_S *head, SDF_DListEntry_S *node);
typedef void (*SDF_DListPosConcatPtr)(SDF_DListEntry_S *pos, SDF_DListHead_S *src);

/***************************************** MAP PART *****************************************/
typedef void (*SDF_MapDtorPtr)(SDF_Map *map);
typedef SDF_Map* (*SDF_MapCtorPtr)(SDF_Traits keyTraits, SDF_Traits valTraits);

typedef bool (*SDF_MapMoveInsertPtr)(SDF_Map *map, void *key, void *val);
typedef SDF_MapIter* (*SDF_MapFindPtr)(SDF_Map *map, void *key);
typedef bool (*SDF_MapErasePtr)(SDF_Map *map, void *key);

/***************************************** VECTOR PART *****************************************/
typedef SDF_Vector_S* (*SDF_CreateVectorByCapacityPtr)(size_t capacity, SDF_Traits traits);
typedef SDF_Vector_S* (*SDF_CreateVectorPtr)(SDF_Traits traits);
typedef void (*SDF_DestroyVectorPtr)(SDF_Vector_S *vector);
typedef void (*SDF_CleanVectorPtr)(SDF_Vector_S *vector);
typedef bool (*SDF_VectorEmplaceBackPtr)(SDF_Vector_S *vector, void *ptr);
typedef void (*SDF_VectorRemoveLastPtr)(SDF_Vector_S *vector);
typedef void (*SDF_VectorRemovePtr)(SDF_Vector_S *vector, size_t index);
typedef bool (*SDF_VectorFindFirstByStartIndexPtr)(SDF_Vector_S *vector, SDF_VectorCompFunc func, void *args,
    size_t startIndex, size_t *index);
typedef bool (*SDF_VectorFindFirstPtr)(SDF_Vector_S *vector, SDF_VectorCompFunc func, void *args, size_t *index);
typedef void* (*SDF_VectorElementAtPtr)(SDF_Vector_S *vector, size_t index);
typedef void* (*SDF_VectorPopElementPtr)(SDF_Vector_S *vector, size_t index);
typedef void (*SDF_VectorSortPtr)(SDF_Vector_S *vector, SDF_VectorSortFunc sortFunc);

/***************************************** CP_WORKER PART *****************************************/
typedef SDF_Worker_S* (*SDF_CreateWorkerPtr)(void);
typedef void (*SDF_DestroyWorkerPtr)(SDF_Worker_S *worker);
typedef uint32_t (*SDF_AddWorkPtr)(SDF_Worker_S *worker, SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb);
typedef void (*SDF_WorkerRunOncePtr)(SDF_Worker_S *worker);
typedef uint32_t (*CP_PostTaskPtr)(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb);
typedef uint32_t (*CP_PostTaskBlockedPtr)(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb, int timeout);
typedef uint32_t (*CP_TimerAddPtr)(int *handle, SDF_TimerParam *param);
typedef void (*CP_TimerDelPtr)(int handle);

/***************************************** STM PART *****************************************/
typedef void (*StateMachineSoftBaseDtorPtr)(StateMachine *stm);
typedef bool (*StateMachineSoftBaseCtorPtr)(StateMachine *stm);
typedef void (*StateDtorPtr)(State *state);
typedef State* (*StateCtorPtr)(StateMachine *stm, const char *name);

/***************************************** MEM PART *****************************************/
typedef void* (*SDF_MemAllocPtr)(size_t size);
typedef void* (*SDF_MemZallocPtr)(size_t size);
typedef void (*SDF_MemFreePtr)(void *ptr);

/***************************************** ADDR PART *****************************************/
typedef struct SDF_EncryptedLogString (*GetEncryptAddrPtr)(const SLE_Addr_S *addr);

/***************************************** SEM PART *****************************************/
typedef SDF_SemHooks_S* (*SDF_SemGetHookPtr)(void);

typedef struct{
    SDF_BuffNewPtr openBuffNew;
    SDF_BuffNewWithReservePtr openBuffNewWithReserve;
    SDF_BuffNewWithExtraReservePtr openBuffNewWithExtraReserve;
    SDF_BuffFreePtr openBuffFree;
    SDF_BuffAppendPtr openBuffAppend;
    SDF_BuffPrependPtr openBuffPrepend;
    SDF_BuffTrimPrefixPtr openBuffTrimPrefix;
    SDF_BuffTrimSuffixPtr openBuffTrimSuffix;
    SDF_BuffCopyPtr openBuffCopy;

    SDF_MapDtorPtr openMapDtor;
    SDF_MapCtorPtr openMapCtor;
    SDF_MapMoveInsertPtr openMapMoveInsert;
    SDF_MapFindPtr openMapFind;
    SDF_MapErasePtr openMapErase;

    SDF_CreateVectorByCapacityPtr openCreateVectorByCapacity;
    SDF_CreateVectorPtr openCreateVector;
    SDF_DestroyVectorPtr openDestroyVector;
    SDF_CleanVectorPtr openCleanVector;
    SDF_VectorEmplaceBackPtr openVectorEmplaceBack;
    SDF_VectorRemoveLastPtr openVectorRemoveLast;
    SDF_VectorRemovePtr openVectorRemove;
    SDF_VectorFindFirstByStartIndexPtr openVectorFindFirstByStartIndex;
    SDF_VectorFindFirstPtr openVectorFindFirst;
    SDF_VectorElementAtPtr openVectorElementAt;
    SDF_VectorPopElementPtr openVectorPopElement;
    SDF_VectorSortPtr openVectorSort;

    SDF_CreateWorkerPtr openCreateWorker;
    SDF_DestroyWorkerPtr openDestroyWorker;
    SDF_AddWorkPtr openAddWork;
    SDF_WorkerRunOncePtr openWorkerRunOnce;
    CP_PostTaskPtr openPostTask;
    CP_PostTaskBlockedPtr openPostTaskBlocked;
    CP_TimerAddPtr openTimerAdd;
    CP_TimerDelPtr openTimerDel;

    StateMachineSoftBaseDtorPtr openStateMachineSoftBaseDtor;
    StateMachineSoftBaseCtorPtr openStateMachineSoftBaseCtor;
    StateDtorPtr openStateDtor;
    StateCtorPtr openStateCtor;

    SDF_MemAllocPtr openMemAlloc;
    SDF_MemZallocPtr openMemZalloc;
    SDF_MemFreePtr openMemFree;

    GetEncryptAddrPtr openGetEncryptAddr;

    SDF_SemGetHookPtr openSemGetHook;

} SDF_OpenFuncList;

#ifdef __cplusplus
}
#endif
#endif