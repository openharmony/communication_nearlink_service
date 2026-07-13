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

#ifndef SDF_STRUCT_H
#define SDF_STRUCT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/***************************************** TIMER PART *****************************************/
typedef void (*SDF_TimerCallback)(void *arg);

typedef struct {
    int handle;
    time_t expires;
    bool period;
    SDF_TimerCallback callback;
    void *args;
} SDF_TimerParam;

/***************************************** BUFF PART *****************************************/
/**
 * @brief 数据缓冲区定义
 * 缓冲区包含headroom（预留给DTAP/DLI header）、data和tailroom（预留给crc16）三部分，详细说明如下所示：
 *  |-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-> buffLen <-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+|
 *  |                                                                                    |
 *  |-+-+-> dataOff <-+-+-|-+-+-+-+-> dataLen <-+-+-+-+-|-+-> bufLen-dataOff-dataLen <-+-|
 *  |                     |                             |                                |
 *  |-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+|
 *  |      headroom       |            data             |             tailroom           |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+|
 *                        ↑                             ↑
 *                   buff+dataOff               buff+dataOff+dataLen
 */
typedef struct SDF_Buff {
    uint64_t dataOff;  // 数据在缓冲区中的偏移量
    uint64_t dataLen;  // 数据长度
    uint64_t buffLen;  // 缓冲区长度
    uint8_t buff[0];   // 缓冲区
} __attribute__((packed)) SDF_Buff_S;

/***************************************** DLIST PART *****************************************/
typedef struct SDF_DListEntry {
    struct SDF_DListEntry *next;
    struct SDF_DListEntry *prev;
} SDF_DListEntry_S;

typedef struct SDF_DListHead_S {
    SDF_DListEntry_S list;
    uint32_t size;
} SDF_DListHead_S;

typedef struct SDF_SListEntry {
    struct SDF_SListEntry *next;
} SDF_SListEntry_S;

typedef SDF_SListEntry_S SDF_SListHead_S;

/***************************************** TRAITS PART *****************************************/
// 定义容器的特征函数
typedef struct tagSDF_Traits {
    void (*dtor)(void *);                                       // destructor析构函数
    int (*cmptor)(const void *, const void *);                  // comparator比较函数
} SDF_Traits;

/***************************************** MAP PART *****************************************/
typedef struct tagSDF_MapIter {
    void *key;
    void *val;
    struct tagSDF_MapIter *next;
} SDF_MapIter;

typedef struct tagSDF_Map {
    SDF_Traits keyTraits;
    SDF_Traits valTraits;
    SDF_MapIter *entry;
} SDF_Map;

/***************************************** SEM PART *****************************************/
/* 信号量 */
typedef void *SDF_Sem;

#define SEM_ALWAYS_WAIT (-1)

typedef enum SDF_SemType {
    SDF_SEM_COUNT_TYPE = 0,    //!< 计数信号量
    SDF_SEM_BINARY_TYPE,       //!< 二进制信号量
    SDF_SEM_TYPE_BUTT
} SDF_SemType_E;

/**
 * @brief           信号量初始化钩子
 * @param[IN/OUT]   pSem 信号量指针
 * @param[IN]       flag , SDF_SemType_E
 * @param[IN]       value 创建信号量的初始值，
 * @return          int
 * @retval          成功返回0,失败返回其他
 * @remarks         SDF_SEM_BINARY_TYPE 类型信号量 value范围[0 , 1]
 * @remarks         SDF_SEM_MUTEX_TYPE 类型信号量 value值无效，传0即可
 */
typedef uint32_t (*SDF_SemInitHook)(SDF_Sem pSem, int flag, uint32_t value);

/**
 * @brief           信号量去初始化钩子
 * @param[IN/OUT]   pSem 信号量指针
 * @return          void
 * @retval          NA
 * @remarks         NA
 */
typedef void (*SDF_SemDeinitHook)(SDF_Sem pSem);

/**
 * @brief           信号量PV操作钩子
 * @param[IN/OUT]   pSem 信号量指针
 * @return          int
 * @retval          成功返回0,失败返回其他
 * @remarks         NA
 */
typedef uint32_t (*SDF_SemPVHook)(SDF_Sem pSem);

