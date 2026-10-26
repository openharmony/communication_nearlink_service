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
#include "fuzzer/FuzzedDataProvider.h"
#include "securec.h"

#include "nai_log.h"
#include "sdf_mem.h"
#include "sdf_thread.h"
#include "sdf_evc.h"
#include "nlstk_schedule.h"
#include "nlstk_init_api.h"
#include "cpfwk_log.h"

#include "nlstk_icce_client.h"
#include "nlstk_icce_server.h"
#include "icce_init.h"
#include "icce_stm.h"
#include "icce_common.h"

#include "nlstk_ssap_app_link.h"
#include "nlstk_ssap_app_client.h"

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
    /* PR-5 B-ICCE: 用户回调记录桩（计数 + 首次一条日志限频） */
    static uint32_t g_fuzzIcceConnCbkCnt = 0;

    static void ConnectionStateChange(SLE_Addr_S *sleAddr, NLSTK_IcceConnectState_E curState,
        NLSTK_IcceConnectState_E prevState, NLSTK_Errcode_E errNumb)
    {
        (void)sleAddr;
        (void)prevState;
        (void)errNumb;
        g_fuzzIcceConnCbkCnt++;
        static bool fuzzIcceCbkLogged = false;
        if (!fuzzIcceCbkLogged) {
            fuzzIcceCbkLogged = true;
            NAI_LOG_INFO("[FUZZ]icce user connectStateChangeCbk reached, curState=%d cnt=%u",
                (int)curState, g_fuzzIcceConnCbkCnt);
        }
    }

    /* 同步 RegApp 拿真实 appId 用（进程级一次，回调仅为占位防 NULL） */
    static void FuzzSsapConnStateStub(int32_t appId, uint8_t state, NLSTK_Errcode_E ret, int32_t reason)
    {
        (void)appId;
        (void)state;
        (void)ret;
        (void)reason;
    }

    static void FuzzSsapRegAppStub(int32_t appId, SLE_Addr_S *addr, NLSTK_Errcode_E ret)
    {
        (void)appId;
        (void)addr;
        (void)ret;
    }

    /* PR-5 B-ICCE: 固定地址七步链 USER_CONNECT(UAPI)→REGISTERED_CBK→LINK_STATE_CHANGE(CONNECTED)
     * →GET_SERVICE_CBK→READ_PROPERTY_CBK(→DEV_CONNECTED+用户回调)→USER_DISCONNECT，
     * extData 用栈上变量地址，逐步读 IcceFindDeviceByAddr 的 state 记录轨迹（首输入一条） */
    static SLE_Addr_S g_fuzzIcceChainAddr = {
        .type = PUBLIC_ADDRESS,
        .addr = {0x64, 0x65, 0x76, 0x64, 0x43, 0x31},
    };

    void FuzzIcceStateChain(uint8_t *data, size_t size)
    {
        FuzzedDataProvider fdp(data, size);
        NLSTK_IcceClientCallBack_S cbs = {};
        cbs.connectStateChangeCbk = &ConnectionStateChange;
        (void)NLSTK_IcceRegisterReadInfoCallBack(&cbs);
        bool firstConnect = (IcceFindDeviceByAddr(&g_fuzzIcceChainAddr) == NULL);
        (void)NLSTK_IcceConnect(&g_fuzzIcceChainAddr);
        if (firstConnect) {
            sleep(1); /* 仅首连接等设备向量创建，后续输入直入注入 */
        }
        IcceDevice_S *dev = IcceFindDeviceByAddr(&g_fuzzIcceChainAddr);
        if (dev == NULL) {
            static bool fuzzIcceChainBlockedLogged = false;
            if (!fuzzIcceChainBlockedLogged) {
                fuzzIcceChainBlockedLogged = true;
                NAI_LOG_INFO("[FUZZ]icce chain blocked: device not created (scheduler/ssap down)");
            }
            return;
        }
        /* 真实 appId 优先级：设备自身异步注册回填 > 进程级同步 RegApp 缓存 > fdp 兜底 */
        static int32_t fuzzIcceRealAppId = -1;
        static bool fuzzIcceAppIdTried = false;
        if (!fuzzIcceAppIdTried) {
            fuzzIcceAppIdTried = true;
            NLSTK_SsapAppClientCb_S ssapCb = {};
            ssapCb.onConnectionStateChanged = FuzzSsapConnStateStub;
            ssapCb.onRegisterApp = FuzzSsapRegAppStub;
            if (NLSTK_SsapClientRegApp(&fuzzIcceRealAppId, &ssapCb, &g_fuzzIcceChainAddr)
                != NLSTK_ERRCODE_SUCCESS) {
                fuzzIcceRealAppId = -1;
            }
        }
        const char *appIdSrc = "fdp";
        int32_t appId = (int32_t)fdp.ConsumeIntegral<int32_t>();
        if (dev->appId >= 0) {
            appId = dev->appId;
            appIdSrc = "dev";
        } else if (fuzzIcceRealAppId >= 0) {
            appId = fuzzIcceRealAppId;
            appIdSrc = "syncReg";
        }
        uint8_t trace[6] = {0};
        trace[0] = dev->state;
        IcceStmParam param = {.what = ICCE_ON_REGISTERED_CBK, .extData = (void *)&appId, .reason = 0};
        IcceClientStmCall(dev, param);
        dev = IcceFindDeviceByAddr(&g_fuzzIcceChainAddr);
        trace[1] = (dev != NULL) ? dev->state : (uint8_t)ICCE_APPID_UNREGISTERED;
        uint8_t linkState = SSAP_CONNECT_STATE_CONNECTED;
        param.what = ICCE_ON_LINK_STATE_CHANGE;
        param.extData = (void *)&linkState;
        IcceClientStmCall(dev, param);
        dev = IcceFindDeviceByAddr(&g_fuzzIcceChainAddr);
        trace[2] = (dev != NULL) ? dev->state : (uint8_t)ICCE_APPID_UNREGISTERED;
        param.what = ICCE_ON_GET_SERVICE_CBK;
        param.extData = NULL;
        IcceClientStmCall(dev, param);
        dev = IcceFindDeviceByAddr(&g_fuzzIcceChainAddr);
        trace[3] = (dev != NULL) ? dev->state : (uint8_t)ICCE_APPID_UNREGISTERED;
        NLSTK_SsapClientReadPropertyInfo_S prop = {};
        param.what = ICCE_ON_READ_PROPERTY_CBK;
        param.extData = (void *)&prop;
        IcceClientStmCall(dev, param);
        dev = IcceFindDeviceByAddr(&g_fuzzIcceChainAddr);
        trace[4] = (dev != NULL) ? dev->state : (uint8_t)ICCE_APPID_UNREGISTERED;
        param.what = ICCE_ON_USER_DISCONNECT;
        param.extData = NULL;
        IcceClientStmCall(dev, param);
        dev = IcceFindDeviceByAddr(&g_fuzzIcceChainAddr);
        trace[5] = (dev != NULL) ? dev->state : (uint8_t)ICCE_APPID_UNREGISTERED;
        static bool fuzzIcceChainLogged = false;
        if (!fuzzIcceChainLogged) {
            fuzzIcceChainLogged = true;
            NAI_LOG_INFO("[FUZZ]icce chain appId(%s=%d) trace: %u->%u->%u->%u->%u->%u, userCbkCnt=%u",
                appIdSrc, (int)appId, (unsigned)trace[0], (unsigned)trace[1], (unsigned)trace[2],
                (unsigned)trace[3], (unsigned)trace[4], (unsigned)trace[5], g_fuzzIcceConnCbkCnt);
        }
    }

    /* PR-5 B-ICCE: A/B 固定地址交错，向 A 注入断开事件后对比 B 的 state 是否被波及
     * （仅记录不 abort），反向验证状态机按设备隔离 */
    static SLE_Addr_S g_fuzzIcceAddrA = {
        .type = PUBLIC_ADDRESS,
        .addr = {0x64, 0x65, 0x76, 0x64, 0x41, 0x41},
    };
    static SLE_Addr_S g_fuzzIcceAddrB = {
        .type = PUBLIC_ADDRESS,
        .addr = {0x64, 0x65, 0x76, 0x64, 0x42, 0x42},
    };

    void FuzzIcceInterleave(uint8_t *data, size_t size)
    {
        (void)data;
        (void)size;
        bool firstConnect = (IcceFindDeviceByAddr(&g_fuzzIcceAddrA) == NULL ||
            IcceFindDeviceByAddr(&g_fuzzIcceAddrB) == NULL);
        (void)NLSTK_IcceConnect(&g_fuzzIcceAddrA);
        (void)NLSTK_IcceConnect(&g_fuzzIcceAddrB);
        if (firstConnect) {
            sleep(1);
        }
        IcceDevice_S *devA = IcceFindDeviceByAddr(&g_fuzzIcceAddrA);
        IcceDevice_S *devB = IcceFindDeviceByAddr(&g_fuzzIcceAddrB);
        if (devA == NULL || devB == NULL) {
            static bool fuzzIcceIlvBlockedLogged = false;
            if (!fuzzIcceIlvBlockedLogged) {
                fuzzIcceIlvBlockedLogged = true;
                NAI_LOG_INFO("[FUZZ]icce interleave blocked: device not created (scheduler/ssap down)");
            }
            return;
        }
        uint8_t stateA0 = devA->state;
        uint8_t stateB0 = devB->state;
        uint8_t discState = SSAP_CONNECT_STATE_DISCONNECTED;
        IcceStmParam param = {.what = ICCE_ON_LINK_STATE_CHANGE, .extData = (void *)&discState, .reason = 0};
        IcceClientStmCall(devA, param);
        param.what = ICCE_ON_USER_DISCONNECT;
        param.extData = NULL;
        IcceClientStmCall(devA, param);
        devA = IcceFindDeviceByAddr(&g_fuzzIcceAddrA);
        devB = IcceFindDeviceByAddr(&g_fuzzIcceAddrB);
        uint8_t stateA1 = (devA != NULL) ? devA->state : (uint8_t)ICCE_APPID_UNREGISTERED;
        uint8_t stateB1 = (devB != NULL) ? devB->state : (uint8_t)ICCE_APPID_UNREGISTERED;
        static bool fuzzIcceIlvLogged = false;
        if (!fuzzIcceIlvLogged) {
            fuzzIcceIlvLogged = true;
            NAI_LOG_INFO("[FUZZ]icce interleave: A %u->%u, B %u->%u (B unchanged=%d)",
                (unsigned)stateA0, (unsigned)stateA1, (unsigned)stateB0, (unsigned)stateB1,
                stateB0 == stateB1 ? 1 : 0);
        }
    }

    void FuzzIcceClientApi(uint8_t *data, size_t size)
    {
        FuzzedDataProvider fdp(data, size);
        NLSTK_IcceClientCallBack_S cbs = {0};
        cbs.connectStateChangeCbk = &ConnectionStateChange;
        NLSTK_IcceRegisterReadInfoCallBack(&cbs);
        SLE_Addr_S addr = {0};
        addr.type = fdp.ConsumeIntegral<uint8_t>();
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
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
    void FuzzIcceServerApi(uint8_t *data, size_t size)
    {
        FuzzedDataProvider fdp(data, size);
        NLSTK_IcceServiceInfo_S icceServiceInfo = {0};
        icceServiceInfo.iccePort = fdp.ConsumeIntegral<uint16_t>();
        icceServiceInfo.iccePortRight = fdp.ConsumeIntegral<uint8_t>();
        NLSTK_IcceCreateIcceInstance(&icceServiceInfo);
        NLSTK_IcceDestroyIcceInstance();
    }

    void FuzzIcceApi(uint8_t *data, size_t size)
    {
        if (data == nullptr || size < 1) {
            return;
        }
        /* 固定地址用例先行：既有的随机地址 Connect 会每输入向设备向量追加节点
         * （evc 线程写入与主线程读并发），放在后面并靠其自带 sleep 排空 */
        FuzzIcceStateChain(data, size);
        FuzzIcceInterleave(data, size);
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
    /* ext 插件依赖真实芯片：InitStack 中途回滚会连带 ScheduleDisable+SdfDeinit，
     * 不重建则 NLSTK_IcceConnect 等全部 UAPI 投递静默失败、设备向量永远为空
     * （hidstack_fuzzer 同款回退，红蓝对抗实证） */
    if (NLSTK_InitStack() != NLSTK_ERRCODE_SUCCESS) {
        (void)SDF_ThreadInit(10);
        (void)SDF_EvcInit();
        (void)ScheduleEnable();
    }
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