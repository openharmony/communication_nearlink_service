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

#include "nearlink_cloud_pair_server.h"
#include "interface_cloud_pair_service.h"
#include "log_util.h"
#include "nearlink_cloud_pair_def.h"
namespace OHOS {
namespace Nearlink {
NearlinkCloudPairServer::NearlinkCloudPairServer()
{
    HILOGI("NearlinkCloudPairServer called.");
}

NearlinkCloudPairServer::~NearlinkCloudPairServer()
{
    HILOGI("~NearlinkCloudPairServer called.");
}

NlErrCode NearlinkCloudPairServer::UpdateCloudDeviceInfoList(std::vector<NearlinkCloudPairDevice> &cloudDeviceInfos)
{
    if (InterfaceCloudPairService::GetInstance().UpdateCloudDeviceInfoList(cloudDeviceInfos)) {
        return NL_NO_ERROR;
    }
    return NL_ERR_INTERNAL_ERROR;
}

NlErrCode NearlinkCloudPairServer::GetCloudPairState(const std::string &address, int32_t &cloudPairState)
{
    if (InterfaceCloudPairService::GetInstance().GetCloudPairState(address, cloudPairState)) {
        return NL_NO_ERROR;
    }
    return NL_ERR_INTERNAL_ERROR;
}

} // namespace Nearlink
} // namespace OHOS