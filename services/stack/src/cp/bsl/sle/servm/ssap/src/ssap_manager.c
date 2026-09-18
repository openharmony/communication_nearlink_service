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

#define SSAP_REASSEM_GROW_STEP (SSAP_STACK_MTU_MAX * 4)   // 分包重组缓冲区扩容步长（4倍MTU上限）

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
    PrintFormatHexWithSpaces(dataBuf, dataSize, false);

    uint8_t ret = SSAP_CheckOpcode(link, op);
    if (ret == SSAP_ERRCODE_UNSUPPORT_PDU) {
        SSAP_PduErrorRsp(link, op, 0, SSAP_ERRCODE_UNSUPPORT_PDU);
    }
    if (ret != SSAP_ERRCODE_SUCCESS) {
        CP_LOG_ERROR("[SSAP] recv opcode check failed, errcode = %x", ret);
        return SSAP_STACK_FAILED;
    }

    uint8_t type = SSAP_GetOpcodeType(op);
    // 分片报文：分发给各信令处理函数自行组包，组包未完成前不清理任务
    if (SSAP_IsFragPkt(buff)) {
        // 分片响应/确认：task有效时组包完成后由组包工具清理任务；task失效时对齐单包路径清理并续跑队列
        if (type == SSAP_TRANS_RSP || type == SSAP_TRANS_ACK) {
            if (SsapTaskValid(link)) {
                SSAP_DataDispatch(link, buff);
            } else {
                SSAP_LinkClearCurrentTask(link);
                SSAP_ExcuteProcessTask(link);
            }
        } else {
            SSAP_DataDispatch(link, buff);
        }
        return SSAP_STACK_SUCCESS;
    }
    // 非响应或者ACK消息，直接处理
    if (type != SSAP_TRANS_RSP && type != SSAP_TRANS_ACK) {
        SSAP_DataDispatch(link, buff);
        return SSAP_STACK_SUCCESS;
    }
    // 如果收到的是响应或者ACK消息，需要调用完成任务的函数，释放任务参数和相关的buffer
    // appid解注册后task会无效，回复无需处理
    if (SsapTaskValid(link)) {
        SSAP_DataDispatch(link, buff);
    }
    SSAP_LinkClearCurrentTask(link);
    // 处理完消息之后，尝试继续执行链路上的任务
    SSAP_ExcuteProcessTask(link);
    return SSAP_STACK_SUCCESS;
}

/**
 * @brief  判断报文是否为SSAP分片报文
 */
bool SSAP_IsFragPkt(SDF_Buff_S *buff)
{
    if (buff == NULL || SDF_DataLenGet(buff) < SSAP_PDU_BASE_LEN) {
        return false;
    }
    uint8_t *data = SDF_DataOffset(buff);
    // 仅ctrl含fragment字段的PDU参与分包判断（白名单），其余PDU（错误响应、信息交换、
    // 服务发现请求、按UUID读取请求）的ctrl低bit不是分片标记，不参与分包
    switch (data[0]) {
        case SSAP_FIND_STRUCTURE_RSP:
        case SSAP_FIND_STRUCTURE_BY_UUID_RSP:
        case SSAP_READ_REQ:
        case SSAP_READ_RSP:
        case SSAP_READ_BY_UUID_RSP:
        case SSAP_WRITE_CMD:
        case SSAP_WRITE_REQ:
        case SSAP_WRITE_RSP:
        case SSAP_VALUE_NTF:
        case SSAP_VALUE_IND:
        case SSAP_VALUE_ACK:
        case SSAP_CALL_METHOD_CMD:
        case SSAP_CALL_METHOD_REQ:
        case SSAP_CALL_METHOD_RSP:
            return (data[1] & SSAP_CTRL_FRAG_MASK) != SSAP_CTRL_NO_FRAG;
        default:
            return false;
    }
}

