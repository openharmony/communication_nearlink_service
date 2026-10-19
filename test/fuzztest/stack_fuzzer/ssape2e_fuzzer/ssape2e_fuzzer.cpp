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
#include "ssape2e_fuzzer.h"
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
#include "ssap_type.h"
#include "ssap_utils.h"
#include "dli_layer_callback.h"

typedef struct SlePkt {
    uint8_t *data;
    uint32_t size;
} SlePkt;

int SleSendDliPacket(const SlePkt *packet)
{
    (void)packet;
    return 0;
}

namespace OHOS {

/* 对端模拟闭环：同一进程内建两条 SSAP 链路（服务端/客户端），发送回调互相
 * 把出向报文投递到对端链路的 SSAP_Recv——客户端请求走服务端真实解析
 * （SSAPS_*ReqHandle 全族），服务端响应回喂客户端解析（SSAPC_*RspHandle
 * 全族），任务生命周期/opcode 期待/分片白名单全部走真实逻辑。 */
static DTAP_Data_Info_S g_serverInfo = {0};
static DTAP_Data_Info_S g_clientInfo = {0};

static NLSTK_SsapUuid_S g_uuid1 = {0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0xB7, 0x20,
    0xEA, 0x11, 0xFC, 0x70, 0x80, 0xA8, 0xBE, 0x37};
static NLSTK_SsapUuid_S g_uuid2 = {0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08,
    0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00};
static NLSTK_SsapUuid_S g_uuid3 = {0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0xB7, 0x20,
    0xEA, 0x11, 0xFC, 0x70, 0x80, 0xA8, 0xBE, 0x38};
static NLSTK_SsapUuid_S g_uuid4 = {0x10, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03,
    0x02, 0x01, 0x00, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E};

static uint8_t g_valueBuf[16] = {0};

/* 出向报文所有权口径（与产品单测 MockSendCb 一致）：
 * 带 REPLY_MASK 的 REQ/RSP/IND 出向报文经 SSAP_LinkSetTask 存为 curTask
 * （供重建/超时路径），CMD/NTF/ACK 由发送回调释放；
 * 投递给对端的是副本，避免跨链路所有权纠缠。 */
static void DeliverToPeer(const DTAP_Data_Info_S *info, SDF_Buff_S *buff)
{
    uint32_t len = (uint32_t)SDF_DataLenGet(buff);
    if (len == 0) {
        return;
    }
    SDF_Buff_S *copy = SDF_BuffNew(len);
    if (copy == nullptr) {
        return;
    }
    uint8_t *dst = SDF_BuffAppend(copy, len);
    if (dst != nullptr) {
        (void)memcpy_s(dst, len, SDF_DataOffset(buff), len);
        (void)SSAP_Recv(const_cast<DTAP_Data_Info_S *>(info), copy);
    }
    SDF_BuffFree(copy);
}

static void ServerSendCb(SSAP_Link_S *link, SDF_Buff_S *buff, uint8_t opcode)
{
    uint8_t type = SSAP_GetOpcodeType(opcode);
    if ((type & SSAP_REPLY_MASK) != 0) {
        SSAP_LinkSetTask(link, buff, opcode);
    }
    DeliverToPeer(&g_clientInfo, buff);
    if ((type & SSAP_REPLY_MASK) == 0) {
        SDF_BuffFree(buff);
    }
}

static void ClientSendCb(SSAP_Link_S *link, SDF_Buff_S *buff, uint8_t opcode)
{
    uint8_t type = SSAP_GetOpcodeType(opcode);
    if ((type & SSAP_REPLY_MASK) != 0) {
        SSAP_LinkSetTask(link, buff, opcode);
    }
    DeliverToPeer(&g_serverInfo, buff);
    if ((type & SSAP_REPLY_MASK) == 0) {
        SDF_BuffFree(buff);
    }
}

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
    SSAP_StartService(NULL);
}

