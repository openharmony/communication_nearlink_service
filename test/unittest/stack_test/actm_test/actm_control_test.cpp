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

#include "gtest/gtest.h"
#include "securec.h"

#include "sdf_addr.h"
#include "sdf_mem.h"

#include "stack_schedule_mock.h"
#include "stack_schedule_stub.h"
#include "stack_cm_mock.h"
#include "stack_cm_stub.h"
#include "stack_dtap_mock.h"
#include "stack_dtap_stub.h"
#include "stack_qosm_mock.h"
#include "stack_qosm_stub.h"
#include "stack_dli_mock.h"
#include "stack_dli_stub.h"

#include "cm_def.h"
#include "nlstk_sm.h"
#include "nlstk_cfgdb.h"
#include "nlstk_cfgdb_api.h"

#include "actm_api.h"
#include "actm.h"
#include "actm_tbl.h"
#include "actm_qosm_adapter.h"
#include "actm_control.h"

#define QOSM_DOWN 0
#define QOSM_UP   1

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

namespace {
SLE_Addr_S g_addrA = {.type = PUBLIC_ADDRESS, .addr = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66}};
SLE_Addr_S g_addrB = {.type = PUBLIC_ADDRESS, .addr = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}};

const uint16_t TEST_GID = 8;
const int32_t TEST_APP_A = 0x60;
const int32_t TEST_APP_B = 0x61;

int g_eventCount = 0;
uint8_t g_lastEvent = 0xFF;
uint8_t g_lastEventResult = 0xFF;
SLE_Addr_S g_lastEventAddr = {0};
int g_propCount = 0;
uint8_t g_propNum = 0;
int g_locationCount = 0;
bool g_lastIsLeft = false;
int g_streamTypeCount = 0;
uint32_t g_lastStreamType = 0;

void ResetCaptures()
{
    g_eventCount = 0;
    g_lastEvent = 0xFF;
    g_lastEventResult = 0xFF;
    memset_s(&g_lastEventAddr, sizeof(g_lastEventAddr), 0, sizeof(g_lastEventAddr));
    g_propCount = 0;
    g_propNum = 0;
    g_locationCount = 0;
    g_lastIsLeft = false;
    g_streamTypeCount = 0;
    g_lastStreamType = 0;
}

void StubEventCbk(SLE_Addr_S *addr, uint8_t eventType, uint8_t result, void *param)
{
    g_eventCount++;
    g_lastEvent = eventType;
    g_lastEventResult = result;
    if (addr != NULL) {
        (void)memcpy_s(&g_lastEventAddr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    }
}

void StubPropCbk(SLE_Addr_S *addr, uint8_t num, NLSTK_ActmProp_S *prop)
{
    g_propCount++;
    g_propNum = num;
}

void StubBitCbk(NLSTK_ActmBitrateChange_S *bitrate) {}

void StubLocationCbk(SLE_Addr_S *addr, bool isLeft)
{
    g_locationCount++;
    g_lastIsLeft = isLeft;
}

void StubStreamTypeCbk(SLE_Addr_S *addr, uint32_t availableStreamType)
{
    g_streamTypeCount++;
    g_lastStreamType = availableStreamType;
}

void StubUpdownCbk(NLSTK_ActmAutoRateSendMsg_S *upDownParam) {}

void StubDirectionCbk(SLE_Addr_S *addr, NLSTK_ActmSetDirection_S *param) {}

void PushPoint(ActmRemoteDevice_S *device, uint8_t pointId, uint8_t propId, uint8_t type)
{
    ActmAccessPoint_S *point = (ActmAccessPoint_S *)SDF_MemZalloc(sizeof(ActmAccessPoint_S));
    ASSERT_NE(point, nullptr);
    point->pointId = pointId;
    point->propId = propId;
    point->type = type;
    SDF_VectorEmplaceBack(device->points, point);
}

ActmProp_S *PushProp(ActmRemoteDevice_S *device, uint8_t propId, uint8_t propType)
{
    ActmProp_S *prop = (ActmProp_S *)SDF_MemZalloc(sizeof(ActmProp_S));
    EXPECT_NE(prop, nullptr);
    prop->propId = propId;
    prop->propType = propType;
    prop->acceptType = 0x21;
    SDF_VectorEmplaceBack(device->props, prop);
    return prop;
}

ActmRemoteDevice_S *CreateGroupedDevice(const SLE_Addr_S *addr, uint16_t gid, int32_t appId)
{
    SLE_Addr_S tmp = *addr;
    EXPECT_EQ(ActmCreateRemoteDevice(&tmp), (uint32_t)NLSTK_OK);
    ActmRemoteDevice_S *device = ActmFindDeviceByAddr(&tmp);
    EXPECT_NE(device, nullptr);
    device->appId = appId;
    device->info.ctrlHandle = 0x11;
    PushPoint(device, 1, 1, NLSTK_ACTM_SINK_POINT);
    PushProp(device, 1, NLSTK_ACTM_SINK_POINT);
    AddDeviceToGroup(device, gid);
    return device;
}

ActmControlRsp_S *MakeControlRsp(int32_t appId, uint8_t opcode, uint8_t pointNum, uint8_t rescode)
{
    uint8_t *buf = (uint8_t *)SDF_MemZalloc(sizeof(ActmControlRsp_S) + pointNum * sizeof(ActmPointControlRes_S));
    ActmControlRsp_S *rsp = (ActmControlRsp_S *)buf;
    rsp->appId = appId;
    rsp->opcode = opcode;
    rsp->pointNum = pointNum;
    for (uint8_t i = 0; i < pointNum; i++) {
        rsp->param[i].pointId = i + 1;
        rsp->param[i].rescode = rescode;
    }
    return rsp;
}

void SetManufacturerQosIdAbility(const SLE_Addr_S *addr, bool enabled)
{
    NLSTK_ManufacturerAbility_S ability = {0};
    if (enabled) {
        ability.ability[CFGDB_QOS_ID_CONFIG / 8] = 1 << (CFGDB_QOS_ID_CONFIG % 8);
    }
    SLE_Addr_S tmp = *addr;
    NLSTK_CfgdbSetManufacturerAbility(&tmp, &ability);
}
} // namespace

