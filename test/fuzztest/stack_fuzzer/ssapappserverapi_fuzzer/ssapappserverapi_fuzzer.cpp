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
#include "ssaps_service.h"
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

    /* SS-B2 观测记录：AddService 经调度异步执行，真实句柄域由产品回调在调度
     * 线程上报（本线程只读；uint16/bool 观测容忍时序交错）。后续调用统一
     * "真实句柄优先、原抖动兜底"，回调到/未到两种时序皆为被测路径。 */
    static uint16_t g_realSvcStart = 0;      /* onAddService：服务起始句柄 */
    static uint16_t g_realSvcEnd = 0;        /* onAddService：服务结束句柄 */
    static uint16_t g_realPropHandle = 0;    /* onAddService：首个属性句柄 */
    static uint16_t g_realMethodHandle = 0;  /* onAddService：首个方法句柄 */
    static uint16_t g_authRequestId = 0;     /* onWritePropertyAuthorizeRequest：真实 requestId */
    static bool g_addSvcOk = false;          /* onAddService 成功（ret==SUCCESS 且 service 非空） */
    static bool g_regAppOk = false;          /* onRegisterApp 成功 */
    static uint16_t g_lastCbHandle = 0;      /* 读/写/更新等业务回调的句柄落点 */
    static bool g_isExistObserved = false;   /* CheckExistByUuid 落点（Remove 后应为不存在，仅记录） */
    static uint16_t g_churnHandles[8] = {0}; /* HandleChurn 各轮 Add 上报的服务句柄序列 */
    static uint8_t g_churnCount = 0;
    static bool g_churnActive = false;       /* 仅 HandleChurn 期间记录句柄序列 */

    /* 静态观测回调：只读取/记录产品上报的真实值，不触碰产品侧内存生命周期。 */
    static void FuzzOnRegisterApp(int32_t id, NLSTK_Errcode_E ret)
    {
        (void)id;
        g_regAppOk = (ret == NLSTK_ERRCODE_SUCCESS);
    }

    static void FuzzOnAddService(int32_t id, SSAP_Service_S *service, NLSTK_Errcode_E ret)
    {
        (void)id;
        g_addSvcOk = (service != nullptr) && (ret == NLSTK_ERRCODE_SUCCESS);
        if (service == nullptr) {
            return;
        }
        /* 句柄域由产品分配后随 service 上报（ssaps_service.c:252-259）；向量
         * 元素为指针，照 ssaps_service.c 内 SDF_VectorElementAt 用法取出。 */
        g_realSvcStart = service->handle;
        g_realSvcEnd = service->endHandle;
        if (service->properties != nullptr && service->properties->size > 0) {
            SSAP_Property_S *property = (SSAP_Property_S *)SDF_VectorElementAt(service->properties, 0);
            if (property != nullptr) {
                g_realPropHandle = property->handle;
            }
        }
        if (service->methods != nullptr && service->methods->size > 0) {
            SSAP_Method_S *method = (SSAP_Method_S *)SDF_VectorElementAt(service->methods, 0);
            if (method != nullptr) {
                g_realMethodHandle = method->handle;
            }
        }
        if (g_churnActive && g_churnCount < sizeof(g_churnHandles) / sizeof(g_churnHandles[0])) {
            g_churnHandles[g_churnCount++] = service->handle;
        }
    }

    static void FuzzOnSetPropertyValue(int32_t id, NLSTK_SsapServerOnSetPropertyParam_S *param, NLSTK_Errcode_E ret)
    {
        (void)id;
        (void)ret;
        g_lastCbHandle = (param != nullptr) ? param->handle : 0;
    }

    static void FuzzOnReadProperty(int32_t id, NLSTK_SsapServerReadPropertyInfo_S *property)
    {
        (void)id;
        g_lastCbHandle = (property != nullptr) ? property->handle : 0;
    }

    static void FuzzOnWriteProperty(int32_t id, NLSTK_SsapServerWritePropertyInfo_S *property)
    {
        (void)id;
        g_lastCbHandle = (property != nullptr) ? property->handle : 0;
    }

    static void FuzzOnWriteAuthReq(int32_t id, uint16_t requestId, NLSTK_SsapServerWritePropertyInfo_S *property)
    {
        /* needAuth 挂起时产品上报真实 requestId（ssaps_server_app.c:435）；
         * 本工程无 SSAP 链路该回调正常不触发，记录以备 AuthorizeResult 复用。 */
        (void)id;
        g_authRequestId = requestId;
        g_lastCbHandle = (property != nullptr) ? property->handle : 0;
    }

    void FuzzSsapServerRegApp(void)
    {
        /* 填真实静态回调：AddService 异步走调度，真实句柄域只能经
         * onAddService(appId, service, ret) 观测（ssaps_server_app.c:287-308）。 */
        NLSTK_SsapAppServerCb_S cb = {0};
        cb.onRegisterApp = FuzzOnRegisterApp;
        cb.onAddService = FuzzOnAddService;
        cb.onSetPropertyValue = FuzzOnSetPropertyValue;
        cb.onReadProperty = FuzzOnReadProperty;
        cb.onWriteProperty = FuzzOnWriteProperty;
        cb.onWritePropertyAuthorizeRequest = FuzzOnWriteAuthReq;
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
         * SSAP_HANDLE_MIN(0x10) 起确定：0x10=服务 0x11=属性 0x12=方法
         * （事件不分配句柄，ssaps_service.c 注释"当前仅有Property,method"）。 */
        static uint16_t cccdValue = 0x0001;
        NLSTK_SsapServiceDescriptorParam_S descriptor = {};
        descriptor.type = DESC_TYPE_CLIENT_CONFIG;
        descriptor.operation.operationValue = SSAP_OPERATE_INDICATION_DESCRIPTOR_CLIENT_CONFIGURATION_WRITE;
        descriptor.val.data = (uint8_t *)&cccdValue;
        descriptor.val.len = sizeof(cccdValue);

        NLSTK_SsapServicePropertyParam_S property = {};
        property.type = ITEM_TYPE_STD_PROPERTY;
        (void)memcpy_s(property.uuid.uuid, SSAP_UUID128_LEN, g_fuzzServiceUuid, SSAP_UUID128_LEN);
        property.operation.operationValue = SSAP_OPERATE_INDICATION_READ | SSAP_OPERATE_INDICATION_WRITE |
            SSAP_OPERATE_INDICATION_NOTIFY;
        property.permission.permissionValue = SSAP_PERMISSION_NO_NEED;
        property.descriptorNum = 1;
        property.descriptors = &descriptor;

        NLSTK_SsapServiceMethodParam_S method = {};
        method.type = ITEM_TYPE_STD_METHOD;
        (void)memcpy_s(method.uuid.uuid, SSAP_UUID128_LEN, g_fuzzServiceUuid, SSAP_UUID128_LEN);
        method.permission.permissionValue = SSAP_PERMISSION_NO_NEED;

        NLSTK_SsapServiceEventParam_S event = {};
        event.type = ITEM_TYPE_STD_EVENT;
        (void)memcpy_s(event.uuid.uuid, SSAP_UUID128_LEN, g_fuzzServiceUuid, SSAP_UUID128_LEN);

        NLSTK_ServiceParam_S service = {};
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

    void FuzzSsapServerHandleChurn(uint8_t* data, size_t size)
    {
        /* SS-B3 句柄管理交错：Add→Remove 连打 N 轮（N=数据尾字节%8，FDP 整型
         * 自尾部消费）。UAPI 入口即深拷贝（SsapAllocServiceParam，
         * nlstk_ssap_app_server.c:149），故每轮在栈上重建局部参数即可；
         * Remove 取上一轮观测句柄（首轮 SSAP_HANDLE_MIN 兜底）——调度 FIFO
         * 下与分配器首次适应复用值吻合，未命中即"删除不存在句柄"路径。
         * g_churnHandles 序列数值相同＝释放合并+首次适应的复用语义，非泄漏
         * （ssap_handle.c:53-159）。 */
        FuzzedDataProvider provider(data, size);
        uint8_t rounds = provider.ConsumeIntegral<uint8_t>() % FUZZ_EIGHT;
        g_churnCount = 0;
        g_churnActive = true;
        for (uint8_t i = 0; i < rounds; i++) {
            NLSTK_SsapServicePropertyParam_S property = {};
            property.type = ITEM_TYPE_STD_PROPERTY;
            (void)memcpy_s(property.uuid.uuid, SSAP_UUID128_LEN, g_fuzzServiceUuid, SSAP_UUID128_LEN);
            property.permission.permissionValue = SSAP_PERMISSION_NO_NEED;

            NLSTK_ServiceParam_S service = {};
            (void)memcpy_s(service.serviceStatement.uuid.uuid, SSAP_UUID128_LEN, g_fuzzServiceUuid, SSAP_UUID128_LEN);
            service.serviceStatement.serviceType = ITEM_TYPE_STD_PRIMARY_SERVICE;
            service.servicePropertyNum = 1;
            service.property = &property;
            NLSTK_SsapServerAddService(appId, &service);
            uint16_t churnHandle = (g_churnCount > 0) ? g_churnHandles[g_churnCount - 1] : SSAP_HANDLE_MIN;
            NLSTK_SsapServerRemoveService(appId, churnHandle);
        }
        g_churnActive = false;
    }

    void FuzzSsapServerRemoveServices(uint8_t* data, size_t size)
    {
        /* 真实服务起始句柄优先（onAddService 观测），未观测到时保留
         * 原句柄域抖动（0x10~0x13），data 决定命中哪个已注册条目。 */
        uint16_t handle = (g_realSvcStart != 0) ? g_realSvcStart :
            SSAP_HANDLE_MIN + (uint16_t)(data[0] % FUZZ_FOUR);
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

    void FuzzSsapServerAddAuthServices(void)
    {
        /* 授权属性变体：主服务 permission 全 NO_NEED，挂起授权路径需要
         * SSAP_PERMISSION_AUTHORIZATION_NEED 的属性真实入表。本工程无 SSAP
         * 链路，远端写请求无法产生，onWritePropertyAuthorizeRequest 不会
         * 自然触发——授权挂起路径由 ssape2e_fuzzer 覆盖，本工程覆盖
         * AuthorizeResult 结果接口的错配/晚到。 */
        NLSTK_SsapServicePropertyParam_S authProperty = {};
        authProperty.type = ITEM_TYPE_STD_PROPERTY;
        (void)memcpy_s(authProperty.uuid.uuid, SSAP_UUID128_LEN, g_fuzzServiceUuid, SSAP_UUID128_LEN);
        authProperty.operation.operationValue = SSAP_OPERATE_INDICATION_READ | SSAP_OPERATE_INDICATION_WRITE;
        authProperty.permission.permissionValue = SSAP_PERMISSION_AUTHORIZATION_NEED;

        NLSTK_ServiceParam_S service = {};
        (void)memcpy_s(service.serviceStatement.uuid.uuid, SSAP_UUID128_LEN, g_fuzzServiceUuid, SSAP_UUID128_LEN);
        service.serviceStatement.serviceType = ITEM_TYPE_STD_PRIMARY_SERVICE;
        service.servicePropertyNum = 1;
        service.property = &authProperty;
        NLSTK_SsapServerAddService(appId, &service);
    }

    void FuzzSsapServerLateAuthorizeResult(void)
    {
        /* Add→在途→Remove→晚到 AuthorizeResult：先删首个真实服务再回授权
         * 结果，属性已删 → ITEM_INEXIST/找不到挂起项分支（ssaps_server.c:
         * 1486-1498；AuthorizeResult 按 requestId 全局查找、appId 仅范围
         * 校验，nlstk_ssap_app_server.c:234-247，找不到仅日志）。 */
        uint16_t handle = (g_realSvcStart != 0) ? g_realSvcStart : SSAP_HANDLE_MIN;
        NLSTK_SsapServerRemoveService(appId, handle);
        NLSTK_SsapServerAuthorizeResult(appId, g_authRequestId, true);
    }

    void FuzzSsapServerUpdatePropertyValue(uint8_t* data, size_t size)
    {
        /* Add 成功（g_addSvcOk）则用回调观测的真实属性句柄；未观测到时保留
         * SSAP_HANDLE_MIN+1 邻句柄抖动路径（覆盖错误句柄分支）。 */
        uint16_t handle = (g_addSvcOk && (g_realPropHandle != 0)) ? g_realPropHandle :
            SSAP_HANDLE_MIN + 1 + (uint16_t)(data[0] % FUZZ_THREE);
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
        g_isExistObserved = isExist;   /* Remove 后应转不存在，仅记录不断言 */
    }

    void FuzzSsapServerSendMethodCallRes(uint8_t* data, size_t size)
    {
        /* requestId 优先观测值（onWritePropertyAuthorizeRequest 记录过），
         * 否则 data[0] 走错误号路径。 */
        uint16_t requestId = (g_authRequestId != 0) ? g_authRequestId : data[0];
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
        FuzzSsapServerHandleChurn(data, size);
        FuzzSsapServerUpdatePropertyValue(data, size);
        FuzzSsapServerUpdateDescriptorValue(data, size);
        FuzzSsapServerUpdateAndNotifyProperty(data, size);
        FuzzSsapServerAddAuthServices();
        FuzzSsapServerAuthorizeResult(data, size);
        FuzzSsapServerLateAuthorizeResult();
        FuzzSsapServerSendMethodCallRes(data, size);
        FuzzSsapServerCheckServiceExistByUuid();
        FuzzSsapServerRemoveServices(data, size);
        FuzzSsapServerCheckServiceExistByUuid();
        FuzzSsapServerClearServices();
        FuzzSsapServerDeRegApp();
    }
}

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void) argc;
    (void) argv;
    /* 补调度三件套：RegApp 走 SchedulePostTaskBlocked、AddService 走
     * SchedulePostTask，调度器不活则 appId 无效、全部 UAPI 直接失败；
     * InitStack 回滚会销毁调度器，须重建（照 uapi 模式）。ext 失败时
     * SSAP_Init（ssap_manager.c:686）未运行：句柄分配器/服务向量缺失，
     * AddService 恒走无句柄异常分支（红蓝对抗实证）——照
     * ssape2e_fuzzer.cpp:190 显式初始化。 */
    bool stackInited = NLSTK_InitStack() == NLSTK_ERRCODE_SUCCESS;
    (void)SDF_ThreadInit(10);
    (void)SDF_EvcInit();
    (void)ScheduleEnable();
    if (stackInited) {
        (void)NLSTK_EnableStack();
    } else {
        (void)SSAPS_ServiceInit();
    }
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