/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#ifndef NEARLINK_NATIVE_TOKEN_MOCK_H
#define NEARLINK_NATIVE_TOKEN_MOCK_H

#include <cstdint>
#include <string>

namespace OHOS {
namespace Nearlink {

class NearlinkMockNativeToken {
public:
    explicit NearlinkMockNativeToken(const std::string& process);
    ~NearlinkMockNativeToken();
private:
    uint64_t selfToken_;
};

} // namespace OHOS
} // namespace Nearlink
#endif // NEARLINK_NATIVE_TOKEN_MOCK_H