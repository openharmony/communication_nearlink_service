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
#include "dli_cmd.h"

#include "securec.h"

#include "sdf_addr.h"
#include "sdf_mem.h"
#include "dli_log.h"
#include "dli_opcode.h"
#include "dli_errno.h"
#include "dli_def.h"
#include "dli_cmd_struct.h"
#include "dli_event_struct.h"
#include "dli_layer.h"
#include "dli_layer_stru.h"
#include "dli_event.h"
#include "dli_dev_discovery_event.h"
#include "dli_secu_event.h"
#include "dli_connect_event.h"
#include "dli_layer_utils.h"
#include "dli_layer_config.h"
#include "dli_sapi.h"

#define DLI_DATA_OFFSET 4
#define DLI_DEFAULT_OPCODE 0
#define DEFAULT_FRAME_TYPE_4_CONN_SCAN_WINDOW 480
#define DEFAULT_FRAME_TYPE_4_CONN_SCAN_INTERVAL 480
#define SCAN_PHY_COUNTT_MAX 2

// 新增Bit72定义, 增加白名单列表设备V2特性是否支持
#define DLI_LOCAL_FEATURE_INDEX_9 9
#define DLI_LOCAL_FEATURE_IS_SUPPORT_CONN_BYPASS_ADV 0x01

// 系统初始化阶段由NBC模块读取本端特性在回调后进行设置才能读取，否则无效
static DLI_LocalFeatures_S g_dliLocalFeatures = { 0 };

static void DLI_ExecuteCommandCbkDo(
    DLI_ManagerContext *context, void *arg, uint32_t argLen, uint16_t evtOpcode, uint16_t status)
{
    DLI_LOGD("dli execute command cbk do status = 0x%04X", status);
    if (context->cbk != NULL) {
        DLI_ExecuteCmdRetParam par;
        // 完成事件，evtOpcode = opcode = cmd
        par.cmdOpcode = evtOpcode;
        // 公共事件需要去掉4个字节的公共字段，只传递数据
        if (status == DLI_SUCCESS && arg != NULL && argLen > DLI_DATA_OFFSET) {
            par.size = argLen - DLI_DATA_OFFSET;
            par.eventParameter = (uint8_t *)arg + DLI_DATA_OFFSET;
        } else {
            par.size = 0;
            par.eventParameter = NULL;
        }
        // 回调到cbk，cbk是在发送命令时保存在context中的
        context->cbk(context->cbkContext, status, &par);
    }
}

static void DLI_ExecuteCommandCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    // context param在调用完后会清理
    DLI_LOGD("execute DLI_CMD_COMPLETE_EVT cbk enter, evtOpcode: 0x%04X", evtOpcode);
    DLI_CHECK_RETURN(context != NULL, "context is null");
    DLI_ManagerContext *managerContext = (DLI_ManagerContext *)context;
    uint16_t status = DLI_SUCCESS;
    // 完成事件的parameters[0]字段一定是status
    if (arg != NULL && len >= sizeof(DLI_CommandComplete)) {
        status = ((DLI_CommandComplete *)arg)->parameters[0];
    }
    DLI_InnerEventCbkUnReg(managerContext->innerCbkTable, 1);
    DLI_ExecuteCommandCbkDo(managerContext, arg, len, evtOpcode, status);
}

static DLI_ManagerContext *DLI_CreateManagerContext(
    uint16_t cmd, uint16_t event, DLI_ExecuteCmdCbk cbk, void *cbkContext, uint16_t cbkContextLen)
{
    DLI_ManagerContext *managerContext = (DLI_ManagerContext *)SDF_MemZalloc(sizeof(DLI_ManagerContext));
    if (managerContext == NULL) {
        DLI_LOGE("managerContext new fail");
        return NULL;
    }

    if (event == DLI_CMD_COMPLETE_EVT) {
        DLI_InnerCbkLineStru *innerCbkTable = (DLI_InnerCbkLineStru *)SDF_MemAlloc(sizeof(DLI_InnerCbkLineStru));
        if (innerCbkTable == NULL) {
            DLI_LOGE("innerCbkTable new fail");
            SDF_MemFree(managerContext);
            return NULL;
        }
        innerCbkTable->opcode = cmd;
        innerCbkTable->func = (void *)DLI_ExecuteCommandCbk;
        // 动态注册完成事件的DLI内部处理方法(各个模块不会自己注册，这是属于公共处理)
        DLI_InnerEventCbkReg(innerCbkTable, 1);
        managerContext->innerCbkTable = innerCbkTable;
    }

    managerContext->cbk = cbk;

    if (cbkContext == NULL || cbkContextLen == 0) {
        return managerContext;
    }
    void *context = (void *)SDF_MemAlloc(cbkContextLen);
    if (context == NULL) {
        DLI_LOGE("cbkContext new fail");
        return managerContext;
    }
    // cbkContext由控制面释放
    (void)memcpy_s(context, cbkContextLen, cbkContext, cbkContextLen);
    managerContext->cbkContext = context;

    return managerContext;
}

void TimeoutCallback(void *param)
{
    /**
     * 状态事件失败成功，都会更新num；超时，删除节点(layer中)，没更新num(cmdNode->info->event设置的是最终事件，情况极少)
     * 完成事件失败成功，都会更新num；超时，删除节点(layer中)，更新num
     * 最终事件不用更新num（状态事件中已经更新）；超时，会删除节点(layer中)，不需要更新num（状态事件中已经更新）
     * 自主上报事件不会进入超时分支，不需要考虑
     */
    DLI_CHECK_RETURN(param, "param is null");
    DLI_CmdTxNode *cmdNode = (DLI_CmdTxNode *)param;
    DLI_CommandErrorStru errEvt = {0};
    errEvt.status = DLI_COMMAND_TIMEOUT;
    errEvt.cmd = cmdNode->info->cmd;
    errEvt.event = cmdNode->info->event;
    DLI_LOGI("TimeoutCallback cmd: 0x%04X, event: 0x%04X", cmdNode->info->cmd, cmdNode->info->event);
    if (cmdNode->info->event == DLI_CMD_COMPLETE_EVT) {
        DLI_CmdNumSet(DLI_DEFAULT_CMD_NUM);
    } else {
        // 区分是状态事件超时还是最终事件超时：isRecvStatusEvt=true，表示收到了状态事件，就是最终事件超时，否则就是状态事件超时
        if (!cmdNode->isRecvStatusEvt) {
            DLI_CmdNumSet(DLI_DEFAULT_CMD_NUM);
        }
    }
    // 回调到0x00ee中通知上层模块超时
    RecvEventHandler(DLI_CMD_ERROR_EVT, cmdNode->info->context, (void *)&errEvt, sizeof(DLI_CommandErrorStru));
}

void ContextFree(void *context)
{
    // 回调处理完成后释放context
    DLI_CHECK_RETURN(context, "context is null");
    DLI_ManagerContext *managerContext = (DLI_ManagerContext *)context;
    SDF_MemFree(managerContext->cbkContext);
    managerContext->cbkContext = NULL;
    SDF_MemFree(managerContext->innerCbkTable);
    managerContext->innerCbkTable = NULL;
    SDF_MemFree(managerContext);
}

bool DLI_IsSupportNewDisMeasure(void)
{
    int version = DLI_GetDliVersion();
    if (version == DLI_VERSION_1_1) {
        return true;
    }
    return false;
}

static bool DLI_NeedEraseCmd(uint16_t cmd)
{
    return cmd == DLI_ENABLE_ENCRYPTION;
}

static uint32_t DLI_ExecuteCommand(uint16_t cmd, uint16_t event, void *inParam, uint16_t paramLen,
    DLI_ExecuteCmdCbk cbk, void *cbkContext, uint16_t cbkContextLen)
{
    DLI_LOGI("dli execute cmd: 0x%04X, event: 0x%04X, paramLen: %hu", cmd, event, paramLen);
    if (cmd > DLI_TEST_END || cmd < DLI_SET_EVENT_MASK || (event == DLI_CMD_COMPLETE_EVT && cbk == NULL)) {
        DLI_LOGE("arg is invalid");
        return DLI_STACK_PARAMS_ERRNO;
    }
    DLI_CmdStru *cmdStruct = DLI_DefaultCmdStruCreate(cmd, event, inParam, paramLen);
    if (cmdStruct == NULL) {
        DLI_LOGE("cmdStruct 0x%04X new fail", cmd);
        return DLI_STACK_MEM_ERRNO;
    }
    cmdStruct->needErase = DLI_NeedEraseCmd(cmdStruct->cmd);

    DLI_ManagerContext *managerContext = DLI_CreateManagerContext(cmd, event, cbk, cbkContext, cbkContextLen);
    if (managerContext == NULL) {
        DLI_LOGE("create managerContext fail");
        DLI_EraseParInfo(cmdStruct);
        SDF_MemFree(cmdStruct);
        return DLI_STACK_MEM_ERRNO;
    }

    cmdStruct->context = managerContext;
    cmdStruct->contextFree = ContextFree;
    cmdStruct->timeoutCallback = TimeoutCallback;

    uint32_t ret = DLI_CmdSend(cmdStruct);
    if (ret != DLI_SUCCESS) {
        DLI_EraseParInfo(cmdStruct);
        SDF_MemFree(cmdStruct);
        ContextFree(managerContext);
        DLI_LOGE("send dli cmd:0x%04X failed, ret =%u", cmd, ret);
        return ret;
    }
    DLI_LOGD("send dli cmd:0x%04X success", cmd);
    return ret;
}

