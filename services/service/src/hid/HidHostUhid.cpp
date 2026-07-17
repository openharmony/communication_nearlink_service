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
#include "HidHostUhid.h"
#include "HidHostService.h"
#include "log_util.h"
#include "nearlink_dft_exception.h"
#include "qos.h"
#include "SleInterfaceManager.h"
#include "ServiceManagerPluginLoader.h"

namespace OHOS {
namespace Nearlink {
HidHostUhid::HidHostUhid(std::string address)
{
    address_ = address;
    fd_ = -1;
    keepPolling_.store(false);
    readyForData_.store(false);
    taskId_ = 0;
}

HidHostUhid::~HidHostUhid()
{
    LOG_INFO("[UHID] ~HidHostUhid");
    keepPolling_.store(false);
    Destroy();
}

int HidHostUhid::Open()
{
    HILOG_COMM_INFO("[%{public}s:%{public}d][UHID]:Open Start", __FUNCTION__, __LINE__);

    if (fd_ < 0) {
        LOG_INFO("[UHID]:fd is null,creat fd");
        fd_ = open(UHID_DEVICE_PATH, O_RDWR);
        if (fd_ < 0) {
            HILOG_COMM_ERROR("[%{public}s:%{public}d][UHID]:open %{public}s failed, err:%{public}s",
                __FUNCTION__, __LINE__, UHID_DEVICE_PATH, strerror(errno));
            return HID_HOST_FAILURE;
        } else {
            HILOG_COMM_INFO("[%{public}s:%{public}d][UHID]:uhid fd = %{public}d", __FUNCTION__, __LINE__, fd_);
        }
        fdsan_exchange_owner_tag(fd_, 0, LOG_DOMAIN);
        PollEventThread();
        LOG_INFO("[UHID]:uhid fd is not null,fd = %{public}d", fd_);
    }

    return HID_HOST_SUCCESS;
}

int HidHostUhid::Close()
{
    LOG_INFO("[UHID] Close: signal polling thread to stop");
    readyForData_.store(false);
    keepPolling_.store(false);
    waitingTimes.store(0);
    return HID_HOST_SUCCESS;
}

int HidHostUhid::SendData(uint8_t* pRpt, uint16_t len)
{
    LOG_DEBUG("[UHID]");
    if (fd_ >= 0 && waitingTimes < MAX_WAITING_TIME) {
        uint32_t pollingAttempts = 0;
        while (!readyForData_.load() && pollingAttempts < MAX_POLLING_ATTEMPTS) {
            usleep(POLLING_SLEEP_DURATION_US);
            pollingAttempts++;
        }
        if(!readyForData_.load()){
            waitingTimes++;
        }
    }
    // Send the HID data to the kernel.
    if ((fd_ >= 0) && readyForData_.load()) {
        WritePackUhid(fd_, pRpt, len);
    } else {
        LOG_ERROR("[UHID] failed, fd_:%{public}d, readyForData_:%{public}d, len:%{public}hu",
            fd_, readyForData_.load(), len);
        return HID_HOST_FAILURE;
    }
    return HID_HOST_SUCCESS;
}

int HidHostUhid::SendControlData(uint8_t* pRpt, uint16_t len)
{
    LOG_INFO("[UHID]");
    if (fd_ >= 0 && waitingTimes < MAX_WAITING_TIME) {
        uint32_t pollingAttempts = 0;
        while (!readyForData_.load() && pollingAttempts < MAX_POLLING_ATTEMPTS) {
            usleep(POLLING_SLEEP_DURATION_US);
            pollingAttempts++;
        }
        if(!readyForData_.load()){
            waitingTimes++;
        }
    }
    // Send the HID control data to the kernel.
    if ((fd_ >= 0) && readyForData_.load()) {
        if (taskType_ == HID_HOST_DATA_TYPE_GET_REPORT) {
            SendGetReportReplyUhid(fd_, taskId_, HID_HOST_SUCCESS, pRpt, len);
        } else if (taskType_ == HID_HOST_DATA_TYPE_SET_REPORT) {
            SendSetReportReplyUhid(fd_, taskId_, HID_HOST_SUCCESS);
        } else {
            LOG_ERROR("[UHID], Unknow taskType_:%{public}d", taskType_);
        }
        taskId_ = 0;
        taskType_ = -1;
    } else {
        LOG_ERROR("[UHID] failed, fd_:%{public}d, readyForData_:%{public}d, len:%{public}hu",
            fd_, readyForData_.load(), len);
        return HID_HOST_FAILURE;
    }
    return HID_HOST_SUCCESS;
}

int HidHostUhid::SendHandshake(uint16_t err)
{
    LOG_INFO("[UHID], err:%{public}hu", err);
    if (fd_ >= 0 && waitingTimes < MAX_WAITING_TIME) {
        uint32_t pollingAttempts = 0;
        while (!readyForData_.load() && pollingAttempts < MAX_POLLING_ATTEMPTS) {
            usleep(POLLING_SLEEP_DURATION_US);
            pollingAttempts++;
        }
        if(!readyForData_.load()){
            waitingTimes++;
        }
    }
    // Send the HID handshake to the kernel.
    if ((fd_ >= 0) && readyForData_.load()) {
        if (taskType_ == HID_HOST_DATA_TYPE_GET_REPORT) {
            SendGetReportReplyUhid(fd_, taskId_, err, nullptr, 0);
        } else if (taskType_ == HID_HOST_DATA_TYPE_SET_REPORT) {
            SendSetReportReplyUhid(fd_, taskId_, err);
        } else {
            LOG_ERROR("[UHID], Unknow taskType_:%{public}d", taskType_);
        }
        taskId_ = 0;
        taskType_ = -1;
    } else {
        LOG_ERROR("[UHID] failed, fd_:%{public}d, readyForData_:%{public}d",
            fd_, readyForData_.load());
        return HID_HOST_FAILURE;
    }
    return HID_HOST_SUCCESS;
}

int HidHostUhid::SendGetReportReplyUhid(int fd, int id, uint16_t err, uint8_t* rpt, uint16_t len)
{
    struct uhid_event ev;
    memset_s(&ev, sizeof(ev), 0, sizeof(ev));
    ev.type = UHID_FEATURE_ANSWER;
    ev.u.feature_answer.id = static_cast<unsigned int>(id);
    ev.u.feature_answer.err = err;
    ev.u.feature_answer.size = len;
    if (len > sizeof(ev.u.feature_answer.data)) {
        LOG_WARN("[UHID]: Report size greater than allowed size");
        return HID_HOST_FAILURE;
    }
    if (memcpy_s(ev.u.feature_answer.data, sizeof(ev.u.feature_answer.data), rpt, len) != EOK) {
        LOG_ERROR("[UHID]memcpy error");
        return HID_HOST_FAILURE;
    }
    return WriteUhid(fd, &ev);
}

int HidHostUhid::SendSetReportReplyUhid(int fd, int id, uint16_t err)
{
    struct uhid_event ev;
    memset_s(&ev, sizeof(ev), 0, sizeof(ev));
    ev.type = UHID_SET_REPORT_REPLY;
    ev.u.set_report_reply.id = static_cast<unsigned int>(id);
    ev.u.set_report_reply.err = err;

    return WriteUhid(fd, &ev);
}

int HidHostUhid::SendHidInfo(const HidInformation& hidInf)
{
    LOG_INFO("[UHID]: start.");

    struct uhid_event ev;
    int ret ;

    if (fd_ < 0) {
        LOG_WARN("[UHID]Error: fd = %{public}d, dscp_len = %{public}hu", fd_, hidInf.descLength);
        return HID_HOST_FAILURE;
    }
    // Create and send hid descriptor to kernel
    memset_s(&ev, sizeof(ev), 0, sizeof(ev));
    ev.type = UHID_CREATE;
    int uniqLength = snprintf_s(reinterpret_cast<char*>(ev.u.create.uniq), sizeof(ev.u.create.uniq), address_.length(),
        "%s", address_.c_str());
    if (uniqLength < 0) {
        LOG_ERROR("[UHID]: snprintf length error, uniqLength = %{public}d", uniqLength);
    }
    ev.u.create.rd_size = hidInf.descLength;
    ev.u.create.rd_data = hidInf.descInfo.get();
    ev.u.create.bus = BUS_BLUETOOTH;
    std::string disName = "Nearlink HID";
    SleInterfaceAdapter *adapterInterfaceSle =
        (SleInterfaceAdapter *)(SleInterfaceManager::GetInstance()->GetAdapter(ADAPTER_SLE));
    if (adapterInterfaceSle != nullptr) {
        RawAddress device(address_);
        ev.u.create.vendor = adapterInterfaceSle->GetDeviceVendorId(device);
        ev.u.create.product = adapterInterfaceSle->GetDeviceProductId(device);
        ev.u.create.version = adapterInterfaceSle->GetDeviceVersion(device);
        std::string name = adapterInterfaceSle->GetDeviceName(device);
        if (name != "") {
            disName = name;
        }
        LOG_INFO("[UHID]: vendor = %{public}d, product = %{public}d", ev.u.create.vendor, ev.u.create.product);
    }

    if (strncpy_s((char*)ev.u.create.name, sizeof(ev.u.create.name), disName.c_str(), disName.size())
        != EOK) {
        LOG_ERROR("[UHID]: memcpy error");
    }
    ev.u.create.country = hidInf.ctryCode;
    ret = WriteUhid(fd_, &ev);

    HILOG_COMM_INFO("[%{public}s:%{public}d][UHID]: wrote descriptor to fd = %{public}d, "
        "dscp_len = %{public}hu,result = %{public}d", __FUNCTION__, __LINE__, fd_, hidInf.descLength, ret);

    if (ret) {
        LOG_ERROR("[UHID]: Error: failed to send DSCP, result = %{public}d", ret);
        /* The HID report descriptor is corrupted. Close the driver. */
        fdsan_close_with_tag(fd_, LOG_DOMAIN);
        fd_ = -1;
        return HID_HOST_FAILURE;
    }

    return HID_HOST_SUCCESS;
}

int HidHostUhid::Destroy()
{
    if (fd_ >= 0) {
        struct uhid_event ev; // references linux kernel uhid.h
        memset_s(&ev, sizeof(ev), 0, sizeof(ev));
        ev.type = UHID_DESTROY;

        WriteUhid(fd_, &ev);
        LOG_INFO("[UHID]: Closing fd=%{public}d", fd_);
        fdsan_close_with_tag(fd_, LOG_DOMAIN);
        fd_ = -1;
    }
    return HID_HOST_SUCCESS;
}

int HidHostUhid::WriteUhid(int fd, const struct uhid_event* ev)
{
    ssize_t ret;
    do {
    } while ((ret = write(fd, ev, sizeof(*ev))) == -1 && errno == EINTR);
    if (ret < 0) {
        int rtn = errno;
        HILOG_COMM_ERROR("[%{public}s:%{public}d][UHID]: Cannot write to uhid:%{public}s",
            __FUNCTION__, __LINE__, strerror(errno));
        return rtn;
    } else if (ret != static_cast<ssize_t>(sizeof(*ev))) {
        HILOG_COMM_ERROR("[%{public}s:%{public}d][UHID]: Wrong size written to uhid: %{public}zd != %{public}zu",
            __FUNCTION__, __LINE__, ret, sizeof(*ev));
        return EFAULT;
    }
    LOG_INFO("[UHID]: write success");
    ServiceManagerPluginLoader::GetInstance()->HidDataStatisticsProc(address_);
    return HID_HOST_SUCCESS;
}

int HidHostUhid::WritePackUhid(int fd, uint8_t* rpt, uint16_t len)
{
    LOG_DEBUG("[UHID] enter");

    struct uhid_event ev;
    memset_s(&ev, sizeof(ev), 0, sizeof(ev));
    ev.type = UHID_INPUT;
    ev.u.input.size = len;
    if (len > sizeof(ev.u.input.data)) {
        LOG_WARN("[UHID]: Report size greater than allowed size");
        return HID_HOST_FAILURE;
    }
    if (memcpy_s(ev.u.input.data, sizeof(ev.u.input.data), rpt, len) != EOK) {
        LOG_ERROR("[UHID]: memcpy error");
        return HID_HOST_FAILURE;
    }

    return WriteUhid(fd, &ev);
}

void HidHostUhid::PollEventThread()
{
    auto self = shared_from_this();
    ffrt::submit([self] { self->PollEventThreadSub(); }, {}, {},
        ffrt::task_attr().qos(ffrt::qos_user_interactive));
}

void HidHostUhid::PollEventThreadSub()
{
    LOG_INFO("fd:%{public}d execute", fd_);
    int ret = OHOS::QOS::SetThreadQos(OHOS::QOS::QosLevel::QOS_USER_INTERACTIVE);
    if (ret != 0) {
        HILOGE("Increase thread priority failed, ret: %{public}d", ret);
    }
    struct pollfd pfds[1];
    keepPolling_.store(true);
    pfds[0].fd = fd_;
    pfds[0].events = POLLIN;

    // Set the uhid fd as non-blocking to ensure we never block the BTU thread
    SetUhidNonBlocking(fd_);

    while (keepPolling_.load()) {
        int pollRet = -1;
        do {
        } while ((pollRet = poll(pfds, 1, POLL_TIMEOUT)) == -1 && errno == EINTR);
        if (pollRet < 0) {
            LOG_ERROR("[UHID]: Cannot poll for fds: %{public}s", strerror(errno));
            break;
        }
        if (pfds[0].revents & POLLNVAL) {
            LOG_ERROR("[UHID]: fd invalid (closed), exit polling thread");
            break;
        }
        if (pfds[0].revents & POLLIN) {
            LOG_INFO("[UHID]: POLLIN");
            int readRet = ReadUhidEvent();
            if (readRet != 0) {
                LOG_ERROR("[UHID]: readUhidEvent Failed.");
                break;
            }
        }
    }

    keepPolling_.store(false);
    OHOS::QOS::ResetThreadQos();
    LOG_INFO("polling thread exited.");
}

void HidHostUhid::SetUhidNonBlocking(int fd)
{
    int opts = fcntl(fd, F_GETFL);
    if (opts < 0 || opts > UINT8_MAX) {
        LOG_ERROR("[UHID]: Getting flags failed (%{public}s)", strerror(errno));
        return;
    }

    /* set fd O_NONBLOCK */
    uint8_t opt = static_cast<uint8_t>(opts);
    if (fcntl(fd, F_SETFL, opt | O_NONBLOCK) < 0) {
        LOG_ERROR("[UHID]: Setting non-blocking flag failed (%{public}s)", strerror(errno));
    }
}

int HidHostUhid::ReadUhidEvent()
{
    struct uhid_event ev;
    ssize_t ret = 0;
    memset_s(&ev, sizeof(ev), 0, sizeof(ev));
    do {
    } while ((ret = read(fd_, &ev, sizeof(ev))) == -1 && errno == EINTR);

    if (ret <= 0) {
        LOG_ERROR("[UHID]:read err");
        return HID_HOST_FAILURE;
    }
    HILOG_COMM_INFO("[%{public}s:%{public}d][UHID]:ev.type:%{public}u", __FUNCTION__, __LINE__, ev.type);

    switch (ev.type) {
        case UHID_START:
            LOG_INFO("[UHID]:UHID_START from uhid-dev");
            readyForData_.store(true);
            waitingTimes.store(0);
            break;
        case UHID_STOP:
            LOG_INFO("[UHID]:UHID_STOP from uhid-dev");
            readyForData_.store(false);
            break;
        case UHID_OPEN:
            LOG_INFO("[UHID]:UHID_OPEN from uhid-dev");
            readyForData_.store(true);
            waitingTimes.store(0);
            DftCacheHidFinish(address_, HID_HOST_SUCCESS);
            break;
        case UHID_CLOSE:
            LOG_INFO("[UHID]:UHID_CLOSE from uhid-dev");
            readyForData_.store(false);
            break;
        case UHID_OUTPUT:
            ReadUhidOutPut(ev);
            break;
        case UHID_OUTPUT_EV:
            LOG_INFO("[UHID]:UHID_OUTPUT_EV from uhid-dev");
            break;
        case UHID_FEATURE:
            ReadUhidFeature(ev);
            break;
        case UHID_SET_REPORT:
            ReadUhidSetReport(ev);
            break;
        default:
            LOG_INFO("[UHID]:Invalid event from uhid-dev: %{public}u", ev.type);
    }

    return HID_HOST_SUCCESS;
}

void HidHostUhid::ReadUhidOutPut(uhid_event ev)
{
    HidHostService *hidHostService = HidHostService::GetService();
    NL_CHECK_RETURN(hidHostService, "[UHID]:hidHostService is nullptr.");

    if (ev.u.output.rtype == UHID_FEATURE_REPORT) {
        hidHostService->HidHostSetReport(address_, HID_HOST_FEATURE_REPORT, ev.u.output.size, ev.u.output.data);
    } else if (ev.u.output.rtype == UHID_OUTPUT_REPORT) {
        hidHostService->HidHostSendReport(address_, HID_HOST_OUTPUT_REPORT, ev.u.output.size, ev.u.output.data);
    } else {
        hidHostService->HidHostSetReport(address_, HID_HOST_INPUT_REPORT, ev.u.output.size, ev.u.output.data);
    }
}

void HidHostUhid::ReadUhidFeature(uhid_event ev)
{
    LOG_INFO("[UHID]:UHID_FEATURE from uhid-dev id=%{public}u, rnum=%{public}hhu, rtype=%{public}hhu",
        ev.u.feature.id, ev.u.feature.rnum, ev.u.feature.rtype);

    HidHostService *hidHostService = HidHostService::GetService();
    NL_CHECK_RETURN(hidHostService, "[UHID]:hidHostService is nullptr.");

    if (ev.u.feature.rtype == UHID_FEATURE_REPORT) {
        hidHostService->HidHostGetReport(address_, ev.u.feature.rnum, 0, HID_HOST_FEATURE_REPORT);
    } else if (ev.u.feature.rtype == UHID_OUTPUT_REPORT) {
        hidHostService->HidHostGetReport(address_, ev.u.feature.rnum, 0, HID_HOST_OUTPUT_REPORT);
    } else {
        hidHostService->HidHostGetReport(address_, ev.u.feature.rnum, 0, HID_HOST_INPUT_REPORT);
    }

    taskId_ = static_cast<int>(ev.u.feature.id);
    taskType_ = HID_HOST_DATA_TYPE_GET_REPORT;
}

void HidHostUhid::ReadUhidSetReport(uhid_event ev)
{
    LOG_INFO("[UHID]:UHID_SET_REPORT id=%{public}u,rnum=%{public}hhu,rtype=%{public}hhu,size=%{public}hu",
        ev.u.set_report.id, ev.u.set_report.rnum, ev.u.set_report.rtype, ev.u.set_report.size);

    HidHostService *hidHostService = HidHostService::GetService();
    NL_CHECK_RETURN(hidHostService, "[UHID]:hidHostService is nullptr.");

    if (ev.u.set_report.rtype == UHID_FEATURE_REPORT) {
        hidHostService->HidHostSetReport(address_, HID_HOST_FEATURE_REPORT, ev.u.set_report.size,
            ev.u.set_report.data);
    } else if (ev.u.set_report.rtype == UHID_OUTPUT_REPORT) {
        hidHostService->HidHostSetReport(address_, HID_HOST_OUTPUT_REPORT, ev.u.set_report.size, ev.u.set_report.data);
    } else {
        hidHostService->HidHostSetReport(address_, HID_HOST_INPUT_REPORT, ev.u.set_report.size, ev.u.set_report.data);
    }

    taskId_ = static_cast<int>(ev.u.feature.id);
    taskType_ = HID_HOST_DATA_TYPE_SET_REPORT;
}

} // namespace Sle
} // namespace OHOS
