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
/**
 * @file hadm_config_cm.h
 * @brief 配置管理模块接口定义
 */

#ifndef HADM_CONFIG_CM_H
#define HADM_CONFIG_CM_H

#include "hadm_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 设置连接参数到配置管理模块
 * 该函数用于将连接参数设置到配置管理模块，以便模块能够根据这些参数进行相应的配置和管理。
 * @param addr 设备地址信息，指向设备地址结构体的指针
 * @param param 连接参数，指向连接参数结构体的指针
 * @return uint32_t 返回错误码，0表示成功，非零表示失败
 */
uint32_t HadmSetConnectionParamToCm(SLE_Addr_S *addr, HadmConnectionParam_S *param);

#ifdef __cplusplus
}
#endif
#endif /* HADM_CONFIG_CM_H */