void DLI_ReadBufferSizeCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    (void)context;
    DLI_LOGI("enter read buffer size cbk");
    DLI_CHECK_RETURN(cmdRes != NULL && cmdRes->eventParameter != NULL, "arg is nullptr");
    DLI_ReadBufSizeEvt *param = (DLI_ReadBufSizeEvt *)cmdRes->eventParameter;
    if (status != DLI_SUCCESS) {
        DLI_LOGE("read buffer size fail, status = 0x%04X", status);
        return;
    }
    DLI_AllDataSet(param->acbTxDataLen, param->acbTxDataNum, param->icbTxDataLen, param->icbTxDataNum);
}

uint32_t DLI_SetPublicAddress(uint8_t *addr)
{
    DLI_CHECK_RETURN_RET(addr, DLI_STACK_PARAMS_ERRNO, "addr is null");
    DLI_AddrStru cmd = {0};
    (void)memcpy_s(cmd.addr, SLE_ADDR_LEN, addr, SLE_ADDR_LEN);

    uint32_t ret = DLI_ExecuteCommand(DLI_SET_PUBLIC_ADDRESS,
        DLI_CMD_COMPLETE_EVT,
        &cmd,
        sizeof(DLI_AddrStru),
        DLI_GetCbk(DLI_CBK_SET_PUBLIC_ADDRESS),
        NULL,
        0);
    DLI_LOGD("set public address ret = %u", ret);
    return ret;
}

uint32_t DLI_SetHostChannelClassification(uint8_t *channelMap, uint32_t len)
{
    DLI_CHECK_RETURN_RET(channelMap && (len == CHANNEL_MAP_LEN), DLI_STACK_PARAMS_ERRNO, "channelMap is null");
    uint32_t ret = DLI_ExecuteCommand(DLI_SET_HOST_CHANNEL_CLASSIFICATION,
        DLI_CMD_COMPLETE_EVT,
        channelMap,
        sizeof(uint8_t) * CHANNEL_MAP_LEN,
        DLI_GetCbk(DLI_CBK_SET_HOST_CHANNEL_CLASSIFICATION),
        NULL,
        0);
    DLI_LOGD("set host channel classification ret = %u", ret);
    return ret;
}

uint32_t DLI_ReadBufferSize(void)
{
    uint32_t ret =
        DLI_ExecuteCommand(DLI_READ_LOCAL_BUFFER, DLI_CMD_COMPLETE_EVT, NULL, 0, DLI_ReadBufferSizeCbk, NULL, 0);
    DLI_LOGD("read buffer size ret = %u", ret);
    return ret;
}

uint32_t DLI_ReadLocalFeatures(void)
{
    uint32_t ret = DLI_ExecuteCommand(
        DLI_READ_LOCAL_SUPPORT_FEATS, DLI_CMD_COMPLETE_EVT, NULL, 0, DLI_GetCbk(DLI_CBK_READ_LOCAL_FEATURE), NULL, 0);
    DLI_LOGD("read local features ret = %u", ret);
    return ret;
}

void DLI_SetLocalFeatures(DLI_LocalFeatures_S *features)
{
    DLI_CHECK_RETURN(features != NULL, "features is null");
    (void)memcpy_s(&g_dliLocalFeatures, sizeof(DLI_LocalFeatures_S), features, sizeof(DLI_LocalFeatures_S));
}

uint32_t DLI_ReadLocalVersion(void)
{
    uint32_t ret = DLI_ExecuteCommand(DLI_READ_LOCAL_VERSION_INFORMATION,
        DLI_CMD_COMPLETE_EVT,
        NULL,
        0,
        DLI_GetCbk(DLI_CBK_READ_LOCAL_VERSION),
        NULL,
        0);
    DLI_LOGD("read local version ret = %u", ret);
    return ret;
}

uint32_t DLI_GetPublicAddress(DLI_PublicAddrParam *param)
{
    DLI_CHECK_RETURN_RET(param, DLI_STACK_PARAMS_ERRNO, "param is null");
    uint32_t ret = DLI_ExecuteCommand(DLI_GET_PUBLIC_ADDRESS,
        DLI_CMD_COMPLETE_EVT,
        param,
        sizeof(DLI_PublicAddrParam),
        DLI_GetCbk(DLI_CBK_GET_PUBLIC_ADDRESS),
        NULL,
        0);
    DLI_LOGD("get public address ret = %u", ret);
    return ret;
}

uint32_t DLI_ReadCommConfigValue(void)
{
    uint32_t readBufSizeRet = DLI_ReadBufferSize();
    if (readBufSizeRet != DLI_SUCCESS) {
        return DLI_STACK_READ_COMM_CONFIG_VAL_ERRNO;
    }
    return DLI_SUCCESS;
}

/* ---------------------------------------------设备发现命令-------------------------------------------------------*/

uint8_t DLI_GetPhyCountByFrameType(uint8_t frameType)
{
    uint8_t phyCount = 0;
    if ((frameType & SCAN_FRAME_TYPE_1) != 0) {
        phyCount++;
    }
    if ((frameType & SCAN_FRAME_TYPE_4) != 0) {
        phyCount++;
    }
    return phyCount;
}

uint32_t DLI_SetScanParam(DLI_ScanParam *scanParam)
{
    DLI_CHECK_RETURN_RET(scanParam, DLI_STACK_PARAMS_ERRNO, "scanParam is null");

    uint8_t frameType = scanParam->frameFormatInd;
    uint8_t phyCount = DLI_GetPhyCountByFrameType(frameType);

    size_t scanParamSize = sizeof(DLI_ScanParam) + phyCount * sizeof(DLI_ScanParamCoreNoPhy);

    DLI_ScanParam *cmd = (DLI_ScanParam *)SDF_MemZalloc(scanParamSize);
    DLI_CHECK_RETURN_RET(cmd != NULL, DLI_STACK_MEM_ERRNO, "cmd malloc error");

    cmd->ownAddrType = scanParam->ownAddrType;
    cmd->scanFilterPolicy = scanParam->scanFilterPolicy;
    cmd->frameFormatInd = frameType;

    for (uint8_t i = 0; i < phyCount; i++) {
        cmd->param[i].scanType = scanParam->param[i].scanType;
        cmd->param[i].scanInterval = scanParam->param[i].scanInterval;
        cmd->param[i].scanWindow = scanParam->param[i].scanWindow;
    }
    DLI_LOGI("frameType: %02x, scanPhyCount: %u", frameType, phyCount);
    uint8_t idx = 0;
    if ((cmd->frameFormatInd & SCAN_FRAME_TYPE_1) != 0) {
        DLI_LOGI("frameType1, idx: %u, scanType: %u, interval: %u, window: %u", idx,
            cmd->param[idx].scanType, cmd->param[idx].scanInterval, cmd->param[idx].scanWindow);
        idx++;
    }
    if ((cmd->frameFormatInd & SCAN_FRAME_TYPE_4) != 0) {
        DLI_LOGI("frameType4, idx: %u, scanType: %u, interval: %u, window: %u", idx,
            cmd->param[idx].scanType, cmd->param[idx].scanInterval, cmd->param[idx].scanWindow);
        idx++;
    }
    uint32_t ret = DLI_ExecuteCommand(DLI_SET_SCAN_PARAMETERS,
        DLI_CMD_COMPLETE_EVT,
        cmd,
        scanParamSize,
        DLI_GetCbk(DLI_CBK_SET_SCAN_PARAMS),
        NULL,
        0);
    SDF_MemFree(cmd);
    DLI_LOGD("set scan param ret = %u", ret);
    return ret;
}

