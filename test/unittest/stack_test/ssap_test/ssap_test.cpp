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
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <iostream>
#include "ssaps_server.h"
#include "ssapc_client.h"
#include "ssaps_server_api.h"
#include "sdf_buff.h"
#include "sdf_mem.h"
#include "ssaps_service.h"
#include "ssap_manager.h"
#include "ssap_utils.h"
#include "stack_ssap_mock.h"
#include "stack_ssap_stub.h"
#include "cpfwk_log.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

extern "C" {
    #include "ssap_link_state.c"
    #include "ssap_handle.c"
    #include "ssap_manager.c"
    #include "ssap_utils.c"
    #include "ssapc_app_link_sm.c"
    void SSAP_LinkAddCurrentTask(SSAP_Link_S *link, SSAP_TaskParam_S *param);
    void SSAP_CleanLinkTaskByAppId(SSAP_Link_S *link, int32_t appId);
    void SSAP_ErrorRspHandle(SSAP_Link_S *link, SDF_Buff_S *sdfBuff);
}

class UT_SSAP_TEST : public testing::Test {
public:
    NiceMock<SsapMock> ssapMock;
protected:
    void SetUp() override
    {
        EXPECT_CALL(ssapMock, SDF_CreateVectorByCapacity).WillRepeatedly(TEST_SDF_CreateVectorByCapacity);
        EXPECT_CALL(ssapMock, SDF_CreateVector).WillRepeatedly(TEST_SDF_CreateVector);
        EXPECT_CALL(ssapMock, SDF_DestroyVector).WillRepeatedly(TEST_SDF_DestroyVector);
        EXPECT_CALL(ssapMock, SDF_CleanVector).WillRepeatedly(TEST_SDF_CleanVector);
        EXPECT_CALL(ssapMock, SDF_VectorEmplaceBack).WillRepeatedly(TEST_SDF_VectorEmplaceBack);
        EXPECT_CALL(ssapMock, SDF_VectorRemove).WillRepeatedly(TEST_SDF_VectorRemove);
        EXPECT_CALL(ssapMock, SDF_VectorRemoveLast).WillRepeatedly(TEST_SDF_VectorRemoveLast);
        EXPECT_CALL(ssapMock, SDF_VectorFindFirstByStartIndex).WillRepeatedly(TEST_SDF_VectorFindFirstByStartIndex);
        EXPECT_CALL(ssapMock, SDF_VectorFindFirst).WillRepeatedly(TEST_SDF_VectorFindFirst);
        EXPECT_CALL(ssapMock, SDF_VectorElementAt).WillRepeatedly(TEST_SDF_VectorElementAt);
        EXPECT_CALL(ssapMock, SDF_VectorPopElement).WillRepeatedly(TEST_SDF_VectorPopElement);
        EXPECT_CALL(ssapMock, SDF_VectorSort).WillRepeatedly(TEST_SDF_VectorSort);
        SSAP_ServerInit();
        SSAP_LinkInit();
    }

    void TearDown() override
    {
        SSAP_ServerDeInit();
    }
};

void SendCbStub(SSAP_Link_S *link, SDF_Buff_S *buff, uint8_t opcode)
{
    return;
}

TEST_F(UT_SSAP_TEST, SSAP_LinkAddCurrentTask)
{
    SSAP_Link_S link;
    memset_s(&link, sizeof(SSAP_Link_S), 0, sizeof(SSAP_Link_S));
    SSAP_TaskParam_S *para = (SSAP_TaskParam_S *)SDF_MemZalloc(sizeof(SSAP_TaskParam_S));
    link.curTask.param = para;
    link.timerHandle = SSAP_TIMER_NO_USED_HANDLE;
    SSAP_LinkAddCurrentTask(&link, para);
    EXPECT_EQ(link.curTask.param, para);
}

TEST_F(UT_SSAP_TEST, SSAP_DelTimer)
{
    SSAP_Link_S link;
    link.timerHandle = 0;
    SSAP_DelTimer(&link);
    EXPECT_EQ(link.timerHandle, SSAP_TIMER_NO_USED_HANDLE);
}

TEST_F(UT_SSAP_TEST, SsapTaskGetAppId)
{
    SSAP_Link_S link;
    int32_t ret;
    SSAP_TaskParam_S para = {0};
    para.appId = 1;
    link.timerHandle = 0;
    link.curTask.param = &para;
    ret = SsapTaskGetAppId(NULL);
    EXPECT_EQ(ret, SSAP_APP_INVALID_ID);
    ret = SsapTaskGetAppId(&link);
    EXPECT_EQ(ret, 1);
}