static void AddService2()
{
    SSAP_ParamAddService_S *serviceParam1 =
        (SSAP_ParamAddService_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddService_S));
    serviceParam1->serviceType = ITEM_TYPE_STD_PRIMARY_SERVICE;
    memcpy_s(&serviceParam1->uuid, sizeof(NLSTK_SsapUuid_S), &g_uuid3, sizeof(NLSTK_SsapUuid_S));
    SSAP_CacheService(serviceParam1);
    SDF_MemFree(serviceParam1);
    SSAP_ParamAddProperty_S *propertyParam2 =
        (SSAP_ParamAddProperty_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddProperty_S) + 2);
    memcpy_s(&propertyParam2->uuid, sizeof(NLSTK_SsapUuid_S), &g_uuid4, sizeof(NLSTK_SsapUuid_S));
    propertyParam2->val.len = 2;
    propertyParam2->val.value[0] = 0xAB;
    propertyParam2->val.value[1] = 0xCD;
    propertyParam2->operation.operationValue = 0x1F; /* 读|写|通知 */
    SSAP_CacheProperty(propertyParam2);
    SDF_MemFree(propertyParam2);
    SSAP_StartService(NULL);
}

static void RunRequestTask(SSAP_Link_S *link, SSAP_ProcessTaskFunc func, void *arg)
{
    SSAP_TaskParam_S taskParam = {.appId = 1, .arg = arg, .freeFunc = SDF_MemFree,
        .func = func, .timeout = 3000, .valid = true, .appCallback = NULL};
    SSAP_ProcessRequestTask(link, &taskParam, false);
}

