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

#include "dli_layer.h"

#include <stdint.h>
#include "securec.h"
#include <signal.h>
#include <unistd.h>
#include <stdatomic.h>

#include "dli_event_struct.h"
#include "dli_layer_utils.h"
#include "dli_sapi.h"
#include "dli_thread.h"
#include "dli_def.h"
#include "dli_log.h"
#include "dli_errno.h"
#include "sdf_mem.h"
#include "sdf_thread.h"
#include "dli_layer_config.h"
#include "dli_layer_callback.h"
#include "sdf_dlist.h"
#include "sdf_mutex.h"
#include "sdf_timer.h"
#include "sdf_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DLI_DATA_MAX_CNT 10000
#define CMD_EVC_TIMEOUT 10000 /* 10000 ms */
#define DLI_CONNECTION_HDL_BIT_LEN 12
#define DLI_PB_BIT_LEN 2
#define DLI_PAYLOAD_BITS_LEN 2
#define DLI_RESET_EVENT_SIZE 5
#define DLI_DATA_LEN_OFFSET 2
#define DLI_TRY_SEND_CNT 3
#define DLI_TRY_SEND_SLEEP_TIME 10 /* 10 ms */

#define DLI_STATUS_EVENT                        0x0001
#define DLI_COMPLETE_EVENT                      0x0002
#define DLI_OPCODE_BYTE_LENGTH                  2

struct DLI_EventTaskParam {
    uint16_t event;
    DLI_CmdTxNode *node;
    SDF_Buff_S *buf;
};

typedef struct RecvPacket {
    uint32_t size;
    uint8_t data[0];
} RecvPacket;

struct DLI_LayerInfo {
    SDF_DListHead_S cmdTx;
    SDF_DListHead_S dataRx; // used stack main thread
    uint8_t pdFlag;
    bool inited;
};

static struct DLI_LayerInfo g_info;

static inline bool DLI_Controlinited(void)
{
    return g_info.inited;
}

static uint32_t DLI_LayerInfoInit(void)
{
    DLI_CHECK_RETURN_RET(!DLI_Controlinited(),
        DLI_STACK_INITED_ERRNO, "DLI_LayerInfoInit already inited");
    SDF_DListHeadInit(&(g_info.cmdTx));
    SDF_DListHeadInit(&(g_info.dataRx));
    g_info.pdFlag = UINT8_MAX;
    DLI_CmdNumSet(DLI_DEFAULT_CMD_NUM);
    g_info.inited = true;
    return 0;
}

void DLI_LayerInfoDeInit(void)
{
    DLI_CHECK_RETURN(DLI_Controlinited(), "DLI_LayerInfoDeInit not inited");
    DLI_CmdNumSet(DLI_DEFAULT_CMD_NUM);
    DLI_CmdDlistDestroy(&(g_info.cmdTx));
    DLI_RxDataDlistDestroy(&(g_info.dataRx));
    g_info.pdFlag = UINT8_MAX;
    g_info.inited = false;
}

static void DLI_PacketReceived(SlePacketType type, const SlePacket *packet);
uint32_t DLI_LayerInit(void)
{
    DLI_LOGI("DLI_LayerInit begin");
    uint32_t ret = DLI_ThreadInit();
    if (ret != 0) {
        return ret;
    }
    DLI_LOGI("DLI_LayerInit end");
    return 0;
}

uint32_t DLI_LayerEnable(void)
{
    uint32_t ret = DLI_LayerInfoInit();
    DLI_CHECK_RETURN_RET(ret == 0, ret, "DLI_LayerInfoInit err");
    ret = DLI_SapiInit(DLI_PacketReceived);
    if (ret != 0) {
        DLI_LayerInfoDeInit();
        DLI_LOGE("DLI_SapiInit err");
    }
    return ret;
}

