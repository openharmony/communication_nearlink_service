/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "ssapclientnew_fuzzer.h"
#include "securec.h"

#include "ssapc_client.h"
#include "ssapc_client_api.h"
#include "ssap_manager.h"
#include "ssap_link.h"

#include "nai_log.h"
#include "sdf_mem.h"
#include "sdf_thread.h"
#include "sdf_evc.h"
#include "nlstk_init_api.h"
#include "ssap_type.h"

#define SSAP_FUZZ_IDX1 1
#define SSAP_FUZZ_IDX2 2

typedef struct SlePkt {
    uint8_t *data;
    uint32_t size;
} SlePkt;

int SleSendDliPacket(const SlePkt *packet)
{
    return 0;
}

namespace OHOS {
    void SendCb(SSAP_Link_S *link, SDF_Buff_S *buff, uint8_t opcode)
    {
        (void)link;
        (void)buff;
        (void)opcode;
    }
    void FuzzSsapClientHandle(uint8_t* data, size_t size)
    {
        SLE_Addr_S sleAddr = {0};
        sleAddr.type = PUBLIC_ADDRESS;
        SSAP_Link_S *link = SSAP_CreateSsapLink(&sleAddr, data[0], SendCb);
        if (link == nullptr) {
            return;
        }
        SDF_Buff_S *sdfBuff = SDF_BuffNewWithReserve(size);
        uint8_t *buf = SDF_BuffAppend(sdfBuff, size);
        if (sdfBuff == nullptr || buf == nullptr) {
            SDF_BuffFree(sdfBuff);
            return;
        }
        memcpy_s(buf, size, data, size);
        SSAPC_ExchangeInfoRspHandle(link, sdfBuff);
        SSAPC_ReadRspHandle(link, sdfBuff);
        SSAPC_ReadByUuidRspHandle(link, sdfBuff);
        SSAPC_WriteRspHandle(link, sdfBuff);
        SSAPC_ValueNtfHandle(link, sdfBuff);
        SSAPC_ValueIndHandle(link, sdfBuff);
        SSAP_DeleteSsapLinkByAddr(&sleAddr);
    }

    void FuzzSsapClientPkt(uint8_t* data, size_t size)
    {
        SLE_Addr_S sleAddr = {0};
        sleAddr.addr[0] = SSAP_FUZZ_IDX1;
        SSAP_Link_S *link = SSAP_CreateSsapLink(&sleAddr, data[0], SendCb);
        if (link == nullptr) {
            return;
        }
        SDF_Buff_S *sdfBuff = SDF_BuffNewWithReserve(size);
        uint8_t *buf = SDF_BuffAppend(sdfBuff, size);
        if (sdfBuff == nullptr || buf == nullptr) {
            SDF_BuffFree(sdfBuff);
            return;
        }
        memcpy_s(buf, size, data, size);
        link->curTask.buff = sdfBuff;

        if (size >= sizeof(SSAP_ParamFind_S)) {
            SSAP_ParamFind_S *testData = (SSAP_ParamFind_S *)SDF_MemZalloc(sizeof(SSAP_ParamFind_S));
            memcpy_s(testData, sizeof(SSAP_ParamFind_S), data, sizeof(SSAP_ParamFind_S));
            SSAPC_FindReq(link, (void*)testData);
            SDF_MemFree(testData);
        }

        if (size >= sizeof(SSAP_WriteCmdInfo_S)) {
            SSAP_WriteCmdInfo_S *testData = (SSAP_WriteCmdInfo_S *)SDF_MemZalloc(sizeof(SSAP_WriteCmdInfo_S) + size);
            memcpy_s(testData, sizeof(SSAP_WriteCmdInfo_S), data, sizeof(SSAP_WriteCmdInfo_S));
            testData->value.len = size;
            memcpy_s(testData->value.value, size, data, size);
            SSAPC_WriteCmd(link, (void*)testData);
            SDF_MemFree(testData);
        }

        if (size >= sizeof(SSAP_WriteReqInfo_S)) {
            SSAP_WriteReqInfo_S *testData = (SSAP_WriteReqInfo_S *)SDF_MemZalloc(sizeof(SSAP_WriteReqInfo_S) + size);
            memcpy_s(testData, sizeof(SSAP_WriteReqInfo_S), data, sizeof(SSAP_WriteReqInfo_S));
            testData->value.len = size;
            memcpy_s(testData->value.value, size, data, size);
            SSAPC_WriteReq(link, (void*)testData);
            SDF_MemFree(testData);
        }

        if (size >= sizeof(SSAP_ValueAckInfo_S)) {
            SSAP_ValueAckInfo_S *testData = (SSAP_ValueAckInfo_S *)SDF_MemZalloc(sizeof(SSAP_ValueAckInfo_S) + size);
            memcpy_s(testData, sizeof(SSAP_ValueAckInfo_S), data, sizeof(SSAP_ValueAckInfo_S));
            testData->value.len = size;
            memcpy_s(testData->value.value, size, data, size);
            SSAPC_ValueAck(link, (void*)testData);
            SDF_MemFree(testData);
        }

        SSAP_ParamFindByUuid_S *testData = (SSAP_ParamFindByUuid_S *)SDF_MemZalloc(sizeof(SSAP_ParamFindByUuid_S));
        memcpy_s(testData, sizeof(SSAP_ParamFindByUuid_S), data, size);
        SSAPC_FindByUuidReq(link, (void*)testData);
        SSAP_DeleteSsapLinkByAddr(&sleAddr);
    }

