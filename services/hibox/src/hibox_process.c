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
#define LOG_TAG "hibox_process"

#include "hibox_process.h"
#include "hibox_dts.h"
#include "hibox_service.h"
#include "hibox_pairconn.h"
#include "hibox_audio.h"
#include "hibox_hitws.h"

static HiboxSendReqMsgHandler g_hiboxSendReqFunc;
static HiboxSendRspMsgHandler g_hiboxSendRspFunc;
static HiboxMsgIndHandler g_hiboxParseIndFunc;
static HiboxMsgCfmHandler g_hiboxParseCfmFunc;

static HiboxDataTypeTable g_dataTypeTable[] = {
    { WEAR_STATUS, SID_DTS_MGMT, CMDID_WEAR_STATUS, sizeof(DtsWearData), sizeof(HiboxRspCommonResult) },
    { DTS_CAP, SID_DTS_MGMT, CMDID_CAPABILITY, sizeof(DtsCapData), sizeof(DtsCapData) },
    { DTS_REPORT, SID_DTS_MGMT, CMDID_AT_REPORT, sizeof(AtStrData), sizeof(AtStrData) },
    { AUDIO_EXCEPTION, SID_DTS_MGMT, CMDID_AUDIO_EXCEPTION, sizeof(AudioExceptionData), sizeof(AudioExceptionData) },
    { EARBUDS_NATRUE, SID_PAIR_CONN, CMDID_EARBUDS_NATURE, sizeof(EarbudsNature), sizeof(HiboxRspCommonResult) },
    { QUERY_BUSINESS, SID_PAIR_CONN, CMDID_QUERY_CONN, sizeof(QueryPairConn), sizeof(QueryPairConnCfmResult) },
    { VENDOR_ACCOUNT_HASH, SID_PAIR_CONN, CMDID_VENDOR_ACCOUNT_HASH, sizeof(VendorAccountHash),
        sizeof(HiboxRspCommonResult) },
    { HITWS_ISO_HANDOVER, SID_HITWS_CTRL, CMDID_HITWS_ROLE_HANDOVER_INFORM, sizeof(HitwsIsoHandover),
        sizeof(HitwsIsoHandover) },
    { AUDIO_CAP_QUERY, SID_AUDIO_MGMT, CMDID_AUDIO_CAP, sizeof(DeviceAudioCapsInfo), sizeof(DeviceAudioCapsInfo) },
    /* *< echo 8,B */
    { NEARLINK_AUDIO_PROFILE, SID_AUDIO_MGMT, CMDID_NEARLINK_PROFILE_STATE, sizeof(HiBoxNearlinkProfile),
        sizeof(HiboxHdapCommonRsp) },
    /* *< echo 8,C */
    { NEARLINK_AUDIO_DATAPATH, SID_AUDIO_MGMT, CMDID_NEARLINK_OUTPUT_DATAPATH, sizeof(HiboxNearlinkDataPath),
        sizeof(HiboxHdapCommonRsp) },
    /* *< echo 5,E */
    { AUTO_CONN_SWITCH_CFM, SID_PAIR_CONN, CMDID_AUTO_CONN_SWITCH, sizeof(MultConnAutoConnSwitch),
        sizeof(HiboxRspCommonResult) },
    /* *<echo 5,11 */
    { NOTIFY_SLE_DISCONNECT_PROFILE, SID_PAIR_CONN, CMDID_NOTIFY_SLE_DISCONNECT_PROFILE,
        sizeof(NotifySleDisconnectProfile), sizeof(HiboxRspCommonResult)},
    /* *< echo 5,12 */
    { SLE_ALL_PROFILE_CONNECTED, SID_PAIR_CONN, CMDID_SLE_ALL_PROFILE_CONNECTED, sizeof(HiboxSleProfileState),
        sizeof(HiboxHdapCommonRsp)},
    /* *< echo 1,2 */
    {DEVICE_MANUFACTURE_ABILITY, SID_SEVICE_MGMT, CMDID_DEVICE_ABILITY, sizeof(ManufacturerAbilityInfo),
        sizeof(ManufacturerAbilityInfo)},
    /* *< echo 8,E */
    { NEARLINK_DUAL_REC_PARAM, SID_AUDIO_MGMT, CMDID_DUAL_REC_PARAM_REPORT, sizeof(HiBoxNearlinkDualRecCaps),
        sizeof(DeviceDualRecParmInfo)},
};

