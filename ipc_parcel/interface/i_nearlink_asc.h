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

#ifndef I_NEARLINK_ASC_H
#define I_NEARLINK_ASC_H

#include "iremote_broker.h"
#include "nearlink_service_ipc_interface_code.h"
#include "i_nearlink_asc_callback.h"
#include "../parcel/nearlink_raw_address.h"
#include "nearlink_errorcode.h"

namespace OHOS {
namespace Nearlink {
namespace {
const std::string PROFILE_ASC = "ASCServer";
const uint32_t MAX_AUDIO_DEVICE_COUNT = 10;
const uint32_t MAX_VIRTUAL_AUDIO_DEVICE_COUNT = 100;
}  // namespace
class INearlinkASC : public OHOS::IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.ipc.INearlinkASC");

    virtual NlErrCode RegisterApplication(const sptr<INearlinkASCCallback> &callback) = 0;
    virtual NlErrCode DeregisterApplication(const sptr<INearlinkASCCallback> &callback) = 0;
    virtual NlErrCode AudioControl(const NearlinkRawAddress &device, AudioStreamType streamType, int cmd) = 0;
    virtual NlErrCode GetAudioDeviceList(std::vector<NearlinkRawAddress> &devices) = 0;
    virtual NlErrCode GetVirtualAudioDeviceList(std::vector<NearlinkRawAddress> &devices) = 0;
    virtual NlErrCode GetSupportStreamType(const NearlinkRawAddress &device, uint32_t& supportStreamType) = 0;
    virtual NlErrCode GetAudioDeviceCodecInfo(const NearlinkRawAddress &device,
        std::map<AudioStreamType, AudioStreamCodecInfo> &info) = 0;
    virtual NlErrCode SetActiveSinkDevice(const NearlinkRawAddress &device, uint32_t supportStreamType) = 0;
    virtual NlErrCode GetDualRecordAbility(const NearlinkRawAddress &device, bool &isSupport) = 0;
    virtual NlErrCode GetKaraokeAbility(const NearlinkRawAddress &device, bool &isSupport) = 0;
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // I_NEARLINK_ASC_H