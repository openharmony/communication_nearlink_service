/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#ifndef HIBOX_DEF
#define HIBOX_DEF

#include "hibox_common.h"

#pragma pack(1)

/**
 * @brief echo3,2 req and rsp data struct
 */
typedef struct {
    uint8_t type;
    uint8_t role;
} HitwsIsoHandover;

/**
 * @brief echo4,2 req data struct, rsp data define in HiboxRspCommonResult
 * if only send leftWear the other value should set to 0xff
 */
typedef struct {
    uint8_t leftWear; /**< left earbuds wear state,0x00:not wear;0x01:in ear;0xFF:invalid */
    uint8_t rightWear; /**< right earbuds wear state,0x00:not wear;0x01:in ear;0xFF:invalid */
    uint8_t freemanWear;/**< freeman wear state,0x00:not wear;0x01:in ear;0xFF:invalid */
} DtsWearData;

/**
 * @brief echo4,3 req data struct, rsp data define in HiboxRspCommonResult
 */
typedef struct {
    uint8_t dtsType; /**< define in enum DtsType */
    uint8_t atString[DTS_DATA_MAX_LEN + 1]; /** < at String */
} AtStrData;

enum DtsType {
    AT_REPORT = 0x00,
    BATTERY_REPORT = 0x01,
};

/* echo 5,12 通知耳机音频服务连接完成 */
typedef struct {
    uint8_t state;
} HiboxSleProfileState;

/* echo 8,1 8,2 8,3 8,4 8,5 8,B 8,C rsp data */
typedef struct {
    uint8_t setResult; /* 0x00:success; 0x01:fail */
} HiboxHdapCommonRsp;

/* echo 8,C req data */
/* 通知耳机手机手动选择耳机为哪些音频流出声设备 bitmap @ref HiBoxAudioType
 * 当前12字节含义
 *    未定义 | 音乐 | 蜂窝通话 | 语音助手 | 铃声 | VOIP通话 | 游戏 | 录音 | 提醒 | 视频 | 导航 | 告警
 * 每字节取值
 *    0：无效值  1：释放  2：强选
 */
typedef struct {
    uint8_t audioType[NEARLINK_STREAM_TYPE_NUM];
} HiboxNearlinkDataPath;

#pragma pack()

#endif