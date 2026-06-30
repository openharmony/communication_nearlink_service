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
#include "securec.h"
#include "sdf_mem.h"
#include "nlstk_log.h"
#include "nlstk_ssap_app_server.h"
#include "nlstk_ccp_ccs_server.h"
#include "ccp_type.h"
#include "ccp_utils.h"

#define CCP_UUID_FIFTEENTH_BYTE 14
#define CCP_UUID_SIXTEENTH_BYTE 15

static uint8_t g_ssapStdBaseUuid[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

typedef struct {
    NLSTK_CcpCcsPropertyType_E type;
    uint16_t uuid;
} CcpCcsPropertyTypeUuid_S;

static CcpCcsPropertyTypeUuid_S g_ccpPropertyTypeUuid[NLSTK_CCP_CCS_MAX_PROPERTY] = {
    { NLSTK_CCP_CCS_INSTANCE_NAME,      CCP_CCS_INSTANCE_NAME_UUID },
    { NLSTK_CCP_CCS_INSTANCE_ICON,      CCP_CCS_INSTANCE_ICON_UUID },
    { NLSTK_CCP_CCS_FEATURE_STATUS,     CCP_CCS_FEATURE_STATUS_UUID },
    { NLSTK_CCP_CCS_PROTOCOL_SUPPORT,   CCP_CCS_PROTOCOL_SUPPORT_UUID },
    { NLSTK_CCP_CCS_CALLIN_OUT_INFO,    CCP_CCS_CALLIN_OUT_INFO_UUID },
    { NLSTK_CCP_CCS_CALL_STATUS,        CCP_CCS_CALL_STATUS_UUID },
    { NLSTK_CCP_CCS_CALL_TERMINATION,   CCP_CCS_CALL_TERMINATION_UUID },
    { NLSTK_CCP_CCS_MEDIA_INSTANCE_ID,  CCP_CCS_MEDIA_INSTANCE_ID_UUID },
    { NLSTK_CCP_CCS_NETWORK_SELECTION,  CCP_CCS_NETWORK_SELECTION_UUID },
    { NLSTK_CCP_CCS_CALL_REQ_SUPPORT,   CCP_CCS_CALL_REQ_SUPPORT_UUID },
};

uint16_t CcpGetPropertyUuidByType(NLSTK_CcpCcsPropertyType_E propertyType)
{
    if (propertyType >= NLSTK_CCP_CCS_MAX_PROPERTY) {
        return CCP_CCS_INVALID_UUID;
    }
    return g_ccpPropertyTypeUuid[propertyType].uuid;
}

NLSTK_CcpCcsPropertyType_E CcpGetPropertyTypeByUuid(uint16_t uuid)
{
    for (int i = 0; i < NLSTK_CCP_CCS_MAX_PROPERTY; i++) {
        if (g_ccpPropertyTypeUuid[i].uuid == uuid) {
            return g_ccpPropertyTypeUuid[i].type;
        }
    }
    NLSTK_LOG_ERROR("[CCP] invaild uuid, uuid = %u", uuid);
    return NLSTK_CCP_CCS_MAX_PROPERTY;
}

NLSTK_SsapUuid_S CcpConvertUuidToStru(uint16_t uuid)
{
    NLSTK_SsapUuid_S uuidStru;
    for (int i = 0; i < CCP_UUID_FIFTEENTH_BYTE; i++) {
        uuidStru.uuid[i] = g_ssapStdBaseUuid[i];
    }
    uuidStru.uuid[CCP_UUID_FIFTEENTH_BYTE] = (uint8_t)((uuid & 0xFF00) >> 0x08);
    uuidStru.uuid[CCP_UUID_SIXTEENTH_BYTE] = (uint8_t)(uuid & 0x00FF);
    return uuidStru;
}

uint16_t CcpConvertUuidTo16Bits(NLSTK_SsapUuid_S uuidStru)
{
    uint16_t uuid = 0;
    uuid |= (uint16_t)(uuidStru.uuid[CCP_UUID_FIFTEENTH_BYTE]) << 0x08;
    uuid |= (uint16_t)(uuidStru.uuid[CCP_UUID_SIXTEENTH_BYTE]);
    return uuid;
}

static uint32_t CcpCopyMemoryField(uint8_t **dest, size_t destLen, uint8_t *src, size_t srcLen)
 
{
    NLSTK_CHECK_RETURN(dest != NULL, NLSTK_ERRCODE_POINTER_NULL, "[MCP] dest is null");
    if (srcLen != 0) {
        NLSTK_CHECK_RETURN(src != NULL, NLSTK_ERRCODE_POINTER_NULL, "[MCP] src is null");
        *dest = (uint8_t *)SDF_MemZalloc(destLen);
        NLSTK_CHECK_RETURN(*dest != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[MCP] dest malloc fail");
        if (memcpy_s(*dest, destLen, src, srcLen) != EOK) {
            NLSTK_LOG_ERROR("[CCP] memcpy_s error");
            return NLSTK_ERRCODE_MEMCPY_FAIL;   // 该函数失败不释放内存，由调用者负责统一释放
        }
    }
    // 如果srcLen为0，不认为是失败，直接跳过
    return NLSTK_ERRCODE_SUCCESS;
}

// 内存申请失败由调用者负责统一释放
uint32_t CcpCopyCallControlInfo(NLSTK_CcpCallControlInfo_S *dest, NLSTK_CcpCallControlInfo_S *src)
{
    (void)memcpy_s(dest, sizeof(NLSTK_CcpCallControlInfo_S), src, sizeof(NLSTK_CcpCallControlInfo_S));
    dest->instanceName.data = NULL;
    dest->instanceIcon.icon = NULL;
    dest->protocolSupport.data = NULL;
    dest->networkSelection.data = NULL;
    dest->callInOutInfo.userInfo.data = NULL;
    dest->callInOutInfo.userAlias.data = NULL;
    dest->callStatus.callId = NULL;
    dest->callStatus.networkId = NULL;
    dest->callStatus.callStatus = NULL;
    dest->callStatus.callFlag = NULL;

    // 这里拷贝通话控制信息时执行可选拷贝策略，有该成员则拷贝，没有或者len为0则跳过拷贝，不认为是失败
    uint32_t ret = NLSTK_ERRCODE_SUCCESS;
    ret = CcpCopyMemoryField(&dest->instanceName.data, dest->instanceName.len,
        src->instanceName.data, src->instanceName.len);
    NLSTK_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, ret, "[CCP] copy instance name fail");
    ret = CcpCopyMemoryField(&dest->instanceIcon.icon, dest->instanceIcon.len,
        src->instanceIcon.icon, src->instanceIcon.len);
    NLSTK_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, ret, "[CCP] copy instance icon fail");
    ret = CcpCopyMemoryField(&dest->protocolSupport.data, dest->protocolSupport.len,
        src->protocolSupport.data, src->protocolSupport.len);
    NLSTK_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, ret, "[CCP] copy protocol support fail");
    ret = CcpCopyMemoryField(&dest->networkSelection.data, dest->networkSelection.len,
        src->networkSelection.data, src->networkSelection.len);
    NLSTK_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, ret, "[CCP] copy network selection fail");
    ret = CcpCopyMemoryField(&dest->callInOutInfo.userInfo.data, dest->callInOutInfo.userInfo.len,
        src->callInOutInfo.userInfo.data, src->callInOutInfo.userInfo.len);
    NLSTK_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, ret, "[CCP] copy user info fail");
    ret = CcpCopyMemoryField(&dest->callInOutInfo.userAlias.data, dest->callInOutInfo.userAlias.len,
        src->callInOutInfo.userAlias.data, src->callInOutInfo.userAlias.len);
    NLSTK_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, ret, "[CCP] copy user alias fail");
    ret = CcpCopyMemoryField(&dest->callStatus.callId, dest->callStatus.callCount,
        src->callStatus.callId, src->callStatus.callCount);
    NLSTK_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, ret, "[CCP] copy call id fail");
    ret = CcpCopyMemoryField(&dest->callStatus.networkId, dest->callStatus.callCount,
        src->callStatus.networkId, src->callStatus.callCount);
    NLSTK_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, ret, "[CCP] copy network id fail");
    ret = CcpCopyMemoryField(&dest->callStatus.callStatus, dest->callStatus.callCount,
        src->callStatus.callStatus, src->callStatus.callCount);
    NLSTK_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, ret, "[CCP] copy call status fail");
    ret = CcpCopyMemoryField(&dest->callStatus.callFlag, dest->callStatus.callCount,
        src->callStatus.callFlag, src->callStatus.callCount);
    NLSTK_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, ret, "[CCP] copy call flag fail");
    return NLSTK_ERRCODE_SUCCESS;
}