class ActmControlTest : public testing::Test {
public:
    NiceMock<ScheduleMock> scheduleMock;
    NiceMock<CmMock> cmMock;
    NiceMock<DtapMock> dtapMock;
    NiceMock<QosmMock> qosmMock;
    NiceMock<DliMock> dliMock;

protected:
    void SetUp() override
    {
        ResetCaptures();
        TEST_ScheduleInit();
        EXPECT_CALL(scheduleMock, SchedulePostTask).WillRepeatedly(TEST_SchedulePostTaskStub);
        EXPECT_CALL(scheduleMock, SchedulePostTaskBlocked).WillRepeatedly(TEST_SchedulePostTaskBlockedStub);

        TEST_CM_Init();
        EXPECT_CALL(cmMock, CM_RegLogicLinkListener).WillRepeatedly(TEST_CM_RegLogicLinkListener);
        EXPECT_CALL(cmMock, CM_UnRegLogicLinkListener).WillRepeatedly(TEST_CM_UnRegLogicLinkListener);
        EXPECT_CALL(cmMock, CM_GetLogicLinkByAddr).WillRepeatedly(Return((uint32_t)0));
        EXPECT_CALL(cmMock, CM_GetLogicLinkByLcid).WillRepeatedly(Return(1));

        EXPECT_CALL(dtapMock, DTAP_RegisterDataRecvCb).WillRepeatedly(TEST_DTAP_RegisterDataRecvCb);
        EXPECT_CALL(dtapMock, DTAP_UnregisterDataRecvCb).WillRepeatedly(TEST_DTAP_UnregisterDataRecvCb);
        EXPECT_CALL(dtapMock, DTAP_DataSend).WillRepeatedly(TEST_DTAP_DataSend);

        EXPECT_CALL(qosmMock, QOSM_AutoRateRegisterCallback).WillRepeatedly(TEST_QOSM_AutoRateRegisterCallback);
        EXPECT_CALL(qosmMock, QOSM_AutoRateUnregisterCallback).WillRepeatedly(TEST_QOSM_AutoRateUnregisterCallback);
        EXPECT_CALL(qosmMock, QOSM_AutoRateSetAutorateParam).WillRepeatedly(
            AUTORATE_QOSM_AutoRateSetAutorateParam);
        EXPECT_CALL(qosmMock, QOSM_AutoRateAddConnection).WillRepeatedly(TEST_QOSM_AutoRateAddConnection);
        EXPECT_CALL(qosmMock, QOSM_AutoRateAddDataPath).WillRepeatedly(TEST_QOSM_AutoRateAddDataPath);
        EXPECT_CALL(qosmMock, QOSM_AutoRateDeleteDataPath).WillRepeatedly(TEST_QOSM_AutoRateDeleteDataPath);
        EXPECT_CALL(qosmMock, QOSM_AutoRateDeleteConnection).WillRepeatedly(TEST_QOSM_AutoRateDeleteConnection);
        EXPECT_CALL(qosmMock, QOSM_AutoRateSetEarphoneFeedback).WillRepeatedly(TEST_QOSM_AutoRateSetEarphoneFeedback);

        TEST_DLI_Init();
        EXPECT_CALL(dliMock, DLI_CmdCbkReg).WillRepeatedly(TEST_DLI_CmdCbkReg);
        EXPECT_CALL(dliMock, DLI_CmdCbkUnReg).WillRepeatedly(TEST_DLI_CmdCbkUnReg);
        EXPECT_CALL(dliMock, DLI_SetMeasureParam).WillRepeatedly(TEST_DLI_SetMeasureParam);
        EXPECT_CALL(dliMock, DLI_SetMeasureEnable).WillRepeatedly(TEST_DLI_SetMeasureEnable);
        EXPECT_CALL(dliMock, DLI_ReadRemoteMeasureCaps).WillRepeatedly(TEST_DLI_ReadRemoteMeasureCaps);
        EXPECT_CALL(dliMock, DLI_EnableIMGEncryption).WillRepeatedly(TEST_DLI_EnableIMGEncryption);

        TEST_QOSM_SetRecvAutoRateMsgResult(QOSM_SUCCESS);
        TEST_QOSM_SetRemoveParamResult(QOSM_SUCCESS);

        SSAP_LinkInit();
        SSAP_Init();

        CdsmInit();
        SmInit();
        ActmInit();
        ActmEnable();
        CfgdbInit();
        NLSTK_ActmCbk_S cbk = {0};
        cbk.eventCbk = StubEventCbk;
        cbk.propCbk = StubPropCbk;
        cbk.bitCbk = StubBitCbk;
        cbk.locationCbk = StubLocationCbk;
        cbk.streamTypeCbk = StubStreamTypeCbk;
        cbk.callBitUpDownCbk = StubUpdownCbk;
        cbk.addDataPathCbk = StubDirectionCbk;
        NLSTK_ActmRegisterCallback(&cbk);
    }