uint32_t DLI_EnableScan(DLI_ScanEnable *scanEnable)
{
    DLI_CHECK_RETURN_RET(scanEnable, DLI_STACK_PARAMS_ERRNO, "scanEnable is null");
    DLI_LOGI("scan enable = 0x%x, filterDuplicates = 0x%x", scanEnable->enable, scanEnable->filterDuplicates);

    uint32_t ret = DLI_ExecuteCommand(DLI_SET_SCAN_ENABLE,
        DLI_CMD_COMPLETE_EVT,
        scanEnable,
        sizeof(DLI_ScanEnable),
        DLI_GetCbk(DLI_CBK_ENABLE_SCAN),
        NULL,
        0);
    DLI_LOGD("set scan enable ret = %u", ret);
    return ret;
}

uint32_t DLI_ReadMaximumAdvDataLen(void)
{
    uint32_t ret = DLI_ExecuteCommand(DLI_READ_MAXIMUM_ADVERTISING_DATA_LENGTH,
        DLI_CMD_COMPLETE_EVT,
        NULL,
        0,
        DLI_GetCbk(DLI_CBK_READ_ADV_DATA_LEN),
        NULL,
        0);
    DLI_LOGD("read max adv data len ret = %u", ret);
    return ret;
}

uint32_t DLI_SetAdvParam(DLI_AdvParam *advParam)
{
    DLI_CHECK_RETURN_RET(advParam, DLI_STACK_PARAMS_ERRNO, "advParam is null");
    DLI_AdvCbkContext cbkContext = {.advHandle = advParam->advHandle};

    uint32_t ret = DLI_ExecuteCommand(DLI_SET_ADVERTISING_PARAMETERS,
        DLI_CMD_COMPLETE_EVT,
        advParam,
        sizeof(DLI_AdvParam),
        DLI_GetCbk(DLI_CBK_SET_ADV_PARAMS),
        &cbkContext,
        sizeof(DLI_AdvCbkContext));
    DLI_LOGI("set adv param ret = %u, advHandle:0x%02x, advMode:0x%02x, advGtRole:0x%02x, primAdvChannelMap:0x%02x, "
        "primAdvFrameFormat:0x%02x, secondAdvFrameFormat:0x%02x, secondAdvPhy:0x%02x, secondAdvPilot:0x%02x, "
        "secondAdvMcs:0x%02x, secondAdvMaxSkip:0x%02x, advSid:0x%02x",
        ret, advParam->advHandle, advParam->advMode, advParam->advGtRole, advParam->primAdvChannelMap,
        advParam->primAdvFrameFormat, advParam->secondAdvFrameFormat, advParam->secondAdvPhy, advParam->secondAdvPilot,
        advParam->secondAdvMcs, advParam->secondAdvMaxSkip, advParam->advSid);
    DLI_LOGI("connIntervalMin:0x%04x, connIntervalMax:0x%04x, "
        "maxLatency:0x%04x,supervisionTimeout:0x%04x, minCeLength:0x%04x, maxCeLength:0x%04x",
        advParam->connIntervalMin, advParam->connIntervalMax, advParam->maxLatency,
        advParam->supervisionTimeout, advParam->minCeLength, advParam->maxCeLength);
    return ret;
}

uint32_t DLI_SetAdvData(DLI_AdvData *advData)
{
    DLI_CHECK_RETURN_RET(advData && advData->advDataLen, DLI_STACK_PARAMS_ERRNO, "adv data is null");
    DLI_LOGI("dataLen = %hu, operation = 0x%02X", advData->advDataLen, advData->operation);

    uint8_t dataLen = advData->advDataLen;
    uint32_t cmdSize = (uint32_t)sizeof(DLI_AdvData) + dataLen;
    DLI_AdvData *cmd = (DLI_AdvData *)SDF_MemZalloc(cmdSize);
    DLI_CHECK_RETURN_RET(cmd != NULL, DLI_STACK_MEM_ERRNO, "cmd malloc error");

    cmd->advHandle = advData->advHandle;
    cmd->operation = advData->operation;
    cmd->selection = advData->selection;
    cmd->advDataLen = advData->advDataLen;
    (void)memcpy_s(cmd->advData, dataLen, advData->advData, dataLen);

    DLI_AdvCbkContext cbkContext = {0};
    cbkContext.advHandle = advData->advHandle;
    uint32_t ret = DLI_ExecuteCommand(DLI_SET_ADVERTISING_DATA,
        DLI_CMD_COMPLETE_EVT,
        cmd,
        cmdSize,
        DLI_GetCbk(DLI_CBK_SET_ADV_DATA),
        &cbkContext,
        sizeof(DLI_AdvCbkContext));
    SDF_MemFree(cmd);
    DLI_LOGD("set adv data ret = %u", ret);
    return ret;
}

uint32_t DLI_SetScanRspData(DLI_ScanRspData *scanRspData)
{
    DLI_CHECK_RETURN_RET(scanRspData && scanRspData->scanRspDataLen, DLI_STACK_PARAMS_ERRNO, "scan rsp data is null");
    DLI_LOGI("dataLen = %hu, operation = 0x%02X", scanRspData->scanRspDataLen, scanRspData->operation);

    uint8_t dataLen = scanRspData->scanRspDataLen;
    uint32_t cmdSize = (uint32_t)sizeof(DLI_ScanRspData) + dataLen;
    DLI_ScanRspData *cmd = (DLI_ScanRspData *)SDF_MemZalloc(cmdSize);
    DLI_CHECK_RETURN_RET(cmd != NULL, DLI_STACK_MEM_ERRNO, "cmd malloc error");

    cmd->advHandle = scanRspData->advHandle;
    cmd->operation = scanRspData->operation;
    cmd->selection = scanRspData->selection;
    cmd->scanRspDataLen = scanRspData->scanRspDataLen;
    (void)memcpy_s(cmd->scanRspData, dataLen, scanRspData->scanRspData, dataLen);

    DLI_AdvCbkContext cbkContext = {0};
    cbkContext.advHandle = scanRspData->advHandle;
    uint32_t ret = DLI_ExecuteCommand(DLI_SET_SCAN_RESPONSE_DATA,
        DLI_CMD_COMPLETE_EVT,
        cmd,
        cmdSize,
        DLI_GetCbk(DLI_CBK_SET_ADV_DATA),
        &cbkContext,
        sizeof(DLI_AdvCbkContext));
    SDF_MemFree(cmd);
    DLI_LOGD("set scan rsp data ret = %u", ret);
    return ret;
}

uint32_t DLI_EnableAdv(uint8_t advHandle, DLI_AdvEnable *advEnable)
{
    DLI_SetAdvertisingEnable cmd = {0};
    DLI_AdvCbkContext cbkContext = {0};

    cbkContext.advHandle = advHandle;
    cmd.advHandle = advHandle;
    /* 禁用广播时参数可以为空 */
    if (advEnable != NULL) {
        cmd.enable = advEnable->enable;
        cmd.duration = advEnable->duration;
        cmd.maxAdvEvents = advEnable->maxAdvEvents;
    }
    DLI_LOGI("advHandle = 0x%02X, advEnable = %u", cmd.advHandle, cmd.enable);
    uint32_t ret = DLI_ExecuteCommand(DLI_SET_ADVERTISING_ENABLE,
        DLI_CMD_COMPLETE_EVT,
        &cmd,
        sizeof(cmd),
        DLI_GetCbk(DLI_CBK_ENABLE_ADV),
        &cbkContext,
        sizeof(DLI_AdvCbkContext));
    DLI_LOGD("set adv enable ret = %u", ret);
    return ret;
}

uint32_t DLI_ReadAdvSetsNum(void)
{
    uint32_t ret = DLI_ExecuteCommand(DLI_READ_NUMBER_OF_SUPPORTED_ADVERTISING_SETS,
        DLI_CMD_COMPLETE_EVT,
        NULL,
        0,
        DLI_GetCbk(DLI_CBK_READ_ADV_SETS_NUM),
        NULL,
        0);
    DLI_LOGD("read adv sets num ret = %u", ret);
    return ret;
}

uint32_t DLI_RemoveAdvSet(uint8_t advHandle)
{
    DLI_RemoveAdvertisingSet cmd = {0};
    DLI_AdvCbkContext cbkContext = {0};

    DLI_LOGI("advHandle = 0x%02X", advHandle);
    cbkContext.advHandle = advHandle;
    cmd.advHandle = advHandle;
    uint32_t ret = DLI_ExecuteCommand(DLI_REMOVE_ADVERTISING_SET,
        DLI_CMD_COMPLETE_EVT,
        &cmd,
        sizeof(cmd),
        DLI_GetCbk(DLI_CBK_REMOVE_ADV),
        &cbkContext,
        sizeof(DLI_AdvCbkContext));
    DLI_LOGD("remove adv set ret = %u", ret);
    return ret;
}

