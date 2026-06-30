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

#include "icce_fuzzer.h"
#include "securec.h"

#include "nai_log.h"
#include "sdf_mem.h"
#include "nlstk_init_api.h"
#include "cpfwk_log.h"

#include "nlstk_icce_client.h"
#include "nlstk_icce_server.h"
#include "icce_init.h"
#include "icce_stm.h"
#include "icce_common.h"

#include "nlstk_ssap_app_link.h"

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
    static void ConnectionStateChange(SLE_Addr_S *sleAddr, NLSTK_IcceConnectState_E curState, 
        NLSTK_IcceConnectState_E prevState, NLSTK_Errcode_E errNumb)
    {
        return;
    }
    void FuzzIcceClientApi(uint8_t* data, size_t size)
    {
        NLSTK_IcceClientCallBack_S cbs = {0};
        cbs.connectStateChangeCbk = &ConnectionStateChange;
        NLSTK_IcceRegisterReadInfoCallBack(&cbs);
        if (size > sizeof(SLE_Addr_S)) {
            SLE_Addr_S addr = {0};
            (void)memcpy_s(&addr, sizeof(SLE_Addr_S), data, sizeof(SLE_Addr_S));
            NLSTK_IcceConnect(&addr);
            sleep(1);
            IcceDevice_S *info = IcceFindDeviceByAddr(&addr);
            if (info != NULL && info->appId >= 0) {
                uint8_t state = SSAP_CONNECT_STATE_CONNECTED;
                IcceStmParam param = {.what = ICCE_ON_LINK_STATE_CHANGE, .extData = (void *)&state};
                IcceClientStmCall(info, param);
            }
            int32_t port = 0;
            NLSTK_IcceGetPort(&addr, &port);
            NLSTK_GetConnectionsDeviceNum();
            NLSTK_IcceDisconnect(&addr);
        }
    }
    void FuzzIcceServerApi(uint8_t* data, size_t size)
    {
        if (size > sizeof(NLSTK_IcceServiceInfo_S)) {
            NLSTK_IcceServiceInfo_S icceServiceInfo = {0};
            (void)memcpy_s(&icceServiceInfo, sizeof(NLSTK_IcceServiceInfo_S), data, sizeof(NLSTK_IcceServiceInfo_S));
            NLSTK_IcceCreateIcceInstance(&icceServiceInfo);
        }
        NLSTK_IcceDestroyIcceInstance();
    }

    void FuzzIcceApi(uint8_t *data, size_t size)
    {
        if (data == nullptr || size < 1) {
            return;
        }
        FuzzIcceClientApi(data, size);
        FuzzIcceServerApi(data, size);
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
    IcceClientEnable();
    IcceServerEnable();
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    uint8_t *fuzzData = (uint8_t *)malloc(size);
    if (fuzzData == nullptr) {
        return 0;
    }
    memcpy_s(fuzzData, size, data, size);
    OHOS::FuzzIcceApi(fuzzData, size);
    free(fuzzData);
    return 0;
}