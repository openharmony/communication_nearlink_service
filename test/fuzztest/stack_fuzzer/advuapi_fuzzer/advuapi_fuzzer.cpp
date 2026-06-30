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
 
#include "advuapi_fuzzer.h"
#include "securec.h"
#include "nai_log.h"
#include "sdf_mem.h"
#include "nlstk_devd_def.h"
#include "nlstk_devd_api.h"
#include "nlstk_init_api.h"
#include "cpfwk_log.h"
#include "cm_api.h"
#include "devd_tbl.h"
#include "devd_adv.h"
#include "devd_local.h"
#include "nlstk_devd.h"
#include "dli_cmd_struct.h"
#include "devd_cbk.h"
#include "../../nl_utils/fuzztest_utils.h"
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
    FUZZ_TWENTY     = 20,
    FUZZ_TWOHUNDREDFIFTYSIX = 256,
    FUZZ_NUM_END
} FUZZ_NUM_E;

namespace OHOS {
    SleDliThreadUtil &g_dliThreadUtil = SleDliThreadUtil::GetInstance();

    void MockAdvNodeCbk(NLSTK_DevdAdvCbkParam_S *nodeParam) {}
    void MockAdvCbk(NLSTK_DevdAdvCbkParam_S *param) {}
    void FuzzEnableAdv(const uint8_t *fuzzData, size_t size)
    {
        if (size < sizeof(NLSTK_DevdSetAdvEnable_S)) {
            return;
        }

        NLSTK_DevdSetAdvEnable_S *advEnable =
            (NLSTK_DevdSetAdvEnable_S *)SDF_MemZalloc(sizeof(NLSTK_DevdSetAdvEnable_S));
        CP_CHECK_LOG_RETURN_VOID(advEnable != nullptr, "advEnable new fail");
        uint8_t handle = FUZZ_ONE;
        
        (void)memcpy_s(advEnable, sizeof(NLSTK_DevdSetAdvEnable_S), fuzzData, sizeof(NLSTK_DevdSetAdvEnable_S));
        advEnable->advHandle = handle;
        advEnable->enable = ADV_ENABLED;

        // 设定最大节点数量
        DLI_AdvCbkContext *cbkContext = (DLI_AdvCbkContext *)SDF_MemZalloc(sizeof(DLI_AdvCbkContext));
        CP_CHECK_LOG_RETURN_VOID(cbkContext != nullptr, "[Fuzz]memory alloc error");
        cbkContext->advHandle = handle;
        struct DLI_ExecuteCmdRetParam *cmdRes1 = (DLI_ExecuteCmdRetParam *)SDF_MemZalloc(
            sizeof(DLI_ExecuteCmdRetParam));
        CP_CHECK_LOG_RETURN_VOID(cmdRes1 != nullptr, "[Fuzz]memory alloc error");
        cmdRes1->size = sizeof(DevdAdvTerminatedEvent_S);
        cmdRes1->eventParameter = (DevdAdvTerminatedEvent_S *)SDF_MemZalloc(sizeof(DevdAdvTerminatedEvent_S));
        CP_CHECK_LOG_RETURN_VOID(cmdRes1->eventParameter != nullptr, "[Fuzz]memory alloc error");

        NLSTK_DevdCreateAdvHandle(MockAdvCbk);
        SDF_DListHead_S *list = DEVD_ADV_LIST;
        NLSTK_DevdAdvEventCbk cbk = MockAdvNodeCbk;
        NLSTK_DevdAdvCbkParam_S *nodeParam = (NLSTK_DevdAdvCbkParam_S *)SDF_MemZalloc(sizeof(NLSTK_DevdAdvCbkParam_S));
        CP_CHECK_LOG_RETURN_VOID(nodeParam != nullptr, "[Fuzz]memory alloc error");
        nodeParam->advHandle = handle;
        nodeParam->result = FUZZ_ONE;
        nodeParam->event = FUZZ_TWO;
        DevdAdvNode_S *fuzzNode = DevdGetAdvNode(handle, list);
        if (fuzzNode == nullptr) {
            fuzzNode = DevdCreateAdvNode(handle, cbk, list);
        }

        // 确保解析16位的数据
        advEnable->duration = fuzzData[FUZZ_TWO] | (fuzzData[FUZZ_THREE] << FUZZ_EIGHT);
        advEnable->advHandle = handle;

        NLSTK_DevdEnableAdv(advEnable);
        SDF_MemFree(advEnable);
        SDF_MemFree(cbkContext);
        SDF_MemFree(cmdRes1->eventParameter);
        SDF_MemFree(cmdRes1);
        SDF_MemFree(nodeParam);
    }