    void TearDown() override
    {
        CfgdbDeinit();
        ActmDisable();
        ActmDeinit();
        SmDeInit();
        CdsmDeInit();
        SSAP_DeInit();
        SSAP_LinkDeInit();
        NLSTK_ActmCbk_S zeroCbk = {0};
        NLSTK_ActmRegisterCallback(&zeroCbk);
    }
};

// 配置响应成功后设备进入配置完成并上报配置事件
TEST_F(ActmControlTest, UT_ACTM_CTL_001)
{
    ActmRemoteDevice_S *device = CreateGroupedDevice(&g_addrA, TEST_GID, TEST_APP_A);
    device->ssapState = ACTM_START_CONFIG;
    ActmControlRsp_S *rsp = MakeControlRsp(TEST_APP_A, ACTM_CONTROL_CONFIG, 1, 0);
    ActmControlStreamRes(rsp);
    SDF_MemFree(rsp);
    EXPECT_EQ(device->ssapState, (uint8_t)ACTM_CONFIG_COMPL);
    EXPECT_EQ(g_eventCount, 1);
    EXPECT_EQ(g_lastEvent, (uint8_t)NLSTK_ACTM_EVENT_CONFIG);
    EXPECT_EQ(g_lastEventResult, (uint8_t)NLSTK_ACTM_SUCCESS);
    EXPECT_EQ(memcmp(&g_lastEventAddr, &g_addrA, sizeof(SLE_Addr_S)), 0);
}

// 组内还有设备在配置中时不提前播报，全部完成后逐设备播报
TEST_F(ActmControlTest, UT_ACTM_CTL_002)
{
    ActmRemoteDevice_S *deviceA = CreateGroupedDevice(&g_addrA, TEST_GID, TEST_APP_A);
    ActmRemoteDevice_S *deviceB = CreateGroupedDevice(&g_addrB, TEST_GID, TEST_APP_B);
    deviceA->ssapState = ACTM_START_CONFIG;
    deviceB->ssapState = ACTM_START_CONFIG;
    ActmControlRsp_S *rspA = MakeControlRsp(TEST_APP_A, ACTM_CONTROL_CONFIG, 1, 0);
    ActmControlStreamRes(rspA);
    SDF_MemFree(rspA);
    EXPECT_EQ(g_eventCount, 0);
    ActmControlRsp_S *rspB = MakeControlRsp(TEST_APP_B, ACTM_CONTROL_CONFIG, 1, 0);
    ActmControlStreamRes(rspB);
    SDF_MemFree(rspB);
    EXPECT_EQ(deviceA->ssapState, (uint8_t)ACTM_CONFIG_COMPL);
    EXPECT_EQ(deviceB->ssapState, (uint8_t)ACTM_CONFIG_COMPL);
    EXPECT_EQ(g_eventCount, 2);
}

