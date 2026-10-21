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
#include "fuzzer/FuzzedDataProvider.h"
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
        FuzzedDataProvider fdp(data, size);
        uint32_t ret = HadmInitDliCmdVec();
        CP_CHECK_LOG_RETURN_VOID(ret == NLSTK_ERRCODE_SUCCESS, "[FUZZ]HadmInitDliCmdVec error");
        uint16_t lcid = fdp.ConsumeIntegral<uint16_t>();
        HadmSoundingParam_S *fuzzParams = (HadmSoundingParam_S *)SDF_MemZalloc(
            sizeof(HadmSoundingParam_S));
        if (fuzzParams == NULL) {
            return;
        }
        fuzzParams->configId = fdp.ConsumeIntegral<uint8_t>();
        fuzzParams->schedulingTimeslot = fdp.ConsumeIntegral<uint8_t>();
        fuzzParams->rttPhy = fdp.ConsumeIntegral<uint8_t>();
        fuzzParams->freqHoppingMode = fdp.ConsumeIntegral<uint8_t>();
        fuzzParams->fmFreq = fdp.ConsumeIntegral<uint16_t>();
        fuzzParams->fmInteractionType = fdp.ConsumeIntegral<uint16_t>();
        fuzzParams->occurrenceGroupPeriod = fdp.ConsumeIntegral<uint16_t>();
        fuzzParams->fmOccurrenceGroupInterval = fdp.ConsumeIntegral<uint16_t>();
        fuzzParams->pmMeasureType1Interval = fdp.ConsumeIntegral<uint16_t>();
        fuzzParams->pmMeasureType2Interval = fdp.ConsumeIntegral<uint16_t>();
        fuzzParams->fmTIp1Time = fdp.ConsumeIntegral<uint16_t>();
        fuzzParams->pmTIp2Time = fdp.ConsumeIntegral<uint16_t>();
        fuzzParams->fmTGuard = fdp.ConsumeIntegral<uint8_t>();
        fuzzParams->fmSignal2Length = fdp.ConsumeIntegral<uint8_t>();
        fuzzParams->pmInitAntCount = fdp.ConsumeIntegral<uint8_t>();
        fuzzParams->pmInitSignal2Tone = fdp.ConsumeIntegral<uint8_t>();
        fuzzParams->pmReflAntCount = fdp.ConsumeIntegral<uint8_t>();
        fuzzParams->pmReflSignal2Tone = fdp.ConsumeIntegral<uint8_t>();
        fuzzParams->pmFreqHoppingBand = fdp.ConsumeIntegral<uint8_t>();
        for (uint8_t i = 0; i < HADM_MEASURE_PM_BAND_24G_LEN; i++) {
            fuzzParams->pm2400mBand[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        fuzzParams->glpMode = fdp.ConsumeIntegral<uint8_t>();
        fuzzParams->sleHadmMode = fdp.ConsumeIntegral<uint8_t>();
        fuzzParams->isCsParamChg = fdp.ConsumeIntegral<uint8_t>();
        fuzzParams->freqSpace = fdp.ConsumeIntegral<uint8_t>();
        fuzzParams->conAnchorNum = fdp.ConsumeIntegral<uint8_t>();
        fuzzParams->refreshRate = fdp.ConsumeIntegral<uint8_t>();
        fuzzParams->acbInterval = fdp.ConsumeIntegral<uint16_t>();
        fuzzParams->csInterval = fdp.ConsumeIntegral<uint16_t>();
        HadmSetMeasureParam(lcid, fuzzParams);

        uint8_t csEnable = fdp.ConsumeIntegral<uint8_t>();
        uint16_t enableLcid = fdp.ConsumeIntegral<uint16_t>();
        HadmSetMeasureEnable(enableLcid, csEnable);

        uint16_t expectRspType = fdp.ConsumeIntegral<uint16_t>();
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
        /* g_hadmCbkList 由 mock DLI_CmdCbkReg 在 HadmInit 时按
         * hadm_listen_dli.c 注册表填充；未注册时跳过避免空指针。 */
        DLI_ExecuteCmdCbk cbk = g_hadmCbkList[DLI_CBK_SET_MEASURE_CONFIG_PARAM];
        if (cbk == nullptr) {
            return;
        }
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
        if (cbk == nullptr) {
            return;
        }
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
        FuzzedDataProvider fdp(data, size);
        HadmSoundingCbk_S cbk = {0};
        cbk.controlResultCbk = MockHadmSoundingControlResultCbk;
        cbk.reportIqDataCbk = MockHadmSoundingIqCbk;
        cbk.soundingStateReportCbk = MockHadmSoundingStateReport;

        HadmRegCbk(&cbk);
        HadmInitSoundingCbManager();

        SLE_Addr_S addr = {0};
        addr.type = fdp.ConsumeIntegral<uint8_t>();
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        HadmConnectionParam_S param = {0};
        param.version = fdp.ConsumeIntegral<uint8_t>();
        param.localIndex = fdp.ConsumeIntegral<uint16_t>();
        param.intervalMin = fdp.ConsumeIntegral<uint16_t>();
        param.intervalMax = fdp.ConsumeIntegral<uint16_t>();
        param.eventIfs = fdp.ConsumeIntegral<uint8_t>();
        param.eventEfs = fdp.ConsumeIntegral<uint16_t>();
        param.maxLatency = fdp.ConsumeIntegral<uint16_t>();
        param.supervisionTimeout = fdp.ConsumeIntegral<uint16_t>();
        param.systemTimeUint = fdp.ConsumeIntegral<uint8_t>();
        param.txRxFlag = fdp.ConsumeIntegral<uint8_t>();
        HadmSoundingParam_S soundParam = {0};
        soundParam.configId = fdp.ConsumeIntegral<uint8_t>();
        soundParam.schedulingTimeslot = fdp.ConsumeIntegral<uint8_t>();
        soundParam.rttPhy = fdp.ConsumeIntegral<uint8_t>();
        soundParam.freqHoppingMode = fdp.ConsumeIntegral<uint8_t>();
        soundParam.fmFreq = fdp.ConsumeIntegral<uint16_t>();
        soundParam.fmInteractionType = fdp.ConsumeIntegral<uint16_t>();
        soundParam.occurrenceGroupPeriod = fdp.ConsumeIntegral<uint16_t>();
        soundParam.fmOccurrenceGroupInterval = fdp.ConsumeIntegral<uint16_t>();
        soundParam.pmMeasureType1Interval = fdp.ConsumeIntegral<uint16_t>();
        soundParam.pmMeasureType2Interval = fdp.ConsumeIntegral<uint16_t>();
        soundParam.fmTIp1Time = fdp.ConsumeIntegral<uint16_t>();
        soundParam.pmTIp2Time = fdp.ConsumeIntegral<uint16_t>();
        soundParam.fmTGuard = fdp.ConsumeIntegral<uint8_t>();
        soundParam.fmSignal2Length = fdp.ConsumeIntegral<uint8_t>();
        soundParam.pmInitAntCount = fdp.ConsumeIntegral<uint8_t>();
        soundParam.pmInitSignal2Tone = fdp.ConsumeIntegral<uint8_t>();
        soundParam.pmReflAntCount = fdp.ConsumeIntegral<uint8_t>();
        soundParam.pmReflSignal2Tone = fdp.ConsumeIntegral<uint8_t>();
        soundParam.pmFreqHoppingBand = fdp.ConsumeIntegral<uint8_t>();
        for (uint8_t i = 0; i < HADM_MEASURE_PM_BAND_24G_LEN; i++) {
            soundParam.pm2400mBand[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        soundParam.glpMode = fdp.ConsumeIntegral<uint8_t>();
        soundParam.sleHadmMode = fdp.ConsumeIntegral<uint8_t>();
        soundParam.isCsParamChg = fdp.ConsumeIntegral<uint8_t>();
        soundParam.freqSpace = fdp.ConsumeIntegral<uint8_t>();
        soundParam.conAnchorNum = fdp.ConsumeIntegral<uint8_t>();
        soundParam.refreshRate = fdp.ConsumeIntegral<uint8_t>();
        soundParam.acbInterval = fdp.ConsumeIntegral<uint16_t>();
        soundParam.csInterval = fdp.ConsumeIntegral<uint16_t>();
        HadmStartSounding(&addr, &param, &soundParam);

        HadmStopSounding(&addr);

        uint8_t state = fdp.ConsumeIntegral<uint8_t>();
        HadmGetSoundingState(&addr, &state);

        uint32_t maxNum = fdp.ConsumeIntegral<uint32_t>();
        HadmGetSoundingAddrInfo(&addr, maxNum);

        /* 量测配置/使能命令完成上报（原死代码接入）：走
         * hadm_listen_dli.c EvtReportSetCsConfig/EvtReportSetCsEnable 路径。 */
        FuzzHADMEvtReportSetConfig();
        FuzzHADMEvtReportSetEnable();

        HadmDeInitLinkCbManager();
    }

    void FuzzTestHadmConfig(const uint8_t* data, size_t size)
    {
        FuzzedDataProvider fdp(data, size);
        SLE_Addr_S addr = {0};
        addr.type = fdp.ConsumeIntegral<uint8_t>();
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        HadmConnectionParam_S param = {0};
        param.version = fdp.ConsumeIntegral<uint8_t>();
        param.localIndex = fdp.ConsumeIntegral<uint16_t>();
        param.intervalMin = fdp.ConsumeIntegral<uint16_t>();
        param.intervalMax = fdp.ConsumeIntegral<uint16_t>();
        param.eventIfs = fdp.ConsumeIntegral<uint8_t>();
        param.eventEfs = fdp.ConsumeIntegral<uint16_t>();
        param.maxLatency = fdp.ConsumeIntegral<uint16_t>();
        param.supervisionTimeout = fdp.ConsumeIntegral<uint16_t>();
        param.systemTimeUint = fdp.ConsumeIntegral<uint8_t>();
        param.txRxFlag = fdp.ConsumeIntegral<uint8_t>();
        HadmSetConnectionParamToCm(&addr, &param);

        uint16_t lcid = fdp.ConsumeIntegral<uint16_t>();
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
    FuzzedDataProvider fdp(data, size);
    std::vector<uint8_t> fuzzVec = fdp.ConsumeBytes<uint8_t>(size);
    uint8_t *fuzzData = fuzzVec.data();
    size_t fuzzSize = fuzzVec.size();
    OHOS::FuzzGeneralEvtRegisterCbks(static_cast<uint8_t *>(fuzzData), fuzzSize);
    OHOS::FuzzSetMeasure(fuzzData, fuzzSize);
    OHOS::FuzzTestHadmApi(fuzzData, fuzzSize);
    OHOS::FuzzTestHadmConfig(fuzzData, fuzzSize);
    HadmDeInit();
    return 0;
}