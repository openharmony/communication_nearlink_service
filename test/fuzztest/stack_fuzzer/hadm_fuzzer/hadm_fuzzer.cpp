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

#include "hadm_fuzzer.h"
#include "hadm_dft.h"
#include "securec.h"
#include "cpfwk_log.h"
#include "cm_errno.h"
#include "cm_logic_link_api.h"
#include "dli_cmd_struct.h"
#include "dli_event_struct.h"
#include "dli_errno.h"
#include "sdf_mem.h"
#include "sdf_addr.h"
#include "sdf_vector.h"
#include "sdf_buff.h"
#include "hadm_api.h"
#include "hadm_init.h"
#include "hadm_config_dli.h"
#include "hadm_sm.h"
#include "cm_api.h"
#include "hadm_config_cm.h"
#include "hadm_link_manager.h"
#include "sdf_worker.h"

#define HADM_PARAM_NUM 3
#define HADM_LOCAL_FEATURES_LEN 10
#define HADM_MOCK_LCID 123
#define HADM_SET_MEASURE_CONFIG_PARAM_LEN 49

typedef enum {
    FUZZ_ZERO    = 0,
    FUZZ_ONE     = 1,
    FUZZ_TWO     = 2,
    FUZZ_THREE   = 3,
    FUZZ_FOUR    = 4,
    FUZZ_FIVE    = 5,
    FUZZ_SIX     = 6,
    FUZZ_SEVEN     = 7,
    FUZZ_EIGHT     = 8,
    FUZZ_SIXTEEN     = 16,
    FUZZ_TWENTY     = 20,
    FUZZ_TWENTYFOUR     = 24,
    FUZZ_TWOHUNDREDFIFTYSIX = 256,
    FUZZ_NUM_END
} FUZZ_NUM_E;

static DLI_ExecuteCmdCbk g_hadmCbkList[DLI_CBK_MAX] = {0};
static CM_LogicLinkCbks_S g_logicLinkCbks[CM_MODULE_ID_MAX] = {{0}};

// Mock implementation
extern "C" uint32_t CM_RegLogicLinkListener(CM_LogicLinkCbks_S *cbks)
{
    CP_LOG_INFO("Enter Mocked CM_RegLogicLinkListener.");
    CP_CHECK_LOG_RETURN((cbks != NULL && cbks->moduleId < CM_MODULE_ID_MAX), CM_INVALID_PARAM_ERR, "param is null");
    for (uint8_t i = 0; i < (uint8_t)(sizeof(g_logicLinkCbks) / sizeof(CM_LogicLinkCbks_S)); i++) {
        if (cbks->moduleId == i) {
            g_logicLinkCbks[i] = *cbks;
            return CM_SUCCESS;
        }
    }
    CP_LOG_ERROR("[FUZZ] mock CM_RegLogicLinkCbks failed, moduleId:%d", cbks->moduleId);
    return CM_FAIL;
}

extern "C" uint32_t CM_UnRegLogicLinkListener(uint8_t moduleId)
{
    CP_LOG_INFO("Enter Mocked CM_UnRegLogicLinkListener.");
    (void) moduleId;
    return 0;
}

extern "C" uint32_t CM_ConnectUpdateParamReq(CM_ConnectUpdateParamReq_S *param)
{
    CP_LOG_INFO("Enter Mocked CM_ConnectUpdateParamReq.");
    CP_CHECK_LOG_RETURN(param != NULL, CM_INVALID_PARAM_ERR, "param is null");
    return CM_SUCCESS;
}

extern "C" void DLI_SetLocalFeatures(DLI_LocalFeatures_S *features)
{
    CP_LOG_INFO("Enter Mocked DLI_SetLocalFeatures.");
    (void)features;
}

extern "C" uint8_t *DLI_GetLocalFeatures(void)
{
    CP_LOG_INFO("Enter Mocked DLI_GetLocalFeatures.");
    static uint8_t features[HADM_LOCAL_FEATURES_LEN] = {0};
    return features;
}

extern "C" uint32_t DLI_ReadLocalMeasureCaps(void)
{
    CP_LOG_INFO("Enter Mocked DLI_ReadLocalMeasureCaps.");
    return 0;
}

extern "C" uint32_t DLI_ReadRemoteMeasureCaps(DLI_ReadRemoteMeasureCapsParam *param)
{
    CP_LOG_INFO("Enter Mocked DLI_ReadRemoteMeasureCaps.");
    (void) param;
    return 0;
}

