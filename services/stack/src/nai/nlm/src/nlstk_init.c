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

#include <dlfcn.h>

#include "sdf_evc.h"
#include "sdf_timer.h"
#include "sdf_event.h"
#include "sdf_worker.h"
#include "sdf_thread.h"

#include "nlstk_public_define.h"
#include "nlstk_log.h"
#include "nlstk_schedule.h"
#include "nlstk_reg_cbk_ext.h"
#include "nai_dft.h"

#include "dli.h"
#include "dli_callback.h"
#include "dli_cmd.h"
#include "cm_api.h"
#include "cm_errno.h"
#include "cm_logic_link_api.h"
#include "dli_errno.h"
#include "dtap.h"
#include "dtap_errno.h"
#include "qosm.h"
#include "qosm_errno.h"
#include "transport.h"
#include "transport_errno.h"
#include "bnl_proxy.h"

#include "nlstk_devd.h"
#include "devd_scan_api.h"
#include "nlstk_sm.h"
#include "nlstk_sm_api.h"
#include "hadm_init.h"
#include "ssap_manager.h"
#include "nlstk_cfgdb.h"
#include "cdsm.h"
#include "nlstk_mcp_media.h"
#include "nlstk_mcp_volume.h"
#include "nlstk_ccp.h"
#include "actm.h"
#include "micp_dev.h"
#include "icce_init.h"
#include "hid_client_init.h"
#include "dis_common.h"

#include "nlstk_init_api.h"
#include "nlstk_port_server.h"
#include "nlstk_port_client.h"
#include "port_server_init.h"
#include "port_client_init.h"
#include "sl_ext_func_wrapper.h"
#include "dli_reg_ext_func.h"
#include "common_reg_ext_func.h"
#include "collab_ext_func_wrapper.h"
#include "collab_reg_ext_func.h"
#include "devd_ext_func_wrapper.h"
#include "devd_reg_ext_func.h"
#include "qosm_reg_ext_func.h"
#include "bnl_reg_ext_func.h"
#include "bas_client.h"
#include "hadm_reg_ext_func.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STACK_MAX_THREAD 5
#define DEFAULT_WAIT_TIME 1000

#ifdef __aarch64__
#define NL_STK_LIB_PLUGIN_PATH_NAME "/system/lib64/libnearlink_stack_ext.z.so"
#else
#define NL_STK_LIB_PLUGIN_PATH_NAME "/system/lib/libnearlink_stack_ext.z.so"
#endif

typedef uint32_t (*STK_ExtInitPtr)(const STK_Callback *cb);
typedef void (*STK_ExtDeinitPtr)(void);

typedef uint32_t (*STK_ExtModuleInitPtr)(void);
typedef void (*STK_ExtModuleDeinitPtr)(void);

bool g_isStackInited = false;
bool g_isStackEnabled = false;
void *g_pluginSoHandle = NULL;

static uint32_t SdfInit(void)
{
    uint32_t ret = SDF_ThreadInit(STACK_MAX_THREAD);
    if (ret != NLSTK_OK) {
        return ret;
    }
    ret = SDF_EvcInit();
    if (ret != NLSTK_OK) {
        SDF_ThreadDeinit();
        return ret;
    }
    return NLSTK_OK;
}

static void SdfDeinit(void)
{
    SDF_EvcDeinit();
    SDF_ThreadDeinit();
}

static void DftReportKill(uint16_t switchType)
{
    DftParamC *params = (DftParamC*)SDF_MemZalloc(NAI_DFT_PARAM_SIZE_2 * sizeof(DftParamC));
    NLSTK_CHECK_RETURN_VOID(params != NULL, "DftParamC create failed");
    char timestamp[TIME_STAMP_SIZE] = {0};
    DftGetTimestamp(timestamp, TIME_STAMP_SIZE);
    params[PARAM_INDEX_0] = CreateUi16ParamC(SWITCH_TYPE_INFO, switchType);
    params[PARAM_INDEX_1] = CreateStrParamC(SWITCH_END_TIME, TIME_STAMP_SIZE, timestamp);

    DftManagerReport(DFT_SWITCH_EXCEP, params, NAI_DFT_PARAM_SIZE_2);
    DftFreeBasicTypeParams(params, NAI_DFT_PARAM_SIZE_2);
}

