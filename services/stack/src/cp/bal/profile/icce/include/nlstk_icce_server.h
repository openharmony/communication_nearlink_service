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
#ifndef NLSTK_ICCE_SERVER_H
#define NLSTK_ICCE_SERVER_H

#include <stdint.h>
#include "nlstk_icce_def.h"
#include "nlstk_public_define.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief
* 1) 调用SSAP接口注册appId
* 2）调用SSAP接口添加ICCE服务
* 3）读取ICCE服务信息
* @param[in] icceServiceInfo ICCE服务信息
*/
uint32_t NLSTK_IcceCreateIcceInstance(NLSTK_IcceServiceInfo_S *icceServiceInfo);

/**
* @brief
* 1) 调用SSAP接口注销appId
* 2）调用SSAP接口删除ICCE服务
* 3）释放ICCE服务信息
*/
uint32_t NLSTK_IcceDestroyIcceInstance(void);


#ifdef __cplusplus
}
#endif
#endif /* NLSTK_ICCE_SERVER_H */