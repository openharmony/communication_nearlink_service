/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#ifndef HID_HOST_UHID_H
#define HID_HOST_UHID_H

#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <string>
#include <linux/uhid.h>
#include "HidHostDefines.h"
#include "BaseDef.h"
#include "ffrt_inner.h"

namespace OHOS {
namespace Nearlink {
constexpr int MAX_POLLING_ATTEMPTS = 10;
constexpr int MAX_WAITING_TIME = 22;
constexpr int POLLING_SLEEP_DURATION_US = 5000;
constexpr int POLL_TIMEOUT = 50; // poll timeout, unit ms
constexpr int CLOSE_POLLING_THREAD_TIMEOUT_SEC = 2; // 2s
class HidHostUhid {
public:
    explicit HidHostUhid(std::string address);
    ~HidHostUhid();

    int Destroy();
    int SendHidInfo(const HidInformation& hidInf);
    int SendData(uint8_t* pRpt, uint16_t len);
    int SendControlData(uint8_t* pRpt, uint16_t len);
    int SendHandshake(uint16_t err);
    int Close();
    int Open();
    void PollEventThread();
    void PollEventThreadSub();

private:
    int WriteUhid(int fd, const struct uhid_event* ev);
    int WritePackUhid(int fd, uint8_t* rpt, uint16_t len);
    void SetUhidNonBlocking(int fd);
    int ReadUhidEvent();
    int SendGetReportReplyUhid(int fd, int id, uint16_t err, uint8_t* rpt, uint16_t len);
    int SendSetReportReplyUhid(int fd, int id, uint16_t err);
    void ReadUhidOutPut(uhid_event ev);
    void ReadUhidFeature(uhid_event ev);
    void ReadUhidSetReport(uhid_event ev);

    int fd_ = -1;
    std::atomic_bool keepPolling_ = false;
    std::atomic_bool readyForData_ = false;
    std::string address_;
    int taskId_ = 0;
    int taskType_ = -1;
    ffrt::condition_variable cvfull_;
    ffrt::mutex hidMutex_;
    bool isPollingThreadClosed_ = true;
    std::atomic<int> waitingTimes = 0;

    SLE_DISALLOW_COPY_AND_ASSIGN(HidHostUhid);
}; // HidHostUhid
} // namespace Sle
} // namespace OHOS
#endif // HID_HOST_UHID_H