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

#include "cfgdb_fuzzer.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "nlstk_cfgdb_api.h"
#include "nai_log.h"
#include "nlstk_init_api.h"
#include "sdf_evc.h"
#include "sdf_mem.h"
#include "sdf_thread.h"
#include "securec.h"

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

static void FillAddr(FuzzedDataProvider &fdp, SLE_Addr_S &addr)
{
    addr.type = fdp.ConsumeIntegral<uint8_t>();
    for (int i = 0; i < SLE_ADDR_LEN; i++) {
        addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
    }
}

void FuzzCfgdbSetManufacturerAbility(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    SLE_Addr_S addr = {0};
    FillAddr(fdp, addr);
    NLSTK_ManufacturerAbility_S ability;
    (void)memset_s(&ability, sizeof(ability), 0, sizeof(ability));
    for (int i = 0; i < NLSTK_MANUFACTURER_ABILITY_LEN; i++) {
        ability.ability[i] = fdp.ConsumeIntegral<uint8_t>();
    }
    (void)NLSTK_CfgdbSetManufacturerAbility(&addr, &ability);
}

void FuzzCfgdbRemoveManufacturerAbility(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    SLE_Addr_S addr = {0};
    FillAddr(fdp, addr);
    (void)NLSTK_CfgdbRemoveManufacturerAbility(&addr);
}

void FuzzCfgdbGetPublicAddress(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    (void)fdp;
    SLE_Addr_S addr = {0};
    (void)NLSTK_CfgdbGetPublicAddress(&addr);
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
    OHOS::FuzzCfgdbSetManufacturerAbility(fuzzData, size);
    OHOS::FuzzCfgdbRemoveManufacturerAbility(fuzzData, size);
    OHOS::FuzzCfgdbGetPublicAddress(fuzzData, size);
    SDF_MemFree(fuzzData);
    return 0;
}