// 开流响应成功后上报开流事件，组内未建表的地址被安全跳过
TEST_F(ActmControlTest, UT_ACTM_CTL_003)
{
    ActmRemoteDevice_S *device = CreateGroupedDevice(&g_addrA, TEST_GID, TEST_APP_A);
    SLE_Addr_S phantom = {.type = PUBLIC_ADDRESS, .addr = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06}};
    ActmQosmAddDevice(TEST_GID, &phantom);
    device->ssapState = ACTM_START_OPEN;
    ActmControlRsp_S *rsp = MakeControlRsp(TEST_APP_A, ACTM_CONTROL_OPEN_PATH, 1, 0);
    ActmControlStreamRes(rsp);
    SDF_MemFree(rsp);
    EXPECT_EQ(device->ssapState, (uint8_t)ACTM_OPEN_COMPL);
    EXPECT_EQ(g_eventCount, 1);
    EXPECT_EQ(g_lastEvent, (uint8_t)NLSTK_ACTM_EVENT_OPEN);
}

// 起流响应成功且服务质量连接就绪时播报传输事件并进入播放态
TEST_F(ActmControlTest, UT_ACTM_CTL_004)
{
    ActmRemoteDevice_S *device = CreateGroupedDevice(&g_addrA, TEST_GID, TEST_APP_A);
    ActmStream_S *stream = ActmCreateStream(device, NLSTK_ACTM_SINK_POINT, NLSTK_ACTM_UNICAST);
    ASSERT_NE(stream, nullptr);
    device->curStreamId = stream->streamId;
    stream->qosIndex = QOSINDEX_3;
    stream->codec.codecId = L2HC_CODEC_NORMAL;
    ASSERT_NE(ActmAllocConnHandle(&g_addrA, TEST_GID), QOSM_INVALID_HANDLE);
    ActmQosmLink_S *link = ActmFindQosmLinkByAddr(TEST_GID, &g_addrA);
    ASSERT_NE(link, nullptr);
    link->state = ACTM_QOSM_CONNECTED;
    device->ssapState = ACTM_START_TRANS;
    device->qosmState = ACTM_CONN_COMPL;
    ActmControlRsp_S *rsp = MakeControlRsp(TEST_APP_A, ACTM_CONTROL_TRANS, 1, 0);
    ActmControlStreamRes(rsp);
    SDF_MemFree(rsp);
    EXPECT_EQ(device->ssapState, (uint8_t)ACTM_START_PLAY_COMPL);
    EXPECT_EQ(g_lastEvent, (uint8_t)NLSTK_ACTM_EVENT_TRANS);
    EXPECT_EQ(g_lastEventResult, (uint8_t)NLSTK_ACTM_SUCCESS);
}

// 起流响应成功但服务质量失败时上报传输失败并回空闲；未开始服务质量流程的设备不播报
TEST_F(ActmControlTest, UT_ACTM_CTL_005)
{
    ActmRemoteDevice_S *device = CreateGroupedDevice(&g_addrA, TEST_GID, TEST_APP_A);
    device->ssapState = ACTM_START_TRANS;
    device->qosmState = ACTM_QOSM_FAIL;
    ActmControlRsp_S *rsp = MakeControlRsp(TEST_APP_A, ACTM_CONTROL_TRANS, 1, 0);
    ActmControlStreamRes(rsp);
    SDF_MemFree(rsp);
    EXPECT_EQ(device->ssapState, (uint8_t)ACTM_SSAP_IDLE);
    EXPECT_EQ(g_eventCount, 1);
    EXPECT_EQ(g_lastEvent, (uint8_t)NLSTK_ACTM_EVENT_TRANS);
    EXPECT_EQ(g_lastEventResult, (uint8_t)NLSTK_ACTM_QOSM_ERROR);
}

// 停流响应成功后上报停流事件
TEST_F(ActmControlTest, UT_ACTM_CTL_006)
{
    ActmRemoteDevice_S *device = CreateGroupedDevice(&g_addrA, TEST_GID, TEST_APP_A);
    device->ssapState = ACTM_START_STOP;
    ActmControlRsp_S *rsp = MakeControlRsp(TEST_APP_A, ACTM_CONTROL_STOP, 1, 0);
    ActmControlStreamRes(rsp);
    SDF_MemFree(rsp);
    EXPECT_EQ(device->ssapState, (uint8_t)ACTM_STOP_COMPL);
    EXPECT_EQ(g_lastEvent, (uint8_t)NLSTK_ACTM_EVENT_STOP);
}

