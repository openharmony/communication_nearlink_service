/****************************************************************************
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
 ****************************************************************************/

/****************************************************************************
 *
 * this file contains antenna dfx function of QOSM module.
 *
 ***************************************************************************/
#ifndef QOSM_ANTENNA_DFX_H
#define QOSM_ANTENNA_DFX_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void QOSM_AntennaDfxSendQueryCmd(void);
int QOSM_AntennaDfxGetAntennaPolicy(void);

#ifdef __cplusplus
}
#endif

#endif