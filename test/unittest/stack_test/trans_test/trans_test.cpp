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
#include <gtest/gtest.h>
#include "securec.h"

#include "sdf_buff.h"
#include "dli_callback.h"
#include "dli_layer_callback.h"
#include "dtap.h"
#include "dtap_trans.h"
#include "dtap_errno.h"
#include "cm_errno.h"
#include "cm_logic_link_api.h"
#include "transport.h"
#include "transport_cltp.h"
#include "transport_errno.h"
#include "transport_internal.h"
#include "sle_logic_link_mgr.h"
#include "sdf_mem.h"
#include "sdf_worker.h"
#include "cp_worker.h"
#include "byte_codec.h"
#include "dli_layer.h"
#include "cm_api.h"
#include "dtap.h"
#include "stack_schedule_mock.h"
#include "stack_schedule_stub.h"
#include "stack_cm_mock.h"
#include "stack_cm_stub.h"

static int myTestRecvDataCbk(const TRANS_Addr_S *addr, uint8_t *data, uint16_t len)
{
    (void)addr;
    (void)data;
    (void)len;
    return 0;
}

static void myTestSendDataCbk(const SLE_Addr_S *addr, uint8_t tcid, uint16_t srcPort, uint8_t result)
{
    (void)addr;
    (void)tcid;
    (void)srcPort;
    (void)result;
}

static void myTestChannelStatusChangeCbk(uint16_t lcid, uint16_t tcid, uint8_t result)
{
    (void)lcid;
    (void)tcid;
    (void)result;
}

static void TestDliCallbackInit(void)
{
    DLI_Callback dliCallback = {0};
    dliCallback.postOtherThread = CP_PostTask;
    dliCallback.postOtherBlockedThread = CP_PostTaskBlocked;
    dliCallback.dftReportKill = NULL;
    dliCallback.recvAcbHandler = DTAP_DataRecv;
    DLI_SetCallback(&dliCallback);
    return;
}

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

class UT_TRANS_TEST : public testing::Test {
public:
    NiceMock<ScheduleMock> scheduleMock;
    NiceMock<CmMock> cmMock;
protected:
    // SetUP 在每一个 TEST_F 测试开始前执行一次
    virtual void SetUp()
    {
        TEST_ScheduleInit();
        EXPECT_CALL(scheduleMock, SchedulePostTask).WillRepeatedly(TEST_SchedulePostTaskStub);
        EXPECT_CALL(scheduleMock, SchedulePostTaskBlocked).WillRepeatedly(TEST_SchedulePostTaskBlockedStub);
        EXPECT_CALL(scheduleMock, ScheduleTimerAdd).WillRepeatedly(TEST_ScheduleTimerAddStub);
        EXPECT_CALL(scheduleMock, ScheduleTimerDel).WillRepeatedly(TEST_ScheduleTimerDelStub);
        TestDliCallbackInit();
        DLI_LayerInit();
        DLI_LayerEnable();
        TEST_CM_Init();
        EXPECT_CALL(cmMock, CM_RegTransChannelListener).WillRepeatedly(TEST_CM_RegTransChannelListener);
        EXPECT_CALL(cmMock, CM_UnRegTransChannelListener).WillRepeatedly(TEST_CM_UnRegTransChannelListener);
        EXPECT_CALL(cmMock, CM_GetLogicLinkConnectedSize).WillRepeatedly(TEST_CM_GetLogicLinkConnectedSize);
        DTAP_Init();
        TRANS_Init();
    }

    // TearDown 在每一个 TEST_F 测试完成后执行一次
    virtual void TearDown()
    {
        TRANS_DeInit();
        DTAP_DeInit();
        TEST_CM_DeInit();
        DLI_LayerDisable();
        DLI_LayerDeinit();
        TEST_StackScheduleDeInit();
    }

    // SetUpTestCase 在所有 TEST_F 测试开始前执行一次
    static void SetUpTestCase()
    {
    }

