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

#include "ssapclientuapi_fuzzer.h"
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
#include "nlstk_schedule.h"
#include "nlstk_log.h"
#include "sdf_mem.h"
#include "sm_stm.h"
#include "sm_slink.h"
#include "sdf_map.h"
#include "ssapc_cache.h"
#include "sle_logic_link_mgr.h"
#include "nlstk_schedule.h"
#include "nlstk_ssap_app_link.h"
#include "nlstk_ssap_app_client.h"

extern SDF_Map *g_slinkMap;

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

static NLSTK_SsapUuid_S g_uuid1 = {.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02}};

void SSAP_DeleteSsapLinkByAddr_stub(void *param)
{
    SSAP_DeleteSsapLinkByAddr((SLE_Addr_S *)param);
}

void SleLogicLinkRemove_stub(void *param)
{
    SleLogicLinkRemove((SleLogicLink_S *)param);
}

namespace OHOS {

    char random_hex_byte()
    {
        const char* hex_digits = "0123456789abcdef";
        int random_byte = rand() % 16;
        return hex_digits[random_byte];
    }

    void SsapClientFreeFuncCb (NLSTK_SsapServ_S *service, uint16_t serviceNum)
    {
        return;
    }

    void SsapClientCleanAppResultCb()
    {
        return;
    }

    void SendFunc(SSAP_Link_S *link, SDF_Buff_S *buff, uint8_t opcode)
    {
        (void)link;
        (void)buff;
        (void)opcode;
    }

    void FuzzSsapClientUapi(uint8_t* data, size_t size)
    {
        if (size < SLE_ADDR_LEN) {
            return;
        }
        FuzzedDataProvider provider(data, size);
        int32_t appId = -1;
        NLSTK_SsapAppClientCb_S cb = {0};
        SLE_Addr_S addr = {0};
        memcpy_s(addr.addr, SLE_ADDR_LEN, data, SLE_ADDR_LEN);
        SleLogicLinkInit();
        SleLogicLink_S *node = SleLogicLinkAdd(&addr);
        if (node == nullptr) {
            return;
        }
        SSAP_LinkInit();
        node->lcid = provider.ConsumeIntegral<uint16_t>();
        SSAP_Link_S *link = SSAP_CreateSsapLink(&addr, provider.ConsumeIntegral<uint16_t>(), SendFunc);
        if (link == nullptr) {
            SleLogicLinkRemove(node);
            return;
        }

        NLSTK_SsapClientRegApp(&appId, &cb, &addr);

        uint16_t mtu = 128;
        NLSTK_SsapClientExchangeMtu(appId, mtu);

        NLSTK_SsapClientDiscoverServices(appId, 0, 0, provider.ConsumeIntegral<uint8_t>());

        NLSTK_SsapClientDiscoverServicesByUuid(appId, &g_uuid1, provider.ConsumeIntegral<uint16_t>(),
            provider.ConsumeIntegral<uint16_t>(), provider.ConsumeIntegral<uint8_t>());

        NLSTK_SsapServ_S *service;
        uint16_t serviceNum = 0;
        NLSTK_SsapClientFreeFunc func = &SsapClientFreeFuncCb;
        NLSTK_SsapClientGetServices(appId, &service, &serviceNum, &func);

        NLSTK_SsapClientGetServicesByUuid(appId, &g_uuid1, &service, &serviceNum, &func);

        NLSTK_SsapClientReadProperty(appId, provider.ConsumeIntegral<uint16_t>());

        NLSTK_SsapClientGetPropertyNtf(appId, provider.ConsumeIntegral<uint16_t>());

        NLSTK_SsapClientSetPropertyNtf(appId, provider.ConsumeIntegral<uint16_t>(), true);

        NLSTK_VariableData_S value = {0};
        value.len = size;
        value.data = data;
        NLSTK_SsapClientWriteProperty(appId, provider.ConsumeIntegral<uint16_t>(), &value, true);
        NLSTK_SsapClientWriteProperty(appId, provider.ConsumeIntegral<uint16_t>(), &value, false);

        NLSTK_SsapClientCallMethod(appId, provider.ConsumeIntegral<uint16_t>(), &value, true);
        NLSTK_SsapClientCallMethod(appId, provider.ConsumeIntegral<uint16_t>(), &value, false);

        NLSTK_SsapCleanClientApp(SsapClientCleanAppResultCb);

        SchedulePostTask(SSAP_DeleteSsapLinkByAddr_stub, (void *)&addr, NULL);
        SchedulePostTask(SleLogicLinkRemove_stub, (void *)node, NULL);

        NLSTK_SsapClientDeregApp(appId);
    }

