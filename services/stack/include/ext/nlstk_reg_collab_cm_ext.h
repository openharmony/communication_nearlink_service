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

#ifndef NLSTK_REG_COLLAB_CM_EXT_H
#define NLSTK_REG_COLLAB_CM_EXT_H

#include <stdint.h>
#include "nlstk_stm_collab_ext.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t (*startConnReq)(void);
    void (*notifyConnFailed)(void);
} COLLAB_CollabCmCbk_S;

typedef struct {
    uint32_t (*regCollabFunc)(const CM_LinkCollabFunc_S *func);
    void (*unRegCollabFunc)(void);
} COLLAB_CmCollabFunc_S;

#ifdef __cplusplus
}
#endif

#endif