/* ---------------------------------------------连接命令-------------------------------------------------------*/

static uint32_t VersionLocalIndexPack(uint8_t version, uint16_t localIndex)
{
    return (((uint32_t)(localIndex) << SLE_SHIFT_BITS_16) | (version));
}

uint32_t DLI_ReadAcceptFilterListSize(void)
{
    uint32_t ret = DLI_ExecuteCommand(DLI_READ_ACCESS_FILTER_LIST_SIZE,
        DLI_CMD_COMPLETE_EVT,
        NULL,
        0,
        DLI_GetCbk(DLI_CBK_READ_ACCESS_FLT_LIST_SIZE),
        NULL,
        0);
    DLI_LOGD("read accept filter list size ret = %u", ret);
    return ret;
}

uint32_t DLI_ClearAcceptFilterList(void)
{
    uint32_t ret = DLI_ExecuteCommand(DLI_CLEAR_ACCESS_FILTER_LIST,
        DLI_CMD_COMPLETE_EVT,
        NULL,
        0,
        DLI_GetCbk(DLI_CBK_CLEAR_ACCESS_FLT_LIST),
        NULL,
        0);
    DLI_LOGD("clear accept filter list ret = %u", ret);
    return ret;
}

uint32_t DLI_AddDeviceToAcceptFilterList(SLE_Addr_S *addr)
{
    DLI_CHECK_RETURN_RET(addr != NULL, DLI_STACK_PARAMS_ERRNO, "addr is null");
    DLI_AddrStru addDevice = {0};
    (void)memcpy_s(addDevice.addr, SLE_ADDR_LEN, addr->addr, SLE_ADDR_LEN);

    uint32_t ret = DLI_ExecuteCommand(DLI_ADD_DEVICE_TO_ACCESS_FILTER_LIST,
        DLI_CMD_COMPLETE_EVT,
        &addDevice,
        sizeof(DLI_AddrStru),
        DLI_GetCbk(DLI_CBK_ADD_DEV_TO_ACCESS_FLT_LIST),
        NULL,
        0);
    DLI_LOGD("add device to accept filter list ret = %u", ret);
    return ret;
}

bool DLI_IsSupportConnBypassAdv(void)
{
    if ((g_dliLocalFeatures.feats[DLI_LOCAL_FEATURE_INDEX_9] &
        DLI_LOCAL_FEATURE_IS_SUPPORT_CONN_BYPASS_ADV) == DLI_LOCAL_FEATURE_IS_SUPPORT_CONN_BYPASS_ADV) {
        return true;
    }
    return false;
}

uint32_t DLI_RemoveDeviceFromAcceptFilterList(SLE_Addr_S *addr)
{
    DLI_CHECK_RETURN_RET(addr, DLI_STACK_PARAMS_ERRNO, "addr is null");
    DLI_LOGI("remove device from access filter list, addr: %s", GET_ENC_ADDR(addr));

    DLI_AddrStru addDevice = {0};

    (void)memcpy_s(addDevice.addr, SLE_ADDR_LEN, addr->addr, SLE_ADDR_LEN);

    uint32_t ret = DLI_ExecuteCommand(DLI_REMOVE_DEVICE_FROM_ACCESS_FILTER_LIST,
        DLI_CMD_COMPLETE_EVT,
        &addDevice,
        sizeof(DLI_AddrStru),
        DLI_GetCbk(DLI_CBK_RMV_DEV_FROM_ACCESS_FLT_LIST),
        NULL,
        0);
    DLI_LOGD("remove device from accept filter list ret = %u", ret);
    return ret;
}

static void DLI_BuildCreateMultiConnParam(DLI_ConnectionCreateParam *param, DLI_ConnectionCreateParamMultiInd *cmd)
{
    uint8_t scanFrameType = DLI_SCAN_BIT_FRAME_FORMAT_1_IND | DLI_SCAN_BIT_FRAME_FORMAT_4_IND;
    uint8_t connFrameType = CONN_BIT_FRAME_FORMAT_1_IND | CONN_BIT_FRAME_FORMAT_4_IND;

    cmd->initiatorFilterPolicy = param->initiatorFilterPolicy;
    cmd->ownAddressType = param->ownAddressType;
    cmd->peerAddressType = param->peerAddressType;
    (void)memcpy_s(cmd->peerAddress, SLE_ADDR_LEN, param->peerAddress, SLE_ADDR_LEN);
    cmd->gtNegotiateInd = param->gtNegotiateInd;
    cmd->scanFrameFormatInd = scanFrameType;
    cmd->connePhy = 0;
    cmd->connPilot = DEFAULT_CONN_PILOT;
    cmd->connFrameFormatMSeqInd = DEFAULT_FRAME_FORMAT_M_SEQ_IND;
    cmd->connFrameFormatInd = connFrameType;

    cmd->scanParam[0].scanInterval = param->scanInterval;
    cmd->scanParam[0].scanWindow = param->scanWindow;
    cmd->scanParam[1].scanInterval = DEFAULT_FRAME_TYPE_4_CONN_SCAN_INTERVAL;
    cmd->scanParam[1].scanWindow = DEFAULT_FRAME_TYPE_4_CONN_SCAN_WINDOW;

    cmd->connParam[0].connIntervalMin = param->connectionIntervalMin;
    cmd->connParam[0].connIntervalMax = param->connectionIntervalMax;
    cmd->connParam[0].supervisionTimeout = param->supervisionTimeout;
    cmd->connParam[1].connIntervalMin = DEFAULT_FRAME_4_CONN_INTERVAL;
    cmd->connParam[1].connIntervalMax = DEFAULT_FRAME_4_CONN_INTERVAL;
    cmd->connParam[1].supervisionTimeout = DEFAULT_FRAME_TYPE_4_ADV_TIMEOUT;

    for (uint8_t i = 0; i < CONN_VALID_MULTI_OF_FRAME_TYPE_IND; i++) {
        cmd->connParam[i].maxLatency = param->maxLatency;
        cmd->connParam[i].minCeLength = param->minCeLength;
        cmd->connParam[i].maxCeLength = param->maxCeLength;
    }
}

static void DLI_BuildCreateSingleConnParam(DLI_ConnectionCreateParam *param, DLI_ConnectionCreateParamSingleInd *cmd)
{
    uint8_t scanFrameType = param->bitFrameType;
    uint8_t connFrameType = (param->bitFrameType == DLI_SCAN_BIT_FRAME_FORMAT_4_IND) ?
        CONN_BIT_FRAME_FORMAT_4_IND : DLI_SCAN_BIT_FRAME_FORMAT_1_IND;

    cmd->initiatorFilterPolicy = param->initiatorFilterPolicy;
    cmd->ownAddressType = param->ownAddressType;
    cmd->peerAddressType = param->peerAddressType;
    (void)memcpy_s(cmd->peerAddress, SLE_ADDR_LEN, param->peerAddress, SLE_ADDR_LEN);
    cmd->gtNegotiateInd = param->gtNegotiateInd;
    cmd->scanFrameFormatInd = scanFrameType;
    cmd->connePhy = 0;
    cmd->connPilot = DEFAULT_CONN_PILOT;
    cmd->connFrameFormatMSeqInd = DEFAULT_FRAME_FORMAT_M_SEQ_IND;
    cmd->connFrameFormatInd = connFrameType;

    if (param->bitFrameType == DLI_SCAN_BIT_FRAME_FORMAT_4_IND) {
        cmd->connParam.connIntervalMin = DEFAULT_FRAME_4_CONN_INTERVAL;
        cmd->connParam.connIntervalMax = DEFAULT_FRAME_4_CONN_INTERVAL;
        cmd->connParam.supervisionTimeout = DEFAULT_FRAME_TYPE_4_ADV_TIMEOUT;
        cmd->scanParam.scanInterval = DEFAULT_FRAME_TYPE_4_CONN_SCAN_INTERVAL;
        cmd->scanParam.scanWindow = DEFAULT_FRAME_TYPE_4_CONN_SCAN_WINDOW;
    } else {
        cmd->connParam.connIntervalMin = param->connectionIntervalMin;
        cmd->connParam.connIntervalMax = param->connectionIntervalMax;
        cmd->connParam.supervisionTimeout = param->supervisionTimeout;
        cmd->scanParam.scanInterval = param->scanInterval;
        cmd->scanParam.scanWindow = param->scanWindow;
    }
    cmd->connParam.maxLatency = param->maxLatency;
    cmd->connParam.minCeLength = param->minCeLength;
    cmd->connParam.maxCeLength = param->maxCeLength;
}

