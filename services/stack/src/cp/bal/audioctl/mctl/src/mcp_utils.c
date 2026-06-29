/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include "securec.h"
#include "sdf_mem.h"
#include "nlstk_log.h"
#include "ssap_utils.h"
#include "nlstk_ssap_app_server.h"
#include "mcp_utils.h"

#define MCP_UUID_FIFTEENTH_BYTE 14
#define MCP_UUID_SIXTEENTH_BYTE 15

static uint8_t g_ssapStdBaseUuid[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

typedef struct {
    NLSTK_McpPropertyType_E type;
    uint16_t uuid;
} McpPropertyTypeUuid_S;

static McpPropertyTypeUuid_S g_mcpMediaPropertyTypeUuid[NLSTK_MCP_MEDIA_MAX_PROPERTY] = {
    { NLSTK_MCP_MEDIA_INSTANCE_NAME,      MCP_MEDIA_INSTANCE_NAME_UUID },
    { NLSTK_MCP_MEDIA_INSTANCE_ICON,      MCP_MEDIA_INSTANCE_ICON_UUID },
    { NLSTK_MCP_MEDIA_BASIC_INFO,         MCP_MEDIA_BASIC_INFO_UUID },
    { NLSTK_MCP_MEDIA_EXTENDED_INFO,      MCP_MEDIA_EXTENDED_INFO_UUID },
    { NLSTK_MCP_MEDIA_IDENTIFIER_INFO,    MCP_MEDIA_IDENTIFIER_INFO_UUID },
    { NLSTK_MCP_MEDIA_PLAYBACK_POSITION,  MCP_MEDIA_PLAYBACK_POSITION_UUID },
    { NLSTK_MCP_MEDIA_SEGMENT_INFO,       MCP_MEDIA_SEGMENT_INFO_UUID },
    { NLSTK_MCP_PLAYBACK_SPEED,           MCP_PLAYBACK_SPEED_UUID },
    { NLSTK_MCP_SEEK_SPEED,               MCP_SEEK_SPEED_UUID },
    { NLSTK_MCP_FEATURE_SUPPORT,          MCP_FEATURE_SUPPORT_UUID },
    { NLSTK_MCP_PLAYBACK_ORDER,           MCP_PLAYBACK_ORDER_UUID },
    { NLSTK_MCP_PLAYBACK_STATE,           MCP_PLAYBACK_STATE_UUID },
    { NLSTK_MCP_MEDIA_INSTANCE_ID,        MCP_MEDIA_INSTANCE_ID_UUID },
};

uint16_t McpGetPropertyUuidByType(NLSTK_McpPropertyType_E propertyType)
{
    if (propertyType >= NLSTK_MCP_MEDIA_MAX_PROPERTY) {
        return MCP_INVALID_UUID;
    }
    return g_mcpMediaPropertyTypeUuid[propertyType].uuid;
}

NLSTK_McpPropertyType_E McpGetPropertyTypeByUuid(uint16_t uuid)
{
    for (int i = 0; i < NLSTK_MCP_MEDIA_MAX_PROPERTY; i++) {
        if (g_mcpMediaPropertyTypeUuid[i].uuid == uuid) {
            return g_mcpMediaPropertyTypeUuid[i].type;
        }
    }
    return NLSTK_MCP_MEDIA_MAX_PROPERTY;
}

NLSTK_SsapUuid_S McpConvertUuidToStru(uint16_t uuid)
{
    NLSTK_SsapUuid_S uuidStru;
    for (int i = 0; i < MCP_UUID_FIFTEENTH_BYTE; i++) {
        uuidStru.uuid[i] = g_ssapStdBaseUuid[i];
    }
    uuidStru.uuid[MCP_UUID_FIFTEENTH_BYTE] = (uint8_t)((uuid & 0xFF00) >> 0x08);
    uuidStru.uuid[MCP_UUID_SIXTEENTH_BYTE] = (uint8_t)(uuid & 0x00FF);
    return uuidStru;
}

uint16_t McpConvertUuidTo16Bits(NLSTK_SsapUuid_S uuidStru)
{
    uint16_t uuid = 0;
    uuid |= (uint16_t)(uuidStru.uuid[MCP_UUID_FIFTEENTH_BYTE]) << 0x08;
    uuid |= (uint16_t)(uuidStru.uuid[MCP_UUID_SIXTEENTH_BYTE]);
    return uuid;
}

void McpFreeMediaInfo(void *ptr)
{
    NLSTK_McpMediaInfo_S *info = (NLSTK_McpMediaInfo_S *)ptr;
    if (info == NULL) {
        return;
    }
    if (info->instanceName.data != NULL) {
        SDF_MemFree(info->instanceName.data);
    }
    if (info->mediaBaseInfo.mediaName != NULL) {
        SDF_MemFree(info->mediaBaseInfo.mediaName);
    }
    if (info->optionalItem.instanceIcon.iconValue != NULL) {
        SDF_MemFree(info->optionalItem.instanceIcon.iconValue);
    }
    if (info->optionalItem.mediaExtendedInfo.data != NULL) {
        SDF_MemFree(info->optionalItem.mediaExtendedInfo.data);
    }
    if (info->optionalItem.segmentInfo.data != NULL) {
        SDF_MemFree(info->optionalItem.segmentInfo.data);
    }
    SDF_MemFree(info);
}

void McpFreeUpdatePropertyParam(void *ptr)
{
    McpUpdateMediaPropertyParam_S *param = (McpUpdateMediaPropertyParam_S *)ptr;
    if (param == NULL) {
        return;
    }
    if (param->value != NULL) {
        if (param->value->data != NULL) {
            SDF_MemFree(param->value->data);
        }
        SDF_MemFree(param->value);
    }
    SDF_MemFree(param);
}

void McpFreeSetStreamVolumeParam(void *ptr)
{
    McpSetStreamVolumeParam_S *param = (McpSetStreamVolumeParam_S *)ptr;
    if (param == NULL) {
        return;
    }
    if (param->volumeArray != NULL) {
        SDF_MemFree(param->volumeArray);
    }
    SDF_MemFree(param);
}

static uint32_t CopyMemoryField(uint8_t **dest, size_t destLen, uint8_t *src, size_t srcLen)
{
    NLSTK_CHECK_RETURN(dest != NULL, NLSTK_ERRCODE_POINTER_NULL, "[MCP] dest is null");
    if (srcLen != 0) {
        NLSTK_CHECK_RETURN(src != NULL, NLSTK_ERRCODE_POINTER_NULL, "[MCP] src is null");
        *dest = (uint8_t *)SDF_MemZalloc(destLen);
        NLSTK_CHECK_RETURN(*dest != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[MCP] dest malloc fail");
        if (memcpy_s(*dest, destLen, src, srcLen) != EOK) {
            NLSTK_LOG_ERROR("[MCP] memcpy_s error");
            return NLSTK_ERRCODE_MEMCPY_FAIL;
        }
    }
    return NLSTK_ERRCODE_SUCCESS;
}

// 深拷贝MediaInfo，若失败由外部释放内存
uint32_t McpCopyMediaInfo(NLSTK_McpMediaInfo_S *dest, NLSTK_McpMediaInfo_S *src)
{
    (void)memcpy_s(dest, sizeof(NLSTK_McpMediaInfo_S), src, sizeof(NLSTK_McpMediaInfo_S));
    dest->instanceName.data = NULL;
    dest->mediaBaseInfo.mediaName = NULL;
    dest->optionalItem.instanceIcon.iconValue = NULL;
    dest->optionalItem.mediaExtendedInfo.data = NULL;
    dest->optionalItem.segmentInfo.data = NULL;
    uint32_t ret = NLSTK_ERRCODE_SUCCESS;
    ret = CopyMemoryField(&dest->instanceName.data, dest->instanceName.len,
        src->instanceName.data, src->instanceName.len);
    NLSTK_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, ret, "[MCP] copy instance name fail");
    ret = CopyMemoryField(&dest->mediaBaseInfo.mediaName, dest->mediaBaseInfo.mediaNameLen,
        src->mediaBaseInfo.mediaName, src->mediaBaseInfo.mediaNameLen);
    NLSTK_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, ret, "[MCP] copy media name fail");
    ret = CopyMemoryField(&dest->optionalItem.instanceIcon.iconValue, dest->optionalItem.instanceIcon.iconLen,
        src->optionalItem.instanceIcon.iconValue, src->optionalItem.instanceIcon.iconLen);
    NLSTK_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, ret, "[MCP] copy instance icon fail");
    ret = CopyMemoryField(&dest->optionalItem.mediaExtendedInfo.data, dest->optionalItem.mediaExtendedInfo.len,
        src->optionalItem.mediaExtendedInfo.data, src->optionalItem.mediaExtendedInfo.len);
    NLSTK_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, ret, "[MCP] copy extended info fail");
    ret = CopyMemoryField(&dest->optionalItem.segmentInfo.data, dest->optionalItem.segmentInfo.len,
        src->optionalItem.segmentInfo.data, src->optionalItem.segmentInfo.len);
    NLSTK_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, ret, "[MCP] copy segment info fail");
    return NLSTK_ERRCODE_SUCCESS;
}

