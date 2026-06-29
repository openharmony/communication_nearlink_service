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
#ifndef SSAPS_SERVICE_PARAM_H
#define SSAPS_SERVICE_PARAM_H

#include "ssap_type.h"
#include "nlstk_ssap_app_server.h"
#include "sdf_vector.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 释放服务对象及相关资源
 *
 * @param val 指向需要释放的服务对象的指针
 */
void FreeService(void *val);

/**
 * @brief 释放属性对象及相关资源
 *
 * @param val 指向需要释放的属性对象的指针
 */
void FreeProperty(void *val);

/**
 * @brief 释放描述符对象及相关资源
 *
 * @param val 指向需要释放的描述符对象的指针
 */
void FreeDescriptor(void *val);

/**
 * @brief 释放客户端属性配置描述符对象及相关资源
 *
 * @param val 指向需要释放的客户端配置对象的指针
 */
void FreeClientConfig(void *val);

/**
 * @brief 分配并初始化一个 SSAP 服务对象，基于给定的 NLSTK 服务参数
 *
 * @param service 指向 NLSTK 服务参数结构的指针
 * @return SSAP_Service_S* 返回指向新分配的 SSAP 服务对象的指针，如果失败则返回 NULL
 */
SSAP_Service_S *SsapAllocServiceParam(const NLSTK_ServiceParam_S *service);
#ifdef __cplusplus
}
#endif
#endif