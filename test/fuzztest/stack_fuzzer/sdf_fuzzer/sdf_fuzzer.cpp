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

#include "sdf_fuzzer.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "securec.h"
#include "sdf_addr.h"
#include "sdf_dlist.h"
#include "sdf_errdef.h"
#include "sdf_event.h"
#include "sdf_evc.h"
#include "sdf_log_api.h"
#include "sdf_map.h"
#include "sdf_mem.h"
#include "sdf_slist.h"
#include "sdf_stm.h"
#include "sdf_struct.h"
#include "sdf_thread.h"
#include "sdf_vector.h"
#include "sdf_worker.h"

namespace OHOS {
static void FuzzEvcCb(int handle, void *arg)
{
    (void)handle;
    (void)arg;
}
static void FuzzEventCb(void *arg)
{
    (void)arg;
}
static uint32_t FuzzModifyCb(void *arg)
{
    (void)arg;
    return 0;
}
static void FuzzThreadFunc(uintptr_t arg0, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3)
{
    (void)arg0;
    (void)arg1;
    (void)arg2;
    (void)arg3;
}
static void FuzzThreadLocalCleanup(void *data)
{
    (void)data;
}
static void FuzzWorkCb(void *arg)
{
    (void)arg;
}
static void FuzzWorkFreeCb(void *arg)
{
    (void)arg;
}
static void FuzzLogHook(const char *fmt, ...)
{
    (void)fmt;
}
static int FuzzIntCmp(const void *lhs, const void *rhs)
{
    return *(const int *)lhs - *(const int *)rhs;
}
static void FuzzIntDtor(void *ptr)
{
    (void)ptr;
}

/* evc + event：实例创建（含超 SDF_EVC_MAX_INSTANCE_NUM 上限）、监听（重复/
 * 非法参数/超 SDF_EVC_MAX_EVENT_NUM 上限）、Modify/Cancel、EventAdd 错句柄/
 * Post/Del。USER_EVENT 与 close 事件的 write 触发路径经 EventPost 覆盖。 */
void FuzzSdfEvcEvent(const uint8_t *data, size_t size)
{
    (void)data;
    (void)size;
    (void)SDF_EvcDeinit(); /* 未 init 时 deinit 边界 */
    (void)SDF_EvcInit();
    (void)SDF_EvcInstanceCreate(nullptr, nullptr); /* 非法参数 */
    int handle = -1;
    if (SDF_EvcInstanceCreate(&handle, "fuzz_ev") == 0 && handle >= 0) {
        SDF_EvcEvent evt = {};
        evt.type = SDF_EVC_EVENT;
        evt.callback = FuzzEvcCb;
        evt.args = nullptr;
        evt.freeFunc = nullptr;
        (void)SDF_EvcListenEvent(handle, &evt);
        (void)SDF_EvcListenEvent(handle, nullptr); /* 非法参数 */
        /* 重复监听至超上限（sdf_evc.c:358-361 eventVector->size >= 128） */
        for (int i = 0; i < SDF_EVC_MAX_EVENT_NUM + 1; i++) {
            evt.eventHandle = -1;
            (void)SDF_EvcListenEvent(handle, &evt);
        }
        if (evt.eventHandle >= 0) {
            (void)SDF_ModifyEventArgs(evt.eventHandle, FuzzModifyCb);
            (void)SDF_ModifyEventArgs(-1, FuzzModifyCb); /* 非法句柄 */
            SDF_EvcCancelEvent(evt.eventHandle);
            SDF_EvcCancelEvent(evt.eventHandle); /* 重复取消 */
        }
        int evHandle = -1;
        SDF_EventParam param = {};
        param.handle = handle;
        param.callback = FuzzEventCb;
        param.args = nullptr;
        (void)SDF_EventAdd(&evHandle, &param);
        (void)SDF_EventAdd(nullptr, nullptr); /* 非法参数 */
        (void)SDF_EventPost(-1); /* 错误句柄 */
        if (evHandle >= 0) {
            (void)SDF_EventPost(evHandle);
            SDF_EventDel(evHandle);
        }
        SDF_EvcInstanceClose(handle);
        SDF_EvcInstanceClose(handle); /* 重复关闭 */
    }
    /* 超实例上限（sdf_evc.c:277-279 size >= 3） */
    int overHandles[SDF_EVC_MAX_INSTANCE_NUM + 2] = {0};
    for (size_t i = 0; i < sizeof(overHandles) / sizeof(overHandles[0]); i++) {
        overHandles[i] = -1;
        (void)SDF_EvcInstanceCreate(&overHandles[i], "fuzz_ev_over");
    }
    for (size_t i = 0; i < sizeof(overHandles) / sizeof(overHandles[0]); i++) {
        if (overHandles[i] >= 0) {
            SDF_EvcInstanceClose(overHandles[i]);
        }
    }
    (void)SDF_EvcDeinit();
}

/* thread + worker：Init(0) 拒绝、创建耗尽 freelist（maxNum=2 建 4 条）、
 * Join/SelfId/Yield/Sleep、ThreadLocal 四件套、worker 带/不带 freeCb 的
 * AddWork→RunOnce→Destroy。 */
void FuzzSdfThreadWorker(const uint8_t *data, size_t size)
{
    (void)data;
    (void)size;
    (void)SDF_ThreadInit(0); /* maxNum==0 拒绝（sdf_thread.c:61-63） */
    (void)SDF_ThreadInit(2);
    SDF_ThreadCreateParam_S param = {};
    param.name = "fuzz_thr";
    param.policy = SDF_THREAD_SCHED_OTHER;
    param.priority = 0;
    param.stackSize = 4096; /* 1 页栈，足够跑空 hook */
    param.hook = FuzzThreadFunc;
    uint32_t tid = SDF_THREAD_INVALID_ID;
    (void)SDF_ThreadCreate(&tid, &param);
    (void)SDF_ThreadCreate(nullptr, nullptr); /* 非法参数 */
    uint32_t extra = SDF_THREAD_INVALID_ID;
    for (int i = 0; i < 3; i++) { /* freelist 耗尽（sdf_thread.c:231-244） */
        (void)SDF_ThreadCreate(&extra, &param);
    }
    (void)SDF_ThreadSelfId();
    SDF_ThreadYield();
    SDF_ThreadSleep(1);
    (void)SDF_ThreadJoin(tid);
    (void)SDF_ThreadJoin(SDF_THREAD_INVALID_ID); /* 非法 id */

    SDF_ThreadLocalKey key;
    if (SDF_ThreadLocalKeyInit(&key, FuzzThreadLocalCleanup) == 0) {
        uint32_t localVal = 1;
        (void)SDF_ThreadSetLocal(key, &localVal);
        (void)SDF_ThreadGetLocal(key);
        (void)SDF_ThreadCleanupLocal(key);
    }

    SDF_Worker_S *worker = SDF_CreateWorker();
    if (worker != nullptr) {
        (void)SDF_AddWork(worker, FuzzWorkCb, nullptr, nullptr); /* 无 freeCb */
        uint32_t workArg = 2;
        (void)SDF_AddWork(worker, FuzzWorkCb, &workArg, FuzzWorkFreeCb); /* 带 freeCb */
        SDF_WorkerRunOnce(worker);
        SDF_WorkerRunOnce(worker); /* 空队列退出（sdf_worker.c:93-95） */
        SDF_DestroyWorker(worker); /* 未执行 work 的 freeCb 释放（:50-56） */
    }
    (void)SDF_AddWork(nullptr, FuzzWorkCb, nullptr, nullptr); /* worker NULL */
    SDF_WorkerRunOnce(nullptr);
    SDF_DestroyWorker(nullptr);
    SDF_ThreadDeinit();
}

/* dsl：SList 全 API、DList Count/Move/Add/Del/PosConcat、Vector 容量 0/越界/
 * RemoveLast/Pop/Sort/扩容、Map 非法 traits/重复键/Erase NULL。 */
void FuzzSdfDsl(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    /* SList：宏初始化 + Add/HeadDel/Foreach/Del */
    SDF_SListEntry_S sHead;
    SDF_SListEntry_S sNodes[4];
    SDF_SListHeadInit(&sHead);
    for (size_t i = 0; i < sizeof(sNodes) / sizeof(sNodes[0]); i++) {
        SDF_SListEntryInit(&sNodes[i]);
        SDF_SListHeadInsert(&sHead, &sNodes[i]);
    }
    (void)SDF_SListIsEmpty(&sHead);
    SDF_SListEntry_S *iter = nullptr;
    SDF_SListElmForeach(iter, &sHead)
    {
        (void)iter;
    }
    (void)SDF_SListHeadDel(&sHead);
    (void)SDF_SListDel(&sNodes[1]);

    /* DList：Count/Add/Del/Move/PosConcat */
    SDF_DListHead_S dHead1;
    SDF_DListHead_S dHead2;
    SDF_DListEntry_S dNodes[4];
    SDF_DListHeadInit(&dHead1);
    SDF_DListHeadInit(&dHead2);
    for (size_t i = 0; i < sizeof(dNodes) / sizeof(dNodes[0]); i++) {
        SDF_DListEntryInit(&dNodes[i]);
        SDF_DListAdd(&dHead1, &dNodes[i], &dHead1.list, dHead1.list.next);
    }
    (void)SDF_DListCount(&dHead1);
    SDF_DListMove(&dHead2, &dHead1);
    (void)SDF_DListDel(&dHead2, &dNodes[0]);
    SDF_DListPosConcat(dHead1.list.next, &dHead2);

    /* Vector：capacity=0 → NULL；扩容；越界；RemoveLast/Pop/Sort */
    SDF_Traits traits = {.dtor = nullptr, .cmptor = nullptr};
    (void)SDF_CreateVectorByCapacity(0, traits); /* capacity==0 → NULL（:26-28） */
    SDF_Vector_S *vec = SDF_CreateVectorByCapacity(2, traits);
    if (vec != nullptr) {
        int elems[5] = {0};
        for (int i = 0; i < 5; i++) { /* 超容量触发扩容（grow 因子 2） */
            elems[i] = (int)fdp.ConsumeIntegral<uint8_t>();
            (void)SDF_VectorEmplaceBack(vec, &elems[i]);
        }
        (void)SDF_VectorElementAt(vec, 0);
        (void)SDF_VectorElementAt(vec, 100); /* 越界 → NULL（:152） */
        (void)SDF_VectorPopElement(vec, 0);
        (void)SDF_VectorPopElement(vec, 100); /* 越界 → NULL（:160） */
        (void)SDF_VectorRemove(vec, 100);    /* 越界静默返回（:107） */
        SDF_VectorSort(vec, FuzzIntCmp);      /* :176 */
        SDF_VectorRemoveLast(vec);            /* :124 */
        SDF_CleanVector(vec);
        SDF_DestroyVector(vec);
    }
    SDF_VectorSort(nullptr, FuzzIntCmp); /* NULL/空 → 直接返回 */

    /* Map：非法 traits → NULL（:39-42）；重复键 → false（:57）；Erase NULL/不存在 */
    SDF_Traits badKeyTraits = {.dtor = nullptr, .cmptor = nullptr};
    SDF_Traits valTraits = {.dtor = nullptr, .cmptor = nullptr};
    (void)SDF_MapCtor(badKeyTraits, valTraits); /* keyTraits.cmptor NULL → NULL */
    SDF_Traits keyTraits = {.dtor = FuzzIntDtor, .cmptor = FuzzIntCmp};
    SDF_Map *map = SDF_MapCtor(keyTraits, valTraits);
    if (map != nullptr) {
        int keys[2] = {fdp.ConsumeIntegral<uint8_t>(), fdp.ConsumeIntegral<uint8_t>()};
        int vals[2] = {0, 1};
        (void)SDF_MapMoveInsert(map, &keys[0], &vals[0]);
        (void)SDF_MapMoveInsert(map, &keys[0], &vals[1]); /* 重复键 */
        (void)SDF_MapFind(map, &keys[0]);
        (void)SDF_MapErase(map, &keys[0]);
        (void)SDF_MapErase(nullptr, nullptr); /* NULL → false（:88-89） */
        (void)SDF_MapErase(map, &keys[1]);    /* 不存在键 */
        SDF_MapDtor(map);
    }
}

/* stm：软基状态机——默认 Entry/Dispatch/Exit 空实现与 UNKNOWN 状态名。 */
void FuzzSdfStm(const uint8_t *data, size_t size)
{
    (void)data;
    (void)size;
    StateMachine stm;
    (void)memset_s(&stm, sizeof(stm), 0, sizeof(stm));
    if (StateMachineSoftBaseCtor(&stm)) {
        (void)stm.GetCurrentStateName(&stm); /* 无 current → UNKNOWN（:134-140） */
        State *state = StateCtor(&stm, "FuzzState");
        if (state != nullptr) {
            Message msg = {.what = 1, .extData = nullptr};
            state->Entry(state);    /* 默认空实现（:147-157） */
            state->Dispatch(state, msg);
            state->Exit(state);
            (void)stm.EmplaceNewState(&stm, state);
            (void)stm.GetCurrentStateName(&stm);
        }
        StateMachineSoftBaseDtor(&stm);
    }
}

/* dfx：日志钩子/级别、errno 四件套分解、地址加密 NULL。 */
void FuzzSdfDfx(const uint8_t *data, size_t size)
{
    (void)data;
    (void)size;
    (void)SDF_LogHookReg(FuzzLogHook);
    (void)SDF_LogHookReg(nullptr);
    SDF_LogLevelSet(SDF_LOG_LEVEL_DEBUG);
    (void)SDF_LogLevelGet();
    SDF_LogLevelSet(static_cast<SDF_LogLevel_E>(0xFF)); /* 越界级别 */
    uint32_t err = (uint32_t)0x12345678;
    (void)SDF_ErrGetCompId(err);
    (void)SDF_ErrGetModId(err);
    (void)SDF_ErrGetSubmodId(err);
    (void)SDF_ErrGetErrId(err);
    (void)SDF_ErrGetCompId(0);
    (void)GetEncryptAddr(nullptr); /* NULL 地址（sdf_addr.c:20） */
}

void FuzzSdfAllApi(const uint8_t *data, size_t size)
{
    FuzzSdfEvcEvent(data, size);
    FuzzSdfThreadWorker(data, size);
    FuzzSdfDsl(data, size);
    FuzzSdfStm(data, size);
    FuzzSdfDfx(data, size);
}
}  // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    uint8_t *fuzzData = (uint8_t *)malloc(size);
    if (fuzzData == nullptr) {
        return 0;
    }
    (void)memcpy_s(fuzzData, size, data, size);
    OHOS::FuzzSdfAllApi(fuzzData, size);
    free(fuzzData);
    return 0;
}
