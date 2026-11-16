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
#include <cstring>
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "securec.h"

#include "stack_schedule_mock.h"
#include "stack_schedule_stub.h"
#include "stack_cm_mock.h"
#include "stack_cm_stub.h"
#include "stack_dtap_mock.h"
#include "stack_dtap_stub.h"
#include "stack_sdf_mem_mock.h"
#include "stack_sdf_mem_stub.h"

#include "sdf_addr.h"
#include "sdf_buff.h"
#include "sdf_mem.h"
#include "sdf_vector.h"
#include "nlstk_ssap_app_client.h"
#include "nlstk_ssap_app_link.h"
#include "ssap_type.h"
#include "ssap_pkt.h"
#include "ssap_manager.h"
#include "ssapc_app.h"
#include "ssapc_app_link_sm.h"
#include "ssapc_app_util.h"
#include "ssapc_cache.h"
#include "ssapc_client.h"
#include "ssapc_client_api.h"
#include "ssap_link.h"
#include "ssap_link_state.h"
#include "ssap_utils.h"
#include "ssap_handle.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

extern "C" {
extern SsapcAppRegParam_S *g_regList[NLSTK_SSAP_CLIENT_APP_MAX_NUM];
}

namespace {
constexpr int32_t TEST_REASON = 7;
constexpr uint16_t TEST_LCID = 1;
constexpr uint16_t TEST_HANDLE = 0x0010;
constexpr uint16_t TEST_END_HANDLE = 0x0020;

SLE_Addr_S g_addrA = {.type = PUBLIC_ADDRESS, .addr = {0x03, 0x03, 0x04, 0x04, 0x05, 0x05}};
SLE_Addr_S g_addrB = {.type = PUBLIC_ADDRESS, .addr = {0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F}};
NLSTK_SsapUuid_S g_uuidA = {.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02}};
NLSTK_SsapUuid_S g_uuidB = {.uuid = {0x38, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03}};

struct ConnRecord {
    int32_t appId;
    uint8_t state;
    NLSTK_Errcode_E ret;
    int32_t reason;
};
ConnRecord g_connRec[16];
int32_t g_connCnt = 0;

struct PropRecord {
    int32_t appId;
    uint16_t handle;
    uint16_t len;
    uint8_t firstByte;
};
PropRecord g_propRec[16];
int32_t g_propCnt = 0;

struct EventRecord {
    int32_t appId;
    uint16_t handle;
};
EventRecord g_eventRec[16];
int32_t g_eventCnt = 0;

struct ServChangeRecord {
    int32_t appId;
    uint16_t handle;
};
ServChangeRecord g_servChangeRec[8];
int32_t g_servChangeCnt = 0;

int32_t g_cleanResultCnt = 0;
int32_t g_freeFuncCnt = 0;
int32_t g_regAppCbCnt = 0;
int32_t g_regAppCbId = -1;
NLSTK_Errcode_E g_regAppCbRet = NLSTK_ERRCODE_FAIL;

uint8_t g_txBuf[1024] = {0};
uint16_t g_txLen = 0;
uint8_t g_txOpcode = 0;
int32_t g_txCnt = 0;

void ResetRecords()
{
    (void)memset_s(g_connRec, sizeof(g_connRec), 0, sizeof(g_connRec));
    g_connCnt = 0;
    (void)memset_s(g_propRec, sizeof(g_propRec), 0, sizeof(g_propRec));
    g_propCnt = 0;
    (void)memset_s(g_eventRec, sizeof(g_eventRec), 0, sizeof(g_eventRec));
    g_eventCnt = 0;
    (void)memset_s(g_servChangeRec, sizeof(g_servChangeRec), 0, sizeof(g_servChangeRec));
    g_servChangeCnt = 0;
    g_cleanResultCnt = 0;
    g_freeFuncCnt = 0;
    g_regAppCbCnt = 0;
    g_regAppCbId = -1;
    g_regAppCbRet = NLSTK_ERRCODE_FAIL;
    (void)memset_s(g_txBuf, sizeof(g_txBuf), 0, sizeof(g_txBuf));
    g_txLen = 0;
    g_txOpcode = 0;
    g_txCnt = 0;
}

void RecConnState(int32_t appId, uint8_t state, NLSTK_Errcode_E ret, int32_t reason)
{
    if (g_connCnt >= 16) {
        return;
    }
    g_connRec[g_connCnt] = {appId, state, ret, reason};
    g_connCnt++;
}

void RecPropertyChanged(int32_t appId, NLSTK_SsapClientReadPropertyInfo_S *info)
{
    if (g_propCnt >= 16 || info == nullptr) {
        return;
    }
    g_propRec[g_propCnt] = {appId, info->handle, info->value.len,
        info->value.len > 0 ? info->value.data[0] : 0};
    g_propCnt++;
}

void RecEvent(int32_t appId, NLSTK_SsapClientEventInfo_S *info)
{
    if (g_eventCnt >= 16 || info == nullptr) {
        return;
    }
    g_eventRec[g_eventCnt] = {appId, info->handle};
    g_eventCnt++;
}

void RecServiceChange(int32_t appId, uint16_t startHandle, NLSTK_SsapUuid_S *uuid)
{
    (void)uuid;
    if (g_servChangeCnt >= 8) {
        return;
    }
    g_servChangeRec[g_servChangeCnt] = {appId, startHandle};
    g_servChangeCnt++;
}

void RecCleanResult()
{
    g_cleanResultCnt++;
}

void RecRegApp(int32_t appId, SLE_Addr_S *addr, NLSTK_Errcode_E ret)
{
    (void)addr;
    g_regAppCbCnt++;
    g_regAppCbId = appId;
    g_regAppCbRet = ret;
}

void FillClientCb(NLSTK_SsapAppClientCb_S *cb)
{
    (void)memset_s(cb, sizeof(NLSTK_SsapAppClientCb_S), 0, sizeof(NLSTK_SsapAppClientCb_S));
    cb->onConnectionStateChanged = RecConnState;
    cb->onPropertyChanged = RecPropertyChanged;
    cb->onEvent = RecEvent;
    cb->onServiceChange = RecServiceChange;
}

int32_t RegisterApp(const SLE_Addr_S *addr)
{
    int32_t appId = -1;
    NLSTK_SsapAppClientCb_S cb;
    FillClientCb(&cb);
    NLSTK_Errcode_E ret = NLSTK_SsapClientRegApp(&appId, &cb, const_cast<SLE_Addr_S *>(addr));
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        return -1;
    }
    return appId;
}

void SeedApp(int32_t appId, NLSTK_SsapConnectLinkState_E state)
{
    SsapcAppSetLinkState(appId, state);
}

void SeedOper(int32_t appId, NLSTK_SsapClientLinkChangeEvent_E op)
{
    if (op == SSAP_LINK_CHANGE_EVENT_BUTT) {
        SsapcClearLinkOper(appId);
    } else {
        SsapcCacheLinkOper(appId, op);
    }
}

void FireEvent(int32_t appId, NLSTK_SsapClientLinkChangeEvent_E event)
{
    SsapClientLinkStateMachineCall(appId, event, TEST_REASON);
}

void CaptureSendCb(SSAP_Link_S *link, SDF_Buff_S *buff, uint8_t opcode)
{
    if (g_txCnt == 0) {
        g_txLen = (uint16_t)SDF_DataLenGet(buff);
        if (g_txLen > sizeof(g_txBuf)) {
            g_txLen = sizeof(g_txBuf);
        }
        (void)memcpy_s(g_txBuf, sizeof(g_txBuf), SDF_DataOffset(buff), g_txLen);
        g_txOpcode = opcode;
    }
    g_txCnt++;
    uint8_t type = SSAP_GetOpcodeType(opcode);
    if ((type & SSAP_REPLY_MASK) == 0) {
        SDF_BuffFree(buff);
    } else {
        SSAP_LinkSetTask(link, buff, opcode);
    }
}

struct ReadPropsRecord {
    int32_t appId;
    uint8_t errCode;
    uint8_t size;
    uint16_t handles[4];
    uint8_t errorCodes[4];
};
ReadPropsRecord g_readPropsRec;

void ResetReadPropsRecord()
{
    (void)memset_s(&g_readPropsRec, sizeof(g_readPropsRec), 0, sizeof(g_readPropsRec));
}

void RecReadProps(int32_t appId, void *arg)
{
    SSAP_ReadByHandleComplete_S *complete = (SSAP_ReadByHandleComplete_S *)arg;
    g_readPropsRec.appId = appId;
    g_readPropsRec.errCode = complete->errCode;
    g_readPropsRec.size = (uint8_t)(complete->readVals != nullptr ? complete->readVals->size : 0);
    for (uint8_t i = 0; i < g_readPropsRec.size && i < 4; i++) {
        SSAP_ValuePkt_S *pkt = (SSAP_ValuePkt_S *)SDF_VectorElementAt(complete->readVals, i);
        g_readPropsRec.handles[i] = pkt->handle;
        g_readPropsRec.errorCodes[i] = pkt->errorCode;
    }
}
}  // namespace

class UT_SSAPC_APP : public testing::Test {
public:
    NiceMock<ScheduleMock> scheduleMock;
    NiceMock<CmMock> cmMock;
    NiceMock<DtapMock> dtapMock;
    NiceMock<SdfMemMock> sdfMemMock;

protected:
    void SetUp() override
    {
        ResetRecords();
        ResetReadPropsRecord();
        ON_CALL(sdfMemMock, SDF_MemZalloc).WillByDefault(TEST_SDF_MemZalloc);
        TEST_ScheduleInit();
        EXPECT_CALL(scheduleMock, SchedulePostTask).WillRepeatedly(TEST_SchedulePostTaskStub);
        EXPECT_CALL(scheduleMock, SchedulePostTaskBlocked).WillRepeatedly(TEST_SchedulePostTaskBlockedStub);
        EXPECT_CALL(scheduleMock, ScheduleTimerAdd).WillRepeatedly(TEST_ScheduleTimerAddStub);
        EXPECT_CALL(scheduleMock, ScheduleTimerDel).WillRepeatedly(TEST_ScheduleTimerDelStub);
        TEST_CM_Init();
        EXPECT_CALL(cmMock, CM_RegLogicLinkListener).WillRepeatedly(TEST_CM_RegLogicLinkListener);
        EXPECT_CALL(cmMock, CM_UnRegLogicLinkListener).WillRepeatedly(TEST_CM_UnRegLogicLinkListener);
        EXPECT_CALL(dtapMock, DTAP_RegisterDataRecvCb).WillRepeatedly(TEST_DTAP_RegisterDataRecvCb);
        EXPECT_CALL(dtapMock, DTAP_UnregisterDataRecvCb).WillRepeatedly(TEST_DTAP_UnregisterDataRecvCb);
        EXPECT_CALL(dtapMock, DTAP_DataSend).WillRepeatedly(TEST_DTAP_DataSend);
        SSAP_Init();
    }
    void TearDown() override
    {
        SSAP_DeInit();
        TEST_StackScheduleDeInit();
        TEST_CM_DeInit();
        if (g_hdlAlloc != nullptr) {
            SSAP_DestroyHandleAllocator();
        }
    }
};

/**
 * @tc.name: SsapcLinkSmIdleConnect001
 * @tc.desc: 空闲态下发建链：链路进入连接中且不提前上报；残留缓存操作被防御性清掉
 * @tc.type: FUNC
 */
