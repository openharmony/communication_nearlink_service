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
 
#include "advfree_fuzzer.h"
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
        if (size < sizeof(NLSTK_DevdSetAdvEnable_S)) {
            return;
        }

        NLSTK_DevdSetAdvEnable_S params = {0};
        if (memcpy_s(&params, sizeof(NLSTK_DevdSetAdvEnable_S), fuzzData, sizeof(NLSTK_DevdSetAdvEnable_S)) != EOK) {
            NAI_LOG_ERROR("memcpy_s fail\n");
            return;
        }
        DevdEnableAdv(&params);
    }

    void FuzzDEVDSetTxPower(const uint8_t *fuzzData, size_t size)
    {
        if (size < sizeof(NLSTK_DevdSetTxPower_S)) {
            return;
        }

        NLSTK_DevdSetTxPower_S params = {0};
        if (memcpy_s(&params, sizeof(NLSTK_DevdSetTxPower_S), fuzzData, sizeof(NLSTK_DevdSetTxPower_S)) != EOK) {
            NAI_LOG_ERROR("memcpy_s fail\n");
            return;
        }
        DevdSetTxPower(&params);
    }

    void FuzzFreeAdvData(const uint8_t *fuzzData, size_t size)
    {
        if (size < sizeof(NLSTK_DevdAdvData_S)) {
            return;
        }

        NLSTK_DevdAdvData_S *advData = (NLSTK_DevdAdvData_S *)malloc(sizeof(NLSTK_DevdAdvData_S));
        if (!advData) {
            return;
        }

        advData->advDataLen = (fuzzData[FUZZ_ZERO] << FUZZ_EIGHT) | (fuzzData[FUZZ_ONE]);
        advData->scanRspDataLen = (fuzzData[FUZZ_TWO] << FUZZ_EIGHT) | (fuzzData[FUZZ_THREE]);
        advData->advData = (uint8_t *)malloc(advData->advDataLen);
        advData->scanRspData = (uint8_t *)malloc(advData->scanRspDataLen);

        if (advData->advData && size >= sizeof(NLSTK_DevdAdvData_S) + advData->advDataLen) {
            (void)memcpy_s(advData->advData, advData->advDataLen, fuzzData + sizeof(NLSTK_DevdAdvData_S),
                advData->advDataLen);
        }
        if (advData->scanRspData && size >= sizeof(NLSTK_DevdAdvData_S) + advData->advDataLen + advData->scanRspDataLen) {
            (void)memcpy_s(advData->scanRspData, advData->scanRspDataLen, fuzzData + sizeof(NLSTK_DevdAdvData_S) +
                advData->advDataLen, advData->scanRspDataLen);
        }

        DevdFreeAdvData(advData);
    }

    void FuzzFreeSetAdvData(const uint8_t *fuzzData, size_t size)
    {
        if (size < sizeof(NLSTK_DevdSetAdvData_S)) {
            return;
        }

        NLSTK_DevdSetAdvData_S *advData = (NLSTK_DevdSetAdvData_S *)malloc(sizeof(NLSTK_DevdSetAdvData_S));
        if (!advData) {
            return; // 内存分配失败
        }

        advData->advHandle = fuzzData[0];
        advData->data.advDataLen = (fuzzData[FUZZ_ONE] << FUZZ_EIGHT) | (fuzzData[FUZZ_TWO]);
        advData->data.scanRspDataLen = (fuzzData[FUZZ_THREE] << FUZZ_EIGHT) | (fuzzData[FUZZ_FOUR]);

        size_t offset = FUZZ_FIVE;
        advData->data.advData = (uint8_t *)malloc(advData->data.advDataLen);
        if (advData->data.advData && size >= offset + advData->data.advDataLen) {
            (void)memcpy_s(advData->data.advData, advData->data.advDataLen,
                fuzzData + offset, advData->data.advDataLen);
        }

        offset += advData->data.advDataLen;
        advData->data.scanRspData = (uint8_t *)malloc(advData->data.scanRspDataLen);
        if (advData->data.scanRspData && size >= offset + advData->data.scanRspDataLen) {
            (void)memcpy_s(advData->data.scanRspData, advData->data.scanRspDataLen, fuzzData + offset,
                advData->data.scanRspDataLen);
        }

        DevdFreeSetAdvData(advData);
    }

    void FuzzFreeSetAdvDataParam(const uint8_t *fuzzData, size_t size)
    {
        if (size < sizeof(DevdSetAdvParams_S)) {
            return;
        }

        DevdSetAdvParams_S *advParams = (DevdSetAdvParams_S *)malloc(sizeof(DevdSetAdvParams_S));
        if (!advParams) {
            return;
        }

        advParams->accessMode = fuzzData[0];
        (void)memcpy_s(&advParams->param, sizeof(DevdAdvParam_S), fuzzData + 1, sizeof(DevdAdvParam_S));

        advParams->param.handle = fuzzData[1];
        advParams->param.discoveryLevel = fuzzData[FUZZ_TWO];
        advParams->param.filterPolicy = fuzzData[FUZZ_THREE];
        advParams->param.sid = fuzzData[FUZZ_FOUR];
        advParams->param.txPower = fuzzData[FUZZ_FIVE];

        (void)memcpy_s(&advParams->param.basic, sizeof(DevdAdvBasicParam_S), fuzzData + FUZZ_SIX,
            sizeof(DevdAdvBasicParam_S));
        (void)memcpy_s(&advParams->param.connect, sizeof(DevdAdvConnectParam_S), fuzzData + FUZZ_SIX +
            sizeof(DevdAdvBasicParam_S), sizeof(DevdAdvConnectParam_S));
        (void)memcpy_s(&advParams->param.phy, sizeof(DevdAdvPhyParam_S), fuzzData + FUZZ_SIX +
            sizeof(DevdAdvBasicParam_S) + sizeof(DevdAdvConnectParam_S), sizeof(DevdAdvPhyParam_S));
        (void)memcpy_s(&advParams->param.scan, sizeof(DevdAdvScanParam_S),
            fuzzData + FUZZ_SIX + sizeof(DevdAdvBasicParam_S)
            + sizeof(DevdAdvConnectParam_S) + sizeof(DevdAdvPhyParam_S), sizeof(DevdAdvScanParam_S));

        size_t offset = 6 + sizeof(DevdAdvBasicParam_S) + sizeof(DevdAdvConnectParam_S) + sizeof(DevdAdvPhyParam_S)
            + sizeof(DevdAdvScanParam_S);
        advParams->data.advData = (uint8_t *)malloc(advParams->data.advDataLen);
        if (advParams->data.advData && size >= offset + advParams->data.advDataLen) {
            (void)memcpy_s(advParams->data.advData, advParams->data.advDataLen, fuzzData +
                offset, advParams->data.advDataLen);
        }

        offset += advParams->data.advDataLen;
        advParams->data.scanRspData = (uint8_t *)malloc(advParams->data.scanRspDataLen);
        if (advParams->data.scanRspData && size >= offset + advParams->data.scanRspDataLen) {
            (void)memcpy_s(advParams->data.scanRspData, advParams->data.scanRspDataLen, fuzzData + offset,
                advParams->data.scanRspDataLen);
        }

        DevdFreeSetAdvParams(advParams);
    }

    void FuzzFreeAdvNode(const uint8_t *fuzzData, size_t size)
    {
        if (size < sizeof(DevdAdvNode_S) + sizeof(uint16_t) * FUZZ_TWO) {
            return;
        }
        int ii = 0;
        DevdAdvNode_S *node = (DevdAdvNode_S *)SDF_MemZalloc(sizeof(DevdAdvNode_S));
        CP_CHECK_LOG_RETURN_VOID(node != NULL, "[Fuzz]memory alloc error");
        node->handle = 1;
        node->param = (DevdAdvParam_S *)SDF_MemZalloc(sizeof(DevdAdvParam_S));
        node->tempParam  = (DevdAdvParam_S *)SDF_MemZalloc(sizeof(DevdAdvParam_S));
        if (node->param == nullptr || node->tempParam == nullptr) {
            return;
        }
        
        node->data = (NLSTK_DevdAdvData_S *)SDF_MemZalloc(sizeof(NLSTK_DevdAdvData_S));
        node->tempData = (NLSTK_DevdAdvData_S *)SDF_MemZalloc(sizeof(NLSTK_DevdAdvData_S));
        CP_CHECK_LOG_RETURN_VOID(node->data != NULL, "[Fuzz]memory alloc error");
        CP_CHECK_LOG_RETURN_VOID(node->tempData != NULL, "[Fuzz]memory alloc error");
        node->data->advDataLen = (fuzzData[ii] << FUZZ_EIGHT) | (fuzzData[ii + FUZZ_ONE]);
        node->data->scanRspDataLen = (fuzzData[ii + FUZZ_TWO] << FUZZ_EIGHT) | (fuzzData[ii + FUZZ_THREE]);
        ii += sizeof(uint16_t) * FUZZ_TWO;
        node->data->advData = (uint8_t *)SDF_MemZalloc(node->data->advDataLen);
        node->data->scanRspData = (uint8_t *)SDF_MemZalloc(node->data->scanRspDataLen);
        CP_CHECK_LOG_RETURN_VOID(node->data->advData != NULL, "[Fuzz]memory alloc error");
        CP_CHECK_LOG_RETURN_VOID(node->data->scanRspData != NULL, "[Fuzz]memory alloc error");
        if (node->data->advData && size >= sizeof(DevdAdvNode_S) + sizeof(uint16_t) * FUZZ_TWO +
            node->data->advDataLen) {
            (void)memcpy_s(node->data->advData, node->data->advDataLen, fuzzData + ii, node->data->advDataLen);
        }
        if (node->data->scanRspData && size >= sizeof(DevdAdvNode_S) + sizeof(uint16_t) * FUZZ_TWO +
            node->data->advDataLen + node->data->scanRspDataLen) {
            (void)memcpy_s(node->data->scanRspData, node->data->scanRspDataLen, fuzzData + ii +
                node->data->advDataLen, node->data->scanRspDataLen);
        }
        ii += (node->data->advDataLen + node->data->scanRspDataLen);

        DevdFreeAdvNode(node);
    }

    void FuzzAdvApi(const uint8_t *fuzzData, size_t size)
    {
        DevdLocalDeviceInit();
        FuzzDEVDEnableAdv(fuzzData, size);
        FuzzDEVDSetTxPower(fuzzData, size);
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