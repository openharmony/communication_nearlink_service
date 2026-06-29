/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include "sdf_mem.h"
#include "cm_def.h"

#include "ssap_server_find_fuzzer.h"
#include "ssaps_server.h"
#include "ssaps_server_api.h"
#include "ssaps_service.h"
#include "ssap_link.h"
#include "ssap_manager.h"
#include "ssap_common.h"

extern "C" void CM_StubDeviceType(uint8_t type);

static NLSTK_SsapUuid_S g_uuid1 = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01, 0x02};
static NLSTK_SsapUuid_S g_uuid2 = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
    0x0c, 0x0d, 0x0e, 0x0f, 0x10};
static NLSTK_SsapUuid_S g_uuid3 = {0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c,
    0x0d, 0x0e, 0x0f, 0x10, 0x11};
static NLSTK_SsapUuid_S g_uuid4 = {0x10, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00,
    0x0a, 0x0b, 0x0c, 0x0d, 0x0e};

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
        if (size < 2) {
            return;
        }

        SSAP_LinkInit();
        SSAPS_ServiceInit();
        AddService1();
        AddService2();

        SLE_Addr_S sleAddr = {.type = PUBLIC_ADDRESS, .addr = {0x03, 0x03, 0x04, 0x04, 0x05, 0x05}};
        (void)SSAP_CreateSsapLink(&sleAddr, 0, SendFunc);

        if (rand() %2 == 0) {
            CM_StubDeviceType(CM_DEVTYPE_NEW);
        } else {
            CM_StubDeviceType(CM_DEVTYPE_OLD);
        }

        SDF_Buff_S *sdfBuff = SDF_BuffNewWithReserve(size + 1);
        uint8_t *buf = SDF_BuffAppend(sdfBuff, size + 1);
        uint8_t reqOpcode = rand() % 2 == 0 ? SSAP_FIND_STRUCTURE_REQ : SSAP_FIND_STRUCTURE_BY_UUID_REQ;
        buf[0] = reqOpcode;
        (void)memcpy_s(buf + 1, size, data, size);

        DTAP_Data_Info_S info = {0};
        SSAP_Recv(&info, sdfBuff);

        SDF_BuffFree(sdfBuff);
        SSAP_DeleteSsapLinkByAddr(&sleAddr);

        SSAP_LinkDeInit();
        SSAPS_ServiceDeInit();
    }
}

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void) argc;
    (void) argv;
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
    OHOS::FuzzDliSsapRecvPkt(static_cast<uint8_t *>(fuzzData), size);
    SDF_MemFree(fuzzData);
    return 0;
}