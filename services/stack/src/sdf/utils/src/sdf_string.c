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
#include "securec.h"
#include "sdf_string.h"

typedef enum {
    SDF_ONE     = 1,
    SDF_TWO,
    SDF_THREE,
    SDF_NUM_END
} SIZE_NUM_E;

struct SDF_Uint8LogString GetFormatHexWithSpaces(const uint8_t *src, size_t size)
{
    struct SDF_Uint8LogString res;
    int pos = 0;    // 用来追踪当前字符串位置
    for (size_t i = 0; i < size && pos < SDF_UINT8_LOG_STR_LEN - 1; i++) {
        // 每个字节以两位十六进制输出
        pos += sprintf_s(&res.buf[pos], SDF_UINT8_LOG_STR_LEN - 1 - pos, "%02X", src[i]);
        if (i < size - 1 && pos < SDF_UINT8_LOG_STR_LEN - 1) {
            res.buf[pos++] = ' ';
        }
    }
    res.buf[pos] = '\0';  // 结束符
    return res;
}

struct SDF_Uint8LogString GetFormatHexWithoutSpaces(const uint8_t *src, size_t size)
{
    struct SDF_Uint8LogString res;
    int pos = 0;    // 用来追踪当前字符串位置
    for (size_t i = 0; i < size && pos < SDF_UINT8_LOG_STR_LEN - 1; i++) {
        // 每个字节以两位十六进制输出
        pos += sprintf_s(&res.buf[pos], SDF_UINT8_LOG_STR_LEN - 1 - pos, "%02X", src[i]);
    }
    res.buf[pos] = '\0';  // 结束符
    return res;
}

struct SDF_Uint8LogString GetEncryptedHexWithSpaces(const uint8_t *src, size_t size)
{
    struct SDF_Uint8LogString res;
    int pos = 0;    // 用来追踪当前字符串位置
    if (size == 1 || size == SDF_TWO) {
        // 如果字节流长度为1或2，则正常输出
        for (size_t i = 0; i < size && pos < SDF_UINT8_LOG_STR_LEN - 1; i++) {
            pos += sprintf_s(&res.buf[pos], SDF_UINT8_LOG_STR_LEN - 1 - pos, "%02X", src[i]);
            if (i < size - 1 && pos < SDF_UINT8_LOG_STR_LEN - 1) {
                res.buf[pos++] = ' ';
            }
        }
    } else if (size == SDF_THREE) {
        // 如果字节流长度为3，加密中间字节
        pos += sprintf_s(&res.buf[pos], SDF_UINT8_LOG_STR_LEN - 1 - pos, "%02X", src[0]);
        res.buf[pos++] = ' ';
        res.buf[pos++] = '*';
        res.buf[pos++] = '*';
        res.buf[pos++] = ' ';
        pos += sprintf_s(&res.buf[pos], SDF_UINT8_LOG_STR_LEN - 1 - pos, "%02X", src[SDF_TWO]);
    } else if (size > SDF_THREE) {
        // 如果字节流长度大于3，除头部两字节及尾部一字节，都加密
        // 输出头部两个字节
        pos += sprintf_s(&res.buf[pos], SDF_UINT8_LOG_STR_LEN - 1 - pos, "%02X", src[0]);
        res.buf[pos++] = ' ';
        pos += sprintf_s(&res.buf[pos], SDF_UINT8_LOG_STR_LEN - 1 - pos, "%02X", src[1]);
        res.buf[pos++] = ' ';
        // 输出中间的字节（用 "**" 替代）
        for (size_t i = 2; i < size - 1; i++) {
            if (pos + SDF_THREE < SDF_UINT8_LOG_STR_LEN - 1) {
                res.buf[pos++] = '*';
                res.buf[pos++] = '*';
                res.buf[pos++] = ' ';
            }
        }
        // 输出尾部一个字节
        pos += sprintf_s(&res.buf[pos], SDF_UINT8_LOG_STR_LEN - 1 - pos, "%02X", src[size - 1]);
    }
    res.buf[pos] = '\0';  // 结束符
    return res;
}
