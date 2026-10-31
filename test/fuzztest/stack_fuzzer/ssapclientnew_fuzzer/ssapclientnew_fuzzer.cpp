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

#include "ssapclientnew_fuzzer.h"
#include "securec.h"
#include "fuzzer/FuzzedDataProvider.h"

#include "ssapc_client.h"
#include "ssapc_client_api.h"
#include "ssap_manager.h"
#include "ssap_link.h"
#include "ssapc_cache.h"
#include "sle_logic_link_mgr.h"
#include "cm_def.h"

#include "nai_log.h"
#include "sdf_mem.h"
#include "sdf_thread.h"
#include "sdf_evc.h"
#include "nlstk_init_api.h"
#include "ssap_type.h"

#define SSAP_FUZZ_IDX1 1
#define SSAP_FUZZ_IDX2 2

typedef struct SlePkt {
    uint8_t *data;
    uint32_t size;
} SlePkt;

int SleSendDliPacket(const SlePkt *packet)
{
    return 0;
}

namespace OHOS {
    void SendCb(SSAP_Link_S *link, SDF_Buff_S *buff, uint8_t opcode)
    {
        (void)link;
        (void)buff;
        (void)opcode;
    }
    void FuzzSsapClientHandle(uint8_t* data, size_t size)
    {
        FuzzedDataProvider fdp(data, size);
        SLE_Addr_S sleAddr = {0};
        sleAddr.type = fdp.ConsumeIntegral<uint8_t>();
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            sleAddr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        uint8_t lcid = fdp.ConsumeIntegral<uint8_t>();
        SSAP_Link_S *link = SSAP_CreateSsapLink(&sleAddr, lcid, SendCb);
        if (link == nullptr) {
            return;
        }
        SDF_Buff_S *sdfBuff = SDF_BuffNewWithReserve(size);
        uint8_t *buf = SDF_BuffAppend(sdfBuff, size);
        if (sdfBuff == nullptr || buf == nullptr) {
            SDF_BuffFree(sdfBuff);
            return;
        }
        for (size_t i = 0; i < size; i++) {
            buf[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        SSAPC_ExchangeInfoRspHandle(link, sdfBuff);
        SSAPC_ReadRspHandle(link, sdfBuff);
        SSAPC_ReadByUuidRspHandle(link, sdfBuff);
        SSAPC_WriteRspHandle(link, sdfBuff);
        SSAPC_ValueNtfHandle(link, sdfBuff);
        SSAPC_ValueIndHandle(link, sdfBuff);

        /* 合法响应模板族（取自产品客户端单测夹具，含边界/错误码变体）：
         * 随机字节几乎全被报文守卫拦截，模板保证进入真实成功解析路径；
         * 每个模板同时喂给全部 Rsp/Ntf/Ind 处理器，覆盖 opcode 交叉分支。 */
        static const uint8_t rspTemplates[][13] = {
            {0x03, 0x33, 0x00, 0x02, 0x01, 0x03},                                    /* exchange rsp */
            {0x01, 0x00, 0x02, 0x00, 0x00, 0x01},                                    /* error rsp */
            {0x09, 0x03, 0x01},                                                      /* read rsp 短 */
            {0x09, 0x0b, 0x06, 0x00},                                                /* read rsp */
            {0x09, 0x07, 0x81, 0x01, 0x81, 0x02},                                    /* read rsp 多项 */
            {0x0b, 0x03, 0xFF, 0x00, 0x01},                                          /* read by uuid rsp */
            {0x0b, 0x07, 0xEE, 0x00, 0x01, 0x80, 0x01, 0xFF, 0x00, 0x02, 0x80,
                0x01, 0x02},                                                          /* read by uuid rsp 多项 */
            {0x0b, 0x0F, 0xEE, 0x00, 0x01, 0x80, 0x01, 0xFF, 0x00, 0x06, 0x00},      /* 长度边界 */
            {0x0e, 0x03, 0x11, 0x00, 0x00, 0x06},                                    /* write rsp */
            {0x0f, 0x03, 0x11, 0x00, 0x01, 0x80, 0x01, 0xFF},                        /* value ntf */
        };
        static const size_t tplLens[] = {6, 6, 3, 4, 6, 5, 13, 11, 6, 8};
        for (size_t t = 0; t < sizeof(rspTemplates) / sizeof(rspTemplates[0]); t++) {
            SDF_Buff_S *tplBuff = SDF_BuffNew(tplLens[t]);
            if (tplBuff == nullptr) {
                continue;
            }
            uint8_t *tplBuf = SDF_BuffAppend(tplBuff, tplLens[t]);
            if (tplBuf != nullptr) {
                (void)memcpy_s(tplBuf, tplLens[t], rspTemplates[t], tplLens[t]);
                SSAPC_ExchangeInfoRspHandle(link, tplBuff);
                SSAPC_ReadRspHandle(link, tplBuff);
                SSAPC_ReadByUuidRspHandle(link, tplBuff);
                SSAPC_WriteRspHandle(link, tplBuff);
                SSAPC_ValueNtfHandle(link, tplBuff);
                SSAPC_ValueIndHandle(link, tplBuff);
            }
            SDF_BuffFree(tplBuff);
        }
        SSAP_DeleteSsapLinkByAddr(&sleAddr);
    }

    void FuzzSsapClientPkt(uint8_t* data, size_t size)
    {
        FuzzedDataProvider fdp(data, size);
        SLE_Addr_S sleAddr = {0};
        sleAddr.addr[0] = SSAP_FUZZ_IDX1;
        uint8_t lcid = fdp.ConsumeIntegral<uint8_t>();
        SSAP_Link_S *link = SSAP_CreateSsapLink(&sleAddr, lcid, SendCb);
        if (link == nullptr) {
            return;
        }
        SDF_Buff_S *sdfBuff = SDF_BuffNewWithReserve(size);
        uint8_t *buf = SDF_BuffAppend(sdfBuff, size);
        if (sdfBuff == nullptr || buf == nullptr) {
            SDF_BuffFree(sdfBuff);
            return;
        }
        for (size_t i = 0; i < size; i++) {
            buf[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        link->curTask.buff = sdfBuff;

        if (size >= sizeof(SSAP_ParamFind_S)) {
            SSAP_ParamFind_S *testData = (SSAP_ParamFind_S *)SDF_MemZalloc(sizeof(SSAP_ParamFind_S));
            testData->appId = fdp.ConsumeIntegral<int32_t>();
            testData->addr.type = fdp.ConsumeIntegral<uint8_t>();
            for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
                testData->addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
            }
            testData->type = static_cast<SSAP_FindType_E>(fdp.ConsumeIntegral<uint8_t>());
            testData->startHandle = fdp.ConsumeIntegral<uint16_t>();
            testData->endHandle = fdp.ConsumeIntegral<uint16_t>();
            SSAPC_FindReq(link, (void*)testData);
            SDF_MemFree(testData);
        }

        if (size >= sizeof(SSAP_WriteCmdInfo_S)) {
            SSAP_WriteCmdInfo_S *testData = (SSAP_WriteCmdInfo_S *)SDF_MemZalloc(sizeof(SSAP_WriteCmdInfo_S) + size);
            testData->addr.type = fdp.ConsumeIntegral<uint8_t>();
            for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
                testData->addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
            }
            testData->handle = fdp.ConsumeIntegral<uint16_t>();
            testData->type = fdp.ConsumeIntegral<uint8_t>();
            testData->value.len = size;
            for (size_t i = 0; i < size; i++) {
                testData->value.value[i] = fdp.ConsumeIntegral<uint8_t>();
            }
            SSAPC_WriteCmd(link, (void*)testData);
            SDF_MemFree(testData);
        }

        if (size >= sizeof(SSAP_WriteReqInfo_S)) {
            SSAP_WriteReqInfo_S *testData = (SSAP_WriteReqInfo_S *)SDF_MemZalloc(sizeof(SSAP_WriteReqInfo_S) + size);
            testData->addr.type = fdp.ConsumeIntegral<uint8_t>();
            for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
                testData->addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
            }
            testData->handle = fdp.ConsumeIntegral<uint16_t>();
            testData->type = fdp.ConsumeIntegral<uint8_t>();
            testData->value.len = size;
            for (size_t i = 0; i < size; i++) {
                testData->value.value[i] = fdp.ConsumeIntegral<uint8_t>();
            }
            SSAPC_WriteReq(link, (void*)testData);
            SDF_MemFree(testData);
        }

        if (size >= sizeof(SSAP_ValueAckInfo_S)) {
            SSAP_ValueAckInfo_S *testData = (SSAP_ValueAckInfo_S *)SDF_MemZalloc(sizeof(SSAP_ValueAckInfo_S) + size);
            testData->addr.type = fdp.ConsumeIntegral<uint8_t>();
            for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
                testData->addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
            }
            testData->value.len = size;
            for (size_t i = 0; i < size; i++) {
                testData->value.value[i] = fdp.ConsumeIntegral<uint8_t>();
            }
            SSAPC_ValueAck(link, (void*)testData);
            SDF_MemFree(testData);
        }

        SSAP_ParamFindByUuid_S *testData = (SSAP_ParamFindByUuid_S *)SDF_MemZalloc(sizeof(SSAP_ParamFindByUuid_S));
        testData->appId = fdp.ConsumeIntegral<int32_t>();
        testData->addr.type = fdp.ConsumeIntegral<uint8_t>();
        for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
            testData->addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        testData->type = static_cast<SSAP_FindType_E>(fdp.ConsumeIntegral<uint8_t>());
        for (uint8_t i = 0; i < SSAP_UUID128_LEN; i++) {
            testData->uuid.uuid[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        testData->startHandle = fdp.ConsumeIntegral<uint16_t>();
        testData->endHandle = fdp.ConsumeIntegral<uint16_t>();
        SSAPC_FindByUuidReq(link, (void*)testData);
        SDF_MemFree(testData);
        SSAP_DeleteSsapLinkByAddr(&sleAddr);
    }

    void FuzzSsapClientPktOther(uint8_t* data, size_t size)
    {
        FuzzedDataProvider fdp(data, size);
        SLE_Addr_S sleAddr = {0};
        sleAddr.addr[0] = SSAP_FUZZ_IDX2;
        uint8_t lcid = fdp.ConsumeIntegral<uint8_t>();
        SSAP_Link_S *link = SSAP_CreateSsapLink(&sleAddr, lcid, SendCb);
        if (link == nullptr) {
            return;
        }
        SDF_Buff_S *sdfBuff = SDF_BuffNewWithReserve(size);
        uint8_t *buf = SDF_BuffAppend(sdfBuff, size);
        if (sdfBuff == nullptr || buf == nullptr) {
            SDF_BuffFree(sdfBuff);
            return;
        }
        for (size_t i = 0; i < size; i++) {
            buf[i] = fdp.ConsumeIntegral<uint8_t>();
        }
        link->curTask.buff = sdfBuff;

        if (size >= sizeof(SSAP_ExchangeInfoReqInfo_S)) {
            SSAP_ExchangeInfoReqInfo_S *testData =
                (SSAP_ExchangeInfoReqInfo_S *)SDF_MemZalloc(sizeof(SSAP_ExchangeInfoReqInfo_S));
            testData->appId = fdp.ConsumeIntegral<int32_t>();
            testData->addr.type = fdp.ConsumeIntegral<uint8_t>();
            for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
                testData->addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
            }
            testData->mtu = fdp.ConsumeIntegral<uint16_t>();
            testData->version = fdp.ConsumeIntegral<uint16_t>();
            SSAPC_ExchangeInfoReq(link, (void*)testData);
            SDF_MemFree(testData);
        }

        if (size >= sizeof(SSAP_ReadReqInfo_S)) {
            SSAP_ReadReqInfo_S *testData = (SSAP_ReadReqInfo_S *)SDF_MemZalloc(sizeof(SSAP_ReadReqInfo_S));
            testData->appId = fdp.ConsumeIntegral<int32_t>();
            testData->addr.type = fdp.ConsumeIntegral<uint8_t>();
            for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
                testData->addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
            }
            testData->handle = fdp.ConsumeIntegral<uint16_t>();
            testData->type = fdp.ConsumeIntegral<uint8_t>();
            SSAPC_ReadReq(link, (void*)testData);
            SDF_MemFree(testData);
        }

        if (size >= sizeof(SSAP_ReadByUuidReqInfo_S)) {
            SSAP_ReadByUuidReqInfo_S *testData =
                (SSAP_ReadByUuidReqInfo_S *)SDF_MemZalloc(sizeof(SSAP_ReadByUuidReqInfo_S));
            testData->appId = fdp.ConsumeIntegral<int32_t>();
            testData->addr.type = fdp.ConsumeIntegral<uint8_t>();
            for (uint8_t i = 0; i < SLE_ADDR_LEN; i++) {
                testData->addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
            }
            for (uint8_t i = 0; i < SSAP_UUID128_LEN; i++) {
                testData->uuid.uuid[i] = fdp.ConsumeIntegral<uint8_t>();
            }
            testData->handleStart = fdp.ConsumeIntegral<uint16_t>();
            testData->handleEnd = fdp.ConsumeIntegral<uint16_t>();
            testData->dataType = fdp.ConsumeIntegral<uint8_t>();
            SSAPC_ReadByUuidReq(link, (void*)testData);
            SDF_MemFree(testData);
        }

        SSAP_DeleteSsapLinkByAddr(&sleAddr);
    }

