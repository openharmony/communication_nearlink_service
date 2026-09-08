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

#include "cdsmstack_fuzzer.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "cdsm_api.h"
#include "cdsm_event.h"
#include "nai_log.h"
#include "nlstk_init_api.h"
#include "sdf_evc.h"
#include "sdf_mem.h"
#include "sdf_thread.h"
#include "securec.h"

#define FUZZ_CDSM_MAX_ADDR_NUM 4

typedef struct SlePkt {
    uint8_t *data;
    uint32_t size;
} SlePkt;

int SleSendDliPacket(const SlePkt *packet)
{
    (void)packet;
    return 0;
}

namespace OHOS {

static void CdsmEventCbkImpl(NLSTK_CdsmEvent_S *event)
{
    (void)event;
}

static void FillAddr(FuzzedDataProvider &fdp, SLE_Addr_S &addr)
{
    addr.type = fdp.ConsumeIntegral<uint8_t>();
    for (int i = 0; i < SLE_ADDR_LEN; i++) {
        addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
    }
}

void FuzzCdsmCreateSet(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    SLE_Addr_S addr = {0};
    FillAddr(fdp, addr);
    (void)NLSTK_CdsmCreateSet(&addr);
}

void FuzzCdsmRemoveSet(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    uint32_t gid = fdp.ConsumeIntegral<uint32_t>();
    NLSTK_CdsmRemoveSet(gid);
}

void FuzzCdsmRecoverMeb(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    uint32_t gid = fdp.ConsumeIntegral<uint32_t>();
    uint8_t num = fdp.ConsumeIntegral<uint8_t>() % FUZZ_CDSM_MAX_ADDR_NUM;
    SLE_Addr_S addr[FUZZ_CDSM_MAX_ADDR_NUM];
    (void)memset_s(addr, sizeof(addr), 0, sizeof(addr));
    for (uint8_t i = 0; i < num; i++) {
        FillAddr(fdp, addr[i]);
    }
    NLSTK_CdsmRecoverMeb(gid, num, addr);
}

void FuzzCdsmAdv(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    SLE_Addr_S addr = {0};
    FillAddr(fdp, addr);
    NLSTK_CdsmStartAdv(&addr);
    NLSTK_CdsmStopAdv(&addr);
}

void FuzzCdsmConnect(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    SLE_Addr_S addr = {0};
    FillAddr(fdp, addr);
    (void)NLSTK_CdsmConnect(&addr);
    (void)NLSTK_CdsmDisconnect(&addr);
}

void FuzzCdsmFind(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    SLE_Addr_S addr = {0};
    FillAddr(fdp, addr);
    (void)NLSTK_CdsmFindGidByAddr(&addr);
    uint32_t gid = fdp.ConsumeIntegral<uint32_t>();
    (void)NLSTK_CdsmFindAllAddrByGid(gid);
}

}  // namespace OHOS

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc;
    (void)argv;
    NLSTK_InitStack();
    NLSTK_EnableStack();
    (void)SDF_ThreadInit(10);
    (void)SDF_EvcInit();
    NLSTK_CdsmRegisterEventCbk(OHOS::CdsmEventCbkImpl);
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    uint8_t *fuzzData = (uint8_t *)malloc(size);
    if (fuzzData == nullptr) {
        return 0;
    }
    (void)memcpy_s(fuzzData, size, data, size);
    OHOS::FuzzCdsmCreateSet(fuzzData, size);
    OHOS::FuzzCdsmRemoveSet(fuzzData, size);
    OHOS::FuzzCdsmRecoverMeb(fuzzData, size);
    OHOS::FuzzCdsmAdv(fuzzData, size);
    OHOS::FuzzCdsmConnect(fuzzData, size);
    OHOS::FuzzCdsmFind(fuzzData, size);
    SDF_MemFree(fuzzData);
    return 0;
}
