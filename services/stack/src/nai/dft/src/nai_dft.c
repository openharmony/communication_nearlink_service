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
#include <stdint.h>
#include <time.h>

#include "nai_dft.h"
#include "cpfwk_log.h"
#include "securec.h"

void DftGetTimestamp(char *timestamp, uint16_t timelen)
{
    CP_CHECK_LOG_RETURN_VOID(timestamp != NULL && timelen > 0, "[DFT] timestamp is null");
    time_t rawtime;
    struct tm timeinfo;
    (void)time(&rawtime);
    if (localtime_r(&rawtime, &timeinfo) == NULL) {
        CP_LOG_ERROR("[DFT] localtime_r failed");
        return;
    }
    
    (void)memset_s(timestamp, timelen, 0, timelen);
    size_t size = strftime(timestamp, timelen, "%Y-%m-%d:%H:%M:%S", &timeinfo);
    if (size == 0) {
        CP_LOG_ERROR("[DFT] strftime failed");
        return;
    }
}

struct SDF_EncryptedLogString GetAddr(const SLE_Addr_S *addr)
{
    struct SDF_EncryptedLogString res = {0};
    (void)sprintf_s(res.buf, SDF_ENC_LOG_STR_LEN, "%02X:%02X:%02X:%02X:%02X:%02X(%u)",
        addr->addr[0], addr->addr[1], addr->addr[2],                // 0 1 2 for macaddr index
        addr->addr[3], addr->addr[4], addr->addr[5], addr->type);   // 3 4 5 for macaddr index
    return res;
}

DftParamC CreateUi16ParamC(uint16_t paramId, uint16_t value)
{
    DftParamC p = {paramId, DFT_UINT16};
    p.u.v.ui16 = value;
    return p;
}

DftParamC CreateStrParamC(int paramId, uint16_t size, const char *str)
{
    DftParamC p = {paramId, DFT_STRING};
    CP_CHECK_LOG_RETURN(str != NULL, p, "CreateStrParamC str is null");
    p.u.v.s = (char *)SDF_MemZalloc(size + 1);
    if (p.u.v.s == NULL) {
        CP_LOG_ERROR("SDF_MemZalloc failed");
        return p;
    }
    memcpy_s(p.u.v.s, size, str, size);
    p.u.v.s[size] = '\0';
    return p;
}

DftParamC CreateRefParamC(uint16_t paramId, DftSubEventRefC *ref)
{
    DftParamC p = {paramId, DFT_SUB_REF};
    CP_CHECK_LOG_RETURN(ref != NULL, p, "CreateRefParamC ref is null");
    p.u.ref = ref;
    return p;
}

DftSubEventRefC *CreateSubEventRef(uint16_t paramId, DftParamC params[], size_t size)
{
    DftSubEventRefC *subEventRef = (DftSubEventRefC *)SDF_MemZalloc(sizeof(DftSubEventRefC));
    CP_CHECK_LOG_RETURN(subEventRef != NULL, NULL, "DftSubEventRefC create failed");
    subEventRef->eventId = paramId;
    subEventRef->params = params;
    subEventRef->size = size;
    return subEventRef;
}

void DftFreeBasicTypeParams(DftParamC *params, size_t size)
{
    CP_CHECK_LOG_RETURN_VOID(params != NULL, "DftFreeBasicTypeParams params is null");
    for (size_t i = 0; i < size; i++) {
        if (params[i].t == DFT_STRING) {
            SDF_MemFree(params[i].u.v.s);
        }
    }
    SDF_MemFree(params);
}

void DftFreeParamsWithSubParam(DftParamC *params, size_t size)
{
    CP_CHECK_LOG_RETURN_VOID(params != NULL, "DftFreeParamsWithSubParam params is null");
    for (size_t i = 0; i < size; i++) {
        if (params[i].t == DFT_SUB_REF && params[i].u.ref != NULL) {
            DftFreeBasicTypeParams(params[i].u.ref->params, params[i].u.ref->size);
            SDF_MemFree(params[i].u.ref);
        } else if (params[i].t == DFT_STRING) {
            SDF_MemFree(params[i].u.v.s);
        }
    }
    SDF_MemFree(params);
}

void NAI_DftCacheUint16(SLE_Addr_S *addr, uint16_t eventId, uint16_t paramId, uint16_t paramVal)
{
    CP_CHECK_LOG_RETURN_VOID(addr != NULL, "[DFT] DftParamC create failed");

    DftParamC *params = (DftParamC *)SDF_MemZalloc(NAI_DFT_PARAM_SIZE_2 * sizeof(DftParamC));
    CP_CHECK_LOG_RETURN_VOID(params != NULL, "DftParamC create failed");

    const char *inputAddr = DFT_GET_ADDR(addr);
    params[PARAM_INDEX_0] = CreateStrParamC(NAI_DEVICE_ADDR, DFT_ADDR_SIZE, inputAddr);
    params[PARAM_INDEX_1] = CreateUi16ParamC(paramId, paramVal);

    DftManagerCache((DftEventEnum)eventId, params, NAI_DFT_PARAM_SIZE_2);
    DftFreeBasicTypeParams(params, NAI_DFT_PARAM_SIZE_2);
}

