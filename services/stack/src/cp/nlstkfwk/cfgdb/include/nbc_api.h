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
#ifndef NBC_API_H
#define NBC_API_H

#include "sdf_addr.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 该接口为兼容终端仓上仍被cm调用的nbc模块 */
SLE_Addr_S *NBC_GetPublicAddress(void);

#ifdef __cplusplus
}
#endif
#endif