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

/* PR-3 批次A（CM-10/11/12）事件闭环入口声明放在 cmicb_fuzzer.cpp 的 OHOS 命名空间内
 * （头文件规范不允许在本文件声明命名空间），本头文件仅承载工程宏。 */
#ifndef TEST_FUZZTEST_CMICB_EVT_FUZZER_H
#define TEST_FUZZTEST_CMICB_EVT_FUZZER_H

#include <cstddef>
#include <cstdint>
#include <unistd.h>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>

#ifndef FUZZ_PROJECT_NAME
#define FUZZ_PROJECT_NAME "cmicb_fuzzer"
#endif

#endif // TEST_FUZZTEST_CMICB_EVT_FUZZER_H
