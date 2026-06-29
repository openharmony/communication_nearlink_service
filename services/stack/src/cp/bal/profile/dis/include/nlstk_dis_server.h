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
#ifndef NLSTK_DIS_SERVER_H
#define NLSTK_DIS_SERVER_H

#include "nlstk_dis_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief
* 1) 调用SSAP接口注册appId
* 2) 添加DIS服务 NLSTK_SsapAddService()
* 3) 读取设备信息
* @param[in] devInfo 设备信息
*/
NLSTK_Errcode_E NLSTK_DisCreateInstance(NLSTK_DeviceInfo_S* devInfo);

NLSTK_Errcode_E NLSTK_DisUpdateLocalDeviceName(NLSTK_VariableData_S* name);

/**
* @brief
* 1) 调用SSAP接口注销appId
* 2) 删除DIS服务
* 3) 释放设备信息
*/
NLSTK_Errcode_E NLSTK_DisDestroyInstance(void);

#ifdef __cplusplus
}
#endif
#endif /* NLSTK_DIS_SERVER_H */