static uint32_t DLI_CreateMultiIndConnection(DLI_ConnCbkContext *cbkContext, DLI_ConnectionCreateParam *param)
{
    DLI_ConnectionCreateParamMultiInd *cmd =
        (DLI_ConnectionCreateParamMultiInd *)SDF_MemZalloc(sizeof(DLI_ConnectionCreateParamMultiInd));
    DLI_CHECK_RETURN_RET(cmd != NULL, DLI_STACK_MEM_ERRNO, "cmd malloc error");

    DLI_BuildCreateMultiConnParam(param, cmd);

    for (uint32_t i = 0; i < CONN_VALID_MULTI_OF_FRAME_TYPE_IND; i++) {
        DLI_LOGI("bitFrameType:0x%02x, scanFrameType=%hhu, connFrameType=0x%04x, scanInterval=0x%04x, "
            "scanWindow=0x%04x, connIntervalMin=0x%04x, connIntervalMax=0x%04x, maxLatency=0x%04x, "
            "supervisionTimeout=0x%04x, minCeLength=0x%04x, maxCeLength=0x%04x",
            param->bitFrameType,
            cmd->scanFrameFormatInd,
            cmd->connFrameFormatInd,
            cmd->scanParam[i].scanInterval,
            cmd->scanParam[i].scanWindow,
            cmd->connParam[i].connIntervalMin,
            cmd->connParam[i].connIntervalMax,
            cmd->connParam[i].maxLatency,
            cmd->connParam[i].supervisionTimeout,
            cmd->connParam[i].minCeLength,
            cmd->connParam[i].maxCeLength);
    }

    uint32_t ret = DLI_ExecuteCommand(DLI_CREATE_CONNECTION,
        DLI_CONNECTION_COMPLETE_EVT,
        cmd,
        sizeof(DLI_ConnectionCreateParamMultiInd),
        NULL,
        cbkContext,
        sizeof(DLI_ConnCbkContext));
    DLI_LOGD("create connection ret = %u", ret);
    SDF_MemFree(cmd);
    return ret;
}

static uint32_t DLI_CreateSingleIndConnection(DLI_ConnCbkContext *cbkContext, DLI_ConnectionCreateParam *param)
{
    if ((param->bitFrameType != DLI_SCAN_BIT_FRAME_FORMAT_1_IND) &&
        (param->bitFrameType != DLI_SCAN_BIT_FRAME_FORMAT_4_IND)) {
        DLI_LOGE("not support single ind connection bitFrameType:0x%02x", param->bitFrameType);
        return DLI_INVALID_PARAMETERS;
    }
    DLI_ConnectionCreateParamSingleInd *cmd =
        (DLI_ConnectionCreateParamSingleInd *)SDF_MemZalloc(sizeof(DLI_ConnectionCreateParamSingleInd));
    DLI_CHECK_RETURN_RET(cmd != NULL, DLI_STACK_MEM_ERRNO, "cmd malloc error");

    DLI_BuildCreateSingleConnParam(param, cmd);

    DLI_LOGI("bitFrameType:0x%02x, scanFrameType=%hhu, connFrameType=0x%04x, scanInterval=0x%04x, scanWindow=0x%04x, "
        "connIntervalMin=0x%04x, connIntervalMax=0x%04x, maxLatency=0x%04x, "
        "supervisionTimeout=0x%04x, minCeLength=0x%04x, maxCeLength=0x%04x",
        param->bitFrameType,
        cmd->scanFrameFormatInd,
        cmd->connFrameFormatInd,
        cmd->scanParam.scanInterval,
        cmd->scanParam.scanWindow,
        cmd->connParam.connIntervalMin,
        cmd->connParam.connIntervalMax,
        cmd->connParam.maxLatency,
        cmd->connParam.supervisionTimeout,
        cmd->connParam.minCeLength,
        cmd->connParam.maxCeLength);

    uint32_t ret = DLI_ExecuteCommand(DLI_CREATE_CONNECTION,
        DLI_CONNECTION_COMPLETE_EVT,
        cmd,
        sizeof(DLI_ConnectionCreateParamSingleInd),
        NULL,
        cbkContext,
        sizeof(DLI_ConnCbkContext));
    DLI_LOGD("create connection ret = %u", ret);
    SDF_MemFree(cmd);
    return ret;
}

uint32_t DLI_CreateConnection(uint8_t version, uint16_t localIndex, DLI_ConnectionCreateParam *param)
{
    DLI_CHECK_RETURN_RET(param, DLI_STACK_PARAMS_ERRNO, "param is null");

    DLI_ConnCbkContext cbkContext = {0};
    cbkContext.versionAndLocalIndex = VersionLocalIndexPack(version, localIndex);

    uint32_t ret = DLI_SUCCESS;
    if ((param->bitFrameType & DLI_SCAN_BIT_FRAME_FORMAT_1_IND) &&
        (param->bitFrameType & DLI_SCAN_BIT_FRAME_FORMAT_4_IND)) {
        ret = DLI_CreateMultiIndConnection(&cbkContext, param);
    } else {
        ret = DLI_CreateSingleIndConnection(&cbkContext, param);
    }
    return ret;
}

uint32_t DLI_CancelCreateConnection(void)
{
    uint32_t ret = DLI_ExecuteCommand(
        DLI_CREATE_CONNECTION_CANCEL, DLI_CMD_COMPLETE_EVT, NULL, 0, DLI_GetCbk(DLI_CBK_CONNECT_CANCEL), NULL, 0);
    DLI_LOGD("cancel create connection ret = %u", ret);
    return ret;
}

uint32_t DLI_Disconnect(uint8_t version, uint16_t localIndex, DLI_DisconnectParam *param)
{
    DLI_CHECK_RETURN_RET(param, DLI_STACK_PARAMS_ERRNO, "param is null");
    DLI_ConnCbkContext cbkContext = {0};
    cbkContext.versionAndLocalIndex = VersionLocalIndexPack(version, localIndex);
    cbkContext.connHandle = param->connHandle;

    uint32_t ret = DLI_ExecuteCommand(DLI_DISCONNECT,
        DLI_DISCONNECTION_COMPLETE_EVT,
        param,
        sizeof(DLI_DisconnectParam),
        NULL,
        &cbkContext,
        sizeof(DLI_ConnCbkContext));
    DLI_LOGD("disconnection ret = %u", ret);
    return ret;
}

uint32_t DLI_UpdateConnectionParam(uint8_t version, uint16_t localIndex, DLI_ConnectionUpdateParam *param)
{
    DLI_CHECK_RETURN_RET(param, DLI_STACK_PARAMS_ERRNO, "param is null");
    DLI_ConnCbkContext cbkContext = {0};
    cbkContext.versionAndLocalIndex = VersionLocalIndexPack(version, localIndex);
    cbkContext.connHandle = param->connHandle;

    DLI_LOGI("param update:connHandle:0x%04X, intervalMin:0x%04X, intervalMax:0x%04X txRxInterval:0x%02X, "
        "eventInterval:0x%04X, latency:0x%04X, timeout:0x%04X, uint:0x%02X, tx_rx:0x%02X",
        param->connHandle,
        param->connIntervalMin,
        param->connIntervalMax,
        param->txRxInterval,
        param->eventInterval,
        param->maxLatency,
        param->supervisionTimeout,
        param->systemTimeUnit,
        param->txRxFlag);

    uint32_t ret = DLI_ExecuteCommand(DLI_CONNECTION_UPDATE,
        DLI_CONNECTION_UPDATE_EVT,
        param,
        sizeof(DLI_ConnectionUpdateParam),
        NULL,
        &cbkContext,
        sizeof(DLI_ConnCbkContext));
    DLI_LOGD("update connection param ret = %u", ret);
    return ret;
}

uint32_t DLI_SetPhy(DLI_SetPhyParam *param)
{
    DLI_CHECK_RETURN_RET(param, DLI_STACK_PARAMS_ERRNO, "param is null");
    uint32_t ret =
        DLI_ExecuteCommand(DLI_SET_PHY, DLI_SET_PHY_COMPLETE_EVT, param, sizeof(DLI_SetPhyParam), NULL, NULL, 0);
    DLI_LOGD("set phy ret = %u", ret);
    return ret;
}

uint32_t DLI_SetDataLength(DLI_SetDataLenParam *param)
{
    DLI_CHECK_RETURN_RET(param, DLI_STACK_PARAMS_ERRNO, "param is null");
    DLI_ConnCbkContext cbkContext = {0};
    cbkContext.connHandle = param->connHandle;

    uint32_t ret = DLI_ExecuteCommand(DLI_SET_DATA_LEN,
        DLI_CMD_COMPLETE_EVT,
        param,
        sizeof(DLI_SetDataLenParam),
        DLI_GetCbk(DLI_CBK_SET_DATA_LEN),
        &cbkContext,
        0);
    DLI_LOGD("set data length ret = %u", ret);
    return ret;
}

