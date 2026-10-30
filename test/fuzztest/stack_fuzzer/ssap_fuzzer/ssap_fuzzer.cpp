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
#include "ssap_fuzzer.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "ssaps_server.h"
#include "ssaps_server_api.h"
#include "ssaps_service.h"
#include "ssapc_client.h"
#include "ssapc_client_api.h"
#include "ssap_link.h"
#include "ssap_manager.h"
#include "ssap_common.h"
#include "sdf_mem.h"
#include "sdf_thread.h"
#include "sdf_evc.h"
#include "nai_log.h"
#include "nlstk_init_api.h"
#include "nlstk_schedule.h"
#include "ssap_type.h"
#include "dli_layer_callback.h"
#include "sdf_string.h"

static NLSTK_SsapUuid_S g_uuid1 = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01, 0x02};
static NLSTK_SsapUuid_S g_uuid2 = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
    0x0c, 0x0d, 0x0e, 0x0f, 0x10};
static NLSTK_SsapUuid_S g_uuid3 = {0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c,
    0x0d, 0x0e, 0x0f, 0x10, 0x11};
static NLSTK_SsapUuid_S g_uuid4 = {0x10, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00,
    0x0a, 0x0b, 0x0c, 0x0d, 0x0e};

static uint8_t reqPkt1[] = {0x04, 0x11, 0x01, 0x00, 0xFF, 0x00};
static uint8_t reqPkt2[] = {0x04, 0x13, 0x01, 0x00, 0xFF, 0x00};
static uint8_t reqPkt3[] = {0x06, 0x11, 0x01, 0x00, 0xFF, 0x00, 0x02, 0x01};
static uint8_t reqPkt4[] = {0x06, 0x13, 0x01, 0x00, 0xFF, 0x00, 0x03, 0x02};
static uint8_t reqPkt5[] = {0x0a, 0x00, 0x01, 0x00, 0xFF, 0x00, 0x00, 0x02, 0x01};
/* 读/写族合法报文与长度边界模板（取自产品单测夹具，保证字段合法） */
static uint8_t readReqPkts[][5] = {
    {0x08, 0x03, 0x11, 0x00, 0x00},
    {0x08, 0x03, 0x12, 0x00, 0x00},
    {0x08, 0x03, 0x11, 0x00, 0x01},
};
static uint8_t writeReqPkts[][6] = {
    {0x0d, 0x03, 0x11, 0x00, 0x00, 0x06},
    {0x0d, 0x03, 0x11, 0x00, 0x01, 0x07},
    {0x0d, 0x03, 0x12, 0x00, 0x00, 0x06},
    {0x0d, 0x03, 0x12, 0x00, 0x00},
};
static uint8_t writeCmdPkts[][6] = {
    {0x0C, 0x07, 0x11, 0x00, 0x00, 0x06},
    {0x0C, 0x13, 0x11, 0x00, 0x00, 0x06},
    {0x0C, 0x00, 0x11, 0x00, 0x00, 0x06},
    {0x0C, 0x03, 0x11, 0x00, 0x00, 0x06},
};

