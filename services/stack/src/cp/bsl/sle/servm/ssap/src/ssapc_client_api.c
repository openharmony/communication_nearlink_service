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
#include "cpfwk_log.h"
#include "ssapc_client.h"
#include "ssap_manager.h"
#include "ssapc_client_api.h"

void SSAP_ExchangeInfoReq(int32_t appId, int64_t timeout, SsapTaskAppCallback appCallback,
    SSAP_ExchangeInfoReqInfo_S *exchangeInfoReqInfo)
{
    CP_CHECK_LOG_RETURN_VOID(exchangeInfoReqInfo != NULL, "[SSAP] SSAP_ExchangeInfoReq arg is null");
    SSAP_Link_S *link = SSAP_FindSsapLinkByAddr(&exchangeInfoReqInfo->addr);
    CP_CHECK_LOG_RETURN_VOID(link != NULL, "[SSAP] SSAP_ExchangeInfoReq link is null");
    SSAP_ExchangeInfoReqInfo_S *copyParam =
        (SSAP_ExchangeInfoReqInfo_S *)SDF_MemZalloc(sizeof(SSAP_ExchangeInfoReqInfo_S));
    CP_CHECK_LOG_RETURN_VOID(copyParam != NULL, "[SSAP] SSAP_ExchangeInfoReq malloc is null");
    (void)memcpy_s(copyParam, sizeof(SSAP_ExchangeInfoReqInfo_S), exchangeInfoReqInfo,
        sizeof(SSAP_ExchangeInfoReqInfo_S));
    SSAP_TaskParam_S taskParam = {.appId = appId, .arg = copyParam, .freeFunc = SDF_MemFree,
        .func = SSAPC_ExchangeInfoReq, .timeout = timeout, .valid = true, .appCallback = appCallback};
    SSAP_ProcessRequestTask(link, &taskParam, false);
}

void SSAP_FindReq(int32_t appId, int64_t timeout, SsapTaskAppCallback appCallback, SSAP_ParamFind_S *findParam)
{
    CP_CHECK_LOG_RETURN_VOID(findParam != NULL, "[SSAP] SSAP_FindReq arg is null");
    SSAP_Link_S *link = SSAP_FindSsapLinkByAddr(&findParam->addr);
    CP_CHECK_LOG_RETURN_VOID(link != NULL, "[SSAP] SSAP_FindReq link is null");
    SSAP_ParamFind_S *copyParam = (SSAP_ParamFind_S *)SDF_MemZalloc(sizeof(SSAP_ParamFind_S));
    CP_CHECK_LOG_RETURN_VOID(copyParam != NULL, "[SSAP] SSAP_FindReq malloc is null");
    (void)memcpy_s(copyParam, sizeof(SSAP_ParamFind_S), findParam, sizeof(SSAP_ParamFind_S));
    SSAP_TaskParam_S taskParam = {.appId = appId, .arg = copyParam, .freeFunc = SDF_MemFree, .func = SSAPC_FindReq,
        .timeout = timeout, .valid = true, .appCallback = appCallback};
    SSAP_ProcessRequestTask(link, &taskParam, false);
}

void SSAP_FindPriorityReq(int32_t appId, int64_t timeout, SsapTaskAppCallback appCallback, SSAP_ParamFind_S *findParam)
{
    CP_CHECK_LOG_RETURN_VOID(findParam != NULL, "[SSAP] SSAP_FindPriorityReq arg is null");
    SSAP_Link_S *link = SSAP_FindSsapLinkByAddr(&findParam->addr);
    CP_CHECK_LOG_RETURN_VOID(link != NULL, "[SSAP] SSAP_FindPriorityReq link is null");
    SSAP_ParamFind_S *copyParam = (SSAP_ParamFind_S *)SDF_MemZalloc(sizeof(SSAP_ParamFind_S));
    CP_CHECK_LOG_RETURN_VOID(copyParam != NULL, "[SSAP] SSAP_FindPriorityReq malloc is null");
    (void)memcpy_s(copyParam, sizeof(SSAP_ParamFind_S), findParam, sizeof(SSAP_ParamFind_S));
    SSAP_TaskParam_S taskParam = {.appId = appId, .arg = copyParam, .freeFunc = SDF_MemFree, .func = SSAPC_FindReq,
        .timeout = timeout, .valid = true, .appCallback = appCallback};
    SSAP_ProcessHighPriorityRequestTask(link, &taskParam, false);
}

