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
#ifndef ACTM_L2HC_H
#define ACTM_L2HC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --------------采样率------------- */
#define L2HC_RATE_8      0x0001 /* 8KHz */
#define L2HC_RATE_11_025 0x0002 /* 11.025KHz */
#define L2HC_RATE_16     0x0004 /* 16KHz */
#define L2HC_RATE_22_05  0x0008 /* 22.05KHz */
#define L2HC_RATE_24     0x0010 /* 24KHz */
#define L2HC_RATE_32     0x0020 /* 32KHz */
#define L2HC_RATE_44_1   0x0040 /* 44.1KHz */
#define L2HC_RATE_48     0x0080 /* 48KHz */
#define L2HC_RATE_88_2   0x0100 /* 88.2KHz */
#define L2HC_RATE_96     0x0200 /* 96KHz */
#define L2HC_RATE_176_4  0x0400 /* 176.4KHz */
#define L2HC_RATE_192    0x0800 /* 192KHz */
#define L2HC_RATE_384    0x1000 /* 384KHz */

/* --------------采样深度------------- */
#define L2HC_DEPTH_8  0x01 /* 8bit */
#define L2HC_DEPTH_16 0x02 /* 16bit */
#define L2HC_DEPTH_24 0x04 /* 24bit */
#define L2HC_DEPTH_32 0x08 /* 32bit */

/* --------------音频通道类型------------- */
#define L2HC_CHANNEL_SINGLE 0x01 /* 单声道 */
#define L2HC_CHANNEL_DOUBLE 0x02 /* 双声道立体声 */

/* --------------帧长模式------------- */
#define L2HC_FRAME_5  0x04 /* 5ms帧模式 */
#define L2HC_FRAME_10 0x08 /* 10ms帧模式 */

/* --------------编码速率------------- */
#define L2HC_BPS_48s       0x0000000000000004 /* 单声道48kbps */
#define L2HC_BPS_64s       0x0000000000000008 /* 单声道64kbps */
#define L2HC_BPS_96s       0x0000000000000010 /* 单声道96kbps */
#define L2HC_BPS_128s      0x0000000000000020 /* 单声道128kbps */
#define L2HC_BPS_160s      0x0000000000000040 /* 单声道160kbps */
#define L2HC_BPS_240s      0x0000000000000080 /* 单声道240kbps */
#define L2HC_BPS_320s      0x0000000000000100 /* 单声道320kbps */
#define L2HC_BPS_480s      0x0000000000000200 /* 单声道480kbps */
#define L2HC_BPS_750s      0x0000000000000400 /* 单声道750kbps */
#define L2HC_BPS_960s      0x0000000000000800 /* 单声道960kbps */
#define L2HC_BPS_1150s     0x0000000000001000 /* 单声道1150kbps */
#define L2HC_BPS_2300s     0x0000000000002000 /* 单声道2300kbps */
#define L2HC_BPS_96d       0x0000000000400000 /* 双声道96kbps */
#define L2HC_BPS_128d      0x0000000000800000 /* 双声道128kbps */
#define L2HC_BPS_192d      0x0000000001000000 /* 双声道192kbps */
#define L2HC_BPS_256d      0x0000000002000000 /* 双声道256kbps */
#define L2HC_BPS_320d      0x0000000004000000 /* 双声道320kbps */
#define L2HC_BPS_480d      0x0000000008000000 /* 双声道480kbps */
#define L2HC_BPS_640d      0x0000000010000000 /* 双声道640kbps */
#define L2HC_BPS_960d      0x0000000020000000 /* 双声道960kbps */
#define L2HC_BPS_1500d     0x0000000040000000 /* 双声道1500kbps */
#define L2HC_BPS_1920d     0x0000000080000000 /* 双声道1920kbps */
#define L2HC_BPS_2300d     0x0000000100000000 /* 双声道2300kbps */
#define L2HC_BPS_4600d     0x0000000200000000 /* 双声道4600kbps */

#define L2HC_TYPE_VERSION 0x01
#define L2HC_TYPE_RATE    0x02
#define L2HC_TYPE_DEPTH   0x03
#define L2HC_TYPE_CHANNEL 0x04
#define L2HC_TYPE_FRAME   0x05
#define L2HC_TYPE_BPS     0x06
#define L2HC_BPS_LEN      5

#define CODEC_L2HC_PARAM_LEN 24

typedef struct {
    uint8_t version; // 版本
    uint16_t rate;   // 采样率
    uint8_t depth;   // 采样深度
    uint8_t channel; // 音频通道类型
    uint16_t frame;  // 帧长模式
    uint64_t bps;    // 编码速率
} NLSTK_L2HCParam_S;

#define L2HC_CODEC_NORMAL 0x01
#define L2HC_CODEC_VOICE  0x02
#define L2HC_CODEC_CUSTOMIZE 0xff

uint32_t L2HCGetRate(uint8_t index);
uint8_t L2HCGetDepth(uint8_t index);
uint16_t L2HCGetBps(uint8_t codecId, uint8_t index);
uint8_t L2HCGetFrame(uint8_t index);

#ifdef __cplusplus
}
#endif
#endif