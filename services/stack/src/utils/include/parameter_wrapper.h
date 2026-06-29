/****************************************************************************
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
****************************************************************************/

#ifndef PARAMETER_WRAPPER_H
#define PARAMETER_WRAPPER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 获取数据类型为int32_t的系统属性值
 * @key:	        系统属性名称
 * @default_value:	默认值
 * @return  若key存在，且key对应的value是[INT32_MIN, INT32_MAX]之间的值，返回系统属性值。其余情况返回默认值
 */
int32_t PropertyGetInt32(const char* key, int32_t default_value);

#ifdef __cplusplus
}
#endif

#endif  // PARAMETER_WRAPPER_H