static int DLI_CmdListSendInner(DLI_CmdStru *info)
{
    DLI_LOGI("DLI_SapiSend, cmd:0x%04x, event:0x%04x", info->cmd, info->event);
    uint8_t *p = info->par;
    *p = DLI_DATATYPE_CMD;
    DLI_ENCODE2BYTE(p + 1, info->cmd);
    DLI_ENCODE2BYTE(p + 1 + DLI_PAYLOAD_BITS_LEN, info->parLen);
    int result = DLI_SapiSend(p, info->parLen + DLI_HEADER);
    DLI_FileWriteHandler(DLI_DATATYPE_CMD, p, info->parLen + DLI_HEADER, result);
    return result;
}

static void DLI_CmdTimerCallback(void *arg)
{
    DLI_CHECK_RETURN(arg != NULL, "DLI_CmdTimerCallback arg is null");
    DLI_CmdTxNode *cmdNode = (DLI_CmdTxNode*)arg;
    DLI_CHECK_RETURN(cmdNode->info != NULL, "cmdNode info is null");

    DLI_LOGI("DLI_CmdTimerCallback cmd %hu timeout", cmdNode->info->cmd);
    // 默认超时删除对应cmd节点，触发下一条命令
    SDF_DListElmDel(&(g_info.cmdTx), cmdNode, node);

    if (cmdNode->info->timeoutCallback != NULL) {
        if (DLI_PostOtherThread(cmdNode->info->timeoutCallback, (void *)cmdNode, DLI_CmdNodeDestroy) != 0) {
            DLI_LOGE("post timeout evt failed.");
        }
    }

    DLI_PostNextTask(DLI_CMD_TASK);
}

static inline void DLI_TimerParamInit(SDF_TimerParam *param)
{
    param->handle = DLI_ThreadEvcHandleGet();
    param->period = false;
    param->expires = CMD_EVC_TIMEOUT;
    param->args = NULL;
    param->callback = DLI_CmdTimerCallback;
}

static void DLI_CmdListSend(void)
{
    SDF_TimerParam param;
    DLI_TimerParamInit(&param);

    DLI_CmdTxNode *cmdNode = NULL;
    DLI_CmdTxNode *tmp = NULL;
    SDF_DListElmSafeForeach(cmdNode, tmp, &(g_info.cmdTx), node) {
        if (!DLI_IsCmdAllow()) {
            DLI_LOGD("dli cmd: 0x%04X is not allow", cmdNode->info->cmd);
            return;
        }
        if (cmdNode->isSent) {
            DLI_LOGD("cmd 0x%04X is sent, but no recv event 0x%04X", cmdNode->info->cmd, cmdNode->info->event);
            continue;
        }
        // 加入链表的时候已经保证了info指针不会为空
        if (DLI_CmdListSendInner(cmdNode->info) != 0) {
            DLI_LOGE("DLI_CmdListSendInner failed");
            break;
        }
        cmdNode->isSent = true;
        param.args = (void*)cmdNode;
        uint32_t ret = SDF_TimerAdd(&cmdNode->handle, &param);
        if (ret != 0) {
            DLI_LOGE("SDF_TimerAdd failed, errno %u", ret);
        }
        DLI_CmdNumSubOne();
    }
}

static uint32_t DLI_CmdAddToList(DLI_CmdTxNode *cmdNode)
{
    DLI_CHECK_RETURN_RET(SDF_DListCount(&(g_info.cmdTx)) <= DLI_DATA_MAX_CNT,
        DLI_STACK_MEM_ERRNO,
        "DLI_CmdAddToList cnt is more than max cnt %u",
        DLI_DATA_MAX_CNT);
    SDF_DListElmTailInsert(&(g_info.cmdTx), cmdNode, node);
    return 0;
}

static void DLI_CmdSendInner(void *param)
{
    DLI_CmdTxNode *cmdNode = (DLI_CmdTxNode *)param;
    if (!DLI_Controlinited()) {
        DLI_LOGE("DLI_Controlinited is not init");
        DLI_CmdNodeDestroy((void *)cmdNode);
        return;
    }

    uint32_t ret = DLI_CmdAddToList(cmdNode);
    DLI_CmdListSend();
    if (ret == 0) {
        return;
    }

    // 再次尝试加入链表
    ret = DLI_CmdAddToList(cmdNode);
    if (ret != 0) {
        DLI_CmdNodeDestroy((void *)cmdNode);
        DLI_LOGE("DLI_CmdSendInner add cmd to list errno %u", ret);
    }
}

