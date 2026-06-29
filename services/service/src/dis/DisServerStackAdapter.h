/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#ifndef DIS_SERVER_STACK_ADAPTER_H
#define DIS_SERVER_STACK_ADAPTER_H

#include <string>
#include "nlstk_dis_def.h"
#include "nlstk_dis_server.h"

namespace OHOS {
namespace Nearlink {

constexpr uint32_t DIS_DEVICE_SMARTPHONE = 0x000201; // 0x000201 smart phone

struct DisServerInfo {
    DisServerInfo() : appearanceId_(DIS_DEVICE_SMARTPHONE) {}
    std::string versionInfo_;
    std::string nameInfo_;
    std::string manufactureInfo_;
    uint32_t appearanceId_;
};

class DisServerStackAdapter {
public:
    DisServerStackAdapter();
    ~DisServerStackAdapter();
    void CreateServerInfo(DisServerInfo &info);
    void DestroyServerInfo();
    void UpdateLocalDeviceName(const std::string name);
private:
    void FreeDisInfo(NLSTK_DeviceInfo_S *info);
};

} // namespace Nearlink
} // namespace OHOS

#endif // DIS_SERVER_STACK_ADAPTER_H