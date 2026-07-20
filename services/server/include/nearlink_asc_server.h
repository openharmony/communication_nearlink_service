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

#ifndef OHOS_NEARLINK_STANDARD_ASC_SERVER_H
#define OHOS_NEARLINK_STANDARD_ASC_SERVER_H

#include <map>

#include "nearlink_asc_stub.h"
#include "nearlink_types.h"
#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "system_ability.h"

namespace OHOS {
namespace Nearlink {
// 音频框架UID
constexpr int AUDIO_SERVER_UID          = 1041;

class NearlinkASCServer : public NearlinkASCStub {
public:
    explicit NearlinkASCServer();
    ~NearlinkASCServer() override;

    NlErrCode RegisterApplication(const sptr<INearlinkASCCallback> &callback) override;
    NlErrCode DeregisterApplication(const sptr<INearlinkASCCallback> &callback) override;

    NlErrCode AudioControl(const NearlinkRawAddress &device, AudioStreamType streamType, int cmd) override;
    NlErrCode GetAudioDeviceList(std::vector<NearlinkRawAddress>& devices) override;
    NlErrCode GetVirtualAudioDeviceList(std::vector<NearlinkRawAddress> &devices) override;
    NlErrCode GetSupportStreamType(const NearlinkRawAddress &device, uint32_t& supportStreamType) override;
    NlErrCode GetAudioDeviceCodecInfo(const NearlinkRawAddress &device, std::map<AudioStreamType,
        AudioStreamCodecInfo> &info) override;
    NlErrCode SetActiveSinkDevice(const NearlinkRawAddress &device, uint32_t supportStreamType) override;
    NlErrCode GetDualRecordAbility(const NearlinkRawAddress &device, bool &isSupport) override;
    NlErrCode GetKaraokeAbility(const NearlinkRawAddress &device, bool &isSupport) override;
private:
   NEARLINK_DISALLOW_COPY_AND_ASSIGN(NearlinkASCServer);
   NEARLINK_DECLARE_IMPL();
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // OHOS_NEARLINK_STANDARD_ASC_SERVER_H