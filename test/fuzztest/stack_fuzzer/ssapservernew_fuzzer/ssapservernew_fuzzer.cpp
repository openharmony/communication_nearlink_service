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
#include "ssapservernew_fuzzer.h"
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
#include "cm_api.h"
#include "sle_logic_link_mgr.h"
#include "cm_signaling_version.h"
#include "SleDliLayerAdapter.h"
#include "ssap_type.h"
#include "nlstk_ssap_app_server.h"
#include "fuzzer/FuzzedDataProvider.h"

int SleSendDliPacket(const SlePacket *packet)
{
    return SUCCESS;
}

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

static NLSTK_SsapUuid_S g_uuid1 = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01, 0x02};
static NLSTK_SsapUuid_S g_uuid2 = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
    0x0c, 0x0d, 0x0e, 0x0f, 0x10};
static NLSTK_SsapUuid_S g_uuid3 = {0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c,
    0x0d, 0x0e, 0x0f, 0x10, 0x11};
static NLSTK_SsapUuid_S g_uuid4 = {0x10, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00,
    0x0a, 0x0b, 0x0c, 0x0d, 0x0e};
static NLSTK_SsapUuid_S g_uuid2Net = {0x10, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08, 0x07, 0x06,
    0x05, 0x04, 0x03, 0x02, 0x01};
static NLSTK_SsapUuid_S g_uuid4Net = {0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
    0x06, 0x07, 0x08, 0x09, 0x10};

/* 句柄域推导常量（遗留 Cache/StartService 路径）：分配规则见 ssaps_service.c:281-328/56-68 ——
 * handleNum = 1 + 属性数（描述符不计数、不分配句柄，仅挂在属性向量下），服务声明取区间 start，
 * 属性依次 start+1..；g_curHandle 自 SSAP_INIT_HANDLE=0x10 单调前移（ssaps_service.c:45）。
 * SSAPS_ServiceDeInit 复位 g_curHandle=0x10（ssaps_service.c:387）但仅由栈级 SSAP_DeInit 触发
 * （ssap_manager.c:698），本工程每输入只跑 SSAP_LinkInit/LinkDeInit 不复位 → 下列布局对首个
 * 输入精确（AddService1: 0x10~0x12；AddService2: 0x13~0x14），后续输入游标每输入前移
 * +15（Handle1/3/4 各消费 5 句柄），常量退化为确定性扰动范围；以实施 SHA 为准。 */
static const uint16_t SVC1_HANDLE = 0x10;       /* AddService1 服务声明；属性2=0x12（未单列常量） */
static const uint16_t SVC1_PROP1_HANDLE = 0x11; /* AddService1 属性1；AddService2 服务声明=0x13（未单列常量） */
static const uint16_t SVC2_PROP1_HANDLE = 0x14; /* AddService2 属性（end=0x14） */