    // TearDownTestCase 在所有 TEST_F 测试完成后执行一次
    static void TearDownTestCase()
    {
    }
};

static CM_TransChan_S *CM_TransChanCreate(uint8_t castMode, uint16_t lcid, uint8_t srcTcid,
    uint8_t dstTcid, uint8_t mode)
{
    CM_TransChan_S *channel = (CM_TransChan_S *)SDF_MemZalloc(sizeof(CM_TransChan_S));
    if (channel == NULL) {
        return NULL;
    }
    channel->castMode = castMode;
    channel->lcid = lcid;
    channel->srcTcid = srcTcid;
    channel->dstTcid = dstTcid;
    channel->config.transMode = mode;
    channel->config.mtu = UINT16_MAX;
    channel->config.mps = UINT16_MAX;

    return channel;
}

TEST_F(UT_TRANS_TEST, TRANS_CltpPktProc_Buff_Test)
{
    SDF_Buff_S *buff = SDF_BuffNewWithReserve(100);
    EXPECT_NE(buff, nullptr);
    EXPECT_EQ(TRANS_CltpPktProc(buff), TRANS_FAIL);
    EXPECT_NE(SDF_BuffAppend(buff, sizeof(TRANS_CltpHeader_S)), nullptr);
    uint64_t dataLen = SDF_DataLenGet(buff);
    int32_t ret = TRANS_CltpPktProc(buff);
    EXPECT_EQ(ret, TRANS_SUCCESS);
    EXPECT_NE(SDF_BuffAppend(buff, sizeof(TRANS_CltpHeader_S)), nullptr);
    TRANS_CltpHeader_S *cltpHeader = (TRANS_CltpHeader_S *)SDF_DataOffset(buff);
    cltpHeader->basic.optionLen = 1;
    EXPECT_EQ(TRANS_CltpPktProc(buff), TRANS_FAIL);
    EXPECT_NE(SDF_BuffAppend(buff, sizeof(TRANS_CltpHeaderOpts_S)), nullptr);
    EXPECT_EQ(TRANS_CltpPktProc(buff), TRANS_FAIL);
    TRANS_CltpHeaderOpts_S *defaultOpts = (TRANS_CltpHeaderOpts_S *)cltpHeader->option;
    ENCODE2BYTE_BIG((uint8_t *)&(defaultOpts->optionBitmap), TRANS_CLTP_OPT_PAYLOAD_MASK);
    ENCODE2BYTE_BIG((uint8_t *)&(defaultOpts->payloadLen), 1);
    EXPECT_EQ(TRANS_CltpPktProc(buff), TRANS_FAIL);
    ENCODE2BYTE_BIG((uint8_t *)&(defaultOpts->payloadLen), 0);
    EXPECT_EQ(TRANS_CltpPktProc(buff), TRANS_SUCCESS);
    SDF_BuffFree(buff);
}

TEST_F(UT_TRANS_TEST, TRANS_CltpPktProc_Buff_Success_Twice)
{
    int dataLen = 1;
    SDF_Buff_S *b = (SDF_Buff_S *)malloc(sizeof(SDF_Buff_S) + sizeof(uint8_t));
    b->dataOff = 0;
    b->dataLen = dataLen;
    b->buffLen = 1;
    b->buff[0] = 0x01;
    int32_t ret = TRANS_CltpPktProc(b);
    EXPECT_EQ(ret, TRANS_FAIL);

    int buffLen = 1e6;
    SDF_Buff_S *b1 = (SDF_Buff_S *)malloc(sizeof(SDF_Buff_S) + sizeof(uint8_t) * buffLen);
    b1->dataOff = 0;
    b1->dataLen = 1e1;
    b1->buffLen = 1;
    for (int i = 0; i < 1e1; i++) {
        b1->buff[1] = 0x01;
    }
    ret = TRANS_CltpPktProc(b1);
    EXPECT_EQ(ret, TRANS_SUCCESS);
    free(b);
    free(b1);
}