uint32_t DLI_ReadRemoteRssi(DLI_ConnHandleStru *param)
{
    DLI_CHECK_RETURN_RET(param, DLI_STACK_PARAMS_ERRNO, "param is null");
    uint32_t ret = DLI_ExecuteCommand(DLI_READ_REMOTER_RSSI,
        DLI_CMD_COMPLETE_EVT,
        param,
        sizeof(DLI_ConnHandleStru),
        DLI_GetCbk(DLI_CBK_READ_REMOTE_RSSI),
        NULL,
        0);
    DLI_LOGD("read remote rssi ret = %u", ret);
    return ret;
}

uint32_t DLI_ReadRemoteFeatures(DLI_ConnHandleStru *param)
{
    DLI_CHECK_RETURN_RET(param, DLI_STACK_PARAMS_ERRNO, "param is null");
    DLI_ConnCbkContext cbkContext = {0};
    cbkContext.connHandle = param->connHandle;
    uint32_t ret = DLI_ExecuteCommand(DLI_READ_REMOTE_FEATURES,
        DLI_READ_REMOTE_FEATURES_COMPLETE_EVT,
        param,
        sizeof(DLI_ConnHandleStru),
        NULL,
        &cbkContext,
        0);
    DLI_LOGD("read remote features ret = %u", ret);
    return ret;
}

uint32_t DLI_ReadRemoteVersion(DLI_ConnHandleStru *param)
{
    DLI_CHECK_RETURN_RET(param, DLI_STACK_PARAMS_ERRNO, "param is null");
    DLI_ConnCbkContext cbkContext = {0};
    cbkContext.connHandle = param->connHandle;
    uint32_t ret = DLI_ExecuteCommand(DLI_READ_REMOTE_VERSION,
        DLI_READ_REMOTE_VERSION_COMPLETE_EVT,
        param,
        sizeof(DLI_ConnHandleStru),
        NULL,
        &cbkContext,
        0);
    DLI_LOGD("read remote version ret = %u", ret);
    return ret;
}

uint32_t DLI_RemoteConnectionParamReqReply(DLI_RemConParamReqReplyParam *param)
{
    DLI_CHECK_RETURN_RET(param, DLI_STACK_PARAMS_ERRNO, "param is null");
    uint32_t ret = DLI_ExecuteCommand(DLI_CONNECTION_PARAM_REQ_REPLY,
        DLI_CMD_COMPLETE_EVT,
        param,
        sizeof(DLI_RemConParamReqReplyParam),
        DLI_GetCbk(DLI_CBK_REMOTE_CONNECT_PARAM_REQ_REPLY),
        NULL,
        0);
    DLI_LOGD("remote connection parameter request reply ret = %u", ret);
    return ret;
}

uint32_t DLI_SetMcs(DLI_SetMcsParam *param)
{
    DLI_CHECK_RETURN_RET(param, DLI_STACK_PARAMS_ERRNO, "param is null");
    uint32_t ret = DLI_ExecuteCommand(
        DLI_SET_MCS, DLI_CMD_COMPLETE_EVT, param, sizeof(DLI_SetMcsParam), DLI_GetCbk(DLI_CBK_SET_MCS), NULL, 0);
    DLI_LOGD("set mcs ret = %u", ret);
    return ret;
}

/* ---------------------------------------------sm模块命令-------------------------------------------------------*/
uint32_t DLI_SetControllerData(DLI_ControllerData *data)
{
    DLI_CHECK_RETURN_RET(data, DLI_STACK_PARAMS_ERRNO, "data is null");
    uint32_t ret = DLI_ExecuteCommand(DLI_SET_CONTROLLER_DATA,
        DLI_CMD_COMPLETE_EVT,
        data,
        sizeof(DLI_ControllerData) + data->dataLength,
        DLI_GetCbk(DLI_CBK_SEND_CONTROLLER_DATA),
        NULL,
        0);
    DLI_LOGD("set controller data ret = %u", ret);
    return ret;
}

uint32_t DLI_EnableEncryption(DLI_EnableEncryptParam *param)
{
    DLI_CHECK_RETURN_RET(param, DLI_STACK_PARAMS_ERRNO, "param is null");
    DLI_SmCbkContext cbkContext = {0};
    cbkContext.connHandle = param->connHandle;
    DLI_LOGI("enable encryption, cryptoAlgo 0x%02X, keyDerivAlgo 0x%02X, integrChkInd %d.",
        param->cryptoAlgo,
        param->keyDerivAlgo,
        param->integrChkInd);
    uint32_t ret = DLI_ExecuteCommand(DLI_ENABLE_ENCRYPTION,
        DLI_ENCRYPTION_CHANGE_EVT,
        param,
        sizeof(DLI_EnableEncryptParam),
        NULL,
        &cbkContext,
        sizeof(DLI_SmCbkContext));
    DLI_LOGD("enable encryption ret = %u", ret);
    return ret;
}

uint32_t DLI_Encrypt(DLI_EncryptParam *param)
{
    DLI_CHECK_RETURN_RET(param, DLI_STACK_PARAMS_ERRNO, "param is null");
    uint32_t ret = DLI_ExecuteCommand(
        DLI_ENCRYPT, DLI_CMD_COMPLETE_EVT, param, sizeof(DLI_EncryptParam), DLI_GetCbk(DLI_CBK_ENCRYPT), NULL, 0);
    DLI_LOGD("encrypt ret = %u", ret);
    return ret;
}

uint32_t DLI_EncryptionParamReqReply(DLI_EncryptReqReplyParam *param)
{
    DLI_CHECK_RETURN_RET(param, DLI_STACK_PARAMS_ERRNO, "param is null");
    uint32_t ret = DLI_ExecuteCommand(DLI_ENCRYPTION_PARAMETER_REQUEST_REPLY,
        DLI_CMD_COMPLETE_EVT,
        param,
        sizeof(DLI_EncryptReqReplyParam),
        DLI_GetCbk(DLI_CBK_ENCRYPT_PARAM_REQ_REPLY),
        NULL,
        0);
    DLI_LOGD("encryption parameter request reply ret = %u", ret);
    return ret;
}

uint32_t DLI_EncryptionParamReqNegativeReply(DLI_ConnHandleStru *param)
{
    DLI_CHECK_RETURN_RET(param, DLI_STACK_PARAMS_ERRNO, "param is null");
    uint32_t ret = DLI_ExecuteCommand(DLI_ENCRYPTION_PARAMETER_REQUEST_NEGATIVE_REPLY,
        DLI_CMD_COMPLETE_EVT,
        param,
        sizeof(DLI_ConnHandleStru),
        DLI_GetCbk(DLI_CBK_ENCRYPT_PARAM_REQ_NEG_REPLY),
        NULL,
        0);
    DLI_LOGD("encryption parameter request negative reply ret = %u", ret);
    return ret;
}

uint32_t DLI_ReadSupportCryptoAlgo(void)
{
    uint32_t ret = DLI_ExecuteCommand(DLI_READ_SUPPORT_CRYPTOGRAPHY_ALGORITHM,
        DLI_CMD_COMPLETE_EVT,
        NULL,
        0,
        DLI_GetCbk(DLI_CBK_READ_SUPPORT_CRYPTO_ALGO),
        NULL,
        0);
    DLI_LOGD("read support crypto algorithm ret = %u", ret);
    return ret;
}

uint32_t DLI_EnableIMGEncryption(DLI_IMGEncryptParam *param)
{
    DLI_CHECK_RETURN_RET(param != NULL, DLI_STACK_PARAMS_ERRNO, "param is null");
    uint16_t handler = param->handler;
    DLI_SmCbkContext cbkContext = {0};
    cbkContext.connHandle = handler;
    cbkContext.isGroup = true;
    DLI_ENCODE2BYTE_LITTLE(&param->handler, handler);
    uint32_t ret = DLI_ExecuteCommand(DLI_ENABLE_IMG_ENCRYPTION,
        DLI_ENCRYPTION_CHANGE_EVT,
        (void *)param,
        sizeof(DLI_IMGEncryptParam),
        NULL,
        (void *)&cbkContext,
        sizeof(DLI_SmCbkContext));
    DLI_LOGD("enable img(%hu) encryption ret = %u", handler, ret);
    return ret;
}

/* ---------------------------------------------hadm模块命令-------------------------------------------------------*/

