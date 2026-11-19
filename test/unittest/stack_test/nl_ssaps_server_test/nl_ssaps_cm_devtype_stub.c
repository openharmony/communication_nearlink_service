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
#include "cm_def.h"
#include "cm_signaling_internal.h"

// 用例切换对端设备类型：默认 UNKNOWN 走新版协议路径，置 OLD 走老版本路径
uint8_t g_nlSsapsDevType = CM_DEVTYPE_UNKNOWN;

uint8_t CM_GetLogicLinkDeviceType(uint16_t lcid)
{
    (void)lcid;
    return g_nlSsapsDevType;
}
