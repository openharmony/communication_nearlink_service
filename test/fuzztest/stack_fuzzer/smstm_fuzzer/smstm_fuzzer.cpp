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
#include "sle_crypto.h"
#include "sdf_mem.h"
#include "sdf_map.h"
#include "sle_logic_link_mgr.h"
#include "nlstk_init_api.h"
#include "nlstk_sm_api.h"
#include "sm_stm.h"
#include "sm_slink.h"
#include "sm_errcode.h"
#include "smstm_fuzzer.h"
#include "fuzzer/FuzzedDataProvider.h"

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
    FUZZ_TWENTY  = 20,
    FUZZ_ONEHUNDREDTWENTYEIGHT = 128,
    FUZZ_TWOHUNDREDFIFTYSIX    = 256,
    FUZZ_NUM_END
} FUZZ_NUM_E;

namespace OHOS {

    /* PR-5 SM-A2: 外部结果回调记录桩(认证/加密/移除计数), 状态轨迹与回调到达证据 */
    static uint32_t g_fuzzSmAuthCbkCnt = 0;
    static uint32_t g_fuzzSmEncpCbkCnt = 0;
    static uint32_t g_fuzzSmRemoveCbkCnt = 0;
    static void FuzzSmStartCbk(NLSTK_SmPairingStart_S *params) { (void)params; }
    static void FuzzSmRemoveCbk(NLSTK_SmPairingRemove_S *params) { (void)params; g_fuzzSmRemoveCbkCnt++; }
    static void FuzzSmRequestCbk(NLSTK_SmPairingRequest_S *params) { (void)params; }
    static void FuzzSmAuthCbk(NLSTK_SmAuthComplete_S *params) { (void)params; g_fuzzSmAuthCbkCnt++; }
    static void FuzzSmEncCbk(NLSTK_SmEncComplete_S *params) { (void)params; g_fuzzSmEncpCbkCnt++; }
    static void FuzzSmImgMsgCbk(NLSTK_SmSendImgMsgCmpl_S *params) { (void)params; }

    /* PR-5 SM-A2: 固定地址铺垫一条真实逻辑链(SleLogicLinkAdd), Bind 成功后
     * PASSIVE/ACTIVE_START 才能迁移 NEGO; 全局链表按地址去重保持有界 */
    static SLE_Addr_S g_stmLinkAddr = {
        .type = PUBLIC_ADDRESS,
        .addr = {0x64, 0x65, 0x76, 0x64, 0x53, 0x4d},
    };

    static void FuzzStmEnsureLogicLink(void)
    {
        if (SleLogicLinkGetByAddr(&g_stmLinkAddr) != nullptr) {
            return;
        }
        (void)SleLogicLinkAdd(&g_stmLinkAddr);
    }