namespace OHOS {
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
        propertyParam2->permission.permissionValue = 0x02;
        SSAP_CacheProperty(propertyParam2);
        SDF_MemFree(propertyParam2);
        SSAP_StartService(NULL);
    }

    /* ===== app 路径：方法真实入缓存（遗留 Cache 族无方法，ssaps_service.h:49-53） ===== */
    /* onAddService 回调实录句柄：AddService 成功后由 SsapServerAppStartServiceCallBack
     * （ssaps_server_app.c:307）在调度线程回填真实方法句柄 */
    static uint16_t g_appMethodHandle = 0;

    static void SsapServerFuzzOnAddService(int32_t appId, SSAP_Service_S *service, NLSTK_Errcode_E ret)
    {
        (void)appId;
        /* 异常分支 service==NULL（SsapServerAppAddServiceExceptionCallBack），不覆盖实录值 */
        if (service == nullptr || ret != NLSTK_ERRCODE_SUCCESS || service->methods == nullptr ||
            service->methods->size == 0) {
            return;
        }
        SSAP_Method_S *method = (SSAP_Method_S *)SDF_VectorElementAt(service->methods, 0);
        if (method != nullptr) {
            g_appMethodHandle = method->handle;
        }
    }

    /* RegApp(阻塞)+AddService(异步)：服务经 SsapServerAddService 入 g_services，句柄由
     * SSAP_HandleAllocate 首适自 0x10 分配（ssaps_service.c:226-279，与遗留 g_curHandle
     * 游标互不相干）：1 属性+1 方法 → 服务/属性/方法三连句柄。返回 appId（<0 失败）。
     * 随后用阻塞的 CheckServiceExistByUuid 作 FIFO 屏障（同 worker 尾插保序，
     * sdf_worker.c SDF_AddWork/SDF_WorkerRunOnce）：其返回即 AddService 已执行完毕、
     * 真实方法句柄已实录。 */
    static int32_t SsapServerAppMethodCache(void)
    {
        int32_t srvAppId = -1;
        NLSTK_SsapAppServerCb_S cb = {};
        cb.onAddService = SsapServerFuzzOnAddService;
        if (NLSTK_SsapServerRegApp(&cb, &srvAppId) != NLSTK_ERRCODE_SUCCESS || srvAppId < 0) {
            return -1;
        }
        NLSTK_SsapServicePropertyParam_S property = {};
        property.type = ITEM_TYPE_STD_PROPERTY;
        (void)memcpy_s(&property.uuid, sizeof(NLSTK_SsapUuid_S), &g_uuid4, sizeof(NLSTK_SsapUuid_S));
        property.operation.operationValue = 1;
        NLSTK_SsapServiceMethodParam_S method = {};
        method.type = ITEM_TYPE_STD_METHOD;
        (void)memcpy_s(&method.uuid, sizeof(NLSTK_SsapUuid_S), &g_uuid3, sizeof(NLSTK_SsapUuid_S));
        NLSTK_ServiceParam_S service = {};
        service.serviceStatement.serviceType = ITEM_TYPE_STD_PRIMARY_SERVICE;
        (void)memcpy_s(&service.serviceStatement.uuid, sizeof(NLSTK_SsapUuid_S), &g_uuid2,
            sizeof(NLSTK_SsapUuid_S));
        service.servicePropertyNum = 1;
        service.property = &property;
        service.serviceMethodNum = 1;
        service.method = &method;
        g_appMethodHandle = 0;
        (void)NLSTK_SsapServerAddService(srvAppId, &service);
        bool isExist = false;
        (void)NLSTK_SsapServerCheckServiceExistByUuid(srvAppId, &service.serviceStatement.uuid, &isExist);
        return srvAppId;
    }

    SSAP_BufferedOperation_S *SsapServerFuzzGet(uint8_t *data, size_t size)
    {
        SSAP_BufferedOperation_S *value = (SSAP_BufferedOperation_S *)
            SDF_MemZalloc(sizeof(SSAP_BufferedOperation_S) + size);
        if (value == nullptr) {
            return nullptr;
        }
        (void)memcpy_s(value, size, data, size);
        value->value.len = static_cast<uint16_t>(size);
        (void)memcpy_s(value->value.value, size, data, size);
        return value;
    }

    void SendFunc(SSAP_Link_S *link, SDF_Buff_S *buff, uint8_t opcode)
    {
        (void)link;
        (void)buff;
        (void)opcode;
    }

    void FuzzSsapServerHandle1(uint8_t *data, size_t size)
    {
        if (size >= FUZZ_TWOHUNDREDFIFTYSIX) {
            return;
        }
        FuzzedDataProvider provider(data, size);
        AddService1();
        AddService2();
        SLE_Addr_S sleAddr;
        uint32_t len = size > SLE_ADDR_LEN ? SLE_ADDR_LEN : size;
        (void)memcpy_s(sleAddr.addr, SLE_ADDR_LEN, data, len);
        sleAddr.type = PUBLIC_ADDRESS;
        SleLogicLinkInit();
        SleLogicLink_S *node = SleLogicLinkAdd(&sleAddr);
        if (node == nullptr) {
            return;
        }
        node->lcid = provider.ConsumeIntegral<uint16_t>();
        SSAP_Link_S *link = SSAP_CreateSsapLink(&sleAddr, provider.ConsumeIntegral<uint16_t>(), SendFunc);
        if (link == nullptr) {
            SleLogicLinkRemove(node);
            return;
        }
        SDF_Buff_S *sdfBuff = SDF_BuffNewWithReserve(size);
        uint8_t *buf = SDF_BuffAppend(sdfBuff, size);
        if (sdfBuff == nullptr || buf == nullptr) {
            SDF_BuffFree(sdfBuff);
            SSAP_DeleteSsapLinkByAddr(&sleAddr);
            SleLogicLinkRemove(node);
            return;
        }
        (void)memcpy_s(buf, size, data, size);

        SSAPS_WriteCmdHandle(link, sdfBuff);
        SSAPS_WriteReqHandle(link, sdfBuff);
        SSAPS_ReadReqHandle(link, sdfBuff);
        SSAPS_ReadByUuidReqHandle(link, sdfBuff);
        SSAPS_ExchangeInfoReqHandle(link, sdfBuff);
        SSAPS_ValueAckHandle(link, sdfBuff);

        buf[0] = provider.ConsumeIntegral<uint8_t>() >= 0x80 ? SSAP_FIND_STRUCTURE_REQ : SSAP_FIND_STRUCTURE_BY_UUID_REQ;
        SSAPS_FindReqHandle(link, sdfBuff);
        /* hisi old dev */
        node->companyId = 0x007C; // COMPANY_ID_HISI
        node->protocolVersion = 0xFFFF; // INVALID_VERSION
        CM_SetDeviceLinkDeviceType(node->lcid, false);
        node->devType = 1; /* hisi old */
        SSAPS_FindReqHandle(link, sdfBuff);

        SDF_BuffFree(sdfBuff);

        /* 结构化 find 变体：句柄域三态（空范围/边界句柄/交叉跨服务）× findType(PROPERTY/METHOD)，
         * 上面 buf 直填 fuzz 字节的 find 保留为扰动路径；itemType=MIX 使 CheckFindItemType 恒真
         * （ssaps_server_find.c:65-78），命中与否只取决于句柄域。 */
        int32_t srvAppId = SsapServerAppMethodCache();
        SDF_Buff_S *findBuff = (SDF_Buff_S *)SDF_MemZalloc(sizeof(SDF_Buff_S) + sizeof(SSAP_PduFindStructReq_S));
        if (findBuff != nullptr) {
            findBuff->buffLen = sizeof(SSAP_PduFindStructReq_S);
            findBuff->dataLen = sizeof(SSAP_PduFindStructReq_S);
            SSAP_PduFindStructReq_S *findReq = (SSAP_PduFindStructReq_S *)SDF_DataOffset(findBuff);
            findReq->msgCode = SSAP_FIND_STRUCTURE_REQ;
            findReq->ctrl.itemType = FIND_ITEM_TYPE_MIX;
            findReq->ctrl.findType = (provider.ConsumeIntegral<uint8_t>() % FUZZ_TWO) == 0 ?
                FIND_STRUCTURE_TYPE_PROPERTY : FIND_STRUCTURE_TYPE_METHOD;
            /* 边界态句柄：METHOD 且方法已实录时用真实方法句柄（命中），否则推导属性句柄 */
            uint16_t bndHandle = (findReq->ctrl.findType == FIND_STRUCTURE_TYPE_METHOD &&
                g_appMethodHandle != 0) ? g_appMethodHandle : SVC1_PROP1_HANDLE;
            for (int state = 0; state < FUZZ_THREE; state++) {
                if (state == 0) { /* 空范围：命中为空，走 ITEM_INEXIST 分支 */
                    findReq->startHandle = 0x01;
                    findReq->endHandle = SVC1_HANDLE - 1;
                } else if (state == 1) { /* 边界：start=end=方法/属性句柄 */
                    findReq->startHandle = bndHandle;
                    findReq->endHandle = bndHandle;
                } else { /* 交叉跨服务：SVC1 服务声明 ~ SVC2 属性 */
                    findReq->startHandle = SVC1_HANDLE;
                    findReq->endHandle = SVC2_PROP1_HANDLE;
                }
                SSAPS_FindReqHandle(link, findBuff);
            }
            if (srvAppId >= 0 && g_appMethodHandle != 0) {
                /* 真实方法句柄精确命中：FindMethodList 非空 → 响应组包路径
                 * （ssaps_server_find.c:1052-1077/:1106+），否则遗留服务恒 ITEM_INEXIST */
                findReq->ctrl.findType = FIND_STRUCTURE_TYPE_METHOD;
                findReq->startHandle = g_appMethodHandle;
                findReq->endHandle = g_appMethodHandle;
                SSAPS_FindReqHandle(link, findBuff);
            }
            SDF_MemFree(findBuff);
        }

        sdfBuff = (SDF_Buff_S *)SDF_MemZalloc(sizeof(SDF_Buff_S) + sizeof(SSAP_PduCallMethodReq_S));
        sdfBuff->buffLen = sizeof(SSAP_PduCallMethodReq_S);
        sdfBuff->dataLen = sizeof(SSAP_PduCallMethodReq_S);
        buf = SDF_DataOffset(sdfBuff);
        SSAP_PduCallMethodReq_S *callMethodMsg = (SSAP_PduCallMethodReq_S *)buf;
        callMethodMsg->ctrl.fragment = 0x03;
        if (srvAppId >= 0 && g_appMethodHandle != 0) {
            /* 真实方法句柄优先：GetMethodByHandle 命中 → 方法调用入队/回调分发路径
             * （ssaps_server.c:1204-1227）；下方 fuzz handle 保留为错误路径 */
            callMethodMsg->handle = g_appMethodHandle;
            SSAPS_MethodCmdHandle(link, sdfBuff);
        }
        callMethodMsg->handle = provider.ConsumeIntegral<uint16_t>();

        SSAPS_MethodCmdHandle(link, sdfBuff);
        SDF_MemFree(sdfBuff);

        if (srvAppId >= 0) {
            /* 收尾注销：清 app 服务并归还句柄区间（SsapServerCleanAppService →
             * SSAPS_RemoveService → SSAP_HandleRelease），句柄分配器恢复基线 */
            NLSTK_SsapServerDeregisterApplication(srvAppId);
        }
        SSAP_DeleteSsapLinkByAddr(&sleAddr);
        SleLogicLinkRemove(node);
    }

    void FuzzSsapServerHandle2(uint8_t *data, size_t size)
    {
        if (size >= FUZZ_TWOHUNDREDFIFTYSIX) {
            return;
        }
        FuzzedDataProvider provider(data, size);
        SLE_Addr_S sleAddr;
        uint32_t len = size > SLE_ADDR_LEN ? SLE_ADDR_LEN : size;
        (void)memcpy_s(sleAddr.addr, SLE_ADDR_LEN, data, len);
        sleAddr.type = PUBLIC_ADDRESS;
        SleLogicLinkInit();
        SleLogicLink_S *node = SleLogicLinkAdd(&sleAddr);
        if (node == nullptr) {
            return;
        }
        node->lcid = provider.ConsumeIntegral<uint16_t>();
        SSAP_Link_S *link = SSAP_CreateSsapLink(&sleAddr, provider.ConsumeIntegral<uint16_t>(), SendFunc);
        if (link == nullptr) {
            SleLogicLinkRemove(node);
            return;
        }

        uint32_t realSize = sizeof(SSAP_PduReadByUuidReq_S) + 0x10;
        SDF_Buff_S *sdfBuff1 = SDF_BuffNewWithReserve(realSize);
        uint8_t *buf1 = SDF_BuffAppend(sdfBuff1, realSize);
        SSAP_PduReadByUuidReq_S *pdu1 = (SSAP_PduReadByUuidReq_S *)buf1;
        pdu1->msgCode = SSAP_READ_BY_UUID_REQ;
        pdu1->ctrl.uuidType = SSAP_READ_BY_CUSTOM_UUID_CONTROL;
        pdu1->handleStart = 0;
        pdu1->handleEnd = provider.ConsumeIntegral<uint16_t>();
        (void)memcpy_s(pdu1->uuid, 0x10, &g_uuid2Net, 0x10);
        SDF_Buff_S *sdfBuff2 = SDF_BuffNewWithReserve(realSize);
        uint8_t *buf2 = SDF_BuffAppend(sdfBuff2, realSize);
        SSAP_PduReadByUuidReq_S *pdu2 = (SSAP_PduReadByUuidReq_S *)buf2;
        pdu2->msgCode = SSAP_READ_BY_UUID_REQ;
        pdu2->ctrl.uuidType = SSAP_READ_BY_CUSTOM_UUID_CONTROL;
        pdu2->handleStart = 0;
        pdu2->handleEnd = provider.ConsumeIntegral<uint16_t>();
        (void)memcpy_s(pdu2->uuid, 0x10, &g_uuid4Net, 0x10);

        if (sdfBuff1 != NULL) {
            SSAPS_ReadByUuidReqHandle(link, sdfBuff1);
        }
        if (sdfBuff2 != NULL) {
            SSAPS_ReadByUuidReqHandle(link, sdfBuff2);
        }

        SDF_BuffFree(sdfBuff1);
        SDF_BuffFree(sdfBuff2);
        SSAP_DeleteSsapLinkByAddr(&sleAddr);
        SleLogicLinkRemove(node);
    }

    void FuzzSsapServerHandle3(uint8_t *data, size_t size)
    {
        if (size < SLE_ADDR_LEN || size >= FUZZ_TWOHUNDREDFIFTYSIX) {
            return;
        }
        FuzzedDataProvider provider(data, size);
        AddService1();
        AddService2();
        SLE_Addr_S sleAddr;
        (void)memcpy_s(sleAddr.addr, SLE_ADDR_LEN, data, SLE_ADDR_LEN);
        sleAddr.type = PUBLIC_ADDRESS;
        SleLogicLinkInit();
        SleLogicLink_S *node = SleLogicLinkAdd(&sleAddr);
        if (node == nullptr) {
            return;
        }
        node->lcid = provider.ConsumeIntegral<uint16_t>();
        SSAP_Link_S *link = SSAP_CreateSsapLink(&sleAddr, provider.ConsumeIntegral<uint16_t>(), SendFunc);
        if (link == nullptr) {
            SleLogicLinkRemove(node);
            return;
        }
        SDF_Buff_S *sdfBuff = (SDF_Buff_S *)SDF_MemZalloc(sizeof(SDF_Buff_S) + SSAP_READ_REQ_PDU_LEN);
        if (sdfBuff == NULL) {
            return;
        }
        (void)memcpy_s(sdfBuff->buff, SSAP_READ_REQ_PDU_LEN, data, SSAP_READ_REQ_PDU_LEN);
        sdfBuff->dataLen = SSAP_READ_REQ_PDU_LEN;
        SSAP_PduReadReq_S *readReq = (SSAP_PduReadReq_S *)SDF_DataOffset(sdfBuff);
        Ssap_PduReadReqItem_S *readReqItem = (Ssap_PduReadReqItem_S *)(readReq->items);
        /* 半数输入用推导属性句柄（首输入精确命中真实属性），size%8 保留为扰动路径 */
        readReqItem->handle = provider.ConsumeIntegral<uint8_t>() >= 0x80 ? SVC1_PROP1_HANDLE :
            (size % FUZZ_EIGHT);
        SSAPS_ReadReqHandle(link, sdfBuff);

        /* hisi old dev */
        node->companyId = 0x007C; // COMPANY_ID_HISI
        node->protocolVersion = 0xFFFF; // INVALID_VERSION
        CM_SetDeviceLinkDeviceType(node->lcid, false);
        node->devType = 1; /* hisi old */

        SDF_BuffFree(sdfBuff);
        SSAP_DeleteSsapLinkByAddr(&sleAddr);
        SleLogicLinkRemove(node);
    }

    void FuzzSsapServerHandle4(uint8_t *data, size_t size)
    {
        size_t curIndex = 0;
        if (size < SLE_ADDR_LEN + 2 || size > 255) {
            return;
        }
        AddService1();
        AddService2();
        SLE_Addr_S sleAddr;
        (void)memcpy_s(sleAddr.addr, SLE_ADDR_LEN, data, SLE_ADDR_LEN);
        curIndex += SLE_ADDR_LEN;
        sleAddr.type = PUBLIC_ADDRESS;
        SleLogicLinkInit();
        SleLogicLink_S *node = SleLogicLinkAdd(&sleAddr);
        if (node == nullptr) {
            return;
        }
        node->lcid = data[curIndex++];
        SSAP_Link_S *link = SSAP_CreateSsapLink(&sleAddr, node->lcid, SendFunc);
        if (link == nullptr) {
            SleLogicLinkRemove(node);
            return;
        }

        size_t len = sizeof(SDF_Buff_S) + sizeof(SSAP_PduReadByUuidReq_S) + sizeof(NLSTK_SsapUuid_S);
        SDF_Buff_S *sdfBuff = (SDF_Buff_S *)SDF_MemZalloc(len);
        if (sdfBuff == NULL) {
            return;
        }
        sdfBuff->dataLen = sizeof(SSAP_PduReadByUuidReq_S) + sizeof(NLSTK_SsapUuid_S);
        sdfBuff->buffLen = sizeof(SSAP_PduReadByUuidReq_S) + sizeof(NLSTK_SsapUuid_S);
        SSAP_PduReadByUuidReq_S *readByUuidReq = (SSAP_PduReadByUuidReq_S *)SDF_DataOffset(sdfBuff);
        readByUuidReq->handleEnd = data[curIndex++];
        memcpy_s(readByUuidReq->uuid, sizeof(NLSTK_SsapUuid_S), &g_uuid1, sizeof(NLSTK_SsapUuid_S));
        
        SSAPS_ReadByUuidReqHandle(link, sdfBuff);

        /* hisi old dev */
        node->companyId = 0x007C; // COMPANY_ID_HISI
        node->protocolVersion = 0xFFFF; // INVALID_VERSION
        CM_SetDeviceLinkDeviceType(node->lcid, false);
        node->devType = 1; /* hisi old */

        SDF_BuffFree(sdfBuff);
        SSAP_DeleteSsapLinkByAddr(&sleAddr);
        SleLogicLinkRemove(node);
    }

    void FuzzSsapSendAndRecvPkt(uint8_t *data, size_t size)
    {
        if (size < sizeof(SSAP_ValueInfo_S) || size > 255) {
            return;
        }
        SLE_Addr_S sleAddr;
        uint32_t len = size > SLE_ADDR_LEN ? SLE_ADDR_LEN : size;
        (void)memcpy_s(sleAddr.addr, SLE_ADDR_LEN, data, len);
        sleAddr.type = PUBLIC_ADDRESS;
        SSAP_Link_S *link = SSAP_CreateSsapLink(&sleAddr, data[0], SendFunc);
        if (link == nullptr) {
            return;
        }
        SSAP_ValueInfo_S *testData = (SSAP_ValueInfo_S *)SDF_MemZalloc(sizeof(SSAP_ValueInfo_S) + size);
        if (testData == nullptr) {
            SSAP_DeleteSsapLinkByAddr(&sleAddr);
            return;
        }
        memcpy_s(testData, sizeof(SSAP_ValueInfo_S), data, sizeof(SSAP_ValueInfo_S));
        testData->value.len = size;
        memcpy_s(testData->value.value, size, data, size);
        SSAPS_ValueNtf(link, static_cast<void *>(testData));
        SSAPS_ValueInd(link, static_cast<void *>(testData));
        SDF_MemFree(testData);
        SSAP_DeleteSsapLinkByAddr(&sleAddr);
    }

    void FuzzSsapServerApi(uint8_t *data, size_t size)
    {
        if (size > 255) {
            return;
        }
        FuzzedDataProvider provider(data, size);
        SLE_Addr_S sleAddr;
        uint32_t len = size > SLE_ADDR_LEN ? SLE_ADDR_LEN : size;
        (void)memcpy_s(sleAddr.addr, SLE_ADDR_LEN, data, len);
        sleAddr.type = PUBLIC_ADDRESS;
        SSAP_Link_S *link = SSAP_CreateSsapLink(&sleAddr, provider.ConsumeIntegral<uint16_t>(), SendFunc);
        if (link == nullptr) {
            return;
        }
        SSAP_BufferedOperation_S *value = SsapServerFuzzGet(data, size);
        if (value == nullptr) {
            SSAP_DeleteSsapLinkByAddr(&sleAddr);
            return;
        }
        SSAPS_UpdateItemValueByHandle(static_cast<void *>(value));
        SDF_MemFree(value);
        SSAP_SendResponseValue_S *response =
            (SSAP_SendResponseValue_S *)SDF_MemZalloc(sizeof(SSAP_SendResponseValue_S) + size);
        if (response == nullptr) {
            SSAP_DeleteSsapLinkByAddr(&sleAddr);
            return;
        }
        response->status = provider.ConsumeIntegral<uint16_t>();
        response->requestId = provider.ConsumeIntegral<uint16_t>();
        response->len = size;
        if (size >= 0x02) {
            response->requestId = provider.ConsumeIntegral<uint16_t>();
        }
        (void)memcpy_s(response->data, response->len, data, size);
        SSAPS_SendUserResponse(static_cast<void *>(response));
        SDF_MemFree(response);
        SSAP_ParamRemoveService_S *removeParam =
            (SSAP_ParamRemoveService_S *)SDF_MemZalloc(sizeof(SSAP_ParamRemoveService_S));
        if (removeParam == nullptr) {
            SSAP_DeleteSsapLinkByAddr(&sleAddr);
            return;
        }
        removeParam->startHandle = provider.ConsumeIntegral<uint16_t>();
        removeParam->endHandle = provider.ConsumeIntegral<uint16_t>();
        if (size >= 0x02) {
            removeParam->endHandle = provider.ConsumeIntegral<uint16_t>();
        }
        SSAP_RemoveService(static_cast<void *>(removeParam));
        SDF_MemFree(removeParam);
        SSAP_DeleteSsapLinkByAddr(&sleAddr);
    }

    void FuzzProecessTaskFunc(SSAP_Link_S *link, void *arg)
    {
        (void)link;
        (void)arg;
    }

    void SsapServerRegisterAppImpl(int32_t appId, NLSTK_Errcode_E ret)
    {
        (void)appId;
        (void)ret;
    }

    void SsapServerCleanAppResultCbImpl(void)
    {
        return;
    }

    void FuzzSsapServerAsyncApi(uint8_t *data, size_t size)
    {
        if (data == nullptr || size < 1) {
            return;
        }
        FuzzedDataProvider provider(data, size);
        NLSTK_SsapAppServerCb_S cb = {0};
        cb.onRegisterApp = SsapServerRegisterAppImpl;
        (void)NLSTK_SsapServerRegAppAsyn(&cb);

        int32_t appId = provider.ConsumeIntegral<int32_t>();
        NLSTK_SsapServerDeregisterApplicationAsync(appId);

        NLSTK_ServiceParam_S *service =
            (NLSTK_ServiceParam_S *)SDF_MemZalloc(sizeof(NLSTK_ServiceParam_S));
        if (service != nullptr) {
            NLSTK_SsapFreeServiceParam(service);
        }

        (void)NLSTK_SsapCleanServerApp(SsapServerCleanAppResultCbImpl);
    }

    void FuzzSsapLinkApi(uint8_t *data, size_t size)
    {
        SLE_Addr_S sleAddr;
        FuzzedDataProvider provider(data, size);
        uint32_t len = size > SLE_ADDR_LEN ? SLE_ADDR_LEN : size;
        (void)memcpy_s(sleAddr.addr, SLE_ADDR_LEN, data, len);
        sleAddr.type = PUBLIC_ADDRESS;
        SSAP_Link_S *link = SSAP_CreateSsapLink(&sleAddr, provider.ConsumeIntegral<uint16_t>(), SendFunc);
        if (link == nullptr) {
            return;
        }
        SSAP_FindSsapLinkByAddr(&sleAddr);
        (void)SSAP_FindSsapLinkByLcid(provider.ConsumeIntegral<uint16_t>());
        // SSAP_LinkSetTask
        SSAP_TaskParam_S taskParam = {.appId = 0, .arg = static_cast<void *>(data), .freeFunc = NULL,
            .func = FuzzProecessTaskFunc, .timeout = 3000, .valid = true, .appCallback = NULL};
        (void)SSAP_AllocTaskParam(&taskParam);
        (void)SSAP_GetLastBuff(link);
        SSAP_AddTaskParamToLink(link, &taskParam);
        (void)SSAP_LinkGetFirstTaskParam(link);
        SSAP_ExcuteProcessTask(link);
        SSAP_CheckOpcode(link, provider.ConsumeIntegral<uint8_t>());
        SSAP_DeleteSsapLinkByAddr(&sleAddr);
    }

    void FuzzStackServerApiDataProc(uint8_t* data, size_t size)
    {
        if (data == nullptr || size < 1) {
            return;
        }
        SSAP_LinkInit();
        FuzzSsapLinkApi(data, size);
        FuzzSsapServerHandle1(data, size);
        FuzzSsapServerHandle2(data, size);
        FuzzSsapSendAndRecvPkt(data, size);
        FuzzSsapServerApi(data, size);
        FuzzSsapServerAsyncApi(data, size);
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
    } else {
        NAI_LOG_INFO("NAI_InitStack success");
        res = NLSTK_EnableStack();
        if (res != 0) {
            NAI_LOG_ERROR("NLSTK_EnableStack failed, ret:%{public}d", res);
        } else {
            NAI_LOG_INFO("NLSTK_EnableStack success");
        }
    }
    /* 补调度三件套（幂等保障，成功/失败分支均可达）：app 路径 RegApp/Deregister 走
     * SchedulePostTaskBlocked、AddService 走 SchedulePostTask，调度器不活则 appId 恒 -1；
     * InitStack/EnableStack 失败回滚会销毁调度器（nlstk_init.c:510/:533），失败分支不再
     * early-return（返回值不拦 libFuzzer 继续跑输入）——照 ssapappserverapi_fuzzer.cpp:210-230
     * / uapi 先例；ScheduleEnable 幂等（nlstk_schedule.c:65-71）。 */
    (void)SDF_ThreadInit(10);
    (void)SDF_EvcInit();
    (void)ScheduleEnable();
    NAI_LOG_INFO("ssapservernew_fuzzer init success");
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
    OHOS::FuzzStackServerApiDataProc(fuzzData, size);
    free(fuzzData);
    return 0;
}