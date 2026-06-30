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
#ifndef SLE_DLI_LAYER_ADAPTER_H
#define SLE_DLI_LAYER_ADAPTER_H

#pragma once

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Dli hal backend status
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

typedef enum {
    DLI_VERSION_1_0 = 0,
    DLI_VERSION_1_1 = 1,
} DliVersion;

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

int SleHalInit(SleDliCallbackFunc *callbacks);
void SleReset();
int SleSendDliPacket(const SlePacket *packet);
void SleHalClose(void);
int GetDliVersion(void); // DliVersion

typedef int (*SleHalInitFunc)(SleDliCallbackFunc *callbacks);
typedef int (*SleSendDliPacketFunc)(const SlePacket *packet);
typedef void (*SleHalCloseFunc)(void);

#ifdef __cplusplus
}
#endif

#endif