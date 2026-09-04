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

#ifndef ANI_NEARLINK_DATATRANSFER_CALLBACK_H
#define ANI_NEARLINK_DATATRANSFER_CALLBACK_H

#include <mutex>
#include <shared_mutex>
#include "stdexcept"

#include "ohos.nearlink.dataTransfer.proj.hpp"
#include "ohos.nearlink.dataTransfer.impl.hpp"
#include "taihe/runtime.hpp"
#include "ani_event_module.h"
#include "nearlink_sle_datatransfer.h"

namespace OHOS {
namespace Nearlink {

class AniNearlinkDataTransferCallback : public SleDataTransferCallback {
public:
    AniNearlinkDataTransferCallback();
    ~AniNearlinkDataTransferCallback() override = default;

    static std::shared_ptr<AniNearlinkDataTransferCallback> GetInstance();

    void RegisterConnectionStateChanged(
        ::taihe::callback_view<void(::ohos::nearlink::dataTransfer::ConnectionResult const&)> callback);
    void DeregisterConnectionStateChanged(
        ::taihe::optional_view<::taihe::callback<void(::ohos::nearlink::dataTransfer::ConnectionResult const&)>>
        callback);
    void RegisterReadData(
        ::taihe::callback_view<void(::ohos::nearlink::dataTransfer::DataParams const&)> callback);
    void DeregisterReadData(
        ::taihe::optional_view<::taihe::callback<void(::ohos::nearlink::dataTransfer::DataParams const&)>> callback);
    void OnConnectionStateChanged(const ConnectionParams &result) override;
    void OnReceiveData(const DataParams &result) override;

    EventModule<void(::ohos::nearlink::dataTransfer::ConnectionResult const& data)> connectionStateChangedEvent_;
    EventModule<void(::ohos::nearlink::dataTransfer::DataParams const& data)> readDataEvent_;

private:
    std::mutex callbackMutex_{};
};
}  // namespace Nearlink
}  // namespace OHOS

#endif // ANI_NEARLINK_DATATRANSFER_CALLBACK_H