uint32_t DLI_ReadLocalMeasureCaps(void)
{
    uint32_t ret = DLI_ExecuteCommand(DLI_READ_LOCAL_MEASURE_CAPS,
        DLI_READ_LOCAL_MEASURE_CAPS_STATUS_EVT, NULL, 0, NULL, NULL, 0);
    DLI_LOGI("read local cs caps ret = %u", ret);
    return ret;
}

uint32_t DLI_ReadRemoteMeasureCaps(DLI_ReadRemoteMeasureCapsParam *param)
{
    DLI_CHECK_RETURN_RET(param, DLI_STACK_PARAMS_ERRNO, "param is null");
    uint32_t ret = DLI_ExecuteCommand(DLI_READ_REMOTE_MEASURE_CAPS,
        DLI_READ_REMOTE_MEASURE_CAPS_STATUS_EVT,
        param,
        sizeof(DLI_ReadRemoteMeasureCapsParam),
        NULL,
        NULL,
        0);
    DLI_LOGD("read remote cs caps ret = %u", ret);
    return ret;
}

uint32_t DLI_SetMeasureParam(DLI_SetMeasureConfigParam *param)
{
    DLI_CHECK_RETURN_RET(param, DLI_STACK_PARAMS_ERRNO, "param is null");
    uint32_t ret = DLI_ExecuteCommand(DLI_SET_MEASURE_CONFIG_PARAM,
        DLI_CMD_COMPLETE_EVT,
        param,
        sizeof(DLI_SetMeasureConfigParam),
        DLI_GetCbk(DLI_CBK_SET_MEASURE_CONFIG_PARAM),
        NULL,
        0);
    DLI_LOGI("set cs param ret = %u", ret);
    return ret;
}

uint32_t DLI_SetMeasureEnable(DLI_SetMeasureEnableParam *param)
{
    DLI_CHECK_RETURN_RET(param, DLI_STACK_PARAMS_ERRNO, "param is null");
    uint32_t ret = DLI_ExecuteCommand(DLI_SET_MEASURE_EN,
        DLI_CMD_COMPLETE_EVT,
        param,
        sizeof(DLI_SetMeasureEnableParam),
        DLI_GetCbk(DLI_CBK_SET_MEASURE_EN),
        NULL,
        0);
    DLI_LOGD("set cs enable ret = %u, enable = %hhu", ret, param->enable);
    return ret;
}

/* ---------------------------------------------同步链路命令-------------------------------------------------------*/

uint32_t DLI_SetICGParam(DLI_ICGParam *param, DLI_ICGCbkParam *cbkParam)
{
    DLI_CHECK_RETURN_RET(param != NULL && cbkParam != NULL, DLI_STACK_PARAMS_ERRNO, "param is null");

    size_t icgParamSize = sizeof(DLI_SetICGParameter) + param->icbCnt * sizeof(uint8_t) +
        param->icbCnt * param->paramCnt * sizeof(struct DLI_SetICBParam);
    DLI_SetICGParameter *cmd = (DLI_SetICGParameter *)SDF_MemZalloc(icgParamSize);
    DLI_CHECK_RETURN_RET(cmd != NULL, DLI_STACK_MEM_ERRNO, "malloc cmd failed");

    cmd->id = param->id;
    DLI_ENCODE3BYTE_LITTLE(&cmd->sduIntervalG2T, param->sduIntervalG2T);
    DLI_ENCODE3BYTE_LITTLE(&cmd->sduIntervalT2G, param->sduIntervalT2G);
    cmd->sca = param->sca;
    cmd->packing = param->packing;
    cmd->framing = param->framing;
    DLI_ENCODE2BYTE_LITTLE(&cmd->maxLatencyG2T, param->maxLatencyG2T);
    DLI_ENCODE2BYTE_LITTLE(&cmd->maxLatencyT2G, param->maxLatencyT2G);
    cmd->icbCnt = param->icbCnt;
    cmd->paramCnt = param->paramCnt;
    for (uint8_t i = 0; i < param->icbCnt; i++) {
        DLI_SetICBParameter *icb = (DLI_SetICBParameter *)(cmd->icbParam +
            i * (sizeof(uint8_t) + param->paramCnt * sizeof(struct DLI_SetICBParam)));
        icb->id = param->icbParam[i].id;
        for (uint8_t j = 0; j < param->paramCnt; j++) {
            DLI_ENCODE2BYTE_LITTLE(&icb->param[j].maxSduG2T, param->icbParam[i].param[j].maxSduG2T);
            DLI_ENCODE2BYTE_LITTLE(&icb->param[j].maxSduT2G, param->icbParam[i].param[j].maxSduT2G);
            icb->param[j].rtnG2T = param->icbParam[i].param[j].rtnG2T;
            icb->param[j].rtnT2G = param->icbParam[i].param[j].rtnT2G;
        }
    }

    uint32_t ret = DLI_ExecuteCommand(param->opCode,
        DLI_CMD_COMPLETE_EVT,
        cmd,
        (uint16_t)icgParamSize,
        DLI_GetCbk(param->opCode == DLI_SET_IMG_PARAM ? DLI_CBK_SET_IMG_PARAM : DLI_CBK_SET_IOG_PARAM),
        cbkParam,
        sizeof(DLI_ICGCbkParam));
    SDF_MemFree(cmd);
    DLI_LOGD("set icg param ret = %u", ret);
    return ret;
}

uint32_t DLI_SetICGTestParam(DLI_ICGTestParam *param, DLI_ICGCbkParam *cbkParam)
{
    DLI_CHECK_RETURN_RET(param != NULL && cbkParam != NULL, DLI_STACK_PARAMS_ERRNO, "param is null");

    size_t icgParamSize = sizeof(DLI_SetICGTestParameter) + param->paramCnt * (sizeof(DLI_ICBTestParam));
    DLI_SetICGTestParameter *cmd = (DLI_SetICGTestParameter *)SDF_MemZalloc(icgParamSize);
    DLI_CHECK_RETURN_RET(cmd != NULL, DLI_STACK_MEM_ERRNO, "malloc cmd failed");

    cmd->id = param->id;
    DLI_ENCODE3BYTE_LITTLE(&cmd->sduIntervalG2T, param->sduIntervalG2T);
    DLI_ENCODE3BYTE_LITTLE(&cmd->sduIntervalT2G, param->sduIntervalT2G);
    cmd->ftG2T = param->ftG2T;
    cmd->ftT2G = param->ftT2G;
    DLI_ENCODE2BYTE_LITTLE(&cmd->icbInterval, param->icbInterval);
    cmd->sca = param->sca;
    cmd->packing = param->packing;
    cmd->framing = param->framing;
    cmd->paramCnt = param->paramCnt;
    for (uint8_t i = 0; i < param->paramCnt; i++) {
        DLI_ICBTestParam *icb = (DLI_ICBTestParam *)(cmd->param + i * sizeof(DLI_ICBTestParam));
        icb->id = param->icbParam[i].id;
        icb->nse = param->icbParam[i].nse;
        DLI_ENCODE2BYTE_LITTLE(&icb->maxSduG2T, param->icbParam[i].maxSduG2T);
        DLI_ENCODE2BYTE_LITTLE(&icb->maxSduT2G, param->icbParam[i].maxSduT2G);
        DLI_ENCODE2BYTE_LITTLE(&icb->maxPduG2T, param->icbParam[i].maxPduG2T);
        DLI_ENCODE2BYTE_LITTLE(&icb->maxPduT2G, param->icbParam[i].maxPduT2G);
        icb->frameG2T = param->icbParam[i].frameG2T;
        icb->frameT2G = param->icbParam[i].frameT2G;
        icb->phyG2T = param->icbParam[i].phyG2T;
        icb->phyT2G = param->icbParam[i].phyT2G;
        icb->mcsG2T = param->icbParam[i].mcsG2T;
        icb->mcsT2G = param->icbParam[i].mcsT2G;
        icb->pilotG2T = param->icbParam[i].pilotG2T;
        icb->pilotT2G = param->icbParam[i].pilotT2G;
        icb->bnG2T = param->icbParam[i].bnG2T;
        icb->bnT2G = param->icbParam[i].bnT2G;
    }

    uint32_t ret = DLI_ExecuteCommand(param->opCode,
        DLI_CMD_COMPLETE_EVT,
        cmd,
        (uint16_t)icgParamSize,
        DLI_GetCbk(param->opCode == DLI_SET_IMG_PARAM_TEST ? DLI_CBK_SET_IMG_PARAM : DLI_CBK_SET_IOG_PARAM),
        cbkParam,
        sizeof(DLI_ICGCbkParam));
    SDF_MemFree(cmd);
    DLI_LOGD("set icg test param ret = %u", ret);
    return ret;
}

