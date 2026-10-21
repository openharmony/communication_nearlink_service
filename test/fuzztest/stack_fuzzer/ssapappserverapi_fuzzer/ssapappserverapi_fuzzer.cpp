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
#include "sdf_mem.h"
#include "sdf_thread.h"
#include "sdf_evc.h"
#include "nlstk_log.h"
#include "nlstk_init_api.h"
#include "nlstk_schedule.h"
#include "nlstk_ssap_app_server.h"
#include "ssap_handle.h"
#include "ssapappserverapi_fuzzer.h"
#include "fuzzer/FuzzedDataProvider.h"

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

namespace OHOS {

    int32_t appId = -1;

    /* 已注册服务的 128 位 UUID（Bluetooth SIG 基址）；AddServices/CheckExist 共用，
     * CheckServiceExistByUuid 用同 UUID 才能命中 exist=true 分支。 */
    static const uint8_t g_fuzzServiceUuid[SSAP_UUID128_LEN] = {
        0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80,
        0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };

    void FuzzSsapServerRegApp(void)
    {
        NLSTK_SsapAppServerCb_S cb = {0};
        NLSTK_SsapServerRegApp(&cb, &appId);
    }

    void FuzzSsapServerDeRegApp(void)
    {
        NLSTK_SsapServerDeregisterApplication(appId);
    }

    void FuzzSsapServerSetMtu(uint8_t* data, size_t size)
    {
        uint16_t mtu = data[0];
        NLSTK_SsapServerSetMtu(mtu);
    }

    void FuzzSsapServerAddServices(void)
    {
        /* 参数填实：全零参数（无 uuid/operation/permission/descriptor）时属性
         * 生成与句柄分配路径浅层即返。描述符带 CLIENT_CONFIG+可写指示，
         * property/method 各带 uuid+permission。首个服务句柄从
         * SSAP_HANDLE_MIN(0x10) 起确定：0x10=服务 0x11=属性 0x12=方法 0x13=事件。 */
        static uint16_t cccdValue = 0x0001;
        NLSTK_SsapServiceDescriptorParam_S descriptor = {0};
        descriptor.type = DESC_TYPE_CLIENT_CONFIG;
        descriptor.operation.operationValue = SSAP_OPERATE_INDICATION_DESCRIPTOR_CLIENT_CONFIGURATION_WRITE;
        descriptor.val.data = (uint8_t *)&cccdValue;
        descriptor.val.len = sizeof(cccdValue);

        NLSTK_SsapServicePropertyParam_S property = {0};
        property.type = ITEM_TYPE_STD_PROPERTY;
        (void)memcpy_s(property.uuid.uuid, SSAP_UUID128_LEN, g_fuzzServiceUuid, SSAP_UUID128_LEN);
        property.operation.operationValue = SSAP_OPERATE_INDICATION_READ | SSAP_OPERATE_INDICATION_WRITE |
            SSAP_OPERATE_INDICATION_NOTIFY;
        property.permission.permissionValue = SSAP_PERMISSION_NO_NEED;
        property.descriptorNum = 1;
        property.descriptors = &descriptor;

        NLSTK_SsapServiceMethodParam_S method = {0};
        method.type = ITEM_TYPE_STD_METHOD;
        (void)memcpy_s(method.uuid.uuid, SSAP_UUID128_LEN, g_fuzzServiceUuid, SSAP_UUID128_LEN);
        method.permission.permissionValue = SSAP_PERMISSION_NO_NEED;

        NLSTK_SsapServiceEventParam_S event = {0};
        event.type = ITEM_TYPE_STD_EVENT;
        (void)memcpy_s(event.uuid.uuid, SSAP_UUID128_LEN, g_fuzzServiceUuid, SSAP_UUID128_LEN);

        NLSTK_ServiceParam_S service = {0};
        (void)memcpy_s(service.serviceStatement.uuid.uuid, SSAP_UUID128_LEN, g_fuzzServiceUuid, SSAP_UUID128_LEN);
        service.serviceStatement.serviceType = ITEM_TYPE_STD_PRIMARY_SERVICE;
        service.servicePropertyNum = 1;
        service.property = &property;
        service.serviceMethodNum = 1;
        service.method = &method;
        service.serviceEventNum = 1;
        service.event = &event;
        NLSTK_SsapServerAddService(appId, &service);
    }

    void FuzzSsapServerRemoveServices(uint8_t* data, size_t size)
    {
        /* 用真实句柄域（0x10~0x13），data 仅决定命中哪个已注册条目。 */
        uint16_t handle = SSAP_HANDLE_MIN + (uint16_t)(data[0] % FUZZ_FOUR);
        NLSTK_SsapServerRemoveService(appId, handle);
    }

