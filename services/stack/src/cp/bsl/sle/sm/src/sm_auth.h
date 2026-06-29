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
#ifndef SM_AUTH_H
#define SM_AUTH_H

#include "sm_slink.h"
#include "sm_struct.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*SmAuthStartFunc_S)(SmSLink_S *slink);

SmSLinkPkgDispatcher_S SmGetAuthPkgDispatcher(NLSTK_SmAuthMethod_E authMethod);
SmAuthStartFunc_S SmGetAuthStartFunc(NLSTK_SmAuthMethod_E authMethod);

#ifdef __cplusplus
}
#endif

#endif /* SM_AUTH_H */