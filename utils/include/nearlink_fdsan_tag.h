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

#ifndef NEARLINK_FDSAN_TAG_H
#define NEARLINK_FDSAN_TAG_H

#include <stdint.h>

/* Use the platform fdsan header when the sysroot ships one; otherwise declare
 * the OHOS musl exports directly (same symbols HidHostUhid.cpp links against). */
#if defined(__has_include)
#if __has_include(<fdsan.h>)
#include <fdsan.h>
#define NEARLINK_FDSAN_HEADER_FOUND 1
#elif __has_include(<sys/fdsan.h>)
#include <sys/fdsan.h>
#define NEARLINK_FDSAN_HEADER_FOUND 1
#endif
#endif
#ifndef NEARLINK_FDSAN_HEADER_FOUND
#ifdef __cplusplus
extern "C" {
#endif
void fdsan_exchange_owner_tag(int fd, uint64_t old_tag, uint64_t new_tag);
int fdsan_close_with_tag(int fd, uint64_t tag);
#ifdef __cplusplus
}
#endif
#endif

/* fdsan owner tags: one stable value per owning module, allocated from the
 * nearlink LOG_DOMAIN family (utils/include/log.h, 0xD000150). The tag shows up
 * in fdsan reports and names the module owning the fd at that moment. */

#define NEARLINK_FDSAN_TAG_TIMER  0xD000151 /* services/common NearlinkTimer: epoll/event/timer fd */
#define NEARLINK_FDSAN_TAG_SOCKET 0xD000152 /* socket pair fds of PortSocketManager/WorkerContext,
                                               * incl. datatransfer channel ends closed by PortInfo,
                                               * frameworks callback and ipc proxy */
#define NEARLINK_FDSAN_TAG_SNOOP  0xD000153 /* services/hardware SleDliSnoop log file fd */
#define NEARLINK_FDSAN_TAG_DEVCFG 0xD000154 /* services/device_manager AdapterDeviceConfig fd */

#endif /* NEARLINK_FDSAN_TAG_H */
