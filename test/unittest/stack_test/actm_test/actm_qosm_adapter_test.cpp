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
#include "dli_event_struct.h"
#include "dli_cmd_struct.h"
#include "dli_opcode.h"
#include "ssap_link.h"
#include "ssap_manager.h"
#include "cdsm.h"

#include "actm_api.h"
#include "actm.h"
#include "actm_tbl.h"
#include "actm_callback.h"
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

const uint16_t TEST_GID = 6;
const uint16_t TEST_LCID = 3;
const uint16_t IMG_GHANDLE = 0xa4;

int g_eventCount = 0;
uint8_t g_lastEvent = 0xFF;
uint8_t g_lastEventResult = 0xFF;
NLSTK_ActmQosmInfo_S g_lastTransInfo = {0};
int g_bitCount = 0;
NLSTK_ActmBitrateChange_S g_lastBitrate = {0};
int g_upDownCount = 0;
NLSTK_ActmAutoRateSendMsg_S g_lastUpdown = {0};
int g_directionCount = 0;
uint8_t g_lastDirectionResult = 0xFF;

void ResetCaptures()
{
    g_eventCount = 0;
    g_lastEvent = 0xFF;
    g_lastEventResult = 0xFF;
    memset_s(&g_lastTransInfo, sizeof(g_lastTransInfo), 0, sizeof(g_lastTransInfo));
    g_bitCount = 0;
    memset_s(&g_lastBitrate, sizeof(g_lastBitrate), 0, sizeof(g_lastBitrate));
    g_upDownCount = 0;
    memset_s(&g_lastUpdown, sizeof(g_lastUpdown), 0, sizeof(g_lastUpdown));
    g_directionCount = 0;
    g_lastDirectionResult = 0xFF;
}

void StubEventCbk(SLE_Addr_S *addr, uint8_t eventType, uint8_t result, void *param)
{
    g_eventCount++;
    g_lastEvent = eventType;
    g_lastEventResult = result;
    if (eventType == NLSTK_ACTM_EVENT_TRANS && result == NLSTK_ACTM_SUCCESS && param != NULL) {
        (void)memcpy_s(&g_lastTransInfo, sizeof(g_lastTransInfo), param, sizeof(g_lastTransInfo));
    }
}

void StubPropCbk(SLE_Addr_S *addr, uint8_t num, NLSTK_ActmProp_S *prop) {}

void StubBitCbk(NLSTK_ActmBitrateChange_S *bitrate)
{
    g_bitCount++;
    (void)memcpy_s(&g_lastBitrate, sizeof(g_lastBitrate), bitrate, sizeof(g_lastBitrate));
}

void StubLocationCbk(SLE_Addr_S *addr, bool isLeft) {}

void StubStreamTypeCbk(SLE_Addr_S *addr, uint32_t availableStreamType) {}

void StubUpdownCbk(NLSTK_ActmAutoRateSendMsg_S *upDownParam)
{
    g_upDownCount++;
    (void)memcpy_s(&g_lastUpdown, sizeof(g_lastUpdown), upDownParam, sizeof(g_lastUpdown));
}

void StubDirectionCbk(SLE_Addr_S *addr, NLSTK_ActmSetDirection_S *param)
{
    g_directionCount++;
    g_lastDirectionResult = param->result;
}

void PushPoint(ActmRemoteDevice_S *device, uint8_t pointId, uint8_t propId, uint8_t type)
{
    ActmAccessPoint_S *point = (ActmAccessPoint_S *)SDF_MemZalloc(sizeof(ActmAccessPoint_S));
    ASSERT_NE(point, nullptr);
    point->pointId = pointId;
    point->propId = propId;
    point->type = type;
    SDF_VectorEmplaceBack(device->points, point);
}

void PushProp(ActmRemoteDevice_S *device, uint8_t propId, uint8_t propType)
{
    ActmProp_S *prop = (ActmProp_S *)SDF_MemZalloc(sizeof(ActmProp_S));
    ASSERT_NE(prop, nullptr);
    prop->propId = propId;
    prop->propType = propType;
    prop->ability.comm = 0x01;
    SDF_VectorEmplaceBack(device->props, prop);
}

ActmRemoteDevice_S *CreateGroupedDevice(const SLE_Addr_S *addr, uint16_t gid)
{
    SLE_Addr_S tmp = *addr;
    EXPECT_EQ(ActmCreateRemoteDevice(&tmp), (uint32_t)NLSTK_OK);
    ActmRemoteDevice_S *device = ActmFindDeviceByAddr(&tmp);
    EXPECT_NE(device, nullptr);
    PushPoint(device, 1, 1, NLSTK_ACTM_SINK_POINT);
    PushProp(device, 1, NLSTK_ACTM_SINK_POINT);
    AddDeviceToGroup(device, gid);
    return device;
}

ActmStream_S *CreateDeviceStream(ActmRemoteDevice_S *device, uint8_t pointType)
{
    ActmStream_S *stream = ActmCreateStream(device, pointType, NLSTK_ACTM_UNICAST);
    EXPECT_NE(stream, nullptr);
    device->curStreamId = stream->streamId;
    stream->qosIndex = QOSINDEX_3;
    stream->codec.codecId = L2HC_CODEC_NORMAL;
    return stream;
}

void ArmGroupConnected(ActmRemoteDevice_S *device, ActmStream_S *stream)
{
    ActmSetQosmParam(device, stream);
    TEST_QOSM_NotifyParam(TEST_GID, QOSM_PARAM_SETTED, false, QOSM_SUCCESS);
    TEST_QOSM_NotifyConnection(TEST_GID, QOSM_CONNECTION_ADDED, QOSM_SUCCESS);
}