HWTEST_F(UT_SSAPC_APP, SsapcLinkSmIdleConnect001, TestSize.Level1)
{
    // 预置一个残留的缓存操作，验证建链入口先清理
    int32_t appId = RegisterApp(&g_addrA);
    ASSERT_GE(appId, 0);
    SeedOper(appId, SSAP_USER_DISCONNECT);

    FireEvent(appId, SSAP_USER_CONNECT);
    EXPECT_EQ(SsapcAppGetLinkState(appId), SSAP_CONNECT_STATE_CONNECTING);
    EXPECT_EQ(SsapcGetLinkOper(appId), SSAP_LINK_CHANGE_EVENT_BUTT);
    EXPECT_EQ(g_connCnt, 0);
}

/**
 * @tc.name: SsapcLinkSmIdleConnectFail002
 * @tc.desc: 空闲态建链失败：状态落已断开并携带连接失败码与原因值上报
 * @tc.type: FUNC
 */
HWTEST_F(UT_SSAPC_APP, SsapcLinkSmIdleConnectFail002, TestSize.Level1)
{
    int32_t appId = RegisterApp(&g_addrA);
    ASSERT_GE(appId, 0);
    EXPECT_CALL(scheduleMock, SchedulePostTask).WillRepeatedly(TEST_SchedulePostTaskStubFail);

    FireEvent(appId, SSAP_USER_CONNECT);
    EXPECT_EQ(SsapcAppGetLinkState(appId), SSAP_CONNECT_STATE_DISCONNECTED);
    EXPECT_EQ(g_connCnt, 1);
    EXPECT_EQ(g_connRec[0].appId, appId);
    EXPECT_EQ(g_connRec[0].state, SSAP_CONNECT_STATE_DISCONNECTED);
    EXPECT_EQ(g_connRec[0].ret, NLSTK_ERRCODE_CONN_FAIL);
    EXPECT_EQ(g_connRec[0].reason, TEST_REASON);
}

/**
 * @tc.name: SsapcLinkSmIdleConnectAlready003
 * @tc.desc: 底层链路已是连接态时再建链：直接落已连接并立即上报成功
 * @tc.type: FUNC
 */
HWTEST_F(UT_SSAPC_APP, SsapcLinkSmIdleConnectAlready003, TestSize.Level1)
{
    int32_t appId = RegisterApp(&g_addrA);
    ASSERT_GE(appId, 0);
    SsapLinkHandleRecordLinkStateFromCm(&g_addrA, SSAP_CONNECT_STATE_CONNECTED);

    FireEvent(appId, SSAP_USER_CONNECT);
    EXPECT_EQ(SsapcAppGetLinkState(appId), SSAP_CONNECT_STATE_CONNECTED);
    EXPECT_EQ(g_connCnt, 1);
    EXPECT_EQ(g_connRec[0].state, SSAP_CONNECT_STATE_CONNECTED);
    EXPECT_EQ(g_connRec[0].ret, NLSTK_ERRCODE_SUCCESS);
}

/**
 * @tc.name: SsapcLinkSmIdleDisconnect004
 * @tc.desc: 空闲态下发断链：从未建过链的应用直接收到一次断链成功上报，状态保持空闲
 * @tc.type: FUNC
 */