    void FuzzSsapServerClearServices(void)
    {
        NLSTK_SsapServerClearServices(appId);
    }

    void FuzzSsapServerAuthorizeResult(uint8_t* data, size_t size)
    { 
        uint16_t requestId = data[0];
        bool allow = true;
        NLSTK_SsapServerAuthorizeResult(appId, requestId, allow);
    }

    void FuzzSsapServerUpdatePropertyValue(uint8_t* data, size_t size)
    {
        /* 属性句柄=0x11（首个服务 0x10 之后第一个属性），保留 fuzz 抖动覆盖邻句柄。 */
        uint16_t handle = SSAP_HANDLE_MIN + 1 + (uint16_t)(data[0] % FUZZ_THREE);
        NLSTK_VariableData_S *value;
        value = (NLSTK_VariableData_S *)SDF_MemZalloc(sizeof(NLSTK_VariableData_S) + size);
        value->len = size;
        NLSTK_SsapServerUpdatePropertyValue(appId, handle, value);
    }

    void FuzzSsapServerUpdateDescriptorValue(uint8_t* data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        uint16_t handle = SSAP_HANDLE_MIN + 1 + (uint16_t)(provider.ConsumeIntegral<uint8_t>() % FUZZ_THREE);
        uint8_t type = DESC_TYPE_CLIENT_CONFIG;
        NLSTK_VariableData_S *value;
        value = (NLSTK_VariableData_S *)SDF_MemZalloc(sizeof(NLSTK_VariableData_S) + size);
        value->len = size;
        NLSTK_SsapServerUpdateDescriptorValue(appId, handle, type, value);
    }

    void FuzzSsapServerUpdateAndNotifyProperty(uint8_t* data, size_t size)
    {
        if (size < FUZZ_SIX) {
            return;
        }

        uint16_t handle = SSAP_HANDLE_MIN + 1 + (uint16_t)(data[0] % FUZZ_THREE);
        SLE_Addr_S addr = {
            .type = PUBLIC_ADDRESS,
            .addr = { data[0], data[1], data[2], data[3], data[4], data[5] },
        };
        NLSTK_VariableData_S *value;
        value = (NLSTK_VariableData_S *)SDF_MemZalloc(sizeof(NLSTK_VariableData_S) + size);
        value->len = size;
        NLSTK_SsapServerUpdateAndNotifyProperty(appId, handle, &addr, value);
    }

    void FuzzSsapServerCheckServiceExistByUuid(void)
    {
        NLSTK_SsapUuid_S uuid = {0};
        (void)memcpy_s(uuid.uuid, SSAP_UUID128_LEN, g_fuzzServiceUuid, SSAP_UUID128_LEN);
        bool isExist;
        NLSTK_SsapServerCheckServiceExistByUuid(appId, &uuid, &isExist);
    }

    void FuzzSsapServerSendMethodCallRes(uint8_t* data, size_t size)
    {
        uint16_t requestId = data[0];
        NLSTK_VariableData_S *value;
        value = (NLSTK_VariableData_S *)SDF_MemZalloc(sizeof(NLSTK_VariableData_S) + size);
        value->len = size;
        NLSTK_SsapServerSendMethodCallRes(appId, requestId, value);
    }

    void FuzzStackServerApiDataProc(uint8_t* data, size_t size)
    {
        if (data == nullptr || size < 1) {
            return;
        }
        FuzzSsapServerRegApp();
        FuzzSsapServerSetMtu(data, size);
        FuzzSsapServerAddServices();
        FuzzSsapServerRemoveServices(data, size);
        FuzzSsapServerClearServices();
        FuzzSsapServerAuthorizeResult(data, size);
        FuzzSsapServerUpdatePropertyValue(data, size);
        FuzzSsapServerUpdateDescriptorValue(data, size);
        FuzzSsapServerUpdateAndNotifyProperty(data, size);
        FuzzSsapServerCheckServiceExistByUuid();
        FuzzSsapServerSendMethodCallRes(data, size);
        FuzzSsapServerDeRegApp();
    }
}

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void) argc;
    (void) argv;
    /* 补调度三件套：RegApp 走 SchedulePostTaskBlocked、AddService 走
     * SchedulePostTask，调度器不活则 appId 无效、全部 UAPI 直接失败；
     * InitStack 回滚会销毁调度器，末尾 ScheduleEnable 幂等重建（照 uapi 模式）。 */
    (void)NLSTK_InitStack();
    (void)NLSTK_EnableStack();
    (void)SDF_ThreadInit(10);
    (void)SDF_EvcInit();
    (void)ScheduleEnable();
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