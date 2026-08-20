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
#define FUZZ_CCP_VALUE_BUF_LEN 64

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

static uint16_t FillVariableData(FuzzedDataProvider &fdp, NLSTK_VariableData_S &var, uint8_t *buf)
{
    uint16_t len = fdp.ConsumeIntegral<uint8_t>() % FUZZ_CCP_VAR_DATA_MAX_LEN;
    for (uint16_t i = 0; i < len; i++) {
        buf[i] = fdp.ConsumeIntegral<uint8_t>();
    }
    var.len = len;
    var.data = buf;
    return len;
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
    uint8_t value[FUZZ_CCP_VALUE_BUF_LEN] = {0};
    for (int i = 0; i < FUZZ_CCP_VALUE_BUF_LEN; i++) {
        value[i] = fdp.ConsumeIntegral<uint8_t>();
    }
    (void)NLSTK_CcpUpdateCcsProperty(instanceId, property, value);
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
    OHOS::FuzzCcpCreateCcsInstance(fuzzData, size);
    OHOS::FuzzCcpCallControlResult(fuzzData, size);
    OHOS::FuzzCcpCcsAuthorizeResult(fuzzData, size);
    OHOS::FuzzCcpUpdateCcsProperty(fuzzData, size);
    OHOS::FuzzCcpDeleteCcsInstance(fuzzData, size);
    OHOS::FuzzCcpCreateVas(fuzzData, size);
    OHOS::FuzzCcpVasControlResult(fuzzData, size);
    OHOS::FuzzCcpVasState(fuzzData, size);
    (void)NLSTK_CcpDeleteVoiceAssistantService();
    SDF_MemFree(fuzzData);
    return 0;
}
