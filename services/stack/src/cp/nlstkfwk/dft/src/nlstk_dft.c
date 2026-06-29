/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include <time.h>
#include "securec.h"
#include "nlstk_log.h"

#include "nlstk_dft_api.h"
#include "nlstk_dft.h"

static NLSTK_DftCallback_S g_dftCb = {0};

NLSTK_Errcode_E NLSTK_DftRegisterCallback(NLSTK_DftCallback_S *callback)
{
    if (callback == NULL) {
        return NLSTK_ERRCODE_FAIL;
    }
    g_dftCb.cacheCb = callback->cacheCb;
    g_dftCb.reportCb = callback->reportCb;
    return NLSTK_ERRCODE_SUCCESS;
}

void DftGetCurTimestampStr(NLSTK_DftParamValueString_S *str)
{
    if (str == NULL) {
        return;
    }
    (void)memset_s(str, sizeof(NLSTK_DftParamValueString_S), 0, sizeof(NLSTK_DftParamValueString_S));
    time_t rawtime;
    struct tm timeinfo;
    (void)time(&rawtime);
    // 使用 localtime_r 并检查错误
    if (localtime_r(&rawtime, &timeinfo) == NULL) {
        NLSTK_LOG_ERROR("localtime_r failed");
        return;
    }
    size_t size = strftime(str->str, NLSTK_DFT_PARAM_VALUE_STR_MAX_LEN, "%Y-%m-%d:%H:%M:%S", &timeinfo);
    if (size == 0 || size == NLSTK_DFT_PARAM_VALUE_STR_MAX_LEN) {
        return;
    }
    str->str[NLSTK_DFT_PARAM_VALUE_STR_MAX_LEN - 1] = '\0';
}

void DftCacheTimestamp(SLE_Addr_S *addr, uint16_t eventId, uint16_t paramId)
{
    NLSTK_DftParamValueString_S timeStr = {0};
    DftGetCurTimestampStr(&timeStr);
    DftCache(addr, eventId, paramId, NLSTK_DFT_PARAM_VALUE_TYPE_STRING, &timeStr);
}

void DftCache(SLE_Addr_S *addr, uint16_t eventId, uint16_t paramId,
    NLSTK_DftParamValueType_E paramType, void *param)
{
    if (g_dftCb.cacheCb == NULL) {
        return;
    }
    g_dftCb.cacheCb(addr, eventId, paramId, paramType, param);
}

void DftReport(SLE_Addr_S *addr, uint16_t eventId, uint16_t paramId, uint16_t res)
{
    if (g_dftCb.reportCb == NULL) {
        return;
    }
    g_dftCb.reportCb(addr, eventId, paramId, res);
}