static NLSTK_VariableData_S *McpValueConvert(void *data, uint16_t len)
{
    NLSTK_CHECK_RETURN(data != NULL, NULL, "[MCP] data is null");
    NLSTK_VariableData_S *value = (NLSTK_VariableData_S *)SDF_MemZalloc(sizeof(NLSTK_VariableData_S));
    NLSTK_CHECK_RETURN(value != NULL, NULL, "[MCP] value malloc fail");
    value->len = len;
    value->data = (uint8_t *)SDF_MemZalloc(value->len);
    if (value->data == NULL) {
        NLSTK_LOG_ERROR("[MCP] value data malloc fail");
        SDF_MemFree(value);
        return NULL;
    }
    (void)memcpy_s(value->data, value->len, data, len);
    return value;
}

static NLSTK_VariableData_S *McpLengthValueConvert(void *data)
{
    NLSTK_VariableData_S *param = (NLSTK_VariableData_S *)data;
    NLSTK_CHECK_RETURN(param != NULL, NULL, "[MCP] param is null");
    if (param->len != 0) {
        NLSTK_CHECK_RETURN(param->data != NULL, NULL, "[MCP] param data is null");
    }
    NLSTK_VariableData_S *value = (NLSTK_VariableData_S *)SDF_MemZalloc(sizeof(NLSTK_VariableData_S));
    NLSTK_CHECK_RETURN(value != NULL, NULL, "[MCP] value malloc fail");
    value->len = param->len;
    if (value->len == 0) {
        return value;
    }
    value->data = (uint8_t *)SDF_MemZalloc(value->len);
    if (value->data == NULL) {
        NLSTK_LOG_ERROR("[MCP] value data malloc fail");
        SDF_MemFree(value);
        return NULL;
    }
    (void)memcpy_s(value->data, value->len, param->data, param->len);
    return value;
}

