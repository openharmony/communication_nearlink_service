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

#ifndef ASC_SERVICE_H
#define ASC_SERVICE_H

#include <list>
#include <queue>
#include <mutex>
#include <vector>
#include <cmath>
#include <cstring>
#include "SleInterfaceProfileASC.h"
#include "SleInterfaceAdapter.h"
#include "nearlink_def.h"
#include "nearlink_safe_map.h"
#include "BaseObserverList.h"
#include "ProfileServiceManager.h"
#include "context.h"
#include "actm_api_type.h"
#include "actm_api.h"
#include "actm_l2hc.h"
#include "QosM.h"
#include "ASCMessage.h"
#include "nearlink_safe_map.h"
#include "nearlink_safe_set.h"
#include "nearlink_safe_list.h"
#include "nearlink_timer.h"
#include "audio_volume_client_manager.h"
#include "audio_devices_client_manager.h"
#include "audio_system_client_engine_manager.h"
#include "audio_stream_client_manager.h"
#include "audio_system_client_policy_manager.h"
#include "audio_routing_client_manager.h"
#include "audio_spatialization_manager.h"
#include "audio_combine_denoising_manager.h"
#include "cm_def.h"
#include "SleInterfaceProfileMic.h"
#include "TwsService.h"

namespace OHOS {
namespace Nearlink {
class AudioSceneChangedListener : public AudioStandard::AudioManagerAudioSceneChangedCallback {
public:
    void OnAudioSceneChange(const AudioStandard::AudioScene audioScene) override;
};

class ASCService : public ProfileASC, public utility::Context {
public:
    static ASCService *GetService();
    explicit ASCService();
    virtual ~ASCService();
    utility::Context *GetContext() override;
    void Init();

    int AudioControl(const RawAddress &device, AudioStreamType streamType, int cmd) override;
    void StopSink() override;
    int GetAudioDeviceList(std::vector<NearlinkRawAddress>& devices) override;
    int GetVirtualAudioDeviceList(std::vector<NearlinkRawAddress>& devices) override;
    int GetSupportStreamType(const NearlinkRawAddress &device, uint32_t& supportStreamType) override;
    int GetAudioDeviceCodecInfo(const NearlinkRawAddress &device, std::map<AudioStreamType,
        AudioStreamCodecInfo> &info) override;
    int SetActiveSinkDevice(const NearlinkRawAddress &device, uint32_t supportStreamType) override;
    int UpdateDeviceRole(const RawAddress &device, uint8_t devRole) override;

    int RegisterApplication(const std::shared_ptr<InterfaceASCCallback> &callback) override;
    int DeregisterApplication() override;
    int DeregisterApplication(const std::shared_ptr<InterfaceASCCallback> &callback) override;
    void RegisterAudioPreferredOutPutDeviceChangeListener();
    void UnregisterAudioPreferredOutPutDeviceChangeListener();
    void RegisterObserver(ASCObserver &disObserver) override;
    void DeregisterObserver(ASCObserver &disObserver) override;

    void Enable() override;
    void Disable() override;
    int Connect(const RawAddress &device) override;
    int Disconnect(const RawAddress &device) override;
    std::list<RawAddress> GetConnectDevices() override;
    int GetConnectState() override;
    bool UpdateSleVirtualDevice(int32_t cmd, const RawAddress &device) override;
    int32_t SetMusicMuteWhenAudioRelease();
    int32_t SetMusicUnmute();
    bool IsAudioOutputToNlAudio();
    void SetCurrentDeviceMute();
    void RegisterListener();
    /*
     * 回调函数
     */
    void CbkGetProperty(const RawAddress& device, const std::vector<AscProp>& properties);
    void CbkReadPropFail(const RawAddress& device, uint8_t result);
    void CbkCreateStream(const RawAddress& device, uint8_t result, AscStreamInfo streamInfo);
    void CbkConfigStream(const RawAddress& device, uint8_t result);
    void CbkBitrateChanged(const RawAddress &device, uint8_t result);
    void CbkOpenStream(const RawAddress& device, uint8_t result, const AscQosmInfo& qosmInfo);
    void CbkStartStream(const RawAddress& device, uint8_t result, const AscQosmInfo& qosmInfo);
    void CbkStopStream(const RawAddress& device, uint8_t result, uint16_t connHandle);
    void CbkReleaseStream(const RawAddress& device, uint8_t result, uint16_t connHandle);
    void CbkDisconnect(const RawAddress& device, uint8_t result);
    void ClearWhenDisconnect(const RawAddress& device);

