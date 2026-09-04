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

#include "ani_nearlink_ssap_server.h"
#include "ani_nearlink_ssap_server_callback.h"
#include "log.h"
#include "nearlink_errorcode.h"
#include "nearlink_host.h"
#include "nearlink_remote_device.h"
#include "ani_nearlink_utils.h"
#include "ani_nearlink_error.h"
#include "ani_nearlink_ssap_utils.h"
#include "nearlink_utils.h"

namespace OHOS {
namespace Nearlink {
void SsapServerImpl::AddService(const ::ohos::nearlink::ssap::Service &service)
{
    ANI_NL_ASSERT_RETURN_VOID(server_ != nullptr, NL_ERR_INTERNAL_ERROR);
    AniSsapService aniService;
    int32_t ret = ConvertServiceToNative(service, aniService);
    ANI_NL_ASSERT_RETURN_VOID(ret == NL_NO_ERROR, ret);
    std::unique_ptr<SsapService> ssapService {nullptr};
    ret = AniSsapServiceToSsapService(aniService, ssapService);
    ANI_NL_ASSERT_RETURN_VOID(ret == NL_NO_ERROR, ret);
    NlErrCode err = server_->AddService(*ssapService);
    ANI_NL_ASSERT_RETURN_VOID(err == NL_NO_ERROR, err);
}

void SsapServerImpl::RemoveService(const ::taihe::string &serviceUuid)
{
    ANI_NL_ASSERT_RETURN_VOID(server_ != nullptr, NL_ERR_INTERNAL_ERROR);
    std::string uuidStr = std::string(serviceUuid);
    ANI_NL_ASSERT_RETURN_VOID(IsValidUuid(uuidStr), NL_ERR_INVALID_UUID);
    ConvertUuidToUpperCase(uuidStr);
    ANI_NL_ASSERT_RETURN_VOID(!CheckBaseUuid(uuidStr), NL_ERR_STANDARD_UUID_NOT_ALLOWED);
    UUID uuid = UUID::FromString(uuidStr);
    SsapService nativeService(uuid, SsapServiceType::VENDOR_PROMARY);
    NlErrCode err = server_->RemoveSsapService(nativeService);
    ANI_NL_ASSERT_RETURN_VOID(err == NL_NO_ERROR, err);
}

void SsapServerImpl::Close()
{
    ANI_NL_ASSERT_RETURN_VOID(server_ != nullptr, NL_ERR_INVALID_PARAM);
    NlErrCode err = server_->Close();
    ANI_NL_ASSERT_RETURN_VOID(err == NL_NO_ERROR, err);
}

void SsapServerImpl::NotifyPropertyChanged(const ::taihe::string &address,
    const::ohos::nearlink::ssap::Property &property)
{
    ANI_NL_ASSERT_RETURN_VOID(server_ != nullptr, NL_ERR_INTERNAL_ERROR);
    std::string sleAddress = std::string(address);
    ANI_NL_ASSERT_RETURN_VOID(IsValidAddr(sleAddress), NL_ERR_INVALID_ADDRESS);
    AniSsapProperty aniProperty;
    int32_t ret = ConvertPropertyToNative(property, aniProperty);
    ANI_NL_ASSERT_RETURN_VOID(ret == NL_NO_ERROR, ret);
    SsapProperty nativeProperty(0, aniProperty.propertyUuid_, aniProperty.operation_, 0);
    Nearlink::NearlinkRemoteDevice device(sleAddress, static_cast<int>(NlTransportType::NL_TRANSPORT_SLE));
    nativeProperty.SetValue(&aniProperty.value_[0], aniProperty.value_.size());
    NlErrCode err = server_->NotifyPropertyChanged(device, nativeProperty, false);
    ANI_NL_ASSERT_RETURN_VOID(err == NL_NO_ERROR, err);
}

void SsapServerImpl::SendResponse(const ::ohos::nearlink::ssap::ServerResponse &response)
{
    ANI_NL_ASSERT_RETURN_VOID(server_ != nullptr, NL_ERR_INVALID_PARAM);
    AniSsapServerResponse aniResponse;
    int32_t ret = ConvertTaiheServerResponseToNative(response, aniResponse);
    ANI_NL_ASSERT_RETURN_VOID(ret == NL_NO_ERROR, NL_ERR_INVALID_PARAM);
    ANI_NL_ASSERT_RETURN_VOID(IsValidAddr(aniResponse.address_), NL_ERR_INVALID_ADDRESS);
    NlErrCode err = server_->AuthorizeResponse(static_cast<uint16_t>(aniResponse.requestId_), true);
    ANI_NL_ASSERT_RETURN_VOID(err == NL_NO_ERROR, err);
}

void SsapServerImpl::OnConnectionStateChange(
    ::taihe::callback_view<void(::ohos::nearlink::ssap::ConnectionChangeState const& data)> callback)
{
    if (callback_) {
        callback_->connectionStateEvent_.RegisterEvent(callback);
    }
}

void SsapServerImpl::OffConnectionStateChange(
    ::taihe::optional_view<::taihe::callback<void(::ohos::nearlink::ssap::ConnectionChangeState const& data)>> callback)
{
    if (callback_) {
        callback_->connectionStateEvent_.DeregisterEvent(callback);
    }
}

void SsapServerImpl::OnPropertyRead(
    ::taihe::callback_view<void(::ohos::nearlink::ssap::PropertyReadRequest const& data)> callback)
{
    if (callback_) {
        callback_->propertyReadEvent_.RegisterEvent(callback);
    }
}

void SsapServerImpl::OffPropertyRead(
    ::taihe::optional_view<::taihe::callback<void(::ohos::nearlink::ssap::PropertyReadRequest const& data)>> callback)
{
    if (callback_) {
        callback_->propertyReadEvent_.DeregisterEvent(callback);
    }
}

void SsapServerImpl::OnPropertyWrite(
    ::taihe::callback_view<void(::ohos::nearlink::ssap::PropertyWriteRequest const& data)> callback)
{
    if (callback_) {
        callback_->propertyWriteEvent_.RegisterEvent(callback);
    }
}

void SsapServerImpl::OffPropertyWrite(
    ::taihe::optional_view<::taihe::callback<void(::ohos::nearlink::ssap::PropertyWriteRequest const& data)>> callback)
{
    if (callback_) {
        callback_->propertyWriteEvent_.DeregisterEvent(callback);
    }
}

void SsapServerImpl::OnMtuChange(::taihe::callback_view<void(int32_t data)> callback)
{
    if (callback_) {
        callback_->mtuChangeEvent_.RegisterEvent(callback);
    }
}

void SsapServerImpl::OffMtuChange(::taihe::optional_view<::taihe::callback<void(int32_t data)>>callback)
{
    if (callback_) {
        callback_->mtuChangeEvent_.DeregisterEvent(callback);
    }
}
}  // namespace Nearlink
}  // namespace OHOS