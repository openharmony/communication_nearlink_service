/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include "securec.h"
#include "sdf_errno_base.h"
#include "sdf_log_api.h"
#ifndef NEARLINK_SERVICE_STACK_LOCAL_TEST
#include "log.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  默认日志等级为SDF_LOG_LEVEL_ERROR
 */
uint32_t g_logLevel = SDF_LOG_LEVEL_DEBUG;

/**
 * @brief  存储日志打印函数地址
 */
SDF_LogHook g_fnLogOut = (SDF_LogHook)(uintptr_t)printf;

/**
 * @brief     日志打印接口注册函数
 * @param[IN] fnHook 日志打印钩子,不能为NULL
 * @return    uint32_t
 * @retval    成功(SDF_OK)
 * @retval    失败(SDF_ERR)
 * @remarks   该接口可选用,未注册将默认使用printf;
 */
uint32_t SDF_LogHookReg(SDF_LogHook fnHook)
{
    if (fnHook != NULL) {
        g_fnLogOut = fnHook;
        return SDF_OK;
    }
    return SDF_LOG_ERROR_INVALID_PARAM;
}

/**
 * @brief     设置日志记录级别
 * @param[IN] logLevel 日志等级
 * @return    void
 * @retval    NA
 * @remarks   该接口可选用,默认等级SDF_LOG_LEVEL_ERROR
 */
void SDF_LogLevelSet(SDF_LogLevel_E logLevel)
{
    /* 将用户设置的LOG级别信息赋给全局变量 */
    g_logLevel = (uint32_t)logLevel;
    return;
}

/**
 * @brief     获取日志记录级别
 * @return    uint32_t
 * @retval    日志等级
 * @remarks
 */
uint32_t SDF_LogLevelGet()
{
    return g_logLevel;
}

#ifndef NEARLINK_SERVICE_STACK_LOCAL_TEST
void SDF_FormatLog(char *temp, uint32_t size, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    (void)memset_s(temp, size, 0, size);
    int ret = vsnprintf_s(temp, size, size - 1, fmt, args);
    if (ret < 0) {
        HILOGE("vsnprintf_s error");
        va_end(args);
        return;
    }
    va_end(args);
}
#endif

void SDF_GetDateTime(char *buf, size_t bufSize)
{
    time_t rawtime;
    struct tm timeinfo;

    (void)time(&rawtime);
    localtime_r(&rawtime, &timeinfo);
    (void)strftime(buf, bufSize, "%y/%m/%d %H:%M:%S", &timeinfo);
}

#ifdef __cplusplus
}
#endif