void SSAP_FindByUuidReq(int32_t appId, int64_t timeout, SsapTaskAppCallback appCallback,
    SSAP_ParamFindByUuid_S *findParam)
{
    CP_CHECK_LOG_RETURN_VOID(findParam != NULL, "[SSAP] SSAP_FindByUuidReq arg is null");
    SSAP_Link_S *link = SSAP_FindSsapLinkByAddr(&findParam->addr);
    CP_CHECK_LOG_RETURN_VOID(link != NULL, "[SSAP] SSAP_FindReq link is null");
    SSAP_ParamFindByUuid_S *copyParam = (SSAP_ParamFindByUuid_S *)SDF_MemZalloc(sizeof(SSAP_ParamFindByUuid_S));
    CP_CHECK_LOG_RETURN_VOID(copyParam != NULL, "[SSAP] SSAP_FindReq malloc is null");
    (void)memcpy_s(copyParam, sizeof(SSAP_ParamFindByUuid_S), findParam, sizeof(SSAP_ParamFindByUuid_S));
    SSAP_TaskParam_S taskParam = {.appId = appId, .arg = copyParam, .freeFunc = SDF_MemFree,
        .func = SSAPC_FindByUuidReq, .timeout = timeout, .valid = true, .appCallback = appCallback};
    SSAP_ProcessRequestTask(link, &taskParam, false);
}

void SSAP_FindByUuidPriorityReq(int32_t appId, int64_t timeout, SsapTaskAppCallback appCallback,
    SSAP_ParamFindByUuid_S *findParam)
{
    CP_CHECK_LOG_RETURN_VOID(findParam != NULL, "[SSAP] SSAP_FindByUuidReq arg is null");
    SSAP_Link_S *link = SSAP_FindSsapLinkByAddr(&findParam->addr);
    CP_CHECK_LOG_RETURN_VOID(link != NULL, "[SSAP] SSAP_FindByUuidReq link is null");
    SSAP_ParamFindByUuid_S *copyParam = (SSAP_ParamFindByUuid_S *)SDF_MemZalloc(sizeof(SSAP_ParamFindByUuid_S));
    CP_CHECK_LOG_RETURN_VOID(copyParam != NULL, "[SSAP] SSAP_FindByUuidReq malloc is null");
    (void)memcpy_s(copyParam, sizeof(SSAP_ParamFindByUuid_S), findParam, sizeof(SSAP_ParamFindByUuid_S));
    SSAP_TaskParam_S taskParam = {.appId = appId, .arg = copyParam, .freeFunc = SDF_MemFree,
        .func = SSAPC_FindByUuidReq, .timeout = timeout, .valid = true, .appCallback = appCallback};
    SSAP_ProcessHighPriorityRequestTask(link, &taskParam, false);
}

/**
 * @brief  客户端通过句柄和数据类型读取服务端数据
 */
void SSAP_ReadReq(int32_t appId, int64_t timeout, SsapTaskAppCallback appCallback, SSAP_ReadReqInfo_S *readReqInfo)
{
    CP_CHECK_LOG_RETURN_VOID(readReqInfo != NULL, "[SSAP] SSAP_ReadReq arg is null");
    SSAP_Link_S *link = SSAP_FindSsapLinkByAddr(&readReqInfo->addr);
    CP_CHECK_LOG_RETURN_VOID(link != NULL, "[SSAP] SSAP_ReadReq link is null");
    SSAP_ReadReqInfo_S *copyParam = (SSAP_ReadReqInfo_S *)SDF_MemZalloc(sizeof(SSAP_ReadReqInfo_S));
    CP_CHECK_LOG_RETURN_VOID(copyParam != NULL, "[SSAP] SSAP_ReadReq malloc is null");
    (void)memcpy_s(copyParam, sizeof(SSAP_ReadReqInfo_S), readReqInfo, sizeof(SSAP_ReadReqInfo_S));
    SSAP_TaskParam_S taskParam = {.appId = appId, .arg = copyParam, .freeFunc = SDF_MemFree, .func = SSAPC_ReadReq,
        .timeout = timeout, .valid = true, .appCallback = appCallback};
    SSAP_ProcessRequestTask(link, &taskParam, false);
}

/**
 * @brief  客户端通过句柄和数据类型读取多个服务端数据
 */