    void FuzzSsapClientUapi2(uint8_t* data, size_t size)
    {
        if (size < SLE_ADDR_LEN) {
            return;
        }
        FuzzedDataProvider provider(data, size);
        SLE_Addr_S addr = {0};
        memcpy_s(addr.addr, SLE_ADDR_LEN, data, SLE_ADDR_LEN);

        SleLogicLinkInit();
        SleLogicLink_S *node = SleLogicLinkAdd(&addr);
        if (node == nullptr) {
            return;
        }
        node->lcid = provider.ConsumeIntegral<uint16_t>();
        SSAP_Link_S *link = SSAP_CreateSsapLink(&addr, provider.ConsumeIntegral<uint16_t>(), SendFunc);
        if (link == nullptr) {
            SleLogicLinkRemove(node);
            return;
        }

        int32_t appId = -1;
        NLSTK_SsapAppClientCb_S cb = {0};
        
        SLE_Addr_S *key = (SLE_Addr_S *)SDF_MemAlloc(sizeof(SLE_Addr_S));
        memcpy_s(key, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));
        SmSLink_S *slink = SmSLinkCtor(&addr);
        g_slinkMap = SDF_MapCtor(SLE_ADDR_TRAITS(), MAKE_TRAITS(SmSLinkDtor, NULL));
        SDF_MapMoveInsert(g_slinkMap, key, slink);

        NLSTK_SsapClientRegApp(&appId, &cb, &addr);

        NLSTK_SsapClientReadPropertyByUuid(appId, &g_uuid1, 0, provider.ConsumeIntegral<uint16_t>());

        NLSTK_SsapClientReadDescriptor(appId, provider.ConsumeIntegral<uint16_t>(), provider.ConsumeIntegral<uint8_t>());

        NLSTK_SsapClientGetPropertyInd(appId, provider.ConsumeIntegral<uint16_t>());

        NLSTK_SsapClientSetPropertyInd(appId, provider.ConsumeIntegral<uint16_t>(), provider.ConsumeBool());

        NLSTK_VariableData_S value = {0};
        value.len = size;
        value.data = data;
        NLSTK_SsapClientWriteDescriptor(appId, provider.ConsumeIntegral<uint16_t>(), &value,
            provider.ConsumeIntegral<uint8_t>(), provider.ConsumeBool());

        SchedulePostTask(SSAP_DeleteSsapLinkByAddr_stub, (void *)&addr, NULL);
        SchedulePostTask(SleLogicLinkRemove_stub, (void *)node, NULL);

        NLSTK_SsapClientDeregApp(appId);
        if (g_slinkMap) {
            SDF_MapDtor(g_slinkMap);
            g_slinkMap = NULL;
        }
    }

    void FuzzSsapClientUapi3(void)
    {
        NLSTK_SsapServ_S *serv = (NLSTK_SsapServ_S *)SDF_MemZalloc(sizeof(NLSTK_SsapServ_S));
        NLSTK_SsapPrty_S *property = (NLSTK_SsapPrty_S *)SDF_MemZalloc(sizeof(NLSTK_SsapPrty_S));
        NLSTK_SsapDtor_S *descriptor = (NLSTK_SsapDtor_S *)SDF_MemZalloc(sizeof(NLSTK_SsapDtor_S));
        property->descriptorNum = 1;
        property->descriptors = descriptor;
        serv->properties = property;
        serv->propertyNum = 1;
        uint16_t num = 1;
        SsapcCacheFreeServices(serv, num);
    }

    void FuzzSsapClientApi(uint8_t* data, size_t size)
    {
        if (data == nullptr || size < 1) {
            return;
        }
        FuzzSsapClientUapi(data, size);
        FuzzSsapClientUapi2(data, size);
        FuzzSsapClientUapi3();
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
    OHOS::FuzzSsapClientApi(static_cast<uint8_t *>(fuzzData), size);
    SDF_MemFree(fuzzData);
    return 0;
}