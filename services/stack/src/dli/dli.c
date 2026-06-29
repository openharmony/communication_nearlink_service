/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#include "dli.h"

#include "securec.h"

#include "dli_opcode.h"
#include "dli_errno.h"
#include "dli_log.h"
#include "dli_layer.h"
#include "dli_layer_callback.h"
#include "dli_thread.h"
#include "dli_event.h"
#include "dli_dev_discovery_event.h"
#include "dli_connect_event.h"
#include "dli_hadm_event.h"
#include "dli_secu_event.h"
#include "dli_layer_config.h"
#include "dli_nbc_event.h"
#include "sdf_mem.h"

#define DLI_CP_BLOCK_TIMEOUT 3000 // ms
#define DLI_BLOCK_TIMEOUT 4000 // ms
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t ret;
} DLI_EnableParams;

static bool g_dliIsInited = false;

static const DLI_InnerCbkLineStru g_smInnerCbkList[] = {
    {DLI_ENCRYPTION_PARAMETER_REQUEST_EVT, (void *)DLI_EncryptParamReqCbk},
    {DLI_ENCRYPTION_CHANGE_EVT, (void *)DLI_EncryptChangeCbk},
    {DLI_CONTROLLER_DATA_EVT, (void *)DLI_ControllerDataCbk},
};

static const DLI_InnerCbkLineStru g_cmInnerCbkList[] = {
    {DLI_DATA_LENGTH_CHANGE_EVT, (void *)DLI_DataLengthChangeCbk},
    {DLI_CONNECTION_COMPLETE_EVT, (void *)DLI_ConnectionCbk},
    {DLI_DISCONNECTION_COMPLETE_EVT, (void *)DLI_DisconnectionCbk},
    {DLI_READ_REMOTE_FEATURES_COMPLETE_EVT, (void *)DLI_ReadRemoteFeaturesCbk},
    {DLI_READ_REMOTE_VERSION_COMPLETE_EVT, (void *)DLI_ReadRemoteVersionCbk},
    {DLI_CONNECTION_UPDATE_EVT, (void *)DLI_ConnectionUpdateCbk},
    {DLI_REMOTE_CONNECTION_PARAMETER_REQUEST_EVT, (void *)DLI_RemoteConnParamReqCbk},
    {DLI_ACB_LOW_LATENCY_EN_EVT, (void *)DLI_AcbLowLatencyEnableCbk},
    {DLI_SET_PHY_COMPLETE_EVT, (void *)DLI_SetPhyCbk},
    {DLI_SET_ACB_EVT_PARAM_EVT, (void *)DLI_SetAcbEvtParamCbk},
    {DLI_FREQ_BAND_SWITCH_VENDOR_EVENT, (void *)DLI_FreqBandSwitchCbk},
    {DLI_IOB_CONNECT_REQ_EVT, (void *)DLI_IOBConnectReqCbk},
    {DLI_IOB_ESTABLISHED_EVT_STD, (void *)DLI_IOBEstablishedCbkStd},
    {DLI_IOB_ESTABLISHED_EVT, (void *)DLI_IOBEstablishedCbk},
    {DLI_IOB_PARAM_REPORT_EVT, (void *)DLI_IOBReportParamCbk},
    {DLI_IOG_LABEL_REPORT_EVT, (void *)DLI_IOGLabelReportCbk},
    {DLI_IOG_UPDATE_PARAM_EVT, (void *)DLI_IOGUpdateParamCbk},
    {DLI_IMB_CONNECT_REQ_EVT, (void *)DLI_IMBConnectReqCbk},
    {DLI_IMB_ESTABLISHED_EVT_STD, (void *)DLI_IMBEstablishedCbkStd},
    {DLI_IMB_ESTABLISHED_EVT, (void *)DLI_IMBEstablishedCbk},
    {DLI_IMB_PARAM_REPORT_EVT, (void *)DLI_IMBReportParamCbk},
    {DLI_IMG_LABEL_REPORT_EVT, (void *)DLI_IMGLabelReportCbk},
    {DLI_IMG_UPDATE_PARAM_EVT, (void *)DLI_IMGUpdateParamCbk},
};