void NotifyImgEncrypted(uint16_t gHandle, uint8_t status)
{
    DLI_EncryptChangeEvt evt = {0};
    evt.status = status;
    evt.connHandle = gHandle;
    DLI_ExecuteCmdRetParam cmdRes = {0};
    cmdRes.eventParameter = &evt;
    TEST_DLI_EventCbk(DLI_CBK_ENCRYPT_CHANGE, 0, &cmdRes);
}
} // namespace

class ActmQosmAdapterTest : public testing::Test {
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
        EXPECT_CALL(cmMock, CM_GetLogicLinkByAddr).WillRepeatedly(Invoke([](SLE_Addr_S *addr,
            CM_LogicLink_S *logicLink) {
            logicLink->lcid = TEST_LCID;
            return (uint32_t)0;
        }));
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

// 建组后可按事件组号与组句柄查到，重复建组保持一份，未知编号查不到
TEST_F(ActmQosmAdapterTest, UT_ACTM_QOSM_001)
{
    EXPECT_EQ(ActmCreateQosmGroup(TEST_GID, 2), (uint32_t)NLSTK_OK);
    EXPECT_EQ(ActmCreateQosmGroup(TEST_GID, 2), (uint32_t)NLSTK_OK);
    ActmQosmGroup_S *group = ActmFindQosmGroupById(TEST_GID);
    ASSERT_NE(group, nullptr);
    EXPECT_EQ(group->icgId, TEST_GID);
    EXPECT_EQ(group->state, (uint8_t)ACTM_QOSM_PARAM_DELETE);
    EXPECT_EQ(group->linkCnt, 2);
    EXPECT_EQ(ActmFindQosmGroupById(0x77), nullptr);
    EXPECT_EQ(ActmFindQosmGroupByHandle(IMG_GHANDLE), nullptr);
}

// 未占用的链路可分配句柄并记录地址，占满后返回无效句柄；按地址与句柄均可查回
TEST_F(ActmQosmAdapterTest, UT_ACTM_QOSM_002)
{
    ActmCreateQosmGroup(TEST_GID, 2);
    uint16_t handleA = ActmAllocConnHandle(&g_addrA, TEST_GID);
    uint16_t handleB = ActmAllocConnHandle(&g_addrB, TEST_GID);
    EXPECT_NE(handleA, QOSM_INVALID_HANDLE);
    EXPECT_NE(handleB, QOSM_INVALID_HANDLE);
    EXPECT_EQ(ActmGetConnHandle(&g_addrA, TEST_GID), handleA);
    EXPECT_EQ(ActmGetConnHandle(&g_addrB, TEST_GID), handleB);
    EXPECT_EQ(ActmAllocConnHandle(&g_addrA, TEST_GID), QOSM_INVALID_HANDLE);
    ActmQosmLink_S *linkA = ActmFindQosmLinkByHandle(TEST_GID, handleA);
    ASSERT_NE(linkA, nullptr);
    EXPECT_EQ(linkA->state, (uint8_t)ACTM_QOSM_DISCONNECTED);
    EXPECT_EQ(ActmFindQosmLinkByAddr(TEST_GID, &g_addrA)->connHandle, handleA);
    EXPECT_EQ(ActmFindQosmLinkByHandle(TEST_GID, 0x99), nullptr);
}

// 同一地址重复入组只记一份，空地址不入组
TEST_F(ActmQosmAdapterTest, UT_ACTM_QOSM_003)
{
    ActmCreateQosmGroup(TEST_GID, 2);
    ActmQosmAddDevice(TEST_GID, &g_addrA);
    ActmQosmAddDevice(TEST_GID, &g_addrA);
    ActmQosmGroup_S *group = ActmFindQosmGroupById(TEST_GID);
    EXPECT_EQ(group->devices->size, (size_t)1);
    ActmQosmAddDevice(TEST_GID, nullptr);
    EXPECT_EQ(group->devices->size, (size_t)1);
    EXPECT_EQ(ActmFindQosmLinkByAddr(TEST_GID, &g_addrA), nullptr);
}

// 首个设备设参成功后进入设参中，QOSM 收到的参数与流配置一致
TEST_F(ActmQosmAdapterTest, UT_ACTM_QOSM_004)
{
    ActmRemoteDevice_S *device = CreateGroupedDevice(&g_addrA, TEST_GID);
    ActmStream_S *stream = CreateDeviceStream(device, NLSTK_ACTM_SINK_POINT);
    stream->codec.l2hc.bpsConf = 0x2;
    QOSM_AutoRateParam captured = {0};
    EXPECT_CALL(qosmMock, QOSM_AutoRateSetAutorateParam(_))
        .WillOnce(DoAll(SaveArgPointee<0>(&captured), Return(QOSM_SUCCESS)));
    ActmSetQosmParam(device, stream);
    EXPECT_EQ(device->qosmState, (uint8_t)ACTM_SET_PARAM);
    EXPECT_EQ(ActmFindQosmGroupById(TEST_GID)->state, (uint8_t)ACTM_QOSM_PARAM_ADDING);
    EXPECT_EQ(captured.qosId, (uint8_t)TEST_GID);
    EXPECT_EQ(captured.qosIndex, (QOSM_QosIndex)QOSINDEX_3);
    EXPECT_EQ(captured.linkCnt, 2);
    EXPECT_EQ(captured.lcidCnt, 1);
    EXPECT_EQ(captured.lcid[0], TEST_LCID);
}

// 组内已有设备参数就绪时，新设备不再重复向 QOSM 设参而直接进入就绪并触发开通道
TEST_F(ActmQosmAdapterTest, UT_ACTM_QOSM_005)
{
    ActmRemoteDevice_S *deviceA = CreateGroupedDevice(&g_addrA, TEST_GID);
    ActmStream_S *streamA = CreateDeviceStream(deviceA, NLSTK_ACTM_SINK_POINT);
    ActmRemoteDevice_S *deviceB = CreateGroupedDevice(&g_addrB, TEST_GID);
    ActmStream_S *streamB = CreateDeviceStream(deviceB, NLSTK_ACTM_SINK_POINT);
    EXPECT_CALL(qosmMock, QOSM_AutoRateSetAutorateParam(_)).WillOnce(Return(QOSM_SUCCESS));
    EXPECT_CALL(qosmMock, QOSM_AutoRateAddConnection(_)).WillRepeatedly(Return(QOSM_SUCCESS));
    ActmSetQosmParam(deviceA, streamA);
    TEST_QOSM_NotifyParam(TEST_GID, QOSM_PARAM_SETTED, false, QOSM_SUCCESS);
    EXPECT_CALL(qosmMock, QOSM_AutoRateSetAutorateParam(_)).Times(0);
    ActmSetQosmParam(deviceB, streamB);
    EXPECT_EQ(deviceA->qosmState, (uint8_t)ACTM_ADD_CONN);
    EXPECT_EQ(deviceB->qosmState, (uint8_t)ACTM_ADD_CONN);
    EXPECT_EQ(ActmFindQosmLinkByAddr(TEST_GID, &g_addrA)->connHandle, (uint16_t)0xa5);
    EXPECT_EQ(deviceB->connHandle, (uint16_t)0xa6);
    EXPECT_EQ(ActmGetConnHandle(&g_addrB, TEST_GID), (uint16_t)0xa6);
    EXPECT_EQ(ActmFindQosmLinkByAddr(TEST_GID, &g_addrB)->state, (uint8_t)ACTM_QOSM_CONNECTING);
}

// 组内其他设备正在设参时，新设备等待，不向 QOSM 发请求
TEST_F(ActmQosmAdapterTest, UT_ACTM_QOSM_006)
{
    ActmRemoteDevice_S *deviceA = CreateGroupedDevice(&g_addrA, TEST_GID);
    ActmRemoteDevice_S *deviceB = CreateGroupedDevice(&g_addrB, TEST_GID);
    ActmStream_S *streamB = CreateDeviceStream(deviceB, NLSTK_ACTM_SINK_POINT);
    deviceA->qosmState = ACTM_SET_PARAM;
    EXPECT_CALL(qosmMock, QOSM_AutoRateSetAutorateParam(_)).Times(0);
    ActmSetQosmParam(deviceB, streamB);
    EXPECT_EQ(deviceB->qosmState, (uint8_t)ACTM_QOSM_IDLE);
}

// QOSM 设参失败时设备回到空闲且不上报事件
TEST_F(ActmQosmAdapterTest, UT_ACTM_QOSM_007)
{
    ActmRemoteDevice_S *device = CreateGroupedDevice(&g_addrA, TEST_GID);
    ActmStream_S *stream = CreateDeviceStream(device, NLSTK_ACTM_SINK_POINT);
    EXPECT_CALL(qosmMock, QOSM_AutoRateSetAutorateParam(_)).WillOnce(Return(QOSM_FAIL));
    ActmSetQosmParam(device, stream);
    EXPECT_EQ(device->qosmState, (uint8_t)ACTM_QOSM_IDLE);
    EXPECT_EQ(device->ssapState, (uint8_t)ACTM_SSAP_IDLE);
    EXPECT_EQ(g_eventCount, 0);
}

// 参数设置成功的回调让组进入就绪、设备进入加连接，链路句柄回填为回调给定值
TEST_F(ActmQosmAdapterTest, UT_ACTM_QOSM_008)
{
    ActmRemoteDevice_S *device = CreateGroupedDevice(&g_addrA, TEST_GID);
    ActmStream_S *stream = CreateDeviceStream(device, NLSTK_ACTM_SINK_POINT);
    EXPECT_CALL(qosmMock, QOSM_AutoRateSetAutorateParam(_)).WillOnce(Return(QOSM_SUCCESS));
    EXPECT_CALL(qosmMock, QOSM_AutoRateAddConnection(_)).WillOnce(Return(QOSM_SUCCESS));
    ActmSetQosmParam(device, stream);
    TEST_QOSM_NotifyParam(TEST_GID, QOSM_PARAM_SETTED, false, QOSM_SUCCESS);
    ActmQosmGroup_S *group = ActmFindQosmGroupById(TEST_GID);
    EXPECT_EQ(group->state, (uint8_t)ACTM_QOSM_PARAM_ADD);
    EXPECT_EQ(device->qosmState, (uint8_t)ACTM_ADD_CONN);
    ActmQosmLink_S *link = ActmFindQosmLinkByAddr(TEST_GID, &g_addrA);
    EXPECT_EQ(link->connHandle, (uint16_t)0xa5);
    EXPECT_EQ(link->state, (uint8_t)ACTM_QOSM_CONNECTING);
}

// 参数回调带失败结果时组回到删除态，设参中的设备被清回空闲
TEST_F(ActmQosmAdapterTest, UT_ACTM_QOSM_009)
{
    ActmRemoteDevice_S *device = CreateGroupedDevice(&g_addrA, TEST_GID);
    ActmStream_S *stream = CreateDeviceStream(device, NLSTK_ACTM_SINK_POINT);
    EXPECT_CALL(qosmMock, QOSM_AutoRateSetAutorateParam(_)).WillOnce(Return(QOSM_SUCCESS));
    ActmSetQosmParam(device, stream);
    TEST_QOSM_NotifyParam(TEST_GID, QOSM_PARAM_SETTED, false, QOSM_FAIL);
    EXPECT_EQ(ActmFindQosmGroupById(TEST_GID)->state, (uint8_t)ACTM_QOSM_PARAM_DELETE);
    EXPECT_EQ(device->qosmState, (uint8_t)ACTM_QOSM_IDLE);
}

// 回调链路数与组链路数不一致时按失败处理，组回到删除态
TEST_F(ActmQosmAdapterTest, UT_ACTM_QOSM_010)
{
    ActmCreateQosmGroup(TEST_GID, 1);
    ActmQosmAddDevice(TEST_GID, &g_addrA);
    TEST_QOSM_NotifyParam(TEST_GID, QOSM_PARAM_SETTED, false, QOSM_SUCCESS);
    EXPECT_EQ(ActmFindQosmGroupById(TEST_GID)->state, (uint8_t)ACTM_QOSM_PARAM_DELETE);
}

// 参数移除成功的回调清空组句柄与全部链路
TEST_F(ActmQosmAdapterTest, UT_ACTM_QOSM_011)
{
    ActmRemoteDevice_S *device = CreateGroupedDevice(&g_addrA, TEST_GID);
    ActmStream_S *stream = CreateDeviceStream(device, NLSTK_ACTM_SINK_POINT);
    EXPECT_CALL(qosmMock, QOSM_AutoRateSetAutorateParam(_)).WillOnce(Return(QOSM_SUCCESS));
    EXPECT_CALL(qosmMock, QOSM_AutoRateAddConnection(_)).WillOnce(Return(QOSM_SUCCESS));
    ActmSetQosmParam(device, stream);
    TEST_QOSM_NotifyParam(TEST_GID, QOSM_PARAM_SETTED, false, QOSM_SUCCESS);
    ActmQosmLink_S *link = ActmFindQosmLinkByAddr(TEST_GID, &g_addrA);
    ASSERT_NE(link, nullptr);
    TEST_QOSM_NotifyParam(TEST_GID, QOSM_PARAM_REMOVED, false, QOSM_SUCCESS);
    ActmQosmGroup_S *group = ActmFindQosmGroupById(TEST_GID);
    EXPECT_EQ(group->state, (uint8_t)ACTM_QOSM_PARAM_DELETE);
    EXPECT_EQ(group->gHandle, (uint16_t)QOSM_INVALID_HANDLE);
    EXPECT_FALSE(group->isImg);
    EXPECT_EQ(link->used, false);
    EXPECT_EQ(link->state, (uint8_t)ACTM_QOSM_DISCONNECTED);
}

// 参数移除回调带失败结果时组状态保持不变
TEST_F(ActmQosmAdapterTest, UT_ACTM_QOSM_012)
{
    ActmCreateQosmGroup(TEST_GID, 2);
    ActmFindQosmGroupById(TEST_GID)->state = ACTM_QOSM_PARAM_ADD;
    TEST_QOSM_NotifyParam(TEST_GID, QOSM_PARAM_REMOVED, false, QOSM_FAIL);
    EXPECT_EQ(ActmFindQosmGroupById(TEST_GID)->state, (uint8_t)ACTM_QOSM_PARAM_ADD);
}

// 加密组参数回调记录组句柄且组停留在设参中，等待加密结果
TEST_F(ActmQosmAdapterTest, UT_ACTM_QOSM_013)
{
    ActmRemoteDevice_S *device = CreateGroupedDevice(&g_addrA, TEST_GID);
    ActmStream_S *stream = CreateDeviceStream(device, NLSTK_ACTM_SINK_POINT);
    EXPECT_CALL(qosmMock, QOSM_AutoRateSetAutorateParam(_)).WillOnce(Return(QOSM_SUCCESS));
    ActmSetQosmParam(device, stream);
    TEST_QOSM_NotifyParam(TEST_GID, QOSM_PARAM_SETTED, true, QOSM_SUCCESS);
    ActmQosmGroup_S *group = ActmFindQosmGroupById(TEST_GID);
    EXPECT_EQ(group->state, (uint8_t)ACTM_QOSM_PARAM_ADDING);
    EXPECT_EQ(group->gHandle, IMG_GHANDLE);
    EXPECT_TRUE(group->isImg);
    EXPECT_EQ(device->qosmState, (uint8_t)ACTM_SET_PARAM);
}

// 加密成功事件让组进入就绪并继续开通道流程
TEST_F(ActmQosmAdapterTest, UT_ACTM_QOSM_014)
{
    ActmRemoteDevice_S *device = CreateGroupedDevice(&g_addrA, TEST_GID);
    ActmStream_S *stream = CreateDeviceStream(device, NLSTK_ACTM_SINK_POINT);
    EXPECT_CALL(qosmMock, QOSM_AutoRateSetAutorateParam(_)).WillOnce(Return(QOSM_SUCCESS));
    EXPECT_CALL(qosmMock, QOSM_AutoRateAddConnection(_)).WillOnce(Return(QOSM_SUCCESS));
    ActmSetQosmParam(device, stream);
    TEST_QOSM_NotifyParam(TEST_GID, QOSM_PARAM_SETTED, true, QOSM_SUCCESS);
    NotifyImgEncrypted(IMG_GHANDLE, 0);
    EXPECT_EQ(ActmFindQosmGroupById(TEST_GID)->state, (uint8_t)ACTM_QOSM_PARAM_ADD);
    EXPECT_EQ(device->qosmState, (uint8_t)ACTM_ADD_CONN);
}

// 加密失败事件按设参失败处理，设备回空闲
TEST_F(ActmQosmAdapterTest, UT_ACTM_QOSM_015)
{
    ActmRemoteDevice_S *device = CreateGroupedDevice(&g_addrA, TEST_GID);
    ActmStream_S *stream = CreateDeviceStream(device, NLSTK_ACTM_SINK_POINT);
    EXPECT_CALL(qosmMock, QOSM_AutoRateSetAutorateParam(_)).WillOnce(Return(QOSM_SUCCESS));
    ActmSetQosmParam(device, stream);
    TEST_QOSM_NotifyParam(TEST_GID, QOSM_PARAM_SETTED, true, QOSM_SUCCESS);
    NotifyImgEncrypted(IMG_GHANDLE, 1);
    EXPECT_EQ(ActmFindQosmGroupById(TEST_GID)->state, (uint8_t)ACTM_QOSM_PARAM_ADD);
    EXPECT_EQ(device->qosmState, (uint8_t)ACTM_QOSM_IDLE);
    EXPECT_EQ(device->ssapState, (uint8_t)ACTM_SSAP_IDLE);
}

// 连接建立成功后链路进入已连接，开始播报事件携带流的编解码信息与链路计数
TEST_F(ActmQosmAdapterTest, UT_ACTM_QOSM_016)
{
    ActmRemoteDevice_S *device = CreateGroupedDevice(&g_addrA, TEST_GID);
    ActmStream_S *stream = CreateDeviceStream(device, NLSTK_ACTM_SINK_POINT);
    ArmGroupConnected(device, stream);
    ActmQosmLink_S *link = ActmFindQosmLinkByAddr(TEST_GID, &g_addrA);
    EXPECT_EQ(link->state, (uint8_t)ACTM_QOSM_CONNECTED);
    EXPECT_EQ(device->qosmState, (uint8_t)ACTM_CONN_COMPL);
    EXPECT_EQ(g_lastEvent, (uint8_t)NLSTK_ACTM_EVENT_TRANS);
    EXPECT_EQ(g_lastEventResult, (uint8_t)NLSTK_ACTM_SUCCESS);
    EXPECT_EQ(g_lastTransInfo.linkCnt, 1);
    EXPECT_EQ(g_lastTransInfo.codecId, L2HC_CODEC_NORMAL);
    EXPECT_EQ(device->ssapState, (uint8_t)ACTM_START_PLAY_COMPL);
}

// 高保真录音流走终端到网关方向的参数获取路径，同样完成播报
TEST_F(ActmQosmAdapterTest, UT_ACTM_QOSM_017)
{
    ActmRemoteDevice_S *device = CreateGroupedDevice(&g_addrA, TEST_GID);
    ActmStream_S *stream = CreateDeviceStream(device, NLSTK_ACTM_SINK_POINT);
    stream->qosIndex = QOSINDEX_5;
    ArmGroupConnected(device, stream);
    EXPECT_EQ(g_lastEvent, (uint8_t)NLSTK_ACTM_EVENT_TRANS);
    EXPECT_EQ(device->ssapState, (uint8_t)ACTM_START_PLAY_COMPL);
}

// 连接失败时已进入传输完成的设备收到传输失败事件并回空闲
TEST_F(ActmQosmAdapterTest, UT_ACTM_QOSM_018)
{
    ActmRemoteDevice_S *device = CreateGroupedDevice(&g_addrA, TEST_GID);
    ActmStream_S *stream = CreateDeviceStream(device, NLSTK_ACTM_SINK_POINT);
    EXPECT_CALL(qosmMock, QOSM_AutoRateSetAutorateParam(_)).WillOnce(Return(QOSM_SUCCESS));
    EXPECT_CALL(qosmMock, QOSM_AutoRateAddConnection(_)).WillOnce(Return(QOSM_SUCCESS));
    ActmSetQosmParam(device, stream);
    TEST_QOSM_NotifyParam(TEST_GID, QOSM_PARAM_SETTED, false, QOSM_SUCCESS);
    device->ssapState = ACTM_TRANS_COMPL;
    TEST_QOSM_NotifyConnection(TEST_GID, QOSM_CONNECTION_ADDED, QOSM_FAIL);
    EXPECT_EQ(g_lastEvent, (uint8_t)NLSTK_ACTM_EVENT_TRANS);
    EXPECT_EQ(g_lastEventResult, (uint8_t)NLSTK_ACTM_QOSM_ERROR);
    EXPECT_EQ(device->qosmState, (uint8_t)ACTM_QOSM_IDLE);
    EXPECT_EQ(ActmFindQosmLinkByAddr(TEST_GID, &g_addrA), nullptr);
}

// 连接失败时已开始播放的设备收到释放事件，链路被回收
TEST_F(ActmQosmAdapterTest, UT_ACTM_QOSM_019)
{
    ActmRemoteDevice_S *device = CreateGroupedDevice(&g_addrA, TEST_GID);
    ActmStream_S *stream = CreateDeviceStream(device, NLSTK_ACTM_SINK_POINT);
    ArmGroupConnected(device, stream);
    ResetCaptures();
    TEST_QOSM_NotifyConnection(TEST_GID, QOSM_CONNECTION_ADDED, QOSM_FAIL);
    EXPECT_EQ(g_lastEvent, (uint8_t)NLSTK_ACTM_EVENT_RELEASE);
    EXPECT_EQ(device->ssapState, (uint8_t)ACTM_SSAP_IDLE);
    EXPECT_EQ(device->qosmState, (uint8_t)ACTM_QOSM_FAIL);
}

// 连接删除时按设备所处状态回收：空闲回空闲、传输中记失败、删连接完成后发释放事件
TEST_F(ActmQosmAdapterTest, UT_ACTM_QOSM_020)
{
    ActmRemoteDevice_S *device = CreateGroupedDevice(&g_addrA, TEST_GID);
    ActmStream_S *stream = CreateDeviceStream(device, NLSTK_ACTM_SINK_POINT);
    ArmGroupConnected(device, stream);
    device->qosmState = ACTM_DEL_CONN;
    device->ssapState = ACTM_RELEASE_COMPL;
    ResetCaptures();
    TEST_QOSM_NotifyConnection(TEST_GID, QOSM_CONNECTION_DELETED, QOSM_SUCCESS);
    EXPECT_EQ(device->qosmState, (uint8_t)ACTM_QOSM_IDLE);
    EXPECT_EQ(device->ssapState, (uint8_t)ACTM_SSAP_IDLE);
    EXPECT_EQ(g_lastEvent, (uint8_t)NLSTK_ACTM_EVENT_RELEASE);
    EXPECT_EQ(device->connHandle, 0);
    EXPECT_EQ(ActmFindQosmLinkByAddr(TEST_GID, &g_addrA), nullptr);
}

// 未知组或未知句柄的连接回调不产生任何副作用
TEST_F(ActmQosmAdapterTest, UT_ACTM_QOSM_021)
{
    TEST_QOSM_NotifyConnection(0x77, QOSM_CONNECTION_ADDED, QOSM_SUCCESS);
    ActmCreateQosmGroup(TEST_GID, 2);
    TEST_QOSM_NotifyConnection(TEST_GID, QOSM_CONNECTION_ADDED, QOSM_SUCCESS);
    EXPECT_EQ(g_eventCount, 0);
    EXPECT_EQ(ActmFindQosmGroupById(TEST_GID)->state, (uint8_t)ACTM_QOSM_PARAM_DELETE);
}

// 单播流设置上下行数据通路，成功回调后方向位生效并上报设置结果
TEST_F(ActmQosmAdapterTest, UT_ACTM_QOSM_022)
{
    ActmRemoteDevice_S *device = CreateGroupedDevice(&g_addrA, TEST_GID);
    ActmStream_S *stream = CreateDeviceStream(device, NLSTK_ACTM_SINK_POINT);
    ArmGroupConnected(device, stream);
    ActmQosmLink_S *link = ActmFindQosmLinkByAddr(TEST_GID, &g_addrA);
    QOSM_AutoRateDataPath captured = {0};
    EXPECT_CALL(qosmMock, QOSM_AutoRateAddDataPath(_))
        .Times(2)
        .WillRepeatedly(DoAll(SaveArgPointee<0>(&captured), Return(QOSM_SUCCESS)));
    ActmSetDataPath(device, NLSTK_ACTM_DIRECTION_BOTH);
    EXPECT_EQ(captured.qosId, (uint8_t)TEST_GID);
    EXPECT_EQ(captured.connHandle, link->connHandle);
    ResetCaptures();
    TEST_QOSM_NotifyDataPath(TEST_GID, QOSM_DATAPATH_ADDED, QOSM_SUCCESS, QOSM_DOWN);
    EXPECT_NE(link->direction & NLSTK_ACTM_DIRECTION_DOWN, 0);
    EXPECT_EQ(g_directionCount, 1);
    EXPECT_EQ(g_lastDirectionResult, (uint8_t)NLSTK_ACTM_SUCCESS);
    TEST_QOSM_NotifyDataPath(TEST_GID, QOSM_DATAPATH_ADDED, QOSM_SUCCESS, QOSM_UP);
    EXPECT_NE(link->direction & NLSTK_ACTM_DIRECTION_UP, 0);
}

// 已是相同方向时不再向 QOSM 发通路请求
TEST_F(ActmQosmAdapterTest, UT_ACTM_QOSM_023)
{
    ActmRemoteDevice_S *device = CreateGroupedDevice(&g_addrA, TEST_GID);
    ActmStream_S *stream = CreateDeviceStream(device, NLSTK_ACTM_SINK_POINT);
    ArmGroupConnected(device, stream);
    ActmQosmLink_S *link = ActmFindQosmLinkByAddr(TEST_GID, &g_addrA);
    link->direction = NLSTK_ACTM_DIRECTION_BOTH;
    EXPECT_CALL(qosmMock, QOSM_AutoRateAddDataPath(_)).Times(0);
    EXPECT_CALL(qosmMock, QOSM_AutoRateDeleteDataPath(_)).Times(0);
    ActmSetDataPath(device, NLSTK_ACTM_DIRECTION_BOTH);
}

// 加密组播流的下行通路走组句柄，通路就绪回调逐链路生效
TEST_F(ActmQosmAdapterTest, UT_ACTM_QOSM_024)
{
    ActmRemoteDevice_S *device = CreateGroupedDevice(&g_addrA, TEST_GID);
    ActmStream_S *stream = CreateDeviceStream(device, NLSTK_ACTM_SINK_POINT);
    EXPECT_CALL(qosmMock, QOSM_AutoRateSetAutorateParam(_)).WillOnce(Return(QOSM_SUCCESS));
    EXPECT_CALL(qosmMock, QOSM_AutoRateAddConnection(_)).WillOnce(Return(QOSM_SUCCESS));
    ActmSetQosmParam(device, stream);
    TEST_QOSM_NotifyParam(TEST_GID, QOSM_PARAM_SETTED, true, QOSM_SUCCESS);
    NotifyImgEncrypted(IMG_GHANDLE, 0);
    ActmQosmGroup_S *group = ActmFindQosmGroupById(TEST_GID);
    EXPECT_EQ(group->gHandle, IMG_GHANDLE);
    EXPECT_EQ(device->qosmState, (uint8_t)ACTM_ADD_CONN);
    QOSM_AutoRateDataPath captured = {0};
    EXPECT_CALL(qosmMock, QOSM_AutoRateAddDataPath(_))
        .WillOnce(DoAll(SaveArgPointee<0>(&captured), Return(QOSM_SUCCESS)));
    ActmSetDataPath(device, NLSTK_ACTM_DIRECTION_DOWN);
    EXPECT_EQ(captured.connHandle, IMG_GHANDLE);
    EXPECT_NE(group->direction & NLSTK_ACTM_DIRECTION_DOWN, 0);
    ActmQosmLink_S *link = ActmFindQosmLinkByAddr(TEST_GID, &g_addrA);
    EXPECT_TRUE(link->needReportDataPath);
    ResetCaptures();
    TEST_QOSM_NotifyDataPathByHandle(IMG_GHANDLE, QOSM_DATAPATH_ADDED, QOSM_SUCCESS, QOSM_DOWN);
    EXPECT_EQ(g_directionCount, 0);
    link->state = ACTM_QOSM_CONNECTED;
    TEST_QOSM_NotifyDataPathByHandle(IMG_GHANDLE, QOSM_DATAPATH_ADDED, QOSM_SUCCESS, QOSM_DOWN);
    EXPECT_NE(link->direction & NLSTK_ACTM_DIRECTION_DOWN, 0);
    EXPECT_EQ(g_directionCount, 1);
    EXPECT_EQ(g_lastDirectionResult, (uint8_t)NLSTK_ACTM_SUCCESS);
}

// 通路回调的方向非法、重复添加或删除未设置方向时均被防御分支拦下
TEST_F(ActmQosmAdapterTest, UT_ACTM_QOSM_025)
{
    ActmRemoteDevice_S *device = CreateGroupedDevice(&g_addrA, TEST_GID);
    ActmStream_S *stream = CreateDeviceStream(device, NLSTK_ACTM_SINK_POINT);
    ArmGroupConnected(device, stream);
    ActmQosmLink_S *link = ActmFindQosmLinkByAddr(TEST_GID, &g_addrA);
    link->direction = 0;
    ResetCaptures();
    TEST_QOSM_NotifyDataPath(TEST_GID, QOSM_DATAPATH_DELETED, QOSM_SUCCESS, QOSM_DOWN);
    EXPECT_EQ(link->direction, 0);
    TEST_QOSM_NotifyDataPath(TEST_GID, QOSM_DATAPATH_ADDED, QOSM_FAIL, QOSM_DOWN);
    EXPECT_EQ(g_directionCount, 1);
    EXPECT_EQ(g_lastDirectionResult, (uint8_t)NLSTK_ACTM_QOSM_ERROR);
    EXPECT_FALSE(link->needReportDataPath);
    TEST_QOSM_NotifyDataPath(TEST_GID, QOSM_DATAPATH_ADDED, QOSM_SUCCESS, QOSM_UP);
    TEST_QOSM_NotifyDataPath(TEST_GID, QOSM_DATAPATH_ADDED, QOSM_SUCCESS, QOSM_UP);
    EXPECT_NE(link->direction & NLSTK_ACTM_DIRECTION_UP, 0);
}

// 组播通路操作失败时按方向回退组方向位
TEST_F(ActmQosmAdapterTest, UT_ACTM_QOSM_026)
{
    ActmCreateQosmGroup(TEST_GID, 2);
    ActmQosmGroup_S *group = ActmFindQosmGroupById(TEST_GID);
    group->gHandle = IMG_GHANDLE;
    group->direction = NLSTK_ACTM_DIRECTION_DOWN;
    TEST_QOSM_NotifyDataPathByHandle(IMG_GHANDLE, QOSM_DATAPATH_ADDED, QOSM_FAIL, QOSM_DOWN);
    EXPECT_EQ(group->direction, 0);
    TEST_QOSM_NotifyDataPathByHandle(IMG_GHANDLE, QOSM_DATAPATH_DELETED, QOSM_FAIL, QOSM_DOWN);
    EXPECT_NE(group->direction & NLSTK_ACTM_DIRECTION_DOWN, 0);
}

// 码率变化回调把 QOSM 参数逐字段映射后通知上层
TEST_F(ActmQosmAdapterTest, UT_ACTM_QOSM_027)
{
    ActmRemoteDevice_S *device = CreateGroupedDevice(&g_addrA, TEST_GID);
    ActmStream_S *stream = CreateDeviceStream(device, NLSTK_ACTM_SINK_POINT);
    ArmGroupConnected(device, stream);
    ResetCaptures();
    TEST_QOSM_NotifyBitrate(TEST_GID);
    EXPECT_EQ(g_bitCount, 1);
    EXPECT_EQ(g_lastBitrate.groupId, TEST_GID);
}

// 自动调速消息按逻辑链路回查地址，查不到的链路地址保持空
TEST_F(ActmQosmAdapterTest, UT_ACTM_QOSM_028)
{
    ActmCreateQosmGroup(TEST_GID, 2);
    CM_LogicLink_S linkInfo = {0};
    (void)memcpy_s(&linkInfo.addr, sizeof(SLE_Addr_S), &g_addrA, sizeof(SLE_Addr_S));
    EXPECT_CALL(cmMock, CM_GetLogicLinkByLcid(_, _))
        .WillOnce(DoAll(SetArgPointee<1>(linkInfo), Return(0)))
        .WillRepeatedly(Return(1));
    QOSM_AutoRateSendMsgCb msg = {0};
    msg.qosId = TEST_GID;
    msg.linkCnt = 2;
    msg.lcid[0] = TEST_LCID;
    msg.lcid[1] = TEST_LCID + 1;
    msg.labelId = 9;
    msg.msgType = CHANGE_PEER_BITRATE_REQ;
    TEST_QOSM_NotifyAutoRateMsg(&msg);
    EXPECT_EQ(g_upDownCount, 1);
    EXPECT_EQ(g_lastUpdown.linkCnt, 2);
    EXPECT_EQ(g_lastUpdown.labelId, 9);
    EXPECT_EQ(memcmp(&g_lastUpdown.addr[0], &g_addrA, sizeof(SLE_Addr_S)), 0);
    SLE_Addr_S zeroAddr = {0};
    EXPECT_EQ(memcmp(&g_lastUpdown.addr[1], &zeroAddr, sizeof(SLE_Addr_S)), 0);
}

// 更新耳机支持的码率集合时按流配置换算并提交反馈
TEST_F(ActmQosmAdapterTest, UT_ACTM_QOSM_029)
{
    ActmRemoteDevice_S *device = CreateGroupedDevice(&g_addrA, TEST_GID);
    ActmStream_S *stream = CreateDeviceStream(device, NLSTK_ACTM_SINK_POINT);
    stream->codec.codecId = L2HC_CODEC_VOICE;
    QOSM_AutoRateEarphoneFeedbackParam captured = {};
    EXPECT_CALL(qosmMock, QOSM_AutoRateSetEarphoneFeedback(_))
        .WillOnce(DoAll(SaveArgPointee<0>(&captured), Return(QOSM_SUCCESS)));
    ActmUpdateBitRate(device, 0x6);
    EXPECT_EQ(captured.supportedBitrateCnt, 2U);
    EXPECT_EQ(captured.supportedBitrate[0], 8U);
    EXPECT_EQ(captured.supportedBitrate[1], 16U);
    EXPECT_CALL(qosmMock, QOSM_AutoRateSetEarphoneFeedback(_)).WillOnce(Return(QOSM_FAIL));
    ActmUpdateBitRate(device, 0x6);
}

// 接收自动调速消息按设备所在组转发给 QOSM；设备不在任何已建组时不转发
TEST_F(ActmQosmAdapterTest, UT_ACTM_QOSM_030)
{
    ActmRemoteDevice_S *device = CreateGroupedDevice(&g_addrA, TEST_GID);
    EXPECT_EQ(TEST_QOSM_GetRecvAutoRateMsgCount(), 0U);
    ActmRecvAutoRateMsg(device, QOSINDEX_3, 1, CHANGE_PEER_BITRATE_REQ, 0);
    EXPECT_EQ(TEST_QOSM_GetRecvAutoRateMsgCount(), 1U);
    ActmRemoteDevice_S *unknown = CreateGroupedDevice(&g_addrB, TEST_GID + 1);
    unknown->groupId = 0x77;
    ActmRecvAutoRateMsg(unknown, QOSINDEX_3, 1, CHANGE_PEER_BITRATE_REQ, 0);
    EXPECT_EQ(TEST_QOSM_GetRecvAutoRateMsgCount(), 1U);
}

// 删除已配方向的连接先清上下行通路再删 QOSM 连接
TEST_F(ActmQosmAdapterTest, UT_ACTM_QOSM_031)
{
    ActmRemoteDevice_S *device = CreateGroupedDevice(&g_addrA, TEST_GID);
    ActmStream_S *stream = CreateDeviceStream(device, NLSTK_ACTM_SINK_POINT);
    ArmGroupConnected(device, stream);
    ActmQosmLink_S *link = ActmFindQosmLinkByAddr(TEST_GID, &g_addrA);
    link->direction = NLSTK_ACTM_DIRECTION_BOTH;
    EXPECT_CALL(qosmMock, QOSM_AutoRateDeleteDataPath(_)).Times(2).WillRepeatedly(Return(QOSM_SUCCESS));
    EXPECT_CALL(qosmMock, QOSM_AutoRateDeleteConnection(_)).WillOnce(Return(QOSM_SUCCESS));
    ActmDelConnection(device, link->connHandle);
    link->direction = 0;
    EXPECT_CALL(qosmMock, QOSM_AutoRateDeleteConnection(_)).WillOnce(Return(QOSM_SUCCESS));
    ActmDelConnection(device, link->connHandle);
}

// 上层未注册回调时各类通知静默丢弃不崩溃
TEST_F(ActmQosmAdapterTest, UT_ACTM_QOSM_032)
{
    NLSTK_ActmCbk_S zeroCbk = {0};
    NLSTK_ActmRegisterCallback(&zeroCbk);
    ActmEventCbk(&g_addrA, NLSTK_ACTM_EVENT_TRANS, NLSTK_ACTM_SUCCESS, NULL);
    TEST_QOSM_NotifyBitrate(TEST_GID);
    TEST_QOSM_NotifyAutoRateMsg(nullptr);
    EXPECT_EQ(g_eventCount, 0);
    EXPECT_EQ(g_bitCount, 0);
    EXPECT_EQ(g_upDownCount, 0);
}

// 未分配链路句柄的组收到连接回调无副作用
TEST_F(ActmQosmAdapterTest, UT_ACTM_QOSM_033)
{
    ActmRemoteDevice_S *device = CreateGroupedDevice(&g_addrA, TEST_GID);
    ActmStream_S *stream = CreateDeviceStream(device, NLSTK_ACTM_SINK_POINT);
    ActmCreateQosmGroup(TEST_GID + 1, 2);
    ActmQosmAddDevice(TEST_GID + 1, &g_addrB);
    ActmQosmGroup_S *group = ActmFindQosmGroupById(TEST_GID + 1);
    group->state = ACTM_QOSM_PARAM_ADD;
    ResetCaptures();
    ArmGroupConnected(device, stream);
    TEST_QOSM_NotifyConnection(TEST_GID + 1, QOSM_CONNECTION_ADDED, QOSM_SUCCESS);
    EXPECT_EQ(g_eventCount, 1);
}