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
#include "SleProfileConnectManager.h"
#include "SleInterfaceAdapter.h"
#include "SleInterfaceAdapterSub.h"
#include "SleInterfaceManager.h"
#include "SleInterfaceProfileHidHost.h"
#include "SleInterfaceProfileDis.h"
#include "SleInterfaceProfileBas.h"
#include "SleInterfaceProfileIcce.h"
#include "SleInterfaceProfileASC.h"
#include "HidHostDefines.h"
#include "SleInterfaceProfileVcp.h"
#include "nearlink_dft_exception.h"
#include "nearlink_datashare_helper.h"
#include "nearlink_common_event_helper.h"
#include "IcceService.h"
#include "SleInterfaceProfilePort.h"
#include "SleInterfaceProfileCdsm.h"
#include "PortService.h"
#include "SleInterfaceProfileTws.h"
#include "SleConfig.h"
#include "parameters.h"
#include "ThreadUtil.h"
#include "cm_api.h"
#include "TwsService.h"
#include "DisService.h"
#include "SleInterfaceProfileMic.h"
#include "SleRemoteDeviceAdapter.h"

namespace OHOS {
namespace Nearlink {
constexpr int SLE_ADAPTER_PROF_CONN_COUNT_NONE = 0;
constexpr int SLE_ADAPTER_PROF_CONN_COUNT_ONE = 1;
constexpr int ALL_PROFILE_CONNECT_SUCCESS = 0;
const int DEVICE_CLASS_VEHICLE_LOCK = 4100;
const int DEVICE_IN_EAR_EARPHONE = 2049;
const int DEVICE_SMART_WATCH = 1025;
const int DEVICE_NETWORKING = 0x000F00;
const std::string NO_SUPPORT_PROFILE = "no support profile";

SleProfileConnectManager *g_sleProfileConnectManageImpl_ = nullptr;
struct SleProfileConnectManager::impl {
    class DisCallback : public DisObserver {
    public:
        ~DisCallback() = default;
        void OnConnectionStateChanged(const RawAddress &deviceAddress, int state, int oldState) override
        {
            NL_CHECK_RETURN(g_sleProfileConnectManageImpl_, "g_sleProfileConnectManageImpl_ is null.");
            HILOGD("dis connect state changed state:%{public}d, oldState:%{public}d, %{public}s", state, oldState,
                GetEncryptAddr(deviceAddress.GetAddress()).c_str());
            DoInAdapterThread([sleProfileConnectManageImplPtr = g_sleProfileConnectManageImpl_,
                deviceAddress, state, oldState]()-> void {
                    sleProfileConnectManageImplPtr->ProfileConnectionStateChangedTask(
                        PROFILE_NAME_DIS, deviceAddress, state, oldState);
            });
        }
    };

    class ASCCallback : public ASCObserver {
    public:
        ~ASCCallback() = default;
        void OnConnectionStateChanged(const RawAddress &deviceAddress, int state, int oldState) override
        {
            HILOGD("ASC connect state changed state:%{public}d, oldState:%{public}d, %{public}s", state, oldState,
                GetEncryptAddr(deviceAddress.GetAddress()).c_str());
            NL_CHECK_RETURN(g_sleProfileConnectManageImpl_, "g_sleProfileConnectManageImpl_ is null.");
            DoInAdapterThread([sleProfileConnectManageImplPtr = g_sleProfileConnectManageImpl_,
                deviceAddress, state, oldState]()-> void {
                    sleProfileConnectManageImplPtr->ProfileConnectionStateChangedTask(
                        PROFILE_NAME_ASC, deviceAddress, state, oldState);
            });
        }
    };

    class IcceCallback : public IcceObserver {
    public:
        ~IcceCallback() = default;
        void OnConnectionStateChanged(const RawAddress &device, int curState, int prevState) override
        {
            NL_CHECK_RETURN(g_sleProfileConnectManageImpl_, "g_sleProfileConnectManageImpl_ is null.");
            HILOGI("icce connect state changed state:%{public}d, oldState:%{public}d, %{public}s", curState, prevState,
                GetEncryptAddr(device.GetAddress()).c_str());
            DoInAdapterThread([sleProfileConnectManageImplPtr = g_sleProfileConnectManageImpl_,
                device, curState, prevState]()-> void {
                    sleProfileConnectManageImplPtr->ProfileConnectionStateChangedTask(
                        PROFILE_NAME_ICCE, device, curState, prevState);
            });
        }
    };

    class HidHostCallback : public HidHostObserver {
    public:
        ~HidHostCallback() = default;
        void OnConnectionStateChanged(const RawAddress &deviceAddress, int state, int oldState) override
        {
            NL_CHECK_RETURN(g_sleProfileConnectManageImpl_, "g_sleProfileConnectManageImpl_ is null.");
            HILOGI("hidHost connect state changed state:%{public}d, oldState:%{public}d, %{public}s", state, oldState,
                GetEncryptAddr(deviceAddress.GetAddress()).c_str());
            DoInAdapterThread([sleProfileConnectManageImplPtr = g_sleProfileConnectManageImpl_,
                deviceAddress, state, oldState]()-> void {
                    sleProfileConnectManageImplPtr->ProfileConnectionStateChangedTask(
                        PROFILE_NAME_HID_HOST, deviceAddress, state, oldState);
            });
        }
    };

    class BasCallback : public BasObserver {
    public:
        ~BasCallback() = default;
        void OnConnectionStateChanged(const RawAddress &deviceAddress, int state, int oldState) override
        {
            NL_CHECK_RETURN(g_sleProfileConnectManageImpl_, "g_sleProfileConnectManageImpl_ is null.");
            HILOGD("bas connect state changed state:%{public}d, oldState:%{public}d, %{public}s", state, oldState,
                GetEncryptAddr(deviceAddress.GetAddress()).c_str());
            DoInAdapterThread([sleProfileConnectManageImplPtr = g_sleProfileConnectManageImpl_,
                deviceAddress, state, oldState]()-> void {
                    sleProfileConnectManageImplPtr->ProfileConnectionStateChangedTask(
                        PROFILE_NAME_BAS, deviceAddress, state, oldState);
            });
        }
    };

    class PortCallback : public PortObserver {
    public:
        ~PortCallback() = default;
        void OnConnectionStateChanged(const RawAddress &deviceAddress, int state, int oldState) override
        {
            NL_CHECK_RETURN(g_sleProfileConnectManageImpl_, "g_sleProfileConnectManageImpl_ is null.");
            HILOGD("portprofile connect state changed state:%{public}d, oldState:%{public}d, %{public}s",
                state, oldState, GetEncryptAddr(deviceAddress.GetAddress()).c_str());
            DoInAdapterThread([sleProfileConnectManageImplPtr = g_sleProfileConnectManageImpl_,
                deviceAddress, state, oldState]()-> void {
                    sleProfileConnectManageImplPtr->ProfileConnectionStateChangedTask(
                        PROFILE_NAME_PORT, deviceAddress, state, oldState);
            });
        }
    };

    class CdsmCallback : public CdsmObserver {
    public:
        ~CdsmCallback() = default;
        void OnConnectionStateChanged(const RawAddress &deviceAddress, int state, int oldState) override
        {
            NL_CHECK_RETURN(g_sleProfileConnectManageImpl_, "g_sleProfileConnectManageImpl_ is null.");
            HILOGD("cdsm profile connect state changed state:%{public}d, oldState:%{public}d, %{public}s",
                state, oldState, GetEncryptAddr(deviceAddress.GetAddress()).c_str());
            DoInAdapterThread([sleProfileConnectManageImplPtr = g_sleProfileConnectManageImpl_,
                deviceAddress, state, oldState]()-> void {
                    sleProfileConnectManageImplPtr->ProfileConnectionStateChangedTask(
                        PROFILE_NAME_CDSM, deviceAddress, state, oldState);
            });
        }
    };

    class TwsCallback : public TwsObserver {
    public:
        ~TwsCallback() = default;
        void OnConnectionStateChanged(const RawAddress &deviceAddress, int state, int oldState) override
        {
            NL_CHECK_RETURN(g_sleProfileConnectManageImpl_, "g_sleProfileConnectManageImpl_ is null.");
            HILOGD("Tws profile connect state changed state:%{public}d, oldState:%{public}d, %{public}s",
                state, oldState, GetEncryptAddr(deviceAddress.GetAddress()).c_str());
            DoInAdapterThread([sleProfileConnectManageImplPtr = g_sleProfileConnectManageImpl_,
                deviceAddress, state, oldState]()-> void {
                    sleProfileConnectManageImplPtr->ProfileConnectionStateChangedTask(
                        PROFILE_NAME_TWS, deviceAddress, state, oldState);
            });
        }
    };