    /* ===== SS-C1 客户端响应解码深路径（ssapc_client.c 中后段，miss 385 行）：
     * 随机报文被请求匹配守卫拦截，此处为每族 Rsp 处理器铺"合法 curTask 请求报文 + 结构化
     * 响应模板"（内部直调铺垫组）：FIND 解码族（STD/CUS/MIX × 服务/属性/方法/事件/结构、
     * V10 旧设备 ByUuid 族）、READ 单/多值、READ_BY_UUID 控制码位、WRITE 结果三态、
     * CALL_METHOD_RSP、VALUE_NTF 服务变更条目、VALUE_IND 多条目+ACK 回发。 */
    static uint32_t g_cliTaskCbCnt = 0;
    static void SsapCliFuzzTaskCb(int32_t appId, void *arg)
    {
        (void)arg;
        (void)appId;
        g_cliTaskCbCnt++;
    }

    static SDF_Buff_S *SsapCliFuzzBuff(const uint8_t *bytes, uint32_t len)
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

    /* 铺 curTask：请求报文按族给合法布局（FindRsp/ReadRsp/WriteRsp 等经 SSAP_GetLastBuff
     * 反解上一请求，随机字节过不了长度/字段校验）；上一轮残留先释放防泄漏 */
    static void SsapCliFuzzSetTask(SSAP_Link_S *link, uint8_t opcode, const uint8_t *req, uint32_t reqLen)
    {
        if (link->curTask.buff != nullptr) {
            SDF_BuffFree(link->curTask.buff);
            link->curTask.buff = nullptr;
        }
        if (link->curTask.param != nullptr) {
            SDF_MemFree(link->curTask.param);
            link->curTask.param = nullptr;
        }
        SDF_Buff_S *reqBuff = SsapCliFuzzBuff(req, reqLen);
        SSAP_TaskParam_S *param = (SSAP_TaskParam_S *)SDF_MemZalloc(sizeof(SSAP_TaskParam_S));
        if (reqBuff == nullptr || param == nullptr) {
            SDF_BuffFree(reqBuff);
            SDF_MemFree(param);
            return;
        }
        param->appId = 0;
        param->timeout = 1000;
        param->valid = true;
        param->appCallback = SsapCliFuzzTaskCb;
        link->curTask.param = param;
        SSAP_LinkSetTask(link, reqBuff, opcode);
    }

