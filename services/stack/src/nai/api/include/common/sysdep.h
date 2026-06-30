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
 * Description: sysdep
 */
#ifndef SYSTEMDEP_H
#define SYSTEMDEP_H

#include "datatype.h"

#ifdef __cplusplus
extern "C" {
#endif

#define decode2byte_little(_ptr)     (uint16_t)(*(uint8_t *)(_ptr) | (*(uint8_t *)((_ptr) + 1) << 8))
#define decode4byte_little(_ptr) \
    (uint32_t)(*(uint8_t *)(_ptr) | (*(uint8_t *)((_ptr) + 1) << 8) \
    | (*(uint8_t *)((_ptr) + 2) << 16) | (*(uint8_t *)((_ptr) + 3) << 24))
#define encode2byte_little(_ptr, data) \
do { \
    *((uint8_t *)(_ptr) + 1) = (uint8_t)((uint16_t)(data) >> 8); \
    *(uint8_t *)(_ptr) = (uint8_t)(data); \
} while (0)
#define encode4byte_little(_ptr, data) \
do { \
    *(uint8_t *)((_ptr) + 3) = (uint8_t)((data) >> 24); \
    *(uint8_t *)((_ptr) + 2) = (uint8_t)((data) >> 16); \
    *(uint8_t *)((_ptr) + 1) = (uint8_t)((data) >> 8); \
    *(uint8_t *)(_ptr) = (uint8_t)(data); \
} while (0)

#define decode2byte_big(_ptr)     (uint16_t)(*(uint8_t *)((_ptr) + 1) | (*(uint8_t *)(_ptr) << 8))
#define decode4byte_big(_ptr) \
    (uint32_t)(*(uint8_t *)((_ptr) + 3) | (*(uint8_t *)((_ptr) + 2) << 8) \
    | (*(uint8_t *)((_ptr) + 1) << 16) | (*(uint8_t *)(_ptr) << 24))
#define encode2byte_big(_ptr, data) \
do { \
    *(uint8_t *)(_ptr) = (uint8_t)((data) >> 8); \
    *(uint8_t *)((_ptr) + 1) = (uint8_t)(data); \
} while (0)

#define encode4byte_big(_ptr, data) \
do { \
    *(uint8_t *)(_ptr) = (uint8_t)((data) >> 24); \
    *(uint8_t *)((_ptr) + 1) = (uint8_t)((data) >> 16); \
    *(uint8_t *)((_ptr) + 2) = (uint8_t)((data) >> 8); \
    *(uint8_t *)((_ptr) + 3) = (uint8_t)(data); \
} while (0)

#define UINT64_TO_BE_STREAM(p, u64) do { \
        *(p)++ = (uint8_t)((u64) >> 56); \
        *(p)++ = (uint8_t)((u64) >> 48); \
        *(p)++ = (uint8_t)((u64) >> 40); \
        *(p)++ = (uint8_t)((u64) >> 32); \
        *(p)++ = (uint8_t)((u64) >> 24); \
        *(p)++ = (uint8_t)((u64) >> 16); \
        *(p)++ = (uint8_t)((u64) >> 8);  \
        *(p)++ = (uint8_t)(u64);         \
} while (0)
#define UINT32_TO_STREAM(p, u32) do {    \
        *(p)++ = (uint8_t)(u32);         \
        *(p)++ = (uint8_t)((u32) >> 8);  \
        *(p)++ = (uint8_t)((u32) >> 16); \
        *(p)++ = (uint8_t)((u32) >> 24); \
} while (0)
#define UINT24_TO_STREAM(p, u24) do {    \
        *(p)++ = (uint8_t)(u24);         \
        *(p)++ = (uint8_t)((u24) >> 8);  \
        *(p)++ = (uint8_t)((u24) >> 16); \
} while (0)
#define UINT16_TO_STREAM(p, u16) do {   \
        *(p)++ = (uint8_t)(u16);        \
        *(p)++ = (uint8_t)((u16) >> 8); \
} while (0)
#define UINT8_TO_STREAM(p, u8) do { \
        *(p)++ = (uint8_t)(u8); \
} while (0)
#define INT8_TO_STREAM(p, u8) do { \
        *(p)++ = (int8_t)(u8); \
} while (0)
#define ARRAY_TO_STREAM(p, a, len) do { \
        int ijk;                                                      \
        for (ijk = 0; ijk < (len); ijk++) { \
            *(p)++ = (uint8_t)(a)[ijk]; \
        } \
    } while (0)
#define STRING_TO_STREAM(p, src, count) do { \
        uint32_t _index; \
        for (_index = 0; _index < (count); _index++) { \
            *(p)++ = *(((uint8_t *)(src)) + _index); \
        } \
    } while (0)
#define STREAM_TO_STRING(dest, p, count) do { \
        uint32_t _index; \
        for (_index = 0; _index < (count); _index++) { \
            *(((uint8_t *)(dest)) + _index) = *(p)++; \
        } \
    } while (0)
#define STREAM_TO_INT8(u8, p) do {  \
        (u8) = (*((int8_t *)(p)));  \
        (p) += 1;                   \
} while (0)
#define STREAM_TO_UINT8(u8, p) do { \
        (u8) = (uint8_t)(*(p));     \
        (p) += 1;                   \
} while (0)
#define STREAM_TO_UINT16(u16, p) do {                                 \
        (u16) = ((uint16_t)(*(p)) + (((uint16_t)(*((p) + 1))) << 8)); \
        (p) += 2;                                                     \
} while (0)
#define STREAM_TO_UINT24(u32, p) do {                                     \
        (u32) = (((uint32_t)(*(p))) + ((((uint32_t)(*((p) + 1)))) << 8) + \
                ((((uint32_t)(*((p) + 2)))) << 16));                      \
        (p) += 3;                                                         \
} while (0)
#define STREAM_TO_UINT32(u32, p) do {                                     \
        (u32) = (((uint32_t)(*(p))) + ((((uint32_t)(*((p) + 1)))) << 8) + \
                ((((uint32_t)(*((p) + 2)))) << 16) +                      \
                ((((uint32_t)(*((p) + 3)))) << 24));                      \
        (p) += 4;                                                         \
} while (0)


#define CONST_UNUSED(x) do { \
    ((x)=(x)); \
} while (0)
#define unused(var) do { \
    (void)(var); \
} while (0)

#define NEW     btos_new
#define FREE    btos_free
#define NEW_    btos_new
#define NEW_S   btos_new_s
#define MEM_NEW     btos_new
#define MEM_FREE    btos_free

#ifdef __cplusplus
}
#endif

#endif