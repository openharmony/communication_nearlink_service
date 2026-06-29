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

#include "nearlink_sle_advertiser.h"
#include "napi_async_callback.h"
#include "napi_event_subscribe_module.h"

namespace OHOS {
namespace Nearlink {

const char *const  REGISTER_ADVERTISING_STATE_INFO_NAME = "advertisingStateChange";
class NapiNearlinkAdvertiseCallback : public SleAdvertiseCallback {
public:
    NapiNearlinkAdvertiseCallback();
    ~NapiNearlinkAdvertiseCallback() override = default;

    static std::shared_ptr<NapiNearlinkAdvertiseCallback> GetInstance(void);

    void OnStartResultEvent(int result, int advHandle) override;
    void OnStopResultEvent(int result, int advHandle) override;
    void OnSetAdvDataEvent(int result) override;
    void OnGetAdvHandleEvent(int result, int advHandle) override;
    NapiAsyncWorkMap asyncPromiseMap_ {};
    NapiEventSubscribeModule eventSubscribe;
private:
    std::mutex callbackMutex_ {};
};

class AdvertisingStateChangeInfo : public NapiNativeObject {
public:
    AdvertisingStateChangeInfo(int advHandle, int advState) : advHandle_(advHandle), advState_(advState) {}
    ~AdvertisingStateChangeInfo() override = default;

    napi_value ToNapiValue(napi_env env) const override;
private:
    int advHandle_;
    int advState_;
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // NAPI_NEARLINK_SLE_ADVERTISE_CALLBACK_H
