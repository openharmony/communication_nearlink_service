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
#define LOG_TAG "hibox_audio"

#include "hibox_audio.h"
#include "hibox_process.h"
#include "hibox_local.h"

static uint16_t HiboxDualRecParamComb(uint8_t *buffer, uint16_t bufferLen, HiboxParseMsgInd *arg)
{
    uint16_t offset = 0;
    NL_CHECK_RETURN_RET(arg->datalen <= sizeof(DeviceDualRecParmInfo), offset,
        "datalen: %{public}d is wrong", arg->datalen);
    DeviceDualRecParmInfo *audioData = (DeviceDualRecParmInfo *)(arg->data);
    SERVICE_HIBOX_ADD_TLV(buffer, bufferLen, offset, NEARLINK_DUAL_REC_RSP_CODEC_INFO_TYPE,
        sizeof(DeviceDualRecParmInfo), audioData);
    return offset;
}

static uint16_t HiboxNotifyNearlinkOutDataPathComb(uint8_t *buffer, uint16_t bufferLen, HiboxParseMsgInd *arg)
{
    uint16_t offset = 0;
    NL_CHECK_RETURN_RET(arg->datalen <= sizeof(HiboxNearlinkDataPath), offset,
        "datalen: %{public}d is wrong", arg->datalen);
    HiboxNearlinkDataPath *audioData = (HiboxNearlinkDataPath *)(arg->data);
    SERVICE_HIBOX_ADD_TLV(buffer, bufferLen, offset, NEARLINK_OUTPUT_DATAPATH_AUDIO_TYPE, arg->datalen,
        audioData->audioType);
    return offset;
}

static uint16_t HiboxAudioCapComb(uint8_t *buffer, uint16_t bufferLen, HiboxParseMsgInd *arg)
{
    uint16_t offset = 0;
    NL_CHECK_RETURN_RET(buffer && arg, offset, "input param invalid");
    NL_CHECK_RETURN_RET(arg->datalen == sizeof(DeviceAudioCapsInfo), offset,
        "datalen: %{public}d is wrong", arg->datalen);
    DeviceAudioCapsInfo *pairData = (DeviceAudioCapsInfo *)(arg->data);
    SERVICE_HIBOX_ADD_TLV(buffer, bufferLen, offset, AUDIO_CAPS_TYPE, AUDIO_CAP_LEN,
        &(pairData->caps));
    SERVICE_HIBOX_ADD_TLV(buffer, bufferLen, offset, AUDIO_ABS_VOLUME_TYPE, AUDIO_ABSVOLUME_LEN,
        &(pairData->absVolume));
    SERVICE_HIBOX_ADD_TLV(buffer, bufferLen, offset, AUDIO_JOIN_INTERVAL_TYPE, AUDIO_INTERVAL_LEN,
        &(pairData->joinIntervalMs));
    SERVICE_HIBOX_ADD_TLV(buffer, bufferLen, offset, AUDIO_DDM_INTERVAL_TYPE, AUDIO_DDM_LATENCY_LEN,
        &(pairData->musicIntervalMs));

    return offset;
}

/* echo 8,a req data */
static bool HiboxAudioCapResultParse(uint8_t type, uint16_t length, uint8_t* value, void* parseResult)
{
    uint8_t *typeValue = value;
    ALOGD("type: %{public}d, length: %{public}d", type, length);
    DeviceAudioCapsInfo *result = (DeviceAudioCapsInfo *)parseResult;
    switch (type) {
        case AUDIO_CAPS_TYPE:
            if (length != AUDIO_CAP_LEN) {
                return false;
            }
            STREAM_TO_UINT32(result->caps, typeValue);
            break;
        case AUDIO_ABS_VOLUME_TYPE:
            if (length != AUDIO_ABSVOLUME_LEN) {
                return false;
            }
            STREAM_TO_UINT8(result->absVolume, typeValue);
            break;
        case AUDIO_JOIN_INTERVAL_TYPE:
            if (length != AUDIO_INTERVAL_LEN) {
                return false;
            }
            STREAM_TO_UINT16(result->joinIntervalMs, typeValue);
            break;
        case AUDIO_DDM_INTERVAL_TYPE:
            if (length != AUDIO_DDM_LATENCY_LEN) {
                return false;
            }
            STREAM_TO_UINT16(result->musicIntervalMs, typeValue);
            STREAM_TO_UINT16(result->hwaIntervalMs, typeValue);
            STREAM_TO_UINT16(result->otherIntervalMs, typeValue);
            break;
        case AUDIO_SLE_INTERVAL_TYPE:
            if (length != AUDIO_SLE_INTERVAL_LEN) {
                HILOGI("[HiboxAudioCapResultParse]: length != AUDIO_SLE_INTERVAL_LEN, length:%{public}d", length);
                return false;
            }
            STREAM_TO_INT16(result->sleConfig.offset, typeValue);
            STREAM_TO_UINT8(result->sleConfig.sceneNum, typeValue);
            for (uint8_t i = 0; i < MAX_SCENE_NUM; i++) {
                STREAM_TO_UINT8(result->sleConfig.timesMap[i].scene, typeValue);
                STREAM_TO_UINT8(result->sleConfig.timesMap[i].times, typeValue);
            }
            break;
        default:
            break;
    }

    return true;
}

