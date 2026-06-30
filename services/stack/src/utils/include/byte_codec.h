/****************************************************************************
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
****************************************************************************/

#ifndef BYTE_CODEC_H
#define BYTE_CODEC_H

#ifdef __cplusplus
extern "C" {
#endif

#define BITS_OF_BYTE 8

#define DECODE2BYTE_LITTLE(_ptr) (uint16_t)(*(uint8_t *)(_ptr) | (*((uint8_t *)(_ptr) + 1) << 8))

#define DECODE3BYTE_LITTLE(_ptr)                                          \
    (uint32_t)(*(uint8_t *)(_ptr) | (*((uint8_t *)(_ptr) + 1) << 8) |     \
    (*((uint8_t *)(_ptr) + 2) << 16))

#define DECODE4BYTE_LITTLE(_ptr)                                          \
    (uint32_t)(*(uint8_t *)(_ptr) | (*((uint8_t *)(_ptr) + 1) << 8) |     \
    (*((uint8_t *)(_ptr) + 2) << 16) | (*((uint8_t *)(_ptr) + 3) << 24))

#define ENCODE2BYTE_LITTLE(_ptr, data)                           \
do {                                                             \
    *((uint8_t *)(_ptr) + 1) = (uint8_t)((uint16_t)(data) >> 8); \
    *(uint8_t *)(_ptr) = (uint8_t)(data);                        \
} while (0)

#define ENCODE3BYTE_LITTLE(_ptr, value)                          \
do {                                                             \
    *((uint8_t *)(_ptr) + 2) = (uint8_t)((value) >> 16);         \
    *((uint8_t *)(_ptr) + 1) = (uint8_t)((value) >> 8);          \
    *(uint8_t *)(_ptr) = (uint8_t)(value);                       \
} while (0)

#define ENCODE4BYTE_LITTLE(_ptr, value)                          \
do {                                                             \
    *((uint8_t *)(_ptr) + 3) = (uint8_t)((value) >> 24);         \
    *((uint8_t *)(_ptr) + 2) = (uint8_t)((value) >> 16);         \
    *((uint8_t *)(_ptr) + 1) = (uint8_t)((value) >> 8);          \
    *(uint8_t *)(_ptr) = (uint8_t)(value);                       \
} while (0)

#define ENCODE2BYTE_BIG(_ptr, data)                              \
do {                                                             \
    *(uint8_t *)(_ptr) = (uint8_t)((data) >> 8);                 \
    *((uint8_t *)(_ptr) + 1) = (uint8_t)(data);                  \
} while (0)

#define DECODE2BYTE_BIG(_ptr) (uint16_t)(*((uint8_t *)(_ptr) + 1) | (*(uint8_t *)(_ptr) << 8))

#ifdef __cplusplus
}
#endif

#endif  // BYTE_CODEC_H