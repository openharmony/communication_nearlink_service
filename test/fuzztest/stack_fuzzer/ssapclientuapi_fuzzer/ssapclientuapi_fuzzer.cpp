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

#include "ssapclientuapi_fuzzer.h"
#include "securec.h"

#include "fuzzer/FuzzedDataProvider.h"
#include "ssapc_client.h"
#include "ssapc_client_api.h"
#include "ssap_manager.h"
#include "ssap_link.h"
#include "ssap_link_state.h"
#include "nai_log.h"
#include "sdf_mem.h"
#include "sdf_thread.h"
#include "sdf_evc.h"
#include "nlstk_init_api.h"
#include "nlstk_schedule.h"
#include "nlstk_log.h"
#include "sdf_mem.h"
#include "sm_stm.h"
#include "sm_slink.h"
#include "sdf_map.h"
#include "ssapc_cache.h"
#include "sle_logic_link_mgr.h"
#include "nlstk_schedule.h"
#include "nlstk_ssap_app_link.h"
#include "nlstk_ssap_app_client.h"

#include "ssapc_app.h"
#include "dli_callback.h"
#include "cp_worker.h"
#include "ssapc_app_link_sm.h"

extern SDF_Map *g_slinkMap;

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
    FUZZ_ONEHUNDREDTWENTYEIGHT = 128,
    FUZZ_TWOHUNDREDFIFTYSIX    = 256,
    FUZZ_NUM_END
} FUZZ_NUM_E;

static NLSTK_SsapUuid_S g_uuid1 = {.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02}};

void SSAP_DeleteSsapLinkByAddr_stub(void *param)
{
    SSAP_DeleteSsapLinkByAddr((SLE_Addr_S *)param);
}

void SleLogicLinkRemove_stub(void *param)
{
    SleLogicLinkRemove((SleLogicLink_S *)param);
}

namespace OHOS {

    void SsapClientFreeFuncCb (NLSTK_SsapServ_S *service, uint16_t serviceNum)
    {
        return;
    }

    void SsapClientCleanAppResultCb()
    {
        return;
    }

    void SendFunc(SSAP_Link_S *link, SDF_Buff_S *buff, uint8_t opcode)
    {
        (void)link;
        (void)buff;
        (void)opcode;
    }