uint32_t DLI_RemoveICGParam(DLI_CmdOpcode opCode, DLI_ICGCbkParam *cbkParam)
{
    DLI_CHECK_RETURN_RET(cbkParam != NULL, DLI_STACK_PARAMS_ERRNO, "cbkParam is null");
    uint32_t ret = DLI_ExecuteCommand(opCode,
        DLI_CMD_COMPLETE_EVT,
        &cbkParam->id,
        sizeof(cbkParam->id),
        DLI_GetCbk(DLI_CBK_REMOVE_ICG_PARAM),
        cbkParam,
        sizeof(DLI_ICGCbkParam));
    DLI_LOGD("remove icg param ret = %u", ret);
    return ret;
}

uint32_t DLI_CreateICB(DLI_ICBConnectionParam *param, DLI_ICGCbkParam *cbkParam)
{
    DLI_CHECK_RETURN_RET(param != NULL && cbkParam != NULL, DLI_STACK_PARAMS_ERRNO, "param is null");

    size_t cmdSize = sizeof(DLI_CreateICBParameter) + param->channelCnt * (sizeof(DLI_ICBChannel));
    DLI_CreateICBParameter *cmd = (DLI_CreateICBParameter *)SDF_MemZalloc(cmdSize);
    DLI_CHECK_RETURN_RET(cmd != NULL, DLI_STACK_MEM_ERRNO, "malloc cmd failed");

    cmd->id = param->id;
    cmd->channelCnt = param->channelCnt;
    for (uint8_t i = 0; i < param->channelCnt; i++) {
        DLI_ICBChannel *channel = (DLI_ICBChannel *)(cmd->channel + i * sizeof(DLI_ICBChannel));
        DLI_ENCODE2BYTE_LITTLE(&channel->connHandle, param->channel[i].connHandle);
        DLI_ENCODE2BYTE_LITTLE(&channel->lcid, param->channel[i].lcid);
    }
    uint16_t event;
    if (param->opCode == DLI_CREATE_IMB) {
        event = DLI_IMB_ESTABLISHED_EVT_STD;
    } else {
        event = DLI_IOB_ESTABLISHED_EVT_STD;
    }
    uint32_t ret = DLI_ExecuteCommand(param->opCode,
        event,
        cmd,
        (uint16_t)cmdSize,
        NULL,
        cbkParam,
        sizeof(DLI_ICGCbkParam));
    SDF_MemFree(cmd);
    DLI_LOGD("create param ret = %u", ret);
    return ret;
}

uint32_t DLI_DisconnectICB(DLI_DisconnectParam *param, DLI_ICGCbkParam *cbkParam)
{
    DLI_CHECK_RETURN_RET(param != NULL && cbkParam != NULL, DLI_STACK_PARAMS_ERRNO, "param is null");

    uint16_t connHandle = param->connHandle;
    DLI_ENCODE2BYTE_LITTLE(&param->connHandle, connHandle);

    uint32_t ret = DLI_ExecuteCommand(DLI_DISCONNECT,
        DLI_DISCONNECTION_COMPLETE_EVT,
        param,
        sizeof(DLI_DisconnectParam),
        NULL,
        cbkParam,
        sizeof(DLI_ICGCbkParam));
    DLI_LOGD("disconnection ret = %u", ret);
    return ret;
}

uint32_t DLI_AcceptICBReq(DLI_AcceptICBReqParam *param)
{
    DLI_CHECK_RETURN_RET(param, DLI_STACK_PARAMS_ERRNO, "param is null");
    uint16_t connHandle;
    DLI_ENCODE2BYTE_LITTLE(&connHandle, param->connHandle);
    // Tips: 被动连接没有处理V1兼容问题
    uint32_t ret = DLI_ExecuteCommand(param->opCode,
        param->opCode == DLI_ACCEPT_IMB_REQ ? DLI_IMB_ESTABLISHED_EVT : DLI_IOB_ESTABLISHED_EVT,
        &connHandle,
        sizeof(connHandle),
        NULL,
        &param->type,
        sizeof(param->type));
    DLI_LOGD("accept param req ret = %u", ret);
    return ret;
}

uint32_t DLI_RejectICBReq(DLI_RejectICBReqParam *param)
{
    DLI_CHECK_RETURN_RET(param != NULL, DLI_STACK_PARAMS_ERRNO, "param is null");
    DLI_RejectICBReqParameter cmd = {};
    DLI_ENCODE2BYTE_LITTLE(&cmd.connHandle, param->connHandle);
    cmd.reason = param->reason;
    uint32_t ret = DLI_ExecuteCommand(param->opCode, DLI_CMD_COMPLETE_EVT, &cmd, sizeof(cmd),
        DLI_GetCbk(param->regOpCode), &param->id, sizeof(param->id));
    DLI_LOGD("reject param req ret = %u", ret);
    return ret;
}

uint32_t DLI_SetupICBDataPath(DLI_SetupICBDataPathParam *dataPath, DLI_ICGCbkParam *cbkParam)
{
    DLI_CHECK_RETURN_RET(dataPath != NULL && cbkParam != NULL, DLI_STACK_PARAMS_ERRNO, "dataPath is null");
    DLI_CHECK_RETURN_RET(dataPath->codecConfigLen != 0, DLI_STACK_PARAMS_ERRNO, "codec config len is 0");

    size_t dataPathSize = sizeof(DLI_ICBDataPathParameter) + dataPath->codecConfigLen;
    DLI_ICBDataPathParameter *cmd = (DLI_ICBDataPathParameter *)SDF_MemZalloc(dataPathSize);
    DLI_CHECK_RETURN_RET(cmd != NULL, DLI_STACK_MEM_ERRNO, "malloc cmd failed");

    DLI_ENCODE2BYTE_LITTLE(&cmd->connHandle, dataPath->connHandle);
    cmd->direction = dataPath->direction;
    cmd->pathId = dataPath->pathId;
    cmd->codec.codecId = dataPath->codec.codecId;
    DLI_ENCODE2BYTE_LITTLE(&cmd->codec.vendorId, dataPath->codec.vendorId);
    DLI_ENCODE2BYTE_LITTLE(&cmd->codec.vendorCodecId, dataPath->codec.vendorCodecId);
    DLI_ENCODE3BYTE_LITTLE(&cmd->controllerDelay, dataPath->controllerDelay);
    cmd->codecConfigLen = dataPath->codecConfigLen;
    if (memcpy_s(cmd->codecConfigData, dataPath->codecConfigLen,
        dataPath->codecConfigData, dataPath->codecConfigLen) != EOK) {
        SDF_MemFree(cmd);
        DLI_LOGE("memcpy codec config data failed");
        return DLI_STACK_MEM_ERRNO;
    }

    uint32_t ret = DLI_ExecuteCommand(DLI_SETUP_ICB_DATA_PATH,
        DLI_CMD_COMPLETE_EVT,
        cmd,
        (uint16_t)dataPathSize,
        DLI_GetCbk(DLI_CBK_ICB_SETUP_DATA_PATH),
        cbkParam,
        sizeof(DLI_ICGCbkParam));
    SDF_MemFree(cmd);
    DLI_LOGD("set icb data path ret = %u", ret);
    return ret;
}

uint32_t DLI_RemoveICBDataPath(DLI_RemoveICBDataPathParam *dataPath, DLI_ICGCbkParam *cbkParam)
{
    DLI_CHECK_RETURN_RET(dataPath != NULL && cbkParam != NULL, DLI_STACK_PARAMS_ERRNO, "dataPath is null");

    uint16_t connHandle = dataPath->connHandle;
    DLI_ENCODE2BYTE_LITTLE(&dataPath->connHandle, connHandle);
    uint32_t ret = DLI_ExecuteCommand(DLI_REMOVE_ICB_DATA_PATH,
        DLI_CMD_COMPLETE_EVT,
        dataPath,
        sizeof(DLI_RemoveICBDataPathParam),
        DLI_GetCbk(DLI_CBK_ICB_REMOVE_DATA_PATH),
        cbkParam,
        sizeof(DLI_ICGCbkParam));
    DLI_LOGD("remove icb data path ret = %u", ret);
    return ret;
}

uint16_t DLI_GetAcbDataLen(void)
{
    return DLI_DataLenGet(ACB_DATA_TYPE);
}

uint32_t DLI_SetCmd(DLI_CmdParams *params)
{
    DLI_CHECK_RETURN_RET(params, DLI_STACK_PARAMS_ERRNO, "params is null");
    uint32_t ret = DLI_ExecuteCommand(params->cmd, params->event, params->inParam,
        params->paramLen, params->cbk, params->cbkContext, params->cbkContextLen);
    DLI_LOGD("Set cmd ret = %u", ret);
    return ret;
}
