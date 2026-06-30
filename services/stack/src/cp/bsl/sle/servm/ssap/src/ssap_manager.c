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
#include "securec.h"
#include "sdf_string.h"
#include "sdf_mem.h"
#include "sdf_trace.h"
#include "cpfwk_log.h"
#include "cm_api.h"
#include "cm_errno.h"
#include "cm_logic_link_api.h"
#include "dtap.h"
#include "dtap_tcid.h"
#include "dtap_errno.h"
#include "ssaps_server.h"
#include "ssapc_client.h"
#include "ssap_link.h"
#include "ssap_utils.h"
#include "ssapc_app.h"
#include "ssaps_service.h"
#include "nlstk_ssap_app_link.h"
#include "ssap_link_state.h"
#include "ssap_app_link.h"
#include "ssaps_server_app.h"
#include "ssapc_cache.h"
#include "ssapc_app_util.h"
#include "ssap_manager.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define SSAP_INIT_FIND_REQ_START_HANDLE 0x0001
#define SSAP_INIT_FIND_REQ_END_HANDLE 0xFFFF
#define MAX_PRINT_LEN 200

typedef void (*Dispatch)(SSAP_Link_S *, SDF_Buff_S *);

static void SSAP_ErrorRspHandle(SSAP_Link_S *link, SDF_Buff_S *sdfBuff);

static Dispatch g_dispatchTable[] = {
    NULL,
    SSAP_ErrorRspHandle,
    SSAPS_ExchangeInfoReqHandle,
    SSAPC_ExchangeInfoRspHandle,
    SSAPS_FindReqHandle,
    SSAPC_FindRspHandle,
    SSAPS_FindReqHandle,
    SSAPC_FindRspHandle,
    SSAPS_ReadReqHandle,
    SSAPC_ReadRspHandle,
    SSAPS_ReadByUuidReqHandle,
    SSAPC_ReadByUuidRspHandle,
    SSAPS_WriteCmdHandle,
    SSAPS_WriteReqHandle,
    SSAPC_WriteRspHandle,
    SSAPC_ValueNtfHandle,
    SSAPC_ValueIndHandle,
    SSAPS_ValueAckHandle,
    SSAPS_MethodCmdHandle,
    SSAPS_MethodReqHandle,
    SSAPC_CallMethodRspHandle
};

/**
 * @brief  根据消息码分发处理函数
 */
static void SSAP_DataDispatch(SSAP_Link_S *link, SDF_Buff_S *sdfBuff)
{
    // 内部函数，调用点SSAP_Recv对sdfBuff已判断合法，无需再判断
    uint8_t *data = SDF_DataOffset(sdfBuff);
    uint8_t op = *data;
    CP_CHECK_LOG_RETURN_VOID(op < sizeof(g_dispatchTable) / sizeof(g_dispatchTable[0]), "[SSAP] op invalid");
    CP_CHECK_LOG_RETURN_VOID(g_dispatchTable[op] != NULL, "[SSAP] diapatch op not support: %d", op);
    g_dispatchTable[op](link, sdfBuff);
}

void SSAP_ProcessRequestTask(SSAP_Link_S *link, SSAP_TaskParam_S *param, bool delay)
{
    CP_CHECK_LOG_RETURN_VOID(link != NULL && param != NULL, "[SSAP] param is null");
    SSAP_TaskParam_S *task = SSAP_AllocTaskParam(param);
    if (task == NULL) {
        CP_LOG_ERROR("[SSAP] task create failed");
        if (param->freeFunc != NULL) {
            param->freeFunc(param->arg);
        }
        return;
    }
    SSAP_AddTaskParamToLink(link, task);
    // 当前仅有建链成功之后的MTU交互会延迟,为了不影响其他流程的底层芯片交互
    if (!delay) {
        SSAP_ExcuteProcessTask(link);
    }
}

void SSAP_ProcessHighPriorityRequestTask(SSAP_Link_S *link, SSAP_TaskParam_S *param, bool delay)
{
    CP_CHECK_LOG_RETURN_VOID(link != NULL, "[SSAP] param is null");
    SSAP_TaskParam_S *task = SSAP_AllocTaskParam(param);
    if (task == NULL) {
        CP_LOG_ERROR("[SSAP] task create failed");
        if (param->freeFunc != NULL) {
            param->freeFunc(param->arg);
        }
        return;
    }
    SSAP_AddHighPriorityTaskParamToLink(link, task);
    // 当前仅有建链成功之后的MTU交互会延迟,为了不影响其他流程的底层芯片交互
    if (!delay) {
        SSAP_ExcuteProcessTask(link);
    }
}

