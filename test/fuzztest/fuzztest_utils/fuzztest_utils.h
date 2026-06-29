/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef FUZZTEST_UTILS_H
#define FUZZTEST_UTILS_H

#include <string>
#include "securec.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "raw_address.h"
#include "sle_uuid.h"

namespace OHOS {
constexpr size_t FUZZ_DATA_LEN = 6;
namespace Nearlink {
uint32_t GetU32Data(const uint8_t* data);
uint64_t GetU64Data(const uint8_t* data);
uint16_t GetU16IntData(const uint8_t* data);
std::string BuildAddressString(FuzzedDataProvider &provider);
RawAddress BuildRawAddress(FuzzedDataProvider &provider);
Uuid BuildUuid(FuzzedDataProvider &provider);
}  // namespace Nearlink
}  // namespace OHOS
#endif  // FUZZTEST_UTILS_H