uint32_t DLI_CmdSend(DLI_CmdStru *cmd)
{
    DLI_CHECK_RETURN_RET(cmd, DLI_STACK_PARAMS_ERRNO, "cmd is null");

    DLI_CmdTxNode *node = DLI_CmdNodeCreate(cmd);
    DLI_CHECK_RETURN_RET(node, DLI_STACK_MEM_ERRNO, "node is null");

    uint32_t ret = DLI_PostTask(DLI_CmdSendInner, (void*)node, NULL);
    if (ret != 0) {
        // node需要释放，入参cmd需求调用者自己释放
        node->info = NULL;
        DLI_CmdNodeDestroy((void *)node);
    }

    return ret;
}

static uint8_t DLI_PdFlagGet(uint64_t remainLen, uint16_t oneSendLen)
{
    uint8_t pdFlag = DLI_FIRST_FRAGMENT;
    switch (g_info.pdFlag) {
        case DLI_FIRST_FRAGMENT:
        case DLI_MIDDLE_FRAGMENT:
            pdFlag = (remainLen > oneSendLen) ? DLI_MIDDLE_FRAGMENT : DLI_LAST_FRAGMENT;
            break;
        case DLI_LAST_FRAGMENT:
        case DLI_FULL_FRAGMENT:
        case UINT8_MAX:
            pdFlag = (remainLen > oneSendLen) ? DLI_FIRST_FRAGMENT : DLI_FULL_FRAGMENT;
            break;
        default:
            DLI_LOGE("DLI_PdFlagGet PdFlag %hhu is err", pdFlag);
            break;
    }
    g_info.pdFlag = pdFlag;
    return pdFlag;
}

uint32_t DLI_GetDataFragmentNums(SDF_Buff_S *buf)
{
    uint16_t bufferLen = DLI_DataLenGet(ACB_DATA_TYPE);
    return bufferLen == 0 ? 0 : (SDF_DataLenGet(buf) + bufferLen - 1) / bufferLen;
}

uint32_t DLI_GetFragmentMaxLen(void)
{
    return DLI_DataLenGet(ACB_DATA_TYPE) + DLI_HEADER;
}

uint32_t DLI_SplitData(DLI_DataStru *data, SDF_Buff_S *fragmentBuf[], uint32_t fragmentCnt)
{
    uint16_t bufferLen = DLI_DataLenGet(ACB_DATA_TYPE);
    uint64_t dataOffset = 0;
    for (uint32_t i = 0; i < fragmentCnt; i++) {
        // 当前节点的全部数据
        uint8_t *info = SDF_DataOffset(data->buf);
        // 当前发送的数据,包含dli的5字节头,且已经保证预留了5字节的dli HEADER
        uint8_t *p = info - DLI_HEADER + dataOffset;
        uint64_t remainLen = SDF_DataLenGet(data->buf) - dataOffset;
        /* 5 bytes header | type (8 bit) | handle (12 bit) | pbFlag (2 bit) |
                        | ts (1 bit) | prio (1 bit) | length (16 bit) | */
        *p = data->type;
        DLI_ENCODE2BYTE(p + 1, data->lcid |
            (DLI_PdFlagGet(remainLen, bufferLen) << DLI_CONNECTION_HDL_BIT_LEN) |
            (data->ts << (DLI_CONNECTION_HDL_BIT_LEN + DLI_PB_BIT_LEN)) |
            (data->prio << (DLI_CONNECTION_HDL_BIT_LEN + DLI_PB_BIT_LEN + 1)));

        uint16_t sendLen = remainLen > bufferLen ? bufferLen : remainLen;
        DLI_ENCODE2BYTE(p + 1 + DLI_PAYLOAD_BITS_LEN, sendLen);

        uint16_t fragmentLen = sendLen + DLI_HEADER;
        uint8_t *fragmentData = SDF_BuffAppend(fragmentBuf[i], fragmentLen);
        if (fragmentData == NULL || memcpy_s(fragmentData, fragmentLen, p, fragmentLen) != EOK) {
            DLI_LOGE("copy fragment data failed");
            return DLI_STACK_MEM_ERRNO;
        }
        dataOffset += sendLen;
    }
    return DLI_SUCCESS;
}

