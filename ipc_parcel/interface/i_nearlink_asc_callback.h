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

#ifndef OHOS_NEARLINK_STANDARD_ASC_CALLBACK_INTERFACE_H
#define OHOS_NEARLINK_STANDARD_ASC_CALLBACK_INTERFACE_H

#include "iremote_broker.h"
#include "nearlink_ASC_source.h"
#include "nearlink_service_ipc_interface_code.h"
#include "nearlink_raw_address.h"
#include "nearlink_asc_audio_control_result.h"
#include "nearlink_asc_audio_stream_info.h"

namespace OHOS {
namespace Nearlink {
class INearlinkASCCallback : public OHOS::IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.ipc.INearlinkASCCallback");

    virtual void OnAudioControl(const NearlinkRawAddress &device, const NearlinkASCAudioControlResult& result) = 0;
    virtual void OnAddSleAudioDevice(const NearlinkRawAddress &device, uint32_t streamType, int32_t mediaVolume,
        int32_t callVolume) = 0;
    virtual void OnDeleteSleAudioDevice(const NearlinkRawAddress &device) = 0;
    virtual void OnSleAudioDeviceActionChanged(const NearlinkRawAddress &device,
        const NearlinkASCAudioStreamInfo &streamInfo, int action) = 0;
    virtual void OnAddSleVirtualAudioDevice(const NearlinkRawAddress &device, AudioStreamType streamType) = 0;
    virtual void OnDeleteSleVirtualAudioDevice(const NearlinkRawAddress &device) = 0;
    virtual uid_t GetUid() = 0;
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // OHOS_NEARLINK_STANDARD_ASC_CALLBACK_INTERFACE_H