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
#define LOG_TAG "hibox_pairconn"

#include "hibox_pairconn.h"
#include "hibox_process.h"
#include "hibox_local.h"

static uint16_t HiboxVendorAccountHashComb(uint8_t *buffer, uint16_t bufferLen, HiboxParseMsgInd *arg)
{
    uint16_t offset = 0;
    NL_CHECK_RETURN_RET(arg->datalen == sizeof(VendorAccountHash), offset,
        "datalen: %{public}d is wrong", arg->datalen);
    VendorAccountHash *vendorAccountHash = (VendorAccountHash *)(arg->data);
    SERVICE_HIBOX_ADD_TLV(buffer, bufferLen, offset, VENDOR_HONGMENG_OS_TYPE, HONGMENG_OS_TYPE_LEN,
        &(vendorAccountHash->deviceOsType));
    SERVICE_HIBOX_ADD_TLV(buffer, bufferLen, offset, VENDOR_ACCOUNT_HASH_TYPE, VENDOR_ACCOUNT_HASH_LEN,
        &(vendorAccountHash->accountHash));
    return offset;
}

static uint16_t HiboxSleProfileSateReqComb(uint8_t *buffer, uint16_t bufferLen, HiboxParseMsgInd *arg)
{
    uint16_t offset = 0;
    NL_CHECK_RETURN_RET(arg->datalen == sizeof(HiboxSleProfileState), offset,
        "datalen: %{public}d is wrong", arg->datalen);
    HiboxSleProfileState *state = (HiboxSleProfileState *)(arg->data);
    SERVICE_HIBOX_ADD_TLV(buffer, bufferLen, offset, ALL_AUDIO_PROFILE_CONNECTED, SLE_ALL_PROFILE_CONNECTED_LEN,
        &(state->state));
    return offset;
}

static uint16_t HiboxQueryBusinessComb(uint8_t *buffer, uint16_t bufferLen, HiboxParseMsgInd *arg)
{
    uint16_t offset = 0;
    NL_CHECK_RETURN_RET(arg->datalen == sizeof(QueryPairConn), offset,
        "datalen: %{public}d is wrong", arg->datalen);
    QueryPairConn *pairData = (QueryPairConn *)(arg->data);
    SERVICE_HIBOX_ADD_TLV(buffer, bufferLen, offset, QUERY_BUSINESS_CONN_TYPE,
                           QUERY_BUSINESS_CONN_TYPE_LEN, &(pairData->connType));
    SERVICE_HIBOX_ADD_TLV(buffer, bufferLen, offset, QUERY_BUSINESS_ACCOUNT_HASH_TYPE,
                           VENDOR_ACCOUNT_HASH_LEN, &(pairData->accountHash));
    SERVICE_HIBOX_ADD_TLV(buffer, bufferLen, offset, QUERY_BUSINESS_MOBILE_BUSINESS_TYPE_TYPE,
                           MOBILE_BUSINESS_LEN, &(pairData->mobileBusinessType));
    return offset;
}

static bool HiboxQueryConnRspParse(uint8_t type, uint16_t length, uint8_t* value, void* parseResult)
{
    NL_CHECK_RETURN_RET(parseResult, false, "parseResult is NULL");
    uint8_t *typeValue = value;
    ALOGD("type: %{public}d, length: %{public}d", type, length);
    /* use this struct to pass parse result to corresponding hibox report function */
    QueryPairConnCfmResult *result = (QueryPairConnCfmResult *)parseResult;
    switch (type) {
        case QUERY_CONN_RSP_TYPE:
            if (length != QUERY_CONN_LEN) {
                return false;
            }
            result->msgType = type;
            STREAM_TO_UINT8(result->typeValue, typeValue);
            break;
        default:
            ALOGW("unknown type: %{public}d", type);
            break;
    }
    ALOGD("type:%{public}d,value:%{public}d", result->msgType, result->typeValue);
    return true;
}
static bool HiboxAutoConnSwitchParse(uint8_t type, uint16_t length, uint8_t* value, void* parseResult)
{
    if (parseResult == NULL) {
        ALOGW("parseResult is NULL");
        return false;
    }
    uint8_t *typeValue = value;
    ALOGD("type: %{public}d, length: %{public}d", type, length);
    MultConnAutoConnSwitch *result = (MultConnAutoConnSwitch *)parseResult;
    switch (type) {
        case AUTO_CONN_SWITCH_TYPE:
            if (length != AUTO_CONN_SWITCH_LEN) {
                return false;
            }
            STREAM_TO_UINT8(result->autoConnSwitch, typeValue);
            break;
        default:
            ALOGW("unknown type: %{public}d", type);
            break;
    }
    ALOGD("autoConnSwitch: %{public}d ", result->autoConnSwitch);
    return true;
}

