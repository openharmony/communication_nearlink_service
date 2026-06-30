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
#define LOG_TAG "hibox_hitws"

#include "hibox_hitws.h"
#include "hibox_process.h"
#include "hibox_local.h"

static bool HiboxIsoHandoverParser(uint8_t type, uint16_t length, uint8_t *value, void *parseResult)
{
    if (parseResult == NULL) {
        ALOGW("parseResult is NULL");
        return false;
    }
    ALOGD("type: %{public}d, length: %{public}d", type, length);
    uint8_t *typeValue = value;
    HitwsIsoHandover *result = (HitwsIsoHandover *)parseResult;
    switch (type) {
        case HITWS_ISO_HANDOVER_TYPE:
            if (length != HIBOX_ISO_COMMON_LEN) {
                return false;
            }
            result->type = HITWS_ISO_HANDOVER_TYPE;
            STREAM_TO_UINT8(result->role, typeValue);
            break;
        case HITWS_ISO_HANDOVER_CLOSE_TYPE:
            if (length != HIBOX_ISO_COMMON_LEN) {
                return false;
            }
            result->type = HITWS_ISO_HANDOVER_CLOSE_TYPE;
            STREAM_TO_UINT8(result->role, typeValue);
            break;
        default:
            ALOGW("unknown type: %{public}d", type);
            break;
    }
    ALOGD("role: %{public}d", result->role);
    return true;
}

static void HiboxIsoHandover(uint8_t *data, uint16_t dataLen, uint8_t *result)
{
    ALOGW("dataLen: %{public}d", dataLen);
    HiboxParseMsgInd *parseResult = (HiboxParseMsgInd *)result;
    NL_CHECK_RETURN(parseResult, "parseResult is null");
    parseResult->msgType = HITWS_ISO_HANDOVER;
    parseResult->datalen = 0;
    uint16_t parseLen = HiboxTlvParseHandler(data, dataLen, HiboxIsoHandoverParser, parseResult->data);
    if (parseLen > sizeof(HitwsIsoHandover)) {
        ALOGW("parseLen: %{public}d", parseLen);
        return;
    }
    parseResult->datalen = sizeof(HitwsIsoHandover);
}

static uint16_t HiboxIsoHandoverComb(uint8_t *buffer, uint16_t bufferLen, HiboxParseMsgInd *arg)
{
    uint16_t offset = 0;
    NL_CHECK_RETURN_RET(arg->datalen == sizeof(HitwsIsoHandover), offset,
        "datalen: %{public}d is wrong", arg->datalen);
    HitwsIsoHandover *isoData = (HitwsIsoHandover *)(arg->data);
    SERVICE_HIBOX_ADD_TLV(buffer, bufferLen, offset, isoData->type, HIBOX_ISO_COMMON_LEN, &(isoData->role));
    return offset;
}

bool HiboxHitwsManagerReqParse(uint8_t cmdId, uint8_t *tlv, uint16_t len, uint8_t *result)
{
    NL_CHECK_RETURN_RET(tlv && len > HIBOX_TLV_HEADER_LEN && result, false, "command is %{public}d", cmdId);
    ALOGD("command is %{public}d", cmdId);
    switch (cmdId) {
        case CMDID_HITWS_ROLE_HANDOVER_INFORM:
            HiboxIsoHandover(tlv, len, result);
            break;
        default:
            return false;
    }
    return true;
}

uint16_t HiboxHitwsManagerRspComb(uint8_t *buffer, uint16_t bufferLen, HiboxParseMsgInd *arg)
{
    uint16_t offset = 0;
    NL_CHECK_RETURN_RET(buffer && arg, offset, "buffer or arg is NULL");
    switch (arg->msgType) {
        case HITWS_ISO_HANDOVER:
            offset = HiboxIsoHandoverComb(buffer, bufferLen, arg);
            break;
        default:
            break;
    }
    return offset;
}