static NLSTK_VariableData_S *McpMediaInstanceIconConvert(void *data)
{
    NLSTK_McpInstanceIcon_S *param = (NLSTK_McpInstanceIcon_S *)data;
    NLSTK_CHECK_RETURN(param != NULL, NULL, "[MCP] param is null");
    if (param->iconLen != 0) {
        NLSTK_CHECK_RETURN(param->iconValue != NULL, NULL, "[CCP] param icon is null");
    }
    NLSTK_VariableData_S *value = (NLSTK_VariableData_S *)SDF_MemZalloc(sizeof(NLSTK_VariableData_S));
    NLSTK_CHECK_RETURN(value != NULL, NULL, "[MCP] value malloc fail");
    value->len = param->iconLen + MCP_MEDIA_ICON_TYPE_LEN;
    value->data = (uint8_t *)SDF_MemZalloc(value->len);
    if (value->data == NULL) {
        NLSTK_LOG_ERROR("[MCP] value data malloc fail");
        SDF_MemFree(value);
        return NULL;
    }
    value->data[0] = param->iconType;
    (void)memcpy_s(value->data + MCP_MEDIA_ICON_TYPE_LEN, param->iconLen, param->iconValue, param->iconLen);
    return value;
}

static NLSTK_VariableData_S *McpMediaBasicInfoConvert(void *data)
{
    NLSTK_McpMediaBaseInfo_S *param = (NLSTK_McpMediaBaseInfo_S *)data;
    NLSTK_CHECK_RETURN(param != NULL && param->mediaName != NULL, NULL, "[MCP] param is null");
    NLSTK_VariableData_S *value = (NLSTK_VariableData_S *)SDF_MemZalloc(sizeof(NLSTK_VariableData_S));
    NLSTK_CHECK_RETURN(value != NULL, NULL, "[MCP] value malloc fail");
    value->len = param->mediaNameLen + MCP_MEDIA_NAME_OFFSET;
    value->data = (uint8_t *)SDF_MemZalloc(value->len);
    if (value->data == NULL) {
        NLSTK_LOG_ERROR("[MCP] value data malloc fail");
        SDF_MemFree(value);
        return NULL;
    }
    value->data[0] = param->mediaType;
    (void)memcpy_s(value->data + MCP_MEDIA_TYPE_LEN, MCP_MEDIA_DURATION_LEN, &param->duration, MCP_MEDIA_DURATION_LEN);
    value->data[MCP_MEDIA_NAME_LEN_OFFSET] = param->mediaNameLen;
    if (param->mediaNameLen != 0) {
        (void)memcpy_s(value->data + MCP_MEDIA_NAME_OFFSET, param->mediaNameLen, param->mediaName, param->mediaNameLen);
    }
    return value;
}