    void FuzzSsapClientUapi(uint8_t* data, size_t size)
    {
        /* addrB 需要第二段地址字节：data[SLE_ADDR_LEN .. 2*SLE_ADDR_LEN] */
        if (size < 2 * SLE_ADDR_LEN) {
            return;
        }
        FuzzedDataProvider provider(data, size);
        int32_t appIdA1 = -1;
        int32_t appIdA2 = -1;
        int32_t appIdB = -1;
        NLSTK_SsapAppClientCb_S cb = {0};
        SLE_Addr_S addrA = {0};
        SLE_Addr_S addrB = {0};
        memcpy_s(addrA.addr, SLE_ADDR_LEN, data, SLE_ADDR_LEN);
        /* addrB（第二设备）地址取 data 偏移 [SLE_ADDR_LEN .. 2*SLE_ADDR_LEN] 的稳定
         * 字节（直接切片，非 FDP 消费，不影响尾部取数序列）；首字节强制与 addrA
         * 互补，保证双设备地址恒不同，隔离语义稳定成立 */
        memcpy_s(addrB.addr, SLE_ADDR_LEN, data + SLE_ADDR_LEN, SLE_ADDR_LEN);
        addrB.addr[0] = (uint8_t)(addrA.addr[0] ^ 0xFF);
        SleLogicLinkInit();
        SleLogicLink_S *nodeA = SleLogicLinkAdd(&addrA);
        if (nodeA == nullptr) {
            return;
        }
        SSAP_LinkInit();
        nodeA->lcid = provider.ConsumeIntegral<uint16_t>();
        uint16_t lcidA = provider.ConsumeIntegral<uint16_t>();
        SSAP_Link_S *linkA = SSAP_CreateSsapLink(&addrA, lcidA, SendFunc);
        if (linkA == nullptr) {
            SleLogicLinkRemove(nodeA);
            return;
        }
        /* addrB 独立逻辑链路 + SSAP 链路，lcid 由 addrA 侧确定性派生保证互异
         * （不新增 FDP 消费：新随机源只允许追加在既有消费序列尾部） */
        SleLogicLink_S *nodeB = SleLogicLinkAdd(&addrB);
        if (nodeB == nullptr) {
            return; /* ponytail: 仅 OOM 可能失败，addrA 侧残留不清理 */
        }
        nodeB->lcid = (uint16_t)(nodeA->lcid ^ 0x4000);
        SSAP_Link_S *linkB = SSAP_CreateSsapLink(&addrB, (uint16_t)(lcidA ^ 0x8000), SendFunc);
        if (linkB == nullptr) {
            SleLogicLinkRemove(nodeB); /* ponytail: 同上，仅 OOM 可能 */
            return;
        }

        /* addrA 同址双 app + addrB 单 app 共用 SSAP 链路环境：RegApp 深拷贝 cb 后
         * 经 SchedulePostTaskBlocked 注册（nlstk_ssap_app_client.c:56-84），cb 全零
         * 即回调全部为空，产品侧回调下发处已判空 */
        NLSTK_SsapClientRegApp(&appIdA1, &cb, &addrA);
        NLSTK_SsapClientRegApp(&appIdA2, &cb, &addrA);
        NLSTK_SsapClientRegApp(&appIdB, &cb, &addrB);

        /* 铺底 addrA 实际链路已连接（app 状态仍为 IDLE），USER_CONNECT 才能从
         * IDLE 走 UserConnectInIdleState 真实迁移；reason 取 fuzz 字节保留错误码
         * 路径（既有消费位次不变，下方 addrA/addrB 事件共用） */
        SsapLinkHandleRecordLinkStateFromCm(&addrA, SSAP_CONNECT_STATE_CONNECTED);
        uint8_t reason = provider.ConsumeIntegral<uint8_t>();
        /* addrA 双 app 事件交错驱动（保留原事件枚举遍历，同事件先 appA1 后 appA2）：
         * USER_CONNECT 使两 app 均迁入 CONNECTED（ssapc_app_link_sm.c:156-160）；
         * appA1 的 USER_DISCONNECT 先到，SsapcAppClientDisconnectCmLinkCheck
         * （ssapc_app_link_sm.c:95-126）发现同址 appA2 仍为 CONNECTED/CONNECTING
         * （:119-121）→ 拒绝 CM_DirectConnectRemove，仅改状态上报（:410-414）；
         * 轮到 appA2 同事件时 appA1 已 DISCONNECTED，检查放行 → 真实 CM 断链
         * （:397-408），随后 LOGIC_LINK_DISCONNECTED 走 Disconnecting→Disconnected
         * 迁移（:544-556） */
        {
            static const NLSTK_SsapClientLinkChangeEvent_E events[] = {
                SSAP_USER_CONNECT,
                SSAP_LOGIC_LINK_CONNECTED,
                SSAP_USER_DISCONNECT,
                SSAP_LOGIC_LINK_DISCONNECTED,
                SSAP_LINK_CHANGE_EVENT_BUTT,
            };
            for (size_t i = 0; i < sizeof(events) / sizeof(events[0]); i++) {
                SsapClientLinkStateMachineCall(appIdA1, events[i], reason);
                SsapClientLinkStateMachineCall(appIdA2, events[i], reason);
            }
        }

        uint16_t mtu = 128;
        NLSTK_SsapClientExchangeMtu(appIdA1, mtu);

        NLSTK_SsapClientDiscoverServices(appIdA1, 0, 0, provider.ConsumeIntegral<uint8_t>());

        NLSTK_SsapClientDiscoverServicesByUuid(appIdA1, &g_uuid1, provider.ConsumeIntegral<uint16_t>(),
            provider.ConsumeIntegral<uint16_t>(), provider.ConsumeIntegral<uint8_t>());

        NLSTK_SsapServ_S *service;
        uint16_t serviceNum = 0;
        NLSTK_SsapClientFreeFunc func = &SsapClientFreeFuncCb;
        NLSTK_SsapClientGetServices(appIdA1, &service, &serviceNum, &func);

        NLSTK_SsapClientGetServicesByUuid(appIdA1, &g_uuid1, &service, &serviceNum, &func);

        NLSTK_SsapClientReadProperty(appIdA1, provider.ConsumeIntegral<uint16_t>());

        NLSTK_SsapClientGetPropertyNtf(appIdA1, provider.ConsumeIntegral<uint16_t>());

        NLSTK_SsapClientSetPropertyNtf(appIdA1, provider.ConsumeIntegral<uint16_t>(), true);

        NLSTK_VariableData_S value = {0};
        value.len = size;
        value.data = data;
        NLSTK_SsapClientWriteProperty(appIdA1, provider.ConsumeIntegral<uint16_t>(), &value, true);
        NLSTK_SsapClientWriteProperty(appIdA1, provider.ConsumeIntegral<uint16_t>(), &value, false);

        NLSTK_SsapClientCallMethod(appIdA1, provider.ConsumeIntegral<uint16_t>(), &value, true);
        NLSTK_SsapClientCallMethod(appIdA1, provider.ConsumeIntegral<uint16_t>(), &value, false);

        /* —— 以下均为尾部新增消费（追加在既有消费序列末尾，旧 corpus 既有取值
         * 不变，数据耗尽时 FDP 返回 0）—— addrB 三变体驱动，variant 取 fuzz 字节 %3：
         * 变体0：铺底 addrB 已连接后完整走 USER_CONNECT→LOGIC_LINK_CONNECTED→
         *   USER_DISCONNECT→LOGIC_LINK_DISCONNECTED（单 app 放行断链，与旧单 app
         *   遍历语义对齐）；
         * 变体1（未连接）：不铺底直接 USER_CONNECT，IDLE 态经
         *   SsapLinkHandleUserConnect 真实发起 CM_DirectConnectAdd
         *   （ssap_link_state.c:88-95），fuzzer 进程内无对端，覆盖 CM 受理进
         *   CONNECTING（:94）与失败回 DISCONNECTED+CONN_FAIL 上报（:91）两路；
         * 变体2（连接中）：USER_CONNECT 进入 CONNECTING 后再投 USER_DISCONNECT
         *   （经 SsapcCacheLinkOper 缓存断链，ssapc_app_link_sm.c:293-307）与
         *   USER_CONNECT（缓存抵消清除，:263-280）；若 CM 未受理则落到
         *   Disconnected 态韧性分支 */
        uint8_t variant = provider.ConsumeIntegral<uint8_t>() % FUZZ_THREE;
        if (variant == 0) {
            SsapLinkHandleRecordLinkStateFromCm(&addrB, SSAP_CONNECT_STATE_CONNECTED);
            static const NLSTK_SsapClientLinkChangeEvent_E eventsB[] = {
                SSAP_USER_CONNECT,
                SSAP_LOGIC_LINK_CONNECTED,
                SSAP_USER_DISCONNECT,
                SSAP_LOGIC_LINK_DISCONNECTED,
                SSAP_LINK_CHANGE_EVENT_BUTT,
            };
            for (size_t i = 0; i < sizeof(eventsB) / sizeof(eventsB[0]); i++) {
                SsapClientLinkStateMachineCall(appIdB, eventsB[i], reason);
            }
        } else if (variant == 1) {
            SsapClientLinkStateMachineCall(appIdB, SSAP_USER_CONNECT, reason);
        } else {
            SsapClientLinkStateMachineCall(appIdB, SSAP_USER_CONNECT, reason);
            SsapClientLinkStateMachineCall(appIdB, SSAP_USER_DISCONNECT, reason);
            SsapClientLinkStateMachineCall(appIdB, SSAP_USER_CONNECT, reason);
        }

        /* 多 app 独立缓存覆盖：ReadProperty/SetPropertyNtf/WriteProperty 在 appA2/appB 上各打一次 */
        NLSTK_SsapClientReadProperty(appIdA2, provider.ConsumeIntegral<uint16_t>());
        NLSTK_SsapClientSetPropertyNtf(appIdA2, provider.ConsumeIntegral<uint16_t>(), true);
        NLSTK_SsapClientWriteProperty(appIdA2, provider.ConsumeIntegral<uint16_t>(), &value, true);
        NLSTK_SsapClientReadProperty(appIdB, provider.ConsumeIntegral<uint16_t>());
        NLSTK_SsapClientSetPropertyNtf(appIdB, provider.ConsumeIntegral<uint16_t>(), true);
        NLSTK_SsapClientWriteProperty(appIdB, provider.ConsumeIntegral<uint16_t>(), &value, true);

        NLSTK_SsapCleanClientApp(SsapClientCleanAppResultCb);

        /* 收尾顺序沿用既有"先调度删链后 Dereg"模式，每 addr 成对一致；DeregApp 经
         * SsapcAppDeregister 清缓存并按 appId 清残留任务（ssapc_app.c:197-210） */
        SchedulePostTask(SSAP_DeleteSsapLinkByAddr_stub, (void *)&addrA, NULL);
        SchedulePostTask(SleLogicLinkRemove_stub, (void *)nodeA, NULL);
        SchedulePostTask(SSAP_DeleteSsapLinkByAddr_stub, (void *)&addrB, NULL);
        SchedulePostTask(SleLogicLinkRemove_stub, (void *)nodeB, NULL);

        NLSTK_SsapClientDeregApp(appIdA1);
        NLSTK_SsapClientDeregApp(appIdA2);
        NLSTK_SsapClientDeregApp(appIdB);
    }

