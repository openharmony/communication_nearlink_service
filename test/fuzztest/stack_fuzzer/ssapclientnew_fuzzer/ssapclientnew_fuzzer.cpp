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

#include "ssapclientnew_fuzzer.h"
#include "securec.h"
#include "fuzzer/FuzzedDataProvider.h"

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
        FuzzedDataProvider fdp(data, size);
        SLE_Addr_S sleAddr = {0};
        sleAddr.type = fdp.ConsumeIntegral<uint8_t>();
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            sleAddr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        uint8_t lcid = fdp.ConsumeIntegral<uint8_t>();
        SSAP_Link_S *link = SSAP_CreateSsapLink(&sleAddr, lcid, SendCb);
        if (link == nullptr) {
            return;
        }
        SDF_Buff_S *sdfBuff = SDF_BuffNewWithReserve(size);
        uint8_t *buf = SDF_BuffAppend(sdfBuff, size);
        if (sdfBuff == nullptr || buf == nullptr) {
            SDF_BuffFree(sdfBuff);
            return;
        }
        for (size_t i = 0; i < size; i++) {
            buf[i] = fdp.ConsumeIntegral<uint8_t>();
        }
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
        FuzzedDataProvider fdp(data, size);
        SLE_Addr_S sleAddr = {0};
        sleAddr.addr[0] = SSAP_FUZZ_IDX1;
        uint8_t lcid = fdp.ConsumeIntegral<uint8_t>();
        SSAP_Link_S *link = SSAP_CreateSsapLink(&sleAddr, lcid, SendCb);
        if (link == nullptr) {
            return;
        }
        SDF_Buff_S *sdfBuff = SDF_BuffNewWithReserve(size);
        uint8_t *buf = SDF_BuffAppend(sdfBuff, size);
        if (sdfBuff == nullptr || buf == nullptr) {
            SDF_BuffFree(sdfBuff);
            return;
        }
        for (size_t i = 0; i < size; i++) {
            buf[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        link->curTask.buff = sdfBuff;

        if (size >= sizeof(SSAP_ParamFind_S)) {
            SSAP_ParamFind_S *testData = (SSAP_ParamFind_S *)SDF_MemZalloc(sizeof(SSAP_ParamFind_S));
            testData->appId = fdp.ConsumeIntegral<int32_t>();
            testData->addr.type = fdp.ConsumeIntegral<uint8_t>();
            for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
                testData->addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
            }
            testData->type = static_cast<SSAP_FindType_E>(fdp.ConsumeIntegral<uint8_t>());
            testData->startHandle = fdp.ConsumeIntegral<uint16_t>();
            testData->endHandle = fdp.ConsumeIntegral<uint16_t>();
            SSAPC_FindReq(link, (void*)testData);
            SDF_MemFree(testData);
        }

        if (size >= sizeof(SSAP_WriteCmdInfo_S)) {
            SSAP_WriteCmdInfo_S *testData = (SSAP_WriteCmdInfo_S *)SDF_MemZalloc(sizeof(SSAP_WriteCmdInfo_S) + size);
            testData->addr.type = fdp.ConsumeIntegral<uint8_t>();
            for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
                testData->addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
            }
            testData->handle = fdp.ConsumeIntegral<uint16_t>();
            testData->type = fdp.ConsumeIntegral<uint8_t>();
            testData->value.len = size;
            for (size_t i = 0; i < size; i++) {
                testData->value.value[i] = fdp.ConsumeIntegral<uint8_t>();
            }
            SSAPC_WriteCmd(link, (void*)testData);
            SDF_MemFree(testData);
        }

        if (size >= sizeof(SSAP_WriteReqInfo_S)) {
            SSAP_WriteReqInfo_S *testData = (SSAP_WriteReqInfo_S *)SDF_MemZalloc(sizeof(SSAP_WriteReqInfo_S) + size);
            testData->addr.type = fdp.ConsumeIntegral<uint8_t>();
            for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
                testData->addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
            }
            testData->handle = fdp.ConsumeIntegral<uint16_t>();
            testData->type = fdp.ConsumeIntegral<uint8_t>();
            testData->value.len = size;
            for (size_t i = 0; i < size; i++) {
                testData->value.value[i] = fdp.ConsumeIntegral<uint8_t>();
            }
            SSAPC_WriteReq(link, (void*)testData);
            SDF_MemFree(testData);
        }

        if (size >= sizeof(SSAP_ValueAckInfo_S)) {
            SSAP_ValueAckInfo_S *testData = (SSAP_ValueAckInfo_S *)SDF_MemZalloc(sizeof(SSAP_ValueAckInfo_S) + size);
            testData->addr.type = fdp.ConsumeIntegral<uint8_t>();
            for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
                testData->addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
            }
            testData->value.len = size;
            for (size_t i = 0; i < size; i++) {
                testData->value.value[i] = fdp.ConsumeIntegral<uint8_t>();
            }
            SSAPC_ValueAck(link, (void*)testData);
            SDF_MemFree(testData);
        }

        SSAP_ParamFindByUuid_S *testData = (SSAP_ParamFindByUuid_S *)SDF_MemZalloc(sizeof(SSAP_ParamFindByUuid_S));
        testData->appId = fdp.ConsumeIntegral<int32_t>();
        testData->addr.type = fdp.ConsumeIntegral<uint8_t>();
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            testData->addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        testData->type = static_cast<SSAP_FindType_E>(fdp.ConsumeIntegral<uint8_t>());
        for (uint8_t i = 0; i < SSAP_UUID128_LEN; i++) {
            testData->uuid.uuid[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        testData->startHandle = fdp.ConsumeIntegral<uint16_t>();
        testData->endHandle = fdp.ConsumeIntegral<uint16_t>();
        SSAPC_FindByUuidReq(link, (void*)testData);
        SDF_MemFree(testData);
        SSAP_DeleteSsapLinkByAddr(&sleAddr);
    }

    void FuzzSsapClientPktOther(uint8_t* data, size_t size)
    {
        FuzzedDataProvider fdp(data, size);
        SLE_Addr_S sleAddr = {0};
        sleAddr.addr[0] = SSAP_FUZZ_IDX2;
        uint8_t lcid = fdp.ConsumeIntegral<uint8_t>();
        SSAP_Link_S *link = SSAP_CreateSsapLink(&sleAddr, lcid, SendCb);
        if (link == nullptr) {
            return;
        }
        SDF_Buff_S *sdfBuff = SDF_BuffNewWithReserve(size);
        uint8_t *buf = SDF_BuffAppend(sdfBuff, size);
        if (sdfBuff == nullptr || buf == nullptr) {
            SDF_BuffFree(sdfBuff);
            return;
        }
        for (size_t i = 0; i < size; i++) {
            buf[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        link->curTask.buff = sdfBuff;

        if (size >= sizeof(SSAP_ExchangeInfoReqInfo_S)) {
            SSAP_ExchangeInfoReqInfo_S *testData =
                (SSAP_ExchangeInfoReqInfo_S *)SDF_MemZalloc(sizeof(SSAP_ExchangeInfoReqInfo_S));
            testData->appId = fdp.ConsumeIntegral<int32_t>();
            testData->addr.type = fdp.ConsumeIntegral<uint8_t>();
            for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
                testData->addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
            }
            testData->mtu = fdp.ConsumeIntegral<uint16_t>();
            testData->version = fdp.ConsumeIntegral<uint16_t>();
            SSAPC_ExchangeInfoReq(link, (void*)testData);
            SDF_MemFree(testData);
        }

        if (size >= sizeof(SSAP_ReadReqInfo_S)) {
            SSAP_ReadReqInfo_S *testData = (SSAP_ReadReqInfo_S *)SDF_MemZalloc(sizeof(SSAP_ReadReqInfo_S));
            testData->appId = fdp.ConsumeIntegral<int32_t>();
            testData->addr.type = fdp.ConsumeIntegral<uint8_t>();
            for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
                testData->addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
            }
            testData->handle = fdp.ConsumeIntegral<uint16_t>();
            testData->type = fdp.ConsumeIntegral<uint8_t>();
            SSAPC_ReadReq(link, (void*)testData);
            SDF_MemFree(testData);
        }

        if (size >= sizeof(SSAP_ReadByUuidReqInfo_S)) {
            SSAP_ReadByUuidReqInfo_S *testData =
                (SSAP_ReadByUuidReqInfo_S *)SDF_MemZalloc(sizeof(SSAP_ReadByUuidReqInfo_S));
            testData->appId = fdp.ConsumeIntegral<int32_t>();
            testData->addr.type = fdp.ConsumeIntegral<uint8_t>();
            for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
                testData->addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
            }
            for (uint8_t i = 0; i < SSAP_UUID128_LEN; i++) {
                testData->uuid.uuid[i] = fdp.ConsumeIntegral<uint8_t>();
            }
            testData->handleStart = fdp.ConsumeIntegral<uint16_t>();
            testData->handleEnd = fdp.ConsumeIntegral<uint16_t>();
            testData->dataType = fdp.ConsumeIntegral<uint8_t>();
            SSAPC_ReadByUuidReq(link, (void*)testData);
            SDF_MemFree(testData);
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
    FuzzedDataProvider fdp(data, size);
    std::vector<uint8_t> fuzzVec = fdp.ConsumeBytes<uint8_t>(size);
    uint8_t *fuzzData = fuzzVec.data();
    size_t fuzzSize = fuzzVec.size();
    OHOS::FuzzSsapClientApi(fuzzData, fuzzSize);
    return 0;
}