void SSAP_ProcessNormalTask(SSAP_Link_S *link, SSAP_ProcessTaskFunc func, void *arg, SSAP_TaskArgFreeFunc freeFunc)
{
    CP_CHECK_LOG_RETURN_VOID(link != NULL, "[SSAP] param is null");
    if (func != NULL) {
        func(link, arg);
    }
    if (freeFunc != NULL) {
        freeFunc(arg);
    }
}

void PrintFormatHexWithSpaces(const uint8_t *dataBuf, size_t dataSize)
{
    char dataStr[SSAP_STACK_MTU_MAX + SSAP_STACK_MTU_MAX + 1] = { 0 };
    int count = 0;
    for (uint32_t i = 0; i < dataSize; i++) {
        (void)sprintf_s(&dataStr[2 * count], (SSAP_STACK_MTU_MAX - count) * 2, "%02x", dataBuf[i]); // 2 hex char
        if (++count >= SSAP_STACK_MTU_MAX - 1) {
            break;
        }
    }

    size_t dataStrLen = strlen(dataStr);
    for (uint32_t i = 0, j = 0; i < dataStrLen; j++) {
        char temp[MAX_PRINT_LEN + 1];
        memset_s(temp, sizeof(temp), 0, sizeof(temp));
        if (dataStrLen - i >= MAX_PRINT_LEN) {
            if (memcpy_s(temp, sizeof(temp), &dataStr[i], MAX_PRINT_LEN) != EOK) {
                HILOGE("memcpy_s failed");
                return;
            }
            i += MAX_PRINT_LEN;
        } else {
            if (memcpy_s(temp, sizeof(temp), &dataStr[i], dataStrLen - i) != EOK) {
                HILOGE("memcpy_s failed");
                return;
            }
            i += dataStrLen - i;
        }

        CP_LOG_DEBUG("[SSAP] recv msg[%d]: %s", j, temp);
    }
}

int SSAP_Recv(DTAP_Data_Info_S *info, SDF_Buff_S *buff)
{
    CP_CHECK_LOG_RETURN(info != NULL && buff != NULL, SSAP_STACK_FAILED, "[SSAP] recv info or buff is null");
    uint8_t *dataBuf = SDF_DataOffset(buff);
    CP_CHECK_LOG_RETURN(SDF_DataLenGet(buff) <= UINT32_MAX, SSAP_STACK_FAILED, "[SSAP] recv datalen is invalid");
    uint32_t dataSize = (uint32_t)SDF_DataLenGet(buff);
    CP_CHECK_LOG_RETURN(dataSize != 0, SSAP_STACK_FAILED, "[SSAP] recv datalen is zero");
    SSAP_Link_S *link = SSAP_FindSsapLinkByLcid(info->lcid);
    CP_CHECK_LOG_RETURN(link != NULL, SSAP_STACK_FAILED, "[SSAP] cant find link");
    uint8_t op = *(SDF_DataOffset(buff));
    CP_LOG_DEBUG("[SSAP] recv msg len: %d, opcode: 0x%x", dataSize, op);
    PrintFormatHexWithSpaces(dataBuf, dataSize);

    uint8_t ret = SSAP_CheckOpcode(link, op);
    if (ret == SSAP_ERRCODE_UNSUPPORT_PDU) {
        SSAP_PduErrorRsp(link, op, 0, SSAP_ERRCODE_UNSUPPORT_PDU);
    }
    if (ret != SSAP_ERRCODE_SUCCESS) {
        CP_LOG_ERROR("[SSAP] recv opcode check failed, errcode = %x", ret);
        return SSAP_STACK_FAILED;
    }

    uint8_t type = SSAP_GetOpcodeType(op);
    // 非响应或者ACK消息，直接处理
    if (type != SSAP_TRANS_RSP && type != SSAP_TRANS_ACK) {
        SSAP_DataDispatch(link, buff);
        return SSAP_STACK_SUCCESS;
    }
    // 如果收到的是响应或者ACK消息，需要调用完成任务的函数，释放任务参数和相关的buffer
    // 初始的find不需要处理rsp
    if (link->hasInitReqTask && type == SSAP_TRANS_RSP) {
        link->hasInitReqTask = false;
    } else {
        // appid解注册后task会无效，回复无需处理
        if (SsapTaskValid(link)) {
            SSAP_DataDispatch(link, buff);
        }
    }
    SSAP_LinkClearCurrentTask(link);
    // 处理完消息之后，尝试继续执行链路上的任务
    SSAP_ExcuteProcessTask(link);
    return SSAP_STACK_SUCCESS;
}