    void FuzzSsapClientUapi2(uint8_t* data, size_t size)
    {
        if (size < SLE_ADDR_LEN) {
            return;
        }
        FuzzedDataProvider provider(data, size);
        SLE_Addr_S addr = {0};
        memcpy_s(addr.addr, SLE_ADDR_LEN, data, SLE_ADDR_LEN);

        SleLogicLinkInit();
        SleLogicLink_S *node = SleLogicLinkAdd(&addr);
        if (node == nullptr) {
            return;
        }
        node->lcid = provider.ConsumeIntegral<uint16_t>();
        SSAP_Link_S *link = SSAP_CreateSsapLink(&addr, provider.ConsumeIntegral<uint16_t>(), SendFunc);
        if (link == nullptr) {
            SleLogicLinkRemove(node);
            return;
        }

        int32_t appId = -1;
        NLSTK_SsapAppClientCb_S cb = {0};
        
        SLE_Addr_S *key = (SLE_Addr_S *)SDF_MemAlloc(sizeof(SLE_Addr_S));
        memcpy_s(key, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));
        SmSLink_S *slink = SmSLinkCtor(&addr);
        g_slinkMap = SDF_MapCtor(SLE_ADDR_TRAITS(), MAKE_TRAITS(SmSLinkDtor, NULL));
        SDF_MapMoveInsert(g_slinkMap, key, slink);