void CcpFreeCallControlInfo(void *ptr)
{
    NLSTK_CcpCallControlInfo_S *baseInfo = (NLSTK_CcpCallControlInfo_S *)ptr;
    if (baseInfo == NULL) {
        return;
    }
    if (baseInfo->instanceName.data != NULL) {
        SDF_MemFree(baseInfo->instanceName.data);
    }
    if (baseInfo->instanceIcon.icon != NULL) {
        SDF_MemFree(baseInfo->instanceIcon.icon);
    }
    if (baseInfo->protocolSupport.data != NULL) {
        SDF_MemFree(baseInfo->protocolSupport.data);
    }
    if (baseInfo->callInOutInfo.userInfo.data != NULL) {
        SDF_MemFree(baseInfo->callInOutInfo.userInfo.data);
    }
    if (baseInfo->callInOutInfo.userAlias.data != NULL) {
        SDF_MemFree(baseInfo->callInOutInfo.userAlias.data);
    }
    if (baseInfo->callStatus.callId != NULL) {
        SDF_MemFree(baseInfo->callStatus.callId);
    }
    if (baseInfo->callStatus.networkId != NULL) {
        SDF_MemFree(baseInfo->callStatus.networkId);
    }
    if (baseInfo->callStatus.callStatus != NULL) {
        SDF_MemFree(baseInfo->callStatus.callStatus);
    }
    if (baseInfo->callStatus.callFlag != NULL) {
        SDF_MemFree(baseInfo->callStatus.callFlag);
    }
    if (baseInfo->networkSelection.data != NULL) {
        SDF_MemFree(baseInfo->networkSelection.data);
    }
    SDF_MemFree(baseInfo);
}

