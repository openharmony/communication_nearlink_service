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
  * @file sdf_errdef.h
 * @brief 错误码基本定义与规划原则
 *        1.明确定义错误码范围：按照不同功能模块对错误码进行分类，每个分类分配一个独立的范围
 *        2.统一管理错误码信息：文档化每个错误码的含义，以及导致错误可能的原因
 *        3.保持错误码的稳定性：尽量避免变更已定义的错误码
 *        4.错误码层次结构设计：按照组件、模块、子模块设计错误码的层次结构，并确保错误码唯一
 *        5.错误码辅助问题定位：错误码捕获、错误码统计、错误码记录日志
 * @version 1.0
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
