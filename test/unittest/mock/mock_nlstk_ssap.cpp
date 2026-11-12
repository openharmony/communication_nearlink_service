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

// NLSTK ssap C-API 边界 mock：链接抢占——本文件定义优先于 libnearlink_stack.so；发号/清理语义对齐真栈
#include <array>
#include <cstdint>
#include <map>
#include <set>
#include <vector>

#include "log.h"
#include "nlstk_public_define_ext.h"
#include "nlstk_ssap_app_client.h"
#include "nlstk_ssap_app_server.h"
#include "ssaps_server_app.h"

#include "mock_nlstk_ssap.h"

namespace {
// 无需加锁：调用都来自 ssap 串行队列或阻塞等待中的测试线程
constexpr int32_t CLIENT_APP_MAX = NLSTK_SSAP_CLIENT_APP_MAX_NUM;
constexpr int32_t SERVER_APP_MAX = NLSTK_SSAP_SERVER_APP_MAX_NUM;
bool g_clientUsed[CLIENT_APP_MAX] = {false};
bool g_serverUsed[SERVER_APP_MAX] = {false};
using UuidKey = std::vector<uint8_t>;
std::map<int32_t, std::set<UuidKey>> g_serverServices;
std::map<int32_t, MockNlstkSsapAddedService> g_lastAddedService;

UuidKey ToKey(const NLSTK_SsapUuid_S *uuid)
{
    const uint8_t *begin = uuid->uuid;
    return UuidKey(begin, begin + sizeof(uuid->uuid));
}

int32_t AllocSlot(bool used[], int32_t max)
{
    for (int32_t i = 0; i < max; i++) {
        if (!used[i]) {
            used[i] = true;
            return i;
        }
    }
    return SSAP_APP_INVALID_ID;
}
} // namespace

const MockNlstkSsapAddedService *MockNlstkSsapGetLastAddedService(int32_t appId)
{
    auto it = g_lastAddedService.find(appId);
    return (it == g_lastAddedService.end()) ? nullptr : &it->second;
}

