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
#ifndef MESSAGE_H
#define MESSAGE_H

namespace utility {
struct Message {
public:
    /**
     * @brief Construct a new Message object.
     *
     * @param what Message Identifier.
     * @param arg1 Message first arg.
     * @param arg2 Message second arg.
     * @since 6
     */
    explicit Message(int what, int arg1 = 0, void *arg2 = nullptr) : whatM(what), arg1M(arg1), arg2M(arg2){};

    /**
     * @brief Construct a new Message object.
     *
     * @since 6
     */
    Message() = default;

    /**
     * @brief Destroy the Message object.
     *
     * @since 6
     */
    ~Message() = default;

    int whatM = 0;
    int arg1M = 0;
    void *arg2M = nullptr;
};
}  // namespace utility

#endif  // MESSAGE_H