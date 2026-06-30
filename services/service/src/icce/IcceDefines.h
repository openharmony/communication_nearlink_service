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
#ifndef ICCE_DEFINES_H
#define ICCE_DEFINES_H

#include <cstdint>
#include <string>
#include "ssap_data.h"

namespace OHOS {
namespace Nearlink {

constexpr int ICCE_SUCCESS = 0;
constexpr int ICCE_FAILURE = 1;
constexpr int ICCE_INVAILD = -1;

const std::string PAIR_ICCE_REACH_MAX_NUM = "ICCE Connect reach max";
const std::string PAIR_ICCE_NOT_SUPPORT = "ICCE is not supporting";

enum OperationIndication : uint8_t {
    OPERATION_READ = 0x01, /**< readable */
    OPERATION_WRITE_NO_RESPONSE = 0x02,
    OPERATION_WRITE_WITH_RESPONSE = 0x04,
    OPERATION_NOTIFY = 0x08,
    OPERATION_INDICATION = 0x10,
    OPERATION_BROADCAST = 0x20,
};

} // namespace Sle
} // namespace OHOS
#endif // ICCE_DEFINES_H

