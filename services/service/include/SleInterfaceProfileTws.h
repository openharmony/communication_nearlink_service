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

#ifndef SLE_INTERFACE_PROFILE_TWS_H
#define SLE_INTERFACE_PROFILE_TWS_H

#include "SleInterfaceProfile.h"
#include "sle_uuid.h"

namespace OHOS {
namespace Nearlink {
class TwsObserver {
public:
    virtual ~TwsObserver() = default;
    /**
     * ConnectionState Changed
     * @param  deviceAddress  sle address
     * @param  state          new state
     * @param  oldState       old state
     */
    virtual void OnConnectionStateChanged(const RawAddress &deviceAddress, int state, int oldState) {};
};

class InterfaceTwsClientObserver {
public:
    virtual ~InterfaceTwsClientObserver() = default;
    virtual void OnTwsRemoteInfo(const std::string &address, const std::vector<uint8_t> &value);
};

class TwsDeviceInfo {
public:
    static constexpr uint32_t DEVICE_MODEL_ID_LEN = 6;
    static constexpr uint32_t DEVICE_SUBMODEL_ID_LEN = 2;
    static constexpr uint32_t DEVICE_ICON_ID_LEN = 4;
    static constexpr uint32_t DEVICE_DEV_TYPE_LEN = 2;

    TwsDeviceInfo() = default;
    ~TwsDeviceInfo() = default;

public:
    std::string productId_ {};
    std::string vendorId_ = {};
    std::string version_ = {};
    std::string devType_ = {};
    std::string iconId_ = {};
    std::string newModelId_ = {};
    std::string modelId_ = {};
};

enum class TwsRoleType : uint8_t {
    ROLE_TYPE_PRIMARY     = 0x01,       /* 角色类型：主 */
    ROLE_TYPE_SECONDARY      = 0x02,       /* 角色类型：从 */
};

enum class TwsNatureType :uint8_t {
    TWS_LEFT     = 0x00,       /* 左耳 */
    TWS_RIGHT    = 0x01,       /* 右耳 */
};

/* 佩戴状态 */
enum class TwsWearDetect : uint8_t {
    NOT_WORN = 0x00,       /* 未佩戴 */
    WORN = 0x01,           /* 已佩戴 */
    WORN_INVALID = 0xff,   /* 未知佩戴状态 */
};

/* 是否支持佩戴检测 */
enum class TwsWearDetectSupport : int32_t {
    SUPPORT_UNKNOWN = -1,    /* 未知佩戴检测 */
    SUPPORT_OFF = 0,        /* 不支持佩戴检测 */
    SUPPORT_ON = 1,           /* 支持佩戴检测 */
};

struct TwsDevWearStatus {
    uint8_t leftStatus;       /* 左耳佩戴状态 @ref TwsWearDetect */
    uint8_t rightStatus;      /* 右耳佩戴状态 @ref TwsWearDetect */
};

class ProfileTws : public SleInterfaceProfile {
public:
    /**
     * @brief  register observer
     *
     * @param  observer         function pointer
     */
    virtual void RegisterObserver(TwsObserver &observer) = 0;
    /**
     * @brief  deregister observer
     *
     * @param  observer         function pointer
     */
    virtual void DeregisterObserver(TwsObserver &observer) = 0;

    /* 获取设备角色，取值: @ref ProfileTwsRoleType */
    virtual void TwsGetDeviceRole(const RawAddress &devAddr, uint8_t &devRole) = 0;

    /* 获取左右耳信息，取值: @ref ProfileTwsRoleType */
    virtual uint8_t TwsGetDeviceNature(const RawAddress &devAddr) = 0;

    /* 获取设备媒体流状态 */
    virtual uint8_t TwsGetDeviceAudioMusicType(const RawAddress &devAddr) = 0;

    /* 查询设备是否支持佩戴检测 */
    virtual bool TwsIsSupportWearDetect(const RawAddress &devAddr) = 0;

    /* 查询设备佩戴状态，取值: @ref ProfileTwsWearStatus */
    virtual void TwsGetDeviceWearStatus(const RawAddress &devAddr, TwsDevWearStatus &wearStatus) = 0;

    virtual void TwsEnableWearDetection(const RawAddress &devAddr) = 0;

    virtual void TwsDisableWearDetection(const RawAddress &devAddr) = 0;

    virtual bool TwsIsDeviceWearing(const RawAddress &devAddr) = 0;

    virtual int TwsGetWearDetectionState(const RawAddress &devAddr) = 0;

    virtual void GetTwsAudioDelay(const RawAddress &devAddr, uint32_t &delayValue) = 0;
    /* 更新默认角色 @ref TwsRoleType */
    virtual void TwsUpdateDeviceDefaultRole(const RawAddress &devAddr, const uint8_t roleType) = 0;

    /* 查询是否支持虚拟自动连接 */
    virtual bool TwsIsSupportVirtualAutoConnect(const RawAddress &devAddr) = 0;

    virtual void SetVirtualAutoConnectType(const RawAddress &devAddr, int32_t connType, int32_t businessType) = 0;

    virtual int RegisterApplication(const std::shared_ptr<InterfaceTwsClientObserver> &callback) = 0;
    virtual int DeregisterApplication() = 0;

    /* 双连接抢占 */
    virtual void TwsSendUserSelection(const RawAddress &device,
        const std::vector<struct AudioStreamInfo> &streamInfo) = 0;

    virtual void QueryStreamState(const RawAddress &devAddr, std::vector<struct AudioStreamInfo> &streamData) = 0;

    /* 通话挂断时间戳 */
    virtual void UpdateHangUpTimeStamp(RawAddress &devAddr) = 0;

    /* 处理暂停列表 */
    virtual void ProcPauseRecordMap() = 0;

    /* 通知服务连接完成 */
    virtual void SendProfileConnected(const RawAddress &devAddr) = 0;

    /* 设置支持Tws设备的能力位图 */
    virtual void SetDeviceManufacturerAbility(
        const RawAddress &device, const std::array<uint8_t, SLE_MANU_ABILITY_LEN> &manuAbility) const = 0;

};

} // namespace Sle
} // namespace OHOS

#endif /* END OF SLE_INTERFACE_PROFILE_TWS_H */