    bool CheckStartStreamCondition(const RawAddress& device, uint8_t result, AudioStreamType streamType);
    bool IsStreamExists(const RawAddress& device, AudioStreamType streamType);
    const NearlinkRawAddress GetActiveSinkDevice() const override;
    void SleAudioDeviceActionChanged(const NearlinkRawAddress &device, int action) override;
    void SleAudioDeviceActionChanged(const NearlinkRawAddress &device,
        const std::vector<struct AudioStreamInfo> &streamData, int action) override;
    bool IsCalling() override;
    bool IsPlaying(const RawAddress &device) override;
    bool GetAscQosmInfo(const RawAddress& device, AscQosmInfo& qosmInfo) override;
    void SendPlayOrPauseByWearDetection(const RawAddress &devAddr, uint8_t playOrPauseKey) override;
    void AcbSubrateChanged(const RawAddress &device, uint32_t subrate) override;
    void PhyChanged(RawAddress device, uint8_t frameType, uint8_t phyType, uint8_t status) override;
    void AcbSubrateChangeReq(const RawAddress &device, const SleAcbSubrateParam &subrateParam) override;
    bool GetDualRecordAbility(const RawAddress &device) override;
    void ProcessBaseEvent(const ASCMessage &event);
    void ProcessAssistEvent(const ASCMessage &event);
    void ProcessSpatialAudioEvent(const ASCMessage &event);
    void ProcessEvent(const ASCMessage &event);
    void PostEvent(const ASCMessage &event);
    void UpdateDeviceNature(const RawAddress &device);
    bool GetLocalDualRecordAbility();
    bool GetLocalKaraokeAbility();
    bool GetKaraokeAbility(const RawAddress &device) override;
    bool IsEqualCodec(const AscCodecIdKey &codecA, const AscCodecIdKey &codecB);
    void ProcessCodecFCVersion(AscCodecIdKey &codec);
    bool GetLocalVocieCallFrameFourAbility();
    bool GetLongRangeVoiceCallAbility(const RawAddress &device);
    uint8_t SelectCodecSampleRatePolicyInDualRec(AscCodecIdKey codec, uint16_t peerSampleRate);
    uint8_t SelectCodecBitDepthPolicyInDualRec(AscCodecIdKey codec, uint8_t inDepth);
    uint8_t SelectCodecChannelPolicyInDualRec(AscCodecIdKey codec, uint16_t peerChannel);
    void SetSleVoiceStatusFlag(const RawAddress& device);
    bool IsSleVoiceExisted(const RawAddress& device);
    bool IsTwsVoiceExistedWithCos(const RawAddress& device);

    class SleSpatialAudioModeChangeListener : public AudioStandard::AudioSpatializationEnabledChangeCallback {
    public:
        explicit SleSpatialAudioModeChangeListener() {};
        ~SleSpatialAudioModeChangeListener() {};

    private:
        void OnSpatializationEnabledChange(const bool &enabled) override {};
        void OnSpatializationEnabledChangeForAnyDevice(const std::shared_ptr<AudioStandard::AudioDeviceDescriptor>
            &deviceDescriptor, const bool &enabled) override;
    };

    class SleSpatialAudioHeadTrackingChangeListener :
        public AudioStandard::AudioHeadTrackingEnabledChangeCallback {
    public:
        explicit SleSpatialAudioHeadTrackingChangeListener() {};
        ~SleSpatialAudioHeadTrackingChangeListener() {};

    private:
        void OnHeadTrackingEnabledChange(const bool &enabled) override {};
        void OnHeadTrackingEnabledChangeForAnyDevice(const std::shared_ptr<AudioStandard::AudioDeviceDescriptor>
            &deviceDescriptor, const bool &enabled) override;
    };

    class SleSpatialAudioSourceTypeChangeListener :
        public AudioStandard::AudioSpatialAudioSourceTypeChangeCallback {
    public:
        explicit SleSpatialAudioSourceTypeChangeListener() {};
        ~SleSpatialAudioSourceTypeChangeListener() {};

    private:
        void OnSpatialAudioSourceTypeChange(const AudioStandard::SpatialAudioSourceType &mode) override;
    };

    class SleSpatialAudioAdaptiveSwitchChangeListener :
        public AudioStandard::AudioAdaptiveSpatialRenderingEnabledChangeCallback {
    public:
        explicit SleSpatialAudioAdaptiveSwitchChangeListener() {};
        ~SleSpatialAudioAdaptiveSwitchChangeListener() {};
    private:
        void OnAdaptiveSpatialRenderingEnabledChangeForAnyDevice(
            const std::shared_ptr<AudioStandard::AudioDeviceDescriptor> &deviceDescriptor,
            const bool &enabled) override;
    };

    class SleAudioPreferredOutPutDeviceChangeListener :
        public AudioStandard::AudioPreferredOutputDeviceChangeCallback {
    public:
        explicit SleAudioPreferredOutPutDeviceChangeListener() {};
        ~SleAudioPreferredOutPutDeviceChangeListener() {};
        AudioStandard::DeviceType outPutDeviceType_ = AudioStandard::DeviceType::DEVICE_TYPE_DEFAULT;
    private:
        void OnPreferredOutputDeviceUpdated(const std::vector<std::shared_ptr<AudioStandard::AudioDeviceDescriptor>>
            &deviceDescriptor) override;
    };

    class AscMicStateObserver : public MicStateObserver {
    public:
        explicit AscMicStateObserver() {}
        ~AscMicStateObserver() override {}

    private:
        void OnMicStateChanged(const RawAddress &device, uint8_t micState) override;
    };

private:
    // StartPlaying/StopPlaying结果统计
    typedef struct {
        bool    isStartFinished;
        bool    isStopFinished;
        uint8_t startPlayingResult;
        uint8_t stopPlayingResult;
    } StreamResult;

    // 用于记录上一次的播放信息，供本次决定是否低码率起播
    typedef struct {
        RawAddress      addr;              // report地址
        Qos             qos;               // QOS
        uint8_t         codecId;           // 编解码器ID
        uint8_t         autoRateBpsBit;    // 码率
        time_t          timeStamp;         // 停播时间
    } ASCPlayRecord;

