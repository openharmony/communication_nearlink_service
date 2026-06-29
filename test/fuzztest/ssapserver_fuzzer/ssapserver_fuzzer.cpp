/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include <thread>
#include "fuzzer/FuzzedDataProvider.h"
#include "ssapserver_fuzzer.h"
#include "nearlink_native_token_mock.h"
#include "../nl_utils/fuzztest_utils.h"
#include "nearlink_ssap_server_stub.h"
#include "nearlink_ssap_server_server.h"
#include "nearlink_host_server.h"
#include "nearlink_ssap_property.h"
#include "nearlink_ssap_event.h"
#include "ThreadUtil.h"
#include "log.h"
#include "securec.h"
#include "raw_address.h"
#include "ssaps_server.h"
#include "ssaps_service.h"
#include "ssaps_server_api.h"
#include "sdf_mem.h"

#define private public
#include "ssap_server_stack_adapter.h"

using namespace std;
using namespace OHOS::Nearlink;
namespace OHOS {

class MockNearlinkSsapServerCallbackStub : public IRemoteStub<INearlinkSsapServerCallback> {
public:
    void OnMtuChanged(const NearlinkSsapDevice &device, uint16_t mtu) {}
    void OnAddService(const NearlinkSsapServiceParcel &service, int ret) {}
    void OnPropertyReadRequest(
        const NearlinkSsapDevice &device, const NearlinkSsapPropertyParcel &property, int ret) {}
    void OnDescriptorReadRequest(
        const NearlinkSsapDevice &device, const NearlinkSsapDescriptorParcel &descriptor, int ret) {}
    void OnPropertyWriteRequest(
        const NearlinkSsapDevice &device, const NearlinkSsapPropertyParcel &property, int ret) {}
    void OnDescriptorWriteRequest(
        const NearlinkSsapDevice &device, const NearlinkSsapDescriptorParcel &descriptor, int ret) {}
    void OnNotifyPropertyChanged(
        const NearlinkSsapDevice &device, const Uuid &uuid, uint16_t handle, int ret) {}
    void OnNotifyEventChanged(
        const NearlinkSsapDevice &device, const Uuid &uuid, uint16_t handle, int ret) {}
    void OnConnectionStateChanged(const NearlinkSsapDevice &device, uint8_t state, int reason) {}
};

namespace {
static NLSTK_SsapUuid_S g_uuid = {.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02}};
constexpr int FUZZ_TWO = 2;
constexpr int FUZZ_FIVE = 5;
constexpr int HOST_FUZZ_DELAY_100_MS = 100;
constexpr int HOST_FUZZ_DELAY_5000_MS = 5000;
constexpr uint32_t MESSAGE_SIZE = NearlinkSsapServerInterfaceCode::SSAP_SERVER_BUTT;
sptr<NearlinkSsapServerServer> g_ssapSever = new (std::nothrow) NearlinkSsapServerServer();
sptr<INearlinkSsapServerCallback> g_ssapServerCb = new (std::nothrow) MockNearlinkSsapServerCallbackStub();
ThreadUtil &g_threadUtil = ThreadUtil::GetInstance();
}

int32_t SsapServerOnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply)
{
    NL_CHECK_RETURN_RET(g_ssapSever, TRANSACTION_ERR, "g_ssapSever is nullptr");
    MessageOption option = {MessageOption::TF_SYNC};
    HILOGI("SsapServerOnRemoteRequest, cmd(%{public}d)", code);
    return g_ssapSever->OnRemoteRequest(code, data, reply, option);
}

void AddServiceFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSsapServerStub::GetDescriptor());
    data.WriteInt32(provider.ConsumeIntegral<int32_t>()); // appId

    NearlinkSsapServiceParcel svc;
    svc.isPrimary_ = true;
    svc.uuid_.ConvertFrom16Bits(provider.ConsumeIntegral<uint16_t>());
    data.WriteParcelable(&svc);

    int32_t ret = SsapServerOnRemoteRequest(NearlinkSsapServerInterfaceCode::SSAP_SERVER_ADD_SERVICE, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
}

void ClearServicesFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSsapServerStub::GetDescriptor());
    data.WriteInt32(provider.ConsumeIntegral<int32_t>()); // appId
    int32_t ret = SsapServerOnRemoteRequest(NearlinkSsapServerInterfaceCode::SSAP_SERVER_CLEAR_SERVICES, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
}

void CancelConnectionFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSsapServerStub::GetDescriptor());
    data.WriteInt32(provider.ConsumeIntegral<int32_t>()); // appId

    std::string addrStr = BuildAddressString(provider);
    RawAddress rawAddr(addrStr);
    SsapDevice dev(rawAddr, provider.ConsumeIntegral<uint8_t>()); // rawAdrr transport
    NearlinkSsapDevice devParcel(dev);
    data.WriteParcelable(&devParcel);

    int32_t ret = SsapServerOnRemoteRequest(
        NearlinkSsapServerInterfaceCode::SSAP_SERVER_CANCEL_CONNECTION, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
}

void RegisterApplicationFuzzTest(const uint8_t *fuzzData, size_t size)
{
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSsapServerStub::GetDescriptor());
    if (g_ssapServerCb != nullptr) {
        data.WriteRemoteObject(g_ssapServerCb->AsObject());
    }

    int32_t ret = SsapServerOnRemoteRequest(NearlinkSsapServerInterfaceCode::SSAP_SERVER_REGISTER, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
}

void DeregisterApplicationFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSsapServerStub::GetDescriptor());
    data.WriteInt32(provider.ConsumeIntegral<int32_t>()); // appId

    int32_t ret = SsapServerOnRemoteRequest(NearlinkSsapServerInterfaceCode::SSAP_SERVER_DEREGISTER, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
}

void NotifyClientFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSsapServerStub::GetDescriptor());
    data.WriteInt32(provider.ConsumeIntegral<int32_t>()); // appId

    UUID propertyUuid = UUID(provider.ConsumeIntegral<uint16_t>());
    SsapProperty property(0, propertyUuid, provider.ConsumeIntegral<uint32_t>(), 0);
    NearlinkSsapPropertyParcel propertyParcel = (NearlinkSsapPropertyParcel)Nearlink::Property(property.GetHandle(),
        Uuid::ConvertFrom128Bits(property.GetUuid().ConvertTo128Bits()), property.GetOperationIndication());
    data.WriteParcelable(&propertyParcel);

    std::string addrStr = BuildAddressString(provider);
    RawAddress rawAddr(addrStr);
    SsapDevice dev(rawAddr, provider.ConsumeIntegral<uint8_t>()); // rawAdrr transport
    NearlinkSsapDevice devParcel(dev);
    data.WriteParcelable(&devParcel);

    data.WriteBool(provider.ConsumeBool()); // needConfirm

    int32_t ret = SsapServerOnRemoteRequest(NearlinkSsapServerInterfaceCode::SSAP_SERVER_NOTIFY_CLIENT, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
}

void NotifyEventFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSsapServerStub::GetDescriptor());
    data.WriteInt32(provider.ConsumeIntegral<int32_t>()); // appId

    UUID eventUuid = UUID(provider.ConsumeIntegral<uint16_t>());
    uint16_t handle = provider.ConsumeIntegral<uint16_t>();
    int32_t type = provider.ConsumeIntegral<int32_t>();
    SsapEvent event(handle, type, eventUuid);
    NearlinkSsapEventParcel eve(Event(event.GetHandle(),
        Uuid::ConvertFrom128Bits(event.GetUuid().ConvertTo128Bits())));
    data.WriteParcelable(&eve);

    std::vector<uint8_t> value = provider.ConsumeBytes<uint8_t>(FUZZ_TWO);
    data.WriteUInt8Vector(value);

    std::string addrStr = BuildAddressString(provider);
    RawAddress rawAddr(addrStr);
    SsapDevice dev(rawAddr, provider.ConsumeIntegral<uint8_t>()); // rawAdrr transport
    NearlinkSsapDevice devParcel(dev);
    data.WriteParcelable(&devParcel);

    data.WriteBool(provider.ConsumeBool()); // needConfirm

    int32_t ret = SsapServerOnRemoteRequest(NearlinkSsapServerInterfaceCode::SSAP_SERVER_NOTIFY_EVENT, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
}

void SetPropertyValueFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSsapServerStub::GetDescriptor());
    data.WriteInt32(provider.ConsumeIntegral<int32_t>()); // appId

    UUID propertyUuid = UUID(provider.ConsumeIntegral<uint16_t>());
    SsapProperty property(0, propertyUuid, provider.ConsumeIntegral<uint32_t>(), 0);
    NearlinkSsapPropertyParcel propertyParcel = (NearlinkSsapPropertyParcel)Nearlink::Property(property.GetHandle(),
        Uuid::ConvertFrom128Bits(property.GetUuid().ConvertTo128Bits()), property.GetOperationIndication());
    data.WriteParcelable(&propertyParcel);

    int32_t ret = SsapServerOnRemoteRequest(NearlinkSsapServerInterfaceCode::SSAP_SERVER_SET_PROPERTY, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
}

void SetDescriptorValueFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSsapServerStub::GetDescriptor());
    data.WriteInt32(provider.ConsumeIntegral<int32_t>()); // appId

    uint16_t handle = provider.ConsumeIntegral<uint16_t>();
    int32_t type = provider.ConsumeIntegral<int32_t>();
    int32_t permission = provider.ConsumeIntegral<int32_t>();
    SsapDescriptor descriptor = SsapDescriptor(handle, type, permission);
    uint8_t vecLen = provider.ConsumeIntegral<uint8_t>();
    std::vector<uint8_t> dataVec = provider.ConsumeBytes<uint8_t>(vecLen);
    const uint8_t *value = static_cast<const uint8_t*>(dataVec.data());
    descriptor.SetValue(value, dataVec.size());
    NearlinkSsapDescriptorParcel desc =
        (NearlinkSsapDescriptorParcel)Nearlink::Descriptor(descriptor.GetHandle(), descriptor.GetDescriptorType());
    data.WriteParcelable(&desc);

    int32_t ret = SsapServerOnRemoteRequest(NearlinkSsapServerInterfaceCode::SSAP_SERVER_SET_DESCRIPTOR, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
}

void ConnectFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSsapServerStub::GetDescriptor());
    data.WriteInt32(provider.ConsumeIntegral<int32_t>()); // appId

    std::string addrStr = BuildAddressString(provider);
    RawAddress rawAddr(addrStr);
    SsapDevice dev(rawAddr, provider.ConsumeIntegral<uint8_t>()); // rawAdrr transport
    NearlinkSsapDevice devParcel(dev);
    data.WriteParcelable(&devParcel);

    data.WriteUint8(provider.ConsumeIntegral<uint8_t>()); // secureReq

    data.WriteBool(provider.ConsumeBool()); // autoConnect

    int32_t ret = SsapServerOnRemoteRequest(NearlinkSsapServerInterfaceCode::SSAP_SERVER_CONNECT, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
}

void RemoveServiceFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSsapServerStub::GetDescriptor());
    data.WriteInt32(provider.ConsumeIntegral<int32_t>()); // appId

    NearlinkSsapServiceParcel svc;
    svc.isPrimary_ = true;
    svc.uuid_.ConvertFrom16Bits(provider.ConsumeIntegral<uint16_t>());
    data.WriteParcelable(&svc);

    int32_t ret = SsapServerOnRemoteRequest(NearlinkSsapServerInterfaceCode::SSAP_SERVER_REMOVE_SERVICE, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
}

void AuthorizeResponseFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSsapServerStub::GetDescriptor());
    data.WriteInt32(provider.ConsumeIntegral<int32_t>()); // appId
    data.WriteUint16(provider.ConsumeIntegral<uint16_t>()); // requestId
    data.WriteBool(provider.ConsumeBool()); // allow

    int32_t ret = SsapServerOnRemoteRequest(
        NearlinkSsapServerInterfaceCode::SSAP_SERVER_AUTHORIZE_RESPONSE, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
}