static bool SSAP_ReassemAppend(SSAP_Link_S *link, const uint8_t *data, uint32_t len)
{
    CP_CHECK_LOG_RETURN(link->fragCtx.reassemBuff != NULL, false, "[SSAP] reassem buff is null");
    SDF_Buff_S *buff = link->fragCtx.reassemBuff;
    if (SDF_DataLenGet(buff) + len > SSAP_REASSEM_MAX_SIZE) {
        CP_LOG_ERROR("[SSAP] reassem data over max size, total: %d", (int32_t)(SDF_DataLenGet(buff) + len));
        SDF_BuffFree(buff);
        link->fragCtx.reassemBuff = NULL;
        return false;
    }
    // 尾部空间不足时扩容，避免重组大报文时追加失败
    if (SDF_BuffTailRoom(buff) < len) {
        uint32_t newSize = SDF_BuffLenGet(buff) + len + SSAP_REASSEM_GROW_STEP;
        SDF_Buff_S *newBuff = SDF_BuffNewWithReserve(newSize);
        if (newBuff == NULL) {
            CP_LOG_ERROR("[SSAP] reassem buff grow fail");
            SDF_BuffFree(buff);
            link->fragCtx.reassemBuff = NULL;
            return false;
        }
        uint8_t *dst = SDF_BuffAppend(newBuff, SDF_DataLenGet(buff));
        if (dst == NULL) {
            CP_LOG_ERROR("[SSAP] reassem buff copy fail");
            SDF_BuffFree(newBuff);
            SDF_BuffFree(buff);
            link->fragCtx.reassemBuff = NULL;
            return false;
        }
        (void)memcpy_s(dst, SDF_DataLenGet(buff), SDF_DataOffset(buff), SDF_DataLenGet(buff));
        SDF_BuffFree(buff);
        link->fragCtx.reassemBuff = newBuff;
        buff = newBuff;
    }
    uint8_t *dst = SDF_BuffAppend(buff, len);
    if (dst == NULL) {
        CP_LOG_ERROR("[SSAP] reassem buff append fail");
        SDF_BuffFree(buff);
        link->fragCtx.reassemBuff = NULL;
        return false;
    }
    (void)memcpy_s(dst, len, data, len);
    return true;
}

/**
 * @brief  分片接收超时回调：分片未在超时时间内收齐时清空重组缓存
 */
static void SSAP_ReassemTimeoutCbk(void *arg)
{
    CP_LOG_INFO("[SSAP] reassem timeout, drop unfinished reassem buff");
    CP_CHECK_LOG_RETURN_VOID(arg != NULL, "[SSAP] reassem timeout cbk arg is null");
    SSAP_Link_S *link = (SSAP_Link_S *)arg;
    if (link->fragCtx.reassemBuff != NULL) {
        SDF_BuffFree(link->fragCtx.reassemBuff);
        link->fragCtx.reassemBuff = NULL;
    }
    SSAP_DelReassemTimer(link);
}

/**
 * @brief  BEGIN分片处理：丢弃未完成的旧缓存，还原ctrl后创建重组缓存并启动超时定时器
 */