extern "C" uint32_t DLI_SetMeasureParam(DLI_SetMeasureConfigParam *param)
{
    CP_LOG_INFO("Enter Mocked DLI_SetMeasureParam.");
    (void) param;
    return 0;
}

extern "C" uint32_t DLI_SetMeasureEnable(DLI_SetMeasureEnableParam *param)
{
    CP_LOG_INFO("Enter Mocked DLI_SetMeasureEnable.");
    (void) param;
    return 0;
}

extern "C" bool DLI_IsSupportNewDisMeasure(void)
{
    return true;
}

extern "C" void DLI_CmdCbkUnReg(const ModuleType module, const DLI_InnerCbkLineStru *innerTable, const uint32_t innerSize,
    const DLI_CbkLineStru *table, const uint32_t size)
{
    CP_LOG_INFO("Enter Mocked DLI_CmdCbkUnReg.");
    (void) module;
    (void)innerTable;
    (void)innerSize;
    (void) table;
    (void) size;
    return;
}

extern "C" uint32_t DLI_CmdCbkReg(const ModuleType module, const DLI_InnerCbkLineStru *innerTable, const uint32_t innerSize,
    const DLI_CbkLineStru *table, const uint32_t size)
{
    CP_LOG_INFO("Enter Mocked DLI_CmdCbkReg.");
    (void) module;
    CP_LOG_INFO("Enter Mocked DLI_CmdCbkReg.");
    CP_CHECK_LOG_RETURN(size != 0 && table != NULL, DLI_INVALID_PARAMETERS, "size = 0 or param is null");
    for (uint32_t i = 0; i < size; i++) {
        uint16_t opcode = table[i].opcode;
        DLI_ExecuteCmdCbk func = table[i].func;
        if (func == NULL) {
            CP_LOG_ERROR("add cbk func is null, opcode=0x%04X", opcode);
            continue;
        }
        g_hadmCbkList[opcode] = func;
    }
    return DLI_SUCCESS;
}

extern "C" uint32_t SchedulePostTaskBlocked(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb, int timeout)
{
    CP_LOG_INFO("Enter Mocked SchedulePostTaskBlocked.");
    if (cb != NULL) {
        cb(arg);
    }

    if (freeCb != NULL) {
        freeCb(arg);
    }

    return NLSTK_OK;
}

extern "C" uint32_t SchedulePostTask(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb)
{
    CP_LOG_INFO("Enter Mocked SchedulePostTask.");
    if (cb != NULL) {
        cb(arg);
    }

    if (freeCb != NULL) {
        freeCb(arg);
    }

    return NLSTK_OK;
}

namespace OHOS {
    // mock 函数
    void MockCMExecLogicLinkModuleCbks(uint8_t moduleId, CM_LogicLinkState_S *state)
    {
        if (g_logicLinkCbks[moduleId].logicLinkCbk != NULL) {
            g_logicLinkCbks[moduleId].logicLinkCbk(state);
            CP_LOG_ERROR("moduleId:%d, logic link cbk end", g_logicLinkCbks[moduleId].moduleId);
        }
    }

    void MockCMExecLogicLinkCbks(CM_LogicLinkState_S *state)
    {
        uint8_t count = (uint8_t)(sizeof(g_logicLinkCbks) / sizeof(CM_LogicLinkCbks_S));
        if (state->result == CM_LINK_STATE_CONNECTED) {
            for (uint8_t i = 0; i < count; i++) {
                MockCMExecLogicLinkModuleCbks(i, state);
            }
        } else if (state->result == CM_LINK_STATE_DISCONNECTED) {
            // 逆序执行，跟连接时相反
            for (uint8_t i = count; i > 0; i--) {
                MockCMExecLogicLinkModuleCbks(i - 1, state);
            }
        }
    }

    void MockCMExecLogicLinkRemoteFeaturesCbks(CM_LogicLinkRemoteFeatures_S *param)
    {
        for (uint8_t i = 0; i < (uint8_t)(sizeof(g_logicLinkCbks) / sizeof(CM_LogicLinkCbks_S)); i++) {
            if (g_logicLinkCbks[i].remoteFeaturesCbk != NULL) {
                g_logicLinkCbks[i].remoteFeaturesCbk(param);
                CP_LOG_ERROR("moduleId:%d, logic link remote features cbk end", g_logicLinkCbks[i].moduleId);
            }
        }
    }

    void MockHadmSoundingIqCbk(SLE_Addr_S *addr, HadmSoundingIqData_S *args)
    {
        (void)addr;
        (void)args;
    }