void CcpFreeUpdateCcsPropertyParam(void *ptr)
{
    CcpUpdateCcsPropertyParam_S *param = (CcpUpdateCcsPropertyParam_S *)ptr;
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

static NLSTK_VariableData_S *CcpValueConvert(void *data, uint16_t len)
{
    NLSTK_CHECK_RETURN(data != NULL, NULL, "[CCP] data is null");
    NLSTK_VariableData_S *value = (NLSTK_VariableData_S *)SDF_MemZalloc(sizeof(NLSTK_VariableData_S));
    NLSTK_CHECK_RETURN(value != NULL, NULL, "[CCP] value malloc fail");
    value->len = len;
    value->data = (uint8_t *)SDF_MemZalloc(value->len);
    if (value->data == NULL) {
        NLSTK_LOG_ERROR("[CCP] value data malloc fail");
        SDF_MemFree(value);
        return NULL;
    }
    (void)memcpy_s(value->data, value->len, data, len);
    return value;
}

static NLSTK_VariableData_S *CcpCcsLengthValueConvert(void *data)
{
    NLSTK_VariableData_S *param = (NLSTK_VariableData_S *)data;
    NLSTK_CHECK_RETURN(param != NULL, NULL, "[CCP] param is null");
    if (param->len != 0) {
        NLSTK_CHECK_RETURN(param->data != NULL, NULL, "[CCP] param data is null");
    }
    NLSTK_CHECK_RETURN(param->len < CCP_CCS_MAX_DATA_LEN , NULL, "[CCP] param len is invalid");
    NLSTK_VariableData_S *value = (NLSTK_VariableData_S *)SDF_MemZalloc(sizeof(NLSTK_VariableData_S));
    NLSTK_CHECK_RETURN(value != NULL, NULL, "[CCP] value malloc fail");
    value->len = param->len;
    if (value->len == 0) {
        return value;
    }
    value->data = (uint8_t *)SDF_MemZalloc(value->len);
    if (value->data == NULL) {
        NLSTK_LOG_ERROR("[CCP] value data malloc fail");
        SDF_MemFree(value);
        return NULL;
    }
    (void)memcpy_s(value->data, value->len, param->data, param->len);
    return value;
}

static NLSTK_VariableData_S *CcpCcsInstanceIconConvert(void *data)
{
    NLSTK_CcpInstanceIcon_S *param = (NLSTK_CcpInstanceIcon_S *)data;
    NLSTK_CHECK_RETURN(param != NULL, NULL, "[CCP] param is null");
    if (param->len != 0) {
        NLSTK_CHECK_RETURN(param->icon != NULL, NULL, "[CCP] param icon is null");
    }
    NLSTK_CHECK_RETURN(param->len < CCP_CCS_MAX_DATA_LEN, NULL, "[CCP] param len is invalid");
    NLSTK_VariableData_S *value = (NLSTK_VariableData_S *)SDF_MemZalloc(sizeof(NLSTK_VariableData_S));
    NLSTK_CHECK_RETURN(value != NULL, NULL, "[CCP] value malloc fail");
    value->len = param->len + CCP_ICON_TYPE_LEN;
    value->data = (uint8_t *)SDF_MemZalloc(value->len);
    if (value->data == NULL) {
        NLSTK_LOG_ERROR("[CCP] value data malloc fail");
        SDF_MemFree(value);
        return NULL;
    }
    value->data[0] = param->type;
    (void)memcpy_s(value->data + CCP_ICON_TYPE_LEN, param->len, param->icon, param->len);
    return value;
}

static NLSTK_VariableData_S *CcpCcsCallInOutInfoConvert(void *data)
{
    NLSTK_CcpCallInOutInfo_S *param = (NLSTK_CcpCallInOutInfo_S *)data;
    NLSTK_CHECK_RETURN(param != NULL, NULL, "[CCP] param is null");
    if (param->userInfo.len != 0) {
        NLSTK_CHECK_RETURN(param->userInfo.data != NULL, NULL, "[CCP] param userInfo is null");
    }
    if (param->userAlias.len != 0) {
        NLSTK_CHECK_RETURN(param->userAlias.data != NULL, NULL, "[CCP] param userAlias is null");
    }
    NLSTK_CHECK_RETURN(param->userInfo.len <= UINT8_MAX && param->userAlias.len <= UINT8_MAX, NULL,
        "[CCP] userInfo or userAlias len error");
    NLSTK_VariableData_S *value = (NLSTK_VariableData_S *)SDF_MemZalloc(sizeof(NLSTK_VariableData_S));
    NLSTK_CHECK_RETURN(value != NULL, NULL, "[CCP] value malloc fail");
    if (CCP_CALLIN_OUT_LEN > (UINT16_MAX - param->userInfo.len) ||
        (CCP_CALLIN_OUT_LEN + param->userInfo.len) > (UINT16_MAX - param->userAlias.len)) {
        SDF_MemFree(value);
        return NULL;
    }
    value->len = CCP_CALLIN_OUT_LEN + param->userInfo.len + param->userAlias.len;
    value->data = (uint8_t *)SDF_MemZalloc(value->len);
    if (value->data == NULL) {
        NLSTK_LOG_ERROR("[CCP] value data malloc fail");
        SDF_MemFree(value);
        return NULL;
    }
    size_t index = 0;
    value->data[index++] = param->callId;
    value->data[index++] = param->networkId;
    value->data[index++] = param->callFlag;
    value->data[index++] = param->userInfo.len;
    (void)memcpy_s(value->data + index, param->userInfo.len, param->userInfo.data, param->userInfo.len);
    index += param->userInfo.len;
    value->data[index++] = param->userAlias.len;
    (void)memcpy_s(value->data + index, param->userAlias.len, param->userAlias.data, param->userAlias.len);
    return value;
}

static NLSTK_VariableData_S *CcpCcsCallStatusConvert(void *data)
{
    NLSTK_CcpCallStatues_S *param = (NLSTK_CcpCallStatues_S *)data;
    NLSTK_CHECK_RETURN(param != NULL, NULL, "[CCP] param is null");
    if (param->callCount != 0) {
        NLSTK_CHECK_RETURN(param->callId != NULL && param->networkId != NULL && param->callStatus != NULL &&
            param->callFlag != NULL, NULL, "[CCP] param data is null");
    }
    NLSTK_CHECK_RETURN(param->callCount <= CCP_CCS_MAX_CALL_COUNT, NULL, "[CCP] callCount is invalid");
    NLSTK_VariableData_S *value = (NLSTK_VariableData_S *)SDF_MemZalloc(sizeof(NLSTK_VariableData_S));
    NLSTK_CHECK_RETURN(value != NULL, NULL, "[CCP] value malloc fail");
    value->len = param->callCount * CCP_CALL_STATUS_LEN + CCP_CALL_COUNT_LEN;
    value->data = (uint8_t *)SDF_MemZalloc(value->len);
    if (value->data == NULL) {
        NLSTK_LOG_ERROR("[CCP] value data malloc fail");
        SDF_MemFree(value);
        return NULL;
    }
    NLSTK_LOG_INFO("[CCP] CcpCcsCallStatusConvert value->len=%u", value->len);
    value->data[0] = param->callCount;
    for (int i = 0; i < param->callCount; i++) {
        size_t offset = CCP_CALL_COUNT_LEN;
        value->data[i * CCP_CALL_STATUS_LEN + offset++] = param->callId[i];
        value->data[i * CCP_CALL_STATUS_LEN + offset++] = param->networkId[i];
        value->data[i * CCP_CALL_STATUS_LEN + offset++] = param->callStatus[i];
        value->data[i * CCP_CALL_STATUS_LEN + offset++] = param->callFlag[i];
    }
    return value;
}

NLSTK_VariableData_S *CcpCcsPropertyValueConvert(void *data, NLSTK_CcpCcsPropertyType_E type)
{
    NLSTK_CHECK_RETURN(data != NULL, NULL, "[CCP] data is null");
    switch (type) {
        case NLSTK_CCP_CCS_INSTANCE_NAME:
        case NLSTK_CCP_CCS_PROTOCOL_SUPPORT:
        case NLSTK_CCP_CCS_NETWORK_SELECTION:
            return CcpCcsLengthValueConvert(data);
        case NLSTK_CCP_CCS_INSTANCE_ICON:
            return CcpCcsInstanceIconConvert(data);
        case NLSTK_CCP_CCS_FEATURE_STATUS:
        case NLSTK_CCP_CCS_MEDIA_INSTANCE_ID:
            return CcpValueConvert(data, sizeof(uint8_t));
        case NLSTK_CCP_CCS_CALLIN_OUT_INFO:
            return CcpCcsCallInOutInfoConvert(data);
        case NLSTK_CCP_CCS_CALL_STATUS:
            return CcpCcsCallStatusConvert(data);
        case NLSTK_CCP_CCS_CALL_TERMINATION:
            return CcpValueConvert(data, sizeof(NLSTK_CcpCallTermination_S));
        case NLSTK_CCP_CCS_CALL_REQ_SUPPORT:
            return CcpValueConvert(data, sizeof(uint16_t));
        default:
            return NULL;
    }
}