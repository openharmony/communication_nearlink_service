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

#include "ohos.nearlink.dataTransfer.impl.hpp"
#include "ani_nearlink_datatransfer_callback.h"
#include "log_util.h"

namespace OHOS {
namespace Nearlink {

AniNearlinkDataTransferCallback::AniNearlinkDataTransferCallback() = default;

std::shared_ptr<AniNearlinkDataTransferCallback> AniNearlinkDataTransferCallback::GetInstance()
{
    static std::shared_ptr<AniNearlinkDataTransferCallback> instance =
        std::make_shared<AniNearlinkDataTransferCallback>();
    return instance;
}

void AniNearlinkDataTransferCallback::OnConnectionStateChanged(const ConnectionParams &result)
{
    ::ohos::nearlink::dataTransfer::ConnectionResult taiheResult = {
        .address = static_cast<::taihe::string>(result.GetAddress()),
        .uuid = static_cast<::taihe::string>(result.GetUuid()),
        .mtu = 0,
        .state = ohos::nearlink::constant::ConnectionState::from_value(result.GetState())
    };

    connectionStateChangedEvent_.PublishEvent(taiheResult);
}

void AniNearlinkDataTransferCallback::OnReceiveData(const DataParams &result)
{
    size_t valueSize = 0;
    const uint8_t *valueData = result.GetData(&valueSize).get();
    ::taihe::array<uint8_t> taiheData = ::taihe::array<uint8_t>(::taihe::copy_data_t{}, valueData, valueSize);
    ::ohos::nearlink::dataTransfer::DataParams taiheResult = {
        .address = static_cast<::taihe::string>(result.GetAddress()),
        .uuid = static_cast<::taihe::string>(result.GetUuid()),
        .data = std::move(taiheData)
    };

    readDataEvent_.PublishEvent(taiheResult);
}

}  // namespace Nearlink
}  // namespace OHOS