    void MockHadmSoundingControlResultCbk(SLE_Addr_S *addr, HadmUserOperate_E ctrlType, NLSTK_Errcode_E errorcode)
    {
        (void)addr;
        (void)ctrlType;
        (void)errorcode;
    }
    void MockHadmSoundingStateReport(HadmSoundingStateInfo_S *state)
    {
        (void)state;
    }

    // fuzz测试用例函数
    void FuzzGeneralEvtRegisterCbks(const uint8_t *data, size_t size)
    {
        (void)data;
        (void)size;
        HadmInitSoundingCbManager();

        CM_LogicLinkState_S state = {0};
        state.result = CM_LINK_STATE_CONNECTED;
        MockCMExecLogicLinkCbks(&state);        // HadmLinstenCmLinkReport
        state.result = CM_LINK_STATE_DISCONNECTED;
        MockCMExecLogicLinkCbks(&state);        // HadmLinstenCmLinkReport
        CM_LogicLinkRemoteFeatures_S remoteFeatures = {0};
        MockCMExecLogicLinkRemoteFeaturesCbks(&remoteFeatures);     // ReadRemoteCsCbk
        HadmDeInitLinkCbManager();
    }

    void FuzzSetMeasure(const uint8_t* data, size_t size)
    {
        // 模拟一个有效的参数
        if (size < sizeof(uint16_t) + sizeof(HadmSoundingParam_S) + sizeof(uint16_t) + sizeof(uint8_t)) {
            return;
        }
        uint32_t ret = HadmInitDliCmdVec();
        CP_CHECK_LOG_RETURN_VOID(ret == NLSTK_ERRCODE_SUCCESS, "[FUZZ]HadmInitDliCmdVec error");
        int8_t offset = 0;
        uint16_t lcid = (data[FUZZ_ZERO] << FUZZ_EIGHT) | (data[FUZZ_ONE]);
        offset += FUZZ_TWO;
        HadmSoundingParam_S *fuzzParams = (HadmSoundingParam_S *)SDF_MemZalloc(
            sizeof(HadmSoundingParam_S));
        (void)memcpy_s(fuzzParams, sizeof(HadmSoundingParam_S), data + offset, sizeof(HadmSoundingParam_S));
        offset += sizeof(HadmSoundingParam_S);
        HadmSetMeasureParam(lcid, fuzzParams);

        uint8_t csEnable = data[offset];
        offset += FUZZ_ONE;
        HadmSetMeasureEnable(lcid, csEnable);

        uint16_t expectRspType = (data[offset] << FUZZ_EIGHT) | (data[offset + FUZZ_ONE]);
        offset += FUZZ_TWO;
        HadmPopLastDliCmd(&expectRspType);
        HadmDeInitDliCmdVec();

        SDF_MemFree(fuzzParams);
    }

    void FuzzHADMEvtReportLocalCsCaps(void)
    {
        DLI_ExecuteCmdCbk cbk = g_hadmCbkList[DLI_CBK_READ_LOCAL_MEASURE_CAPS];
        DLI_ReadLocalCsCapsEvt evt = {0};
        (void)memset_s(evt.caps, sizeof(evt.caps), 0, sizeof(evt.caps));
        DLI_ExecuteCmdRetParam cmdRes = {0};
        cmdRes.cmdOpcode = 0;
        cmdRes.eventParameter = &evt;
        cmdRes.size = (uint32_t)(sizeof(DLI_ReadLocalCsCapsEvt));
        cbk(nullptr, 0, &cmdRes);
    }

    void FuzzHADMEvtReportRemoteCsCaps(void)
    {
        DLI_ExecuteCmdCbk cbk = g_hadmCbkList[DLI_CBK_READ_REMOTE_MEASURE_CAPS];
        DLI_ReadRemoteCsCapsEvt evt = {0};
        evt.status = 0;
        evt.connHandle = 0;
        (void)memset_s(evt.caps, sizeof(evt.caps), 0, sizeof(evt.caps));
        DLI_ExecuteCmdRetParam cmdRes = {0};
        cmdRes.cmdOpcode = 0;
        cmdRes.eventParameter = &evt;
        cmdRes.size = (uint32_t)(sizeof(DLI_ReadRemoteCsCapsEvt));
        cbk(nullptr, 0, &cmdRes);
    }