static SSAP_ReassemStatus_E SSAP_ReassemStart(SSAP_Link_S *link, uint8_t op, const uint8_t *dataBuf,
    uint32_t dataSize)
{
    // 异常情况：上一包还未组包完成，丢弃旧的重组数据
    if (link->fragCtx.reassemBuff != NULL) {
        CP_LOG_ERROR("[SSAP] recv fragment begin with unfinished reassem, drop old");
        SDF_BuffFree(link->fragCtx.reassemBuff);
        link->fragCtx.reassemBuff = NULL;
        SSAP_DelReassemTimer(link);
    }
    // 还原ctrl：fragment位恢复为NO_FRAG，WRITE类报文同时将oper恢复为立即写入
    uint8_t newCtrl = (dataBuf[1] & ~SSAP_CTRL_FRAG_MASK) | SSAP_CTRL_NO_FRAG;
    if (op == SSAP_WRITE_REQ || op == SSAP_WRITE_CMD) {
        newCtrl &= ~SSAP_CTRL_WRITE_OPER_MASK;
    }
    // 首次申请按扩容步长预留空间，避免后续小分片反复触发扩容（扩容需全量拷贝）
    SDF_Buff_S *reassemBuff = SDF_BuffNewWithReserve(dataSize + SSAP_REASSEM_GROW_STEP);
    if (reassemBuff == NULL) {
        CP_LOG_ERROR("[SSAP] reassem buff malloc fail");
        return SSAP_REASSEM_DROP;
    }
    uint8_t *dst = SDF_BuffAppend(reassemBuff, dataSize);
    if (dst == NULL) {
        CP_LOG_ERROR("[SSAP] reassem buff create fail");
        SDF_BuffFree(reassemBuff);
        return SSAP_REASSEM_DROP;
    }
    dst[0] = op;
    dst[1] = newCtrl;
    (void)memcpy_s(dst + SSAP_PDU_BASE_LEN, dataSize - SSAP_PDU_BASE_LEN,
        dataBuf + SSAP_PDU_BASE_LEN, dataSize - SSAP_PDU_BASE_LEN);
    link->fragCtx.reassemBuff = reassemBuff;
    link->fragCtx.reassemOp = op;
    // 启动分片接收超时定时器，分片未收齐超时后清理重组上下文
    SSAP_DelReassemTimer(link);
    if (!SSAP_StartReassemTimer(link, SSAP_ReassemTimeoutCbk)) {
        CP_LOG_ERROR("[SSAP] start reassem timer failed, drop reassem buff");
        SDF_BuffFree(link->fragCtx.reassemBuff);
        link->fragCtx.reassemBuff = NULL;
        return SSAP_REASSEM_DROP;
    }
    CP_LOG_INFO("[SSAP] recv fragment begin, opcode: 0x%x, fragSize: %d", op, dataSize);
    return SSAP_REASSEM_WAIT;
}

/**
 * @brief  MID/END分片处理：追加载荷，END到达时组包完成并通过complete输出完整报文
 */
static SSAP_ReassemStatus_E SSAP_ReassemContinue(SSAP_Link_S *link, SDF_Buff_S *buff, SDF_Buff_S **complete)
{
    uint8_t *dataBuf = SDF_DataOffset(buff);
    uint32_t dataSize = (uint32_t)SDF_DataLenGet(buff);
    uint8_t op = dataBuf[0];
    uint8_t fragType = dataBuf[1] & SSAP_CTRL_FRAG_MASK;
    // 无重组上下文或opcode不一致，分片异常，直接丢弃
    if (link->fragCtx.reassemBuff == NULL || link->fragCtx.reassemOp != op) {
        CP_LOG_ERROR("[SSAP] recv fragment mid/end without begin, drop, opcode: 0x%x", op);
        return SSAP_REASSEM_DROP;
    }
    if (!SSAP_ReassemAppend(link, dataBuf + SSAP_PDU_BASE_LEN, dataSize - SSAP_PDU_BASE_LEN)) {
        CP_LOG_ERROR("[SSAP] reassem append fail, drop, opcode: 0x%x", op);
        SSAP_DelReassemTimer(link);
        return SSAP_REASSEM_DROP;
    }
    if (fragType == SSAP_CTRL_FRAG_MID) {
        CP_LOG_DEBUG("[SSAP] recv fragment mid, opcode: 0x%x, fragSize: %d", op, dataSize);
        return SSAP_REASSEM_WAIT;
    }
    // 末片到达，组包完成，完整报文交给信令处理函数
    CP_LOG_INFO("[SSAP] recv fragment end, opcode: 0x%x, totalSize: %d", op,
        (int32_t)SDF_DataLenGet(link->fragCtx.reassemBuff));
    *complete = link->fragCtx.reassemBuff;
    link->fragCtx.reassemBuff = NULL;
    SSAP_DelReassemTimer(link);
    return SSAP_REASSEM_COMPLETE;
}

