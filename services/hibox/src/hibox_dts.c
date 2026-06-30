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
#define LOG_TAG "hibox_dts"

#include "hibox_dts.h"
#include "hibox_process.h"
#include "hibox_local.h"

#define MIN_AT_STR_DATA_LEN 2 // AtStrData

static bool HiboxWearStatusParser(uint8_t type, uint16_t length, uint8_t* value, void* parseResult)
{
    if (parseResult == NULL) {
        ALOGW("parseResult is NULL");
        return false;
    }
    uint8_t *typeValue = value;
    ALOGD("type: %{public}d, length: %{public}d", type, length);
    /* use this struct to pass parse result to corresponding hibox report function */
    DtsWearData *result = (DtsWearData *)parseResult;
    switch (type) {
        case LEFT_WEAR_TYPE:
            if (length != LEFT_WEAR_TYPE_LEN) {
                return false;
            }
            STREAM_TO_UINT8(result->leftWear, typeValue);
            break;
        case RIGHT_WEAR_TYPE:
            if (length != RIGHT_WEAR_TYPE_LEN) {
                return false;
            }
            STREAM_TO_UINT8(result->rightWear, typeValue);
            break;
        case FREEMAN_WEAR_TYPE:
            if (length != FREEMAN_WEAR_TYPE_LEN) {
                return false;
            }
            STREAM_TO_UINT8(result->freemanWear, typeValue);
            break;
        default:
            ALOGW("unknown type: %{public}d", type);
            break;
    }
    ALOGD("leftWear=%{public}d, rightWear=%{public}d", result->leftWear, result->rightWear);
    return true;
}

/*
 * Parse hibox tlv for AT string.
 */
static bool HiboxAtStringParser(uint8_t type, uint16_t length, uint8_t* value, void* parseResult)
{
    if (parseResult == NULL) {
        ALOGW("parseResult is nullptr");
        return false;
    }
    uint8_t *typeValue = value;
    ALOGD("type: %{public}d, length: %{public}d", type, length);
    /* use this struct to pass parse result to corresponding hibox report function */
    AtStrData* atStr = (AtStrData *)parseResult;
    switch (type) {
        case BATTERY_REPORT_TYPE:
            atStr->dtsType = BATTERY_REPORT;
            if (length > DTS_DATA_MAX_LEN || length < DTS_DATA_MIN_LEN) {
                return false;
            }
            memset_s(atStr->atString, DTS_DATA_MAX_LEN + 1, 0, DTS_DATA_MAX_LEN + 1);
            STREAM_TO_ARRAY(atStr->atString, typeValue, length);
            break;
        case DEVICE_INFO_REPORT_TYPE:
            atStr->dtsType = AT_REPORT;
            if (length > DTS_DATA_MAX_LEN || length < DTS_DATA_MIN_LEN) {
                return false;
            }
            memset_s(atStr->atString, DTS_DATA_MAX_LEN + 1, 0, DTS_DATA_MAX_LEN + 1);
            STREAM_TO_ARRAY(atStr->atString, typeValue, length);
            break;
        default:
            ALOGD("unknown type: %{public}d", type);
            break;
    }
    ALOGD("dtsType:%{public}d", atStr->dtsType);
    return true;
}

static void HiboxAtStringReport(uint8_t *data, uint16_t dataLen, uint8_t *result)
{
    HiboxParseMsgInd *parseResult = (HiboxParseMsgInd *)result;
    NL_CHECK_RETURN(parseResult, "malloc fail");
    parseResult->msgType = DTS_REPORT;
    parseResult->datalen = 0;
    uint16_t parseLen = HiboxTlvParseHandler(data, dataLen, HiboxAtStringParser, parseResult->data);
    if (parseLen > DTS_DATA_MAX_LEN) {
        ALOGW("parseLen: %{public}d invalid", parseLen);
        return;
    }
    parseResult->datalen = parseLen + 1;
}

/*
 * Parse hibox tlv for exception.
 */
