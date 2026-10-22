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

#include "securec.h"
#include "sdf_mem.h"
#include "sdf_addr.h"
#include "sdf_thread.h"
#include "sdf_evc.h"
#include "nlstk_log.h"
#include "nlstk_init_api.h"
#include "nlstk_schedule.h"
#include "hid_client_api.h"
#include "hid_client_init.h"
#include "hidstack_fuzzer.h"
#include "SleDliThreadUtil.h"
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
    FUZZ_TWENTY  = 20,
    FUZZ_ONEHUNDREDTWENTYEIGHT = 128,
    FUZZ_TWOHUNDREDFIFTYSIX    = 256,
} FUZZ_NUM_E;

namespace OHOS {
    SleDliThreadUtil &g_dliThreadUtil = SleDliThreadUtil::GetInstance();

    void FuzzHidRegClientCbk(uint8_t* fuzzData, size_t size)
    {
        HidClientCallBack_S cbk = {0};
        HidRegClientCbk(&cbk);
    }

    void FuzzHidConnect(uint8_t* fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);
        SLE_Addr_S addr = {0};
        addr.type = fdp.ConsumeIntegral<uint8_t>();
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        HidConnect(&addr);
    }

    void FuzzHidDisconnect(uint8_t* fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);
        SLE_Addr_S addr = {0};
        addr.type = fdp.ConsumeIntegral<uint8_t>();
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        HidDisconnect(&addr);
    }

    void FuzzHidGetInformation(uint8_t* fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);
        SLE_Addr_S addr = {0};
        addr.type = fdp.ConsumeIntegral<uint8_t>();
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        HidInformation_S *info = NULL;
        HidFreeFunc freeFunc = NULL;
        HidGetInformation(&addr, &info, &freeFunc);
        if (freeFunc != NULL) {
            freeFunc(info);
        }
    }

    void FuzzHidReadProperty(uint8_t* fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);
        SLE_Addr_S addr = {0};
        addr.type = fdp.ConsumeIntegral<uint8_t>();
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        HidPropertyType_E type = static_cast<HidPropertyType_E>(
            fdp.ConsumeIntegral<uint8_t>() % HID_PROPERTY_TYPE_BUFF);
        HidReportIdAndType_S reportIdAndType = {0};
        reportIdAndType.reportId = fdp.ConsumeIntegral<uint8_t>() % FUZZ_EIGHT;
        reportIdAndType.reportType = fdp.ConsumeIntegral<uint8_t>() % FUZZ_EIGHT;
        HidReadProperty(&addr, type, &reportIdAndType);
    }

    void FuzzHidWriteProperty(uint8_t* fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);
        SLE_Addr_S addr = {0};
        addr.type = fdp.ConsumeIntegral<uint8_t>();
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        HidPropertyType_E type = HID_TYPE_AND_FORMAT_DESC;
        HidTypeAndFormatDesc_S value1 = {0};
        std::vector<uint8_t> descBuf = fdp.ConsumeBytes<uint8_t>(FUZZ_TWOHUNDREDFIFTYSIX);
        value1.descLen = static_cast<uint16_t>(descBuf.size());
        value1.desc = descBuf.data();
        value1.type = fdp.ConsumeIntegral<uint8_t>();
        HidWriteProperty(&addr, type, &value1);

        type = HID_WORK_STATUS_INDICATION;
        uint8_t value2 = fdp.ConsumeIntegral<uint8_t>();
        HidWriteProperty(&addr, type, &value2);

        type = static_cast<HidPropertyType_E>(fdp.ConsumeIntegral<uint8_t>() % FUZZ_THREE + FUZZ_THREE);
        HidReportInfo_S value3 = {};
        value3.reportIdAndType.reportId = fdp.ConsumeIntegral<uint8_t>() % FUZZ_EIGHT;
        value3.reportIdAndType.reportType = fdp.ConsumeIntegral<uint8_t>() % FUZZ_EIGHT;
        std::vector<uint8_t> reportBuf = fdp.ConsumeBytes<uint8_t>(FUZZ_TWOHUNDREDFIFTYSIX);
        value3.reportInfoValue.len = static_cast<uint16_t>(reportBuf.size());
        value3.reportInfoValue.data = reportBuf.data();
        HidWriteProperty(&addr, type, &value3);
    }

    void FuzzHidGetConnectState(uint8_t* fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);
        SLE_Addr_S addr = {0};
        addr.type = fdp.ConsumeIntegral<uint8_t>();
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
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

    /* ext 插件依赖真实芯片，fuzzer 进程内 InitStack 中途回滚（ScheduleDisable→
     * g_schedule=NULL）。HidEnable→HidDeviceInit 只建设备向量（不经调度器），
     * 但 HID UAPI 全部经 SchedulePostTask 投递：必须先重建调度器，否则设备向量
     * 无消费者（红蓝对抗实证）。 */
    bool stackInited = NLSTK_InitStack() == NLSTK_ERRCODE_SUCCESS;
    (void)SDF_ThreadInit(10);
    (void)SDF_EvcInit();
    if (stackInited) {
        (void)NLSTK_EnableStack();
    } else {
        (void)ScheduleEnable();
        if (HidEnable() != NLSTK_ERRCODE_SUCCESS) {
            NLSTK_LOG_INFO("hidstack_fuzzer HidEnable fallback failed");
        }
    }
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