static void AddService1()
{
    SSAP_ParamAddService_S *serviceParam = (SSAP_ParamAddService_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddService_S));
    serviceParam->serviceType = ITEM_TYPE_STD_PRIMARY_SERVICE;
    memcpy_s(&serviceParam->uuid, sizeof(NLSTK_SsapUuid_S), &g_uuid1, sizeof(NLSTK_SsapUuid_S));
    SSAP_CacheService(serviceParam);
    SDF_MemFree(serviceParam);
    SSAP_ParamAddProperty_S *propertyParam =
        (SSAP_ParamAddProperty_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddProperty_S) + 1);
    memcpy_s(&propertyParam->uuid, sizeof(NLSTK_SsapUuid_S), &g_uuid2, sizeof(NLSTK_SsapUuid_S));
    propertyParam->val.len = 1;
    propertyParam->val.value[0] = 0xFF;
    propertyParam->operation.operationValue = 1;
    SSAP_CacheProperty(propertyParam);
    SDF_MemFree(propertyParam);
    SSAP_ParamAddDescriptor_S *descParam =
        (SSAP_ParamAddDescriptor_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddDescriptor_S) + 1);
    descParam->type = DESC_TYPE_PROPERTY_INSTRUCTION;
    descParam->val.len = 1;
    descParam->val.value[0x00] = 0xEE;
    descParam->permission.permissionValue = 0x04;
    descParam->operation.operationValue = 0x07;
    SSAP_CacheDescriptor(descParam);
    SDF_MemFree(descParam);
    SSAP_ParamAddProperty_S *propertyParam1 =
        (SSAP_ParamAddProperty_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddProperty_S) + 1);
    memcpy_s(&propertyParam1->uuid, sizeof(NLSTK_SsapUuid_S), &g_uuid2, sizeof(NLSTK_SsapUuid_S));
    propertyParam1->val.len = 1;
    propertyParam1->val.value[0x00] = 0xAA;
    propertyParam1->operation.operationValue = 0x73F;
    propertyParam1->permission.permissionValue = 0x04;
    propertyParam1->operation.operationValue = 0x19;
    SSAP_CacheProperty(propertyParam1);
    SDF_MemFree(propertyParam1);
    SSAP_ParamAddDescriptor_S *descParam1 =
        (SSAP_ParamAddDescriptor_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddDescriptor_S) + 0x02);
    descParam1->type = DESC_TYPE_CLIENT_CONFIG;
    descParam1->val.len = 0x02;
    descParam1->val.value[0] = 0x01;
    descParam1->val.value[1] = 0x00;
    descParam1->operation.operationValue = 0x73F;
    SSAP_CacheDescriptor(descParam1);
    SDF_MemFree(descParam1);
    SSAP_StartService(NULL);
}

static void AddService2()
{
    SSAP_ParamAddService_S *serviceParam1 = (SSAP_ParamAddService_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddService_S));
    serviceParam1->serviceType = ITEM_TYPE_STD_PRIMARY_SERVICE;
    memcpy_s(&serviceParam1->uuid, sizeof(NLSTK_SsapUuid_S), &g_uuid3, sizeof(NLSTK_SsapUuid_S));
    SSAP_CacheService(serviceParam1);
    SDF_MemFree(serviceParam1);
    SSAP_ParamAddProperty_S *propertyParam2 =
        (SSAP_ParamAddProperty_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddProperty_S) + 0x02);
    memcpy_s(&propertyParam2->uuid, sizeof(NLSTK_SsapUuid_S), &g_uuid4, sizeof(NLSTK_SsapUuid_S));
    propertyParam2->val.len = 0x02;
    propertyParam2->val.value[0] = 0xAB;
    propertyParam2->val.value[1] = 0xCD;
    propertyParam2->operation.operationValue = 1;
    SSAP_CacheProperty(propertyParam2);
    SDF_MemFree(propertyParam2);
    SSAP_StartService(NULL);
}

namespace OHOS {

    void SendFunc(SSAP_Link_S *link, SDF_Buff_S *buff, uint8_t opcode)
    {
        (void)link;
        (void)buff;
        (void)opcode;
    }