/**
 * @brief  各信令处理函数的分片组包工具
 * @return SSAP_REASSEM_WAIT：分片已缓存，等待后续分片；SSAP_REASSEM_COMPLETE：组包完成，complete输出完整报文；
 *         SSAP_REASSEM_DROP：分片异常，已丢弃；SSAP_REASSEM_CANCEL：写入类分片被取消，缓存已清空
 */
SSAP_ReassemStatus_E SSAP_ReassemFragment(SSAP_Link_S *link, SDF_Buff_S *buff, SDF_Buff_S **complete)
{
    CP_CHECK_LOG_RETURN(link != NULL && buff != NULL && complete != NULL, SSAP_REASSEM_DROP,
        "[SSAP] reassem param is null");
    uint8_t *dataBuf = SDF_DataOffset(buff);
    uint32_t dataSize = (uint32_t)SDF_DataLenGet(buff);
    CP_CHECK_LOG_RETURN(dataSize >= SSAP_PDU_BASE_LEN, SSAP_REASSEM_DROP, "[SSAP] frag pkt len invalid");
    uint8_t op = dataBuf[0];
    // 取消写入：WRITE类报文ctrl中oper为取消写入(0b10)时，清空已缓存的分片，由调用方决定是否响应
    if ((op == SSAP_WRITE_REQ || op == SSAP_WRITE_CMD) &&
        (dataBuf[1] & SSAP_CTRL_WRITE_OPER_MASK) == (SSAP_CTRL_WRITE_CANCEL << SSAP_CTRL_WRITE_OPER_SHIFT)) {
        CP_LOG_INFO("[SSAP] recv write cancel, opcode: 0x%x", op);
        if (link->fragCtx.reassemBuff != NULL) {
            SDF_BuffFree(link->fragCtx.reassemBuff);
            link->fragCtx.reassemBuff = NULL;
        }
        SSAP_DelReassemTimer(link);
        return SSAP_REASSEM_CANCEL;
    }
    uint8_t fragType = dataBuf[1] & SSAP_CTRL_FRAG_MASK;
    *complete = NULL;
    // 单个分片包长度防护：分片不允许超过MTU（对端可能任意发送），超限丢弃并清理重组上下文
    if (dataSize > link->mtu) {
        CP_LOG_ERROR("[SSAP] recv frag pkt len over mtu, drop, opcode: 0x%x, len: %d, mtu: %d",
            op, dataSize, link->mtu);
        if (link->fragCtx.reassemBuff != NULL) {
            SDF_BuffFree(link->fragCtx.reassemBuff);
            link->fragCtx.reassemBuff = NULL;
        }
        SSAP_DelReassemTimer(link);
        return SSAP_REASSEM_DROP;
    }
    if (fragType == SSAP_CTRL_FRAG_BEGIN) {
        return SSAP_ReassemStart(link, op, dataBuf, dataSize);
    }
    if (fragType == SSAP_CTRL_FRAG_MID || fragType == SSAP_CTRL_FRAG_END) {
        return SSAP_ReassemContinue(link, buff, complete);
    }
    return SSAP_REASSEM_DROP;
}

/**
 * @brief  分片接收统一处理入口：分片报文组包完成后调用proc处理完整报文
 *         响应/确认类报文组包完成后自动清理链路任务并继续执行排队任务
 * @param [in] proc       组包完成后的完整报文处理回调
 * @param [in] cancelProc 取消写入回调（WRITE类分片被取消时调用，可为NULL）
 * @return true=报文已按分片处理（含WAIT/DROP/CANCEL）；false=非分片报文，调用方按单包处理
 */