TEST_F(UT_SSAP_TEST, SsapLinkHandleTryConnect)
{
    SsapcAppLinkStateManager_S man;
    NLSTK_ConnParam_S para;
    memset_s(&man, sizeof(SsapcAppLinkStateManager_S), 0, sizeof(SsapcAppLinkStateManager_S));
    memset_s(&para, sizeof(NLSTK_ConnParam_S), 0, sizeof(NLSTK_ConnParam_S));
    SsapLinkHandleTryConnect(&man, &para);
}

TEST_F(UT_SSAP_TEST, SSAP_HandleAllocatorGrow)
{
    g_hdlAlloc->capacity = 1;
    g_hdlAlloc->blocks = NULL;
    bool ret = SSAP_HandleAllocatorGrow();
    EXPECT_EQ(ret, true);
}

TEST_F(UT_SSAP_TEST, SSAP_TimeoutCbk)
{
    SSAP_TimeoutCbk(NULL);
    SSAP_Link_S link;
    SSAP_TaskParam_S *para = (SSAP_TaskParam_S *)SDF_MemZalloc(sizeof(SSAP_TaskParam_S));
    memset_s(&link, sizeof(SSAP_Link_S), 0, sizeof(SSAP_Link_S));
    link.curTask.param = NULL;
    SSAP_TimeoutCbk(&link);
    CP_LOG_ERROR("**** SSAP_TimeoutCbk 1");

    para->valid = true;
    link.curTask.param = para;
    link.hasInitReqTask = true;
    link.curTask.opcode = SSAP_EXCHANGE_INFO_REQ;
    SSAP_TimeoutCbk(&link);
    CP_LOG_ERROR("**** SSAP_TimeoutCbk 2");

    if (link.curTask.param == NULL) {
        link.curTask.param = (SSAP_TaskParam_S *)SDF_MemZalloc(sizeof(SSAP_TaskParam_S));
        link.curTask.param->valid = true;
    }
    link.curTask.opcode = SSAP_FIND_STRUCTURE_REQ;
    SSAP_TimeoutCbk(&link);
    CP_LOG_ERROR("**** SSAP_TimeoutCbk 3");

    if (link.curTask.param == NULL) {
        link.curTask.param = (SSAP_TaskParam_S *)SDF_MemZalloc(sizeof(SSAP_TaskParam_S));
        link.curTask.param->valid = true;
    }
    link.curTask.opcode = SSAP_READ_REQ;
    SSAP_TimeoutCbk(&link);
    CP_LOG_ERROR("**** SSAP_TimeoutCbk 4");

    if (link.curTask.param == NULL) {
        link.curTask.param = (SSAP_TaskParam_S *)SDF_MemZalloc(sizeof(SSAP_TaskParam_S));
        link.curTask.param->valid = true;
    }
    link.curTask.opcode = SSAP_READ_BY_UUID_REQ;
    SSAP_TimeoutCbk(&link);
    CP_LOG_ERROR("**** SSAP_TimeoutCbk 5");

    if (link.curTask.param == NULL) {
        link.curTask.param = (SSAP_TaskParam_S *)SDF_MemZalloc(sizeof(SSAP_TaskParam_S));
        link.curTask.param->valid = true;
    }
    link.curTask.opcode = SSAP_CALL_METHOD_REQ;
    SSAP_TimeoutCbk(&link);
    CP_LOG_ERROR("**** SSAP_TimeoutCbk 6");

    if (link.curTask.param == NULL) {
        link.curTask.param = (SSAP_TaskParam_S *)SDF_MemZalloc(sizeof(SSAP_TaskParam_S));
        link.curTask.param->valid = true;
    }
    link.curTask.opcode = SSAP_VALUE_IND;
    SSAP_TimeoutCbk(&link);
    CP_LOG_ERROR("**** SSAP_TimeoutCbk 7");

    if (link.curTask.param == NULL) {
        link.curTask.param = (SSAP_TaskParam_S *)SDF_MemZalloc(sizeof(SSAP_TaskParam_S));
        link.curTask.param->valid = true;
    }
    link.curTask.opcode = SSAP_CODE_MAX;
    SSAP_TimeoutCbk(&link);
    CP_LOG_ERROR("**** SSAP_TimeoutCbk 8");
}