void SsapServerFuzzTest(const uint8_t* fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    uint32_t code = (provider.ConsumeIntegral<uint32_t>() % MESSAGE_SIZE);
    auto addr = BuildAddressString(provider);

    MessageParcel data;
    MessageParcel reply;

    std::u16string descriptor = NearlinkSsapServerStub::GetDescriptor();
    data.WriteInterfaceToken(descriptor);
    data.WriteInt32(provider.ConsumeIntegral<int32_t>());
    data.WriteString(addr.c_str());
    data.RewindRead(0);

    int32_t ret = SsapServerOnRemoteRequest(code, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
}

static void AddService()
{
    SSAP_ParamAddService_S *serviceParam = (SSAP_ParamAddService_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddService_S));
    serviceParam->serviceType = ITEM_TYPE_STD_PRIMARY_SERVICE;
    (void)memcpy_s(&serviceParam->uuid, sizeof(NLSTK_SsapUuid_S), &g_uuid, sizeof(NLSTK_SsapUuid_S));
    SSAP_CacheService(serviceParam);
    SDF_MemFree(serviceParam);
    SSAP_ParamAddDescriptor_S *descParam =
        (SSAP_ParamAddDescriptor_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddDescriptor_S) + 1);
    descParam->type = DESC_TYPE_PROPERTY_INSTRUCTION;
    descParam->val.len = 1;
    descParam->val.value[0] = 0xEE;
    SSAP_CacheDescriptor(descParam);
    SDF_MemFree(descParam);
    SSAP_StartService(NULL);
}

void OnMtuChangedFuzzTest(const uint8_t* fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    SLE_Addr_S *addr = (SLE_Addr_S *)malloc(sizeof(SLE_Addr_S));

    int appId = provider.ConsumeIntegral<int>();
    addr->type = provider.ConsumeIntegral<uint8_t>();
    for (int i = 0; i <= FUZZ_FIVE; i++) {
        addr->addr[i] = provider.ConsumeIntegral<uint8_t>();
    }
    uint16_t mtu = provider.ConsumeIntegral<uint16_t>();
    SsapServerStackAdapter::OnMtuChanged(appId, addr, mtu);
    free(addr);
}

void OnAddServiceFuzzTest(const uint8_t* fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    AddService();
    NLSTK_Errcode_E ret = NLSTK_ERRCODE_SUCCESS;
    SSAP_Service_S *service = (SSAP_Service_S *)malloc(sizeof(SSAP_Service_S));

    int appId = provider.ConsumeIntegral<int>();
    service->appId = provider.ConsumeIntegral<int32_t>();
    service->handle = provider.ConsumeIntegral<uint16_t>();
    service->endHandle = provider.ConsumeIntegral<uint16_t>();
    service->uuid = g_uuid;
    service->properties = SSAPS_GetServices();
    service->serviceType = ITEM_TYPE_MAX;
    service->protocol = SSAP_SERVICE_TYPE_GLE;
    SsapServerStackAdapter::OnAddService(appId, service, ret);
    free(service);
}

void OnSetPropertyValueFuzzTest(const uint8_t* fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    NLSTK_Errcode_E ret = NLSTK_ERRCODE_SUCCESS;
    NLSTK_SsapServerOnSetPropertyParam_S *param =
        (NLSTK_SsapServerOnSetPropertyParam_S *)malloc(sizeof(NLSTK_SsapServerOnSetPropertyParam_S));

    int appId = provider.ConsumeIntegral<int>();
    param->handle = provider.ConsumeIntegral<uint16_t>();
    param->uuid = g_uuid;
    SsapServerStackAdapter::OnSetPropertyValue(appId, param, ret);
    free(param);
}

void OnSetDescriptorValueFuzzTest(const uint8_t* fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    NLSTK_Errcode_E ret = NLSTK_ERRCODE_SUCCESS;
    NLSTK_SsapServerOnSetDescriptorParam_S *param =
        (NLSTK_SsapServerOnSetDescriptorParam_S *)malloc(sizeof(NLSTK_SsapServerOnSetDescriptorParam_S));

    int appId = provider.ConsumeIntegral<int>();
    param->handle = provider.ConsumeIntegral<uint16_t>();
    param->uuid = g_uuid;
    param->type = provider.ConsumeIntegral<uint8_t>();
    SsapServerStackAdapter::OnSetDescriptorValue(appId, param, ret);
    free(param);
}

