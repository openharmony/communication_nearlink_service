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
/**
 * @brief HADM模块初始化函数
 * @return uint32_t 状态码，0表示成功，非0表示失败
 * @details 初始化HADM模块，包括以下步骤：
 *          1. 注册DLI回调函数
 *          2. 注册CM链路监听回调函数
 *          3. 初始化链路控制块管理模块
 * @note 如果任何初始化步骤失败，将调用HadmDeInit进行反初始化，并返回相应的错误码
 */

#include "nlstk_log.h"
#include "nlstk_public_define.h"
#include "hadm_listen_cm.h"
#include "hadm_listen_dli.h"
#include "hadm_link_manager.h"
#include "hadm_config_dli.h"
#include "hadm_init.h"

uint32_t HadmInit(void)
{
    /* Step 1: 注册DLI回调函数 */
    uint32_t ret = HadmRegDliCbk();
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[HADM] Register DLI callbacks fail.");
        HadmDeInit();
        return ret;
    }

    /* Step 2: 注册CM链路监听回调函数 */
    ret = HadmRegListenCmLink();
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[HADM] Register CM logic link listener callback fail.");
        HadmDeInit();
        return ret;
    }

    /* Step 3: 初始化链路控制块管理模块 */
    ret = HadmInitSoundingCbManager();
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[HADM] Initialize link callback manager fail.");
        HadmDeInit();
        return ret;
    }
    ret = HadmInitDliCmdVec();
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[HADM] Initialize DLI command vector fail.");
        HadmDeInit();
        return ret;
    }

    return ret;
}

/**
 * @brief HADM模块反初始化函数
 * @return void
 * @details 反初始化HADM模块，包括以下步骤：
 *          1. 反初始化链路控制块管理模块
 *          2. 反注册CM链路监听回调函数
 *          3. 反注册DLI回调函数
 */
void HadmDeInit(void)
{
    /* Step 1: 反初始化链路控制块管理模块 */
    HadmDeInitLinkCbManager();

    /* Step 2: 反注册CM链路监听回调函数 */
    HadmUnregListenCmLink();

    /* Step 3: 反注册DLI回调函数 */
    HadmUnRegDliCbk();

    HadmDeInitDliCmdVec();
}