TEST_F(UT_SSAP_TEST, SSAP_PduErrorRsp)
{
    SSAP_Link_S link;
    memset_s(&link, sizeof(SSAP_Link_S), 0, sizeof(SSAP_Link_S));
    link.sendFunc = SendCbStub;

    SSAP_PduErrorRsp(&link, 0, 0, 0);
}

TEST_F(UT_SSAP_TEST, SSAP_ErrorRspHandle)
{
    SSAP_Link_S link;
    SDF_Buff_S *buff = (SDF_Buff_S *)SDF_MemZalloc(sizeof(SDF_Buff_S) + sizeof(SSAP_PduErrRsp_S));
    SSAP_PduErrRsp_S rsp;
    memset_s(&link, sizeof(SSAP_Link_S), 0, sizeof(SSAP_Link_S));
    memset_s(&rsp, sizeof(SSAP_PduErrRsp_S), 0, sizeof(SSAP_PduErrRsp_S));

    buff->dataLen = UINT64_MAX;
    SSAP_ErrorRspHandle(&link, buff);

    buff->dataLen = 0;
    SSAP_ErrorRspHandle(&link, buff);

    buff->dataLen = sizeof(SSAP_PduErrRsp_S);
    memcpy_s(buff->buff, sizeof(SSAP_PduErrRsp_S), &rsp, sizeof(SSAP_PduErrRsp_S));
    rsp.msgCodeReq = link.curTask.opcode - 1;
    SSAP_ErrorRspHandle(&link, buff);

    rsp.msgCodeReq = SSAP_READ_REQ;
    memcpy_s(buff->buff, sizeof(SSAP_PduErrRsp_S), &rsp, sizeof(SSAP_PduErrRsp_S));
    link.curTask.opcode = SSAP_READ_REQ;
    SSAP_ErrorRspHandle(&link, buff);

    rsp.msgCodeReq = SSAP_VALUE_IND;
    memcpy_s(buff->buff, sizeof(SSAP_PduErrRsp_S), &rsp, sizeof(SSAP_PduErrRsp_S));
    link.curTask.opcode = SSAP_VALUE_IND;
    SSAP_ErrorRspHandle(&link, buff);

    rsp.msgCodeReq = SSAP_READ_BY_UUID_REQ;
    memcpy_s(buff->buff, sizeof(SSAP_PduErrRsp_S), &rsp, sizeof(SSAP_PduErrRsp_S));
    link.curTask.opcode = SSAP_READ_BY_UUID_REQ;
    SSAP_ErrorRspHandle(&link, buff);

    rsp.msgCodeReq = SSAP_CALL_METHOD_REQ;
    memcpy_s(buff->buff, sizeof(SSAP_PduErrRsp_S), &rsp, sizeof(SSAP_PduErrRsp_S));
    link.curTask.opcode = SSAP_CALL_METHOD_REQ;
    SSAP_ErrorRspHandle(&link, buff);

    rsp.msgCodeReq = SSAP_CODE_MAX;
    memcpy_s(buff->buff, sizeof(SSAP_PduErrRsp_S), &rsp, sizeof(SSAP_PduErrRsp_S));
    link.curTask.opcode = SSAP_CODE_MAX;
    SSAP_ErrorRspHandle(&link, buff);
}

TEST_F(UT_SSAP_TEST, SSAP_IsUuidEqual)
{
    NLSTK_SsapUuid_S uuid, target;
    memset_s(&uuid, sizeof(NLSTK_SsapUuid_S), 0, sizeof(NLSTK_SsapUuid_S));
    memset_s(&target, sizeof(NLSTK_SsapUuid_S), 0, sizeof(NLSTK_SsapUuid_S));

    bool ret = SSAP_IsUuidEqual(NULL, NULL);
    EXPECT_EQ(ret, true);
    ret = SSAP_IsUuidEqual(&uuid, NULL);
    EXPECT_EQ(ret, false);
}

TEST_F(UT_SSAP_TEST, SSAP_GetOpcodeType)
{
    uint8_t ret = SSAP_GetOpcodeType(SSAP_CODE_MAX);
    EXPECT_EQ(ret, SSAP_TRANS_INVALID);
}

TEST_F(UT_SSAP_TEST, SSAP_SwapBuffer)
{
    SSAP_SwapBuffer(NULL, 0, NULL, 1);
}

