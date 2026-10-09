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
 
#include "advfree_fuzzer.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "securec.h"
#include "nai_log.h"
#include "sdf_mem.h"
#include "nlstk_init_api.h"
#include "cpfwk_log.h"
#include "cm_api.h"
#include "devd_tbl.h"
#include "devd_adv.h"
#include "nlstk_devd.h"
#include "devd_local.h"
#include "nlstk_devd_def.h"
#include "SleDliThreadUtil.h"
#include "common_ext_func_wrapper.h"

typedef struct SlePkt {
    uint8_t *data;
    uint32_t size;
} SlePkt;

int SleSendDliPacket(const SlePkt *packet)
{
    return 0;
}

typedef enum {
    FUZZ_ZERO    = 0,
    FUZZ_ONE     = 1,
    FUZZ_TWO     = 2,
    FUZZ_THREE   = 3,
    FUZZ_FOUR    = 4,
    FUZZ_FIVE    = 5,
    FUZZ_SIX     = 6,
    FUZZ_SEVEN     = 7,
    FUZZ_EIGHT     = 8,
    FUZZ_NUM_END
} FUZZ_NUM_E;

namespace OHOS {
    SleDliThreadUtil &g_dliThreadUtil = SleDliThreadUtil::GetInstance();

    void FuzzDEVDEnableAdv(const uint8_t *fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);