// 释放响应成功且组内连接已删完时上报释放事件并回收链路
TEST_F(ActmControlTest, UT_ACTM_CTL_007)
{
    ActmRemoteDevice_S *device = CreateGroupedDevice(&g_addrA, TEST_GID, TEST_APP_A);
    ASSERT_NE(ActmAllocConnHandle(&g_addrA, TEST_GID), QOSM_INVALID_HANDLE);
    ActmQosmLink_S *link = ActmFindQosmLinkByAddr(TEST_GID, &g_addrA);
    ASSERT_NE(link, nullptr);
    device->connHandle = link->connHandle;
    device->qosmState = ACTM_DEL_COMPL;
    device->ssapState = ACTM_START_RELEASE;
    ActmControlRsp_S *rsp = MakeControlRsp(TEST_APP_A, ACTM_CONTROL_RELEASE, 1, 0);
    ActmControlStreamRes(rsp);
    SDF_MemFree(rsp);
    EXPECT_EQ(device->ssapState, (uint8_t)ACTM_SSAP_IDLE);
    EXPECT_EQ(device->qosmState, (uint8_t)ACTM_QOSM_IDLE);
    EXPECT_EQ(g_lastEvent, (uint8_t)NLSTK_ACTM_EVENT_RELEASE);
    EXPECT_EQ(device->connHandle, 0);
}

// 组内还有设备在释放中时不提前播报释放事件
TEST_F(ActmControlTest, UT_ACTM_CTL_008)
{
    ActmRemoteDevice_S *device = CreateGroupedDevice(&g_addrA, TEST_GID, TEST_APP_A);
    ActmRemoteDevice_S *deviceB = CreateGroupedDevice(&g_addrB, TEST_GID, TEST_APP_B);
    device->qosmState = ACTM_DEL_COMPL;
    device->ssapState = ACTM_START_RELEASE;
    deviceB->qosmState = ACTM_DEL_CONN;
    ActmControlRsp_S *rsp = MakeControlRsp(TEST_APP_A, ACTM_CONTROL_RELEASE, 1, 0);
    ActmControlStreamRes(rsp);
    SDF_MemFree(rsp);
    EXPECT_EQ(g_eventCount, 0);
    EXPECT_EQ(device->ssapState, (uint8_t)ACTM_RELEASE_COMPL);
}

// 码率更新响应成功后提交耳机反馈并上报码率事件
TEST_F(ActmControlTest, UT_ACTM_CTL_009)
{
    ActmRemoteDevice_S *device = CreateGroupedDevice(&g_addrA, TEST_GID, TEST_APP_A);
    ActmStream_S *stream = ActmCreateStream(device, NLSTK_ACTM_SINK_POINT, NLSTK_ACTM_UNICAST);
    ASSERT_NE(stream, nullptr);
    device->curStreamId = stream->streamId;
    stream->codec.codecId = L2HC_CODEC_NORMAL;
    stream->codec.l2hc.bpsRange = 0x3;
    ActmControlRsp_S *rsp = MakeControlRsp(TEST_APP_A, ACTM_CONTROL_BITRATE_UPDATE, 1, 0);
    ActmControlStreamRes(rsp);
    SDF_MemFree(rsp);
    EXPECT_EQ(g_lastEvent, (uint8_t)NLSTK_ACTM_EVENT_BITRATE);
    EXPECT_EQ(g_lastEventResult, (uint8_t)NLSTK_ACTM_SUCCESS);
}

// 配置响应失败时上报错误码并让设备下线回空闲
TEST_F(ActmControlTest, UT_ACTM_CTL_010)
{
    ActmRemoteDevice_S *device = CreateGroupedDevice(&g_addrA, TEST_GID, TEST_APP_A);
    device->ssapState = ACTM_START_CONFIG;
    ActmControlRsp_S *rsp = MakeControlRsp(TEST_APP_A, ACTM_CONTROL_CONFIG, 1, 0x0A);
    ActmControlStreamRes(rsp);
    SDF_MemFree(rsp);
    EXPECT_EQ(g_eventCount, 1);
    EXPECT_EQ(g_lastEventResult, 0x0A);
    EXPECT_EQ(device->ssapState, (uint8_t)ACTM_SSAP_IDLE);
}

