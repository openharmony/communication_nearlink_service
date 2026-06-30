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
#ifndef HIBOX_HITWS
#define HIBOX_HITWS

#include "hibox_def.h"

#define HIBOX_ISO_COMMON_LEN 1
enum HiboxHitwsCommandId {
    CMDID_HITWS_ISO_PARAM = 0x01,
    CMDID_HITWS_ROLE_HANDOVER_INFORM = 0x02,
    CMDID_HITWS_VOLUME_ADJUST = 0x03,
    CMDID_HITWS_EARBUD_ACTION = 0x04,
    CMDID_HITWS_RECOVERY_ACTION = 0x05,
};

enum HiboxIsoConfigType {
    HITWS_ISO_CONFIG_TYPE = 0x01,
    HITWS_POLAR_CONFIG_TYPE = 0x02,
};

enum HiboxIsoHandoverType {
    HITWS_ISO_HANDOVER_TYPE = 0x01,
    HITWS_ISO_HANDOVER_CLOSE_TYPE = 0x02,
};

enum HiboxIsoRecoveryType {
    HITWS_ISO_RECOVERY_TYPE = 0x01,
};

bool HiboxHitwsManagerReqParse(uint8_t cmdId, uint8_t *tlv, uint16_t len, uint8_t *result);
uint16_t HiboxHitwsManagerRspComb(uint8_t *buffer, uint16_t bufferLen, HiboxParseMsgInd *arg);

#endif