static bool HiboxIsHiboxReq(uint8_t *data, uint16_t len)
{
    uint8_t header;
    uint16_t recvMIC;
    uint16_t calcMIC;

    NL_CHECK_RETURN_RET(data && len > HIBOX_MIN_PAYLOAD_LEN, false, "length error, len=%{public}d", len);
    header = data[0];
    NL_CHECK_RETURN_RET(header == HIBOX_REQ_FIXED_HEADER, false,
        "data header isn't fixed 0x3F, header=%{public}u", header);

    uint8_t *p = data + len - HIBOX_TLV_MIC_LEN;
    STREAM_TO_UINT16(recvMIC, p);
    calcMIC = HiboxL2cuDoCheckMic(data, len - HIBOX_TLV_MIC_LEN);
    NL_CHECK_RETURN_RET(recvMIC == calcMIC, false,
        "MIC error, recvd=0x%{public}x, calculate=0x%{public}x", recvMIC, calcMIC);

    return true;
}

static bool HiboxIsHiboxRsp(uint8_t *data, uint16_t len)
{
    uint16_t recvMIC;
    uint16_t calcMIC;
    NL_CHECK_RETURN_RET(data && len >= HIBOX_MIN_PAYLOAD_LEN, false, "length error, len=%{public}d", len);

    uint8_t header = data[0];
    NL_CHECK_RETURN_RET(header == HIBOX_RSP_FIXED_HEADER, false,
        "data header isn't fixed 0xF3, header=%{public}u", header);

    uint8_t *p = data + len - HIBOX_TLV_MIC_LEN;
    STREAM_TO_UINT16(recvMIC, p);
    calcMIC = HiboxL2cuDoCheckMic(data, len - HIBOX_TLV_MIC_LEN);
    NL_CHECK_RETURN_RET(recvMIC == calcMIC, false,
        "MIC error, recvd=0x%{public}x, calculate=0x%{public}x", recvMIC, calcMIC);

    return true;
}

static bool HiboxGetSidByDataType(uint8_t dataType, uint8_t *sid, uint8_t *cmdId)
{
    uint8_t len;
    int i;
    len = sizeof(g_dataTypeTable) / sizeof(HiboxDataTypeTable);
    for (i = 0; i < len; i++) {
        HiboxDataTypeTable handler = g_dataTypeTable[i];
        if (dataType == handler.dataType) {
            *sid = handler.sid;
            *cmdId = handler.cmdId;
            return true;
        }
    }
    return false;
}

static uint16_t HiboxCombileRspMsgByMsgTypePart(uint8_t *buffer, uint16_t bufferLen,
    HiboxParseMsgInd *arg)
{
    uint16_t offset = 0;
    switch (arg->msgType) {
        case HDAP_CODEC_REPORT:
        case HDAP_SET_CODEC:
        case HDAP_START_BUSINESS:
        case HDAP_STOP_BUSINESS:
        case HDAP_SEND_KARAOKE_PARAM:
        case HDAP_VERSION_PARAM:
        case AUDIO_CODEC_RATE:
        case NOTIFY_PROFILE_STATE:
        case NOTIFY_OUTPUT_PATH:
        case AUDIO_CAP_QUERY:
        case NEARLINK_AUDIO_PROFILE:
        case NEARLINK_AUDIO_DATAPATH:
        case NEARLINK_DUAL_REC_PARAM:
            offset = HiboxAudioMgmtRspComb(buffer, bufferLen, arg);
            break;
        case HITWS_ISO_CONFIG:
        case HITWS_ISO_HANDOVER:
        case HITWS_ISO_RESTART:
            offset = HiboxHitwsManagerRspComb(buffer, bufferLen, arg);
            break;
        default:
            break;
    }
    return offset;
}

static uint16_t HiboxCombileRspMsgByMsgType(uint8_t *buffer, uint16_t bufferLen, HiboxParseMsgInd *arg)
{
    uint16_t offset = 0;
    switch (arg->msgType) {
        case DTS_CAP:
        case WEAR_STATUS:
        case DTS_REPORT:
        case AUDIO_EXCEPTION:
            offset = HiboxDtsMgmtRspComb(buffer, bufferLen, arg);
            break;
        default:
            offset = HiboxCombileRspMsgByMsgTypePart(buffer, bufferLen, arg);
            break;
    }
    return offset;
}

static uint16_t HiboxGetParseReqResultLen(uint8_t sid, uint8_t cmdId)
{
    uint8_t len;
    int i;
    len = sizeof(g_dataTypeTable) / sizeof(HiboxDataTypeTable);
    for (i = 0; i < len; i++) {
        HiboxDataTypeTable handler = g_dataTypeTable[i];
        if ((sid == handler.sid) && (cmdId == handler.cmdId)) {
            return handler.parseReqLen;
        }
    }
    return 0;
}