static const DLI_InnerCbkLineStru g_devdInnerCbkList[] = {
    {DLI_ADVERTISING_REPORT_EVT, (void *)DLI_AdvReportCbk},
    {DLI_ADVERTISING_TERMINATED_EVT, (void *)DLI_AdvTerminatedCbk},
};

static const DLI_InnerCbkLineStru g_nbcInnerCbkList[] = {
    {DLI_CHIP_RESET_NOTIFY_EVT, (void *)DLI_ChipResetNotifyCbk},
};

static const DLI_InnerCbkLineStru g_hadmInnerCbkList[] = {
    {DLI_READ_REMOTE_MEASURE_CAPS_STATUS_VENDOR_EVT, (void *)DLI_ReadRemoteCsCapsCbk},
    {DLI_MEASURE_IQ_REPORT_VENDOR_EVT, (void *)DLI_CsIqReportCbk},
    {DLI_READ_REMOTE_MEASURE_CAPS_STATUS_EVT, (void *)DLI_ReadRemoteCsCapsCbk},
    {DLI_READ_LOCAL_MEASURE_CAPS_STATUS_EVT, (void *)DLI_ReadLocalCsCapsCbk},
    {DLI_MEASURE_IQ_REPORT_EVT, (void *)DLI_CsIqReportCbk},
    {DLI_MEASURE_STATE_CHANGE_EVT, (void *)DLI_MeasureStateChangeCbk},
};

static void DLI_VendorEventCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode);
static const DLI_InnerCbkLineStru g_commInnerCbkList[] = {
    {DLI_CMD_ERROR_EVT, (void *)DLI_CommandErrorCbk},
    {DLI_CMD_STATUS_EVT, (void *)DLI_CommandStatusCbk},
    {DLI_NUMBER_OF_COMPLETED_PACKETS_EVT, (void *)DLI_NumberOfCompletedPacketsCbk},
    {DLI_VENDOR_EVENT_EVT, (void *)DLI_VendorEventCbk},
};

static void DLI_VendorEventCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    DLI_LOGD("dispatchvendor event");
    DLI_AcbSubrateCbk(context, arg, len, evtOpcode);
    DLI_ChanInfoCbk(context, arg, len, evtOpcode);
}

static void DLI_InitInner(void *param)
{
    DLI_SetRecvEventCallback(RecvEventHandler);
    DLI_InitEventCbkList();
    uint32_t ret = DLI_InnerEventCbkReg(g_commInnerCbkList, sizeof(g_commInnerCbkList) / sizeof(DLI_InnerCbkLineStru));
    if (ret != DLI_SUCCESS) {
        DLI_LOGE("dli inner event cbk register failed, ret=%u", ret);
        return;
    }
    ret = DLI_LayerInit();
    if (ret != DLI_SUCCESS) {
        DLI_InnerEventCbkUnReg(g_commInnerCbkList, sizeof(g_commInnerCbkList) / sizeof(DLI_InnerCbkLineStru));
        DLI_LOGE("DLI_LayerInit failed, ret=%u", ret);
        return;
    }
}

uint32_t DLI_Init(void)
{
    DLI_LOGI("enter");
    if (g_dliIsInited) {
        DLI_LOGE("dli has inited");
        return DLI_STACK_INITED_ERRNO;
    }

    uint32_t ret = DLI_PostOtherBlockedThread(DLI_InitInner, NULL, NULL, DLI_CP_BLOCK_TIMEOUT);
    if (ret != DLI_SUCCESS) {
        DLI_LOGE("DLI_PostOtherBlockedThread failed, ret=%u", ret);
        return DLI_STACK_INITED_ERRNO;
    }

    g_dliIsInited = true;
    DLI_LOGI("DLI_Init success");
    return DLI_SUCCESS;
}

