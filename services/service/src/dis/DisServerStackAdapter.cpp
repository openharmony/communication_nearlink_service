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
#include <memory>
#include "DisServerStackAdapter.h"
#include "SleServiceFfrtLog.h"
#include "nlstk_dis_server.h"

namespace OHOS {
namespace Nearlink {
namespace {
// 调用方需释放内存
bool ConvertStringData(const std::string& inStr, NLSTK_VariableData_S *outValue)
{
    NL_CHECK_RETURN_RET(outValue, false, "outValue is nullptr.");
    size_t len = inStr.size();
    NL_CHECK_RETURN_RET(len > 0, false, "invalid len(%{public}zu).", len);

    outValue->data = new (std::nothrow) uint8_t[len];
    NL_CHECK_RETURN_RET(outValue->data, false, "data is nullptr.");
    outValue->len = len;
    std::copy(inStr.begin(), inStr.end(), outValue->data);
    return true;
}
}

DisServerStackAdapter::DisServerStackAdapter()
{}

DisServerStackAdapter::~DisServerStackAdapter() = default;

void DisServerStackAdapter::FreeDisInfo(NLSTK_DeviceInfo_S *info)
{
    NL_CHECK_RETURN(info, "info is null");
    if (info->softwareVersion.data) {
        delete[] info->softwareVersion.data;
        info->softwareVersion.data = nullptr;
    }

    if (info->deviceLocalAlias.data) {
        delete[] info->deviceLocalAlias.data;
        info->deviceLocalAlias.data = nullptr;
    }

    if (info->manufacturerInfo.data) {
        delete[] info->manufacturerInfo.data;
        info->manufacturerInfo.data = nullptr;
    }
}

void DisServerStackAdapter::CreateServerInfo(DisServerInfo &info)
{
    // 当前仅使用4个设备信息
    NLSTK_DeviceInfo_S devInfo {};

    // 组装softwareVersion
    if (!ConvertStringData(info.versionInfo_, &devInfo.softwareVersion)) {
        HILOGE("convert softwareVersion failed.");
    }
    // 组装deviceLocalAlias
    if (!ConvertStringData(info.nameInfo_, &devInfo.deviceLocalAlias)) {
        HILOGE("convert deviceLocalAlias failed.");
    }
    // 组装manufacturerInfo
    if (!ConvertStringData(info.manufactureInfo_, &devInfo.manufacturerInfo)) {
        HILOGE("convert deviceLocalAlias failed.");
    }
    // 组装appearanceId
    devInfo.deviceAppearance = info.appearanceId_;

    NLSTK_Errcode_E ret = NLSTK_DisCreateInstance(&devInfo);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        HILOGE("CreateServerInfo failed, ret(%{public}d)", ret);
    }
    FreeDisInfo(&devInfo);
}

void DisServerStackAdapter::DestroyServerInfo()
{
    NLSTK_Errcode_E ret = NLSTK_DisDestroyInstance();
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        HILOGE("DestroyServerInfo failed, ret(%{public}d)", ret);
    }
}

void DisServerStackAdapter::UpdateLocalDeviceName(const std::string name)
{
    NLSTK_VariableData_S value {};
    if (!ConvertStringData(name, &value)) {
        HILOGE("convert name failed.");
        return;
    }
    NLSTK_Errcode_E ret = NLSTK_DisUpdateLocalDeviceName(&value);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        HILOGE("UpdateLocalDeviceName failed, ret(%{public}d)", ret);
    }
    delete[] value.data;
    value.data = nullptr;
}
} // namespace Nearlink
} // namespace OHOS