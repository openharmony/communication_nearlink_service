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

/**
 * @addtogroup Nearlink
 * @{
 *
 * @brief Defines a nearlink system that provides basic nearlink connection and profile functions,
 *        including port, etc.
 *
 * @since 6
 */

/**
 * @file nearlink_socket_outputstream.h
 *
 * @brief Declares port inputstream framework functions, including basic functions.
 *
 * @since 6
 */

#ifndef NEARLINK_SOCKET_OUTPUTSTREAM_H
#define NEARLINK_SOCKET_OUTPUTSTREAM_H

#include "nearlink_def.h"
#include "nearlink_types.h"

namespace OHOS {
namespace Nearlink {

enum SocketTransState {
    SLE_TRANS_RESULT_SUCCESS = 0,            // 数据发送成功
    SLE_TRANS_RESULT_INTERNAL_FAULT = -1,    // 内部错误
    SLE_TRANS_RESULT_CACHE_FULL = -2,        // 超出数据发送缓存
    SLE_TRANS_RESULT_INVALID_PARAM = -3,     // 无效参数
};  // 数据发送结果

/**
 * @brief Class for port output stream functions.
 *
 * @since 6
 */
class OutputStream {
public:
    /**
     * @brief A constructor used to create an OutputStream instance.
     *
     * @param socketFd Socket fd.
     * @since 6
     */
    explicit OutputStream(int socketFd);

    /**
     * @brief Destroy the OutputStream object.
     *
     * @since 6
     */
    ~OutputStream();

    /**
     * @brief Socket write.
     *
     * @param buf Data to be written.
     * @param length The length of data to be written.
     * @return Returns <b> SLE_TRANS_RESULT_SUCCESS </b> operation succeeded.
     *         Returns <b> SLE_TRANS_RESULT_INTERNAL_FAULT </b> interal error
     *         Returns <b> SLE_TRANS_RESULT_CACHE_FULL </b> socket cache list is full, operation failed.
     * @since 6
     */
    SocketTransState Write(const uint8_t *buf, size_t length);

private:
    int socketFd_;
    OutputStream() = delete;
};
} // namespace Nearlink
} // namespace OHOS
#endif  // NEARLINK_OUTPUTSTREAM_H