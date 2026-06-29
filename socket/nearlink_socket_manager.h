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

#ifndef NEARLINK_SOCKET_MANAGER_H
#define NEARLINK_SOCKET_MANAGER_H

#include <functional>
#include <atomic>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <memory>
#include <nearlink_types.h>
#include "nearlink_socket_inputstream.h"
#include "nearlink_socket_outputstream.h"
#include <log.h>

namespace OHOS {
namespace Nearlink {
constexpr int SOCKET_BUFFER_THRESHOLD_REPORT = 40 * 1024;
constexpr int SOCKET_BUFFER_SIZE = 50 * 1024;

// 设置文件描述符为非阻塞模式
void SetNonblock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        HILOGE("Failed to get flags for fd = %{public}d errno:%{public}d", fd, errno);
        return;
    }
    uint8_t flag = static_cast<uint8_t>(flags);
    if (fcntl(fd, F_SETFL, flag | O_NONBLOCK) == -1) {
        HILOGE("Failed to set flags nonblock for fd = %{public}d errno:%{public}d", fd, errno);
    };
}

// Port信息结构体
struct PortInfo {

    PortInfo() = delete;
    PortInfo(uint16_t port, std::string address, uint16_t mtu, int fd)
        : port_(port), address_(address), mtu_(mtu), fd_(fd) {
        inputStream = std::make_shared<InputStream>(fd_);
        outputStream = std::make_shared<OutputStream>(fd_);
    }

    ~PortInfo()
    {
        HILOGI("PortInfo close");
        if (fd_ != -1) {
            close(fd_);
        }
    }
public:
    uint16_t port_;
    std::string address_;
    uint16_t mtu_;
    int fd_;
    std::shared_ptr<InputStream> inputStream;
    std::shared_ptr<OutputStream> outputStream;
};

struct SocketConnectionParams {
    uint16_t portId;     // local portId
    std::string address; // remote address
    SocketConnectionParams(uint16_t portId_, std::string address_) : portId(portId_), address(address_)
    {};

    bool operator == (const SocketConnectionParams &rhs) const {
        return portId == rhs.portId && address == rhs.address;
    }

    bool operator < (const SocketConnectionParams& rhs) const {
        if (portId < rhs.portId) {
            return true;
        } else if (portId > rhs.portId) {
            return false;
        } else {
            return address < rhs.address;
        }
    }
};

// Worker线程信息结构 用于 控制 工作线程启动和退出
struct WorkerContext {
    int epfd;
    int wakeReadFd;
    int wakeWriteFd;
    std::shared_ptr<InputStream> inputStream;
    std::shared_ptr<OutputStream> outputStream;
    std::atomic<bool> running{false};

    WorkerContext() : epfd(-1), wakeReadFd(-1), wakeWriteFd(-1) {
        HILOGI("construct enter");
        epfd = epoll_create1(0);
        if (epfd == -1) {
            HILOGE("Failed to create worker epfd");
            return;
        }

        int fds[2];

        if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == -1) {
            HILOGE("Failed to create worker wake pair");
            return;
        }

        HILOGD("create : wakeReadFd %{public}d, wakeWriteFd: %{public}d", fds[0], fds[1]);
        SetNonblock(fds[0]); // read fd
        SetNonblock(fds[1]); // write fd

        wakeReadFd = fds[0];
        wakeWriteFd = fds[1];

        inputStream = std::make_shared<InputStream>(wakeReadFd);
        outputStream = std::make_shared<OutputStream>(wakeWriteFd);

        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = wakeReadFd;
        if (epoll_ctl(epfd, EPOLL_CTL_ADD, wakeReadFd, &ev) == -1) {
            HILOGE("Failed to EPOLL_CTL_ADD wakeReadFd");
        };
    }

    ~WorkerContext() {
        if (epfd != -1) {
            close(epfd);
        }
        if (wakeReadFd != -1) {
            close(wakeReadFd);
        }
        if (wakeWriteFd != -1) {
            close(wakeWriteFd);
        }
    }

private:
    int pipe2(int* fds) {
        return pipe(fds);
    }
};

// 端口Socket管理类
class PortSocketManager {
public:

    PortSocketManager();
    ~PortSocketManager();

    // called by service
    int CreateSocketPair(uint16_t portId, const std::string &address, uint16_t mtu,
        std::function<void(std::shared_ptr<InputStream>, uint16_t, std::string)> dataCallback);
    // called by framework
    void SetSocket(uint16_t portId, const std::string &address, uint16_t mtu, int fd,
        std::function<void(std::shared_ptr<InputStream>, uint16_t, std::string)> dataCallback);
    SocketTransState SendData(uint16_t portId, const std::string &address, const uint8_t* buffer, size_t bufferSize,
        size_t dataSize);

    void DestroyPort(uint16_t portId);
    void DestroyPeerPort(uint16_t portId, const std::string &address);

    void Listen(uint16_t portId, const std::string &address);
    void RunThread();
private:
    ReturnValue SetBufferSize(int fd, int bufferSize);
    void CheckFdCapability(int fd);
    static bool IsValidFd(int fd);
    NEARLINK_DECLARE_IMPL();
    NEARLINK_DISALLOW_COPY_AND_ASSIGN(PortSocketManager);
};

} // namespace Nearlink
} // namespace OHOS
#endif  // NEARLINK_SOCKET_MANAGER_H