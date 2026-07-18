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
#ifndef TWS_SERVICE_H
#define TWS_SERVICE_H

#include <map>
#include <list>
#include <mutex>
#include <vector>
#include <cmath>
#include <cstring>

#include "SleInterfaceAdapter.h"
#include "nearlink_def.h"
#include "nearlink_safe_map.h"
#include "BaseObserverList.h"
#include "ProfileServiceManager.h"
#include "context.h"
#include "log_util.h"
#include "nearlink_ASC_source.h"
#include "SleInterfaceProfileCdsm.h"

#include "TwsDefines.h"
#include "TwsMessage.h"
#include "TwsHiBoxParser.h"
#include "TwsSharedLibApi.h"
#include "TwsClient.h"
#include "DeviceBatteryManager.h"

namespace OHOS {
namespace Nearlink {

class TwsClientData {
public:
    explicit TwsClientData(RawAddress peerAddr) : peerAddr_(peerAddr) {}
    TwsClientData() {}
    ~TwsClientData() = default;

    void UpdateData(uint8_t dataType, TwsClientData &clientData);

public:
    RawAddress peerAddr_ = RawAddress(INVALID_MAC_ADDRESS); /* 对端地址 */
    int clientState = static_cast<int>(SleConnectState::DISCONNECTED); /* 服务连接状态 */
    uint8_t subCmd_ = 0;                                     /* 命令子类 */
    uint8_t roleType_ = 0;                                  /* 角色类型，1:Primary  2:Secondary */
    std::vector<uint8_t> devDtsCap_ = {};                   /* 对端设备的数通能力 */
    TwsWearStateData wearData_ = {};                        /* 佩戴状态 */
    TwsDeviceDatas devInfo_ = {};                           /* 设备信息（product ID） */
    uint8_t nature_ = 0;                                    /* 耳机左右角色 0x00:left ;0x01:right */
    SleLatencyConfig sleConfig_ = {};                       /* 音画同步TLV */
    BatteryInfo vendorBatt_ = {};                            /* 私有电量 */
    HiBoxNearlinkProfile audioState_ = {};                  /* 音频状态 */
    uint8_t captureRes_ = static_cast<uint8_t>(TwsCaptureResult::INVALID); /* 抢占响应 */
    int virtualAutoConnType_ = 0;                           /* 自动连接，连接类型 */
    int virtualAutoBussinessType_ = 0;                      /* 自动连接，业务类型 */
    uint8_t autoConnSwitch_ = static_cast<uint8_t>(TwsAutoConnSwitch::OFF);    /* 自动连接开关，0：不支持，1：支持 */
    int wearDetectionState_ = 0;                            /* 佩戴检测状态 */
    AudioExceptionData audioExcep_ = {};                    /* 耳机音频异常 */
    NotifyDisconnectProfile notifyValue_ = {};              /* 静默数传断链标识 */
    std::array<uint8_t, DEVICE_MANUFACTURER_ABILITY_LEN> manufacturerAbility_ = {0}; /* 能力位图 128bits */

private:
    void HandleRoleTypeUpdate(TwsClientData &clientData);
    void HandleDtsCapUpdate(TwsClientData &clientData);
    void HandleWearStatusUpdate(TwsClientData &clientData);
    void HandleAtCmdData(TwsClientData &clientData);
    void HandleAudioExcep(TwsClientData &clientData);
    void HandleQueryPreemption(TwsClientData &clientData);
    void HandleAutoConnectSwitch(TwsClientData &clientData);
    void HandleManufacturerAbility(TwsClientData &clientData);
};

class TwsService : public ProfileTws, public utility::Context {
public:
    /* Profile管理模块依赖的基本接口 */
    explicit TwsService();
    virtual ~TwsService();

    utility::Context *GetContext() override;
    void Enable() override;
    void Disable() override;

    int RegisterApplication(const std::shared_ptr<InterfaceTwsClientObserver> &callback) override;
    int DeregisterApplication() override;

    void RegisterObserver(TwsObserver &TwsObserver) override;
    void DeregisterObserver(TwsObserver &TwsObserver) override;

    int Connect(const RawAddress &device) override;
    int Disconnect(const RawAddress &device) override;

    int GetConnectState() override;
    std::list<RawAddress> GetConnectDevices() override;

