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

#ifndef OHOS_NEARLINK_CCP_CLIENT_SERVER_H
#define OHOS_NEARLINK_CCP_CLIENT_SERVER_H

#include "nearlink_def.h"
#include "nearlink_ccp_client_stub.h"
#include "nearlink_types.h"

namespace OHOS {
namespace Nearlink {

class NearlinkCcpClientServer : public NearlinkCcpClientStub {
public:
    explicit NearlinkCcpClientServer();
    ~NearlinkCcpClientServer() override;

    int32_t PhoneStateChanged(int32_t callId, int32_t numActive, int32_t numHeld, int32_t callState,
        const std::string &number) override;
    int32_t CallDetailsChanged(int32_t callId, int32_t callState, int32_t callType) override;
private:
    NEARLINK_DISALLOW_COPY_AND_ASSIGN(NearlinkCcpClientServer);
};

}  // namespace Nearlink
}  // namespace OHOS
#endif // OHOS_NEARLINK_CCP_CLIENT_SERVER_H
