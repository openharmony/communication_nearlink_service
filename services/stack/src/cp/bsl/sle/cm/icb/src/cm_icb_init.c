/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "cm_icb_init.h"
#include "cm_errno.h"
#include "cm_icb_mgr.h"
#include "cm_log.h"

static volatile bool g_isInited = false;

uint32_t CM_ICBInit(void)
{
    CM_CHECK_RETURN_RET(!g_isInited, CM_SUCCESS, "icb mgr is already inited");
    uint32_t ret = CM_ICBMgrInit();
    if (ret == CM_SUCCESS) {
        g_isInited = true;
    }
    return ret;
}

uint32_t CM_ICBDeinit(void)
{
    CM_CHECK_RETURN_RET(g_isInited, CM_SUCCESS, "icb mgr is already deinited");
    uint32_t ret = CM_ICBMgrDeinit();
    if (ret == CM_SUCCESS) {
        g_isInited = false;
    }
    return ret;
}

uint32_t CM_ICBEnable(void)
{
    CM_CHECK_RETURN_RET(g_isInited, CM_SUCCESS, "icb mgr is not inited");
    return CM_ICBMgrEnable();
}

uint32_t CM_ICBDisable(void)
{
    CM_CHECK_RETURN_RET(g_isInited, CM_SUCCESS, "icb mgr is not inited");
    return CM_ICBMgrDisable();
}

bool CM_ICBIsInited(void)
{
    return g_isInited;
}
