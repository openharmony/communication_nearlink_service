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
#ifndef SDF_STRING_H
#define SDF_STRING_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SDF_UINT8_LOG_STR_LEN 256
struct SDF_Uint8LogString {
    char buf[SDF_UINT8_LOG_STR_LEN];
};

struct SDF_Uint8LogString GetFormatHexWithSpaces(const uint8_t *src, size_t size);
struct SDF_Uint8LogString GetFormatHexWithoutSpaces(const uint8_t *src, size_t size);
struct SDF_Uint8LogString GetEncryptedHexWithSpaces(const uint8_t *src, size_t size);

#define SDF_GET_UINT8_STR(src, size) ((const char *)(GetFormatHexWithSpaces(src, size).buf))
#define SDF_GET_UINT8_STR_NO_SPACE(src, size) ((const char *)(GetFormatHexWithoutSpaces(src, size).buf))
#define SDF_GET_ENC_STR(src, size) ((const char *)(GetEncryptedHexWithSpaces(src, size).buf))

#ifdef __cplusplus
}
#endif

#endif