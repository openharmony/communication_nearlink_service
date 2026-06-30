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
#ifndef ICCE_UTILS_H
#define ICCE_UTILS_H

#include "sdf_addr.h"
#include "raw_address.h"

namespace OHOS {
namespace Nearlink {

SLE_Addr_S ConvertToStackAddr(const RawAddress &addr);
RawAddress ConvertSleAddrToRawAddress(SLE_Addr_S *addr);

} // namespace Nearlink
} // namespace OHOS
#endif // ICCE_UTILS_H