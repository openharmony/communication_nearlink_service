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

#ifndef SDF_ERRDEF_H
#define SDF_ERRDEF_H

#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

#define SDF_ERRNO_COMP_MASK 0xF
#define SDF_ERRNO_MOD_MASK 0xF
#define SDF_ERRNO_SUBMOD_MASK 0xFF
#define SDF_ERRNO_ID_MASK 0xFFFFF

#define SDF_ERRNO_COMP_SHIFT 28
#define SDF_ERRNO_MOD_SHIFT 24
#define SDF_ERRNO_SUBMOD_SHIFT 16

/**
 * @brief 子系统定义
 */
typedef enum SDF_StackComp {
    COMP_NAI = 1, /**< 北向 */
    COMP_CP, /**< 控制面 */
    COMP_DP, /**< 数据面 */
    COMP_DLI, /**< DLI */
    COMP_SDF, /**< SDF */
} SDF_StackComp_E;

/**
 * @ingroup errdef
 * @brief 错误码层次结构定义
 *        ---------------------------------------------------------------
 *        |  COMP (4bit)  |  MOD (4bit)  | SUBMOD(8bit) |   ERRNO(16bit) |
 *        ---------------------------------------------------------------
 * @remarks SUBMOD 可选，不使用时填0
 */
#define SDF_MAKE_ERRNO(comp, mod, submod, id) \
    (((SDF_ERRNO_COMP_MASK & (comp)) << SDF_ERRNO_COMP_SHIFT) |       \
     ((SDF_ERRNO_MOD_MASK & (mod)) << SDF_ERRNO_MOD_SHIFT) |          \
     ((SDF_ERRNO_SUBMOD_MASK & (submod)) << SDF_ERRNO_SUBMOD_SHIFT) | \
     (SDF_ERRNO_ID_MASK & (id)))

uint32_t SDF_ErrGetCompId(uint32_t err);
uint32_t SDF_ErrGetModId(uint32_t err);
uint32_t SDF_ErrGetSubmodId(uint32_t err);
uint32_t SDF_ErrGetErrId(uint32_t err);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