    class VcpClientCallback : public VcpClientObserver {
    public:
        ~VcpClientCallback() = default;
        void OnConnectionStateChanged(const RawAddress &deviceAddress, int state, int oldState) override
        {
            NL_CHECK_RETURN(g_sleProfileConnectManageImpl_, "g_sleProfileConnectManageImpl_ is null.");
            HILOGD("vcp client profile connect state changed state:%{public}d, oldState:%{public}d, %{public}s",
                   state, oldState, GetEncryptAddr(deviceAddress.GetAddress()).c_str());
            DoInAdapterThread([sleProfileConnectManageImplPtr = g_sleProfileConnectManageImpl_,
                deviceAddress, state, oldState]()-> void {
                    sleProfileConnectManageImplPtr->ProfileConnectionStateChangedTask(
                        PROFILE_NAME_VCP, deviceAddress, state, oldState);
            });
        }
    };

    class MicCallback : public MicObserver {
    public:
        ~MicCallback() = default;
        void OnConnectionStateChanged(const RawAddress &deviceAddress, int state, int oldState) override
        {
            NL_CHECK_RETURN(g_sleProfileConnectManageImpl_, "g_sleProfileConnectManageImpl_ is null.");
            HILOGD("mic profile connect state changed state:%{public}d, oldState:%{public}d, %{public}s",
                   state, oldState, GetEncryptAddr(deviceAddress.GetAddress()).c_str());
            DoInAdapterThread([sleProfileConnectManageImplPtr = g_sleProfileConnectManageImpl_,
                deviceAddress, state, oldState]()-> void {
                    sleProfileConnectManageImplPtr->ProfileConnectionStateChangedTask(
                        PROFILE_NAME_MIC, deviceAddress, state, oldState);
            });
        }
    };

    NearlinkSafeMap<std::string, std::shared_ptr<SleProfileConnectInst>> SleProfileConnectInstSafeList_;
    DisCallback disCallback_;
    HidHostCallback hidHostCallback_;
    BasCallback basCallback_;
    IcceCallback icceCallback_;
    PortCallback portCallback_;
    CdsmCallback cdsmCallback_;
    ASCCallback audioCallback_;
    TwsCallback twsCallback_;
    VcpClientCallback VcpClientCallback_;
    MicCallback micCallback_;
};

void SleProfileConnectManager::SleProfileConnectInst::SsapClientCallback::OnConnectionStateChanged(
    uint8_t state, int ret)
{
    NL_CHECK_RETURN(g_sleProfileConnectManageImpl_, "g_sleProfileConnectManageImpl_ is null.");
    DoInAdapterThread([sleProfileConnectManageImplPtr = g_sleProfileConnectManageImpl_,
        device = device_, state]()-> void {
            sleProfileConnectManageImplPtr->SsapConnectionStateChangedTask(device, state);
    });
}

void SleProfileConnectManager::SleProfileConnectInst::SsapClientCallback::OnDiscoverComplete(int ret)
{
    NL_CHECK_RETURN(g_sleProfileConnectManageImpl_, "g_sleProfileConnectManageImpl_ is null.");
    DoInAdapterThread([sleProfileConnectManageImplPtr = g_sleProfileConnectManageImpl_,
        device = device_, ret]()-> void {
            sleProfileConnectManageImplPtr->SsapServicesDiscoveredTask(device, ret);
    });
}

SleProfileConnectManager::SleProfileConnectInst::SleProfileConnectInst(const RawAddress &addr) : addr_(addr),
    ssapCallback_(std::make_shared<SsapClientCallback>(addr))
{
    auto timeoutFunc = [addr]() {
        HILOG_COMM_INFO("[%{public}s:%{public}d]SleProfileConnectInst Timeout!", __FUNCTION__, __LINE__);
        DftReportPairInfo(addr.GetAddress(), PAIR_CONN_PATH_SSAP, SSAP_FAILURE, PAIR_SSAP_TIMEOUT);
        NL_CHECK_RETURN(g_sleProfileConnectManageImpl_, "g_sleProfileConnectManageImpl_ is null.");
        DoInAdapterThread([sleProfileConnectManageImplPtr = g_sleProfileConnectManageImpl_, addr]()-> void {
            sleProfileConnectManageImplPtr->ProfileConnTimeoutTask(addr);
        });
    };
    timer_ = std::make_shared<NearlinkTimer>(timeoutFunc);
}

std::string SleProfileConnectManager::SleProfileConnectInst::GetProfileConnectionStateName(
    int adapterProfConnState)
{
    switch (adapterProfConnState) {
        case SLE_ADAPTER_PROF_CONN_STATE_UNUSED:
            return "SLE_ADAPTER_PROF_CONN_STATE_UNUSED";
        case SLE_ADAPTER_PROF_CONN_STATE_ACB_CONNECTING:
            return "SLE_ADAPTER_PROF_CONN_STATE_ACB_CONNECTING";
        case SLE_ADAPTER_PROF_CONN_STATE_SSAP_CONNECTING:
            return "SLE_ADAPTER_PROF_CONN_STATE_SSAP_CONNECTING";
        case SLE_ADAPTER_PROF_CONN_STATE_DISCOVERING:
            return "SLE_ADAPTER_PROF_CONN_STATE_DISCOVERING";
        case SLE_ADAPTER_PROF_CONN_STATE_PROFILE_CONNECTING:
            return "SLE_ADAPTER_PROF_CONN_STATE_PROFILE_CONNECTING";
        case SLE_ADAPTER_PROF_CONN_STATE_CONNECTED:
            return "SLE_ADAPTER_PROF_CONN_STATE_CONNECTED";
        case SLE_ADAPTER_PROF_CONN_STATE_DISCONNECTING:
            return "SLE_ADAPTER_PROF_CONN_STATE_DISCONNECTING";
        default:
            return "";
    }
}

void SleProfileConnectManager::SleProfileConnectInst::SetState(uint8_t state)
{
    HILOGI("SleProfileConnectInstList_ device:%{public}s newstate:%{public}s",
        GetEncryptAddr(addr_.GetAddress()).c_str(), GetProfileConnectionStateName(state).c_str());
    StopConnTimer();
    state_ = state;
    if (state_ > SLE_ADAPTER_PROF_CONN_STATE_UNUSED && state_ < SLE_ADAPTER_PROF_CONN_STATE_CONNECTED) {
        StartConnTimer();
    }
    if (state_ == SLE_ADAPTER_PROF_CONN_STATE_WAIT_DISCONNECTED) {
        // 等待对端设备，确保双方都能对比profile的交集后再断连
        StartDisconnTimer();
    }
}

void SleProfileConnectManager::SleProfileConnectInst::SetSilentPortFlag(bool isSilentPort)
{
    HILOGI("SleProfileConnectInstList_ device:%{public}s isSilentPort:%{public}d",
        GetEncryptAddr(addr_.GetAddress()).c_str(), isSilentPort);
    isSilentPort_ = isSilentPort;
}

void SleProfileConnectManager::SleProfileConnectInst::SetProfileConnected(const std::string &profileName)
{
    HILOGD("device:%{public}s profileName:%{public}s", GetEncryptAddr(addr_.GetAddress()).c_str(), profileName.c_str());
    supportProfile_.EnsureInsert(profileName, static_cast<int>(SleConnectState::CONNECTED));
}

int SleProfileConnectManager::SleProfileConnectInst::GetProfileState(const std::string &profileName)
{
    int state = static_cast<int>(SleConnectState::DISCONNECTED);
    if (!supportProfile_.GetValue(profileName, state)) {
        HILOGE("get profile state fail, arr: :%{public}s, profile name: :%{public}s",
                GET_ENCRYPT_ADDR(addr_), profileName.c_str());
    }

    return state;
}

void SleProfileConnectManager::SleProfileConnectInst::AddSupportProfile(const std::string &profileName)
{
    HILOGI("device:%{public}s profileName:%{public}s", GetEncryptAddr(addr_.GetAddress()).c_str(), profileName.c_str());
    supportProfile_.EnsureInsert(profileName, static_cast<int>(SleConnectState::DISCONNECTED));
}

void SleProfileConnectManager::SleProfileConnectInst::UpdateProfileState(const std::string &profileName,
    const int newState)
{
    auto updateState = [newState](std::string proofile, int &state) -> void {
        state = newState;
    };
    supportProfile_.GetValueAndOpt(profileName, updateState);

    HILOGD("device:%{public}s profileName:%{public}s update state:%{public}d",
        GetEncryptAddr(addr_.GetAddress()).c_str(), profileName.c_str(), newState);
}

int SleProfileConnectManager::SleProfileConnectInst::GetConnectedProfileNumInner()
{
    int connectedCnt = 0;
    supportProfile_.Iterate([&connectedCnt](std::string first, int &state) -> void {
        if (state == static_cast<int>(SleConnectState::CONNECTED)) {
            connectedCnt++;
        }
    });

    HILOGD("device:%{public}s,connected cnt:%{public}d", GetEncryptAddr(addr_.GetAddress()).c_str(), connectedCnt);

    return connectedCnt;
}

int SleProfileConnectManager::SleProfileConnectInst::GetSupportedProfileNumInner()
{
    return supportProfile_.Size();
}

std::string SleProfileConnectManager::SleProfileConnectInst::GetNextDisconnectedProfile()
{
    // 私有服务在最前
    int state = static_cast<int>(SleConnectState::INVALID_STATE);
    if (supportProfile_.GetValue(PROFILE_NAME_TWS, state) &&
        (state == static_cast<int>(SleConnectState::DISCONNECTED))) {
        return PROFILE_NAME_TWS;
    }
    // 合作集其次
    if (supportProfile_.GetValue(PROFILE_NAME_CDSM, state) &&
        (state == static_cast<int>(SleConnectState::DISCONNECTED))) {
        return PROFILE_NAME_CDSM;
    }
    // 音量控制在音频流前面
    if (supportProfile_.GetValue(PROFILE_NAME_VCP, state) &&
        (state == static_cast<int>(SleConnectState::DISCONNECTED))) {
        return PROFILE_NAME_VCP;
    }

    // 其余除ASC以外的profile
    std::string profileName = "";
    supportProfile_.Find([&profileName](std::string first, int second) -> bool {
        if ((second == static_cast<int>(SleConnectState::DISCONNECTED)) &&
            (first != PROFILE_NAME_ASC) && (first != PROFILE_NAME_BAS)) {
            profileName = first;
            return true;
        }
        return false;
    });

    if (profileName != "") {
        return profileName;
    }

    // 音频流放最后
    if (supportProfile_.GetValue(PROFILE_NAME_ASC, state) &&
        (state == static_cast<int>(SleConnectState::DISCONNECTED))) {
        return PROFILE_NAME_ASC;
    }

    // BAS最后
    if (supportProfile_.GetValue(PROFILE_NAME_BAS, state) &&
        (state == static_cast<int>(SleConnectState::DISCONNECTED))) {
        return PROFILE_NAME_BAS;
    }

    return profileName;
}

std::string SleProfileConnectManager::SleProfileConnectInst::GetNextConnectedProfile()
{
    std::string profileName = "";
    supportProfile_.Find([&profileName](std::string first, int second) -> bool {
        if (second == static_cast<int>(SleConnectState::CONNECTED)) {
            profileName = first;
            return true;
        }
        return false;
    });
    return profileName;
}

SleProfileConnectManager::SleProfileConnectManager()
    : pimpl(std::make_unique<SleProfileConnectManager::impl>())
{
    NL_CHECK_RETURN(pimpl, "pimpl is null.");
    g_sleProfileConnectManageImpl_ = this;
}

SleProfileConnectManager::~SleProfileConnectManager()
{
    NL_CHECK_RETURN(pimpl, "pimpl is null.");
    HILOGI("SleProfileConnectInstSafeList_ Clear");
    pimpl->SleProfileConnectInstSafeList_.Clear();
}

void SleProfileConnectManager::Init(SleProfileConnectManagerFucs funcs)
{
    funcs_ = funcs;
    RegisterHidHostObserver();
    RegisterDisObserver();
    RegisterBasObserver();
    RegisterIcceObserver();
    RegisterPortObserver();
    RegisterAudioObserver();
    RegisterCdsmObserver();
    RegisterTwsObserver();
    RegisterVcpClientObserver();
    RegisterMicObserver();
}

void SleProfileConnectManager::RegisterHidHostObserver()
{
    ProfileHidHost *hidHostService = static_cast<ProfileHidHost *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_HID_HOST));
    NL_CHECK_RETURN(hidHostService, "cant find hid host service");
    hidHostService->RegisterObserver(pimpl->hidHostCallback_);
}

