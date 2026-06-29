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
#ifndef HADM_DEFINES_H
#define HADM_DEFINES_H

#include <cstdint>
#include <string>

namespace OHOS {
namespace Nearlink {

// 地址最大值
const int MAX_RANGING_DEVICE = 1;

// 车钥匙属性值
const int DEVICE_CLASS_VEHICLE_LOCK = 4100;

// 接口调用返回值
const int HADM_SUCCESS = 0;
const int HADM_FAILURE = -1;

// 测距启停状态
const int HADM_SOUNDING_STATE_STOPED = 0;
const int HADM_SOUNDING_STATE_STARTED = 1;
 
// 测距启停行为
const int HADM_SOUNDING_USER_START = 1;
const int HADM_SOUNDING_USER_STOP = 2;

// 链路连接参数更新
const uint16_t HADM_CONN_COEXIST_INTERVAL = 0x320; // 链路调度最小间隔 800*0.125ms=100ms
const uint8_t HADM_CONN_EVENT_IFS = 125; // 链路调度间隔.125us
const uint16_t HADM_CONN_SUPERVISION_TIMEOUT = 500; // 超时时间5000ms
const uint8_t HADM_CONN_TIME_UNIT = 4; // 系统调度时隙, 125us

// 测量配置
const uint16_t HADM_CONFIG_FREQ = 0x0015; // 初始化阶段的频点
const uint16_t HADM_CONFIG_INTERACTION_TYPE = 0x0002; // 初始化阶段交互类型指示
const uint16_t HADM_CONFIG_GROUP_INTERVAL = 0x0078; // 初始化阶段事件间间隔
const uint16_t HADM_CONFIG_MEASURE_INTERVAL_TYPE1 = 0x0064; // 测量帧间隔
const uint16_t HADM_CONFIG_MEASURE_INTERVAL_TYPE2 = 0x003C; // 测量帧间隔
const uint16_t HADM_CONFIG_INTRA_EVENT_INTERVAL = 0x0078; // 事件内间隔
const uint8_t HADM_CONFIG_T_GUARD = 0x0A; // 初始化阶段的测量帧的内部切换间隔
const uint8_t HADM_CONFIG_SIGNAL2_LEN = 0x64; // 先发链路测量信号2的长度
const uint8_t HADM_CONFIG_INIT_SIGNAL2_TONE = 0x02; // 先发链路测量信号2 多音指示
const uint8_t HADM_CONFIG_REFL_SIGNAL2_TONE = 0x02; // 后发链路测量信号2 多音指示
const uint8_t HADM_CONFIG_FREQ_HOP_BAND = 0x01; // 跳频频带
const uint8_t HADM_CONFIG_ANCHOR_NUM = 0x01; // 需要连接的锚点数量
const size_t HADM_MEASURE_PM_24G_BAND_LEN = 10; // 跳频信道指示的长度
const uint32_t HDAM_SOUNDING_TS_MAX_DIFF = 10; // HADM双端IQ测量值时间戳最大差值
const uint8_t HADM_SOUNDING_FRESH_RATE = 4; // 刷新率档位

// RSSI溢出值
const uint16_t HADM_RSSI_CHANGE_FLAG = 256;

// 能接受的最小连接参数 slot 0x320*0.125ms = 100ms
const uint32_t MIN_CONNECTION_INTERVAL_LIMIT = 0X320;

// 测距白名单
const char* const CALLER_NAME_FINDNETWORK = "findnetwork";
const int CALLER_UID_FINDNETWORK = 7518;

// IQ存储路径长度
const int FILE_STRING_LENGTH = 100;

} // namespace Nearlink
} // namespace OHOS
#endif // HADM_DEFINES_H