void NAI_DftCacheStr(SLE_Addr_S *addr, uint16_t eventId, uint16_t paramId, void *param)
{
    CP_CHECK_LOG_RETURN_VOID(addr != NULL && param != NULL, "[DFT] DftParamC create failed");
    DftParamC *params = (DftParamC *)SDF_MemZalloc(NAI_DFT_PARAM_SIZE_2 * sizeof(DftParamC));
    CP_CHECK_LOG_RETURN_VOID(params != NULL, "DftParamC create failed");

    const char *inputAddr = DFT_GET_ADDR(addr);
    params[PARAM_INDEX_0] = CreateStrParamC(NAI_DEVICE_ADDR, DFT_ADDR_SIZE, inputAddr);

    NLSTK_DftParamValueString_S *inParam = (NLSTK_DftParamValueString_S *)param;
    params[PARAM_INDEX_1] = CreateStrParamC(paramId, DFT_STRING_LEN, inParam->str);

    DftManagerCache((DftEventEnum)eventId, params, NAI_DFT_PARAM_SIZE_2);
    DftFreeBasicTypeParams(params, NAI_DFT_PARAM_SIZE_2);
}

void NAI_DftCache(SLE_Addr_S *addr, uint16_t eventId, uint16_t paramId, NLSTK_DftParamValueType_E paramType,
    void *param)
{
    CP_LOG_DEBUG("[DFT] NAI_DftCache, eventId: %u, paramId: %u", eventId, paramId);
    switch (paramType) {
        case NLSTK_DFT_PARAM_VALUE_TYPE_UINT16:
            {
                uint16_t *paramPtr = (uint16_t *)param;
                CP_CHECK_LOG_RETURN_VOID(paramPtr != NULL, "[DFT] cache uint16 param failed, since param is null");
                uint16_t paramVal = *paramPtr;
                NAI_DftCacheUint16(addr, eventId, paramId, paramVal);
            }
            break;
        case NLSTK_DFT_PARAM_VALUE_TYPE_STRING:
            {
                NAI_DftCacheStr(addr, eventId, paramId, param);
            }
            break;
        default:
            CP_LOG_DEBUG("[DFT] Unknown param type");
    }
}

void NAI_DftReport(SLE_Addr_S *addr, uint16_t eventId, uint16_t paramId, uint16_t res)
{
    CP_LOG_DEBUG("[DFT] NAI_DftReport, eventId: %u, paramId: %u, res: %u", eventId, paramId, res);
    CP_CHECK_LOG_RETURN_VOID(addr != NULL, "[DFT] DftParamC create failed");
    DftParamC *params = (DftParamC *)SDF_MemZalloc(NAI_DFT_PARAM_SIZE_3 * sizeof(DftParamC));
    CP_CHECK_LOG_RETURN_VOID(params != NULL, "DftParamC create failed");

    const char *inputAddr = DFT_GET_ADDR(addr);
    params[PARAM_INDEX_0] = CreateStrParamC(SM_DEVICE_ADDR, DFT_ADDR_SIZE, inputAddr);
    params[PARAM_INDEX_1] = CreateUi16ParamC(paramId, res);

    // 组装peerInfo
    DftParamC *peerParams = (DftParamC *)SDF_MemZalloc(NAI_DFT_PARAM_SIZE_2 * sizeof(DftParamC));
    if (peerParams == NULL) {
        DftFreeParamsWithSubParam(params, NAI_DFT_PARAM_SIZE_2);
        return;
    }

    peerParams[PARAM_INDEX_0] = CreateStrParamC(PEER_INFO_ADDR, DFT_ADDR_SIZE, inputAddr);
    peerParams[PARAM_INDEX_1] = CreateUi16ParamC(PEER_INFO_TYPE, NL_TRANS_SLE);
    DftSubEventRefC *peerInfoRef = CreateSubEventRef(DFT_PEER_INFO, peerParams, DFT_PEER_INFO_PARAMS_SIZE);
    if (peerInfoRef == NULL) {
        DftFreeParamsWithSubParam(peerParams, NAI_DFT_PARAM_SIZE_2);
        DftFreeParamsWithSubParam(params, NAI_DFT_PARAM_SIZE_2);
        return;
    }
    params[PARAM_INDEX_2] = CreateRefParamC(SM_ENCP_PEER_INFO, peerInfoRef);
    DftManagerReport((DftEventEnum)eventId, params, NAI_DFT_PARAM_SIZE_3);
    DftFreeParamsWithSubParam(params, NAI_DFT_PARAM_SIZE_3);
}

uint32_t NAI_DftInit(void)
{
    CP_LOG_DEBUG("NAI_DftInit begin!");
    NLSTK_DftCallback_S dftCb = {0};
    dftCb.cacheCb = NAI_DftCache;
    dftCb.reportCb = NAI_DftReport;
    return NLSTK_DftRegisterCallback(&dftCb);
}