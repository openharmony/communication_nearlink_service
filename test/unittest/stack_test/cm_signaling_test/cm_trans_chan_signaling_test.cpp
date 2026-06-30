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

#include <cstdint>

#include "gtest/gtest.h"

#include "cp_errno_base.h"
#include "cp_worker.h"
#include "cm_errno.h"
#include "cm_logic_link_api.h"
#include "cm_signaling_manage.h"
#include "cm_signaling_struct.h"
#include "cm_signaling_trans_channel.h"
#include "sdf_errno_base.h"
#include "sdf_evc.h"
#include "sdf_mem.h"
#include "sdf_thread.h"
#include "nlstk_schedule.h"
#include "CP_Timer_mocker.h"

#define STACK_FUZZ_THREAD_NUM 5

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

static uint32_t CM_SendSignalingDataCbkUTStub(uint8_t pi, uint8_t tcid,
    uint16_t lcid, SDF_Buff_S *buff)
{
    if (buff == NULL) {
        return CM_SUCCESS;
    }
    SDF_BuffFree(buff);
    return CM_SUCCESS;
}

class UT_TRANS_CHAN_SIGNALING : public testing::Test {
protected:
    // SetUP 在每一个 TEST_F 测试开始前执行一次
    virtual void SetUp()
    {}

    // TearDown 在每一个 TEST_F 测试完成后执行一次
    virtual void TearDown()
    {}

    // SetUpTestCase 在所有 TEST_F 测试开始前执行一次
    static void SetUpTestCase()
    {
        EXPECT_EQ(SDF_ThreadInit(STACK_FUZZ_THREAD_NUM), SDF_OK);
        EXPECT_EQ(SDF_EvcInit(), SDF_OK);
        EXPECT_EQ(CM_SignalingCacheInit(), CM_SUCCESS);
        EXPECT_EQ(ScheduleEnable(), CP_OK);
        CM_SetSendSignalingDataCbk(CM_SendSignalingDataCbkUTStub);
    }

    // TearDownTestCase 在所有 TEST_F 测试完成后执行一次
    static void TearDownTestCase()
    {
        CM_SetSendSignalingDataCbk(NULL);
        ScheduleDisable();
        CM_SignalingTransChanCbksUnregister();
        CM_SignalingCacheDeinit();
        SDF_EvcDeinit();
        SDF_ThreadDeinit();
    }
};

void CM_SignalingTransChanEstablishReqCbkStub(uint16_t lcid, uint8_t reqId, CM_SignalingTransChanEstablishReq_S *req)
{
    (void)lcid;
    (void)reqId;
    (void)req;
}

void CM_SignalingTransChanEstablishRspCbkStub(uint16_t lcid, CM_SignalingTransChanEstablishRsp_S *rsp)
{
    (void)lcid;
    (void)rsp;
}

void CM_SignalingTransChanReleaseReqCbkStub(uint16_t lcid, uint8_t reqId, CM_SignalingTransChanReleaseReq_S *req)
{
    (void)lcid;
    (void)reqId;
    (void)req;
}

void CM_SignalingTransChanReleaseRspCbkStub(uint16_t lcid, CM_SignalingTransChanReleaseRsp_S *rsp)
{
    (void)lcid;
    (void)rsp;
}

TEST_F(UT_TRANS_CHAN_SIGNALING, CM_SignalingTransChanCbksRegisterFail)
{
    CM_SignalingTransChanCbks_S cbks = {0};
    uint32_t ret = CM_SignalingTransChanCbksRegister(&cbks);
    EXPECT_EQ(ret, CM_NULL_POINTER);
}

TEST_F(UT_TRANS_CHAN_SIGNALING, CM_SignalingTransChanCbksRegisterSucc)
{
    CM_SignalingTransChanCbks_S cbks = {
        .establishReqCbk = CM_SignalingTransChanEstablishReqCbkStub,
        .establishRspCbk = CM_SignalingTransChanEstablishRspCbkStub,
        .releaseReqCbk = CM_SignalingTransChanReleaseReqCbkStub,
        .releaseRspCbk = CM_SignalingTransChanReleaseRspCbkStub,
    };
    uint32_t ret = CM_SignalingTransChanCbksRegister(&cbks);
    EXPECT_EQ(ret, CM_SUCCESS);
}