    /* SDF_MapMoveInsert 重复 key 时返回 false 且不接管 key/slink: 回收并复用已登记条目,
     * 避免每次输入恒定泄漏(设备实证: 修复前 RSS 线性增长至 OOM)。产品侧同款用法见 sm.c:689。 */
    static SmSLink_S *FuzzStmGetSLink(const SLE_Addr_S *addr)
    {
        SLE_Addr_S *key = (SLE_Addr_S *)SDF_MemAlloc(sizeof(SLE_Addr_S));
        if (key == nullptr) {
            return nullptr;
        }
        (void)memcpy_s(key, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
        SmSLink_S *slink = SmSLinkCtor(addr);
        if (slink == nullptr) {
            SDF_MemFree(key);
            return nullptr;
        }
        if (!SDF_MapMoveInsert(g_slinkMap, key, slink)) {
            SDF_MemFree(key);
            SmSLinkDtor(slink);
            SDF_MapIter *iter = SDF_MapFind(g_slinkMap, addr);
            if (iter == nullptr) {
                return nullptr;
            }
            slink = (SmSLink_S *)iter->val;
        }
        return slink;
    }

    void FuzzStmInitEncpParamReqReply(const uint8_t* fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);

        SLE_Addr_S addr;
        (void)memset_s(&addr, sizeof(SLE_Addr_S), 0, sizeof(SLE_Addr_S));
        addr.type = fdp.ConsumeIntegral<uint8_t>();
        for (int i = 0; i < SLE_ADDR_LEN; i++) {
            addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        SmSLink_S *slink = FuzzStmGetSLink(&addr);
        if (slink == nullptr) {
            return;
        }
        STM_MFUNC(slink->stm, Transition, g_smStateName[SM_STATE_INIT]);
        STM_MFUNC(slink->stm, ProcessMessage, (Message) { .what = SM_ENCP_PARAM_REQ_REPLY });
        SDF_MapErase(g_slinkMap, &addr);
    }

    void FuzzStmInitRemovePair(const uint8_t* fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);

        SLE_Addr_S addr;
        (void)memset_s(&addr, sizeof(SLE_Addr_S), 0, sizeof(SLE_Addr_S));
        addr.type = fdp.ConsumeIntegral<uint8_t>();
        for (int i = 0; i < SLE_ADDR_LEN; i++) {
            addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        SmSLink_S *slink = FuzzStmGetSLink(&addr);
        if (slink == nullptr) {
            return;
        }
        STM_MFUNC(slink->stm, Transition, g_smStateName[SM_STATE_INIT]);
        STM_MFUNC(slink->stm, ProcessMessage, (Message) { .what = SM_REMOVE_PAIR });
        SDF_MapErase(g_slinkMap, &addr);
    }

    void FuzzStmNegoSuccess(const uint8_t* fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);

        SLE_Addr_S addr;
        (void)memset_s(&addr, sizeof(SLE_Addr_S), 0, sizeof(SLE_Addr_S));
        addr.type = fdp.ConsumeIntegral<uint8_t>();
        for (int i = 0; i < SLE_ADDR_LEN; i++) {
            addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        SmSLink_S *slink = FuzzStmGetSLink(&addr);
        if (slink == nullptr) {
            return;
        }
        STM_MFUNC(slink->stm, Transition, g_smStateName[SM_STATE_NEGO]);
        STM_MFUNC(slink->stm, ProcessMessage, (Message) { .what = SM_NEGO_SUCCESS });
        SDF_MapErase(g_slinkMap, &addr);
    }

    void FuzzStmNegoTimeout(const uint8_t* fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);

        SLE_Addr_S addr;
        (void)memset_s(&addr, sizeof(SLE_Addr_S), 0, sizeof(SLE_Addr_S));
        addr.type = fdp.ConsumeIntegral<uint8_t>();
        for (int i = 0; i < SLE_ADDR_LEN; i++) {
            addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        SmSLink_S *slink = FuzzStmGetSLink(&addr);
        if (slink == nullptr) {
            return;
        }
        STM_MFUNC(slink->stm, Transition, g_smStateName[SM_STATE_NEGO]);
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_TIMEOUT, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
        SDF_MapErase(g_slinkMap, &addr);
    }

    void FuzzStmNegoRemovePair(const uint8_t* fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);

        SLE_Addr_S addr;
        (void)memset_s(&addr, sizeof(SLE_Addr_S), 0, sizeof(SLE_Addr_S));
        addr.type = fdp.ConsumeIntegral<uint8_t>();
        for (int i = 0; i < SLE_ADDR_LEN; i++) {
            addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        SmSLink_S *slink = FuzzStmGetSLink(&addr);
        if (slink == nullptr) {
            return;
        }
        STM_MFUNC(slink->stm, Transition, g_smStateName[SM_STATE_NEGO]);
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_REMOVE_PAIR, .extData = (void *)(uintptr_t)SM_ERR_ACTIVE_CANCEL });
        SDF_MapErase(g_slinkMap, &addr);
    }

    void FuzzStmNegoEncpParamReqReply(const uint8_t* fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);

        SLE_Addr_S addr;
        (void)memset_s(&addr, sizeof(SLE_Addr_S), 0, sizeof(SLE_Addr_S));
        addr.type = fdp.ConsumeIntegral<uint8_t>();
        for (int i = 0; i < SLE_ADDR_LEN; i++) {
            addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        SmSLink_S *slink = FuzzStmGetSLink(&addr);
        if (slink == nullptr) {
            return;
        }
        STM_MFUNC(slink->stm, Transition, g_smStateName[SM_STATE_NEGO]);
        STM_MFUNC(slink->stm, ProcessMessage, (Message) { .what = SM_ENCP_PARAM_REQ_REPLY });
        SDF_MapErase(g_slinkMap, &addr);
    }

    void FuzzStmAuthSuccess(const uint8_t* fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);

        SLE_Addr_S addr;
        (void)memset_s(&addr, sizeof(SLE_Addr_S), 0, sizeof(SLE_Addr_S));
        addr.type = fdp.ConsumeIntegral<uint8_t>();
        for (int i = 0; i < SLE_ADDR_LEN; i++) {
            addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        SmSLink_S *slink = FuzzStmGetSLink(&addr);
        if (slink == nullptr) {
            return;
        }
        STM_MFUNC(slink->stm, Transition, g_smStateName[SM_STATE_AUTH]);
        STM_MFUNC(slink->stm, ProcessMessage, (Message) { .what = SM_AUTH_SUCCESS });
        SDF_MapErase(g_slinkMap, &addr);
    }

    void FuzzStmAuthTimeout(const uint8_t* fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);

        SLE_Addr_S addr;
        (void)memset_s(&addr, sizeof(SLE_Addr_S), 0, sizeof(SLE_Addr_S));
        addr.type = fdp.ConsumeIntegral<uint8_t>();
        for (int i = 0; i < SLE_ADDR_LEN; i++) {
            addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        SmSLink_S *slink = FuzzStmGetSLink(&addr);
        if (slink == nullptr) {
            return;
        }
        STM_MFUNC(slink->stm, Transition, g_smStateName[SM_STATE_AUTH]);
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_TIMEOUT, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
        SDF_MapErase(g_slinkMap, &addr);
    }

    void FuzzStmAuthRemovePair(const uint8_t* fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);

        SLE_Addr_S addr;
        (void)memset_s(&addr, sizeof(SLE_Addr_S), 0, sizeof(SLE_Addr_S));
        addr.type = fdp.ConsumeIntegral<uint8_t>();
        for (int i = 0; i < SLE_ADDR_LEN; i++) {
            addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        SmSLink_S *slink = FuzzStmGetSLink(&addr);
        if (slink == nullptr) {
            return;
        }
        STM_MFUNC(slink->stm, Transition, g_smStateName[SM_STATE_AUTH]);
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_REMOVE_PAIR, .extData = (void *)(uintptr_t)SM_ERR_ACTIVE_CANCEL });
        SDF_MapErase(g_slinkMap, &addr);
    }

    void FuzzStmEncpActiveStart(const uint8_t* fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);

        SLE_Addr_S addr;
        (void)memset_s(&addr, sizeof(SLE_Addr_S), 0, sizeof(SLE_Addr_S));
        addr.type = fdp.ConsumeIntegral<uint8_t>();
        for (int i = 0; i < SLE_ADDR_LEN; i++) {
            addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        SmSLink_S *slink = FuzzStmGetSLink(&addr);
        if (slink == nullptr) {
            return;
        }
        STM_MFUNC(slink->stm, Transition, g_smStateName[SM_STATE_ENCP]);
        STM_MFUNC(slink->stm, ProcessMessage, (Message) { .what = SM_ACTIVE_START });
        SDF_MapErase(g_slinkMap, &addr);
    }

    void FuzzStmEncpSuccess(const uint8_t* fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);

        SLE_Addr_S addr;
        (void)memset_s(&addr, sizeof(SLE_Addr_S), 0, sizeof(SLE_Addr_S));
        addr.type = fdp.ConsumeIntegral<uint8_t>();
        for (int i = 0; i < SLE_ADDR_LEN; i++) {
            addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        SmSLink_S *slink = FuzzStmGetSLink(&addr);
        if (slink == nullptr) {
            return;
        }
        STM_MFUNC(slink->stm, Transition, g_smStateName[SM_STATE_ENCP]);
        STM_MFUNC(slink->stm, ProcessMessage, (Message) { .what = SM_ENCP_SUCCESS });
        SDF_MapErase(g_slinkMap, &addr);
    }

    void FuzzStmEncpParamReqReply(const uint8_t* fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);

        SLE_Addr_S addr;
        (void)memset_s(&addr, sizeof(SLE_Addr_S), 0, sizeof(SLE_Addr_S));
        addr.type = fdp.ConsumeIntegral<uint8_t>();
        for (int i = 0; i < SLE_ADDR_LEN; i++) {
            addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        SmSLink_S *slink = FuzzStmGetSLink(&addr);
        if (slink == nullptr) {
            return;
        }
        STM_MFUNC(slink->stm, Transition, g_smStateName[SM_STATE_ENCP]);
        STM_MFUNC(slink->stm, ProcessMessage, (Message) { .what = SM_ENCP_PARAM_REQ_REPLY });
        SDF_MapErase(g_slinkMap, &addr);
    }

    void FuzzStmEncpInternalError(const uint8_t* fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);

        SLE_Addr_S addr;
        (void)memset_s(&addr, sizeof(SLE_Addr_S), 0, sizeof(SLE_Addr_S));
        addr.type = fdp.ConsumeIntegral<uint8_t>();
        for (int i = 0; i < SLE_ADDR_LEN; i++) {
            addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        SmSLink_S *slink = FuzzStmGetSLink(&addr);
        if (slink == nullptr) {
            return;
        }
        STM_MFUNC(slink->stm, Transition, g_smStateName[SM_STATE_ENCP]);
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_INTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_UNSPECIFIED_REASON });
        SDF_MapErase(g_slinkMap, &addr);
    }

    void FuzzStmEncpExternalError(const uint8_t* fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);

        SLE_Addr_S addr;
        (void)memset_s(&addr, sizeof(SLE_Addr_S), 0, sizeof(SLE_Addr_S));
        addr.type = fdp.ConsumeIntegral<uint8_t>();
        for (int i = 0; i < SLE_ADDR_LEN; i++) {
            addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        SmSLink_S *slink = FuzzStmGetSLink(&addr);
        if (slink == nullptr) {
            return;
        }
        STM_MFUNC(slink->stm, Transition, g_smStateName[SM_STATE_ENCP]);
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_EXTERNAL_ERROR, .extData = (void *)(uintptr_t)SM_ERR_STATUS_KEY_MISSING });
        SDF_MapErase(g_slinkMap, &addr);
    }

    void FuzzStmEncpRemovePair(const uint8_t* fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);

        SLE_Addr_S addr;
        (void)memset_s(&addr, sizeof(SLE_Addr_S), 0, sizeof(SLE_Addr_S));
        addr.type = fdp.ConsumeIntegral<uint8_t>();
        for (int i = 0; i < SLE_ADDR_LEN; i++) {
            addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        SmSLink_S *slink = FuzzStmGetSLink(&addr);
        if (slink == nullptr) {
            return;
        }
        STM_MFUNC(slink->stm, Transition, g_smStateName[SM_STATE_ENCP]);
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_REMOVE_PAIR, .extData = (void *)(uintptr_t)SM_ERR_ACTIVE_CANCEL });
        SDF_MapErase(g_slinkMap, &addr);
    }

    void FuzzStmFullDisconnect(const uint8_t* fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);

        SLE_Addr_S addr;
        (void)memset_s(&addr, sizeof(SLE_Addr_S), 0, sizeof(SLE_Addr_S));
        addr.type = fdp.ConsumeIntegral<uint8_t>();
        for (int i = 0; i < SLE_ADDR_LEN; i++) {
            addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        SmSLink_S *slink = FuzzStmGetSLink(&addr);
        if (slink == nullptr) {
            return;
        }
        STM_MFUNC(slink->stm, Transition, g_smStateName[SM_STATE_FULL]);
        STM_MFUNC(slink->stm, ProcessMessage, (Message) { .what = SM_LCHANNEL_DISCONN });
        SDF_MapErase(g_slinkMap, &addr);
    }

    void FuzzStmFullRemovePair(const uint8_t* fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);

        SLE_Addr_S addr;
        (void)memset_s(&addr, sizeof(SLE_Addr_S), 0, sizeof(SLE_Addr_S));
        addr.type = fdp.ConsumeIntegral<uint8_t>();
        for (int i = 0; i < SLE_ADDR_LEN; i++) {
            addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        SmSLink_S *slink = FuzzStmGetSLink(&addr);
        if (slink == nullptr) {
            return;
        }
        STM_MFUNC(slink->stm, Transition, g_smStateName[SM_STATE_FULL]);
        STM_MFUNC(slink->stm, ProcessMessage, (Message) { .what = SM_REMOVE_PAIR });
        SDF_MapErase(g_slinkMap, &addr);
    }

    /* PR-5 SM-A2 状态缺口补充(强制迁移的内部健壮性组, 计划 A2 记账):
     * PASSIVE/ACTIVE/RECOVER_START、MISS 族、FULL+ACTIVE、鉴权方法与角色分支 */

    void FuzzStmInitPassiveStartLinked(const uint8_t* fuzzData, size_t size)
    {
        (void)fuzzData;
        (void)size;
        FuzzStmEnsureLogicLink();
        SmSLink_S *slink = FuzzStmGetSLink(&g_stmLinkAddr);
        if (slink == nullptr) {
            return;
        }
        STM_MFUNC(slink->stm, Transition, g_smStateName[SM_STATE_INIT]);
        STM_MFUNC(slink->stm, ProcessMessage, (Message) { .what = SM_PASSIVE_START });
        SDF_MapErase(g_slinkMap, &g_stmLinkAddr);
    }

    void FuzzStmInitActiveStartLinked(const uint8_t* fuzzData, size_t size)
    {
        (void)fuzzData;
        (void)size;
        FuzzStmEnsureLogicLink();
        SmSLink_S *slink = FuzzStmGetSLink(&g_stmLinkAddr);
        if (slink == nullptr) {
            return;
        }
        STM_MFUNC(slink->stm, Transition, g_smStateName[SM_STATE_INIT]);
        STM_MFUNC(slink->stm, ProcessMessage, (Message) { .what = SM_ACTIVE_START });
        SDF_MapErase(g_slinkMap, &g_stmLinkAddr);
    }

    void FuzzStmInitRecoverStart(const uint8_t* fuzzData, size_t size)
    {
        SLE_Addr_S addr;
        (void)memset_s(&addr, sizeof(SLE_Addr_S), 0, sizeof(SLE_Addr_S));
        addr.type = fdp.ConsumeIntegral<uint8_t>();
        for (int i = 0; i < SLE_ADDR_LEN; i++) {
            addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        SmSLink_S *slink = FuzzStmGetSLink(&addr);
        if (slink == nullptr) {
            return;
        }
        STM_MFUNC(slink->stm, Transition, g_smStateName[SM_STATE_INIT]);
        STM_MFUNC(slink->stm, ProcessMessage, (Message) { .what = SM_RECOVER_START });
        SDF_MapErase(g_slinkMap, &addr);
    }

    void FuzzStmEncpFailKeyMissingLinked(const uint8_t* fuzzData, size_t size)
    {
        (void)fuzzData;
        (void)size;
        FuzzStmEnsureLogicLink();
        SmSLink_S *slink = FuzzStmGetSLink(&g_stmLinkAddr);
        if (slink == nullptr) {
            return;
        }
        STM_MFUNC(slink->stm, Transition, g_smStateName[SM_STATE_ENCP]);
        /* KEY_MISSING 的 ENCP_FAIL: 迁移 MISS 并递归 SM_MISS_HANDLE, 重绑成功再入 NEGO */
        STM_MFUNC(slink->stm, ProcessMessage, (Message) {
            .what = SM_ENCP_FAIL, .extData = (void *)(uintptr_t)SM_ERR_STATUS_KEY_MISSING });
        SDF_MapErase(g_slinkMap, &g_stmLinkAddr);
    }

    void FuzzStmMissLchannelDisconn(const uint8_t* fuzzData, size_t size)
    {
        SLE_Addr_S addr;
        (void)memset_s(&addr, sizeof(SLE_Addr_S), 0, sizeof(SLE_Addr_S));
        addr.type = fdp.ConsumeIntegral<uint8_t>();
        for (int i = 0; i < SLE_ADDR_LEN; i++) {
            addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        SmSLink_S *slink = FuzzStmGetSLink(&addr);
        if (slink == nullptr) {
            return;
        }
        STM_MFUNC(slink->stm, Transition, g_smStateName[SM_STATE_MISS]);
        STM_MFUNC(slink->stm, ProcessMessage, (Message) { .what = SM_LCHANNEL_DISCONN });
        SDF_MapErase(g_slinkMap, &addr);
    }

    void FuzzStmMissRemovePair(const uint8_t* fuzzData, size_t size)
    {
        SLE_Addr_S addr;
        (void)memset_s(&addr, sizeof(SLE_Addr_S), 0, sizeof(SLE_Addr_S));
        addr.type = fdp.ConsumeIntegral<uint8_t>();
        for (int i = 0; i < SLE_ADDR_LEN; i++) {
            addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        SmSLink_S *slink = FuzzStmGetSLink(&addr);
        if (slink == nullptr) {
            return;
        }
        STM_MFUNC(slink->stm, Transition, g_smStateName[SM_STATE_MISS]);
        STM_MFUNC(slink->stm, ProcessMessage, (Message) { .what = SM_REMOVE_PAIR });
        SDF_MapErase(g_slinkMap, &addr);
    }

    void FuzzStmFullActiveStart(const uint8_t* fuzzData, size_t size)
    {
        SLE_Addr_S addr;
        (void)memset_s(&addr, sizeof(SLE_Addr_S), 0, sizeof(SLE_Addr_S));
        addr.type = fdp.ConsumeIntegral<uint8_t>();
        for (int i = 0; i < SLE_ADDR_LEN; i++) {
            addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        SmSLink_S *slink = FuzzStmGetSLink(&addr);
        if (slink == nullptr) {
            return;
        }
        STM_MFUNC(slink->stm, Transition, g_smStateName[SM_STATE_FULL]);
        /* 已配对态再来配对事件: 应保持 FULL 并回调 SM_PAIR_OK, 不重入配对 */
        STM_MFUNC(slink->stm, ProcessMessage, (Message) { .what = SM_ACTIVE_START });
        SDF_MapErase(g_slinkMap, &addr);
    }

    void FuzzStmAuthEntryMethods(const uint8_t* fuzzData, size_t size)
    {
        SLE_Addr_S addr;
        (void)memset_s(&addr, sizeof(SLE_Addr_S), 0, sizeof(SLE_Addr_S));
        addr.type = fdp.ConsumeIntegral<uint8_t>();
        for (int i = 0; i < SLE_ADDR_LEN; i++) {
            addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        SmSLink_S *slink = FuzzStmGetSLink(&addr);
        if (slink == nullptr) {
            return;
        }
        /* AuthEntry 按 authMethod 选分发器与启动函数: 六种方法逐输入轮换 */
        slink->negoParams.authMethod = fdp.ConsumeIntegral<uint8_t>() % SM_AUTH_METHOD_MAX;
        slink->gNode.ioAbility = SM_IO_DISPLAY_ONLY;
        slink->tNode.ioAbility = SM_IO_KEYBOARD_ONLY;
        STM_MFUNC(slink->stm, Transition, g_smStateName[SM_STATE_AUTH]);
        SDF_MapErase(g_slinkMap, &addr);
    }

    void FuzzStmEncpEntryRoles(const uint8_t* fuzzData, size_t size)
    {
        SLE_Addr_S addr;
        (void)memset_s(&addr, sizeof(SLE_Addr_S), 0, sizeof(SLE_Addr_S));
        addr.type = fdp.ConsumeIntegral<uint8_t>();
        for (int i = 0; i < SLE_ADDR_LEN; i++) {
            addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        SmSLink_S *slink = FuzzStmGetSLink(&addr);
        if (slink == nullptr) {
            return;
        }
        /* EncpEntry 仅 G 角色调用 SmEncpEnable: 原用例恒走默认 G, 补 T 分支 */
        slink->role = fdp.ConsumeBool() ? SM_T_NODE : SM_G_NODE;
        STM_MFUNC(slink->stm, Transition, g_smStateName[SM_STATE_ENCP]);
        SDF_MapErase(g_slinkMap, &addr);
    }

    void FuzzSMApi(const uint8_t *fuzzData, size_t size)
    {
        FuzzStmInitEncpParamReqReply(fuzzData, size);
        FuzzStmInitRemovePair(fuzzData, size);
        FuzzStmNegoSuccess(fuzzData, size);
        FuzzStmNegoTimeout(fuzzData, size);
        FuzzStmNegoRemovePair(fuzzData, size);
        FuzzStmNegoEncpParamReqReply(fuzzData, size);
        FuzzStmAuthSuccess(fuzzData, size);
        FuzzStmAuthTimeout(fuzzData, size);
        FuzzStmAuthRemovePair(fuzzData, size);
        FuzzStmEncpActiveStart(fuzzData, size);
        FuzzStmEncpSuccess(fuzzData, size);
        FuzzStmEncpParamReqReply(fuzzData, size);
        FuzzStmEncpInternalError(fuzzData, size);
        FuzzStmEncpExternalError(fuzzData, size);
        FuzzStmEncpRemovePair(fuzzData, size);
        FuzzStmFullDisconnect(fuzzData, size);
        FuzzStmFullRemovePair(fuzzData, size);
        FuzzStmInitPassiveStartLinked(fuzzData, size);
        FuzzStmInitActiveStartLinked(fuzzData, size);
        FuzzStmInitRecoverStart(fuzzData, size);
        FuzzStmEncpFailKeyMissingLinked(fuzzData, size);
        FuzzStmMissLchannelDisconn(fuzzData, size);
        FuzzStmMissRemovePair(fuzzData, size);
        FuzzStmFullActiveStart(fuzzData, size);
        FuzzStmAuthEntryMethods(fuzzData, size);
        FuzzStmEncpEntryRoles(fuzzData, size);
    }
}

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void) argc;
    (void) argv;
    NLSTK_InitStack();
    NLSTK_EnableStack();
    NLSTK_SmCryptoAlgoFuncs_S algoFuncs = {
        .randNumFunc = Crypto_RandNumGenerate,
        .pubPriKeyFunc = Crypto_PubPriKeyPairGenerate,
        .secKeyFunc = Crypto_SecKeyGenerate,
        .derivedKeyFunc = Crypto_DerivedKeyGenerate,
    .sha256HashFunc = Crypto_Sha256,
    };
    NLSTK_SmRegAlgoFuncs(&algoFuncs);
    NLSTK_SmCallbacks_S extCbks = {
        .startCbk = FuzzSmStartCbk,
        .removeCbk = FuzzSmRemoveCbk,
        .requestCbk = FuzzSmRequestCbk,
        .authCbk = FuzzSmAuthCbk,
        .encCbk = FuzzSmEncCbk,
        .imgMsgCbk = FuzzSmImgMsgCbk,
    };
    NLSTK_SmRegExternalCbks(&extCbks);
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
    OHOS::FuzzSMApi(fuzzData, size);
    SDF_MemFree(fuzzData);
    return 0;
}