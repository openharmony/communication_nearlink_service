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
        /* 设备类型铺底：node 存在且 devType=NEW → 新设备解码族；companyId=HISI(0x007C)
         * 且 devType=UNKNOWN → CM 判定为 OLD（cm_signaling_version.c:67-87）→ V10 解码族 */
        SleLogicLink_S *devNode = SleLogicLinkAdd(&cliAddr);
        if (devNode == nullptr) {
            return;
        }
        const uint16_t cliLcid = 0x5C1D;
        devNode->lcid = cliLcid;
        devNode->devType = CM_DEVTYPE_NEW;
        SSAP_Link_S *link = SSAP_CreateSsapLink(&cliAddr, cliLcid, SendCb);
        if (link == nullptr) {
            SleLogicLinkRemove(devNode);
            return;
        }
        (void)SsapcCacheCreate(&cliAddr); /* SsapcCacheServ 族解码入缓存的前置条目 */

        /* —— 1. FIND_RSP 解码族（SSAPC_FindRspDecode:763-810，设备 NEW）——
         * 请求 ctrl = findType（bit0-2）；响应 ctrl = 0x03(NO_FRAG) | itemType<<2 */
        const uint8_t svcStd[7] = {0x11, 0x00, 0x12, 0x00, 0x28, 0x19, 0x03};
        const uint8_t svcCus[21] = {0x11, 0x00, 0x12, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
            0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x03};
        const uint8_t propStd[11] = {0x11, 0x00, 0x28, 0x19, 0x01, 0x00, 0x00, 0x00, 0x02, 0x01, 0x02};
        const uint8_t propCus[25] = {0x11, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
            0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x01, 0x00, 0x00, 0x00, 0x02, 0x01, 0x02};
        const uint8_t mixIndStd = 0x01;  /* indicator: type=STD(0) count=1 */
        const uint8_t mixIndCus = 0x81;  /* indicator: type=CUS(1) count=1 */
        /* 结构发现条目：服务/描述符无 oper（GetOperationLenByItemType=0，ssap_utils.c:166-178），
         * 属性/方法/事件带 oper4；布局 = handle2 + itemType1 + uuid2 + [oper4] + descCount1 + descs */
        const uint8_t struSvc[6] = {0x10, 0x00, ITEM_TYPE_STD_PRIMARY_SERVICE, 0x28, 0x19, 0x00};
        const uint8_t struProp[11] = {0x11, 0x00, ITEM_TYPE_STD_PROPERTY, 0x28, 0x19, 0x01, 0x00, 0x00,
            0x00, 0x01, 0x01};
        const uint8_t struMethod[10] = {0x12, 0x00, ITEM_TYPE_STD_METHOD, 0x28, 0x19, 0x02, 0x00, 0x00,
            0x00, 0x00};
        const uint8_t struEvent[10] = {0x13, 0x00, ITEM_TYPE_STD_EVENT, 0x28, 0x19, 0x04, 0x00, 0x00,
            0x00, 0x00};
        const uint8_t struDesc[6] = {0x14, 0x00, ITEM_TYPE_STD_DESCRIPTOR, 0x28, 0x19, 0x00};

        uint8_t req[32];
        uint8_t rsp[80];
        /* 1a. findType=PRIMARY_SERVICE × itemType 三态（DecodePrimaryService:309-320） */
        for (uint8_t itemType = 0; itemType < 3; itemType++) {
            (void)memset_s(req, sizeof(req), 0, sizeof(req));
            req[0] = SSAP_FIND_STRUCTURE_REQ;
            req[1] = FIND_STRUCTURE_TYPE_PRIMARY_SERVICE;
            req[2] = 0x01; /* startHandle=1：触发 FindRspCleanCache 清服务缓存+SetCurFindByUuid（:730-738） */
            req[4] = 0xFF;
            req[5] = 0xFF;
            SsapCliFuzzSetTask(link, SSAP_FIND_STRUCTURE_REQ, req, 6);
            uint32_t rspLen = 2;
            rsp[0] = SSAP_FIND_STRUCTURE_RSP;
            rsp[1] = (uint8_t)(0x03 | (itemType << 2));
            if (itemType == 0) { /* STD：两条标准服务条目 */
                (void)memcpy_s(rsp + 2, sizeof(svcStd), svcStd, sizeof(svcStd));
                (void)memcpy_s(rsp + 2 + sizeof(svcStd), sizeof(svcStd), svcStd, sizeof(svcStd));
                rspLen = 2 + 2 * sizeof(svcStd);
            } else if (itemType == 1) { /* CUS */
                (void)memcpy_s(rsp + 2, sizeof(svcCus), svcCus, sizeof(svcCus));
                rspLen = 2 + sizeof(svcCus);
            } else { /* MIX：指示头+条目 × STD/CUS 各一 */
                rsp[2] = mixIndStd;
                (void)memcpy_s(rsp + 3, sizeof(svcStd), svcStd, sizeof(svcStd));
                rsp[3 + sizeof(svcStd)] = mixIndCus;
                (void)memcpy_s(rsp + 4 + sizeof(svcStd), sizeof(svcCus), svcCus, sizeof(svcCus));
                rspLen = 4 + sizeof(svcStd) + sizeof(svcCus);
            }
            SsapCliFuzzFeed(link, SSAPC_FindRspHandle, rsp, rspLen);
        }
        /* 1b. findType=PROPERTY/METHOD/EVENT（DecodeProperty + 成员类型分派 :495-519） */
        const uint8_t memberFindTypes[] = {FIND_STRUCTURE_TYPE_PROPERTY, FIND_STRUCTURE_TYPE_METHOD,
            FIND_STRUCTURE_TYPE_EVENT};
        for (size_t f = 0; f < sizeof(memberFindTypes) / sizeof(memberFindTypes[0]); f++) {
            (void)memset_s(req, sizeof(req), 0, sizeof(req));
            req[0] = SSAP_FIND_STRUCTURE_REQ;
            req[1] = memberFindTypes[f];
            req[4] = 0xFF;
            req[5] = 0xFF;
            SsapCliFuzzSetTask(link, SSAP_FIND_STRUCTURE_REQ, req, 6);
            /* STD 属性条目（带 2 描述符） */
            rsp[0] = SSAP_FIND_STRUCTURE_RSP;
            rsp[1] = 0x03; /* itemType=STANDARD */
            (void)memcpy_s(rsp + 2, sizeof(propStd), propStd, sizeof(propStd));
            SsapCliFuzzFeed(link, SSAPC_FindRspHandle, rsp, 2 + sizeof(propStd));
            /* MIX 属性条目 */
            rsp[1] = (uint8_t)(0x03 | (FIND_ITEM_TYPE_MIX << 2));
            rsp[2] = mixIndStd;
            (void)memcpy_s(rsp + 3, sizeof(propStd), propStd, sizeof(propStd));
            SsapCliFuzzFeed(link, SSAPC_FindRspHandle, rsp, 3 + sizeof(propStd));
            /* CUS 属性条目 */
            rsp[1] = (uint8_t)(0x03 | (FIND_ITEM_TYPE_CUSTOMIZE << 2));
            (void)memcpy_s(rsp + 2, sizeof(propCus), propCus, sizeof(propCus));
            SsapCliFuzzFeed(link, SSAPC_FindRspHandle, rsp, 2 + sizeof(propCus));
        }
        /* 1c. findType=SERVICE_STRUCTURE × MIX（DecodeMixStructure:639-686：
         * 服务声明/属性/方法/事件/描述符（default 分支）成员 + 尾部排序更新 + 残余数据分支） */
        (void)memset_s(req, sizeof(req), 0, sizeof(req));
        req[0] = SSAP_FIND_STRUCTURE_REQ;
        req[1] = FIND_STRUCTURE_TYPE_SERVICE_STRUCTURE;
        req[4] = 0xFF;
        req[5] = 0xFF;
        SsapCliFuzzSetTask(link, SSAP_FIND_STRUCTURE_REQ, req, 6);
        rsp[0] = SSAP_FIND_STRUCTURE_RSP;
        rsp[1] = (uint8_t)(0x03 | (FIND_ITEM_TYPE_MIX << 2));
        uint32_t struOff = 2;
        (void)memcpy_s(rsp + struOff, sizeof(struSvc), struSvc, sizeof(struSvc));
        struOff += sizeof(struSvc);
        (void)memcpy_s(rsp + struOff, sizeof(struProp), struProp, sizeof(struProp));
        struOff += sizeof(struProp);
        (void)memcpy_s(rsp + struOff, sizeof(struMethod), struMethod, sizeof(struMethod));
        struOff += sizeof(struMethod);
        (void)memcpy_s(rsp + struOff, sizeof(struEvent), struEvent, sizeof(struEvent));
        struOff += sizeof(struEvent);
        (void)memcpy_s(rsp + struOff, sizeof(struDesc), struDesc, sizeof(struDesc));
        struOff += sizeof(struDesc);
        rsp[struOff] = 0xEE; /* 残余 1 字节：解析完成员后的 extra data 分支（:682-685） */
        SsapCliFuzzFeed(link, SSAPC_FindRspHandle, rsp, struOff + 1);

        /* —— 2. V10 旧设备 ByUuid 解码族（:793-798 → *ByUuidV10）—— */
        devNode->companyId = 0x007C; /* COMPANY_ID_HISI：devType 未定 → CM 判 OLD */
        devNode->devType = CM_DEVTYPE_UNKNOWN;
        const uint8_t v10Svc[5] = {0x11, 0x00, 0x12, 0x00, 0x03};   /* handle2+end2+member1 */
        const uint8_t v10Prop[9] = {0x11, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x01, 0x02};
        for (uint8_t itemType = 0; itemType < 3; itemType++) {
            (void)memset_s(req, sizeof(req), 0, sizeof(req));
            req[0] = SSAP_FIND_STRUCTURE_BY_UUID_REQ;
            req[1] = FIND_STRUCTURE_TYPE_PRIMARY_SERVICE;
            req[4] = 0xFF;
            req[5] = 0xFF;
            for (int i = 0; i < SSAP_UUID128_LEN; i++) {
                req[6 + i] = 0x10 + i; /* 请求侧 16 字节 uuid（GetFindReqUuid 回填请求 uuid） */
            }
            SsapCliFuzzSetTask(link, SSAP_FIND_STRUCTURE_BY_UUID_REQ, req, 6 + SSAP_UUID128_LEN);
            rsp[0] = SSAP_FIND_STRUCTURE_BY_UUID_RSP;
            rsp[1] = (uint8_t)(0x03 | (itemType << 2));
            (void)memcpy_s(rsp + 2, sizeof(v10Svc), v10Svc, sizeof(v10Svc));
            SsapCliFuzzFeed(link, SSAPC_FindRspHandle, rsp, 2 + sizeof(v10Svc));

            /* V10 属性族（DecodePropertyByUuidV10） */
            (void)memset_s(req, sizeof(req), 0, sizeof(req));
            req[0] = SSAP_FIND_STRUCTURE_BY_UUID_REQ;
            req[1] = FIND_STRUCTURE_TYPE_PROPERTY;
            req[4] = 0xFF;
            req[5] = 0xFF;
            for (int i = 0; i < SSAP_UUID128_LEN; i++) {
                req[6 + i] = 0x20 + i;
            }
            SsapCliFuzzSetTask(link, SSAP_FIND_STRUCTURE_BY_UUID_REQ, req, 6 + SSAP_UUID128_LEN);
            rsp[1] = (uint8_t)(0x03 | (itemType << 2));
            (void)memcpy_s(rsp + 2, sizeof(v10Prop), v10Prop, sizeof(v10Prop));
            SsapCliFuzzFeed(link, SSAPC_FindRspHandle, rsp, 2 + sizeof(v10Prop));
        }
        devNode->devType = CM_DEVTYPE_NEW; /* 恢复新设备，后续族不受影响 */

        /* —— 3. READ_RSP 单/多值（SSAPC_ReadRspHandle:985-1011）—— */
        /* 3a. 单值成功：req 1 条目；rsp ctrl error=0，items 为裸 value */
        {
            uint8_t readReq[5] = {SSAP_READ_REQ, 0x03, 0x11, 0x00, 0x00};
            SsapCliFuzzSetTask(link, SSAP_READ_REQ, readReq, sizeof(readReq));
            uint8_t readOk[6] = {SSAP_READ_RSP, 0x03, 0xAA, 0xBB, 0xCC, 0xDD};
            SsapCliFuzzFeed(link, SSAPC_ReadRspHandle, readOk, sizeof(readOk));
            /* 3b. 单值错误：rsp ctrl error=1，items = {len:15|success:1(=0 错误), errCode} */
            uint8_t readErr[6] = {SSAP_READ_RSP, 0x0B, 0x05, 0x00, 0x00, 0x00};
            SsapCliFuzzFeed(link, SSAPC_ReadRspHandle, readErr, sizeof(readErr));
        }
        /* 3c. 多值：req 3 条目（CountReqHandleNum=3 → MultiReadRspDecode）；
         * 前两项成功（len 1），第三项错误 → FillErrorItems 按句柄数补齐（:915-941） */
        {
            uint8_t readMultiReq[11] = {SSAP_READ_REQ, 0x03, 0x11, 0x00, 0x00, 0x12, 0x00, 0x00,
                0x13, 0x00, 0x00};
            SsapCliFuzzSetTask(link, SSAP_READ_REQ, readMultiReq, sizeof(readMultiReq));
            uint8_t multiRsp[12] = {SSAP_READ_RSP, 0x0B,
                0x81, 0x00, 0xA1,            /* item1: len=1 success */
                0x81, 0x00, 0xA2,            /* item2: len=1 success */
                0x00, 0x05, 0x00, 0x00};     /* item3: len=0x0005 失败码 */
            SsapCliFuzzFeed(link, SSAPC_ReadRspHandle, multiRsp, sizeof(multiRsp));
            /* 3d. 仅单个错误项 + 3 句柄请求 → FillErrorItems 按句柄数补齐错误项（:915-941） */
            uint8_t multiShort[4] = {SSAP_READ_RSP, 0x0B, 0x05, 0x00};
            SsapCliFuzzFeed(link, SSAPC_ReadRspHandle, multiShort, sizeof(multiShort));
        }

        /* —— 4. READ_BY_UUID_RSP 控制码位（SSAPC_ReadByUuidRspDecode:1013-1062，
         *    items 自 buf+2 起、dataLen=len-2；指示字 bit15=1 表示携带 value）—— */
        {
            uint8_t rbuReq[9] = {SSAP_READ_BY_UUID_REQ, SSAP_READ_BY_CUSTOM_UUID_CONTROL,
                0x01, 0x00, 0xFF, 0xFF, 0x00, 0x28, 0x19};
            SsapCliFuzzSetTask(link, SSAP_READ_BY_UUID_REQ, rbuReq, sizeof(rbuReq));
            /* 单实例（ctrl=0x03）：{handle, value} */
            uint8_t rbuSingle[8] = {SSAP_READ_BY_UUID_RSP, 0x03, 0x11, 0x00, 0x55, 0x66, 0x77, 0x88};
            SsapCliFuzzFeed(link, SSAPC_ReadByUuidRspHandle, rbuSingle, sizeof(rbuSingle));
            /* 多实例位(bit2)：一项错误（指示=错误码）、一项带值（指示=0x8001|len） */
            uint8_t rbuMulti[11] = {SSAP_READ_BY_UUID_RSP, 0x07, 0x11, 0x00, 0x02, 0x00,
                0x12, 0x00, 0x01, 0x80, 0xEE};
            SsapCliFuzzFeed(link, SSAPC_ReadByUuidRspHandle, rbuMulti, sizeof(rbuMulti));
            /* 错误指示位(bit3)：{handle, errIndication} */
            uint8_t rbuErr[6] = {SSAP_READ_BY_UUID_RSP, 0x0B, 0x11, 0x00, 0x05, 0x00};
            SsapCliFuzzFeed(link, SSAPC_ReadByUuidRspHandle, rbuErr, sizeof(rbuErr));
            /* 结果集为空（多实例指示长度不足中断解码）→ ITEM_INEXIST（:1079-1081） */
            uint8_t rbuEmpty[5] = {SSAP_READ_BY_UUID_RSP, 0x07, 0x11, 0x00, 0x00};
            SsapCliFuzzFeed(link, SSAPC_ReadByUuidRspHandle, rbuEmpty, sizeof(rbuEmpty));
        }

        /* —— 5. WRITE_RSP 结果三态（SSAPC_WriteRspHandle:1165-1213；
         *    请求 ctrl：frag=3 | verify(bit5)=0x20；响应 ctrl：frag=3 | result<<2）—— */
        {
            /* 5a. 成功 + 请求带原值返回（verify=1）：items={handle,type,value} */
            uint8_t wrReq[8] = {SSAP_WRITE_REQ, 0x23, 0x11, 0x00, 0x00, 0x01, 0x02, 0x03};
            SsapCliFuzzSetTask(link, SSAP_WRITE_REQ, wrReq, sizeof(wrReq));
            uint8_t wrOkVerify[8] = {SSAP_WRITE_RSP, 0x03, 0x11, 0x00, 0x00, 0x01, 0x02, 0x03};
            SsapCliFuzzFeed(link, SSAPC_WriteRspHandle, wrOkVerify, sizeof(wrOkVerify));
            /* 5b. 成功 + 请求不带原值（verify=0）但 len>2：:1199 的 len==2 校验失败分支
             * （该分支与 :1176 len>2 守卫互斥，疑似死码，见报告） */
            uint8_t wrReq2[8] = {SSAP_WRITE_REQ, 0x03, 0x11, 0x00, 0x00, 0x01, 0x02, 0x03};
            SsapCliFuzzSetTask(link, SSAP_WRITE_REQ, wrReq2, sizeof(wrReq2));
            uint8_t wrOkPlain[3] = {SSAP_WRITE_RSP, 0x03, 0x00};
            SsapCliFuzzFeed(link, SSAPC_WriteRspHandle, wrOkPlain, sizeof(wrOkPlain));
            /* 5c. 部分写入（result=PART=1<<2）：错误项 {errorNum,handle,errorCode}，len=6 */
            uint8_t wrPart[6] = {SSAP_WRITE_RSP, 0x07, 0x01, 0x11, 0x00, 0x0D};
            SsapCliFuzzFeed(link, SSAPC_WriteRspHandle, wrPart, sizeof(wrPart));
            /* 5d. 取消写入（result=CANCEL=2<<2，请求 oper 非 CANCEL） */
            uint8_t wrCancel[6] = {SSAP_WRITE_RSP, 0x0B, 0x01, 0x11, 0x00, 0x0E};
            SsapCliFuzzFeed(link, SSAPC_WriteRspHandle, wrCancel, sizeof(wrCancel));
        }

        /* —— 6. CALL_METHOD_RSP（:1401-1428）—— */
        {
            uint8_t cmReq[6] = {SSAP_CALL_METHOD_REQ, 0x03, 0x12, 0x00, 0x01, 0x02};
            SsapCliFuzzSetTask(link, SSAP_CALL_METHOD_REQ, cmReq, sizeof(cmReq));
            uint8_t cmRsp[6] = {SSAP_CALL_METHOD_RSP, 0x03, 0x99, 0x88, 0x77, 0x66};
            SsapCliFuzzFeed(link, SSAPC_CallMethodRspHandle, cmRsp, sizeof(cmRsp));
        }

        /* —— 7. VALUE_NTF 多条目 + 服务变更条目（:1267-1302）—— */
        {
            /* 两项普通通知 + 一项服务变更（handle=0x000E 且新设备 → HandleServChangeEvent:1215-1238） */
            uint8_t ntf[22] = {SSAP_VALUE_NTF, 0x03,
                0x11, 0x00, 0x02, 0x00, 0xAA, 0xBB,          /* item1: handle 0x11 len2 */
                0x12, 0x00, 0x02, 0x00, 0xCC, 0xDD,          /* item2: handle 0x12 len2 */
                0x0E, 0x00, 0x04, 0x00, 0x00, 0x10, 0x00, 0x14}; /* item3: 服务变更句柄域 */
            SsapCliFuzzFeed(link, SSAPC_ValueNtfHandle, ntf, sizeof(ntf));
            /* 条目声明长度越界 → 中断分支（:1289-1292） */
            uint8_t ntfTrunc[7] = {SSAP_VALUE_NTF, 0x03, 0x11, 0x00, 0x04, 0x00, 0xAA};
            SsapCliFuzzFeed(link, SSAPC_ValueNtfHandle, ntfTrunc, sizeof(ntfTrunc));
        }
        /* —— 8. VALUE_IND 多条目 → ACK 回发（SSAPC_ValueIndHandle:1378-1396 → SendAck:1348-1372
         *    → SSAP_ValueAck → SSAPC_ValueAck 组包经 sendFunc 桩）—— */
        {
            uint8_t ind[13] = {SSAP_VALUE_IND, 0x03,
                0x11, 0x00, 0x02, 0x00, 0xAA, 0xBB,
                0x12, 0x00, 0x01, 0x00, 0xCC};
            SsapCliFuzzFeed(link, SSAPC_ValueIndHandle, ind, sizeof(ind));
            /* 条目越界中断 → itemCount=0 → SendAck 早退（:1350-1352） */
            uint8_t indTrunc[7] = {SSAP_VALUE_IND, 0x03, 0x11, 0x00, 0x04, 0x00, 0xAA};
            SsapCliFuzzFeed(link, SSAPC_ValueIndHandle, indTrunc, sizeof(indTrunc));
        }

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
        /* 扰动消费（分支已由常量模板覆盖） */
        (void)provider.ConsumeIntegral<uint8_t>();

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