/**
  * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef SDF_EVC_H
#define SDF_EVC_H

#include <stdint.h>
#include <stdbool.h>
#include "sdf_errno_base.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SDF_EVC_MAX_INSTANCE_NUM 3
#define SDF_EVC_MAX_EVENT_NUM 128

#define SDF_EVC_ERROR_ALREADY_INIT              SDF_MAKE_BSL_EVC_ERRNO(2)
#define SDF_EVC_ERROR_MEM_FAILED                SDF_MAKE_BSL_EVC_ERRNO(3)
#define SDF_EVC_ERROR_MAX_INSTANCE              SDF_MAKE_BSL_EVC_ERRNO(4)
#define SDF_EVC_ERROR_EPOLL_CREATE_FAILED       SDF_MAKE_BSL_EVC_ERRNO(5)
#define SDF_EVC_ERROR_EPOLL_CTL_FAILED          SDF_MAKE_BSL_EVC_ERRNO(6)
#define SDF_EVC_ERROR_THREAD_FAILED             SDF_MAKE_BSL_EVC_ERRNO(7)
#define SDF_EVC_ERROR_WRONG_HANDLE              SDF_MAKE_BSL_EVC_ERRNO(8)
#define SDF_EVC_ERROR_MAX_LISTEN_EVENT          SDF_MAKE_BSL_EVC_ERRNO(9)
#define SDF_EVC_ERROR_ALREADY_LISTENED          SDF_MAKE_BSL_EVC_ERRNO(10)
#define SDF_EVC_ERROR_MUTEX_FAILED              SDF_MAKE_BSL_EVC_ERRNO(11)
#define SDF_EVC_ERROR_INVALID_PARAM             SDF_MAKE_BSL_EVC_ERRNO(12)
#define SDF_EVC_ERROR_NOT_INIT                  SDF_MAKE_BSL_EVC_ERRNO(13)
#define SDF_EVC_ERROR_VECTOR_FAIL               SDF_MAKE_BSL_EVC_ERRNO(14)
#define SDF_EVC_ERROR_FD_CREATE_FAILED          SDF_MAKE_BSL_EVC_ERRNO(16)

typedef void (*SDF_EvcEventCallback)(int handle, void *arg);
typedef void (*SDF_EvcEventArgsFreeFunc)(void *arg);
typedef uint32_t (*SDF_EvcEventModifyFunc)(void *arg);

typedef enum {
    SDF_EVC_TIMER = 1,
    SDF_EVC_EVENT,
    SDF_EVC_USER_EVENT,
} SDF_EvcEventType_E;

typedef struct {
    SDF_EvcEventType_E type;
    int eventHandle;
    SDF_EvcEventCallback callback;
    void *args;
    SDF_EvcEventArgsFreeFunc freeFunc;
} SDF_EvcEvent;

uint32_t SDF_EvcInit(void);
void SDF_EvcDeinit(void);
uint32_t SDF_EvcInstanceCreate(int *handle, const char *name);
void SDF_EvcInstanceClose(int handle);
uint32_t SDF_EvcListenEvent(int handle, SDF_EvcEvent *event);
uint32_t SDF_ModifyEventArgs(int eventHandle, SDF_EvcEventModifyFunc func);
void SDF_EvcCancelEvent(int eventHandle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
