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
 * this file contains dtap transparent transmission mode implement.
 *
 ***************************************************************************/

#include <stddef.h>
#include <stdint.h>

#include "securec.h"

#include "cm_def.h"
#include "dli_layer.h"
#include "dtap_errno.h"
#include "dtap_trans.h"
#include "sdf_mem.h"

static uint8_t DTAP_GetTransparentModeType(void)
{
    return CM_TRANS_MODE_TRANSPARENT;
}

static uint32_t DTAP_SendTransparentFrame(DTAP_Channel_S *transChan, uint8_t pi, SDF_Buff_S *buff)
{
    if (SDF_DataLenGet(buff) > transChan->mps) {
        return DTAP_TRANS_TRANSPARENT_PACK_EXCEED_MPS;
    }

    return DTAP_SendFrame(transChan, buff);
}

static void DTAP_TransparentTransFrame(DTAP_Channel_S *transChan, SDF_Buff_S *buff,
                                       int (*recvCb)(DTAP_Data_Info_S *, SDF_Buff_S *))
{
    DTAP_Data_Info_S info = {0};

    info.pi = 0;
    info.lcid = transChan->lcid;
    info.tcid = transChan->srcTcid;
    if (recvCb != NULL) {
        recvCb(&info, buff);
    }
}

static DTAP_TransMode_S g_dtapTransparentMode = {
    .getModeType = DTAP_GetTransparentModeType,
    .checkFrame = NULL,
    .recvFrame = NULL,
    .sendFrame = DTAP_SendTransparentFrame,
    .transFrame = DTAP_TransparentTransFrame,
    .setTransChannelStatus = NULL,
};

void DTAP_RegisterTransparentMode(void)
{
    DTAP_RegisterTransMode(&g_dtapTransparentMode);
}