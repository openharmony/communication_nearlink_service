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
#include "nlstk_public_define.h"
#include "nlstk_log.h"
#include "nlstk_schedule.h"
#include "securec.h"
#include "sdf_mem.h"
#include "nlstk_ssap_app_server.h"
#include "port_server.h"
#include "port_type.h"
#include "nlstk_port_server.h"
#include "ssap_utils.h"

NLSTK_Errcode_E NLSTK_PortAddByUuid(NLSTK_SsapUuid_S *uuid, uint16_t manufactureId, uint16_t portId)
{
    NLSTK_CHECK_RETURN(uuid != NULL, NLSTK_ERRCODE_POINTER_NULL, "[PORT] uuid is null");
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_PortDeleteByUuid(NLSTK_SsapUuid_S *uuid)
{
    NLSTK_CHECK_RETURN(uuid != NULL, NLSTK_ERRCODE_POINTER_NULL, "[PORT] uuid is null");
    return NLSTK_ERRCODE_SUCCESS;
}