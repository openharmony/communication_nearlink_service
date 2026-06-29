/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#ifndef VCP_CLIENT_SERVICE_H
#define VCP_CLIENT_SERVICE_H

#include "nearlink_types.h"
#include "SleInterfaceProfileVcp.h"
#include "VcpStackAdapter.h"
#include "VcpDefines.h"
#include "BaseObserverList.h"
#include "nearlink_safe_map.h"

namespace OHOS {
namespace Nearlink {

enum VolumeAdditionalInfo : uint8_t {
    SLE_INIT_VOLUME,
    SLE_SERVER_CHANGE_VOLUME,
    SLE_CLIENT_CHANGE_VOLUME,
};

class VcpService : public ProfileVcp, public utility::Context {
public:
    static VcpService *GetService();
    explicit VcpService();
    ~VcpService() override;
    utility::Context *GetContext() override;

    void Enable() override;
    void Disable() override;

    int Connect(const RawAddress &device) override;
    int Disconnect(const RawAddress &device) override;
    int GetConnectState() override;
    std::list<RawAddress> GetConnectDevices() override;

    void RegisterObserver(VcpClientObserver &serviceObserver) override;
    void DeregisterObserver(VcpClientObserver &serviceObserver) override;

    int GetDeviceMediaVolume(const RawAddress &device) override;
    int GetDeviceCallVolume(const RawAddress &device) override;

    /* 音频框架触发 */
    void SetDeviceAbsoluteVolume(const RawAddress &device, int32_t volumeLevel, uint8_t streamType) override;
    /* 提供给ASC服务调用触发 */
    void SwitchAbsVolumeDevice(const RawAddress &device, bool isNeedSetVolume = true);
    /* 音频流音量全量设置接口 */
    void SetAllStreamVolume(const RawAddress &device);

private:
    void EnableTask();
    void DisableTask();
    void ConnectTask(const RawAddress &device);
    void DisconnectTask(const RawAddress &device);
    void SetDeviceAbsoluteVolumeTask(const RawAddress &device, int32_t volume, uint8_t streamType);
    void DftVolumeControlInSource(const RawAddress &device, int32_t volume, int32_t storeVolume);
    void SwitchAbsVolumeDeviceTask(const RawAddress &device, bool isNeedSetVolume);

    /* 协议栈回调的处理 */
    bool IsActiveDevice(const RawAddress &device, RawAddress &reportAddr);
    void ProcessVolumeChangeEvent(const RawAddress &addr, uint8_t volume);
    void ProcessMuteStatusChangeEvent(const RawAddress &addr, uint8_t muteStatus);
    void NotifyStateChanged(const RawAddress &device, int state, int preState);
    void NotifyVolumeChanged(const RawAddress &device, const std::list<StreamVolume> &streamVolumes);

    void SaveDeviceVolume(const RawAddress &device, int32_t volume, bool isInCall);
    int GetDeviceVolume(const RawAddress &device);
    RawAddress GetAscActiveDevice();
    bool IsCalling() const;
    bool SetCdsmAbsoluteVolume(const RawAddress &device, int32_t vcpVolume, uint8_t streamType,
        bool isNeedReport = true);
    bool SaveDeviceVolumeTask(const RawAddress &device, int32_t volume, bool isInCall);
    void SetVolumeAdapter(const RawAddress &device, int32_t vcpVolume, uint8_t streamType, bool isNeedReport = true);

    inline int32_t SystemToVcpVolume(int32_t volume)
    {
        if (musicMaxVolumeLevel_ == 0) {
            return VCP_MAX_VOL;
        }
        return std::floor(static_cast<double>(volume) * VCP_MAX_VOL / musicMaxVolumeLevel_);
    }

    inline int32_t VcpToSystemVolume(uint8_t volume) const
    {
        return std::round(static_cast<double>(volume) * musicMaxVolumeLevel_ / VCP_MAX_VOL);
    }

    inline int32_t GetDefaultVolume() const
    {
        if (musicMaxVolumeLevel_ < VCP_DEFAULT_VOLUME_LEVEL) {
            return VCP_DEFAULT_VOLUME_LEVEL;
        }
        return musicMaxVolumeLevel_ / VCP_DEFAULT_VOLUME_DIVISOR;
    }

private:
    /* 保存外部注册的连接状态观察者 */
    BaseObserverList<VcpClientObserver> vcpClientObservers_ {};

    int32_t musicMaxVolumeLevel_ = 0;

    // service status
    std::atomic_bool isStarted_ = ATOMIC_FLAG_INIT;
    // service status
    std::atomic_bool isShuttingDown_ = ATOMIC_FLAG_INIT;
    // stream volume supported flag
    NearlinkSafeMap<std::string, bool> streamVolumeAbility_;

    NEARLINK_DECLARE_IMPL();
};
}
}

#endif // VCP_CLIENT_SERVICE_H