// 多次尝试发送
static int DLI_TrySend(const uint8_t *data, uint32_t len)
{
    int i;
    for (i = 0; i < DLI_TRY_SEND_CNT; i++) {
        if (DLI_SapiSend(data, len) == 0) {
            return 0;
        }
        SDF_ThreadSleep(DLI_TRY_SEND_SLEEP_TIME);
    }
    DLI_LOGW("DLI_TrySend count %d, all send fail", DLI_TRY_SEND_CNT);
    return -1;
}

static void DLI_DataSendInner(void *param)
{
    DLI_DataStru *data = (DLI_DataStru *)param;
    if (!DLI_Controlinited()) {
        DLI_LOGE("DLI_Controlinited is not init");
        DLI_DataStruDestroy(data);
        return;
    }
    uint8_t *buffer = SDF_DataOffset(data->buf);
    uint64_t bufferLen = SDF_DataLenGet(data->buf);
    int result = DLI_TrySend(buffer, bufferLen);
    DLI_LOGD("DLI_DataSendInner sendLen %hu, result %d", (bufferLen - DLI_HEADER), result);
    DLI_FileWriteHandler(data->type, buffer, bufferLen, result);
    DLI_DataStruDestroy(data);
}

static inline bool isDataParamValid(const DLI_DataStru *data)
{
    return (data != NULL && data->buf != NULL &&
        SDF_DataLenGet(data->buf) <= DLI_MAX_TXRX_DATA_LEN);
}

uint32_t DLI_DataSend(DLI_DataStru *data)
{
    DLI_CHECK_RETURN_RET(isDataParamValid(data), DLI_STACK_PARAMS_ERRNO, "param is error");
    return DLI_PostTask(DLI_DataSendInner, (void*)data, NULL);
}

static inline bool DLI_IsFirstPb(uint8_t pb)
{
    return (pb == DLI_FIRST_FRAGMENT || pb == DLI_FULL_FRAGMENT);
}

static bool DLI_CheckPb(uint8_t prePb, uint8_t curPb)
{
    if ((prePb != DLI_MIDDLE_FRAGMENT && prePb == curPb) || prePb > curPb) {
        DLI_LOGW("recv cur pb %hhu, pre pb %hhu, will Discard the data of the node and re-store it", curPb, prePb);
        return false;
    }
    return true;
}

static DLI_RecvDataNode *DLI_ReceiveDataNodeUpdate(SDF_Buff_S *buf, uint16_t lcid, uint16_t pb)
{
    // buf内存第一次直接插入节点，后续内存不够重新分配，需要在此函数内释放入参buf
    DLI_RecvDataNode *dataNode = DLI_DataNodeFind(&(g_info.dataRx), lcid);
    if (dataNode == NULL) {
        if (!DLI_IsFirstPb(pb)) {
            DLI_LOGE("recv pb %hu err", pb);
            SDF_MemFree(buf);
            return NULL;
        }
        dataNode = DLI_RecvDataNodeCreate(lcid, pb, buf);
        if (dataNode == NULL) {
            SDF_MemFree(buf);
            return NULL;
        }
        SDF_DListElmTailInsert(&(g_info.dataRx), dataNode, node);
        return dataNode;
    }

    if (!DLI_CheckPb(dataNode->pb, pb)) {
        // flag检验出错，如果当前是首个分片的包，释放原有的数据，填充新的数据
        if (DLI_IsFirstPb(pb)) {
            SDF_BuffFree(dataNode->buf);
            dataNode->buf = buf;
            dataNode->pb = pb;
            return dataNode;
        }
        goto ERR;
    }

    if (!DLI_ReciveDataUpdate(dataNode, lcid, pb, buf)) {
        DLI_LOGE("DLI_ReciveDataUpdate failed");
        goto ERR;
    }
    SDF_BuffFree(buf);
    return dataNode;

ERR:
    SDF_DListElmDel(&(g_info.dataRx), dataNode, node);
    DLI_RecvDataNodeDestroy(dataNode);
    SDF_BuffFree(buf);
    return NULL;
}