TEST_F(UT_TRANS_TEST, TRANS_CltpHeaderBuild_Fail)
{
    SDF_Buff_S *S = SDF_BuffNew(10);
    uint16_t srcPort = 1;
    uint16_t dstPort = 0;
    uint32_t ret = TRANS_CltpHeaderBuild(srcPort, dstPort, S);
    EXPECT_EQ(ret, TRANS_SEND_DATA_APPEND_FAILED);
    SDF_BuffFree(S);
}

TEST_F(UT_TRANS_TEST, TRANS_CltpHeaderBuild_Success)
{
    SDF_Buff_S *S = SDF_BuffNew(1e7);
    S->dataOff = 1e6;
    uint16_t srcPort = 1;
    uint16_t dstPort = 0;
    uint32_t ret = TRANS_CltpHeaderBuild(srcPort, dstPort, S);
    EXPECT_EQ(ret, TRANS_SUCCESS);
    SDF_BuffFree(S);
}

TEST_F(UT_TRANS_TEST, TRANS_RegisterCbks_Fail)
{
    uint32_t ret = TRANS_RegisterCbks(NULL);
    EXPECT_EQ(ret, TRANS_REGISTER_CBKS_NULL_PTR);
    TRANS_UnregisterCbks();
}

TEST_F(UT_TRANS_TEST, TRANS_RegisterCbks_Success)
{
    TRANS_Cbks_S cbks = {NULL};
    cbks.recvDataCbk = [](const TRANS_Addr_S *addr, uint8_t *data, uint16_t len) -> int { return 0; };
    cbks.sendDataCbk = [](const SLE_Addr_S *addr, uint8_t tcid, uint16_t srcPort, uint8_t result) -> void { return; };

    uint32_t ret = TRANS_RegisterCbks(&cbks);
    EXPECT_EQ(ret, TRANS_SUCCESS);
    TRANS_UnregisterCbks();
}

TEST_F(UT_TRANS_TEST, TRANS_SendData_Null)
{
    uint32_t ret = TRANS_SendData(NULL, NULL, 0);
    EXPECT_EQ(ret, TRANS_SEND_DATA_INVALID_PARAMS);
    TRANS_Addr_S addr;
    ret = TRANS_SendData(&addr, NULL, 0);
    EXPECT_EQ(ret, TRANS_SEND_DATA_INVALID_PARAMS);
    uint8_t data[] = {0};
    ret = TRANS_SendData(&addr, data, 0);
    EXPECT_EQ(ret, TRANS_SEND_DATA_INVALID_PARAMS);
}

TEST_F(UT_TRANS_TEST, TRANS_SendData_InvalidDataLen)
{
    TRANS_Addr_S addr;
    (void)memset_s(&addr, sizeof(addr), 0, sizeof(addr));
    addr.proto = TRANS_PROTO_CONNECTIONLESS;
    uint8_t data[] = {0};
    uint32_t ret = TRANS_SendData(&addr, data, DTAP_MAX_PAYLOAD_LEN);
    EXPECT_EQ(ret, TRANS_SEND_DATA_INVALID_PARAMS);
}

TEST_F(UT_TRANS_TEST, TRANS_SendData_Fail)
{
    SLE_Addr_S *sleAddr = (SLE_Addr_S *)malloc(sizeof(SLE_Addr_S));
    memset_s(sleAddr, sizeof(SLE_Addr_S), 0x00, sizeof(SLE_Addr_S));
    TRANS_Addr_S addr = {
        .devAddr = *sleAddr,
        .tcid = 1,
        .proto = TRANS_PROTO_CONNECTIONLESS,
        .srcPort = 2,
        .dstPort = 3,
    };
    uint8_t data[10] = {0};
    int dataLen = 10;
    uint32_t ret = TRANS_SendData(&addr, data, dataLen);
    EXPECT_NE(ret, TRANS_SUCCESS);
    addr.proto = TRANS_PROTO_CONNECTION;
    ret = TRANS_SendData(&addr, data, dataLen);
    EXPECT_NE(ret, TRANS_SUCCESS);
    addr.proto = (TRANS_Protocol_E)UINT8_MAX;
    ret = TRANS_SendData(&addr, data, dataLen);
    EXPECT_NE(ret, TRANS_SUCCESS);
    addr.proto = TRANS_PROTO_CONNECTIONLESS;
    ret = TRANS_SendData(&addr, data, dataLen);
    EXPECT_NE(ret, TRANS_SUCCESS);
    free(sleAddr);
}

