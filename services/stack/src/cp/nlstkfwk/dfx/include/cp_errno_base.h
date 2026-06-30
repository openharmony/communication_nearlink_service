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
#ifndef CP_ERRNO_BASE_H
#define CP_ERRNO_BASE_H

#include "sdf_errdef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CP_OK 0

typedef enum tagCP_Module {
    CP_MOD_FWK = 1,
    CP_MOD_NBC,
    CP_MOD_NCP,
} CP_Module;

typedef enum tagCP_FWK_SubModule {
    CP_FWK_SUBMOD_ALL = 1,
} CP_FWK_SubModule;

typedef enum tagCP_NCP_SubModule {
    CP_NCP_SUBMOD_CM = 1,
    CP_NCP_SUBMOD_DEVD,
    CP_NCP_SUBMOD_HADM,
    CP_NCP_SUBMOD_MM,
    CP_NCP_SUBMOD_QOSM,
    CP_NCP_SUBMOD_SERVM,
    CP_NCP_SUBMOD_SM,
    CP_NCP_SUBMOD_COLLAB,
} CP_NCP_SubModule;

#define CP_MAKE_FWK_ALL_ERRNO(id)     SDF_MAKE_ERRNO(COMP_CP, CP_MOD_FWK, CP_FWK_SUBMOD_ALL, (id))

#define CP_MAKE_NCP_SM_ERRNO(id)      SDF_MAKE_ERRNO(COMP_CP, CP_MOD_NCP, CP_NCP_SUBMOD_SM, (id))

#define CP_MAKE_NCP_DEVD_ERRNO(id)    SDF_MAKE_ERRNO(COMP_CP, CP_MOD_NCP, CP_NCP_SUBMOD_DEVD, (id))

#define CP_MAKE_NCP_CM_ERRNO(id)      SDF_MAKE_ERRNO(COMP_CP, CP_MOD_NCP, CP_NCP_SUBMOD_CM, (id))

#define CP_MAKE_NCP_HADM_ERRNO(id)    SDF_MAKE_ERRNO(COMP_CP, CP_MOD_NCP, CP_NCP_SUBMOD_HADM, (id))

#define CP_MAKE_NCP_QOSM_ERRNO(id)    SDF_MAKE_ERRNO(COMP_CP, CP_MOD_NCP, CP_NCP_SUBMOD_QOSM, (id))

#define CP_MAKE_NCP_COLLAB_ERRNO(id)  SDF_MAKE_ERRNO(COMP_CP, CP_MOD_NCP, CP_NCP_SUBMOD_COLLAB, (id))

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CP_ERRNO_BASE_H */