    void FuzzHADMEvtReportSetConfig(void)
    {
        DLI_ExecuteCmdCbk cbk = g_hadmCbkList[DLI_CBK_SET_MEASURE_CONFIG_PARAM];
        DLI_CommandStatus evt = {0};
        evt.status = 0;
        DLI_ExecuteCmdRetParam cmdRes = {0};
        cmdRes.cmdOpcode = 0;
        cmdRes.eventParameter = &evt;
        cmdRes.size = (uint32_t)(sizeof(DLI_CommandStatus));
        cbk(nullptr, 0, &cmdRes);
    }

    void FuzzHADMEvtReportSetEnable(void)
    {
        DLI_ExecuteCmdCbk cbk = g_hadmCbkList[DLI_CBK_SET_MEASURE_EN];
        DLI_CommandStatus evt = {0};
        evt.status = 0;
        DLI_ExecuteCmdRetParam cmdRes = {0};
        cmdRes.cmdOpcode = 0;
        cmdRes.eventParameter = &evt;
        cmdRes.size = (uint32_t)(sizeof(DLI_CommandStatus));
        cbk(nullptr, 0, &cmdRes);
    }

    void FuzzTestHadmApi(const uint8_t* data, size_t size)
    {
        // 模拟一个有效的参数
        if (size < sizeof(HadmConnectionParam_S) + sizeof(HadmSoundingParam_S) + FUZZ_ONE + FUZZ_FOUR) {
            return;
        }
        int8_t offset = FUZZ_ZERO;
        HadmSoundingCbk_S cbk = {0};
        cbk.controlResultCbk = MockHadmSoundingControlResultCbk;
        cbk.reportIqDataCbk = MockHadmSoundingIqCbk;
        cbk.soundingStateReportCbk = MockHadmSoundingStateReport;

        HadmRegCbk(&cbk);
        HadmInitSoundingCbManager();

        SLE_Addr_S addr = {0};
        HadmConnectionParam_S param = {0};
        (void)memcpy_s(&param, sizeof(HadmConnectionParam_S), data + offset, sizeof(HadmConnectionParam_S));
        offset += sizeof(HadmConnectionParam_S);
        HadmSoundingParam_S soundParam = {0};
        (void)memcpy_s(&soundParam, sizeof(HadmSoundingParam_S), data + offset, sizeof(HadmSoundingParam_S));
        offset += sizeof(HadmSoundingParam_S);
        HadmStartSounding(&addr, &param, &soundParam);

        HadmStopSounding(&addr);

        uint8_t state = data[offset];
        offset += FUZZ_ONE;
        HadmGetSoundingState(&addr, &state);

        uint32_t maxNum = (data[offset] << FUZZ_TWENTYFOUR) | (data[offset + FUZZ_ONE] << FUZZ_SIXTEEN) |
            (data[offset + FUZZ_TWO] << FUZZ_EIGHT) | (data[offset + FUZZ_THREE]);
        offset += FUZZ_FOUR;
        HadmGetSoundingAddrInfo(&addr, maxNum);

        HadmDeInitLinkCbManager();
    }

    void FuzzTestHadmConfig(const uint8_t* data, size_t size)
    {
        // 模拟一个有效的参数
        if (size < sizeof(HadmConnectionParam_S) + FUZZ_TWO) {
            return;
        }
        int8_t offset = FUZZ_ZERO;
        SLE_Addr_S addr = {0};
        HadmConnectionParam_S param = {0};
        (void)memcpy_s(&param, sizeof(HadmConnectionParam_S), data + offset, sizeof(HadmConnectionParam_S));
        offset += sizeof(HadmConnectionParam_S);
        HadmSetConnectionParamToCm(&addr, &param);

        uint16_t lcid = (data[offset] << FUZZ_EIGHT) | (data[offset + FUZZ_ONE]);
        offset += FUZZ_TWO;
        HadmReadRemoteMeasureCaps(lcid);
    }
}

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void) argc;
    (void) argv;
    HadmInit();
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    uint8_t *fuzzData = (uint8_t*)SDF_MemAlloc(size);
    CP_CHECK_LOG_RETURN(fuzzData != nullptr, 0, "Fuzz data memory alloc failed.");
    (void)memcpy_s(fuzzData, size, data, size);
    OHOS::FuzzGeneralEvtRegisterCbks(static_cast<uint8_t *>(fuzzData), size);
    OHOS::FuzzSetMeasure(fuzzData, size);
    OHOS::FuzzTestHadmApi(fuzzData, size);
    OHOS::FuzzTestHadmConfig(fuzzData, size);
    HadmDeInit();
    SDF_MemFree(fuzzData);
    return 0;
}