void SSAP_ReadReqProps(int32_t appId, int64_t timeout, SsapTaskAppCallback appCallback,
    SSAP_ReadPropsInfo_S *readPropsInfo)
{
    CP_CHECK_LOG_RETURN_VOID(readPropsInfo != NULL, "[SSAP] SSAP_ReadReqProps arg is null");
    SSAP_Link_S *link = SSAP_FindSsapLinkByAddr(&readPropsInfo->addr);
    CP_CHECK_LOG_RETURN_VOID(link != NULL, "[SSAP] SSAP_ReadReqProps link is null");
    SSAP_ReadPropsInfo_S *copyParam = (SSAP_ReadPropsInfo_S *)SDF_MemZalloc(sizeof(SSAP_ReadPropsInfo_S) +
        readPropsInfo->num * sizeof(uint16_t));
    CP_CHECK_LOG_RETURN_VOID(copyParam != NULL, "[SSAP] SSAP_ReadReqProps malloc fail");
    (void)memcpy_s(copyParam, sizeof(SSAP_ReadPropsInfo_S) + readPropsInfo->num * sizeof(uint16_t), readPropsInfo,
        sizeof(SSAP_ReadPropsInfo_S) + readPropsInfo->num * sizeof(uint16_t));
    SSAP_TaskParam_S taskParam = {.appId = appId, .arg = copyParam, .freeFunc = SDF_MemFree, .func = SSAPC_ReadProps,
        .timeout = timeout, .valid = true, .appCallback = appCallback};
    SSAP_ProcessRequestTask(link, &taskParam, false);
}

/**
 * @brief  客户端通过UUID和数据类型读取服务端数据
 */
void SSAP_ReadByUuidReq(int32_t appId, int64_t timeout, SsapTaskAppCallback appCallback,
    SSAP_ReadByUuidReqInfo_S *readByUuidReqInfo)
{
    CP_CHECK_LOG_RETURN_VOID(readByUuidReqInfo != NULL, "[SSAP] SSAP_ReadByUuidReq arg is null");
    SSAP_Link_S *link = SSAP_FindSsapLinkByAddr(&readByUuidReqInfo->addr);
    CP_CHECK_LOG_RETURN_VOID(link != NULL, "[SSAP] SSAP_ReadByUuidReq link is null");
    SSAP_ReadByUuidReqInfo_S *copyParam = (SSAP_ReadByUuidReqInfo_S *)SDF_MemZalloc(sizeof(SSAP_ReadByUuidReqInfo_S));
    CP_CHECK_LOG_RETURN_VOID(copyParam != NULL, "[SSAP] SSAP_ReadByUuidReq malloc is null");
    (void)memcpy_s(copyParam, sizeof(SSAP_ReadByUuidReqInfo_S), readByUuidReqInfo, sizeof(SSAP_ReadByUuidReqInfo_S));
    SSAP_TaskParam_S taskParam = {.appId = appId, .arg = copyParam, .freeFunc = SDF_MemFree,
        .func = SSAPC_ReadByUuidReq, .timeout = timeout, .valid = true, .appCallback = appCallback};
    SSAP_ProcessRequestTask(link, &taskParam, false);
}

/**
 * @brief  发送无需响应的写入报文
 */
void SSAP_WriteCmd(SSAP_WriteCmdInfo_S *writeCmdInfo)
{
    CP_CHECK_LOG_RETURN_VOID(writeCmdInfo != NULL, "[SSAP] SSAP_WriteCmd arg is null");
    SSAP_Link_S *link = SSAP_FindSsapLinkByAddr(&writeCmdInfo->addr);
    CP_CHECK_LOG_RETURN_VOID(link != NULL, "[SSAP] SSAP_WriteCmd link is null");
    uint32_t size = sizeof(SSAP_WriteCmdInfo_S) + writeCmdInfo->value.len;
    SSAP_WriteCmdInfo_S *copyParam = (SSAP_WriteCmdInfo_S *)SDF_MemZalloc(size);
    CP_CHECK_LOG_RETURN_VOID(copyParam != NULL, "[SSAP] SSAP_WriteCmd malloc is null");
    (void)memcpy_s(copyParam, size, writeCmdInfo, size);
    SSAP_ProcessNormalTask(link, SSAPC_WriteCmd, copyParam, SDF_MemFree);
}

/**
 * @brief  客户端有响应写入服务端数据
 */
void SSAP_WriteReq(int32_t appId, int64_t timeout, SsapTaskAppCallback appCallback, SSAP_WriteReqInfo_S *writeReqInfo)
{
    CP_CHECK_LOG_RETURN_VOID(writeReqInfo != NULL, "[SSAP] SSAP_WriteReq arg is null");
    SSAP_Link_S *link = SSAP_FindSsapLinkByAddr(&writeReqInfo->addr);
    CP_CHECK_LOG_RETURN_VOID(link != NULL, "[SSAP] SSAP_WriteReq link is null");
    CP_CHECK_LOG_RETURN_VOID(
        writeReqInfo->value.len <= SSAP_MAX_VALUE_LENTH, "[SSAP] SSAP_WriteReq value len over max");
    SSAP_WriteReqInfo_S *copyParam = (SSAP_WriteReqInfo_S *)SDF_MemZalloc(sizeof(SSAP_WriteReqInfo_S) +
        writeReqInfo->value.len);
    CP_CHECK_LOG_RETURN_VOID(copyParam != NULL, "[SSAP] SSAP_WriteReq malloc is null");
    uint32_t size = sizeof(SSAP_WriteReqInfo_S) + writeReqInfo->value.len;
    (void)memcpy_s(copyParam, size, writeReqInfo, size);
    SSAP_TaskParam_S taskParam = {.appId = appId, .arg = copyParam, .freeFunc = SDF_MemFree, .func = SSAPC_WriteReq,
        .timeout = timeout, .valid = true, .appCallback = appCallback};
    SSAP_ProcessRequestTask(link, &taskParam, false);
}