    void StartUp();
    void ShutDown();
    void ProcessConnectEvent(const ASCMessage &event);
    void ProcessDisConnectEvent(const ASCMessage &event);
    void ProcessStartPlayingEvent(const ASCMessage &event);
    void StartPlayingExcute(const RawAddress& device, AudioStreamType streamType, bool isStopDelaying,
        AudioStreamType stopDelayingStreamType);
    void SyncWhenStartPlaying(bool isSync, const RawAddress* startedDevice, const RawAddress* idleDevice);
    void ProcessStopPlayingEvent(const ASCMessage &event);
    void ProcessStackCbkEvent(const ASCMessage &event);
    void ProcessStackCbkProp(const ASCMessage &event);
    bool IsConnectedMemberExist(const RawAddress &device);
    bool IsStartMemberExist(const RawAddress &device);
    void ProcessSetActiveSinkDeviceEvent(const ASCMessage &event);
    void ProcessUpdateVirtualDeviceEvent(const ASCMessage &event);
    void ProcessUpdateDeviceRoleEvent(const ASCMessage &event);
    void ProcessChangeBitrateEvent(const ASCMessage &event);
    void ProcessVoiceCallAutoRateEvent(const ASCMessage &event);
    void ProcessFrameTypeChangedEvent(const ASCMessage &event);
    void ProcessAutoRateEvent(const ASCMessage &event);
    AudioStreamType GetStopStreamType(const RawAddress &device);
    void ExcuteDelayStop(const RawAddress &device);
    void ProcessStopDelayTimeOutEvent(const ASCMessage &event);
    void ProcessConnRptDelayTimeOutEvent(const ASCMessage &event);
    void ProcessSendPlayOrPauseEvent(const ASCMessage &event);
    void ProcessAudioSceneChangeEvent(const ASCMessage &event);
    void ProcessStopSinkEvent(const ASCMessage &event);
    void ProcessSubrateChangedEvent(const ASCMessage &event);
    void ProcessSubrateChangeReq(const ASCMessage &event);
    void ProcessStackLocationChangeCbk(const ASCMessage &event);
    void ProcessStackStreamTypeChangeCbk(const ASCMessage &event);
    void SetSubrate(const RawAddress &device, const SleAcbSubrateParam &subrateParam);
    void ProcessUpdateDeviceNature(const ASCMessage &event);
    void ProcessSpatialAudioSourceTypeEvent(const ASCMessage &event);
    void ProcessSpatialAudioAdaptiveSwitchRenderEvent(const ASCMessage &event);

    /*
     * 内部状态/标记位管理函数
     */
    ASCState GetASCStatus(const RawAddress &device) const;
    void SetASCStatus(const RawAddress &device, ASCState state);
    void SaveStreamInfo(const RawAddress &device, AscStreamInfo streamInfo);
    AscStreamInfo GetStreamInfo(const RawAddress &device, uint8_t pointType) const;
    bool IsStreamIdValid(const RawAddress &device, uint8_t pointType);
    void ASCToDftCacheTime(const RawAddress &device, ASCState state);
    bool IsNeedDisconnect(const RawAddress &device) const;
    void SetNeedDisconnect(const RawAddress &device, bool isNeedDisconnect);
    int DisconnProcAction(const RawAddress &device, ASCState state);
    AudioStreamType GetProcessingStreamType(const RawAddress &device) const;
    void SetProcessingStreamType(const RawAddress &device, AudioStreamType streamType);
    bool IsStopDelaying(const RawAddress &device) const;
    void SetStopDelayingFlag(const RawAddress &device, bool isStopDelaying);
    bool IsSpatialConfiguring(const RawAddress &device) const;
    void SetSpatialConfiguringFlag(const RawAddress &device, bool isSpatialConfiguring);
    bool IsStopDoing(const RawAddress &device) const;
    void SetStopDoingFlag(const RawAddress &device, bool isStopDoing);
    bool GetAutoRateBps(const RawAddress &device, uint16_t& autoRateBps) const;
    void SetAutoRateBps(const RawAddress &device, uint16_t autoRateBps);
    void SetPlayRecord(const RawAddress &device, Qos qos, uint8_t codecId, uint8_t autoRateBpsBit);
    bool IsSync(const RawAddress &device) const;
    void SetSyncFlag(const RawAddress &device, bool isSync);
    uint64_t GetBpsRange(const RawAddress &device) const;
    void SetBpsRange(const RawAddress &device, uint64_t bpsRange);
    bool IsCoSetDeviceExist(const RawAddress& device, RawAddress& coSetDevice);
    bool IsCoSetDeviceStarted(const RawAddress& device, RawAddress& coSetDevice);
    int GetConnectedCnt(const RawAddress& device);
    void GetAllReportAddr(std::map<std::string, RawAddress>& reportAddrMap);
    void GetAllVirtualAudioAddr(std::set<std::string>& virtualAddrSet);
    int GetCoSetNum();
    bool IsLeftEarDevice(const RawAddress &device);
    bool IsConnected(const RawAddress &device);
    void SetDeviceRole(const RawAddress& device);
    bool IsRolePrimary(const RawAddress& device);
    bool IsNeedReportAudioDevice(const RawAddress& device, int cmd);
    bool IsNeedReportAudioControlComplete(const RawAddress& device, int cmd, AudioStreamType streamType, int result);
    bool IsNeedReportAudioControlFailed(int cmd, StreamResult& resultCoSet, const RawAddress& coSetDevice,
        ASCState coSetState);
    void OnAddDeleteAudioDevice(const RawAddress& device, int cmd);
    void OnConnectionStateChanged(const RawAddress& device, int cmd, int result, int reason);
    RawAddress GetReportAddr(const RawAddress& device);
    AudioStreamType GetReconfigStream(const RawAddress &device) const;
    void SetReconfigStream(const RawAddress &device, AudioStreamType streamType);
    void SetIsCallingFlag(bool isCalling) override;
    bool GetIsCallingFlag();
    void SendEventByWearDetection(const RawAddress &device, uint8_t playOrPauseKey);
    void SendPlayOrPauseIfNeed(const RawAddress &device);
    void CheckAndSetStopDelayingFlag(const RawAddress& device);
    void ProcWhenIOBCreated(const RawAddress& device, const AscQosmInfo& qosmInfo);
    void CbkReconfigStopping(const RawAddress& device, uint8_t result, AudioStreamType streamType);
    void NotifyConnectAudioDevice(bool isConnect);

