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
#ifndef MCP_DEFS_H
#define MCP_DEFS_H

#include "audio_stutter.h"

constexpr uint8_t MCP_INSTANCE_ID = 0xFF;   // 此为媒体播放控制服务中的媒体实例标识必选字段，当前协议并未明确规定使用，值为模块内开发自定义

/* MCP event */
constexpr int MCP_EVT_STARTUP = 1;
constexpr int MCP_EVT_SHUTDOWN = 2;
constexpr int MCP_EVT_PLAY = 3;
constexpr int MCP_EVT_STOP = 4;
constexpr int MCP_EVT_PAUSE = 5;
constexpr int MCP_EVT_FAST_FOR = 6;
constexpr int MCP_EVT_PRE_MEDIA = 7;
constexpr int MCP_EVT_NEXT_MEDIA = 8;
constexpr int MCP_EVT_START_SUCCESS = 9;

/* Operation ID list for MCP commands */
#define MCP_ID_PLAY 0x11        /* 播放 */
#define MCP_ID_STOP 0x12        /* 停止 */
#define MCP_ID_PAUSE 0x13       /* 暂停 */
#define MCP_ID_FAST_FOR 0x14    /* 快进 */
#define MCP_ID_PRE_MEDIA 0x15   /* 上一个 */
#define MCP_ID_NEXT_MEDIA 0x16  /* 下一个 */

/* MCP define av playback state */
constexpr int MCP_PLAYBACK_STATE_PLAY = 1;
constexpr int MCP_PLAYBACK_STATE_PAUSE = 2;
/* MCP define timers */
constexpr int32_t MCP_PLAYBACK_PAUSE_DEBOUNCE_TIMEOUT = 1100; // 1100ms
constexpr int32_t MCP_STATE_CHANGE_WAIT_TIMEOUT = 1100; // 1100ms
/* 最大上限数量, 音频框架对齐规格所有pipe中的流加起来最多256条 */
constexpr int32_t MAX_ROUTE_FLAG_LIST_SIZE = 500;

enum class McpRenderState : uint8_t {
    KUninitalize = 0,
    KPausing = 1,
    KPlaying = 2,
};
constexpr int32_t ALL_UID = -1; // 被监听应用的uid，-1监听所有
constexpr int32_t MUTE_MAP = (1 << OHOS::AudioStandard::BadDataTransferType::NO_DATA_TRANS) |
            (1 << OHOS::AudioStandard::BadDataTransferType::SILENCE_DATA_TRANS); // 检测静音流，数据为0/音量为0
constexpr int64_t CHECK_TIME_INTERVAL = 3000000000; // 3000000000ns  3s
constexpr int64_t CHECK_TIME_INTERVAL_MS = 3000; // 3s
constexpr int32_t BAD_FRAMES_RATIO = 100;
// 空静音流管控场景
enum MCP_MUTE_CONTROL : uint8_t {
    NL_SLE_MCP_EMPTY_STREAM = 1,
    NL_SLE_MCP_MUTE_STREAM_DATA_ZERO = 2,
    NL_SLE_MCP_MUTE_STREAM_VOL_ZERO = 3,
};
#endif // MCP_DEFS_H