static uint32_t DliCallbackInit(void)
{
    DLI_Callback dliCallback = {0};
    dliCallback.postOtherThread = SchedulePostTask;
    dliCallback.postOtherBlockedThread = SchedulePostTaskBlocked;
    dliCallback.dftReportKill = DftReportKill;
    dliCallback.recvAcbHandler = DTAP_DataRecv;
    dliCallback.getExtRegOpcode = DLI_GetExtFuncList()->getExtRegOpcode;
    dliCallback.hadmProcessCsCaps = HADM_GetExtFuncList()->processCsCaps;
    return DLI_SetCallback(&dliCallback);
}

static uint32_t DpInit(void)
{
    uint32_t ret = DliCallbackInit();
    if (ret != NLSTK_OK) {
        return NLSTK_ERR;
    }
    ret = DLI_Init();
    if (ret != NLSTK_OK) {
        goto FAIL_DLI;
    }
    ret = CM_Init();
    if (ret != CM_SUCCESS) {
        goto FAIL_CM;
    }
    ret = DTAP_Init();
    if (ret != DTAP_SUCCESS) {
        goto FAIL_DTAP;
    }
    ret = QOSM_Init();
    if (ret != QOSM_SUCCESS) {
        goto FAIL_QOSM;
    }
    ret = TRANS_Init();
    if (ret != TRANS_SUCCESS) {
        goto FAIL_TRANS;
    }
    return NLSTK_OK;
FAIL_TRANS:
    QOSM_DeInit();
FAIL_QOSM:
    DTAP_DeInit();
FAIL_DTAP:
    CM_DeInit();
FAIL_CM:
    DLI_DeInit();
FAIL_DLI:
    DLI_SetCallback(NULL);
    return NLSTK_ERR;
}

static void DpDeinit(void)
{
    TRANS_DeInit();
    QOSM_DeInit();
    DTAP_DeInit();
    CM_DeInit();
    DLI_DeInit();
    DLI_SetCallback(NULL);
}

static uint32_t BalInit(void)
{
    uint32_t ret = DevdScanInit();
    if (ret != NLSTK_OK) {
        return NLSTK_ERR;
    }
    ret = CdsmInit();
    if (ret != NLSTK_OK) {
        DevdScanDeInit();
        return NLSTK_ERR;
    }
    ret = ActmInit();
    if (ret != NLSTK_OK) {
        DevdScanDeInit();
        CdsmDeInit();
        return NLSTK_ERR;
    }
    ret = MicpDevInit();
    if (ret != NLSTK_OK) {
        DevdScanDeInit();
        ActmDeinit();
        CdsmDeInit();
        return NLSTK_ERR;
    }
    return NLSTK_OK;
}

static void BalDeInit(void)
{
    MicpDevDeinit();
    ActmDeinit();
    CdsmDeInit();
    (void)DevdScanDeInit();
}

static void StackInitInner(void *arg)
{
    uint32_t *result = (uint32_t *)arg;
    uint32_t ret = CfgdbInit();
    if (ret != NLSTK_OK) {
        *result = NLSTK_ERR;
        return;
    }
    ret = DevdInit();
    if (ret != NLSTK_OK) {
        goto FAIL_DEVD;
    }
    ret = SmInit();
    if (ret != NLSTK_OK) {
        goto FAIL_SM;
    }
    ret = SSAP_Init();
    if (ret != NLSTK_OK) {
        goto FAIL_SSAP;
    }
    ret = HadmInit();
    if (ret != NLSTK_OK) {
        goto FAIL_HADM;
    }
    ret = BalInit();
    if (ret != NLSTK_OK) {
        goto FAIL_BAL;
    }
    ret = BNL_Init();
    if (ret != NLSTK_OK) {
        goto FAIL_BNL;
    }
    *result = NLSTK_OK;
    return;
FAIL_BNL:
    BalDeInit();
FAIL_BAL:
    HadmDeInit();
FAIL_HADM:
    SSAP_DeInit();
FAIL_SSAP:
    SmDeInit();
FAIL_SM:
    DevdDeInit();
FAIL_DEVD:
    CfgdbDeinit();
    *result = NLSTK_ERR;
    return;
}

