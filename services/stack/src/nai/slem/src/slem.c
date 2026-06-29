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
 * Description: sle manager. thread not safety
 */
#include <stddef.h>
#include <stdbool.h>
#include "sle_crypto.h"
#include "nai_log.h"
#include "nai_dft.h"
#include "nlstk_init_api.h"
#include "slem.h"

void slem_registerCallbacks(SlemCallbacks *callbacks)
{
    return;
}

void slem_deregisterCallbacks(void)
{
    return;
}

int slem_initialize(void)
{
    if (NAI_DftInit() != NLSTK_ERRCODE_SUCCESS) {
        NAI_LOG_ERROR("Gle init stack dft failed.");
    }
    return NLSTK_InitStack();
}

int slem_close(void)
{
    NLSTK_DeinitStack();
    return 0;
}

int slem_enable(void)
{
    return NLSTK_EnableStack();
}

int slem_disable(void)
{
    NLSTK_DisableStack();
    return 0;
}

int slem_is_enabled(void)
{
    return 0;
}