static void HiboxAudioCapParse(uint8_t* data, uint16_t dataLen, uint8_t *result)
{
    HiboxParseMsgInd *parseResult = (HiboxParseMsgInd *)result;
    NL_CHECK_RETURN(parseResult, "parseResult is NULL ");

    parseResult->msgType = AUDIO_CAP_QUERY;
    uint16_t parseLen = HiboxTlvParseHandler(data, dataLen, HiboxAudioCapResultParse, parseResult->data);
    if (parseLen != sizeof(DeviceAudioCapsInfo)) {
        ALOGW("parseLen: %{public}d", parseLen);
    }
    parseResult->datalen = sizeof(DeviceAudioCapsInfo);
    if (parseLen == 0) {
        parseResult->datalen = 0;
    }
    result = (uint8_t *)parseResult;
}

/* echo 8,B 请求消息解析 */
static bool HiboxNearlinkProfileStateParse(uint8_t type, uint16_t length, uint8_t* value, void* parseResult)
{
    if (parseResult == NULL) {
        return false;
    }
    uint8_t *typeValue = value;
    HiBoxNearlinkProfile *result = (HiBoxNearlinkProfile *)parseResult;

    switch (type) {
        case NEARLINK_PROFILE_AUDIO_TYPE:
            if (length > NEARLINK_STREAM_TYPE_NUM) {
                return false;
            }
            STREAM_TO_ARRAY(result->audioType, typeValue, length);
            break;
        case NEARLINK_PROFILE_DEVICE_NAME_TYPE:
            (void)memset_s(result->targetDevName, sizeof(result->targetDevName), 0, sizeof(result->targetDevName));
            if (length > NEARLINK_DEVICE_NAME_LEN) {
                return false;
            }
            STREAM_TO_ARRAY(result->targetDevName, typeValue, length);
            break;
        case NEARLINK_PROFILE_STREAM_SERVICE_TYPE:
            if (length != sizeof(result->audioServiceType)) {
                return false;
            }
            result->audioServiceType = typeValue[0];
            break;
        default:
            ALOGE("msg type not support:%{public}u", type);
            break;
    }

    return true;
}

static void HiboxNearlinkProfileState(uint8_t* data, uint16_t dataLen, uint8_t *result)
{
    HiboxParseMsgInd *parseResult = (HiboxParseMsgInd *)result;
    NL_CHECK_RETURN(parseResult, "parseResult is null");
    parseResult->msgType = NEARLINK_AUDIO_PROFILE;
    parseResult->datalen = sizeof(HiBoxNearlinkProfile); /* 结构体的deviceName长度不固定，最长32byte，不检查固定长度。 */
    uint16_t parseLen = HiboxTlvParseHandler(data, dataLen, HiboxNearlinkProfileStateParse, parseResult->data);
    if (parseLen == 0) {
        parseResult->datalen = 0;
    }
    result = (uint8_t *)parseResult;
}

/* echo 8,E 请求消息解析 */
static bool HiboxNearlinkDualRecParamParse(uint8_t type, uint16_t length, uint8_t* value, void* parseResult)
{
    NL_CHECK_RETURN_RET(parseResult, false, "parseResult is null");
    uint8_t *typeValue = value;
    HiBoxNearlinkDualRecCaps *result = (HiBoxNearlinkDualRecCaps *)parseResult;
    switch (type) {
        case NEARLINK_DUAL_REC_REQ_CODEC_INFO_TYPE:
            if (length > sizeof(HiBoxNearlinkDualRecCaps)) {
                return false;
            }
            STREAM_TO_ARRAY(result, typeValue, length);
            break;
        default:
            ALOGE("msg type not support:%{public}u", type);
            break;
    }

    return true;
}

