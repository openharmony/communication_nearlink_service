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

#include "port_fuzzer.h"
#include "securec.h"

#include "nai_log.h"
#include "sdf_mem.h"
#include "nlstk_init_api.h"
#include "cpfwk_log.h"

#include "nlstk_port_client.h"
#include "nlstk_port_server.h"
#include "port_server_init.h"
#include "port_client_init.h"
#include "port_client.h"
#include "port_stm.h"
#include "port_type.h"

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
    void FuzzPortClientApi(uint8_t* data, size_t size)
    {
        if (size > sizeof(NLSTK_PortClientCallBack_S)) {
            NLSTK_PortClientCallBack_S clientCallback = {0};
            (void)memcpy_s(&clientCallback, sizeof(NLSTK_PortClientCallBack_S), data, sizeof(NLSTK_PortClientCallBack_S));
            NLSTK_PortClientRegCbk(&clientCallback);
            NLSTK_PortClientDeregCbk();
        }
        if (size > sizeof(NLSTK_SsapUuid_S)) {
            SLE_Addr_S addr = {0};
            (void)memcpy_s(&addr, sizeof(SLE_Addr_S), data, sizeof(SLE_Addr_S));
            NLSTK_ConnParam_S connParam = { 0 };
            NLSTK_PortConnect(&addr, &connParam);
            sleep(1);
            PortInfoCache_S *portInfoCache = PortFindInfoCacheByAddr(&addr);
            if (portInfoCache != NULL && portInfoCache->appId >= 0) {
                PortConnStateMsg_S connStateMsg = { .state = SSAP_CONNECT_STATE_CONNECTED, .ret = NLSTK_ERRCODE_SUCCESS,
                    .reason = 0 };
                PortStmParam_S msg = { .what = PORT_ON_STATE_CHANGED, .extData = (void *)&connStateMsg };
                PortStateMachineCall(portInfoCache, msg);
            }
            int state = 0;
            NLSTK_PortGetConnectState(&addr, &state);
            NLSTK_SsapUuid_S uuid = {0};
            (void)memcpy_s(&uuid, sizeof(NLSTK_SsapUuid_S), data, sizeof(NLSTK_SsapUuid_S));
            uint16_t portId = 0;
            NLSTK_PortGetDevicePortIdByUuid(&addr, &uuid, &portId);
            int portConnectNum = 0;
            NLSTK_PortGetConnectDeviceNum(&addr, &portConnectNum);
            NLSTK_PortDisconnect(&addr);
        }
    }
    void FuzzPortServerApi(uint8_t* data, size_t size)
    {
        if (size > sizeof(NLSTK_SsapUuid_S)) {
            NLSTK_SsapUuid_S uuid = {0};
            (void)memcpy_s(&uuid, sizeof(NLSTK_SsapUuid_S), data, sizeof(NLSTK_SsapUuid_S));
            uint16_t manufactureId = 0;
            uint16_t portId = 0;
            (void)memcpy_s(&manufactureId, sizeof(uint16_t), data, sizeof(uint16_t));
            (void)memcpy_s(&manufactureId, sizeof(uint16_t), data + sizeof(uint16_t), sizeof(uint16_t));
            NLSTK_PortAddByUuid(&uuid, manufactureId, portId);
            NLSTK_PortDeleteByUuid(&uuid);
        }
    }

    void FuzzPortApi(uint8_t *data, size_t size)
    {
        if (data == nullptr || size < 1) {
            return;
        }
        FuzzPortClientApi(data, size);
        FuzzPortServerApi(data, size);
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
    (void)PortClientEnable();
    (void)PortServerEnable();
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
    OHOS::FuzzPortApi(fuzzData, size);
    free(fuzzData);
    return 0;
}