    /* 对外接口 */
    void TwsGetDeviceRole(const RawAddress &devAddr, uint8_t &devRole) override;
    uint8_t TwsGetDeviceNature(const RawAddress &devAddr) override;
    uint8_t TwsGetDeviceAudioMusicType(const RawAddress &devAddr) override;
    void GetTwsAudioDelay(const RawAddress &devAddr, uint32_t &audioDelay) override;
    bool TwsIsSupportWearDetect(const RawAddress &devAddr) override;
    void TwsGetDeviceWearStatus(const RawAddress &devAddr, TwsDevWearStatus &wearStatus) override;
    void TwsEnableWearDetection(const RawAddress &devAddr) override;
    void TwsDisableWearDetection(const RawAddress &devAddr) override;
    bool TwsIsDeviceWearing(const RawAddress &devAddr) override;
    int TwsGetWearDetectionState(const RawAddress &devAddr) override;
    void TwsUpdateDeviceDefaultRole(const RawAddress &devAddr, const uint8_t roleType) override;
    void TwsSendUserSelection(const RawAddress &device,
        const std::vector<struct AudioStreamInfo> &streamInfo) override;
    bool TwsIsSupportVirtualAutoConnect(const RawAddress &devAddr) override;
    void SetVirtualAutoConnectType(const RawAddress &devAddr, int32_t connType, int32_t businessType) override;
    void QueryStreamState(const RawAddress &devAddr, std::vector<struct AudioStreamInfo> &streamData) override;
    void ProcPauseRecordMap() override;
    void UpdateHangUpTimeStamp(RawAddress &devAddr) override;
    void SendProfileConnected(const RawAddress &devAddr) override;
    void SetDeviceManufacturerAbility(const RawAddress &device,
        const std::array<uint8_t, SLE_MANU_ABILITY_LEN> &manuAbility) const override;

    /* 内部模块交互 */
    static TwsService *GetService();

    void NotifyStateChanged(const RawAddress &device, TwsClientState preState, TwsClientState toState);
    void PostEvent(const TwsMessage &event);
    void NotifyAudioDeviceAction(const RawAddress &devAddr, int action);
    void UpdateDeviceWearState(const RawAddress &devAddr, const TwsWearStateData &wearData);

    void UpdateClientData(uint8_t dataType, TwsClientData &clientData);

    RawAddress GetReportAddr(const RawAddress &device);

    NearlinkSafeMap<std::string, TwsDevWearStatus> twsDevWearStatus_ {};

    void ResetVirtualAutoSwitch(const RawAddress &devAddr);
    void RecoverDataFromConf(const RawAddress &devAddr);
    RawAddress GetPrimaryAddr(const RawAddress &device);
    void TwsServiceSendReqInner(const TwsMessage &event);
    void ProcessEvent(const TwsMessage &event);
private:
    const std::map<const TwsClientState, const int> stateMap_ = {
        { TwsClientState::TWS_STATE_DISCONNECTED, static_cast<int>(SleConnectState::DISCONNECTED) },
        { TwsClientState::TWS_STATE_CONNECTING, static_cast<int>(SleConnectState::CONNECTING) },
        { TwsClientState::TWS_STATE_DISCONNECTING, static_cast<int>(SleConnectState::DISCONNECTING) },
        { TwsClientState::TWS_STATE_CONNECTED, static_cast<int>(SleConnectState::CONNECTED) }
    };

    BaseObserverList<TwsObserver> twsObservers_ {};
    NearlinkSafeMap<std::string, std::shared_ptr<TwsClient>> twsClient_ {};
    NearlinkSafeMap<std::string, std::shared_ptr<TwsClientData>> twsClientDatas_ {};
    NearlinkSafeMap<std::string, TwsPauseRecord> pauseRecordMap_ {};
    NearlinkSafeMap<std::string, int64_t> hangUpRecord_ {};
    /* 缓存消息，应用场景：客户端未连接时上层发送消息，此时缓存消息，待连接完成后发送消息 */
    NearlinkSafeMap<std::string, TwsCacheMessage> msgCache_ {};
    std::shared_ptr<TwsClientData> CreateNewClientData(RawAddress peerAddr);
    std::shared_ptr<InterfaceTwsClientObserver> callback_ = nullptr;
    std::mutex callbackMutex_;
    std::atomic_bool isStarted_ = ATOMIC_FLAG_INIT;
    std::atomic_bool isShuttingDown_ = ATOMIC_FLAG_INIT;
    int64_t lastSendPlayTime_ = -1L;
    std::array<uint8_t, SLE_MANU_ABILITY_LEN> manuAbility_; /* 本端设备能力位图 */

