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

#include <stdio.h> /* fdsan 声明由平台随 <stdio.h> 提供，不得手写 */

/* fdsan owner tag：每个持有 fd 的模块一个固定值。号段独立于 LOG_DOMAIN（避免与
 * 日志分层号混淆回溯），用 'N''L'（0x4E4C）前缀便于在 fdsan 报错中辨认 nearlink
 * 家族；具体段位待与平台 tag 登记约定核对后如需调整只改宏值。
 * 注：HidHostUhid.cpp 旧写法直接用 LOG_DOMAIN 作 tag，登记为遗留，另行整改。 */

#define NEARLINK_FDSAN_TAG_TIMER     0x4E4C0001 /* NearlinkTimer 的 epoll/event/timer fd */
#define NEARLINK_FDSAN_TAG_SOCKET    0x4E4C0002 /* socketpair fd 全族：PortSocketManager/WorkerContext，
                                                 * 含数传通道两端及 IPC 收包侧 */
#define NEARLINK_FDSAN_TAG_SNOOP     0x4E4C0003 /* SleDliSnoop 日志文件 fd */
#define NEARLINK_FDSAN_TAG_DEVCFG    0x4E4C0004 /* AdapterDeviceConfig 配置文件 fd */
#define NEARLINK_FDSAN_TAG_SDF_TIMER 0x4E4C0005 /* sdf timer fd */
#define NEARLINK_FDSAN_TAG_SDF_EVENT 0x4E4C0006 /* sdf event fd */
#define NEARLINK_FDSAN_TAG_SDF_EVC   0x4E4C0007 /* sdf evc 实例 fd（epoll/关闭事件） */

#endif /* NEARLINK_FDSAN_TAG_H */
