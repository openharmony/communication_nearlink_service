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
#include "cpfwk_log.h"
#include "ssap_pkt.h"
#include "ssaps_server_app.h"
#include "ssap_common.h"

static uint16_t g_defaultServerMtu = SSAP_STACK_MTU_MAX;

void SSAP_SetServerExchangeInfo(void *arg)
{
    CP_CHECK_LOG_RETURN_VOID(arg != NULL, "[SSAP] SSAP_SetServerExchangeInfo arg is null");
    SSAP_ServerExchangeInfo_S *info = (SSAP_ServerExchangeInfo_S *)arg;
    CP_CHECK_LOG_RETURN_VOID(info->mtuSize >= SSAP_STACK_MTU_DEFAULT && info->mtuSize <= SSAP_STACK_MTU_MAX,
        "[SSAP] SSAP_SetServerExchangeInfo wrong mtu: %d", info->mtuSize);
    g_defaultServerMtu = info->mtuSize;
    CP_LOG_INFO("[SSAP] set default mtu size: %d", g_defaultServerMtu);
}

uint16_t SSAP_GetServerMtu(void)
{
    return g_defaultServerMtu;
}
