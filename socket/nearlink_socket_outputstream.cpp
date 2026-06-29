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

#ifndef LOG_TAG
#define LOG_TAG "nearlink_fwk_socket_outputstream"
#endif

#include <unistd.h>
#include <cerrno>
#include <cinttypes>
#include <sys/time.h>
#include "nearlink_socket_outputstream.h"
#include "log.h"
#include "sys/socket.h"
#include "sys/ioctl.h"

namespace OHOS {
namespace Nearlink {

static constexpr int32_t SOCKET_SEND_TIME_THRESHOLD = 1000; // 1000ms
static constexpr int32_t SOCKET_PACKET_HEAD_LENGTH = 1512;
static int64_t GetNowTimestamp(void)
{
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    int64_t timestamp = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    return timestamp;
}

OutputStream::OutputStream(int socketFd) : socketFd_(socketFd)
{}

OutputStream::~OutputStream()
{}

SocketTransState OutputStream::Write(const uint8_t *buf, size_t length)
{
    NL_CHECK_RETURN_RET(socketFd_ != -1, SLE_TRANS_RESULT_INTERNAL_FAULT, "socket closed.");

    int32_t bufSize;
    socklen_t optlen = sizeof(bufSize);
    int sockOptRet = getsockopt(socketFd_, SOL_SOCKET, SO_SNDBUF, &bufSize, &optlen);
    unsigned long bytesInBuffer = 0;
    int ioctlRet = ioctl(socketFd_, TIOCOUTQ, &bytesInBuffer);
    HILOGD("sockOptRet : %{public}d, ioctlRet : %{public}d, bufSize : %{public}u, bytesInBuffer %{public}lu",
        sockOptRet, ioctlRet, bufSize, bytesInBuffer);
    if (sockOptRet != -1 && ioctlRet != -1 && static_cast<unsigned long>(bufSize) > bytesInBuffer) { // -1代表无权限获取发送队列大小
        // 该方法是跟踪send前socket发送通道是否占满导致发包阻塞
        unsigned long availableLength = static_cast<unsigned long>(bufSize) - bytesInBuffer;
        int32_t sendLength = static_cast<int32_t>(length) + SOCKET_PACKET_HEAD_LENGTH;
        HILOGD("availableLength=%{public}lu, sendLength=%{public}lu",
            availableLength, static_cast<unsigned long>(sendLength));
        if (availableLength < static_cast<unsigned long>(sendLength)) {
            HILOGW("send queue is full, availableLength is %{public}lu, sendlength is %{public}d",
                availableLength, sendLength);
            return SLE_TRANS_RESULT_CACHE_FULL;
        }
    }
    int64_t beginTimestamp = GetNowTimestamp();
    HILOGD("sendSocket socketFd : %{public}d", socketFd_);
    auto res = send(socketFd_, buf, length, MSG_NOSIGNAL);
    int64_t endTimestamp = GetNowTimestamp();
    if (endTimestamp - beginTimestamp > SOCKET_SEND_TIME_THRESHOLD) {
        HILOGE("socket send time %{public}" PRId64, endTimestamp - beginTimestamp);
    }

    HILOGD("socket write data len=%{public}zd", res);
    if (res <= 0) {
        HILOGE("socket write exception! ret:%{public}zd errno:%{public}d", res, errno);
        return SLE_TRANS_RESULT_INTERNAL_FAULT;
    }
    return SLE_TRANS_RESULT_SUCCESS;
}
}  // namespace Nearlink
}  // namespace OHOS