HWTEST_F(UT_SSAPC_APP, SsapcLinkSmIdleDisconnect004, TestSize.Level1)
{
    int32_t appId = RegisterApp(&g_addrA);
    ASSERT_GE(appId, 0);

    FireEvent(appId, SSAP_USER_DISCONNECT);
    EXPECT_EQ(SsapcAppGetLinkState(appId), SSAP_CONNECT_STATE_IDLE);
    EXPECT_EQ(g_connCnt, 1);
    EXPECT_EQ(g_connRec[0].state, SSAP_CONNECT_STATE_DISCONNECTED);
    EXPECT_EQ(g_connRec[0].ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(g_connRec[0].reason, TEST_REASON);
}

/**
 * @tc.name: SsapcLinkSmIdleLinkConnected005
 * @tc.desc: 空闲态收到链路已连事件：无缓存时静默；有缓存时防御性清理并上报断链
 * @tc.type: FUNC
 */
HWTEST_F(UT_SSAPC_APP, SsapcLinkSmIdleLinkConnected005, TestSize.Level1)
{
    int32_t appId = RegisterApp(&g_addrA);
    ASSERT_GE(appId, 0);
    FireEvent(appId, SSAP_LOGIC_LINK_CONNECTED);
    EXPECT_EQ(g_connCnt, 0);
    EXPECT_EQ(SsapcAppGetLinkState(appId), SSAP_CONNECT_STATE_IDLE);

    SeedOper(appId, SSAP_USER_CONNECT);
    FireEvent(appId, SSAP_LOGIC_LINK_CONNECTED);
    EXPECT_EQ(g_connCnt, 1);
    EXPECT_EQ(g_connRec[0].state, SSAP_CONNECT_STATE_DISCONNECTED);
    EXPECT_EQ(g_connRec[0].ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(SsapcGetLinkOper(appId), SSAP_LINK_CHANGE_EVENT_BUTT);
}

/**
 * @tc.name: SsapcLinkSmIdleCacheReplay006
 * @tc.desc: 断链期间到达的建链请求先被缓存，底层断链完成后自动重放发起建链
 * @tc.type: FUNC
 */
HWTEST_F(UT_SSAPC_APP, SsapcLinkSmIdleCacheReplay006, TestSize.Level1)
{
    int32_t appId = RegisterApp(&g_addrA);
    ASSERT_GE(appId, 0);
    // 底层先建链成功，再发起断链，使底层处于断开中
    SsapLinkHandleRecordLinkStateFromCm(&g_addrA, SSAP_CONNECT_STATE_CONNECTED);
    SeedApp(appId, SSAP_CONNECT_STATE_IDLE);
    ASSERT_EQ(SsapLinkHandleUserDisconnect(&g_addrA), SSAP_CONNECT_STATE_DISCONNECTING);

    FireEvent(appId, SSAP_USER_CONNECT);
    EXPECT_EQ(SsapcGetLinkOper(appId), SSAP_USER_CONNECT);
    EXPECT_EQ(SsapcAppGetLinkState(appId), SSAP_CONNECT_STATE_IDLE);
    EXPECT_EQ(g_connCnt, 0);

    // 底层断链完成，状态机重放缓存的建链
    SsapLinkHandleRecordLinkStateFromCm(&g_addrA, SSAP_CONNECT_STATE_DISCONNECTED);
    FireEvent(appId, SSAP_LOGIC_LINK_DISCONNECTED);
    EXPECT_EQ(SsapcGetLinkOper(appId), SSAP_LINK_CHANGE_EVENT_BUTT);
    EXPECT_EQ(SsapcAppGetLinkState(appId), SSAP_CONNECT_STATE_CONNECTING);
    EXPECT_EQ(g_connCnt, 0);
}

/**
 * @tc.name: SsapcLinkSmConnectingSweep007
 * @tc.desc: 连接中态四事件全扫：断链被缓存、再次建链抵消缓存、建链成功落已连接、断链失败重放缓存
 * @tc.type: FUNC
 */
HWTEST_F(UT_SSAPC_APP, SsapcLinkSmConnectingSweep007, TestSize.Level1)
{
    int32_t appId = RegisterApp(&g_addrA);
    ASSERT_GE(appId, 0);
    SeedApp(appId, SSAP_CONNECT_STATE_CONNECTING);

    // 无缓存时重复建链：无动作
    FireEvent(appId, SSAP_USER_CONNECT);
    EXPECT_EQ(g_connCnt, 0);
    EXPECT_EQ(SsapcGetLinkOper(appId), SSAP_LINK_CHANGE_EVENT_BUTT);

    // 连接中下发断链：缓存断链操作
    FireEvent(appId, SSAP_USER_DISCONNECT);
    EXPECT_EQ(SsapcGetLinkOper(appId), SSAP_USER_DISCONNECT);
    EXPECT_EQ(SsapcAppGetLinkState(appId), SSAP_CONNECT_STATE_CONNECTING);

    // 再次建链：与缓存断链抵消
    FireEvent(appId, SSAP_USER_CONNECT);
    EXPECT_EQ(SsapcGetLinkOper(appId), SSAP_LINK_CHANGE_EVENT_BUTT);

    // 底层建链成功：落已连接并上报；缓存的断链被同步重放（底层无链路记录，直接落已断开并再次上报）
    SeedOper(appId, SSAP_USER_DISCONNECT);
    FireEvent(appId, SSAP_LOGIC_LINK_CONNECTED);
    EXPECT_EQ(SsapcAppGetLinkState(appId), SSAP_CONNECT_STATE_DISCONNECTED);
    EXPECT_EQ(g_connCnt, 2);
    EXPECT_EQ(g_connRec[0].appId, appId);
    EXPECT_EQ(g_connRec[0].state, SSAP_CONNECT_STATE_CONNECTED);
    EXPECT_EQ(g_connRec[0].ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(g_connRec[1].appId, appId);
    EXPECT_EQ(g_connRec[1].state, SSAP_CONNECT_STATE_DISCONNECTED);
    EXPECT_EQ(g_connRec[1].ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(SsapcGetLinkOper(appId), SSAP_LINK_CHANGE_EVENT_BUTT);

    // 另一个应用走建链失败：落已断开并上报，重放的建链发起到底层进入连接中
    int32_t appId2 = RegisterApp(&g_addrB);
    ASSERT_GE(appId2, 0);
    SeedApp(appId2, SSAP_CONNECT_STATE_CONNECTING);
    SeedOper(appId2, SSAP_USER_CONNECT);
    FireEvent(appId2, SSAP_LOGIC_LINK_DISCONNECTED);
    EXPECT_EQ(SsapcAppGetLinkState(appId2), SSAP_CONNECT_STATE_CONNECTING);
    EXPECT_EQ(g_connCnt, 3);
    EXPECT_EQ(g_connRec[2].appId, appId2);
    EXPECT_EQ(g_connRec[2].state, SSAP_CONNECT_STATE_DISCONNECTED);
    EXPECT_EQ(g_connRec[2].ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(SsapcGetLinkOper(appId2), SSAP_LINK_CHANGE_EVENT_BUTT);
}

/**
 * @tc.name: SsapcLinkSmConnectedDisconnect008
 * @tc.desc: 已连接态断链三腿：共址他应用占用时只改本应用状态、独占时进入断开中、底层断链失败落已断开
 * @tc.type: FUNC
 */
HWTEST_F(UT_SSAPC_APP, SsapcLinkSmConnectedDisconnect008, TestSize.Level1)
{
    int32_t appId = RegisterApp(&g_addrA);
    int32_t appIdPeer = RegisterApp(&g_addrA);
    ASSERT_GE(appId, 0);
    ASSERT_GE(appIdPeer, 0);
    SeedApp(appId, SSAP_CONNECT_STATE_CONNECTED);
    SeedApp(appIdPeer, SSAP_CONNECT_STATE_CONNECTED);
    SsapLinkHandleRecordLinkStateFromCm(&g_addrA, SSAP_CONNECT_STATE_CONNECTED);

    // 同地址他应用仍处于已连接：不允许真正断链，仅本应用状态置为已断开并上报
    ResetRecords();
    FireEvent(appId, SSAP_USER_DISCONNECT);
    EXPECT_EQ(SsapcAppGetLinkState(appId), SSAP_CONNECT_STATE_DISCONNECTED);
    EXPECT_EQ(SsapcAppGetLinkState(appIdPeer), SSAP_CONNECT_STATE_CONNECTED);
    EXPECT_EQ(g_connCnt, 1);
    EXPECT_EQ(g_connRec[0].appId, appId);
    EXPECT_EQ(g_connRec[0].state, SSAP_CONNECT_STATE_DISCONNECTED);
    EXPECT_EQ(g_connRec[0].ret, NLSTK_ERRCODE_SUCCESS);

    // 他应用改为连接中：依旧阻断真断链
    SeedApp(appIdPeer, SSAP_CONNECT_STATE_CONNECTING);
    SeedApp(appId, SSAP_CONNECT_STATE_CONNECTED);
    ResetRecords();
    FireEvent(appId, SSAP_USER_DISCONNECT);
    EXPECT_EQ(SsapcAppGetLinkState(appId), SSAP_CONNECT_STATE_DISCONNECTED);
    EXPECT_EQ(g_connCnt, 1);

    // 他应用不再占用（处于断开中）：独占断链成功进入断开中，不提前上报
    SeedApp(appIdPeer, SSAP_CONNECT_STATE_DISCONNECTING);
    SeedApp(appId, SSAP_CONNECT_STATE_CONNECTED);
    ResetRecords();
    FireEvent(appId, SSAP_USER_DISCONNECT);
    EXPECT_EQ(SsapcAppGetLinkState(appId), SSAP_CONNECT_STATE_DISCONNECTING);
    EXPECT_EQ(g_connCnt, 0);

    // 底层断链调用失败：直接落已断开并上报成功
    SeedApp(appId, SSAP_CONNECT_STATE_CONNECTED);
    // 上一腿把底层置为断开中，重新铺已连接使本腿真正走到底层断链调用
    SsapLinkHandleRecordLinkStateFromCm(&g_addrA, SSAP_CONNECT_STATE_CONNECTED);
    EXPECT_CALL(scheduleMock, SchedulePostTask).WillRepeatedly(TEST_SchedulePostTaskStubFail);
    ResetRecords();
    FireEvent(appId, SSAP_USER_DISCONNECT);
    EXPECT_EQ(SsapcAppGetLinkState(appId), SSAP_CONNECT_STATE_DISCONNECTED);
    EXPECT_EQ(g_connCnt, 1);
    EXPECT_EQ(g_connRec[0].ret, NLSTK_ERRCODE_SUCCESS);
}

/**
 * @tc.name: SsapcLinkSmConnectedSweep009
 * @tc.desc: 已连接态其余事件：重复建链幂等上报、异常重连事件重放缓存、链路断开落已断开并重放
 * @tc.type: FUNC
 */
HWTEST_F(UT_SSAPC_APP, SsapcLinkSmConnectedSweep009, TestSize.Level1)
{
    int32_t appId = RegisterApp(&g_addrA);
    ASSERT_GE(appId, 0);
    SeedApp(appId, SSAP_CONNECT_STATE_CONNECTED);

    // 已连接再建链：幂等上报已连接
    FireEvent(appId, SSAP_USER_CONNECT);
    EXPECT_EQ(g_connCnt, 1);
    EXPECT_EQ(g_connRec[0].state, SSAP_CONNECT_STATE_CONNECTED);
    EXPECT_EQ(SsapcAppGetLinkState(appId), SSAP_CONNECT_STATE_CONNECTED);

    // 已连接又收到链路已连：无缓存时静默
    ResetRecords();
    FireEvent(appId, SSAP_LOGIC_LINK_CONNECTED);
    EXPECT_EQ(g_connCnt, 0);

    // 链路断开：落已断开并上报，缓存的建链被重放，底层重建后进入连接中
    SeedApp(appId, SSAP_CONNECT_STATE_CONNECTED);
    SeedOper(appId, SSAP_USER_CONNECT);
    ResetRecords();
    FireEvent(appId, SSAP_LOGIC_LINK_DISCONNECTED);
    EXPECT_EQ(g_connCnt, 1);
    EXPECT_EQ(g_connRec[0].state, SSAP_CONNECT_STATE_DISCONNECTED);
    EXPECT_EQ(g_connRec[0].ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(SsapcAppGetLinkState(appId), SSAP_CONNECT_STATE_CONNECTING);
    EXPECT_EQ(SsapcGetLinkOper(appId), SSAP_LINK_CHANGE_EVENT_BUTT);
}

/**
 * @tc.name: SsapcLinkSmDisconnectingSweep010
 * @tc.desc: 断开中态四事件：建链被缓存、重复断链不叠加、异常收到已连上报故障、断链完成重放缓存
 * @tc.type: FUNC
 */
HWTEST_F(UT_SSAPC_APP, SsapcLinkSmDisconnectingSweep010, TestSize.Level1)
{
    int32_t appId = RegisterApp(&g_addrA);
    ASSERT_GE(appId, 0);
    SeedApp(appId, SSAP_CONNECT_STATE_DISCONNECTING);

    // 无缓存时建链：缓存建链操作
    FireEvent(appId, SSAP_USER_CONNECT);
    EXPECT_EQ(SsapcGetLinkOper(appId), SSAP_USER_CONNECT);
    EXPECT_EQ(SsapcAppGetLinkState(appId), SSAP_CONNECT_STATE_DISCONNECTING);

    // 已缓存建链时再断链：抵消缓存
    FireEvent(appId, SSAP_USER_DISCONNECT);
    EXPECT_EQ(SsapcGetLinkOper(appId), SSAP_LINK_CHANGE_EVENT_BUTT);

    // 断开中重复断链：无额外动作
    ResetRecords();
    FireEvent(appId, SSAP_USER_DISCONNECT);
    EXPECT_EQ(g_connCnt, 0);

    // 断开中异常收到链路已连：上报连接失败并清理缓存
    SeedOper(appId, SSAP_USER_CONNECT);
    ResetRecords();
    FireEvent(appId, SSAP_LOGIC_LINK_CONNECTED);
    EXPECT_EQ(SsapcAppGetLinkState(appId), SSAP_CONNECT_STATE_DISCONNECTED);
    EXPECT_EQ(g_connCnt, 1);
    EXPECT_EQ(g_connRec[0].ret, NLSTK_ERRCODE_CONN_FAIL);
    EXPECT_EQ(SsapcGetLinkOper(appId), SSAP_LINK_CHANGE_EVENT_BUTT);

    // 断链完成：上报成功并重放缓存的建链，重放的建链发起到底层进入连接中
    SeedApp(appId, SSAP_CONNECT_STATE_DISCONNECTING);
    SeedOper(appId, SSAP_USER_CONNECT);
    ResetRecords();
    FireEvent(appId, SSAP_LOGIC_LINK_DISCONNECTED);
    EXPECT_EQ(SsapcAppGetLinkState(appId), SSAP_CONNECT_STATE_CONNECTING);
    EXPECT_EQ(g_connCnt, 1);
    EXPECT_EQ(g_connRec[0].ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(SsapcGetLinkOper(appId), SSAP_LINK_CHANGE_EVENT_BUTT);
}

/**
 * @tc.name: SsapcLinkSmDisconnectedSweep011
 * @tc.desc: 已断开态四事件：建链按空闲路径执行、断链直接上报、链路事件按缓存重放或忽略
 * @tc.type: FUNC
 */
HWTEST_F(UT_SSAPC_APP, SsapcLinkSmDisconnectedSweep011, TestSize.Level1)
{
    int32_t appId = RegisterApp(&g_addrA);
    ASSERT_GE(appId, 0);
    SeedApp(appId, SSAP_CONNECT_STATE_DISCONNECTED);

    // 已断开再断链：直接上报断链成功
    FireEvent(appId, SSAP_USER_DISCONNECT);
    EXPECT_EQ(g_connCnt, 1);
    EXPECT_EQ(g_connRec[0].state, SSAP_CONNECT_STATE_DISCONNECTED);
    EXPECT_EQ(g_connRec[0].ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(SsapcAppGetLinkState(appId), SSAP_CONNECT_STATE_DISCONNECTED);

    // 他应用触发同地址链路已连：本应用无缓存时静默
    ResetRecords();
    FireEvent(appId, SSAP_LOGIC_LINK_CONNECTED);
    EXPECT_EQ(g_connCnt, 0);

    // 同地址链路断开且本应用有缓存：清理并重放缓存操作，重放的断链在已断开态直接上报
    SeedOper(appId, SSAP_USER_DISCONNECT);
    ResetRecords();
    FireEvent(appId, SSAP_LOGIC_LINK_DISCONNECTED);
    EXPECT_EQ(g_connCnt, 1);
    EXPECT_EQ(g_connRec[0].state, SSAP_CONNECT_STATE_DISCONNECTED);
    EXPECT_EQ(g_connRec[0].ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(SsapcGetLinkOper(appId), SSAP_LINK_CHANGE_EVENT_BUTT);
}

/**
 * @tc.name: SsapcLinkSmGuard012
 * @tc.desc: 越界事件与未注册应用触发状态机均安全返回，无状态与上报变化
 * @tc.type: FUNC
 */
HWTEST_F(UT_SSAPC_APP, SsapcLinkSmGuard012, TestSize.Level1)
{
    int32_t appId = RegisterApp(&g_addrA);
    ASSERT_GE(appId, 0);
    FireEvent(appId, SSAP_LINK_CHANGE_EVENT_BUTT);
    EXPECT_EQ(g_connCnt, 0);
    EXPECT_EQ(SsapcAppGetLinkState(appId), SSAP_CONNECT_STATE_IDLE);

    SsapClientLinkStateMachineCall(NLSTK_SSAP_CLIENT_APP_MAX_NUM + 1, SSAP_USER_CONNECT, TEST_REASON);
    SsapClientLinkStateMachineCall(-1, SSAP_USER_CONNECT, TEST_REASON);
    EXPECT_EQ(g_connCnt, 0);
}

/**
 * @tc.name: SsapcAppGetAppIdListByAddr013
 * @tc.desc: 按地址收集应用列表只命中同地址已注册应用，容量不足时按上限截断
 * @tc.type: FUNC
 */
HWTEST_F(UT_SSAPC_APP, SsapcAppGetAppIdListByAddr013, TestSize.Level1)
{
    int32_t appA1 = RegisterApp(&g_addrA);
    int32_t appA2 = RegisterApp(&g_addrA);
    int32_t appB = RegisterApp(&g_addrB);
    ASSERT_GE(appA1, 0);
    ASSERT_GE(appA2, 0);
    ASSERT_GE(appB, 0);

    int32_t idList[NLSTK_SSAP_CLIENT_APP_MAX_NUM] = {0};
    uint32_t cnt = SsapcAppGetAppIdListByAddr(&g_addrA, idList, NLSTK_SSAP_CLIENT_APP_MAX_NUM);
    EXPECT_EQ(cnt, 2U);
    EXPECT_EQ(idList[0], appA1);
    EXPECT_EQ(idList[1], appA2);

    // 容量为 1 时只返回第一个匹配
    int32_t oneList[1] = {0};
    EXPECT_EQ(SsapcAppGetAppIdListByAddr(&g_addrA, oneList, 1), 1U);
    EXPECT_EQ(SsapcAppGetAppIdListByAddr(nullptr, idList, NLSTK_SSAP_CLIENT_APP_MAX_NUM), 0U);
}

/**
 * @tc.name: SsapcAppRegisterFields014
 * @tc.desc: 注册成功后登记项字段完整：地址、初始空闲态、无缓存操作与默认交互超时
 * @tc.type: FUNC
 */
HWTEST_F(UT_SSAPC_APP, SsapcAppRegisterFields014, TestSize.Level1)
{
    int32_t appId = RegisterApp(&g_addrA);
    ASSERT_GE(appId, 0);
    ASSERT_LT(appId, NLSTK_SSAP_CLIENT_APP_MAX_NUM);
    ASSERT_NE(g_regList[appId], nullptr);
    EXPECT_EQ(memcmp(&g_regList[appId]->addr, &g_addrA, sizeof(SLE_Addr_S)), 0);
    EXPECT_EQ(g_regList[appId]->linkState, SSAP_CONNECT_STATE_IDLE);
    EXPECT_EQ(g_regList[appId]->nextOperator, SSAP_LINK_CHANGE_EVENT_BUTT);
    EXPECT_GT(g_regList[appId]->timeout, 0);
    EXPECT_NE(g_regList[appId]->cb, nullptr);
}

/**
 * @tc.name: SsapcAppRegisterExhaust015
 * @tc.desc: 应用id占满五十个后再注册失败，不越界不崩溃
 * @tc.type: FUNC
 */
HWTEST_F(UT_SSAPC_APP, SsapcAppRegisterExhaust015, TestSize.Level1)
{
    for (int32_t i = 0; i < NLSTK_SSAP_CLIENT_APP_MAX_NUM; i++) {
        int32_t appId = RegisterApp(&g_addrA);
        ASSERT_EQ(appId, i) << "register loop at " << i;
    }
    EXPECT_EQ(RegisterApp(&g_addrA), -1);
}

/**
 * @tc.name: SsapcAppDeregister016
 * @tc.desc: 注销清空登记项并解除回调引用；非法应用号与重复注销均安全无动作
 * @tc.type: FUNC
 */
HWTEST_F(UT_SSAPC_APP, SsapcAppDeregister016, TestSize.Level1)
{
    int32_t appId = RegisterApp(&g_addrA);
    ASSERT_GE(appId, 0);
    NLSTK_SsapClientDeregApp(appId);
    EXPECT_EQ(g_regList[appId], nullptr);

    // 重复注销与非法应用号：无崩溃、无状态变化
    NLSTK_SsapClientDeregApp(appId);
    NLSTK_SsapClientDeregApp(-1);
    NLSTK_SsapClientDeregApp(NLSTK_SSAP_CLIENT_APP_MAX_NUM);
    EXPECT_EQ(g_connCnt, 0);
}

/**
 * @tc.name: SsapcAppIdValidBoundary017
 * @tc.desc: 应用号有效性判定边界：服务变更专用号恒有效、负数与超界恒无效、未注册槽位无效
 * @tc.type: FUNC
 */
HWTEST_F(UT_SSAPC_APP, SsapcAppIdValidBoundary017, TestSize.Level1)
{
    EXPECT_TRUE(IsAppIdValid(101));
    EXPECT_FALSE(IsAppIdValid(-1));
    EXPECT_FALSE(IsAppIdValid(NLSTK_SSAP_CLIENT_APP_MAX_NUM));
    EXPECT_FALSE(IsAppIdValid(0));

    int32_t appId = RegisterApp(&g_addrA);
    ASSERT_GE(appId, 0);
    EXPECT_TRUE(IsAppIdValid(appId));
    EXPECT_EQ(GetAddrByAppId(-1), nullptr);
    EXPECT_EQ(GetConnParamByAppId(-1), nullptr);
    EXPECT_NE(GetAddrByAppId(appId), nullptr);
}

/**
 * @tc.name: SsapcAppRegisterAsync018
 * @tc.desc: 异步注册携带注册回调钩子时回调收到应用号与成功码，未携带钩子时安全跳过
 * @tc.type: FUNC
 */
HWTEST_F(UT_SSAPC_APP, SsapcAppRegisterAsync018, TestSize.Level1)
{
    NLSTK_ConnParam_S connParam = {0};
    NLSTK_SsapAppClientCb_S cb;
    FillClientCb(&cb);
    cb.onRegisterApp = RecRegApp;
    NLSTK_Errcode_E ret = NLSTK_SsapClientRegAppAsyn(&g_addrA, &connParam, &cb);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(g_regAppCbCnt, 1);
    EXPECT_GE(g_regAppCbId, 0);
    EXPECT_EQ(g_regAppCbRet, NLSTK_ERRCODE_SUCCESS);

    cb.onRegisterApp = nullptr;
    ret = NLSTK_SsapClientRegAppAsyn(&g_addrA, &connParam, &cb);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(g_regAppCbCnt, 1);
}

/**
 * @tc.name: SsapcAppNotifyClientCleanUp019
 * @tc.desc: 清理通知逐个回收空闲或已断开的应用；未注册结果回调时不触发回调
 * @tc.type: FUNC
 */
HWTEST_F(UT_SSAPC_APP, SsapcAppNotifyClientCleanUp019, TestSize.Level1)
{
    int32_t app1 = RegisterApp(&g_addrA);
    int32_t app2 = RegisterApp(&g_addrB);
    ASSERT_GE(app1, 0);
    ASSERT_GE(app2, 0);

    SsapcAppNotifyClientCleanUp();
    EXPECT_EQ(g_regList[app1], nullptr);
    EXPECT_EQ(g_regList[app2], nullptr);
    // 未设置结果回调时不触发，仅完成清理
    EXPECT_EQ(g_cleanResultCnt, 0);
}

/**
 * @tc.name: SsapcAppNotifyClientCleanUpBlocked020
 * @tc.desc: 遇到仍处连接中的应用时清理中止，其后的应用不被回收
 * @tc.type: FUNC
 */
HWTEST_F(UT_SSAPC_APP, SsapcAppNotifyClientCleanUpBlocked020, TestSize.Level1)
{
    int32_t app1 = RegisterApp(&g_addrA);
    int32_t app2 = RegisterApp(&g_addrB);
    int32_t app3 = RegisterApp(&g_addrA);
    ASSERT_GE(app1, 0);
    ASSERT_GE(app2, 0);
    ASSERT_GE(app3, 0);
    SeedApp(app2, SSAP_CONNECT_STATE_CONNECTED);

    SsapcAppNotifyClientCleanUp();
    EXPECT_EQ(g_regList[app1], nullptr);
    EXPECT_NE(g_regList[app2], nullptr);
    // 遇到连接中的应用即中止，其后注册的应用保持原样
    EXPECT_NE(g_regList[app3], nullptr);
}

/**
 * @tc.name: SsapcClientCleanAppMixed021
 * @tc.desc: 全量清理对连接中的应用发起底层断链，对可回收应用立即回收，全部完成后回调上层
 * @tc.type: FUNC
 */
HWTEST_F(UT_SSAPC_APP, SsapcClientCleanAppMixed021, TestSize.Level1)
{
    int32_t appIdle = RegisterApp(&g_addrA);
    int32_t appConn = RegisterApp(&g_addrB);
    ASSERT_GE(appIdle, 0);
    ASSERT_GE(appConn, 0);
    SeedApp(appConn, SSAP_CONNECT_STATE_CONNECTED);

    SsapcClientCleanApp((void *)RecCleanResult);
    // 空闲应用被回收，连接中应用保留并等待底层断链
    EXPECT_EQ(g_regList[appIdle], nullptr);
    EXPECT_NE(g_regList[appConn], nullptr);
    EXPECT_EQ(g_cleanResultCnt, 0);

    // 底层断链完成后再次通知，回收剩余应用并回调上层
    SeedApp(appConn, SSAP_CONNECT_STATE_DISCONNECTED);
    SsapcAppNotifyClientCleanUp();
    EXPECT_EQ(g_regList[appConn], nullptr);
    EXPECT_EQ(g_cleanResultCnt, 1);
    EXPECT_FALSE(SsapGetClientCleanUp());
}

/**
 * @tc.name: SsapcClientAppDeinit022
 * @tc.desc: 反初始化回收全部登记项且清空结果回调指针
 * @tc.type: FUNC
 */
HWTEST_F(UT_SSAPC_APP, SsapcClientAppDeinit022, TestSize.Level1)
{
    int32_t app1 = RegisterApp(&g_addrA);
    int32_t app2 = RegisterApp(&g_addrB);
    ASSERT_GE(app1, 0);
    ASSERT_GE(app2, 0);
    SeedApp(app1, SSAP_CONNECT_STATE_CONNECTED);

    SsapcClientAppDeinit();
    for (int32_t i = 0; i < NLSTK_SSAP_CLIENT_APP_MAX_NUM; i++) {
        EXPECT_EQ(g_regList[i], nullptr);
    }
}

/**
 * @tc.name: SsapcAppPropertyNtfDispatch023
 * @tc.desc: 属性通知只分发给订阅了对应句柄且开关匹配的应用，未订阅应用不收到
 * @tc.type: FUNC
 */
HWTEST_F(UT_SSAPC_APP, SsapcAppPropertyNtfDispatch023, TestSize.Level1)
{
    int32_t appSub = RegisterApp(&g_addrA);
    int32_t appOther = RegisterApp(&g_addrA);
    int32_t appB = RegisterApp(&g_addrB);
    ASSERT_GE(appSub, 0);
    ASSERT_GE(appOther, 0);
    ASSERT_GE(appB, 0);
    ASSERT_EQ(SsapcCacheCreate(&g_addrA), NLSTK_ERRCODE_SUCCESS);
    ASSERT_EQ(SsapcCacheSetCpcd(appSub, &g_addrA, TEST_HANDLE, CPCD_INDICATION_ENABLE),
        NLSTK_ERRCODE_SUCCESS);
    ASSERT_EQ(SsapcCacheSetCpcd(appOther, &g_addrA, TEST_HANDLE, CPCD_NOTIFICATION_ENABLE),
        NLSTK_ERRCODE_DIRECT_RETURN);

    uint8_t payload[2] = {0x5A, 0xA5};
    SSAP_ValuePkt_S *pkt = (SSAP_ValuePkt_S *)SDF_MemZalloc(sizeof(SSAP_ValuePkt_S) + sizeof(payload));
    ASSERT_NE(pkt, nullptr);
    (void)memcpy_s(&pkt->addr, sizeof(SLE_Addr_S), &g_addrA, sizeof(SLE_Addr_S));
    pkt->opCode = SSAP_VALUE_IND;
    pkt->controlCode = 0;
    pkt->handle = TEST_HANDLE;
    pkt->value.len = sizeof(payload);
    (void)memcpy_s(pkt->value.value, sizeof(payload), payload, sizeof(payload));

    SsapcAppPropertyNtf(pkt);
    // 指示报文只送达打开了指示开关的应用
    EXPECT_EQ(g_propCnt, 1);
    EXPECT_EQ(g_propRec[0].appId, appSub);
    EXPECT_EQ(g_propRec[0].handle, TEST_HANDLE);
    EXPECT_EQ(g_propRec[0].len, 2U);
    EXPECT_EQ(g_propRec[0].firstByte, 0x5A);
    SDF_MemFree(pkt);
}

/**
 * @tc.name: SsapcAppEventNtfDispatch024
 * @tc.desc: 事件通知按地址广播给同地址全部注册应用，异地址应用不收到
 * @tc.type: FUNC
 */
HWTEST_F(UT_SSAPC_APP, SsapcAppEventNtfDispatch024, TestSize.Level1)
{
    int32_t appA1 = RegisterApp(&g_addrA);
    int32_t appA2 = RegisterApp(&g_addrA);
    int32_t appB = RegisterApp(&g_addrB);
    ASSERT_GE(appA1, 0);
    ASSERT_GE(appA2, 0);
    ASSERT_GE(appB, 0);

    SSAP_ValuePkt_S *pkt = (SSAP_ValuePkt_S *)SDF_MemZalloc(sizeof(SSAP_ValuePkt_S));
    ASSERT_NE(pkt, nullptr);
    (void)memcpy_s(&pkt->addr, sizeof(SLE_Addr_S), &g_addrA, sizeof(SLE_Addr_S));
    pkt->opCode = SSAP_VALUE_NTF;
    pkt->controlCode = 0x04;
    pkt->handle = TEST_HANDLE;

    SsapcAppPropertyNtf(pkt);
    EXPECT_EQ(g_eventCnt, 2);
    EXPECT_EQ(g_eventRec[0].appId, appA1);
    EXPECT_EQ(g_eventRec[1].appId, appA2);
    EXPECT_EQ(g_eventRec[0].handle, TEST_HANDLE);
    SDF_MemFree(pkt);
}

/**
 * @tc.name: SsapcAppServChangeDispatch025
 * @tc.desc: 服务变更按缓存中的服务身份分发到同地址应用；查不到服务身份时不分发
 * @tc.type: FUNC
 */
HWTEST_F(UT_SSAPC_APP, SsapcAppServChangeDispatch025, TestSize.Level1)
{
    int32_t app1 = RegisterApp(&g_addrA);
    int32_t app2 = RegisterApp(&g_addrA);
    ASSERT_GE(app1, 0);
    ASSERT_GE(app2, 0);
    ASSERT_EQ(SsapcCacheCreate(&g_addrA), NLSTK_ERRCODE_SUCCESS);
    SsapCacheServInfo_S servInfo = {0};
    servInfo.handle = TEST_HANDLE;
    servInfo.endHandle = TEST_END_HANDLE;
    servInfo.uuid = g_uuidA;
    ASSERT_EQ(SsapcCacheServ(&g_addrA, &servInfo), NLSTK_ERRCODE_SUCCESS);
    SsapcCacheServDiscFinish(&g_addrA);

    // 未建链路实体：服务变更只做回调分发，不再下发发现
    SSAP_Link_S *link = SSAP_CreateSsapLink(&g_addrA, TEST_LCID, CaptureSendCb);
    ASSERT_NE(link, nullptr);
    ResetRecords();
    SsapcAppServChange(&g_addrA, TEST_HANDLE, TEST_END_HANDLE);
    EXPECT_EQ(g_servChangeCnt, 2);
    EXPECT_EQ(g_servChangeRec[0].appId, app1);
    EXPECT_EQ(g_servChangeRec[1].appId, app2);
    // 同步触发重发现任务，报文经发送钩子截获
    EXPECT_GT(g_txCnt, 0);
    SSAP_DeleteSsapLinkByAddr(&g_addrA);

    // 句柄查不到服务身份：不产生任何分发
    ResetRecords();
    SsapcAppServChange(&g_addrA, TEST_END_HANDLE, TEST_END_HANDLE);
    EXPECT_EQ(g_servChangeCnt, 0);
}

/**
 * @tc.name: SsapcCacheServLifecycle001
 * @tc.desc: 服务入缓登记句柄范围，重复句柄被拒绝，最大句柄取各服务结束句柄
 * @tc.type: FUNC
 */
HWTEST_F(UT_SSAPC_APP, SsapcCacheServLifecycle001, TestSize.Level1)
{
    ASSERT_EQ(SsapcCacheCreate(&g_addrA), NLSTK_ERRCODE_SUCCESS);

    SsapCacheServInfo_S serv1 = {0};
    serv1.handle = 0x0011;
    serv1.endHandle = 0x0015;
    serv1.uuid = g_uuidA;
    SsapCacheServInfo_S serv2 = {0};
    serv2.handle = 0x0020;
    serv2.endHandle = 0x0030;
    serv2.uuid = g_uuidB;
    ASSERT_EQ(SsapcCacheServ(&g_addrA, &serv1), NLSTK_ERRCODE_SUCCESS);
    ASSERT_EQ(SsapcCacheServ(&g_addrA, &serv2), NLSTK_ERRCODE_SUCCESS);

    // 重复句柄拒绝
    EXPECT_EQ(SsapcCacheServ(&g_addrA, &serv1), NLSTK_ERRCODE_FAIL);
    // 未建缓存的地址与空指针入参拒绝
    EXPECT_EQ(SsapcCacheServ(&g_addrB, &serv1), NLSTK_ERRCODE_FAIL);
    EXPECT_EQ(SsapcCacheServ(nullptr, &serv1), NLSTK_ERRCODE_POINTER_NULL);
    EXPECT_EQ(SsapcCacheServ(&g_addrA, nullptr), NLSTK_ERRCODE_POINTER_NULL);

    EXPECT_EQ(SsapcCacheGetMaxServHandle(&g_addrA), 0x0030U);
    EXPECT_EQ(SsapcCacheGetMaxServHandle(&g_addrB), 0U);
    EXPECT_EQ(SsapcCacheGetMaxServHandle(nullptr), 0U);
}

/**
 * @tc.name: SsapcCacheMemberLifecycle002
 * @tc.desc: 属性、方法、事件按句柄归入所属服务并深拷贝描述符列表，错句柄与错地址拒绝
 * @tc.type: FUNC
 */
HWTEST_F(UT_SSAPC_APP, SsapcCacheMemberLifecycle002, TestSize.Level1)
{
    ASSERT_EQ(SsapcCacheCreate(&g_addrA), NLSTK_ERRCODE_SUCCESS);
    SsapCacheServInfo_S serv = {0};
    serv.handle = TEST_HANDLE;
    serv.endHandle = TEST_END_HANDLE;
    serv.uuid = g_uuidA;
    serv.memberValue = MEMBER_TYPE_PROPERTY | MEMBER_TYPE_METHOD | MEMBER_TYPE_EVENT;
    ASSERT_EQ(SsapcCacheServ(&g_addrA, &serv), NLSTK_ERRCODE_SUCCESS);

    NLSTK_SsapDtor_S dtors[2] = {{0x0001, DESC_TYPE_CLIENT_CONFIG, {{{0}}}, {{{0}}}}};
    NLSTK_SsapPrty_S prty = {0};
    prty.handle = 0x0012;
    prty.uuid = g_uuidA;
    prty.descriptorNum = 2;
    prty.descriptors = dtors;
    ASSERT_EQ(SsapcCachePrty(&g_addrA, &prty), NLSTK_ERRCODE_SUCCESS);

    NLSTK_SsapPrty_S method = {0};
    method.handle = 0x0013;
    method.uuid = g_uuidA;
    ASSERT_EQ(SsapcCacheMethod(&g_addrA, &method), NLSTK_ERRCODE_SUCCESS);

    NLSTK_SsapPrty_S event = {0};
    event.handle = 0x0014;
    event.uuid = g_uuidA;
    ASSERT_EQ(SsapcCacheEvent(&g_addrA, &event), NLSTK_ERRCODE_SUCCESS);

    // 句柄不属于任何服务：拒绝
    NLSTK_SsapPrty_S orphan = {0};
    orphan.handle = 0x0099;
    EXPECT_EQ(SsapcCachePrty(&g_addrA, &orphan), NLSTK_ERRCODE_FAIL);
    EXPECT_EQ(SsapcCacheMethod(&g_addrB, &method), NLSTK_ERRCODE_FAIL);
    EXPECT_EQ(SsapcCacheEvent(nullptr, &event), NLSTK_ERRCODE_POINTER_NULL);
}

/**
 * @tc.name: SsapcCacheSortAndFindSeq003
 * @tc.desc: 服务排序后按属性、方法、事件顺序推进发现游标，全部查完报告完成
 * @tc.type: FUNC
 */
HWTEST_F(UT_SSAPC_APP, SsapcCacheSortAndFindSeq003, TestSize.Level1)
{
    ASSERT_EQ(SsapcCacheCreate(&g_addrA), NLSTK_ERRCODE_SUCCESS);
    SsapCacheServInfo_S serv = {0};
    serv.handle = TEST_HANDLE;
    serv.endHandle = TEST_END_HANDLE;
    serv.uuid = g_uuidA;
    serv.memberValue = MEMBER_TYPE_PROPERTY | MEMBER_TYPE_METHOD | MEMBER_TYPE_EVENT;
    ASSERT_EQ(SsapcCacheServ(&g_addrA, &serv), NLSTK_ERRCODE_SUCCESS);

    NLSTK_SsapPrty_S props[2] = {{0}, {0}};
    props[0].handle = 0x0012;
    props[1].handle = 0x0011;
    ASSERT_EQ(SsapcCachePrty(&g_addrA, &props[0]), NLSTK_ERRCODE_SUCCESS);
    ASSERT_EQ(SsapcCachePrty(&g_addrA, &props[1]), NLSTK_ERRCODE_SUCCESS);
    NLSTK_SsapPrty_S method = {0};
    method.handle = 0x0015;
    ASSERT_EQ(SsapcCacheMethod(&g_addrA, &method), NLSTK_ERRCODE_SUCCESS);

    ASSERT_EQ(SsapcCacheSortCachedServ(&g_addrA, TEST_HANDLE), NLSTK_ERRCODE_SUCCESS);
    // 排序后服务结束句柄被改写为最大成员句柄（方法句柄 0x15）

    bool isFinish = false;
    SSAP_FindType_E findType = FIND_STRUCTURE_TYPE_PROPERTY;
    uint16_t startHandle = 0;
    uint16_t endHandle = 0;
    // 属性阶段：从已缓存最大属性句柄之后继续
    SsapcCacheGetNextFindMember(&g_addrA, &isFinish, &findType, &startHandle, &endHandle);
    EXPECT_FALSE(isFinish);
    EXPECT_EQ(findType, FIND_STRUCTURE_TYPE_PROPERTY);
    EXPECT_EQ(startHandle, 0x0013U);
    EXPECT_EQ(endHandle, 0x0015U);

    // 完成标记须传成员区间内的句柄（产品侧传上一轮发现的起始句柄）
    SsapcCacheServMemberDiscFinish(&g_addrA, FIND_STRUCTURE_TYPE_PROPERTY, 0x0013);
    // 方法句柄即区间上限：方法阶段自动完结，不发出查询
    // 事件阶段：未缓存事件，从服务起始句柄之后全扫
    SsapcCacheGetNextFindMember(&g_addrA, &isFinish, &findType, &startHandle, &endHandle);
    EXPECT_FALSE(isFinish);
    EXPECT_EQ(findType, FIND_STRUCTURE_TYPE_EVENT);
    EXPECT_EQ(startHandle, TEST_HANDLE + 1);
    EXPECT_EQ(endHandle, 0x0015U);

    SsapcCacheServMemberDiscFinish(&g_addrA, FIND_STRUCTURE_TYPE_EVENT, 0x0011);
    // 全部查完
    SsapcCacheGetNextFindMember(&g_addrA, &isFinish, &findType, &startHandle, &endHandle);
    EXPECT_TRUE(isFinish);

    // 空指针入参与未知地址安全返回
    SsapcCacheGetNextFindMember(nullptr, &isFinish, &findType, &startHandle, &endHandle);
    SsapcCacheGetNextFindMember(&g_addrB, &isFinish, &findType, &startHandle, &endHandle);
}

/**
 * @tc.name: SsapcCacheServDiscFinish004
 * @tc.desc: 服务发现完成后缓存翻转入已完成表；按通用唯一标识符发现时同标识旧记录被替换
 * @tc.type: FUNC
 */
HWTEST_F(UT_SSAPC_APP, SsapcCacheServDiscFinish004, TestSize.Level1)
{
    ASSERT_EQ(SsapcCacheCreate(&g_addrA), NLSTK_ERRCODE_SUCCESS);
    SsapCacheServInfo_S serv = {0};
    serv.handle = TEST_HANDLE;
    serv.endHandle = TEST_END_HANDLE;
    serv.uuid = g_uuidA;
    ASSERT_EQ(SsapcCacheServ(&g_addrA, &serv), NLSTK_ERRCODE_SUCCESS);
    SsapcCacheServDiscFinish(&g_addrA);

    NLSTK_SsapServ_S *services = nullptr;
    uint16_t num = 0;
    ASSERT_EQ(SsapcCacheGetServices(&g_addrA, 0, nullptr, &services, &num), NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(num, 1U);
    EXPECT_EQ(services[0].handle, TEST_HANDLE);
    SsapcCacheFreeServices(services, num);

    // 标记按标识符发现，再次完成后同标识旧记录被移除，仅保留最新一条
    SsapCacheServInfo_S servAgain = {0};
    servAgain.handle = 0x0030;
    servAgain.endHandle = 0x0040;
    servAgain.uuid = g_uuidA;
    ASSERT_EQ(SsapcCacheServ(&g_addrA, &servAgain), NLSTK_ERRCODE_SUCCESS);
    SsapcCacheSetCurFindByUuid(&g_addrA, true, &g_uuidA);
    SsapcCacheServDiscFinish(&g_addrA);
    ASSERT_EQ(SsapcCacheGetServices(&g_addrA, 0, &g_uuidA, &services, &num), NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(num, 1U);
    EXPECT_EQ(services[0].handle, 0x0030U);
    SsapcCacheFreeServices(services, num);

    bool isByUuid = false;
    NLSTK_SsapUuid_S outUuid = {0};
    ASSERT_EQ(SsapcCacheIsCurFindByUuid(&g_addrA, &isByUuid, &outUuid), NLSTK_ERRCODE_SUCCESS);
    EXPECT_TRUE(isByUuid);
    EXPECT_EQ(memcmp(&outUuid, &g_uuidA, sizeof(NLSTK_SsapUuid_S)), 0);
}

/**
 * @tc.name: SsapcCacheGetServicesFilter005
 * @tc.desc: 按标识符过滤服务列表，成员数据完整拷贝，无匹配与空缓存返回失败
 * @tc.type: FUNC
 */
HWTEST_F(UT_SSAPC_APP, SsapcCacheGetServicesFilter005, TestSize.Level1)
{
    ASSERT_EQ(SsapcCacheCreate(&g_addrA), NLSTK_ERRCODE_SUCCESS);
    SsapCacheServInfo_S serv1 = {0};
    serv1.handle = TEST_HANDLE;
    serv1.endHandle = TEST_END_HANDLE;
    serv1.uuid = g_uuidA;
    SsapCacheServInfo_S serv2 = {0};
    serv2.handle = 0x0030;
    serv2.endHandle = 0x0040;
    serv2.uuid = g_uuidB;
    ASSERT_EQ(SsapcCacheServ(&g_addrA, &serv1), NLSTK_ERRCODE_SUCCESS);
    ASSERT_EQ(SsapcCacheServ(&g_addrA, &serv2), NLSTK_ERRCODE_SUCCESS);
    NLSTK_SsapPrty_S prty = {0};
    prty.handle = 0x0012;
    prty.uuid = g_uuidA;
    ASSERT_EQ(SsapcCachePrty(&g_addrA, &prty), NLSTK_ERRCODE_SUCCESS);
    ASSERT_EQ(SsapcCacheSortCachedServ(&g_addrA, TEST_HANDLE), NLSTK_ERRCODE_SUCCESS);
    SsapcCacheServDiscFinish(&g_addrA);

    NLSTK_SsapServ_S *services = nullptr;
    uint16_t num = 0;
    ASSERT_EQ(SsapcCacheGetServices(&g_addrA, 3, &g_uuidA, &services, &num), NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(num, 1U);
    EXPECT_EQ(services[0].appId, 3);
    EXPECT_EQ(services[0].propertyNum, 1U);
    EXPECT_EQ(services[0].properties[0].handle, 0x0012U);
    SsapcCacheFreeServices(services, num);

    // 无匹配标识符与未建缓存地址失败
    NLSTK_SsapUuid_S uuidNone = {0};
    EXPECT_EQ(SsapcCacheGetServices(&g_addrA, 0, &uuidNone, &services, &num), NLSTK_ERRCODE_FAIL);
    EXPECT_EQ(SsapcCacheGetServices(&g_addrB, 0, nullptr, &services, &num), NLSTK_ERRCODE_GETRECORD_FAIL);
    EXPECT_EQ(SsapcCacheGetServices(nullptr, 0, nullptr, &services, &num), NLSTK_ERRCODE_POINTER_NULL);
}

/**
 * @tc.name: SsapcCacheGetUuidByHandle006
 * @tc.desc: 按句柄反查服务或成员的标识符，命中返回原值，未命中清零返回
 * @tc.type: FUNC
 */
HWTEST_F(UT_SSAPC_APP, SsapcCacheGetUuidByHandle006, TestSize.Level1)
{
    ASSERT_EQ(SsapcCacheCreate(&g_addrA), NLSTK_ERRCODE_SUCCESS);
    SsapCacheServInfo_S serv = {0};
    serv.handle = TEST_HANDLE;
    serv.endHandle = TEST_END_HANDLE;
    serv.uuid = g_uuidA;
    ASSERT_EQ(SsapcCacheServ(&g_addrA, &serv), NLSTK_ERRCODE_SUCCESS);
    NLSTK_SsapPrty_S prty = {0};
    prty.handle = 0x0012;
    prty.uuid = g_uuidB;
    ASSERT_EQ(SsapcCachePrty(&g_addrA, &prty), NLSTK_ERRCODE_SUCCESS);
    SsapcCacheServDiscFinish(&g_addrA);

    NLSTK_SsapUuid_S uuid = {0};
    SsapcCacheGetUuidByHandle(&g_addrA, &uuid, TEST_HANDLE);
    EXPECT_EQ(memcmp(&uuid, &g_uuidA, sizeof(NLSTK_SsapUuid_S)), 0);

    (void)memset_s(&uuid, sizeof(uuid), 0xFF, sizeof(uuid));
    SsapcCacheGetUuidByHandle(&g_addrA, &uuid, 0x0012);
    EXPECT_EQ(memcmp(&uuid, &g_uuidB, sizeof(NLSTK_SsapUuid_S)), 0);

    (void)memset_s(&uuid, sizeof(uuid), 0xFF, sizeof(uuid));
    SsapcCacheGetUuidByHandle(&g_addrA, &uuid, 0x0099);
    NLSTK_SsapUuid_S zeroUuid = {0};
    EXPECT_EQ(memcmp(&uuid, &zeroUuid, sizeof(NLSTK_SsapUuid_S)), 0);
}

/**
 * @tc.name: SsapcCacheCpcdConfig007
 * @tc.desc: 订阅配置按句柄聚合多应用设置，重复设置替换旧值，注销时清掉对应应用配置
 * @tc.type: FUNC
 */
HWTEST_F(UT_SSAPC_APP, SsapcCacheCpcdConfig007, TestSize.Level1)
{
    int32_t app1 = RegisterApp(&g_addrA);
    int32_t app2 = RegisterApp(&g_addrA);
    ASSERT_GE(app1, 0);
    ASSERT_GE(app2, 0);
    ASSERT_EQ(SsapcCacheCreate(&g_addrA), NLSTK_ERRCODE_SUCCESS);

    // 首个应用的首次设置直接生效
    EXPECT_EQ(SsapcCacheSetCpcd(app1, &g_addrA, TEST_HANDLE, CPCD_NOTIFICATION_ENABLE),
        NLSTK_ERRCODE_SUCCESS);
    SDF_Vector_S *config = SsapcCacheGetCpcdConfig(&g_addrA, TEST_HANDLE);
    ASSERT_NE(config, nullptr);
    EXPECT_EQ(config->size, 1U);

    // 同应用重复设置：旧配置被替换，仍只有一条
    EXPECT_EQ(SsapcCacheSetCpcd(app1, &g_addrA, TEST_HANDLE, CPCD_INDICATION_ENABLE),
        NLSTK_ERRCODE_DIRECT_RETURN);
    config = SsapcCacheGetCpcdConfig(&g_addrA, TEST_HANDLE);
    ASSERT_NE(config, nullptr);
    EXPECT_EQ(config->size, 1U);
    SsapCpcdCfg_S *cfg = (SsapCpcdCfg_S *)SDF_VectorElementAt(config, 0);
    EXPECT_EQ(cfg->appId, app1);
    EXPECT_EQ(cfg->cpcdVal, CPCD_INDICATION_ENABLE);

    // 第二个应用加入同一句柄
    EXPECT_EQ(SsapcCacheSetCpcd(app2, &g_addrA, TEST_HANDLE, CPCD_NOTIFICATION_ENABLE),
        NLSTK_ERRCODE_DIRECT_RETURN);
    config = SsapcCacheGetCpcdConfig(&g_addrA, TEST_HANDLE);
    ASSERT_NE(config, nullptr);
    EXPECT_EQ(config->size, 2U);

    // 应用注销后其配置被清除
    SsapcCacheCleanAppCpcd(app1, &g_addrA);
    config = SsapcCacheGetCpcdConfig(&g_addrA, TEST_HANDLE);
    ASSERT_NE(config, nullptr);
    EXPECT_EQ(config->size, 1U);
    cfg = (SsapCpcdCfg_S *)SDF_VectorElementAt(config, 0);
    EXPECT_EQ(cfg->appId, app2);

    // 未建句柄与未建缓存地址查不到配置
    EXPECT_EQ(SsapcCacheGetCpcdConfig(&g_addrA, 0x0099), nullptr);
    EXPECT_EQ(SsapcCacheGetCpcdConfig(&g_addrB, TEST_HANDLE), nullptr);
    EXPECT_EQ(SsapcCacheSetCpcd(app1, nullptr, TEST_HANDLE, 0), NLSTK_ERRCODE_POINTER_NULL);
}

/**
 * @tc.name: SsapcCacheCleanAndDestroy008
 * @tc.desc: 清空服务缓存复位发现状态与标识符标记，销毁地址缓存后查询全部落空
 * @tc.type: FUNC
 */
HWTEST_F(UT_SSAPC_APP, SsapcCacheCleanAndDestroy008, TestSize.Level1)
{
    ASSERT_EQ(SsapcCacheCreate(&g_addrA), NLSTK_ERRCODE_SUCCESS);
    SsapCacheServInfo_S serv = {0};
    serv.handle = TEST_HANDLE;
    serv.endHandle = TEST_END_HANDLE;
    serv.uuid = g_uuidA;
    ASSERT_EQ(SsapcCacheServ(&g_addrA, &serv), NLSTK_ERRCODE_SUCCESS);
    SsapcCacheSetCurFindByUuid(&g_addrA, true, &g_uuidA);
    SsapcCacheServDiscFinish(&g_addrA);

    ASSERT_EQ(SsapcCacheCleanServ(&g_addrA), NLSTK_ERRCODE_SUCCESS);
    bool isByUuid = true;
    NLSTK_SsapUuid_S uuid = {0};
    ASSERT_EQ(SsapcCacheIsCurFindByUuid(&g_addrA, &isByUuid, &uuid), NLSTK_ERRCODE_SUCCESS);
    EXPECT_FALSE(isByUuid);
    EXPECT_EQ(SsapcCacheGetMaxServHandle(&g_addrA), 0U);
    EXPECT_EQ(SsapcCacheCleanServ(&g_addrB), NLSTK_ERRCODE_GETRECORD_FAIL);
    EXPECT_EQ(SsapcCacheCleanServ(nullptr), NLSTK_ERRCODE_POINTER_NULL);

    SsapcCacheDestroy(&g_addrA);
    NLSTK_SsapServ_S *services = nullptr;
    uint16_t num = 0;
    EXPECT_EQ(SsapcCacheGetServices(&g_addrA, 0, nullptr, &services, &num), NLSTK_ERRCODE_GETRECORD_FAIL);
}

/**
 * @tc.name: SsapcClientFindReqPkt001
 * @tc.desc: 结构发现请求报文携带正确的消息码与句柄范围
 * @tc.type: FUNC
 */
HWTEST_F(UT_SSAPC_APP, SsapcClientFindReqPkt001, TestSize.Level1)
{
    SSAP_Link_S *link = SSAP_CreateSsapLink(&g_addrA, TEST_LCID, CaptureSendCb);
    ASSERT_NE(link, nullptr);

    SSAP_ParamFind_S findParam = {0};
    findParam.appId = 0;
    findParam.type = FIND_STRUCTURE_TYPE_PRIMARY_SERVICE;
    findParam.startHandle = TEST_HANDLE;
    findParam.endHandle = TEST_END_HANDLE;
    (void)memcpy_s(&findParam.addr, sizeof(SLE_Addr_S), &g_addrA, sizeof(SLE_Addr_S));
    SSAP_FindReq(0, 1000, nullptr, &findParam);

    EXPECT_GE(g_txCnt, 1);
    EXPECT_EQ(g_txOpcode, SSAP_FIND_STRUCTURE_REQ);
    ASSERT_GE(g_txLen, sizeof(SSAP_PduFindStructReq_S));
    EXPECT_EQ(g_txBuf[0], SSAP_FIND_STRUCTURE_REQ);
    uint16_t start = (uint16_t)(g_txBuf[2] | (g_txBuf[3] << 8));
    uint16_t end = (uint16_t)(g_txBuf[4] | (g_txBuf[5] << 8));
    EXPECT_EQ(start, TEST_HANDLE);
    EXPECT_EQ(end, TEST_END_HANDLE);
    SSAP_DeleteSsapLinkByAddr(&g_addrA);
}

/**
 * @tc.name: SsapcClientFindRspGuard002
 * @tc.desc: 长度不足结构发现响应头的报文被直接丢弃，不触发任务回调也不崩溃
 * @tc.type: FUNC
 */
HWTEST_F(UT_SSAPC_APP, SsapcClientFindRspGuard002, TestSize.Level1)
{
    SSAP_Link_S *link = SSAP_CreateSsapLink(&g_addrA, TEST_LCID, CaptureSendCb);
    ASSERT_NE(link, nullptr);

    // 单字节包：既够不成分片标记也被长度守卫拦下
    uint8_t raw[1] = {SSAP_FIND_STRUCTURE_RSP};
    SDF_Buff_S *buff = SDF_BuffNewWithReserve(sizeof(raw));
    ASSERT_NE(buff, nullptr);
    uint8_t *buf = SDF_BuffAppend(buff, sizeof(raw));
    ASSERT_NE(buf, nullptr);
    (void)memcpy_s(buf, sizeof(raw), raw, sizeof(raw));

    SSAPC_FindRspHandle(link, buff);
    SDF_BuffFree(buff);
    // 长度守卫触发：无任务登记、无重组残留
    EXPECT_EQ(link->curTask.buff, nullptr);
    EXPECT_EQ(link->fragCtx.reassemBuff, nullptr);
    SSAP_DeleteSsapLinkByAddr(&g_addrA);
}

/**
 * @tc.name: SsapcClientMultiReadRspErrorFill003
 * @tc.desc: 多句柄读响应只带一个错误项时按请求句柄数补齐，每个句柄逐项携带原因值
 * @tc.type: FUNC
 */
HWTEST_F(UT_SSAPC_APP, SsapcClientMultiReadRspErrorFill003, TestSize.Level1)
{
    SSAP_Link_S *link = SSAP_CreateSsapLink(&g_addrA, TEST_LCID, CaptureSendCb);
    ASSERT_NE(link, nullptr);

    const uint8_t handleNum = 3;
    uint16_t handles[handleNum] = {0x0011, 0x0012, 0x0013};
    uint32_t reqSize = sizeof(SSAP_ReadPropsInfo_S) + sizeof(uint16_t) * handleNum;
    SSAP_ReadPropsInfo_S *reqInfo = (SSAP_ReadPropsInfo_S *)SDF_MemZalloc(reqSize);
    ASSERT_NE(reqInfo, nullptr);
    reqInfo->appId = 0;
    reqInfo->type = DESC_TYPE_PROPERTY_RESERVE;
    reqInfo->num = handleNum;
    for (uint8_t i = 0; i < handleNum; i++) {
        reqInfo->handles[i] = handles[i];
    }
    (void)memcpy_s(&reqInfo->addr, sizeof(SLE_Addr_S), &g_addrA, sizeof(SLE_Addr_S));
    SSAP_ReadReqProps(0, 1000, RecReadProps, reqInfo);
    SDF_MemFree(reqInfo);
    ASSERT_GE(g_txCnt, 1);

    // 响应仅一个错误项：成功位为 0，长度字段即错误码
    uint8_t rsp[4] = {SSAP_READ_RSP, SSAP_CTRL_NO_FRAG};
    uint16_t item = SSAP_ERRCODE_FORBID_READ;
    (void)memcpy_s(&rsp[2], sizeof(item), &item, sizeof(item));
    SDF_Buff_S *rspBuff = SDF_BuffNewWithReserve(sizeof(rsp));
    ASSERT_NE(rspBuff, nullptr);
    uint8_t *rspBuf = SDF_BuffAppend(rspBuff, sizeof(rsp));
    ASSERT_NE(rspBuf, nullptr);
    (void)memcpy_s(rspBuf, sizeof(rsp), rsp, sizeof(rsp));

    SSAPC_ReadRspHandle(link, rspBuff);
    SDF_BuffFree(rspBuff);
    EXPECT_EQ(g_readPropsRec.appId, 0);
    EXPECT_EQ(g_readPropsRec.errCode, SSAP_ERRCODE_SUCCESS);
    EXPECT_EQ(g_readPropsRec.size, handleNum);
    for (uint8_t i = 0; i < handleNum; i++) {
        EXPECT_EQ(g_readPropsRec.handles[i], handles[i]);
        EXPECT_EQ(g_readPropsRec.errorCodes[i], SSAP_ERRCODE_FORBID_READ);
    }
    SSAP_DeleteSsapLinkByAddr(&g_addrA);
}

/**
 * @tc.name: SsapcClientValueInd004
 * @tc.desc: 数据指示按订阅分发属性变化并自动回复确认报文，过短报文被丢弃
 * @tc.type: FUNC
 */
HWTEST_F(UT_SSAPC_APP, SsapcClientValueInd004, TestSize.Level1)
{
    int32_t appId = RegisterApp(&g_addrA);
    ASSERT_GE(appId, 0);
    ASSERT_EQ(SsapcCacheCreate(&g_addrA), NLSTK_ERRCODE_SUCCESS);
    ASSERT_EQ(SsapcCacheSetCpcd(appId, &g_addrA, TEST_HANDLE, CPCD_INDICATION_ENABLE),
        NLSTK_ERRCODE_SUCCESS);
    SSAP_Link_S *link = SSAP_CreateSsapLink(&g_addrA, TEST_LCID, CaptureSendCb);
    ASSERT_NE(link, nullptr);

    // 正常指示：头两字节 + 句柄 + 长度 + 两个数据字节，控制码标记非分片的属性指示
    uint8_t raw[8] = {SSAP_VALUE_IND, SSAP_CTRL_NO_FRAG};
    uint16_t handle = TEST_HANDLE;
    uint16_t len = 2;
    (void)memcpy_s(&raw[2], sizeof(handle), &handle, sizeof(handle));
    (void)memcpy_s(&raw[4], sizeof(len), &len, sizeof(len));
    raw[6] = 0x11;
    raw[7] = 0x22;
    SDF_Buff_S *buff = SDF_BuffNewWithReserve(sizeof(raw));
    ASSERT_NE(buff, nullptr);
    uint8_t *buf = SDF_BuffAppend(buff, sizeof(raw));
    ASSERT_NE(buf, nullptr);
    (void)memcpy_s(buf, sizeof(raw), raw, sizeof(raw));

    SSAPC_ValueIndHandle(link, buff);
    SDF_BuffFree(buff);
    EXPECT_EQ(g_propCnt, 1);
    EXPECT_EQ(g_propRec[0].appId, appId);
    EXPECT_EQ(g_propRec[0].handle, TEST_HANDLE);
    EXPECT_EQ(g_propRec[0].len, 2U);
    EXPECT_EQ(g_propRec[0].firstByte, 0x11);
    // 指示处理后回一次确认报文
    EXPECT_GE(g_txCnt, 1);

    // 过短报文：直接丢弃
    ResetRecords();
    uint8_t shortRaw[4] = {SSAP_VALUE_IND, SSAP_CTRL_NO_FRAG, 0, 0};
    SDF_Buff_S *shortBuff = SDF_BuffNewWithReserve(sizeof(shortRaw));
    ASSERT_NE(shortBuff, nullptr);
    uint8_t *shortBuf = SDF_BuffAppend(shortBuff, sizeof(shortRaw));
    ASSERT_NE(shortBuf, nullptr);
    (void)memcpy_s(shortBuf, sizeof(shortRaw), shortRaw, sizeof(shortRaw));
    SSAPC_ValueIndHandle(link, shortBuff);
    SDF_BuffFree(shortBuff);
    EXPECT_EQ(g_propCnt, 0);
    SSAP_DeleteSsapLinkByAddr(&g_addrA);
}

/**
 * @tc.name: SsapUtilsUuid001
 * @tc.desc: 标识符相等判定覆盖双空、单空、内容相等与不等四类输入
 * @tc.type: FUNC
 */
HWTEST_F(UT_SSAPC_APP, SsapUtilsUuid001, TestSize.Level1)
{
    EXPECT_TRUE(SSAP_IsUuidEqual(nullptr, nullptr));
    EXPECT_FALSE(SSAP_IsUuidEqual(nullptr, &g_uuidA));
    EXPECT_FALSE(SSAP_IsUuidEqual(&g_uuidA, nullptr));
    EXPECT_TRUE(SSAP_IsUuidEqual(&g_uuidA, &g_uuidA));
    EXPECT_FALSE(SSAP_IsUuidEqual(&g_uuidA, &g_uuidB));
}

/**
 * @tc.name: SsapUtilsUuidStd002
 * @tc.desc: 标准标识符判定要求前缀逐字节匹配标准基，任一前缀字节不同即判非标准
 * @tc.type: FUNC
 */
HWTEST_F(UT_SSAPC_APP, SsapUtilsUuidStd002, TestSize.Level1)
{
    // 用短源数据借道填充出标准基标识符
    NLSTK_SsapUuid_S stdUuid = {0};
    uint8_t shortSrc[2] = {0};
    SSAP_GetUuidFromPktBuf(&stdUuid, shortSrc, sizeof(shortSrc));
    EXPECT_TRUE(SSAP_CheckUuidStd(&stdUuid));

    // 前缀之外的字节不影响标准判定
    stdUuid.uuid[15] = 0xEE;
    EXPECT_TRUE(SSAP_CheckUuidStd(&stdUuid));

    // 任意前缀字节不同即非标准
    NLSTK_SsapUuid_S broken = stdUuid;
    broken.uuid[2] = static_cast<uint8_t>(~broken.uuid[2]);
    EXPECT_FALSE(SSAP_CheckUuidStd(&broken));
}

/**
 * @tc.name: SsapUtilsItemType003
 * @tc.desc: 描述符个数长度只对服务、属性、方法、事件类条目生效，服务引用类条目取句柄对长度
 * @tc.type: FUNC
 */
HWTEST_F(UT_SSAPC_APP, SsapUtilsItemType003, TestSize.Level1)
{
    EXPECT_EQ(GetDescriptorCountLenByItemType(ITEM_TYPE_STD_PRIMARY_SERVICE), 1U);
    EXPECT_EQ(GetDescriptorCountLenByItemType(ITEM_TYPE_STD_PROPERTY), 1U);
    EXPECT_EQ(GetDescriptorCountLenByItemType(ITEM_TYPE_STD_METHOD), 1U);
    EXPECT_EQ(GetDescriptorCountLenByItemType(ITEM_TYPE_STD_EVENT), 1U);
    EXPECT_EQ(GetDescriptorCountLenByItemType(ITEM_TYPE_VENDOR_PROPERTY), 1U);
    EXPECT_EQ(GetDescriptorCountLenByItemType(ITEM_TYPE_STD_SERVICE_REFERENCE), 0U);
    EXPECT_EQ(GetDescriptorCountLenByItemType(ITEM_TYPE_VENDOR_SERVICE_REFERENCE), 0U);

    EXPECT_EQ(GetStartEndHandleLenByItemType(ITEM_TYPE_STD_SERVICE_REFERENCE),
        SSAP_HANDLE_LEN + SSAP_HANDLE_LEN);
    EXPECT_EQ(GetStartEndHandleLenByItemType(ITEM_TYPE_VENDOR_SERVICE_REFERENCE),
        SSAP_HANDLE_LEN + SSAP_HANDLE_LEN);
    EXPECT_EQ(GetStartEndHandleLenByItemType(ITEM_TYPE_STD_PROPERTY), 0U);
}

/**
 * @tc.name: SsapHandleAllocator001
 * @tc.desc: 句柄分配支持部分占用、恰好占满与不足失败，释放后相邻区间合并回连续空闲
 * @tc.type: FUNC
 */
HWTEST_F(UT_SSAPC_APP, SsapHandleAllocator001, TestSize.Level1)
{
    // SetUp 的 SSAP_Init 已随服务端初始化创建分配器，先销毁产品实例再用受控容量重建
    if (g_hdlAlloc != nullptr) {
        SSAP_DestroyHandleAllocator();
    }
    ASSERT_TRUE(SSAP_InitHandleAllocator(4));
    SSAP_HandleRange_S a = {0};
    SSAP_HandleRange_S b = {0};
    SSAP_HandleRange_S c = {0};

    // 低段连切三块：部分占用从空闲块头部切走
    ASSERT_TRUE(SSAP_HandleAllocate(3, &a));
    EXPECT_EQ(a.start, SSAP_HANDLE_MIN);
    EXPECT_EQ(a.end, SSAP_HANDLE_MIN + 2);
    ASSERT_TRUE(SSAP_HandleAllocate(3, &b));
    ASSERT_TRUE(SSAP_HandleAllocate(4, &c));
    EXPECT_EQ(c.start, SSAP_HANDLE_MIN + 6);

    // 超过总容量：失败
    SSAP_HandleRange_S tooBig = {0};
    EXPECT_FALSE(SSAP_HandleAllocate(SSAP_HANDLE_MAX, &tooBig));

    // 先释放中段再释放首段：首段插入后与中段右合并
    ASSERT_TRUE(SSAP_HandleRelease(b));
    ASSERT_TRUE(SSAP_HandleRelease(a));
    // 释放尾段：左合并后与剩余空闲块级联合并，整段回收
    ASSERT_TRUE(SSAP_HandleRelease(c));

    // 全部回收后可一次性分配大块
    SSAP_HandleRange_S merged = {0};
    ASSERT_TRUE(SSAP_HandleAllocate(10, &merged));
    EXPECT_EQ(merged.start, SSAP_HANDLE_MIN);
    EXPECT_EQ(merged.end, SSAP_HANDLE_MIN + 9);

    // 逆序区间拒绝
    SSAP_HandleRange_S badRange = {5, 1};
    EXPECT_FALSE(SSAP_HandleRelease(badRange));
    SSAP_DestroyHandleAllocator();
}

/**
 * @tc.name: SsapHandleAllocatorGrow002
 * @tc.desc: 空闲块数组不足时自动倍增容量；扩容分配失败时释放返回失败不崩溃
 * @tc.type: FUNC
 */
HWTEST_F(UT_SSAPC_APP, SsapHandleAllocatorGrow002, TestSize.Level1)
{
    // SetUp 的 SSAP_Init 已随服务端初始化创建分配器，先销毁产品实例再用受控容量重建
    if (g_hdlAlloc != nullptr) {
        SSAP_DestroyHandleAllocator();
    }
    ASSERT_TRUE(SSAP_InitHandleAllocator(1));
    SSAP_HandleRange_S range = {0};
    ASSERT_TRUE(SSAP_HandleAllocate(0xFFFF - SSAP_HANDLE_MIN + 1, &range));

    // 连续释放多个不相邻区间，迫使空闲块数组扩容
    SSAP_HandleRange_S first = {0x0100, 0x0101};
    SSAP_HandleRange_S second = {0x0200, 0x0201};
    SSAP_HandleRange_S third = {0x0300, 0x0301};
    ASSERT_TRUE(SSAP_HandleRelease(first));
    ASSERT_TRUE(SSAP_HandleRelease(second));
    ASSERT_TRUE(SSAP_HandleRelease(third));
    EXPECT_EQ(g_hdlAlloc->size, 3U);
    EXPECT_GE(g_hdlAlloc->capacity, 3U);
    SSAP_DestroyHandleAllocator();

    // 扩容分配失败：释放返回失败
    ASSERT_TRUE(SSAP_InitHandleAllocator(1));
    ASSERT_TRUE(SSAP_HandleAllocate(0xFFFF - SSAP_HANDLE_MIN + 1, &range));
    ASSERT_TRUE(SSAP_HandleRelease(first));
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc)
        .WillOnce(TEST_SDF_MemZallocFail)
        .WillRepeatedly(TEST_SDF_MemZalloc);
    EXPECT_FALSE(SSAP_HandleRelease(second));
    SSAP_DestroyHandleAllocator();
}

/**
 * @tc.name: SsapLinkTableCrud001
 * @tc.desc: 链路表支持按地址与链路号查询，删除后查不到
 * @tc.type: FUNC
 */
HWTEST_F(UT_SSAPC_APP, SsapLinkTableCrud001, TestSize.Level1)
{
    SSAP_Link_S *link = SSAP_CreateSsapLink(&g_addrA, TEST_LCID, CaptureSendCb);
    ASSERT_NE(link, nullptr);
    EXPECT_EQ(link->status, SSAP_LINK_IDLE);

    EXPECT_EQ(SSAP_FindSsapLinkByAddr(&g_addrA), link);
    EXPECT_EQ(SSAP_FindSsapLinkByAddr(&g_addrB), nullptr);
    EXPECT_EQ(SSAP_FindSsapLinkByLcid(TEST_LCID), link);
    EXPECT_EQ(SSAP_FindSsapLinkByLcid(TEST_LCID + 1), nullptr);

    SSAP_DeleteSsapLinkByAddr(&g_addrA);
    EXPECT_EQ(SSAP_FindSsapLinkByAddr(&g_addrA), nullptr);
    EXPECT_EQ(SSAP_FindSsapLinkByLcid(TEST_LCID), nullptr);
}

/**
 * @tc.name: SsapcCbRefCount001
 * @tc.desc: 内容相同的回调注册两次共用同一实例，引用计数归零后实例被回收
 * @tc.type: FUNC
 */
HWTEST_F(UT_SSAPC_APP, SsapcCbRefCount001, TestSize.Level1)
{
    // 附加独有钩子使本用例的回调内容区别于注册助手，引用计数不受其他用例影响
    NLSTK_SsapAppClientCb_S cb;
    FillClientCb(&cb);
    cb.onFindService = [](int32_t appId, NLSTK_Errcode_E ret) { (void)appId; (void)ret; };

    NLSTK_SsapAppClientCb_S *inst1 = SsapcCbGet(&cb);
    ASSERT_NE(inst1, nullptr);
    NLSTK_SsapAppClientCb_S *inst2 = SsapcCbGet(&cb);
    EXPECT_EQ(inst1, inst2);

    // 引用计数归零：实例被回收，空指针入参安全返回
    SsapcCbDestroy(inst1);
    SsapcCbDestroy(inst1);

    NLSTK_SsapAppClientCb_S *inst3 = SsapcCbGet(&cb);
    ASSERT_NE(inst3, nullptr);
    EXPECT_EQ(SsapcCbGet(nullptr), nullptr);
    SsapcCbDestroy(inst3);
    SsapcCbDestroy(nullptr);
}

/**
 * @tc.name: SsapManagerTaskAllocFail001
 * @tc.desc: 任务参数分配失败时立即释放入参并跳过执行，不残留任务到链路
 * @tc.type: FUNC
 */
HWTEST_F(UT_SSAPC_APP, SsapManagerTaskAllocFail001, TestSize.Level1)
{
    SSAP_Link_S *link = SSAP_CreateSsapLink(&g_addrA, TEST_LCID, CaptureSendCb);
    ASSERT_NE(link, nullptr);

    SSAP_TaskParam_S param = {0};
    param.appId = 0;
    param.arg = SDF_MemZalloc(4);
    ASSERT_NE(param.arg, nullptr);
    param.freeFunc = [](void *arg) {
        g_freeFuncCnt++;
        SDF_MemFree(arg);
    };

    EXPECT_CALL(sdfMemMock, SDF_MemZalloc)
        .WillOnce(TEST_SDF_MemZallocFail)
        .WillRepeatedly(TEST_SDF_MemZalloc);
    SSAP_ProcessRequestTask(link, &param, false);
    EXPECT_EQ(g_freeFuncCnt, 1);
    SSAP_DeleteSsapLinkByAddr(&g_addrA);
}