TEST_F(UT_SSAP_TEST, IsAppIdValid)
{
    bool ret = IsAppIdValid(NLSTK_SSAP_CLIENT_APP_MAX_NUM);
    EXPECT_EQ(ret, false);
}

extern SsapcAppRegParam_S *g_regList[NLSTK_SSAP_CLIENT_APP_MAX_NUM];

TEST_F(UT_SSAP_TEST, SsapcAppReadProps)
{
    SSAP_ReadPropsInfo_S para;

    SsapcAppReadProps(NULL);

    para.appId = NLSTK_SSAP_CLIENT_APP_MAX_NUM;
    SsapcAppReadProps(&para);

    g_regList[0] = (SsapcAppRegParam_S *)SDF_MemZalloc(sizeof(SsapcAppRegParam_S));
    para.appId = 0;
    SsapcAppReadProps(&para);
}

TEST_F(UT_SSAP_TEST, SsapcAppWriteReq)
{
    NLSTK_SsapClientWriteBaseInfo_S para;
    memset_s(&para, sizeof(NLSTK_SsapClientWriteBaseInfo_S), 0, sizeof(NLSTK_SsapClientWriteBaseInfo_S));

    SsapcAppWriteReq(NULL);

    SsapcAppWriteReq(&para);

    g_regList[0] = (SsapcAppRegParam_S *)SDF_MemZalloc(sizeof(SsapcAppRegParam_S));
    SsapcAppWriteReq(&para);
}

TEST_F(UT_SSAP_TEST, SsapAppClientLinkConnectedInIdleState)
{
    g_regList[0] = (SsapcAppRegParam_S *)SDF_MemZalloc(sizeof(SsapcAppRegParam_S));

    g_regList[0]->nextOperator = SSAP_USER_CONNECT;
    SsapAppClientLinkConnectedInIdleState(0, 0);

    g_regList[0]->nextOperator = SSAP_LINK_CHANGE_EVENT_BUTT;
    SsapAppClientLinkConnectedInIdleState(0, 0);
}

TEST_F(UT_SSAP_TEST, SsapAppClientLinkDisconnectedInIdleState)
{
    g_regList[0] = (SsapcAppRegParam_S *)SDF_MemZalloc(sizeof(SsapcAppRegParam_S));

    g_regList[0]->nextOperator = SSAP_USER_CONNECT;
    SsapAppClientLinkDisconnectedInIdleState(0, 0);

    g_regList[0]->nextOperator = SSAP_LINK_CHANGE_EVENT_BUTT;
    SsapAppClientLinkDisconnectedInIdleState(0, 0);
}

TEST_F(UT_SSAP_TEST, SsapAppClientUserConnectInConnectingState)
{
    g_regList[0] = (SsapcAppRegParam_S *)SDF_MemZalloc(sizeof(SsapcAppRegParam_S));

    g_regList[0]->nextOperator = SSAP_USER_DISCONNECT;
    SsapAppClientUserConnectInConnectingState(0, 0);

    g_regList[0]->nextOperator = SSAP_USER_CONNECT;
    SsapAppClientUserConnectInConnectingState(0, 0);
}

TEST_F(UT_SSAP_TEST, SsapAppClientUserDisconnectInConnectingState)
{
    g_regList[0] = (SsapcAppRegParam_S *)SDF_MemZalloc(sizeof(SsapcAppRegParam_S));

    g_regList[0]->nextOperator = SSAP_USER_CONNECT;
    SsapAppClientUserDisconnectInConnectingState(0, 0);
}

TEST_F(UT_SSAP_TEST, SsapAppClientLinkConnectedInConnectingState)
{
    g_regList[0] = (SsapcAppRegParam_S *)SDF_MemZalloc(sizeof(SsapcAppRegParam_S));

    g_regList[0]->nextOperator = SSAP_USER_CONNECT;
    SsapAppClientLinkConnectedInConnectingState(0, 0);
}

TEST_F(UT_SSAP_TEST, SsapAppClientLinkDisconnectedInConnectingState)
{
    g_regList[0] = (SsapcAppRegParam_S *)SDF_MemZalloc(sizeof(SsapcAppRegParam_S));

    g_regList[0]->nextOperator = SSAP_USER_CONNECT;
    SsapAppClientLinkDisconnectedInConnectingState(0, 0);
}

TEST_F(UT_SSAP_TEST, SsapAppClientUserConnectInConnectedState)
{
    SsapAppClientUserConnectInConnectedState(0, 0);
}

