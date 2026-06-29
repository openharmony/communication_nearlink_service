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
#include "actm_l2hc.h"

static uint32_t g_rateTable[][2] = {
    {0x0, 8000}, {0x1, 11025}, {0x2, 16000}, {0x3, 22050}, {0x4, 24000},
    {0x5, 32000}, {0x6, 44100}, {0x7, 48000}, {0x8, 88200}, {0x9, 96000},
    {0xa, 176400}, {0xb, 192000}, {0xc, 384000}
};

static uint8_t g_depthTable[][2] = {
    {0x0, 8}, {0x1, 16}, {0x2, 24}, {0x3, 32}, {0x4, 32}
};

static uint16_t g_l2hcNormalBpsTable[][2] = {
    {0x2, 48}, {0x3, 64}, {0x4, 96}, {0x5, 128}, {0x6, 160},
    {0x7, 240}, {0x8, 320}, {0x9, 480}, {0xa, 750}, {0xb, 960},
    {0xc, 1150}, {0xd, 2300}, {0x16, 96}, {0x17, 128}, {0x18, 192},
    {0x19, 256}, {0x1a, 320}, {0x1b, 480}, {0x1c, 640}, {0x1d, 960},
    {0x1e, 1500}, {0x1f, 1920}, {0x20, 2300}, {0x21, 4600}
};

static uint16_t g_l2hcVoiceBpsTable[][2] = {
    {0x1, 8}, {0x2, 16}, {0x3, 32}, {0x4, 48}, {0x5, 64},
    {0x6, 96}, {0x7, 128}, {0x15, 16}, {0x16, 32}, {0x17, 64},
    {0x18, 96}, {0x19, 128}, {0x1a, 192}, {0x1b, 256}
};

static uint8_t g_frameTable[][2] = {
    {0x3, 5}, {0x4, 10}
};

uint32_t L2HCGetRate(uint8_t index)
{
    for (uint8_t i = 0; i < sizeof(g_rateTable) / (sizeof(uint32_t) + sizeof(uint32_t)); i++) {
        if (g_rateTable[i][0] == index) {
            return g_rateTable[i][1];
        }
    }
    return 0;
}

uint8_t L2HCGetDepth(uint8_t index)
{
    for (uint8_t i = 0; i < sizeof(g_depthTable) / (sizeof(uint8_t) + sizeof(uint8_t)); i++) {
        if (g_depthTable[i][0] == index) {
            return g_depthTable[i][1];
        }
    }
    return 0;
}

static uint16_t L2HCNormalGetBps(uint8_t index)
{
    for (uint8_t i = 0; i < sizeof(g_l2hcNormalBpsTable) / (sizeof(uint16_t) + sizeof(uint16_t)); i++) {
        if (g_l2hcNormalBpsTable[i][0] == index) {
            return g_l2hcNormalBpsTable[i][1];
        }
    }
    return 0;
}

static uint16_t L2HCVoiceGetBps(uint8_t index)
{
    for (uint8_t i = 0; i < sizeof(g_l2hcVoiceBpsTable) / (sizeof(uint16_t) + sizeof(uint16_t)); i++) {
        if (g_l2hcVoiceBpsTable[i][0] == index) {
            return g_l2hcVoiceBpsTable[i][1];
        }
    }
    return 0;
}

uint16_t L2HCGetBps(uint8_t codecId, uint8_t index)
{
    switch (codecId) {
        case L2HC_CODEC_NORMAL:
        case L2HC_CODEC_CUSTOMIZE:
            return L2HCNormalGetBps(index);
        case L2HC_CODEC_VOICE:
            return L2HCVoiceGetBps(index);
        default:
            return 0;
    }
}

uint8_t L2HCGetFrame(uint8_t index)
{
    for (uint8_t i = 0; i < sizeof(g_frameTable) / (sizeof(uint8_t) + sizeof(uint8_t)); i++) {
        if (g_frameTable[i][0] == index) {
            return g_frameTable[i][1];
        }
    }
    return 0;
}