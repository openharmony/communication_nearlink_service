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
#include "cm_signaling_trans_channel.h"
#include "cm_trans_channel_mgr.h"
#include "cm_errno.h"
#include "cm_log.h"

static CM_SignalingTransChanCbks_S g_testSignalingTransChanCbks = { 0 };
static bool g_testSignalingTransChanEstablishReqSendFailed = false;
static bool g_testSignalingTransChanEstablishRspSendFailed = false;
static bool g_testSignalingTransChanReleaseRspSendFailed = false;
static bool g_testSignalingTransChanReleaseReqSendFailed = false;
static bool g_testSignalingTransChanDestTcidInvalid = false;
static uint8_t g_testSignalingTransChanEstablishRspSendResult = CM_RESULT_ESTABLISH_SUCCESS;

void CM_SetSignalingTransChanEstablishReqSend(bool failed)
{
    g_testSignalingTransChanEstablishReqSendFailed = failed;
}

uint32_t CM_SignalingTransChanEstablishReqSend(uint16_t lcid, CM_SignalingTransChanEstablishReq_S *req)
{
    CM_LOGI("CM_SignalingTransChanEstablishReqSendStub");
    if (g_testSignalingTransChanEstablishReqSendFailed) {
        return CM_FAIL;
    }
    EXPECT_EQ(CM_TRANS_CHANNEL_IS_DYNAMIC(req->srcTcid), true);
    return CM_SUCCESS;
}

void CM_SetSignalingTransChanReleaseReqSend(bool failed)
{
    g_testSignalingTransChanReleaseReqSendFailed = failed;
}

uint32_t CM_SignalingTransChanReleaseReqSend(uint16_t lcid, CM_SignalingTransChanReleaseReq_S *req)
{
    CM_LOGI("CM_SignalingTransChanReleaseReqSendStub");
    if (g_testSignalingTransChanReleaseReqSendFailed) {
        return CM_FAIL;
    }
    EXPECT_EQ(CM_TRANS_CHANNEL_IS_DYNAMIC(req->srcTcid), true);
    EXPECT_EQ(CM_TRANS_CHANNEL_IS_DYNAMIC(req->dstTcid), true);
    return CM_SUCCESS;
}

void CM_SetSignalingTransChanEstablishRspSend(bool failed)
{
    g_testSignalingTransChanEstablishRspSendFailed = failed;
}

void CM_SetSignalingTransChanDestTcidInvalid(bool invalid)
{
    g_testSignalingTransChanDestTcidInvalid = invalid;
}

void CM_SetSignalingTransChanEstablishRspSendResult(uint8_t result)
{
    g_testSignalingTransChanEstablishRspSendResult = result;
}

uint32_t CM_SignalingTransChanEstablishRspSend(uint16_t lcid, uint8_t reqId,
                                               CM_SignalingTransChanEstablishRsp_S *rsp)
{
    CM_LOGI("CM_SignalingTransChanEstablishRspSendStub");
    if (g_testSignalingTransChanEstablishRspSendFailed) {
        return CM_FAIL;
    }
    EXPECT_EQ(CM_TRANS_CHANNEL_IS_DYNAMIC(rsp->srcTcid), true);
    if (!g_testSignalingTransChanDestTcidInvalid) {
        EXPECT_EQ(CM_TRANS_CHANNEL_IS_DYNAMIC(rsp->dstTcid), true);
    } else {
        EXPECT_EQ(CM_TRANS_CHANNEL_IS_DYNAMIC(rsp->dstTcid), false);
    }
    EXPECT_EQ(rsp->result, g_testSignalingTransChanEstablishRspSendResult);
    return CM_SUCCESS;
}

void CM_SetSignalingTransChanReleaseRspSend(bool failed)
{
    g_testSignalingTransChanReleaseRspSendFailed = failed;
}

uint32_t CM_SignalingTransChanReleaseRspSend(uint16_t lcid, uint8_t reqId,
    CM_SignalingTransChanReleaseRsp_S *rsp)
{
    CM_LOGI("CM_SignalingTransChanReleaseRspSendStub");
    if (g_testSignalingTransChanReleaseRspSendFailed) {
        return CM_FAIL;
    }
    EXPECT_EQ(CM_TRANS_CHANNEL_IS_DYNAMIC(rsp->srcTcid), true);
    EXPECT_EQ(CM_TRANS_CHANNEL_IS_DYNAMIC(rsp->dstTcid), true);
    return CM_SUCCESS;
}

uint32_t CM_SignalingTransChanCbksRegister(const CM_SignalingTransChanCbks_S *cbks)
{
    CM_CHECK_RETURN_RET(cbks != NULL && cbks->establishReqCbk != NULL && cbks->establishRspCbk != NULL &&
                            cbks->releaseReqCbk != NULL && cbks->releaseRspCbk != NULL,
                        CM_NULL_POINTER, "transport channel signaling cbks is null");
    g_testSignalingTransChanCbks = *cbks;
    return CM_SUCCESS;
}

CM_SignalingTransChanCbks_S* CM_GetSignalingTransChanCbks(void)
{
    return &g_testSignalingTransChanCbks;
}