/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#ifndef TEST_FUZZTEST_CM_LINK_FUZZER_H
#define TEST_FUZZTEST_CM_LINK_FUZZER_H

#include <cstdint>
#include <cstddef>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t FuzzCmLinkInit(void);

void FuzzCmLinkDeInit(void);

void FuzzCmLinkApi(uint8_t *data, size_t size);

#ifdef __cplusplus
}
#endif

#endif // TEST_FUZZTEST_CM_LINK_FUZZER_H
