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
#define LOG_TAG "hibox_local"

#include "hibox_local.h"
#include "hibox_process.h"

/* CRC16: x^16 + x^15 + x^2 + 1 */
uint16_t HiboxL2cuDoCheckMic(uint8_t *buf, uint16_t len)
{
    uint16_t crc = 0x0000;

    for (uint16_t i = 0; i < len; i++) {
        crc = crc ^ (*buf++ << HIBOX_BYTE_BIT_LEN); /* Shift left 8 bit */

        for (uint16_t j = 0; j < HIBOX_BYTE_BIT_LEN; j++) {
            if (crc & 0x8000) { /* Retrieve the most significant bit */
                crc = (crc << 1) ^ CRC16_POLY;
            } else {
                crc <<= 1;
            }
        }

        crc &= 0xFFFF;
    }

    return (crc);
}

static bool HiboxCommonRspParse(uint8_t type, uint16_t length, uint8_t* value, void* parseResult)
{
    if (parseResult == NULL) {
        ALOGW("parseResult is NULL");
        return false;
    }
    uint8_t *typeValue = value;
    /* use this struct to pass parse result to corresponding hibox report function */
    HiboxRspCommonResult *result = (HiboxRspCommonResult *)parseResult;
    ALOGD("type: %{public}d, length: %{public}d", type, length);
    switch (type) {
        case HIBOX_RSP_TYPE_RESULT:
            if (length != HIBOX_COMMON_MSG_VALUE_LEN) {
                return false;
            }
            STREAM_TO_UINT8(result->rspResult, typeValue);
            break;
        default:
            break;
    }
    ALOGI("rspResult: %{public}d", result->rspResult);
    return true;
}

void HiboxCommResultParse(uint8_t msgType, uint8_t *data, uint16_t dataLen, uint8_t *result)
{
    HiboxParseMsgInd *parseResult = (HiboxParseMsgInd *)result;
    NL_CHECK_RETURN(parseResult, "parseResult is null");
    parseResult->msgType = msgType;
    parseResult->datalen = 0;
    uint16_t parseLen = HiboxTlvParseHandler(data, dataLen, HiboxCommonRspParse, parseResult->data);
    if (parseLen > sizeof(HiboxRspCommonResult)) {
        ALOGW("parseLen: %{public}d", parseLen);
        return;
    }
    parseResult->datalen = sizeof(HiboxRspCommonResult);
}

uint16_t HiboxCommonResultComb(uint8_t *buffer, uint16_t bufferLen, HiboxParseMsgInd *arg)
{
    uint16_t offset = 0;
    NL_CHECK_RETURN_RET(buffer && arg, offset, "buffer or arg is NULL");
    NL_CHECK_RETURN_RET(arg->datalen == HIBOX_COMMON_MSG_VALUE_LEN, offset,
        "datalen:%{public}d is wrong", arg->datalen);
    uint8_t errCode = arg->data[0];
    SERVICE_HIBOX_ADD_TLV(buffer, bufferLen, offset, HIBOX_RSP_TYPE_RESULT, HIBOX_COMMON_MSG_VALUE_LEN, &errCode);
    return offset;
}

bool HiboxCompareArrayToValue(uint8_t *array, uint8_t value, uint16_t length)
{
    for (int i = 0; i < length; i++) {
        if (array[i] == value) {
            continue;
        }
        return false;
    }
    return true;
}

uint16_t HiboxCombineTlv(uint8_t *buf, uint16_t bufLen, uint8_t type, uint16_t length, const void *value)
{
    NL_CHECK_RETURN_RET(buf && value && length > 0, HIBOX_INVALID_TLV_LEN,
        "type=%{public}d, length=%{public}d, bufLen=%{public}d, buf or value is NULL", type, length, bufLen);
    NL_CHECK_RETURN_RET(bufLen >= HIBOX_TLV_HEADER_LEN, HIBOX_INVALID_TLV_LEN,
        "type=%{public}d, length=%{public}d, bufLen=%{public}d, bufLen < HIBOX_TLV_HEADER_LEN", type, length, bufLen);
    NL_CHECK_RETURN_RET(bufLen - HIBOX_TLV_HEADER_LEN >= length, HIBOX_INVALID_TLV_LEN,
        "type=%{public}d, length=%{public}d, bufLen=%{public}d, length is big", type, length, bufLen);
    NL_CHECK_RETURN_RET(!HiboxCompareArrayToValue((uint8_t *)value, HIBOX_INVALID_DATA, length), HIBOX_INVALID_TLV_LEN,
        "type=%{public}d, length=%{public}d, bufLen=%{public}d, value = %{public}d",
        type, length, bufLen, *((uint8_t *)value));
    uint8_t *p = buf;
    UINT8_TO_STREAM(p, type);
    UINT16_TO_STREAM(p, length);
    if (length > 0) {
        HIBOX_STRING_TO_STREAM(p, value, length);
    }
    return p - buf;
}

void HiboxCombileMsg(uint8_t *buf, uint16_t bufLen, uint8_t sid, uint8_t cmdId,
    uint8_t *tlvData, uint16_t offset, uint8_t dataType)
{
    NL_CHECK_RETURN(buf && bufLen >= (offset + HIBOX_MIN_PAYLOAD_LEN), "the input param is bad");

    uint8_t *checkData = buf;
    UINT8_TO_STREAM(buf, dataType);
    UINT8_TO_STREAM(buf, sid);
    UINT8_TO_STREAM(buf, cmdId);
    errno_t ret = memcpy_s(buf, bufLen - HIBOX_HEAD_FIELD_LEN, tlvData, offset);
    NL_CHECK_RETURN(ret == EOK, "memcpy_s fail");
    buf += offset;
    uint16_t checksum = HiboxL2cuDoCheckMic(checkData, offset + HIBOX_HEAD_FIELD_LEN);
    UINT16_TO_STREAM(buf, checksum);
}

/* Basic function for Parse TLV in Hibox */
uint16_t HiboxTlvParseHandler(uint8_t* data, uint16_t dataLen, HiboxTlvParserFunc parseFunc,
    void* parseResult)
{
    uint16_t parseLen = 0;
    NL_CHECK_RETURN_RET(data, parseLen, "invalid data");
    int32_t leftLen = dataLen;
    uint8_t type;
    uint16_t length;

    ALOGD("dataLen = %{public}d", dataLen);
    /* parse TLV one by one */
    while (leftLen >= HIBOX_TLV_HEADER_LEN) {
        STREAM_TO_UINT8(type, data); // Parse Type, data point to length
        STREAM_TO_UINT16(length, data); // Parse Length, data point to value
        leftLen = leftLen - HIBOX_TLV_HEADER_LEN - length; // remainder data length and header length
        ALOGW("type:%{public}d, length:%{public}d, leftLen:%{public}d", type, length, leftLen);
        if (length == 0 || leftLen < 0) { // length of value is invalid
            ALOGW("value length not valid");
            parseLen = 0;
            break;
        }
        bool result = parseFunc(type, length, data, parseResult);
        if (!result) {
            ALOGE("parseFunc type:%{public}d or length:%{public}d is not right", type, length);
            parseLen = 0;
            break;
        }
        data += length; // point to next Type
        parseLen += length;
    }
    ALOGD("parse data parseLen=%{public}d", parseLen);
    return parseLen;
}
