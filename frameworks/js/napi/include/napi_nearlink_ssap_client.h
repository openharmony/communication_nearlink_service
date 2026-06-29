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
#ifndef NAPI_NEARLINK_SSAP_CLIENT_H_
#define NAPI_NEARLINK_SSAP_CLIENT_H_

#include "nearlink_ssap_client.h"
#include "log.h"
#include "nearlink_remote_device.h"
#include "napi_nearlink_ssap_client_callback.h"

namespace OHOS {
namespace Nearlink {
class NapiNearlinkSsapClient {
public:
    static napi_value CreateClient(napi_env env, napi_callback_info info);
    static void DefineSsapClientJSClass(napi_env env, napi_value exports);
    static napi_value SsapClientConstructor(napi_env env, napi_callback_info info);

    static napi_value OnPropertyChange(napi_env env, napi_callback_info info);
    static napi_value OffPropertyChange(napi_env env, napi_callback_info info);
    static napi_value OnEventNotify(napi_env env, napi_callback_info info);
    static napi_value OffEventNotify(napi_env env, napi_callback_info info);
    static napi_value OnConnectionStateChange(napi_env env, napi_callback_info info);
    static napi_value OffConnectionStateChange(napi_env env, napi_callback_info info);
    static napi_value OnMtuChange(napi_env env, napi_callback_info info);
    static napi_value OffMtuChange(napi_env env, napi_callback_info info);
    static napi_value Connect(napi_env env, napi_callback_info info);
    static napi_value Disconnect(napi_env env, napi_callback_info info);
    static napi_value Close(napi_env env, napi_callback_info info);
    static napi_value GetServices(napi_env env, napi_callback_info info);
    static napi_value GetServicesByUuid(napi_env env, napi_callback_info info);
    static napi_value RequestMtuSize(napi_env env, napi_callback_info info);
    static napi_value ReadProperty(napi_env env, napi_callback_info info);
    static napi_value CallMethod(napi_env env, napi_callback_info info);
    static napi_value ReadPropertyByUuid(napi_env env, napi_callback_info info);
    static napi_value WriteProperty(napi_env env, napi_callback_info info);
    static napi_value ReadDescriptor(napi_env env, napi_callback_info info);
    static napi_value WriteDescriptor(napi_env env, napi_callback_info info);
    static napi_value SetPropertyNotification(napi_env env, napi_callback_info info);
    static napi_value SetPropertyIndication(napi_env env, napi_callback_info info);

    std::shared_ptr<SsapClient> &GetClient()
    {
        return client_;
    }

    std::shared_ptr<NapiNearlinkSsapClientCallback> GetCallback()
    {
        return callback_;
    }

    std::shared_ptr<NearlinkRemoteDevice> GetDevice()
    {
        return device_;
    }

    explicit NapiNearlinkSsapClient(std::string &deviceId)
    {
        HILOGI("enter");
        device_ = std::make_shared<NearlinkRemoteDevice>(deviceId, 1);
        client_ = SsapClient::CreateSsapClient(device_);
        callback_ = std::make_shared<NapiNearlinkSsapClientCallback>();
        callback_->SetClient(this);
    }
    ~NapiNearlinkSsapClient()
    {
        callback_->SetClient(nullptr);
    }

    static thread_local napi_ref consRef_;

private:
    std::shared_ptr<SsapClient> client_ = nullptr;
    std::shared_ptr<NapiNearlinkSsapClientCallback> callback_;
    std::shared_ptr<NearlinkRemoteDevice> device_ = nullptr;
};
} // namespace Nearlink
} // namespace OHOS
#endif /* NAPI_NEARLINK_SSAP_CLIENT_H_ */