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
#include "cm_api.h"
#include "sle_logic_link_mgr.h"
#include "cm_signaling_version.h"
#include "SleDliLayerAdapter.h"
#include "ssap_type.h"
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

        sdfBuff = (SDF_Buff_S *)SDF_MemZalloc(sizeof(SDF_Buff_S) + sizeof(SSAP_PduCallMethodReq_S));
        sdfBuff->buffLen = sizeof(SSAP_PduCallMethodReq_S);
        sdfBuff->dataLen = sizeof(SSAP_PduCallMethodReq_S);
        buf = SDF_DataOffset(sdfBuff);
        SSAP_PduCallMethodReq_S *callMethodMsg = (SSAP_PduCallMethodReq_S *)buf;
        callMethodMsg->ctrl.fragment = 0x03;
        callMethodMsg->handle = provider.ConsumeIntegral<uint16_t>();

        SSAPS_MethodCmdHandle(link, sdfBuff);
        SDF_MemFree(sdfBuff);

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
        readReqItem->handle = (size % FUZZ_EIGHT);
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