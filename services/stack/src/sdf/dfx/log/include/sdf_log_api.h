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

#ifndef SDF_LOG_API_H
#define SDF_LOG_API_H

#include <stddef.h>
#include <stdio.h>
#include "sdf_errno_base.h"
#ifndef NEARLINK_SERVICE_STACK_LOCAL_TEST
#include "log.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Thread Error Code
/* The parameter is invalid. */
#define SDF_LOG_ERROR_INVALID_PARAM    SDF_MAKE_DFX_LOG_ERRNO(2)

#define SDF_LOG_COMP_NAI "NAI"
#define SDF_LOG_COMP_CP "CP"
#define SDF_LOG_COMP_DP "DP"
#define SDF_LOG_COMP_SDF "SDF"

/**
 * @brief 日志等级
 */
typedef enum SDF_LogLevel {
    SDF_LOG_LEVEL_FATAL = 0, /**< 严重级别 */
    SDF_LOG_LEVEL_ERROR, /**< 错误级别 */
    SDF_LOG_LEVEL_WARNING, /**< 告警级别 */
    SDF_LOG_LEVEL_INFO, /**< 信息级别 */
    SDF_LOG_LEVEL_DEBUG, /**< 调试级别 */
} SDF_LogLevel_E;

#ifdef FATAL
#undef FATAL
#endif
#define FATAL 0

#ifdef ERROR
#undef ERROR
#endif
#define ERROR 1

#ifdef WARN
#undef WARN
#endif
#define WARN 2

#ifdef INFO
#undef INFO
#endif
#define INFO 3

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 4

/**
 * @brief     日志输出钩子
 * @param[IN] fmt 格式化字符串
 * @return    void
 * @retval    NA
 * @remarks
 */
typedef void (*SDF_LogHook)(const char *fmt, ...);

/**
 * @brief     日志打印接口注册函数
 * @param[IN] fnHook 日志打印钩子,不能为NULL
 * @return    uint32_t
 * @retval    成功(SDF_OK)
 * @retval    失败(SDF_ERR)
 * @remarks   该接口可选用,未注册将默认使用printf;
 */
uint32_t SDF_LogHookReg(SDF_LogHook fnHook);

/**
 * @brief     设置日志记录级别
 * @param[IN] logLevel 日志等级
 * @return    void
 * @retval    NA
 * @remarks   该接口可选用,默认等级SDF_LOG_LEVEL_ERROR
 */
void SDF_LogLevelSet(SDF_LogLevel_E logLevel);

/**
 * @brief 获取日志记录级别
 * @return    uint32_t
 * @retval    日志等级
 * @remarks
 */
uint32_t SDF_LogLevelGet();

/**
 * @brief 存放当前日志等级
 */
extern uint32_t g_logLevel;

/**
 * @brief 存放日志打印钩子函数
 */
extern SDF_LogHook g_fnLogOut;

#ifndef NEARLINK_SERVICE_STACK_LOCAL_TEST
#define SDF_FMT_MAX_LEN 256

void SDF_FormatLog(char *temp, uint32_t size, const char* fmt, ...);

#define SDF_PUB_LOG_FATAL(comp, fmt, args...)                                       \
    do {                                                                            \
        char tempBuf[SDF_FMT_MAX_LEN];                                              \
        SDF_FormatLog(tempBuf, SDF_FMT_MAX_LEN, fmt, ##args);                       \
        HILOGF("[%{public}s]%{public}s", comp, tempBuf);                            \
    } while (0)

#define SDF_PUB_LOG_ERROR(comp, fmt, args...)                                       \
    do {                                                                            \
        char tempBuf[SDF_FMT_MAX_LEN];                                              \
        SDF_FormatLog(tempBuf, SDF_FMT_MAX_LEN, fmt, ##args);                       \
        HILOGE("[%{public}s]%{public}s", comp, tempBuf);                            \
    } while (0)

#define SDF_PUB_LOG_WARN(comp, fmt, args...)                                        \
    do {                                                                            \
        char tempBuf[SDF_FMT_MAX_LEN];                                              \
        SDF_FormatLog(tempBuf, SDF_FMT_MAX_LEN, fmt, ##args);                       \
        HILOGW("[%{public}s]%{public}s", comp, tempBuf);                            \
    } while (0)

#define SDF_PUB_LOG_INFO(comp, fmt, args...)                                        \
    do {                                                                            \
        char tempBuf[SDF_FMT_MAX_LEN];                                              \
        SDF_FormatLog(tempBuf, SDF_FMT_MAX_LEN, fmt, ##args);                       \
        HILOGI("[%{public}s]%{public}s", comp, tempBuf);                                              \
    } while (0)

#define SDF_PUB_LOG_DEBUG(comp, fmt, args...)                                       \
    do {                                                                            \
        char tempBuf[SDF_FMT_MAX_LEN];                                              \
        SDF_FormatLog(tempBuf, SDF_FMT_MAX_LEN, fmt, ##args);                       \
        HILOGD("[%{public}s]%{public}s", comp, tempBuf);                            \
    } while (0)
#else

#define SDF_DATE_TIME_BUF_SIZE 32
void SDF_GetDateTime(char *buf, size_t bufSize);

/**
 * @brief   日志输出接口
 * @param[IN] comp 模块
 * @param[IN] level 日志等级
 * @param[IN] fmt 格式化字符串
 * @remarks 打印高于配置等级日志
 */
#define SDF_PUB_LOG(comp, level, fmt, args...)                                      \
    do {                                                                            \
        if (g_fnLogOut != NULL) {                                                   \
            if (level <= g_logLevel) {                                              \
                char tempBuf[SDF_DATE_TIME_BUF_SIZE];                                   \
                SDF_GetDateTime(tempBuf, SDF_DATE_TIME_BUF_SIZE);                       \
                g_fnLogOut("[%s][%s][%s] " fmt "\n", tempBuf, comp, #level, ##args);    \
            }                                                                       \
        }                                                                           \
    } while (0)

#define SDF_PUB_LOG_FATAL(comp, fmt, args...) SDF_PUB_LOG(comp, FATAL, fmt, ##args)
#define SDF_PUB_LOG_ERROR(comp, fmt, args...) SDF_PUB_LOG(comp, ERROR, fmt, ##args)
#define SDF_PUB_LOG_WARN(comp, fmt, args...) SDF_PUB_LOG(comp, WARN, fmt, ##args)
#define SDF_PUB_LOG_INFO(comp, fmt, args...) SDF_PUB_LOG(comp, INFO, fmt, ##args)
#define SDF_PUB_LOG_DEBUG(comp, fmt, args...) SDF_PUB_LOG(comp, DEBUG, fmt, ##args)

#endif

#ifdef __cplusplus
}
#endif

#endif /* SDF_LOG_H */
