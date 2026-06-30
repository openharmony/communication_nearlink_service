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

#ifndef NEARLINK_CLOUD_PAIR_DEF_H
#define NEARLINK_CLOUD_PAIR_DEF_H

#include <string>
#include <vector>

namespace OHOS {
namespace Nearlink {

class CloudPairDeviceInfo {
public:
    CloudPairDeviceInfo() = default;
    ~CloudPairDeviceInfo() = default;

    void SetBtAddr(const std::string &btAddr)
    {
        btAddr_ = btAddr;
    }

    std::string GetBtAddr() const
    {
        return btAddr_;
    }

    void SetDeviceName(const std::string &deviceName)
    {
        deviceName_ = deviceName;
    }

    std::string GetDeviceName() const
    {
        return deviceName_;
    }

    void SetToken(const std::vector<uint8_t> &token)
    {
        token_ = token;
    }

    std::vector<uint8_t> GetToken() const
    {
        return token_;
    }

    void SetReportAddr(const std::string &reportAddr)
    {
        reportAddr_ = reportAddr;
    }

    std::string GetReportAddr() const
    {
        return reportAddr_;
    }

    void SetMembersAddr(const std::vector<std::string> &membersAddr)
    {
        membersAddr_ = membersAddr;
    }

    std::vector<std::string> GetMembersAddr() const
    {
        return membersAddr_;
    }

    void SetModel(const std::string &model)
    {
        model_ = model;
    }

    std::string GetModel() const
    {
        return model_;
    }

    void SetSubModelId(const std::string &subModelId)
    {
        subModelId_ = subModelId;
    }

    std::string GetSubModelId() const
    {
        return subModelId_;
    }

    void SetDeviceIconId(const std::string &deviceIconId)
    {
        deviceIconId_ = deviceIconId;
    }

    std::string GetDeviceIconId() const
    {
        return deviceIconId_;
    }
protected:
    std::string btAddr_;
    std::string deviceName_;
    std::vector<uint8_t> token_; // token信息
    std::string reportAddr_ = "00:00:00:00:00:00";
    std::vector<std::string> membersAddr_;  // 组地址信息
    std::string model_{""};
    std::string subModelId_{""};
    std::string deviceIconId_{""};
};

} // namespace Nearlink
} // namespace OHOS

#endif //NEARLINK_CLOUD_PAIR_DEF_H