// 起流响应失败时按连接阶段回退：加连接中只记失败，连接完成后先删连接再上报下线
TEST_F(ActmControlTest, UT_ACTM_CTL_011)
{
    ActmRemoteDevice_S *device = CreateGroupedDevice(&g_addrA, TEST_GID, TEST_APP_A);
    ASSERT_NE(ActmAllocConnHandle(&g_addrA, TEST_GID), QOSM_INVALID_HANDLE);
    ActmQosmLink_S *link = ActmFindQosmLinkByAddr(TEST_GID, &g_addrA);
    ASSERT_NE(link, nullptr);
    link->state = ACTM_QOSM_CONNECTED;
    device->ssapState = ACTM_START_TRANS;
    device->qosmState = ACTM_CONN_COMPL;
    ResetCaptures();
    ActmControlRsp_S *rsp = MakeControlRsp(TEST_APP_A, ACTM_CONTROL_TRANS, 1, 0x0B);
    ActmControlStreamRes(rsp);
    SDF_MemFree(rsp);
    EXPECT_EQ(device->ssapState, (uint8_t)ACTM_SSAP_IDLE);
    EXPECT_EQ(g_lastEvent, (uint8_t)NLSTK_ACTM_EVENT_TRANS);
    EXPECT_EQ(g_lastEventResult, 0x0B);
}

// 起流失败时设备还在加连接阶段则仅记录失败等待连接结果，不立即上报
TEST_F(ActmControlTest, UT_ACTM_CTL_012)
{
    ActmRemoteDevice_S *device = CreateGroupedDevice(&g_addrA, TEST_GID, TEST_APP_A);
    device->ssapState = ACTM_START_TRANS;
    device->qosmState = ACTM_ADD_CONN;
    ActmControlRsp_S *rsp = MakeControlRsp(TEST_APP_A, ACTM_CONTROL_TRANS, 1, 0x0B);
    ActmControlStreamRes(rsp);
    SDF_MemFree(rsp);
    EXPECT_EQ(device->ssapState, (uint8_t)ACTM_START_FAIL);
    EXPECT_EQ(g_eventCount, 0);
}

// 释放响应失败时仍按释放完成处理走回收流程
TEST_F(ActmControlTest, UT_ACTM_CTL_013)
{
    ActmRemoteDevice_S *device = CreateGroupedDevice(&g_addrA, TEST_GID, TEST_APP_A);
    device->qosmState = ACTM_DEL_COMPL;
    device->ssapState = ACTM_START_RELEASE;
    ActmControlRsp_S *rsp = MakeControlRsp(TEST_APP_A, ACTM_CONTROL_RELEASE, 1, 0x0C);
    ActmControlStreamRes(rsp);
    SDF_MemFree(rsp);
    EXPECT_EQ(g_lastEvent, (uint8_t)NLSTK_ACTM_EVENT_RELEASE);
    EXPECT_EQ(device->ssapState, (uint8_t)ACTM_SSAP_IDLE);
}

// 码率响应失败只上报错误，设备保持原状态不下线
TEST_F(ActmControlTest, UT_ACTM_CTL_014)
{
    ActmRemoteDevice_S *device = CreateGroupedDevice(&g_addrA, TEST_GID, TEST_APP_A);
    device->ssapState = ACTM_START_PLAY_COMPL;
    ActmControlRsp_S *rsp = MakeControlRsp(TEST_APP_A, ACTM_CONTROL_BITRATE_UPDATE, 1, 0x0D);
    ActmControlStreamRes(rsp);
    SDF_MemFree(rsp);
    EXPECT_EQ(g_lastEvent, (uint8_t)NLSTK_ACTM_EVENT_BITRATE);
    EXPECT_EQ(g_lastEventResult, 0x0D);
    EXPECT_EQ(device->ssapState, (uint8_t)ACTM_START_PLAY_COMPL);
}

// 私有音频流的配置响应按传输事件处理
TEST_F(ActmControlTest, UT_ACTM_CTL_015)
{
    ActmRemoteDevice_S *device = CreateGroupedDevice(&g_addrA, TEST_GID, TEST_APP_A);
    device->isPriv = true;
    device->ssapState = ACTM_START_CONFIG;
    ActmControlRsp_S *rsp = MakeControlRsp(TEST_APP_A, ACTM_CONTROL_CONFIG, 1, 0);
    ActmControlStreamRes(rsp);
    SDF_MemFree(rsp);
    EXPECT_EQ(device->ssapState, (uint8_t)ACTM_TRANS_COMPL);
}