static bool SSAP_SendBuffToDTAP(uint16_t lcid, SDF_Buff_S *buff)
{
    SDF_Buff_S *tmpBuff = SDF_BuffCopy(buff);
    CP_CHECK_LOG_RETURN(tmpBuff != NULL, false, "[SSAP] copy buf fail");
    DTAP_Data_S data = {0};
    data.lcid = lcid;
    data.tcid = TCID_SLE_SMTC;
    data.buff = tmpBuff;
    if (DTAP_DataSend(&data) != DTAP_SUCCESS) {
        CP_LOG_ERROR("[SSAP] send dtap data failed");
        SDF_BuffFree(tmpBuff);
        return false;
    }
    return true;
}

void SSAP_TimeoutCbk(void *arg)
{
    CP_LOG_INFO("[SSAP] SSAP_TimeoutCbk enter");
    CP_CHECK_LOG_RETURN_VOID(arg != NULL, "[SSAP] timeout cbk arg is null");
    SSAP_Link_S *link = (SSAP_Link_S *)arg;
    CP_LOG_INFO("[SSAP] timeout msg opcode: %d", link->curTask.opcode);
    if (!SsapTaskValid(link)) {
        CP_LOG_INFO("[SSAP] appid %d has been deregistered, no need process timeout callback",
            SsapTaskGetAppId(link));
        SSAP_LinkClearCurrentTask(link);
        SSAP_ExcuteProcessTask(link);
        return;
    }
    if (link->hasInitReqTask) {
        CP_LOG_INFO("[SSAP] ssap init req timeout");
        link->hasInitReqTask = false;
    }
    switch (link->curTask.opcode) {
        case SSAP_EXCHANGE_INFO_REQ:
            SSAPC_ExchangeInfoErrorHandle(link, SSAP_ERRCODE_TIMEOUT);
            break;
        case SSAP_FIND_STRUCTURE_REQ:
        case SSAP_FIND_STRUCTURE_BY_UUID_REQ:
            SSAPC_FindReqErrorHandle(link, SSAP_ERRCODE_TIMEOUT);
            break;
        case SSAP_READ_REQ:
        case SSAP_WRITE_REQ:
            SSAPC_ValueErrorHandle(link, SSAP_ERRCODE_TIMEOUT);
            break;
        case SSAP_READ_BY_UUID_REQ:
            SSAPC_ReadByUuidErrorHandle(link, SSAP_ERRCODE_TIMEOUT);
            break;
        case SSAP_CALL_METHOD_REQ:
            SSAPC_CallMethodErrorHandle(link, SSAP_ERRCODE_TIMEOUT);
            break;
        case SSAP_VALUE_IND:
            CP_LOG_INFO("[SSAP] value ind timeout");
            break;
        default:
            break;
    }

    SDF_SsapTrace(link->addr.addr, EXCEP_SSAP_REQ_TIMEOUT, link->curTask.opcode);
    SSAP_LinkClearCurrentTask(link);
    SSAP_ExcuteProcessTask(link);

    if (CM_DirectConnectRemove(CM_MODULE_SSAP, &(link->addr), CM_DISC_REASON_COMMAND_TIMEOUT) != CM_SUCCESS) {
        CP_LOG_ERROR("[SSAP] Timeout CM_DirectConnectRemove fail, addr: %s", GET_ENC_ADDR(&(link->addr)));
    }
}

static void SSAP_Send(SSAP_Link_S *link, SDF_Buff_S *buff, uint8_t opcode)
{
    CP_CHECK_LOG_RETURN_VOID(link != NULL && buff != NULL, "[SSAP] param is null");
    uint8_t *dataBuf = SDF_DataOffset(buff);
    CP_CHECK_LOG_RETURN_VOID(SDF_DataLenGet(buff) <= UINT32_MAX, "[SSAP] recv datalen is invalid");
    uint32_t dataSize = (uint32_t)SDF_DataLenGet(buff);
    CP_CHECK_LOG_RETURN_VOID(dataSize > 0, "[SSAP] send buff len is 0");
    CP_LOG_DEBUG("[SSAP] send msg len: %d, opcode: 0x%x", dataSize, opcode);
    CP_LOG_DEBUG("[SSAP] send msg: %s", SDF_GET_UINT8_STR(dataBuf, dataSize));
    uint8_t type = SSAP_GetOpcodeType(opcode);
    if (type == SSAP_TRANS_INVALID) {
        CP_LOG_ERROR("[SSAP] unknown opcode");
        SDF_BuffFree(buff);
        return;
    }
    if (!SSAP_SendBuffToDTAP(link->lcid, buff)) {
        SDF_BuffFree(buff);
        return;
    }
    if ((type & SSAP_REPLY_MASK) == 0) {
        SDF_BuffFree(buff);
        return;
    }
    SSAP_LinkSetTask(link, buff, opcode);
    if (!SSAP_StartTimer(link, SSAP_TimeoutCbk)) {
        CP_LOG_ERROR("[SSAP] create timer failed");
    }
}

