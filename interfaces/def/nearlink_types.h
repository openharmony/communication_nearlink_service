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

#ifndef NEARLINK_TYPE_H
#define NEARLINK_TYPE_H

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace OHOS {
namespace Nearlink {

#define NEARLINK_DECLARE_IMPL() \
    struct impl;                 \
    std::unique_ptr<impl> pimpl

#define NEARLINK_DISALLOW_COPY_AND_ASSIGN(TypeName) \
    TypeName(const TypeName &) = delete;             \
    TypeName &operator=(const TypeName &) = delete

#define NEARLINK_DISALLOW_IMPLICIT_CONSTRUCTORS(TypeName) \
    TypeName() = delete;                                   \
    NEARLINK_DISALLOW_COPY_AND_ASSIGN(TypeName)
} // namespace Nearlink
} // namespace OHOS

#endif  // NEARLINK_TYPE_H