static void HiboxAutoConnSwitch(uint8_t* data, uint16_t dataLen, uint8_t *result)
{
    HiboxParseMsgInd *parseResult = (HiboxParseMsgInd *)result;
    NL_CHECK_RETURN(parseResult, "parseResult is NULL");
    parseResult->msgType = AUTO_CONN_SWITCH_CFM;
    parseResult->datalen = 0;
    uint16_t parseLen = HiboxTlvParseHandler(data, dataLen, HiboxAutoConnSwitchParse, parseResult->data);
    if (parseLen > sizeof(MultConnAutoConnSwitch)) {
        ALOGW("parseLen: %{public}d", parseLen);
        return;
    }
    parseResult->datalen = sizeof(MultConnAutoConnSwitch);
}

static void HiboxQueryConnRsp(uint8_t* data, uint16_t dataLen, uint8_t *result)
{
    HiboxParseMsgInd *parseResult = (HiboxParseMsgInd *)result;
    NL_CHECK_RETURN(parseResult, "parseResult is NULL");
    parseResult->msgType = QUERY_BUSINESS;
    parseResult->datalen = 0;
    uint16_t parseLen = HiboxTlvParseHandler(data, dataLen, HiboxQueryConnRspParse, parseResult->data);
    if (parseLen > sizeof(QueryPairConnCfmResult)) {
        ALOGW("parseLen: %{public}d", parseLen);
        return;
    }
    parseResult->datalen = sizeof(QueryPairConnCfmResult);
}

static bool HiboxNotifySleDisconnectProfileParse(uint8_t type, uint16_t length, uint8_t* value, void* parseResult)
{
    if (parseResult == NULL) {
        HILOGW("[hibox_pairconn] parseResult is NULL");
        return false;
    }
    uint8_t *typeValue = value;
    ALOGI("[hibox_pairconn] type: %{public}d, length: %{public}d", type, length);
    NotifySleDisconnectProfile *result = (NotifySleDisconnectProfile *)parseResult;
    switch (type) {
        case NOTIFY_DISCONNECT_PROFILE:
            if (length != NOTIFY_DISCONNECT_PROFILE_LEN) {
                return false;
            }
            STREAM_TO_UINT8(result->notifyValue, typeValue);
            break;
        default:
            ALOGW("[hibox_pairconn] unknown type: %{public}d", type);
            break;
    }
    ALOGI("[hibox_pairconn] type: %{public}d ", type);
    return true;
}

static void HiboxNotifySleDisconnectProfile(uint8_t* data, uint16_t dataLen, uint8_t *result)
{
    HiboxParseMsgInd *parseResult = (HiboxParseMsgInd *)result;
    NL_CHECK_RETURN(parseResult, "[hibox_pairconn] parseResult is null");
    parseResult->msgType = NOTIFY_SLE_DISCONNECT_PROFILE;
    parseResult->datalen = 0;
    uint16_t parseLen = HiboxTlvParseHandler(data, dataLen, HiboxNotifySleDisconnectProfileParse, parseResult->data);
    NL_CHECK_RETURN(parseLen <= sizeof(NotifySleDisconnectProfile), "[hibox_pairconn] parseLen: %{public}d", parseLen);
    parseResult->datalen = sizeof(NotifySleDisconnectProfile);
}

