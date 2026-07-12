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

/****************************************************************************
 *
 * this file contains antenna dfx function of QOSM module.
 *
 ***************************************************************************/
#include "qosm_log.h"
#include "qosm_antenna_dfx.h"
#include "qosm_ext_func_wrapper.h"

#define QOSM_ANTENNA_DFX_INVALID_POLICY (-1)

void QOSM_AntennaDfxSendQueryCmd(void)
{
    QOSM_EXT_AntennaDfxSendQueryCmd();
    return;
}

int QOSM_AntennaDfxGetAntennaPolicy(void)
{
    return QOSM_EXT_AntennaDfxGetAntennaPolicy();
}
