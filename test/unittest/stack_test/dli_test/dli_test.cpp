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

#include "dpfwk_errcode.h"
#include "dpfwk_log.h"
#include "sdf_evc.h"
#include "sdf_timer.h"
#include "sdf_event.h"
#include "sdf_worker.h"
#include "sdf_thread.h"

#include "dli_thread.h"
#include "dli_log.h"
#include "dli_cmd_struct.h"
#include "dli_event_struct.h"
#include "dli_opcode.h"
#include "dli_errno.h"
#include "dli_def.h"
#include "dli_layer.h"
#include "dli.h"
#include "dli_cmd.h"
#include "dli_data_stub.h"
#include "dli_event.h"
#include "dli_dev_discovery_event.h"
#include "dli_connect_event.h"
#include "dli_layer_callback.h"
#include "nlstk_schedule.h"

#define DLI_TEST_NUM 10
#define DLI_DATA_OFFSET 4
#define DATA_INDEX_0 0
#define DATA_INDEX_1 1
#define DATA_INDEX_2 2
#define DATA_INDEX_3 3

#include "stack_schedule_mock.h"
#include "stack_schedule_stub.h"
#include "stack_dli_stub.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;
static uint8_t g_cmdResData = 0;
static uint32_t g_cmdResLen = 0;
#define TEST_IQ_MAX_CHNL_NUM 79

class DliTest : public testing::Test {
public:
    NiceMock<ScheduleMock> scheduleMock;
protected:
    // SetUP 在每一个 TEST_F 测试开始前执行一次
    virtual void SetUp()
    {
        TEST_ScheduleInit();
        EXPECT_CALL(scheduleMock, SchedulePostTask).WillRepeatedly(TEST_SchedulePostTaskStub);
        EXPECT_CALL(scheduleMock, SchedulePostTaskBlocked).WillRepeatedly(TEST_SchedulePostTaskBlockedStub);
        TEST_DLI_Init();
    }

    // TearDown 在每一个 TEST_F 测试完成后执行一次
    virtual void TearDown()
    {
        g_cmdResData = 0;
        g_cmdResLen = 0;
        TEST_StackScheduleDeInit();
    }

    // SetUpTestCase 在所有 TEST_F 测试开始前执行一次
    static void SetUpTestCase()
    {

        uint32_t ret = SDF_ThreadInit(DLI_TEST_NUM);
        EXPECT_EQ(ret, 0);
        ret = SDF_EvcInit();
        EXPECT_EQ(ret, 0);
        DLI_LOGI("UT_DLITHREADTEST SetUpTestCase end");
    }

    // TearDownTestCase 在所有 TEST_F 测试完成后执行一次
    static void TearDownTestCase()
    {
        SDF_EvcDeinit();
        SDF_ThreadDeinit();
    }
};

void ReadAdvDataLenCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    (void)context;
    (void)status;
    DLI_LOGI("ReadAdvDataLenCbk enter");
    g_cmdResData = *((uint8_t *)cmdRes->eventParameter);
    g_cmdResLen = cmdRes->size;
}

void MockEvtCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    (void)context;
    (void)status;
    (void)cmdRes;
    DLI_LOGI("MockEvtCbk enter");
    g_cmdResData = *((uint8_t *)cmdRes->eventParameter);
    g_cmdResLen = cmdRes->size;
}

static const DLI_CbkLineStru hadmTable[] = {
    {DLI_CBK_READ_LOCAL_MEASURE_CAPS, MockEvtCbk},
    {DLI_CBK_READ_REMOTE_MEASURE_CAPS, MockEvtCbk},
    {DLI_CBK_SET_MEASURE_CONFIG_PARAM, MockEvtCbk},
    {DLI_CBK_SET_MEASURE_EN, MockEvtCbk},
    {DLI_CBK_MEASURE_IQ_REPORT, MockEvtCbk},
};

static const DLI_CbkLineStru devdTable[] = {
    {DLI_CBK_READ_ADV_DATA_LEN, ReadAdvDataLenCbk},
    {DLI_CBK_SET_ADV_PARAMS, MockEvtCbk},
    {DLI_CBK_SET_ADV_DATA, MockEvtCbk},
    {DLI_CBK_ENABLE_ADV, MockEvtCbk},
    {DLI_CBK_READ_ADV_SETS_NUM, MockEvtCbk},
    {DLI_CBK_REMOVE_ADV, MockEvtCbk},
    {DLI_CBK_ADV_TERMINATED, MockEvtCbk},
    {DLI_CBK_SET_SCAN_PARAMS, MockEvtCbk},
    {DLI_CBK_ENABLE_SCAN, MockEvtCbk},
    {DLI_CBK_ADV_REPORT, MockEvtCbk},
    {DLI_CBK_SET_SCAN_FILTER, MockEvtCbk},
};

