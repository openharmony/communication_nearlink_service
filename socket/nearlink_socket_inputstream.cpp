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

#ifndef LOG_TAG
#define LOG_TAG "nearlink_fwk_socket_inutputstream"
#endif

#include <unistd.h>
#include <cstddef>
#include "log.h"
#include "nearlink_socket_inputstream.h"
#include <cerrno>
#include "sys/socket.h"

namespace OHOS {
namespace Nearlink {
InputStream::InputStream(int socketFd) : socketFd_(socketFd)
{}

InputStream::~InputStream()
{}

ssize_t InputStream::Read(uint8_t *buf, size_t length)
{
    if (socketFd_ == -1) {
        HILOGE("socket closed");
        return 0;
    }

    auto ret = recv(socketFd_, buf, length, MSG_NOSIGNAL);

    HILOGD("ret:%{public}zd", ret);

    if (ret <= 0) {
        if (ret == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            HILOGD("No data available, continue...");
            return ret;
        }
        HILOGE("socket read exception! ret:%{public}zd errno:%{public}d", ret, errno);
    }
    return ret;
}
}  // namespace Nearlink
}  // namespace OHOS