static inline void DLI_CmdWorkCallback(void *param)
{
    DLI_UNUSED(param);
    DLI_CmdListSend();
}

void DLI_PostNextTask(DLI_TaskType type)
{
    (void)type;
    DLI_CHECK_RETURN(DLI_Controlinited(), "DLI_Controlinited is not init");
    uint32_t ret = DLI_PostTask(DLI_CmdWorkCallback, NULL, NULL);
    if (ret != 0) {
        DLI_LOGE("DLI_PostTask failed, ret %u", ret);
    }
}

static void DLI_AcbPacketReceived(void *param)
{
    SDF_Buff_S *buff = (SDF_Buff_S *)param;
    uint8_t *p = SDF_DataOffset(buff);
    uint16_t dataLen = DLI_DECODE2BYTE(p + DLI_DATA_LEN_OFFSET);
    // DLI_PacketReceived已经保证了SDF_DataLenGet(buff)大于 DLI_HEADER_WITHOUT_TYPE_SIZE
    if (dataLen != SDF_DataLenGet(buff) - DLI_HEADER_WITHOUT_TYPE_SIZE) {
        DLI_LOGW("recv packet size %u(size - 4 = dataLen) not match datalen %hu", SDF_DataLenGet(buff), dataLen);
        SDF_BuffFree(buff);
        return;
    }
    uint16_t lcid = DLI_DECODE2BYTE(p);
    uint8_t pb = (uint8_t)((lcid >> DLI_CONNECTION_HDL_BIT_LEN) & 0x03); /* PB */
    lcid &= ((1U << DLI_CONNECTION_HDL_BIT_LEN) - 1); /* 12bits lcid */
    (void)SDF_BuffTrimPrefix(buff, DLI_HEADER_WITHOUT_TYPE_SIZE);

    DLI_RecvDataNode *dataNode = NULL;
    switch (pb) {
        case DLI_FULL_FRAGMENT:
            DLI_AcbRecvHander(lcid, buff);
            SDF_BuffFree(buff);
            break;
        case DLI_FIRST_FRAGMENT:
        case DLI_MIDDLE_FRAGMENT:
            (void)DLI_ReceiveDataNodeUpdate(buff, lcid, pb);
            break;
        case DLI_LAST_FRAGMENT:
            dataNode = DLI_ReceiveDataNodeUpdate(buff, lcid, pb);
            if (dataNode != NULL) {
                SDF_DListElmDel(&(g_info.dataRx), dataNode, node);
                DLI_AcbRecvHander(lcid, dataNode->buf);
                DLI_RecvDataNodeDestroy(dataNode);
            }
            break;
        default:
            DLI_LOGW("recv pb %hhu err", pb);
            SDF_BuffFree(buff);
            break;
    }
}