        NLSTK_SsapClientRegApp(&appId, &cb, &addr);

        NLSTK_SsapClientReadPropertyByUuid(appId, &g_uuid1, 0, provider.ConsumeIntegral<uint16_t>());

        NLSTK_SsapClientReadDescriptor(appId, provider.ConsumeIntegral<uint16_t>(), provider.ConsumeIntegral<uint8_t>());

        NLSTK_SsapClientGetPropertyInd(appId, provider.ConsumeIntegral<uint16_t>());

        NLSTK_SsapClientSetPropertyInd(appId, provider.ConsumeIntegral<uint16_t>(), provider.ConsumeBool());

        NLSTK_VariableData_S value = {0};
        value.len = size;
        value.data = data;
        NLSTK_SsapClientWriteDescriptor(appId, provider.ConsumeIntegral<uint16_t>(), &value,
            provider.ConsumeIntegral<uint8_t>(), provider.ConsumeBool());

        SchedulePostTask(SSAP_DeleteSsapLinkByAddr_stub, (void *)&addr, NULL);
        SchedulePostTask(SleLogicLinkRemove_stub, (void *)node, NULL);

        NLSTK_SsapClientDeregApp(appId);
        if (g_slinkMap) {
            SDF_MapDtor(g_slinkMap);
            g_slinkMap = NULL;
        }
    }

    void FuzzSsapClientUapi3(void)
    {
        NLSTK_SsapServ_S *serv = (NLSTK_SsapServ_S *)SDF_MemZalloc(sizeof(NLSTK_SsapServ_S));
        NLSTK_SsapPrty_S *property = (NLSTK_SsapPrty_S *)SDF_MemZalloc(sizeof(NLSTK_SsapPrty_S));
        NLSTK_SsapDtor_S *descriptor = (NLSTK_SsapDtor_S *)SDF_MemZalloc(sizeof(NLSTK_SsapDtor_S));
        property->descriptorNum = 1;
        property->descriptors = descriptor;
        serv->properties = property;
        serv->propertyNum = 1;
        uint16_t num = 1;
        SsapcCacheFreeServices(serv, num);
    }

    void FuzzSsapClientAsyncApi(uint8_t *data, size_t size)
    {
        if (size < SLE_ADDR_LEN) {
            return;
        }
        FuzzedDataProvider provider(data, size);
        SLE_Addr_S addr = {0};
        memcpy_s(addr.addr, SLE_ADDR_LEN, data, SLE_ADDR_LEN);
        NLSTK_ConnParam_S connParam = {0};
        connParam.frameType = provider.ConsumeIntegral<uint8_t>();
        NLSTK_SsapAppClientCb_S cb = {0};
        (void)NLSTK_SsapClientRegAppAsyn(&addr, &connParam, &cb);

        int32_t appId = provider.ConsumeIntegral<int32_t>();
        NLSTK_SsapClientDeregAppAsync(appId);

        int64_t timeout = provider.ConsumeIntegral<int64_t>();
        (void)NLSTK_SsapClientSetInteractionTimeout(appId, timeout);

        (void)NLSTK_SsapClientGetServicesAsyn(appId);

        (void)NLSTK_SsapClientGetServicesByUuidAsyn(appId, &g_uuid1);

        uint8_t num = provider.ConsumeIntegral<uint8_t>() % FUZZ_EIGHT;
        uint16_t handles[FUZZ_EIGHT] = {0};
        for (uint8_t i = 0; i < num; i++) {
            handles[i] = provider.ConsumeIntegral<uint16_t>();
        }
        (void)NLSTK_SsapClientReadProperties(appId, handles, num);

        bool multiProcessing = false;
        (void)NLSTK_SsapGetMultiProcessing(appId, &multiProcessing);

        (void)NLSTK_SsapClientConnect(appId);
        (void)NLSTK_SsapClientDisconnect(appId);
    }

    void FuzzSsapClientApi(uint8_t* data, size_t size)
    {
        if (data == nullptr || size < 1) {
            return;
        }
        FuzzSsapClientUapi(data, size);
        FuzzSsapClientUapi2(data, size);
        FuzzSsapClientUapi3();
        FuzzSsapClientAsyncApi(data, size);
    }
}

