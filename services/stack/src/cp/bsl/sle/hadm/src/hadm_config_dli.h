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
/*
 * hadm_config_dli: 该文件用于下发DLI配置.
 */
#ifndef HADM_CONFIG_DLI_H
#define HADM_CONFIG_DLI_H

#include "hadm_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  初始化DLI指令缓存的vector
 */
uint32_t HadmInitDliCmdVec(void);

void HadmDeInitDliCmdVec(void);

// 获取到最开始下发的DLI指令的lcid和期望的返回回调类型
uint16_t HadmPopLastDliCmd(uint16_t *expectRspType);
/**
 * @brief  设置cs config参数
 */
uint32_t HadmSetMeasureParam(uint16_t lcid, HadmSoundingParam_S *args);

/**
 * @brief  设置cs使能参数
 */
uint32_t HadmSetMeasureEnable(uint16_t lcid, uint8_t csEnable);

/**
 * @brief  下发DLI指令，读取远端CS参数
 */
uint32_t HadmReadRemoteMeasureCaps(uint16_t lcid);

#ifdef __cplusplus
}
#endif
#endif