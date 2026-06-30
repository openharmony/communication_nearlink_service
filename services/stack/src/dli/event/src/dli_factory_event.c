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
#include "dli_factory_event.h"

void DLI_RfTxStartCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    // do something
}

void DLI_RfResetCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{}

void DLI_RfTrxEndCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{}

void DLI_RfRxStartCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{}