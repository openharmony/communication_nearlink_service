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
 * @file         sdf_util.h
 * @brief        SDF Util file.
*/

#ifndef SDF_UTIL_H
#define SDF_UTIL_H

#include <string.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define SDF_StrEqual(strA, strB)   (strcmp(strA, strB) == 0)
#define SDF_MAX(a, b) ((a) < (b) ? (b) : (a))
#define SDF_MIN(a, b) ((a) < (b) ? (a) : (b))

/**
 * @brief String deep copy
 *
 * @param s [IN] string
 * @return Deep copy replica of s
 */
char *SDF_StrDup(const char *s);

/**
 * @brief assign uint8_t* to uint32_t
 *
 * @param src [IN] uint8_t*
 * @return uint32_t
 */
uint32_t SDF_PtrAssignUint32(const uint8_t *src);

/**
 * @brief assign uint8_t* to uint64_t
 *
 * @param src [IN] uint8_t*
 * @return uint64_t
 */
uint64_t SDF_PtrAssignUint64(const uint8_t *src);

#ifdef __cplusplus
}
#endif

#endif // SDF_UTIL_H