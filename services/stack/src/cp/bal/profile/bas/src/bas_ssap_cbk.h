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
#ifndef BAS_SSAP_CBK_H
#define BAS_SSAP_CBK_H

#include "bas_type.h"
#include "nlstk_bas_def.h"
#include "nlstk_ssap_app_client.h"

#ifdef __cplusplus
extern "C" {
#endif

NLSTK_SsapAppClientCb_S BasGetSsapCbk(void);

#ifdef __cplusplus
}
#endif
#endif