    /*
     * 音频流打开/音频流关闭缓存队列/已打开列表管理函数
     */
    std::queue<AudioStreamType>& GetStartBuff(const RawAddress& device);
    std::queue<AudioStreamType>& GetStopBuff(const RawAddress& device);
    const std::queue<AudioStreamType>* FindStartBuff(const RawAddress& device);
    const std::queue<AudioStreamType>* FindStopBuff(const RawAddress& device);
    void ClearStartBuff(const RawAddress& device);
    void ClearStopBuff(const RawAddress& device);
    std::list<AudioStreamType>& GetStartedStreamList(const RawAddress& device);
    void RemoveStartedStream(const RawAddress &device, AudioStreamType streamType);
    void ProcSpatialIfNeed(const RawAddress& device, AudioStreamType streamType);
    void ProcAllMemberStartBuff(const RawAddress& device);
    void ProcBuff(const RawAddress& device, ASCState state);
    void ProcStartBuff(const RawAddress& device, ASCState state, bool& isGoOn);
    void ProcStopBuff(const RawAddress& device, ASCState state);
    bool EnableStartPlayingTimer(const RawAddress& device);
    bool DisableStartPlayingTimer(const RawAddress& device);
    void StartPlayingTimeout(const RawAddress& device);
    bool EnableStopPlayingTimer(const RawAddress& device);
    bool DisableStopPlayingTimer(const RawAddress& device);
    void StopPlayingTimeout();
    void DisconnectAcb(const RawAddress& device);
    bool EnableStopDelayTimer(const RawAddress& device);
    bool DisableStopDelayTimer(const RawAddress& device);
    void StopDelayTimeout();
    bool StartConnRptDelayTimer(const RawAddress& device);
    bool StopConnRptDelayTimer(const RawAddress& device);
    void ConnRptDelayTimeout();
    void CancelStopDelay(const RawAddress &device);
    bool IsNeedReconfigSpatialAudio(const RawAddress& device, AudioStreamType streamType, QosM& qosM);

    /*
     * 连接设备管理函数
     */
    void AddConnectDevices(const RawAddress &device);
    void DeleteConnectDevices(const RawAddress &device);
    void DeleteEarliestDevice();

    /*
     * 状态机控制函数
     */
    bool IsStartingOrStopingMemberExist(const RawAddress &device);
    int StartPlaying(const RawAddress &device, AudioStreamType streamType, bool isSync);
    int StopPlaying(const RawAddress &device, AudioStreamType streamType);
    int GetAudioProperty(const RawAddress& device);
    int FillActmImgEncpParam(const RawAddress &device, Qos cos, NLSTK_ActmConfigParam_S &cfgPara);
    int GetCdsmActmImgEncpParam(const RawAddress &device, std::string &encryptGroupKeyStr, uint64_t &giv,
        uint8_t &cryptoAlgo);
    int ConfigStreamToActm(const RawAddress &device, AudioStreamType streamType);
    int ConfigStreamCustomize(const RawAddress &device, NLSTK_ActmConfigParam_S &cfgPara);
    int ConfigStream(const RawAddress &device, AudioStreamType streamType, ASCState state);
    void CreateCommLink(const RawAddress &device, AudioStreamType streamType);
    int DeleteCommLink(const RawAddress &device, AudioStreamType streamType, Qos qos, ASCState targetState);
    int CreateStream(const RawAddress &device);
    int OpenStream(const RawAddress &device, AudioStreamType streamType);
    int StartStream(const RawAddress &device, AudioStreamType streamType);
    int StopStream(const RawAddress &device, AudioStreamType streamType, Qos qos, ASCState targetState);
    int ReleaseStream(const RawAddress &device, AudioStreamType streamType, Qos qos, ASCState targetState);
    Qos GetStreamQos(const RawAddress &device, AudioStreamType streamType);
    bool AddSpatialQos(const RawAddress &device);
    int JudgeReConfig(const RawAddress &device, AudioStreamType streamType, ASCState state, bool& isGoOn);
    void ClearStreamResult(const RawAddress& device);
    bool GetStreamResult(const RawAddress &device, AudioStreamType streamType, StreamResult& streamResult);
    void CheckAndAddPlayingResultInfo(const RawAddress &device, AudioStreamType streamType);
    void SetStartPlayingResult(const RawAddress &device, AudioStreamType streamType, bool isFinished, int result);
    void SetStopPlayingResult(const RawAddress &device, AudioStreamType streamType, bool isFinished, int result);
    bool IsStartPlayingSucc(const StreamResult& streamResult);
    bool IsStartPlayingFail(const StreamResult& streamResult);
    bool IsStartPlayingDoing(const StreamResult& streamResult);
    bool IsStopPlayingSucc(const StreamResult& streamResult);
    bool IsStopPlayingFail(const StreamResult& streamResult);
    bool IsStopPlayingDoing(const StreamResult& streamResult);
    void SwitchAbsVolumeDevice(const RawAddress &device, bool isNeedSetVolume);
    void OpenVoiceAssistant(const RawAddress &device, AudioStreamType streamType);
    void CloseVoiceAssistant(const RawAddress &device, AudioStreamType streamType);
    void StopPlayingExcute(const RawAddress& device, AudioStreamType streamType);
    void JudgeQosWhenStopPlaying(const RawAddress& device, AudioStreamType streamType, bool& isDelayStop,
        bool& isStopStream, bool& isNeedReconfig);