bool SSAP_HandleFragRecv(SSAP_Link_S *link, SDF_Buff_S *sdfBuff, SSAP_FragProcFunc proc,
    SSAP_FragCancelFunc cancelProc)
{
    if (!SSAP_IsFragPkt(sdfBuff)) {
        return false;
    }
    SDF_Buff_S *complete = NULL;
    SSAP_ReassemStatus_E status = SSAP_ReassemFragment(link, sdfBuff, &complete);
    if (status == SSAP_REASSEM_CANCEL && cancelProc != NULL) {
        cancelProc(link);
        return true;
    }
    if (status == SSAP_REASSEM_WAIT || status == SSAP_REASSEM_DROP || status == SSAP_REASSEM_CANCEL) {
        return true;
    }
    // 组包完成：置位组包完成标志后回调proc处理完整报文（长度防护豁免依据），proc返回后复位
    link->fragCtx.reassemComplete = true;
    if (proc != NULL) {
        proc(link, complete);
    }
    link->fragCtx.reassemComplete = false;
    // 响应/确认类组包完成后清理任务（opcode由完整报文推导，与调用方显式传参等价）
    uint8_t type = SSAP_GetOpcodeType(*(SDF_DataOffset(complete)));
    SDF_BuffFree(complete);
    if (type == SSAP_TRANS_RSP || type == SSAP_TRANS_ACK) {
        SSAP_HandleRspTaskComplete(link);
    }
    return true;
}

/**
 * @brief  分片响应/确认报文组包完成后的任务清理，由各响应处理函数在组包完成后调用
 */
void SSAP_HandleRspTaskComplete(SSAP_Link_S *link)
{
    SSAP_LinkClearCurrentTask(link);
    // 处理完消息之后，尝试继续执行链路上的任务
    SSAP_ExcuteProcessTask(link);
}