TEST_F(UT_TRANS_CHAN_SIGNALING, CM_SignalingTransChanEstablishReqSend)
{
    uint32_t ret = CM_SignalingTransChanEstablishReqSend(0, NULL);
    EXPECT_EQ(ret, CM_INVALID_PARAM_ERR);

    CM_SignalingTransChanEstablishReq_S req = {0};
    req.slqiList.slqiNum = 1;
    CM_SignalingPortConfig_S portConfig = {0};
    req.extension.portConfig = &portConfig;
    ret = CM_SignalingTransChanEstablishReqSend(0, &req);
    EXPECT_EQ(ret, CM_SUCCESS);
}

TEST_F(UT_TRANS_CHAN_SIGNALING, CM_SignalingTransChanEstablishReqSendTimeout)
{
    CP_TimerSetExecCallbackAtOnce(true);
    uint32_t ret = CM_SignalingTransChanEstablishReqSend(0, NULL);
    EXPECT_EQ(ret, CM_INVALID_PARAM_ERR);

    CM_SignalingTransChanEstablishReq_S req = {0};
    req.slqiList.slqiNum = 1;
    CM_SignalingPortConfig_S portConfig = {0};
    req.extension.portConfig = &portConfig;
    ret = CM_SignalingTransChanEstablishReqSend(0, &req);
    EXPECT_EQ(ret, CM_SUCCESS);
    CP_TimerSetExecCallbackAtOnce(false);
}

TEST_F(UT_TRANS_CHAN_SIGNALING, CM_SignalingTransChanEstablishRspSend)
{
    uint32_t ret = CM_SignalingTransChanEstablishRspSend(0, 0, NULL);
    EXPECT_EQ(ret, CM_NULL_POINTER);

    CM_SignalingTransChanEstablishRsp_S rsp = {0};
    ret = CM_SignalingTransChanEstablishRspSend(0, 0, &rsp);
    EXPECT_EQ(ret, CM_SUCCESS);
}

TEST_F(UT_TRANS_CHAN_SIGNALING, CM_SignalingTransChanEstablishRspSendTimeout)
{
    CP_TimerSetExecCallbackAtOnce(true);
    uint32_t ret = CM_SignalingTransChanEstablishRspSend(0, 0, NULL);
    EXPECT_EQ(ret, CM_NULL_POINTER);

    CM_SignalingTransChanEstablishRsp_S rsp = {0};
    ret = CM_SignalingTransChanEstablishRspSend(0, 0, &rsp);
    EXPECT_EQ(ret, CM_SUCCESS);
    CP_TimerSetExecCallbackAtOnce(false);
}

TEST_F(UT_TRANS_CHAN_SIGNALING, CM_SignalingTransChanReleaseReqSend)
{
    uint32_t ret = CM_SignalingTransChanReleaseReqSend(0, NULL);
    EXPECT_EQ(ret, CM_NULL_POINTER);

    CM_SignalingTransChanReleaseReq_S req = {0};
    ret = CM_SignalingTransChanReleaseReqSend(0, &req);
    EXPECT_EQ(ret, CM_SUCCESS);
}

TEST_F(UT_TRANS_CHAN_SIGNALING, CM_SignalingTransChanReleaseReqSendTimeout)
{
    CP_TimerSetExecCallbackAtOnce(true);
    uint32_t ret = CM_SignalingTransChanReleaseReqSend(0, NULL);
    EXPECT_EQ(ret, CM_NULL_POINTER);

    CM_SignalingTransChanReleaseReq_S req = {0};
    ret = CM_SignalingTransChanReleaseReqSend(0, &req);
    EXPECT_EQ(ret, CM_SUCCESS);
    CP_TimerSetExecCallbackAtOnce(false);
}

TEST_F(UT_TRANS_CHAN_SIGNALING, CM_SignalingTransChanReleaseRspSend)
{
    uint32_t ret = CM_SignalingTransChanReleaseRspSend(0, 0, NULL);
    EXPECT_EQ(ret, CM_NULL_POINTER);

    CM_SignalingTransChanReleaseRsp_S rsp = {0};
    ret = CM_SignalingTransChanReleaseRspSend(0, 0, &rsp);
    EXPECT_EQ(ret, CM_SUCCESS);
}

TEST_F(UT_TRANS_CHAN_SIGNALING, CM_SignalingTransChanReleaseRspSendTimeout)
{
    CP_TimerSetExecCallbackAtOnce(true);
    uint32_t ret = CM_SignalingTransChanReleaseRspSend(0, 0, NULL);
    EXPECT_EQ(ret, CM_NULL_POINTER);

    CM_SignalingTransChanReleaseRsp_S rsp = {0};
    ret = CM_SignalingTransChanReleaseRspSend(0, 0, &rsp);
    EXPECT_EQ(ret, CM_SUCCESS);
    CP_TimerSetExecCallbackAtOnce(false);
}

