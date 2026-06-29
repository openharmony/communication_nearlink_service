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

#include "adv_fuzzer.h"
#include "securec.h"
#include "nai_log.h"
#include "log.h"
#include "sdf_mem.h"
#include "nlstk_init_api.h"
#include "cpfwk_log.h"
#include "cm_api.h"
#include "devd_tbl.h"
#include "devd_adv.h"
#include "nlstk_devd.h"
#include "devd_local.h"
#include "dli_cmd_struct.h"
#include "dli_errno.h"
#include "devd_cbk.h"
#include "../../nl_utils/fuzztest_utils.h"
#include "SleDliThreadUtil.h"
#include "common_ext_func_wrapper.h"

using namespace OHOS::Nearlink;

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
    FUZZ_SIXTEEN     = 16,
    FUZZ_TWENTY     = 20,
    FUZZ_TWENTYFOUR     = 24,
    FUZZ_TWOHUNDREDFIFTYSIX = 256,
    FUZZ_NUM_END
} FUZZ_NUM_E;

namespace OHOS {
    SleDliThreadUtil &g_dliThreadUtil = SleDliThreadUtil::GetInstance();
    void MockAdvNodeCbk(NLSTK_DevdAdvCbkParam_S *nodeParam) {}

    void FuzzAdvDataCbk(const uint8_t *fuzzData, size_t size)
    {
        if (size < sizeof(DLI_ExecuteCmdRetParam)) {
            return;
        }
        DLI_ExecuteCmdRetParam cmdRes;
        
        uint8_t handle = 1;
        cmdRes.cmdOpcode = (fuzzData[FUZZ_ZERO] << FUZZ_EIGHT) | (fuzzData[FUZZ_ONE]);
        cmdRes.size = (fuzzData[FUZZ_TWO] << FUZZ_TWENTYFOUR) | (fuzzData[FUZZ_THREE] << FUZZ_SIXTEEN) |
            (fuzzData[FUZZ_FOUR] << FUZZ_EIGHT) | (fuzzData[FUZZ_FIVE]);
        cmdRes.size = sizeof(DevdAdvTerminatedEvent_S);
        cmdRes.eventParameter = (DevdAdvTerminatedEvent_S *)SDF_MemZalloc(sizeof(DevdAdvTerminatedEvent_S));
        CP_CHECK_LOG_RETURN_VOID(cmdRes.eventParameter != nullptr, "[Fuzz]memory alloc error");
        (void)memset_s(cmdRes.eventParameter, sizeof(DevdAdvTerminatedEvent_S), 0, sizeof(DevdAdvTerminatedEvent_S));

        // 调用回调函数
        DLI_AdvCbkContext context;
        context.advHandle = handle;
        
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

        DevdEnableAdvCbk(&context, 0, &cmdRes);
        DevdRemoveAdvCbk(&context, 0, &cmdRes);
        DevdAdvTerminatedCbk(&context, 0, &cmdRes);

        SDF_MemFree(cmdRes.eventParameter);
        SDF_MemFree(nodeParam);
    }

    void FuzzGetAdvNode(const uint8_t *fuzzData, size_t size)
    {
        if (size < sizeof(uint8_t) * FUZZ_TWO + sizeof(SDF_DListHead_S)) {
            return;
        }

        uint8_t handle = 1;
        SDF_DListHead_S *list = (SDF_DListHead_S *)malloc(sizeof(SDF_DListHead_S));
        if (list == nullptr) {
            return;
        }
        (void)memcpy_s(list, sizeof(SDF_DListHead_S), fuzzData + sizeof(uint8_t) * FUZZ_TWO, sizeof(SDF_DListHead_S));
        DevdGetAdvNode(handle, &(DevdGetLocalDevice()->advList));
        free(list);
        return;
    }

    void FuzzSetAdv(const uint8_t *fuzzData, size_t size)
    {
        if (size < sizeof(uint8_t) * FUZZ_THREE) {
            return;
        }
        int ii = 0;
        uint8_t handle = 1;

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
        
        DevdSetAdvParams_S *params = (DevdSetAdvParams_S *)SDF_MemZalloc(sizeof(DevdSetAdvParams_S));
        CP_CHECK_LOG_RETURN_VOID(params != nullptr, "[Fuzz]memory alloc error");
        params->param.handle = handle;
        params->data.advDataLen = fuzzData[ii++];
        params->data.advData = (uint8_t *)SDF_MemZalloc(params->data.advDataLen);
        CP_CHECK_LOG_RETURN_VOID(params->data.advData != nullptr, "[Fuzz]memory alloc error");
        DevdSetAdvParam(params);

        NLSTK_DevdSetAdvData_S *arg1 = (NLSTK_DevdSetAdvData_S *)SDF_MemZalloc(sizeof(NLSTK_DevdSetAdvData_S));
        CP_CHECK_LOG_RETURN_VOID(arg1 != nullptr, "[Fuzz]memory alloc error");

        arg1->advHandle = handle;
        arg1->data.advDataLen = fuzzData[ii++];
        arg1->data.scanRspDataLen = fuzzData[ii++];
        arg1->data.advData = (uint8_t *)SDF_MemZalloc(arg1->data.advDataLen);
        CP_CHECK_LOG_RETURN_VOID(arg1->data.advData != nullptr, "[Fuzz]memory alloc error");
        arg1->data.scanRspData = (uint8_t *)SDF_MemZalloc(arg1->data.scanRspDataLen);
        CP_CHECK_LOG_RETURN_VOID(arg1->data.scanRspData != nullptr, "[Fuzz]memory alloc error");
        if (size >= ii + arg1->data.advDataLen) {
            (void)memcpy_s(arg1->data.advData, arg1->data.advDataLen, fuzzData + ii, arg1->data.advDataLen);
            ii += arg1->data.advDataLen;
        }
        if (size >= ii + arg1->data.scanRspDataLen) {
            (void)memcpy_s(arg1->data.scanRspData, arg1->data.scanRspDataLen, fuzzData + ii, arg1->data.scanRspDataLen);
            ii += arg1->data.scanRspDataLen;
        }
        DevdSetAdvData(arg1);

        SDF_MemFree(nodeParam);
        SDF_MemFree(params->data.advData);
        SDF_MemFree(params);
        SDF_MemFree(arg1->data.advData);
        SDF_MemFree(arg1->data.scanRspData);
        SDF_MemFree(arg1);
    }