TEST_F(UT_SSAP_TEST, SsapAppClientLinkConnectedInConnectedState)
{
    g_regList[0] = (SsapcAppRegParam_S *)SDF_MemZalloc(sizeof(SsapcAppRegParam_S));

    g_regList[0]->nextOperator = SSAP_USER_CONNECT;
    SsapAppClientLinkDisconnectedInConnectingState(0, 0);
}

TEST_F(UT_SSAP_TEST, SsapAppClientLinkDisconnectedInConnectedState)
{
    g_regList[0] = (SsapcAppRegParam_S *)SDF_MemZalloc(sizeof(SsapcAppRegParam_S));

    g_regList[0]->nextOperator = SSAP_USER_CONNECT;
    SsapAppClientLinkDisconnectedInConnectedState(0, 0);
}

TEST_F(UT_SSAP_TEST, SsapAppClientUserConnectInDisconnectingState)
{
    g_regList[0] = (SsapcAppRegParam_S *)SDF_MemZalloc(sizeof(SsapcAppRegParam_S));

    g_regList[0]->nextOperator = SSAP_LINK_CHANGE_EVENT_BUTT;
    SsapAppClientUserConnectInDisconnectingState(0, 0);

    g_regList[0]->nextOperator = SSAP_USER_CONNECT;
    SsapAppClientUserConnectInDisconnectingState(0, 0);

    g_regList[0]->nextOperator = SSAP_USER_DISCONNECT;
    SsapAppClientUserConnectInDisconnectingState(0, 0);

    g_regList[0]->nextOperator = SSAP_LOGIC_LINK_CONNECTED;
    SsapAppClientUserConnectInDisconnectingState(0, 0);
}

TEST_F(UT_SSAP_TEST, SsapAppClientUserDisconnectInDisconnectingState)
{
    g_regList[0] = (SsapcAppRegParam_S *)SDF_MemZalloc(sizeof(SsapcAppRegParam_S));

    g_regList[0]->nextOperator = SSAP_LINK_CHANGE_EVENT_BUTT;
    SsapAppClientUserDisconnectInDisconnectingState(0, 0);

    g_regList[0]->nextOperator = SSAP_USER_CONNECT;
    SsapAppClientUserDisconnectInDisconnectingState(0, 0);

    g_regList[0]->nextOperator = SSAP_USER_DISCONNECT;
    SsapAppClientUserDisconnectInDisconnectingState(0, 0);

    g_regList[0]->nextOperator = SSAP_LOGIC_LINK_CONNECTED;
    SsapAppClientUserDisconnectInDisconnectingState(0, 0);
}

TEST_F(UT_SSAP_TEST, SsapAppClientLinkConnectedInDisconnectingState)
{
    SsapAppClientLinkConnectedInDisconnectingState(0, 0);
}

TEST_F(UT_SSAP_TEST, SsapAppClientLinkDisconnectedInDisconnectingState)
{
    g_regList[0] = (SsapcAppRegParam_S *)SDF_MemZalloc(sizeof(SsapcAppRegParam_S));

    g_regList[0]->nextOperator = SSAP_USER_CONNECT;
    SsapAppClientLinkDisconnectedInDisconnectingState(0, 0);
}

TEST_F(UT_SSAP_TEST, SsapAppClientUserConnectInDisconnectedState)
{
    SsapAppClientUserConnectInDisconnectedState(0, 0);
}

TEST_F(UT_SSAP_TEST, SsapAppClientUserDisconnectInDisconnectedState)
{
    SsapAppClientUserDisconnectInDisconnectedState(0, 0);
}

TEST_F(UT_SSAP_TEST, SsapAppClientLinkConnectedInDisconnectedState)
{
    g_regList[0] = (SsapcAppRegParam_S *)SDF_MemZalloc(sizeof(SsapcAppRegParam_S));

    g_regList[0]->nextOperator = SSAP_USER_CONNECT;
    SsapAppClientLinkConnectedInDisconnectedState(0, 0);
}

TEST_F(UT_SSAP_TEST, SsapAppClientLinkDisconnectedInDisconnectedState)
{
    g_regList[0] = (SsapcAppRegParam_S *)SDF_MemZalloc(sizeof(SsapcAppRegParam_S));

    g_regList[0]->nextOperator = SSAP_USER_CONNECT;
    SsapAppClientLinkDisconnectedInDisconnectedState(0, 0);
}
