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
 * @brief        SDF Util source file.
*/

#include "securec.h"
#include "sdf_mem.h"
#include "sdf_util.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

char *SDF_StrDup(const char *s)
{
    if (s == NULL) {
        return NULL;
    }
    size_t size = strlen(s);
    char *ret = SDF_MemZalloc(size + 1);
    if (ret != NULL) {
        if (strncpy_s(ret, size + 1, s, size) != 0) {
            SDF_MemFree(ret);
            ret = NULL;
        }
    }
    return ret;
}

uint32_t SDF_PtrAssignUint32(const uint8_t *src)
{
    if (src == NULL) {
        return 0;
    }
    uint32_t dst;
    (void)memcpy_s(&dst, sizeof(uint32_t), src, sizeof(uint32_t));
    return dst;
}

uint64_t SDF_PtrAssignUint64(const uint8_t *src)
{
    uint64_t dst;
    (void)memcpy_s(&dst, sizeof(uint64_t), src, sizeof(uint64_t));
    return dst;
}

#ifdef __cplusplus
}
#endif
