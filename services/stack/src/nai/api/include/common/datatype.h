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
 * Description: datatype
 */
#ifndef DATA_TYPE_H
#define DATA_TYPE_H
#include <stdbool.h>

#ifndef LPCSTR
#define LPCSTR     const char *
#endif

#ifndef HANDLE_STACK
#define HANDLE_STACK void *
#endif

#ifndef TRUE
#define TRUE    1
#define FALSE    0
#endif

#ifndef SLE_ADDR_LEN
#define SLE_ADDR_LEN    6    /* nearlink address length */
#endif

#define minself(x, y)     if ((uint32_t)(y) < (uint32_t)(x)) { (x) = (y); }
#define maxself(x, y)     if ((uint32_t)(y) > (uint32_t)(x)) { (x) = (y); }

#ifndef MIN
#define MIN(a, b) (((uint32_t)(a) > (uint32_t)(b)) ? (b) : (a))
#endif

#ifndef MAX
#define MAX(a, b) (((uint32_t)(a) > (uint32_t)(b)) ? (a) : (b))
#endif

#ifndef MIN_X
#define MIN_X(a, b) (((a) > (b)) ? (b) : (a))
#endif

#ifndef MAX_X
#define MAX_X(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef NLSTK_ERRCODE
typedef uint32_t NLSTK_ERRCODE;
#endif

#ifndef EOK
/* success */
#define EOK (0)
#endif

#ifndef SYSTEM_LOCK
#define SYSTEM_LOCK        void *
#endif

#ifndef PBYTE
#define PBYTE   unsigned char *
#endif

#ifndef PDWORD
#define PDWORD  unsigned long *
#endif

#ifndef PVOID
#define PVOID    void *
#endif

#ifndef PCHAR
#define PCHAR   signed char *
#endif

#ifndef SYSTEM_LOCK
#define SYSTEM_LOCK        void *
#endif

#define UUID_16BIT_LEN 2
#define UUID_128BIT_LEN 16


#endif