static const DLI_CbkLineStru cmTable[] = {
    {DLI_CBK_READ_ACCESS_FLT_LIST_SIZE, MockEvtCbk},
    {DLI_CBK_READ_LOCAL_FEATURE, MockEvtCbk},
    {DLI_CBK_CONNECT, MockEvtCbk},
    {DLI_CBK_DISCONNECT, MockEvtCbk},
    {DLI_CBK_CLEAR_ACCESS_FLT_LIST, MockEvtCbk},
    {DLI_CBK_ADD_DEV_TO_ACCESS_FLT_LIST, MockEvtCbk},
    {DLI_CBK_RMV_DEV_FROM_ACCESS_FLT_LIST, MockEvtCbk},
    {DLI_CBK_CONNECT_CANCEL, MockEvtCbk},
    {DLI_CBK_REMOTE_CONNECT_PARAM_REQ, MockEvtCbk},
    {DLI_CBK_CONNECT_UPDATE, MockEvtCbk},
    {DLI_CBK_READ_REMOTE_FEATURE, MockEvtCbk},
    {DLI_CBK_READ_REMOTE_VERSION, MockEvtCbk},
    {DLI_CBK_SET_DATA_LEN, MockEvtCbk},
    {DLI_CBK_SET_HOST_CHANNEL_CLASSIFICATION, MockEvtCbk},
    {DLI_CBK_REMOTE_CONNECT_PARAM_REQ_REPLY, MockEvtCbk},
    {DLI_CBK_SET_RX_DATA_FILTER, MockEvtCbk},
    {DLI_CBK_SET_PHY, MockEvtCbk},
    {DLI_CBK_SET_MCS, MockEvtCbk},
    {DLI_CBK_DATA_LEN_CHANGE, MockEvtCbk},
    {DLI_CBK_SET_ACB_EVT_PARAM, MockEvtCbk},
    {DLI_CBK_ACB_LOW_LATENCY_EN, MockEvtCbk},
    {DLI_CBK_READ_REMOTE_RSSI, MockEvtCbk},
    {DLI_CBK_READ_REMOTE_PRIVATE_FEATURE, MockEvtCbk},
    {DLI_CBK_SET_POWER_MODE, MockEvtCbk},
    {DLI_CBK_QUERY_LINK_QUALITY, MockEvtCbk},
    {DLI_CBK_ENABLE_CONN_HIGH_POWER, MockEvtCbk},
    {DLI_CBK_SET_LOCAL_PRIVATE_FEATURE, MockEvtCbk},
    {DLI_CBK_SET_CONN_FRAME_POWER_LEVEL, MockEvtCbk},
    {DLI_CBK_SET_5G_FREQ, MockEvtCbk},
    {DLI_CBK_SET_PEER_DEV_TYPE, MockEvtCbk},
    {DLI_CBK_ENABLE_FREQ_BAND, MockEvtCbk},
    {DLI_CBK_ACB_ENABLE_SUBRATE, MockEvtCbk},
    {DLI_CBK_SET_5G_TEST_MODE, MockEvtCbk},
    {DLI_CBK_ACB_SET_SUBRATE, MockEvtCbk},
    {DLI_CBK_ACB_REQ_SUBRATE, MockEvtCbk},
    {DLI_CBK_RSSI_CHANGE, MockEvtCbk},
    {DLI_CBK_POWER_LEVEL_CHANGE, MockEvtCbk},
    {DLI_CBK_GET_MAC_ID, MockEvtCbk},
    {DLI_CBK_REMOVE_ICG_PARAM, MockEvtCbk},
    {DLI_CBK_SET_MAC_ID, MockEvtCbk},
};

static const DLI_CbkLineStru smTable[] = {
    {DLI_CBK_SEND_CONTROLLER_DATA, MockEvtCbk},
    {DLI_CBK_RECV_CONTROLLER_DATA, MockEvtCbk},
    {DLI_CBK_READ_SUPPORT_CRYPTO_ALGO, MockEvtCbk},
    {DLI_CBK_ENCRYPT, MockEvtCbk},
    {DLI_CBK_ENCRYPT_CHANGE, MockEvtCbk},
    {DLI_CBK_ENCRYPT_PARAM_REQ, MockEvtCbk},
    {DLI_CBK_ENCRYPT_PARAM_REQ_REPLY, MockEvtCbk},
    {DLI_CBK_ENCRYPT_PARAM_REQ_NEG_REPLY, MockEvtCbk},
    {DLI_CBK_SET_TX_POWER, MockEvtCbk},
    {DLI_CBK_ENCRYPT_CHANGE, MockEvtCbk},
};

static const DLI_CbkLineStru nbcTable[] = {
    {DLI_CBK_GET_PUBLIC_ADDRESS, MockEvtCbk},
    {DLI_CBK_READ_LOCAL_VERSION, MockEvtCbk},
    {DLI_CBK_READ_LOCAL_FEATURE, MockEvtCbk},
    {DLI_CBK_RSSI_CHANGE, MockEvtCbk},
    {DLI_CBK_CHIP_RESET_NOTIFY, MockEvtCbk},
    {DLI_CBK_SET_PUBLIC_ADDRESS, MockEvtCbk},
    {DLI_CBK_REMOVE_PEER_DEV_TYPE, MockEvtCbk},
};

static const DLI_CbkLineStru icbTable[] = {
    {DLI_CBK_IMG_LABEL_REPORT, MockEvtCbk},
    {DLI_CBK_IMB_ESTABLISHED, MockEvtCbk},
    {DLI_CBK_SET_IMG_PARAM, MockEvtCbk},
    {DLI_CBK_SET_IOG_PARAM, MockEvtCbk},
};

static uint32_t devdSize = sizeof(devdTable) / sizeof(DLI_CbkLineStru);
static uint32_t cmSize = sizeof(cmTable) / sizeof(DLI_CbkLineStru);
static uint32_t hadmSize = sizeof(hadmTable) / sizeof(DLI_CbkLineStru);
static uint32_t smSize = sizeof(smTable) / sizeof(DLI_CbkLineStru);
static uint32_t nbcSize = sizeof(nbcTable) / sizeof(DLI_CbkLineStru);
static uint32_t icbSize = sizeof(icbTable) / sizeof(DLI_CbkLineStru);

static void DLI_ExecuteCommandCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    DLI_LOGI("------execute DLI_CMD_COMPLETE_EVT cbk enter, evtOpcode: 0x%04X, len=%u------", evtOpcode, len);

    for (int i = 0; i < len; i++) {
        DLI_LOGI("data: 0x%04X", *((uint8_t *)arg + i));
    }

    uint16_t status = DLI_SUCCESS;
    DLI_ExecuteCmdRetParam par;
    par.cmdOpcode = evtOpcode;
    if (len > DLI_DATA_OFFSET && (uint8_t *)arg + DLI_DATA_OFFSET != NULL) {
        par.size = len - DLI_DATA_OFFSET;
        par.eventParameter = (uint8_t *)arg + DLI_DATA_OFFSET;
    }
    DLI_ManagerContext *managerContext = (DLI_ManagerContext *)context;
    ReadAdvDataLenCbk(managerContext->cbkContext, status, &par);
}

void TEST_DliWriteLogStub(uint16_t type, const uint8_t *data, uint32_t len, int result)
{}

