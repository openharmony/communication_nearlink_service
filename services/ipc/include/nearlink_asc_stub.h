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

#ifndef OHOS_NEARLINK_ASC_STUB_H
#define OHOS_NEARLINK_ASC_STUB_H

#include <map>
#include "iremote_stub.h"
#include "i_nearlink_asc.h"
#include "nearlink_permission_item.h"

namespace OHOS {
namespace Nearlink {
class NearlinkASCStub : public IRemoteStub<INearlinkASC> {
public:
    NearlinkASCStub();
    virtual ~NearlinkASCStub();

    int32_t OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;

    using NearlinkASCFunc = int32_t (*)(NearlinkASCStub *stub, MessageParcel &data, MessageParcel &reply);
    using NearlinkASCFuncPerm = std::pair<NearlinkASCFunc, std::shared_ptr<NearLinkPermissionItem>>;

private:
    static int32_t RegisterApplicationInner(NearlinkASCStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t DeregisterApplicationInner(NearlinkASCStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t AudioControlInner(NearlinkASCStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t GetAudioDeviceListInner(NearlinkASCStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t GetVirtualAudioDeviceListInner(NearlinkASCStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t GetSupportStreamTypeInner(NearlinkASCStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t GetAudioDeviceCodecInfoInner(NearlinkASCStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t SetActiveSinkDeviceInner(NearlinkASCStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t GetDualRecordAbilityInner(NearlinkASCStub *stub, MessageParcel &data, MessageParcel &reply);
    static int32_t GetKaraokeAbilityInner(NearlinkASCStub *stub, MessageParcel &data, MessageParcel &reply);

private:
    std::map<uint32_t, NearlinkASCFuncPerm> memberFuncMap_;
    DISALLOW_COPY_AND_MOVE(NearlinkASCStub);
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // OHOS_NEARLINK_ASC_STUB_H