    static void SsapCliFuzzFeed(SSAP_Link_S *link, void (*handler)(SSAP_Link_S *, SDF_Buff_S *),
        const uint8_t *rsp, uint32_t rspLen)
    {
        SDF_Buff_S *rspBuff = SsapCliFuzzBuff(rsp, rspLen);
        if (rspBuff != nullptr) {
            handler(link, rspBuff);
            SDF_BuffFree(rspBuff); /* 非分片路径 handler 不接管 rsp buff，调用方释放 */
        }
    }

    /* shaping 构造器填充 helper：向 rsp 追加 n 个 FDP 域字节（容量由调用方预检） */
    static void SsapCliFuzzFill(FuzzedDataProvider &provider, uint8_t *rsp, uint32_t *rspLen, uint32_t n)
    {
        for (uint32_t i = 0; i < n; i++) {
            rsp[(*rspLen)++] = provider.ConsumeIntegral<uint8_t>();
        }
    }

    void FuzzSsapClientRspDeep(uint8_t* data, size_t size)
    {
        if (data == nullptr || size < SLE_ADDR_LEN) {
            return;
        }
        FuzzedDataProvider provider(data, size);
        SLE_Addr_S cliAddr = {0};
        memcpy_s(cliAddr.addr, SLE_ADDR_LEN, data, SLE_ADDR_LEN);
        cliAddr.addr[5] = (uint8_t)(cliAddr.addr[5] ^ 0xC3); /* 与既有三段用例地址空间错开 */
        SleLogicLinkInit();
        /* 设备档铺底：OLD/NEW 由 FDP 选 —— companyId=HISI(0x007C) 且 devType=UNKNOWN 时
         * CM 判定为 OLD（cm_signaling_version.c:67-87）→ FIND BY_UUID 走 V10 解码族 */
        SleLogicLink_S *devNode = SleLogicLinkAdd(&cliAddr);
        if (devNode == nullptr) {
            return;
        }
        const uint16_t cliLcid = 0x5C1D;
        devNode->lcid = cliLcid;
        bool oldDev = (provider.ConsumeIntegral<uint8_t>() & 0x01) != 0;
        devNode->companyId = oldDev ? 0x007C : 0x0000;
        devNode->devType = oldDev ? CM_DEVTYPE_UNKNOWN : CM_DEVTYPE_NEW;
        SSAP_Link_S *link = SSAP_CreateSsapLink(&cliAddr, cliLcid, SendCb);
        if (link == nullptr) {
            SleLogicLinkRemove(devNode);
            return;
        }
        (void)SsapcCacheCreate(&cliAddr); /* SsapcCacheServ 族解码入缓存的前置条目 */

        /* —— 响应族域表主驱动（取代原 30+ 常量模板全量重放，模板语义下沉 corpus seed_cli_*，
         * req+rsp 成对）：每轮 FDP 选一族，铺对应合法 curTask 请求 + shaping 构造器生成
         * 响应——opcode 取族内合法值，ctrl（itemType/错误位/多实例位）、条目数、长度域、
         * 条目内容字节全 FDP；rsp[80] 栈缓冲由构造器按剩余容量硬顶；截断档（声明长度超
         * 实际）保留为域表档位 —— */
        static const struct {
            uint8_t reqOp;
            uint8_t rspOp;
            void (*handler)(SSAP_Link_S *, SDF_Buff_S *);
        } famTable[] = {
            {SSAP_FIND_STRUCTURE_REQ, SSAP_FIND_STRUCTURE_RSP, SSAPC_FindRspHandle},                 /* 0 */
            {SSAP_FIND_STRUCTURE_BY_UUID_REQ, SSAP_FIND_STRUCTURE_BY_UUID_RSP, SSAPC_FindRspHandle}, /* 1: OLD 档 V10 */
            {SSAP_READ_REQ, SSAP_READ_RSP, SSAPC_ReadRspHandle},                                    /* 2 */
            {SSAP_READ_BY_UUID_REQ, SSAP_READ_BY_UUID_RSP, SSAPC_ReadByUuidRspHandle},              /* 3 */
            {SSAP_WRITE_REQ, SSAP_WRITE_RSP, SSAPC_WriteRspHandle},                                 /* 4 */
            {SSAP_CALL_METHOD_REQ, SSAP_CALL_METHOD_RSP, SSAPC_CallMethodRspHandle},                /* 5 */
            {SSAP_VALUE_NTF, SSAP_VALUE_NTF, SSAPC_ValueNtfHandle},                                 /* 6: 无 curTask 依赖 */
            {SSAP_VALUE_IND, SSAP_VALUE_IND, SSAPC_ValueIndHandle},                                 /* 7: → ACK 回发 */
        };
        uint8_t famIdx = provider.ConsumeIntegral<uint8_t>() % (sizeof(famTable) / sizeof(famTable[0]));
        uint8_t req[7 + SSAP_UUID128_LEN]; /* BY_UUID req：头 6 + uuid16；RBU req：头 7 + uuid16 */
        uint8_t rsp[80];
        uint32_t rspLen = 0;
        uint32_t reqLen = 0;
        (void)memset_s(req, sizeof(req), 0, sizeof(req));
        req[0] = famTable[famIdx].reqOp;
        rsp[0] = famTable[famIdx].rspOp;
        switch (famIdx) {
            case 0:
            case 1: {
                /* FIND/BY_UUID 族（SSAPC_FindRspDecode:763-810）：请求 ctrl=findType(bit0-2)、
                 * 响应 ctrl = 0x03(NO_FRAG) | itemType<<2；V10(OLD) 布局条目无 uuid（回填请求 uuid） */
                bool v10 = (famIdx == 1) && oldDev;
                uint8_t findType = provider.ConsumeIntegral<uint8_t>() % 6;
                uint8_t itemType = provider.ConsumeIntegral<uint8_t>() % 3;
                req[1] = findType;
                req[2] = provider.ConsumeIntegral<uint8_t>() & 0x01; /* startHandle=1：清缓存+SetCurFindByUuid(:730-738) */
                req[4] = 0xFF;
                req[5] = 0xFF;
                reqLen = 6;
                if (famIdx == 1) {
                    for (int i = 0; i < SSAP_UUID128_LEN; i++) {
                        req[6 + i] = provider.ConsumeIntegral<uint8_t>(); /* 请求侧 uuid（GetFindReqUuid） */
                    }
                    reqLen = 6 + SSAP_UUID128_LEN;
                }
                rsp[1] = (uint8_t)(0x03 | (itemType << 2));
                rspLen = 2;
                bool isSvc = findType == FIND_STRUCTURE_TYPE_PRIMARY_SERVICE;
                if (findType == FIND_STRUCTURE_TYPE_SERVICE_STRUCTURE) {
                    /* 结构发现 MIX（DecodeMixStructure:639-686）：成员自描述布局 =
                     * handle2 + itemType1 + uuid(2/16 按 STD/VENDOR 类) + [oper4] + descCount1 + descs */
                    static const uint8_t memTypes[] = {ITEM_TYPE_STD_PRIMARY_SERVICE, ITEM_TYPE_STD_PROPERTY,
                        ITEM_TYPE_STD_METHOD, ITEM_TYPE_STD_EVENT, ITEM_TYPE_STD_DESCRIPTOR,
                        ITEM_TYPE_VENDOR_PRIMARY_SERVICE};
                    uint8_t memNum = provider.ConsumeIntegral<uint8_t>() % 5;
                    for (uint8_t m = 0; m < memNum; m++) {
                        uint8_t memType = memTypes[provider.ConsumeIntegral<uint8_t>() % 6];
                        uint32_t uuidLen = (memType >= ITEM_TYPE_VENDOR_PRIMARY_SERVICE) ? SSAP_UUID128_LEN :
                            SSAP_UUID16_LEN;
                        uint32_t operLen = (memType == ITEM_TYPE_STD_PRIMARY_SERVICE ||
                            memType == ITEM_TYPE_VENDOR_PRIMARY_SERVICE ||
                            memType == ITEM_TYPE_STD_DESCRIPTOR) ? 0 : 4;
                        if (rspLen + 3 + uuidLen + operLen + 2 > sizeof(rsp)) {
                            break;
                        }
                        SsapCliFuzzFill(provider, rsp, &rspLen, 2); /* handle2 */
                        rsp[rspLen++] = memType;
                        SsapCliFuzzFill(provider, rsp, &rspLen, uuidLen);
                        SsapCliFuzzFill(provider, rsp, &rspLen, operLen);
                        uint8_t descCount = provider.ConsumeIntegral<uint8_t>() % 2;
                        if (rspLen + 1 + descCount > sizeof(rsp)) {
                            break;
                        }
                        rsp[rspLen++] = descCount;
                        SsapCliFuzzFill(provider, rsp, &rspLen, descCount);
                    }
                    if ((provider.ConsumeIntegral<uint8_t>() & 0x01) != 0 && rspLen < sizeof(rsp)) {
                        rsp[rspLen++] = 0xEE; /* 残余 1 字节：extra data 分支（:682-685） */
                    }
                } else if (itemType == FIND_ITEM_TYPE_MIX) {
                    /* MIX 指示头（count:7|type:1）+ 条目（svc: handle2+end2+[uuid]+member1；
                     * character: handle2+[uuid]+oper4+descCount1+descs） */
                    uint8_t groupNum = provider.ConsumeIntegral<uint8_t>() % 3;
                    for (uint8_t g = 0; g < groupNum && rspLen + 2 < sizeof(rsp); g++) {
                        bool cus = (provider.ConsumeIntegral<uint8_t>() & 0x01) != 0;
                        uint8_t cnt = provider.ConsumeIntegral<uint8_t>() % 2;
                        uint32_t uuidLen = v10 ? 0 : (cus ? SSAP_UUID128_LEN : SSAP_UUID16_LEN);
                        rsp[rspLen++] = (uint8_t)((cus ? 0x80 : 0x00) | cnt);
                        for (uint8_t c = 0; c < cnt; c++) {
                            uint32_t entryLen = isSvc ? (5 + uuidLen) : (7 + uuidLen);
                            if (rspLen + entryLen + 1 > sizeof(rsp)) {
                                break;
                            }
                            SsapCliFuzzFill(provider, rsp, &rspLen, 2); /* handle2 */
                            if (isSvc) {
                                SsapCliFuzzFill(provider, rsp, &rspLen, 2); /* endHandle2 */
                            }
                            SsapCliFuzzFill(provider, rsp, &rspLen, uuidLen);
                            if (isSvc) {
                                rsp[rspLen++] = provider.ConsumeIntegral<uint8_t>(); /* memberValue1 */
                            } else {
                                SsapCliFuzzFill(provider, rsp, &rspLen, 4); /* oper4 */
                                uint8_t descCount = provider.ConsumeIntegral<uint8_t>() % 2;
                                if (rspLen + 1 + descCount > sizeof(rsp)) {
                                    break;
                                }
                                rsp[rspLen++] = descCount;
                                SsapCliFuzzFill(provider, rsp, &rspLen, descCount);
                            }
                        }
                    }
                } else {
                    /* STD/CUS 单型条目 × 条目数 FDP（%3），域字节全 FDP */
                    uint32_t uuidLen = v10 ? 0 :
                        ((itemType == FIND_ITEM_TYPE_CUSTOMIZE) ? SSAP_UUID128_LEN : SSAP_UUID16_LEN);
                    uint8_t entryNum = provider.ConsumeIntegral<uint8_t>() % 3;
                    for (uint8_t e = 0; e < entryNum; e++) {
                        uint32_t entryLen = isSvc ? (5 + uuidLen) : (7 + uuidLen);
                        if (rspLen + entryLen + 1 > sizeof(rsp)) {
                            break;
                        }
                        SsapCliFuzzFill(provider, rsp, &rspLen, 2); /* handle2 */
                        if (isSvc) {
                            SsapCliFuzzFill(provider, rsp, &rspLen, 2); /* endHandle2 */
                        }
                        SsapCliFuzzFill(provider, rsp, &rspLen, uuidLen);
                        if (isSvc) {
                            rsp[rspLen++] = provider.ConsumeIntegral<uint8_t>(); /* memberValue1 */
                        } else {
                            SsapCliFuzzFill(provider, rsp, &rspLen, 4); /* oper4 */
                            uint8_t descCount = provider.ConsumeIntegral<uint8_t>() % 2;
                            if (rspLen + 1 + descCount > sizeof(rsp)) {
                                break;
                            }
                            rsp[rspLen++] = descCount;
                            SsapCliFuzzFill(provider, rsp, &rspLen, descCount);
                        }
                    }
                }
                break;
            }

            case 2: {
                /* READ_RSP 单/多值（SSAPC_ReadRspHandle:985-1011）：req 条目数 FDP（1..3，
                 * CountReqHandleNum>1 → MultiReadRspDecode），rsp ctrl 错误位 FDP；
                 * 多值条目头 length:15|success:1（LE：成功=0x80|lenHi,lenLo，失败=errCode） */
                req[1] = 0x03;
                uint8_t itemNum = (uint8_t)(provider.ConsumeIntegral<uint8_t>() % 3 + 1);
                for (uint8_t i = 0; i < itemNum; i++) { /* req 条目 = handle2 + type1（自偏移 2 起） */
                    SsapCliFuzzFill(provider, req + 2 + 3 * i, &reqLen, 3);
                }
                reqLen += 2;
                rsp[1] = ((provider.ConsumeIntegral<uint8_t>() & 0x01) != 0) ? 0x0B : 0x03;
                rspLen = 2;
                if (itemNum == 1 && (rsp[1] & 0x08) == 0) {
                    /* 单值成功：items 为裸 value（:828-840） */
                    SsapCliFuzzFill(provider, rsp, &rspLen, provider.ConsumeIntegral<uint8_t>() % 8);
                } else if (itemNum == 1) {
                    /* 单值错误：items = {length2(errCode)}（:841-856） */
                    SsapCliFuzzFill(provider, rsp, &rspLen, 2);
                } else {
                    /* 多值：逐项 success 位 FDP；单项错误+多句柄 → FillErrorItems 补齐（:915-941） */
                    for (uint8_t i = 0; i < itemNum && rspLen + 3 < sizeof(rsp); i++) {
                        bool ok = (provider.ConsumeIntegral<uint8_t>() & 0x01) != 0;
                        uint8_t valLen = provider.ConsumeIntegral<uint8_t>() % 4;
                        if (rspLen + 2 + valLen > sizeof(rsp)) {
                            break;
                        }
                        if (ok) { /* length:15=值长 | success:1=1（LE 高位在第二字节） */
                            rsp[rspLen++] = valLen;
                            rsp[rspLen++] = (uint8_t)(0x80 | (valLen >> 8));
                            SsapCliFuzzFill(provider, rsp, &rspLen, valLen);
                        } else { /* success=0：length 字段即错误码 */
                            rsp[rspLen++] = provider.ConsumeIntegral<uint8_t>() % 20;
                            rsp[rspLen++] = 0x00;
                        }
                    }
                }
                break;
            }
            case 3: {
                /* READ_BY_UUID_RSP 控制码位（:1013-1062）：ctrl 档 FDP 选——单实例(0x03)/
                 * 多实例位(bit2)/错误指示位(bit3)；条目 = handle2 + [指示2] + value */
                req[1] = SSAP_READ_BY_CUSTOM_UUID_CONTROL;
                req[2] = 0x01;
                req[3] = 0x00;
                req[4] = 0xFF;
                req[5] = 0xFF;
                req[6] = provider.ConsumeIntegral<uint8_t>(); /* dataType */
                SsapCliFuzzFill(provider, req + 7, &reqLen, SSAP_UUID128_LEN); /* 请求 uuid16 */
                reqLen += 7;
                uint8_t ctrlTier = provider.ConsumeIntegral<uint8_t>() % 3;
                rsp[1] = (uint8_t)(0x03 | (ctrlTier << 2));
                rspLen = 2;
                uint8_t itemNum = (uint8_t)(provider.ConsumeIntegral<uint8_t>() % 2 + 1);
                for (uint8_t i = 0; i < itemNum && rspLen + 6 < sizeof(rsp); i++) {
                    SsapCliFuzzFill(provider, rsp, &rspLen, 2); /* handle2 */
                    if (ctrlTier != 0) { /* 多实例/错误指示档：指示字 2B（bit15=错误标志/携带值） */
                        bool withVal = (provider.ConsumeIntegral<uint8_t>() & 0x01) != 0;
                        uint8_t valLen = provider.ConsumeIntegral<uint8_t>() % 4;
                        if (rspLen + 2 + valLen > sizeof(rsp)) {
                            break;
                        }
                        rsp[rspLen++] = withVal ? valLen : (uint8_t)(provider.ConsumeIntegral<uint8_t>() % 20);
                        rsp[rspLen++] = withVal ? 0x80 : 0x00; /* bit15：0=错误码 1=值长 */
                        SsapCliFuzzFill(provider, rsp, &rspLen, withVal ? valLen : 0);
                    } else { /* 单实例：{handle, value} 裸值 */
                        SsapCliFuzzFill(provider, rsp, &rspLen, provider.ConsumeIntegral<uint8_t>() % 4);
                    }
                }
                break;
            }
            case 4: {
                /* WRITE_RSP 结果三态（SSAPC_WriteRspHandle:1165-1213）：请求 verify(bit5)=0x20
                 * FDP 选；响应 ctrl = 0x03 | result<<2，result 域 FDP（SUCCESS/PART/CANCEL/RFU） */
                bool verify = (provider.ConsumeIntegral<uint8_t>() & 0x01) != 0;
                req[1] = (uint8_t)(0x03 | (verify ? 0x20 : 0x00));
                SsapCliFuzzFill(provider, req + 2, &reqLen, 5); /* handle2 + type1 + value len2 */
                reqLen += 2;
                uint8_t result = provider.ConsumeIntegral<uint8_t>() % 4;
                rsp[1] = (uint8_t)(0x03 | (result << 2));
                rspLen = 2;
                if (result == 0 && verify) {
                    /* 成功 + 原值返回：items = {handle2, type1, value}（:1181-1190） */
                    SsapCliFuzzFill(provider, rsp, &rspLen, 3 + (provider.ConsumeIntegral<uint8_t>() % 4));
                } else if (result == 0) {
                    /* 成功不带回值：len==2 恰好过 :1199 校验；FDP 位加 1 字节探测
                     * verify=0 且 len>2 的互斥守卫分支（疑似死码，原 wrOkPlain 模板语义） */
                    if ((provider.ConsumeIntegral<uint8_t>() & 0x01) != 0) {
                        SsapCliFuzzFill(provider, rsp, &rspLen, 1);
                    }
                } else {
                    /* PART/CANCEL：错误项 {errorNum1, {handle2, errCode1}×n}（HandleWriteRspHandleError） */
                    uint8_t errNum = provider.ConsumeIntegral<uint8_t>() % 2;
                    if (rspLen + 1 + errNum * 3 <= sizeof(rsp)) {
                        rsp[rspLen++] = errNum;
                        for (uint8_t i = 0; i < errNum; i++) {
                            SsapCliFuzzFill(provider, rsp, &rspLen, 3);
                        }
                    }
                }
                break;
            }
            case 5: {
                /* CALL_METHOD_RSP（:1401-1428）：req = handle2 + 参数值，rsp = 裸返回值 */
                req[1] = 0x03;
                SsapCliFuzzFill(provider, req + 2, &reqLen, 2 + (provider.ConsumeIntegral<uint8_t>() % 4));
                reqLen += 2;
                rsp[1] = 0x03;
                rspLen = 2;
                SsapCliFuzzFill(provider, rsp, &rspLen, provider.ConsumeIntegral<uint8_t>() % 8);
                break;
            }
            case 6:
            case 7: {
                /* VALUE_NTF/IND 多条目（:1267-1302/:1378-1396 → IND 触发 SendAck 回发）：
                 * 条目 = handle2 + len2 + value；服务变更条目（handle=0x000E 且新设备）与
                 * 声明长度越界中断档（:1289-1292）由 FDP 域字节/截断档自然覆盖 */
                rsp[1] = 0x03;
                rspLen = 2;
                bool trunc = (provider.ConsumeIntegral<uint8_t>() & 0x01) != 0; /* 截断档 */
                uint8_t itemNum = (uint8_t)(provider.ConsumeIntegral<uint8_t>() % 3);
                for (uint8_t i = 0; i < itemNum && rspLen + 4 < sizeof(rsp); i++) {
                    SsapCliFuzzFill(provider, rsp, &rspLen, 2); /* handle2（0x0E00=服务变更） */
                    uint8_t valLen = provider.ConsumeIntegral<uint8_t>() % 5; /* 含 4B 服务变更句柄域档 */
                    if (rspLen + 2 + valLen > sizeof(rsp)) {
                        break;
                    }
                    if (trunc && i == itemNum - 1) {
                        rsp[rspLen++] = 0x40; /* 声明 0x40 实际不足 → 条目越界中断分支 */
                        rsp[rspLen++] = 0x00;
                        rsp[rspLen++] = provider.ConsumeIntegral<uint8_t>();
                        break;
                    }
                    rsp[rspLen++] = valLen; /* len2 LE = 实际值长 */
                    rsp[rspLen++] = 0x00;
                    SsapCliFuzzFill(provider, rsp, &rspLen, valLen);
                }
                break;
            }
            default:
                break;
        }
        if (famIdx < 6) { /* NTF/IND 无 curTask 依赖，仅 0-5 族铺请求 */
            SsapCliFuzzSetTask(link, famTable[famIdx].reqOp, req, reqLen);
        }
        SsapCliFuzzFeed(link, famTable[famIdx].handler, rsp, rspLen);

        /* —— 9. client_api 优先级/批量族（ssapc_client_api.c 真实公开入口，miss 124 行）——
         * 直调 handler 不清任务，先清 curTask 使链路回 IDLE，ProcessRequestTask 才能内联执行 */
        SSAP_LinkClearCurrentTask(link);
        {
            SSAP_ParamFind_S findParam = {0};
            findParam.appId = 0;
            (void)memcpy_s(&findParam.addr, sizeof(SLE_Addr_S), &cliAddr, sizeof(SLE_Addr_S));
            findParam.type = FIND_STRUCTURE_TYPE_PRIMARY_SERVICE;
            findParam.startHandle = 1;
            findParam.endHandle = 0xFFFF;
            SSAP_FindPriorityReq(0, 1000, SsapCliFuzzTaskCb, &findParam);
            SSAP_FindReq(0, 1000, SsapCliFuzzTaskCb, &findParam);

            SSAP_ParamFindByUuid_S findByUuid = {0};
            findByUuid.appId = 0;
            (void)memcpy_s(&findByUuid.addr, sizeof(SLE_Addr_S), &cliAddr, sizeof(SLE_Addr_S));
            findByUuid.type = FIND_STRUCTURE_TYPE_PROPERTY;
            findByUuid.startHandle = 1;
            findByUuid.endHandle = 0xFFFF;
            SSAP_FindByUuidPriorityReq(0, 1000, SsapCliFuzzTaskCb, &findByUuid);
            SSAP_FindByUuidReq(0, 1000, SsapCliFuzzTaskCb, &findByUuid);

            /* handles/value 为柔性数组，须按长度堆配（栈对象会越界写） */
            SSAP_ReadPropsInfo_S *readProps = (SSAP_ReadPropsInfo_S *)SDF_MemZalloc(
                sizeof(SSAP_ReadPropsInfo_S) + 2 * sizeof(uint16_t));
            if (readProps != nullptr) {
                readProps->appId = 0;
                (void)memcpy_s(&readProps->addr, sizeof(SLE_Addr_S), &cliAddr, sizeof(SLE_Addr_S));
                readProps->num = 2;
                readProps->handles[0] = 0x11;
                readProps->handles[1] = 0x12;
                SSAP_ReadReqProps(0, 1000, SsapCliFuzzTaskCb, readProps);
                SDF_MemFree(readProps);
            }

            SSAP_CallMethodReqInfo_S *callReq = (SSAP_CallMethodReqInfo_S *)SDF_MemZalloc(
                sizeof(SSAP_CallMethodReqInfo_S) + 2);
            if (callReq != nullptr) {
                callReq->handle = 0x12;
                (void)memcpy_s(&callReq->addr, sizeof(SLE_Addr_S), &cliAddr, sizeof(SLE_Addr_S));
                callReq->value.len = 2;
                callReq->value.value[0] = 0x01;
                callReq->value.value[1] = 0x02;
                SSAP_CallMethodReq(0, 1000, SsapCliFuzzTaskCb, callReq);
                SDF_MemFree(callReq);
            }

            SSAP_CallMethodCmdInfo_S *callCmd = (SSAP_CallMethodCmdInfo_S *)SDF_MemZalloc(
                sizeof(SSAP_CallMethodCmdInfo_S) + 2);
            if (callCmd != nullptr) {
                callCmd->handle = 0x12;
                (void)memcpy_s(&callCmd->addr, sizeof(SLE_Addr_S), &cliAddr, sizeof(SLE_Addr_S));
                callCmd->value.len = 2;
                callCmd->value.value[0] = 0x03;
                callCmd->value.value[1] = 0x04;
                SSAP_CallMethodCmd(callCmd);
                SDF_MemFree(callCmd);
            }
        }
        SsapCliFuzzSetTask(link, 0, nullptr, 0); /* 清 curTask 归还资源 */
        SsapcCacheDestroy(&cliAddr);
        SSAP_DeleteSsapLinkByAddr(&cliAddr);
        SleLogicLinkRemove(devNode);
    }

