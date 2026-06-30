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
 */

#ifndef SDF_ERRNO_BASE_H
#define SDF_ERRNO_BASE_H

#include "sdf_errdef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SDF_OK 0

typedef enum SDF_Module {
    SDF_MOD_DFX = 1,
    SDF_MOD_BSL,
} SDF_Module_E;

typedef enum SDF_DFX_SubModule {
    SDF_DFX_SUBMOD_LOG = 1,
} SDF_DFX_SubModule_E;

typedef enum SDF_BSL_SubModule {
    SDF_BSL_SUBMOD_DSL = 1,
    SDF_BSL_SUBMOD_FSM,
    SDF_BSL_SUBMOD_LOCK,
    SDF_BSL_SUBMOD_MEMM,
    SDF_BSL_SUBMOD_THREAD,
    SDF_BSL_SUBMOD_WORKER,
    SDF_BSL_SUBMOD_EVC,
    SDF_BSL_SUBMOD_TIMER,
} SDF_BSL_SubModule_E;

#define SDF_MAKE_DFX_LOG_ERRNO(id)     SDF_MAKE_ERRNO(COMP_SDF, SDF_MOD_DFX, SDF_DFX_SUBMOD_LOG, (id))

#define SDF_MAKE_BSL_DSL_ERRNO(id)     SDF_MAKE_ERRNO(COMP_SDF, SDF_MOD_BSL, SDF_BSL_SUBMOD_DSL, (id))
#define SDF_MAKE_BSL_FSM_ERRNO(id)     SDF_MAKE_ERRNO(COMP_SDF, SDF_MOD_BSL, SDF_BSL_SUBMOD_FSM, (id))
#define SDF_MAKE_BSL_LOCK_ERRNO(id)    SDF_MAKE_ERRNO(COMP_SDF, SDF_MOD_BSL, SDF_BSL_SUBMOD_LOCK, (id))
#define SDF_MAKE_BSL_MEMM_ERRNO(id)    SDF_MAKE_ERRNO(COMP_SDF, SDF_MOD_BSL, SDF_BSL_SUBMOD_MEMM, (id))
#define SDF_MAKE_BSL_THREAD_ERRNO(id)  SDF_MAKE_ERRNO(COMP_SDF, SDF_MOD_BSL, SDF_BSL_SUBMOD_THREAD, (id))
#define SDF_MAKE_BSL_WORKER_ERRNO(id)  SDF_MAKE_ERRNO(COMP_SDF, SDF_MOD_BSL, SDF_BSL_SUBMOD_WORKER, (id))
#define SDF_MAKE_BSL_EVC_ERRNO(id)     SDF_MAKE_ERRNO(COMP_SDF, SDF_MOD_BSL, SDF_BSL_SUBMOD_EVC, (id))
#define SDF_MAKE_BSL_TIMER_ERRNO(id)   SDF_MAKE_ERRNO(COMP_SDF, SDF_MOD_BSL, SDF_BSL_SUBMOD_TIMER, (id))

#ifdef __cplusplus
}
#endif

#endif /* SDF_ERRNO_BASE_H */
