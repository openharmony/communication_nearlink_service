/**
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
 * @file sdf_typedef.h
 * @brief C99基础类型定义
 */

#ifndef SDF_TYPEDEF_H
#define SDF_TYPEDEF_H

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SDF_SIZE_ALIGNED(sz) __attribute__((__aligned__(sz)))

#define SDF_OFFSET_OF(type, field) ((uintptr_t)offsetof(type, field))

#define SDF_TYPE_OF(ptr)    __typeof__((ptr))

#define SDF_CONTAINER_OF(pos, type, field) ((type *)(((uintptr_t)(pos)) - SDF_OFFSET_OF(type, field)))

#define SDF_PtrTypeConvert(type, ptr)  ((type *)(uintptr_t)(ptr))

#define SDF_ArrayCount(array) (sizeof(array) / sizeof((array)[0]))

/**
 * @brief     将 src 向上圆整为 sz 的倍数, 例: 15 圆整 为 4 的倍数为 ((15 + 3) / 4 ) * 4 = 16
 */
#define SDF_SizeRound(src, sz) ((((src) + (sz) - 1) / (sz)) * (sz))

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SDF_TYPEDEF_H */