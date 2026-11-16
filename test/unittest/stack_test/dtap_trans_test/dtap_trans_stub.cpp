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

#include "dtap_trans_test.h"

#include <cstring>

static CM_DliCbk g_connectCbk = nullptr;
static CM_DliCbk g_disconnectCbk = nullptr;
static COLLAB_TransFuncExt g_collabTransFunc = {};
static DLI_DataNumChangecbk g_acbNumChangeCbk = nullptr;
static uint64_t g_monoTimeMs = 0;

void TEST_DTAP_SetMonoTimeMs(uint64_t ms)
{
    g_monoTimeMs = ms;
}

void TEST_DTAP_CollabReset(void)
{
    g_collabTransFunc = {};
    g_acbNumChangeCbk = nullptr;
}

void TEST_DTAP_SetApBufferNum(uint8_t num)
{
    if (g_collabTransFunc.setApBufferNum != nullptr) {
        g_collabTransFunc.setApBufferNum(num);
    }
}

void TEST_DTAP_AcbNumChange(uint16_t type, uint16_t num)
{
    if (g_acbNumChangeCbk != nullptr) {
        g_acbNumChangeCbk(type, num);
    }
}

static void TEST_DTAP_DliCmdRet(DLI_ExecuteCmdRetParam &cmdRes, uint16_t opcode, void *evt, uint16_t size)
{
    cmdRes.cmdOpcode = opcode;
    cmdRes.size = size;
    cmdRes.eventParameter = evt;
}

void TEST_DTAP_DliConnectDo(uint16_t connHandle, uint16_t status)
{
    if (g_connectCbk == nullptr) {
        return;
    }
    DLI_ConnectionCompleteEvt evt = {0};
    evt.status = status;
    evt.connHandle = connHandle;
    DLI_ExecuteCmdRetParam cmdRes = {};
    TEST_DTAP_DliCmdRet(cmdRes, DLI_CREATE_CONNECTION, &evt, sizeof(evt));
    g_connectCbk(nullptr, status, &cmdRes);
}

void TEST_DTAP_DliDisconnectDo(uint16_t connHandle)
{
    if (g_disconnectCbk == nullptr) {
        return;
    }
    DLI_DisconnectEvt evt = {0};
    evt.status = 0;
    evt.connHandle = connHandle;
    DLI_ExecuteCmdRetParam cmdRes = {};
    TEST_DTAP_DliCmdRet(cmdRes, DLI_DISCONNECT, &evt, sizeof(evt));
    g_disconnectCbk(nullptr, 0, &cmdRes);
}

static uint32_t g_preAssignCnt = 0;
static uint32_t g_continueAssignCnt = 0;

void TEST_DTAP_CollabCntReset(void)
{
    g_preAssignCnt = 0;
    g_continueAssignCnt = 0;
}

uint32_t TEST_DTAP_PreAssignCnt(void)
{
    return g_preAssignCnt;
}

uint32_t TEST_DTAP_ContinueAssignCnt(void)
{
    return g_continueAssignCnt;
}

static uint8_t g_statusResult = 0;
static uint32_t g_statusCnt = 0;

void TEST_DTAP_StatusCbkReset(void)
{
    g_statusCnt = 0;
}

uint32_t TEST_DTAP_StatusCnt(void)
{
    return g_statusCnt;
}

uint8_t TEST_DTAP_LastStatusResult(void)
{
    return g_statusResult;
}

static void TEST_DTAP_TransChannelStatusChangeCbk(uint16_t lcid, uint16_t tcid, uint8_t result)
{
    g_statusCnt++;
    (void)lcid;
    (void)tcid;
    g_statusResult = result;
}

void TEST_DTAP_RegisterStatusCbk(void)
{
    DTAP_Data_Send_Cbks_S cbks = {};
    cbks.transChannelStatusChangeCbk = TEST_DTAP_TransChannelStatusChangeCbk;
    DTAP_RegisterDataSendCbks(&cbks);
}

static std::vector<std::vector<uint8_t>> g_sentFrames;

void TEST_DTAP_SentFramesReset(void)
{
    g_sentFrames.clear();
}

uint32_t TEST_DTAP_DataSendCapture(DLI_DataStru *data)
{
    if (data == nullptr || data->buf == nullptr) {
        return 1;
    }
    uint8_t *head = SDF_DataOffset(data->buf);
    uint64_t len = SDF_DataLenGet(data->buf);
    g_sentFrames.emplace_back(head, head + len);
    SDF_BuffFree(data->buf);
    SDF_MemFree(data);
    return DLI_SUCCESS;
}

uint8_t TEST_DTAP_SentFrameType(size_t idx)
{
    if (idx >= g_sentFrames.size() || g_sentFrames[idx].size() < 2) {
        return 0xFF;
    }
    return g_sentFrames[idx][1] & 0x0F;
}

uint8_t TEST_DTAP_SentFrameByte(size_t idx, size_t off)
{
    if (idx >= g_sentFrames.size() || off >= g_sentFrames[idx].size()) {
        return 0xFF;
    }
    return g_sentFrames[idx][off];
}

size_t TEST_DTAP_SentFrameCnt(void)
{
    return g_sentFrames.size();
}

static uint32_t g_recvCbCnt = 0;
static int g_recvCbRet = 0;

void TEST_DTAP_RecvCbReset(int ret)
{
    g_recvCbCnt = 0;
    g_recvCbRet = ret;
}

uint32_t TEST_DTAP_RecvCbCnt(void)
{
    return g_recvCbCnt;
}

int TEST_DTAP_RecvCb(DTAP_Data_Info_S *info, SDF_Buff_S *buff)
{
    (void)buff;
    (void)info;
    g_recvCbCnt++;
    return g_recvCbRet;
}

extern "C" uint32_t CM_RegisterDliAdapterCbk(CM_DLI_ADAPTER_MODULE module, CM_DLI_ADAPTER_TYPE type, CM_DliCbk cbk)
{
    if (module != CM_DLI_ADAPTER_DTAP) {
        return CM_SUCCESS;
    }
    if (type == CM_DLI_ADAPTER_CONNECT) {
        g_connectCbk = cbk;
    } else if (type == CM_DLI_ADAPTER_DISCONNECT) {
        g_disconnectCbk = cbk;
    }
    return CM_SUCCESS;
}

extern "C" uint32_t CM_UnregisterDliAdapterCbk(CM_DLI_ADAPTER_MODULE module, CM_DLI_ADAPTER_TYPE type)
{
    (void)module;
    (void)type;
    return CM_SUCCESS;
}

extern "C" uint32_t COLLAB_TransFuncRegister(const COLLAB_TransFuncExt *func)
{
    g_collabTransFunc = *func;
    return 0;
}

extern "C" uint32_t COLLAB_PreAssignTransBuffer(uint8_t apOccupiedBufferNum)
{
    (void)apOccupiedBufferNum;
    g_preAssignCnt++;
    return 0;
}

extern "C" uint32_t COLLAB_ContinueAssignTransBuffer(uint8_t apOccupiedBufferNum)
{
    (void)apOccupiedBufferNum;
    g_continueAssignCnt++;
    return 0;
}

extern "C" void DLI_DataNumChangeRegister(DLI_DataNumChangecbk cbk)
{
    g_acbNumChangeCbk = cbk;
}

extern "C" void DLI_DataNumChange(DLI_DataType type, uint16_t dataNum)
{
    (void)type;
    (void)dataNum;
}

extern "C" uint64_t DP_GetMonoTimeMs(void)
{
    return g_monoTimeMs;
}

extern "C" uint64_t DP_GetRealTimeMs(void)
{
    return g_monoTimeMs;
}