void OnReadPropertyAuthorizeRequest(const uint8_t* fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    NLSTK_SsapServerReadPropertyInfo_S *param =
        (NLSTK_SsapServerReadPropertyInfo_S *)malloc(sizeof(NLSTK_SsapServerReadPropertyInfo_S));

    int appId = provider.ConsumeIntegral<int>();
    uint16_t requestId = provider.ConsumeIntegral<uint16_t>();
    param->addr.type = provider.ConsumeIntegral<uint8_t>();
    for (int i = 0; i <= FUZZ_FIVE; i++) {
        param->addr.addr[i] = provider.ConsumeIntegral<uint8_t>();
    }
    param->uuid = g_uuid;
    param->handle = provider.ConsumeIntegral<uint16_t>();
    SsapServerStackAdapter::OnReadPropertyAuthorizeRequest(appId, requestId, param);
    free(param);
}

void OnReadDescriptorAuthorizeRequestFuzzTest(const uint8_t* fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    NLSTK_SsapServerReadDescriptorInfo_S *param =
        (NLSTK_SsapServerReadDescriptorInfo_S *)malloc(sizeof(NLSTK_SsapServerReadDescriptorInfo_S));

    int appId = provider.ConsumeIntegral<int>();
    uint16_t requestId = provider.ConsumeIntegral<uint16_t>();
    param->addr.type = provider.ConsumeIntegral<uint8_t>();
    for (int i = 0; i <= FUZZ_FIVE; i++) {
        param->addr.addr[i] = provider.ConsumeIntegral<uint8_t>();
    }
    param->handle = provider.ConsumeIntegral<uint16_t>();
    param->uuid = g_uuid;
    param->type = provider.ConsumeIntegral<uint8_t>();
    SsapServerStackAdapter::OnReadDescriptorAuthorizeRequest(appId, requestId, param);
    free(param);
}

void OnWritePropertyAuthorizeRequestFuzzTest(const uint8_t* fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    NLSTK_VariableData_S value = {0};
    uint8_t dataLen = provider.ConsumeIntegral<uint8_t>();
    std::vector<uint8_t> vec = provider.ConsumeBytes<uint8_t>(dataLen);
    value.len = vec.size();
    value.data = vec.data();
    NLSTK_SsapServerWritePropertyInfo_S *param =
        (NLSTK_SsapServerWritePropertyInfo_S *)malloc(sizeof(NLSTK_SsapServerWritePropertyInfo_S));

    int appId = provider.ConsumeIntegral<int>();
    uint16_t requestId = provider.ConsumeIntegral<uint16_t>();
    param->addr.type = provider.ConsumeIntegral<uint8_t>();
    for (int i = 0; i <= FUZZ_FIVE; i++) {
        param->addr.addr[i] = provider.ConsumeIntegral<uint8_t>();
    }
    param->handle = provider.ConsumeIntegral<uint16_t>();
    param->uuid = g_uuid;
    param->value = value;
    SsapServerStackAdapter::OnWritePropertyAuthorizeRequest(appId, requestId, param);
    free(param);
}

void OnWriteDescriptorAuthorizeRequestFuzzTest(const uint8_t* fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    NLSTK_VariableData_S value = {0};
    uint8_t dataLen = provider.ConsumeIntegral<uint8_t>();
    std::vector<uint8_t> vec = provider.ConsumeBytes<uint8_t>(dataLen);
    value.len = vec.size();
    value.data = vec.data();
    NLSTK_SsapServerWriteDescriptorInfo_S *param =
        (NLSTK_SsapServerWriteDescriptorInfo_S *)malloc(sizeof(NLSTK_SsapServerWriteDescriptorInfo_S));

    int appId = provider.ConsumeIntegral<int>();
    uint16_t requestId = provider.ConsumeIntegral<uint16_t>();
    param->addr.type = provider.ConsumeIntegral<uint8_t>();
    for (int i = 0; i <= FUZZ_FIVE; i++) {
        param->addr.addr[i] = provider.ConsumeIntegral<uint8_t>();
    }
    param->handle = provider.ConsumeIntegral<uint16_t>();
    param->uuid = g_uuid;
    param->type = provider.ConsumeIntegral<uint8_t>();
    param->value = value;
    SsapServerStackAdapter::OnWriteDescriptorAuthorizeRequest(appId, requestId, param);
    free(param);
}