static uint16_t HiboxGetParseRspResultLen(uint8_t sid, uint8_t cmdId)
{
    uint8_t len;
    int i;
    len = sizeof(g_dataTypeTable) / sizeof(HiboxDataTypeTable);
    for (i = 0; i < len; i++) {
        HiboxDataTypeTable handler = g_dataTypeTable[i];
        if ((sid == handler.sid) && (cmdId == handler.cmdId)) {
            return handler.parseRspLen;
        }
    }
    return 0;
}

void HiboxRegisterProcess(HiboxRegisterFunc *func)
{
    NL_CHECK_RETURN(func, "func is null");
    g_hiboxSendReqFunc = func->sendReqFunc;
    g_hiboxSendRspFunc = func->sendRspFunc;
    g_hiboxParseIndFunc = func->msgIndFunc;
    g_hiboxParseCfmFunc = func->msgCfmFunc;
}

static uint16_t HiboxCombileReqMsgByMsgTypePart(uint8_t *buffer, uint16_t bufferLen,
    HiboxParseMsgInd *arg)
{
    uint16_t offset = 0;
    switch (arg->msgType) {
        case HDAP_CODEC_REPORT:
        case HDAP_SET_CODEC:
        case HDAP_START_BUSINESS:
        case HDAP_STOP_BUSINESS:
        case HDAP_SEND_KARAOKE_PARAM:
        case HDAP_VERSION_PARAM:
        case AUDIO_CODEC_RATE:
        case NOTIFY_PROFILE_STATE:
        case NOTIFY_OUTPUT_PATH:
        case AUDIO_CAP_QUERY:
        case NEARLINK_AUDIO_PROFILE:
        case NEARLINK_AUDIO_DATAPATH:
        case NEARLINK_DUAL_REC_PARAM:
            offset = HiboxAudioMgmtReqComb(buffer, bufferLen, arg);
            break;
        default:
            break;
    }
    return offset;
}

static uint16_t HiboxCombileReqMsgByMsgType(uint8_t *buffer, uint16_t bufferLen, HiboxParseMsgInd *arg)
{
    uint16_t offset = 0;
    switch (arg->msgType) {
        case DTS_CAP:
            offset = HiboxDtsMgmtReqComb(buffer, bufferLen, arg);
            break;
        case QUERY_BUSINESS:
        case VENDOR_ACCOUNT_HASH:
        case SLE_ALL_PROFILE_CONNECTED:
            offset = HiboxPairConnMgmtReqComb(buffer, bufferLen, arg);
            break;
        case DEVICE_MANUFACTURE_ABILITY:
            offset = HiboxDeviceAbilityReqComb(buffer, bufferLen, arg);
            break;
        default:
            offset = HiboxCombileReqMsgByMsgTypePart(buffer, bufferLen, arg);
            break;
    }
    return offset;
}

static bool HiboxParseReqMsg(uint8_t sid, uint8_t cmdId, uint16_t len, uint8_t *tlv, uint8_t *result)
{
    ALOGD("service = %{public}d cmdid = %{public}d", sid, cmdId);
    bool parseResult = false;
    switch (sid) {
        case SID_HITWS_CTRL:
            parseResult = HiboxHitwsManagerReqParse(cmdId, tlv, len, result);
            break;
        case SID_DTS_MGMT:
            parseResult = HiboxDtsMgmtReqParse(cmdId, tlv, len, result);
            break;
        case SID_PAIR_CONN:
            parseResult = HiboxPairConnMgmtReqParse(cmdId, tlv, len, result);
            break;
        case SID_AUDIO_MGMT:
            parseResult = HiboxAudioMgmtReqParse(cmdId, tlv, len, result);
            break;
        default:
            ALOGE("service = %{public}d cmdid = %{public}d", sid, cmdId);
            break;
    }
    return parseResult;
}

static bool HiboxParseRspMsg(uint8_t sid, uint8_t cmdId, uint16_t len, uint8_t *tlv, uint8_t *result)
{
    ALOGD("service = %{public}d cmdid = %{public}d", sid, cmdId);
    bool parseResult = false;
    switch (sid) {
        case SID_SEVICE_MGMT:
            parseResult = HiboxServiceManagerRspParse(cmdId, tlv, len, result);
            break;
        case SID_DTS_MGMT:
            parseResult = HiboxDtsMgmtRspParse(cmdId, tlv, len, result);
            break;
        case SID_PAIR_CONN:
            parseResult = HiboxPairConnMgmtRspParse(cmdId, tlv, len, result);
            break;
        case SID_AUDIO_MGMT:
            parseResult = HiboxAudioMgmtRspParse(cmdId, tlv, len, result);
            break;
        default:
            break;
    }
    return parseResult;
}