        NLSTK_DevdSetAdvEnable_S params = {0};
        params.advHandle = fdp.ConsumeIntegral<uint8_t>();
        params.enable = fdp.ConsumeIntegral<uint8_t>();
        params.duration = fdp.ConsumeIntegral<uint16_t>();
        params.maxAdvEvent = fdp.ConsumeIntegral<uint8_t>();
        DevdEnableAdv(&params);
    }

    void FuzzFreeAdvData(const uint8_t *fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);

        NLSTK_DevdAdvData_S *advData = (NLSTK_DevdAdvData_S *)SDF_MemZalloc(sizeof(NLSTK_DevdAdvData_S));
        if (advData == NULL) {
            return;
        }

        uint8_t advDataLen = fdp.ConsumeIntegral<uint8_t>();
        uint8_t scanRspDataLen = fdp.ConsumeIntegral<uint8_t>();
        advData->advDataLen = advDataLen;
        advData->scanRspDataLen = scanRspDataLen;
        advData->advData = (uint8_t *)SDF_MemZalloc(advDataLen);
        advData->scanRspData = (uint8_t *)SDF_MemZalloc(scanRspDataLen);

        if (advData->advData == NULL || advData->scanRspData == NULL) {
            SDF_MemFree(advData->advData);
            SDF_MemFree(advData->scanRspData);
            SDF_MemFree(advData);
            return;
        }

        for (uint8_t i = 0; i < advDataLen; i++) {
            advData->advData[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        for (uint8_t i = 0; i < scanRspDataLen; i++) {
            advData->scanRspData[i] = fdp.ConsumeIntegral<uint8_t>();
        }

        DevdFreeAdvData(advData);
    }

    void FuzzFreeSetAdvData(const uint8_t *fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);

        NLSTK_DevdSetAdvData_S *advData = (NLSTK_DevdSetAdvData_S *)SDF_MemZalloc(sizeof(NLSTK_DevdSetAdvData_S));
        if (advData == NULL) {
            return;
        }

        advData->advHandle = fdp.ConsumeIntegral<uint8_t>();
        uint8_t advDataLen = fdp.ConsumeIntegral<uint8_t>();
        uint8_t scanRspDataLen = fdp.ConsumeIntegral<uint8_t>();
        advData->data.advDataLen = advDataLen;
        advData->data.scanRspDataLen = scanRspDataLen;

        advData->data.advData = (uint8_t *)SDF_MemZalloc(advDataLen);
        advData->data.scanRspData = (uint8_t *)SDF_MemZalloc(scanRspDataLen);
        if (advData->data.advData == NULL || advData->data.scanRspData == NULL) {
            SDF_MemFree(advData->data.advData);
            SDF_MemFree(advData->data.scanRspData);
            SDF_MemFree(advData);
            return;
        }

        for (uint8_t i = 0; i < advDataLen; i++) {
            advData->data.advData[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        for (uint8_t i = 0; i < scanRspDataLen; i++) {
            advData->data.scanRspData[i] = fdp.ConsumeIntegral<uint8_t>();
        }

        DevdFreeSetAdvData(advData);
    }

    void FuzzFreeSetAdvDataParam(const uint8_t *fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);

        DevdSetAdvParams_S *advParams = (DevdSetAdvParams_S *)SDF_MemZalloc(sizeof(DevdSetAdvParams_S));
        if (advParams == NULL) {
            return;
        }

        advParams->accessMode = fdp.ConsumeIntegral<uint8_t>();
        advParams->param.handle = fdp.ConsumeIntegral<uint8_t>();
        advParams->param.discoveryLevel = fdp.ConsumeIntegral<uint8_t>();
        advParams->param.filterPolicy = fdp.ConsumeIntegral<uint8_t>();
        advParams->param.sid = fdp.ConsumeIntegral<uint8_t>();
        advParams->param.txPower = fdp.ConsumeIntegral<int8_t>();

        advParams->param.basic.advMode = fdp.ConsumeIntegral<uint8_t>();
        advParams->param.basic.gtRole = fdp.ConsumeIntegral<uint8_t>();
        advParams->param.basic.channelMap = fdp.ConsumeIntegral<uint8_t>();
        advParams->param.basic.advMinInterval = fdp.ConsumeIntegral<uint32_t>();
        advParams->param.basic.advMaxInterval = fdp.ConsumeIntegral<uint32_t>();
        advParams->param.basic.ownAddr.type = fdp.ConsumeIntegral<uint8_t>();
        advParams->param.basic.peerAddr.type = fdp.ConsumeIntegral<uint8_t>();
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            advParams->param.basic.ownAddr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
            advParams->param.basic.peerAddr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        advParams->param.basic.primaryFrameType = fdp.ConsumeIntegral<uint8_t>();

        advParams->param.connect.connMinInterval = fdp.ConsumeIntegral<uint16_t>();
        advParams->param.connect.connMaxInterval = fdp.ConsumeIntegral<uint16_t>();
        advParams->param.connect.maxLatency = fdp.ConsumeIntegral<uint16_t>();
        advParams->param.connect.supervisionTimeout = fdp.ConsumeIntegral<uint16_t>();
        advParams->param.connect.minCeLength = fdp.ConsumeIntegral<uint16_t>();
        advParams->param.connect.maxCeLength = fdp.ConsumeIntegral<uint16_t>();

        advParams->param.phy.primAdvFrameFormat = fdp.ConsumeIntegral<uint8_t>();
        advParams->param.phy.primaryPhy = fdp.ConsumeIntegral<uint8_t>();
        advParams->param.phy.secondaryFrame = fdp.ConsumeIntegral<uint8_t>();
        advParams->param.phy.secondaryPhy = fdp.ConsumeIntegral<uint8_t>();
        advParams->param.phy.secondaryPilot = fdp.ConsumeIntegral<uint8_t>();
        advParams->param.phy.secondaryMcs = fdp.ConsumeIntegral<uint8_t>();
        advParams->param.phy.secondaryMaxSkip = fdp.ConsumeIntegral<uint8_t>();

        advParams->param.scan.notifyEnable = fdp.ConsumeIntegral<uint8_t>();
        advParams->param.scan.rxMaxNumber = fdp.ConsumeIntegral<uint8_t>();
        advParams->param.scan.rxMaxDuration = fdp.ConsumeIntegral<uint16_t>();

        uint8_t advDataLen = fdp.ConsumeIntegral<uint8_t>();
        uint8_t scanRspDataLen = fdp.ConsumeIntegral<uint8_t>();
        advParams->data.advDataLen = advDataLen;
        advParams->data.scanRspDataLen = scanRspDataLen;
        advParams->data.advData = (uint8_t *)SDF_MemZalloc(advDataLen);
        advParams->data.scanRspData = (uint8_t *)SDF_MemZalloc(scanRspDataLen);
        if (advParams->data.advData == NULL || advParams->data.scanRspData == NULL) {
            SDF_MemFree(advParams->data.advData);
            SDF_MemFree(advParams->data.scanRspData);
            SDF_MemFree(advParams);
            return;
        }

        for (uint8_t i = 0; i < advDataLen; i++) {
            advParams->data.advData[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        for (uint8_t i = 0; i < scanRspDataLen; i++) {
            advParams->data.scanRspData[i] = fdp.ConsumeIntegral<uint8_t>();
        }

        DevdFreeSetAdvParams(advParams);
    }

    void FuzzFreeAdvNode(const uint8_t *fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);
        DevdAdvNode_S *node = (DevdAdvNode_S *)SDF_MemZalloc(sizeof(DevdAdvNode_S));
        CP_CHECK_LOG_RETURN_VOID(node != NULL, "[Fuzz]memory alloc error");
        node->handle = fdp.ConsumeIntegral<uint8_t>();
        node->param = (DevdAdvParam_S *)SDF_MemZalloc(sizeof(DevdAdvParam_S));
        node->tempParam = (DevdAdvParam_S *)SDF_MemZalloc(sizeof(DevdAdvParam_S));
        if (node->param == NULL || node->tempParam == NULL) {
            SDF_MemFree(node->param);
            SDF_MemFree(node->tempParam);
            SDF_MemFree(node);
            return;
        }

        node->data = (NLSTK_DevdAdvData_S *)SDF_MemZalloc(sizeof(NLSTK_DevdAdvData_S));
        node->tempData = (NLSTK_DevdAdvData_S *)SDF_MemZalloc(sizeof(NLSTK_DevdAdvData_S));
        CP_CHECK_LOG_RETURN_VOID(node->data != NULL, "[Fuzz]memory alloc error");
        CP_CHECK_LOG_RETURN_VOID(node->tempData != NULL, "[Fuzz]memory alloc error");
        uint8_t advDataLen = fdp.ConsumeIntegral<uint8_t>();
        uint8_t scanRspDataLen = fdp.ConsumeIntegral<uint8_t>();
        node->data->advDataLen = advDataLen;
        node->data->scanRspDataLen = scanRspDataLen;
        node->data->advData = (uint8_t *)SDF_MemZalloc(advDataLen);
        node->data->scanRspData = (uint8_t *)SDF_MemZalloc(scanRspDataLen);
        CP_CHECK_LOG_RETURN_VOID(node->data->advData != NULL, "[Fuzz]memory alloc error");
        CP_CHECK_LOG_RETURN_VOID(node->data->scanRspData != NULL, "[Fuzz]memory alloc error");

        for (uint8_t i = 0; i < advDataLen; i++) {
            node->data->advData[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        for (uint8_t i = 0; i < scanRspDataLen; i++) {
            node->data->scanRspData[i] = fdp.ConsumeIntegral<uint8_t>();
        }

        DevdFreeAdvNode(node);
    }

    void FuzzAdvApi(const uint8_t *fuzzData, size_t size)
    {
        DevdLocalDeviceInit();
        FuzzDEVDEnableAdv(fuzzData, size);
        FuzzFreeAdvData(fuzzData, size);
        FuzzFreeSetAdvData(fuzzData, size);
        FuzzFreeSetAdvDataParam(fuzzData, size);
        FuzzFreeAdvNode(fuzzData, size);
        DevdLocalDeviceDeInit();
    }
}

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void) argc;
    (void) argv;
    NLSTK_InitStack();
    NLSTK_EnableStack();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::g_dliThreadUtil.InitThread();
    uint8_t *fuzzData = (uint8_t *)malloc(size);
    if (fuzzData == nullptr) {
        OHOS::g_dliThreadUtil.DestroyQueue();
        return 0;
    }
    memcpy_s(fuzzData, size, data, size);
    OHOS::FuzzAdvApi(fuzzData, size);
    SDF_MemFree(fuzzData);
    OHOS::g_dliThreadUtil.DestroyQueue();
    return 0;
}