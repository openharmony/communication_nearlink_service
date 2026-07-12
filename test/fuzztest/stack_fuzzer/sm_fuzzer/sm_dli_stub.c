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
#include "dli_cmd_struct.h"

#define DLI_CP_BLOCK_TIMEOUT 3000 // ms

#ifdef __cplusplus
extern "C" {
#endif

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

    {DLI_IOB_CONNECT_REQ_EVT, (void *)DLI_IOBConnectReqCbk},
    {DLI_IOB_ESTABLISHED_EVT, (void *)DLI_IOBEstablishedCbk},
    {DLI_IOB_PARAM_REPORT_EVT, (void *)DLI_IOBReportParamCbk},
    {DLI_IOG_LABEL_REPORT_EVT, (void *)DLI_IOGLabelReportCbk},
    {DLI_IOG_UPDATE_PARAM_EVT, (void *)DLI_IOGUpdateParamCbk},
    {DLI_IMB_CONNECT_REQ_EVT, (void *)DLI_IMBConnectReqCbk},
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
    (void)param;
    DLI_SetRecvEventCallback(RecvEventHandler);
    DLI_InitEventCbkList();
    uint32_t ret = DLI_InnerEventCbkReg(g_commInnerCbkList, sizeof(g_commInnerCbkList) / sizeof(DLI_InnerCbkLineStru));
    if (ret != DLI_SUCCESS) {
        DLI_LOGE("dli inner event cbk register failed, ret=%u", ret);
        return;
    }
    return;
}

uint32_t DLI_Init(void)
{
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

static void DLI_DeInitInner(void *parma)
{
    DLI_InnerEventCbkUnReg(g_commInnerCbkList, sizeof(g_commInnerCbkList) / sizeof(DLI_InnerCbkLineStru));
    DLI_DeInitEventCbkList();
    DLI_SetRecvEventCallback(NULL);
}

void DLI_DeInit(void)
{
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

uint32_t DLI_Enable(void)
{
    return DLI_LayerInit();
}

void DLI_Disable(void)
{
    DLI_LayerDeinit();
}

DLI_ExecuteCmdCbk g_smCbkFunc[10];

int g_smCbkFuncSize;

uint32_t DLI_CmdCbkReg(const ModuleType module, const DLI_InnerCbkLineStru *innerTable, const uint32_t innerSize,
    const DLI_CbkLineStru *table, const uint32_t size)
{
    (void)innerTable;
    (void)innerSize;
    if (module != SM)
    {
        return 0;
    }
    for (uint32_t i = 0; i < size; i++) {
        g_smCbkFunc[i] = table[i].func;
    }
    g_smCbkFuncSize = size;
    return 0;
}

void DLI_CmdCbkUnReg(const ModuleType module, const DLI_InnerCbkLineStru *innerTable, const uint32_t innerSize,
    const DLI_CbkLineStru *table, const uint32_t size)
{
    (void)innerTable;
    (void)innerSize;
    if ((size == 0) || (table == NULL)) {
        return;
    }
    switch (module) {
        case DEVD:
            DLI_InnerEventCbkUnReg(g_devdInnerCbkList, sizeof(g_devdInnerCbkList) / sizeof(DLI_InnerCbkLineStru));
            break;
        case CM:
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
        default:
            DLI_LOGE("invalid module type, module = %d", module);
            break;
    }
    DLI_RemoveCbks(table, size);
}

#ifdef __cplusplus
}
#endif
