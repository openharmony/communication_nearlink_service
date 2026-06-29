/**
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
 * @file         dli_data_stub.h
 * @brief        dli桩函数
*/

/* 后续直接依赖so，此文件删除，暂时使用函数打桩的方式，编译及测试通过，后续验证方便 */
#ifndef DLI_DATA_STUB_H
#define DLI_DATA_STUB_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef NEARLINK_SERVICE_STACK_LOCAL_TEST
typedef enum {
    SUCCESS,
    TRANSPORT_ERROR,
    INITIALIZATION_ERROR,
    UNKNOWN,
} SleInitStatus;

typedef enum {
    PACKET_TYPE_UNKNOWN = 0,
    PACKET_TYPE_CMD = 1,
    PACKET_TYPE_ACB = 2,
    PACKET_TYPE_SCO = 3,
    PACKET_TYPE_EVENT = 4,
    PACKET_TYPE_ISO = 5,
    PACKET_TYPE_SLE_CMD = 0xA1,
    PACKET_TYPE_SLE_EVENT = 0xA2,
    PACKET_TYPE_SLE_ACB = 0xA3,
    PACKET_TYPE_SLE_ICB = 0xA4
} SlePacketType;

// Dli packet received from backend
typedef struct SlePacket {
    uint8_t *data;
    uint32_t size;
} SlePacket;

// SleDliCallbacks register to hal by upperlayer protocol
typedef struct SleDliCallbackFunc {
    void (*initializationComplete)(SleInitStatus status);
    void (*dliPacketReceived)(SlePacketType type, const SlePacket *packet);
} SleDliCallbackFunc;

typedef int (*SleHalInitFunc)(SleDliCallbackFunc *callbacks);
typedef int (*SleSendDliPacketFunc)(const SlePacket *packet);

int SleHalInit(SleDliCallbackFunc *func);
// send to drive
int SleSendDliPacket(const SlePacket *packet);
void SleHalClose(void);
void SleReset();

// send to dli
void SleSendToDliStub(SlePacketType type, const SlePacket *packet);
int GetDliVersion(void);
#endif
#ifdef __cplusplus
}
#endif

#endif