void OnReadPropertyFuzzTest(const uint8_t* fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    NLSTK_SsapServerReadPropertyInfo_S *param =
        (NLSTK_SsapServerReadPropertyInfo_S *)malloc(sizeof(NLSTK_SsapServerReadPropertyInfo_S));

    int appId = provider.ConsumeIntegral<int>();
    param->addr.type = provider.ConsumeIntegral<uint8_t>();
    for (int i = 0; i <= FUZZ_FIVE; i++) {
        param->addr.addr[i] = provider.ConsumeIntegral<uint8_t>();
    }
    param->uuid = g_uuid;
    param->handle = provider.ConsumeIntegral<uint16_t>();
    SsapServerStackAdapter::OnReadProperty(appId, param);
    free(param);
}

void OnReadDescriptorFuzzTest(const uint8_t* fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    NLSTK_SsapServerReadDescriptorInfo *param =
        (NLSTK_SsapServerReadDescriptorInfo *)malloc(sizeof(NLSTK_SsapServerReadDescriptorInfo));

    int appId = provider.ConsumeIntegral<int>();
    param->addr.type = provider.ConsumeIntegral<uint8_t>();
    for (int i = 0; i <= FUZZ_FIVE; i++) {
        param->addr.addr[i] = provider.ConsumeIntegral<uint8_t>();
    }
    param->handle = provider.ConsumeIntegral<uint16_t>();
    param->uuid = g_uuid;
    param->type = provider.ConsumeIntegral<uint8_t>();
    SsapServerStackAdapter::OnReadDescriptor(appId, param);
    free(param);
}

void OnWritePropertyFuzzTest(const uint8_t* fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    NLSTK_VariableData_S value = {0};
    uint8_t dataLen = provider.ConsumeIntegral<uint8_t>();
    std::vector<uint8_t> vec = provider.ConsumeBytes<uint8_t>(dataLen);
    value.len = vec.size();
    value.data = vec.data();
    NLSTK_SsapServerWritePropertyInfo_S *param =
        (NLSTK_SsapServerWritePropertyInfo_S *)malloc(sizeof(NLSTK_SsapServerWritePropertyInfo_S));

    int appId = provider.ConsumeIntegral<int>();
    param->addr.type = provider.ConsumeIntegral<uint8_t>();
    for (int i = 0; i <= FUZZ_FIVE; i++) {
        param->addr.addr[i] = provider.ConsumeIntegral<uint8_t>();
    }
    param->handle = provider.ConsumeIntegral<uint16_t>();
    param->uuid = g_uuid;
    param->value = value;
    SsapServerStackAdapter::OnWriteProperty(appId, param);
    free(param);
}

void OnWriteDescriptorFuzzTest(const uint8_t* fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    NLSTK_VariableData_S value = {0};
    uint8_t dataLen = provider.ConsumeIntegral<uint8_t>();
    std::vector<uint8_t> vec = provider.ConsumeBytes<uint8_t>(dataLen);
    value.len = vec.size();
    value.data = vec.data();
    NLSTK_SsapServerWriteDescriptorInfo_S *param =
        (NLSTK_SsapServerWriteDescriptorInfo_S *)malloc(sizeof(NLSTK_SsapServerWriteDescriptorInfo_S));

    int appId = provider.ConsumeIntegral<int>();
    param->addr.type = provider.ConsumeIntegral<uint8_t>();
    for (int i = 0; i <= FUZZ_FIVE; i++) {
        param->addr.addr[i] = provider.ConsumeIntegral<uint8_t>();
    }
    param->handle = provider.ConsumeIntegral<uint16_t>();
    param->uuid = g_uuid;
    param->type = provider.ConsumeIntegral<uint8_t>();
    param->value = value;
    SsapServerStackAdapter::OnWriteDescriptor(appId, param);
    free(param);
}