TEST_F(UT_TRANS_TEST, TRANS_RecvData)
{
    SDF_Buff_S *buff = SDF_BuffNew(sizeof(DTAP_BasicFrameHeader_S) +
        sizeof(uint8_t) + sizeof(TRANS_CltpHeader_S) + sizeof(uint8_t));
    EXPECT_NE(buff, nullptr);
    DTAP_BasicFrameHeader_S *dtapHdr = (DTAP_BasicFrameHeader_S *)SDF_BuffAppend(buff, sizeof(DTAP_BasicFrameHeader_S)
        + sizeof(uint8_t));
    EXPECT_NE(dtapHdr, nullptr);
    dtapHdr->header.tcid = CM_TCID_SLE_CUTC;
    dtapHdr->header.frameType = DTAP_FRAME_BASIC;
    ENCODE2BYTE_LITTLE(&dtapHdr->header.length, sizeof(uint8_t));

    CM_TransChannelStateList_S stateLists;
    SDF_Traits transChannelTraits = {.dtor = SDF_MemFree};
    stateLists.channelVector = SDF_CreateVector(transChannelTraits);
    EXPECT_NE(stateLists.channelVector, nullptr);

    CM_TransChan_S *chan = CM_TransChanCreate(0, 0, CM_TCID_SLE_CUTC, CM_TCID_SLE_CUTC, CM_TRANS_MODE_BASIC);
    EXPECT_NE(chan, nullptr);
    bool isSuccess = SDF_VectorEmplaceBack(stateLists.channelVector, chan);
    EXPECT_EQ(isSuccess, true);

    stateLists.result = CM_TRANS_CHANNEL_STATE_ACTIVATED;
    TEST_CM_TransChannelDo(&stateLists);

    DLI_AcbRecvHander(0, buff);

    dtapHdr = (DTAP_BasicFrameHeader_S *)SDF_BuffPrepend(buff, sizeof(DTAP_BasicFrameHeader_S) + sizeof(uint8_t));
    EXPECT_NE(dtapHdr, nullptr);
    EXPECT_NE(SDF_BuffAppend(buff, 1), nullptr);
    ENCODE2BYTE_LITTLE(&dtapHdr->header.length, sizeof(uint8_t) + sizeof(uint8_t));
    DLI_AcbRecvHander(0, buff);

    dtapHdr = (DTAP_BasicFrameHeader_S *)SDF_BuffPrepend(buff, sizeof(DTAP_BasicFrameHeader_S) + sizeof(uint8_t));
    EXPECT_NE(dtapHdr, nullptr);
    *(uint8_t *)(dtapHdr->pi) = DTAP_PI_LWCLTP;
    DLI_AcbRecvHander(0, buff);

    dtapHdr = (DTAP_BasicFrameHeader_S *)SDF_BuffPrepend(buff, sizeof(DTAP_BasicFrameHeader_S) + sizeof(uint8_t));
    EXPECT_NE(dtapHdr, nullptr);
    EXPECT_NE(SDF_BuffAppend(buff, sizeof(TRANS_CltpHeader_S)), nullptr);
    ENCODE2BYTE_LITTLE(&dtapHdr->header.length, sizeof(uint8_t) + sizeof(uint8_t) + sizeof(TRANS_CltpHeader_S));
    DLI_AcbRecvHander(0, buff);

    EXPECT_NE(SDF_BuffPrepend(buff, sizeof(DTAP_BasicFrameHeader_S) + sizeof(uint8_t)), nullptr);
    EXPECT_NE(dtapHdr, nullptr);
    TRANS_CltpHeader_S *cltpHdr = (TRANS_CltpHeader_S *)(SDF_DataOffset(buff) + sizeof(DTAP_BasicFrameHeader_S)
        + sizeof(uint8_t));
    cltpHdr->basic.version = TRANS_PROTO_VERSION;
    cltpHdr->basic.optionLen = 1;
    DLI_AcbRecvHander(0, buff);

    EXPECT_NE(SDF_BuffPrepend(buff, sizeof(DTAP_BasicFrameHeader_S) + sizeof(uint8_t)), nullptr);
    EXPECT_NE(dtapHdr, nullptr);
    cltpHdr = (TRANS_CltpHeader_S *)(SDF_DataOffset(buff) + sizeof(DTAP_BasicFrameHeader_S) + sizeof(uint8_t));
    cltpHdr->basic.optionLen = 0;
    DLI_AcbRecvHander(0, buff);

    SDF_BuffFree(buff);
    stateLists.result = CM_TRANS_CHANNEL_STATE_RELEASED;
    TEST_CM_TransChannelDo(&stateLists);
}