void SleProfileConnectManager::RegisterDisObserver()
{
    ProfileDis *disService = static_cast<ProfileDis *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_DIS));
    NL_CHECK_RETURN(disService, "cant find dis service");
    disService->RegisterObserver(pimpl->disCallback_);
}

void SleProfileConnectManager::RegisterBasObserver()
{
    ProfileBas *basService = static_cast<ProfileBas *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_BAS));
    NL_CHECK_RETURN(basService, "cant find dis service");
    basService->RegisterObserver(pimpl->basCallback_);
}

void SleProfileConnectManager::RegisterAudioObserver()
{
    ProfileASC *audioService = static_cast<ProfileASC *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_ASC));
    NL_CHECK_RETURN(audioService, "cant find audio service");
    audioService->RegisterObserver(pimpl->audioCallback_);
}

void SleProfileConnectManager::RegisterIcceObserver()
{
    ProfileIcce *icceClient = static_cast<ProfileIcce *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_ICCE));
    NL_CHECK_RETURN(icceClient, "cant find icce client");
    icceClient->RegisterObserver(pimpl->icceCallback_);
}

void SleProfileConnectManager::RegisterPortObserver()
{
    ProfilePort *portService = static_cast<ProfilePort *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_PORT));
    NL_CHECK_RETURN(portService, "cant find port profile service");
    portService->RegisterObserver(pimpl->portCallback_);
}

void SleProfileConnectManager::RegisterCdsmObserver()
{
    ProfileCdsm *cdsmService = static_cast<ProfileCdsm *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_CDSM));
    NL_CHECK_RETURN(cdsmService, "Can't find cdsm profile service");
    cdsmService->RegisterObserver(pimpl->cdsmCallback_);
}

void SleProfileConnectManager::RegisterTwsObserver()
{
    ProfileTws *twsService = static_cast<ProfileTws *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_TWS));
    NL_CHECK_RETURN(twsService, "Can't find tws profile service");
    twsService->RegisterObserver(pimpl->twsCallback_);
}

void SleProfileConnectManager::RegisterVcpClientObserver()
{
    ProfileVcp *vcpService = static_cast<ProfileVcp *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_VCP));
    NL_CHECK_RETURN(vcpService, "Can't find vcp client profile service");
    vcpService->RegisterObserver(pimpl->VcpClientCallback_);
}

void SleProfileConnectManager::RegisterMicObserver()
{
    ProfileMic *micService = static_cast<ProfileMic *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_MIC));
    NL_CHECK_RETURN(micService, "Can't find mic profile service");
    micService->RegisterObserver(pimpl->micCallback_);
}

void SleProfileConnectManager::ConnectProfile(const std::string &profileName, const RawAddress &device)
{
    if (funcs_.sendImgSecuConfig != nullptr && profileName == PROFILE_NAME_ASC) {
        // 连接ACS服务前，向对端设备发送组播安全配置消息
        funcs_.sendImgSecuConfig(device);
    }
    LOG_INFO("profileName=%{public}s device=%{public}s",
        profileName.c_str(), GetEncryptAddr(device.GetAddress()).c_str());
    SleInterfaceProfile *profileService = SleInterfaceProfileManager::GetInstance().GetProfileService(profileName);
    NL_CHECK_RETURN(profileService, "profileService is null.");
    profileService->Connect(device);
}

void SleProfileConnectManager::DisconnectProfile(const std::string &profileName, const RawAddress &device) const
{
    HILOGI("[SleProfileConnectManager] profileName=%{public}s device=%{public}s",
        profileName.c_str(), GET_ENCRYPT_ADDR(device));
    SleInterfaceProfile *profileService = SleInterfaceProfileManager::GetInstance().GetProfileService(profileName);
    NL_CHECK_RETURN(profileService, "profileService is null.");
    profileService->Disconnect(device);
}

void SleProfileConnectManager::ProfileConnectionStateChangedTask(
    const std::string &profileName, const RawAddress &device, int state, int oldState)
{
    LOG_INFO("profileName=%{public}s device=%{public}s, newState=%{public}d, oldState=%{public}d",
        profileName.c_str(), GetEncryptAddr(device.GetAddress()).c_str(), state, oldState);
    switch (state) {
        case static_cast<int>(SleConnectState::CONNECTED):
            ProcessProfileConnected(profileName, device);
            break;
        case static_cast<int>(SleConnectState::DISCONNECTED):
            ProcessProfileDisconnected(profileName, device);
            break;
        default:
            break;
    }
}