// 多点响应中任一点带错误码即按整体错误处理
TEST_F(ActmControlTest, UT_ACTM_CTL_016)
{
    ActmRemoteDevice_S *device = CreateGroupedDevice(&g_addrA, TEST_GID, TEST_APP_A);
    device->ssapState = ACTM_START_CONFIG;
    ActmControlRsp_S *rsp = MakeControlRsp(TEST_APP_A, ACTM_CONTROL_CONFIG, 2, 0);
    rsp->param[1].rescode = 0x0E;
    ActmControlStreamRes(rsp);
    SDF_MemFree(rsp);
    EXPECT_EQ(g_lastEventResult, 0x0E);
    EXPECT_EQ(device->ssapState, (uint8_t)ACTM_SSAP_IDLE);
}

// 未知应用标识或未知操作码的响应被安全忽略
TEST_F(ActmControlTest, UT_ACTM_CTL_017)
{
    ActmRemoteDevice_S *device = CreateGroupedDevice(&g_addrA, TEST_GID, TEST_APP_A);
    device->ssapState = ACTM_START_CONFIG;
    ActmControlRsp_S *unknownApp = MakeControlRsp(0x7777, ACTM_CONTROL_CONFIG, 1, 0);
    ActmControlStreamRes(unknownApp);
    SDF_MemFree(unknownApp);
    ActmControlRsp_S *unknownOpcode = MakeControlRsp(TEST_APP_A, 0x7F, 1, 0);
    ActmControlStreamRes(unknownOpcode);
    SDF_MemFree(unknownOpcode);
    EXPECT_EQ(device->ssapState, (uint8_t)ACTM_START_CONFIG);
    EXPECT_EQ(g_eventCount, 0);
}

// 属性上报按访问点逐点填充，缺属性组的点跳过；登记了位置句柄时附加位置通知
TEST_F(ActmControlTest, UT_ACTM_CTL_018)
{
    ActmRemoteDevice_S *device = CreateGroupedDevice(&g_addrA, TEST_GID, TEST_APP_A);
    PushPoint(device, 2, 9, NLSTK_ACTM_SOURCE_POINT);
    device->info.locationHandle = 0x13;
    device->isLeft = true;
    NotifyAudioProp(device);
    EXPECT_EQ(g_propCount, 1);
    EXPECT_EQ(g_propNum, 2);
    EXPECT_EQ(g_locationCount, 1);
    EXPECT_TRUE(g_lastIsLeft);
    EXPECT_NE(ActmFindQosmGroupById(device->configGid), nullptr);
}

// 双耳可用流类型不一致时不上报，一致后上报一次
TEST_F(ActmControlTest, UT_ACTM_CTL_019)
{
    ActmRemoteDevice_S *deviceA = CreateGroupedDevice(&g_addrA, TEST_GID, TEST_APP_A);
    ActmRemoteDevice_S *deviceB = CreateGroupedDevice(&g_addrB, TEST_GID, TEST_APP_B);
    deviceA->availableStreamType = 0x0F;
    deviceB->availableStreamType = 0x01;
    ActmReportAvailableStreamType(deviceA, 0x0F);
    EXPECT_EQ(g_streamTypeCount, 0);
    deviceB->availableStreamType = 0x0F;
    ActmReportAvailableStreamType(deviceA, 0x0F);
    EXPECT_EQ(g_streamTypeCount, 1);
    EXPECT_EQ(g_lastStreamType, 0x0F);
}

// 配置访问点报文按透传方式与厂商能力开关组织：透传走端口短报文，非透传带服务质量参数
TEST_F(ActmControlTest, UT_ACTM_CTL_020)
{
    ActmRemoteDevice_S *device = CreateGroupedDevice(&g_addrA, TEST_GID, TEST_APP_A);
    ActmAccessPoint_S *point = ActmFindPointById(device, 1);
    ASSERT_NE(point, nullptr);
    ActmStream_S *stream = ActmCreateStream(device, NLSTK_ACTM_SINK_POINT, NLSTK_ACTM_UNICAST);
    ASSERT_NE(stream, nullptr);
    device->curStreamId = stream->streamId;
    NLSTK_ActmConfig_S config = {0};
    config.codec.codecId = L2HC_CODEC_NORMAL;
    config.channel.qosId = QOSINDEX_3;
    config.channel.trans = 0;
    ActmPointControlInfo_S info = {0};
    ActmConfigPoint(device, stream, point, &config, &info);
    uint8_t transparentLen = info.dataLen;
    SDF_MemFree(info.data);
    EXPECT_GT(transparentLen, 0);
    config.channel.trans = 1;
    SetManufacturerQosIdAbility(&g_addrA, false);
    ActmConfigPoint(device, stream, point, &config, &info);
    uint8_t withoutAbilityLen = info.dataLen;
    SDF_MemFree(info.data);
    SetManufacturerQosIdAbility(&g_addrA, true);
    ActmConfigPoint(device, stream, point, &config, &info);
    uint8_t withAbilityLen = info.dataLen;
    SDF_MemFree(info.data);
    EXPECT_NE(withoutAbilityLen, withAbilityLen);
    EXPECT_EQ(stream->qosIndex, (uint8_t)QOSINDEX_3);
    EXPECT_EQ(stream->codec.codecId, L2HC_CODEC_NORMAL);
}