static uint32_t GetU32Data(const uint8_t *data)
{
    return (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | (data[3]);
}

HWTEST_F(DliTest, TestCaseInit, TestSize.Level1)
{
    uint32_t ret = DLI_Init();
    ASSERT_EQ(0, ret);
    ret = DLI_Enable();
    ASSERT_EQ(0, ret);
    DLI_Disable();
    ret = DLI_CmdCbkReg(DEVD, nullptr, 0, devdTable, devdSize);
    ASSERT_EQ(0, ret);
    ret = DLI_CmdCbkReg(CM, nullptr, 0, cmTable, cmSize);
    ASSERT_EQ(0, ret);
    ret = DLI_CmdCbkReg(HADM, nullptr, 0, hadmTable, hadmSize);
    ASSERT_EQ(0, ret);
    ret = DLI_CmdCbkReg(SM, nullptr, 0, smTable, smSize);
    ASSERT_EQ(0, ret);
    ret = DLI_CmdCbkReg(NBC, nullptr, 0, nbcTable, nbcSize);
    ASSERT_EQ(0, ret);
    ret = DLI_CmdCbkReg(CM_ICB, nullptr, 0, icbTable, icbSize);
    ASSERT_EQ(0, ret);
}

HWTEST_F(DliTest, TestCaseSendCommonCmd, TestSize.Level1)
{
    DLI_SetWriteFileCallback(TEST_DliWriteLogStub);
    DLI_PublicAddrParam cmd = {.mediaAccessLayerIDType = 0};
    uint32_t ret = DLI_GetPublicAddress(&cmd);
    ASSERT_EQ(0, ret);

    uint8_t channelMap[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    ret = DLI_SetHostChannelClassification(channelMap, 10);
    ASSERT_EQ(0, ret);

    ret = DLI_ReadLocalFeatures();
    ASSERT_EQ(0, ret);

    ret = DLI_ReadBufferSize();
    ASSERT_EQ(0, ret);

    DLI_LocalFeatures_S feature = {0};
    DLI_SetLocalFeatures(&feature);
    ASSERT_EQ(0, ret);

    ret = DLI_ReadLocalVersion();
    ASSERT_EQ(0, ret);

    ret = DLI_ReadCommConfigValue();
    ASSERT_EQ(0, ret);

    // cbk is null
    uint8_t addr[] = {0x01, 0x01, 0x01, 0x01, 0x01, 0x01};
    ret = DLI_SetPublicAddress(addr);
    ASSERT_EQ(0, ret);
}

HWTEST_F(DliTest, TestCaseSendHadmCmd, TestSize.Level1)
{
    uint32_t ret = DLI_ReadLocalMeasureCaps();
    ASSERT_EQ(0, ret);

    DLI_ReadRemoteMeasureCapsParam param1 = {0};
    ret = DLI_ReadRemoteMeasureCaps(&param1);
    ASSERT_EQ(0, ret);

    DLI_SetMeasureConfigParam param2 = {0};
    ret = DLI_SetMeasureParam(&param2);
    ASSERT_EQ(0, ret);

    DLI_SetMeasureEnableParam param3 = {0};
    ret = DLI_SetMeasureEnable(&param3);
    ASSERT_EQ(0, ret);
}

HWTEST_F(DliTest, TestCaseSendSmCmd, TestSize.Level1)
{
    uint32_t ret = DLI_ReadSupportCryptoAlgo();
    ASSERT_EQ(0, ret);

    DLI_ControllerData param1 = {0};
    ret = DLI_SetControllerData(&param1);
    ASSERT_EQ(0, ret);

    DLI_EnableEncryptParam param2 = {0};
    ret = DLI_EnableEncryption(&param2);
    ASSERT_EQ(0, ret);

    DLI_ConnHandleStru param3 = {0};
    ret = DLI_EncryptionParamReqNegativeReply(&param3);
    ASSERT_EQ(0, ret);

    DLI_EncryptReqReplyParam param4 = {0};
    ret = DLI_EncryptionParamReqReply(&param4);
    ASSERT_EQ(0, ret);

    DLI_EncryptParam param5 = {};
    ret = DLI_Encrypt(&param5);
    ASSERT_EQ(0, ret);

    DLI_IMGEncryptParam param6 = {};
    ret = DLI_EnableIMGEncryption(&param6);
    ASSERT_EQ(0, ret);

    ret = DLI_GetAcbDataLen();
    ASSERT_EQ(0, ret);
}

HWTEST_F(DliTest, TestCaseSendCmCmd, TestSize.Level1)
{
    uint32_t ret = DLI_ReadAcceptFilterListSize();
    ASSERT_EQ(0, ret);

    ret = DLI_ClearAcceptFilterList();
    ASSERT_EQ(0, ret);

    SLE_Addr_S param1 = {0};
    uint8_t addr[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    (void)memcpy_s(param1.addr, SLE_ADDR_LEN, addr, SLE_ADDR_LEN);
    ret = DLI_AddDeviceToAcceptFilterList(&param1);
    ASSERT_EQ(0, ret);

    ret = DLI_RemoveDeviceFromAcceptFilterList(&param1);
    ASSERT_EQ(0, ret);

    DLI_ConnectionCreateParam param2 = {0};
    uint8_t version = 0;
    uint16_t localIndex = 0;
    param2.bitFrameType = DLI_SCAN_BIT_FRAME_FORMAT_1_IND;
    ret = DLI_CreateConnection(version, localIndex, &param2);
    ASSERT_EQ(0, ret);

    param2.bitFrameType = DLI_SCAN_BIT_FRAME_FORMAT_4_IND;
    ret = DLI_CreateConnection(version, localIndex, &param2);
    ASSERT_EQ(0, ret);

    param2.bitFrameType = DLI_SCAN_BIT_FRAME_FORMAT_1_IND | DLI_SCAN_BIT_FRAME_FORMAT_4_IND;
    ret = DLI_CreateConnection(version, localIndex, &param2);
    ASSERT_EQ(0, ret);

    ret = DLI_CancelCreateConnection();
    ASSERT_EQ(0, ret);

    DLI_DisconnectParam param3 = {0};
    ret = DLI_Disconnect(version, localIndex, &param3);
    ASSERT_EQ(0, ret);

    DLI_ConnectionUpdateParam param4 = {0};
    ret = DLI_UpdateConnectionParam(version, localIndex, &param4);
    ASSERT_EQ(0, ret);

    DLI_SetPhyParam param5 = {0};
    ret = DLI_SetPhy(&param5);
    ASSERT_EQ(0, ret);

    DLI_SetDataLenParam param7 = {0};
    ret = DLI_SetDataLength(&param7);
    ASSERT_EQ(0, ret);

    DLI_ConnHandleStru param9 = {0};
    ret = DLI_ReadRemoteRssi(&param9);
    ASSERT_EQ(0, ret);

    ret = DLI_ReadRemoteFeatures(&param9);
    ASSERT_EQ(0, ret);

    ret = DLI_ReadRemoteVersion(&param9);
    ASSERT_EQ(0, ret);

    DLI_RemConParamReqReplyParam param10 = {0};
    ret = DLI_RemoteConnectionParamReqReply(&param10);
    ASSERT_EQ(0, ret);

    DLI_SetMcsParam param11 = {0};
    ret = DLI_SetMcs(&param11);
    ASSERT_EQ(0, ret);
}

HWTEST_F(DliTest, TestCaseSendDevdCmd, TestSize.Level1)
{
    uint8_t data[] = {0x01, 0x01, 0x01, 0x01, 0x01, 0x01};
    DLI_ScanParam scanparam = {0};
    scanparam.ownAddrType = data[DATA_INDEX_0];
    scanparam.scanFilterPolicy = data[DATA_INDEX_1];
    scanparam.frameFormatInd = SCAN_FRAME_TYPE_1;
    uint32_t ret = DLI_SetScanParam(&scanparam);
    ASSERT_EQ(0, ret);

    scanparam.frameFormatInd = SCAN_FRAME_TYPE_1 | SCAN_FRAME_TYPE_4;
    ret = DLI_SetScanParam(&scanparam);
    ASSERT_EQ(0, ret);

    DLI_ScanEnable scanEnable = {0};
    ret = DLI_EnableScan(&scanEnable);
    ASSERT_EQ(0, ret);

    ret = DLI_ReadAdvSetsNum();
    ASSERT_EQ(0, ret);

    DLI_AdvParam advParam = {0};
    advParam.advHandle = data[DATA_INDEX_0];
    advParam.advMode = data[DATA_INDEX_1];
    advParam.advGtRole = data[DATA_INDEX_2];
    advParam.primAdvIntervalMin[DATA_INDEX_0] = data[DATA_INDEX_0];
    advParam.primAdvIntervalMin[DATA_INDEX_1] = data[DATA_INDEX_1];
    advParam.primAdvIntervalMin[DATA_INDEX_2] = data[DATA_INDEX_2];
    advParam.primAdvIntervalMax[DATA_INDEX_0] = data[DATA_INDEX_0];
    advParam.primAdvIntervalMax[DATA_INDEX_1] = data[DATA_INDEX_1];
    advParam.primAdvIntervalMax[DATA_INDEX_2] = data[DATA_INDEX_2];
    advParam.primAdvChannelMap = data[DATA_INDEX_3];
    advParam.primAdvFrameFormat = PRIM_ADV_FRAME_TYPE_1;
    (void)memcpy_s(advParam.ownAddr, SLE_ADDR_LEN, data, SLE_ADDR_LEN);
    (void)memcpy_s(advParam.peerAddr, SLE_ADDR_LEN, data, SLE_ADDR_LEN);
    (void)memcpy_s(&advParam.connIntervalMin, sizeof(DLI_ConnParam), data, sizeof(DLI_ConnParam));
    ret = DLI_SetAdvParam(&advParam);
    ASSERT_EQ(0, ret);

    advParam.primAdvFrameFormat = PRIM_ADV_FRAME_TYPE_4_M_0;
    ret = DLI_SetAdvParam(&advParam);
    ASSERT_EQ(0, ret);

    uint16_t dataOff = sizeof(uint8_t) + sizeof(uint8_t);
    uint8_t dataLen = SDF_ArrayCount(data) - dataOff;
    DLI_AdvData *advData = (DLI_AdvData *)SDF_MemZalloc(sizeof(DLI_AdvData) + dataLen);
    ASSERT_NE(advData, nullptr);
    advData->advHandle = data[DATA_INDEX_0];
    advData->operation = data[DATA_INDEX_1];
    (void)memcpy_s(advData->advData, dataLen, data, dataLen);
    advData->advDataLen = dataLen;
    ret = DLI_SetAdvData(advData, dataOff);
    ASSERT_EQ(0, ret);
    SDF_MemFree(advData);

    DLI_ScanRspData *scanRspData = (DLI_ScanRspData *)SDF_MemZalloc(sizeof(DLI_ScanRspData) + dataLen);
    ASSERT_NE(scanRspData, nullptr);
    scanRspData->advHandle = data[DATA_INDEX_0];
    scanRspData->operation = data[DATA_INDEX_1];
    (void)memcpy_s(scanRspData->scanRspData, dataLen, data, dataLen);
    scanRspData->scanRspDataLen = dataLen;
    ret = DLI_SetScanRspData(scanRspData, dataOff);
    ASSERT_EQ(0, ret);
    SDF_MemFree(scanRspData);

    ret = DLI_ReadMaximumAdvDataLen();
    ASSERT_EQ(0, ret);

    uint8_t advHandle = 0;
    DLI_AdvEnable param = {0};
    ret = DLI_EnableAdv(advHandle, &param);
    ASSERT_EQ(0, ret);

    ret = DLI_RemoveAdvSet(advHandle);
    ASSERT_EQ(0, ret);
}

HWTEST_F(DliTest, TestCaseSendIcbCmd, TestSize.Level1)
{
    DLI_SetWriteFileCallback(TEST_DliWriteLogStub);
    DLI_ICGParam freqParam = {0};
    DLI_ICGCbkParam cbkParam = {0};
    freqParam.opCode = DLI_SET_IMG_PARAM;
    uint32_t ret = DLI_SetICGParam(&freqParam, &cbkParam);
    ASSERT_EQ(0, ret);

    DLI_ICGTestParam testParam = {0};
    testParam.opCode = DLI_SET_IMG_PARAM_TEST;
    ret = DLI_SetICGTestParam(&testParam, &cbkParam);
    ASSERT_EQ(0, ret);

    DLI_CmdOpcode opcode = DLI_REMOVE_ICB_DATA_PATH;
    ret = DLI_RemoveICGParam(opcode, &cbkParam);
    ASSERT_EQ(0, ret);

    DLI_ICBConnectionParam connParam = {0};
    connParam.opCode = DLI_CREATE_IMB;
    ret = DLI_CreateICB(&connParam, &cbkParam);
    ASSERT_EQ(0, ret);

    DLI_DisconnectParam disParam = {0};
    ret = DLI_DisconnectICB(&disParam, &cbkParam);
    ASSERT_EQ(0, ret);

    DLI_AcceptICBReqParam acceptParam = {0};
    ret = DLI_AcceptICBReq(&acceptParam);
    ASSERT_NE(0, ret);

    DLI_RejectICBReqParam rejectParam = {};
    ret = DLI_RejectICBReq(&rejectParam);
    ASSERT_NE(0, ret);

    DLI_SetupICBDataPathParam setUpParam = {0};
    ret = DLI_SetupICBDataPath(&setUpParam, &cbkParam);
    ASSERT_NE(0, ret);
    
    DLI_SetupICBDataPathParam icbDataPath;
    (void)memset_s(&icbDataPath, sizeof(DLI_SetupICBDataPathParam), 0, sizeof(DLI_SetupICBDataPathParam));
    uint8_t cfg[] = {0x01, 0x02, 0x03, 0x04};
    icbDataPath.codecConfigLen = (uint8_t)sizeof(cfg);
    icbDataPath.codecConfigData = (uint8_t *)SDF_MemZalloc(icbDataPath.codecConfigLen);
    DLI_CHECK_RETURN(icbDataPath.codecConfigData, "codecConfigData malloc failed");
    (void)memcpy_s(icbDataPath.codecConfigData, SDF_ArrayCount(cfg), cfg, SDF_ArrayCount(cfg));
    ret = DLI_SetupICBDataPath(&icbDataPath, &cbkParam);
    ASSERT_NE(0, ret);
    SDF_MemFree(icbDataPath.codecConfigData);

    DLI_RemoveICBDataPathParam removeParam = {0};
    ret = DLI_RemoveICBDataPath(&removeParam, &cbkParam);
    ASSERT_NE(0, ret);
}

HWTEST_F(DliTest, TestCaseRecvEvent001, TestSize.Level1)
{
    uint8_t data[] = {0x06, 0x0c, 0x01, 0x00, 0xfb, 0x00};
    uint32_t size = SDF_ArrayCount(data);
    uint16_t event = 0x0002;
    uint16_t cmd = 0x0C06;

    DLI_InnerCbkLineStru *innerCbkTable = (DLI_InnerCbkLineStru *)SDF_MemZalloc(sizeof(DLI_InnerCbkLineStru));
    ASSERT_NE(innerCbkTable, nullptr);

    innerCbkTable->opcode = cmd;
    innerCbkTable->func = &DLI_ExecuteCommandCbk;
    DLI_InnerEventCbkReg(innerCbkTable, 1);

    DLI_ManagerContext *managerContext = (DLI_ManagerContext *)SDF_MemZalloc(sizeof(DLI_ManagerContext));
    ASSERT_NE(managerContext, nullptr);

    managerContext->innerCbkTable = innerCbkTable;

    managerContext->cbk = DLI_GetCbk(DLI_CBK_READ_ADV_DATA_LEN);

    RecvEventHandler(event, managerContext, data, size);
    ASSERT_EQ(0xfb, g_cmdResData);
    ASSERT_EQ(size - DLI_DATA_OFFSET, g_cmdResLen);

    SDF_MemFree(innerCbkTable);
    SDF_MemFree(managerContext);
}

HWTEST_F(DliTest, TestCaseRecvEvent004, TestSize.Level1)
{
    // cmd cmd | num | status
    RecvEventHandler(DLI_CMD_STATUS_EVT, NULL, NULL, 0);
    ASSERT_EQ(0, g_cmdResData);
    ASSERT_EQ(0, g_cmdResLen);

    uint8_t data[] = {0x01, 0x14, 0x01, 0x01};
    uint32_t size = SDF_ArrayCount(data);
    RecvEventHandler(DLI_CMD_STATUS_EVT, NULL, data, size);
    ASSERT_EQ(0x01, g_cmdResData);
    ASSERT_EQ(sizeof(DLI_CommandErrorStru), g_cmdResLen);
}

HWTEST_F(DliTest, TestCaseRecvEvent005, TestSize.Level1)
{
    RecvEventHandler(DLI_CMD_ERROR_EVT, NULL, NULL, 0);
    ASSERT_EQ(0, g_cmdResData);
    ASSERT_EQ(0, g_cmdResLen);

    // status status| opcode opcode | req
    uint8_t data[] = {0x01, 0x00, 0x02, 0x10, 0x00};
    uint32_t size = SDF_ArrayCount(data);
    RecvEventHandler(DLI_CMD_ERROR_EVT, NULL, data, size);
    ASSERT_NE(size, g_cmdResLen);
}

HWTEST_F(DliTest, TestCaseRecvEvent006, TestSize.Level1)
{
    RecvEventHandler(DLI_ENCRYPTION_PARAMETER_REQUEST_EVT, NULL, NULL, 0);
    ASSERT_EQ(0, g_cmdResData);
    ASSERT_EQ(0, g_cmdResLen);

    DLI_EncryptParamReqEvt data = {0};
    data.connHandle = 0x0E;
    uint32_t size = sizeof(DLI_EncryptParamReqEvt);
    RecvEventHandler(DLI_ENCRYPTION_PARAMETER_REQUEST_EVT, NULL, (uint8_t *)&data, size);
    ASSERT_EQ(0x0E, g_cmdResData);
    ASSERT_EQ(size, g_cmdResLen);
}

HWTEST_F(DliTest, TestCaseRecvEvent007, TestSize.Level1)
{
    RecvEventHandler(DLI_DATA_LENGTH_CHANGE_EVT, NULL, NULL, 0);
    ASSERT_EQ(0, g_cmdResData);
    ASSERT_EQ(0, g_cmdResLen);

    DLI_DataLenChangeEvt data = {0};
    data.connHandle = 0x03;
    uint32_t size = sizeof(DLI_DataLenChangeEvt);
    RecvEventHandler(DLI_DATA_LENGTH_CHANGE_EVT, NULL, (uint8_t *)&data, size);
    ASSERT_EQ(0x03, g_cmdResData);
    ASSERT_EQ(size, g_cmdResLen);
}

HWTEST_F(DliTest, TestCaseRecvEvent008, TestSize.Level1)
{
    RecvEventHandler(DLI_READ_REMOTE_FEATURES_COMPLETE_EVT, NULL, NULL, 0);
    ASSERT_EQ(0, g_cmdResData);
    ASSERT_EQ(0, g_cmdResLen);

    DLI_ReadRemoteFeatsEvt data = {0};
    uint32_t size = sizeof(DLI_ReadRemoteFeatsEvt);
    RecvEventHandler(DLI_READ_REMOTE_FEATURES_COMPLETE_EVT, NULL, (uint8_t *)&data, size);
    ASSERT_EQ(0x00, g_cmdResData);
    ASSERT_EQ(size, g_cmdResLen);
}

HWTEST_F(DliTest, TestCaseRecvEvent009, TestSize.Level1)
{
    RecvEventHandler(DLI_CONTROLLER_DATA_EVT, NULL, NULL, 0);
    ASSERT_EQ(0, g_cmdResData);
    ASSERT_EQ(0, g_cmdResLen);

    uint8_t data[] = {0x01, 0x01, 0x01, 0x01, 0x01, 0x01};
    uint8_t len = sizeof(DLI_ControllerDataEvt) + SDF_ArrayCount(data);
    DLI_ControllerDataEvt *cmd = (DLI_ControllerDataEvt *)SDF_MemZalloc(len);
    DLI_CHECK_RETURN(cmd != NULL, "cmd malloc failed");
    (void)memset_s(cmd, len, 0, len);
    cmd->len = SDF_ArrayCount(data);
    (void)memcpy_s(cmd->data, SDF_ArrayCount(data), data, SDF_ArrayCount(data));
    RecvEventHandler(DLI_CONTROLLER_DATA_EVT, NULL, (uint8_t *)cmd, len);
    ASSERT_EQ(0x00, g_cmdResData);
    SDF_MemFree(cmd);
}

HWTEST_F(DliTest, TestCaseRecvEvent010, TestSize.Level1)
{
    RecvEventHandler(DLI_READ_REMOTE_MEASURE_CAPS_STATUS_VENDOR_EVT, NULL, NULL, 0);
    ASSERT_EQ(0, g_cmdResData);
    ASSERT_EQ(0, g_cmdResLen);

    DLI_ReadRemoteCsCapsEvt data = {0};
    uint32_t size = sizeof(DLI_ReadRemoteCsCapsEvt);
    RecvEventHandler(DLI_READ_REMOTE_MEASURE_CAPS_STATUS_VENDOR_EVT, NULL, (uint8_t *)&data, size);
    ASSERT_EQ(0x00, g_cmdResData);
    ASSERT_EQ(size, g_cmdResLen);
}

HWTEST_F(DliTest, TestCaseRecvEvent011, TestSize.Level1)
{
    RecvEventHandler(DLI_ENCRYPTION_CHANGE_EVT, NULL, NULL, 0);
    ASSERT_EQ(0, g_cmdResData);
    ASSERT_EQ(0, g_cmdResLen);

    DLI_EncryptChangeEvt cmd = {0};
    cmd.status = 0x00;
    cmd.connHandle = 0x0001;
    cmd.encryptChange = 0x01;

    RecvEventHandler(DLI_ENCRYPTION_CHANGE_EVT, NULL, (uint8_t *)&cmd, sizeof(DLI_EncryptChangeEvt));
    ASSERT_EQ(0x01, g_cmdResData);
    ASSERT_EQ(sizeof(DLI_EncryptChangeEvt), g_cmdResLen);

    cmd.status = DLI_COMMAND_TIMEOUT;
    cmd.connHandle = 0x0001;
    cmd.encryptChange = 0x01;

    RecvEventHandler(DLI_ENCRYPTION_CHANGE_EVT, NULL, (uint8_t *)&cmd, sizeof(DLI_EncryptChangeEvt));
    ASSERT_EQ(0x01, g_cmdResData);
}

HWTEST_F(DliTest, TestCaseRecvEvent013, TestSize.Level1)
{
    RecvEventHandler(DLI_SET_PHY_COMPLETE_EVT, NULL, NULL, 0);
    ASSERT_EQ(0, g_cmdResData);
    ASSERT_EQ(0, g_cmdResLen);

    DLI_SetPhyEvt data = {0};
    uint32_t size = sizeof(DLI_SetPhyEvt);
    RecvEventHandler(DLI_SET_PHY_COMPLETE_EVT, NULL, (uint8_t *)&data, size);
    ASSERT_EQ(0x00, g_cmdResData);
    ASSERT_EQ(size, g_cmdResLen);
}

HWTEST_F(DliTest, TestCaseRecvEvent014, TestSize.Level1)
{
    RecvEventHandler(DLI_DISCONNECTION_COMPLETE_EVT, NULL, NULL, 0);
    ASSERT_EQ(0, g_cmdResData);
    ASSERT_EQ(0, g_cmdResLen);

    DLI_DisconnectEvt data = {0};
    uint32_t size = sizeof(DLI_DisconnectEvt);
    RecvEventHandler(DLI_DISCONNECTION_COMPLETE_EVT, NULL, (uint8_t *)&data, size);
    ASSERT_EQ(0x00, g_cmdResData);
    ASSERT_EQ(size, g_cmdResLen);
}

HWTEST_F(DliTest, TestCaseRecvEvent015, TestSize.Level1)
{
    RecvEventHandler(DLI_SET_ACB_EVT_PARAM_EVT, NULL, NULL, 0);
    ASSERT_EQ(0, g_cmdResData);
    ASSERT_EQ(0, g_cmdResLen);

    DLI_SetAcbEvtParamEvt data = {0};
    uint32_t size = sizeof(DLI_SetAcbEvtParamEvt);
    RecvEventHandler(DLI_SET_ACB_EVT_PARAM_EVT, NULL, (uint8_t *)&data, size);
    ASSERT_EQ(0x00, g_cmdResData);
    ASSERT_EQ(size, g_cmdResLen);
}

HWTEST_F(DliTest, TestCaseRecvEvent016, TestSize.Level1)
{
    RecvEventHandler(DLI_READ_REMOTE_RSSI_EVT, NULL, NULL, 0);
    ASSERT_EQ(0, g_cmdResData);
    ASSERT_EQ(0, g_cmdResLen);
}

HWTEST_F(DliTest, TestCaseRecvEvent017, TestSize.Level1)
{
    RecvEventHandler(DLI_ACB_LOW_LATENCY_EN_EVT, NULL, NULL, 0);
    ASSERT_EQ(0, g_cmdResData);
    ASSERT_EQ(0, g_cmdResLen);

    DLI_AcbLowLatencyEnableEvt data = {0};
    uint32_t size = sizeof(DLI_AcbLowLatencyEnableEvt);
    RecvEventHandler(DLI_ACB_LOW_LATENCY_EN_EVT, NULL, (uint8_t *)&data, size);
    ASSERT_EQ(0x00, g_cmdResData);
    ASSERT_EQ(size, g_cmdResLen);
}

HWTEST_F(DliTest, TestCaseRecvEvent018, TestSize.Level1)
{
    RecvEventHandler(DLI_ADVERTISING_TERMINATED_EVT, NULL, NULL, 0);
    ASSERT_EQ(0, g_cmdResData);
    ASSERT_EQ(0, g_cmdResLen);

    DLI_AdvertisingTerminatedEvt data = {0};
    uint32_t size = sizeof(DLI_AdvertisingTerminatedEvt);
    RecvEventHandler(DLI_ADVERTISING_TERMINATED_EVT, NULL, (uint8_t *)&data, size);
    ASSERT_EQ(0x00, g_cmdResData);
    ASSERT_EQ(size, g_cmdResLen);
}

HWTEST_F(DliTest, TestCaseRecvEvent019, TestSize.Level1)
{
    RecvEventHandler(DLI_READ_REMOTE_VERSION_COMPLETE_EVT, NULL, NULL, 0);
    ASSERT_EQ(0, g_cmdResData);
    ASSERT_EQ(0, g_cmdResLen);

    DLI_ReadRemoteVersionEvt data = {0};
    uint32_t size = sizeof(DLI_ReadRemoteVersionEvt);
    RecvEventHandler(DLI_READ_REMOTE_VERSION_COMPLETE_EVT, NULL, (uint8_t *)&data, size);
    ASSERT_EQ(0x00, g_cmdResData);
    ASSERT_EQ(size, g_cmdResLen);
}

HWTEST_F(DliTest, TestCaseRecvEvent020, TestSize.Level1)
{
    RecvEventHandler(DLI_REMOTE_CONNECTION_PARAMETER_REQUEST_EVT, NULL, NULL, 0);
    ASSERT_EQ(0, g_cmdResData);
    ASSERT_EQ(0, g_cmdResLen);

    DLI_RemoteConnParamReqEvt data = {0};
    uint32_t size = sizeof(DLI_RemoteConnParamReqEvt);
    RecvEventHandler(DLI_REMOTE_CONNECTION_PARAMETER_REQUEST_EVT, NULL, (uint8_t *)&data, size);
    ASSERT_EQ(0x00, g_cmdResData);
    ASSERT_EQ(size, g_cmdResLen);
}

HWTEST_F(DliTest, TestCaseRecvEvent021, TestSize.Level1)
{
    RecvEventHandler(DLI_CONNECTION_UPDATE_EVT, NULL, NULL, 0);
    ASSERT_EQ(0, g_cmdResData);
    ASSERT_EQ(0, g_cmdResLen);

    DLI_ConnectionUpdateCmpEvt data = {0};
    uint32_t size = sizeof(DLI_ConnectionUpdateCmpEvt);
    RecvEventHandler(DLI_CONNECTION_UPDATE_EVT, NULL, (uint8_t *)&data, size);
    ASSERT_EQ(0x00, g_cmdResData);
    ASSERT_EQ(size, g_cmdResLen);
}

HWTEST_F(DliTest, TestCaseRecvEvent022, TestSize.Level1)
{
    RecvEventHandler(DLI_CONNECTION_COMPLETE_EVT, NULL, NULL, 0);
    ASSERT_EQ(0, g_cmdResData);
    ASSERT_EQ(0, g_cmdResLen);

    DLI_ConnectionCompleteEvt cmd = {0};
    (void)memset_s(&cmd, sizeof(DLI_ConnectionCompleteEvt), 0, sizeof(DLI_ConnectionCompleteEvt));

    RecvEventHandler(DLI_CONNECTION_COMPLETE_EVT, NULL, (uint8_t *)&cmd, sizeof(DLI_ConnectionCompleteEvt));
    ASSERT_EQ(0x00, g_cmdResData);
    ASSERT_EQ(sizeof(DLI_ConnectionCompleteEvt), g_cmdResLen);
}

HWTEST_F(DliTest, TestCaseRecvEvent023, TestSize.Level1)
{
    RecvEventHandler(DLI_ADVERTISING_REPORT_EVT, NULL, NULL, 0);
    ASSERT_EQ(0, g_cmdResData);
    ASSERT_EQ(0, g_cmdResLen);

    uint8_t data[] = {0x01, 0x01, 0x01, 0x01, 0x01, 0x01};
    uint8_t len = sizeof(DLI_AdvReportEvt) + SDF_ArrayCount(data);
    DLI_AdvReportEvt *cmd = (DLI_AdvReportEvt *)SDF_MemZalloc(len);
    DLI_CHECK_RETURN(cmd != NULL, "cmd malloc failed");
    (void)memset_s(cmd, len, 0, len);
    cmd->dataLength = SDF_ArrayCount(data);
    (void)memcpy_s(cmd->data, SDF_ArrayCount(data), data, SDF_ArrayCount(data));
    RecvEventHandler(DLI_ADVERTISING_REPORT_EVT, NULL, (uint8_t *)cmd, len);
    ASSERT_EQ(0x00, g_cmdResData);
    SDF_MemFree(cmd);
}

HWTEST_F(DliTest, TestCaseRecvEvent024, TestSize.Level1)
{
    RecvEventHandler(DLI_MEASURE_IQ_REPORT_VENDOR_EVT, NULL, NULL, 0);
    ASSERT_EQ(0, g_cmdResData);
    ASSERT_EQ(0, g_cmdResLen);

    // slem info does not match
    // bit位默认都是0，所以data长度为0
    uint8_t len = sizeof(DLI_CsIqReportEvt);
    DLI_CsIqReportEvt *cmd = (DLI_CsIqReportEvt *)SDF_MemZalloc(len);
    DLI_CHECK_RETURN(cmd != NULL, "cmd malloc failed");
    (void)memset_s(cmd, len, 0, len);
    RecvEventHandler(DLI_MEASURE_IQ_REPORT_VENDOR_EVT, NULL, (uint8_t *)cmd, len);
    ASSERT_EQ(0x00, g_cmdResData);
    SDF_MemFree(cmd);

    uint8_t len1 = sizeof(DLI_SlemTof) + sizeof(DLI_SlemChnlMeas) + sizeof(DLI_SlemIqData) * TEST_IQ_MAX_CHNL_NUM + sizeof(DLI_SlemVender);
    uint8_t *cmd1 = (uint8_t *)SDF_MemZalloc(len1);
    DLI_CHECK_RETURN(cmd1 != NULL, "cmd1 malloc failed");
    // 全部置为1，长度根据DLI_GetSlemInfoDataLen计算得到
    (void)memset_s(cmd1, len1, 0xFF, len1);
    RecvEventHandler(DLI_MEASURE_IQ_REPORT_VENDOR_EVT, NULL, cmd1, len1);
    ASSERT_EQ(0xFF, g_cmdResData);
    SDF_MemFree(cmd1);

    RecvEventHandler(DLI_VENDOR_EVENT_EVT, NULL, NULL, 0);
    ASSERT_EQ(0, g_cmdResLen);

    // 码流中不返回opcode，buf:subEventCode，arg（回调给别的模块）
    DLI_AcbSetSubrateEvt sub = {0};
    uint8_t subEventCode = 0x02;
    uint8_t buf[sizeof(uint8_t) + sizeof(sub)];
    (void)memcpy_s(buf, sizeof(uint8_t), &subEventCode, sizeof(uint8_t));
    (void)memcpy_s(buf + sizeof(uint8_t), sizeof(DLI_AcbSetSubrateEvt), &sub, sizeof(DLI_AcbSetSubrateEvt));
    RecvEventHandler(DLI_VENDOR_EVENT_EVT, NULL, (uint8_t *)&buf, SDF_ArrayCount(buf));
    ASSERT_EQ(0x00, g_cmdResData);

    uint8_t subEventCode1 = 0x03;
    uint8_t buf1[sizeof(uint8_t) + sizeof(sub)];
    (void)memcpy_s(buf1, sizeof(uint8_t), &subEventCode1, sizeof(uint8_t));
    (void)memcpy_s(buf1 + sizeof(uint8_t), sizeof(DLI_AcbSetSubrateEvt), &sub, sizeof(DLI_AcbSetSubrateEvt));
    RecvEventHandler(DLI_VENDOR_EVENT_EVT, NULL, (uint8_t *)&buf1, SDF_ArrayCount(buf1));
    ASSERT_EQ(0x00, g_cmdResData);

    DLI_RssiEvt sub2 = {0};
    uint8_t subEventCode2 = 0x01;
    uint8_t buf2[sizeof(uint8_t) + sizeof(sub2)];
    (void)memcpy_s(buf2, sizeof(uint8_t), &subEventCode2, sizeof(uint8_t));
    (void)memcpy_s(buf2 + sizeof(uint8_t), sizeof(DLI_RssiEvt), &sub2, sizeof(DLI_RssiEvt));
    RecvEventHandler(DLI_VENDOR_EVENT_EVT, NULL, (uint8_t *)&buf2, SDF_ArrayCount(buf2));
    ASSERT_EQ(0x00, g_cmdResData);

    DLI_PowerLevelEvt sub3 = {0};
    uint8_t subEventCode3 = 0x13;
    uint8_t buf3[sizeof(uint8_t) + sizeof(sub3)];
    (void)memcpy_s(buf3, sizeof(uint8_t), &subEventCode3, sizeof(uint8_t));
    (void)memcpy_s(buf3 + sizeof(uint8_t), sizeof(DLI_PowerLevelEvt), &sub3, sizeof(DLI_PowerLevelEvt));
    RecvEventHandler(DLI_VENDOR_EVENT_EVT, NULL, (uint8_t *)&buf3, SDF_ArrayCount(buf3));
    ASSERT_EQ(0x00, g_cmdResData);
}
}  // namespace TEST
}  // namespace Nearlink
}  // namespace OHOS