NLSTK_Errcode_E NLSTK_SsapClientRegApp(int32_t *appId, NLSTK_SsapAppClientCb_S *cb, SLE_Addr_S *addr)
{
    HILOGI("[Ssap Mocker] NLSTK_SsapClientRegApp");
    if (appId == nullptr || cb == nullptr || addr == nullptr) {
        return NLSTK_ERRCODE_POINTER_NULL;
    }
    int32_t id = AllocSlot(g_clientUsed, CLIENT_APP_MAX);
    if (id == SSAP_APP_INVALID_ID) {
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    *appId = id;
    return NLSTK_ERRCODE_SUCCESS;
}

void NLSTK_SsapClientDeregApp(int32_t appId)
{
    HILOGI("[Ssap Mocker] NLSTK_SsapClientDeregApp appId=%{public}d", appId);
    if (appId >= 0 && appId < CLIENT_APP_MAX) {
        g_clientUsed[appId] = false;
    }
}

NLSTK_Errcode_E NLSTK_SsapCleanClientApp(NLSTK_SsapClientCleanAppResultCb clientCleanAppResultCb)
{
    HILOGI("[Ssap Mocker] NLSTK_SsapCleanClientApp");
    for (int32_t i = 0; i < CLIENT_APP_MAX; i++) {
        g_clientUsed[i] = false;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SsapClientExchangeMtu(int32_t appId, uint16_t mtu)
{
    HILOGI("[Ssap Mocker] NLSTK_SsapClientExchangeMtu appId=%{public}d mtu=%{public}u", appId, mtu);
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SsapClientDiscoverServices(int32_t appId, uint16_t handleStart, uint16_t handleEnd,
    uint8_t type)
{
    HILOGI("[Ssap Mocker] NLSTK_SsapClientDiscoverServices appId=%{public}d", appId);
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SsapClientDiscoverServicesByUuid(int32_t appId, NLSTK_SsapUuid_S *uuid,
    uint16_t handleStart, uint16_t handleEnd, uint8_t type)
{
    HILOGI("[Ssap Mocker] NLSTK_SsapClientDiscoverServicesByUuid appId=%{public}d", appId);
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SsapClientGetServices(int32_t appId, NLSTK_SsapServ_S **service, uint16_t *serviceNum,
    NLSTK_SsapClientFreeFunc *func)
{
    HILOGI("[Ssap Mocker] NLSTK_SsapClientGetServices appId=%{public}d", appId);
    if (service != nullptr) {
        *service = nullptr;
    }
    if (serviceNum != nullptr) {
        *serviceNum = 0;
    }
    if (func != nullptr) {
        *func = nullptr;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SsapClientGetServicesByUuid(int32_t appId, NLSTK_SsapUuid_S *uuid,
    NLSTK_SsapServ_S **service, uint16_t *serviceNum, NLSTK_SsapClientFreeFunc *func)
{
    HILOGI("[Ssap Mocker] NLSTK_SsapClientGetServicesByUuid appId=%{public}d", appId);
    if (service != nullptr) {
        *service = nullptr;
    }
    if (serviceNum != nullptr) {
        *serviceNum = 0;
    }
    if (func != nullptr) {
        *func = nullptr;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SsapClientReadProperty(int32_t appId, uint16_t handle)
{
    HILOGI("[Ssap Mocker] NLSTK_SsapClientReadProperty appId=%{public}d handle=%{public}u", appId, handle);
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SsapClientReadPropertyByUuid(int32_t appId, NLSTK_SsapUuid_S *uuid,
    uint16_t handleStart, uint16_t handleEnd)
{
    HILOGI("[Ssap Mocker] NLSTK_SsapClientReadPropertyByUuid appId=%{public}d", appId);
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SsapClientReadDescriptor(int32_t appId, uint16_t handle, uint8_t type)
{
    HILOGI("[Ssap Mocker] NLSTK_SsapClientReadDescriptor appId=%{public}d handle=%{public}u", appId, handle);
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SsapClientWriteProperty(int32_t appId, uint16_t handle, NLSTK_VariableData_S *value,
    bool withoutRsp)
{
    HILOGI("[Ssap Mocker] NLSTK_SsapClientWriteProperty appId=%{public}d handle=%{public}u", appId, handle);
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SsapClientWriteDescriptor(int32_t appId, uint16_t handle, NLSTK_VariableData_S *value,
    uint8_t type, bool withoutRsp)
{
    HILOGI("[Ssap Mocker] NLSTK_SsapClientWriteDescriptor appId=%{public}d handle=%{public}u", appId, handle);
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SsapClientGetPropertyNtf(int32_t appId, uint16_t handle)
{
    HILOGI("[Ssap Mocker] NLSTK_SsapClientGetPropertyNtf appId=%{public}d handle=%{public}u", appId, handle);
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SsapClientGetPropertyInd(int32_t appId, uint16_t handle)
{
    HILOGI("[Ssap Mocker] NLSTK_SsapClientGetPropertyInd appId=%{public}d handle=%{public}u", appId, handle);
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SsapClientSetPropertyNtf(int32_t appId, uint16_t handle, bool enable)
{
    HILOGI("[Ssap Mocker] NLSTK_SsapClientSetPropertyNtf appId=%{public}d handle=%{public}u", appId, handle);
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SsapClientSetPropertyInd(int32_t appId, uint16_t handle, bool enable)
{
    HILOGI("[Ssap Mocker] NLSTK_SsapClientSetPropertyInd appId=%{public}d handle=%{public}u", appId, handle);
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SsapClientCallMethod(int32_t appId, uint16_t handle, NLSTK_VariableData_S *value,
    bool withoutRsp)
{
    HILOGI("[Ssap Mocker] NLSTK_SsapClientCallMethod appId=%{public}d handle=%{public}u", appId, handle);
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SsapClientConnect(int32_t appId)
{
    HILOGI("[Ssap Mocker] NLSTK_SsapClientConnect appId=%{public}d", appId);
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SsapClientDisconnect(int32_t appId)
{
    HILOGI("[Ssap Mocker] NLSTK_SsapClientDisconnect appId=%{public}d", appId);
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SsapServerRegApp(NLSTK_SsapAppServerCb_S *cb, int32_t *appId)
{
    HILOGI("[Ssap Mocker] NLSTK_SsapServerRegApp");
    if (cb == nullptr || appId == nullptr) {
        return NLSTK_ERRCODE_POINTER_NULL;
    }
    int32_t id = AllocSlot(g_serverUsed, SERVER_APP_MAX);
    if (id == SSAP_APP_INVALID_ID) {
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    *appId = id;
    return NLSTK_ERRCODE_SUCCESS;
}

void NLSTK_SsapServerDeregisterApplication(int32_t appId)
{
    HILOGI("[Ssap Mocker] NLSTK_SsapServerDeregisterApplication appId=%{public}d", appId);
    if (appId >= 0 && appId < SERVER_APP_MAX) {
        g_serverUsed[appId] = false;
        g_serverServices.erase(appId);
        g_lastAddedService.erase(appId);
    }
}

NLSTK_Errcode_E NLSTK_SsapCleanServerApp(NLSTK_SsapServerCleanAppResultCb serverCleanAppResultCb)
{
    HILOGI("[Ssap Mocker] NLSTK_SsapCleanServerApp");
    for (int32_t i = 0; i < SERVER_APP_MAX; i++) {
        g_serverUsed[i] = false;
    }
    g_serverServices.clear();
    g_lastAddedService.clear();
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SsapServerSetMtu(uint16_t mtu)
{
    HILOGI("[Ssap Mocker] NLSTK_SsapServerSetMtu mtu=%{public}u", mtu);
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SsapServerAddService(int32_t appId, const NLSTK_ServiceParam_S *service)
{
    HILOGI("[Ssap Mocker] NLSTK_SsapServerAddService appId=%{public}d", appId);
    if (service != nullptr) {
        g_serverServices[appId].insert(ToKey(&service->serviceStatement.uuid));
        MockNlstkSsapAddedService record;
        record.serviceUuid = service->serviceStatement.uuid;
        record.serviceType = service->serviceStatement.serviceType;
        for (uint16_t i = 0; i < service->servicePropertyNum && service->property != nullptr; i++) {
            record.propertyUuids.push_back(service->property[i].uuid);
            record.propertyTypes.push_back(service->property[i].type);
        }
        g_lastAddedService[appId] = std::move(record);
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SsapServerRemoveService(int32_t appId, uint16_t handle)
{
    HILOGI("[Ssap Mocker] NLSTK_SsapServerRemoveService appId=%{public}d handle=%{public}u", appId, handle);
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SsapServerClearServices(int32_t appId)
{
    HILOGI("[Ssap Mocker] NLSTK_SsapServerClearServices appId=%{public}d", appId);
    g_serverServices.erase(appId);
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SsapServerAuthorizeResult(int32_t appId, uint16_t requestId, bool allow)
{
    HILOGI("[Ssap Mocker] NLSTK_SsapServerAuthorizeResult appId=%{public}d", appId);
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SsapServerUpdatePropertyValue(int32_t appId, uint16_t handle, NLSTK_VariableData_S *value)
{
    HILOGI("[Ssap Mocker] NLSTK_SsapServerUpdatePropertyValue appId=%{public}d handle=%{public}u", appId,
        handle);
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SsapServerUpdateDescriptorValue(int32_t appId, uint16_t handle, uint8_t type,
    NLSTK_VariableData_S *value)
{
    HILOGI("[Ssap Mocker] NLSTK_SsapServerUpdateDescriptorValue appId=%{public}d handle=%{public}u", appId,
        handle);
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SsapServerUpdateAndNotifyProperty(int32_t appId, uint16_t handle, SLE_Addr_S *addr,
    NLSTK_VariableData_S *value)
{
    HILOGI("[Ssap Mocker] NLSTK_SsapServerUpdateAndNotifyProperty appId=%{public}d handle=%{public}u", appId,
        handle);
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SsapServerCheckServiceExistByUuid(int32_t appId, NLSTK_SsapUuid_S *uuid, bool *isExist)
{
    HILOGI("[Ssap Mocker] NLSTK_SsapServerCheckServiceExistByUuid appId=%{public}d", appId);
    if (uuid == nullptr || isExist == nullptr) {
        return NLSTK_ERRCODE_POINTER_NULL;
    }
    auto it = g_serverServices.find(appId);
    *isExist = (it != g_serverServices.end()) && (it->second.count(ToKey(uuid)) > 0);
    return NLSTK_ERRCODE_SUCCESS;
}
