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

/* ponytail: hid_stm.h / hid_ssap.h（HidStateMachineCall、HidStmEvent_E/HidStmParam_S、
 * HidGetSsapCbk）只在 hid/src，不在 shared_stack_include_dirs（stack_fuzz.gni 仅含
 * hid/include 与 hid/include/inner），14 个 stm 事件与真实 10 回调表当前不可 include、
 * 也不经任何可达头间接暴露（已 grep 核实）→ stm 注入与 HidGetSsapCbk 用例 BLOCKED。
 * 升级路径：产品侧把 hid_stm.h/hid_ssap.h 挪到 hid/include/inner 后，本文件补
 * FuzzHidStmInject（HidStmParam_S 逐事件）与 HidGetSsapCbk 表驱动用例即可。 */

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

    /* PR-5 B-HID: 真实记录桩替代全零回调，四回调各一计数器，到达各打一条日志限频 */
    static uint32_t g_fuzzHidNotifyCnt = 0;
    static uint32_t g_fuzzHidReadCnt = 0;
    static uint32_t g_fuzzHidWriteCnt = 0;
    static uint32_t g_fuzzHidConnCnt = 0;

    static void HidNotifyPropertyCbkStub(SLE_Addr_S *addr, HidPropertyType_E type, void *value)
    {
        (void)addr;
        (void)type;
        (void)value;
        g_fuzzHidNotifyCnt++;
        static bool logged = false;
        if (!logged) {
            logged = true;
            NLSTK_LOG_INFO("hidstack_fuzzer notifyPropertyCbk reached, cnt=%u", g_fuzzHidNotifyCnt);
        }
    }

    static void HidReadPropertyCbkStub(SLE_Addr_S *addr, HidPropertyType_E type, void *value, NLSTK_Errcode_E ret)
    {
        (void)addr;
        (void)type;
        (void)value;
        (void)ret;
        g_fuzzHidReadCnt++;
        static bool logged = false;
        if (!logged) {
            logged = true;
            NLSTK_LOG_INFO("hidstack_fuzzer readPropertyCbk reached, cnt=%u", g_fuzzHidReadCnt);
        }
    }

    static void HidWritePropertyCbkStub(SLE_Addr_S *addr, HidPropertyType_E type, NLSTK_Errcode_E ret)
    {
        (void)addr;
        (void)type;
        (void)ret;
        g_fuzzHidWriteCnt++;
        static bool logged = false;
        if (!logged) {
            logged = true;
            NLSTK_LOG_INFO("hidstack_fuzzer writePropertyCbk reached, cnt=%u", g_fuzzHidWriteCnt);
        }
    }

    static void HidConnectStateChangeCbkStub(SLE_Addr_S *addr, HidConnectState_E state, HidConnectState_E preState,
        NLSTK_Errcode_E ret)
    {
        (void)addr;
        g_fuzzHidConnCnt++;
        static bool logged = false;
        if (!logged) {
            logged = true;
            NLSTK_LOG_INFO("hidstack_fuzzer connectStateChangeCbk reached, state=%d preState=%d ret=%d cnt=%u",
                (int)state, (int)preState, (int)ret, g_fuzzHidConnCnt);
        }
    }

    void FuzzHidRegClientCbk(uint8_t* fuzzData, size_t size)
    {
        HidClientCallBack_S cbk = {};
        cbk.notifyPropertyCbk = HidNotifyPropertyCbkStub;
        cbk.readPropertyCbk = HidReadPropertyCbkStub;
        cbk.writePropertyCbk = HidWritePropertyCbkStub;
        cbk.connectStateChangeCbk = HidConnectStateChangeCbkStub;
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

    /* PR-5 B-HID: 固定地址（跨输入状态可复现）正序生命周期链
     * 注册回调→Connect→GetConnectState→Read→Write→GetInformation→Disconnect→GetConnectState，
     * 外加一条未注册地址直读/直断失败序列；观察点全走 Blocked UAPI（FIFO 于调度队列之后，
     * 无需 sleep），首次输入打印一条状态轨迹 */
    static SLE_Addr_S g_fuzzHidLifeAddr = {
        .type = PUBLIC_ADDRESS,
        .addr = {0x64, 0x65, 0x76, 0x64, 0x48, 0x49},
    };
    static SLE_Addr_S g_fuzzHidFreshAddr = {
        .type = PUBLIC_ADDRESS,
        .addr = {0x64, 0x65, 0x76, 0x64, 0x46, 0x52},
    };

    void FuzzHidLifecycleSequence(uint8_t* fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);
        FuzzHidRegClientCbk(fuzzData, size);
        (void)HidConnect(&g_fuzzHidLifeAddr);
        uint8_t stateAfterConnect = HID_CONNECTED;
        (void)HidGetConnectState(&g_fuzzHidLifeAddr, &stateAfterConnect);
        HidPropertyType_E type = static_cast<HidPropertyType_E>(
            fdp.ConsumeIntegral<uint8_t>() % HID_PROPERTY_TYPE_BUFF);
        HidReportIdAndType_S reportIdAndType = {};
        reportIdAndType.reportId = fdp.ConsumeIntegral<uint8_t>() % FUZZ_EIGHT;
        reportIdAndType.reportType = fdp.ConsumeIntegral<uint8_t>() % FUZZ_EIGHT;
        (void)HidReadProperty(&g_fuzzHidLifeAddr, type, &reportIdAndType);
        HidPropertyType_E workType = HID_WORK_STATUS_INDICATION;
        uint8_t workState = fdp.ConsumeIntegral<uint8_t>();
        (void)HidWriteProperty(&g_fuzzHidLifeAddr, workType, &workState);
        HidInformation_S *info = NULL;
        HidFreeFunc freeFunc = NULL;
        (void)HidGetInformation(&g_fuzzHidLifeAddr, &info, &freeFunc);
        if (freeFunc != NULL) {
            freeFunc(info);
        }
        (void)HidDisconnect(&g_fuzzHidLifeAddr);
        uint8_t stateAfterDisconnect = HID_CONNECTED;
        (void)HidGetConnectState(&g_fuzzHidLifeAddr, &stateAfterDisconnect);
        /* 未注册地址失败序列：无设备 → GetConnectState 走 dev==NULL 分支、读写/断连任务空转 */
        uint8_t freshState = HID_CONNECTED;
        (void)HidGetConnectState(&g_fuzzHidFreshAddr, &freshState);
        (void)HidReadProperty(&g_fuzzHidFreshAddr, type, &reportIdAndType);
        (void)HidDisconnect(&g_fuzzHidFreshAddr);
        static bool fuzzHidLifeLogged = false;
        if (!fuzzHidLifeLogged) {
            fuzzHidLifeLogged = true;
            NLSTK_LOG_INFO("hidstack_fuzzer lifecycle state: afterConnect=%u afterDisconnect=%u fresh=%u, "
                "cbkCnt conn=%u notify=%u read=%u write=%u",
                (unsigned)stateAfterConnect, (unsigned)stateAfterDisconnect, (unsigned)freshState,
                g_fuzzHidConnCnt, g_fuzzHidNotifyCnt, g_fuzzHidReadCnt, g_fuzzHidWriteCnt);
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
        FuzzHidLifecycleSequence(data, size);
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