TEST_F(UT_TRANS_TEST, TRANS_InitTest)
{
    EXPECT_NE(TRANS_Init(), TRANS_SUCCESS);
    TRANS_DeInit();
    DTAP_DeInit();
    DTAP_DeInit();
    EXPECT_NE(TRANS_Init(), TRANS_SUCCESS);
    DTAP_Init();
    EXPECT_EQ(TRANS_Init(), TRANS_SUCCESS);
    TRANS_DeInit();
}

TEST_F(UT_TRANS_TEST, TRANS_ChannelStatusChange)
{
    TRANS_Cbks_S cbks;
    SLE_Addr_S addr;
    (void)memset_s(&addr, sizeof(addr), 0, sizeof(addr));
    cbks.recvDataCbk = myTestRecvDataCbk;
    cbks.sendDataCbk = myTestSendDataCbk;
    uint32_t ret = TRANS_RegisterCbks(&cbks);
    EXPECT_EQ(ret, TRANS_SUCCESS);

    DTAP_Data_Send_Cbks_S sendCbk;
    sendCbk.transChannelStatusChangeCbk = myTestChannelStatusChangeCbk;
    DTAP_RegisterDataSendCbks(&sendCbk);

    DTAP_Channel_S *ch = (DTAP_Channel_S *)malloc(sizeof(DTAP_Channel_S));
    memset_s(ch, sizeof(DTAP_Channel_S), 0x00, sizeof(DTAP_Channel_S));
    DTAP_ReliableChannel_S *reliable = (DTAP_ReliableChannel_S *)malloc(sizeof(DTAP_ReliableChannel_S));
    memset_s(reliable, sizeof(DTAP_ReliableChannel_S), 0x00, sizeof(DTAP_ReliableChannel_S));
    reliable->isTxWindowFull = false;
    reliable->txWindow.txList.size = 1;
    reliable->txWindow.size = 1e2;
    ch->attr = reliable;

    // ch->mode no match
    DTAP_TransChannelStatusChange(ch, 0x00);
    ch->mode = CM_TRANS_MODE_RELIABLE;
    // reliable->isTxWindowFull no match
    DTAP_TransChannelStatusChange(ch, 0x00);
    reliable->isTxWindowFull = true;
    DTAP_TransChannelStatusChange(ch, 0x00);
    EXPECT_EQ(reliable->isTxWindowFull, false);
    TRANS_ChannelSetStatus(&addr, 0x00, 0x00);
    TRANS_UnregisterCbks();
    DTAP_UnRegisterDataSendCbks();
    free(ch);
    free(reliable);
}