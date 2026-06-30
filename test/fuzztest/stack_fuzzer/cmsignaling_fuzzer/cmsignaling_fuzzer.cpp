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

#include "fuzzer/FuzzedDataProvider.h"
#include "cmsignaling_fuzzer.h"
#include "securec.h"

#include "sdf_evc.h"
#include "sdf_timer.h"
#include "sdf_event.h"
#include "sdf_worker.h"
#include "sdf_thread.h"
#include "cm_signaling_struct.h"
#include "cm_signaling_cap.h"
#include "cm_signaling_manage.h"
#include "cm_signaling_version.h"
#include "cm_signaling_trans_channel.h"
#include "sle_logic_link_mgr.h"
#include "cm_logic_link_api.h"
#include "cm_errno.h"
#include "cm_def.h"
#include "cm_log.h"

static SLE_Addr_S g_publicAddress = {.type = PUBLIC_ADDRESS, .addr = {0}};

extern "C" SLE_Addr_S *NBC_GetPublicAddress(void)
{
    return &g_publicAddress;
}

extern "C" SleLogicLink_S *SleLogicLinkGetByLcid(uint16_t lcid)
{
    if (lcid == UINT16_MAX) {
        return nullptr;
    }

    static SleLogicLink_S link;
    return &link;
}

extern "C" uint32_t CM_RegLogicLinkListener(CM_LogicLinkCbks_S *cbks)
{
    if (cbks != nullptr) {
        CM_LogicLinkState_S state = { 0 };
        state.result = CM_LINK_STATE_CONNECTED;
        cbks->logicLinkCbk(&state);
    }
    return 0;
}

extern "C" uint32_t CM_UnRegLogicLinkListener(uint8_t moduleId)
{
    (void)moduleId;
    return 0;
}

