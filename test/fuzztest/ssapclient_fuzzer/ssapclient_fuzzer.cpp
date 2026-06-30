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
#include <thread>
#include "fuzzer/FuzzedDataProvider.h"
#include "ssapclient_fuzzer.h"
#include "nearlink_native_token_mock.h"
#include "../nl_utils/fuzztest_utils.h"
#include "nearlink_ssap_client_stub.h"
#include "nearlink_ssap_client_server.h"
#include "nearlink_host_server.h"
#include "nearlink_ssap_property.h"
#include "nearlink_ssap_method.h"
#include "ThreadUtil.h"
#include "log.h"
#include "securec.h"
#include "raw_address.h"

#define private public
#include "ssap_client_stack_adapter.h"

using namespace std;
using namespace OHOS::Nearlink;
namespace OHOS {

class MockNearlinkSsapClientCallbackStub : public IRemoteStub<INearlinkSsapClientCallback> {
public:
    void OnConnectionStateChanged(int32_t state, int32_t newState) {}
    void OnPropertyChanged(const NearlinkSsapPropertyParcel &property) {}
    void OnReadProperty(int32_t ret, const NearlinkSsapPropertyParcel &property) {}
    void OnReadPropertiesByUuid(int32_t ret, const std::vector<NearlinkSsapPropertyParcel> &properties) {};
    void OnEventNotified(const NearlinkSsapEventParcel &event) {}
    void OnCallMethod(int32_t ret, const NearlinkSsapMethodParcel &method) {}
    void OnWriteProperty(int32_t ret, const NearlinkSsapPropertyParcel &property) {}
    void OnSetPropertyNotification(int32_t ret, bool enable, const NearlinkSsapPropertyParcel &property) {}
    void OnReadDescriptor(int32_t ret, const NearlinkSsapDescriptorParcel &descriptor) {}
    void OnWriteDescriptor(int32_t ret, const NearlinkSsapDescriptorParcel &descriptor) {}
    void OnMtuChanged(int32_t state, uint16_t mtu) {}
    void OnServicesDiscovered(int32_t status) {}
    void OnServicesDiscoveredByUuid(int32_t status, const Uuid &uuid) {}
    void OnConnectionParameterChanged(int32_t interval, int32_t latency, int32_t timeout, int32_t status) {}
    void OnSetPropertyIndication(int32_t ret, bool enable, const NearlinkSsapPropertyParcel &property) {}
    void OnServicesRediscovered(const std::vector<NearlinkSsapServiceParcel> &services) {}
    void OnServiceChanged(uint16_t handle, const NearlinkUuidParcel &uuid) {}
};

namespace {
static NLSTK_SsapUuid_S g_uuid = {.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02}};
constexpr int HOST_FUZZ_DELAY_50_MS = 50;
constexpr int HOST_FUZZ_DELAY_100_MS = 100;
constexpr int HOST_FUZZ_DELAY_5000_MS = 5000;
constexpr uint32_t MESSAGE_SIZE = NearlinkSsapClientInterfaceCode::NL_SSAP_CLIENT_BUTT;
sptr<NearlinkSsapClientServer> g_ssapClient = new (std::nothrow) NearlinkSsapClientServer();
sptr<INearlinkSsapClientCallback> g_ssapClientCb = new (std::nothrow) MockNearlinkSsapClientCallbackStub();
ThreadUtil &g_threadUtil = ThreadUtil::GetInstance();
}

int32_t SsapClientOnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply)
{
    NL_CHECK_RETURN_RET(g_ssapClient, TRANSACTION_ERR, "g_ssapClient is nullptr");
    MessageOption option = {MessageOption::TF_SYNC};
    HILOGI("SsapClientOnRemoteRequest, cmd(%{public}d)", code);
    return g_ssapClient->OnRemoteRequest(code, data, reply, option);
}

void RegisterApplicationFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSsapClientStub::GetDescriptor());
    if (g_ssapClientCb != nullptr) {
        data.WriteRemoteObject(g_ssapClientCb->AsObject());
    }
    data.WriteUint8(provider.ConsumeIntegral<uint8_t>()); // secureReq
    std::string addrStr = BuildAddressString(provider);
    NearlinkRawAddress addr = NearlinkRawAddress(addrStr);
    data.WriteParcelable(&addr);
    data.WriteInt32(provider.ConsumeIntegral<int32_t>()); // transport

    int32_t ret = SsapClientOnRemoteRequest(NearlinkSsapClientInterfaceCode::NL_SSAP_CLIENT_REGISTER_APP, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void DeregisterApplicationFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSsapClientStub::GetDescriptor());
    data.WriteInt32(provider.ConsumeIntegral<int32_t>()); // appId

    int32_t ret = SsapClientOnRemoteRequest(
        NearlinkSsapClientInterfaceCode::NL_SSAP_CLIENT_DEREGISTER_APP, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void ConnectFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSsapClientStub::GetDescriptor());
    data.WriteInt32(provider.ConsumeIntegral<int32_t>()); // appId
    data.WriteBool(provider.ConsumeBool()); // isAutoConnect

    int32_t ret = SsapClientOnRemoteRequest(NearlinkSsapClientInterfaceCode::NL_SSAP_CLIENT_CONNECT, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void DisconnectFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSsapClientStub::GetDescriptor());
    data.WriteInt32(provider.ConsumeIntegral<int32_t>()); // appId

    int32_t ret = SsapClientOnRemoteRequest(NearlinkSsapClientInterfaceCode::NL_SSAP_CLIENT_DIS_CONNECT, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void DiscoveryServicesFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSsapClientStub::GetDescriptor());
    data.WriteInt32(provider.ConsumeIntegral<int32_t>()); // appId

    int32_t ret = SsapClientOnRemoteRequest(
        NearlinkSsapClientInterfaceCode::NL_SSAP_CLIENT_DISCOVERY_SERVICES, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void DiscoverServiceByUuidFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSsapClientStub::GetDescriptor());
    data.WriteInt32(provider.ConsumeIntegral<int32_t>()); // appId
    Uuid uuid;
    uuid.ConvertFrom16Bits(provider.ConsumeIntegral<uint16_t>());
    NearlinkUuidParcel uuidParcel = NearlinkUuidParcel(uuid);
    data.WriteParcelable(&uuidParcel);

    int32_t ret = SsapClientOnRemoteRequest(
        NearlinkSsapClientInterfaceCode::NL_SSAP_CLIENT_DISCOVERY_SERVICES_BY_UUID, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void ReadPropertyFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSsapClientStub::GetDescriptor());
    data.WriteInt32(provider.ConsumeIntegral<int32_t>()); // appId

    UUID propertyUuid = UUID(provider.ConsumeIntegral<uint16_t>());
    SsapProperty property(0, propertyUuid, provider.ConsumeIntegral<uint32_t>(), 0);
    NearlinkSsapPropertyParcel propertyParcel = (NearlinkSsapPropertyParcel)Nearlink::Property(property.GetHandle(),
        Uuid::ConvertFrom128Bits(property.GetUuid().ConvertTo128Bits()), property.GetOperationIndication());
    data.WriteParcelable(&propertyParcel);

    int32_t ret = SsapClientOnRemoteRequest(
        NearlinkSsapClientInterfaceCode::NL_SSAP_CLIENT_READ_PROPERTY, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void CallMethodFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSsapClientStub::GetDescriptor());
    data.WriteInt32(provider.ConsumeIntegral<int32_t>()); // appId

    UUID methodUuid = UUID(provider.ConsumeIntegral<uint16_t>());
    SsapMethod method(0, 0, methodUuid, 0);
    NearlinkSsapMethodParcel methodParcel = (NearlinkSsapMethodParcel)Nearlink::Method(method.GetHandle(),
        Uuid::ConvertFrom128Bits(method.GetUuid().ConvertTo128Bits()));
    data.WriteParcelable(&methodParcel);

    int32_t ret = SsapClientOnRemoteRequest(
        NearlinkSsapClientInterfaceCode::NL_SSAP_CLIENT_CALL_METHOD, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void WritePropertyFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSsapClientStub::GetDescriptor());
    data.WriteInt32(provider.ConsumeIntegral<int32_t>()); // appId

    UUID propertyUuid = UUID(provider.ConsumeIntegral<uint16_t>());
    SsapProperty property(0, propertyUuid, provider.ConsumeIntegral<uint32_t>(), 0);
    NearlinkSsapPropertyParcel propertyParcel = (NearlinkSsapPropertyParcel)Nearlink::Property(property.GetHandle(),
        Uuid::ConvertFrom128Bits(property.GetUuid().ConvertTo128Bits()), property.GetOperationIndication());
    data.WriteParcelable(&propertyParcel);
    data.WriteBool(provider.ConsumeBool());

    int32_t ret = SsapClientOnRemoteRequest(
        NearlinkSsapClientInterfaceCode::NL_SSAP_CLIENT_WRITE_PROPERTY, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void ReadDescriptorFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSsapClientStub::GetDescriptor());
    data.WriteInt32(provider.ConsumeIntegral<int32_t>()); // appId

    uint16_t handle = provider.ConsumeIntegral<uint16_t>();
    int32_t type = provider.ConsumeIntegral<int32_t>();
    int32_t permission = provider.ConsumeIntegral<int32_t>();
    SsapDescriptor descriptor = SsapDescriptor(handle, type, permission);
    NearlinkSsapDescriptorParcel desc =
        (NearlinkSsapDescriptorParcel)Nearlink::Descriptor(descriptor.GetHandle(), descriptor.GetDescriptorType());
    data.WriteParcelable(&desc);

    int32_t ret = SsapClientOnRemoteRequest(
        NearlinkSsapClientInterfaceCode::NL_SSAP_CLIENT_READ_DESCRIPTOR, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void WriteDescriptorFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSsapClientStub::GetDescriptor());
    data.WriteInt32(provider.ConsumeIntegral<int32_t>()); // appId

    uint16_t handle = provider.ConsumeIntegral<uint16_t>();
    int32_t type = provider.ConsumeIntegral<int32_t>();
    int32_t permission = provider.ConsumeIntegral<int32_t>();
    SsapDescriptor descriptor = SsapDescriptor(handle, type, permission);
    NearlinkSsapDescriptorParcel desc =
        (NearlinkSsapDescriptorParcel)Nearlink::Descriptor(descriptor.GetHandle(), descriptor.GetDescriptorType());
    data.WriteParcelable(&desc);
    data.WriteBool(provider.ConsumeBool()); // withoutRespond

    int32_t ret = SsapClientOnRemoteRequest(
        NearlinkSsapClientInterfaceCode::NL_SSAP_CLIENT_WRITE_DESCRIPTOR, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void RequestExchangeMtuFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSsapClientStub::GetDescriptor());
    data.WriteInt32(provider.ConsumeIntegral<int32_t>()); // appId
    data.WriteInt32(provider.ConsumeIntegral<int32_t>()); // mtu

    int32_t ret = SsapClientOnRemoteRequest(
        NearlinkSsapClientInterfaceCode::NL_SSAP_CLIENT_REQUEST_EXCHANGE_MTU, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void RequestConnectionPriorityFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSsapClientStub::GetDescriptor());
    data.WriteInt32(provider.ConsumeIntegral<int32_t>()); // appId
    data.WriteInt32(provider.ConsumeIntegral<int32_t>()); // connPriority

    int32_t ret = SsapClientOnRemoteRequest(
        NearlinkSsapClientInterfaceCode::NL_SSAP_CLIENT_REQUEST_CONNECTION_PRIORITY, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void GetServicesFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSsapClientStub::GetDescriptor());
    data.WriteInt32(provider.ConsumeIntegral<int32_t>()); // appId

    int32_t ret = SsapClientOnRemoteRequest(
        NearlinkSsapClientInterfaceCode::NL_SSAP_CLIENT_GET_SERVICES, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void GetServicesByUuidFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSsapClientStub::GetDescriptor());
    data.WriteInt32(provider.ConsumeIntegral<int32_t>()); // appId
    Uuid uuid;
    uuid.ConvertFrom16Bits(provider.ConsumeIntegral<uint16_t>());
    NearlinkUuidParcel uuidParcel = NearlinkUuidParcel(uuid);
    data.WriteParcelable(&uuidParcel);

    int32_t ret = SsapClientOnRemoteRequest(
        NearlinkSsapClientInterfaceCode::NL_SSAP_CLIENT_GET_SERVICES_BY_UUID, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void RequestPropertyNotificationFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSsapClientStub::GetDescriptor());
    data.WriteInt32(provider.ConsumeIntegral<int32_t>()); // appId
    data.WriteUint16(provider.ConsumeIntegral<uint16_t>()); // propertyHandle
    data.WriteBool(provider.ConsumeBool()); // enable
    data.WriteUint8(provider.ConsumeIntegral<uint8_t>()); // notifyOption

    int32_t ret = SsapClientOnRemoteRequest(
        NearlinkSsapClientInterfaceCode::NL_SSAP_CLIENT_REQUEST_NOTIFICATION, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void SsapClientFuzzTest(const uint8_t* fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    uint32_t code = (provider.ConsumeIntegral<uint32_t>() % MESSAGE_SIZE);
    auto addr = BuildAddressString(provider);

    MessageParcel data;
    MessageParcel reply;

    std::u16string descriptor = NearlinkSsapClientStub::GetDescriptor();
    data.WriteInterfaceToken(descriptor);
    data.WriteInt32(provider.ConsumeIntegral<int32_t>());
    data.WriteString(addr.c_str());
    data.RewindRead(0);

    int32_t ret = SsapClientOnRemoteRequest(code, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void OnMtuChangedFuzzTest(const uint8_t* fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    NLSTK_Errcode_E ret = NLSTK_ERRCODE_SUCCESS;

    int appId = provider.ConsumeIntegral<int>();
    uint16_t mtu = provider.ConsumeIntegral<uint16_t>();
    SsapClientStackAdapter::OnMtuChanged(appId, mtu, ret);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void OnDiscoverCompleteFuzzTest(const uint8_t* fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    NLSTK_Errcode_E ret = NLSTK_ERRCODE_SUCCESS;

    int appId = provider.ConsumeIntegral<int>();
    SsapClientStackAdapter::OnDiscoverComplete(appId, ret);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void OnDiscoverByUuidCompleteFuzzTest(const uint8_t* fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    NLSTK_SsapUuid_S *stackUuid = &g_uuid;
    NLSTK_Errcode_E ret = NLSTK_ERRCODE_SUCCESS;

    int appId = provider.ConsumeIntegral<int>();
    SsapClientStackAdapter::OnDiscoverByUuidComplete(appId, stackUuid, ret);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void OnConnectionStateChangedFuzzTest(const uint8_t* fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    NLSTK_Errcode_E ret = NLSTK_ERRCODE_SUCCESS;

    int appId = provider.ConsumeIntegral<int>();
    uint8_t state = provider.ConsumeIntegral<uint8_t>();
    int reason = provider.ConsumeIntegral<int>();
    SsapClientStackAdapter::OnConnectionStateChanged(appId, state, ret, reason);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void OnReadPropertyFuzzTest(const uint8_t* fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    NLSTK_Errcode_E ret = NLSTK_ERRCODE_SUCCESS;
    NLSTK_VariableData_S value = {0};
    uint8_t dataLen = provider.ConsumeIntegral<uint8_t>();
    std::vector<uint8_t> vec = provider.ConsumeBytes<uint8_t>(dataLen);
    uint8_t *data = vec.data();
    value.len = vec.size();
    value.data = data;
    NLSTK_SsapClientReadPropertyInfo_S *property =
        (NLSTK_SsapClientReadPropertyInfo_S *)malloc(sizeof(NLSTK_SsapClientReadPropertyInfo_S));
    
    int appId = provider.ConsumeIntegral<int>();
    property->handle = provider.ConsumeIntegral<uint16_t>();
    property->uuid = g_uuid;
    property->errorCode = provider.ConsumeIntegral<uint8_t>();
    property->value = value;
    SsapClientStackAdapter::OnReadProperty(appId, property, ret);
    free(property);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void OnCallMethodFuzzTest(const uint8_t* fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    NLSTK_Errcode_E ret = NLSTK_ERRCODE_SUCCESS;
    NLSTK_VariableData_S value = {0};
    uint8_t dataLen = provider.ConsumeIntegral<uint8_t>();
    std::vector<uint8_t> vec = provider.ConsumeBytes<uint8_t>(dataLen);
    uint8_t *data = vec.data();
    value.len = vec.size();
    value.data = data;
    NLSTK_SsapClientCallMethodResult_S *method =
        (NLSTK_SsapClientCallMethodResult_S *)malloc(sizeof(NLSTK_SsapClientCallMethodResult_S));
    
    int appId = provider.ConsumeIntegral<int>();
    method->handle = provider.ConsumeIntegral<uint16_t>();
    method->uuid = g_uuid;
    method->errorCode = provider.ConsumeIntegral<uint8_t>();
    method->value = value;
    SsapClientStackAdapter::OnCallMethod(appId, method, ret);
    free(method);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void OnReadDescriptorFuzzTest(const uint8_t* fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    NLSTK_Errcode_E ret = NLSTK_ERRCODE_SUCCESS;
    NLSTK_VariableData_S value = {0};
    uint8_t dataLen = provider.ConsumeIntegral<uint8_t>();
    std::vector<uint8_t> vec = provider.ConsumeBytes<uint8_t>(dataLen);
    uint8_t *data = vec.data();
    value.len = vec.size();
    value.data = data;
    NLSTK_SsapClientReadDescriptorInfo_S *descriptor =
        (NLSTK_SsapClientReadDescriptorInfo_S *)malloc(sizeof(NLSTK_SsapClientReadDescriptorInfo_S));
    
    int appId = provider.ConsumeIntegral<int>();
    descriptor->handle = provider.ConsumeIntegral<uint16_t>();
    descriptor->type = provider.ConsumeIntegral<uint8_t>();
    descriptor->uuid = g_uuid;
    descriptor->errorCode = provider.ConsumeIntegral<uint8_t>();
    descriptor->value = value;
    SsapClientStackAdapter::OnReadDescriptor(appId, descriptor, ret);
    free(descriptor);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void OnWritePropertyFuzzTest(const uint8_t* fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    NLSTK_Errcode_E ret = NLSTK_ERRCODE_SUCCESS;
    NLSTK_VariableData_S value = {0};
    uint8_t dataLen = provider.ConsumeIntegral<uint8_t>();
    std::vector<uint8_t> vec = provider.ConsumeBytes<uint8_t>(dataLen);
    uint8_t *data = vec.data();
    value.len = vec.size();
    value.data = data;
    NLSTK_SsapClientReadPropertyInfo_S *property =
        (NLSTK_SsapClientReadPropertyInfo_S *)malloc(sizeof(NLSTK_SsapClientReadPropertyInfo_S));
    
    int appId = provider.ConsumeIntegral<int>();
    property->handle = provider.ConsumeIntegral<uint16_t>();
    property->uuid = g_uuid;
    property->errorCode = provider.ConsumeIntegral<uint8_t>();
    property->value = value;
    SsapClientStackAdapter::OnWriteProperty(appId, property, ret);
    free(property);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void OnWriteDescriptorFuzzTest(const uint8_t* fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    NLSTK_Errcode_E ret = NLSTK_ERRCODE_SUCCESS;
    NLSTK_VariableData_S value = {0};
    uint8_t dataLen = provider.ConsumeIntegral<uint8_t>();
    std::vector<uint8_t> vec = provider.ConsumeBytes<uint8_t>(dataLen);
    uint8_t *data = vec.data();
    value.len = vec.size();
    value.data = data;
    NLSTK_SsapClientReadDescriptorInfo_S *descriptor =
        (NLSTK_SsapClientReadDescriptorInfo_S *)malloc(sizeof(NLSTK_SsapClientReadDescriptorInfo_S));
    
    int appId = provider.ConsumeIntegral<int>();
    descriptor->handle = provider.ConsumeIntegral<uint16_t>();
    descriptor->type = provider.ConsumeIntegral<uint8_t>();
    descriptor->uuid = g_uuid;
    descriptor->errorCode = provider.ConsumeIntegral<uint8_t>();
    descriptor->value = value;
    SsapClientStackAdapter::OnWriteDescriptor(appId, descriptor, ret);
    free(descriptor);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void OnGetPropertyNotificationFuzzTest(const uint8_t* fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    NLSTK_SsapUuid_S *stackUuid = &g_uuid;
    NLSTK_Errcode_E ret = NLSTK_ERRCODE_SUCCESS;

    int appId = provider.ConsumeIntegral<int>();
    uint16_t handle = provider.ConsumeIntegral<uint16_t>();
    bool enable = provider.ConsumeBool();
    SsapClientStackAdapter::OnGetPropertyNotification(appId, stackUuid, handle, enable, ret);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void OnGetPropertyIndicationFuzzTest(const uint8_t* fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    NLSTK_SsapUuid_S *stackUuid = &g_uuid;
    NLSTK_Errcode_E ret = NLSTK_ERRCODE_SUCCESS;

    int appId = provider.ConsumeIntegral<int>();
    uint16_t handle = provider.ConsumeIntegral<uint16_t>();
    bool enable = provider.ConsumeBool();
    SsapClientStackAdapter::OnGetPropertyIndication(appId, stackUuid, handle, enable, ret);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void OnSetPropertyNotificationFuzzTest(const uint8_t* fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    NLSTK_SsapUuid_S *stackUuid = &g_uuid;
    NLSTK_Errcode_E ret = NLSTK_ERRCODE_SUCCESS;

    int appId = provider.ConsumeIntegral<int>();
    uint16_t handle = provider.ConsumeIntegral<uint16_t>();
    bool enable = provider.ConsumeBool();
    SsapClientStackAdapter::OnSetPropertyNotification(appId, stackUuid, handle, enable, ret);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void OnSetPropertyIndicationFuzzTest(const uint8_t* fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    NLSTK_SsapUuid_S *stackUuid = &g_uuid;
    NLSTK_Errcode_E ret = NLSTK_ERRCODE_SUCCESS;

    int appId = provider.ConsumeIntegral<int>();
    uint16_t handle = provider.ConsumeIntegral<uint16_t>();
    bool enable = provider.ConsumeBool();
    SsapClientStackAdapter::OnSetPropertyIndication(appId, stackUuid, handle, enable, ret);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void OnPropertyChangedFuzzTest(const uint8_t* fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    NLSTK_VariableData_S value = {0};
    uint8_t dataLen = provider.ConsumeIntegral<uint8_t>();
    std::vector<uint8_t> vec = provider.ConsumeBytes<uint8_t>(dataLen);
    uint8_t *data = vec.data();
    value.len = vec.size();
    value.data = data;
    NLSTK_SsapClientReadPropertyInfo_S *property =
        (NLSTK_SsapClientReadPropertyInfo_S *)malloc(sizeof(NLSTK_SsapClientReadPropertyInfo_S));
    
    int appId = provider.ConsumeIntegral<int>();
    property->handle = provider.ConsumeIntegral<uint16_t>();
    property->uuid = g_uuid;
    property->errorCode = provider.ConsumeIntegral<uint8_t>();
    property->value = value;
    SsapClientStackAdapter::OnPropertyChanged(appId, property);
    free(property);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void OnEventFuzzTest(const uint8_t* fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    NLSTK_VariableData_S value = {0};
    uint8_t dataLen = provider.ConsumeIntegral<uint8_t>();
    std::vector<uint8_t> vec = provider.ConsumeBytes<uint8_t>(dataLen);
    uint8_t *data = vec.data();
    value.len = vec.size();
    value.data = data;
    NLSTK_SsapClientEventInfo_S *property =
        (NLSTK_SsapClientEventInfo_S *)malloc(sizeof(NLSTK_SsapClientEventInfo_S));
    
    int appId = provider.ConsumeIntegral<int>();
    property->handle = provider.ConsumeIntegral<uint16_t>();
    property->uuid = g_uuid;
    property->errorCode = provider.ConsumeIntegral<uint8_t>();
    property->value = value;
    SsapClientStackAdapter::OnEvent(appId, property);
    free(property);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
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
    HILOGI("SsapClientFuzzTest OnStart");
    hostServer->OnStart();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_100_MS));
    HILOGI("SsapClientFuzzTest EnableSle");
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
    OHOS::ConnectFuzzTest(data, size);
    OHOS::DiscoveryServicesFuzzTest(data, size);
    OHOS::DiscoverServiceByUuidFuzzTest(data, size);
    OHOS::ReadPropertyFuzzTest(data, size);
    OHOS::WritePropertyFuzzTest(data, size);
    OHOS::ReadDescriptorFuzzTest(data, size);
    OHOS::WriteDescriptorFuzzTest(data, size);
    OHOS::RequestExchangeMtuFuzzTest(data, size);
    OHOS::RequestConnectionPriorityFuzzTest(data, size);
    OHOS::GetServicesFuzzTest(data, size);
    OHOS::GetServicesByUuidFuzzTest(data, size);
    OHOS::RequestPropertyNotificationFuzzTest(data, size);

    OHOS::OnMtuChangedFuzzTest(data, size);
    OHOS::OnDiscoverCompleteFuzzTest(data, size);
    OHOS::OnDiscoverByUuidCompleteFuzzTest(data, size);
    OHOS::OnConnectionStateChangedFuzzTest(data, size);
    OHOS::OnReadPropertyFuzzTest(data, size);
    OHOS::OnCallMethodFuzzTest(data, size);
    OHOS::OnReadDescriptorFuzzTest(data, size);
    OHOS::OnWritePropertyFuzzTest(data, size);
    OHOS::OnWriteDescriptorFuzzTest(data, size);
    OHOS::OnGetPropertyNotificationFuzzTest(data, size);
    OHOS::OnGetPropertyIndicationFuzzTest(data, size);
    OHOS::OnSetPropertyNotificationFuzzTest(data, size);
    OHOS::OnSetPropertyIndicationFuzzTest(data, size);
    OHOS::OnPropertyChangedFuzzTest(data, size);
    OHOS::OnEventFuzzTest(data, size);

    OHOS::DisconnectFuzzTest(data, size);
    OHOS::DeregisterApplicationFuzzTest(data, size);
    OHOS::SsapClientFuzzTest(data, size);
    OHOS::g_threadUtil.ClearThreadStateMap();
    return 0;
}
#undef private
