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

#ifndef I_REMOTE_DEVICE_QUERY_H
#define I_REMOTE_DEVICE_QUERY_H

#include <cstdint>
#include <string>
#include "raw_address.h"

namespace OHOS {
namespace Nearlink {

class IRemoteDeviceQuery {
public:
    static IRemoteDeviceQuery *GetInstance();

    virtual int GetManufacturerBusinessType(const RawAddress &device) = 0;
    virtual void SetPeerDeviceTypeToController(const RawAddress &device) = 0;
    virtual bool IsAudioDevice(const std::string &address) = 0;
    virtual uint8_t GetPeerDeviceAddrType(const RawAddress &device) = 0;
    virtual void SetConnDirectActive(const RawAddress &device) = 0;

protected:
    virtual ~IRemoteDeviceQuery() = default;
};

} // namespace Nearlink
} // namespace OHOS

#endif // I_REMOTE_DEVICE_QUERY_H
