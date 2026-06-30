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
#ifndef HIBOX_SERVICE
#define HIBOX_SERVICE

#include "hibox_def.h"

#define HIBOX_SERVICE_CAP_LEN 10
enum HiboxServiceCommandId {
    CMDID_SUPPORT_SERVICE = 0x01,
    CMDID_DEVICE_ABILITY = 0x02,
};

enum HiboxServiceType {
    SUPPORT_SERVICE_TYPE = 0x01,
};

enum HiboxManufacturerAbilityType {
    PROTOCOL_TYPE = 0x01,
    OS_TYPE = 0x02,
    MANUFACTURER_ABILITY_BIT_MAP = 0x03,
};

enum HiboxServiceSupport {
    NOT_SUPPORT_SERVICE = 0x00,
    SUPPORT_SERVICE = 0x01,
};

bool HiboxServiceManagerRspParse(uint8_t cmdId, uint8_t *tlv, uint16_t len, uint8_t *result);
uint16_t HiboxDeviceAbilityReqComb(uint8_t *buffer, uint16_t bufferLen, HiboxParseMsgInd *arg);

#endif