static bool HiboxDtsExceptionParser(uint8_t type, uint16_t length, uint8_t* value, void* parseResult)
{
    if (parseResult == NULL) {
        ALOGW("parseResult is nullptr");
        return false;
    }
    uint8_t *typeValue = value;
    ALOGD("type: %{public}d, length: %{public}d", type, length);
    /* use this struct to pass parse result to corresponding hibox report function */
    AudioExceptionData* result = (AudioExceptionData *)parseResult;
    switch (type) {
        case AUDIO_EXCEPTION_TYPE:
            if (length != sizeof(result->audioExceptionType)) {
                return false;
            }
            STREAM_TO_UINT32(result->audioExceptionType, typeValue);
            break;
        case AUDIO_EXCEPTION_RSSI:
            if (length != sizeof(result->audioQualityRssi)) {
                return false;
            }
            STREAM_TO_UINT16(result->audioQualityRssi, typeValue);
            break;
        case AUDIO_EXCEPTION_RETRANS:
            if (length != sizeof(result->audioQualityRetrans)) {
                return false;
            }
            STREAM_TO_UINT32(result->audioQualityRetrans, typeValue);
            break;
        case AUDIO_EXCEPTION_SCENE:
            if (length != sizeof(result->audioQualityScene)) {
                return false;
            }
            STREAM_TO_UINT32(result->audioQualityScene, typeValue);
            break;
        case AUDIO_EXCEPTION_AFH:
            if (length != sizeof(result->audioQualityAfh)) {
                return false;
            }
            STREAM_TO_UINT32(result->audioQualityAfh, typeValue);
            break;
        default:
            ALOGD("unknown type: %{public}d", type);
            break;
    }
    return true;
}

static void HiboxDtsExceptionReport(uint8_t *data, uint16_t dataLen, uint8_t *result)
{
    HiboxParseMsgInd *parseResult = (HiboxParseMsgInd *)result;
    NL_CHECK_RETURN(parseResult, "malloc fail");
    parseResult->msgType = AUDIO_EXCEPTION;
    parseResult->datalen = 0;
    uint16_t parseLen = HiboxTlvParseHandler(data, dataLen, HiboxDtsExceptionParser, parseResult->data);
    if (parseLen > sizeof(AudioExceptionData)) {
        ALOGW("parseLen: %{public}d", parseLen);
        return;
    }
	parseResult->datalen = sizeof(AudioExceptionData);
	if (parseLen == 0) {
		parseResult->datalen = 0;
	}
}

static void HiboxWearStatusReport(uint8_t* data, uint16_t dataLen, uint8_t *result)
{
    HiboxParseMsgInd *parseResult = (HiboxParseMsgInd *)result;
    NL_CHECK_RETURN(parseResult, "parseResult is null");
    parseResult->msgType = WEAR_STATUS;
    parseResult->datalen = 0;
    uint16_t parseLen = HiboxTlvParseHandler(data, dataLen, HiboxWearStatusParser, parseResult->data);
    if (parseLen > sizeof(DtsWearData)) { /* 该条消息处理时，有可能不携带freemanWear的数据 */
        ALOGW("parseLen: %{public}d invalid", parseLen);
        return;
    }
    parseResult->datalen = sizeof(DtsWearData);
}

static uint16_t HiboxDtsCapComb(uint8_t *buffer, uint16_t bufferLen, HiboxParseMsgInd *arg)
{
    uint16_t offset = 0;
    NL_CHECK_RETURN_RET(arg->datalen <= sizeof (DtsCapData), offset, "datalen: %{public}d is wrong", arg->datalen);
    DtsCapData *dtsData = (DtsCapData *)(arg->data);
    SERVICE_HIBOX_ADD_TLV(buffer, bufferLen, offset, DTS_CAP_TYPE, arg->datalen, dtsData->dtsCap);
    return offset;
}