    void DelayStop(const RawAddress& device, AudioStreamType streamType);
    void StopPlayingWhenOtherStreamExist(const RawAddress& device, AudioStreamType streamType,
        bool isNeedReconfig, Qos cos);
    void SyncWhenConnect(const RawAddress& device);
    void SyncWhenDisconnect(const RawAddress& device, const ASCState state);
    void SyncWhenStartStream(const RawAddress& device, AudioStreamType streamType);
    void ProcessMcpInit(const RawAddress& device);
    void ProcessCcpInit(const RawAddress& device);

    void ReleaseStartedStream(const RawAddress& device, AudioStreamType streamType,
        AudioStreamType streamTypeToReconfig, Qos cos);
    void ReconfigStream(const RawAddress& device, AudioStreamType streamType, AudioStreamType streamTypeToReconfig,
        Qos cos);
    int ReconfigStreamInStartedState(const RawAddress &device, AudioStreamType streamType, Qos cos);

    /*
     * 状态判断函数
     */
    bool IsInStartProcess(ASCState state) const;
    inline bool IsConnecting(ASCState state) const
    {
        return (state == NL_SLE_ASC_CONNECTING);
    }

    inline bool IsConnected(ASCState state) const
    {
        return (state == NL_SLE_ASC_CONNECTED);
    }

    bool IsInConnectedState(ASCState state) const;

    inline bool IsDisconnected(ASCState state) const
    {
        return (state == NL_SLE_ASC_DISCONNECTED);
    }

    inline bool IsEnabled(ASCState state) const
    {
        return (state == NL_SLE_ASC_ENABLED);
    }

    inline bool IsCreating(ASCState state) const
    {
        return (state == NL_SLE_ASC_CREATING);
    }

    inline bool IsCreated(ASCState state) const
    {
        return (state == NL_SLE_ASC_CREATED);
    }

    inline bool IsConfiguring(ASCState state) const
    {
        return ((state == NL_SLE_ASC_RECONFIGURING) || (state == NL_SLE_ASC_CONFIGURING));
    }

    inline bool IsSubrateChanged(ASCState state) const
    {
        return ((state == NL_SLE_ASC_CONFIG_SUBRATE_CHANGED) || (state == NL_SLE_ASC_RECONFIG_SUBRATE_CHANGED));
    }

    inline bool IsConfiged(ASCState state) const
    {
        return ((state == NL_SLE_ASC_CONFIGED) || (state == NL_SLE_ASC_RECONFIGED));
    }

    inline bool IsOpening(ASCState state) const
    {
        return (state == NL_SLE_ASC_OPENING);
    }

    inline bool IsOpened(ASCState state) const
    {
        return (state == NL_SLE_ASC_OPENED);
    }

    inline bool IsStarting(ASCState state) const
    {
        return (state == NL_SLE_ASC_STARTING);
    }

    inline bool IsStarted(ASCState state) const
    {
        return (state == NL_SLE_ASC_STARTED);
    }

    bool IsInStopProcess(ASCState state) const;

    inline bool IsStopping(ASCState state) const
    {
        return (state == NL_SLE_ASC_STOPPING);
    }

    inline bool IsReconfigStopping(ASCState state) const
    {
        return (state == NL_SLE_ASC_RECONFIG_STOPPING);
    }

    inline bool IsReleasing(ASCState state) const
    {
        return (state == NL_SLE_ASC_RELEASING);
    }

    inline bool IsReleased(ASCState state) const
    {
        return (state == NL_SLE_ASC_RELEASED);
    }

    inline bool IsCallType(AudioStreamType streamType);

    bool IsL2HCVoice(uint8_t codecId);
    bool IsL2HC(uint8_t codecId);
    bool IsSameAsFormer(const ASCPlayRecord &formerPlayRecord, const RawAddress &device, Qos qos, uint8_t codecId);
    bool IsStartAtLowBps(const ASCPlayRecord &formerPlayRecord, Qos qos);

