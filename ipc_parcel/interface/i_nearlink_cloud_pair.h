/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_NEARLINK_STANDARD_CLOUD_PAIR_INTERFACE_H
#define OHOS_NEARLINK_STANDARD_CLOUD_PAIR_INTERFACE_H

#include "nearlink_service_ipc_interface_code.h"
#include "iremote_broker.h"
#include "nearlink_errorcode.h"
#include "nearlink_cloud_pair_device.h"

namespace OHOS::Nearlink {
namespace {
const std::string CLOUD_PAIR_SERVER = "NearlinkCloudPairServer";
}  // namespace

class INearlinkCloudPair : public OHOS::IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.ipc.INearlinkCloudPair");

    virtual NlErrCode UpdateCloudDeviceInfoList(std::vector<NearlinkCloudPairDevice> &cloudDeviceInfos) = 0;
    virtual NlErrCode GetCloudPairState(const std::string &address, int &cloudPairState) = 0;
};
} // namespace OHOS::Nearlink

#endif // OHOS_NEARLINK_STANDARD_CLOUD_PAIR_INTERFACE_H