// 源访问点只有高保真录音流才保存流配置，普通流不覆盖
TEST_F(ActmControlTest, UT_ACTM_CTL_021)
{
    ActmRemoteDevice_S *device = CreateGroupedDevice(&g_addrA, TEST_GID, TEST_APP_A);
    PushPoint(device, 2, 9, NLSTK_ACTM_SOURCE_POINT);
    ActmAccessPoint_S *sourcePoint = ActmFindPointById(device, 2);
    ASSERT_NE(sourcePoint, nullptr);
    ActmStream_S *stream = ActmCreateStream(device, NLSTK_ACTM_SOURCE_POINT, NLSTK_ACTM_UNICAST);
    ASSERT_NE(stream, nullptr);
    device->curStreamId = stream->streamId;
    NLSTK_ActmConfig_S config = {0};
    config.codec.codecId = L2HC_CODEC_NORMAL;
    config.channel.qosId = QOSINDEX_3;
    config.channel.trans = 1;
    ActmPointControlInfo_S info = {0};
    ActmConfigPoint(device, stream, sourcePoint, &config, &info);
    SDF_MemFree(info.data);
    EXPECT_EQ(stream->qosIndex, 0);
    config.channel.qosId = QOSINDEX_5;
    ActmConfigPoint(device, stream, sourcePoint, &config, &info);
    SDF_MemFree(info.data);
    EXPECT_EQ(stream->qosIndex, (uint8_t)QOSINDEX_5);
}

// 更新码率请求把新码率写回接收方向流的配置
TEST_F(ActmControlTest, UT_ACTM_CTL_022)
{
    ActmRemoteDevice_S *device = CreateGroupedDevice(&g_addrA, TEST_GID, TEST_APP_A);
    ActmStream_S *stream = ActmCreateStream(device, NLSTK_ACTM_SINK_POINT, NLSTK_ACTM_UNICAST);
    ASSERT_NE(stream, nullptr);
    device->curStreamId = stream->streamId;
    ActmChangeBitrate(device, stream, 0x5);
    EXPECT_EQ(stream->codec.l2hc.bpsRange, 0x5);
}

// 设备下线带移除标记时从组内删除该设备地址
TEST_F(ActmControlTest, UT_ACTM_CTL_023)
{
    ActmRemoteDevice_S *device = CreateGroupedDevice(&g_addrA, TEST_GID, TEST_APP_A);
    ActmQosmGroup_S *group = ActmFindQosmGroupById(TEST_GID);
    ASSERT_NE(group, nullptr);
    size_t countBefore = group->devices->size;
    ActmDeviceOffline(device, true);
    EXPECT_EQ(device->ssapState, (uint8_t)ACTM_SSAP_IDLE);
    EXPECT_EQ(group->devices->size, countBefore - 1);
}

// 属性读取在服务未就绪时直接返回，不触发任何协议交互
TEST_F(ActmControlTest, UT_ACTM_CTL_024)
{
    ActmRemoteDevice_S *device = CreateGroupedDevice(&g_addrA, TEST_GID, TEST_APP_A);
    ActmReadProp(device);
    EXPECT_EQ(g_propCount, 0);
}

// 上层未注册回调时控制响应流程静默完成不崩溃
TEST_F(ActmControlTest, UT_ACTM_CTL_025)
{
    NLSTK_ActmCbk_S zeroCbk = {0};
    NLSTK_ActmRegisterCallback(&zeroCbk);
    ActmRemoteDevice_S *device = CreateGroupedDevice(&g_addrA, TEST_GID, TEST_APP_A);
    device->ssapState = ACTM_START_CONFIG;
    ActmControlRsp_S *rsp = MakeControlRsp(TEST_APP_A, ACTM_CONTROL_CONFIG, 1, 0);
    ActmControlStreamRes(rsp);
    SDF_MemFree(rsp);
    EXPECT_EQ(device->ssapState, (uint8_t)ACTM_CONFIG_COMPL);
    EXPECT_EQ(g_eventCount, 0);
}