    /*
     * 状态上报函数
     */
    bool IsConnRptDelaying(const RawAddress &device) const;
    void SetConnRptDelayingFlag(const RawAddress &device, bool isDelaying);
    void DoDeviceConnectedReprot(const RawAddress& device);
    void DeviceReprotDelayCheck(const RawAddress& device);
    void ReportConnectStateChanged(const RawAddress& device, int cmd, int result, int reason);
    int GetReportResultCode(int result, int reason);
    void ReportAudioControlComplete(const RawAddress& device, AudioStreamType streamType, int cmd, int result,
        int reason);
    void ReportAudioControlCompleteSub(const RawAddress& device, AudioStreamType streamType, int cmd, int result,
        int reason);
    uint8_t GetCodeSoundChannelNum();
    uint8_t GetDecodeSoundChannelNum();
    void GetChannelNum(uint8_t codecId, uint8_t pointType, uint8_t& downChannelNum, uint8_t& upChannelNum);
    uint8_t GetCosetDeviceNum(const RawAddress& device);
    uint8_t GetCodecType(uint8_t codecId);
    uint8_t GetDspL2hcVersion(const AscQosmInfo& info);
    std::string GenerateCodecPara(const RawAddress& device, AudioStreamType streamType);
    void AddCoSetChannelInfo(const RawAddress& coSetDevice, std::string& channelInfoStr);
    std::string GenerateChannelInfo(const RawAddress& device, int cmd);
    void SetAudioDisconnInfo(const RawAddress& device, uint16_t connHandle);
    void SetAutorateParameter(const AscBitrateChange& ascBitrate);
    void SetAudioParameter(int cmd, const std::string& codecPara, const std::string& channelInfo);

    /*
     * 音频属性处理函数
     */
    std::vector<AscProp>& GetPropertyList(const RawAddress& device);
    void SaveProperty(const RawAddress& device, const std::vector<AscProp>& properties);
    bool FindBps(const AscProp& prop, uint8_t codecId, uint64_t& bps);
    uint64_t GetSupportBps(const RawAddress &device, NLSTK_ActmPointType_E pointType, uint8_t codecId);
    uint8_t GetBpsBitIndex(const RawAddress &device, uint64_t resultBps, uint8_t codecId, Qos qos);

    void SaveStreamQosmInfo(const RawAddress &device, const AscQosmInfo &info);
    void SetQosmInfoUpdateFlag(const RawAddress &device, bool isUpdated);
    bool IsQosmInfoUpdated(const RawAddress &device);
    bool GetStreamQosmInfo(const RawAddress &device, AscQosmInfo &info);
    void ClearStreamQosmInfo(const RawAddress &device);
    void PrintStartedStreamList(const RawAddress& device);
    void SyncWithCoSetDevice(const RawAddress& device, const RawAddress& coSetDevice, AudioStreamType procStream);
    void ChangeBpsRange();
    /*
     * 断连接场景状态处理函数
     */
    void InitDisconnProcTable();
    void InitMicStateObserver();
    void DeInitMicStateObserver();
    int DisconnProcSetFlag(const RawAddress &device, ASCState state);
    int DisconnProcStatusError(const RawAddress &device, ASCState state);
    int DisconnProcStopPlaying(const RawAddress &device, ASCState state);
    int DisconnProcSetStatus(const RawAddress &device, ASCState state);

