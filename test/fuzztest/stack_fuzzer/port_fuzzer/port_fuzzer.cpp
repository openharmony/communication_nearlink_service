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
#include "fuzzer/FuzzedDataProvider.h"
#include "securec.h"

#include "nai_log.h"
#include "sdf_mem.h"
#include "sdf_thread.h"
#include "sdf_evc.h"
#include "nlstk_schedule.h"
#include "nlstk_init_api.h"
#include "cpfwk_log.h"

#include "nlstk_port_client.h"
#include "nlstk_port_server.h"
#include "port_server_init.h"
#include "port_client_init.h"
#include "port_client.h"
#include "port_stm.h"
#include "port_type.h"
#include "port_server.h"
#include "sdf_vector.h"

#include "nlstk_ssap_app_link.h"

/* port_server.c:33 非静态全局（port_server.c 在 shared_stack_source），直读 size 作 Add/Delete 观测量 */
extern "C" SDF_Vector_S *g_portPrivateList;

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
    static void PortStateChangeCbkStub(SLE_Addr_S *addr, NLSTK_PortConnectState_E curState,
        NLSTK_PortConnectState_E preState, NLSTK_Errcode_E ret)
    {
        (void)addr;
        (void)curState;
        (void)preState;
        (void)ret;
    }

    void FuzzPortClientApi(uint8_t *data, size_t size)
    {
        FuzzedDataProvider fdp(data, size);
        NLSTK_PortClientCallBack_S clientCallback = {0};
        clientCallback.connectStateChangeCbk = PortStateChangeCbkStub;
        NLSTK_PortClientRegCbk(&clientCallback);
        NLSTK_PortClientDeregCbk();

        SLE_Addr_S addr = {0};
        addr.type = fdp.ConsumeIntegral<uint8_t>();
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        NLSTK_ConnParam_S connParam = {0};
        connParam.frameType = fdp.ConsumeIntegral<uint8_t>();
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
        for (uint8_t i = 0; i < SSAP_UUID128_LEN; i++) {
            uuid.uuid[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        uint16_t portId = 0;
        NLSTK_PortGetDevicePortIdByUuid(&addr, &uuid, &portId);
        int portConnectNum = 0;
        NLSTK_PortGetConnectDeviceNum(&addr, &portConnectNum);
        NLSTK_PortDisconnect(&addr);
    }
    void FuzzPortServerApi(uint8_t *data, size_t size)
    {
        FuzzedDataProvider fdp(data, size);
        NLSTK_SsapUuid_S uuid = {0};
        for (uint8_t i = 0; i < SSAP_UUID128_LEN; i++) {
            uuid.uuid[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        uint16_t manufactureId = fdp.ConsumeIntegral<uint16_t>();
        uint16_t portId = fdp.ConsumeIntegral<uint16_t>();
        NLSTK_PortAddByUuid(&uuid, manufactureId, portId);
        NLSTK_PortDeleteByUuid(&uuid);
    }

    /* PR-5 B-PORT: 直调 Inner 覆盖五档 新增→重复Add(已存在swap)→Delete不存在→Delete存在→再Add，
     * uuid 由 fdp 派生，PortPrivateInfo_S 栈上构造，g_portPrivateList->size 逐步观测；
     * 本用例排在 UAPI 用例之前执行，避免与 evc 线程上的 UAPI 投递任务并发操作同一向量 */
    void FuzzPortUuidLifecycle(uint8_t *data, size_t size)
    {
        if (g_portPrivateList == NULL) {
            return;
        }
        FuzzedDataProvider fdp(data, size);
        PortPrivateInfo_S info = {};
        for (uint8_t i = 0; i < SSAP_UUID128_LEN; i++) {
            info.uuid.uuid[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        uint16_t manufactureId = fdp.ConsumeIntegral<uint16_t>();
        uint16_t portId = fdp.ConsumeIntegral<uint16_t>();
        NLSTK_SsapUuid_S absentUuid = {};
        for (uint8_t i = 0; i < SSAP_UUID128_LEN; i++) {
            absentUuid.uuid[i] = (uint8_t)(info.uuid.uuid[i] ^ (i + 1));
        }
        size_t base = g_portPrivateList->size;
        info.manufactureId = manufactureId;
        info.portId = portId;
        PortAddByUuidInner(&info);                    /* 1. 不存在 → 新增档 */
        size_t afterAdd = g_portPrivateList->size;
        uint16_t portBeforeSwap = info.portId;
        info.manufactureId = (uint16_t)(manufactureId + 1);
        info.portId = (uint16_t)(portId + 1);
        PortAddByUuidInner(&info);                    /* 2. 已存在 → swap 档（参数换回旧值） */
        size_t afterSwap = g_portPrivateList->size;
        uint16_t portAfterSwap = info.portId;
        PortDeleteByUuidInner(&absentUuid);           /* 3. 不存在 → 报错返回档 */
        size_t afterAbsentDel = g_portPrivateList->size;
        PortDeleteByUuidInner(&info.uuid);            /* 4. Delete 存在 */
        size_t afterDel = g_portPrivateList->size;
        info.manufactureId = manufactureId;
        info.portId = portId;
        PortAddByUuidInner(&info);                    /* 5. 再 Add 回来 */
        size_t afterReAdd = g_portPrivateList->size;
        PortDeleteByUuidInner(&info.uuid);            /* 清理，不污染后续输入 */
        static bool fuzzPortLifeLogged = false;
        if (!fuzzPortLifeLogged) {
            fuzzPortLifeLogged = true;
            NAI_LOG_INFO("[FUZZ]port uuid lifecycle size: base=%u add=%u swap=%u absentDel=%u del=%u reAdd=%u, "
                "swapBack=%d", (unsigned)base, (unsigned)afterAdd, (unsigned)afterSwap,
                (unsigned)afterAbsentDel, (unsigned)afterDel, (unsigned)afterReAdd,
                portAfterSwap == portBeforeSwap ? 1 : 0);
        }
    }

    /* PR-5 B-PORT: 一次性（仅首输入）直调 Inner 灌不同 uuid 验证容量满守卫（port_server.c
     * NLSTK_PORT_MAX_NUM=0xFF，宏未出 .c 文件故此处用字面量）。Add 成功路径调
     * NLSTK_SsapServerUpdatePropertyValue：其仅校验投递，但 PortGetPropertyValue 在
     * 2+20*size > SSAP_MAX_VALUE_LENTH(1024)（即 size>=52）时被参数校验拒绝并整档回滚，
     * 守卫档(0xFF)大概率灌不满 → 检测到 size 停滞且未满即防御性退出（PARTIAL）；
     * 结束逐个 Delete 清干净。结果依赖 SSAP 本地注册就绪 */
    void FuzzPortCapacityFull(uint8_t *data, size_t size)
    {
        (void)data;
        (void)size;
        static bool fuzzPortCapDone = false;
        if (fuzzPortCapDone) {
            return;
        }
        fuzzPortCapDone = true;
        if (g_portPrivateList == NULL) {
            NAI_LOG_INFO("[FUZZ]port capacity skip: g_portPrivateList null (PortServerEnable failed)");
            return;
        }
        size_t base = g_portPrivateList->size;
        uint32_t added = 0;
        bool guardHit = false;
        bool rollbackStall = false;
        for (uint32_t idx = 0; idx <= FUZZ_TWOHUNDREDFIFTYSIX; idx++) {
            PortPrivateInfo_S info = {};
            info.uuid.uuid[0] = (uint8_t)(idx & 0xFF);
            info.uuid.uuid[1] = (uint8_t)((idx >> 8) & 0xFF);
            info.manufactureId = (uint16_t)idx;
            info.portId = (uint16_t)(idx + 1);
            size_t before = g_portPrivateList->size;
            PortAddByUuidInner(&info);
            size_t after = g_portPrivateList->size;
            if (after == before) {
                if (before >= 0xFF) {
                    guardHit = true;
                } else {
                    rollbackStall = true;
                }
                break;
            }
            added++;
            if (after >= 0xFF) {
                /* 已到 0xFF：再补一发不同 uuid 验证守卫拒绝（size 不得增长） */
                PortPrivateInfo_S probe = {};
                probe.uuid.uuid[0] = 0xEE;
                probe.uuid.uuid[1] = 0xEE;
                PortAddByUuidInner(&probe);
                guardHit = (g_portPrivateList->size == after);
                break;
            }
        }
        for (uint32_t idx = 0; idx < added; idx++) {
            NLSTK_SsapUuid_S uuid = {};
            uuid.uuid[0] = (uint8_t)(idx & 0xFF);
            uuid.uuid[1] = (uint8_t)((idx >> 8) & 0xFF);
            PortDeleteByUuidInner(&uuid);
        }
        NLSTK_SsapUuid_S probeUuid = {};
        probeUuid.uuid[0] = 0xEE;
        probeUuid.uuid[1] = 0xEE;
        PortDeleteByUuidInner(&probeUuid);
        NAI_LOG_INFO("[FUZZ]port capacity base=%u added=%u final=%u guardHit=%d rollbackStall=%d "
            "(full 0xFF guard depends on SSAP update; value len<=1024 rolls back at size>=52)",
            (unsigned)base, added, (unsigned)g_portPrivateList->size, guardHit ? 1 : 0,
            rollbackStall ? 1 : 0);
    }

    void FuzzPortApi(uint8_t *data, size_t size)
    {
        if (data == nullptr || size < 1) {
            return;
        }
        FuzzPortUuidLifecycle(data, size);
        FuzzPortCapacityFull(data, size);
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
    /* ext 插件依赖真实芯片：InitStack 中途回滚会连带 ScheduleDisable+SdfDeinit，
     * 不重建则 PortServerEnable 的 SsapServerRegApp(Blocked 投递)与全部 UAPI 静默失败
     * （hidstack_fuzzer 同款回退，红蓝对抗实证） */
    if (NLSTK_InitStack() != NLSTK_ERRCODE_SUCCESS) {
        (void)SDF_ThreadInit(10);
        (void)SDF_EvcInit();
        (void)ScheduleEnable();
    }
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