void SleProfileConnectManager::ProcessProfileDisconnected(const std::string &profileName, const RawAddress &device)
{
    std::shared_ptr<SleProfileConnectInst> profConnInst = nullptr;
    if (!pimpl->SleProfileConnectInstSafeList_.GetValue(device.GetAddress(), profConnInst)) {
        LOG_ERROR("no SleProfileConnectInst:%{public}s", GET_ENCRYPT_ADDR(device));
        return;
    }

    profConnInst->UpdateProfileState(profileName, static_cast<int>(SleConnectState::DISCONNECTED));

    if (profConnInst->GetState() == SLE_ADAPTER_PROF_CONN_STATE_WAIT_DISCONNECTED) {
        HILOGI("No avaiable service, wait for disconnected");
        SleConnectionChangedParam connChangedParam(static_cast<int>(SleConnectState::DISCONNECTED),
            static_cast<int>(SleConnectState::CONNECTED),
            static_cast<int>(SleConnectReason::CONNECT_FAIL_NO_AVAILABLE_SERVICE));
        funcs_.notifyConnectionStateChanged(device, connChangedParam);
        return;
    }

    if (profConnInst->GetState() == SLE_ADAPTER_PROF_CONN_STATE_PROFILE_CONNECTING) {
        DisconnectAcbInAbnormalSituations(device,
            static_cast<uint8_t>(SleDiscReason::SLE_DISC_REASON_REMOTE_USER_TERMINATED),
            static_cast<int>(SleConnectReason::CONNECT_LOCAL_DISCONNECT));
        return;
    }

    if (profConnInst->GetSilentPortFlag()) {
        HILOGI("ConnectedProfileNum=%{public}d", profConnInst->GetConnectedProfileNumInner());
        std::string nextDisconnectProfileName = profConnInst->GetNextConnectedProfile();
        if (nextDisconnectProfileName == "") {
            NL_CHECK_RETURN(funcs_.notifyConnectionStateChanged, "funcs_.notifyConnectionStateChanged is nullptr");
            int preState = GetProfileConnState(device);
            profConnInst->SetState(SLE_ADAPTER_PROF_CONN_STATE_UNUSED);
            SleConnectionChangedParam connChangedParam(static_cast<int>(SleConnectState::DISCONNECTED), preState);
            funcs_.notifyConnectionStateChanged(device, connChangedParam);
            profConnInst->SetState(SLE_ADAPTER_PROF_CONN_STATE_UNUSED);
            return;
        }
        profConnInst->SetState(SLE_ADAPTER_PROF_CONN_STATE_DISCONNECTING);
        DisconnectProfile(nextDisconnectProfileName, device);
        return;
    }

    if (profConnInst->GetConnectedProfileNumInner() == 0) {
        ClearProfileConnectInfo(device);
    }
}

void SleProfileConnectManager::NotifyProfileState(const std::string &profileName, const RawAddress &device,
    std::shared_ptr<SleProfileConnectInst> profConnInst)
{
    /* 当ASC、VCP两个服务都连接完成时，通知耳机服务连接完成 */
    int ascState = profConnInst->GetProfileState(PROFILE_NAME_ASC);
    int vcpState = profConnInst->GetProfileState(PROFILE_NAME_VCP);
    TwsService* twsService = TwsService::GetService();
    if (ascState == static_cast<int>(SleConnectState::CONNECTED) &&
        vcpState == static_cast<int>(SleConnectState::CONNECTED) &&
        twsService != nullptr) {
        twsService->SendProfileConnected(device);
    }
}

#ifdef WATCH_STANDARD
void SleProfileConnectManager::ProfileConnectedNotifyDataShare(const std::string &profileName,
    const RawAddress &device)
{
    if (profileName == PROFILE_NAME_DIS) {
        const std::string &name = SleConfig::GetInstance().GetPeerName(device.GetAddress());
        int appearance = SleConfig::GetInstance().GetPeerAppearance(device.GetAddress());
 
        NL_CHECK_RETURN(!name.empty(), "Invalid name length");
        if (appearance == DEVICE_CLASS_VEHICLE_LOCK) {
            NearlinkDataShareHelper::GetInstance().SaveNearlinkCarkeyName(name);
        }
    }
}
#endif

void SleProfileConnectManager::ProcessProfileConnected(const std::string &profileName, const RawAddress &device)
{
    HILOGD("SleProfileConnectManager::ProcessProfileConnected, device:%{public}s, profile=%{public}s",
        GetEncryptAddr(device.GetAddress()).c_str(), profileName.c_str());
    std::shared_ptr<SleProfileConnectInst> profConnInst = nullptr;
    if (!pimpl->SleProfileConnectInstSafeList_.GetValue(device.GetAddress(), profConnInst)) {
        LOG_ERROR("no SleProfileConnectInst:%{public}s", GET_ENCRYPT_ADDR(device));
        return;
    }
    if (ProcessProfileConnectedInner(profileName, device, profConnInst)) {
        return;
    }
    if (profConnInst->GetState() != SLE_ADAPTER_PROF_CONN_STATE_PROFILE_CONNECTING) {
        LOG_INFO("state error!");
        return;
    }
    // 收到profile连接，获取下一个要连接的profile
    profConnInst->SetProfileConnected(profileName);
    std::string nextConnectProfileName = profConnInst->GetNextDisconnectedProfile();
    if (nextConnectProfileName == "") {
        NL_CHECK_RETURN(funcs_.notifyConnectionStateChanged, "funcs_.notifyConnectionStateChanged is nullptr");
        int preState = ConvertToProfileConnectionState(profConnInst->GetState());
        profConnInst->SetState(SLE_ADAPTER_PROF_CONN_STATE_CONNECTED);
        SleConnectionChangedParam connChangedParam(static_cast<int>(SleConnectState::CONNECTED), preState,
            static_cast<int>(SleConnectReason::CONNECT_SUCCESS));
        funcs_.notifyConnectionStateChanged(device, connChangedParam);
        DftCachePairConnTime(device.GetAddress(), PAIR_CONN_PATH_ALL_PROFILE, SLE_PROFILE_FINISH_TIME);
        DftReportPairInfo(device.GetAddress(), PAIR_CONN_PATH_ALL_PROFILE, ALL_PROFILE_CONNECT_SUCCESS);
    } else {
        profConnInst->SetState(SLE_ADAPTER_PROF_CONN_STATE_PROFILE_CONNECTING);
        ConnectProfile(nextConnectProfileName, device);
    }

    /* 合作集report连接完成，保存所在合作集设备地址列表 */
    if (profileName == PROFILE_NAME_CDSM) {
        SleRemoteDeviceAdapter::GetInstance()->CdsmSaveData(device);
    }

    /* 当ASC和vcp连接完成时，通知对端服务连接状态 */
    if (profileName == PROFILE_NAME_VCP || profileName == PROFILE_NAME_ASC) {
        NotifyProfileState(profileName, device, profConnInst);
    }

#ifdef WATCH_STANDARD
    ProfileConnectedNotifyDataShare(profileName, device);
#endif
}

bool SleProfileConnectManager::ProcessProfileConnectedInner(const std::string &profileName, const RawAddress &device,
    std::shared_ptr<SleProfileConnectInst> profConnInst)
{
    if(profConnInst->GetState() != SLE_ADAPTER_PROF_CONN_STATE_WAIT_DISCONNECTED || profileName != PROFILE_NAME_DIS) {
        return false;
    }
    int remoteAppearance = SleConfig::GetInstance().GetPeerAppearance(device.GetAddress());
    int localAppearance = DEVICE_SMART_WATCH;
    DisService *disService = DisService::GetDisService();
    if (disService) {
        localAppearance = disService->GetDisServiceAppearance();
    }
    if (remoteAppearance == DEVICE_SMART_WATCH || remoteAppearance == DEVICE_CLASS_VEHICLE_LOCK ||
        localAppearance == DEVICE_SMART_WATCH || localAppearance == DEVICE_CLASS_VEHICLE_LOCK ||
        remoteAppearance == DEVICE_NETWORKING) {
        // 识别为不可用设备，但识别到手表、车钥匙业务场景，不做断连，恢复原有profile连接
        HILOGI("Local or remote is Watch or Carkey, keep connecting");
        profConnInst->SetState(SLE_ADAPTER_PROF_CONN_STATE_PROFILE_CONNECTING);
    } else if (SleRemoteDeviceAdapter::GetInstance()->GetConnDirect(device) ==
        static_cast<int>(SleConnDirect::SLE_CONNECTION_PASSIVE)) {
        // 识别为不可用设备，被连接端不主动做断连，等待连接端断连
        profConnInst->SetState(SLE_ADAPTER_PROF_CONN_STATE_UNUSED);
        // 报告连接状态，避免设置界面弹窗，避免控制中心显示连接态
        SleConnectionChangedParam connChangedParam(static_cast<int>(SleConnectState::DISCONNECTED),
            static_cast<int>(SleConnectState::CONNECTED),
            static_cast<int>(SleConnectReason::CONNECT_REMOTE_DISCONNECT));
        funcs_.notifyConnectionStateChanged(device, connChangedParam);
        return true;
    } else {
        // 识别为不可用设备，连接端主动断连DIS，并且等待定时器触发断连profilemanager的SSAP client
        HILOGI("No avaiable service, disconnect DIS");
        // 报告连接成功，避免设置界面弹窗
        SleConnectionChangedParam connChangedParam(static_cast<int>(SleConnectState::CONNECTED),
            static_cast<int>(SleConnectState::DISCONNECTED), static_cast<int>(SleConnectReason::CONNECT_SUCCESS));
        funcs_.notifyConnectionStateChanged(device, connChangedParam);
        DisconnectProfile(profileName, device);
        return true;
    }
    return false;
}