    /*
     * 通话码率自适应处理函数
     */
    void HandleChangeFrameType();
    void HandleChangeVoiceCallBitrate(uint8_t result);
    void HandleChangeLabelIdRsp(uint8_t direction, uint8_t result);
    uint32_t GetGroupIdByAddress(const RawAddress &device);
    uint8_t GetExistPhyStauts(uint32_t groupId, std::string addr);
    bool UpdateAutorateGroupInfo(const AscBitrateChange &ascBitrate);
    void RemoveAutorateGroupByAddr(const RawAddress &device);
    void UpdatePhyStatus(std::vector<AscPhyStatus>& phyStatusList, bool isLevelUp, std::string dev,
        uint8_t retFrameType, uint8_t retPhyType);
    void RollBackPhyParam();
    bool IsAllCosetFrameTypeChangedSucc(uint32_t groupId);
    bool GetVoiceCallAcbStatus(const RawAddress &device, uint8_t& frameType, uint8_t& phyType);
    void SetVoiceCallAcbStatus(const RawAddress &device, uint8_t frameType, uint8_t phyType);
    void RecoverFrameTypeWhenReleaseStream(const RawAddress &device);
    void SetAutoRateRecvMsg(AscBitrateChange bitchange, NLSTK_ActmAutoRateRecvMsg_S& msg);
    bool IsExcuteChangeLocalBitrateNow(uint32_t groupId);
    void UpdateLocalDspBitrate(const AscBitrateChange& ascBitrate);
    void NotifyVoiceCallAutorateAblityToActm(const RawAddress& device, const AscQosmInfo& qosmInfo);
    void SyncAcbPhyStatusWithCoSetDevice(const RawAddress& device, const RawAddress& coSetDevice);
    /*
     * 编码速率判决函数
     */
    typedef struct {
        Qos                     qos;
        NLSTK_ActmPointType_E   pointType;
        uint8_t                 codecId;
        AudioStreamType         streamType;
    } JudgeL2HCParamStru;
    bool IsVidoeExist(const RawAddress &device, AudioStreamType streamType);
    uint64_t ChooseBps(const RawAddress &device, JudgeL2HCParamStru paramIn, uint64_t& bpsRange);
    uint64_t GetDefineBps(const RawAddress &device, const JudgeL2HCParamStru &paramIn);
    uint64_t GetVoiceCallDefineBps(const RawAddress &device, NLSTK_ActmPointType_E pointType);
    void SetPeerCodecParamConfig(const RawAddress &device, AscCodecIdKey codec, JudgeL2HCParamStru paramIn,
        NLSTK_L2HCConfig_S &l2hcParam);
    Qos ConvertQos(Qos qos);
    void SetStreamPara(const RawAddress &device, AudioStreamType streamType, Qos qos,
        NLSTK_ActmPointType_E pointType, NLSTK_ActmConfig_S *para);
    NLSTK_ActmPointType_E GetPointType(const RawAddress &device, Qos qos);
    AscCodecIdKey SelectPeerCodecId(const RawAddress &device, Qos qos, uint8_t comm);
    AscCodecIdKey SelectPeerCodecIdInMedia(const RawAddress &device, uint8_t comm);
    AscCodecIdKey SelectMediaCodecIdPolicy(const AscProp& prop);
    void GetCommType(Qos qos, uint8_t &commType);
    uint16_t GetPeerCodecSampleRateCap(const RawAddress &device, AscCodecIdKey codec);
    uint16_t GetLocalCodecSampleRateCap(AscCodecIdKey codec);
    uint8_t SelectCodecSampleRatePolicy(const RawAddress &device, AscCodecIdKey codec, JudgeL2HCParamStru paramIn);
    uint8_t GetPeerCodecBitDepthCap(const RawAddress &device, AscCodecIdKey codec);
    uint8_t GetLocalCodecBitDepthCap(AscCodecIdKey codec);
    uint8_t SelectCodecBitDepthPolicy(const RawAddress &device, AscCodecIdKey codec, JudgeL2HCParamStru paramIn);
    uint8_t GetPeerCodecChannelCap(const RawAddress &device, AscCodecIdKey codec);
    uint8_t GetLocalCodecChannelCap(AscCodecIdKey codec);
    uint8_t SelectCodecChannelPolicy(const RawAddress &device, AscCodecIdKey codec, JudgeL2HCParamStru paramIn);
    inline bool IsVoiceCallService(Qos qos)
    {
        return ((qos == NL_SLE_QOS_3) || (qos == NL_SLE_QOS_6));
    }

    inline bool IsMediaService(Qos qos)
    {
        return ((qos == NL_SLE_QOS_1) || (qos == NL_SLE_QOS_2) || (qos == NL_SLE_QOS_4) || (qos == NL_SLE_QOS_5) ||
            (qos == NL_SLE_QOS_7) || (qos == NL_SLE_QOS_8) || (qos == NL_SLE_QOS_9) || (qos == NL_SLE_QOS_10));
    }

    /*
     * 空间音频
     */
    void RegisterSpatialAudioListener();
    void SetSpatialAudioModeEnabled(const bool enabled);
    void SetSpatialAudioHeadTrackingEnabled(const bool enabled);
    void SetSpatialAudioSourceTypeSupported(const uint8_t sourceType);
    void SetSpatialAdaptiveSwitchEnabled(const bool enabled);
    bool IsSpatialAudioModeEnabled(void);
    bool IsSpatialAudioHeadTrackingEnabled(void);
    bool IsSpatialAudioSourceTypeSupported();
    bool IsSpatialAudioAdaptiveSwitchEnabled();
    void ChangeIsoParamIfNeed();

    void ProcessSpatialAudioBaseEvent(const ASCMessage &event);
    void ProcessSpatialAudioMoveEvent(const ASCMessage &event);

    bool IsSpatialAudioModeEnabled(const std::string &macAddr);
    bool IsSpatialAudioHeadTrackingEnabled(const std::string &macAddr);
    bool IsSpatialAudioAdaptiveSwitchEnabled(const std::string &macAddr);

    void RegisterAudioSceneListener();
    void CheckStreamIsNeedNotifyCcp(const RawAddress &device, AudioStreamType type, int cmd);
    int GetDftUpdateVoiceStackReason(int action);

    void SetAutoConnectDevice(const RawAddress &device, bool isActivate);
    void UpdateL2HCParamByTrans(const RawAddress &device, NLSTK_ActmConfig_S *para);
    // 上层管控subrate设置串行化
    bool IsASCNeedStartStreamChangeSubrate(const RawAddress &device);
    ASCSubRateState GetASCSubRateStatus(const RawAddress &device);
    void SetASCStartStreamChangeSubrateFlag(const RawAddress &device, bool val);
    void SetASCSubRateStatus(const RawAddress &device, ASCSubRateState state);
    void SetSubratePreConfigStream(const RawAddress &device);
    void ClearASCSubrateInfo(const RawAddress &device);
    void SerialManagerSubrate(bool &needConfigStream, const RawAddress &device, uint16_t subrate);
    bool IsAllowSubrateChangeReq(const RawAddress &device, const SleAcbSubrateParam &eventParam);
    bool IsRejectInActivateDeviceReq(const RawAddress &device, uint16_t subrate);
    void ProcessCachedSubrate();
    void SetSubrateCachedInfo(const RawAddress &device, const SleAcbSubrateParam &eventParam);
    void UpdateASCToDSPInfo(const RawAddress& device, const AscQosmInfo& info, ASCToDSPInfo &ascToDspInfo);
    std::string ASCToDSPInfoToString(const ASCToDSPInfo& ascToDspInfo);

