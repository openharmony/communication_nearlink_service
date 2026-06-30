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

#include "stackapi_fuzzer.h"
#include "securec.h"

#include "nai_log.h"
#include "nlstk_log.h"
#include "sdf_mem.h"
#include "nlstk_init_api.h"
#include "nlstk_devd_def.h"
#include "nlstk_devd_api.h"
#include "nlstk_cfgdb_api.h"
#include "nlstk_cfgdb.h"
#include "nbc_api.h"
#include "SleDliThreadUtil.h"

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
    SleDliThreadUtil &g_dliThreadUtil = SleDliThreadUtil::GetInstance();

    // mock cbk func, the following functions are used as simulation callbacks. 
    void fuzz_gle_dd_scan_event_cbk(uint8_t event, uint8_t result)
    {
        (void)event;
        (void)result;
    }

    void fuzz_gle_dd_adv_repot_cbk(NLSTK_DevdAdvReportInfo_S *report)
    {
        (void)report;
    }

    void fuzz_gle_dd_scan_filter_cbk(NLSTK_DevdScanFilterInfo_S *info)
    {
        (void)info;
    }

    void fuzz_gle_nbc_read_local_feat(void *param)
    {
        (void)param;
    }

    void fuzz_gle_nbc_chan_info(void *param)
    {
        (void)param;
    }

    void fuzz_gle_nbc_chip_reset_notify(void *param)
    {
        (void)param;
    }

    void fuzz_cfgdb_listern_cbk(void *param)
    {
        (void)param;
    }

    // fuzz func
    void FuzzUapiGleDevd(uint8_t* data, size_t size)
    {
        if (size < FUZZ_ONE) {
            return;
        }
        uint8_t index = 0;
        uint8_t len = data[index++];
        if (size - index < len) {
            return;
        }

        SLE_Addr_S addr = {0};
        (void)NLSTK_CfgdbGetPublicAddress(&addr);

        NLSTK_DevdSleScanExterCbk_S scanExtCbks;
        scanExtCbks.scanCbk = fuzz_gle_dd_scan_event_cbk;
        scanExtCbks.reportCbk = fuzz_gle_dd_adv_repot_cbk;
        scanExtCbks.scanFilterCbk = fuzz_gle_dd_scan_filter_cbk;
        (void)NLSTK_DevdRegScanEventCbk(&scanExtCbks);
    }

    void FuzzUapiGleManager(uint8_t* data, size_t size)
    {
        (void)data;
        (void)size;
        NLSTK_CfgdbCbk_S nbcExtCbks = {0};
        nbcExtCbks.rssiCbk = fuzz_gle_nbc_chan_info;
        nbcExtCbks.chipResetNotifyCbk = fuzz_gle_nbc_chip_reset_notify;
        nbcExtCbks.localFeatureCbk = fuzz_gle_nbc_read_local_feat;
        (void)NLSTK_CfgdbRegisterCbks(&nbcExtCbks);
    }

    void FuzzCfgdb(uint8_t* data, size_t size)
    {
        if (size < FUZZ_TWO) {
            return;
        }
        (void)NLSTK_CfgdbRegisterConfigListener(fuzz_cfgdb_listern_cbk, (NLSTK_CfgdbModule_E)(data[0] % (NLSTK_CFGDB_MODULE_MAX + 1)),
             (NLSTK_CfgdbConfig_E)(data[1] % (NLSTK_CFGDB_CONFIG_MAX + 1)));
        (void)CfgdbGetLocalVersion();
        CfgdbLocalCsCaps_S caps = {0};
        (void)CfgdbReadLocalCsCaps(&caps);
        (void)NBC_GetPublicAddress();
    }

    void FuzzStackApi(uint8_t *data, size_t size)
    {
        if (data == nullptr || size < FUZZ_ONE) {
            return;
        }
        FuzzUapiGleDevd(data, size);
        FuzzUapiGleManager(data, size);
        FuzzCfgdb(data, size);
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
    NLSTK_EnableStack();
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::g_dliThreadUtil.InitThread();
    uint8_t *fuzzData = (uint8_t *)malloc(size);
    if (fuzzData == nullptr) {
        OHOS::g_dliThreadUtil.DestroyQueue();
        return 0;
    }
    memcpy_s(fuzzData, size, data, size);
    OHOS::FuzzStackApi(fuzzData, size);
    free(fuzzData);
    OHOS::g_dliThreadUtil.DestroyQueue();
    return 0;
}