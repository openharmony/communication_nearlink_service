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

#ifndef OHOS_NEARLINK_STANDARD_ASC_CALLBACK_PROXY_H
#define OHOS_NEARLINK_STANDARD_ASC_CALLBACK_PROXY_H

#include "iremote_proxy.h"
#include "i_nearlink_asc_callback.h"

namespace OHOS {
namespace Nearlink {
class NearlinkASCCallbackProxy : public IRemoteProxy<INearlinkASCCallback> {
public:
    explicit NearlinkASCCallbackProxy(const sptr<IRemoteObject> &impl)
        : IRemoteProxy<INearlinkASCCallback>(impl)
    {
        uid_ = getuid();
    }
    ~NearlinkASCCallbackProxy()
    {}

    void OnAudioControl(const NearlinkRawAddress &device, const NearlinkASCAudioControlResult& result) override;
    void OnAddSleAudioDevice(const NearlinkRawAddress &device, uint32_t streamType, int32_t mediaVolume,
        int32_t callVolume) override;
    void OnDeleteSleAudioDevice(const NearlinkRawAddress &device) override;
    void OnSleAudioDeviceActionChanged(const NearlinkRawAddress &device, const NearlinkASCAudioStreamInfo &streamInfo,
        int action) override;
    void OnAddSleVirtualAudioDevice(const NearlinkRawAddress &device, AudioStreamType streamType) override;
    void OnDeleteSleVirtualAudioDevice(const NearlinkRawAddress &device) override;
    uid_t GetUid() override;
private:
    ErrCode InnerTransact(uint32_t code, MessageOption &flags, MessageParcel &data, MessageParcel &reply);
    static inline BrokerDelegator<NearlinkASCCallbackProxy> delegator_;
    uid_t uid_ = 0;
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // OHOS_NEARLINK_STANDARD_ASC_CALLBACK_PROXY_H