TEST_F(UT_TRANS_CHAN_SIGNALING, CM_SignalingTransChanEstablishReqProc_ParseFailed)
{
    CM_SignalingHead_S pkt = {0};
    uint32_t ret = CM_SignalingTransChanEstablishReqProc(0, &pkt);
    EXPECT_NE(ret, CM_SUCCESS);
}

TEST_F(UT_TRANS_CHAN_SIGNALING, CM_SignalingTransChanEstablishReqProc_Success)
{
    uint16_t length = sizeof(CM_SignalingHead_S) + sizeof(CM_TransChanEstablishReqPkt_S) +
        sizeof(CM_TransModeCommonConfig_S) + sizeof(CM_PreferredSlqiList_S) + sizeof(uint8_t) +
        sizeof(CM_TransChanEstablishReqExt_S) + sizeof(CM_PortConfig_S);
    CM_SignalingHead_S *pkt = (CM_SignalingHead_S *)SDF_MemZalloc(length);
    EXPECT_NE(pkt, nullptr);
    pkt->length = length;
    CM_TransChanEstablishReqPkt_S *req = (CM_TransChanEstablishReqPkt_S *)(pkt + 1);
    req->optionOffset = sizeof(CM_TransChanEstablishReqPkt_S) + sizeof(CM_TransModeCommonConfig_S) +
        sizeof(CM_PreferredSlqiList_S) + sizeof(uint8_t);
    CM_PreferredSlqiList_S *slqiList = (CM_PreferredSlqiList_S *)((uint8_t *)req  +
        sizeof(CM_TransChanEstablishReqPkt_S) + sizeof(CM_TransModeCommonConfig_S));
    slqiList->slqiNum = 1;
    CM_TransChanEstablishReqExt_S *ext = (CM_TransChanEstablishReqExt_S *)((uint8_t *)slqiList +
        sizeof(CM_PreferredSlqiList_S) + sizeof(uint8_t));
    ext->pc = 1;

    uint32_t ret = CM_SignalingTransChanEstablishReqProc(0, pkt);
    SDF_MemFree(pkt);
    EXPECT_EQ(ret, CM_SUCCESS);
}

TEST_F(UT_TRANS_CHAN_SIGNALING, CM_SignalingTransChanEstablishReqProc_Success_1)
{
    uint16_t length = sizeof(CM_SignalingHead_S) + sizeof(CM_TransChanEstablishReqPkt_S) +
        sizeof(CM_TransModeCommonConfig_S) + sizeof(CM_PreferredSlqiList_S) + sizeof(uint8_t) +
        sizeof(CM_TransChanEstablishReqExt_S) + sizeof(CM_PortConfig_S);
    CM_SignalingHead_S *pkt = (CM_SignalingHead_S *)SDF_MemZalloc(length);
    EXPECT_NE(pkt, nullptr);
    pkt->length = length;
    CM_TransChanEstablishReqPkt_S *req = (CM_TransChanEstablishReqPkt_S *)(pkt + 1);
    CM_TransModeCommonConfig_S *config =
        (CM_TransModeCommonConfig_S *)((uint8_t *)req + sizeof(CM_TransChanEstablishReqPkt_S));
    config->transMode = CM_TRANS_MODE_TRANSPARENT;
    req->optionOffset = sizeof(CM_TransChanEstablishReqPkt_S) + sizeof(CM_TransModeCommonConfig_S) +
        sizeof(CM_PreferredSlqiList_S) + sizeof(uint8_t);
    CM_PreferredSlqiList_S *slqiList = (CM_PreferredSlqiList_S *)((uint8_t *)req  +
        sizeof(CM_TransChanEstablishReqPkt_S) + sizeof(CM_TransModeCommonConfig_S));
    slqiList->slqiNum = 1;
    CM_TransChanEstablishReqExt_S *ext = (CM_TransChanEstablishReqExt_S *)((uint8_t *)slqiList +
        sizeof(CM_PreferredSlqiList_S) + sizeof(uint8_t));
    ext->pc = 1;

    uint32_t ret = CM_SignalingTransChanEstablishReqProc(0, pkt);
    SDF_MemFree(pkt);
    EXPECT_EQ(ret, CM_INVALID_PARAM_ERR);
}