    void FuzzDevdCbk(uint8_t *fuzzData, size_t size)
    {
        if (size < sizeof(uint8_t) * FUZZ_THREE) {
            return;
        }
        uint8_t handle = 1;
        SDF_DListHead_S *list = DEVD_ADV_LIST;
        NLSTK_DevdAdvEventCbk cbk = MockAdvNodeCbk;
        
        DLI_ExecuteCmdRetParam *cmdRes = (DLI_ExecuteCmdRetParam *)SDF_MemZalloc(sizeof(DLI_ExecuteCmdRetParam));
        CP_CHECK_LOG_RETURN_VOID(cmdRes != nullptr, "[Fuzz]memory alloc error");
        cmdRes->eventParameter = (DevdAdvTerminatedEvent_S *)SDF_MemZalloc(sizeof(DevdAdvTerminatedEvent_S));
        CP_CHECK_LOG_RETURN_VOID(cmdRes->eventParameter != nullptr, "[Fuzz]memory alloc error");
        (void)memset_s(cmdRes->eventParameter, sizeof(DevdAdvTerminatedEvent_S), 0, sizeof(DevdAdvTerminatedEvent_S));
        DLI_AdvCbkContext *cbkContext = (DLI_AdvCbkContext *)SDF_MemZalloc(sizeof(DLI_AdvCbkContext));
        CP_CHECK_LOG_RETURN_VOID(cbkContext != nullptr, "[Fuzz]memory alloc error");
        cbkContext->advHandle = handle;
        uint16_t status = DLI_SUCCESS;  // 0:DLI_SUCCESS

        DevdSetAdvParamCbk(cbkContext, status, cmdRes);

        DevdAdvNode_S *fuzzNode2 = DevdGetAdvNode(handle, list);
        if (fuzzNode2 == nullptr) {
            fuzzNode2 = DevdCreateAdvNode(handle, cbk, list);
        }

        DevdSetAdvDataCbk(cbkContext, status, cmdRes);
        DevdEnableAdvCbk(cbkContext, status, cmdRes);
        DevdGetAdvNode(cbkContext->advHandle, DEVD_ADV_LIST);

        struct DLI_ExecuteCmdRetParam *cmdRes1 = nullptr;
        cmdRes1 = (DLI_ExecuteCmdRetParam *)SDF_MemZalloc(sizeof(DLI_ExecuteCmdRetParam));
        CP_CHECK_LOG_RETURN_VOID(cmdRes1 != nullptr, "[Fuzz]memory alloc error");
        cmdRes1->size = sizeof(DevdAdvTerminatedEvent_S);
        cmdRes1->eventParameter = (DevdAdvTerminatedEvent_S *)SDF_MemZalloc(sizeof(DevdAdvTerminatedEvent_S));
        CP_CHECK_LOG_RETURN_VOID(cmdRes1->eventParameter != nullptr, "[Fuzz]memory alloc error");
        (void)memset_s(cmdRes1->eventParameter, sizeof(DevdAdvTerminatedEvent_S), 0, sizeof(DevdAdvTerminatedEvent_S));

        DevdAdvTerminatedCbk(cbkContext, status, cmdRes1);
        DevdRemoveAdvCbk(cbkContext, status, cmdRes);

        SDF_MemFree(cmdRes->eventParameter);
        SDF_MemFree(cmdRes);
        SDF_MemFree(cbkContext);
        SDF_MemFree(cmdRes1->eventParameter);
        SDF_MemFree(cmdRes1);
    }

    void FuzzAdvApi(uint8_t *fuzzData, size_t size)
    {
        DevdLocalDeviceInit();
        FuzzAdvDataCbk(fuzzData, size);
        FuzzGetAdvNode(fuzzData, size);
        FuzzSetAdv(fuzzData, size);
        FuzzDevdCbk(fuzzData, size);
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