static void DLI_DeInitInner(void *param)
{
    DLI_LayerDeinit();
    DLI_InnerEventCbkUnReg(g_commInnerCbkList, sizeof(g_commInnerCbkList) / sizeof(DLI_InnerCbkLineStru));
    DLI_DeInitEventCbkList();
    DLI_SetRecvEventCallback(NULL);
}

void DLI_DeInit(void)
{
    DLI_LOGI("enter");
    if (!g_dliIsInited) {
        DLI_LOGE("dli has not inited");
        return;
    }

    uint32_t ret = DLI_PostOtherBlockedThread(DLI_DeInitInner, NULL, NULL, DLI_CP_BLOCK_TIMEOUT);
    if (ret != DLI_SUCCESS) {
        DLI_LOGE("DLI_PostOtherBlockedThread failed, ret=%u", ret);
        return;
    }

    g_dliIsInited = false;
    DLI_LOGI("DLI_DeInit success");
}

void DLI_EnableTask(void *arg)
{
    DLI_CHECK_RETURN(arg != NULL, "arg is null");
    DLI_EnableParams *param = (DLI_EnableParams *)arg;
    param->ret = DLI_LayerEnable();
}

uint32_t DLI_Enable(void)
{
    DLI_LOGI("enter");

    DLI_EnableParams *param = SDF_MemZalloc(sizeof(DLI_EnableParams));
    DLI_CHECK_RETURN_RET(param != NULL, DLI_STACK_MEM_ERRNO, "zalloc param failed");
    uint32_t ret = DLI_BlockPostTask(DLI_EnableTask, (void *)param, NULL, DLI_BLOCK_TIMEOUT);
    if (ret != DLI_SUCCESS) {
        if (DLI_PostTask(NULL, (void *)param, SDF_MemFree) != DLI_SUCCESS) {
            SDF_MemFree(param);
        }
        DLI_LOGE("enable failed, ret=%u", ret);
    } else {
        ret = param->ret;
        SDF_MemFree(param);
        DLI_LOGI("enable success ret %u", ret);
    }
    return ret;
}

void DLI_DisableTask(void *arg)
{
    (void)arg;
    DLI_LayerDisable();
}

void DLI_Disable(void)
{
    DLI_LOGI("enter");
    if (DLI_BlockPostTask(DLI_DisableTask, NULL, NULL, DLI_BLOCK_TIMEOUT) != DLI_SUCCESS) {
        DLI_LOGE("disable failed");
    } else {
        DLI_LOGI("disable success");
    }
}

static uint32_t DLI_InnerCbkRegByModule(const ModuleType module,
    const DLI_InnerCbkLineStru *innerTable, const uint32_t innerSize)
{
    uint32_t ret = DLI_SUCCESS;
    switch (module) {
        case DEVD:
            ret = DLI_InnerEventCbkReg(g_devdInnerCbkList, sizeof(g_devdInnerCbkList) / sizeof(DLI_InnerCbkLineStru));
            break;
        case CM:
        case CM_ICB:
        case CM_COMMON:
            ret = DLI_InnerEventCbkReg(g_cmInnerCbkList, sizeof(g_cmInnerCbkList) / sizeof(DLI_InnerCbkLineStru));
            break;
        case SM:
            ret = DLI_InnerEventCbkReg(g_smInnerCbkList, sizeof(g_smInnerCbkList) / sizeof(DLI_InnerCbkLineStru));
            break;
        case HADM:
            ret = DLI_InnerEventCbkReg(g_hadmInnerCbkList, sizeof(g_hadmInnerCbkList) / sizeof(DLI_InnerCbkLineStru));
            break;
        case NBC:
            ret = DLI_InnerEventCbkReg(g_nbcInnerCbkList, sizeof(g_nbcInnerCbkList) / sizeof(DLI_InnerCbkLineStru));
            break;
        case QOSM:
            break;
        default:
            ret = DLI_INVALID_PARAMETERS;
            DLI_LOGW("register a new module %u, need with innerTable", module);
            break;
    }
    return ret;
}

