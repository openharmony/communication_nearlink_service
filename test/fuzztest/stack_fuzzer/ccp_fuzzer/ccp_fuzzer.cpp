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

#include "ccp_fuzzer.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "nlstk_ccp_ccs_server.h"
#include "nlstk_ccp_vas_server.h"
#include "nai_log.h"
#include "nlstk_init_api.h"
#include "sdf_evc.h"
#include "sdf_mem.h"
#include "sdf_thread.h"
#include "securec.h"

#define FUZZ_CCP_VAR_DATA_MAX_LEN 16
#define FUZZ_CCP_MAX_CALL_CNT 8

typedef struct SlePkt {
    uint8_t *data;
    uint32_t size;
} SlePkt;

int SleSendDliPacket(const SlePkt *packet)
{
    (void)packet;
    return 0;
}

namespace OHOS {

static void StartCcsInstImpl(int32_t instanceId, NLSTK_Errcode_E ret)
{
    (void)instanceId;
    (void)ret;
}

static void CcsAuthorizeImpl(uint32_t requestId, int32_t instanceId, NLSTK_CcpCcsPropertyType_E property,
    NLSTK_ServicePropertyOpType_E operation)
{
    (void)requestId;
    (void)instanceId;
    (void)property;
    (void)operation;
}

static void VasStartServiceImpl(uint32_t errorCode)
{
    (void)errorCode;
}

static void VasStateAuthorizeImpl(SLE_Addr_S *addr, uint32_t requestId)
{
    (void)addr;
    (void)requestId;
}

static uint16_t FillBytes(FuzzedDataProvider &fdp, uint8_t *buf, uint16_t maxLen)
{
    uint16_t len = fdp.ConsumeIntegral<uint8_t>() % maxLen;
    for (uint16_t i = 0; i < len; i++) {
        buf[i] = fdp.ConsumeIntegral<uint8_t>();
    }
    return len;
}

static void FillVariableData(FuzzedDataProvider &fdp, NLSTK_VariableData_S &var, uint8_t *buf)
{
    var.len = FillBytes(fdp, buf, FUZZ_CCP_VAR_DATA_MAX_LEN);
    var.data = buf;
}

void FuzzCcpCreateCcsInstance(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    NLSTK_CcpCallControlInfo_S info;
    (void)memset_s(&info, sizeof(info), 0, sizeof(info));
    uint8_t nameBuf[FUZZ_CCP_VAR_DATA_MAX_LEN] = {0};
    uint8_t protoBuf[FUZZ_CCP_VAR_DATA_MAX_LEN] = {0};
    FillVariableData(fdp, info.instanceName, nameBuf);
    info.featureStatus = fdp.ConsumeIntegral<uint8_t>();
    FillVariableData(fdp, info.protocolSupport, protoBuf);
    info.mediaInstanceId = fdp.ConsumeIntegral<uint8_t>();
    info.callRequestSupport = fdp.ConsumeIntegral<uint16_t>();
    info.instanceIconFlag = false;
    info.networkSelectionFlag = false;
    info.startCcsInst = StartCcsInstImpl;
    info.authorize = CcsAuthorizeImpl;
    for (int i = 0; i < NLSTK_CCP_CCS_MAX_PROPERTY; i++) {
        info.propertyRights[i] = fdp.ConsumeIntegral<uint8_t>();
    }
    (void)NLSTK_CcpCreateCcsInstance(&info);
}

void FuzzCcpCallControlResult(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    uint32_t requestId = fdp.ConsumeIntegral<uint32_t>();
    int32_t instanceId = fdp.ConsumeIntegral<int32_t>();
    uint8_t errorCode = fdp.ConsumeIntegral<uint8_t>();
    NLSTK_CcpCallControlResult(requestId, instanceId, errorCode);
}

void FuzzCcpCcsAuthorizeResult(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    uint32_t requestId = fdp.ConsumeIntegral<uint32_t>();
    int32_t instanceId = fdp.ConsumeIntegral<int32_t>();
    NLSTK_CcpCcsPropertyType_E property = static_cast<NLSTK_CcpCcsPropertyType_E>(
        fdp.ConsumeIntegral<uint8_t>() % NLSTK_CCP_CCS_MAX_PROPERTY);
    uint8_t errorCode = fdp.ConsumeIntegral<uint8_t>();
    (void)NLSTK_CcpCcsAuthorizeResult(requestId, instanceId, property, errorCode);
}

void FuzzCcpUpdateCcsProperty(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    int32_t instanceId = fdp.ConsumeIntegral<int32_t>();
    NLSTK_CcpCcsPropertyType_E property = static_cast<NLSTK_CcpCcsPropertyType_E>(
        fdp.ConsumeIntegral<uint8_t>() % NLSTK_CCP_CCS_MAX_PROPERTY);
    uint8_t varBuf[FUZZ_CCP_VAR_DATA_MAX_LEN] = {0};
    uint8_t userBuf[FUZZ_CCP_VAR_DATA_MAX_LEN] = {0};
    uint8_t aliasBuf[FUZZ_CCP_VAR_DATA_MAX_LEN] = {0};
    uint8_t callIdBuf[FUZZ_CCP_MAX_CALL_CNT] = {0};
    uint8_t networkIdBuf[FUZZ_CCP_MAX_CALL_CNT] = {0};
    uint8_t callStatusBuf[FUZZ_CCP_MAX_CALL_CNT] = {0};
    uint8_t callFlagBuf[FUZZ_CCP_MAX_CALL_CNT] = {0};
    switch (property) {
        case NLSTK_CCP_CCS_INSTANCE_NAME:
        case NLSTK_CCP_CCS_PROTOCOL_SUPPORT:
        case NLSTK_CCP_CCS_NETWORK_SELECTION: {
            NLSTK_VariableData_S var = {0};
            FillVariableData(fdp, var, varBuf);
            (void)NLSTK_CcpUpdateCcsProperty(instanceId, property, &var);
            break;
        }
        case NLSTK_CCP_CCS_INSTANCE_ICON: {
            NLSTK_CcpInstanceIcon_S icon = {0};
            icon.type = fdp.ConsumeIntegral<uint8_t>();
            icon.len = FillBytes(fdp, varBuf, FUZZ_CCP_VAR_DATA_MAX_LEN);
            icon.icon = varBuf;
            (void)NLSTK_CcpUpdateCcsProperty(instanceId, property, &icon);
            break;
        }
        case NLSTK_CCP_CCS_FEATURE_STATUS:
        case NLSTK_CCP_CCS_MEDIA_INSTANCE_ID: {
            uint8_t value = fdp.ConsumeIntegral<uint8_t>();
            (void)NLSTK_CcpUpdateCcsProperty(instanceId, property, &value);
            break;
        }
        case NLSTK_CCP_CCS_CALLIN_OUT_INFO: {
            NLSTK_CcpCallInOutInfo_S info = {0};
            info.callId = fdp.ConsumeIntegral<uint8_t>();
            info.networkId = fdp.ConsumeIntegral<uint8_t>();
            info.callFlag = fdp.ConsumeIntegral<uint8_t>();
            info.userInfo.len = FillBytes(fdp, userBuf, FUZZ_CCP_VAR_DATA_MAX_LEN);
            info.userInfo.data = userBuf;
            info.userAlias.len = FillBytes(fdp, aliasBuf, FUZZ_CCP_VAR_DATA_MAX_LEN);
            info.userAlias.data = aliasBuf;
            (void)NLSTK_CcpUpdateCcsProperty(instanceId, property, &info);
            break;
        }
        case NLSTK_CCP_CCS_CALL_STATUS: {
            NLSTK_CcpCallStatues_S status = {0};
            status.callCount = fdp.ConsumeIntegral<uint8_t>() % FUZZ_CCP_MAX_CALL_CNT;
            status.callId = callIdBuf;
            status.networkId = networkIdBuf;
            status.callStatus = callStatusBuf;
            status.callFlag = callFlagBuf;
            for (uint8_t i = 0; i < status.callCount; i++) {
                callIdBuf[i] = fdp.ConsumeIntegral<uint8_t>();
                networkIdBuf[i] = fdp.ConsumeIntegral<uint8_t>();
                callStatusBuf[i] = fdp.ConsumeIntegral<uint8_t>();
                callFlagBuf[i] = fdp.ConsumeIntegral<uint8_t>();
            }
            (void)NLSTK_CcpUpdateCcsProperty(instanceId, property, &status);
            break;
        }
        case NLSTK_CCP_CCS_CALL_TERMINATION: {
            NLSTK_CcpCallTermination_S term = {0};
            term.callId = fdp.ConsumeIntegral<uint8_t>();
            term.terminateReason = fdp.ConsumeIntegral<uint8_t>();
            (void)NLSTK_CcpUpdateCcsProperty(instanceId, property, &term);
            break;
        }
        default:
            (void)NLSTK_CcpUpdateCcsProperty(instanceId, property, nullptr);
            break;
    }
}

/* 与 FuzzCcpUpdateCcsProperty 相同的属性载荷分支，但 property/instanceId 来自外部，
 * 保证与创建时的 instanceId 一致 */
void FuzzCcpUpdateCcsPropertyById(FuzzedDataProvider &fdp, int32_t instanceId)
{
    NLSTK_CcpCcsPropertyType_E property = static_cast<NLSTK_CcpCcsPropertyType_E>(
        fdp.ConsumeIntegral<uint8_t>() % NLSTK_CCP_CCS_MAX_PROPERTY);
    uint8_t varBuf[FUZZ_CCP_VAR_DATA_MAX_LEN] = {0};
    uint8_t userBuf[FUZZ_CCP_VAR_DATA_MAX_LEN] = {0};
    uint8_t aliasBuf[FUZZ_CCP_VAR_DATA_MAX_LEN] = {0};
    uint8_t callIdBuf[FUZZ_CCP_MAX_CALL_CNT] = {0};
    uint8_t networkIdBuf[FUZZ_CCP_MAX_CALL_CNT] = {0};
    uint8_t callStatusBuf[FUZZ_CCP_MAX_CALL_CNT] = {0};
    uint8_t callFlagBuf[FUZZ_CCP_MAX_CALL_CNT] = {0};
    switch (property) {
        case NLSTK_CCP_CCS_INSTANCE_NAME:
        case NLSTK_CCP_CCS_PROTOCOL_SUPPORT:
        case NLSTK_CCP_CCS_NETWORK_SELECTION: {
            NLSTK_VariableData_S var = {0};
            FillVariableData(fdp, var, varBuf);
            (void)NLSTK_CcpUpdateCcsProperty(instanceId, property, &var);
            break;
        }
        case NLSTK_CCP_CCS_INSTANCE_ICON: {
            NLSTK_CcpInstanceIcon_S icon = {0};
            icon.type = fdp.ConsumeIntegral<uint8_t>();
            icon.len = FillBytes(fdp, varBuf, FUZZ_CCP_VAR_DATA_MAX_LEN);
            icon.icon = varBuf;
            (void)NLSTK_CcpUpdateCcsProperty(instanceId, property, &icon);
            break;
        }
        case NLSTK_CCP_CCS_FEATURE_STATUS:
        case NLSTK_CCP_CCS_MEDIA_INSTANCE_ID: {
            uint8_t value = fdp.ConsumeIntegral<uint8_t>();
            (void)NLSTK_CcpUpdateCcsProperty(instanceId, property, &value);
            break;
        }
        case NLSTK_CCP_CCS_CALLIN_OUT_INFO: {
            NLSTK_CcpCallInOutInfo_S info = {0};
            info.callId = fdp.ConsumeIntegral<uint8_t>();
            info.networkId = fdp.ConsumeIntegral<uint8_t>();
            info.callFlag = fdp.ConsumeIntegral<uint8_t>();
            info.userInfo.len = FillBytes(fdp, userBuf, FUZZ_CCP_VAR_DATA_MAX_LEN);
            info.userInfo.data = userBuf;
            info.userAlias.len = FillBytes(fdp, aliasBuf, FUZZ_CCP_VAR_DATA_MAX_LEN);
            info.userAlias.data = aliasBuf;
            (void)NLSTK_CcpUpdateCcsProperty(instanceId, property, &info);
            break;
        }
        case NLSTK_CCP_CCS_CALL_STATUS: {
            NLSTK_CcpCallStatues_S status = {0};
            status.callCount = fdp.ConsumeIntegral<uint8_t>() % FUZZ_CCP_MAX_CALL_CNT;
            status.callId = callIdBuf;
            status.networkId = networkIdBuf;
            status.callStatus = callStatusBuf;
            status.callFlag = callFlagBuf;
            for (uint8_t i = 0; i < status.callCount; i++) {
                callIdBuf[i] = fdp.ConsumeIntegral<uint8_t>();
                networkIdBuf[i] = fdp.ConsumeIntegral<uint8_t>();
                callStatusBuf[i] = fdp.ConsumeIntegral<uint8_t>();
                callFlagBuf[i] = fdp.ConsumeIntegral<uint8_t>();
            }
            (void)NLSTK_CcpUpdateCcsProperty(instanceId, property, &status);
            break;
        }
        case NLSTK_CCP_CCS_CALL_TERMINATION: {
            NLSTK_CcpCallTermination_S term = {0};
            term.callId = fdp.ConsumeIntegral<uint8_t>();
            term.terminateReason = fdp.ConsumeIntegral<uint8_t>();
            (void)NLSTK_CcpUpdateCcsProperty(instanceId, property, &term);
            break;
        }
        default:
            (void)NLSTK_CcpUpdateCcsProperty(instanceId, property, nullptr);
            break;
    }
}

void FuzzCcpDeleteCcsInstance(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    int32_t instanceId = fdp.ConsumeIntegral<int32_t>();
    (void)NLSTK_CcpDeleteCcsInstance(instanceId);
}

void FuzzCcpCreateVas(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    NLSTK_CcpVasInfo_S vasInfo;
    (void)memset_s(&vasInfo, sizeof(vasInfo), 0, sizeof(vasInfo));
    vasInfo.state = fdp.ConsumeIntegral<uint8_t>();
    vasInfo.stateRight = fdp.ConsumeIntegral<uint8_t>();
    vasInfo.startService = VasStartServiceImpl;
    vasInfo.authorize = VasStateAuthorizeImpl;
    (void)NLSTK_CcpCreateVoiceAssistantService(&vasInfo);
}

void FuzzCcpVasControlResult(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    uint32_t requestId = fdp.ConsumeIntegral<uint32_t>();
    uint8_t opCode = fdp.ConsumeIntegral<uint8_t>();
    uint8_t errorCode = fdp.ConsumeIntegral<uint8_t>();
    (void)NLSTK_CcpVasControlResult(requestId, opCode, errorCode);
}

void FuzzCcpVasState(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    uint32_t requestId = fdp.ConsumeIntegral<uint32_t>();
    uint8_t errorCode = fdp.ConsumeIntegral<uint8_t>();
    (void)NLSTK_CcpVasStateAuthorizeResult(requestId, errorCode);
    uint8_t state = fdp.ConsumeIntegral<uint8_t>();
    (void)NLSTK_CcpUpdateVasState(state);
}


/* 单输入内状态一致性：instanceId 由调用方指定并贯穿 创建→控制结果→授权结果→
 * 属性更新→删除；VAS 单例服务同 requestId 贯穿创建→控制→状态授权→更新→删除。 */
void FuzzCcpConsistentSequence(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    NLSTK_CcpCallControlInfo_S info;
    (void)memset_s(&info, sizeof(info), 0, sizeof(info));
    uint8_t nameBuf[FUZZ_CCP_VAR_DATA_MAX_LEN] = {0};
    uint8_t protoBuf[FUZZ_CCP_VAR_DATA_MAX_LEN] = {0};
    FillVariableData(fdp, info.instanceName, nameBuf);
    info.type = NLSTK_CCP_CALL_CONTROL_COMMON_SERVICE;
    info.featureStatus = fdp.ConsumeIntegral<uint8_t>();
    FillVariableData(fdp, info.protocolSupport, protoBuf);
    info.mediaInstanceId = static_cast<int32_t>(1 + fdp.ConsumeIntegral<uint8_t>() % 255);
    info.callRequestSupport = fdp.ConsumeIntegral<uint16_t>();
    info.instanceIconFlag = false;
    info.networkSelectionFlag = false;
    info.startCcsInst = StartCcsInstImpl;
    info.authorize = CcsAuthorizeImpl;
    for (int i = 0; i < NLSTK_CCP_CCS_MAX_PROPERTY; i++) {
        info.propertyRights[i] = fdp.ConsumeIntegral<uint8_t>();
    }
    int32_t instanceId = info.mediaInstanceId;
    uint32_t requestId = fdp.ConsumeIntegral<uint32_t>();
    uint8_t errorCode = fdp.ConsumeIntegral<uint8_t>();
    NLSTK_CcpCcsPropertyType_E authorizeProperty = static_cast<NLSTK_CcpCcsPropertyType_E>(
        fdp.ConsumeIntegral<uint8_t>() % NLSTK_CCP_CCS_MAX_PROPERTY);

    (void)NLSTK_CcpCreateCcsInstance(&info);
    NLSTK_CcpCallControlResult(requestId, instanceId, errorCode);
    (void)NLSTK_CcpCcsAuthorizeResult(requestId, instanceId, authorizeProperty, errorCode);
    FuzzCcpUpdateCcsPropertyById(fdp, instanceId);
    (void)NLSTK_CcpDeleteCcsInstance(instanceId);

    NLSTK_CcpVasInfo_S vasInfo;
    (void)memset_s(&vasInfo, sizeof(vasInfo), 0, sizeof(vasInfo));
    vasInfo.state = fdp.ConsumeIntegral<uint8_t>();
    vasInfo.stateRight = fdp.ConsumeIntegral<uint8_t>();
    vasInfo.startService = VasStartServiceImpl;
    vasInfo.authorize = VasStateAuthorizeImpl;
    uint8_t vasOpCode = fdp.ConsumeIntegral<uint8_t>();
    uint8_t vasState = fdp.ConsumeIntegral<uint8_t>();
    (void)NLSTK_CcpCreateVoiceAssistantService(&vasInfo);
    (void)NLSTK_CcpVasControlResult(requestId, vasOpCode, errorCode);
    (void)NLSTK_CcpVasStateAuthorizeResult(requestId, errorCode);
    (void)NLSTK_CcpUpdateVasState(vasState);
    (void)NLSTK_CcpDeleteVoiceAssistantService();

    /* 保留非法参数路径：instanceId=0 */
    (void)NLSTK_CcpUpdateCcsProperty(0, authorizeProperty, nullptr);
}

}  // namespace OHOS

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc;
    (void)argv;
    NLSTK_InitStack();
    NLSTK_EnableStack();
    (void)SDF_ThreadInit(10);
    (void)SDF_EvcInit();
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    uint8_t *fuzzData = (uint8_t *)malloc(size);
    if (fuzzData == nullptr) {
        return 0;
    }
    (void)memcpy_s(fuzzData, size, data, size);
    OHOS::FuzzCcpConsistentSequence(fuzzData, size);
    SDF_MemFree(fuzzData);
    return 0;
}
