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
#include "cpfwk_log.h"
#include "ssaps_service.h"
#include "ssaps_server.h"
#include "ssaps_server_api.h"

uint32_t SSAP_ServerInit(void)
{
    return SSAPS_ServiceInit();
}

void SSAP_ServerDeInit(void)
{
    SSAPS_ServiceDeInit();
}

void SSAP_CacheService(void *arg)
{
    CP_CHECK_LOG_RETURN_VOID(arg != NULL, "[SSAP] SSAP_CacheService arg is null");
    SSAPS_CacheService((SSAP_ParamAddService_S *)arg);
}

void SSAP_CacheProperty(void *arg)
{
    CP_CHECK_LOG_RETURN_VOID(arg != NULL, "[SSAP] SSAP_CacheProperty arg is null");
    SSAPS_CacheProperty((SSAP_ParamAddProperty_S *)arg);
}

void SSAP_CacheDescriptor(void *arg)
{
    CP_CHECK_LOG_RETURN_VOID(arg != NULL, "[SSAP] SSAP_CacheDescriptor arg is null");
    SSAPS_CacheDescriptor((SSAP_ParamAddDescriptor_S *)arg);
}

void SSAP_StartService(void *arg)
{
    (void)arg;
    SSAPS_StartService();
}

void SSAP_RemoveService(void *arg)
{
    CP_CHECK_LOG_RETURN_VOID(arg != NULL, "[SSAP] SSAP_RemoveService arg is null");
    SSAPS_RemoveService((SSAP_ParamRemoveService_S *)arg);
}

void SSAP_UpdateItemValueByHandle(void *arg)
{
    CP_CHECK_LOG_RETURN_VOID(arg != NULL, "[SSAP] SSAP_UpdateItemValueByHandle arg is null");
    SSAPS_UpdateItemValueByHandle(arg);
}

void SSAP_SendUserResponse(void *arg)
{
    CP_CHECK_LOG_RETURN_VOID(arg != NULL, "[SSAP] SSAP_SendUserResponse arg is null");
    SSAPS_SendUserResponse(arg);
}

void SSAP_SendMethodCallRes(void *arg)
{
    CP_CHECK_LOG_RETURN_VOID(arg != NULL, "[SSAP] SSAP_SendMethodCallRes arg is null");
    SSAPS_SendMethodCallRes(arg);
}