TEST_F(UT_TRANS_CHAN_SIGNALING, CM_SignalingTransChanEstablishReqProc_Success_2)
{
    uint16_t length = sizeof(CM_SignalingHead_S) + sizeof(CM_TransChanEstablishReqPkt_S) +
        sizeof(CM_TransModeCommonConfig_S) + sizeof(CM_PreferredSlqiList_S) + sizeof(uint8_t) +
        sizeof(CM_TransChanEstablishReqExt_S) + sizeof(CM_PortConfig_S);
    CM_SignalingHead_S *pkt = (CM_SignalingHead_S *)SDF_MemZalloc(length);
    EXPECT_NE(pkt, nullptr);
    pkt->length = length;
    CM_TransChanEstablishReqPkt_S *req = (CM_TransChanEstablishReqPkt_S *)(pkt + 1);
    CM_TransModeCommonConfig_S *config =
        (CM_TransModeCommonConfig_S *)((uint8_t *)req + sizeof(CM_TransChanEstablishReqPkt_S));
    config->transMode = CM_TRANS_MODE_STREAM;
    req->optionOffset = sizeof(CM_TransChanEstablishReqPkt_S) + sizeof(CM_TransModeCommonConfig_S) +
        sizeof(CM_PreferredSlqiList_S) + sizeof(uint8_t);
    CM_PreferredSlqiList_S *slqiList = (CM_PreferredSlqiList_S *)((uint8_t *)req  +
        sizeof(CM_TransChanEstablishReqPkt_S) + sizeof(CM_TransModeCommonConfig_S));
    slqiList->slqiNum = 1;
    CM_TransChanEstablishReqExt_S *ext = (CM_TransChanEstablishReqExt_S *)((uint8_t *)slqiList +
        sizeof(CM_PreferredSlqiList_S) + sizeof(uint8_t));
    ext->pc = 1;

    uint32_t ret = CM_SignalingTransChanEstablishReqProc(0, pkt);
    SDF_MemFree(pkt);
    EXPECT_EQ(ret, CM_INVALID_PARAM_ERR);
}

TEST_F(UT_TRANS_CHAN_SIGNALING, CM_SignalingTransChanEstablishReqProc_Success_3)
{
    uint16_t length = sizeof(CM_SignalingHead_S) + sizeof(CM_TransChanEstablishReqPkt_S) +
        sizeof(CM_TransModeCommonConfig_S) + sizeof(CM_PreferredSlqiList_S) + sizeof(uint8_t) +
        sizeof(CM_TransChanEstablishReqExt_S) + sizeof(CM_PortConfig_S);
    CM_SignalingHead_S *pkt = (CM_SignalingHead_S *)SDF_MemZalloc(length);
    EXPECT_NE(pkt, nullptr);
    pkt->length = length;
    CM_TransChanEstablishReqPkt_S *req = (CM_TransChanEstablishReqPkt_S *)(pkt + 1);
    CM_TransModeCommonConfig_S *config =
        (CM_TransModeCommonConfig_S *)((uint8_t *)req + sizeof(CM_TransChanEstablishReqPkt_S));
    config->transMode = CM_TRANS_MODE_RELIABLE;
    req->optionOffset = sizeof(CM_TransChanEstablishReqPkt_S) + sizeof(CM_TransModeCommonConfig_S) +
        sizeof(CM_PreferredSlqiList_S) + sizeof(uint8_t);
    CM_PreferredSlqiList_S *slqiList = (CM_PreferredSlqiList_S *)((uint8_t *)req  +
        sizeof(CM_TransChanEstablishReqPkt_S) + sizeof(CM_TransModeCommonConfig_S));
    slqiList->slqiNum = 1;
    CM_TransChanEstablishReqExt_S *ext = (CM_TransChanEstablishReqExt_S *)((uint8_t *)slqiList +
        sizeof(CM_PreferredSlqiList_S) + sizeof(uint8_t));
    ext->pc = 1;

    uint32_t ret = CM_SignalingTransChanEstablishReqProc(0, pkt);
    SDF_MemFree(pkt);
    EXPECT_EQ(ret, CM_INVALID_PARAM_ERR);
}