void SleProfileConnectManager::SleDisconnectAllProfileForSilentPort(const RawAddress &device) const
{
    // 耳机关盒静默数传，断开所有profile，只保留port数传
    std::shared_ptr<SleProfileConnectInst> profConnInst = nullptr;
    if (!pimpl->SleProfileConnectInstSafeList_.GetValue(device.GetAddress(), profConnInst)) {
        LOG_ERROR("no SleProfileConnectInst:%{public}s", GET_ENCRYPT_ADDR(device));
        return;
    }
    HILOGI("ConnectedProfileNum=%{public}d", profConnInst->GetConnectedProfileNumInner());
    std::string nextDisconnectProfileName = profConnInst->GetNextConnectedProfile();
    if (nextDisconnectProfileName == "") {
        HILOGW("no connected profile!");
        return;
    }
    profConnInst->SetState(SLE_ADAPTER_PROF_CONN_STATE_DISCONNECTING);
    profConnInst->SetSilentPortFlag(true);
    DftCacheDisconnInfoMsg(device.GetAddress(), DFT_DISCONN_DISCONNALLPROFILES);
    DisconnectProfile(nextDisconnectProfileName, device);
}

void SleProfileConnectManager::SleConnectAllProfile(const RawAddress &device) const
{
    DftSetConnectProfileTaskFlag(device.GetAddress());
    InterfaceProfileSsapClient *ssapClientService = GetSsapClientService();
    NL_CHECK_RETURN(ssapClientService, "ssapClientService is null.");
    NL_CHECK_RETURN(funcs_.notifyConnectionStateChanged, "funcs_.notifyConnectionStateChanged is nullptr");
    std::shared_ptr<SleProfileConnectInst> profConnInst = nullptr;
    if (!pimpl->SleProfileConnectInstSafeList_.GetValue(device.GetAddress(), profConnInst)) {
        LOG_INFO("add new SleProfileConnectInst, device:%{public}s",
            GetEncryptAddr(device.GetAddress()).c_str());
        std::shared_ptr<SleProfileConnectInst> profConnInst = std::make_shared<SleProfileConnectInst>(device);
        int appId = ssapClientService->RegisterApplication(profConnInst->GetSsapClientCallback(),
            device, SleTransport::ADAPTER_SLE, 0);
        LOG_INFO("appId_ %{public}d", appId);
        if (appId < 0) {
            return;
        } else if (ssapClientService->Connect(appId, false) != static_cast<int>(ReturnValue::RET_NO_ERROR)) {
            LOG_ERROR("SSAP connect faild!");
            return;
        }
        profConnInst->SetAppId(appId);
        profConnInst->SetState(SLE_ADAPTER_PROF_CONN_STATE_SSAP_CONNECTING);
        pimpl->SleProfileConnectInstSafeList_.EnsureInsert(device.GetAddress(), profConnInst);
        SleConnectionChangedParam connChangedParam(static_cast<int>(SleConnectState::CONNECTING),
            static_cast<int>(SleConnectState::DISCONNECTED), static_cast<int>(SleConnectReason::CONNECT_SUCCESS));
        funcs_.notifyConnectionStateChanged(device, connChangedParam);
        LOG_INFO("SleProfileConnectInstSafeList_ insert device:%{public}s",
            GetEncryptAddr(device.GetAddress()).c_str());
        return;
    } else if (profConnInst->GetAppId() < 0) {
        LOG_ERROR("appId_ is error! appId_=%{public}d", profConnInst->GetAppId());
        ClearProfileConnectInfo(device);
        return;
    } else if (profConnInst->GetState() != SLE_ADAPTER_PROF_CONN_STATE_UNUSED &&
        profConnInst->GetState() != SLE_ADAPTER_PROF_CONN_STATE_ACB_CONNECTING) {
        LOG_ERROR("state error! device:%{public}s, state:%{public}d",
            GetEncryptAddr(device.GetAddress()).c_str(), profConnInst->GetState());
        return;
    } else if (ssapClientService->Connect(profConnInst->GetAppId(), false) !=
        static_cast<int>(ReturnValue::RET_NO_ERROR)) {
        LOG_ERROR("device:%{public}s, SSAP connect faild!", GetEncryptAddr(device.GetAddress()).c_str());
        ClearProfileConnectInfo(device);
        return;
    }
    int preState = ConvertToProfileConnectionState(profConnInst->GetState());
    profConnInst->SetState(SLE_ADAPTER_PROF_CONN_STATE_SSAP_CONNECTING);
    SleConnectionChangedParam connChangedParam(static_cast<int>(SleConnectState::CONNECTING), preState,
        static_cast<int>(SleConnectReason::CONNECT_SUCCESS));
    funcs_.notifyConnectionStateChanged(device, connChangedParam);
}

void SleProfileConnectManager::NotifyConnectAcb(const RawAddress &device)
{
    NL_CHECK_RETURN(funcs_.notifyConnectionStateChanged, "funcs_.notifyConnectionStateChanged is nullptr");
    LOG_INFO("device:%{public}s", GetEncryptAddr(device.GetAddress()).c_str());
    std::shared_ptr<SleProfileConnectInst> profConnInst = nullptr;
    if (!pimpl->SleProfileConnectInstSafeList_.GetValue(device.GetAddress(), profConnInst)) {
        InterfaceProfileSsapClient *ssapClientService = GetSsapClientService();
        NL_CHECK_RETURN(ssapClientService, "ssapClientService is null.");
        std::shared_ptr<SleProfileConnectInst> profConnInst = std::make_shared<SleProfileConnectInst>(device);
        int appId = ssapClientService->RegisterApplication(profConnInst->GetSsapClientCallback(),
            device, SleTransport::ADAPTER_SLE, 0);
        LOG_INFO("appId_ %{public}d", appId);
        if (appId < 0) {
            return;
        }
        profConnInst->SetAppId(appId);
        profConnInst->SetState(SLE_ADAPTER_PROF_CONN_STATE_ACB_CONNECTING);
        pimpl->SleProfileConnectInstSafeList_.EnsureInsert(device.GetAddress(), profConnInst);
        SleConnectionChangedParam connChangedParam(static_cast<int>(SleConnectState::CONNECTING),
            static_cast<int>(SleConnectState::DISCONNECTED), static_cast<int>(SleConnectReason::CONNECT_SUCCESS));
        funcs_.notifyConnectionStateChanged(device, connChangedParam);
        LOG_INFO("SleProfileConnectInstSafeList_ insert device:%{public}s",
            GetEncryptAddr(device.GetAddress()).c_str());
        return;
    }
    int preState = ConvertToProfileConnectionState(profConnInst->GetState());
    profConnInst->SetState(SLE_ADAPTER_PROF_CONN_STATE_ACB_CONNECTING);
    SleConnectionChangedParam connChangedParam(static_cast<int>(SleConnectState::CONNECTING), preState,
        static_cast<int>(SleConnectReason::CONNECT_SUCCESS));
    funcs_.notifyConnectionStateChanged(device, connChangedParam);
}