static uint32_t StackInit(void)
{
    uint32_t *result = SDF_MemZalloc(sizeof(uint32_t));
    NLSTK_CHECK_RETURN(result != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "stack init malloc failed");
    uint32_t ret = SchedulePostTaskBlocked(StackInitInner, result, NULL, DEFAULT_WAIT_TIME);
    if (ret == NLSTK_ERRCODE_TASK_TIMEOUT) {
        NLSTK_LOG_ERROR("StackInit timeout");
        // 在超时场景下，任务并没有被删除，result的内存还会被inner函数使用，因此需要再调度一个任务去释放result
        (void)SchedulePostTask(SDF_MemFree, result, NULL);
        return NLSTK_ERRCODE_TASK_TIMEOUT;
    } else if (ret != NLSTK_OK) {
        SDF_MemFree(result);
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    ret = *result;
    SDF_MemFree(result);
    NLSTK_LOG_INFO("Stack Init ret=%u", ret);
    return ret;
}

static void StackDeinitInner(void *arg)
{
    (void)arg;
    BNL_DeInit();
    BalDeInit();
    HadmDeInit();
    SSAP_DeInit();
    SmDeInit();
    DevdDeInit();
    CfgdbDeinit();
}

static void StackDeinit(void)
{
    uint32_t ret = SchedulePostTaskBlocked(StackDeinitInner, NULL, NULL, DEFAULT_WAIT_TIME);
    if (ret == NLSTK_ERRCODE_TASK_TIMEOUT) {
        NLSTK_LOG_ERROR("StackDeinit timeout");
    } else if (ret != NLSTK_OK) {
        NLSTK_LOG_ERROR("StackDeinit task fail");
    } else {
        NLSTK_LOG_INFO("StackDeinit success");
    }
}

static uint32_t SetRealACBSubrate(NLSTK_SetAcbSubrateParam_S *param,
    uint32_t (*setSubrate)(DLI_ACBSubrateParam *dliParam))
{
    NLSTK_CHECK_RETURN(param != NULL, NLSTK_ERRCODE_PARAM_ERR, "param is null");
    NLSTK_LOG_DEBUG("enter, %02x:%02x:**:**:**:%02x", param->addr.addr[5], param->addr.addr[4], param->addr.addr[0]);
    CM_SetACBSubrateParam subrateParam = {0};
    subrateParam.subrate = param->subrate;
    subrateParam.onlySubrate = param->onlySubrate;
    subrateParam.subrateMax = param->subrateMax;
    subrateParam.maxLatency = param->maxLatency;
    subrateParam.continuationNum = param->continuationNum;
    subrateParam.supervisionTimeout = param->supervisionTimeout;
    subrateParam.addr.type = param->addr.type;
    if (memcpy_s(subrateParam.addr.addr, sizeof(subrateParam.addr.addr), param->addr.addr,
        sizeof(param->addr.addr)) != EOK) {
        NLSTK_LOG_ERROR("copy addr failed");
        return NLSTK_ERRCODE_NOMEM;
    }

    return CM_SetRealACBSubrate(&subrateParam, setSubrate);
}

static uint32_t EnableRealHighPower(NLSTK_EnableConnHighPowerParam_S *inParam,
    uint32_t (*enableConnHighPower)(DLI_EnableConnHighPowerParam *param))
{
    NLSTK_CHECK_RETURN(inParam != NULL, NLSTK_ERRCODE_PARAM_ERR, "inParam is null");
    CM_EnableConnHighPowerReq_S setParam = {0};
    setParam.lcid = inParam->conHandle;
    setParam.enable = inParam->enable;
    setParam.powerLevel = inParam->powerLevel;

    return CM_EnableRealConnHighPower(&setParam, enableConnHighPower);
}

static uint32_t GetAddrByLcid(uint16_t lcid, SLE_Addr_S *addr)
{
    CM_LogicLink_S link = {0};
    NLSTK_CHECK_RETURN(addr != NULL, NLSTK_ERR, "addr is null");
    if (CM_GetLogicLinkByLcid(lcid, &link) != 0) {
        NLSTK_LOG_ERROR("get logic link by lcid failed");
        return NLSTK_ERR;
    }
    (void)memcpy_s(addr, sizeof(SLE_Addr_S), &link.addr, sizeof(SLE_Addr_S));
    return 0;
}

static uint32_t GetLcidByAddr(SLE_Addr_S *addr)
{
    CM_LogicLink_S link = {0};
    if (CM_GetLogicLinkByAddr(addr, &link) != 0) {
        NLSTK_LOG_ERROR("get logic link by addr failed");
        return UINT32_MAX;
    }
    return link.lcid;
}

static STK_Callback g_stkCallback = {
    .postTask = SchedulePostTask,
    .postTaskBlocked = SchedulePostTaskBlocked,
    .timerAdd = ScheduleTimerAdd,
    .timerDel = ScheduleTimerDel,
    .getAddr = GetAddrByLcid,
    .getLcid = GetLcidByAddr,
    .setRealACBSubrate = SetRealACBSubrate,
    .enableRealConnHighPower = EnableRealHighPower,
    .setRemoteFeature = CM_SetRemoteFeature,
};

static uint32_t StackOpenFuncInit(void *soHandle)
{
    STK_ExtInitPtr func = (STK_ExtInitPtr)dlsym(soHandle, "STK_ExtInit");
    if (func == NULL) {
        NLSTK_LOG_ERROR("stack plugin init err: %s", dlerror());
        return NLSTK_ERR;
    }

    if (func(&g_stkCallback) != 0) {
        NLSTK_LOG_ERROR("nlstk init failed");
        return NLSTK_ERR;
    }

    return 0;
}

static void StackOpenFuncDeinit(void *soHandle)
{
    STK_ExtDeinitPtr func = (STK_ExtDeinitPtr)dlsym(soHandle, "STK_ExtDeinit");
    if (func == NULL) {
        NLSTK_LOG_ERROR("stack plugin deinit err: %s", dlerror());
        return;
    }
    func();
}

static void StackExtFuncInit(void *soHandle)
{
    // sub-modules could be added here
    DLI_RegisterExtFunc(soHandle);
    COLLAB_RegisterExtFunc(soHandle);
    COMMON_RegisterExtFunc(soHandle);
    QOSM_RegisterExtFunc(soHandle);
    Devd_RegisterExtFunc(soHandle);
    BNL_RegisterExtFunc(soHandle);
    HADM_RegisterExtFunc(soHandle);
}

static void StackFuncInit(void)
{
    if (g_pluginSoHandle != NULL) {
        NLSTK_LOG_INFO("plugin handle already not null");
        return;
    }

    // check whether this configuration is appropriate
    void *pluginSoHandle = dlopen(NL_STK_LIB_PLUGIN_PATH_NAME, RTLD_NOW | RTLD_GLOBAL);
    if (!pluginSoHandle) {
        NLSTK_LOG_ERROR("dlopen stack plugin so err: %s", dlerror());
        return;
    }

    dlerror();

    uint32_t ret = StackOpenFuncInit(pluginSoHandle);
    if (ret != 0) {
        dlclose(pluginSoHandle);
        return;
    }

    StackExtFuncInit(pluginSoHandle);
    g_pluginSoHandle = pluginSoHandle;
}

static void StackExtFuncDeinit(void)
{
    // sub-modules could be added here
    DLI_DeregisterExtFunc();
    COLLAB_DeregisterExtFunc();
    COMMON_DeregisterExtFunc();
    QOSM_DeregisterExtFunc();
    Devd_DeregisterExtFunc();
    BNL_DeregisterExtFunc();
}

static void StackFuncDeinit(void)
{
    if (g_pluginSoHandle == NULL) {
        return;
    }
    StackOpenFuncDeinit(g_pluginSoHandle);
    StackExtFuncDeinit();
    dlclose(g_pluginSoHandle);
    g_pluginSoHandle = NULL;
}

static uint32_t ExtModuleInit(void)
{
    if (g_pluginSoHandle == NULL) {
        return NLSTK_OK;
    }
    STK_ExtModuleInitPtr func = (STK_ExtModuleInitPtr)dlsym(g_pluginSoHandle, "STK_ExtModuleInit");
    if (func == NULL) {
        NLSTK_LOG_WARN("dlsym STK_ExtModuleInit err: %s", dlerror());
        return NLSTK_OK;
    }
    NLSTK_CHECK_RETURN(func() == 0, NLSTK_ERR, "STK_ExtModuleInit failed");
    return NLSTK_OK;
}

static void ExtModuleDeinit(void)
{
    NLSTK_CHECK_RETURN_VOID(g_pluginSoHandle != NULL, "soHandle is null");
    STK_ExtModuleDeinitPtr cbk = (STK_ExtModuleDeinitPtr)dlsym(g_pluginSoHandle, "STK_ExtModuleDeinit");
    NLSTK_CHECK_RETURN_VOID(cbk != NULL, "dlsym STK_ExtModuleDeinit failed, err: %s", dlerror());
    cbk();
}

NLSTK_Errcode_E NLSTK_InitStack(void)
{
    StackFuncInit();
    uint32_t ret = SdfInit();
    if (ret != NLSTK_OK) {
        return NLSTK_ERRCODE_FAIL;
    }
    ret = ScheduleEnable();
    if (ret != NLSTK_OK) {
        goto FAIL_SCHEDULE;
    }
    ret = DpInit();
    if (ret != NLSTK_OK) {
        goto FAIL_DP;
    }
    ret = ExtModuleInit();
    if (ret != NLSTK_OK) {
        goto FAIL_EXT;
    }
    ret = StackInit();
    if (ret != NLSTK_OK) {
        goto FAIL_NLSTK;
    }
    g_isStackInited = true;
    return NLSTK_ERRCODE_SUCCESS;
FAIL_NLSTK:
    ExtModuleDeinit();
FAIL_EXT:
    DpDeinit();
FAIL_DP:
    ScheduleDisable();
FAIL_SCHEDULE:
    SdfDeinit();
    return NLSTK_ERRCODE_FAIL;
}

void NLSTK_DeinitStack(void)
{
    if (!g_isStackInited) {
        return;
    }
    StackDeinit();
    ExtModuleDeinit();
    DpDeinit();
    ScheduleDisable();
    SdfDeinit();
    StackFuncDeinit();
    g_isStackInited = false;
}

bool NLSTK_IsStackInited(void)
{
    return g_isStackInited;
}

static void ReadDpInfosInner(void *param)
{
    (void)param;
    DLI_ReadCommConfigValue();
}

static uint32_t ReadDpInfos(void)
{
    uint32_t ret = SchedulePostTaskBlocked(ReadDpInfosInner, NULL, NULL, DEFAULT_WAIT_TIME);
    if (ret != NLSTK_OK) {
        NLSTK_LOG_ERROR("SchedulePostTaskBlocked failed, ret=%u", ret);
        return ret;
    }
    return NLSTK_OK;
}

static void DpDisable()
{
    QOSM_Disable();
    CM_Disable();
    DLI_Disable();
}

static uint32_t DpEnable()
{
    uint32_t ret = DLI_Enable();
    if (ret != DLI_SUCCESS) {
        return NLSTK_ERR;
    }
    ret = CM_Enable();
    if (ret != CM_SUCCESS) {
        DLI_Disable();
        return NLSTK_ERR;
    }
    ret = QOSM_Enable();
    if (ret != QOSM_SUCCESS) {
        CM_Disable();
        DLI_Disable();
        return NLSTK_ERR;
    }
    return NLSTK_OK;
}

static void StackEnableInner(void *arg)
{
    uint32_t *result = (uint32_t *)arg;
    SmEnable();
    DevdEnable();
    McpMediaEnable();
    (void)McpVolumeEnable();
    CcpEnable();
    IcceClientEnable();
    IcceServerEnable();
    (void)PortClientEnable();
    (void)PortServerEnable();
    (void)HidEnable();
    (void)BasEnable();
    ActmEnable();
    *result = NLSTK_OK;
    return;
}

static uint32_t StackEnable(void)
{
    uint32_t *result = SDF_MemZalloc(sizeof(uint32_t));
    NLSTK_CHECK_RETURN(result != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "stack enable malloc failed");
    uint32_t ret = SchedulePostTaskBlocked(StackEnableInner, result, NULL, DEFAULT_WAIT_TIME);
    if (ret == NLSTK_ERRCODE_TASK_TIMEOUT) {
        NLSTK_LOG_ERROR("StackEnable timeout");
        // 在超时场景下，任务并没有被删除，result的内存还会被inner函数使用，因此需要再调度一个任务去释放result
        (void)SchedulePostTask(SDF_MemFree, result, NULL);
        return NLSTK_ERRCODE_TASK_TIMEOUT;
    } else if (ret != NLSTK_OK) {
        SDF_MemFree(result);
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    ret = *result;
    SDF_MemFree(result);
    NLSTK_LOG_INFO("stack enable ret=%u", ret);
    return ret;
}

static void StackDisableInner(void *arg)
{
    (void)arg;
    DisDisable();
    ActmDisable();
    CdsmDisable();
    MicpDevDisable();
    HidDisable();
    BasDisable();
    IcceClientDisable();
    PortClientDisable();
    PortServerDisable();
    CcpDisable();
    McpVolumeDisable();
    McpMediaDisable();
}

static void StackDisable(void)
{
    uint32_t ret = SchedulePostTaskBlocked(StackDisableInner, NULL, NULL, DEFAULT_WAIT_TIME);
    if (ret == NLSTK_ERRCODE_TASK_TIMEOUT) {
        NLSTK_LOG_ERROR("StackDisable timeout");
    } else if (ret != NLSTK_OK) {
        NLSTK_LOG_ERROR("StackDisable task fail");
    } else {
        NLSTK_LOG_INFO("StackDisable success");
    }
}

// 不可重入函数，仅在单线程环境下使用
NLSTK_Errcode_E NLSTK_EnableStack(void)
{
    if (g_isStackEnabled) {
        return NLSTK_ERRCODE_SUCCESS;
    }
    uint32_t ret = DpEnable();
    if (ret != NLSTK_OK) {
        NLSTK_LOG_ERROR("DpEnable failed, ret=0x%08X\n", ret);
        return NLSTK_ERRCODE_FAIL;
    }
    NLSTK_LOG_INFO("nai enable stack start");
    ret = ReadDpInfos();
    if (ret != NLSTK_OK) {
        NLSTK_LOG_ERROR("read dp info failed");
        DpDisable();
        return NLSTK_ERRCODE_FAIL;
    }
    ret = CfgdbReadCommConfigValue();
    if (ret != NLSTK_OK) {
        NLSTK_LOG_ERROR("read cfgdb comm config value failed");
        DpDisable();
        return NLSTK_ERRCODE_FAIL;
    }
    ret = StackEnable();
    if (ret != NLSTK_OK) {
        NLSTK_LOG_ERROR("stack component init failed");
        DpDisable();
        return NLSTK_ERRCODE_FAIL;
    }
    g_isStackEnabled = true;
    NLSTK_LOG_INFO("nai enable stack end");
    return NLSTK_ERRCODE_SUCCESS;
}

void NLSTK_DisableStack(void)
{
    if (!g_isStackEnabled) {
        return;
    }
    StackDisable();
    DpDisable();
    g_isStackEnabled = false;
}

bool NLSTK_IsStackEnabled(void)
{
    return g_isStackEnabled;
}

#ifdef __cplusplus
}
#endif
