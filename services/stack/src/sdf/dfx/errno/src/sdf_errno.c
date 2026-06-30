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

#include <stdio.h>
#include <stdint.h>
#include "sdf_errdef.h"

#ifdef __cplusplus
extern "C" {
#endif

uint32_t SDF_ErrGetCompId(uint32_t err)
{
    return (err >> SDF_ERRNO_COMP_SHIFT) & SDF_ERRNO_COMP_MASK;
}

uint32_t SDF_ErrGetModId(uint32_t err)
{
    return (err >> SDF_ERRNO_MOD_SHIFT) & SDF_ERRNO_MOD_MASK;
}

uint32_t SDF_ErrGetSubmodId(uint32_t err)
{
    return (err >> SDF_ERRNO_SUBMOD_SHIFT) & SDF_ERRNO_SUBMOD_MASK;
}

uint32_t SDF_ErrGetErrId(uint32_t err)
{
    return err & SDF_ERRNO_ID_MASK;
}

#ifdef __cplusplus
}
#endif