static DLI_CmdTxNode *DLI_CmdNodeRemove(uint16_t event, uint16_t opcode, uint8_t status)
{
    DLI_CmdTxNode *cmdNode = NULL;
    if (status == DLI_SUCCESS) {
        if (event == DLI_STATUS_EVENT) {
            // 状态事件成功，寻找第一个状态事件未标记的节点，同时标记已经收到，不删除节点，返回null
            cmdNode = DLI_CmdNodeFindNotRecvStatus(&(g_info.cmdTx), opcode);
            if (cmdNode != NULL) {
                cmdNode->isRecvStatusEvt = true;
            }
            return NULL;
        } else if (event == DLI_COMPLETE_EVENT) {
            // 完成事件成功，cmdNode不为null，删除节点
            cmdNode = DLI_CmdNodeFind(&(g_info.cmdTx), event, opcode);
        } else {
            // 最终事件成功，cmdNode不为null，且收到过状态事件，删除节点(否则有可能是自主上报事件)
            cmdNode = DLI_CmdNodeFind(&(g_info.cmdTx), event, opcode);
            if (cmdNode == NULL || !cmdNode->isRecvStatusEvt) {
                return NULL;
            }
        }
    } else {
        if (event == DLI_STATUS_EVENT) {
            // 状态事件失败,删除第一个未收到状态事件的节点
            cmdNode = DLI_CmdNodeFindNotRecvStatus(&(g_info.cmdTx), opcode);
        } else {
            // 其他事件失败,找到对应节点删除
            cmdNode = DLI_CmdNodeFindByOpcode(&(g_info.cmdTx), opcode);
        }
    }
    if (cmdNode == NULL) {
        return NULL;
    }
    if (!cmdNode->isSent) {
        DLI_LOGW("recv event 0x%04x opcode 0x%04x, but cmd not send", event, opcode);
        return NULL;
    }
    if (cmdNode->handle != -1) {
        SDF_TimerDel(cmdNode->handle);
        cmdNode->handle = -1;
    }
    SDF_DListElmDel(&(g_info.cmdTx), cmdNode, node);
    return cmdNode;
}

static inline void DLI_EventDestroy(void *param)
{
    DLI_CHECK_RETURN(param != NULL, "param is null");
    struct DLI_EventTaskParam *info = (struct DLI_EventTaskParam *)param;
    DLI_CmdNodeDestroy(info->node);
    SDF_BuffFree(info->buf);
    SDF_MemFree(info);
}

static void DLI_EventTask(void *param)
{
    DLI_CHECK_RETURN(param != NULL, "param is null");
    struct DLI_EventTaskParam *info = (struct DLI_EventTaskParam *)param;
    DLI_CHECK_RETURN(info->buf != NULL, "info packet is null");

    // 携带上层的context及驱动返回的（剥离4字节的dli头）数据返回给上层
    if (info->node == NULL) {
        DLI_EventRecvHandler(info->event, NULL,
            SDF_DataOffset(info->buf), SDF_DataLenGet(info->buf));
        return;
    }

    DLI_EventRecvHandler(info->event, info->node->info->context,
        SDF_DataOffset(info->buf), SDF_DataLenGet(info->buf));
}

static uint8_t DLI_EventGetStatus(SDF_Buff_S *buf, uint16_t event)
{
    uint8_t status = DLI_SUCCESS;
    if (event == DLI_STATUS_EVENT) {
        if (SDF_DataLenGet(buf) >= sizeof(DLI_CommandStatus)) {
            status = ((DLI_CommandStatus *)SDF_DataOffset(buf))->status;
        } else {
            DLI_LOGE("recv event proc data error");
            status = DLI_UNKNOWN_COMMAND;
        }
    }
    if (event == DLI_COMPLETE_EVENT) {
        if (SDF_DataLenGet(buf) >= sizeof(DLI_CommandComplete) + sizeof(uint8_t)) {
            status = ((DLI_CommandComplete *)SDF_DataOffset(buf))->parameters[0];
        } else {
            DLI_LOGE("recv event proc data error");
            status = DLI_UNKNOWN_COMMAND;
        }
    }
    return status;
}

