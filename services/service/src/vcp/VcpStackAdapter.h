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

#ifndef VCP_STACK_ADAPTER_H
#define VCP_STACK_ADAPTER_H

#include <list>
#include "nearlink_types.h"
#include "raw_address.h"
#include "VcpDefines.h"
#include "nlstk_mcp_volume_client.h"

namespace OHOS {
namespace Nearlink {

struct StreamVolume {
    StreamVolume(int32_t volume, uint8_t volumeStreamType)
        : volume_(volume), volumeStreamType_(volumeStreamType) {}

    StreamVolume(int32_t volume, uint8_t volumeStreamType, uint8_t additionalInfo)
        : volume_(volume), volumeStreamType_(volumeStreamType), additionalInfo_(additionalInfo) {}
    int32_t volume_ = VCP_DEFAULT_VOLUME_LEVEL;
    uint8_t volumeStreamType_ = 0;
    uint8_t additionalInfo_ = 0;
};

enum VolumeStreamType : uint8_t {
    SLE_STREAM_MEDIA = 0, // 媒体
    SLE_STREAM_CALL,      // 通话
    SLE_STREAM_MAX,       // 流类型数目
};

class VcpStackCallback {
public:
    virtual ~VcpStackCallback() = default;

    virtual void OnVolumeChangeEvent(const RawAddress &device, uint8_t volume) {}
    virtual void OnMuteStatusChangeEvent(const RawAddress &device, uint8_t muteStatus) {}
    virtual void OnConnectionStateChanged(const RawAddress &device, uint8_t state, uint8_t preState) {}
    virtual void OnNotifyVolumeChange(const RawAddress &device, const std::list<StreamVolume> &streamVolumes) {}
};

class VcpStackAdapter {
public:
    explicit VcpStackAdapter(VcpStackCallback &callback);
    ~VcpStackAdapter();

    void RegisterStackCbk();
    void DeregisterStackCbk();
    void Connect(const RawAddress &device);
    void Disconnect(const RawAddress &device);
    void SetVolume(const RawAddress &device, int32_t volume);
    void SetStreamVolume(const RawAddress &device, const std::list<StreamVolume> &streamVolumes);
    bool GetStreamVolumeAbility(const RawAddress &device);

private:
    void OnVolumeChangeEventTask(const RawAddress &device, uint8_t volume);
    static void OnVolumeChangeEvent(SLE_Addr_S *addr, uint8_t volume);
    void OnMuteStatusChangeEventTask(const RawAddress &device, uint8_t muteStatus);
    static void OnMuteStatusChangeEvent(SLE_Addr_S *addr, uint8_t muteStatus);
    void OnNotifyVolumeChangeTask(const RawAddress &device, const std::list<StreamVolume> &streamVolumes);
    static void OnNotifyVolumeChange(SLE_Addr_S *addr, NLSTK_McpVolumePropertyType_E type, void *value);
    static void OnSetVolumeRsp(SLE_Addr_S *addr, uint8_t errorCode);
    void OnConnectStateChangeTask(const RawAddress &device, uint8_t state, uint8_t preState);
    static void OnConnectionStateChanged(SLE_Addr_S *addr, uint8_t state, uint8_t preState);

    NEARLINK_DECLARE_IMPL();
};

} // namespace Nearlink
} // namespace OHOS

#endif // VCP_STATEMACHINE_H
