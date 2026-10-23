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
#include "cm_fuzzer.h"
#include "cm_link_fuzzer.h"
#include "cm_common_fuzzer.h"
#include "cm_concurrent_conn_fuzzer.h"
#include <cstdint>
#include "fuzzer/FuzzedDataProvider.h"
#include "securec.h"
#include "cm_signaling_internal.h"

// cm_link_fuzzer.cpp 内的 C++ 导出（OHOS 命名空间，声明不放入头文件以符合规范）
namespace OHOS {
void CM_ConnectActiveOpsFuzz(uint8_t *data, size_t size);
void CM_DynTransLifecycleFuzz(uint8_t *data, size_t size);
uint32_t CmFuzzerSignalingCaptureCb(uint8_t pi, uint8_t tcid, uint16_t lcid, SDF_Buff_S *buff);
}

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc;
    (void)argv;
    // 信令发送回调进程内只注册一次：cm_fuzzer 未编 dtap，捕获 CM 发出的信令并释放 buff（dtap 所有权语义）
    CM_SetSendSignalingDataCbk(OHOS::CmFuzzerSignalingCaptureCb);
    return FuzzCmLinkInit();
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzCmLinkInit();
    FuzzedDataProvider fdp(data, size);
    std::vector<uint8_t> fuzzVec = fdp.ConsumeBytes<uint8_t>(size);
    uint8_t *fuzzData = fuzzVec.data();
    size_t fuzzSize = fuzzVec.size();
    FuzzCmLinkApi(fuzzData, fuzzSize);

    // dyntrans 闭环窗口：仍在 CM_Init 之后、CM_DeInit 之前，每 input 与 Init/DeInit 严格配对
    FuzzCmDynTransInit();
    OHOS::CM_ConnectActiveOpsFuzz(fuzzData, fuzzSize);
    OHOS::CM_DynTransLifecycleFuzz(fuzzData, fuzzSize);
    FuzzCmDynTransDeInit();

    FuzzCmLinkDeInit();

    FuzzCmConcurrentConnInit();
    FuzzCmConcurrentConnApi(fuzzData, fuzzSize);
    FuzzCmConcurrentConnDeInit();

    FuzzCmDliAdaptor(fuzzData, fuzzSize);
    return 0;
}