TEST_F(UT_TRANS_CHAN_SIGNALING, CM_SignalingTransChanEstablishReqProc_Success_4)
{
    uint16_t length = sizeof(CM_SignalingHead_S) + sizeof(CM_TransChanEstablishReqPkt_S) +
        sizeof(CM_TransModeCommonConfig_S) + sizeof(CM_PreferredSlqiList_S) + sizeof(uint8_t) +
        sizeof(CM_TransChanEstablishReqExt_S) + sizeof(CM_PortConfig_S);
    CM_SignalingHead_S *pkt = (CM_SignalingHead_S *)SDF_MemZalloc(length);
    EXPECT_NE(pkt, nullptr);
    pkt->length = length;
    CM_TransChanEstablishReqPkt_S *req = (CM_TransChanEstablishReqPkt_S *)(pkt + 1);
    CM_TransModeCommonConfig_S *config =
        (CM_TransModeCommonConfig_S *)((uint8_t *)req + sizeof(CM_TransChanEstablishReqPkt_S));
    config->transMode = CM_TRANS_MODE_MAX;
    req->optionOffset = sizeof(CM_TransChanEstablishReqPkt_S) + sizeof(CM_TransModeCommonConfig_S) +
        sizeof(CM_PreferredSlqiList_S) + sizeof(uint8_t);
    CM_PreferredSlqiList_S *slqiList = (CM_PreferredSlqiList_S *)((uint8_t *)req  +
        sizeof(CM_TransChanEstablishReqPkt_S) + sizeof(CM_TransModeCommonConfig_S));
    slqiList->slqiNum = 1;
    CM_TransChanEstablishReqExt_S *ext = (CM_TransChanEstablishReqExt_S *)((uint8_t *)slqiList +
        sizeof(CM_PreferredSlqiList_S) + sizeof(uint8_t));
    ext->pc = 1;

    uint32_t ret = CM_SignalingTransChanEstablishReqProc(0, pkt);
    SDF_MemFree(pkt);
    EXPECT_EQ(ret, CM_INVALID_PARAM_ERR);
}

TEST_F(UT_TRANS_CHAN_SIGNALING, CM_SignalingTransChanEstablishRspProc_InvalidDataLen)
{
    CM_SignalingHead_S pkt = {0};
    uint32_t ret = CM_SignalingTransChanEstablishRspProc(0, &pkt);
    EXPECT_EQ(ret, CM_INVALID_PARAM_ERR);
}

TEST_F(UT_TRANS_CHAN_SIGNALING, CM_SignalingTransChanEstablishRspProc_Success)
{
    CM_SignalingHead_S *pkt = (CM_SignalingHead_S *)SDF_MemZalloc(sizeof(CM_SignalingHead_S) +
        sizeof(CM_TransChanEstablishRspPkt_S));
    EXPECT_NE(pkt, nullptr);
    pkt->length = sizeof(CM_TransChanEstablishRspPkt_S);

    uint32_t ret = CM_SignalingTransChanEstablishRspProc(0, pkt);
    SDF_MemFree(pkt);
    EXPECT_EQ(ret, CM_SUCCESS);
}

TEST_F(UT_TRANS_CHAN_SIGNALING, CM_SignalingTransChanReleaseReqProc_InvalidDataLen)
{
    CM_SignalingHead_S pkt = {0};
    uint32_t ret = CM_SignalingTransChanReleaseReqProc(0, &pkt);
    EXPECT_EQ(ret, CM_INVALID_PARAM_ERR);
}

TEST_F(UT_TRANS_CHAN_SIGNALING, CM_SignalingTransChanReleaseReqProc_Success)
{
    CM_SignalingHead_S *pkt = (CM_SignalingHead_S *)SDF_MemZalloc(sizeof(CM_SignalingHead_S) +
        sizeof(CM_TransChanReleaseReqPkt_S));
    EXPECT_NE(pkt, nullptr);
    pkt->length = sizeof(CM_TransChanReleaseReqPkt_S);

    uint32_t ret = CM_SignalingTransChanReleaseReqProc(0, pkt);
    SDF_MemFree(pkt);
    EXPECT_EQ(ret, CM_SUCCESS);
}

TEST_F(UT_TRANS_CHAN_SIGNALING, CM_SignalingTransChanReleaseRspProc_InvalidDataLen)
{
    CM_SignalingHead_S pkt = {0};
    uint32_t ret = CM_SignalingTransChanReleaseRspProc(0, &pkt);
    EXPECT_EQ(ret, CM_INVALID_PARAM_ERR);
}

TEST_F(UT_TRANS_CHAN_SIGNALING, CM_SignalingTransChanReleaseRspProc_Success)
{
    CM_SignalingHead_S *pkt = (CM_SignalingHead_S *)SDF_MemZalloc(sizeof(CM_SignalingHead_S) +
        sizeof(CM_TransChanReleaseRspPkt_S));
    EXPECT_NE(pkt, nullptr);
    pkt->length = sizeof(CM_TransChanReleaseRspPkt_S);

    uint32_t ret = CM_SignalingTransChanReleaseRspProc(0, pkt);
    SDF_MemFree(pkt);
    EXPECT_EQ(ret, CM_SUCCESS);
}

}  // namespace TEST
}  // namespace Nearlink
}  // namespace OHOS