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

#include "dtap_trans.h"

#include "cp_worker.h"
#include "dpfwk_log.h"
#include "dtap_channel.h"
#include "dtap_errno.h"
#include "sdf_timer.h"

static DTAP_TransMode_S *g_dtapTransMode[CM_TRANS_MODE_MAX] = { NULL };

void DTAP_RegisterTransMode(DTAP_TransMode_S *dtapTransMode)
{
    CHECK_AND_RETURN_LOG(DTAP_TAG, dtapTransMode != NULL, "dtapTransMode is null.");

    CHECK_AND_RETURN_LOG(DTAP_TAG, dtapTransMode->getModeType != NULL, "getModeType is null.");
    uint8_t modeType = dtapTransMode->getModeType();
    CHECK_AND_RETURN_LOG(DTAP_TAG, modeType < CM_TRANS_MODE_MAX,
                         "register trans mode failed, mode: %hhu is invalid.", modeType);
    g_dtapTransMode[modeType] = dtapTransMode;
}

DTAP_TransMode_S *DTAP_GetTransMode(uint8_t modeType)
{
    if (modeType >= CM_TRANS_MODE_MAX) {
        return NULL;
    }

    return g_dtapTransMode[modeType];
}

bool DTAP_IsFrameSeqSmaller(uint16_t own, uint16_t other)
{
    return ((uint16_t)(own - other) & DTAP_FRAME_SEQ_SIGN_MASK) == DTAP_FRAME_SEQ_SIGN_MASK;
}

uint16_t DTAP_GetNextFrameSeq(uint16_t seq)
{
    uint16_t nextSeq = seq + 1;
    if (nextSeq > DTAP_FRAME_SEQ_MAX) {
        nextSeq = 0;
    }
    return nextSeq;
}

uint32_t DTAP_TransStartTimer(int *timer, uint16_t expire, bool isPeriodic,
    SDF_TimerCallback cbk, DTAP_Channel_S *channel)
{
    SDF_TimerParam param = {
        .expires = (time_t)expire,
        .period = isPeriodic,
        .callback = cbk,
        .args = channel,
    };

    uint32_t ret = CP_TimerAdd(timer, &param);
    CHECK_AND_RETURN_RET_LOG(DTAP_TAG, ret == SDF_OK, ret, "create timer failed, ret: 0x%08x", ret);
    return DTAP_SUCCESS;
}

void DTAP_TransStopTimer(int *timer)
{
    if (timer == NULL || *timer == INVALID_TIMER_HANDLE) {
        return;
    }

    CP_TimerDel(*timer);
    *timer = INVALID_TIMER_HANDLE;
    return;
}

uint32_t DTAP_TransRestartTimer(int *timer, uint16_t expire, bool isPeriodic,
    SDF_TimerCallback cbk, DTAP_Channel_S *channel)
{
    DTAP_TransStopTimer(timer);

    uint32_t ret = DTAP_TransStartTimer(timer, expire, isPeriodic, cbk, channel);
    CHECK_AND_RETURN_RET_LOG(DTAP_TAG, ret == SDF_OK, ret, "modify timer failed, ret: 0x%08x", ret);
    return ret;
}