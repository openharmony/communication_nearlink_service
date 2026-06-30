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

#ifndef OHOS_NEARLINK_VCP_CLIENT_SERVER_H
#define OHOS_NEARLINK_VCP_CLIENT_SERVER_H

#include <mutex>
#include "nearlink_def.h"
#include "nearlink_vcp_client_stub.h"
#include "nearlink_types.h"
#include "iservice_registry.h"
#include "system_ability.h"

namespace OHOS {
namespace Nearlink {

enum VolumeStreamType : uint8_t {
    SLE_STREAM_MEDIA = 0, // 媒体
    SLE_STREAM_CALL,      // 通话
    SLE_STREAM_MAX,       // 流类型数目
};

class NearlinkVcpClientServer : public NearlinkVcpClientStub {
public:
    explicit NearlinkVcpClientServer();
    ~NearlinkVcpClientServer() override;

    NlErrCode SetDeviceAbsoluteVolume(const NearlinkRawAddress &addr, int32_t volumeLevel, uint8_t streamType) override;
    NlErrCode GetDeviceMediaVolume(const NearlinkRawAddress &addr, int32_t &mediaVolume) override;
    NlErrCode GetDeviceCallVolume(const NearlinkRawAddress &addr, int32_t &callVolume) override;
private:
    NEARLINK_DISALLOW_COPY_AND_ASSIGN(NearlinkVcpClientServer);
};

}  // namespace NearLink
}  // namespace OHOS
#endif // OHOS_NEARLINK_VCP_CLIENT_SERVER_H