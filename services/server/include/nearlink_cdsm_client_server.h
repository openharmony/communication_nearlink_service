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

#ifndef OHOS_NEARLINK_CDSM_CLIENT_SERVER_H
#define OHOS_NEARLINK_CDSM_CLIENT_SERVER_H

#include "nearlink_types.h"
#include "nearlink_cdsm_client_stub.h"

namespace OHOS {
namespace Nearlink {
class NearlinkCdsmClientServer : public NearlinkCdsmClientStub {
public:
    NearlinkCdsmClientServer();
    ~NearlinkCdsmClientServer() override;
    NlErrCode RegisterCdsmClientCallback(const NearlinkRawAddress &addr,
                                         const sptr<INearlinkCdsmClientCallback> &callback) override;
    NlErrCode DeregisterCdsmClientCallback(const NearlinkRawAddress &addr) override;
    NlErrCode GetCdsmInfo(const NearlinkRawAddress &addr, NearlinkCdsInfoParcel &cdsInfo) override;
private:
    NEARLINK_DECLARE_IMPL();
    NEARLINK_DISALLOW_COPY_AND_ASSIGN(NearlinkCdsmClientServer);
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // OHOS_NEARLINK_CDSM_CLIENT_SERVER_H