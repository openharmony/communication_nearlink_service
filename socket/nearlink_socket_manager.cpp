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

#include "nearlink_socket_manager.h"
#include "securec.h"
#include "nearlink_safe_map.h"
#include <thread>
#include <mutex>
#include <future>
#include "log_util.h"
#include "pthread.h"

namespace OHOS {
namespace Nearlink {
namespace {
constexpr int PIPE_BUFFER_SIZE = 1024;
constexpr int MAX_EPOLL_EVENTS = 1024;
constexpr int MIN_BUFFER_SIZE_TO_SET = 4 * 1024;  // 4KB
constexpr int MAX_BUFFER_SIZE_TO_SET = 50 * 1024; // 50KB
constexpr int EPOLL_WAIT_TIME_OUT = 200; // ms
const char* NEARLINK_PORT_THREAD = "nearlink_port_thread";
constexpr int CREATE_SOCKET_FAILD = -1;
constexpr int INVALID_FD = -1;
constexpr int END_TAG_LEN = 1;
}
struct PortSocketManager::impl {
    impl();
    ~impl();
    class WorkerThread; // 工作线程类
    NearlinkSafeMap<SocketConnectionParams, std::shared_ptr<PortInfo>> ports_;
    std::shared_ptr<WorkerThread> worker_ = nullptr;
};

class PortSocketManager::impl::WorkerThread {
public:
    WorkerThread();
    ~WorkerThread();
    void ListenFd(int fd);
    void RemoveFd(int fd);
    void RunThread();
    void StopThread();
    void RegisterCallback(int fd, std::function<void(void)> callback);
    void DeregisterCallback(int fd);
    void ClearData(std::shared_ptr<InputStream> inputStream);

private:
    void Start();
    void Stop();
    void Run();
    bool CheckPipeMsg(int fd);
    std::unique_ptr<WorkerContext> context_;
    std::thread thread_;
    std::mutex controlMutex_; // 控制线程按需启停的保护锁
    std::promise<void> promise;
    NearlinkSafeMap<int, std::function<void(void)>> callbacks_; // <fd, callback>
};

// Function to check read/write capability of a file descriptor
void PortSocketManager::CheckFdCapability(int fd) {
    NL_CHECK_RETURN(fd != INVALID_FD, "Invalid file descriptor");

    int flags = fcntl(fd, F_GETFL);
    NL_CHECK_RETURN(flags != INVALID_FD, "Failed to get flags for fd = %{public}d errno:%{public}d", fd, errno);

    // Determine read/write capability
    bool can_read = false;
    bool can_write = false;

    uint8_t flag = static_cast<uint8_t>(flags);
    if (flag & O_RDONLY) {
        can_read = true;
    } else if (flag & O_WRONLY) {
        can_write = true;
    } else if (flag & O_RDWR) {
        can_read = true;
        can_write = true;
    } else if (flag == 0) {
        can_read = true; // 假设默认可读
        can_write = true; // 假设默认可写
    }

    // Output capabilities
    std::string cap;
    if (can_read && can_write) {
        cap = " READ/WRITE";
    } else if (can_read) {
        cap = " READ_ONLY";
    } else if (can_write) {
        cap = " WRITE_ONLY";
    } else {
        cap = " INVALID (Cannot read/write)";
    }
    HILOGI("File Descriptor capability: %{public}s", cap.c_str());
}

PortSocketManager::PortSocketManager()
{
    if (pimpl == nullptr) {
        pimpl = std::make_unique<impl>();
    }
    HILOGI("successful");
}

PortSocketManager::~PortSocketManager()
{}

bool PortSocketManager::IsValidFd(int fd)
{
    int flags = fcntl(fd, F_GETFD);
    NL_CHECK_RETURN_RET(flags != INVALID_FD, false, "invalid fd");
    return true;
}

int PortSocketManager::CreateSocketPair(uint16_t portId, const std::string &address, uint16_t mtu,
    std::function<void(std::shared_ptr<InputStream>, uint16_t, std::string)> dataCallback)
{
    HILOGI("enter");
    int sv[2];
    NL_CHECK_RETURN_RET(socketpair(AF_UNIX, SOCK_STREAM, 0, sv) != -1,
        CREATE_SOCKET_FAILD, "Failed to create socket pair");

    HILOGD("create service_socket_fd: %{public}d framework_socket_fd: %{public}d, portId: %{public}d",
        sv[0], sv[1], portId);
    SetNonblock(sv[0]); // local fd
    SetNonblock(sv[1]); // remote fd
    if (SetBufferSize(sv[0], MAX_BUFFER_SIZE_TO_SET) != ReturnValue::RET_NO_ERROR
        || SetBufferSize(sv[1], MAX_BUFFER_SIZE_TO_SET) != ReturnValue::RET_NO_ERROR) { // set buff size
        close(sv[0]);
        close(sv[1]);
        return -1;
    }

    std::shared_ptr<PortInfo> port = std::make_shared<PortInfo>(portId, address, mtu, sv[0]);
    SocketConnectionParams params(portId, address);
    HILOGI("portId:%{public}d, address:%{public}s", portId, GetEncryptAddr(address).c_str());
    pimpl->ports_.EnsureInsert(params, port);
    auto callbackFunc = [inputStream = port->inputStream, portId, address, callback = dataCallback]() {
        callback(inputStream, portId, address);
    };
    pimpl->worker_->RegisterCallback(sv[0], callbackFunc);
    HILOGI("create socketpair success");
    return sv[1];
}

// set receiveEvent func
void PortSocketManager::SetSocket(uint16_t portId, const std::string &address, uint16_t mtu, int fd,
    std::function<void(std::shared_ptr<InputStream>, uint16_t, std::string)> dataCallback)
{
    HILOGI("enter");
    NL_CHECK_RETURN(fd != INVALID_FD, "invalid fd");

    CheckFdCapability(fd);

    std::shared_ptr<PortInfo> port = std::make_shared<PortInfo>(portId, address, mtu, fd);
    SocketConnectionParams params(portId, address);
    pimpl->ports_.EnsureInsert(params, port);
    HILOGD("portId:%{public}d, address:%{public}s, fd:%{public}d", portId, GetEncryptAddr(address).c_str(), fd);
    auto callbackFunc = [inputStream = port->inputStream, portId, address, callback = dataCallback]() {
        callback(inputStream, portId, address);
    };
    pimpl->worker_->RegisterCallback(fd, callbackFunc);
    HILOGI("setsocket success");
}

SocketTransState PortSocketManager::SendData(uint16_t portId, const std::string &address, const uint8_t* buffer,
    size_t bufferSize, size_t dataSize)
{
    HILOGD("SendData portId:%{public}d, address: %{public}s", portId, GetEncryptAddr(address).c_str());
    SocketTransState ret = SLE_TRANS_RESULT_SUCCESS;
    NL_CHECK_RETURN_RET(dataSize <= bufferSize, SLE_TRANS_RESULT_INVALID_PARAM, "invalid data length.");
    SocketConnectionParams params(portId, address);
    bool res = pimpl->ports_.GetValueAndOpt(params,
        [buffer, bufferSize, dataSize, &ret](SocketConnectionParams param, std::shared_ptr<PortInfo>& portInfo) {
        if (dataSize > portInfo->mtu_) {
            HILOGW("the packet length exceeds the mtu. packet_len=%{public}zu, mtu=%{public}d",
                dataSize, portInfo->mtu_);
        }
        ret = portInfo->outputStream->Write(buffer, bufferSize);
    });
    NL_CHECK_RETURN_RET(res, ret, "Failed to find socketInfo");
    return ret;
}

void PortSocketManager::DestroyPort(uint16_t portId)
{
    HILOGI("enter");
    pimpl->ports_.FindAndRmv([portId, this](SocketConnectionParams params, const std::shared_ptr<PortInfo>& portInfo) {
        if (params.portId == portId) {
            HILOGI("DestroyPort portId:%{public}d", portId);
            pimpl->worker_->DeregisterCallback(portInfo->fd_);
            pimpl->worker_->RemoveFd(portInfo->fd_);
            HILOGI("Destroy socket success by portId");
            return true;
        }
        return false;
    });
}

void PortSocketManager::DestroyPeerPort(uint16_t portId, const std::string &address)
{
    HILOGI("enter");
    pimpl->ports_.FindAndRmv([this, portId, address](SocketConnectionParams params,
        const std::shared_ptr<PortInfo>& portInfo) {
        if (params.portId == portId && params.address == address) {
            HILOGI("DestroyPeerPort portId:%{public}d, address: %{public}s", portId, GetEncryptAddr(address).c_str());
            pimpl->worker_->DeregisterCallback(portInfo->fd_);
            pimpl->worker_->RemoveFd(portInfo->fd_);
            HILOGI("Destroy socket success by address");
            return true;
        }
        return false;
    });
}

void PortSocketManager::Listen(uint16_t portId, const std::string &address)
{
    SocketConnectionParams param(portId, address);
    pimpl->ports_.GetValueAndOpt(param,
        [this](SocketConnectionParams params, const std::shared_ptr<PortInfo>& portInfo) {
        pimpl->worker_->ListenFd(portInfo->fd_);
        HILOGI("Listen portId:%{public}d, address: %{public}s", params.portId, GetEncryptAddr(params.address).c_str());
    });
}
void PortSocketManager::RunThread()
{
    NL_CHECK_RETURN(pimpl->worker_, "worker_ is nullptr.");
    pimpl->worker_->RunThread();
}

PortSocketManager::impl::impl()
    : worker_(std::make_shared<WorkerThread>())
{}

PortSocketManager::impl::~impl()
{
    auto func = [worker = worker_](const SocketConnectionParams params, const std::shared_ptr<PortInfo>& portInfo) {
        worker->RemoveFd(portInfo->fd_);
    };
    ports_.Iterate(func);
    ports_.Clear();
}

PortSocketManager::impl::WorkerThread::WorkerThread()
    : context_(std::make_unique<WorkerContext>())
{
    HILOGI("constructor");
}

PortSocketManager::impl::WorkerThread::~WorkerThread()
{
    HILOGI("Enter");
    Stop();
}

void PortSocketManager::impl::WorkerThread::Start()
{
    HILOGI("enter");
    context_->running = true;
    if (thread_.joinable()) {
        HILOGI("thread joinable");
        thread_.join();
        HILOGI("thread joinable finished");
    }
    thread_ = std::thread(&WorkerThread::Run, this);
    HILOGI("WorkerThread run");
}

void PortSocketManager::impl::WorkerThread::ListenFd(int fd)
{
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = fd;
    if (epoll_ctl(context_->epfd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        HILOGE("add fd failed");
    };
    HILOGD("EPOLL_CTL_ADD fd: %{public}d", fd);
}

void PortSocketManager::impl::WorkerThread::RemoveFd(int fd)
{
    HILOGD("enter");
    if (epoll_ctl(context_->epfd, EPOLL_CTL_DEL, fd, nullptr) == -1) {
        HILOGD("already delete poll for fds: %{public}s", strerror(errno));
    }
    HILOGD("EPOLL_CTL_DEL fd: %{public}d", fd);
    StopThread();
}

void PortSocketManager::impl::WorkerThread::RunThread()
{
    HILOGI("RunThread enter");
    std::lock_guard<std::mutex> lock(controlMutex_);
    HILOGI("controlMutex_ enter");
    if (!context_->running) {
        HILOGI("need start");
        Start();
    }
}

void PortSocketManager::impl::WorkerThread::StopThread()
{
    HILOGI("StopThread enter");
    std::lock_guard<std::mutex> lock(controlMutex_);
    HILOGI("controlMutex_ enter");
    if (callbacks_.IsEmpty() && context_->running == true) {
        Stop();
    }
}

void PortSocketManager::impl::WorkerThread::RegisterCallback(int fd, std::function<void(void)> callback)
{
    callbacks_.EnsureInsert(fd, callback);
}

void PortSocketManager::impl::WorkerThread::DeregisterCallback(int fd)
{
    callbacks_.Erase(fd);
}

void PortSocketManager::impl::WorkerThread::Stop()
{
    HILOGI("WorkerThread stop enter");
    context_->running = false;
    uint8_t p = 1;
    SocketTransState ret = context_->outputStream->Write(&p, sizeof(p));
    if (ret != SLE_TRANS_RESULT_SUCCESS) {
        HILOGE("Failed to write pipe fd");
    }
    if (thread_.joinable()) {
        HILOGI("thread joinable");
        thread_.join();
        HILOGI("thread joinable finished");
    }
    if (ret == SLE_TRANS_RESULT_SUCCESS) { // 保护多线程场景下 workerThread 走的异常退出流程，没有读取wake_read_fd
        ClearData(context_->inputStream);
    }
}

void PortSocketManager::impl::WorkerThread::ClearData(std::shared_ptr<InputStream> inputStream)
{
    uint8_t buf[SOCKET_BUFFER_SIZE];
    while (true) {
        int ret = inputStream->Read(buf, sizeof(buf));
        if (ret == -1) { // 循环读取数据, 清空socket数据
            break;
        }
    }
}

bool PortSocketManager::impl::WorkerThread::CheckPipeMsg(int fd)
{
    uint8_t buf[PIPE_BUFFER_SIZE];
    int ret = context_->inputStream->Read(buf, sizeof(buf));
    NL_CHECK_RETURN_RET(ret == END_TAG_LEN, false, "end tage len invalid.");
    return true;
}

void PortSocketManager::impl::WorkerThread::Run()
{
    HILOGI("run begin");
    pthread_t threadId = pthread_self();
    pthread_setname_np(threadId, NEARLINK_PORT_THREAD);

    while (context_->running) {
        struct epoll_event events[MAX_EPOLL_EVENTS];
        int nfds = epoll_wait(context_->epfd, events, MAX_EPOLL_EVENTS, EPOLL_WAIT_TIME_OUT);
        if (nfds <= 0) continue;
        for (int i = 0; i < nfds; ++i) {
            bool isValid = PortSocketManager::IsValidFd(events[i].data.fd);
            if (!isValid) { // 检查无效fd
                HILOGI("delete invalid fd.");
                (void)epoll_ctl(context_->epfd, EPOLL_CTL_DEL, events[i].data.fd, nullptr);
                continue;
            }
            if (events[i].events & EPOLLHUP) { // 处理连接关闭的情况
                HILOGI("connection closed.");
                (void)epoll_ctl(context_->epfd, EPOLL_CTL_DEL, events[i].data.fd, nullptr);
                continue;
            }
            if (events[i].data.fd == context_->wakeReadFd && CheckPipeMsg(context_->wakeReadFd)) {
                break;
            }
            std::function<void(void)> callback;
            bool ret = callbacks_.GetValue(events[i].data.fd, callback);
            if (ret) {
                callback();
            } else {
                HILOGW("can not find callbacks_");
            }
        }
    }
    HILOGI("run end");
}

ReturnValue PortSocketManager::SetBufferSize(int fd, int bufferSize)
{
    HILOGD("SetBufferSize bufferSize is %{public}d.", bufferSize);
    NL_CHECK_RETURN_RET(bufferSize >= MIN_BUFFER_SIZE_TO_SET && bufferSize <= MAX_BUFFER_SIZE_TO_SET,
        ReturnValue::RET_BAD_PARAM, "SetBufferSize param is invalid.");

    NL_CHECK_RETURN_RET(fd > 0,
        ReturnValue::RET_BAD_PARAM, "SetBufferSize socket fd is invalid.");

    const std::pair<const char*, int> sockOpts[] = {
        {"recvBuffer", SO_RCVBUF},
        {"sendBuffer", SO_SNDBUF},
    };
    for (auto opt : sockOpts) {
        int curSize = 0;
        socklen_t optlen = sizeof(curSize);
        if (getsockopt(fd, SOL_SOCKET, opt.second, &curSize, &optlen) != 0) {
            HILOGE("SetBufferSize getsockopt %{public}s failed.", opt.first);
            return ReturnValue::RET_BAD_STATUS;
        }
        HILOGD("SetBufferSize %{public}s before set size is %{public}d.", opt.first, curSize);

        if (curSize != bufferSize) {
            int setSize = bufferSize / 2;
            if (setsockopt(fd, SOL_SOCKET, opt.second, &setSize, sizeof(setSize)) != 0) {
                HILOGE("SetBufferSize setsockopt  %{public}s failed.", opt.first);
                return ReturnValue::RET_BAD_STATUS;
            }

            curSize = 0;
            if (getsockopt(fd, SOL_SOCKET, opt.second, &curSize, &optlen) != 0) {
                HILOGE("SetBufferSize after getsockopt %{public}s failed.", opt.first);
                return ReturnValue::RET_BAD_STATUS;
            }
            HILOGD("SetBufferSize %{public}s after set size is %{public}d.", opt.first, curSize);
        }
    }
    return ReturnValue::RET_NO_ERROR;
}

}
}