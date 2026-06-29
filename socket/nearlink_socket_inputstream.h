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

/**
 * @addtogroup Nearlink
 * @{
 *
 * @brief Defines a nearlink system that provides basic nearlink connection and profile functions,
 *        including PORT etc.
 *
 * @since 6
 */

/**
 * @file nearlink_socket_inputstream.h
 *
 * @brief Declares inputstream framework functions, including basic functions.
 *
 * @since 6
 */

#ifndef NEARLINK_SOCKET_INPUTSTREAM_H
#define NEARLINK_SOCKET_INPUTSTREAM_H

#include "nearlink_def.h"
#include "nearlink_types.h"

namespace OHOS {
namespace Nearlink {
/**
 * @brief Class for input stream functions.
 *
 * @since 6
 */
class InputStream {
public:
    /**
     * @brief A constructor used to create an inputStream instance.
     *
     * @param socketFd Socket fd.
     * @since 6
     */
    explicit InputStream(int socketFd);

    /**
     * @brief Destroy the inputStream object.
     *
     * @since 6
     */
    ~InputStream();

    /**
    * @brief Reads data from the Socket.
    *
    * @param buf Buffer to store the read data.
    * @param length Length of data to be read.
    * @return Returns the actual length of data read if greater than <b>0</b>.
    *         Returns <b>0</b> if the socket is closed.
    *         Returns <b>-1</b> if the operation fails or no data is available.
    * @since 6
    */
    ssize_t Read(uint8_t *buf, size_t length);

private:
    int socketFd_;
    InputStream() = delete;
};
} // namespace Nearlink
} // namespace OHOS
#endif  // NEARLINK_INPUTSTREAM_H