    void FuzzSsapClientApi(uint8_t* data, size_t size)
    {
        if (data == nullptr || size < 1) {
            return;
        }
        SSAP_LinkInit();
        FuzzSsapClientHandle(data, size);
        FuzzSsapClientPkt(data, size);
        FuzzSsapClientPktOther(data, size);
        FuzzSsapClientRspDeep(data, size);
        SSAP_LinkDeInit();
    }
}

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void) argc;
    (void) argv;
    uint32_t ret;
    int res;

    ret = NLSTK_InitStack();
    if (ret != 0) {
        NAI_LOG_ERROR("NAI_InitStack failed, ret:%{public}d", ret);
        return 1;
    }
    NAI_LOG_INFO("NAI_InitStack success");
    res = NLSTK_EnableStack();
    if (res != 0) {
        NAI_LOG_ERROR("NLSTK_EnableStack failed, ret:%{public}d", res);
        return 1;
    }
    NAI_LOG_INFO("NLSTK_EnableStack success");
    /* staking builds starburst connection */
    NAI_LOG_INFO("ssapclientnew_fuzzer init success");
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    std::vector<uint8_t> fuzzVec = fdp.ConsumeBytes<uint8_t>(size);
    uint8_t *fuzzData = fuzzVec.data();
    size_t fuzzSize = fuzzVec.size();
    OHOS::FuzzSsapClientApi(fuzzData, fuzzSize);
    return 0;
}