void SleProfileConnectManager::NotifyAcbDisconnected(const RawAddress &device)
{
    NL_CHECK_RETURN(funcs_.notifyConnectionStateChanged, "funcs_.notifyConnectionStateChanged is nullptr");
    LOG_INFO("device:%{public}s", GetEncryptAddr(device.GetAddress()).c_str());
    std::shared_ptr<SleProfileConnectInst> profConnInst = nullptr;
    if (pimpl->SleProfileConnectInstSafeList_.GetValue(device.GetAddress(), profConnInst)) {
        int profileConnectPreState = profConnInst->GetState();
        int preState = ConvertToProfileConnectionState(profileConnectPreState);
        if (profileConnectPreState == SLE_ADAPTER_PROF_CONN_STATE_ACB_CONNECTING) {
            profConnInst->SetState(SLE_ADAPTER_PROF_CONN_STATE_UNUSED);
            SleConnectionChangedParam connChangedParam(static_cast<int>(SleConnectState::DISCONNECTED), preState);
            funcs_.notifyConnectionStateChanged(device, connChangedParam);
        }
    } else {
        LOG_INFO("device:%{public}s profConnInst has already cleared", GetEncryptAddr(device.GetAddress()).c_str());
        SleConnectionChangedParam connChangedParam(static_cast<int>(SleConnectState::DISCONNECTED),
            static_cast<int>(SleConnectState::DISCONNECTED));
        funcs_.notifyConnectionStateChanged(device, connChangedParam);
    }
}

uint8_t SleProfileConnectManager::GetProfileConnectState(const RawAddress &device)
{
    LOG_INFO("device:%{public}s", GetEncryptAddr(device.GetAddress()).c_str());
    std::shared_ptr<SleProfileConnectInst> profConnInst = nullptr;
    if (!pimpl->SleProfileConnectInstSafeList_.GetValue(device.GetAddress(), profConnInst)) {
        LOG_ERROR("no SleProfileConnectInst:%{public}s", GET_ENCRYPT_ADDR(device));
        return SLE_ADAPTER_PROF_CONN_STATE_UNUSED;
    }
    return profConnInst->GetState();
}

void SleProfileConnectManager::SsapConnectionStateChangedTask(const RawAddress &device, uint8_t newState)
{
    LOG_INFO("state:%{public}d", newState);
    std::shared_ptr<SleProfileConnectInst> profConnInst = nullptr;
    if (!pimpl->SleProfileConnectInstSafeList_.GetValue(device.GetAddress(), profConnInst)) {
        LOG_ERROR("no SleProfileConnectInst:%{public}s", GET_ENCRYPT_ADDR(device));
        return;
    }
    if (newState == static_cast<int>(SleConnectState::CONNECTED)) {
        if (SleConfig::GetInstance().GetPeerAppearance(device.GetAddress()) == DEVICE_CLASS_VEHICLE_LOCK) {
            LOG_INFO("skip service discover process for carkey");
            NL_CHECK_RETURN(funcs_.notifyConnectionStateChanged, "funcs_.notifyConnectionStateChanged is nullptr");
            int preState = ConvertToProfileConnectionState(profConnInst->GetState());
            profConnInst->SetState(SLE_ADAPTER_PROF_CONN_STATE_CONNECTED);
            SleConnectionChangedParam connChangedParam(static_cast<int>(SleConnectState::CONNECTED), preState,
                static_cast<int>(SleConnectReason::CONNECT_SUCCESS));
        funcs_.notifyConnectionStateChanged(device, connChangedParam);
            DftReportPairInfo(device.GetAddress(), PAIR_CONN_PATH_ALL_PROFILE, ALL_PROFILE_CONNECT_SUCCESS);
            return;
        }
        DiscoverStart(device);
    } else if (newState == static_cast<int>(SleConnectState::DISCONNECTED) &&
        profConnInst->GetConnectedProfileNumInner() == 0) {
        ClearProfileConnectInfo(device);
    }
}

void SleProfileConnectManager::DiscoverStart(const RawAddress &device)
{
    DftCachePairConnPath(device.GetAddress(), PAIR_CONN_PATH_DISCOVER);
    LOG_INFO("device=%{public}s", GetEncryptAddr(device.GetAddress()).c_str());

    InterfaceProfileSsapClient *ssapClientService = GetSsapClientService();
    NL_CHECK_RETURN(ssapClientService, "ssapClientService is null.");
    std::shared_ptr<SleProfileConnectInst> profConnInst = nullptr;
    if (!pimpl->SleProfileConnectInstSafeList_.GetValue(device.GetAddress(), profConnInst)) {
        LOG_ERROR("no SleProfileConnectInst:%{public}s", GET_ENCRYPT_ADDR(device));
        return;
    }
    int result = ssapClientService->DiscoverServices(profConnInst->GetAppId());
    if (result != static_cast<int>(ReturnValue::RET_NO_ERROR)) {
        LOG_ERROR("DiscoveryServices faild result=%{public}d", result);
        DftCacheDisconnInfoMsg(device.GetAddress(), DFT_DISCONN_DISCOVERFAIL);
        NL_CHECK_RETURN(funcs_.disconnectAcb, "funcs_.disconnectAcb is nullptr");
        funcs_.disconnectAcb(device, static_cast<uint8_t>(SleDiscReason::SLE_DISC_REASON_REMOTE_USER_TERMINATED));
        return;
    }

    NL_CHECK_RETURN(funcs_.notifyConnectionStateChanged, "funcs_.notifyConnectionStateChanged is nullptr");
    int preState = ConvertToProfileConnectionState(profConnInst->GetState());
    profConnInst->SetState(SLE_ADAPTER_PROF_CONN_STATE_DISCOVERING);
    SleConnectionChangedParam connChangedParam(static_cast<int>(SleConnectState::CONNECTING), preState,
        static_cast<int>(SleConnectReason::CONNECT_SUCCESS));
    funcs_.notifyConnectionStateChanged(device, connChangedParam);
}

void SleProfileConnectManager::SsapServicesDiscoveredTask(const RawAddress &device, int result)
{
    LOG_INFO("device:%{public}s, result:%{public}d", GetEncryptAddr(device.GetAddress()).c_str(), result);

    std::shared_ptr<SleProfileConnectInst> profConnInst = nullptr;
    NL_CHECK_RETURN(pimpl->SleProfileConnectInstSafeList_.GetValue(device.GetAddress(), profConnInst),
        "not find SleProfileConnectInst!");
    NL_CHECK_RETURN(profConnInst->GetState() == SLE_ADAPTER_PROF_CONN_STATE_DISCOVERING,
        "not discovering state! state:%{public}d", profConnInst->GetState());
    NL_CHECK_RETURN(funcs_.notifyConnectionStateChanged, "funcs_.notifyConnectionStateChanged is nullptr");

    if (result != static_cast<int>(SsapStatus::SSAP_SUCCESS)) {
        HandleServiceDiscoveryFailure(device, profConnInst);
        return;
    }
    // 获取支持的porfile
    bool isDisSupported = false;
    if (!SetSupportedProfileFlags(profConnInst, device, isDisSupported)) {
        HILOGI("SetSupportedProfileFlags failed. Only DIS service supported!");
        if (isDisSupported) {
            // 连接DIS服务，确保手表等外设配对后可读取相关信息
            ConnectProfile(PROFILE_NAME_DIS, device);
        }
        profConnInst->SetState(SLE_ADAPTER_PROF_CONN_STATE_WAIT_DISCONNECTED);
        return;
    }
    // 启动profile连接
    std::string profileName = profConnInst->GetNextDisconnectedProfile();
    if (profileName == "") {
        LOG_ERROR("no supported profile!");
        DftReportPairInfo(device.GetAddress(), PAIR_CONN_PATH_DISCOVER, SSAP_SUCCESS, NO_SUPPORT_PROFILE);
        int preState = ConvertToProfileConnectionState(profConnInst->GetState());
        profConnInst->SetState(SLE_ADAPTER_PROF_CONN_STATE_DISCONNECTING);
        SleConnectionChangedParam connChangedParam(static_cast<int>(SleConnectState::DISCONNECTING), preState,
            static_cast<int>(SleConnectReason::CONNECT_FAIL_NO_AVAILABLE_SERVICE));
        funcs_.notifyConnectionStateChanged(device, connChangedParam);
        DftCacheDisconnInfoMsg(device.GetAddress(), DFT_DISCONN_NOPROFILE);
        DisconnectAcbInAbnormalSituations(device,
            static_cast<uint8_t>(SleDiscReason::SLE_DISC_REASON_REMOTE_USER_TERMINATED),
            static_cast<int>(SleConnectReason::CONNECT_FAIL_NO_AVAILABLE_SERVICE));
    } else {
        int preState = ConvertToProfileConnectionState(profConnInst->GetState());
        profConnInst->SetState(SLE_ADAPTER_PROF_CONN_STATE_PROFILE_CONNECTING);
        SleConnectionChangedParam connChangedParam(static_cast<int>(SleConnectState::CONNECTING), preState,
            static_cast<int>(SleConnectReason::CONNECT_SUCCESS));
        funcs_.notifyConnectionStateChanged(device, connChangedParam);
        DftCachePairConnTime(device.GetAddress(), PAIR_CONN_PATH_ALL_PROFILE, SLE_PROFILE_START_TIME);
        ConnectProfile(profileName, device);
    }
}