static void HiboxDualRecParamParse(uint8_t* data, uint16_t dataLen, uint8_t *result)
{
    HiboxParseMsgInd *parseResult = (HiboxParseMsgInd *)result;
    NL_CHECK_RETURN(parseResult, "parseResult is null");
    parseResult->msgType = NEARLINK_DUAL_REC_PARAM;
    parseResult->datalen = sizeof(HiBoxNearlinkDualRecCaps);
    uint16_t parseLen = HiboxTlvParseHandler(data, dataLen, HiboxNearlinkDualRecParamParse, parseResult->data);
    if (parseLen == 0) {
        parseResult->datalen = 0;
    }
    result = (uint8_t *)parseResult;
}

static bool HiboxAudioMgmtReqParseExt(uint8_t cmdId, uint8_t *tlv, uint16_t len, uint8_t *result)
{
    NL_CHECK_RETURN_RET(tlv && len > HIBOX_TLV_HEADER_LEN && result, false, "command is %{public}d", cmdId);
    switch (cmdId) {
        case CMDID_DUAL_REC_PARAM_REPORT:
            HiboxDualRecParamParse(tlv, len, result);
            break;
        default:
            ALOGE("command is %{public}d", cmdId);
            return false;
    }
    return true;
}

bool HiboxAudioMgmtReqParse(uint8_t cmdId, uint8_t *tlv, uint16_t len, uint8_t *result)
{
    NL_CHECK_RETURN_RET(tlv && len > HIBOX_TLV_HEADER_LEN && result, false, "command is %{public}d", cmdId);
    ALOGD("command is %{public}d", cmdId);
    switch (cmdId) {
        case CMDID_AUDIO_CAP:
            HiboxAudioCapParse(tlv, len, result);
            break;
        case CMDID_NEARLINK_PROFILE_STATE:
            HiboxNearlinkProfileState(tlv, len, result);
            break;
        default:
            return HiboxAudioMgmtReqParseExt(cmdId, tlv, len, result);
    }
    return true;
}

bool HiboxAudioMgmtRspParse(uint8_t cmdId, uint8_t *tlv, uint16_t len, uint8_t *result)
{
    NL_CHECK_RETURN_RET(tlv && len > HIBOX_TLV_HEADER_LEN && result, false, "command is %{public}d", cmdId);
    ALOGD("command is %{public}d", cmdId);
    switch (cmdId) {
        case CMDID_NEARLINK_OUTPUT_DATAPATH:
            HiboxCommResultParse(NEARLINK_AUDIO_DATAPATH, tlv, len, result);
            break;
        default:
            ALOGE("command is %{public}d", cmdId);
            return false;
    }
    return true;
}

uint16_t HiboxAudioMgmtReqComb(uint8_t *buffer, uint16_t bufferLen, HiboxParseMsgInd *arg)
{
    uint16_t offset = 0;
    NL_CHECK_RETURN_RET(buffer && arg, offset, "buffer or arg is NULL");
    switch (arg->msgType) {
        case NEARLINK_AUDIO_DATAPATH:
        /* echo 8,C */
            offset = HiboxNotifyNearlinkOutDataPathComb(buffer, bufferLen, arg);
            break;
        default:
            break;
    }
    return offset;
}

static uint16_t HiboxAudioMgmtRspCombExt(uint8_t *buffer, uint16_t bufferLen, HiboxParseMsgInd *arg)
{
    uint16_t offset = 0;
    NL_CHECK_RETURN_RET(buffer && arg, offset, "buffer or arg is NULL");
    switch (arg->msgType) {
        case NEARLINK_DUAL_REC_PARAM:
            offset = HiboxDualRecParamComb(buffer, bufferLen, arg);
            break;
        default:
            break;
    }
    return offset;
}

uint16_t HiboxAudioMgmtRspComb(uint8_t *buffer, uint16_t bufferLen, HiboxParseMsgInd *arg)
{
    uint16_t offset = 0;
    NL_CHECK_RETURN_RET(buffer && arg, offset, "buffer or arg is NULL");
    switch (arg->msgType) {
        case AUDIO_CAP_QUERY:
            offset = HiboxAudioCapComb(buffer, bufferLen, arg);
            break;
        case NEARLINK_AUDIO_PROFILE:
            offset = HiboxCommonResultComb(buffer, bufferLen, arg);
            break;
        default:
            offset = HiboxAudioMgmtRspCombExt(buffer, bufferLen, arg);
    }
    return offset;
}

