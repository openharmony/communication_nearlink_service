/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include "sdf_addr.h"
#include "nlstk_log.h"
#include "nlstk_init_api.h"
#include "hid_client_api.h"
#include "hidstack_fuzzer.h"
#include "SleDliThreadUtil.h"

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
    FUZZ_TWENTY  = 20,
    FUZZ_ONEHUNDREDTWENTYEIGHT = 128,
    FUZZ_TWOHUNDREDFIFTYSIX    = 256,
} FUZZ_NUM_E;

namespace OHOS {
    SleDliThreadUtil &g_dliThreadUtil = SleDliThreadUtil::GetInstance();

    char random_hex_byte()
    {
        const char* hex_digits = "0123456789abcdef";
        int random_byte = rand() % FUZZ_SIXTEEN;
        return hex_digits[random_byte];
    }

    void FuzzHidRegClientCbk(uint8_t* fuzzData, size_t size)
    {
        HidClientCallBack_S cbk = {0};
        HidRegClientCbk(&cbk);
    }

    void FuzzHidConnect(uint8_t* fuzzData, size_t size)
    {
        SLE_Addr_S addr = {
            .type = PUBLIC_ADDRESS,
            .addr = {
                random_hex_byte(), random_hex_byte(), random_hex_byte(),
                random_hex_byte(), random_hex_byte(), random_hex_byte(),
            },
        };
        HidConnect(&addr);
    }

    void FuzzHidDisconnect(uint8_t* fuzzData, size_t size)
    {
        SLE_Addr_S addr = {
            .type = PUBLIC_ADDRESS,
            .addr = {
                random_hex_byte(), random_hex_byte(), random_hex_byte(),
                random_hex_byte(), random_hex_byte(), random_hex_byte(),
            },
        };
        HidDisconnect(&addr);
    }

    void FuzzHidGetInformation(uint8_t* fuzzData, size_t size)
    {
        SLE_Addr_S addr = {
            .type = PUBLIC_ADDRESS,
            .addr = {
                random_hex_byte(), random_hex_byte(), random_hex_byte(),
                random_hex_byte(), random_hex_byte(), random_hex_byte(),
            },
        };
        HidInformation_S *info = NULL;
        HidFreeFunc freeFunc = NULL;
        HidGetInformation(&addr, &info, &freeFunc);
        if (freeFunc != NULL) {
            freeFunc(info);
        }
    }

    void FuzzHidReadProperty(uint8_t* fuzzData, size_t size)
    {
        SLE_Addr_S addr = {
            .type = PUBLIC_ADDRESS,
            .addr = {
                random_hex_byte(), random_hex_byte(), random_hex_byte(),
                random_hex_byte(), random_hex_byte(), random_hex_byte(),
            },
        };
        HidPropertyType_E type = (HidPropertyType_E)(rand() % HID_PROPERTY_TYPE_BUFF);
        HidReportIdAndType_S reportIdAndType = {
            .reportId = rand() % FUZZ_EIGHT,
            .reportType = rand() % FUZZ_EIGHT,
        };
        HidReadProperty(&addr, type, &reportIdAndType);
    }

    void FuzzHidWriteProperty(uint8_t* fuzzData, size_t size)
    {
        SLE_Addr_S addr = {
            .type = PUBLIC_ADDRESS,
            .addr = {
                random_hex_byte(), random_hex_byte(), random_hex_byte(),
                random_hex_byte(), random_hex_byte(), random_hex_byte(),
            },
        };
        HidPropertyType_E type = HID_TYPE_AND_FORMAT_DESC;
        HidTypeAndFormatDesc_S value1 = {0};
        value1.descLen = size;
        value1.desc = fuzzData;
        value1.type = rand() % FUZZ_EIGHT;
        HidWriteProperty(&addr, type, &value1);

        type = HID_WORK_STATUS_INDICATION;
        uint8_t value2 = rand() % FUZZ_TWOHUNDREDFIFTYSIX;
        HidWriteProperty(&addr, type, &value2);

        type = (HidPropertyType_E)(rand() % FUZZ_THREE + FUZZ_THREE);
        HidReportInfo_S value3 = {
            .reportIdAndType = { .reportId = rand() % FUZZ_EIGHT, .reportType = rand() % FUZZ_EIGHT, },
            .reportInfoValue = { .len = size, .data = fuzzData, }
        };
        HidWriteProperty(&addr, type, &value3);
    }

    void FuzzHidGetConnectState(uint8_t* fuzzData, size_t size)
    {
        SLE_Addr_S addr = {
            .type = PUBLIC_ADDRESS,
            .addr = {
                random_hex_byte(), random_hex_byte(), random_hex_byte(),
                random_hex_byte(), random_hex_byte(), random_hex_byte(),
            },
        };
        uint8_t state = 0;
        HidGetConnectState(&addr, &state);
    }

    void FuzzHidGetConnectedDevice(uint8_t* fuzzData, size_t size)
    {
        SLE_Addr_S *addrs = NULL;
        size_t num = 0;
        HidFreeFunc freeFunc = NULL;
        HidGetConnectedDevice(&addrs, &num, &freeFunc);
        if (freeFunc != NULL) {
            freeFunc(addrs);
        }
    }

    void FuzzHidApi(uint8_t* data, size_t size)
    {
        if (data == nullptr || size < 1) {
            return;
        }
        FuzzHidRegClientCbk(data, size);
        FuzzHidConnect(data, size);
        FuzzHidDisconnect(data, size);
        FuzzHidGetInformation(data, size);
        FuzzHidReadProperty(data, size);
        FuzzHidWriteProperty(data, size);
        FuzzHidGetConnectState(data, size);
        FuzzHidGetConnectedDevice(data, size);
    }
}

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void) argc;
    (void) argv;

    NLSTK_InitStack();
    NLSTK_EnableStack();
    NLSTK_LOG_INFO("hidstack_fuzzer init success");
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    OHOS::g_dliThreadUtil.InitThread();
    uint8_t *fuzzData = (uint8_t *)SDF_MemZalloc(size);
    if (fuzzData == nullptr) {
        OHOS::g_dliThreadUtil.DestroyQueue();
        return 0;
    }
    (void)memcpy_s(fuzzData, size, data, size);
    OHOS::FuzzHidApi(static_cast<uint8_t *>(fuzzData), size);
    SDF_MemFree(fuzzData);
    OHOS::g_dliThreadUtil.DestroyQueue();
    return 0;
}