void SleProfileConnectManager::HandleServiceDiscoveryFailure(
    const RawAddress &device, std::shared_ptr<SleProfileConnectInst> &profConnInst)
{
    LOG_ERROR("discover failed!");
    if (profConnInst->GetSilentPortFlag()) {
        SleConnectionChangedParam connChangedParam(static_cast<int>(SleConnectState::DISCONNECTED),
            GetProfileConnState(device),
            static_cast<int>(SleConnectReason::CONNECT_FAIL_SERVICE_DISCOVERY));
        funcs_.notifyConnectionStateChanged(device, connChangedParam);
        profConnInst->SetState(SLE_ADAPTER_PROF_CONN_STATE_UNUSED);
        return;
    }

    DftCacheDisconnInfoMsg(device.GetAddress(), DFT_DISCONN_DISCOVERFAIL);
    DisconnectAcbInAbnormalSituations(device,
        static_cast<uint8_t>(SleDiscReason::SLE_DISC_REASON_REMOTE_USER_TERMINATED),
        static_cast<int>(SleConnectReason::CONNECT_FAIL_SERVICE_DISCOVERY));
}

bool SleProfileConnectManager::SetSupportedProfileFlags(std::shared_ptr<SleProfileConnectInst> connInst,
    const RawAddress &device, bool &isDisSupported)
{
    InterfaceProfileSsapClient *ssapClientService = GetSsapClientService();
    NL_CHECK_RETURN_RET(ssapClientService, false, "ssapClientService is null.");
    std::list<Service> services = ssapClientService->GetServices(connInst->GetAppId());
    if (services.empty()) {
        return true;
    }
    auto &manager = ProfileServiceManager::GetInstance();
    // 获取手机支持的业务
    std::unordered_set<SleUuid> systemSupport = manager.GetSystemSupportProfileServices();
    bool hasAvailableService = false;
    int appearance = SleConfig::GetInstance().GetPeerAppearance(device.GetAddress());
    // 取手机支持以及私有服务与对端设备交集(除DIS与PORT业务--手机与手机不允许连接)
    for (auto &service : services) {
        LOG_DEBUG("service.uuid_:%{public}s", service.uuid_.GetEncryptUuid().c_str());
        /* TWS私有服务 */
        if (service.uuid_ == Uuid::ConvertFromString(SLE_UUID_TWS_PROFILE)) {
            /* TWS仅耳机算可用业务 */
            hasAvailableService |= (appearance == DEVICE_IN_EAR_EARPHONE);
            connInst->AddSupportProfile(PROFILE_NAME_TWS);
            continue;
        }
        /* 目前系统支持业务仅支持16位SleUuid */
        auto iter = systemSupport.find(static_cast<SleUuid>(service.uuid_.ConvertTo16Bits()));
        if (iter == systemSupport.end()) {
            // 与系统支持业务无匹配，跳过
            continue;
        }
        isDisSupported |= AddIntersectionProfileServices(connInst, *iter, hasAvailableService, appearance);
    }
    /* 更新设备记录 */
    SleRemoteDeviceAdapter::GetInstance()->SetDeviceIsAvailable(device, hasAvailableService);
    return hasAvailableService;
}

/**
 * 使用表管理ProfileService的配置
 * 当前结构：<SleUuid, <ProfileName, IsAvailable>>
 */
static const std::unordered_map<SleUuid, std::pair<std::string, bool>> uuidToProfileMap = {
    {SleUuid::SLE_STANDARD_SERVICE_HID_UUID, {PROFILE_NAME_HID_HOST, true}},
    {SleUuid::SLE_STANDARD_SERVICE_HID_UUID_PEN, {PROFILE_NAME_HID_HOST, true}},
    {SleUuid::UUID_DEVICE_INFORMATION_SERVICE, {PROFILE_NAME_DIS, false}},  // DIS不算可用业务
    {SleUuid::UUID_DEVICE_INFORMATION_SERVICE_PEN, {PROFILE_NAME_DIS, false}},  // DIS不算可用业务
    {SleUuid::UUID_BATTERY_SERVICE, {PROFILE_NAME_BAS, true}},
    {SleUuid::UUID_BATTERY_SERVICE_PEN, {PROFILE_NAME_BAS, true}},
    {SleUuid::SLE_STANDARD_SERVICE_ICCE_UUID, {PROFILE_NAME_ICCE, false}},   // ICCE仅车钥匙算可用业务
    {SleUuid::UUID_PORT_PROFILE_SERVICE, {PROFILE_NAME_PORT, false}},   // PORT不算可用业务
    {SleUuid::SLE_STANDARD_ASC_MGMT_UUID, {PROFILE_NAME_ASC, true}},
    {SleUuid::SLE_STANDARD_ASC_ABLTY_UUID, {PROFILE_NAME_ASC, true}},
    {SleUuid::SLE_STANDARD_SERVICE_CDSM_UUID, {PROFILE_NAME_CDSM, true}},
    {SleUuid::SLE_STANDARD_SERVICE_VCP_UUID, {PROFILE_NAME_VCP, true}},
    {SleUuid::SLE_STANDARD_SERVICE_MIC_UUID, {PROFILE_NAME_MIC, true}},
};

bool SleProfileConnectManager::AddIntersectionProfileServices(std::shared_ptr<SleProfileConnectInst> &connInst,
    SleUuid sleUuid, bool &hasAvailableService, int appearance)
{
    bool isDisService = false;
    auto it = uuidToProfileMap.find(sleUuid);
    if (it != uuidToProfileMap.end()) {
        if (it->second.second) {
            hasAvailableService |= true;
        }
        if (sleUuid == SleUuid::SLE_STANDARD_SERVICE_ICCE_UUID) {
            hasAvailableService |= (appearance == DEVICE_CLASS_VEHICLE_LOCK);
        }
        if (sleUuid == SleUuid::UUID_DEVICE_INFORMATION_SERVICE ||
            sleUuid == SleUuid::UUID_DEVICE_INFORMATION_SERVICE_PEN) {
            isDisService = true;
        }
        connInst->AddSupportProfile(it->second.first);
        if (sleUuid == SleUuid::SLE_STANDARD_ASC_MGMT_UUID || sleUuid == SleUuid::SLE_STANDARD_ASC_ABLTY_UUID) {
            SleRemoteDeviceAdapter::GetInstance()->SetAudioDeviceFlag(connInst->GetAddr());
        }
    } else {
        /* 系统支持业务获取错误 */
        LOG_ERROR("Get wrong support service:SleUuid-%{public}d", sleUuid);
    }
    return isDisService;
}

void SleProfileConnectManager::DisconnectAcbInAbnormalSituations(
    const RawAddress &device, uint8_t acbDisconnReason, int connChangedReason)
{
    std::shared_ptr<SleProfileConnectInst> profConnInst = nullptr;
    NL_CHECK_RETURN(pimpl->SleProfileConnectInstSafeList_.GetValue(device.GetAddress(), profConnInst),
        "not find SleProfileConnectInst!");
    NL_CHECK_RETURN(funcs_.disconnectAcb, "funcs_.disconnectAcb is nullptr");
    funcs_.disconnectAcb(device, acbDisconnReason);
    NL_CHECK_RETURN(funcs_.notifyConnectionStateChanged, "funcs_.notifyConnectionStateChanged is nullptr");
    if (profConnInst->GetConnectedProfileNumInner() == 0) {
        int preState = ConvertToProfileConnectionState(profConnInst->GetState());
        profConnInst->SetState(SLE_ADAPTER_PROF_CONN_STATE_UNUSED);
        SleConnectionChangedParam connChangedParam(
            static_cast<int>(SleConnectState::DISCONNECTED), preState, connChangedReason);
        funcs_.notifyConnectionStateChanged(device, connChangedParam);
    }

    /* 连接异常时：强制停合作集邀请广播 */
    ProfileCdsm *cdsmService = static_cast<ProfileCdsm *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_CDSM));
    NL_CHECK_RETURN(cdsmService, "cdsmService is null");
    cdsmService->CdsmStopInviteAdv(device, true);
}

void SleProfileConnectManager::DeregisterSsapClientApplication(const RawAddress &addr) const
{
    InterfaceProfileSsapClient *ssapClientService = GetSsapClientService();
    NL_CHECK_RETURN(ssapClientService, "ssapClientService is null.");
    std::shared_ptr<SleProfileConnectInst> profConnInst = nullptr;
    if (!pimpl->SleProfileConnectInstSafeList_.GetValue(addr.GetAddress(), profConnInst)) {
        LOG_ERROR("no SleProfileConnectInst:%{public}s", GET_ENCRYPT_ADDR(addr));
        return;
    }
    int ret = ssapClientService->DeregisterApplication(profConnInst->GetAppId());
    if (ret != static_cast<int>(SsapStatus::SSAP_SUCCESS)) {
        LOG_ERROR("SleProfileConnectInstSafeList_ deregister ssap client service result:%{public}d", ret);
    }
}

