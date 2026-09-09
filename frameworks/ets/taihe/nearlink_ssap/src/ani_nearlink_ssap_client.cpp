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

#include "ani_nearlink_ssap_client.h"
#include "ani_nearlink_ssap_client_callback.h"
#include "log.h"
#include "nearlink_errorcode.h"
#include "nearlink_host.h"
#include "nearlink_remote_device.h"
#include "ani_nearlink_utils.h"
#include "ani_nearlink_error.h"
#include "ani_nearlink_ssap_utils.h"

namespace OHOS {
namespace Nearlink {
void SsapClientImpl::Connect()
{
    HILOGI("enter");
    ANI_NL_ASSERT_RETURN_VOID(client_ != nullptr, NL_ERR_INTERNAL_ERROR);
    ANI_NL_ASSERT_RETURN_VOID(callback_ != nullptr, NL_ERR_INVALID_PARAM);
    NlErrCode err = client_->Connect(callback_);
    ANI_NL_ASSERT_RETURN_VOID(err == NL_NO_ERROR, err);
}

void SsapClientImpl::Disconnect()
{
    HILOGI("enter");
    ANI_NL_ASSERT_RETURN_VOID(client_ != nullptr, NL_ERR_INTERNAL_ERROR);
    NlErrCode err = client_->Disconnect();
    ANI_NL_ASSERT_RETURN_VOID(err == NL_NO_ERROR, err);
}

bool SsapClientImpl::Close()
{
    HILOGI("enter");
    ANI_NL_ASSERT_RETURN(client_ != nullptr, NL_ERR_INTERNAL_ERROR, false);
    NlErrCode err = client_->Close();
    ANI_NL_ASSERT_RETURN(err == NL_NO_ERROR, err, false);
    return true;
}

::taihe::array<::ohos::nearlink::ssap::Service> SsapClientImpl::GetServices()
{
    HILOGI("enter");
    ::taihe::array<::ohos::nearlink::ssap::Service> result {};
    ANI_NL_ASSERT_RETURN(client_ != nullptr, NL_ERR_INTERNAL_ERROR, result);
    NlErrCode err = client_->FindStructure();
    ANI_NL_ASSERT_RETURN(err == NL_NO_ERROR, err, result);
    std::vector<SsapService> services;
    err = client_->GetService(services);
    ANI_NL_ASSERT_RETURN(err == NL_NO_ERROR, err, result);
    std::vector<::ohos::nearlink::ssap::Service> taiheServices;
    for (const auto &service : services) {
        taiheServices.emplace_back(ConvertServiceToTaihe(service));
    }
    result = ::taihe::array<::ohos::nearlink::ssap::Service>(
        ::taihe::copy_data_t{}, taiheServices.data(), taiheServices.size());
    return result;
}

uintptr_t SsapClientImpl::ReadProperty(const ::ohos::nearlink::ssap::Property &property)
{
    HILOGI("enter");
    ANI_NL_ASSERT_RETURN(client_ != nullptr, NL_ERR_INTERNAL_ERROR, reinterpret_cast<uintptr_t>(nullptr));
    AniSsapProperty aniProperty;
    int32_t ret = ConvertPropertyToNative(property, aniProperty);
    ANI_NL_ASSERT_RETURN(ret == NL_NO_ERROR, ret, reinterpret_cast<uintptr_t>(nullptr));
    SsapProperty nativeProperty(0, aniProperty.propertyUuid_, aniProperty.operation_, 0);
    auto func = [ssapClient = client_, nativeProperty]() mutable {
        HILOGI("start read property");
        NlErrCode err = ssapClient->ReadProperty(nativeProperty);
        return TaiheAsyncWorkRet(err);
    };

    ani_env *env = taihe::get_env();
    ANI_NL_ASSERT_RETURN(env, NL_ERR_INTERNAL_ERROR, reinterpret_cast<uintptr_t>(nullptr));
    auto asyncWork = TaiheAsyncWorkFactory::CreateAsyncWork(env, nullptr, func);
    ANI_NL_ASSERT_RETURN(asyncWork, NL_ERR_INTERNAL_ERROR, reinterpret_cast<uintptr_t>(nullptr));
    ANI_NL_ASSERT_RETURN(callback_ != nullptr, NL_ERR_INTERNAL_ERROR, reinterpret_cast<uintptr_t>(nullptr));
    bool success = callback_->asyncPromiseMap_.TryPush(TaiheAsyncType::SSAP_CLIENT_READ_PROPERTY, asyncWork);
    ANI_NL_ASSERT_RETURN(success, NL_ERR_INTERNAL_ERROR, reinterpret_cast<uintptr_t>(nullptr));
    asyncWork->Run();
    return reinterpret_cast<uintptr_t>(asyncWork->GetRet());
}

uintptr_t SsapClientImpl::WriteProperty(const ::ohos::nearlink::ssap::Property &property,
    ::ohos::nearlink::ssap::PropertyWriteType writeType)
{
    HILOGI("enter");
    ANI_NL_ASSERT_RETURN(client_ != nullptr, NL_ERR_INTERNAL_ERROR, reinterpret_cast<uintptr_t>(nullptr));
    AniSsapProperty aniProperty;
    int32_t ret = ConvertPropertyToNative(property, aniProperty);
    ANI_NL_ASSERT_RETURN(ret == NL_NO_ERROR, ret, reinterpret_cast<uintptr_t>(nullptr));
    SsapProperty nativeProperty(0, aniProperty.propertyUuid_, aniProperty.operation_, 0);
    nativeProperty.SetValue(aniProperty.value_.data(), aniProperty.value_.size());
    nativeProperty.SetWriteType(writeType.get_value());
    auto func = [ssapClient = client_, nativeProperty]() mutable {
        HILOGI("start write property");
        NlErrCode err = ssapClient->WriteProperty(nativeProperty);
        return TaiheAsyncWorkRet(err);
    };

    ani_env *env = taihe::get_env();
    ANI_NL_ASSERT_RETURN(env, NL_ERR_INTERNAL_ERROR, reinterpret_cast<uintptr_t>(nullptr));
    auto asyncWork = TaiheAsyncWorkFactory::CreateAsyncWork(env, nullptr, func);
    ANI_NL_ASSERT_RETURN(asyncWork, NL_ERR_INTERNAL_ERROR, reinterpret_cast<uintptr_t>(nullptr));
    ANI_NL_ASSERT_RETURN(callback_ != nullptr, NL_ERR_INTERNAL_ERROR, reinterpret_cast<uintptr_t>(nullptr));
    bool success = callback_->asyncPromiseMap_.TryPush(TaiheAsyncType::SSAP_CLIENT_WRITE_PROPERTY, asyncWork);
    ANI_NL_ASSERT_RETURN(success, NL_ERR_INTERNAL_ERROR, reinterpret_cast<uintptr_t>(nullptr));
    asyncWork->Run();
    return reinterpret_cast<uintptr_t>(asyncWork->GetRet());
}

uintptr_t SsapClientImpl::ReadDescriptor(
    const ::ohos::nearlink::ssap::PropertyDescriptor &descriptor)
{
    HILOGI("enter");
    ANI_NL_ASSERT_RETURN(client_ != nullptr, NL_ERR_INTERNAL_ERROR, reinterpret_cast<uintptr_t>(nullptr));
    AniSsapDescriptor aniDescriptor;
    int32_t ret = ConvertDescriptorToNative(descriptor, aniDescriptor);
    ANI_NL_ASSERT_RETURN(ret == NL_NO_ERROR, ret, reinterpret_cast<uintptr_t>(nullptr));
    ANI_NL_ASSERT_RETURN(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        reinterpret_cast<uintptr_t>(nullptr));
    ANI_NL_ASSERT_RETURN(NearlinkHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF,
        reinterpret_cast<uintptr_t>(nullptr));
    uint16_t handle = 0;
    bool res = client_->GetHandle(aniDescriptor.serviceUuid_, aniDescriptor.propertyUuid_, handle);
    ANI_NL_ASSERT_RETURN(res == true, NL_ERR_INTERNAL_ERROR, reinterpret_cast<uintptr_t>(nullptr));
    SsapDescriptor nativeDescriptor(handle, aniDescriptor.descriptorType_, 0);
    auto func = [ssapClient = client_, nativeDescriptor]() mutable {
        HILOGI("start read descriptor");
        NlErrCode err = ssapClient->ReadDescriptor(nativeDescriptor);
        return TaiheAsyncWorkRet(err);
    };
    ani_env *env = taihe::get_env();
    ANI_NL_ASSERT_RETURN(env, NL_ERR_INTERNAL_ERROR, reinterpret_cast<uintptr_t>(nullptr));
    auto asyncWork = TaiheAsyncWorkFactory::CreateAsyncWork(env, nullptr, func);
    ANI_NL_ASSERT_RETURN(asyncWork, NL_ERR_INTERNAL_ERROR, reinterpret_cast<uintptr_t>(nullptr));
    ANI_NL_ASSERT_RETURN(callback_ != nullptr, NL_ERR_INTERNAL_ERROR, reinterpret_cast<uintptr_t>(nullptr));
    bool success = callback_->asyncPromiseMap_.TryPush(TaiheAsyncType::SSAP_CLIENT_READ_DESCRIPTOR, asyncWork);
    ANI_NL_ASSERT_RETURN(success, NL_ERR_INTERNAL_ERROR, reinterpret_cast<uintptr_t>(nullptr));
    asyncWork->Run();
    return reinterpret_cast<uintptr_t>(asyncWork->GetRet());
}

uintptr_t SsapClientImpl::WriteDescriptor(const ::ohos::nearlink::ssap::PropertyDescriptor &descriptor)
{
    HILOGI("enter");
    ANI_NL_ASSERT_RETURN(client_ != nullptr, NL_ERR_INTERNAL_ERROR, reinterpret_cast<uintptr_t>(nullptr));
    AniSsapDescriptor aniDescriptor;
    int32_t ret = ConvertDescriptorToNative(descriptor, aniDescriptor);
    ANI_NL_ASSERT_RETURN(ret == NL_NO_ERROR, ret, reinterpret_cast<uintptr_t>(nullptr));
    ANI_NL_ASSERT_RETURN(NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT,
        reinterpret_cast<uintptr_t>(nullptr));
    ANI_NL_ASSERT_RETURN(NearlinkHost::GetInstance().IsSleAvailableToCaller(), NL_ERR_SLE_OFF,
        reinterpret_cast<uintptr_t>(nullptr));
    uint16_t handle = 0;
    bool res = client_->GetHandle(aniDescriptor.serviceUuid_, aniDescriptor.propertyUuid_, handle);
    ANI_NL_ASSERT_RETURN(res == true, NL_ERR_INTERNAL_ERROR, reinterpret_cast<uintptr_t>(nullptr));
    SsapDescriptor nativeDescriptor(handle, aniDescriptor.descriptorType_, 0);
    nativeDescriptor.SetValue(aniDescriptor.descriptorValue_.data(), aniDescriptor.descriptorValue_.size());
    auto func = [ssapClient = client_, nativeDescriptor]() mutable {
        HILOGI("start write descriptor");
        NlErrCode err = ssapClient->WriteDescriptor(nativeDescriptor);
        return TaiheAsyncWorkRet(err);
    };
    ani_env *env = taihe::get_env();
    ANI_NL_ASSERT_RETURN(env, NL_ERR_INTERNAL_ERROR, reinterpret_cast<uintptr_t>(nullptr));
    auto asyncWork = TaiheAsyncWorkFactory::CreateAsyncWork(env, nullptr, func);
    ANI_NL_ASSERT_RETURN(asyncWork, NL_ERR_INTERNAL_ERROR, reinterpret_cast<uintptr_t>(nullptr));
    ANI_NL_ASSERT_RETURN(callback_ != nullptr, NL_ERR_INTERNAL_ERROR, reinterpret_cast<uintptr_t>(nullptr));
    bool success = callback_->asyncPromiseMap_.TryPush(TaiheAsyncType::SSAP_CLIENT_WRITE_DESCRIPTOR, asyncWork);
    ANI_NL_ASSERT_RETURN(success, NL_ERR_INTERNAL_ERROR, reinterpret_cast<uintptr_t>(nullptr));
    asyncWork->Run();
    return reinterpret_cast<uintptr_t>(asyncWork->GetRet());
}

uintptr_t SsapClientImpl::SetPropertyNotification(const ::ohos::nearlink::ssap::Property &property, bool enable)
{
    HILOGI("enter");
    ANI_NL_ASSERT_RETURN(client_ != nullptr, NL_ERR_INTERNAL_ERROR, reinterpret_cast<uintptr_t>(nullptr));
    AniSsapProperty aniProperty;
    int32_t ret = ConvertPropertyToNative(property, aniProperty);
    ANI_NL_ASSERT_RETURN(ret == NL_NO_ERROR, ret, reinterpret_cast<uintptr_t>(nullptr));
    SsapProperty nativeProperty(0, aniProperty.propertyUuid_, aniProperty.operation_, 0);
    auto func = [ssapClient = client_, nativeProperty, enable]() mutable {
        NlErrCode err = ssapClient->SetNotifyProperty(nativeProperty, enable);
        return TaiheAsyncWorkRet(err);
    };
    ani_env *env = taihe::get_env();
    ANI_NL_ASSERT_RETURN(env, NL_ERR_INTERNAL_ERROR, reinterpret_cast<uintptr_t>(nullptr));
    auto asyncWork = TaiheAsyncWorkFactory::CreateAsyncWork(env, nullptr, func);
    ANI_NL_ASSERT_RETURN(asyncWork, NL_ERR_INTERNAL_ERROR, reinterpret_cast<uintptr_t>(nullptr));
    ANI_NL_ASSERT_RETURN(callback_ != nullptr, NL_ERR_INTERNAL_ERROR, reinterpret_cast<uintptr_t>(nullptr));
    bool success = callback_->asyncPromiseMap_.TryPush(TaiheAsyncType::SSAP_CLIENT_SET_PROPERTY_NOTIFY, asyncWork);
    ANI_NL_ASSERT_RETURN(success, NL_ERR_INTERNAL_ERROR, reinterpret_cast<uintptr_t>(nullptr));
    asyncWork->Run();
    return reinterpret_cast<uintptr_t>(asyncWork->GetRet());
}

uintptr_t SsapClientImpl::SetPropertyIndication(const ::ohos::nearlink::ssap::Property &property, bool enable)
{
    HILOGI("enter");
    ANI_NL_ASSERT_RETURN(client_ != nullptr, NL_ERR_INTERNAL_ERROR, reinterpret_cast<uintptr_t>(nullptr));
    AniSsapProperty aniProperty;
    int32_t ret = ConvertPropertyToNative(property, aniProperty);
    ANI_NL_ASSERT_RETURN(ret == NL_NO_ERROR, ret, reinterpret_cast<uintptr_t>(nullptr));
    SsapProperty nativeProperty(0, aniProperty.propertyUuid_, aniProperty.operation_, 0);
    auto func = [ssapClient = client_, nativeProperty, enable]() mutable {
        NlErrCode err = ssapClient->SetIndicateProperty(nativeProperty, enable);
        return TaiheAsyncWorkRet(err);
    };
    ani_env *env = taihe::get_env();
    ANI_NL_ASSERT_RETURN(env, NL_ERR_INTERNAL_ERROR, reinterpret_cast<uintptr_t>(nullptr));
    auto asyncWork = TaiheAsyncWorkFactory::CreateAsyncWork(env, nullptr, func);
    ANI_NL_ASSERT_RETURN(asyncWork, NL_ERR_INTERNAL_ERROR, reinterpret_cast<uintptr_t>(nullptr));
    ANI_NL_ASSERT_RETURN(callback_ != nullptr, NL_ERR_INTERNAL_ERROR, reinterpret_cast<uintptr_t>(nullptr));
    bool success = callback_->asyncPromiseMap_.TryPush(TaiheAsyncType::SSAP_CLIENT_SET_PROPERTY_INDICATE, asyncWork);
    ANI_NL_ASSERT_RETURN(success, NL_ERR_INTERNAL_ERROR, reinterpret_cast<uintptr_t>(nullptr));
    asyncWork->Run();
    return reinterpret_cast<uintptr_t>(asyncWork->GetRet());
}

void SsapClientImpl::RequestMtuSize(int mtu)
{
    HILOGI("enter");
    ANI_NL_ASSERT_RETURN_VOID(client_ != nullptr, NL_ERR_INTERNAL_ERROR);
    if (mtu < MIN_MTU_SIZE) {
        HILOGI("convert requested MTU %{public}d to %{public}d", mtu, MIN_MTU_SIZE);
        mtu = MIN_MTU_SIZE;
    } else if (mtu > MAX_MTU_SIZE) {
        HILOGI("convert requested MTU %{public}d to %{public}d", mtu, MAX_MTU_SIZE);
        mtu = MAX_MTU_SIZE;
    }
    NlErrCode err = client_->RequestSleMtuSize(mtu);
    ANI_NL_ASSERT_RETURN_VOID(err == NL_NO_ERROR, err);
}

uintptr_t SsapClientImpl::CallMethod(const ::ohos::nearlink::ssap::Method &method)
{
    HILOGI("enter");
    ANI_NL_ASSERT_RETURN(client_ != nullptr, NL_ERR_INTERNAL_ERROR, reinterpret_cast<uintptr_t>(nullptr));
    AniSsapMethod aniMethod;
    int32_t ret = ConvertMethodToNative(method, aniMethod);
    ANI_NL_ASSERT_RETURN(ret == NL_NO_ERROR, ret, reinterpret_cast<uintptr_t>(nullptr));
    SsapMethod nativeMethod(0, aniMethod.methodUuid_, 0);
    nativeMethod.SetParameter(aniMethod.parameter_.data(), aniMethod.parameter_.size());
    size_t length = 0;
    nativeMethod.GetParameter(&length);
    auto func = [ssapClient = client_, nativeMethod]() mutable {
        size_t length = 0;
        nativeMethod.GetParameter(&length);
        HILOGI("%{public}s", nativeMethod.GetUuid().GetEncryptUuid().c_str());

        NlErrCode err = ssapClient->CallMethod(nativeMethod);
        return TaiheAsyncWorkRet(err);
    };

    ani_env *env = taihe::get_env();
    ANI_NL_ASSERT_RETURN(env, NL_ERR_INTERNAL_ERROR, reinterpret_cast<uintptr_t>(nullptr));
    auto asyncWork = TaiheAsyncWorkFactory::CreateAsyncWork(env, nullptr, func);
    ANI_NL_ASSERT_RETURN(asyncWork, NL_ERR_INTERNAL_ERROR, reinterpret_cast<uintptr_t>(nullptr));
    ANI_NL_ASSERT_RETURN(callback_ != nullptr, NL_ERR_INTERNAL_ERROR, reinterpret_cast<uintptr_t>(nullptr));
    bool success = callback_->asyncPromiseMap_.TryPush(TaiheAsyncType::SSAP_CLIENT_CALL_METHOD, asyncWork);
    ANI_NL_ASSERT_RETURN(success, NL_ERR_INTERNAL_ERROR, reinterpret_cast<uintptr_t>(nullptr));
    asyncWork->Run();
    return reinterpret_cast<uintptr_t>(asyncWork->GetRet());
}

void SsapClientImpl::OnPropertyChange(
    ::taihe::callback_view<void(::ohos::nearlink::ssap::Property const& data)> callback)
{
    HILOGI("enter");
    if (callback_) {
        callback_->eventSubscribe_.RegisterEvent<void(::ohos::nearlink::ssap::Property const&)>(
            SLE_SSAP_CLIENT_CALLBACK_PROPERTY_CHANGE, callback);
    }
}

void SsapClientImpl::OffPropertyChange(
    ::taihe::optional_view<::taihe::callback<void(::ohos::nearlink::ssap::Property const& data)>> callback)
{
    HILOGI("enter");
    if (callback_) {
        callback_->eventSubscribe_.DeregisterEvent<void(::ohos::nearlink::ssap::Property const&)>(
            SLE_SSAP_CLIENT_CALLBACK_PROPERTY_CHANGE, callback);
    }
}

void SsapClientImpl::OnEventNotify(::taihe::callback_view<void(::ohos::nearlink::ssap::Event const& data)> callback)
{
    HILOGI("enter");
    if (callback_) {
        callback_->eventSubscribe_.RegisterEvent<void(::ohos::nearlink::ssap::Event const&)>(
            SLE_SSAP_CLIENT_CALLBACK_EVENT_NOTIFY, callback);
    }
}

void SsapClientImpl::OffEventNotify(
    ::taihe::optional_view<::taihe::callback<void(::ohos::nearlink::ssap::Event const& data)>> callback)
{
    HILOGI("enter");
    if (callback_) {
        callback_->eventSubscribe_.DeregisterEvent<void(::ohos::nearlink::ssap::Event const&)>(
            SLE_SSAP_CLIENT_CALLBACK_EVENT_NOTIFY, callback);
    }
}

void SsapClientImpl::OnConnectionStateChange(
    ::taihe::callback_view<void(::ohos::nearlink::ssap::ConnectionChangeState const& data)> callback)
{
    HILOGI("enter");
    if (callback_) {
        callback_->eventSubscribe_.RegisterEvent<
            void(::ohos::nearlink::ssap::ConnectionChangeState const&)>(
            SLE_SSAP_CLIENT_CALLBACK_CONNECTION_STATE_CHANGE, callback);
    }
}

void SsapClientImpl::OffConnectionStateChange(
    ::taihe::optional_view<::taihe::callback<void(::ohos::nearlink::ssap::ConnectionChangeState const& data)>>callback)
{
    HILOGI("enter");
    if (callback_) {
        callback_->eventSubscribe_.DeregisterEvent<
            void(::ohos::nearlink::ssap::ConnectionChangeState const&)>(
            SLE_SSAP_CLIENT_CALLBACK_CONNECTION_STATE_CHANGE, callback);
    }
}

void SsapClientImpl::OnMtuChange(::taihe::callback_view<void(int32_t data)> callback)
{
    HILOGI("enter");
    if (callback_) {
        callback_->eventSubscribe_.RegisterEvent<void(int32_t)>(
            SLE_SSAP_CLIENT_CALLBACK_MTU_CHANGE, callback);
    }
}

void SsapClientImpl::OffMtuChange(::taihe::optional_view<::taihe::callback<void(int32_t data)>> callback)
{
    HILOGI("enter");
    if (callback_) {
        callback_->eventSubscribe_.DeregisterEvent<void(int32_t)>(
            SLE_SSAP_CLIENT_CALLBACK_MTU_CHANGE, callback);
    }
}
}  // namespace Nearlink
}  // namespace OHOS