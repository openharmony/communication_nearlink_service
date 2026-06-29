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
#ifndef SLE_PROFILE_CONNECT_MANAGER_H
#define SLE_PROFILE_CONNECT_MANAGER_H

#include <unordered_set>
#include "log_util.h"
#include "SleDefs.h"
#include "nearlink_timer.h"
#include "interface_profile_ssap_client.h"
#include "nearlink_safe_map.h"
#include "SleServiceFfrtLog.h"

namespace OHOS {
namespace Nearlink {

struct SleProfileConnectManagerFucs {
    std::function<void(const RawAddress &, const SleConnectionChangedParam &)> notifyConnectionStateChanged = nullptr;
    std::function<void(const RawAddress &, uint8_t)> disconnectAcb = nullptr;
    std::function<void(const RawAddress &)> sendImgSecuConfig = nullptr;
};

class SleProfileConnectManager {
public:
    class SleProfileConnectInst {
    public:
        static constexpr int CONN_TIMEOUT_MS = 11000;  // 11s
        static constexpr int DISCONN_TIMEOUT_MS = 3000;  // 3s
        class SsapClientCallback : public InterfaceSsapClientCallback {
        public:
            explicit SsapClientCallback(const RawAddress &addr) : device_(addr)
            {}
            ~SsapClientCallback()
            {}

            void OnWriteDescriptor(Descriptor &descriptor, int ret) override
            {}

            void OnConnectionStateChanged(uint8_t state, int ret) override;
            void OnDiscoverComplete(int ret) override;
        private:
            RawAddress device_;
        };

        explicit SleProfileConnectInst(const RawAddress &addr);
        ~SleProfileConnectInst()
        {}
        std::string GetProfileConnectionStateName(int adapterProfConnState);
        void SetState(uint8_t state);
        void SetSilentPortFlag(bool isSilentPort);
        void SetProfileConnected(const std::string &profileName);
        int GetProfileState(const std::string &profileName);
        void AddSupportProfile(const std::string &profileName);
        int GetConnectedProfileNumInner();
        int GetSupportedProfileNumInner();
        void UpdateProfileState(const std::string &profileName, const int newState);
        std::string GetNextDisconnectedProfile();
        std::string GetNextConnectedProfile();
        uint8_t GetState()
        {
            return state_;
        }
        bool GetSilentPortFlag()
        {
            return isSilentPort_;
        }
        void SetAppId(int appId)
        {
            appId_ = appId;
        }
        int GetAppId()
        {
            return appId_;
        }
        std::shared_ptr<SsapClientCallback> GetSsapClientCallback()
        {
            return ssapCallback_;
        }
        RawAddress GetAddr()
        {
            return addr_;
        }
        void StartConnTimer() const
        {
            timer_->Start(CONN_TIMEOUT_MS);
        }
        void StopConnTimer() const
        {
            timer_->Stop();
        }
        void StartDisconnTimer() const
        {
            timer_->Start(DISCONN_TIMEOUT_MS);
        }
    private:
        RawAddress addr_;
        int appId_ = -1;
        uint8_t state_ = SLE_ADAPTER_PROF_CONN_STATE_UNUSED;
        bool isSilentPort_ = false;
        std::shared_ptr<NearlinkTimer> timer_ = nullptr;
        std::shared_ptr<SsapClientCallback> ssapCallback_ = nullptr;
        NearlinkSafeMap<std::string, int> supportProfile_;
    };

    InterfaceProfileSsapClient *GetSsapClientService() const;

    SleProfileConnectManager();
    virtual ~SleProfileConnectManager();
    void Init(SleProfileConnectManagerFucs funcs);
    void SleConnectAllProfile(const RawAddress &device) const;
    void SleDisconnectAllProfileForSilentPort(const RawAddress &device) const;
    void NotifyConnectAcb(const RawAddress &device);
    void NotifyAcbDisconnected(const RawAddress &device);
    uint8_t GetProfileConnectState(const RawAddress &device);
    void ClearProfileConnectInfo(const RawAddress &peerAddr) const;
    void ClearAllProfileConnectInfo() const;
    int GetConnectedProfileNum(const RawAddress &peerAddr) const;
    bool IsAllProfileConnected(const RawAddress &peerAddr) const;

    int GetProfileConnState(const RawAddress &device) const;
    bool GetSilentPortState(const RawAddress &device) const;
    void StopTimerIfRePair( const RawAddress &device);

private:
    void RegisterHidHostObserver();
    void RegisterDisObserver();
    void RegisterBasObserver();
    void RegisterIcceObserver();
    void RegisterPortObserver();
    void RegisterCdsmObserver();
    void RegisterAudioObserver();
    void RegisterTwsObserver();
    void RegisterVcpClientObserver();
    void RegisterMicObserver();
    // profile
    void ConnectProfile(const std::string &profileName, const RawAddress &device);
    void DisconnectProfile(const std::string &profileName, const RawAddress &device) const;
    void ProfileConnectionStateChangedTask(
        const std::string &profileName, const RawAddress &device, int state, int oldState);
    void ProcessProfileDisconnected(const std::string &profileName, const RawAddress &device);
    void ProcessProfileConnected(const std::string &profileName, const RawAddress &device);
    bool ProcessProfileConnectedInner(const std::string &profileName, const RawAddress &device,
        std::shared_ptr<SleProfileConnectInst> profConnInst);
    void NotifyProfileState(const std::string &profileName, const RawAddress &device,
        std::shared_ptr<SleProfileConnectInst> profConnInst);
    int ConvertToProfileConnectionState(int adapterProfConnState) const;
    // ssap
    void SsapConnectionStateChangedTask(const RawAddress &device, uint8_t newState);
    void DiscoverStart(const RawAddress &device);
    void SsapServicesDiscoveredTask(const RawAddress &device, int result);
    void HandleServiceDiscoveryFailure(
        const RawAddress &device, std::shared_ptr<SleProfileConnectInst> &profConnInst);
    bool SetSupportedProfileFlags(std::shared_ptr<SleProfileConnectInst> connInst,
        const RawAddress &device, bool &isDisSupported);
    bool AddIntersectionProfileServices(std::shared_ptr<SleProfileConnectInst> &connInst,
        SleUuid sleUuid, bool &isPublicServiceEmpty, int appearance);

    int GetProConnDeviceCount() const;
    void DeregisterSsapClientApplication(const RawAddress &addr) const;
    void DisconnectAcbInAbnormalSituations(const RawAddress &device, uint8_t acbDisconnReason, int connChangedReason);
    void ProfileConnTimeoutTask(const RawAddress &device);
#ifdef WATCH_STANDARD
    void ProfileConnectedNotifyDataShare(const std::string &profileName, const RawAddress &device);
#endif

    SleProfileConnectManagerFucs funcs_;
    SLE_DISALLOW_COPY_AND_ASSIGN(SleProfileConnectManager);
    DECLARE_IMPL();
};

}  // namespace Nearlink
}  // namespace OHOS

#endif  // SLE_PROFILE_CONNECT_MANAGER_H