    void StartUp();
    void ShutDown();

    /* 私有服务消息切换处理接口 */
    void ProcessConnectEvent(const TwsMessage &event);
    void ProcessDisconnectEvent(const TwsMessage &event);
    void TwsServiceStartDecodeRecvData(const TwsMessage &event);
    void TwsServiceSendDataBySsap(const TwsMessage &event); /* 通过SSAP发送数据 */

    /* 佩戴检测处理 */
    void NotifyWearAction(const RawAddress &devAddr, const TwsWearStateData &wearData,
        const TwsDevWearStatus &previousWearStatus, bool &isActiveDevWill);
    bool SaveWearDetectionTask(const RawAddress &device, int wearDetectionState);
    void ChangePlayState(const RawAddress &devAddr, const TwsWearStateData &wearData,
        const TwsDevWearStatus &previousWearStatus, bool &isActiveDevWill, bool &isActiveDevNow);
    void SetCurrentAVPlaybackState(int &currentPlayState);
    void ResumePlayIfNeeded(const RawAddress &devAddr, int pauseReason, int64_t currentTime);
    void PauseAndRecordIfNeeded(const RawAddress &devAddr, int pauseReason, int64_t currentTime);
    int GetPauseReasonNotPlaying(const RawAddress &devAddr, int action, const TwsDevWearStatus &previousWear);
    int GetPauseReason(const RawAddress &devAddr, int action, const TwsDevWearStatus &previousWear,
        int currentPlayState);
    int64_t GetTimeStamp();
    bool IsActiveDeviceNow(const RawAddress &devAddr);
    int GetWearActionRecord(bool leftIn, bool rightIn, bool wasLeftIn, bool wasRightIn);
    int GetWearAction(uint8_t leftWearState, uint8_t rightWearState, const TwsDevWearStatus &previousWear);
    bool IsMusicActive();
    bool IsCalling() const;

    /* 编码、解码处理入口 */
    void TwsServiceProcessSendReq(const TwsMessage &event);
    void TwsServiceProcessSendRsp(const TwsMessage &event);
    void TwsServiceProcessRecvMessage(const TwsMessage &event);
    void TwsServiceProcessSendRemoteInfo(const TwsMessage &event);

    void UpdateClientDataOrCreate(uint8_t dataType, TwsClientData &clientData);
    bool GetVendorAccountHash(uint8_t *outHashArray, size_t arrayLen);
    void TwsSendAccountInfo(const RawAddress &devAddr);

    uint8_t GetScenesFromQoS(uint8_t qosId);
    void SetVirtualAutoConnectTypeInner(const RawAddress &devAddr, int32_t connType, int32_t businessType);
    void SendVirtualAutoConnectType(const RawAddress &devAddr);

    void UpdateClientState(const RawAddress &device, int newState);
    void AddToCacheMessage(const RawAddress &devAddr, TwsHiBoxMsgType msgType);
    void SendSingleCacheMessage(const std::string &devAddr, TwsHiBoxMsgType msgType);
    void SendAllCacheMessage(const RawAddress &devAddr);
    void PauseRecordProcessWhenDisconnect(const RawAddress &device);
    void ResetStreamState(std::vector<NearlinkCdsmInfo> &cdsmInfo);
    void DisconnectProcess(const RawAddress &device);

    void OnProfileStateChange(TwsClientData &clientData); /* 音频流状态变化上报 */
    bool GetProfileStatus(const std::string addr, uint8_t &mediaState, uint8_t &callState);
    bool IsNeedShowCapsule(TwsClientData &clientData);
    void OnClientDataChanged(uint8_t dataType, TwsClientData &clientData);
    void SendCapsuleInfo(RawAddress &devAddr, uint8_t mediaState, std::string &devName);
    void ShowCapsule(TwsClientData &clientData); /* 胶囊显示控制 */
    int64_t GetHangUpTimeStamp(RawAddress &reportAddr);
    void UpdateDualRecordAbility();
    void UpdateDualKaraokeAbility();
    void UpdateVoiceCallFrameFourAndAutoRateAbility();
    uint8_t GetMediaState(TwsClientData &clientData);
};

} // namespace Sle
} // namespace OHOS

#endif /* END of TWS_SERVICE_H */
