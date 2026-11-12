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

#include "hadmstm_fuzzer.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "hadm_dft.h"
#include "securec.h"
#include "cpfwk_log.h"
#include "cm_errno.h"
#include "cm_logic_link_api.h"
#include "dli_cmd_struct.h"
#include "dli_event_struct.h"
#include "dli_errno.h"
#include "nlstk_init_api.h"
#include "nlstk_log.h"
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
#include "cm_logic_link_listener_mgr.h"
#include "sdf_worker.h"
#include "hadm_user_proc.h"
#include "sdf_timer.h"

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

static SLE_Addr_S g_addr = {
    .type = PUBLIC_ADDRESS,
    .addr = {0x33, 0x44, 0x55, 0x66, 0x77, 0x88}
};

// mock schedule
extern "C" uint32_t SchedulePostTaskBlocked(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb, int timeout)
{
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
    if (cb != NULL) {
        cb(arg);
    }

    if (freeCb != NULL) {
        freeCb(arg);
    }

    return NLSTK_OK;
}

extern "C" uint32_t ScheduleEnable(void)
{
    return NLSTK_OK;
}

extern "C" void ScheduleDisable(void)
{
    return;
}

extern "C" uint32_t ScheduleTimerAdd(int *handle, SDF_TimerParam *param)
{
    (void)handle;
    (void)param;
    return CP_OK;
}

extern "C" void ScheduleTimerDel(int handle)
{
    (void)handle;
}

namespace OHOS {
    /* FUZZ-014: CM 公共分发器直调——RemoteFeatures/ConnUpdateParam 两分发器
     * （cm_logic_link_listener_mgr.c:88-107 真实现，遍历 g_logicLinkCbks 静态
     * 数组）此前零调用，各模块注册的能力/参数回调无真实分发。 */
    void FuzzHadmStmPublicDispatchers(uint8_t* data, size_t size)
    {
        FuzzedDataProvider fdp(data, size);
        CM_LogicLinkRemoteFeatures_S rf = {0};
        rf.lcid = fdp.ConsumeIntegral<uint16_t>();
        rf.addr = g_addr;
        rf.features[8] = 0x4; /* 对端支持测距能力位（hadm_listen_cm.c:74-77） */
        CM_ExecLogicLinkRemoteFeaturesCbks(&rf);

        CM_LogicLinkRemoteFeatures_S rfZero = {0};
        rfZero.lcid = fdp.ConsumeIntegral<uint16_t>();
        rfZero.addr = g_addr; /* features 全零变体 */
        CM_ExecLogicLinkRemoteFeaturesCbks(&rfZero);

        CM_LogicLinkConnUpdateParam_S cu = {0};
        cu.lcid = fdp.ConsumeIntegral<uint16_t>();
        cu.addr = g_addr;
        CM_ExecLogicLinkConnUpdateParamCbks(&cu);

        CM_ExecLogicLinkRemoteFeaturesCbks(nullptr);
        CM_ExecLogicLinkConnUpdateParamCbks(nullptr);
    }

