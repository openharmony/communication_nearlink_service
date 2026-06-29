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
#ifndef HIBOX_LOCAL
#define HIBOX_LOCAL

#include "hibox_def.h"

#define HIBOX_INVALID_TLV_LEN 0
#define HIBOX_BYTE_BIT_LEN 8

#define SERVICE_HIBOX_ADD_TLV(buf, bufLen, offset, type, length, value) \
    ((offset) += HiboxCombineTlv(((buf) + (offset)), ((bufLen) - (offset)), (type), (length), (value)))

typedef bool (*HiboxTlvParserFunc)(uint8_t type, uint16_t length, uint8_t* value, void* parseResult);

void HiboxCommResultParse(uint8_t msgType, uint8_t *data, uint16_t dataLen, uint8_t *result);
uint16_t HiboxCommonResultComb(uint8_t *buffer, uint16_t bufferLen, HiboxParseMsgInd *arg);
void HiboxCombileMsg(uint8_t *buf, uint16_t bufLen, uint8_t sid, uint8_t cmdId,
    uint8_t *tlvData, uint16_t offset, uint8_t dataType);
uint16_t HiboxCombineTlv(uint8_t *buf, uint16_t bufLen, uint8_t type, uint16_t length, const void *value);
uint16_t HiboxTlvParseHandler(uint8_t* data, uint16_t dataLen, HiboxTlvParserFunc parseFunc,
    void* parseResult);
bool HiboxCompareArrayToValue(uint8_t *array, uint8_t value, uint16_t length);
uint16_t HiboxL2cuDoCheckMic(uint8_t *buf, uint16_t len);

#endif /* hibox_LOCAL */