    void FuzzSsapClientPktOther(uint8_t* data, size_t size)
    {
        SLE_Addr_S sleAddr = {0};
        sleAddr.addr[0] = SSAP_FUZZ_IDX2;
        SSAP_Link_S *link = SSAP_CreateSsapLink(&sleAddr, data[0], SendCb);
        if (link == nullptr) {
            return;
        }
        SDF_Buff_S *sdfBuff = SDF_BuffNewWithReserve(size);
        uint8_t *buf = SDF_BuffAppend(sdfBuff, size);
        if (sdfBuff == nullptr || buf == nullptr) {
            SDF_BuffFree(sdfBuff);
            return;
        }
        memcpy_s(buf, size, data, size);
        link->curTask.buff = sdfBuff;

        if (size >= sizeof(SSAP_ExchangeInfoReqInfo_S)) {
            SSAPC_ExchangeInfoReq(link, (void*)data);
        }

        if (size >= sizeof(SSAP_ReadReqInfo_S)) {
            SSAPC_ReadReq(link, (void*)data);
        }

        if (size >= sizeof(SSAP_ReadByUuidReqInfo_S)) {
            SSAPC_ReadByUuidReq(link, (void*)data);
        }

        SSAP_DeleteSsapLinkByAddr(&sleAddr);
    }

    void FuzzSsapClientApi(uint8_t* data, size_t size)
    {
        if (data == nullptr || size < 1) {
            return;
        }
        SSAP_LinkInit();
        FuzzSsapClientHandle(data, size);
        FuzzSsapClientPkt(data, size);
        FuzzSsapClientPktOther(data, size);
        SSAP_LinkDeInit();
    }
}

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void) argc;
    (void) argv;
    uint32_t ret;
    int res;

    ret = NLSTK_InitStack();
    if (ret != 0) {
        NAI_LOG_ERROR("NAI_InitStack failed, ret:%{public}d", ret);
        return 1;
    }
    NAI_LOG_INFO("NAI_InitStack success");
    res = NLSTK_EnableStack();
    if (res != 0) {
        NAI_LOG_ERROR("NLSTK_EnableStack failed, ret:%{public}d", res);
        return 1;
    }
    NAI_LOG_INFO("NLSTK_EnableStack success");
    /* staking builds starburst connection */
    NAI_LOG_INFO("ssapclientnew_fuzzer init success");
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    uint8_t *fuzzData = (uint8_t*)malloc(size);
    if (fuzzData == nullptr) {
        return 0;
    }
    memcpy_s(fuzzData, size, data, size);
    OHOS::FuzzSsapClientApi(static_cast<uint8_t *>(fuzzData), size);
    SDF_MemFree(fuzzData);
    return 0;
}