/**
 * @brief  数据指示报文确认
 */
void SSAP_ValueAck(SSAP_ValueAckInfo_S *valueAck)
{
    CP_CHECK_LOG_RETURN_VOID(valueAck != NULL, "[SSAP] SSAP_ValueAck arg is null");
    SSAP_Link_S *link = SSAP_FindSsapLinkByAddr(&valueAck->addr);
    CP_CHECK_LOG_RETURN_VOID(link != NULL, "[SSAP] SSAP_ValueAck link is null");
    SSAP_ValueAckInfo_S *copyParam = (SSAP_ValueAckInfo_S *)SDF_MemZalloc(sizeof(SSAP_ValueAckInfo_S) +
        valueAck->value.len);
    CP_CHECK_LOG_RETURN_VOID(copyParam != NULL, "[SSAP] SSAP_ValueAck malloc is null");
    uint32_t size = sizeof(SSAP_ValueAckInfo_S) + valueAck->value.len;
    (void)memcpy_s(copyParam, size, valueAck, size);
    SSAP_ProcessNormalTask(link, SSAPC_ValueAck, copyParam, SDF_MemFree);
}

/**
 * @brief  请求调用服务端方法(有响应)
 */
void SSAP_CallMethodReq(int32_t appId, int64_t timeout, SsapTaskAppCallback appCallback,
    SSAP_CallMethodReqInfo_S *callMethodInfo)
{
    CP_CHECK_LOG_RETURN_VOID(callMethodInfo != NULL, "SSAP_CallMethodReq arg is null");
    SSAP_Link_S *link = SSAP_FindSsapLinkByAddr(&callMethodInfo->addr);
    CP_CHECK_LOG_RETURN_VOID(link != NULL, "SSAP_CallMethodReq link is null");
    SSAP_CallMethodReqInfo_S *copyParam = (SSAP_CallMethodReqInfo_S *)SDF_MemZalloc(sizeof(SSAP_CallMethodReqInfo_S) +
        callMethodInfo->value.len);
    CP_CHECK_LOG_RETURN_VOID(copyParam != NULL, "SSAP_CallMethodReq malloc is null");
    (void)memcpy_s(copyParam, sizeof(SSAP_CallMethodReqInfo_S) + callMethodInfo->value.len, callMethodInfo,
        sizeof(SSAP_CallMethodReqInfo_S) + callMethodInfo->value.len);
    SSAP_TaskParam_S taskParam = {.appId = appId, .arg = copyParam, .freeFunc = SDF_MemFree,
        .func = SSAPC_CallMethodReq, .timeout = timeout, .valid = true, .appCallback = appCallback};
    SSAP_ProcessRequestTask(link, &taskParam, false);
}

/**
 * @brief  请求调用服务端方法(无响应)
 */
void SSAP_CallMethodCmd(SSAP_CallMethodCmdInfo_S *callMethodInfo)
{
    CP_CHECK_LOG_RETURN_VOID(callMethodInfo != NULL, "SSAP_CallMethodCmd arg is null");
    SSAP_Link_S *link = SSAP_FindSsapLinkByAddr(&callMethodInfo->addr);
    CP_CHECK_LOG_RETURN_VOID(link != NULL, "SSAP_CallMethodCmd link is null");
    SSAP_CallMethodCmdInfo_S *copyParam = (SSAP_CallMethodCmdInfo_S *)SDF_MemZalloc(sizeof(SSAP_CallMethodCmdInfo_S) +
        callMethodInfo->value.len);
    CP_CHECK_LOG_RETURN_VOID(copyParam != NULL, "SSAP_CallMethodCmd malloc is null");
    (void)memcpy_s(copyParam, sizeof(SSAP_CallMethodCmdInfo_S) + callMethodInfo->value.len, callMethodInfo,
        sizeof(SSAP_CallMethodCmdInfo_S) + callMethodInfo->value.len);
    SSAP_ProcessNormalTask(link, SSAPC_CallMethodCmd, copyParam, SDF_MemFree);
}