    void FuzzDliSsapRecvPkt(uint8_t* data, size_t size)
    {
        FuzzedDataProvider fdp(data, size);

        SSAP_LinkInit();
        SSAPS_ServiceInit();
        AddService1();
        AddService2();

        /* 先取 opcode 再解析地址/报文：ConsumeIntegral 从尾部消费，
         * 若放在 buf 循环之后会因 FDP 耗尽恒取 0（模板选择变死代码） */
        uint8_t fuzzOpcode = fdp.ConsumeIntegral<uint8_t>();

        SLE_Addr_S sleAddr = {0};
        sleAddr.type = fdp.ConsumeIntegral<uint8_t>();
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            sleAddr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        SSAP_Link_S *link = SSAP_CreateSsapLink(&sleAddr, 0, SendFunc);

        SDF_Buff_S *sdfBuff = SDF_BuffNewWithReserve(size);
        uint8_t *buf = SDF_BuffAppend(sdfBuff, size);
        for (size_t i = 0; i < size; i++) {
            buf[i] = fdp.ConsumeIntegral<uint8_t>();
        }

        link->curTask.opcode = static_cast<uint8_t>(fuzzOpcode - 1);

        int realSize = sizeof(reqPkt1);
        uint8_t *reqPkt = reqPkt1;
        if (link->curTask.opcode == SSAP_FIND_STRUCTURE_REQ) {
            if ((fdp.ConsumeIntegral<uint8_t>() & 0x03) == 3) {
                reqPkt = reqPkt2;
                realSize = sizeof(reqPkt2);
            } else {
                reqPkt = reqPkt1;
                realSize = sizeof(reqPkt1);
            }
        }

        if (link->curTask.opcode == SSAP_FIND_STRUCTURE_BY_UUID_REQ) {
            if ((fdp.ConsumeIntegral<uint8_t>() & 0x03) == 3) {
                reqPkt = reqPkt3;
                realSize = sizeof(reqPkt3);
            } else {
                reqPkt = reqPkt4;
                realSize = sizeof(reqPkt4);
            }
        }

        if (link->curTask.opcode == SSAP_READ_BY_UUID_REQ) {
            reqPkt = reqPkt5;
            realSize = sizeof(reqPkt5);
        } else if (link->curTask.opcode == SSAP_READ_REQ) {
            uint8_t variant = fdp.ConsumeIntegral<uint8_t>() %
                (sizeof(readReqPkts) / sizeof(readReqPkts[0]));
            reqPkt = readReqPkts[variant];
            realSize = sizeof(readReqPkts[variant]);
        } else if (link->curTask.opcode == SSAP_WRITE_REQ) {
            uint8_t variant = fdp.ConsumeIntegral<uint8_t>() %
                (sizeof(writeReqPkts) / sizeof(writeReqPkts[0]));
            reqPkt = writeReqPkts[variant];
            realSize = sizeof(writeReqPkts[variant]);
        } else if (link->curTask.opcode == SSAP_WRITE_CMD) {
            uint8_t variant = fdp.ConsumeIntegral<uint8_t>() %
                (sizeof(writeCmdPkts) / sizeof(writeCmdPkts[0]));
            reqPkt = writeCmdPkts[variant];
            realSize = sizeof(writeCmdPkts[variant]);
        }

        /* 用 SDF 分配器构造：链路删除时产品侧以 SDF_BuffFree 释放 curTask.buff，
         * 原实现 malloc 与之不匹配（分配器混用） */
        SDF_Buff_S *lastbuff = SDF_BuffNew(realSize);
        if (lastbuff == nullptr) {
            SDF_BuffFree(sdfBuff);
            SSAP_DeleteSsapLinkByAddr(&sleAddr);
            SSAP_LinkDeInit();
            SSAPS_ServiceDeInit();
            return;
        }
        lastbuff->dataOff = 0;
        lastbuff->dataLen = realSize;
        lastbuff->buffLen = realSize;
        memcpy_s(lastbuff->buff, realSize, reqPkt, realSize);
        link->curTask.buff = lastbuff;

        DTAP_Data_Info_S info = {0};
        SSAP_Recv(&info, sdfBuff);

        SDF_BuffFree(sdfBuff);
        SSAP_DeleteSsapLinkByAddr(&sleAddr);

        SSAP_LinkDeInit();
        SSAPS_ServiceDeInit();
    }

    /* ===== SS-M1 manager 深路径（ssap_manager.c，cov_round4 49%）：
     * 分片重组 BEGIN/MID/END/取消/超限/错 opcode（SSAP_ReassemFragment/SSAP_HandleFragRecv）、
     * 分片发送（SSAP_Send/SSAP_SendFragPkt/SSAP_BuildFragBuff）、超时回调 switch
     * （SSAP_TimeoutCbk）、错误响应分发（SSAP_Recv→SSAP_ErrorRspHandle→SSAPC_*ErrorHandle）。
     * 均为 ssap_manager.h 公开接口直调（内部直调铺垫组），sendFunc 用实录桩。 */
    static uint8_t g_mgrOutOpcode = 0;
    static uint32_t g_mgrOutCnt = 0;
    static void SendFuncMgr(SSAP_Link_S *link, SDF_Buff_S *buff, uint8_t opcode)
    {
        (void)link;
        (void)buff;
        g_mgrOutOpcode = opcode;
        g_mgrOutCnt++;
    }
    static uint32_t g_mgrTaskCbCnt = 0;
    static void SsapMgrFuzzTaskCb(int32_t appId, void *arg)
    {
        (void)arg;
        (void)appId;
        g_mgrTaskCbCnt++;
    }
    static uint32_t g_mgrCancelCnt = 0;
    static void SsapMgrFuzzCancelCb(SSAP_Link_S *link)
    {
        (void)link;
        g_mgrCancelCnt++;
    }
    static void SsapMgrFuzzTaskFunc(SSAP_Link_S *link, void *arg)
    {
        (void)link;
        (void)arg;
    }

