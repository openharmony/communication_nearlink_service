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

#include "ohos.nearlink.remoteDevice.proj.hpp"
#include "ohos.nearlink.remoteDevice.impl.hpp"
#include "ani_nearlink_utils.h"
#include "ani_nearlink_error.h"
#include "taihe/runtime.hpp"
#include "stdexcept"
#include "log.h"
#include "nearlink_errorcode.h"
#include "nearlink_host.h"

namespace OHOS {
namespace Nearlink {
using namespace Nearlink;

class RemoteDeviceImpl {
public:
    explicit RemoteDeviceImpl(taihe::string_view address)
    {
        HILOGI("enter");

        std::string remoteAddr = std::string(address);
        device_ = std::make_shared<NearlinkRemoteDevice>(remoteAddr, ADAPTER_SLE);
    }

    ::ohos::nearlink::remoteDevice::DeviceInformation GetDeviceInformation()
    {
        HILOGI("enter");
        DeviceInformation deviceInfo;
        device_->GetDeviceInformation(deviceInfo);
        return {
            .manufacturerData = static_cast<::taihe::string>(deviceInfo.GetManufacturerData()),
            .modelData = static_cast<::taihe::string>(deviceInfo.GetModelData())
        };
    }

private:
    std::shared_ptr<NearlinkRemoteDevice> device_ = nullptr;
};

::ohos::nearlink::remoteDevice::RemoteDevice CreateRemoteDevice(taihe::string_view address)
{
    HILOGI("enter");
    std::string remoteAddr = std::string(address);
    bool checkRet = CheckDeviceIdParam(remoteAddr);
    ANI_NL_ASSERT_RETURN(checkRet, NL_ERR_INVALID_PARAM,
        (taihe::make_holder<RemoteDeviceImpl, ::ohos::nearlink::remoteDevice::RemoteDevice>(nullptr)));
    return taihe::make_holder<RemoteDeviceImpl, ::ohos::nearlink::remoteDevice::RemoteDevice>(remoteAddr);
}
}  // namespace Nearlink
}  // namespace OHOS

// Since these macros are auto-generate, lint will cause false positive.
// NOLINTBEGIN
TH_EXPORT_CPP_API_CreateRemoteDevice(OHOS::Nearlink::CreateRemoteDevice);
// NOLINTEND