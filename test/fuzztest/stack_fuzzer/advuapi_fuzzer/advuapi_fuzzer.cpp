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
 
#include "advuapi_fuzzer.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "securec.h"
#include "nai_log.h"
#include "sdf_mem.h"
#include "nlstk_devd_def.h"
#include "nlstk_devd_api.h"
#include "dli_errno.h" /* PR345 修复：DLI_SUCCESS */
#include "nlstk_init_api.h"
#include "cpfwk_log.h"
#include "cm_api.h"
#include "devd_tbl.h"
#include "devd_adv.h"
#include "devd_local.h"
#include "nlstk_devd.h"
#include "dli_cmd_struct.h"
#include "devd_cbk.h"
#include "nlstk_cfgdb.h"
#include "../../nl_utils/fuzztest_utils.h"
#include "SleDliThreadUtil.h"
#include "common_ext_func_wrapper.h"

typedef struct SlePkt {
    uint8_t *data;
    uint32_t size;
} SlePkt;

int SleSendDliPacket(const SlePkt *packet)
{
    return 0;
}

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
    FUZZ_TWENTY     = 20,
    FUZZ_TWOHUNDREDFIFTYSIX = 256,
    FUZZ_NUM_END
} FUZZ_NUM_E;

namespace OHOS {
    SleDliThreadUtil &g_dliThreadUtil = SleDliThreadUtil::GetInstance();

    void MockAdvNodeCbk(NLSTK_DevdAdvCbkParam_S *nodeParam) {}
    void MockAdvCbk(NLSTK_DevdAdvCbkParam_S *param) {}
    void FuzzEnableAdv(const uint8_t *fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);

        NLSTK_DevdSetAdvEnable_S *advEnable =
            (NLSTK_DevdSetAdvEnable_S *)SDF_MemZalloc(sizeof(NLSTK_DevdSetAdvEnable_S));
        CP_CHECK_LOG_RETURN_VOID(advEnable != nullptr, "advEnable new fail");
        uint8_t handle = FUZZ_ONE;

        advEnable->advHandle = handle;
        advEnable->enable = ADV_ENABLED;

        // 设定最大节点数量
        DLI_AdvCbkContext *cbkContext = (DLI_AdvCbkContext *)SDF_MemZalloc(sizeof(DLI_AdvCbkContext));
        CP_CHECK_LOG_RETURN_VOID(cbkContext != nullptr, "[Fuzz]memory alloc error");
        cbkContext->advHandle = handle;
        struct DLI_ExecuteCmdRetParam *cmdRes1 = (DLI_ExecuteCmdRetParam *)SDF_MemZalloc(
            sizeof(DLI_ExecuteCmdRetParam));
        CP_CHECK_LOG_RETURN_VOID(cmdRes1 != nullptr, "[Fuzz]memory alloc error");
        cmdRes1->size = sizeof(DevdAdvTerminatedEvent_S);
        cmdRes1->eventParameter = (DevdAdvTerminatedEvent_S *)SDF_MemZalloc(sizeof(DevdAdvTerminatedEvent_S));
        CP_CHECK_LOG_RETURN_VOID(cmdRes1->eventParameter != nullptr, "[Fuzz]memory alloc error");

        NLSTK_DevdCreateAdvHandle(MockAdvCbk);
        SDF_DListHead_S *list = DEVD_ADV_LIST;
        NLSTK_DevdAdvEventCbk cbk = MockAdvNodeCbk;
        NLSTK_DevdAdvCbkParam_S *nodeParam = (NLSTK_DevdAdvCbkParam_S *)SDF_MemZalloc(sizeof(NLSTK_DevdAdvCbkParam_S));
        CP_CHECK_LOG_RETURN_VOID(nodeParam != nullptr, "[Fuzz]memory alloc error");
        nodeParam->advHandle = handle;
        nodeParam->result = FUZZ_ONE;
        nodeParam->event = FUZZ_TWO;
        DevdAdvNode_S *fuzzNode = DevdGetAdvNode(handle, list);
        if (fuzzNode == nullptr) {
            uint8_t createHandle = fdp.ConsumeIntegral<uint8_t>();
            fuzzNode = DevdCreateAdvNode(createHandle, cbk, list);
        }