static bool SSAP_SendBuffToDTAP(uint16_t lcid, SDF_Buff_S *buff)
{
    SDF_Buff_S *tmpBuff = SDF_BuffCopy(buff);
    CP_CHECK_LOG_RETURN(tmpBuff != NULL, false, "[SSAP] copy buf fail");
    PrintFormatHexWithSpaces(SDF_DataOffset(tmpBuff), SDF_DataLenGet(tmpBuff), true);
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

/**
 * @brief  发送完成后的任务注册：REPLY类报文保存完整报文并启动超时定时器，非REPLY类释放报文
 */
static void SSAP_RegisterReplyTask(SSAP_Link_S *link, SDF_Buff_S *buff, uint8_t opcode)
{
    uint8_t type = SSAP_GetOpcodeType(opcode);
    if ((type & SSAP_REPLY_MASK) == 0) {
        SDF_BuffFree(buff);
        return;
    }
    SSAP_LinkSetTask(link, buff, opcode);
    if (!SSAP_StartTimer(link, SSAP_TimeoutCbk)) {
        CP_LOG_ERROR("[SSAP] create timer failed");
    }
}

/**
 * @brief  构造单个分片报文：按index设置BEGIN/MID/END标记，WRITE类非末片置oper为继续写入
 */
static SDF_Buff_S *SSAP_BuildFragBuff(SDF_Buff_S *buff, uint32_t index, uint32_t maxPayload)
{
    uint8_t *dataBuf = SDF_DataOffset(buff);
    uint32_t dataSize = (uint32_t)SDF_DataLenGet(buff);
    uint8_t opcode = dataBuf[0];
    uint8_t baseCtrl = dataBuf[1] & ~SSAP_CTRL_FRAG_MASK;
    uint32_t fragCount = (dataSize - SSAP_PDU_BASE_LEN + maxPayload - 1) / maxPayload;
    uint8_t fragType = SSAP_CTRL_FRAG_MID;
    if (index == 0) {
        fragType = SSAP_CTRL_FRAG_BEGIN;
    } else if (index == fragCount - 1) {
        fragType = SSAP_CTRL_FRAG_END;
    }
    uint32_t offset = SSAP_PDU_BASE_LEN + index * maxPayload;
    uint32_t fragPayloadLen = (dataSize - offset > maxPayload) ? maxPayload : (dataSize - offset);
    uint32_t fragLen = SSAP_PDU_BASE_LEN + fragPayloadLen;
    SDF_Buff_S *fragBuff = SDF_BuffNewWithReserve(fragLen);
    if (fragBuff == NULL) {
        CP_LOG_ERROR("[SSAP] send fragment buff malloc fail");
        return NULL;
    }
    uint8_t *fragData = SDF_BuffAppend(fragBuff, fragLen);
    if (fragData == NULL) {
        CP_LOG_ERROR("[SSAP] send fragment buff create fail");
        SDF_BuffFree(fragBuff);
        return NULL;
    }
    fragData[0] = opcode;
    fragData[1] = baseCtrl | fragType;
    // 分包写入报文：非末片指示对端继续接收后续值，末片恢复立即写入
    if ((opcode == SSAP_WRITE_REQ || opcode == SSAP_WRITE_CMD) && fragType != SSAP_CTRL_FRAG_END) {
        fragData[1] &= ~SSAP_CTRL_WRITE_OPER_MASK;
        fragData[1] |= (SSAP_CTRL_WRITE_PART << SSAP_CTRL_WRITE_OPER_SHIFT);
    }
    (void)memcpy_s(fragData + SSAP_PDU_BASE_LEN, fragPayloadLen, dataBuf + offset, fragPayloadLen);
    return fragBuff;
}

/**
 * @brief  分包发送实现（由SSAP_Send统一调用，亦可单独测试）：报文超MTU时按BEGIN/MID/END分片发送
 * @return 分包发送是否成功；报文任务管理（REPLY类设置任务与定时器）与SSAP_Send保持一致
 */
bool SSAP_SendFragPkt(SSAP_Link_S *link, SDF_Buff_S *buff, uint8_t opcode)
{
    CP_CHECK_LOG_RETURN(link != NULL && buff != NULL, false, "[SSAP] param is null");
    uint32_t dataSize = (uint32_t)SDF_DataLenGet(buff);
    CP_CHECK_LOG_RETURN(dataSize > SSAP_PDU_BASE_LEN, false, "[SSAP] frag pkt len invalid");
    uint32_t maxPayload = link->mtu - SSAP_PDU_BASE_LEN;
    uint32_t payloadSize = dataSize - SSAP_PDU_BASE_LEN;
    uint32_t fragCount = (payloadSize + maxPayload - 1) / maxPayload;
    CP_LOG_INFO("[SSAP] send fragmented msg, opcode: 0x%x, size: %d, fragCount: %d", opcode, dataSize, fragCount);
    for (uint32_t index = 0; index < fragCount; index++) {
        SDF_Buff_S *fragBuff = SSAP_BuildFragBuff(buff, index, maxPayload);
        if (fragBuff == NULL) {
            // 构造分片失败时释放原始完整报文，避免内存泄漏；部分已发出的分片由对端重组超时清理
            SDF_BuffFree(buff);
            return false;
        }
        if (!SSAP_SendBuffToDTAP(link->lcid, fragBuff)) {
            CP_LOG_ERROR("[SSAP] send fragment failed, index: %d", index);
            SDF_BuffFree(fragBuff);
            // 发送失败时释放原报文，避免内存泄漏；部分已发出的分片由对端重组超时清理
            SDF_BuffFree(buff);
            return false;
        }
        SDF_BuffFree(fragBuff);
    }
    // 任务注册与SSAP_Send保持一致：REPLY类报文保存完整报文并启动超时定时器
    SSAP_RegisterReplyTask(link, buff, opcode);
    return true;
}

/**
 * @brief  链路发送回调：报文超MTU且对端支持分包时分包发送，否则单包发送（所有信令统一发送入口）
 */
void SSAP_Send(SSAP_Link_S *link, SDF_Buff_S *buff, uint8_t opcode)
{
    CP_CHECK_LOG_RETURN_VOID(link != NULL && buff != NULL, "[SSAP] param is null");
    CP_CHECK_LOG_RETURN_VOID(SDF_DataLenGet(buff) <= UINT32_MAX, "[SSAP] recv datalen is invalid");
    uint32_t dataSize = (uint32_t)SDF_DataLenGet(buff);
    CP_CHECK_LOG_RETURN_VOID(dataSize > 0, "[SSAP] send buff len is 0");
    CP_LOG_DEBUG("[SSAP] send msg len: %d, opcode: 0x%x", dataSize, opcode);
    uint8_t type = SSAP_GetOpcodeType(opcode);
    if (type == SSAP_TRANS_INVALID) {
        CP_LOG_ERROR("[SSAP] unknown opcode");
        SDF_BuffFree(buff);
        return;
    }
    // 报文超MTU且对端支持分包：分包发送（SSAP_SendFragPkt内部完成逐片下发与任务注册）
    if (dataSize > link->mtu && link->fragCtx.fragment) {
        (void)SSAP_SendFragPkt(link, buff, opcode);
        return;
    }
    if (!SSAP_SendBuffToDTAP(link->lcid, buff)) {
        SDF_BuffFree(buff);
        return;
    }
    SSAP_RegisterReplyTask(link, buff, opcode);
}

static void SSAP_CMLogicLinkCbk(CM_LogicLinkState_S *param)
{
    CP_CHECK_LOG_RETURN_VOID(param != NULL, "[SSAP] cm param is null");
    CP_LOG_INFO("[SSAP] cm link lcid: %d, addr: %s, result: %d", param->lcid, GET_ENC_ADDR(&param->addr),
        param->result);
    if (param->result == CM_LINK_STATE_CONNECTED) {
        // 建链后第一条信令默认为EXCHANGE_INFO_REQ：优先完成MTU/分包能力协商，
        // 后续FIND请求才能按协商结果设置响应模式（对端支持分包时置MULTI_RSP，避免服务端超MTU截断）
        SSAP_Link_S *link = SSAP_CreateSsapLink(&param->addr, param->lcid, SSAP_Send);
        CP_CHECK_LOG_RETURN_VOID(link != NULL, "[SSAP] create ssap link failed");
        // 更新链路的实际连接状态
        SsapLinkHandleRecordLinkStateFromCm(&(param->addr),  SSAP_CONNECT_STATE_CONNECTED);
        SsapTriggerLinkStateMachineChange(&(param->addr), SSAP_LOGIC_LINK_CONNECTED, param->discReason);
        NLSTK_Errcode_E cacheCreateRet = SsapcCacheCreate(&(param->addr));
        CP_LOG_INFO("[SSAP] ssap client cache create ret: %d", cacheCreateRet);
        SSAP_ExchangeInfoReqInfo_S *exchangeParam =
            (SSAP_ExchangeInfoReqInfo_S *)SDF_MemZalloc(sizeof(SSAP_ExchangeInfoReqInfo_S));
        CP_CHECK_LOG_RETURN_VOID(exchangeParam != NULL, "[SSAP] init exchange info req malloc failed");
        (void)memcpy_s(&exchangeParam->addr, sizeof(SLE_Addr_S), &param->addr, sizeof(SLE_Addr_S));
        exchangeParam->mtu = link->mtu;
        SSAP_TaskParam_S taskParam = {.appId = SSAP_APP_INVALID_ID, .arg = exchangeParam, .freeFunc = SDF_MemFree,
            .func = SSAPC_ExchangeInfoReq, .timeout = SSAP_INTERACTION_MAX_TIMEOUT, .valid = true};
        // SSAP默认EXCHANGE_INFO_REQ添加至缓存队列。此时配对之类的流程还未结束，不影响芯片交互流程，因此延迟发送
        SSAP_ProcessRequestTask(link, &taskParam, true);
    } else if (param->result == CM_LINK_STATE_DISCONNECTED) {
        SSAP_DeleteSsapLinkByAddr(&param->addr);
        // 更新链路的实际连接状态
        SsapLinkHandleRecordLinkStateFromCm(&(param->addr),  SSAP_CONNECT_STATE_DISCONNECTED);
        SsapTriggerLinkStateMachineChange(&(param->addr), SSAP_LOGIC_LINK_DISCONNECTED, param->discReason);
        SsapcCacheDestroy(&(param->addr));
        SSAPS_CleanPendingVectorByAddr(&(param->addr));
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