void SleProfileConnectManager::ClearProfileConnectInfo(const RawAddress &peerAddr) const
{
    std::shared_ptr<SleProfileConnectInst> profConnInst = nullptr;
    if (!pimpl->SleProfileConnectInstSafeList_.GetValue(peerAddr.GetAddress(), profConnInst)) {
        LOG_ERROR("no SleProfileConnectInst:%{public}s", GET_ENCRYPT_ADDR(peerAddr));
        return;
    }
    NL_CHECK_RETURN(funcs_.notifyConnectionStateChanged, "funcs_.notifyConnectionStateChanged is nullptr");
    int preState = GetProfileConnState(peerAddr);
    DeregisterSsapClientApplication(peerAddr);
    pimpl->SleProfileConnectInstSafeList_.Erase(peerAddr.GetAddress());
    SleConnectionChangedParam connChangedParam(static_cast<int>(SleConnectState::DISCONNECTED), preState);
    funcs_.notifyConnectionStateChanged(peerAddr, connChangedParam);
    LOG_INFO("SleProfileConnectInstSafeList_ erase device:%{public}s", GetEncryptAddr(peerAddr.GetAddress()).c_str());
}

void SleProfileConnectManager::ClearAllProfileConnectInfo() const
{
    HILOGI("SleProfileConnectInstSafeList_ Clear");
    std::vector<RawAddress> device;
    pimpl->SleProfileConnectInstSafeList_.Iterate(
        [this, &device](std::string key, std::shared_ptr<SleProfileConnectInst> profConnInst) -> void {
        if (profConnInst->GetState() == static_cast<uint8_t>(SLE_ADAPTER_PROF_CONN_STATE_UNUSED)) {
            return;
        }
        RawAddress peerAddr(key);
        device.push_back(peerAddr);
    });
    pimpl->SleProfileConnectInstSafeList_.Clear();
    for (const RawAddress &peerAddr : device) {
        SleConnectionChangedParam connChangedParam(static_cast<int>(SleConnectState::DISCONNECTED),
            static_cast<int>(SleConnectState::DISCONNECTING));
        funcs_.notifyConnectionStateChanged(peerAddr, connChangedParam);
    }
}

int SleProfileConnectManager::GetConnectedProfileNum(const RawAddress &peerAddr) const
{
    std::shared_ptr<SleProfileConnectInst> profConnInst = nullptr;
    if (!pimpl->SleProfileConnectInstSafeList_.GetValue(peerAddr.GetAddress(), profConnInst)) {
        LOG_DEBUG("no SleProfileConnectInst,dev:%{public}s", GET_ENCRYPT_ADDR(peerAddr));
        return 0;
    }

    return profConnInst->GetConnectedProfileNumInner();
}

bool SleProfileConnectManager::IsAllProfileConnected(const RawAddress &peerAddr) const
{
    std::shared_ptr<SleProfileConnectInst> profConnInst = nullptr;
    if (!pimpl->SleProfileConnectInstSafeList_.GetValue(peerAddr.GetAddress(), profConnInst)) {
        LOG_DEBUG("no SleProfileConnectInst,dev:%{public}s", GET_ENCRYPT_ADDR(peerAddr));
        return false;
    }

    int supportCnt = profConnInst->GetSupportedProfileNumInner();
    int connectedCnt = profConnInst->GetConnectedProfileNumInner();

    if (supportCnt == 0) {
        LOG_ERROR("no supported profile,addr:%{public}s", GET_ENCRYPT_ADDR(peerAddr));
        return false;
    }

    return (supportCnt == connectedCnt);
}

int SleProfileConnectManager::ConvertToProfileConnectionState(int adapterProfConnState) const
{
    switch (adapterProfConnState) {
        case SLE_ADAPTER_PROF_CONN_STATE_UNUSED:
            return static_cast<int>(SleConnectState::DISCONNECTED);
        case SLE_ADAPTER_PROF_CONN_STATE_ACB_CONNECTING:
        case SLE_ADAPTER_PROF_CONN_STATE_SSAP_CONNECTING:
        case SLE_ADAPTER_PROF_CONN_STATE_DISCOVERING:
        case SLE_ADAPTER_PROF_CONN_STATE_PROFILE_CONNECTING:
            return static_cast<int>(SleConnectState::CONNECTING);
        case SLE_ADAPTER_PROF_CONN_STATE_CONNECTED:
            return static_cast<int>(SleConnectState::CONNECTED);
        case SLE_ADAPTER_PROF_CONN_STATE_DISCONNECTING:
            return static_cast<int>(SleConnectState::DISCONNECTING);
        default:
            return static_cast<int>(SleConnectState::DISCONNECTED);
    }
}

int SleProfileConnectManager::GetProConnDeviceCount() const
{
    int count = 0;
    pimpl->SleProfileConnectInstSafeList_.Iterate([&count](
        const std::string first, std::shared_ptr<SleProfileConnectInst> &second) -> void {
        if (second->GetState() == SLE_ADAPTER_PROF_CONN_STATE_CONNECTED) {
            count++;
        }
    });
    return count;
}

int SleProfileConnectManager::GetProfileConnState(const RawAddress &device) const
{
    std::shared_ptr<SleProfileConnectInst> profConnInst = nullptr;
    if (!pimpl->SleProfileConnectInstSafeList_.GetValue(device.GetAddress(), profConnInst)) {
        LOG_DEBUG("device:%{public}s, not find SleProfileConnectInst!",
                  GetEncryptAddr(device.GetAddress()).c_str());
        return static_cast<int>(SleConnectState::DISCONNECTED);
    }
    return ConvertToProfileConnectionState(profConnInst->GetState());
}

bool SleProfileConnectManager::GetSilentPortState(const RawAddress &device) const
{
    std::shared_ptr<SleProfileConnectInst> profConnInst = nullptr;
    if (!pimpl->SleProfileConnectInstSafeList_.GetValue(device.GetAddress(), profConnInst)) {
        LOG_DEBUG("device:%{public}s, not find SleProfileConnectInst!",
                  GetEncryptAddr(device.GetAddress()).c_str());
        return false;
    }
    return profConnInst->GetSilentPortFlag();
}

InterfaceProfileSsapClient *SleProfileConnectManager::GetSsapClientService() const
{
    return static_cast<InterfaceProfileSsapClient *>
        (SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_SSAP_CLIENT));
}

void SleProfileConnectManager::ProfileConnTimeoutTask(const RawAddress &device)
{
    bool isNeedDisconnectAcb = false;
    pimpl->SleProfileConnectInstSafeList_.GetValueAndOpt(device.GetAddress(),
        [this, device, &isNeedDisconnectAcb](std::string key, std::shared_ptr<SleProfileConnectInst> value) {
            LOG_INFO("Timeout-State(%{public}d)-AppId(%{public}d)", value->GetState(), value->GetAppId());
            InterfaceProfileSsapClient *ssapClientService = this->GetSsapClientService();
            if (value->GetState() == SLE_ADAPTER_PROF_CONN_STATE_WAIT_DISCONNECTED) {
                ssapClientService->Disconnect(value->GetAppId());
                DftCacheDisconnInfoMsg(device.GetAddress(), DFT_DISCONN_NOAVAILABLEPROFILE);
                return;
            }
            isNeedDisconnectAcb = true;
        });
    if (isNeedDisconnectAcb) {
        this->DisconnectAcbInAbnormalSituations(device,
            static_cast<uint8_t>(SleDiscReason::SLE_DISC_REASON_COMMAND_TIMEOUT),
            static_cast<int>(SleConnectReason::CONNECT_FAIL));
        DftCacheDisconnInfoMsg(device.GetAddress(), DFT_DISCONN_PROFILECONNTIMEOUT);
    }
}

void SleProfileConnectManager::StopTimerIfRePair(const RawAddress &device)
{
    std::shared_ptr<SleProfileConnectInst> profConnInst = nullptr;
    if (!pimpl->SleProfileConnectInstSafeList_.GetValue(device.GetAddress(), profConnInst)) {
        LOG_ERROR("no SleProfileConnectInst:%{public}s", GET_ENCRYPT_ADDR(device));
        return;
    }
    profConnInst->StopConnTimer();
}
}  // namespace Nearlink
}  // namespace OHOS