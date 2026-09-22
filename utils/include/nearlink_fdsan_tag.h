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

/* 优先包含平台 fdsan 头；sysroot 没有时直接声明 OHOS musl 导出的符号 */
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

/* fdsan owner tag：每个持有 fd 的模块分配一个固定值，取自 nearlink LOG_DOMAIN 段
 * （utils/include/log.h，0xD000150 起）；fdsan 报错日志里以此识别 fd 归属模块 */

#define NEARLINK_FDSAN_TAG_TIMER  0xD000151 /* NearlinkTimer 的 epoll/event/timer fd */
#define NEARLINK_FDSAN_TAG_SOCKET 0xD000152 /* socketpair fd 全族：PortSocketManager/WorkerContext，
                                               * 含数传通道两端及 IPC 收包侧 */
#define NEARLINK_FDSAN_TAG_SNOOP  0xD000153 /* SleDliSnoop 日志文件 fd */
#define NEARLINK_FDSAN_TAG_DEVCFG 0xD000154 /* AdapterDeviceConfig 配置文件 fd */

#endif /* NEARLINK_FDSAN_TAG_H */