bool HiboxRecvCfmMsg(uint8_t *echoRsp, uint16_t len, uint8_t *args)
{
    if ((echoRsp == NULL) || (len == 0) || (args == NULL)) {
        return false;
    }
    if (!HiboxIsHiboxRsp(echoRsp, len)) {
        return false;
    }
    uint8_t header, serviceId, commandId;
    uint8_t *echoData = echoRsp;
    STREAM_TO_UINT8(header, echoData);
    STREAM_TO_UINT8(serviceId, echoData);
    STREAM_TO_UINT8(commandId, echoData);
    uint8_t *tlv = echoRsp + HIBOX_TLV_HEADER_LEN;
    uint16_t tlvLen = len - HIBOX_MIN_PAYLOAD_LEN;
    ALOGI("serviceId=%{public}u,commandId=%{public}u,Len=%{public}u", serviceId, commandId, len);
    if (tlvLen <= HIBOX_TLV_HEADER_LEN) {
        return false;
    }
    uint8_t *tlvData = (uint8_t *)malloc(tlvLen);
    if (tlvData == NULL) {
        return false;
    }
    if (memcpy_s(tlvData, tlvLen, tlv, tlvLen) != EOK) {
        free(tlvData);
        return false;
    }
    uint16_t parseResultLen = HiboxGetParseRspResultLen(serviceId, commandId);
    if (parseResultLen == 0) {
        free(tlvData);
        return false;
    }
    uint8_t *result = (uint8_t *)malloc(sizeof(HiboxParseMsgInd) + parseResultLen);
    if (result == NULL) {
        free(tlvData);
        return false;
    }
    (void)memset_s(result, parseResultLen, HIBOX_INVALID_DATA, parseResultLen);
    bool parseResult = HiboxParseRspMsg(serviceId, commandId, tlvLen, tlvData, result);
    HiboxParseMsgInd *parseData = (HiboxParseMsgInd *)result;
    if (!parseResult || parseData->datalen == 0) {
        free(tlvData);
        free(result);
        return false;
    }
    uint8_t echoType = HIBOX_RSP;
    g_hiboxParseCfmFunc(echoType, result, sizeof(HiboxParseMsgInd) + parseResultLen, args);
    free(tlvData);
    free(result);
    return true;
}

bool HiboxCombRspMsg(uint8_t *args, uint8_t *rspData, uint16_t dataLen)
{
    if (rspData == NULL || args == NULL) {
        return false;
    }
    if (dataLen <= HIBOX_SOURCE_DATA_HEADER_LEN) {
        return false;
    }

    uint8_t tlvValue[TLV_BUF_LEN] = {0};
    uint16_t bufLen = sizeof(tlvValue);
    uint16_t offset;
    HiboxParseMsgInd *sourceData = (HiboxParseMsgInd *)rspData;
    uint8_t sid = 0;
    uint8_t cmdId = 0;
    ALOGD("arg->datalen = %{public}d, dataLen = %{public}d", sourceData->datalen, dataLen);
    NL_CHECK_RETURN_RET((sourceData->datalen + HIBOX_SOURCE_DATA_HEADER_LEN) == dataLen, false,
        "length not match,%{public}u/%{public}u,msg type:%{public}u,data len:%{public}u",
        (sourceData->datalen + HIBOX_SOURCE_DATA_HEADER_LEN), dataLen, sourceData->msgType, sourceData->datalen);

    bool getResult = HiboxGetSidByDataType(sourceData->msgType, &sid, &cmdId);
    NL_CHECK_RETURN_RET(getResult, false, "dataType = %{public}d", sourceData->msgType);
    offset = HiboxCombileRspMsgByMsgType(tlvValue, bufLen, sourceData);
    NL_CHECK_RETURN_RET(offset != 0, false, "comb rsp fail msgType = %{public}d", sourceData->msgType);
    uint16_t totalLen = offset + HIBOX_MIN_PAYLOAD_LEN;
    uint8_t *buf = (uint8_t *)malloc(totalLen);
    NL_CHECK_RETURN_RET(buf, false, "osi_malloc fail");
    (void)memset_s(buf, totalLen, 0, totalLen);
    HiboxCombileMsg(buf, totalLen, sid, cmdId, tlvValue, offset, HIBOX_RSP_FIXED_HEADER);
    ALOGD("service = %{public}d cmdid = %{public}d tlvlen = %{public}d", sid, cmdId, offset);
    g_hiboxSendRspFunc(args, buf, totalLen);
    free(buf);
    return true;
}