void FuzzSsapE2eSequence(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    SLE_Addr_S serverAddr = {0};
    SLE_Addr_S clientAddr = {0};
    serverAddr.type = fdp.ConsumeIntegral<uint8_t>() % 2;
    clientAddr.type = fdp.ConsumeIntegral<uint8_t>() % 2;
    for (int i = 0; i < SLE_ADDR_LEN; i++) {
        serverAddr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
        clientAddr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
    }
    uint16_t serverLcid = fdp.ConsumeIntegral<uint16_t>();
    uint16_t clientLcid = static_cast<uint16_t>(serverLcid ^ 0x8000); /* 保证互异 */
    uint16_t mtu = fdp.ConsumeIntegral<uint16_t>();
    uint8_t findType = fdp.ConsumeIntegral<uint8_t>() % 2;
    uint16_t handle = fdp.ConsumeIntegral<uint16_t>();
    uint8_t writeType = fdp.ConsumeIntegral<uint8_t>();
    uint8_t valueLen = static_cast<uint8_t>(1 + fdp.ConsumeIntegral<uint8_t>() % sizeof(g_valueBuf));
    for (uint8_t i = 0; i < valueLen; i++) {
        g_valueBuf[i] = fdp.ConsumeIntegral<uint8_t>();
    }
    NLSTK_SsapUuid_S findUuid = {0};
    for (size_t i = 0; i < sizeof(findUuid.uuid); i++) {
        findUuid.uuid[i] = fdp.ConsumeIntegral<uint8_t>();
    }

    SSAP_LinkInit();
    SSAPS_ServiceInit();
    AddService1();
    AddService2();

    SSAP_Link_S *serverLink = SSAP_CreateSsapLink(&serverAddr, serverLcid, ServerSendCb);
    SSAP_Link_S *clientLink = SSAP_CreateSsapLink(&clientAddr, clientLcid, ClientSendCb);
    if (serverLink == nullptr || clientLink == nullptr) {
        goto CLEANUP;
    }
    g_serverInfo.lcid = serverLcid;
    g_clientInfo.lcid = clientLcid;

    /* 1. MTU/版本协商（version ≥ 0x0103 使能分片/多处理上下文） */
    {
        SSAP_ExchangeInfoReqInfo_S *req = (SSAP_ExchangeInfoReqInfo_S *)SDF_MemZalloc(
            sizeof(SSAP_ExchangeInfoReqInfo_S));
        if (req != nullptr) {
            req->mtu = (mtu == 0) ? 0xFF : mtu;
            req->version = 0x0103;
            (void)memcpy_s(&req->addr, sizeof(SLE_Addr_S), &clientAddr, sizeof(SLE_Addr_S));
            RunRequestTask(clientLink, SSAPC_ExchangeInfoReq, req);
        }
    }

    /* 2. 服务发现（服务端走缓存服务库真实组装响应） */
    {
        SSAP_ParamFind_S *req = (SSAP_ParamFind_S *)SDF_MemZalloc(sizeof(SSAP_ParamFind_S));
        if (req != nullptr) {
            req->appId = 1;
            (void)memcpy_s(&req->addr, sizeof(SLE_Addr_S), &clientAddr, sizeof(SLE_Addr_S));
            req->type = static_cast<SSAP_FindType_E>(findType);
            req->startHandle = 1; /* 产品守卫拒绝 startHandle==0 */
            req->endHandle = 0xFFFF;
            RunRequestTask(clientLink, SSAPC_FindReq, req);
        }
    }

    /* 3. 按 UUID 发现 */
    {
        SSAP_ParamFindByUuid_S *req = (SSAP_ParamFindByUuid_S *)SDF_MemZalloc(sizeof(SSAP_ParamFindByUuid_S));
        if (req != nullptr) {
            req->appId = 1;
            (void)memcpy_s(&req->addr, sizeof(SLE_Addr_S), &clientAddr, sizeof(SLE_Addr_S));
            req->type = static_cast<SSAP_FindType_E>(findType);
            (void)memcpy_s(&req->uuid, sizeof(NLSTK_SsapUuid_S), &findUuid, sizeof(NLSTK_SsapUuid_S));
            req->startHandle = 1; /* 同上 */
            req->endHandle = 0xFFFF;
            RunRequestTask(clientLink, SSAPC_FindByUuidReq, req);
        }
    }

    /* 4. 写请求（有响应）/写命令（无响应） */
    {
        SSAP_WriteReqInfo_S *req = (SSAP_WriteReqInfo_S *)SDF_MemZalloc(
            sizeof(SSAP_WriteReqInfo_S) + valueLen);
        if (req != nullptr) {
            (void)memcpy_s(&req->addr, sizeof(SLE_Addr_S), &clientAddr, sizeof(SLE_Addr_S));
            req->handle = handle;
            req->type = writeType;
            req->value.len = valueLen;
            for (uint8_t i = 0; i < valueLen; i++) {
                req->value.value[i] = g_valueBuf[i];
            }
            RunRequestTask(clientLink, SSAPC_WriteReq, req);
        }
        SSAP_WriteCmdInfo_S *cmd = (SSAP_WriteCmdInfo_S *)SDF_MemZalloc(
            sizeof(SSAP_WriteCmdInfo_S) + valueLen);
        if (cmd != nullptr) {
            (void)memcpy_s(&cmd->addr, sizeof(SLE_Addr_S), &clientAddr, sizeof(SLE_Addr_S));
            cmd->handle = handle;
            cmd->type = writeType;
            cmd->value.len = valueLen;
            for (uint8_t i = 0; i < valueLen; i++) {
                cmd->value.value[i] = g_valueBuf[i];
            }
            RunRequestTask(clientLink, SSAPC_WriteCmd, cmd);
        }
    }

    /* 非法路径保留：向服务端链路直接注入未请求的响应（opcode 期待不匹配/
     * 空闲态拒绝分支），以及服务端回错的错误响应闭环 */
    {
        static const uint8_t unsolicitedRsp[] = {0x0e, 0x03, 0x11, 0x00, 0x00, 0x06};
        SDF_Buff_S *bad = SDF_BuffNew(sizeof(unsolicitedRsp));
        if (bad != nullptr) {
            uint8_t *dst = SDF_BuffAppend(bad, sizeof(unsolicitedRsp));
            if (dst != nullptr) {
                (void)memcpy_s(dst, sizeof(unsolicitedRsp), unsolicitedRsp, sizeof(unsolicitedRsp));
                (void)SSAP_Recv(&g_serverInfo, bad);
            }
            SDF_BuffFree(bad);
        }
    }

CLEANUP:
    if (serverLink != nullptr) {
        SSAP_DeleteSsapLinkByAddr(&serverAddr);
    }
    if (clientLink != nullptr) {
        SSAP_DeleteSsapLinkByAddr(&clientAddr);
    }
    SSAPS_ServiceDeInit();
    SSAP_LinkDeInit();
}

}  // namespace OHOS

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc;
    (void)argv;
    NLSTK_InitStack();
    NLSTK_EnableStack();
    (void)SDF_ThreadInit(10);
    (void)SDF_EvcInit();
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    uint8_t *fuzzData = (uint8_t *)malloc(size);
    if (fuzzData == nullptr) {
        return 0;
    }
    (void)memcpy_s(fuzzData, size, data, size);
    OHOS::FuzzSsapE2eSequence(fuzzData, size);
    free(fuzzData);
    return 0;
}