static void SSAP_CMLogicLinkCbk(CM_LogicLinkState_S *param)
{
    CP_CHECK_LOG_RETURN_VOID(param != NULL, "[SSAP] cm param is null");
    CP_LOG_INFO("[SSAP] cm link lcid: %d, addr: %s, result: %d", param->lcid, GET_ENC_ADDR(&param->addr),
        param->result);
    if (param->result == CM_LINK_STATE_CONNECTED) {
        // 此处为了适配老协议栈需要先发送一个find req，因此使用SSAP_CreateSsapLinkWithInitReq接口创建link。
        // 后续老协议栈设备淘汰，可能不再使用初始的find req，因此SSAP_CreateSsapLink接口先保留
        SSAP_Link_S *link = SSAP_CreateSsapLinkWithInitReq(&param->addr, param->lcid, SSAP_Send, true);
        CP_CHECK_LOG_RETURN_VOID(link != NULL, "[SSAP] create ssap link failed");
        // 更新链路的实际连接状态
        SsapLinkHandleRecordLinkStateFromCm(&(param->addr),  SSAP_CONNECT_STATE_CONNECTED);
        SsapTriggerLinkStateMachineChange(&(param->addr), SSAP_LOGIC_LINK_CONNECTED, param->discReason);
        NLSTK_Errcode_E cacheCreateRet = SsapcCacheCreate(&(param->addr));
        CP_LOG_INFO("[SSAP] ssap client cache create ret: %d", cacheCreateRet);
        SSAP_ParamFind_S *findParam = (SSAP_ParamFind_S *)SDF_MemZalloc(sizeof(SSAP_ParamFind_S));
        CP_CHECK_LOG_RETURN_VOID(findParam != NULL, "[SSAP] Init find req malloc failed");
        (void)memcpy_s(&findParam->addr, sizeof(SLE_Addr_S), &param->addr, sizeof(SLE_Addr_S));
        findParam->type = FIND_STRUCTURE_TYPE_PRIMARY_SERVICE;
        findParam->startHandle = SSAP_INIT_FIND_REQ_START_HANDLE;
        findParam->endHandle = SSAP_INIT_FIND_REQ_END_HANDLE;
        SSAP_TaskParam_S taskParam = {.appId = SSAP_APP_INVALID_ID, .arg = findParam, .freeFunc = SDF_MemFree,
            .func = SSAPC_InitFindReq, .timeout = SSAP_INTERACTION_MAX_TIMEOUT, .valid = true};
        // SSAP初始find添加至缓存队列。此时配对之类的流程还未结束，不影响芯片交互流程，因此延迟发送
        SSAP_ProcessRequestTask(link, &taskParam, true);
    } else if (param->result == CM_LINK_STATE_DISCONNECTED) {
        SSAP_DeleteSsapLinkByAddr(&param->addr);
        // 更新链路的实际连接状态
        SsapLinkHandleRecordLinkStateFromCm(&(param->addr),  SSAP_CONNECT_STATE_DISCONNECTED);
        SsapTriggerLinkStateMachineChange(&(param->addr), SSAP_LOGIC_LINK_DISCONNECTED, param->discReason);
        SsapcCacheDestroy(&(param->addr));
        SSAPS_CleanServiceCpcd(&(param->addr));
    }
    // 当启动星闪的链路关闭之后，每次都要检查是否需要清理所有的app资源，并上报状态
    if (SsapGetClientCleanUp()) {
        SsapcAppNotifyClientCleanUp();
    }
    if (SsapGetServerCleanUp()) {
        SsapServerAppCleanUpNotify();
    }
}

uint32_t SSAP_Init(void)
{
    CP_LOG_INFO("[SSAP] init start");
    SSAP_LinkInit();
    CM_LogicLinkCbks_S cmLinkCbk = {0};
    cmLinkCbk.moduleId = CM_MODULE_SSAP;
    cmLinkCbk.logicLinkCbk = SSAP_CMLogicLinkCbk;
    CM_RegLogicLinkListener(&cmLinkCbk);
    DTAP_RegisterDataRecvCb(TCID_SLE_SMTC, SSAP_Recv);
    SSAPS_ServiceInit();
    SsapcCacheInit();
    CP_LOG_INFO("[SSAP] init finish");
    return SDF_OK;
}