static void DLI_InnerCbkUnregByModule(const ModuleType module,
    const DLI_InnerCbkLineStru *innerTable, const uint32_t innerSize)
{
    switch (module) {
        case DEVD:
            DLI_InnerEventCbkUnReg(g_devdInnerCbkList, sizeof(g_devdInnerCbkList) / sizeof(DLI_InnerCbkLineStru));
            break;
        case CM:
        case CM_ICB:
        case CM_COMMON:
            DLI_InnerEventCbkUnReg(g_cmInnerCbkList, sizeof(g_cmInnerCbkList) / sizeof(DLI_InnerCbkLineStru));
            break;
        case SM:
            DLI_InnerEventCbkUnReg(g_smInnerCbkList, sizeof(g_smInnerCbkList) / sizeof(DLI_InnerCbkLineStru));
            break;
        case HADM:
            DLI_InnerEventCbkUnReg(g_hadmInnerCbkList, sizeof(g_hadmInnerCbkList) / sizeof(DLI_InnerCbkLineStru));
            break;
        case NBC:
            DLI_InnerEventCbkUnReg(g_nbcInnerCbkList, sizeof(g_nbcInnerCbkList) / sizeof(DLI_InnerCbkLineStru));
            break;
        case QOSM:
            break;
        default:
            DLI_LOGE("invalid module type, module = %d", module);
            break;
    }
}

static uint32_t DLI_CheckCmdCbkParams(const ModuleType module,
    const DLI_InnerCbkLineStru *innerTable, const uint32_t innerSize,
    const DLI_CbkLineStru *table, const uint32_t size)
{
    DLI_CHECK_RETURN_RET(module >= DEVD && module < EXT_END, DLI_INVALID_PARAMETERS,
        "register module %u error", module);
    DLI_CHECK_RETURN_RET(!(innerTable == NULL && innerSize != 0), DLI_INVALID_PARAMETERS,
        "innerTable is null, but innerSize not is 0");
    DLI_CHECK_RETURN_RET(!(innerTable != NULL && innerSize == 0), DLI_INVALID_PARAMETERS,
        "innerTable is not null, but innerSize is 0");
    DLI_CHECK_RETURN_RET(table != NULL, DLI_INVALID_PARAMETERS, "table is null");
    DLI_CHECK_RETURN_RET(size != 0, DLI_INVALID_PARAMETERS, "size %u is err", size);
    return DLI_SUCCESS;
}

uint32_t DLI_CmdCbkReg(const ModuleType module, const DLI_InnerCbkLineStru *innerTable, const uint32_t innerSize,
    const DLI_CbkLineStru *table, const uint32_t size)
{
    uint32_t ret = DLI_SUCCESS;
    if (DLI_CheckCmdCbkParams(module, innerTable, innerSize, table, size) != DLI_SUCCESS) {
        return DLI_INVALID_PARAMETERS;
    }

    if (innerTable != NULL && innerSize != 0) {
        DLI_LOGW("register a new module type, module = %d", module);
        ret = DLI_InnerEventCbkReg(innerTable, innerSize);
    } else {
        ret = DLI_InnerCbkRegByModule(module, innerTable, innerSize);
    }
    if (ret != DLI_SUCCESS) {
        DLI_LOGW("register module type failed, module = %d", module);
        return ret;
    }
    DLI_AddCbks(table, size);
    return DLI_SUCCESS;
}

void DLI_CmdCbkUnReg(const ModuleType module, const DLI_InnerCbkLineStru *innerTable, const uint32_t innerSize,
    const DLI_CbkLineStru *table, const uint32_t size)
{
    if (DLI_CheckCmdCbkParams(module, innerTable, innerSize, table, size) != DLI_SUCCESS) {
        return;
    }
    if (innerTable != NULL && innerSize != 0) {
        DLI_LOGW("unregister a new module type, module = %d", module);
        DLI_InnerEventCbkUnReg(innerTable, innerSize);
    } else {
        DLI_InnerCbkUnregByModule(module, innerTable, innerSize);
    }
    DLI_RemoveCbks(table, size);
}

#ifdef __cplusplus
}
#endif