extern "C" uint8_t *DLI_GetDevicePublicAddress(void)
{
    static uint8_t g_publicAddr[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
    return g_publicAddr;
}

extern "C" uint32_t CP_TimerAdd(int *handle, SDF_TimerParam *param)
{
    return 0;
}

extern "C" void CP_TimerDel(int handle)
{}

extern "C" uint32_t DLI_ReadLocalMeasureCaps(void)
{
    return 0;
}

static void CmSignalingTransChanEstablishReqCbk(uint16_t lcid, uint8_t reqId,
                                                CM_SignalingTransChanEstablishReq_S *req)
{
    (void)lcid;
    (void)reqId;
    (void)req;
}

static void CmSignalingTransChanEstablishRspCbk(uint16_t lcid, CM_SignalingTransChanEstablishRsp_S *rsp)
{
    (void)lcid;
    (void)rsp;
}

static void CmSignalingTransChanReleaseReqCbk(uint16_t lcid, uint8_t reqId,
                                              CM_SignalingTransChanReleaseReq_S *req)
{
    (void)lcid;
    (void)reqId;
    (void)req;
}

static void CmSignalingTransChanReleaseRspCbk(uint16_t lcid, CM_SignalingTransChanReleaseRsp_S *rsp)
{
    (void)lcid;
    (void)rsp;
}


namespace OHOS {
    const uint16_t TEST_DEFAULT_NUM = 10;
    const uint32_t DLI_FUZZ_LEN = 100;
    const uint16_t TEST_CAP_REQ = 0x01;
    const uint16_t TEST_CAP_RSP = 0x02;
    const uint16_t TEST_INVALID_VALUE = 0xffff;
    const uint16_t TEST_INVALID_LCID = 0xffff;

    uint32_t CM_SendSignalingDataCbkStub(uint8_t pi, uint8_t tcid,
        uint16_t lcid, SDF_Buff_S *buff)
    {
        if (lcid == OHOS::TEST_INVALID_LCID || buff == NULL) {
            return 1;
        }
        SDF_BuffFree(buff);
        return 0;
    }

    void FuzzCmSignalingSendData(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        CM_CapabilityBitmap_S cap;
        CM_SendReqSignalingCapability(OHOS::TEST_INVALID_LCID, &cap);

        uint8_t code = provider.ConsumeIntegral<int8_t>();
        uint16_t buffSize = provider.ConsumeIntegral<uint16_t>();
        std::vector<uint8_t> buffData = provider.ConsumeBytes<uint8_t>(buffSize);
        SDF_Buff_S *buff = CM_CreateSignalingBuff(code, 0, buffData.data(), buffData.size());
        if (buff != NULL) {
            uint16_t lcid = provider.ConsumeIntegral<uint16_t>();
            CM_ProcessRspSignalingCapability(lcid, CM_ParseSignalingBuff(buff));
            CM_ProcessReqSignalingCapability(lcid, CM_ParseSignalingBuff(buff));
        }
        CM_ProcessRspSignalingCapability(0, NULL);
        CM_ProcessReqSignalingCapability(0, NULL);
        SDF_BuffFree(buff);
    }

    void FuzzCmSignalingRecvData(uint8_t *data, size_t size)
    {
        SDF_Buff_S *buff = SDF_BuffNewWithReserve(OHOS::DLI_FUZZ_LEN);
        if (buff == NULL) {
            return;
        }
        uint32_t len = size >= OHOS::DLI_FUZZ_LEN ? OHOS::DLI_FUZZ_LEN : size;
        uint8_t *tmp = SDF_BuffAppend(buff, len);
        (void)memcpy_s(tmp, len, data, len);
        CM_RecvSignalingData(0, buff);
        SDF_BuffFree(buff);
    }

    void FuzzCmSignalingRecvReqData(uint8_t *data, size_t size)
    {
        uint8_t data1[] = {0x01, 0x00, 0x04, 0x00, 0xff, 0x00, 0x00, 0x00};
        SDF_Buff_S *buff = SDF_BuffNewWithReserve(OHOS::DLI_FUZZ_LEN);
        if (buff == NULL) {
            return;
        }
        uint8_t *tmp = SDF_BuffAppend(buff, sizeof(data1));
        (void)memcpy_s(tmp, sizeof(data1), data1, sizeof(data1));
        DTAP_Data_Info_S info;
        CM_RecvSignalingData(&info, buff);
        SDF_BuffFree(buff);
        FuzzCmSignalingRecvData(data, size);
    }

    void FuzzCmSignalingRecvRspData(uint8_t *data, size_t size)
    {
        uint8_t data1[] = {0x02, 00, 0x12, 00, 0xea, 00, 00, 00, 0x01, 00, 0x12,
            0xcf, 0x01, 00, 0x01, 00, 0xa0, 0x02, 0xa0, 0x02, 0x01, 0x01};
        SDF_Buff_S *buff = SDF_BuffNewWithReserve(OHOS::DLI_FUZZ_LEN);
        if (buff == NULL) {
            return;
        }
        uint8_t *tmp = SDF_BuffAppend(buff, sizeof(data1));
        (void)memcpy_s(tmp, sizeof(data1), data1, sizeof(data1));
        DTAP_Data_Info_S info;
        CM_RecvSignalingData(&info, buff);
        SDF_BuffFree(buff);
        FuzzCmSignalingRecvData(data, size);
    }

    void FuzzCmSignalingOtherApi(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        uint16_t lcid = provider.ConsumeIntegral<uint16_t>();
        uint16_t connId = provider.ConsumeIntegral<uint16_t>();
        uint16_t exchangeVersion = provider.ConsumeIntegral<uint16_t>();
        uint16_t protocolVersion = provider.ConsumeIntegral<uint16_t>();
        uint8_t code = provider.ConsumeIntegral<uint8_t>();
        
        SleLogicLink_S *link = SleLogicLinkGetByLcid(lcid);
        if (link != NULL) {
            link->protocolVersion = OHOS::TEST_INVALID_VALUE;
            CM_SetDeviceLinkDeviceType(0, false);
            CM_SetDeviceLinkDeviceType(connId, false);
        }
        (void)CM_GetLogicLinkDeviceType(0);
        (void)CM_GetLogicLinkDeviceType(connId);
        CM_SetLinkExchangeVersion(0, exchangeVersion);
        CM_SetLinkProtocolVersion(0, protocolVersion);
        CM_SetSendSignalingDataCbk(NULL);
        CM_SendBuffToDtap(0, NULL);
        CM_SetSendSignalingDataCbk(CM_SendSignalingDataCbkStub);
        CM_SignalingGetManagerHandler(OHOS::TEST_CAP_REQ);
        CM_SignalingGetManagerHandler(OHOS::TEST_CAP_RSP);
        CM_SignalingGetManagerHandler(code);
    }

    void FuzzCMSignalingTransChanCbksRegister()
    {
        CM_SignalingTransChanCbksRegister(nullptr);
        CM_SignalingTransChanCbks_S cbks = {
            .establishReqCbk = CmSignalingTransChanEstablishReqCbk,
            .establishRspCbk = CmSignalingTransChanEstablishRspCbk,
            .releaseReqCbk = CmSignalingTransChanReleaseReqCbk,
            .releaseRspCbk = CmSignalingTransChanReleaseRspCbk,
        };
        CM_SignalingTransChanCbksRegister(&cbks);
        CM_SignalingTransChanCbksUnregister();
    }

    void FuzzCmSignalingTransChanEstablishReqSend(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        (void)CM_SignalingTransChanEstablishReqSend(0, nullptr);

        CM_SignalingTransChanEstablishReq_S req = {0};
        req.exclusive = provider.ConsumeIntegral<uint8_t>();
        req.measure = provider.ConsumeBool();
        req.slqiList.slqiNum = 1;
        CM_SignalingPortConfig_S portConfig = {0};
        req.extension.portConfig = &portConfig;
        (void)CM_SignalingTransChanEstablishReqSend(0, &req);
    }

    void FuzzCmSignalingTransChanEstablishRspSend(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        (void)CM_SignalingTransChanEstablishRspSend(0, 0, nullptr);

        CM_SignalingTransChanEstablishRsp_S rsp = {0};
        rsp.result = provider.ConsumeIntegral<uint8_t>();
        (void)CM_SignalingTransChanEstablishRspSend(0, 0, &rsp);
    }

    void FuzzCmSignalingTransChanReleaseReqSend(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        (void)CM_SignalingTransChanReleaseReqSend(0, nullptr);

        CM_SignalingTransChanReleaseReq_S req = {0};
        (void)CM_SignalingTransChanReleaseReqSend(0, &req);

        CM_SignalingTransChanEstablishReq_S reqE = {0};
        reqE.srcTcid = provider.ConsumeIntegral<uint8_t>();
        reqE.slqiList.slqiNum = 1;
        CM_SignalingPortConfig_S portConfig = {0};
        reqE.extension.portConfig = &portConfig;
        uint32_t ret = CM_SignalingTransChanEstablishReqSend(0, &reqE);
        if (ret == 0) {
            req.srcTcid = reqE.srcTcid;
            (void)CM_SignalingTransChanReleaseReqSend(0, &req);
        }
    }

    void FuzzCmSignalingTransChanReleaseRspSend(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        provider.ConsumeIntegral<uint8_t>();
        (void)CM_SignalingTransChanReleaseRspSend(0, 0, nullptr);

        CM_SignalingTransChanReleaseRsp_S rsp = {0};
        (void)CM_SignalingTransChanReleaseRspSend(0, 0, &rsp);

        CM_SignalingTransChanEstablishRsp_S rspE = {0};
        rspE.srcTcid = provider.ConsumeIntegral<uint8_t>();
        rspE.dstTcid = provider.ConsumeIntegral<uint8_t>();
        rsp.result = provider.ConsumeIntegral<uint8_t>();
        int ret = CM_SignalingTransChanEstablishRspSend(0, 0, &rspE);
        if (ret == 0) {
            rsp.srcTcid = rspE.srcTcid;
            rsp.dstTcid = rspE.dstTcid;
            (void)CM_SignalingTransChanReleaseRspSend(0, 0, &rsp);
        }
    }

    void FuzzCmSignalingTransChanEstablishReqProc(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        size_t pktSize = sizeof(CM_SignalingHead_S) + sizeof(CM_TransChanEstablishReqPkt_S);
        size_t reqPktSize = sizeof(CM_TransChanEstablishReqPkt_S);
        std::vector<uint8_t> pktData = provider.ConsumeBytes<uint8_t>(reqPktSize);
        if (pktData.size() < reqPktSize) {
            pktData.insert(pktData.end(), reqPktSize - pktData.size(), 0);
        }
        CM_SignalingHead_S *pkt = (CM_SignalingHead_S *)SDF_MemZalloc(pktSize);
        (void)memset_s(pkt, sizeof(CM_SignalingHead_S), 0, sizeof(CM_SignalingHead_S));
        pkt->length = reqPktSize;
        (void)memcpy_s(pkt->data, reqPktSize, pktData.data(), reqPktSize);
        (void)CM_SignalingTransChanEstablishReqProc(0, pkt);

        SDF_MemFree(pkt);
    }

    void FuzzCmSignalingTransChanEstablishRspProc(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        size_t pktSize = sizeof(CM_SignalingHead_S) + sizeof(CM_TransChanEstablishRspPkt_S);
        size_t reqPktSize = sizeof(CM_TransChanEstablishRspPkt_S);
        std::vector<uint8_t> pktData = provider.ConsumeBytes<uint8_t>(reqPktSize);
        if (pktData.size() < reqPktSize) {
            pktData.insert(pktData.end(), reqPktSize - pktData.size(), 0);
        }
        CM_SignalingHead_S *pkt = (CM_SignalingHead_S *)SDF_MemZalloc(pktSize);
        (void)memset_s(pkt, sizeof(CM_SignalingHead_S), 0, sizeof(CM_SignalingHead_S));
        pkt->length = reqPktSize;
        (void)memcpy_s(pkt->data, reqPktSize, pktData.data(), reqPktSize);
        (void)CM_SignalingTransChanEstablishRspProc(0, pkt);

        SDF_MemFree(pkt);
    }

    void FuzzCmSignalingTransChanReleaseReqProc(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        size_t pktSize = sizeof(CM_SignalingHead_S) + sizeof(CM_TransChanReleaseReqPkt_S);
        size_t reqPktSize = sizeof(CM_TransChanReleaseReqPkt_S);
        std::vector<uint8_t> pktData = provider.ConsumeBytes<uint8_t>(reqPktSize);
        if (pktData.size() < reqPktSize) {
            pktData.insert(pktData.end(), reqPktSize - pktData.size(), 0);
        }
        CM_SignalingHead_S *pkt = (CM_SignalingHead_S *)SDF_MemZalloc(pktSize);
        (void)memset_s(pkt, sizeof(CM_SignalingHead_S), 0, sizeof(CM_SignalingHead_S));
        pkt->length = reqPktSize;
        (void)memcpy_s(pkt->data, reqPktSize, pktData.data(), reqPktSize);
        (void)CM_SignalingTransChanReleaseReqProc(0, pkt);

        SDF_MemFree(pkt);
    }

    void FuzzCmSignalingTransChanReleaseRspProc(uint8_t *data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        size_t pktSize = sizeof(CM_SignalingHead_S) + sizeof(CM_TransChanReleaseRspPkt_S);
        size_t reqPktSize = sizeof(CM_TransChanReleaseRspPkt_S);
        std::vector<uint8_t> pktData = provider.ConsumeBytes<uint8_t>(reqPktSize);
        if (pktData.size() < reqPktSize) {
            pktData.insert(pktData.end(), reqPktSize - pktData.size(), 0);
        }
        CM_SignalingHead_S *pkt = (CM_SignalingHead_S *)SDF_MemZalloc(pktSize);
        (void)memset_s(pkt, sizeof(CM_SignalingHead_S), 0, sizeof(CM_SignalingHead_S));
        pkt->length = reqPktSize;
        (void)memcpy_s(pkt->data, reqPktSize, pktData.data(), reqPktSize);
        (void)CM_SignalingTransChanReleaseRspProc(0, pkt);

        SDF_MemFree(pkt);
    }

    void FuzzCmSignalingTransChannel(uint8_t *data, size_t size)
    {
        FuzzCmSignalingTransChanReleaseRspProc(data, size);
        FuzzCmSignalingTransChanReleaseReqProc(data, size);
        FuzzCmSignalingTransChanEstablishRspProc(data, size);
        FuzzCmSignalingTransChanEstablishReqProc(data, size);
        FuzzCmSignalingTransChanReleaseRspSend(data, size);
        FuzzCmSignalingTransChanReleaseReqSend(data, size);
        FuzzCmSignalingTransChanEstablishRspSend(data, size);
        FuzzCMSignalingTransChanCbksRegister();
        FuzzCmSignalingTransChanEstablishReqSend(data, size);
    }

    void FuzzCmSignalingApi(uint8_t* data, size_t size)
    {
        CM_LOGE("FuzzCmSignalingApi enter");
        CM_SignalingInit();
        CM_SetSendSignalingDataCbk(CM_SendSignalingDataCbkStub);
        FuzzCmSignalingOtherApi(data, size);
        FuzzCmSignalingSendData(data, size);
        FuzzCmSignalingRecvRspData(data, size);
        FuzzCmSignalingRecvReqData(data, size);
        FuzzCmSignalingTransChannel(data, size);

        CM_SignalingDeInit();
        CM_LOGE("FuzzCmSignalingApi exit");
    }
}

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void) argc;
    (void) argv;
    /* staking builds starburst connection */

    (void)SDF_ThreadInit(OHOS::TEST_DEFAULT_NUM);
    (void)SDF_EvcInit();
    CM_LOGE("dlilayer_fuzzer init success");
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    uint8_t *fuzzData = (uint8_t*)malloc(size);
    if (fuzzData == nullptr) {
        return 0;
    }
    (void)memcpy_s(fuzzData, size, data, size);
    OHOS::FuzzCmSignalingApi(static_cast<uint8_t *>(fuzzData), size);
    free(fuzzData);
    return 0;
}