void SSAP_DeInit(void)
{
    CP_LOG_INFO("[SSAP] deinit start");
    SSAP_LinkDeInit();
    CM_UnRegLogicLinkListener(CM_MODULE_SSAP);
    DTAP_UnregisterDataRecvCb(TCID_SLE_SMTC);
    SSAPS_ServiceDeInit();
    SsapcCacheDeInit();
    SsapServerAppDeinit();
    SsapcClientAppDeinit();
    SsapLinkStateDeinit();
    CP_LOG_INFO("[SSAP] deinit finish");
}

/**
 * @brief  错误处理报文响应
 */
void SSAP_PduErrorRsp(SSAP_Link_S *link, uint8_t op, uint16_t errorHandle, uint8_t errorCode)
{
    uint32_t realSize = sizeof(SSAP_PduErrRsp_S);
    SDF_Buff_S *sdfBuff = SDF_BuffNewWithReserve(realSize);
    CP_CHECK_LOG_RETURN_VOID(sdfBuff != NULL, "[SSAP] send error rsp sdfBuff malloc fail");
    uint8_t *buf = SDF_BuffAppend(sdfBuff, realSize);
    if (buf == NULL) {
        SDF_BuffFree(sdfBuff);
        CP_LOG_ERROR("[SSAP] send error rsp create buf fail");
        return;
    }
    SSAP_PduErrRsp_S *errRsp = (SSAP_PduErrRsp_S *)buf;

    /* 消息码 */
    errRsp->msgCode = SSAP_ERROR_RSP;
    /* 消息控制码 */
    errRsp->msgCtrl = 0; /* RFU */
    /* 请求消息码 */
    errRsp->msgCodeReq = op;
    /* 错误句柄 */
    errRsp->errHandle = errorHandle;
    /* 错误码 */
    errRsp->errorCode = errorCode;
    CP_LOG_INFO("[SSAP] send error rsp, opcode: 0x%x, errHandle: %d, errorCode: %d.", op, errorHandle, errorCode);
    link->sendFunc(link, sdfBuff, SSAP_ERROR_RSP);
}

static void SSAP_ErrorRspHandle(SSAP_Link_S *link, SDF_Buff_S *sdfBuff)
{
    uint8_t *buf = SDF_DataOffset(sdfBuff);
    CP_CHECK_LOG_RETURN_VOID(SDF_DataLenGet(sdfBuff) <= UINT32_MAX, "[SSAP] recv datalen is invalid");
    uint32_t size = (uint32_t)SDF_DataLenGet(sdfBuff);
    if (size < sizeof(SSAP_PduErrRsp_S)) {
        CP_LOG_ERROR("[SSAP] recv error rsp wrong size, size: %d", size);
        return;
    }
    SSAP_PduErrRsp_S *errorRsp = (SSAP_PduErrRsp_S *)buf;
    CP_LOG_INFO("[SSAP] recv error rsp, opcode: 0x%x, handle: %d, code: %u", errorRsp->msgCodeReq,
        errorRsp->errHandle, errorRsp->errorCode);
    CP_CHECK_LOG_RETURN_VOID(errorRsp->msgCodeReq == link->curTask.opcode, "[SSAP] recv error rsp wrong opcode");
    SDF_SsapTrace(link->addr.addr, EXCEP_SSAP_PDU_ERROR_RSP_RECV, errorRsp->errorCode);
    switch (errorRsp->msgCodeReq) {
        case SSAP_EXCHANGE_INFO_REQ:
            SSAPC_ExchangeInfoErrorHandle(link, errorRsp->errorCode);
            break;
        case SSAP_FIND_STRUCTURE_REQ:
        case SSAP_FIND_STRUCTURE_BY_UUID_REQ:
            SSAPC_FindReqErrorHandle(link, errorRsp->errorCode);
            break;
        case SSAP_READ_REQ:
        case SSAP_WRITE_REQ:
            SSAPC_ValueErrorHandle(link, errorRsp->errorCode);
            break;
        case SSAP_VALUE_IND:
            break;
        case SSAP_READ_BY_UUID_REQ:
            SSAPC_ReadByUuidErrorHandle(link, errorRsp->errorCode);
            break;
        case SSAP_CALL_METHOD_REQ:
            SSAPC_CallMethodErrorHandle(link, errorRsp->errorCode);
            break;
        default:
            break;
    }
}

#ifdef __cplusplus
}
#endif