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
#define LOG_TAG "hibox_service"

#include "hibox_service.h"
#include "hibox_process.h"
#include "hibox_local.h"

static bool HiboxDeviceAbilityParser(uint8_t type, uint16_t length, uint8_t *value, void *parseResult)
{
    if (parseResult == NULL) {
        ALOGW("parseResult is NULL");
        return false;
    }
    ALOGD("type: %{public}d, length: %{public}d", type, length);
    /* use this struct to pass parse result to corresponding hibox report function */
    ManufacturerAbilityInfo *result = (ManufacturerAbilityInfo *)parseResult;
    switch (type) {
        case PROTOCOL_TYPE:
            NL_CHECK_RETURN_RET((length <= DEVICE_PROTOCOL_LEN) &&
                memcpy_s(result->protocolType, sizeof(result->protocolType), value, length) == EOK, false,
                "type: %{public}d, length: %{public}d memcpy fail", type, length);
            break;
        case OS_TYPE:
            NL_CHECK_RETURN_RET((length <= DEVICE_OS_LEN) &&
                memcpy_s(result->deviceOsType, sizeof(result->deviceOsType), value, length) == EOK, false,
                "type: %{public}d, length: %{public}d memcpy fail", type, length);
            break;
        case MANUFACTURER_ABILITY_BIT_MAP:
            NL_CHECK_RETURN_RET((length <= DEVICE_MANUFACTURER_ABILITY_LEN) &&
                memcpy_s(result->manufacturerAbility, sizeof(result->manufacturerAbility), value, length) == EOK,
                false, "type: %{public}d, length: %{public}d memcpy fail", type, length);
            break;
        default:
            ALOGW("unknown type: %{public}d", type);
            break;
    }
    ALOGD("serivceMgmt: %{public}d, parseLen: %{public}d", result->protocolType[0], length);
    return true;
}

static void HiboxDeviceAbilityReport(uint8_t *data, uint16_t dataLen, uint8_t *result)
{
    ALOGW("dataLen: %{public}d", dataLen);
    HiboxParseMsgInd *parseResult = (HiboxParseMsgInd *)result;
    NL_CHECK_RETURN(parseResult, "parseResult is null");
    parseResult->msgType = DEVICE_MANUFACTURE_ABILITY;
    parseResult->datalen = 0;
    uint16_t parseLen = HiboxTlvParseHandler(data, dataLen, HiboxDeviceAbilityParser, parseResult->data);
    if (parseLen > sizeof(ManufacturerAbilityInfo)) {
        ALOGW("parseLen: %{public}d", parseLen);
        return;
    }
    parseResult->datalen = parseLen;
}

bool HiboxServiceManagerRspParse(uint8_t cmdId, uint8_t *tlv, uint16_t len, uint8_t *result)
{
    NL_CHECK_RETURN_RET(tlv && len > HIBOX_TLV_HEADER_LEN && result, false, "command is %{public}d", cmdId);
    ALOGD("command is %{public}d", cmdId);
    switch (cmdId) {
        case CMDID_DEVICE_ABILITY:
            HiboxDeviceAbilityReport(tlv, len, result);
            break;
        default:
            return false;
    }
    return true;
}

// comb echo 1,2 req
uint16_t HiboxDeviceAbilityReqComb(uint8_t *buffer, uint16_t bufferLen, HiboxParseMsgInd *arg)
{
    uint16_t offset = 0;
    NL_CHECK_RETURN_RET(buffer && arg, offset, "buffer or arg is NULL");
    if (arg->datalen != sizeof(ManufacturerAbilityInfo)) {
        ALOGE("datalen: %{public}d is wrong", arg->datalen);
        return offset;
    }
    ManufacturerAbilityInfo *info = (ManufacturerAbilityInfo *)(arg->data);
    ALOGD("serivceMgmt=%{public}d", info->manufacturerAbility[0]);
    SERVICE_HIBOX_ADD_TLV(buffer, bufferLen, offset, PROTOCOL_TYPE, sizeof(info->protocolType), info->protocolType);
    SERVICE_HIBOX_ADD_TLV(buffer, bufferLen, offset, OS_TYPE, sizeof(info->deviceOsType), info->deviceOsType);
    SERVICE_HIBOX_ADD_TLV(buffer, bufferLen, offset, MANUFACTURER_ABILITY_BIT_MAP, sizeof(info->manufacturerAbility),
        info->manufacturerAbility);
    return offset;
}