static bool HiboxDtsCapParser(uint8_t type, uint16_t length, uint8_t* value, void* parseResult)
{
    if (parseResult == NULL) {
        ALOGW("parseResult is NULL");
        return false;
    }
    ALOGD("type: %{public}d, length: %{public}d", type, length);
    /* use this struct to pass parse result to corresponding hibox report function */
    DtsCapData *result = (DtsCapData *)parseResult;
    switch (type) {
        case DTS_CAP_TYPE:
            NL_CHECK_RETURN_RET((length <= DTS_MAX_CAP_LEN) && memcpy_s(result->dtsCap, length, value, length) == EOK,
                false, "memcpy_s fail");
            break;
        default:
            ALOGW("unknown type: %{public}d", type);
            break;
    }
    ALOGD("dtsCap: %{public}d, parseLen: %{public}d", result->dtsCap[0], length);
    return true;
}

static void HiboxDtsCapReport(uint8_t* data, uint16_t dataLen, uint8_t *result)
{
    HiboxParseMsgInd *parseResult = (HiboxParseMsgInd *)result;
    NL_CHECK_RETURN(parseResult, "parseResult is null");
    parseResult->msgType = DTS_CAP;
    errno_t ret = memset_s(parseResult->data, sizeof(DtsCapData), 0, sizeof(DtsCapData));
    NL_CHECK_RETURN(ret == EOK, "memset_s fail");
    parseResult->datalen = 0;
    uint16_t parseLen = HiboxTlvParseHandler(data, dataLen, HiboxDtsCapParser, parseResult->data);
    if (parseLen > sizeof(DtsCapData)) { /* 当前消息最大10字节，数据长度可变，0-10字节 */
        ALOGW("parseLen: %{public}d", parseLen);
        return;
    }
    parseResult->datalen = parseLen;
}

bool HiboxDtsMgmtReqParse(uint8_t cmdId, uint8_t *tlv, uint16_t len, uint8_t *result)
{
    NL_CHECK_RETURN_RET(tlv && len > HIBOX_TLV_HEADER_LEN && result, false, "command is %{public}d", cmdId);
    ALOGD("command is %{public}d", cmdId);
    switch (cmdId) {
        case CMDID_CAPABILITY:
            HiboxDtsCapReport(tlv, len, result);
            break;
        case CMDID_WEAR_STATUS:
            HiboxWearStatusReport(tlv, len, result);
            break;
        case CMDID_AT_REPORT:
            HiboxAtStringReport(tlv, len, result);
            break;
        case CMDID_AUDIO_EXCEPTION:
            HiboxDtsExceptionReport(tlv, len, result);
            break;
        default:
            ALOGE("command is %{public}d", cmdId);
            return false;
    }
    return true;
}

bool HiboxDtsMgmtRspParse(uint8_t cmdId, uint8_t *tlv, uint16_t len, uint8_t *result)
{
    ALOGD("command is %{public}d, len is %{public}d", cmdId, len);
    NL_CHECK_RETURN_RET(tlv && len > HIBOX_TLV_HEADER_LEN && result, false, "command is %{public}d", cmdId);
    ALOGD("command is %{public}d", cmdId);
    switch (cmdId) {
        case CMDID_CAPABILITY:
            HiboxDtsCapReport(tlv, len, result);
            break;
        default:
            ALOGE("command is %{public}d", cmdId);
            return false;
    }
    return true;
}

uint16_t HiboxDtsMgmtReqComb(uint8_t *buffer, uint16_t bufferLen, HiboxParseMsgInd *arg)
{
    uint16_t offset = 0;
    NL_CHECK_RETURN_RET(buffer && arg, offset, "buffer or arg is NULL");
    switch (arg->msgType) {
        case DTS_CAP:
            offset = HiboxDtsCapComb(buffer, bufferLen, arg);
            break;
        default:
            break;
    }
    return offset;
}

uint16_t HiboxDtsMgmtRspComb(uint8_t *buffer, uint16_t bufferLen, HiboxParseMsgInd *arg)
{
    uint16_t offset = 0;
    NL_CHECK_RETURN_RET(buffer && arg, offset, "buffer or arg is NULL");
    switch (arg->msgType) {
        case DTS_CAP:
            offset = HiboxDtsCapComb(buffer, bufferLen, arg);
            break;
        default:
            break;
    }
    return offset;
}
