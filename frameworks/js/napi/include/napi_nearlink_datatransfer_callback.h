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

#ifndef NAPI_NEARLINK_ADVERTISE_CALLBACK_H
#define NAPI_NEARLINK_ADVERTISE_CALLBACK_H

#include "nearlink_sle_datatransfer.h"
#include "napi_async_callback.h"
#include "napi_event_subscribe_module.h"

namespace OHOS::Nearlink {
const char *const SLE_DATATRANSFER_CALLBACK_CONNECTION_STATE_CHANGE = "connectionStateChanged";
const char *const SLE_DATATRANSFER_CALLBACK_READ_DATA = "readData";
class NapiNearlinkDataTransferCallback : public SleDataTransferCallback {
public:
    NapiNearlinkDataTransferCallback();
    ~NapiNearlinkDataTransferCallback() override = default;

    static std::shared_ptr<NapiNearlinkDataTransferCallback> GetInstance(void);

    void OnConnectionStateChanged(const ConnectionParams &result) override;

    void OnReceiveData(const DataParams &result) override;

    NapiAsyncWorkMap asyncPromiseMap_{};
    NapiEventSubscribeModule eventSubscribe;

private:
    std::mutex callbackMutex_{};
};

class ConnectionResult : public NapiNativeObject {
public:
    ConnectionResult(std::string address, std::string uuid, int mtu, int state)
        : address_(address), uuid_(uuid), mtu_(mtu), state_(state)
    {}
    ~ConnectionResult() override = default;

    napi_value ToNapiValue(napi_env env) const override;

private:
    std::string address_;
    std::string uuid_;
    int mtu_;
    int state_;
};

class DataResult : public NapiNativeObject {
public:
    explicit DataResult(const DataParams &dataParams) : dataParams_(dataParams) {}
    ~DataResult() override = default;

    napi_value ToNapiValue(napi_env env) const override;

private:
    DataParams dataParams_;
};
}  // namespace OHOS::Nearlink
#endif  // NAPI_NEARLINK_SLE_DATATRANSFER_CALLBACK_H
