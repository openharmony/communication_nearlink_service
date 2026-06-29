/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#include "securec.h"
#include "sdf_mem.h"
#include "sdf_thread.h"
#include "sdf_evc.h"
#include "nlstk_log.h"
#include "nlstk_init_api.h"
#include "nlstk_ssap_app_server.h"
#include "ssapappserverapi_fuzzer.h"
#include "fuzzer/FuzzedDataProvider.h"

typedef enum {
    FUZZ_ZERO    = 0,
    FUZZ_ONE     = 1,
    FUZZ_TWO     = 2,
    FUZZ_THREE   = 3,
    FUZZ_FOUR    = 4,
    FUZZ_FIVE    = 5,
    FUZZ_SIX     = 6,
    FUZZ_SEVEN   = 7,
    FUZZ_EIGHT   = 8,
    FUZZ_SIXTEEN = 16,
    FUZZ_ONEHUNDREDTWENTYEIGHT = 128,
    FUZZ_TWOHUNDREDFIFTYSIX    = 256,
    FUZZ_NUM_END
} FUZZ_NUM_E;

namespace OHOS {

    int32_t appId = -1;

    void FuzzSsapServerRegApp(void)
    {
        NLSTK_SsapAppServerCb_S cb = {0};
        NLSTK_SsapServerRegApp(&cb, &appId);
    }

    void FuzzSsapServerDeRegApp(void)
    {
        NLSTK_SsapServerDeregisterApplication(appId);
    }

    void FuzzSsapServerSetMtu(uint8_t* data, size_t size)
    {
        uint16_t mtu = data[0];
        NLSTK_SsapServerSetMtu(mtu);
    }

    void FuzzSsapServerAddServices(void)
    {
        NLSTK_SsapServicePropertyParam_S property = {.descriptorNum = 0};

        NLSTK_SsapServiceMethodParam_S method = {.type = ITEM_TYPE_STD_PRIMARY_SERVICE};

        NLSTK_SsapServiceEventParam_S event = {.type = ITEM_TYPE_STD_PRIMARY_SERVICE};

        NLSTK_ServiceParam_S service = {
            .servicePropertyNum = 1,
            .property = &property,
            .serviceMethodNum = 1,
            .method = &method,
            .serviceEventNum = 1,
            .event = &event,
        };
        NLSTK_SsapServerAddService(appId, &service);
    }

    void FuzzSsapServerRemoveServices(uint8_t* data, size_t size)
    {
        uint16_t handle = data[0];
        NLSTK_SsapServerRemoveService(appId, handle);
    }

    void FuzzSsapServerClearServices(void)
    {
        NLSTK_SsapServerClearServices(appId);
    }

    void FuzzSsapServerAuthorizeResult(uint8_t* data, size_t size)
    { 
        uint16_t requestId = data[0];
        bool allow = true;
        NLSTK_SsapServerAuthorizeResult(appId, requestId, allow);
    }

    void FuzzSsapServerUpdatePropertyValue(uint8_t* data, size_t size)
    {
        uint16_t handle = data[0];
        NLSTK_VariableData_S *value;
        value = (NLSTK_VariableData_S *)SDF_MemZalloc(sizeof(NLSTK_VariableData_S) + size);
        value->len = size;
        NLSTK_SsapServerUpdatePropertyValue(appId, handle, value);
    }

    void FuzzSsapServerUpdateDescriptorValue(uint8_t* data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        uint16_t handle = provider.ConsumeIntegral<uint16_t>();
        uint8_t type = provider.ConsumeIntegral<uint8_t>();
        NLSTK_VariableData_S *value;
        value = (NLSTK_VariableData_S *)SDF_MemZalloc(sizeof(NLSTK_VariableData_S) + size);
        value->len = size;
        NLSTK_SsapServerUpdateDescriptorValue(appId, handle, type, value);
    }

    void FuzzSsapServerUpdateAndNotifyProperty(uint8_t* data, size_t size)
    {
        if (size < FUZZ_SIX) {
            return;
        }

        uint16_t handle = data[0];
        SLE_Addr_S addr = {
            .type = PUBLIC_ADDRESS,
            .addr = { data[0], data[1], data[2], data[3], data[4], data[5] },
        };
        NLSTK_VariableData_S *value;
        value = (NLSTK_VariableData_S *)SDF_MemZalloc(sizeof(NLSTK_VariableData_S) + size);
        value->len = size;
        NLSTK_SsapServerUpdateAndNotifyProperty(appId, handle, &addr, value);
    }

    void FuzzSsapServerCheckServiceExistByUuid(void)
    {
        NLSTK_SsapUuid_S uuid = {0};
        bool isExist;
        NLSTK_SsapServerCheckServiceExistByUuid(appId, &uuid, &isExist);
    }

    void FuzzSsapServerSendMethodCallRes(uint8_t* data, size_t size)
    {
        uint16_t requestId = data[0];
        NLSTK_VariableData_S *value;
        value = (NLSTK_VariableData_S *)SDF_MemZalloc(sizeof(NLSTK_VariableData_S) + size);
        value->len = size;
        NLSTK_SsapServerSendMethodCallRes(appId, requestId, value);
    }

    void FuzzStackServerApiDataProc(uint8_t* data, size_t size)
    {
        if (data == nullptr || size < 1) {
            return;
        }
        FuzzSsapServerRegApp();
        FuzzSsapServerSetMtu(data, size);
        FuzzSsapServerAddServices();
        FuzzSsapServerRemoveServices(data, size);
        FuzzSsapServerClearServices();
        FuzzSsapServerAuthorizeResult(data, size);
        FuzzSsapServerUpdatePropertyValue(data, size);
        FuzzSsapServerUpdateDescriptorValue(data, size);
        FuzzSsapServerUpdateAndNotifyProperty(data, size);
        FuzzSsapServerCheckServiceExistByUuid();
        FuzzSsapServerSendMethodCallRes(data, size);
        FuzzSsapServerDeRegApp();
    }
}

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void) argc;
    (void) argv;
    NLSTK_InitStack();
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
    OHOS::FuzzStackServerApiDataProc(fuzzData, size);
    free(fuzzData);
    return 0;
}