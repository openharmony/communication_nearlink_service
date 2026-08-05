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

#include "disclient_fuzzer.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "nlstk_dis_client.h"
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

static void DisConnectStateChangeCbkImpl(SLE_Addr_S *addr, NLSTK_DisConnectState_E curState,
    NLSTK_DisConnectState_E prevState, NLSTK_Errcode_E errNumb)
{
    (void)addr;
    (void)curState;
    (void)prevState;
    (void)errNumb;
}

static void FillAddr(FuzzedDataProvider &fdp, SLE_Addr_S &addr)
{
    addr.type = fdp.ConsumeIntegral<uint8_t>();
    for (int i = 0; i < SLE_ADDR_LEN; i++) {
        addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
    }
}

void FuzzDisProfileConnect(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    SLE_Addr_S addr = {0};
    FillAddr(fdp, addr);
    (void)NLSTK_DisProfileConnect(&addr);
    (void)NLSTK_DisProfileDisconnect(&addr);
}

void FuzzDisReadInfo(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    SLE_Addr_S addr = {0};
    FillAddr(fdp, addr);
    NLSTK_DisInfoType_E type = static_cast<NLSTK_DisInfoType_E>(fdp.ConsumeIntegral<uint8_t>() % 8);
    NLSTK_DisPropData_S outData;
    (void)memset_s(&outData, sizeof(outData), 0, sizeof(outData));
    (void)NLSTK_DisReadInfo(&addr, type, &outData);
}

void FuzzDisReadAllInfo(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    NLSTK_DisAllPropInfo_S propInfo;
    (void)memset_s(&propInfo, sizeof(propInfo), 0, sizeof(propInfo));
    FillAddr(fdp, propInfo.addr);
    (void)NLSTK_DisReadAllInfo(&propInfo);
}

void FuzzDisReadAppearanceInfo(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    SLE_Addr_S addr = {0};
    FillAddr(fdp, addr);
    uint32_t appearance = 0;
    (void)NLSTK_DisReadAppearanceInfo(&addr, &appearance);
}

void FuzzDisGetConnectedDeviceNum(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    (void)fdp;
    uint8_t num = 0;
    (void)NLSTK_GetConnectedDeviceNum(&num);
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
    NLSTK_DisClientCbk_S callback = {0};
    callback.stateChangeCbk = OHOS::DisConnectStateChangeCbkImpl;
    (void)NLSTK_DisRegisterCallbBack(&callback);
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
    OHOS::FuzzDisProfileConnect(fuzzData, size);
    OHOS::FuzzDisReadInfo(fuzzData, size);
    OHOS::FuzzDisReadAllInfo(fuzzData, size);
    OHOS::FuzzDisReadAppearanceInfo(fuzzData, size);
    OHOS::FuzzDisGetConnectedDeviceNum(fuzzData, size);
    SDF_MemFree(fuzzData);
    return 0;
}
