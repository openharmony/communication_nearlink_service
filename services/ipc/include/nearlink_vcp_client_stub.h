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

#ifndef OHOS_NEARLINK_VCP_CLIENT_STUB_H
#define OHOS_NEARLINK_VCP_CLIENT_STUB_H

#include <map>
#include <memory>

#include "iremote_stub.h"
#include "i_nearlink_vcp_client.h"
#include "nearlink_permission_item.h"

namespace OHOS {
namespace Nearlink {
class NearlinkVcpClientStub : public IRemoteStub<INearlinkVcpClient> {
public:
    NearlinkVcpClientStub();
    virtual ~NearlinkVcpClientStub();

    int32_t OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;

    using NearlinkVcpClientFunc = int32_t (*)(NearlinkVcpClientStub *stub, MessageParcel &data, MessageParcel &reply);
    using NearlinkVcpClientFuncPerm = std::pair<NearlinkVcpClientFunc, std::shared_ptr<NearLinkPermissionItem>>;

private:
    static int32_t SetDeviceAbsoluteVolumeInner(NearlinkVcpClientStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t GetDeviceMediaVolumeInner(NearlinkVcpClientStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t GetDeviceCallVolumeInner(NearlinkVcpClientStub *stub, MessageParcel &data, MessageParcel &reply);

private:
    std::map<uint32_t, NearlinkVcpClientFuncPerm> memberFuncMap_;
    DISALLOW_COPY_AND_MOVE(NearlinkVcpClientStub);
};

}  // namespace Nearlink
}  // namespace OHOS
#endif  // OHOS_NEARLINK_VCP_CLIENT_STUB_H