    static SDF_Buff_S *SsapMgrFuzzBuff(const uint8_t *bytes, uint32_t len)
    {
        SDF_Buff_S *buff = SDF_BuffNew(len);
        if (buff == nullptr) {
            return nullptr;
        }
        uint8_t *buf = SDF_BuffAppend(buff, len);
        if (buf == nullptr || memcpy_s(buf, len, bytes, len) != EOK) {
            SDF_BuffFree(buff);
            return nullptr;
        }
        return buff;
    }

    /* 复用链路铺 curTask：清上一轮残留（SSAP_Recv rsp 路径/超时回调会自清，此处防御），
     * req 报文按 opcode 族给合法最小长度，errHandle/ErrorHandle 才能走满 */
    static void SsapMgrFuzzSetTask(SSAP_Link_S *link, uint8_t opcode, const uint8_t *req, uint32_t reqLen)
    {
        if (link->curTask.buff != nullptr) {
            SDF_BuffFree(link->curTask.buff);
            link->curTask.buff = nullptr;
        }
        if (link->curTask.param != nullptr) {
            SDF_MemFree(link->curTask.param);
            link->curTask.param = nullptr;
        }
        SDF_Buff_S *reqBuff = SsapMgrFuzzBuff(req, reqLen);
        SSAP_TaskParam_S *param = (SSAP_TaskParam_S *)SDF_MemZalloc(sizeof(SSAP_TaskParam_S));
        if (reqBuff == nullptr || param == nullptr) {
            SDF_BuffFree(reqBuff);
            SDF_MemFree(param);
            return;
        }
        param->appId = 0;
        param->timeout = 1000;
        param->valid = true;
        param->appCallback = SsapMgrFuzzTaskCb;
        link->curTask.param = param;
        SSAP_LinkSetTask(link, reqBuff, opcode);
    }