NLSTK_VariableData_S *McpMediaValueConvert(void *data, NLSTK_McpPropertyType_E type)
{
    NLSTK_CHECK_RETURN(data != NULL, NULL, "[MCP] data is null");
    switch (type) {
        case NLSTK_MCP_MEDIA_INSTANCE_NAME:
        case NLSTK_MCP_MEDIA_EXTENDED_INFO:
        case NLSTK_MCP_MEDIA_SEGMENT_INFO:
            return McpLengthValueConvert(data);
        case NLSTK_MCP_MEDIA_INSTANCE_ICON:
            return McpMediaInstanceIconConvert(data);
        case NLSTK_MCP_MEDIA_BASIC_INFO:
            return McpMediaBasicInfoConvert(data);
        case NLSTK_MCP_MEDIA_IDENTIFIER_INFO:
            return McpValueConvert(data, sizeof(NLSTK_McpMediaIdInfo_S));
        case NLSTK_MCP_MEDIA_PLAYBACK_POSITION:
            return McpValueConvert(data, sizeof(uint32_t));
        case NLSTK_MCP_PLAYBACK_SPEED:
        case NLSTK_MCP_SEEK_SPEED:
            return McpValueConvert(data, sizeof(uint16_t));
        case NLSTK_MCP_FEATURE_SUPPORT:
            return McpValueConvert(data, sizeof(NLSTK_McpFeaturesSupported_S));
        case NLSTK_MCP_PLAYBACK_ORDER:
        case NLSTK_MCP_PLAYBACK_STATE:
        case NLSTK_MCP_MEDIA_INSTANCE_ID:
            return McpValueConvert(data, sizeof(uint8_t));
        default:
            return NULL;
    }
}