    void FuzzUapiSetAdv(const uint8_t *fuzzData, size_t size)
    {
        if (size < sizeof(NLSTK_DevdSetAdvParams_S)) {
            return;
        }

        NLSTK_DevdSetAdvParams_S advParams;
        NLSTK_DevdAdvParam_S advParam = {0};
        NLSTK_DevdAdvData_S advData;
        size_t ii = 0;

        advParam.advHandle = 1;
        advParam.advIntervalMin = (fuzzData[ii] << FUZZ_EIGHT) | fuzzData[ii + FUZZ_ONE];

        advParam.advIntervalMax = (fuzzData[ii + FUZZ_TWO] << FUZZ_EIGHT) | fuzzData[ii + FUZZ_THREE];
        ii += FUZZ_FOUR;

        advParam.advChannelMap = fuzzData[ii++] % FUZZ_THREE;

        CP_CHECK_LOG_RETURN_VOID(ii + SLE_ADDR_LEN + FUZZ_ONE <= size, "SLE_ADDR_LEN != FUZZ_SIX");

        for (int i = 0; i < SLE_ADDR_LEN; i++) {
            advParam.ownAddr.addr[i] = fuzzData[ii++];
        }

        CP_CHECK_LOG_RETURN_VOID(ii + FUZZ_FOUR <= size, "fuzzsize error");
        uint8_t *advDataBuf = (uint8_t *)malloc(FUZZ_TWOHUNDREDFIFTYSIX);
        uint8_t *scanRspDataBuf = (uint8_t *)malloc(FUZZ_TWOHUNDREDFIFTYSIX);
        CP_CHECK_LOG_RETURN_VOID(advDataBuf != nullptr && scanRspDataBuf != nullptr, "[Fuzz]memory alloc error");

        advData.advDataLen = (fuzzData[ii] << FUZZ_EIGHT) | (fuzzData[ii + FUZZ_ONE]);
        advData.scanRspDataLen = (fuzzData[ii + FUZZ_TWO] << FUZZ_EIGHT) | (fuzzData[ii + FUZZ_THREE]);
        ii += FUZZ_FOUR;

        if (ii + advData.advDataLen + advData.scanRspDataLen <= size &&
            advData.advDataLen <= FUZZ_TWOHUNDREDFIFTYSIX && advData.scanRspDataLen <= FUZZ_TWOHUNDREDFIFTYSIX) {
            if (memcpy_s(advDataBuf, FUZZ_TWOHUNDREDFIFTYSIX, fuzzData + ii, advData.advDataLen) != 0) {
                free(advDataBuf);
                free(scanRspDataBuf);
                return;
            }
            ii += advData.advDataLen;
            if (memcpy_s(scanRspDataBuf, FUZZ_TWOHUNDREDFIFTYSIX, fuzzData + ii, advData.scanRspDataLen) != 0) {
                free(advDataBuf);
                free(scanRspDataBuf);
                return;
            }
            ii += advData.scanRspDataLen;
        }

        // Set adv_data and scan_rsp_data
        advData.advData = advDataBuf;
        advData.scanRspData = scanRspDataBuf;

        // Create a struct to hold the parameters
        advParams.param = advParam;
        advParams.data = advData;
        advParams.param.extParam = nullptr; // Assuming ext_param is not used in this test
        if (memcpy_s(advParams.param.connParam, sizeof(NLSTK_DevdConnParam_S),
            fuzzData, sizeof(NLSTK_DevdConnParam_S)) != 0) {
            free(advDataBuf);
            free(scanRspDataBuf);
            return;
        }

        NLSTK_DevdStartAdv(&advParams);
        free(advDataBuf);
        free(scanRspDataBuf);
    }