    // 移动全景音
    void ProcessColAudioSwitchChangeEvent(const ASCMessage &event);
    void SetColAudioSwitchEnabled(const bool enabled);
    bool IsColAudioStreamExist(const RawAddress &device);
    bool IsNeedTransferForColAudio(AudioStreamType streamType);
    void ChangeIsoParamForColAudioIfNeed();
    void ProcColAudioIfNeed(const RawAddress& device, AudioStreamType streamType);

    // service status
    std::atomic_bool isStarted_ = ATOMIC_FLAG_INIT;

    std::shared_ptr<InterfaceASCCallback> callback_;
    ASCObserver* audioObserver_ = nullptr;

    typedef struct {
        RawAddress      dev;               // dev
        time_t          time;              // time
        bool            isLeft;            // 耳机左右角色
    } ASCConnectedDev;

    typedef struct {
        RawAddress      dev;              // dev
        bool            isCachedProc;     // 是否切换subrate(目前只限定2)
    } SubrateCached;

    // 切subrate 2缓存
    SubrateCached subrateCachedInfo_ {};

    // 已连接设备
    NearlinkSafeMap<std::string, ASCConnectedDev> connectedDev_;
    NearlinkSafeSet<std::string> connectedVirtualDev_;

    // Active sink device出声设备
    RawAddress activeSinkDevice_;
    uint64_t sinkDeviceStreamType_ = 0;
    // 之前的出声设备
    RawAddress formerSinkDevice_;
    ASCPlayRecord formerPlayRecord_ {};

    std::map<std::string, ASCStatus> ascStatusMap_;

    using ASCDisconnectStateProcFunc = int (ASCService::*)(const RawAddress &device, ASCState state);
    std::map<ASCState, ASCDisconnectStateProcFunc> disconnStateProcTable_;

    std::map<std::string, std::queue<AudioStreamType>> startBuff_;
    std::map<std::string, std::list<AudioStreamType>> startedStreamList;
    std::map<std::string, std::queue<AudioStreamType>> stopBuff_;
    std::map<std::string, AudioStreamType> reconfigStreamMap_;
    std::map<std::string, bool> forceReconfigMap_;

    // 流控制
    std::map<std::string, std::map<AudioStreamType, StreamResult>> streamResultMap_;

    // 获取的耳机侧音频属性
    std::map<std::string, std::vector<AscProp>> devicePropList_;
    // 已配置到底层的编解码参数（用于传递给音频框架和DSP）
    std::map<std::string, AscQosmInfo> streamQosmInfo_;

    // 定时器
    std::map<std::string, std::shared_ptr<NearlinkTimer>> startPlayingTimer_;
    std::map<std::string, std::shared_ptr<NearlinkTimer>> stopPlayingTimer_;
    std::map<std::string, std::shared_ptr<NearlinkTimer>> stopDelayTimer_;
    std::map<std::string, std::shared_ptr<NearlinkTimer>> connRptDelayTimer_;
    std::shared_ptr<NearlinkTimer> timerForRestoreVolumeIfPaused_ = nullptr;

    std::shared_ptr<AudioStandard::AudioSpatializationEnabledChangeCallback> spatialAudioModeCallback_ = nullptr;
    std::shared_ptr<AudioStandard::AudioHeadTrackingEnabledChangeCallback> spatialAudioHeadTrackingCallback_ = nullptr;
    std::shared_ptr<AudioStandard::AudioSpatialAudioSourceTypeChangeCallback> spatialAudioSourceTypeCallback_ = nullptr;
    std::shared_ptr<AudioStandard::AudioAdaptiveSpatialRenderingEnabledChangeCallback>
        spatialAudioAdaptiveSwitchCallback_ = nullptr;
    std::shared_ptr<AudioStandard::AudioPreferredOutputDeviceChangeCallback> audioPreferredOutPutDeviceCallback_ =
        nullptr;
    std::atomic<uint8_t> spatialAudioStateMask{ NL_SLE_ASC_SPATIAL_AUDIO_MODE_DISABLED };
    std::atomic<uint8_t> connectAudioDevice_{ 0 };
    std::atomic<int32_t> lastAudioConnectionState_{ static_cast<int32_t>(NL_SLE_ASC_STATE_NOT_CONNECTED) };

    // Listener
    std::atomic<bool> isListenerRegistered_ = false;
    std::shared_ptr<AudioSceneChangedListener> extAudioSceneListener_;
    std::atomic<bool> isCalling_ = false;
    std::atomic<bool> isColAudioEnabled_ = false;
    std::atomic<bool> isColAudioBeforeEnabled_ = false;
    AscMicStateObserver micStateObserver_;

    NearlinkSafeMap<std::string, uint8_t> playOrPauseDevice_;
    std::mutex dualRecCapStateMutex_;
    int8_t dualRecCapState_ = UNKNOWN;
    std::mutex karaokeCapStateMutex_;
    int8_t karaokeCapState_ = UNKNOWN;
    // 上层管控subrate设置串行化
    std::map<std::string, ASCSubRateInfo> ascSubrateMap_;

    std::map<uint32_t, AscAutorateInfo> ascAutorateMap_;
};
} // namespace Nearlink
} // namespace OHOS
#endif // ASC_SERVICE_H