    void FuzzHadmStm(uint8_t* data, size_t size)
    {
        FuzzedDataProvider fdp(data, size);
        CM_LogicLinkState_S cmLinkState = {0};
        cmLinkState.result = CM_LINK_STATE_CONNECTED;
        (void)memcpy_s(&(cmLinkState.addr),sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
        uint8_t moduleId = CM_MODULE_HADM;
        CM_ExecLogicLinkModuleCbks(moduleId, &cmLinkState);   // 回调运行HadmLinstenCmLinkReport,不会重复alloc内存
        HadmSoundingState_E state = static_cast<HadmSoundingState_E>(
            fdp.ConsumeIntegral<uint8_t>() % HADM_SOUNDING_STATE_INVALID);
        uint32_t ret = HadmSetSoundingState(&g_addr, state);
        if (ret == NLSTK_HADM_ERRCODE_CAN_NOT_FIND_LINKCB) {    // 8193
            /* 限频: init未完成时每次输入都会命中, 全量打印会灌满hdc stdout通道楔死会话 */
            static bool fuzzNoLinkCbLogged = false;
            if (!fuzzNoLinkCbLogged) {
                NLSTK_LOG_ERROR("[FUZZ]HadmSetSoundingState failed (no linkcb), ret = %d", ret);
                fuzzNoLinkCbLogged = true;
            }
            return;
        }

        HadmStateMachineEvent_E event = static_cast<HadmStateMachineEvent_E>(
            fdp.ConsumeIntegral<uint8_t>() % HADM_MAX_EVENT_TYPE);
        HadmDliReportRemoteCsParam_S *param = (HadmDliReportRemoteCsParam_S *)SDF_MemZalloc(
            sizeof(HadmDliReportRemoteCsParam_S));
        if (param == nullptr) {
            return;
        }
        param->status = fdp.ConsumeIntegral<uint16_t>();
        param->csCaps.measureSignalCapabilitySupported = fdp.ConsumeIntegral<uint32_t>();
        param->csCaps.multiAntennasSupported = fdp.ConsumeIntegral<uint8_t>();
        param->csCaps.multiAntennasSwitchInterval = fdp.ConsumeIntegral<uint8_t>();
        param->csCaps.type1MinTimeIp1 = fdp.ConsumeIntegral<uint8_t>();
        param->csCaps.type1MinTimeIp2 = fdp.ConsumeIntegral<uint8_t>();
        param->csCaps.type1MinTimeIp3 = fdp.ConsumeIntegral<uint8_t>();
        param->csCaps.type1MinTimeIp4 = fdp.ConsumeIntegral<uint8_t>();
        param->csCaps.type1MinTimeInterEvt = fdp.ConsumeIntegral<uint16_t>();
        param->csCaps.type2MinTimeInterEvt = fdp.ConsumeIntegral<uint16_t>();
        param->csCaps.minTimeInitializeInterEvt = fdp.ConsumeIntegral<uint16_t>();
        param->csCaps.minTimeIntraEvt = fdp.ConsumeIntegral<uint16_t>();
        param->csCaps.minTimeIntraEvtGroup = fdp.ConsumeIntegral<uint16_t>();
        param->csCaps.phaseCaliOffsetCm = fdp.ConsumeIntegral<uint16_t>();
        param->csCaps.tofCaliOffsetM = fdp.ConsumeIntegral<uint16_t>();
        (void)HadmTriggerStateMachine(&g_addr, event, param);

        cmLinkState.result = CM_LINK_STATE_DISCONNECTED;
        CM_ExecLogicLinkModuleCbks(moduleId, &cmLinkState);
        SDF_MemFree(param);
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

    void FuzzHadmUserProc(uint8_t* data, size_t size)
    {
        FuzzedDataProvider fdp(data, size);
        HadmSoundingCbk_S *cbk = (HadmSoundingCbk_S *)SDF_MemZalloc(sizeof(HadmSoundingCbk_S));
        if (cbk == nullptr) {
            return;
        }
        cbk->controlResultCbk = MockHadmSoundingControlResultCbk;
        cbk->reportIqDataCbk = MockHadmSoundingIqCbk;
        cbk->soundingStateReportCbk = MockHadmSoundingStateReport;
        HadmServiceRegisterCallBack(cbk);

        CM_LogicLinkState_S cmLinkState = {0};
        cmLinkState.result = CM_LINK_STATE_CONNECTED;
        (void)memcpy_s(&(cmLinkState.addr),sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
        uint8_t moduleId = CM_MODULE_HADM;
        CM_ExecLogicLinkModuleCbks(moduleId, &cmLinkState);   // 回调运行HadmLinstenCmLinkReport,不会重复alloc内存

        HadmUserStartSoundingParam_S soundingParam = {{0}};
        (void)memcpy_s(&(soundingParam.addr), sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
        HadmUserStartSounding(&soundingParam);

        HadmUserOperate_E operate = static_cast<HadmUserOperate_E>(
            fdp.ConsumeIntegral<uint8_t>() % HADM_USER_STOP_SOUNDING);
        HadmCacheUserOperate(&g_addr, operate);
        HadmSoundingState_E state = static_cast<HadmSoundingState_E>(
            fdp.ConsumeIntegral<uint8_t>() % HADM_SOUNDING_STATE_INVALID);
        HadmSetSoundingState(&g_addr, state);

        HadmSoundingStateParam_S stateParam = {{0}};
        (void)memcpy_s(&(stateParam.addr), sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
        HadmUserGetSoundingState(&stateParam);

        HadmUserStopSounding(&g_addr);

        cmLinkState.result = CM_LINK_STATE_DISCONNECTED;
        CM_ExecLogicLinkModuleCbks(moduleId, &cmLinkState);
        SDF_MemFree(cbk);
    }

    void FuzzHadmStmApi(uint8_t *data, size_t size)
    {
        if (data == nullptr || size < 1) {
            return;
        }
        FuzzHadmStm(data, size);
        FuzzHadmUserProc(data, size);
    }
}

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void) argc;
    (void) argv;
    if (NLSTK_IsStackInited()) {
        NLSTK_DisableStack();
        NLSTK_DeinitStack();
    }
    NLSTK_InitStack();
    if (!NLSTK_IsStackInited()) {
        /* 完整栈初始化未完成(mock调度器下常见): 回退直接初始化HADM, 同 hadm_fuzzer 模式 */
        (void)HadmInit();
    }
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    uint8_t *fuzzData = (uint8_t *)malloc(size);
    if (fuzzData == nullptr) {
        return 0;
    }
    memcpy_s(fuzzData, size, data, size);
    OHOS::FuzzHadmStmPublicDispatchers(fuzzData, size);
    OHOS::FuzzHadmStmApi(fuzzData, size);
    free(fuzzData);
    return 0;
}