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
#ifndef HIBOX_DTS
#define HIBOX_DTS

#include "hibox_def.h"

#define LEFT_WEAR_TYPE_LEN 1
#define RIGHT_WEAR_TYPE_LEN 1
#define FREEMAN_WEAR_TYPE_LEN 1
#define BATTERY_IGNORE_SIZE 2

enum HiboxDtsCommandId {
    CMDID_CAPABILITY = 0x01,
    CMDID_WEAR_STATUS,
    CMDID_AT_REPORT,
    CMDID_AUDIO_EXCEPTION,
};

enum HiboxDtsCapType {
    DTS_CAP_TYPE = 0x01,
};

enum HiboxDtsWearStatusType {
    LEFT_WEAR_TYPE = 0x01,
    RIGHT_WEAR_TYPE,
    FREEMAN_WEAR_TYPE,
};

enum HiboxDtsAtReportType {
    BATTERY_REPORT_TYPE = 0x01,
    DEVICE_INFO_REPORT_TYPE,
};

enum HiboxDtsAudioExceptionType {
    AUDIO_EXCEPTION_TYPE = 0x01,
    AUDIO_EXCEPTION_RSSI,
    AUDIO_EXCEPTION_RETRANS,
    AUDIO_EXCEPTION_SCENE,
    AUDIO_EXCEPTION_AFH,
};

bool HiboxDtsMgmtReqParse(uint8_t cmdId, uint8_t *tlv, uint16_t len, uint8_t *result);
bool HiboxDtsMgmtRspParse(uint8_t cmdId, uint8_t *tlv, uint16_t len, uint8_t *result);
uint16_t HiboxDtsMgmtReqComb(uint8_t *buffer, uint16_t bufferLen, HiboxParseMsgInd *arg);
uint16_t HiboxDtsMgmtRspComb(uint8_t *buffer, uint16_t bufferLen, HiboxParseMsgInd *arg);

#endif