static void UapiRecvAcbStub(uint16_t lcid, SDF_Buff_S *buf)
{
    (void)lcid;
    (void)buf;
}

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void) argc;
    (void) argv;
    ScheduleEnable();
    DLI_Callback dliCallback = {0};
    dliCallback.postOtherThread = CP_PostTask;
    dliCallback.postOtherBlockedThread = CP_PostTaskBlocked;
    dliCallback.recvAcbHandler = UapiRecvAcbStub; /* DLI_SetCallback 守卫要求非空 */
    (void)DLI_SetCallback(&dliCallback);
    (void)NLSTK_InitStack();
    (void)NLSTK_EnableStack();
    (void)SDF_ThreadInit(10);
    (void)SDF_EvcInit();
    /* 设备实证（hilog pid 过滤）：手机固件 /system/lib64 ext so 的
     * STK_ExtModuleInit 在 fuzzer 进程内 DLI 事件注册失败 → InitStack
     * 回滚销毁调度器 → 全部 UAPI post task fail。ScheduleEnable 幂等，
     * 末尾重建保证调度器无论 ext 成败都存活（test-only，不涉产品码）。
     * ThreadInit/EvcInit 曾被 babcbed8 误删，此处恢复（2f71d7d9 实证
     * 缺它们时 RegApp 必败）。 */
    ScheduleEnable();
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    uint8_t *fuzzData = (uint8_t*)malloc(size);
    if (fuzzData == nullptr) {
        return 0;
    }
    memcpy_s(fuzzData, size, data, size);
    OHOS::FuzzSsapClientApi(static_cast<uint8_t *>(fuzzData), size);
    SDF_MemFree(fuzzData);
    return 0;
}