        // 确保解析16位的数据
        advEnable->duration = fdp.ConsumeIntegral<uint16_t>();
        advEnable->maxAdvEvent = fdp.ConsumeIntegral<uint8_t>();
        advEnable->advHandle = handle;

        NLSTK_DevdEnableAdv(advEnable);
        SDF_MemFree(advEnable);
        SDF_MemFree(cbkContext);
        SDF_MemFree(cmdRes1->eventParameter);
        SDF_MemFree(cmdRes1);
        SDF_MemFree(nodeParam);
    }

    void FuzzUapiSetAdv(const uint8_t *fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);

        NLSTK_DevdSetAdvParams_S advParams;
        NLSTK_DevdAdvParam_S advParam = {0};
        NLSTK_DevdAdvData_S advData;

        advParam.advHandle = 1;
        advParam.advIntervalMin = fdp.ConsumeIntegral<uint32_t>();
        advParam.advIntervalMax = fdp.ConsumeIntegral<uint32_t>();
        advParam.advChannelMap = fdp.ConsumeIntegral<uint8_t>() % FUZZ_THREE;
        advParam.ownAddr.type = fdp.ConsumeIntegral<uint8_t>();

        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            advParam.ownAddr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }

        uint8_t *advDataBuf = (uint8_t *)malloc(FUZZ_TWOHUNDREDFIFTYSIX);
        uint8_t *scanRspDataBuf = (uint8_t *)malloc(FUZZ_TWOHUNDREDFIFTYSIX);
        CP_CHECK_LOG_RETURN_VOID(advDataBuf != nullptr && scanRspDataBuf != nullptr, "[Fuzz]memory alloc error");

        uint8_t advDataLen = fdp.ConsumeIntegral<uint8_t>();
        uint8_t scanRspDataLen = fdp.ConsumeIntegral<uint8_t>();
        advData.advDataLen = advDataLen;
        advData.scanRspDataLen = scanRspDataLen;

        for (uint8_t i = 0; i < advDataLen; i++) {
            advDataBuf[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        for (uint8_t i = 0; i < scanRspDataLen; i++) {
            scanRspDataBuf[i] = fdp.ConsumeIntegral<uint8_t>();
        }

        // Set adv_data and scan_rsp_data
        advData.advData = advDataBuf;
        advData.scanRspData = scanRspDataBuf;

        // Create a struct to hold the parameters
        advParams.param = advParam;
        advParams.data = advData;
        advParams.param.extParam = nullptr; // Assuming ext_param is not used in this test
        NLSTK_DevdConnParam_S *connParam = (NLSTK_DevdConnParam_S *)SDF_MemZalloc(sizeof(NLSTK_DevdConnParam_S));
        CP_CHECK_LOG_RETURN_VOID(connParam != nullptr, "[Fuzz]memory alloc error");
        connParam->intervalMin = fdp.ConsumeIntegral<uint16_t>();
        connParam->intervalMax = fdp.ConsumeIntegral<uint16_t>();
        connParam->maxLatency = fdp.ConsumeIntegral<uint16_t>();
        connParam->supervisionTimeout = fdp.ConsumeIntegral<uint16_t>();
        connParam->minCeLength = fdp.ConsumeIntegral<uint16_t>();
        connParam->maxCeLength = fdp.ConsumeIntegral<uint16_t>();
        advParams.param.connParam = connParam;

        NLSTK_DevdStartAdv(&advParams);
        SDF_MemFree(connParam);
        free(advDataBuf);
        free(scanRspDataBuf);
    }

    void FuzzUapiRemoveAdv(const uint8_t *fuzzData, size_t size)
    {
        uint8_t advHandle = 1;
        NLSTK_DevdRemoveAdv(&advHandle);
    }

    void FuzzUpdateAdvData(const uint8_t *fuzzData, size_t size)
    {
        FuzzedDataProvider fdp(fuzzData, size);
        uint8_t handle = 1;

        NLSTK_DevdSetAdvData_S setData;
        setData.advHandle = handle;
        uint8_t advDataLen = fdp.ConsumeIntegral<uint8_t>();
        uint8_t scanRspDataLen = fdp.ConsumeIntegral<uint8_t>();
        setData.data.advDataLen = advDataLen;
        setData.data.scanRspDataLen = scanRspDataLen;

        setData.data.advData = (uint8_t *)SDF_MemZalloc(advDataLen);
        CP_CHECK_LOG_RETURN_VOID(setData.data.advData != nullptr, "[Fuzz]memory alloc error");
        setData.data.scanRspData = (uint8_t *)SDF_MemZalloc(scanRspDataLen);
        CP_CHECK_LOG_RETURN_VOID(setData.data.scanRspData != nullptr, "[Fuzz]memory alloc error");

        for (uint8_t i = 0; i < advDataLen; i++) {
            setData.data.advData[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        for (uint8_t i = 0; i < scanRspDataLen; i++) {
            setData.data.scanRspData[i] = fdp.ConsumeIntegral<uint8_t>();
        }

        NLSTK_DevdSetAdvData(&setData);
        SDF_MemFree(setData.data.advData);
        SDF_MemFree(setData.data.scanRspData);
    }

    void FuzzRemoveAdv(const uint8_t *fuzzData, size_t size)
    {
        if (size < sizeof(uint8_t)) {
            return;
        }
        uint8_t advHandle = 1;

        NLSTK_DevdRemoveAdv(&advHandle);
    }

    /* ========== PR-5 C3: 广播分片闭环（S9） ==========
     * MaxAdvDataLen 无公开 setter（g_maxAdvDatalen 静态于 nlstk_cfgdb.c:76，仅芯片读回包可改），
     * 但默认 251 < 表项上限 0x7FF（devd_tbl.h），>251 字节广播数据天然分片：
     * FIRST/INTERMEDIATE/LAST 由 GetAdvDataOperation 真实决策；分片推进依赖 DLI 回包，
     * fuzz 桩（SleSendDliPacket 返回 0 不回包）下靠直调 DevdSetAdvDataCbk 逐片推进。
     * CfgdbGetMaxAdvNodesNum 默认 1，各阶段串行复用单个节点：create→驱动→remove。 */
    static uint32_t g_advFragCbkCnt = 0;
    static bool g_advFragLogged = false;

    static void FuzzAdvFragCbk(NLSTK_DevdAdvCbkParam_S *param)
    {
        g_advFragCbkCnt++;
        if (!g_advFragLogged) {
            NAI_LOG_INFO("[Fuzz] adv frag cbk first reach, event:%u result:%u", param->event, param->result);
            g_advFragLogged = true;
        }
    }

    static void FuzzFeedSetAdvData(uint8_t handle, uint16_t advLen, uint16_t scanRspLen)
    {
        NLSTK_DevdSetAdvData_S setData = {};
        setData.advHandle = handle;
        setData.data.advDataLen = advLen;
        setData.data.scanRspDataLen = scanRspLen;
        setData.data.advData = (uint8_t *)SDF_MemZalloc(advLen);
        setData.data.scanRspData = (uint8_t *)SDF_MemZalloc(scanRspLen);
        if (setData.data.advData == NULL || (scanRspLen != 0 && setData.data.scanRspData == NULL)) {
            SDF_MemFree(setData.data.advData);
            SDF_MemFree(setData.data.scanRspData);
            return;
        }
        for (uint16_t i = 0; i < advLen; i++) {
            setData.data.advData[i] = (uint8_t)(i & 0xFF);
        }
        DevdSetAdvData(&setData);
        SDF_MemFree(setData.data.advData);
        SDF_MemFree(setData.data.scanRspData);
    }

    static void FuzzRemoveAdvNodeByCbk(uint8_t handle)
    {
        DLI_AdvCbkContext ctx = {};
        ctx.advHandle = handle;
        DevdRemoveAdv(&handle);
        DevdRemoveAdvCbk(&ctx, DLI_SUCCESS, NULL);
    }

    void FuzzAdvFragmentation(const uint8_t *fuzzData, size_t size)
    {
        (void)fuzzData;
        (void)size;
        /* 全常量确定性分片序列: 每输入重放无新增覆盖, 产品侧 SetAdvData/Param 每片叠 INFO
         * 日志, 全量重放构成吞吐/日志杀手(红队 P1-2), 故仅首输入执行 */
        static bool fuzzFragDone = false;
        if (fuzzFragDone) {
            return;
        }
        fuzzFragDone = true;
        uint16_t chunk = CfgdbGetMaxAdvDataLen();
        if (chunk == 0) {
            return;
        }
        uint16_t bigLen = (uint16_t)(chunk * FUZZ_TWO + FUZZ_TWENTY); /* >2 片：FIRST+INTERMEDIATE+LAST */
        uint16_t fragCnt = (uint16_t)((bigLen + chunk - 1) / chunk);
        DevdLocalDeviceDeInit(); /* 清空历史广播节点（默认最大节点数为 1） */
        DevdLocalDeviceInit();

        /* 1. 更新数据路径：大数据 FIRST/INTERMEDIATE/LAST 逐片推进至完成；
         *    再发一轮后中途失败回包（node 有历史数据 → 回 IDLE 保留节点） */
        uint8_t updHandle = NLSTK_DevdCreateAdvHandle(FuzzAdvFragCbk);
        if (updHandle == DEVD_INVALID_ADV_HANDLE) {
            return;
        }
        DLI_AdvCbkContext updCtx = {};
        updCtx.advHandle = updHandle;
        FuzzFeedSetAdvData(updHandle, bigLen, 0);
        for (uint16_t i = 0; i < fragCnt; i++) {
            DevdSetAdvDataCbk(&updCtx, DLI_SUCCESS, NULL);
        }
        FuzzFeedSetAdvData(updHandle, bigLen, 0);
        DevdSetAdvDataCbk(&updCtx, DLI_SUCCESS, NULL); /* FIRST 在途 */
        DevdSetAdvDataCbk(&updCtx, FUZZ_ONE, NULL);    /* 失败回包回滚 */
        FuzzRemoveAdvNodeByCbk(updHandle);

        /* 2. 设置参数+扫描响应分片路径：advData 单片 COMPLETE，scanRsp 大数据分片；
         *    完成后产品侧自动 DevdEnableAdv，直调 EnableAdvCbk 覆盖使能/去使能/失败与终止事件 */
        uint8_t setHandle = NLSTK_DevdCreateAdvHandle(FuzzAdvFragCbk);
        if (setHandle == DEVD_INVALID_ADV_HANDLE) {
            return;
        }
        DLI_AdvCbkContext setCtx = {};
        setCtx.advHandle = setHandle;
        uint8_t *advBuf = (uint8_t *)SDF_MemZalloc(FUZZ_THREE);
        uint8_t *rspBuf = (uint8_t *)SDF_MemZalloc(bigLen);
        if (advBuf == NULL || rspBuf == NULL) {
            SDF_MemFree(advBuf);
            SDF_MemFree(rspBuf);
            FuzzRemoveAdvNodeByCbk(setHandle);
            return;
        }
        DevdSetAdvParams_S advParams = {};
        advParams.param.handle = setHandle;
        advParams.param.basic.advMode = ADV_MODE_NONCONN_SCANABLE; /* 允许下发扫描响应数据 */
        advParams.param.basic.advMinInterval = DEVD_ADV_INTERVAL_MIN;
        advParams.param.basic.advMaxInterval = DEVD_ADV_INTERVAL_MAX;
        advParams.data.advDataLen = FUZZ_THREE;
        advParams.data.advData = advBuf;
        advParams.data.scanRspDataLen = bigLen;
        advParams.data.scanRspData = rspBuf;
        DevdSetAdvParam(&advParams);
        DevdSetAdvParamCbk(&setCtx, DLI_SUCCESS, NULL); /* 参数成功 → SET_ADV_DATA */
        for (uint16_t i = 0; i <= fragCnt; i++) {       /* 1 片 adv + fragCnt 片 scanRsp */
            DevdSetAdvDataCbk(&setCtx, DLI_SUCCESS, NULL);
        }
        DevdEnableAdvCbk(&setCtx, DLI_SUCCESS, NULL); /* 末片完成后自动使能，回包覆盖使能成功分支 */
        NLSTK_DevdSetAdvEnable_S advDisable = {};
        advDisable.advHandle = setHandle;
        DevdEnableAdv(&advDisable);                     /* 去使能下发 */
        DevdEnableAdvCbk(&setCtx, DLI_SUCCESS, NULL);   /* 去使能成功分支 */
        DevdEnableAdvCbk(&setCtx, FUZZ_ONE, NULL);      /* 去使能失败分支 */
        DLI_ExecuteCmdRetParam termRes = {};
        DevdAdvTerminatedEvent_S termEvt = {};
        termEvt.status = DLI_SUCCESS;
        termEvt.advHandle = setHandle;
        termEvt.connHandle = FUZZ_ONE;
        termRes.size = sizeof(DevdAdvTerminatedEvent_S);
        termRes.eventParameter = &termEvt;
        DevdAdvTerminatedCbk(&setCtx, DLI_SUCCESS, &termRes); /* 合法终止事件 */
        termRes.size = FUZZ_ONE;                              /* size 不足被拒 */
        DevdAdvTerminatedCbk(&setCtx, DLI_SUCCESS, &termRes);
        DevdAdvTerminatedCbk(&setCtx, DLI_SUCCESS, NULL);     /* cmdRes 空被拒 */
        FuzzRemoveAdvNodeByCbk(setHandle);

        /* 3. 设置参数失败回滚：node->param 为空 → 释放 tempParam/tempData 并移除节点
         *    （advParams 复用阶段 2 缓冲区，须在其释放前执行） */
        uint8_t paramHandle = NLSTK_DevdCreateAdvHandle(FuzzAdvFragCbk);
        if (paramHandle != DEVD_INVALID_ADV_HANDLE) {
            DLI_AdvCbkContext paramCtx = {};
            paramCtx.advHandle = paramHandle;
            advParams.param.handle = paramHandle;
            DevdSetAdvParam(&advParams);
            DevdSetAdvParamCbk(&paramCtx, FUZZ_ONE, NULL); /* 失败 → 节点回滚移除 */
        }
        SDF_MemFree(advBuf);
        SDF_MemFree(rspBuf);

        /* 4. 无历史数据节点的数据失败回滚：node->data 为空 → 移除节点 */
        uint8_t rollHandle = NLSTK_DevdCreateAdvHandle(FuzzAdvFragCbk);
        if (rollHandle != DEVD_INVALID_ADV_HANDLE) {
            DLI_AdvCbkContext rollCtx = {};
            rollCtx.advHandle = rollHandle;
            FuzzFeedSetAdvData(rollHandle, FUZZ_FOUR, 0);
            DevdSetAdvDataCbk(&rollCtx, FUZZ_ONE, NULL);
        }

        /* 5. 分片中途 Remove + 晚到回包：移除后晚到 SetAdvDataCbk 仅记录不 abort */
        uint8_t midHandle = NLSTK_DevdCreateAdvHandle(FuzzAdvFragCbk);
        if (midHandle != DEVD_INVALID_ADV_HANDLE) {
            DLI_AdvCbkContext midCtx = {};
            midCtx.advHandle = midHandle;
            FuzzFeedSetAdvData(midHandle, bigLen, 0);
            DevdSetAdvDataCbk(&midCtx, DLI_SUCCESS, NULL); /* FIRST 在途 */
            FuzzRemoveAdvNodeByCbk(midHandle);             /* 中途移除 */
            DevdSetAdvDataCbk(&midCtx, DLI_SUCCESS, NULL); /* 晚到回包：节点已空，安全返回 */
        }
    }

    /* ========== PR-10 R5: 广播深边界（非分片路径） ==========
     * 目标 devd_adv.c miss 分支：长度恰好 chunk/chunk+1 边界、busy/ENABLED 态幂等拒绝链、
     * 失败回包后再设置、更新路径 scanRsp 分片（SET_SCAN_RSP_DATA 事件）、BuildAdvParam
     * 连接分支+帧四默认连接间隔覆盖、移除失败回包节点保留。 */
    static uint32_t g_advDeepCbkCnt = 0;
    static bool g_advDeepLogged = false;

    static void FuzzAdvDeepCbk(NLSTK_DevdAdvCbkParam_S *param)
    {
        g_advDeepCbkCnt++;
        if (!g_advDeepLogged) {
            NAI_LOG_INFO("[Fuzz] adv deep cbk first reach, event:%u result:%u", param->event, param->result);
            g_advDeepLogged = true;
        }
    }

    /* R5: 回包成败 FDP 化（bool 域映射 DLI_SUCCESS/失败，失败值沿用 FUZZ_ONE） */
    static uint16_t FuzzCbkStatus(FuzzedDataProvider &fdp)
    {
        return fdp.ConsumeBool() ? DLI_SUCCESS : FUZZ_ONE;
    }

    /* 内部直调 DevdSetAdvParam 铺垫：合法基本参数+数据（DevdSave*ToTbl 深拷贝，返回后可释放） */
    static void FuzzFeedSetAdvParam(uint8_t handle, uint8_t advMode, uint8_t frameType, uint8_t gtRole,
        uint16_t advLen, uint16_t scanRspLen)
    {
        DevdSetAdvParams_S advParams = {};
        advParams.param.handle = handle;
        advParams.param.basic.advMode = advMode;
        advParams.param.basic.gtRole = gtRole;
        advParams.param.basic.advMinInterval = DEVD_ADV_INTERVAL_MIN;
        advParams.param.basic.advMaxInterval = DEVD_ADV_INTERVAL_MAX;
        advParams.param.basic.primaryFrameType = frameType;
        if (gtRole != ADV_GT_ROLE_T_NO_NEGO) {
            advParams.param.connect.connMinInterval = CONN_INTV_MIN;
            advParams.param.connect.connMaxInterval = CONN_INTV_MAX;
            advParams.param.connect.maxLatency = 0;
            advParams.param.connect.supervisionTimeout = CONN_SUPERVISION_TIMEOUT_MAX;
        }
        advParams.data.advDataLen = advLen;
        advParams.data.advData = (uint8_t *)SDF_MemZalloc(advLen);
        advParams.data.scanRspDataLen = scanRspLen;
        advParams.data.scanRspData = (uint8_t *)SDF_MemZalloc(scanRspLen);
        if (advParams.data.advData == NULL || (scanRspLen != 0 && advParams.data.scanRspData == NULL)) {
            SDF_MemFree(advParams.data.advData);
            SDF_MemFree(advParams.data.scanRspData);
            return;
        }
        DevdSetAdvParam(&advParams);
        SDF_MemFree(advParams.data.advData);
        SDF_MemFree(advParams.data.scanRspData);
    }

    void FuzzAdvDeepBoundary(const uint8_t *fuzzData, size_t size)
    {
        /* R5 整改: 去 (void)fuzzData 与 static 一次性门, 每输入执行; 长度/回包成败/模式改 FDP 域表,
         * 深链语义时序(busy 拒绝/LAST 失败回滚/交错 scanRsp 分片)由 seed_adv_deep_* 重放 */
        FuzzedDataProvider fdp(fuzzData, size);
        uint16_t chunk = CfgdbGetMaxAdvDataLen();
        if (chunk == 0) {
            return;
        }
        uint16_t bigLen = (uint16_t)(chunk * FUZZ_TWO + FUZZ_TWENTY);
        uint16_t fragCnt = (uint16_t)((bigLen + chunk - 1) / chunk);
        /* 长度域表: {0, 1, chunk-1, chunk, chunk+1, 2*chunk+20} 边界语义, chunk 动态取 */
        const uint16_t lenDomain[FUZZ_SIX] = {FUZZ_ZERO, FUZZ_ONE, (uint16_t)(chunk - FUZZ_ONE), chunk,
            (uint16_t)(chunk + FUZZ_ONE), bigLen};
        DevdLocalDeviceDeInit(); /* 每输入重置: 默认最大广播节点数 1, 清空历史节点 */
        DevdLocalDeviceInit();

        /* 1. 铺底(首三步常量成功, 审计允许): 参数+单片数据闭环至 ENABLED, 为更新路径提供 node->param;
         *    advMode(合法四模)/帧型(帧一+帧四族)/gtRole(四角色)域表 FDP 选取, 深链组合由种子重放 */
        uint8_t handle = NLSTK_DevdCreateAdvHandle(FuzzAdvDeepCbk);
        if (handle == DEVD_INVALID_ADV_HANDLE) {
            return;
        }
        DLI_AdvCbkContext ctx = {};
        ctx.advHandle = handle;
        FuzzFeedSetAdvParam(handle, fdp.ConsumeIntegral<uint8_t>() % FUZZ_FOUR,
            fdp.ConsumeIntegral<uint8_t>() % FUZZ_FOUR, fdp.ConsumeIntegral<uint8_t>() % FUZZ_FOUR,
            FUZZ_FOUR, FUZZ_ZERO);
        DevdSetAdvParamCbk(&ctx, DLI_SUCCESS, NULL);
        DevdSetAdvDataCbk(&ctx, DLI_SUCCESS, NULL); /* 单片完成自动使能 */
        DevdEnableAdvCbk(&ctx, DLI_SUCCESS, NULL);

        /* 2. 长度边界（更新路径，node 有参数）：域表选取, 种子重放"恰好 chunk 单片 COMPLETE" */
        FuzzFeedSetAdvData(handle, lenDomain[fdp.ConsumeIntegral<uint8_t>() % FUZZ_SIX], FUZZ_ZERO);
        DevdSetAdvDataCbk(&ctx, FuzzCbkStatus(fdp), NULL);
        /* 3. 双片在途链：FIRST 在途 busy 拒绝 + LAST 失败回滚(有历史数据→回 IDLE) + 失败后再设置 */
        FuzzFeedSetAdvData(handle, lenDomain[fdp.ConsumeIntegral<uint8_t>() % FUZZ_SIX], FUZZ_ZERO);
        DevdSetAdvDataCbk(&ctx, FuzzCbkStatus(fdp), NULL); /* >chunk 时 FIRST 在途, 节点 busy */
        NLSTK_DevdSetAdvEnable_S busyDisable = {};
        busyDisable.advHandle = handle;
        DevdEnableAdv(&busyDisable);          /* enable=0 先过 advStatus 检查，busy 态拒绝 */
        FuzzFeedSetAdvData(handle, lenDomain[fdp.ConsumeIntegral<uint8_t>() % FUZZ_SIX],
            FUZZ_ZERO);                       /* busy 态更新数据拒绝 */
        DevdSetAdvDataCbk(&ctx, FuzzCbkStatus(fdp), NULL); /* 在途失败回滚 */
        FuzzFeedSetAdvData(handle, lenDomain[fdp.ConsumeIntegral<uint8_t>() % FUZZ_SIX],
            FUZZ_ZERO);                       /* 失败后再设置 */
        DevdSetAdvDataCbk(&ctx, FuzzCbkStatus(fdp), NULL); /* 单片成功闭环 */

        /* 4. 交错设置（更新路径 scanRsp 分片链）：advData 单片完成→SET_SCAN_RSP_DATA→
         *    scanRsp 大数据 FIRST/INTERMEDIATE/LAST→IDLE→事件通知+数据交换（双长度域表, 成败 FDP） */
        FuzzFeedSetAdvData(handle, lenDomain[fdp.ConsumeIntegral<uint8_t>() % FUZZ_SIX],
            lenDomain[fdp.ConsumeIntegral<uint8_t>() % FUZZ_SIX]);
        DevdSetAdvDataCbk(&ctx, FuzzCbkStatus(fdp), NULL); /* advData 完成，转扫描响应分片 */
        for (uint16_t i = 0; i < fragCnt; i++) {
            DevdSetAdvDataCbk(&ctx, FuzzCbkStatus(fdp), NULL);
        }

        /* 5. 幂等拒绝：再使能/设参/移除均由产品 advStatus 检查决策（模式域表, 形态合法） */
        NLSTK_DevdSetAdvEnable_S reEnable = {};
        reEnable.advHandle = handle;
        reEnable.enable = ADV_ENABLED;
        DevdEnableAdv(&reEnable);
        FuzzFeedSetAdvParam(handle, fdp.ConsumeIntegral<uint8_t>() % FUZZ_FOUR,
            fdp.ConsumeIntegral<uint8_t>() % FUZZ_FOUR, fdp.ConsumeIntegral<uint8_t>() % FUZZ_FOUR,
            FUZZ_FOUR, FUZZ_ZERO);
        DevdRemoveAdv(&handle);
        /* 6. 去使能→回包(成败 FDP)→DISABLED；移除失败回包(节点保留)→再移除→成功回包(节点移除) */
        DevdEnableAdv(&busyDisable);
        DevdEnableAdvCbk(&ctx, FuzzCbkStatus(fdp), NULL);
        DevdRemoveAdv(&handle);
        DevdRemoveAdvCbk(&ctx, FuzzCbkStatus(fdp), NULL);
        DevdRemoveAdv(&handle);
        DevdRemoveAdvCbk(&ctx, FuzzCbkStatus(fdp), NULL);

        /* 7. BuildAdvParam 连接分支+帧四覆盖（内部直调绕过 NLSTK 校验，仅取构建路径）：
         *    gtRole 域表(可协商→连接参数拷贝)、帧型域表(4_M_0→DEFAULT_FRAME_4_CONN_INTERVAL)、
         *    长度域表；回包成败 FDP(失败→无历史参数节点回滚移除) */
        uint8_t connHandle = NLSTK_DevdCreateAdvHandle(FuzzAdvDeepCbk);
        if (connHandle != DEVD_INVALID_ADV_HANDLE) {
            DLI_AdvCbkContext connCtx = {};
            connCtx.advHandle = connHandle;
            FuzzFeedSetAdvParam(connHandle, fdp.ConsumeIntegral<uint8_t>() % FUZZ_FOUR,
                fdp.ConsumeIntegral<uint8_t>() % FUZZ_FOUR, fdp.ConsumeIntegral<uint8_t>() % FUZZ_FOUR,
                lenDomain[fdp.ConsumeIntegral<uint8_t>() % FUZZ_SIX],
                lenDomain[fdp.ConsumeIntegral<uint8_t>() % FUZZ_SIX]);
            DevdSetAdvParamCbk(&connCtx, FuzzCbkStatus(fdp), NULL);
        }

        /* 8. NLSTK 公开入口边界：len=0/超上限(0x7FF+1) 拒绝、非法使能值拒绝、空参拒绝
         *    （空指针/常量边界形态无法随机化, 保留常量, 属错误矩阵） */
        NLSTK_DevdSetAdvData_S badData = {};
        uint8_t tinyBuf[FUZZ_FOUR] = {0};
        badData.advHandle = FUZZ_ONE;
        badData.data.advData = tinyBuf;
        (void)NLSTK_DevdSetAdvData(&badData); /* advDataLen=0 拒绝 */
        badData.data.advDataLen = DEFAULT_MAX_ADV_DATA_LEN + FUZZ_ONE;
        (void)NLSTK_DevdSetAdvData(&badData); /* 超上限拒绝（拷贝前校验返回） */
        NLSTK_DevdSetAdvEnable_S badEnable = {};
        badEnable.advHandle = FUZZ_ONE;
        badEnable.enable = FUZZ_TWO;
        (void)NLSTK_DevdEnableAdv(&badEnable);    /* 非法使能值拒绝 */
        (void)NLSTK_DevdCreateAdvHandle(NULL);    /* 空回调拒绝 */
        (void)NLSTK_DevdRemoveAdv(NULL);          /* 空句柄指针拒绝 */
        (void)NLSTK_DevdSetAdvData(NULL);
        (void)NLSTK_DevdStartAdv(NULL);
        (void)NLSTK_DevdEnableAdv(NULL);

        /* 每输入资源配对: 移除链成败 FDP 化后节点可能残留, 兜底回收
         * (已移除句柄晚到回包走产品"节点为空"安全返回, 见 devd_adv.c GetAdvNode 判空) */
        FuzzRemoveAdvNodeByCbk(handle);
        FuzzRemoveAdvNodeByCbk(connHandle);
    }

    void FuzzAdvApi(const uint8_t *fuzzData, size_t size)
    {
        DevdLocalDeviceInit();
        FuzzUapiSetAdv(fuzzData, size);
        FuzzUpdateAdvData(fuzzData, size);
        FuzzEnableAdv(fuzzData, size);

        FuzzUapiRemoveAdv(fuzzData, size);
        FuzzRemoveAdv(fuzzData, size);
        FuzzAdvFragmentation(fuzzData, size);
        FuzzAdvDeepBoundary(fuzzData, size);
        DevdLocalDeviceDeInit();
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
    OHOS::FuzzAdvApi(fuzzData, size);
    SDF_MemFree(fuzzData);
    OHOS::g_dliThreadUtil.DestroyQueue();
    return 0;
}