    void FuzzSsapManagerDeep(uint8_t* data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        SSAP_LinkInit();
        SSAPS_ServiceInit();
        AddService1();
        AddService2();

        SLE_Addr_S mgrAddr = {0};
        mgrAddr.type = PUBLIC_ADDRESS;
        mgrAddr.addr[0] = 0x3D;
        const uint16_t mgrLcid = 0x3DF1;
        SSAP_Link_S *link = SSAP_CreateSsapLink(&mgrAddr, mgrLcid, SendFuncMgr);
        if (link == nullptr) {
            SSAP_LinkDeInit();
            SSAPS_ServiceDeInit();
            return;
        }

        /* —— 1. 分片重组异常分支（直接 SSAP_ReassemFragment，输入 buff 归调用方）—— */
        /* WRITE_REQ 分片模板：ctrl = oper<<3 | frag（oper: PART=01 END 时 INSTANT=00；frag: BEGIN=0 MID=1 END=2） */
        const uint8_t wrBegin[7] = {SSAP_WRITE_REQ, 0x08, 0x11, 0x00, 0x00, 0xA1, 0xA2};
        const uint8_t wrMid[4] = {SSAP_WRITE_REQ, 0x09, 0xA3, 0xA4};
        const uint8_t wrEnd[3] = {SSAP_WRITE_REQ, 0x02, 0xA5};
        const uint8_t wrCancel[3] = {SSAP_WRITE_REQ, 0x13, 0x00}; /* oper=CANCEL(0b10)<<3 | NO_FRAG(3) */
        const uint8_t rdRspBegin[8] = {SSAP_READ_RSP, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
        SDF_Buff_S *complete = nullptr;
        /* 1a. MID 无上下文 → DROP（ssap_manager.c:336-339） */
        SDF_Buff_S *frag = SsapMgrFuzzBuff(wrMid, sizeof(wrMid));
        if (frag != nullptr) {
            (void)SSAP_ReassemFragment(link, frag, &complete);
            SDF_BuffFree(frag);
        }
        /* 1b. 双 BEGIN：先建 WRITE_REQ 重组上下文，再来 READ_RSP BEGIN → 丢弃旧缓存分支（:285-290） */
        frag = SsapMgrFuzzBuff(wrBegin, sizeof(wrBegin));
        SDF_Buff_S *frag2 = SsapMgrFuzzBuff(rdRspBegin, sizeof(rdRspBegin));
        if (frag != nullptr && frag2 != nullptr) {
            (void)SSAP_ReassemFragment(link, frag, &complete); /* WRITE_REQ BEGIN 建上下文 */
            (void)SSAP_ReassemFragment(link, frag2, &complete); /* READ_RSP BEGIN 丢弃旧缓存 */
            SDF_BuffFree(frag2);
            /* 1c. 上下文 opcode 不一致（WRITE_REQ MID 对 READ_RSP 上下文）→ DROP（:336-339） */
            SDF_Buff_S *frag3 = SsapMgrFuzzBuff(wrMid, sizeof(wrMid));
            if (frag3 != nullptr) {
                (void)SSAP_ReassemFragment(link, frag3, &complete);
                SDF_BuffFree(frag3);
            }
        }
        SDF_BuffFree(frag);
        /* 1d. 取消写——单包形式（oper=CANCEL、frag=NO_FRAG）经 WriteReqHandle 内
         * SSAPS_HandleWriteCancel→SSAPS_SendWriteCancelRsp（ssaps_server_write.c:224-234）；
         * 顺带清掉 1b/1c 残留的 READ_RSP 重组上下文 */
        frag = SsapMgrFuzzBuff(wrCancel, sizeof(wrCancel));
        if (frag != nullptr) {
            SSAPS_WriteReqHandle(link, frag);
            SDF_BuffFree(frag);
        }
        /* 1d2. 取消写——分片形式（oper=CANCEL 且 frag=MID）→ SSAP_ReassemFragment
         * CANCEL 分支（:371-381）+ SSAP_HandleFragRecv cancelProc 回调（:419-421） */
        const uint8_t wrCancelFrag[3] = {SSAP_WRITE_REQ, 0x11, 0x00};
        frag = SsapMgrFuzzBuff(wrCancelFrag, sizeof(wrCancelFrag));
        if (frag != nullptr) {
            (void)SSAP_HandleFragRecv(link, frag, SSAPS_WriteReqHandle, SsapMgrFuzzCancelCb);
            SDF_BuffFree(frag);
        }
        /* 1e. 单片超 MTU → DROP + 重组上下文清理（:385-394） */
        uint16_t oldMtu = link->mtu;
        link->mtu = 8; /* ponytail: 直改链路 mtu 压低上限触发超限分支，用后恢复 */
        uint8_t oversize[16];
        oversize[0] = SSAP_WRITE_REQ;
        oversize[1] = 0x08;
        for (uint8_t i = 2; i < sizeof(oversize); i++) {
            oversize[i] = i;
        }
        frag = SsapMgrFuzzBuff(oversize, sizeof(oversize));
        if (frag != nullptr) {
            (void)SSAP_ReassemFragment(link, frag, &complete);
            SDF_BuffFree(frag);
        }
        link->mtu = oldMtu;
        /* 1f. WRITE_REQ BEGIN→MID→END 组包闭环（经真实服务端 handler，
         * proc=SSAPS_WriteReqHandle、组包完成→SSAP_HandleRspTaskComplete 清任务续跑队列） */
        frag = SsapMgrFuzzBuff(wrBegin, sizeof(wrBegin));
        if (frag != nullptr) {
            SSAPS_WriteReqHandle(link, frag);
            SDF_BuffFree(frag);
        }
        frag = SsapMgrFuzzBuff(wrMid, sizeof(wrMid));
        if (frag != nullptr) {
            SSAPS_WriteReqHandle(link, frag);
            SDF_BuffFree(frag);
        }
        frag = SsapMgrFuzzBuff(wrEnd, sizeof(wrEnd));
        if (frag != nullptr) {
            SSAPS_WriteReqHandle(link, frag);
            SDF_BuffFree(frag);
        }

        /* —— 2. 超时回调 opcode switch（SSAP_TimeoutCbk:480-503），每轮重铺 curTask（回调内部自清）—— */
        static const uint8_t timeoutOps[] = {
            SSAP_EXCHANGE_INFO_REQ, SSAP_FIND_STRUCTURE_REQ, SSAP_FIND_STRUCTURE_BY_UUID_REQ,
            SSAP_READ_REQ, SSAP_WRITE_REQ, SSAP_READ_BY_UUID_REQ, SSAP_CALL_METHOD_REQ,
            SSAP_VALUE_IND, SSAP_ERROR_RSP, /* default 分支 */
        };
        for (size_t i = 0; i < sizeof(timeoutOps) / sizeof(timeoutOps[0]); i++) {
            uint8_t opcode = timeoutOps[i];
            uint8_t req[8] = {opcode, 0x00, 0x11, 0x00, 0x00, 0x01, 0x02, 0x03};
            uint32_t reqLen = (opcode == SSAP_READ_BY_UUID_REQ) ? 7 : 6; /* ReadByUuidErrorHandle 需 >= sizeof(req PDU)=7 */
            SsapMgrFuzzSetTask(link, opcode, req, reqLen);
            SSAP_TimeoutCbk((void *)link);
        }
        /* 2b. 无效 app 任务（valid=false）→ 清任务续跑分支（:473-479） */
        {
            uint8_t req[6] = {SSAP_READ_REQ, 0x00, 0x11, 0x00, 0x00, 0x01};
            SsapMgrFuzzSetTask(link, SSAP_READ_REQ, req, sizeof(req));
            if (link->curTask.param != nullptr) {
                link->curTask.param->valid = false;
            }
            SSAP_TimeoutCbk((void *)link);
        }

        /* —— 3. 错误响应分发（SSAP_Recv op=ERROR_RSP → ErrorRspHandle switch:750-772）—— */
        DTAP_Data_Info_S info = {0};
        info.lcid = mgrLcid;
        for (size_t i = 0; i < sizeof(timeoutOps) / sizeof(timeoutOps[0]) - 1; i++) {
            uint8_t opcode = timeoutOps[i];
            uint8_t req[8] = {opcode, 0x00, 0x11, 0x00, 0x00, 0x01, 0x02, 0x03};
            uint32_t reqLen = (opcode == SSAP_READ_BY_UUID_REQ) ? 7 :
                ((opcode == SSAP_EXCHANGE_INFO_REQ) ? 6 : 6);
            SsapMgrFuzzSetTask(link, opcode, req, reqLen);
            uint8_t errRsp[6] = {SSAP_ERROR_RSP, 0x00, opcode, 0x11, 0x00, SSAP_ERRCODE_TIMEOUT};
            SDF_Buff_S *rspBuff = SsapMgrFuzzBuff(errRsp, sizeof(errRsp));
            if (rspBuff != nullptr) {
                (void)SSAP_Recv(&info, rspBuff);
                SDF_BuffFree(rspBuff);
            }
        }
        /* 3b. msgCodeReq 与 curTask.opcode 不匹配 → ErrorRspHandle 早退（:748） */
        {
            uint8_t req[6] = {SSAP_READ_REQ, 0x00, 0x11, 0x00, 0x00, 0x01};
            SsapMgrFuzzSetTask(link, SSAP_READ_REQ, req, sizeof(req));
            uint8_t errRsp[6] = {SSAP_ERROR_RSP, 0x00, SSAP_CALL_METHOD_REQ, 0x11, 0x00, 0x01};
            SDF_Buff_S *rspBuff = SsapMgrFuzzBuff(errRsp, sizeof(errRsp));
            if (rspBuff != nullptr) {
                (void)SSAP_Recv(&info, rspBuff);
                SDF_BuffFree(rspBuff);
            }
        }
        /* 3c. SSAP_Recv 守卫：lcid 找不到链路 / 数据长度为 0 */
        {
            DTAP_Data_Info_S badInfo = {0};
            badInfo.lcid = 0x7F7F;
            uint8_t any[4] = {SSAP_ERROR_RSP, 0x00, SSAP_READ_REQ, 0x00};
            SDF_Buff_S *rspBuff = SsapMgrFuzzBuff(any, sizeof(any));
            if (rspBuff != nullptr) {
                (void)SSAP_Recv(&badInfo, rspBuff);
                SDF_BuffFree(rspBuff);
            }
            SDF_Buff_S *emptyBuff = SDF_BuffNew(0);
            if (emptyBuff != nullptr) {
                (void)SSAP_Recv(&info, emptyBuff);
                SDF_BuffFree(emptyBuff);
            }
        }

        /* —— 4. 发送路径（SSAP_Send/SSAP_SendFragPkt/SSAP_PduErrorRsp），buff 所有权移交 —— */
        {
            uint8_t any[4] = {SSAP_READ_REQ, 0x00, 0x11, 0x00};
            SDF_Buff_S *buff = SsapMgrFuzzBuff(any, sizeof(any));
            if (buff != nullptr) {
                SSAP_Send(link, buff, 0x99); /* 非法 opcode → SSAP_Trans_INVALID 释放（:616-620） */
            }
            buff = SsapMgrFuzzBuff(any, sizeof(any));
            if (buff != nullptr) {
                SSAP_Send(link, buff, SSAP_READ_REQ); /* DTAP 通道缺失 → 发送失败释放（:626-629） */
            }
            /* 超长报文 + 对端支持分包 → SSAP_SendFragPkt 分片构造（BuildFragBuff 循环，
             * DTAP 发送失败 → 释放并返回 false :584-598） */
            link->fragCtx.fragment = true;
            const uint32_t bigLen = 300;
            SDF_Buff_S *bigBuff = SDF_BuffNew(bigLen);
            uint8_t *bigBuf = (bigBuff != nullptr) ? SDF_BuffAppend(bigBuff, bigLen) : nullptr;
            if (bigBuf != nullptr) {
                bigBuf[0] = SSAP_WRITE_REQ;
                bigBuf[1] = 0x00;
                for (uint32_t i = 2; i < bigLen; i++) {
                    bigBuf[i] = (uint8_t)i;
                }
                (void)SSAP_SendFragPkt(link, bigBuff, SSAP_WRITE_REQ);
            } else {
                SDF_BuffFree(bigBuff);
            }
            link->fragCtx.fragment = false;
            SSAP_PduErrorRsp(link, SSAP_READ_REQ, 0x11, SSAP_ERRCODE_TIMEOUT);
        }

        /* —— 5. 任务队列（ProcessRequestTask/HighPriority delay 排队，链路删除时统一回收；
         * 两次排队各配独立 arg，避免队列回收时双重释放）—— */
        {
            uint64_t *arg = (uint64_t *)SDF_MemZalloc(sizeof(uint64_t));
            if (arg != nullptr) {
                SSAP_TaskParam_S taskParam = {.appId = 0, .arg = arg, .freeFunc = SDF_MemFree,
                    .func = SsapMgrFuzzTaskFunc, .timeout = 3000, .valid = true, .appCallback = nullptr};
                SSAP_ProcessRequestTask(link, &taskParam, true);
            }
            uint64_t *arg2 = (uint64_t *)SDF_MemZalloc(sizeof(uint64_t));
            if (arg2 != nullptr) {
                SSAP_TaskParam_S taskParam2 = {.appId = 0, .arg = arg2, .freeFunc = SDF_MemFree,
                    .func = SsapMgrFuzzTaskFunc, .timeout = 3000, .valid = true, .appCallback = nullptr};
                SSAP_ProcessHighPriorityRequestTask(link, &taskParam2, true);
            }
            SSAP_ProcessNormalTask(link, SsapMgrFuzzTaskFunc, nullptr, nullptr);
        }
        /* 变异字节仅作扰动记录，不驱动分支（常量重放已覆盖全部目标分支） */
        g_mgrOutOpcode = provider.ConsumeIntegral<uint8_t>();

        SSAP_DeleteSsapLinkByAddr(&mgrAddr);
        SSAP_LinkDeInit();
        SSAPS_ServiceDeInit();
    }
}

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void) argc;
    (void) argv;
    /* 补调度三件套（照 ssapservernew_fuzzer.cpp 先例，幂等）：
     * SS-M1 分片重组依赖 SSAP_StartReassemTimer → CP_TimerAdd → ScheduleTimerAdd，
     * 调度器不活则 BEGIN/MID/END 闭环恒走 timer-fail 丢弃分支 */
    (void)SDF_ThreadInit(10);
    (void)SDF_EvcInit();
    (void)ScheduleEnable();
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    std::vector<uint8_t> fuzzVec = fdp.ConsumeBytes<uint8_t>(size);
    uint8_t *fuzzData = fuzzVec.data();
    size_t fuzzSize = fuzzVec.size();
    OHOS::FuzzDliSsapRecvPkt(fuzzData, fuzzSize);
    OHOS::FuzzSsapManagerDeep(fuzzData, fuzzSize);
    return 0;
}