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

#include "dli_layer_config.h"

#include "dli_layer_callback.h"
#include "dli_log.h"
#include "dli_errno.h"

#include "securec.h"
#ifdef __cplusplus
#include <atomic>
using namespace std;
#else
#include <stdatomic.h>

#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MAX_LINK_NUM
#define MAX_LINK_NUM 8
#endif
#define CONFIG_SIZE 2

struct DLI_TxDataConfig {
    atomic_short num;
    atomic_short len;
};

static struct DLI_TxDataConfig g_txDataConfig[CONFIG_SIZE];

void DLI_AllDataSet(uint16_t acbLen, uint16_t acbNum, uint16_t icbLen, uint16_t icbNum)
{
    atomic_store(&(g_txDataConfig[ACB_DATA_TYPE].len), acbLen);
    atomic_store(&(g_txDataConfig[ACB_DATA_TYPE].num), acbNum);
    atomic_store(&(g_txDataConfig[ICB_DATA_TYPE].len), icbLen);
    atomic_store(&(g_txDataConfig[ICB_DATA_TYPE].num), icbNum);
    DLI_LOGI("configuring ACB [len:%hu, num:%hu] and ICB [len %hu, num:%hu] "
        "links", acbLen, acbNum, icbLen, icbNum);

    DLI_DataNumChange(ACB_DATA_TYPE, acbNum);
    DLI_DataNumChange(ICB_DATA_TYPE, icbNum);
}

uint16_t DLI_DataLenGet(DLI_DataType type)
{
    DLI_CHECK_RETURN_RET(type < UNKNOWN_DATA_TYPE, 0, "type is err");
    return atomic_load(&(g_txDataConfig[type].len));
}

uint16_t DLI_DataNumGet(DLI_DataType type)
{
    DLI_CHECK_RETURN_RET(type < UNKNOWN_DATA_TYPE, 0, "type is err");
    return atomic_load(&(g_txDataConfig[type].num));
}

static atomic_ushort g_cmdNum;
bool DLI_IsCmdAllow(void)
{
    return atomic_load(&g_cmdNum) > 0 ? true : false;
}

void DLI_CmdNumSubOne(void)
{
    atomic_fetch_sub(&g_cmdNum, DLI_DEFAULT_CMD_NUM);
}

void DLI_CmdNumSet(uint8_t num)
{
    atomic_store(&g_cmdNum, num);
}

#ifdef __cplusplus
}
#endif