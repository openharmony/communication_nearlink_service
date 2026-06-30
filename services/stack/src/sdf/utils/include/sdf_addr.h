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
 * @file         sdf_def.h
 * @brief        协议栈公共定义
 */
#ifndef SDF_DEF_H
#define SDF_DEF_H

#include <stdint.h>
#include "sdf_traits.h"
#include "sdf_struct.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SDF_UNUSED(var) (void)(var)

int SDF_CompareSleAddr(const void *lhs_, const void *rhs_);
#define SLE_ADDR_TRAITS() MAKE_TRAITS(SDF_MemFree, SDF_CompareSleAddr)

struct SDF_EncryptedLogString GetEncryptAddr(const SLE_Addr_S *addr);
#define GET_ENC_ADDR(addr) ((const char *)(GetEncryptAddr(addr).buf))

#ifdef __cplusplus
}
#endif

#endif