static bool HiboxEarbudsNatureParse(uint8_t type, uint16_t length, uint8_t *value, void *parseResult)
{
    if (parseResult == NULL) {
        ALOGW("parseResult is NULL");
        return false;
    }
    uint8_t *typeValue = value;
    ALOGD("type: %{public}d, length: %{public}d", type, length);
    EarbudsNature *result = (EarbudsNature *)parseResult;
    switch (type) {
        case EARBUDS_NATURE_TYPE:
            if (length != EARBUDS_NATURE_LEN) {
                return false;
            }
            STREAM_TO_UINT8(result->nature, typeValue);
            break;
        default:
            ALOGW("unknown type: %{public}d", type);
            break;
    }
    ALOGD("nature: %{public}d", result->nature);
    return true;
}

void HiboxEarbudsNature(uint8_t* data, uint16_t dataLen, uint8_t *result)
{
    HiboxParseMsgInd *parseResult = (HiboxParseMsgInd *)result;
    NL_CHECK_RETURN(parseResult, "[hibox_pairconn] parseResult is null");
    parseResult->msgType = EARBUDS_NATRUE;
    parseResult->datalen = 0;
    uint16_t parseLen = HiboxTlvParseHandler(data, dataLen, HiboxEarbudsNatureParse, parseResult->data);
    if (parseLen > sizeof(EarbudsNature)) {
        ALOGW("parseLen: %{public}d", parseLen);
        return;
    }
    parseResult->datalen = sizeof(EarbudsNature);
}

bool HiboxPairConnMgmtRspParse(uint8_t cmdId, uint8_t *tlv, uint16_t len, uint8_t *result)
{
    NL_CHECK_RETURN_RET(tlv && len > HIBOX_TLV_HEADER_LEN && result, false, "command is %{public}d", cmdId);
    ALOGD("command is %{public}d", cmdId);
    switch (cmdId) {
        case CMDID_QUERY_CONN:
            HiboxQueryConnRsp(tlv, len, result);
            break;
        case CMDID_VENDOR_ACCOUNT_HASH:
            HiboxCommResultParse(VENDOR_ACCOUNT_HASH, tlv, len, result);
            break;
        case CMDID_SLE_ALL_PROFILE_CONNECTED:
            HiboxCommResultParse(SLE_ALL_PROFILE_CONNECTED, tlv, len, result);
            break;
        default:
            ALOGE("command is %{public}d", cmdId);
            return false;
    }
    return true;
}

bool HiboxPairConnMgmtReqParse(uint8_t cmdId, uint8_t *tlv, uint16_t len, uint8_t *result)
{
    NL_CHECK_RETURN_RET(tlv && len > HIBOX_TLV_HEADER_LEN && result, false, "command is %{public}d", cmdId);
    ALOGD("command is %{public}d", cmdId);
    switch (cmdId) {
        case CMDID_EARBUDS_NATURE:
            HiboxEarbudsNature(tlv, len, result);
            break;
        case CMDID_AUTO_CONN_SWITCH:
            HiboxAutoConnSwitch(tlv, len, result);
            break;
        case CMDID_NOTIFY_SLE_DISCONNECT_PROFILE:
            HiboxNotifySleDisconnectProfile(tlv, len, result);
            break;
        default:
            ALOGE("command is %{public}d", cmdId);
            return false;
    }
    return true;
}

uint16_t HiboxPairConnMgmtReqComb(uint8_t *buffer, uint16_t bufferLen, HiboxParseMsgInd *arg)
{
    uint16_t offset = 0;
    NL_CHECK_RETURN_RET(buffer && arg, offset, "buffer or arg is NULL");
    switch (arg->msgType) {
        case QUERY_BUSINESS:
            offset = HiboxQueryBusinessComb(buffer, bufferLen, arg);
            break;
        case VENDOR_ACCOUNT_HASH:
            offset = HiboxVendorAccountHashComb(buffer, bufferLen, arg);
            break;
        case SLE_ALL_PROFILE_CONNECTED:
            offset = HiboxSleProfileSateReqComb(buffer, bufferLen, arg);
            break;
        default:
            break;
    }
    ALOGD("msgType=%{public}d, offset=%{public}d", arg->msgType, offset);
    return offset;
}
