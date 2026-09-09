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

#ifndef ANI_NEARLINK_SSAP_SERVER_H
#define ANI_NEARLINK_SSAP_SERVER_H

#include "nearlink_remote_device.h"
#include "nearlink_ssap_server.h"
#include "ohos.nearlink.ssap.proj.hpp"
#include "ohos.nearlink.ssap.impl.hpp"
#include "ani_nearlink_ssap_server_callback.h"
#include "taihe/runtime.hpp"
#include "ani_nearlink_ssap_utils.h"
namespace OHOS {
namespace Nearlink {
class SsapServerImpl {
public:
    void AddService(const ::ohos::nearlink::ssap::Service &service);
    void RemoveService(const ::taihe::string &serviceUuid);
    void Close();
    void NotifyPropertyChanged(const ::taihe::string &address, const ::ohos::nearlink::ssap::Property &property);
    void SendResponse(const ::ohos::nearlink::ssap::ServerResponse &response);
    void OnConnectionStateChange(
        ::taihe::callback_view<void(::ohos::nearlink::ssap::ConnectionChangeState const& data)> callback);
    void OffConnectionStateChange(
        ::taihe::optional_view<::taihe::callback<void(::ohos::nearlink::ssap::ConnectionChangeState const& data)>>
        callback);
    void OnPropertyRead(::taihe::callback_view<void(::ohos::nearlink::ssap::PropertyReadRequest const& data)> callback);
    void OffPropertyRead(
        ::taihe::optional_view<::taihe::callback<void(::ohos::nearlink::ssap::PropertyReadRequest const& data)>>
        callback);
    void OnPropertyWrite(
        ::taihe::callback_view<void(::ohos::nearlink::ssap::PropertyWriteRequest const& data)> callback);
    void OffPropertyWrite(
        ::taihe::optional_view<::taihe::callback<void(::ohos::nearlink::ssap::PropertyWriteRequest const& data)>>
        callback);
    void OnMtuChange(::taihe::callback_view<void(int32_t data)> callback);
    void OffMtuChange(::taihe::optional_view<::taihe::callback<void(int32_t data)>> callback);
    SsapServerImpl()
    {
        callback_ = std::make_shared<AniSsapServerCallback>();
        std::shared_ptr<SsapServerCallback> tmp = std::static_pointer_cast<SsapServerCallback>(callback_);
        server_ = SsapServer::CreateSsapServer(tmp);
    }

private:
    std::shared_ptr<AniSsapServerCallback> callback_ = nullptr;
    std::shared_ptr<SsapServer> server_ = nullptr;
};
}  // namespace Nearlink
}  // namespace OHOS

#endif // ANI_NEARLINK_SSAP_SERVER_H