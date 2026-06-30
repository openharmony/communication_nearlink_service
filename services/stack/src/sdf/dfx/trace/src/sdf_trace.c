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
#include "sdf_trace.h"
#ifndef NEARLINK_SERVICE_STACK_LOCAL_TEST
#include "nearlink_dft_excep_c.h"
#include "nearlink_dft_database.h"
#endif

#define ACCESS_TYPE_SLE 1

#ifndef NEARLINK_SERVICE_STACK_LOCAL_TEST
void SDF_SsapTrace(uint8_t *addr, int errorCode, int subErrorCode)
{
    WriteTemplateThreeExcep(DFT_STACK_SSAP_EVENT_EXCEP, addr, ACCESS_TYPE_SLE, errorCode, subErrorCode);
}
#else
void SDF_SsapTrace(uint8_t *addr, int sceneCode, int subSceneCode)
{
    (void)addr;
    (void)sceneCode;
    (void)subSceneCode;
}
#endif