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

#include "qosm.h"

#include "cp_worker.h"
#include "qosm_errno.h"
#include "qosm_icg_mgr.h"
#include "qosm_log.h"
#include "qosm_trans_channel.h"

static void QOSM_InitInner(void *args)
{
    (void)args;
    uint32_t ret = QOSM_TransChannelInit();
    if (ret != QOSM_SUCCESS) {
        QOSM_LOGE("QOSM_TransChannelInit failed.");
    }
}

uint32_t QOSM_Init(void)
{
    uint32_t ret = CP_PostTask(QOSM_InitInner, NULL, NULL);
    QOSM_CHECK_RETURN_RET(ret == CP_OK, QOSM_FAIL, "post task failed.");
    return QOSM_SUCCESS;
}

static void QOSM_DeInitInner(void *args)
{
    QOSM_TransChannelDeInit();
}

void QOSM_DeInit(void)
{
    uint32_t ret = CP_PostTask(QOSM_DeInitInner, NULL, NULL);
    if (ret != CP_OK) {
        QOSM_LOGE("post task failed.");
    }
}

static void QOSM_EnableInner(void *args)
{
    QOSM_ICGMgrEnable();
}

uint32_t QOSM_Enable(void)
{
    QOSM_LOGI("enter");
    uint32_t ret = CP_PostTask(QOSM_EnableInner, NULL, NULL);
    if (ret != CP_OK) {
        QOSM_LOGE("post task failed.");
        return QOSM_POST_TASK_ERR;
    }
    return QOSM_SUCCESS;
}

static void QOSM_DisableInner(void *args)
{
    QOSM_ICGMgrDisable();
}

void QOSM_Disable(void)
{
    uint32_t ret = CP_PostTask(QOSM_DisableInner, NULL, NULL);
    if (ret != CP_OK) {
        QOSM_LOGE("post task failed.");
    }
}