void OnNotifyPropertyFuzzTest(const uint8_t* fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    NLSTK_Errcode_E ret = NLSTK_ERRCODE_SUCCESS;
    NLSTK_VariableData_S value = {0};
    uint8_t dataLen = provider.ConsumeIntegral<uint8_t>();
    std::vector<uint8_t> vec = provider.ConsumeBytes<uint8_t>(dataLen);
    value.len = vec.size();
    value.data = vec.data();
    NLSTK_SsapServerOnNotifyPropertyParam_S *param =
        (NLSTK_SsapServerOnNotifyPropertyParam_S *)malloc(sizeof(NLSTK_SsapServerOnNotifyPropertyParam_S));

    int appId = provider.ConsumeIntegral<int>();
    param->addr.type = provider.ConsumeIntegral<uint8_t>();
    for (int i = 0; i <= FUZZ_FIVE; i++) {
        param->addr.addr[i] = provider.ConsumeIntegral<uint8_t>();
    }
    param->handle = provider.ConsumeIntegral<uint16_t>();
    param->uuid = g_uuid;
    param->value = value;
    SsapServerStackAdapter::OnNotifyProperty(appId, param, ret);
    free(param);
}

void OnConnectionStateChanged(const uint8_t* fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    NLSTK_Errcode_E ret = NLSTK_ERRCODE_SUCCESS;
    SLE_Addr_S *addr = (SLE_Addr_S *)malloc(sizeof(SLE_Addr_S));

    int appId = provider.ConsumeIntegral<int>();
    addr->type = provider.ConsumeIntegral<uint8_t>();
    for (int i = 0; i <= FUZZ_FIVE; i++) {
        addr->addr[i] = provider.ConsumeIntegral<uint8_t>();
    }
    uint8_t state = provider.ConsumeIntegral<uint8_t>();
    int reason = provider.ConsumeIntegral<int>();
    SsapServerStackAdapter::OnConnectionStateChanged(appId, addr, state, ret, reason);
    free(addr);
}
}

// fuzzer init
extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void) argc;
    (void) argv;
    // mock token
    NearlinkMockNativeToken mock("nearlink_service");

    OHOS::sptr<NearlinkHostServer> hostServer = NearlinkHostServer::GetInstance();
    if (hostServer == nullptr) {
        return 0;
    }
    HILOGI("SsapServerFuzzTest OnStart");
    hostServer->OnStart();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_100_MS));
    HILOGI("SsapServerFuzzTest EnableSle");
    hostServer->EnableSle();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_5000_MS));
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr) {
        return 0;
    }

    OHOS::g_threadUtil.InitThreadStateMap();
    OHOS::RegisterApplicationFuzzTest(data, size);
    OHOS::AddServiceFuzzTest(data, size);
    OHOS::SetDescriptorValueFuzzTest(data, size);
    OHOS::CancelConnectionFuzzTest(data, size);
    OHOS::NotifyClientFuzzTest(data, size);
    OHOS::NotifyEventFuzzTest(data, size);
    OHOS::SetPropertyValueFuzzTest(data, size);
    OHOS::ConnectFuzzTest(data, size);
    OHOS::AuthorizeResponseFuzzTest(data, size);

    OHOS::OnMtuChangedFuzzTest(data, size);
    OHOS::OnAddServiceFuzzTest(data, size);
    OHOS::OnSetPropertyValueFuzzTest(data, size);
    OHOS::OnSetDescriptorValueFuzzTest(data, size);
    OHOS::OnReadPropertyAuthorizeRequest(data, size);
    OHOS::OnReadDescriptorAuthorizeRequestFuzzTest(data, size);
    OHOS::OnWritePropertyAuthorizeRequestFuzzTest(data, size);
    OHOS::OnWriteDescriptorAuthorizeRequestFuzzTest(data, size);
    OHOS::OnReadPropertyFuzzTest(data, size);
    OHOS::OnReadDescriptorFuzzTest(data, size);
    OHOS::OnWritePropertyFuzzTest(data, size);
    OHOS::OnWriteDescriptorFuzzTest(data, size);
    OHOS::OnNotifyPropertyFuzzTest(data, size);
    OHOS::OnConnectionStateChanged(data, size);

    OHOS::SsapServerFuzzTest(data, size);
    OHOS::RemoveServiceFuzzTest(data, size);
    OHOS::DeregisterApplicationFuzzTest(data, size);
    OHOS::ClearServicesFuzzTest(data, size);
    OHOS::g_threadUtil.ClearThreadStateMap();
    return 0;
}
#undef private