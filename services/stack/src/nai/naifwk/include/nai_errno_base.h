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

#ifndef NAI_ERRNO_BASE_H
#define NAI_ERRNO_BASE_H

#include "sdf_errdef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NAI_OK 0

typedef enum tagNAI_Module {
    NAI_MOD_API = 1,
    NAI_MOD_CLI,
    NAI_MOD_NLM,
} NAI_Module;

typedef enum tagNAI_API_SubModule {
    NAI_API_SUBMOD_ALL = 1,
} NAI_API_SubModule;

typedef enum tagNAI_CLI_SubModule {
    NAI_CLI_SUBMOD_ALL = 1,
} NAI_CLI_SubModule;

typedef enum tagNAI_NLM_SubModule {
    NAI_NLM_SUBMOD_ALL = 1,
} NAI_NLM_SubModule;

#define NAI_MAKE_API_ALL_ERRNO(id)      SDF_MAKE_ERRNO(COMP_NAI, NAI_MOD_API, NAI_API_SUBMOD_ALL, (id))
#define NAI_MAKE_CLI_ALL_ERRNO(id)      SDF_MAKE_ERRNO(COMP_NAI, NAI_MOD_CLI, NAI_CLI_SUBMOD_ALL, (id))
#define NAI_MAKE_NLM_ALL_ERRNO(id)      SDF_MAKE_ERRNO(COMP_NAI, NAI_MOD_NLM, NAI_NLM_SUBMOD_ALL, (id))

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NAI_ERRNO_BASE_H */
