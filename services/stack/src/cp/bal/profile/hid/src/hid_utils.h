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
#ifndef HID_UTILS_H
#define HID_UTILS_H

#include <stdint.h>
#include "ssap_type.h"
#include "hid_def.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HID_PARSE_TO_UINT8(u8, p) do {                              \
        (u8) = ((uint8_t)(*(p)));                                   \
        (p) += 1;                                                   \
} while (0)

#define HID_PARSE_TO_UINT16(u16, p) do {                                \
        (u16) = ((uint16_t)(*(p)) + (((uint16_t)(*((p) + 1))) << 8));   \
        (p) += 2;                                                       \
} while (0)

bool HidCompAppId(void *ptr, void *args);

bool HidCompAddr(void *ptr, void *args);

bool HidCompHandle(void *ptr, void *args);

bool HidCompReportIdAndType(void *ptr, void *args);

NLSTK_SsapUuid_S HidConvertUuidToStru(uint16_t uuid);

uint16_t HidConvertUuidTo16Bits(NLSTK_SsapUuid_S uuidStru);

HidPropertyType_E HidGetPropertyTypeByUuid(uint16_t uuid);

void HidFreeWriteParam(void *ptr);

void HidFreeReport(void *ptr);

void HidFreeDevice(void *ptr);

NLSTK_VariableData_S *HidValueConvert(HidPropertyType_E type, void *value);

#ifdef __cplusplus
}
#endif

#endif