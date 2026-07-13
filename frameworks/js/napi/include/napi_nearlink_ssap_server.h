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
#ifndef NAPI_NEARLINK_SSAP_SERVER_H_
#define NAPI_NEARLINK_SSAP_SERVER_H_

#include <vector>
#include "nearlink_ssap_server.h"
#include "log.h"
#include "napi_nearlink_ssap_server_callback.h"
#include "nearlink_safe_list.h"

namespace OHOS {
namespace Nearlink {
class NapiNearlinkSsapServer {

public:
    static napi_value CreateServer(napi_env env, napi_callback_info info);
    static void DefineSsapServerJSClass(napi_env env, napi_value exports);
    static napi_value SsapServerConstructor(napi_env env, napi_callback_info info);

    static napi_value OnConnectionStateChange(napi_env env, napi_callback_info info);
    static napi_value OffConnectionStateChange(napi_env env, napi_callback_info info);
    static napi_value OnPropertyRead(napi_env env, napi_callback_info info);
    static napi_value OffPropertyRead(napi_env env, napi_callback_info info);
    static napi_value OnPropertyWrite(napi_env env, napi_callback_info info);
    static napi_value OffPropertyWrite(napi_env env, napi_callback_info info);
    static napi_value OnMtuChange(napi_env env, napi_callback_info info);
    static napi_value OffMtuChange(napi_env env, napi_callback_info info);

    static napi_value AddService(napi_env env, napi_callback_info info);
    static napi_value Close(napi_env env, napi_callback_info info);
    static napi_value RemoveService(napi_env env, napi_callback_info info);
    static napi_value NotifyPropertyChanged(napi_env env, napi_callback_info info);
    static napi_value SendResponse(napi_env env, napi_callback_info info);
    static napi_value Disconnect(napi_env env, napi_callback_info info);
    std::shared_ptr<SsapServer> &GetServer()
    {
        return server_;
    }
    std::shared_ptr<NapiNearlinkSsapServerCallback> GetCallback()
    {
        return callback_;
    }

    NapiNearlinkSsapServer()
    {
        HILOGI("enter");
        callback_ = std::make_shared<NapiNearlinkSsapServerCallback>();
        std::shared_ptr<SsapServerCallback> tmp = std::static_pointer_cast<SsapServerCallback>(callback_);
        server_ = SsapServer::CreateSsapServer(tmp);
    }
    ~NapiNearlinkSsapServer() = default;

    static thread_local napi_ref consRef_;
    static NearlinkSafeList<std::string> deviceList;
private:
    std::shared_ptr<SsapServer> server_ = nullptr;
    std::shared_ptr<NapiNearlinkSsapServerCallback> callback_;
    static napi_value DescriptorInit(napi_env env, napi_value exports);
    static napi_value PermissionInit(napi_env env, napi_value exports);
};
}  // namespace Nearlink
}  // namespace OHOS
#endif /* NAPI_NEARLINK_SSAP_SERVER_H_ */