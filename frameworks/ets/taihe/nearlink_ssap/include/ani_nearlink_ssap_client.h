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

#ifndef ANI_NEARLINK_SSAP_CLIENT_H
#define ANI_NEARLINK_SSAP_CLIENT_H

#include "nearlink_remote_device.h"
#include "nearlink_ssap_client.h"
#include "ohos.nearlink.ssap.proj.hpp"
#include "ohos.nearlink.ssap.impl.hpp"
#include "ani_nearlink_ssap_client_callback.h"
#include "taihe/runtime.hpp"
#include "ani_nearlink_ssap_utils.h"
#include "ani_nearlink_utils.h"
#include "ani_nearlink_error.h"
namespace OHOS {
namespace Nearlink {
class SsapClientImpl {
public:
    void Connect();
    void Disconnect();
    bool Close();
    ::taihe::array<::ohos::nearlink::ssap::Service> GetServices();
    uintptr_t ReadProperty(const ::ohos::nearlink::ssap::Property &property);
    uintptr_t WriteProperty(const ::ohos::nearlink::ssap::Property &property,
        ::ohos::nearlink::ssap::PropertyWriteType writeType);
    uintptr_t ReadDescriptor(const ::ohos::nearlink::ssap::PropertyDescriptor &descriptor);
    uintptr_t WriteDescriptor(const ::ohos::nearlink::ssap::PropertyDescriptor &descriptor);
    uintptr_t SetPropertyNotification(const ::ohos::nearlink::ssap::Property &property, bool enable);
    uintptr_t SetPropertyIndication(const ::ohos::nearlink::ssap::Property &property, bool enable);
    void RequestMtuSize(int mtu);
    uintptr_t CallMethod(const ::ohos::nearlink::ssap::Method &method);
    void OnPropertyChange(
        ::taihe::callback_view<void(::ohos::nearlink::ssap::Property const& data)> callback);
    void OffPropertyChange(
        ::taihe::optional_view<::taihe::callback<void(::ohos::nearlink::ssap::Property const& data)>> callback);
    void OnEventNotify(
        ::taihe::callback_view<void(::ohos::nearlink::ssap::Event const& data)> callback);
    void OffEventNotify(
        ::taihe::optional_view<::taihe::callback<void(::ohos::nearlink::ssap::Event const& data)>> callback);
    void OnConnectionStateChange(
        ::taihe::callback_view<void(::ohos::nearlink::ssap::ConnectionChangeState const& data)> callback);
    void OffConnectionStateChange(
        ::taihe::optional_view<::taihe::callback<void(::ohos::nearlink::ssap::ConnectionChangeState const& data)>>
        callback);
    void OnMtuChange(::taihe::callback_view<void(int32_t data)> callback);
    void OffMtuChange(::taihe::optional_view<::taihe::callback<void(int32_t data)>> callback);
    explicit SsapClientImpl(std::string &address)
    {
        HILOGI("enter");
        device_ = std::make_shared<NearlinkRemoteDevice>(address, ADAPTER_SLE);
        callback_ = std::make_shared<AniSsapClientCallback>();
        client_ = SsapClient::CreateSsapClient(device_);
        callback_->SetClient(this);
    }
    ~SsapClientImpl()
    {
        callback_->SetClient(nullptr);
    }

    std::shared_ptr<SsapClient> &GetClient()
    {
        return client_;
    }

    std::shared_ptr<AniSsapClientCallback> GetCallback()
    {
        return callback_;
    }

    std::shared_ptr<NearlinkRemoteDevice> GetDevice()
    {
        return device_;
    }
private:
    std::shared_ptr<AniSsapClientCallback> callback_ = nullptr;
    std::shared_ptr<NearlinkRemoteDevice> device_ = nullptr;
    std::shared_ptr<SsapClient> client_ = nullptr;
};
}  // namespace Nearlink
}  // namespace OHOS

#endif // ANI_NEARLINK_SSAP_CLIENT_H