/**
 * @brief           信号量超时等待钩子
 * @param[IN/OUT]   pSem 信号量指针
 * @param[IN]       timeout 超时时间,单位ms,传入ALWAYS_WAIT(-1)一直等待，
 * @return          int
 * @retval          成功返回0,超时返回errno，失败返回其他
 * @remarks         SDF_SEM_MUTEX_TYPE 类型信号量，超时无效
 */
typedef uint32_t (*SDF_SemTimeWaitHook)(SDF_Sem pSem, int timeout);

typedef struct SDF_SemHooks {
    SDF_SemInitHook initHook;
    SDF_SemDeinitHook deinitHook;
    SDF_SemPVHook postHook;
    SDF_SemPVHook waitHook;
    SDF_SemPVHook tryWaitHook;
    SDF_SemTimeWaitHook timeWaitHook;
    size_t semSize; // 信号量结构大小
} SDF_SemHooks_S;

/***************************************** VECTOR PART *****************************************/
typedef struct SDF_Vector {
    void **buf;
    size_t size;
    size_t capacity;
    SDF_Traits traits;
} SDF_Vector_S;

typedef bool (*SDF_VectorCompFunc)(void *ptr, void *args);
typedef int (*SDF_VectorSortFunc)(const void *a, const void *b);

/***************************************** CP_WORKER PART *****************************************/
typedef void (*SDF_WorkCb)(void *arg);
typedef void (*SDF_FreeWorkArg)(void *arg);

/* 互斥锁 */
typedef void* SDF_MutexLock;
typedef void* SDF_MutexLockAttr;

typedef struct SDF_Work {
    SDF_DListEntry_S entry;
    SDF_WorkCb cb;
    SDF_FreeWorkArg freeCb;
    void *arg;
} SDF_Work_S;

typedef struct SDF_Worker {
    SDF_DListHead_S head;
    SDF_MutexLock mutex;
    SDF_MutexLockAttr mutexAttr;
} SDF_Worker_S;

typedef struct SDF_BlockTask {
    SDF_Sem sem;
    SDF_WorkCb func;
    SDF_FreeWorkArg freeCb;
    void *arg;
} SDF_BlockTask_S;

/***************************************** STM PART *****************************************/
typedef struct tagMessage {
    int what;
    void *extData;
} Message;

struct tagState;

typedef struct tagStateMachine {
    void (*Transition)(struct tagStateMachine *stm, const char *targetStateName);
    void (*ProcessMessage)(struct tagStateMachine *stm, Message msg);
    bool (*EmplaceNewState)(struct tagStateMachine *stm, struct tagState *state);
    const char *(*GetCurrentStateName)(struct tagStateMachine *stm);

    struct tagState *states_;
    struct tagState *current_;
} StateMachine;

typedef struct tagState {
    void (*Transition)(struct tagState *state, const char *targetStateName);
    void (*Entry)(struct tagState *state);
    void (*Exit)(struct tagState *state);
    void (*Dispatch)(struct tagState *state, Message msg);

    StateMachine *stm_;
    char *name_;
    struct tagState *next;
} State;

/***************************************** ADDR PART *****************************************/
typedef enum {
    PUBLIC_ADDRESS = 0x0,       /* 第一类短机构标识 */
    LONG_MECH_PUBLIC_ADDRESS,   /* 第二类长机构标识 */
    LOCAL_MECH_PUBLIC_ADDRESS,  /* 第三类机构本地标识 */
    RPA_RESOLV_ADDRESS,         /* 可解析随机标识 */
    RPA_UNRESOLV_ADDRESS,       /* 不可解析随机标识 */
    RESERVE_ADDRESS,            /* 第五类联盟预留标识块 */
    RANDOM_ADDRESS,             /* 私有标识 */
    ADDR_TYPE_END
} SLE_AddrType_E;

#define SDF_ENC_LOG_STR_LEN 64
struct SDF_EncryptedLogString {
    char buf[SDF_ENC_LOG_STR_LEN];
};

#define SLE_ADDR_LEN 6
typedef struct SLE_Addr {
    uint8_t type;
    uint8_t addr[SLE_ADDR_LEN];
} SLE_Addr_S;
#ifdef __cplusplus
}
#endif

#endif /* SDF_STRUCT_H */