bool HiboxCombReqMsg(uint8_t *addr, uint8_t *source, uint16_t dataLen)
{
    if (source == NULL || addr == NULL) {
        return false;
    }
    if (dataLen <= HIBOX_SOURCE_DATA_HEADER_LEN || dataLen > TLV_BUF_LEN) {
        return false;
    }
    uint8_t tlvValue[TLV_BUF_LEN] = {0};
    uint16_t bufLen = sizeof(tlvValue);
    uint16_t offset;
    HiboxParseMsgInd *arg = (HiboxParseMsgInd *)source;
    uint8_t sid = 0;
    uint8_t cmdId = 0;
    if ((arg->datalen + HIBOX_SOURCE_DATA_HEADER_LEN) != dataLen) {
        ALOGE("length not match");
        return false;
    }
    bool getResult = HiboxGetSidByDataType(arg->msgType, &sid, &cmdId);
    if (!getResult) {
        ALOGE("get sid fail msgType = %{public}d", arg->msgType);
        return false;
    }
    ALOGI("req msgType=0x%{public}02x,service Id=0x%{public}02x,commandId=0x%{public}02x,len=%{public}u",
        arg->msgType, sid, cmdId, dataLen);
    offset = HiboxCombileReqMsgByMsgType(tlvValue, bufLen, arg);
    if (offset == 0) {
        ALOGE("comb req fail msgType = %{public}#x", arg->msgType);
        return false;
    }
    uint16_t totalLen = offset + HIBOX_MIN_PAYLOAD_LEN;
    uint8_t *buf = (uint8_t *)malloc(totalLen);
    if (buf == NULL) {
        ALOGE("osi_malloc fail");
        return false;
    }
    (void)memset_s(buf, totalLen, 0, totalLen);
    HiboxCombileMsg(buf, totalLen, sid, cmdId, tlvValue, offset, HIBOX_REQ_FIXED_HEADER);
    ALOGD("service = %{public}d cmdid = %{public}d tlvlen = %{public}d", sid, cmdId, offset);
    g_hiboxSendReqFunc(addr, buf, totalLen);
    free(buf);
    return true;
}

bool HiboxRecvIndMsg(uint8_t *echoReq, uint16_t len, uint8_t *args)
{
    if ((echoReq == NULL) || (len == 0) || (args == NULL)) {
        return false;
    }
    if (!HiboxIsHiboxReq(echoReq, len)) {
        return false;
    }
    uint8_t header;
    uint8_t serviceId;
    uint8_t commandId;
    uint8_t *echoData = echoReq;
    STREAM_TO_UINT8(header, echoData);
    STREAM_TO_UINT8(serviceId, echoData);
    STREAM_TO_UINT8(commandId, echoData);
    uint16_t tlvLen = len - HIBOX_MIN_PAYLOAD_LEN;
    ALOGI("serviceId=%{public}d, commandId=%{public}d, tlvLen=%{public}d", serviceId, commandId, tlvLen);
    if (tlvLen <= HIBOX_TLV_HEADER_LEN) {
        ALOGE("tlvLen:%{public}d is error", tlvLen);
        return false;
    }
    uint16_t parseResultLen = HiboxGetParseReqResultLen(serviceId, commandId);
    ALOGI("parseResultLen:%{public}d", parseResultLen);
    if (parseResultLen == 0) {
        return false;
    }
    uint8_t *result = (uint8_t *)malloc(sizeof(HiboxParseMsgInd) + parseResultLen);
    NL_CHECK_RETURN_RET(result, false, "alloc result fail");
    (void)memset_s(result, sizeof(HiboxParseMsgInd) + parseResultLen, HIBOX_INVALID_DATA,
        sizeof(HiboxParseMsgInd) + parseResultLen);
    ALOGD("echoData[0]:%{public}d, echoData[1]:%{public}d", echoData[0], echoData[1]);
    bool parseResult = HiboxParseReqMsg(serviceId, commandId, tlvLen, echoData, result);
    HiboxParseMsgInd *parseData = (HiboxParseMsgInd *)result;
    if (!parseResult || parseData->datalen == 0) {
        ALOGE("parse data fail,datalen=%{public}d,parseResult=%{public}d", parseData->datalen, parseResult);
        free(result);
        return false;
    }
    /* report parse msg to service */
    uint8_t echoType = HIBOX_REQ;
    ALOGD("msgType:%{public}d", result[0]);
    g_hiboxParseIndFunc(echoType, result, sizeof(HiboxParseMsgInd) + parseResultLen, args);
    free(result);
    return true;
}