static void DLI_EventPacketReceived(void *param)
{
    uint16_t opcode = 0;
    SDF_Buff_S *buf = (SDF_Buff_S *)param;
    uint16_t event = DLI_DECODE2BYTE(SDF_DataOffset(buf));
    if (event == DLI_STATUS_EVENT || event == DLI_COMPLETE_EVENT) {
        if (SDF_DataLenGet(buf) >= DLI_HEADER_WITHOUT_TYPE_SIZE + DLI_OPCODE_BYTE_LENGTH) {
            opcode = DLI_DECODE2BYTE(SDF_DataOffset(buf) + DLI_HEADER_WITHOUT_TYPE_SIZE);
        } else {
            DLI_LOGE("recv event proc data error");
        }
    }

    (void)SDF_BuffTrimPrefix(buf, DLI_HEADER_WITHOUT_TYPE_SIZE);
    uint8_t status = DLI_EventGetStatus(buf, event);
    DLI_CmdTxNode *node = DLI_CmdNodeRemove(event, opcode, status);
    struct DLI_EventTaskParam *info =
        (struct DLI_EventTaskParam *)SDF_MemZalloc(sizeof(struct DLI_EventTaskParam));
    if (info == NULL) {
        DLI_LOGE("no find node by event %hu opcode %hu", event, opcode);
        DLI_CmdNodeDestroy((void *)node);
        SDF_BuffFree(buf);
        return;
    }

    info->buf = buf; // buf 在cp线程中释放
    info->event = event;
    info->node = node;
    if (DLI_PostOtherThread(DLI_EventTask, (void*)info, DLI_EventDestroy) != 0) {
        DLI_LOGE("post cp DLI_EventTask failed");
    }
}

static SDF_Buff_S *DLI_CreateRxBuff(const SlePacket *packet)
{
    SDF_Buff_S *buf = SDF_BuffNew(packet->size);
    DLI_CHECK_RETURN_RET(buf, NULL, "SDF_BuffNew error");
    uint8_t *data = SDF_BuffAppend(buf, packet->size);
    if (data == NULL) {
        SDF_BuffFree(buf);
        DLI_LOGE("DLI_PacketReceived SDF_BuffAppend error");
        return NULL;
    }
    (void)memcpy_s(data, packet->size, packet->data, packet->size);
    return buf;
}

static void DLI_PacketReceived(SlePacketType type, const SlePacket *packet)
{
    uint32_t ret = 1;
    DLI_CHECK_RETURN(
        packet && packet->data && packet->size >= DLI_HEADER_WITHOUT_TYPE_SIZE && packet->size <= UINT16_MAX,
        "DLI_PacketReceived param is error");

    DLI_FileWriteHandler(type, packet->data, packet->size, 0);
    uint8_t resetEvent[DLI_RESET_EVENT_SIZE] = {0xff, 0xff, 0x01, 0x00, 0xc7};
    if ((type == PACKET_TYPE_SLE_EVENT) && packet->size == DLI_RESET_EVENT_SIZE &&
        !memcmp(resetEvent, packet->data, DLI_RESET_EVENT_SIZE)) {
        DLI_LOGE("chipset is reset, execute kill().");
        DLI_DftReportKill(DLI_CHIP_KILL);
        kill(getpid(), SIGKILL);
        return;
    }

    SDF_Buff_S *buf = DLI_CreateRxBuff(packet);
    DLI_CHECK_RETURN(buf, "DLI_CreateRxBuff error");
    switch (type) {
        case DLI_DATATYPE_ACB:
            // 直接切主线程，处理acb数据
            ret = DLI_PostOtherThread(DLI_AcbPacketReceived, (void*)buf, NULL);
            break;
        case DLI_DATATYPE_ICB:
            DLI_LOGW("no support packet_type: %d", type);
            break;
        case DLI_DATATYPE_EVENT:
            // 直接切dli线程，处理cmd对应的事件，recvPacket内存由dli线程管理
            ret = DLI_PostTask(DLI_EventPacketReceived, (void*)buf, NULL);
            break;
        default:
            DLI_LOGW("unknown packet_type:%d", type);
            break;
    }
    if (ret != 0) {
        DLI_LOGW("PostTask ret 0x%08x error or type %d error", ret, type);
        SDF_BuffFree(buf);
    }
}

void DLI_LayerDeinit(void)
{
    DLI_LOGI("DLI_LayerDeinit begin");
    DLI_ThreadDeinit();
    DLI_LOGI("DLI_LayerDeinit end");
}

void DLI_LayerDisable(void)
{
    DLI_SapiDeinit();
    DLI_LayerInfoDeInit();
}

#ifdef __cplusplus
}
#endif