    void FuzzUapiRemoveAdv(const uint8_t *fuzzData, size_t size)
    {
        uint8_t advHandle = 1;
        NLSTK_DevdRemoveAdv(&advHandle);
    }

    void FuzzUpdateAdvData(const uint8_t *fuzzData, size_t size)
    {
        if (size < sizeof(NLSTK_DevdSetAdvData_S)) {
            return;
        }
        int ii = 0;
        uint8_t handle = 1;

        NLSTK_DevdSetAdvData_S setData;
        setData.advHandle = handle;
        setData.data.advDataLen = (fuzzData[ii] << FUZZ_EIGHT) | (fuzzData[ii + FUZZ_ONE]);
        setData.data.scanRspDataLen = (fuzzData[ii + FUZZ_TWO] << FUZZ_EIGHT) | (fuzzData[ii + FUZZ_THREE]);
        ii += sizeof(uint16_t) * FUZZ_TWO;

        setData.data.advData = (uint8_t *)SDF_MemZalloc(setData.data.advDataLen);
        CP_CHECK_LOG_RETURN_VOID(setData.data.advData != nullptr, "[Fuzz]memory alloc error");
        setData.data.scanRspData = (uint8_t *)SDF_MemZalloc(setData.data.scanRspDataLen);
        CP_CHECK_LOG_RETURN_VOID(setData.data.scanRspData != nullptr, "[Fuzz]memory alloc error");

        if (size >= setData.data.advDataLen + ii) {
            (void)memcpy_s(setData.data.advData, setData.data.advDataLen, fuzzData + ii, setData.data.advDataLen);
        } else {
            SDF_MemFree(setData.data.advData);
            SDF_MemFree(setData.data.scanRspData);
            return;
        }
        if (size >= setData.data.advDataLen + setData.data.scanRspDataLen + ii) {
            (void)memcpy_s(setData.data.scanRspData, setData.data.scanRspDataLen, fuzzData + ii +
                setData.data.advDataLen, setData.data.scanRspDataLen);
        } else {
            SDF_MemFree(setData.data.advData);
            SDF_MemFree(setData.data.scanRspData);
            return;
        }

        NLSTK_DevdSetAdvData(&setData);
        SDF_MemFree(setData.data.advData);
        SDF_MemFree(setData.data.scanRspData);
    }

    void FuzzRemoveAdv(const uint8_t *fuzzData, size_t size)
    {
        if (size < sizeof(uint8_t)) {
            return;
        }
        uint8_t advHandle = 1;

        NLSTK_DevdRemoveAdv(&advHandle);
    }

    void FuzzAdvApi(const uint8_t *fuzzData, size_t size)
    {
        DevdLocalDeviceInit();
        FuzzUapiSetAdv(fuzzData, size);
        FuzzUpdateAdvData(fuzzData, size);
        FuzzEnableAdv(fuzzData, size);
        
        FuzzUapiRemoveAdv(fuzzData, size);
        FuzzRemoveAdv(fuzzData, size);
        DevdLocalDeviceDeInit();
    }

}

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void) argc;
    (void) argv;
    if (NLSTK_IsStackInited()) {
        NLSTK_DisableStack();
        NLSTK_DeinitStack();
    }
    NLSTK_InitStack();
    NLSTK_EnableStack();
    return 0;
}

/* Fuzzer entry point */
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