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
#ifndef NLSTK_PUBLIC_DEFINE_H
#define NLSTK_PUBLIC_DEFINE_H

#include <stdint.h>
#include "sdf_addr.h"
#include "nlstk_public_define_ext.h"

#ifdef __cplusplus
extern "C" {
#endif

// 连接参数：新增字段初始默认值需要定义为0
typedef struct {
    uint8_t frameType;
} NLSTK_ConnParam_S;

typedef struct {
    SLE_Addr_S addr;
    NLSTK_ConnParam_S param;
} NLSTK_ConnAddrParam_S;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
