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
#include "SleAdapter.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <future>
#include <thread>

#include "BaseObserverList.h"
#include "ClassCreator.h"
#include "Compat.h"
#include "cm_api.h"
#include "cm_icb_api.h"
#include "interface_scan_service.h"
#include "interface_advertiser_service.h"
#include "SleFeature.h"
#include "SleInterfaceProfileHidHost.h"
#include "SleInterfaceProfileManager.h"
#include "SleServiceManager.h"
#include "SleUtils.h"
#include "interface_profile_ssap_client.h"
#include "log.h"
#include "log_util.h"
#include "nearlink_dft_exception.h"
#include "nearlink_dft_utils.h"
#include "nearlink_dft_manager.h"
#include "nearlink_permission_manager.h"
#include "nearlink_remote_device.h"
#include "nearlink_safe_list.h"
#include "securec.h"
#include "slem.h"
#include "SleInterfaceProfileASC.h"
#include "SleInterfaceProfileDis.h"
#include "SleProfileConnectManager.h"
#include "SleInterfaceProfileTws.h"
#include "DisService.h"
#include "nearlink_sle_datatransfer_service.h"
#include "nearlink_common_event_helper.h"
#include "nearlink_dft_ue.h"
#include "nearlink_safe_hashmap.h"
#include "DialogPairing.h"
#include "parameters.h"
#include "SleDliSnoop.h"
#include "nearlink_dft_device_data.h"
#include "cm_errno.h"
#include "interface_cloud_pair_service.h"
#include "DeviceBatteryManager.h"
#include "SleRemoteDeviceManager.h"
#include "SleCoexistManager.h"
#include "SleCoexistData.h"
#include "SleControllerService.h"
#ifdef NEARLINK_KIA_ENABLE
#include "SleKiaManager.h"
#endif

#include "sdf_addr.h"
#include "nlstk_cfgdb_api.h"
#include "nlstk_sm_api.h"
#include "nlstk_public_define_ext.h"
#include "nlstk_sle_errcode.h"
#include "TwsService.h"
#include "nearlink_dft_exception.h"
#include "nearlink_dft_utils.h"
#include "SleNameChangeManager.h"
#include "SleReconnectManager.h"
#include "nearlink_def.h"
#include "parameter_manager.h"
#include "IServiceManagerPlugin.h"
#include "ServiceManagerPluginLoader.h"

namespace OHOS {
namespace Nearlink {
namespace {
static SleAdapter *g_sleAdapterImpl = nullptr;
constexpr int SLE_FREQMAP_LEN = 10;
const std::string INVALID_NAME = "";
static constexpr int SLE_CONNECTABLE_DURATION_UNLIMITED = 0;
static constexpr int SECONDS_TO_MILLISECONDS = 1000;
static constexpr int SLE_5G_FREQ_TEST_MODE_TYPE = 1;
constexpr int SLE_PASSIVE_PAIRING_DIALOG_TIMEOUT_MS = 1000;  // 1s
constexpr uint16_t DIS_UUID_SSAP_DEVICE_NAME_ID = 0x103F;
constexpr int ENABLE_RETRY_MAX = 3;

static const std::set<int> SLE_DISCONN_REASONS_NEED_BG_CONN = {
    NLSTK_SLE_CONNECTION_TIMEOUT, NLSTK_SLE_CONNECTION_FAILED_TO_BE_ESTABLISHED,
    NLSTK_SLE_CONNECTION_TERMINATED_MIC_FAILURE, NLSTK_SLE_REMOTE_USER_TERMINATED_CONNECTION
};

static const std::set<int> SLE_ACB_PREVIOUS_STATE = {
    static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTING),
    static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED)
};

#ifdef NO_PAIRING_DIALOG
const std::string SLE_NO_PAIR_DIALOG = "true";
const std::string SLE_NO_PAIR_DIALOG_PARAM = "persist.watch_system.nearlink_show_dialog";

bool IsPairingDialogNeeded(void)
{
    std::string param = OHOS::system::GetParameter(SLE_NO_PAIR_DIALOG_PARAM, "");
    if (param.empty()) {
        HILOGI("get watch_system.nearlink_show_dialog param empty");
        return true;
    }
    HILOGI("get watch_system.nearlink_show_dialog param value=%{public}s", param.c_str());
    return (param.compare(SLE_NO_PAIR_DIALOG) == 0) ? false : true;
}
#endif

static constexpr int SLE_AUDIO_PERIPHERAL_DEVICE_NUMBER = 2;
/* pair<对端设备1的状态  对端设备2的状态>  综合后上报给framework的状态 */
std::map<std::pair<int, int>, int> SLE_PROFILE_STATE_CONVERT = {
    {{ static_cast<int>(SleConnectState::CONNECTING), static_cast<int>(SleConnectState::CONNECTING) },
        static_cast<int>(SleConnectState::CONNECTING) },
    {{ static_cast<int>(SleConnectState::CONNECTING), static_cast<int>(SleConnectState::CONNECTED) },
        static_cast<int>(SleConnectState::CONNECTED) },
    {{ static_cast<int>(SleConnectState::CONNECTING), static_cast<int>(SleConnectState::DISCONNECTING) },
        static_cast<int>(SleConnectState::CONNECTING) },
    {{ static_cast<int>(SleConnectState::CONNECTING), static_cast<int>(SleConnectState::DISCONNECTED) },
        static_cast<int>(SleConnectState::CONNECTING) },

    {{ static_cast<int>(SleConnectState::CONNECTED), static_cast<int>(SleConnectState::CONNECTING) },
        static_cast<int>(SleConnectState::CONNECTED) },
    {{ static_cast<int>(SleConnectState::CONNECTED), static_cast<int>(SleConnectState::CONNECTED) },
        static_cast<int>(SleConnectState::CONNECTED) },
    {{ static_cast<int>(SleConnectState::CONNECTED), static_cast<int>(SleConnectState::DISCONNECTING) },
        static_cast<int>(SleConnectState::CONNECTED) },
    {{ static_cast<int>(SleConnectState::CONNECTED), static_cast<int>(SleConnectState::DISCONNECTED) },
        static_cast<int>(SleConnectState::CONNECTED) },

    {{ static_cast<int>(SleConnectState::DISCONNECTING), static_cast<int>(SleConnectState::CONNECTING) },
        static_cast<int>(SleConnectState::CONNECTING) },
    {{ static_cast<int>(SleConnectState::DISCONNECTING), static_cast<int>(SleConnectState::CONNECTED) },
        static_cast<int>(SleConnectState::CONNECTED) },
    {{ static_cast<int>(SleConnectState::DISCONNECTING), static_cast<int>(SleConnectState::DISCONNECTING) },
        static_cast<int>(SleConnectState::DISCONNECTING) },
    {{ static_cast<int>(SleConnectState::DISCONNECTING), static_cast<int>(SleConnectState::DISCONNECTED) },
        static_cast<int>(SleConnectState::DISCONNECTING) },

    {{ static_cast<int>(SleConnectState::DISCONNECTED), static_cast<int>(SleConnectState::CONNECTING) },
        static_cast<int>(SleConnectState::CONNECTING) },
    {{ static_cast<int>(SleConnectState::DISCONNECTED), static_cast<int>(SleConnectState::CONNECTED) },
        static_cast<int>(SleConnectState::CONNECTED) },
    {{ static_cast<int>(SleConnectState::DISCONNECTED), static_cast<int>(SleConnectState::DISCONNECTING) },
        static_cast<int>(SleConnectState::DISCONNECTING) },
    {{ static_cast<int>(SleConnectState::DISCONNECTED), static_cast<int>(SleConnectState::DISCONNECTED) },
        static_cast<int>(SleConnectState::DISCONNECTED) },
};

void PublishDeviceConnectionState(uint32_t acbCount, int connState)
{
    if (SleServiceManager::GetInstance()->GetState(SleTransport::ADAPTER_SLE) != STATE_TURN_ON) {
        return; // 仅星闪开启状态时上报
    }
    if (acbCount == 0 && connState == CM_STATE_DISCONNECTED) {
        NearlinkHelper::NearlinkCommonEventHelper::PublishDeviceConnectionStateEvent(
            static_cast<int>(SleConnectState::DISCONNECTED));
    } else if (acbCount == 1 && connState == CM_STATE_CONNECTED) {
        NearlinkHelper::NearlinkCommonEventHelper::PublishDeviceConnectionStateEvent(
            static_cast<int>(SleConnectState::CONNECTED));
    }
}

}

struct SleAdapter::impl {
    explicit impl(SleAdapter &sleAdapter);
    impl(const impl &);
    impl &operator=(const impl &);
    ~impl();

    class SleConnectableAdvertiserCallback;
    BaseObserverList<ISlePeripheralCallback> slePeripheralCallback_;
    BaseObserverList<ISleConnectionCallback> sleConnectionUpdateCallback_;
    BaseObserverList<ISleDeviceRssiCallback> sleDeviceRssiCallback_;

    class SleNameChangeCallback;
    std::shared_ptr<SleNameChangeCallback> sleNameChangeCallback_{ nullptr };

    std::shared_ptr<SleConnectableAdvertiserCallback> sleConnectableAdvertiserCallback_ =
        std::make_shared<SleConnectableAdvertiserCallback>(this);

    NearlinkSafeList<std::string> credibleDevice_;
    std::atomic_uint32_t acbCount_ = 0;

    SleSecurity sleSecurity_;
    SleProfileConnectManager sleProfileConnectManager_;
    std::shared_ptr<SleCoexist> sleCoexist_ {nullptr};

    bool btmEnableFlag_ = false;
    std::atomic<uint8_t> sleConnectableHandle_ =
        static_cast<uint8_t>(SleAdvertisingHandle::SLE_INVALID_ADVERTISING_HANDLE);
    std::atomic<int> sleDisconnReason_ = static_cast<int>(SleConnectReason::CONNECT_NONE);
    int32_t duration_ {0};
    std::shared_ptr<NearlinkTimer> sleConnectableTimer_ {nullptr};
    NearlinkSafeSet<std::string> needReconnectDevices_;
    NearlinkSafeList<std::string> needReportPairDevices_;
    NearlinkSafeMap<std::string, uint8_t> addrAndFrameTypeMap_;
    // 被动配对场景拉起弹窗
    std::function<void()> passivePairingDialogFunc_ {nullptr};
    std::shared_ptr<NearlinkTimer> slePassivePairingDialogTimer_ {nullptr};
    std::shared_ptr<ServiceSsapConnectInst> passivePairingSsapConnInst_ = nullptr;
};

SleAdapter::impl::impl(SleAdapter &sleAdapter) : sleSecurity_(sleAdapter), sleCoexist_(std::make_shared<SleCoexist>())
{}

SleAdapter::impl::~impl()
{}

class SleAdapter::impl::SleConnectableAdvertiserCallback : public ISleAdvertiserCallback {
public:
    explicit SleConnectableAdvertiserCallback(SleAdapter::impl *pimpl) : pimpl_(pimpl) {};
    ~SleConnectableAdvertiserCallback() override = default;

    void OnStartResultEvent(int result, uint8_t advHandle) override
    {
        HILOGI("Connectable adv result: %{public}d, advHandle: %{public}d", result, advHandle);
        NL_CHECK_RETURN(pimpl_->sleConnectableHandle_.load() == advHandle, "not connectable adv");
        if (result != ADV_RESULT_SUCCESS) {
            HILOGI("Start connectable adv fail");
            pimpl_->sleConnectableHandle_.store(
                static_cast<uint8_t>(SleAdvertisingHandle::SLE_INVALID_ADVERTISING_HANDLE));
        }
    }

    void OnAutoStopAdvEvent(uint8_t advHandle) override
    {
        HILOGI("Connectable adv advHandle: %{public}d", advHandle);
        NL_CHECK_RETURN(pimpl_->sleConnectableHandle_.load() == advHandle, "not connectable adv");
        pimpl_->sleConnectableHandle_.store(static_cast<uint8_t>(SleAdvertisingHandle::SLE_INVALID_ADVERTISING_HANDLE));
    }

    void OnStopResultEvent(int result, uint8_t advHandle) override
    {
        HILOGI("Connectable adv result: %{public}d, advHandle: %{public}d", result, advHandle);
        NL_CHECK_RETURN(pimpl_->sleConnectableHandle_.load() == advHandle, "not connectable adv");
        if (result == static_cast<int>(ADV_RESULT_SUCCESS)) {
            pimpl_->sleConnectableHandle_.store(
                static_cast<uint8_t>(SleAdvertisingHandle::SLE_INVALID_ADVERTISING_HANDLE));
        }
    }

    void OnEnableResultEvent(int result, uint8_t advHandle) override
    {
        HILOGI("Connectable adv result: %{public}d, advHandle: %{public}d", result, advHandle);
        NL_CHECK_RETURN(pimpl_->sleConnectableHandle_.load() == advHandle, "not connectable adv");
        pimpl_->sleConnectableHandle_.store(static_cast<uint8_t>(SleAdvertisingHandle::SLE_INVALID_ADVERTISING_HANDLE));
    }

    void OnDisableResultEvent(int result, uint8_t advHandle) override
    {
        HILOGI("Connectable adv result: %{public}d, advHandle: %{public}d", result, advHandle);
        NL_CHECK_RETURN(pimpl_->sleConnectableHandle_.load() == advHandle, "not connectable adv");
        pimpl_->sleConnectableHandle_.store(static_cast<uint8_t>(SleAdvertisingHandle::SLE_INVALID_ADVERTISING_HANDLE));
    }

    void OnSetAdvDataEvent(int result, uint8_t advHandle) override
    {
        HILOGI("Connectable adv result: %{public}d, advHandle: %{public}d", result, advHandle);
        NL_CHECK_RETURN(pimpl_->sleConnectableHandle_.load() == advHandle, "not connectable adv");
    }

private:
    SleAdapter::impl *pimpl_ = nullptr;
};

class SleAdapter::impl::SleNameChangeCallback : public ISleNameChangeListener {
public:
    explicit SleNameChangeCallback(std::weak_ptr<impl> implPtr) : pimpl_(implPtr) {};
    ~SleNameChangeCallback() override = default;

    void OnLocalNameChanged(const std::string &newLocalName) override
    {
        HILOGI("enter");
        auto ptr = pimpl_.lock();
        NL_CHECK_RETURN(ptr, "SleAdapter impl is destroyed.");
        NL_CHECK_RETURN(ptr->sleConnectableHandle_.load() !=
            static_cast<uint8_t>(SleAdvertisingHandle::SLE_INVALID_ADVERTISING_HANDLE), "Advertising is not started.");
        SleAdvertiserDataImpl advData;
        SleAdvertiserDataImpl scanResponse;
        advData.SetFlags(static_cast<uint8_t>(SleAdvertiserFlag::SLE_ADV_FLAG_GENERAL_DISC));
        std::string serviceData;
        DisService *disService = DisService::GetDisService();
        NL_CHECK_RETURN(disService, "disService is null.");
        disService->GetDisServiceData(serviceData);
        Uuid serviceUuid = Uuid::ConvertFromString(SLE_UUID_DIS);
        advData.AddServiceData(serviceUuid, serviceData);
        InterfaceAdvertiserService::GetInstance().SetAdvertisingData(advData, scanResponse,
            ptr->sleConnectableHandle_.load());
    }

private:
    std::weak_ptr<impl> pimpl_;
};

SleAdapter::SleAdapter()
    : utility::Context(ADAPTER_NAME_SLE, "5.0"),
    pimpl(std::make_shared<SleAdapter::impl>(*this)),
    adapterProperties_(SleRemoteDeviceAdapter::GetInstance())
{
    LOG_DEBUG("[SleAdapter] %{public}s:Create", Name().c_str());
    g_sleAdapterImpl = this;
    pimpl->sleNameChangeCallback_ = std::make_shared<SleAdapter::impl::SleNameChangeCallback>(pimpl);
}

SleAdapter::~SleAdapter() {}

void SleAdapter::Enable()
{
    LOG_DEBUG("[SleAdapter] %{public}s", Name().c_str());
    DoInAdapterThread([this]() -> void {
        this->EnableTask();
    });
}

bool SleAdapter::EnableTask()
{
    LOG_DEBUG("[SleAdapter] enter");
    SleDliSnoop::GetInstance().CreateSnoopFile(true);
    if (RegisterCallbackToNbc() != NLSTK_ERRCODE_SUCCESS) {
        LOG_ERROR("[SleAdapter]:RegisterCallbackToNbc failed!");
    }
    int32_t result = NLSTK_ERRCODE_SUCCESS;
    ServiceManagerPluginLoader::GetInstance()->RegisterCallbackExt(
        RegisterCallbackModule::REGISTER_CALLBACK_MODULE_NBC, result);
    if (result != NLSTK_ERRCODE_SUCCESS) {
        LOG_ERROR("[SleAdapter]:RegisterCallbackToNbcExt failed!");
    }
    bool ret = false;
    for (int i = 1; i <= ENABLE_RETRY_MAX; ++i) {
        ret = (slem_enable() == NLSTK_ERRCODE_SUCCESS);
        LOG_INFO("[SleAdapter] slem_enable ret = %{public}d, retry: %{public}d", ret, i);
        if (ret) {
            break;
        }
        if (i < ENABLE_RETRY_MAX) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    if (!ret) {
        pimpl->btmEnableFlag_ = false;
        LOG_ERROR("[SleAdapter]:cm enable failed!");
    } else {
        pimpl->btmEnableFlag_ = true;
        LoadConfig();
        ret = (InitSlemAndCm() == NLSTK_ERRCODE_SUCCESS);
        InterfaceAdvertiserService::GetInstance().RegisterSleConnectableAdvertiserCallback(
            pimpl->sleConnectableAdvertiserCallback_);
        SleNameChangeManager::GetInstance().RegisterLocalNameObserver(pimpl->sleNameChangeCallback_);
        LOG_DEBUG("[SleAdapter]:cm enable successfully!");
    }
    GetContext()->OnEnable(ADAPTER_NAME_SLE, ret);
    return ret;
}

int SleAdapter::InitSlemAndCm()
{
    HILOGI("enter");
    int ret = RegisterCallbackToCm();
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        LOG_ERROR("[SleAdapter]:RegisterCallbackToCm failed!");
    }
    CM_ReadAcceptFilterListSize();
    RegisterBleSecurityCallback();
    InterfaceCloudPairService::GetInstance().Init();
    std::vector<std::string> pairedAddrList = SleConfig::GetInstance().GetPairedAddrList();
    ReadPeerDeviceInfoFromConf(pairedAddrList);
    CdsmService *cdsmService = CdsmService::GetService();
    if (cdsmService != nullptr) {
        cdsmService->CdsmRecoverFromConf();
    }
    adapterProperties_->SavePeerDevices2Smp();
    InitSleProfileConnectManager();
    SleReconnectManager::GetInstance().SetReconnDeviceParam();
    ParameterManager::NotifyTriggerInitialization();

    NL_CHECK_RETURN_RET(pimpl->sleCoexist_, false, "sleCoexist_ is null");
    pimpl->sleCoexist_->Init();
    return ret;
}

void SleAdapter::InitSleProfileConnectManager()
{
    auto notifyConnectionStateChanged = [this](
        const RawAddress &device, const SleConnectionChangedParam &connChangedParam) {
        NotifyConnectionStateChanged(device, connChangedParam);
    };
    auto disconnectAction = [this](const RawAddress &device, uint8_t reason) {
        DisconnectAction(device, reason);
    };
    auto sendImgSecuConfig = [this](const RawAddress &device) {
        SendImgSecuConfig(device);
    };

    auto onAllProfileDisconnected = [this](const RawAddress &device) {
        OnAllProfileDisconnected(device);
    };

    SleProfileConnectManagerFucs funcs = {
        .notifyConnectionStateChanged = notifyConnectionStateChanged,
        .disconnectAcb = disconnectAction,
        .sendImgSecuConfig = sendImgSecuConfig,
        .onAllProfileDisconnected = onAllProfileDisconnected,
    };
    pimpl->sleProfileConnectManager_.Init(funcs);
}

void SleAdapter::Disable()
{
    LOG_DEBUG("[SleAdapter]:%{public}s", Name().c_str());
    DoInAdapterThread([this]() -> void {
        this->DisableTask();
    });
}

bool SleAdapter::DisableTask()
{
    HILOGI("[SleAdapter] enter");
    if (!pimpl->btmEnableFlag_) {
        GetContext()->OnDisable(ADAPTER_NAME_SLE, pimpl->btmEnableFlag_);
        return false;
    }
    InterfaceCloudPairService::GetInstance().ClearCloudDeviceMap(false);

    adapterProperties_->SavePeerDeviceInfoToConf();
    adapterProperties_->ClearPeerDeviceGroupId();
    ClearPeerDeviceInfo();
    int ret = SleProperties::GetInstance().SetBondableMode(static_cast<int>(BondableMode::BONDABLE_MODE_OFF));
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        LOG_ERROR("[SleAdapter]:SetBondableMode failed!");
    }

    InterfaceAdvertiserService::GetInstance().StopAdvertisingAll();
    InterfaceScanService::GetInstance().StopAllScan();
    InterfaceScanService::GetInstance().ClearScanResultInfo();

    DeregisterAllCallback();

    ret = (slem_disable() == NLSTK_ERRCODE_SUCCESS);
    if (!ret) {
        LOG_ERROR("[SleAdapter]:BTM Disable failed!");
    } else {
        LOG_DEBUG("[SleAdapter]:BTM Disable successfully!");
    }
    SleDliSnoop::GetInstance().SnoopShutDown();
    GetContext()->OnDisable(ADAPTER_NAME_SLE, ret);
    return ret;
}

// SleAdapter开启完成，启动已配对设备的回连操作
void SleAdapter::PostEnable()
{
    LOG_DEBUG("[SleAdapter]:%{public}s", Name().c_str());

    DoInAdapterThread([this]() -> void {
        this->PostEnableTask();
    });
}

bool SleAdapter::PostEnableTask() const
{
    LOG_DEBUG("[SleAdapter] enter");
    SendBgConnList();
    SendDirectConnList();
    return true;
}

void SleAdapter::LoadConfig() const
{
    LOG_DEBUG("[SleAdapter] enter");

    bool ret = SleProperties::GetInstance().LoadSleConfigInfo();
    if (!ret) {
        LOG_ERROR("[SleAdapter]:LoadSleConfigInfo File failed!");
    } else {
        LOG_DEBUG("[SleAdapter]:LoadSleConfigInfo File success!");
    }

    ret &= SleProperties::GetInstance().GetAddrFromController();
    if (!ret) {
        LOG_ERROR("[SleAdapter]:GetAddrFromController File failed!");
        SleProperties::GetInstance().SaveDefaultValues();
    } else {
        LOG_DEBUG("[SleAdapter]:GetAddrFromController File success!");
    }
}

int SleAdapter::DeregisterAllCallback() const
{
    LOG_DEBUG("[SleAdapter] enter");
    SleNameChangeManager::GetInstance().DeregisterLocalNameObserver(pimpl->sleNameChangeCallback_);
    DeregisterBleSecurityCallback();
    return DeregisterCallbackToBtm();
}

void SleAdapter::SetConnFrameType(const std::string &addr, uint8_t frameType)
{
    pimpl->addrAndFrameTypeMap_.Insert(addr, frameType);
}

void SleAdapter::DelConnFrameType(const std::string &addr)
{
    pimpl->addrAndFrameTypeMap_.Erase(addr);
}

bool SleAdapter::GetConnFrameType(const std::string &addr, uint8_t &frameType) const
{
    return pimpl->addrAndFrameTypeMap_.GetValue(addr, frameType);
}

std::string SleAdapter::GetLocalAddress() const
{
    LOG_DEBUG("[SleAdapter] enter");
    return SleProperties::GetInstance().GetLocalAddress();
}

SLE_Addr_S SleAdapter::GetLocalSleAddress() const
{
    LOG_DEBUG("[SleAdapter] enter");
    return SleProperties::GetInstance().GetLocalSleAddress();
}

std::string SleAdapter::GetLocalName() const
{
    LOG_DEBUG("[SleAdapter] enter");
    return SleProperties::GetInstance().GetLocalName();
}

bool SleAdapter::SetLocalName(const std::string &name) const
{
    return SleProperties::GetInstance().SetLocalName(name);
}

std::string SleAdapter::GetDeviceName(const RawAddress &device) const
{
    std::promise<std::string> promise;
    DoInAdapterThread([this, device, &promise]() {
        std::string name = GetDeviceNameTask(device);
        promise.set_value(name);
    });
    std::string result = promise.get_future().get();
    return result;
}

std::string SleAdapter::GetDeviceNameTask(const RawAddress &device) const
{
    return adapterProperties_->GetDeviceName(device);
}

std::vector<Uuid> SleAdapter::GetDeviceUuids(const RawAddress &device) const
{
    std::promise<std::vector<Uuid>> promise;
    DoInAdapterThread([this, device, &promise]() {
        std::vector<Uuid> result = GetDeviceUuidsTask(device);
        promise.set_value(result);
    });
    std::vector<Uuid> uuids = promise.get_future().get();
    return uuids;
}

std::vector<Uuid> SleAdapter::GetDeviceUuidsTask(const RawAddress &device) const
{
    LOG_DEBUG("[SleAdapter] enter");
    return adapterProperties_->GetDeviceUuids(device);
}

std::vector<RawAddress> SleAdapter::GetPairedDevices() const
{
    LOG_DEBUG("[SleAdapter] enter");
    return adapterProperties_->GetPairedDevices();
}

std::vector<RawAddress> SleAdapter::GetConnectedDevices() const
{
    LOG_DEBUG("[SleAdapter] enter");
    return adapterProperties_->GetConnectedDevices();
}

bool SleAdapter::HasConnectedDevice()
{
    return adapterProperties_->HasConnectedDevice();
}

bool SleAdapter::SleFreqHopping(const std::vector<uint8_t> &freq)
{
    LOG_INFO("[SleAdapter] SleFreqHopping enter");
    NL_CHECK_RETURN_RET(freq.size() == CM_CHANNEL_MAP_LEN, false, "[SleAdapter] freq size invalid");
    CM_SetChannelMapReq_S req = {0};
    for (uint8_t i = 0; i < CM_CHANNEL_MAP_LEN; i++) {
        req.channelMap[i] = freq[i];
        LOG_DEBUG("freqMap: 0x%{public}x", req.channelMap[i]);
    }
    uint32_t ret = CM_SetHostChannelClassification(&req);
    NL_CHECK_RETURN_RET(ret == 0, false, "[SleAdapter] fail ret=%u", ret);
    return true;
}

void SleAdapter::SetSleConnectionMode(int32_t connectionMode, int32_t duration)
{
    DoInAdapterThread([this, connectionMode, duration]() -> void {
        SetSleConnectionModeTask(connectionMode, duration);
    });
}

void SleAdapter::SetSleConnectionModeTask(int32_t connectionMode, int32_t duration)
{
    HILOGI("[SleAdapter] connection mode is %{public}d, duration is %{public}d seconds", connectionMode, duration);
    pimpl->duration_ = duration;
    if (connectionMode == static_cast<int>(SLEConnectionMode::CONNECTION_MODE_CONNECTABLE)) {
        SetSleConnectable();
    } else if (connectionMode == static_cast<int>(SLEConnectionMode::CONNECTION_MODE_UNCONNECTABLE)) {
        SetSleUnconnectable();
    } else {
        HILOGE("[SleAdapter] wrong connection mode: %{public}d", connectionMode);
        return;
    }
}

void SleAdapter::SetSleConnectable()
{
    HILOGI("[SleAdapter] enter");
    if (pimpl->sleConnectableHandle_.load() !=
        static_cast<uint8_t>(SleAdvertisingHandle::SLE_INVALID_ADVERTISING_HANDLE)) {
        UpdateSleConnectableTimer();
        return;
    }
    pimpl->sleConnectableHandle_.store(InterfaceAdvertiserService::GetInstance().GetConnectableAdvertiserHandle());
    NL_CHECK_RETURN(
        pimpl->sleConnectableHandle_.load() !=
            static_cast<uint8_t>(SleAdvertisingHandle::SLE_INVALID_ADVERTISING_HANDLE),
            "[SleAdapter] set sle connectable invalid handle");
    UpdateSleConnectableTimer();
    SleAdvertiserSettingsImpl settingsImpl;
    settingsImpl.SetConnectable(true);
    settingsImpl.SetInterval(static_cast<int>(AdvInterval::ADV_SLE_CONNECTABLE_ADV_INTERBAL));
    std::array<uint8_t, Nearlink::RawAddress::SLE_ADDRESS_BYTE_LEN> addr;
    SLE_Addr_S gleAddr = GetLocalSleAddress();
    std::copy(std::begin(gleAddr.addr), std::end(gleAddr.addr), std::begin(addr));
    settingsImpl.SetOwnAddr(addr);
    settingsImpl.SetOwnAddrType(static_cast<int>(SLE_ADDR_TYPE::SLE_PUBLIC_ADDRESS_TYPE));
    SleAdvertiserDataImpl advData;
    SleAdvertiserDataImpl scanResponse;
    // 广播数据构造
    advData.SetFlags(static_cast<uint8_t>(SleAdvertiserFlag::SLE_ADV_FLAG_GENERAL_DISC));
    std::string serviceData;
    DisService *disService = DisService::GetDisService();
    NL_CHECK_RETURN(disService, "disService is null.");
    disService->GetDisServiceData(serviceData);
    Uuid serviceUuid = Uuid::ConvertFromString(SLE_UUID_DIS);
    advData.AddServiceData(serviceUuid, serviceData);
    InterfaceAdvertiserService::GetInstance().StartAdvertising(
        settingsImpl, advData, scanResponse, pimpl->sleConnectableHandle_.load());
}

void SleAdapter::SetSleUnconnectable()
{
    HILOGI("[SleAdapter] enter");
    NL_CHECK_RETURN(
        pimpl->sleConnectableHandle_.load() !=
            static_cast<uint8_t>(SleAdvertisingHandle::SLE_INVALID_ADVERTISING_HANDLE),
            "[SleAdapter] already unconnectable");
    ClearSleConnectableTimer();
    HILOGI("sleConnectableHandle=%{public}d", pimpl->sleConnectableHandle_.load());
    InterfaceAdvertiserService::GetInstance().StopAdvertising(pimpl->sleConnectableHandle_.load());
}

void SleAdapter::UpdateSleConnectableTimer()
{
    HILOGI("[SleAdapter] enter");
    ClearSleConnectableTimer();
    if (pimpl->duration_  == SLE_CONNECTABLE_DURATION_UNLIMITED) {
        return;
    }
    auto timeoutFunc = [this]() -> void {
        HILOGI("[SleAdapter] sle connectable timeout");
        SetSleUnconnectable();
    };
    pimpl->sleConnectableTimer_ = std::make_shared<NearlinkTimer>(timeoutFunc);
    int32_t time = pimpl->duration_ * SECONDS_TO_MILLISECONDS;
    pimpl->sleConnectableTimer_->Start(time, false);
}

void SleAdapter::ClearSleConnectableTimer()
{
    if (pimpl->sleConnectableTimer_ != nullptr) {
        pimpl->sleConnectableTimer_->Stop();
        pimpl->sleConnectableTimer_ = nullptr;
    }
}

/* 私有Audio：清理旧合作集信息 */
bool SleAdapter::ProcClearOldCdsmGroup(const RawAddress &reportAddr, const RawAddress &collabAddr,
    bool eraseDeviceIfNeed) const
{
    HILOGI("[SleAdapter]reportAddr: %{public}s, collabAddr: %{public}s",
        GetEncryptAddr(reportAddr.GetAddress()).c_str(), GetEncryptAddr(collabAddr.GetAddress()).c_str());
    CdsmService *cdsmService = CdsmService::GetService();
    NL_CHECK_RETURN_RET(cdsmService, false, "[SleAdapter]cdsmService is null.");
    RawAddress otherDev;
    bool isReplace = false;
    /*
        1、左耳(非report)丢失，右耳与新左耳重新耦合后与手机配对，手机直接删除旧左耳配对信息
        2、右耳(report)丢失，左耳与新右耳重新耦合后与手机配对，手机直接删除旧的右耳和左耳配对信息
        一个耳机是原来已有的，另一个耳机之前不是一对，则删除对应合作集
    */
    bool isRealExist = cdsmService->CdsmCheckIsCooperationDevice(reportAddr);
    if (isRealExist && eraseDeviceIfNeed) {
        if (cdsmService->CdsmGetOtherAddr(reportAddr, otherDev) && otherDev != collabAddr) {
            cdsmService->CdsmDeleteGroup(reportAddr);
            CancelPairingTask(otherDev);
            adapterProperties_->RemovePeripheralDevice(reportAddr.GetAddress());
            HILOGI("[SleAdapter]:Existing reportAddr:%{public}s, Lost dev:%{public}s",
                GET_ENCRYPT_ADDR(reportAddr), GET_ENCRYPT_ADDR(otherDev));
        }
    }

    bool isCooperaExist = cdsmService->CdsmCheckIsCooperationDevice(collabAddr);
    if (isCooperaExist && eraseDeviceIfNeed) {
        if (cdsmService->CdsmGetOtherAddr(collabAddr, otherDev) && otherDev != reportAddr) {
            cdsmService->CdsmDeleteGroup(collabAddr);
            CancelPairingTask(otherDev);
            adapterProperties_->RemovePeripheralDevice(collabAddr.GetAddress());
            HILOGI("[SleAdapter]:Existing collabAddr:%{public}s, Lost dev:%{public}s",
                GET_ENCRYPT_ADDR(collabAddr), GET_ENCRYPT_ADDR(otherDev));
        }
    }

    /*
        3、耳夹式耳机(不区分左右耳), 双耳交换后重新恢厂, report改变需删除旧合作集
        eraseDeviceIfNeed - true : 通过靠近发现弹框重配对，删除原有peerList缓存，走重新配对流程
                            false: 通过已配对列表回连重配对，保留原有peerList缓存，走回连流程
    */
    RawAddress oldReportAddr;
    if (cdsmService->CdsmGetReportAddr(reportAddr, oldReportAddr) && reportAddr != oldReportAddr) {
        isReplace = true;
        HILOGI("[SleAdapter]:Existing collabAddr:%{public}s, exchang reportAddr:%{public}s",
            GET_ENCRYPT_ADDR(collabAddr), GET_ENCRYPT_ADDR(reportAddr));
        adapterProperties_->SetPairStatus(reportAddr, static_cast<int>(SlePairState::SLE_PAIR_NONE));
        adapterProperties_->SetPairStatus(collabAddr, static_cast<int>(SlePairState::SLE_PAIR_NONE));

        NotifyPairStatusChanged(oldReportAddr, static_cast<int>(SlePairState::SLE_PAIR_CANCELING),
            static_cast<int>(SlePairState::SLE_PAIR_NONE),
            static_cast<uint8_t>(PairingStateChangeReason::PAIRING_SUCCESS));
        cdsmService->CdsmReplaceOldReportAddr(oldReportAddr, reportAddr);
        if (eraseDeviceIfNeed) {
            adapterProperties_->RemovePeripheralDevice(reportAddr.GetAddress());
            adapterProperties_->RemovePeripheralDevice(collabAddr.GetAddress());
        }
    }
    return isReplace;
}

bool SleAdapter::ProcClearCommonEarphoneOldCdsmGroup(const RawAddress &newReportAddr, const RawAddress &oldReportAddr,
    bool eraseDeviceIfNeed) const
{
    HILOGI("[SleAdapter][Common]newReportAddr: %{public}s, oldReportAddr: %{public}s",
        GetEncryptAddr(newReportAddr.GetAddress()).c_str(), GetEncryptAddr(oldReportAddr.GetAddress()).c_str());
    CdsmService *cdsmService = CdsmService::GetService();
    NL_CHECK_RETURN_RET(cdsmService, false, "[SleAdapter]cdsmService is null..");
    bool isReplace = false;
    /*
        3、耳夹式耳机(不区分左右耳), 双耳交换后重新配对（可能恢厂，或者不恢厂）, report改变需删除旧合作集
        eraseDeviceIfNeed - true : 通过靠近发现弹框重配对，删除原有peerList缓存，走重新配对流程
                            false: 通过已配对列表回连重配对，保留原有peerList缓存，走回连流程
    */
    if (newReportAddr != oldReportAddr) {
        isReplace = true;
        HILOGI("[SleAdapter][Common]Case3: Existing oldReportAddr:%{public}s, exchange newReportAddr:%{public}s",
            GET_ENCRYPT_ADDR(oldReportAddr), GET_ENCRYPT_ADDR(newReportAddr));
        // 这里需要先将双耳的配对状态设置为NONE，以方便下面NotifyPairStatusChanged可以成功通知外部观察者(有一只是配对状态就不会真正通知)
        adapterProperties_->SetPairStatus(newReportAddr, static_cast<int>(SlePairState::SLE_PAIR_NONE));
        adapterProperties_->SetPairStatus(oldReportAddr, static_cast<int>(SlePairState::SLE_PAIR_NONE));

        NotifyPairStatusChanged(oldReportAddr, static_cast<int>(SlePairState::SLE_PAIR_CANCELING),
            static_cast<int>(SlePairState::SLE_PAIR_NONE),
            static_cast<uint8_t>(PairingStateChangeReason::PAIRING_SUCCESS));
        cdsmService->CdsmReplaceOldReportAddr(oldReportAddr, newReportAddr);
        if (eraseDeviceIfNeed) {
            HILOGI("[SleAdapter][Common]Erase device reportAddr:%{public}s", GET_ENCRYPT_ADDR(newReportAddr));
            // 由于三方耳机的配对流程约束且双耳没有共LinkKey，因此这里只删除reportAddr
            // 如果和私有耳机一样，这里也删了另外一个设备的缓存，由于三方耳机没有共LinkKey，只有在ACB连上的时候才会写缓存，
            // 那么假如此时取消配对，旧的report地址会因缓存中找不到设备耳取消配对失败，新report地址因为后续的连接流程，则可以取消成功
            // 如果之后再重新开关星闪，config文件中，还留存有一个旧report地址的信息，且此时合作集的report地址已经被更新为了新report地址
            // 就会出现已配对列表中，新report地址显示为地址而且设备名的情况，且此时将无法取消配对。
            adapterProperties_->RemovePeripheralDevice(newReportAddr.GetAddress());
        }
    }
    return isReplace;
}

void SleAdapter::ProcCreateCdsmGroupAndEraseDevice(const RawAddress &reportAddr, const RawAddress &otherAddr) const
{
    pimpl->needReportPairDevices_.Insert(reportAddr.GetAddress());
    adapterProperties_->SetPrePairStatus(reportAddr, static_cast<int>(SlePairState::SLE_PAIR_PAIRED));
    adapterProperties_->SetPairStatus(reportAddr, static_cast<int>(SlePairState::SLE_PAIR_PAIRING));
    adapterProperties_->SetPrePairStatus(otherAddr, static_cast<int>(SlePairState::SLE_PAIR_PAIRED));
    adapterProperties_->SetPairStatus(otherAddr, static_cast<int>(SlePairState::SLE_PAIR_PAIRING));
    NotifyPairStatusChanged(reportAddr, static_cast<int>(SlePairState::SLE_PAIR_NONE),
        static_cast<int>(SlePairState::SLE_PAIR_PAIRING),
        static_cast<uint8_t>(PairingStateChangeReason::PAIRING_SUCCESS));
    HILOGI("[SleAdapter]:Replace old cdsm, need report new device %{public}s online", GET_ENCRYPT_ADDR(reportAddr));
}

/* report地址和主耳地址 */
void SleAdapter::ProcCreateCdsmGroup(const RawAddress &reportAddr, const RawAddress &realAddr,
    bool eraseDeviceIfNeed) const
{
    if (InterfaceCloudPairService::GetInstance().IsCloudDeviceCreatePair(reportAddr)) {
        return;
    }
    /* 创建合作集 */
    int businessType = InterfaceScanService::GetInstance().GetManufacturerBusinessType(reportAddr.GetAddress());
    CdsmService *cdsmService = CdsmService::GetService();
    NL_CHECK_RETURN(cdsmService, "[SleAdapter]cdsmService is null.");
    if (businessType == Nearlink::SLE_PRIVATE_AUDIO_BUSINESS_TYPE) {
        RawAddress collabAddr = InterfaceScanService::GetInstance().GetCollaborateAddress(reportAddr);
        RawAddress otherAddr = collabAddr;
        if (collabAddr == reportAddr) {
            otherAddr = realAddr;
        }
        bool isReplace = ProcClearOldCdsmGroup(reportAddr, otherAddr, eraseDeviceIfNeed);
        if (!isReplace) {
            std::vector<RawAddress> devList;
            devList.push_back(realAddr);
            devList.push_back(collabAddr);
            cdsmService->CdsmCreateGroup(reportAddr, devList, true);
        } else if (!eraseDeviceIfNeed) {
            ProcCreateCdsmGroupAndEraseDevice(reportAddr, otherAddr);
        }
        /* 私有设备上报 */
        DftCacheCdsmInfo(realAddr.GetAddress(), reportAddr.GetAddress(), collabAddr.GetAddress());
    } else {
        // 1. 没有配对记录 --- 只有私有耳机才能在没有配对以前，就获取到双耳的信息，用来提前创建合作集；公版这里不需要处理
        bool isExistCdsm = cdsmService->CdsmCheckIsCooperationDevice(reportAddr);
        NL_CHECK_RETURN(isExistCdsm, "[SleAdapter]%{public}s not private device, no exist group info, "
            "not need to pre create cdsm group.", GET_ENCRYPT_ADDR(reportAddr));
        // 2. 有合作集，只是左右互换后恢厂或者重新起配对
        // 2.1 设置上点击原有配对记录连接：
        //          ConnectAllowsProfile(旧report) --->
        //          KeyMissing(如果双耳都在盒内，假设默认右为report，则此时新report地址在发广播，会先连接新report地址，后触发右耳KeyMissing;
        //                     如果单耳在盒关盒，恰好左耳在盒外，那此时左耳会发广播，会先连接左耳，即旧的report地址，后触发左耳KeyMissing;
        //                     注意：拉起配对弹窗时，无论KeyMissing的设备是哪个，都会根据当前合作集的缓存，统一转化为report地址) --->
        //          Dialog --->
        //          SetPairConfirmation(这个函数的入口，会通过GetRealAddress，找到当前合作集中ACB连接的那个设备，优先级：入参地址--->另外一个地址)
        // case1: 左右互换后恢厂，且双耳均在盒内，即新report(右耳)在发广播 ----> 同2.2.2(可解决)
        // 2.2 点击新扫描到的设备：
        //          StartPair(新report) ---> KeyMissing(新report) ---> Dialog(新report) ---> SetPairConfirmation(新report)
        bool isOldReport = cdsmService->CdsmCheckIsCooperationReport(reportAddr);
        if (!isOldReport) { // 入参中新report地址，在此刻的缓存中，还不是合作集的report地址，说明report地址发生了变换
            RawAddress oldReportAddr;
            cdsmService->CdsmGetOtherAddr(reportAddr, oldReportAddr);
            ProcClearCommonEarphoneOldCdsmGroup(reportAddr, oldReportAddr, eraseDeviceIfNeed);
            if (!eraseDeviceIfNeed) {
                ProcCreateCdsmGroupAndEraseDevice(reportAddr, oldReportAddr);
            }
        }
    }
}

bool SleAdapter::StartPair(const RawAddress &device)
{
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    DoInAdapterThread([this, device, &promise]() {
        bool ret = this->StartPairTask(device);
        promise.set_value(ret);
    });
    return future.get();
}

bool SleAdapter::StartPairTask(const RawAddress &device)
{
    RawAddress realAddr = InterfaceCloudPairService::GetInstance().GetCloudDeviceRealAddress(device);
    if (realAddr.GetAddress().empty()) {
        realAddr = InterfaceScanService::GetInstance().GetCurrentAddress(device);
    }
    ProcCreateCdsmGroup(device, realAddr, true);
    LOG_INFO("[SleAdapter] addr: %{public}s", GetEncryptAddr(realAddr.GetAddress()).c_str());

    std::shared_ptr<const SlePeripheralDevice> remoteDevice = adapterProperties_->GetRemoteDevice(device);
    if (remoteDevice) {
        NL_CHECK_RETURN_RET(remoteDevice->GetAcbConnectState() !=
            static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTING), true, "[SleAdapter] device is connecting");
        NL_CHECK_RETURN_RET(remoteDevice->GetPairedStatus() != static_cast<int>(SlePairState::SLE_PAIR_PAIRING),
            true, "[SleAdapter] failed pairStatus: %{public}d", remoteDevice->GetPairedStatus());
    }

    int32_t curCloudPairState = NL_CLOUD_PAIR_STATE::CLOUD_PAIR_INVALID;
    if (InterfaceCloudPairService::GetInstance().GetCloudPairState(realAddr.GetAddress(), curCloudPairState) &&
        curCloudPairState == NL_CLOUD_PAIR_STATE::CLOUD_PAIR_CREATE_PAIR) {
        DftCachePairConnType(realAddr.GetAddress(), CLOUD_PAIR, NO_CONN);
    } else {
        DftCachePairConnType(realAddr.GetAddress(), FIRST_TIME_CONN, NO_CONN);
    }
    if (IsAcbConnectedTask(realAddr)) {
        uint8_t peerAddrType = adapterProperties_->GetPeerDeviceAddrType(realAddr);
        LOG_INFO("acb connected");
        bool ret = pimpl->sleSecurity_.StartPair(realAddr, peerAddrType);
        NL_CHECK_RETURN_RET(ret, false, "[SleAdapter]:failed");
        PairingStatus(realAddr);
        return true;
    }
    PairingStatus(realAddr);
    DoInAdapterThread([this, realAddr]() -> void {
        this->ConnectAcb(realAddr);
    });
    return true;
}

bool SleAdapter::StartCrediblePair(const RawAddress &device)
{
    HILOGI("[SleAdapter] enter");
    InterfaceCloudPairService::GetInstance().SetCrediblePairState(device);
    if (StartPair(device)) {
        RawAddress reportAddr(device);
        CdsmService *cdsmService = CdsmService::GetService();
        if (cdsmService != nullptr) {
            cdsmService->CdsmGetReportAddr(device, reportAddr);
        }
        pimpl->credibleDevice_.Insert(reportAddr.GetAddress());
        return true;
    }
    return false;
}

/* 合作集成员设备取消配对，并发取消 */
void SleAdapter::CdsmCancelPairingProcess(const RawAddress &device) const
{
    std::vector<NearlinkCdsmInfo> cdsmInfo;
    CdsmService *cdsmService = CdsmService::GetService();
    NL_CHECK_RETURN(cdsmService != nullptr && cdsmService->CdsmGetAllMemberInfo(device, cdsmInfo) == NL_NO_ERROR,
        "[cdsm adapter]:cancel pair process,get cdsm info by member addr failed!");

    for (const auto &member : cdsmInfo) {
        int pairState = adapterProperties_->GetPairStatus(member.addr_);
        HILOGI("[cdsm adapter]:cdsm member,cancel pair,addr:%{public}s,pair state:%{public}d",
            GetEncryptAddr(member.addr_.GetAddress()).c_str(), pairState);
        // 合作集成员设备在加密后连接前，需要特殊处理去删除配对记录
        bool isMemberNeedCancel = (pairState == static_cast<int>(SlePairState::SLE_PAIR_NONE) &&
            cdsmService->CdsmCheckIsCooperationMember(member.addr_));
        if (!isMemberNeedCancel && (pairState != static_cast<int>(SlePairState::SLE_PAIR_PAIRED))) {
            continue;
        }

        uint8_t addrType = adapterProperties_->GetPeerDeviceAddrType(member.addr_);
        pimpl->sleSecurity_.CancelPairing(member.addr_, addrType);
        DftCacheDisconnInfoMsg(member.addr_.GetAddress(), "", DISCONN_CLICK_UNPAIR);

        adapterProperties_->SetPairStatus(member.addr_, static_cast<int>(SlePairState::SLE_PAIR_CANCELING));
        adapterProperties_->SetCdsmAddrType(member.addr_, static_cast<int>(SleCdsmAddrType::CDSM_TYPE_NONE));
    }
}

bool SleAdapter::CancelPairing(const RawAddress &device)
{
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    DoInAdapterThread([this, device, &promise]() {
        bool ret = this->CancelPairingTask(device);
        promise.set_value(ret);
    });
    return future.get();
}

bool SleAdapter::CancelPairingTask(const RawAddress &device) const
{
    RawAddress realAddr = adapterProperties_->GetRealAddress(device);
    HILOGI("cancel pairing,addr:%{public}s,real addr:%{public}s", GET_ENCRYPT_ADDR(device),
        GET_ENCRYPT_ADDR(realAddr));
    if (InterfaceCloudPairService::GetInstance().CancelCloudPairing(realAddr)) {
        RemoveNotPairedCloudDevice(realAddr);
        return true;
    }
    HILOGI("cancel pairing,addr:%{public}s,real addr:%{public}s", GET_ENCRYPT_ADDR(device),
        GET_ENCRYPT_ADDR(realAddr));
    std::shared_ptr<const SlePeripheralDevice> remoteDevice = adapterProperties_->GetRemoteDevice(realAddr);
    if (remoteDevice) {
        int pairState = remoteDevice->GetPairedStatus();
        if ((static_cast<int>(SlePairState::SLE_PAIR_CANCELING) == pairState) ||
            (static_cast<int>(SlePairState::SLE_PAIR_NONE) == pairState)) {
            HILOGI("CancelPairing failed, because of SLE_PAIR_NONE or PAIR_CANCELING! %{public}d",
                pairState);
            return false;
        }
        pimpl->needReconnectDevices_.Erase(realAddr.GetAddress());

        ProfileHidHost *hidService = static_cast<ProfileHidHost *>(
            SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_HID_HOST));
        if (hidService != nullptr) {
            int deviceState = static_cast<int32_t>(hidService->GetDeviceState(realAddr));
            DftCacheUnPairInfo(realAddr.GetAddress(), GetDeviceAppearance(realAddr),
                NearLinkPermissionManager::GetCallingName(), deviceState, pairState);
        }

        uint8_t peerAddrType = adapterProperties_->GetPeerDeviceAddrType(realAddr);
        if (pimpl->sleSecurity_.CancelPairing(realAddr, peerAddrType)) {
            DftCacheDisconnInfoMsg(realAddr.GetAddress(), "", DISCONN_CLICK_UNPAIR);
            adapterProperties_->SetPrePairStatus(realAddr, pairState);
            adapterProperties_->SetPairStatus(realAddr, static_cast<int>(SlePairState::SLE_PAIR_CANCELING));
        } else {
            HILOGE("[SleAdapter]CancelPairing failed, because of smp cancel pair failed!");
            return false;
        }
        CdsmCancelPairingProcess(realAddr);
    } else {
        HILOGE("[SleAdapter]CancelPairing failed, because of not find the remote device!");
        return false;
    }
    return true;
}

void SleAdapter::RemoveNotPairedCloudDevice(const RawAddress &device) const
{
    CdsmService *cdsmService = CdsmService::GetService();
    std::vector<NearlinkCdsmInfo> cdsmInfo = {};
    if (cdsmService != nullptr && cdsmService->CdsmGetAllMemberInfo(device, cdsmInfo) == NL_NO_ERROR) {
        for (auto &member : cdsmInfo) {
            CancelPairCompleteInner(member.addr_);
            if (cdsmService->CdsmCheckIsCooperationReport(member.addr_)) {
                pimpl->credibleDevice_.Erase(member.addr_.GetAddress());
            }
        }
        NotifyPairStatusChanged(device, static_cast<int32_t>(SlePairState::SLE_PAIR_PAIRED),
            static_cast<int32_t>(SlePairState::SLE_PAIR_NONE),
            static_cast<uint8_t>(PairingStateChangeReason::PAIRING_LOCAL_CANCELED));
    }
}

bool SleAdapter::RemovePair(const RawAddress &device)
{
    DoInAdapterThread([this, device]() {
        bool result = false;
        if (!ProcCdsmDisconnectAllProfile(device, result,
            static_cast<uint8_t>(SleDiscReason::SLE_DISC_REASON_CANCEL_PAIR))) {
            DisconnectAllProfileInner(device, result,
                static_cast<uint8_t>(SleDiscReason::SLE_DISC_REASON_CANCEL_PAIR));
        }
    });
    return CancelPairing(device);
}

bool SleAdapter::Disconnect(const RawAddress &addr)
{
    LOG_DEBUG("[SleAdapter] enter");
    DoInAdapterThread([this, addr]() -> void {
        this->DisconnectAction(addr, static_cast<uint8_t>(SleDiscReason::SLE_DISC_REASON_REMOTE_USER_TERMINATED));
    });
    return true;
}

bool SleAdapter::RemoveAllPairs()
{
    std::promise<bool> promise;
    DoInAdapterThread([this, &promise]() {
        bool result = RemoveAllPairsTask();
        promise.set_value(result);
    });
    bool ret = promise.get_future().get();
    return ret;
}

bool SleAdapter::RemoveAllPairsTask()
{
    LOG_INFO("[SleAdapter] enter");

    std::vector<RawAddress> removeDevices;
    pimpl->credibleDevice_.Clear();
    pimpl->needReconnectDevices_.Clear();
    adapterProperties_->RemoveAllPairsProcess(removeDevices);
    if (!removeDevices.empty()) {
        SleConfig::GetInstance().Save();
        SleServiceManager::GetInstance()->OnPairDevicesRemoved(SleTransport::ADAPTER_SLE, removeDevices);
    }
    return true;
}

bool SleAdapter::SetPairingPassCode(const RawAddress &device, const std::string &passCode)
{
    HILOG_COMM_INFO("[%{public}s:%{public}d][SleAdapter] addr: %{public}s",
        __FUNCTION__, __LINE__, GET_ENCRYPT_ADDR(device));
    NL_CHECK_RETURN_RET(IsAcbConnected(device), false, "[SleAdapter] device is not connected");
    uint8_t addrType = adapterProperties_->GetPeerDeviceAddrType(device);
    bool ret = pimpl->sleSecurity_.SetPairingPassCode(device, passCode, addrType);
    NL_CHECK_RETURN_RET(ret, false, "[SleAdapter] set passCode failed");
    NearlinkDftUe::GetInstance().WriteCommonUe(DFT_PAIRING_CFM_UE, device, SETPAIRPASSCODE, CALLSUCCESS,
                                               NearLinkPermissionManager::GetCallingName());
    return true;
}

bool SleAdapter::SetPairingConfirmation(const RawAddress &device) const
{
    RawAddress realAddr = adapterProperties_->GetRealAddress(device);
    if (adapterProperties_->IsAudioDevice(device.GetAddress())) {
        RawAddress reportAddr = InterfaceScanService::GetInstance().GetReportAddrByCurrentAddress(realAddr);
        ProcCreateCdsmGroup(reportAddr, realAddr, false);
    }
    HILOG_COMM_INFO("[%{public}s:%{public}d][SleAdapter] addr: %{public}s",
        __FUNCTION__, __LINE__, GET_ENCRYPT_ADDR(realAddr));
    NL_CHECK_RETURN_RET(IsAcbConnected(realAddr), false, "[SleAdapter] device is not connected");
    uint8_t addrType = adapterProperties_->GetPeerDeviceAddrType(realAddr);
    bool ret = pimpl->sleSecurity_.SetPairingConfirmation(realAddr, addrType);
    NL_CHECK_RETURN_RET(ret, false, "[SleAdapter] set cfm failed");
    NearlinkDftUe::GetInstance().WriteCommonUe(DFT_PAIRING_CFM_UE, realAddr, SETPAIRCONFIRM, CALLSUCCESS,
                                               NearLinkPermissionManager::GetCallingName());
    return true;
}

bool SleAdapter::IsBondedFromLocal(const RawAddress &device) const
{
    std::promise<bool> promise;
    DoInAdapterThread([this, device, &promise]() {
        bool result = adapterProperties_->IsBondedFromLocal(device);
        promise.set_value(result);
    });
    bool ret = promise.get_future().get();
    return ret;
}

bool SleAdapter::PairRequestReply(const RawAddress &device, bool accept) const
{
    std::promise<bool> promise;
    DoInAdapterThread([this, device, accept, &promise]() {
        bool result = PairRequestReplyTask(device, accept);
        promise.set_value(result);
    });
    bool ret = promise.get_future().get();
    return ret;
}

bool SleAdapter::PairRequestReplyTask(const RawAddress &device, bool accept) const
{
    HILOGI("[SleAdapter]:%{public}d", accept);
    RawAddress realAddr = adapterProperties_->GetRealAddress(device);
    uint8_t addrType = adapterProperties_->GetPeerDeviceAddrType(realAddr);
    return pimpl->sleSecurity_.PairRequestReply(realAddr, addrType, accept);
}

bool SleAdapter::IsAcbConnected(const RawAddress &device) const
{
    std::promise<bool> promise;
    DoInAdapterThread([this, device, &promise]() {
        bool result = IsAcbConnectedTask(device);
        promise.set_value(result);
    });
    bool ret = promise.get_future().get();
    return ret;
}

bool SleAdapter::IsAcbConnectedTask(const RawAddress &device) const
{
    LOG_DEBUG("[SleAdapter] enter");
    return adapterProperties_->IsAcbConnected(device);
}

bool SleAdapter::IsAcbEncrypted(const RawAddress &device) const
{
    std::promise<bool> promise;
    DoInAdapterThread([this, device, &promise]() {
        bool result = IsAcbEncryptedTask(device);
        promise.set_value(result);
    });
    bool ret = promise.get_future().get();
    return ret;
}

bool SleAdapter::IsAcbEncryptedTask(const RawAddress &device) const
{
    LOG_DEBUG("[SleAdapter] enter");
    return adapterProperties_->IsAcbEncrypted(device);
}

uint8_t SleAdapter::GetLinkRole(const RawAddress &device) const
{
    std::promise<uint8_t> promise;
    DoInAdapterThread([this, device, &promise]() {
        uint8_t result = GetLinkRoleTask(device);
        promise.set_value(result);
    });
    uint8_t role = promise.get_future().get();
    return role;
}

uint8_t SleAdapter::GetLinkRoleTask(const RawAddress &device) const
{
    LOG_DEBUG("[SleAdapter] enter");
    RawAddress realAddr = adapterProperties_->GetRealAddress(device);
    uint8_t role = adapterProperties_->GetLinkRole(realAddr);
    HILOGI("local role addr: %{public}s, role:%{public}u", GetEncryptAddr(device.GetAddress()).c_str(), role);
    return role;
}

utility::Context *SleAdapter::GetContext()
{
    return this;
}

int SleAdapter::GetPairState(const RawAddress &device) const
{
    if (InterfaceCloudPairService::GetInstance().ChkCloudDeviceAndPermission(device)) {
        return static_cast<int>(SlePairState::SLE_PAIR_PAIRED);
    }
    return adapterProperties_->GetPairState(device);
}

int SleAdapter::GetAcbState(const RawAddress &device) const
{
    std::promise<int> promise;
    DoInAdapterThread([this, device, &promise]() {
        int state = GetAcbStateTask(device);
        promise.set_value(state);
    });
    int acbState = promise.get_future().get();
    return acbState;
}

int SleAdapter::GetAcbStateTask(const RawAddress &device) const
{
    return adapterProperties_->GetAcbState(device.GetAddress());
}

bool SleAdapter::SetAcbDisConnReasonTask(const std::string &address, int reason) const
{
    return SleRemoteDeviceManager::GetInstance()->SetAcbDisConnReason(address, reason);
}

int SleAdapter::GetAcbDisConnReasonTask(const std::string &address) const
{
    return SleRemoteDeviceManager::GetInstance()->GetAcbDisConnReason(address);
}

uint32_t SleAdapter::GetAcbCount() const
{
    return pimpl->acbCount_.load();
}

int SleAdapter::GetBondableMode() const
{
    LOG_DEBUG("[SleAdapter] enter");
    return SleProperties::GetInstance().GetBondableMode();
}

bool SleAdapter::SetBondableMode(int mode) const
{
    LOG_DEBUG("[SleAdapter]:%{public}d", mode);
    return (SleProperties::GetInstance().SetBondableMode(mode) == NLSTK_ERRCODE_SUCCESS);
}

uint32_t SleAdapter::GetSleMaxAdvertisingDataLength() const
{
    LOG_DEBUG("[SleAdapter] enter");
    return SleFeature::GetInstance().GetBleMaximumAdvertisingDataLength();
}

int SleAdapter::GetIoCapability() const
{
    LOG_DEBUG("[SleAdapter] enter");
    return SleProperties::GetInstance().GetIoCapability();
}

bool SleAdapter::SetIoCapability(int ioCapability) const
{
    LOG_DEBUG("[SleAdapter] enter");
    return SleProperties::GetInstance().SetIoCapability(ioCapability);
}

bool SleAdapter::IsSleEnabled() const
{
    LOG_DEBUG("[SleAdapter] enter");
    int status = SleServiceManager::GetInstance()->GetState(SleTransport::ADAPTER_SLE);
    return (status == SleStateID::STATE_TURN_ON);
}

void SleAdapter::RegisterSlePeripheralCallback(ISlePeripheralCallback &callback) const
{
    LOG_DEBUG("[SleAdapter] enter");
    pimpl->slePeripheralCallback_.Register(callback);
}

void SleAdapter::DeregisterSlePeripheralCallback(ISlePeripheralCallback &callback) const
{
    LOG_DEBUG("[SleAdapter] enter");
    pimpl->slePeripheralCallback_.Deregister(callback);
}

void SleAdapter::RegisterSleDeviceRssiCallback(ISleDeviceRssiCallback &callback) const
{
    pimpl->sleDeviceRssiCallback_.Register(callback);
}

void SleAdapter::DeregisterSleDeviceRssiCallback(ISleDeviceRssiCallback &callback) const
{
    pimpl->sleDeviceRssiCallback_.Deregister(callback);
}

void SleAdapter::RegisterSleConnectionCallback(ISleConnectionCallback &callback) const
{
    HILOGD("[SleAdapter] enter");
    pimpl->sleConnectionUpdateCallback_.Register(callback);
}

void SleAdapter::DeregisterSleConnectionCallback(ISleConnectionCallback &callback) const
{
    HILOGD("[SleAdapter] enter");
    pimpl->sleConnectionUpdateCallback_.Deregister(callback);
}

void SleAdapter::RegisterBleSecurityCallback()
{
    LOG_DEBUG("[SleAdapter] enter");
    pimpl->sleSecurity_.RegisterCallbackToSm();
    pimpl->sleSecurity_.RegisterCryptoFunctionsToSm();
}

void SleAdapter::DeregisterBleSecurityCallback() const
{
    LOG_DEBUG("[SleAdapter] enter");
    pimpl->sleSecurity_.DeregisterCallbackToGap();
}

bool SleAdapter::RegisterSleAdapterObserver(IAdapterSleObserver &observer) const
{
    LOG_DEBUG("[SleAdapter] enter");
    SleProperties::GetInstance().RegisterSleAdapterObserver(observer);
    return true;
}

bool SleAdapter::DeregisterSleAdapterObserver(IAdapterSleObserver &observer) const
{
    LOG_DEBUG("[SleAdapter] enter");
    SleProperties::GetInstance().DeregisterSleAdapterObserver(observer);
    return true;
}

void SleAdapter::SetSlePeripheralDeviceBasicInfo(const std::string &addr,
                                                 std::shared_ptr<SlePeripheralDevice> &value) const
{
    RawAddress rawAddr(addr);
    value->SetAddress(rawAddr);
    uint8_t type = SleConfig::GetInstance().GetPeerAddressType(addr);
    value->SetAddressType(type);

    std::string name = SleConfig::GetInstance().GetPeerName(addr);
    value->SetName(name);

    int appearance = SleConfig::GetInstance().GetPeerAppearance(addr);
    value->SetAppearance(appearance);
    DftCachePeerInfo(addr, name, appearance);
    DftDeviceManager::GetInstance().AddDevice(rawAddr, appearance, name);

    std::string aliasName = SleConfig::GetInstance().GetPeerAlias(addr);
    value->SetAliasName(aliasName);

    int io = SleConfig::GetInstance().GetPeerDeviceIoCapability(addr);
    value->SetIoCapability(io);

    value->SetPrePairedStatus(static_cast<int>(SlePairState::SLE_PAIR_NONE));
    std::string linkKeyStr = SleConfig::GetInstance().GetLinkKey(addr);
    if (linkKeyStr.empty()) {
        value->SetPairedStatus(static_cast<int>(SlePairState::SLE_PAIR_NONE));
    } else {
        value->SetPairedStatus(static_cast<int>(SlePairState::SLE_PAIR_PAIRED));
    }

    int activePair = SleConfig::GetInstance().GetPairDirect(addr);
    value->SetPairDirection(activePair);

    bool isAudioDevice = SleConfig::GetInstance().GetIsAudioDeviceFlag(addr);
    value->SetIsAudioDeviceFlag(isAudioDevice);

    std::string btAddr = SleConfig::GetInstance().GetBtAddrBySleAddr(addr);
    value->SetBtAddr(btAddr);

    int sleBusinessType = SleConfig::GetInstance().GetSleBusiness(addr);
    value->SetManufacturerBusiness(sleBusinessType);

    uint8_t cryptoAlgo = static_cast<uint8_t>(SleConfig::GetInstance().GetCryptoAlgo(addr));
    value->SetCryptoAlgo(cryptoAlgo);

    uint8_t keyDerivAlgo = static_cast<uint8_t>(SleConfig::GetInstance().GetKeyDerivAlgo(addr));
    value->SetKeyDerivAlgo(keyDerivAlgo);

    uint8_t integrChkInd = static_cast<uint8_t>(SleConfig::GetInstance().GetIntegrChk(addr));
    value-> SetIntegrChkInd(integrChkInd);

    std::string encryptGroupKeyStr = SleConfig::GetInstance().GetGroupKey(addr);
    value->SetEncryptGroupKeyStr(encryptGroupKeyStr);

    uint64_t giv = SleConfig::GetInstance().GetGiv(addr);
    value->SetGiv(giv);

    bool isUserDisconnected = SleConfig::GetInstance().GetUserDisconnectedFlag(addr);
    value->SetIsUserDisconnected(isUserDisconnected);

    bool isDeviceAvailable = SleConfig::GetInstance().GetAvailableControl(addr);
    value->SetIsDeviceAvailable(isDeviceAvailable);

    std::string manuAbility = SleConfig::GetInstance().GetManufacturerAbility(addr);
    std::array<uint8_t, SLE_MANU_ABILITY_LEN> manfacturerAbility;
    SleUtils::ConvertHexStringToInt(manuAbility, manfacturerAbility.data(), SLE_MANU_ABILITY_LEN);
    value->SetManufacturerAbility(manfacturerAbility);
}

void SleAdapter::SetSlePeripheralDeviceCdsmInfo(const std::string &addr,
                                                std::shared_ptr<SlePeripheralDevice> &value) const
{
    int cdsmAddrType = SleConfig::GetInstance().GetCdsmAddrType(addr);
    value->SetCdsmAddrType(cdsmAddrType);

    int sleBusinessType = SleConfig::GetInstance().GetSleBusiness(addr);
    if (sleBusinessType == SLE_PRIVATE_AUDIO_BUSINESS_TYPE) {
        TwsService *twsService = TwsService::GetService();
        NL_CHECK_RETURN(twsService != nullptr, "[SleAdapter]tws service instance invalid");
        twsService->RecoverDataFromConf(RawAddress(addr));
    }
}

void SleAdapter::SetSlePeripheralDeviceModelInfo(const std::string &addr,
    std::shared_ptr<SlePeripheralDevice> &value) const
{
    std::string modelId = SleConfig::GetInstance().GetDeviceModelId(addr);
    if (!modelId.empty()) {
        value->SetModelId(modelId);
    }

    std::string newModelId = SleConfig::GetInstance().GetDeviceNewModelId(addr);
    if (!newModelId.empty()) {
        value->SetNewModelId(newModelId);
    }

    std::string subModelId = SleConfig::GetInstance().GetDeviceSubModelId(addr);
    if (!subModelId.empty()) {
        value->SetSubModelId(subModelId);
    }

    std::string iconId = SleConfig::GetInstance().GetDeviceIconId(addr);
    if (!iconId.empty()) {
        value->SetIconId(iconId);
    }

    std::string devType = SleConfig::GetInstance().GetDeviceDevType(addr);
    if (!devType.empty()) {
        value->SetDevType(devType);
    }
    LOG_INFO("[SleAdapter]Read device model info by config, and then set to device=%{public}s, "
        "modelId=%{public}s, newModelId=%{public}s, subModelId=%{public}s, iconId=%{public}s, devType=%{public}s",
        GetEncryptAddr(addr).c_str(), modelId.c_str(), newModelId.c_str(), subModelId.c_str(), iconId.c_str(),
        devType.c_str());
}

void SleAdapter::ReadPeerDeviceInfoFromConf(const std::vector<std::string> &pairedAddrList) const
{
    LOG_INFO("[SleAdapter] enter");
    for (auto addr : pairedAddrList) {
        RawAddress rawAddr(addr);
        int32_t curCloudPairState = NL_CLOUD_PAIR_STATE::CLOUD_PAIR_INVALID;
        InterfaceCloudPairService::GetInstance().GetCloudPairState(addr, curCloudPairState);
        if (!INVALID_MAC_ADDRESS.compare(rawAddr.GetAddress()) || rawAddr.GetAddress().empty() ||
            curCloudPairState == NL_CLOUD_PAIR_STATE::CLOUD_PAIR_NONE) {
            continue;
        }
        std::shared_ptr<SlePeripheralDevice> remote = std::make_shared<SlePeripheralDevice>();
        SetSlePeripheralDeviceBasicInfo(addr, remote);
        SetSlePeripheralDeviceCdsmInfo(addr, remote);
        SetSlePeripheralDeviceModelInfo(addr, remote);
        LOG_INFO("[SleAdapter] %{public}s add in deviceList, appearance: %{public}d",
            GetEncryptAddr(rawAddr.GetAddress()).c_str(), remote->GetAppearance());
        adapterProperties_->AddPeripheralDevice(addr, remote);
    }
}

void SleAdapter::ClearPeerDeviceInfo() const
{
    LOG_INFO("[SleAdapter] enter");
    pimpl->sleProfileConnectManager_.ClearAllProfileConnectInfo();
    adapterProperties_->RemoveAllPeripheralDevices();
    pimpl->credibleDevice_.Clear();
    pimpl->needReconnectDevices_.Clear();
    pimpl->needReportPairDevices_.Clear();
    DftDeviceManager::GetInstance().DelAllDevice();
}

void SleAdapter::ReadRemoteRssiCallback(CM_ReadRemoteRssiRsp_S *param)
{
    NL_CHECK_RETURN(param, "[SleAdapter] param is null");
    NL_CHECK_RETURN(g_sleAdapterImpl, "[SleAdapter] g_sleAdapterImpl is null");

    std::string addr = GetAddressByConnHandle(param->lcid);
    RawAddress device(addr);
    int rssi = param->rssi;
    int status = param->status;
    LOG_INFO("ReadRemote rssi=%{public}d", rssi);
    g_sleAdapterImpl->pimpl->sleDeviceRssiCallback_.ForEach([&device, rssi, status](ISleDeviceRssiCallback &observer) {
        observer.OnReadRemoteRssiEvent(device, rssi, status);
    });
}

void SleAdapter::ReadFeatureVersionCallback(CM_ReadRemoteFeatureVersionRsp_S *param)
{
    if (param == nullptr) {
        return;
    }
    NL_CHECK_RETURN(g_sleAdapterImpl, "[SleAdapter] g_sleAdapterImpl is null");
    RawAddress peerAddr = RawAddress::ConvertToString(param->addr.addr);
    uint8_t frameType = static_cast<uint8_t>(SleConnFrameType::SLE_CONN_FRAME_TYPE_1);
    g_sleAdapterImpl->pimpl->addrAndFrameTypeMap_.GetValue(peerAddr.GetAddress(), frameType);

    if (frameType == static_cast<uint8_t>(SleConnFrameType::SLE_CONN_FRAME_TYPE_4)) {
        LOG_DEBUG("[SleAdapter] frame 4");
        return;
    }
    LOG_DEBUG("[SleAdapter] frame 1");
    uint8_t phyType = CM_PHY_TYPE_1M;
    if (g_sleAdapterImpl->adapterProperties_->IsAudioDevice(peerAddr.GetAddress())) {
        frameType = CM_RADIO_FRAME_TYPE_1;
    } else {
        frameType = CM_RADIO_FRAME_TYPE_2;
    }
    g_sleAdapterImpl->SetPhy(peerAddr, frameType, phyType);
    return;
}

void SleAdapter::SetPhy(const RawAddress &device, uint8_t frameType, uint8_t phyType)
{
    CM_SetPhyReq_S phyParam;
    (void)memset_s(&phyParam, sizeof(CM_SetPhyReq_S), 0x00, sizeof(CM_SetPhyReq_S));
    phyParam.lcid = adapterProperties_->GetLcidByAddress(device);
    phyParam.txFormat = frameType;
    phyParam.rxFormat = frameType;
    phyParam.txPhy = phyType;
    phyParam.rxPhy = phyType;
    phyParam.txPilotDensity = CM_PILOT_DENSITY_16_TO_1;
    phyParam.rxPilotDensity = CM_PILOT_DENSITY_16_TO_1;
    phyParam.gFeedback = 0;
    phyParam.tFeedback = 0;
    CM_SetPhy(&phyParam);
}

void SleAdapter::SetPhyCallback(CM_SetPhyRsp_S *param)
{
    NL_CHECK_RETURN(param, "[SleAdapter] param is null");
    LOG_DEBUG("[SleAdapter]co_handle:%{public}d, status:%{public}d", param->lcid, param->status);
    CM_SetPhyRsp_S phyParam = {};
    NL_CHECK_RETURN(memcpy_s(&phyParam, sizeof(CM_SetPhyRsp_S), param,
        sizeof(CM_SetPhyRsp_S)) == EOK, "[SleAdapter] memcpy_s err");

    DoInAdapterThread([sleAdapterImpl = g_sleAdapterImpl, phyParam]() -> void {
        sleAdapterImpl->PhyChangedTask(phyParam);
    });
}

void SleAdapter::PhyChangedTask(const CM_SetPhyRsp_S &param)
{
    NL_CHECK_RETURN(param.txFormat == param.rxFormat, "tx rx frame type is not same");
    NL_CHECK_RETURN(param.txPhy == param.rxPhy, "tx rx phy type is not same");
    std::string addr = GetAddressByConnHandle(param.lcid);
    RawAddress device = RawAddress(addr);

    ProfileASC *ascService = static_cast<ProfileASC *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_ASC));
    NL_CHECK_RETURN(ascService, "cant find ASC service");
    ascService->PhyChanged(device, param.txFormat, param.txPhy, param.status);
}

void SleAdapter::ReadLocalFeatureCallback(void *param)
{
    NL_CHECK_RETURN(param, "[SleAdapter] param is null");
    NLSTK_CfgdbLocalFeatures_S parameter = *(reinterpret_cast<NLSTK_CfgdbLocalFeatures_S *>(param));
    LOG_INFO("[SleAdapter] status:%{public}d ", parameter.status);
    SleFeature::GetInstance().SetLocalFeature(&parameter);
}

void SleAdapter::FreqBandChanged(CM_FreqBandSwitchParam *param)
{
    HILOGI("Enter");
    NL_CHECK_RETURN(param, "[SleAdapter] param is null");
    CM_FreqBandSwitchParam freqBandParam = {};
    NL_CHECK_RETURN(memcpy_s(&freqBandParam, sizeof(CM_FreqBandSwitchParam), param,
        sizeof(CM_FreqBandSwitchParam)) == EOK, "[SleAdapter] memcpy_s err");

    DoInAdapterThread([sleAdapterImpl = g_sleAdapterImpl, freqBandParam]() -> void {
        sleAdapterImpl->FreqBandChangedTask(freqBandParam);
    });
}

void SleAdapter::FreqBandChangedTask(const CM_FreqBandSwitchParam &param)
{
    HILOGI("Enter");
    RawAddress device(adapterProperties_->GetAddressByLcid(param.lcid));
    int32_t freqBand = param.newFreqBand;
    pimpl->slePeripheralCallback_.ForEach([&device, freqBand](ISlePeripheralCallback &observer) {
        observer.OnLinkFreqBandChanged(device, freqBand);
    });
}

void SleAdapter::AcbSubrateChanged(CM_AcbSubrateCbParam_S *param)
{
    HILOGD("Enter");
    NL_CHECK_RETURN(param, "[SleAdapter] param is null");
    CM_AcbSubrateCbParam_S subrateParam = {};
    NL_CHECK_RETURN(memcpy_s(&subrateParam, sizeof(CM_AcbSubrateCbParam_S), param,
        sizeof(CM_AcbSubrateCbParam_S)) == EOK, "[SleAdapter] memcpy_s err");

    DoInAdapterThread([sleAdapterImpl = g_sleAdapterImpl, subrateParam]() -> void {
        sleAdapterImpl->AcbSubrateChangedTask(subrateParam);
    });
}

void SleAdapter::AcbSubrateChangedTask(const CM_AcbSubrateCbParam_S &param)
{
    HILOGD("Enter");
    RawAddress device = RawAddress::ConvertToString(param.addr.addr);
    if (!adapterProperties_->IsAudioDevice(device.GetAddress())) {
        HILOGI("It is not a audio device");
        return;
    }

    uint32_t subrate = param.subrate;

    ProfileASC *ascService = static_cast<ProfileASC *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_ASC));
    NL_CHECK_RETURN(ascService, "cant find ASC service");
    ascService->AcbSubrateChanged(device, subrate);
}

void SleAdapter::AcbSubrateChangeReq(CM_AcbSubrateCbParam_S *param)
{
    HILOGI("Enter");
    NL_CHECK_RETURN(param, "[SleAdapter] param is null");
    CM_AcbSubrateCbParam_S subrateParam = {};
    NL_CHECK_RETURN(memcpy_s(&subrateParam, sizeof(CM_AcbSubrateCbParam_S), param,
        sizeof(CM_AcbSubrateCbParam_S)) == EOK, "[SleAdapter] memcpy_s err");

    DoInAdapterThread([sleAdapterImpl = g_sleAdapterImpl, subrateParam]() -> void {
        sleAdapterImpl->AcbSubrateChangeReqTask(subrateParam);
    });
}

void SleAdapter::ReadAcceptFilterListSizeCallbackTask(const CM_ReadAcceptFilterListSize_S &param)
{
    HILOGD("[SleAdapter] Read accept filter list size result: status=%{public}u, size=%{public}u",
        param.status, param.listSize);

    if (param.status == 0) {
        SleReconnectManager::GetInstance().SetBgConnMaxNum(param.listSize);
    }
}

void SleAdapter::ReadAcceptFilterListSizeCallback(CM_ReadAcceptFilterListSize_S *param)
{
    HILOGI("Enter");
    NL_CHECK_RETURN(param, "[SleAdapter] param is null");
    CM_ReadAcceptFilterListSize_S fltListSizeParam = {};
    NL_CHECK_RETURN(memcpy_s(&fltListSizeParam, sizeof(CM_ReadAcceptFilterListSize_S), param,
        sizeof(CM_ReadAcceptFilterListSize_S)) == EOK, "[SleAdapter] memcpy_s err");
    DoInAdapterThread([sleAdapterImpl = g_sleAdapterImpl, fltListSizeParam]() -> void {
        sleAdapterImpl->ReadAcceptFilterListSizeCallbackTask(fltListSizeParam);
    });
}

void SleAdapter::HidCoexModeCallback(CM_HidCoexModeParam_S *param)
{
    NL_CHECK_RETURN(param, "[SleAdapter] param is null");
    NL_CHECK_RETURN(param->eventType == CM_SLE_CBK_EVENT_GET_HID_COEX_INTERVAL ||
        param->eventType == CM_SLE_CBK_EVENT_HID_COEX_MODE_PARAM_UPDATE, "[SleAdapter] invalid eventType");
    RawAddress device = RawAddress::ConvertToString(param->addr.addr);
    if (param->eventType == CM_SLE_CBK_EVENT_GET_HID_COEX_INTERVAL) {
        SleControllerService::GetInstance().GetSleHidCoexInterval(device.GetAddress(), param->incomingInterval,
            param->coexInterval);
    } else {
        SleControllerService::GetInstance().UpdateSleHidCoexModePendingInterval(device.GetAddress(),
            param->incomingInterval);
    }
}

void SleAdapter::AcbSubrateChangeReqTask(const CM_AcbSubrateCbParam_S &param)
{
    HILOGI("Enter");
    RawAddress device = RawAddress::ConvertToString(param.addr.addr);
    ProfileASC *ascService = static_cast<ProfileASC *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_ASC));
    NL_CHECK_RETURN(ascService, "cant find ASC service");
    SleAcbSubrateParam subrateParam;
    subrateParam.subrate = param.subrate;
    subrateParam.subrateMax = param.subrateMax;
    subrateParam.maxLatency = param.maxLatency;
    subrateParam.continuationNum = param.continuationNum;
    subrateParam.supervisionTimeout = param.supervisionTimeout;
    ascService->AcbSubrateChangeReq(device, subrateParam);
}

void SleAdapter::ChipResetNotify(void *param)
{
    LOG_INFO("[SleAdapter] chip reset");
    NL_CHECK_RETURN(param, "[SleAdapter] param is null");
    SleServiceManager::GetInstance()->OnChipResetNotify();
}

int SleAdapter::RegisterCallbackToCm()
{
    LOG_DEBUG("[SleAdapter] enter");

    CM_ConnectCbks_S cbks = {};
    cbks.connRemoteUpdateParamReqCbk = &SleAdapter::ConnectionUpdateRequestCallback;
    cbks.connUpdateParamCbk = &SleAdapter::ConnectionUpdateCallback;
    cbks.readRemoteRssiCbk = &SleAdapter::ReadRemoteRssiCallback;
    cbks.readRemoteFeatureVersionCbk = &SleAdapter::ReadFeatureVersionCallback;
    cbks.setPhyCbk = &SleAdapter::SetPhyCallback;
    cbks.setAcbSubrateCbk = &SleAdapter::AcbSubrateChanged;
    cbks.reqAcbSubrateCbk = &SleAdapter::AcbSubrateChangeReq;
    cbks.readAcceptFilterListSizeCbk = &SleAdapter::ReadAcceptFilterListSizeCallback;
    cbks.hidCoexModeCbk = &SleAdapter::HidCoexModeCallback;

    uint32_t ret = CM_Init();
    if (ret != 0) {
        LOG_ERROR("[SleAdapter]:CM_Init failed, ret:%{public}u", ret);
        return NLSTK_ERRCODE_FAIL;
    }
    ret = CM_ListenFreqBandSwitchEvent(&SleAdapter::FreqBandChanged);
    if (ret != 0) {
        LOG_ERROR("[SleAdapter] CM_ListenFreqBandSwitchEvent failed, ret=%{public}u", ret);
        return NLSTK_ERRCODE_FAIL;
    }
    ret = CM_RegConnectCbks(&cbks);
    if (ret != 0) {
        (void)CM_UnlistenFreqBandSwitchEvent(&SleAdapter::FreqBandChanged);
        LOG_ERROR("[SleAdapter]:CM_RegConnectCbks failed, ret:%{public}u", ret);
        return NLSTK_ERRCODE_FAIL;
    }

    CM_LogicLinkCbks_S cmCbk = {};
    cmCbk.moduleId = CM_MODULE_ADPT;
    cmCbk.logicLinkCbk = &SleAdapter::AcbConnectionStateCallback;
    CM_RegLogicLinkListener(&cmCbk);
    return NLSTK_ERRCODE_SUCCESS;
}

int SleAdapter::DeregisterCallbackToBtm() const
{
    LOG_DEBUG("[SleAdapter]");

    if (!pimpl->btmEnableFlag_) {
        return NLSTK_ERRCODE_STATUS_ERR;
    }
    int ret = NLSTK_ERRCODE_SUCCESS;
    return ret;
}

std::string CombineChannelAndNoise(const std::vector<uint8_t>& rssiIndex,
    const std::vector<int8_t>& actualRssiValue)
{
    std::string result;
    result.append(std::to_string(rssiIndex[0]));
    result.append(":");
    result.append(std::to_string(actualRssiValue[0]));
    for (int i = 1; i < CHANNEL_INDEX_NUM; ++i) {
        result.append("/");
        result.append(std::to_string(rssiIndex[i]));
        result.append(":");
        result.append(std::to_string(actualRssiValue[i]));
    }
    return result;
}

void SleAdapter::RssiChangedCallback(void *param)
{
    NL_CHECK_RETURN(g_sleAdapterImpl != nullptr, "param is null");
    NbcCallbackParam chipInfo = *(reinterpret_cast<NbcCallbackParam *>(param));
    NL_CHECK_RETURN(chipInfo.data != nullptr && chipInfo.dataLen >= sizeof(DisconChipInfo), "param error");
    DisconChipInfo info = *(reinterpret_cast<DisconChipInfo *>(chipInfo.data));

    DoInAdapterThread([sleAdapterImpl = g_sleAdapterImpl, info]() -> void {
        sleAdapterImpl->RssiChangedCallbackTask(info);
    });
}

void SleAdapter::RssiChangedCallbackTask(const DisconChipInfo &info)
{
    std::vector<uint8_t> rsssIndex(std::begin(info.rssiIdx), std::end(info.rssiIdx));
    std::vector<int8_t> channelNoise(std::begin(info.actualRssiValue), std::end(info.actualRssiValue));
    std::string noise = CombineChannelAndNoise(rsssIndex, channelNoise);
    std::string addr = GetAddressByConnHandle(info.connHandle);
    HILOGI("device:%{public}s, channel noise:%{public}s, rssi:%{public}d", GET_ENCRYPT_ADDR(RawAddress(addr)),
        noise.c_str(), info.signalStrength);
    ServiceManagerPluginInterface::GetInstance()->RssiChangedCbkProc(addr, info.signalStrength);
    DftCacheDisconChipInfo(addr, info.signalStrength, noise);
}

std::string SleAdapter::GetAddressByConnHandle(uint16_t connHandle)
{
    // 异步链路connHandle等价于lcid，例如D2D场景
    std::string addr = g_sleAdapterImpl->adapterProperties_->GetAddressByLcid(connHandle);
    HILOGI("connHandle:%{public}d, device:%{public}s", connHandle, GET_ENCRYPT_ADDR(RawAddress(addr)));

    // 同步链路connHandle先转换成异步链路的lcid，再取地址，例如音频场景
    if (addr == INVALID_MAC_ADDRESS) {
        uint16_t lcid = CM_GetLcidByConnHandle(connHandle);
        addr = g_sleAdapterImpl->adapterProperties_->GetAddressByLcid(lcid);
        HILOGI("lcid:%{public}d, GetAddressByLcid:%{public}s", lcid, GET_ENCRYPT_ADDR(RawAddress(addr)));
    }
    return addr;
}

void SleAdapter::PowerLevelChangedCallback(void *param)
{
    NL_CHECK_RETURN(g_sleAdapterImpl != nullptr, "param is null");
    NbcCallbackParam chipInfo = *(reinterpret_cast<NbcCallbackParam *>(param));
    NL_CHECK_RETURN(chipInfo.data != nullptr && chipInfo.dataLen >= sizeof(PowerLevelInfo), "param error");
    PowerLevelInfo info = *(reinterpret_cast<PowerLevelInfo *>(chipInfo.data));

    DoInAdapterThread([sleAdapterImpl = g_sleAdapterImpl, info]() -> void {
        sleAdapterImpl->PowerLevelChangedTask(info);
    });
}

void SleAdapter::PowerLevelChangedTask(const PowerLevelInfo &info)
{
    std::string addr = GetAddressByConnHandle(info.connHandle);
    HILOGI("device:%{public}s, power level:%{public}d", GetEncryptAddr(addr).c_str(), info.powerLevel);
    RawAddress device(addr);
    uint8_t powerLevel = info.powerLevel;
    pimpl->slePeripheralCallback_.ForEach([&device, powerLevel](ISlePeripheralCallback &observer) {
        observer.OnLinkPowerLevelChanged(device, powerLevel);
    });
}

int SleAdapter::RegisterCallbackToNbc()
{
    LOG_DEBUG("[SleAdapter] enter");
    NLSTK_CfgdbCbk_S cbks;
    cbks.rssiCbk = &SleAdapter::RssiChangedCallback;
    cbks.powerLevelCbk = &SleAdapter::PowerLevelChangedCallback;
    cbks.localFeatureCbk = &SleAdapter::ReadLocalFeatureCallback;
    cbks.chipResetNotifyCbk = &SleAdapter::ChipResetNotify;
    int ret = NLSTK_CfgdbRegisterCbks(&cbks);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        LOG_ERROR("[SleAdapter]:NLSTK_CfgdbRegisterCbks failed!");
    }
    return ret;
}

void SleAdapter::AcbConnectionStateCallback(CM_LogicLinkState_S *param)
{
    NL_CHECK_RETURN(param != NULL && g_sleAdapterImpl != nullptr, "param is null");
    CM_LogicLinkState_S connResult;
    NL_CHECK_RETURN(memcpy_s(&connResult, sizeof(CM_LogicLinkState_S), param,
        sizeof(CM_LogicLinkState_S)) == EOK, "[SleAdapter] memcpy_s err");
    DoInAdapterThread([sleAdapterImpl = g_sleAdapterImpl, connResult]() -> void {
        sleAdapterImpl->ConnectionStateTask(connResult);
    });
}

void SleAdapter::ConnectionStateTask(const CM_LogicLinkState_S &connResult)
{
    RawAddress peerAddr = RawAddress::ConvertToString(connResult.addr.addr);
    DftCachePeerInfoTime(peerAddr.GetAddress(), PEER_CONN_LAST_TIME);
    uint8_t result = connResult.result;
    uint16_t lcid = connResult.lcid;
    uint8_t role = connResult.role;
    if (result == CM_STATE_CONNECTED) {
        ConnectionCompleteTask(connResult.addr, lcid, role, connResult.connCompleteType);
        NL_CHECK_RETURN(pimpl->sleCoexist_, "sleCoexist_ is null");
        pimpl->sleCoexist_->ConnectionStatusChanged(lcid, connResult.addr);
    } else if (result == CM_STATE_DISCONNECTED) {
        DisconnectionCompleteTask(lcid, peerAddr, connResult.discReason);
        SleCoexistManager::GetInstance()->OnConnectionRemoved(lcid);
    }
    int acbCount = adapterProperties_->GetConnectedCnt();
    pimpl->acbCount_.store(acbCount);
    PublishDeviceConnectionState(acbCount, result);
    DftCacheAcbFinishConn(peerAddr.GetAddress(), connResult.result, connResult.discReason, acbCount, lcid);
}

void SleAdapter::OnAcbStateChanged(const RawAddress &device, int connectState, int reason) const
{
    InterfaceCloudPairService::GetInstance().HandleAcbStateChanged(device, connectState, reason);
    pimpl->slePeripheralCallback_.ForEach([device, connectState, reason](ISlePeripheralCallback &observer) {
        observer.OnAcbStateChanged(device, connectState, reason);
    });
    NL_CHECK_RETURN(g_sleAdapterImpl != nullptr, "param is null");
    if (reason == static_cast<int>(SleDiscReason::SLE_DISC_REASON_CANCEL_PAIR)) {
        LOG_INFO("remote device deletes pair record, addr:%{public}s", GetEncryptAddr(device.GetAddress()).c_str());
        DoInAdapterThread([sleAdapterImpl = g_sleAdapterImpl, device]() -> void {
            sleAdapterImpl->CancelPairingTask(device);
        });
    }
}

bool SleAdapter::ConnectionCompleteTaskInner(int pairState, uint16_t lcid,
    const RawAddress &peerAddr, const SLE_Addr_S &addr, uint8_t role) const
{
    HILOGI("[SleAdapter]device: %{public}s, pairState: %{public}d", GET_ENCRYPT_ADDR(peerAddr), pairState);
    if (pairState == static_cast<int>(SlePairState::SLE_PAIR_PAIRED)) {
        DftCacheSecurityPairType(peerAddr.GetAddress(), true);
        SleConfig::GetInstance().SetUserDisconnectedFlag(peerAddr.GetAddress(), false);
        if (role == SLE_T_NODE || // T节点不发起加密流程；云配对设备识别到token变化后重配，不发起加密
            InterfaceCloudPairService::GetInstance().CloudDeviceConnectionComplete(peerAddr)) {
            return true;
        }
        pimpl->sleSecurity_.GapSleRequestSecurity(lcid, addr, SLE_CONNECTION_ROLE_PRIMARY);
    } else if (pairState == static_cast<int>(SlePairState::SLE_PAIR_PAIRING)) {
        pimpl->sleSecurity_.StartPair(peerAddr, addr.type);
        DftCacheSecurityPairType(peerAddr.GetAddress(), true);
    } else {
        bool pairStateNone = pairState == static_cast<int>(SlePairState::SLE_PAIR_NONE);
        bool linkKeyEmpty = SleConfig::GetInstance().GetLinkKey(peerAddr.GetAddress()).empty();
        if (pairStateNone && !linkKeyEmpty) {    // 配对状态被设置为了NONE，但是其实本地仍有linkKey信息，也需要主动发起配对
            HILOGI("[SleAdapter]device: %{public}s, pair state is none but has cache link key, need start pair.",
                GET_ENCRYPT_ADDR(peerAddr));
            PairingStatus(peerAddr);    // 设置配对状态从NONE->PAIRING
            pimpl->sleSecurity_.StartPair(peerAddr, addr.type);
            DftCacheSecurityPairType(peerAddr.GetAddress(), true);
        }
    }
    return true;
}

void SleAdapter::CreateNewPeripheralDevice(const SLE_Addr_S &addr, uint16_t lcid, uint8_t role,
    uint8_t connCompleteType) const
{
    const RawAddress peerAddr = RawAddress::ConvertToString(addr.addr);
    std::shared_ptr<SlePeripheralDevice> peerDevice = std::make_shared<SlePeripheralDevice>();
    peerDevice->SetLcid(lcid);
    peerDevice->SetRoles(role);
    peerDevice->SetAddressType(addr.type);
    peerDevice->SetAddress(peerAddr);
    peerDevice->SetAcbConnectState(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED));
    peerDevice->SetPrePairedStatus(static_cast<int>(SlePairState::SLE_PAIR_NONE));
    peerDevice->SetPairedStatus(static_cast<int>(SlePairState::SLE_PAIR_NONE));
    DftDeviceManager::GetInstance().AddDevice(peerAddr);
    adapterProperties_->AddPeripheralDevice(peerAddr.GetAddress(), peerDevice);
    int sleBusinessType = SleConfig::GetInstance().GetSleBusiness(peerAddr.GetAddress());
    HILOGI("[SleAdapter] add device(%{public}s) enter, sleBusinessType %{public}d",
        GET_ENCRYPT_ADDR(peerAddr), sleBusinessType);
    CdsmService *cdsmService = CdsmService::GetService();
    NL_CHECK_RETURN(cdsmService, "cdsmService is null");
    /* 合作集成员首次配对 */
    if (sleBusinessType != Nearlink::SLE_PRIVATE_AUDIO_BUSINESS_TYPE &&
        cdsmService->CdsmCheckIsCooperationMember(peerAddr)) {
        // 如果不是私有耳机，且设备为合作集成员
        adapterProperties_->HandleCdsmMemberFirstPairing(peerAddr);
        /* 合作集成员地址主动发起配对 */
        HILOGI("cdsm member first pairing enter,addr:%{public}s", GetEncryptAddr(peerAddr.GetAddress()).c_str());
        pimpl->sleSecurity_.StartPair(peerAddr, addr.type);
        PairingStatus(peerAddr);
    }
}

void SleAdapter::ConnectionCompleteTask(const SLE_Addr_S &addr, uint16_t lcid, uint8_t role,
    uint8_t connCompleteType) const
{
    const RawAddress peerAddr = RawAddress::ConvertToString(addr.addr);
    HILOGI("[SleAdapter] lcid is %{public}hu role:%{public}d", lcid, role);
    int pairState = adapterProperties_->GetPairStatus(peerAddr);
    if (adapterProperties_->ConnectionCompleteHelper(peerAddr, lcid, role, addr.type)) {
        NL_CHECK_RETURN(ConnectionCompleteTaskInner(pairState, lcid, peerAddr, addr, role), "connect task fail");
    } else {
        CreateNewPeripheralDevice(addr, lcid, role, connCompleteType);
    }
    if (IsScanConnTypeAndFrameType4(peerAddr, connCompleteType) &&
        !SleRemoteDeviceAdapter::GetInstance()->IsAudioDevice(peerAddr.GetAddress())) {
        ServiceManagerPluginInterface::GetInstance()->SetConnFrameType4Subrate(peerAddr);
    }
    OnAcbStateChanged(peerAddr, static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED), ACB_CONNECT_SUCCESS);
    LOG_DEBUG("[SleAdapter] delete sleAccessList %{public}s", GetEncryptAddr(peerAddr.GetAddress()).c_str());
    CdsmService *cdsmService = CdsmService::GetService();
    /* 停合作集邀请广播 */
    if (cdsmService != nullptr) {
        cdsmService->CdsmStopInviteAdv(peerAddr, false);
    }
}

bool SleAdapter::IsDisconnectedByUser(int acbConnState, int pairState, int reason) const
{
    if (acbConnState == static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTING) &&
        pairState == static_cast<int>(SlePairState::SLE_PAIR_PAIRED) &&
        reason == static_cast<int>(NLSTK_SLE_CONNECTION_TERMINATED_BY_LOCAL_HOST)) {
        HILOGI("Device is disconnceted by user");
        return true;
    }
    return false;
}

bool SleAdapter::NeedBgConn(int acbConnState, int pairState, const RawAddress& peerAddr, int reason) const
{
    if (pairState != static_cast<int>(SlePairState::SLE_PAIR_PAIRED)) {
        HILOGI("not SLE_PAIR_PAIRED, no need bg conn");
        return false;
    }

    // 关星闪断连场景不回连
    if (SleServiceManager::GetInstance()->GetState(SleTransport::ADAPTER_SLE) != STATE_TURN_ON) {
        HILOGI("active disconnect and not STATE_TURN_ON, no need bg conn");
        return false;
    }

    if (acbConnState > static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED)) {
        HILOGD("ENCRYPTED, need bg conn");
        return true;
    }

    if (adapterProperties_->IsAudioDevice(peerAddr.GetAddress())) {
        HILOGD("audio device, need bg conn");
        return true;
    }

    if (pimpl->needReconnectDevices_.Count(peerAddr.GetAddress()) > 0) {
        // 非首次配对，加密失败仅重试一次
        pimpl->needReconnectDevices_.Erase(peerAddr.GetAddress());
        return true;
    }
    bool isNeedReconn = SLE_DISCONN_REASONS_NEED_BG_CONN.count(reason) > 0;
    ServiceManagerPluginInterface::GetInstance()->SleReconnectProc(isNeedReconn, acbConnState, peerAddr,
        reason, SLE_DISCONN_REASONS_NEED_BG_CONN);
    return isNeedReconn;
}

void SleAdapter::DisconnectionCompleteTask(uint16_t lcid, const RawAddress &peerAddr, int reason) const
{
    LOG_INFO("[SleAdapter] handle is %{public}d, reason: 0x%{public}x", lcid, reason);
    std::shared_ptr<const SlePeripheralDevice> remoteDevice = adapterProperties_->GetRemoteDevice(peerAddr);
    if (remoteDevice) {
        int pairState = remoteDevice->GetPairedStatus();
        int acbConnState = remoteDevice->GetAcbConnectState();
        int prePairState = remoteDevice->GetPrePairedStatus();
        adapterProperties_->SetAcbState(peerAddr.GetAddress(),
            static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED));
        adapterProperties_->SetLcid(peerAddr.GetAddress(), INVALID_LCID);
        adapterProperties_->SetConnDirect(peerAddr, static_cast<int>(SleConnDirect::SLE_CONNECTION_PASSIVE));
        LOG_INFO("[SleAdapter] PairedStatus: %{public}u, AcbConnectState: %{public}d", pairState, acbConnState);
        bool isNeedBgConn = NeedBgConn(acbConnState, pairState, peerAddr, reason);
        SleReconnectManager::GetInstance().OnDeviceDisConnected(peerAddr, isNeedBgConn);

        if (IsDisconnectedByUser(acbConnState, pairState, reason)) {
            SleConfig::GetInstance().SetUserDisconnectedFlag(peerAddr.GetAddress(), true);
        }
        int unpairedReason = HandleDisconnAndUnpairedReason(reason);
        LOG_INFO("[SleAdapter] sleDisconnReason_: %{public}d", pimpl->sleDisconnReason_.load());
        if (pairState == static_cast<int>(SlePairState::SLE_PAIR_PAIRING)) {
            // 已配对应用重新发起配对的场景，需要把配对状态恢复成已配对
            if (prePairState == static_cast<int>(SlePairState::SLE_PAIR_PAIRED)) {
                adapterProperties_->SetPrePairStatus(peerAddr, static_cast<int>(SlePairState::SLE_PAIR_PAIRING));
                adapterProperties_->SetPairStatus(peerAddr, static_cast<int>(SlePairState::SLE_PAIR_PAIRED));
            } else {
                CancelPairComplete(peerAddr, NLSTK_ERRCODE_SUCCESS, unpairedReason);
            }
        }
        uint8_t profileConnectState = pimpl->sleProfileConnectManager_.GetProfileConnectState(peerAddr);
        if (profileConnectState != SLE_ADAPTER_PROF_CONN_STATE_UNUSED) {
            SetAcbDisConnReasonTask(peerAddr.GetAddress(), reason);
            HILOGI("[SleAdapter] profile not all disconnected, defer reconnect check, state:%{public}d",
                profileConnectState);
        } else {
            if (isNeedBgConn) {
                adapterProperties_->AddBgConnDevice(peerAddr.GetAddress());
            }
            pimpl->sleProfileConnectManager_.ClearProfileConnectInfo(peerAddr);
        }
    }
    if (!InterfaceCloudPairService::GetInstance().IsPreparingRepair(peerAddr)) {
        pimpl->sleProfileConnectManager_.NotifyAcbDisconnected(peerAddr);
    }
    OnAcbStateChanged(peerAddr, static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED), reason);

    CdsmService *cdsmService = CdsmService::GetService();
    if (cdsmService != nullptr) {
        cdsmService->CdsmStopInviteAdv(peerAddr, true);
    }
}

int SleAdapter::HandleDisconnAndUnpairedReason(int reason) const
{
    int unpairedReason = static_cast<int>(PairingStateChangeReason::PAIRING_FAILURE);
    switch (reason) {
        case static_cast<int>(NLSTK_SLE_CONNECTION_TERMINATED_BY_LOCAL_HOST):
            pimpl->sleDisconnReason_.store(static_cast<int>(SleConnectReason::CONNECT_LOCAL_DISCONNECT));
            break;
        case static_cast<int>(NLSTK_SLE_REMOTE_USER_TERMINATED_CONNECTION):
            pimpl->sleDisconnReason_.store(static_cast<int>(SleConnectReason::CONNECT_REMOTE_DISCONNECT));
            break;
        case static_cast<int>(NLSTK_SLE_CONNECTION_TIMEOUT):
            pimpl->sleDisconnReason_.store(static_cast<int>(SleConnectReason::CONNECT_FAIL_ACB_CONNECTION));
            unpairedReason = static_cast<int>(PairingStateChangeReason::PAIRING_ACB_CONNECTION_FAILED);
            break;
        default:
            break;
    }
    return unpairedReason;
}

void SleAdapter::ConnectionUpdateCallback(CM_ConnectUpdateParamRsp_S *param)
{
    LOG_DEBUG("[SleAdapter]");
    NL_CHECK_RETURN(param != nullptr && g_sleAdapterImpl != nullptr, "param is null");
    CM_ConnectUpdateParamRsp_S paramInfo;
    (void)memcpy_s(&paramInfo, sizeof(CM_ConnectUpdateParamRsp_S),
                   param, sizeof(CM_ConnectUpdateParamRsp_S));
    DoInAdapterThread([sleAdapterImpl = g_sleAdapterImpl, paramInfo]() -> void {
        sleAdapterImpl->ConnectionUpdateTask(paramInfo);
    });
}

void SleAdapter::ConnectionUpdateTask(const CM_ConnectUpdateParamRsp_S &param) const
{
    pimpl->sleConnectionUpdateCallback_.ForEach([param](ISleConnectionCallback &observer) {
        observer.OnConnectionUpdate(param.lcid, param.extension.interval,
            param.extension.interval, param.extension.latency);
    });

    NL_CHECK_RETURN(pimpl->sleCoexist_, "sleCoexist_ is null");
    NL_CHECK_RETURN(param.result == CM_STATE_CONNECTED, "not connected");
    pimpl->sleCoexist_->ConnectionParamChanged(param);
}

bool SleAdapter::GetConnectionParam(std::string device, uint16_t &timeout, uint16_t &maxLatency) const
{
    RawAddress addr(device);
    uint8_t peerAddrType = adapterProperties_->GetPeerDeviceAddrType(addr);
    SLE_Addr_S tmpAddr;
    (void)memset_s(&tmpAddr, sizeof(tmpAddr), 0x0, sizeof(tmpAddr));
    tmpAddr.type = peerAddrType;
    addr.ConvertToUint8(tmpAddr.addr, SLE_ADDR_LEN);

    // 直接调用 Manager
    bool ret = SleCoexistManager::GetInstance()->GetConnectionParam(tmpAddr, timeout, maxLatency);
    NL_CHECK_RETURN_RET(ret, false, "GetConnectionParam fail");
    HILOGI("timeout: 0x%{public}x, max_latency: 0x%{public}x", timeout, maxLatency);
    return true;
}

void SleAdapter::ConnectionUpdateRequestCallback(CM_ConnectRemoteUpdateParamReq_S *param)
{
    NL_CHECK_RETURN(param, "[SleAdapter] param is null");
    NL_CHECK_RETURN(g_sleAdapterImpl, "[SleAdapter] g_sleAdapterImpl is null");
    uint16_t connHandle = param->lcid;
    uint16_t minInterval = param->intervalMin;
    uint16_t maxInterval = param->intervalMax;
    uint16_t maxLatency = param->maxLatency;
    g_sleAdapterImpl->pimpl->sleConnectionUpdateCallback_.ForEach([connHandle, minInterval,
        maxInterval, maxLatency] (
        ISleConnectionCallback &observer) {
            observer.OnConnectionUpdate(connHandle, minInterval, maxInterval, maxLatency);
    });
    HILOGI("ConnectionParamChanged conn_hdl=0x%{public}x, interval_min=0x%{public}x, interval_max=0x%{public}x",
        param->lcid, minInterval, maxInterval);
}

bool SleAdapter::GetConnectionParam(std::string device, uint16_t &timeout, uint16_t &maxLatency,
    uint16_t &interval) const
{
    RawAddress addr(device);
    uint8_t peerAddrType = adapterProperties_->GetPeerDeviceAddrType(addr);
    SLE_Addr_S tmpAddr;
    (void)memset_s(&tmpAddr, sizeof(tmpAddr), 0x0, sizeof(tmpAddr));
    tmpAddr.type = peerAddrType;
    addr.ConvertToUint8(tmpAddr.addr, SLE_ADDR_LEN);
 
    // 直接调用 Manager
    bool ret = SleCoexistManager::GetInstance()->GetConnectionParam(tmpAddr, timeout, maxLatency, interval);
    NL_CHECK_RETURN_RET(ret, false, "GetConnectionParam fail");
    HILOGI("timeout: 0x%{public}x, max_latency: 0x%{public}x, interval: 0x%{public}x", timeout, maxLatency, interval);
    return true;
}

void SleAdapter::CancelPairComplete(const RawAddress &device, const int status, const int unpairedReason) const
{
    LOG_INFO("[SleAdapter]:result: %{public}d, reason: %{public}d, dev:%{public}s.",
        status, unpairedReason, GET_ENCRYPT_ADDR(device));
    if (status != NLSTK_ERRCODE_SUCCESS) {
        HILOGE("[SleAdapter]:status is not NLSTK_ERRCODE_SUCCESS.");
        DftReportUnPairFailInfo(device.GetAddress(), status, 1);
        return;
    }
    std::shared_ptr<const SlePeripheralDevice> remoteDevice = adapterProperties_->GetRemoteDevice(device);
    NL_CHECK_RETURN(remoteDevice, "addr %{public}s is not start.", GetEncryptAddr(device.GetAddress()).c_str());
    int preState = remoteDevice->GetPairedStatus();
    int acbState = remoteDevice->GetAcbConnectState();
    adapterProperties_->SetPrePairStatus(device, preState);
    adapterProperties_->SetPairStatus(device, static_cast<int>(SlePairState::SLE_PAIR_NONE));

    bool isCdsmAcbConnected = IsAcbConnectedTask(device);
    /* 可信设备列表存取使用reportAddr */
    RawAddress reportAddr(device);
    RawAddress otherAddr(device);
    CdsmService *cdsmService = CdsmService::GetService();
    if (cdsmService != nullptr) {
        cdsmService->CdsmGetReportAddr(device, reportAddr);
        cdsmService->CdsmGetOtherAddr(device, otherAddr);
    }
    CancelPairCompleteInner(device);
    int reason = unpairedReason == static_cast<int>(PairingStateChangeReason::PAIRING_INVALID_REASON) ?
        static_cast<int>(PairingStateChangeReason::PAIRING_FAILURE) : unpairedReason;
    if (preState == static_cast<int>(SlePairState::SLE_PAIR_CANCELING)) {
        HILOGI("preState is SLE_PAIR_CANCELING, so the reason of unpairing is PAIRING_LOCAL_CANCELED.");
        reason = static_cast<int>(PairingStateChangeReason::PAIRING_LOCAL_CANCELED);
    } else if (adapterProperties_->IsAudioDevice(otherAddr.GetAddress())) {
        CancelPairCompleteInner(otherAddr);
    }
    pimpl->credibleDevice_.Erase(reportAddr.GetAddress());
    /* 耳机恢复出厂后下云，删配对记录后不上报，上层应用不感知配对记录变化 */
    if (InterfaceCloudPairService::GetInstance().CancelCloudPairComplete(device, preState, reason,
            isCdsmAcbConnected, acbState)) {
        return;
    }
    /* 报取消配对 */
    NotifyPairStatusChanged(device, preState, static_cast<int>(SlePairState::SLE_PAIR_NONE), reason);
    DisconnectAction(device, static_cast<uint8_t>(SleDiscReason::SLE_DISC_REASON_CANCEL_PAIR));
    DftDeviceManager::GetInstance().DelDevice(device);
}

void SleAdapter::CancelPairCompleteInner(const RawAddress &device) const
{
    /* 删本地数据 */
    bool ret = SleConfig::GetInstance().RemovePairedDevice(device.GetAddress());
    DftDealUnPairLinkKeyInfo(device.GetAddress(), ret);
    if (!ret) {
        HILOGE("[SleAdapter]:config RemoveSection failed");
    } else {
        DftReportUnPairSuccessInfo(device.GetAddress(), 0, 0);
    }
    SleReconnectManager::GetInstance().RemoveAutoConnAudioDevConf(device);
    SleReconnectManager::GetInstance().OnDeviceUnpaired(device);
    SleConfig::GetInstance().Save();
    LOG_INFO("Erase device addr %{public}s.", GetEncryptAddr(device.GetAddress()).c_str());
    adapterProperties_->RemovePeripheralDevice(device.GetAddress());
    SleRemoteDeviceAdapter::GetInstance()->RemovePeerDeviceTypeToController(device);
}

void SleAdapter::PairComplete(const RawAddress &device, const int status) const
{
    HILOGI("[SleAdapter]:result %{public}d.", status);
    int pairState = static_cast<int>(SlePairState::SLE_PAIR_NONE);
    int connDirect = static_cast<int>(SleConnDirect::SLE_CONNECTION_PASSIVE);
    int pairDirect = static_cast<int>(SlePairDirect::SLE_PAIR_DEFAULT);
    if (status == SM_PAIR_OK) {
        PairCmpSuccess(device, pairState, connDirect, pairDirect);
    } else if (status == SM_PAIR_ERROR) {
        PairCmpFail(device, pairState);
    }
}

void SleAdapter::PairCmpSuccess(const RawAddress &device, int pairState, int connDirect, int pairDirect) const
{
    std::shared_ptr<const SlePeripheralDevice> remoteDevice = adapterProperties_->GetRemoteDevice(device);
    NL_CHECK_RETURN(remoteDevice, "addr %{public}s is not start.", GetEncryptAddr(device.GetAddress()).c_str());

    pairState = remoteDevice->GetPairedStatus();
    if (pairState == static_cast<int>(SlePairState::SLE_PAIR_PAIRING)) {
        connDirect = remoteDevice->GetConnDirect();
        pairDirect = remoteDevice->GetPairDirection();
        adapterProperties_->SetPrePairStatus(device, static_cast<int>(SlePairState::SLE_PAIR_PAIRING));
        adapterProperties_->SetPairStatus(device, static_cast<int>(SlePairState::SLE_PAIR_PAIRED));
    }

    NL_CHECK_RETURN_LOGD(pairState == static_cast<int>(SlePairState::SLE_PAIR_PAIRING), "not in pairing");
    NotifyPairStatusChanged(device, static_cast<int>(SlePairState::SLE_PAIR_PAIRING),
        static_cast<int>(SlePairState::SLE_PAIR_PAIRED),
        static_cast<uint8_t>(PairingStateChangeReason::PAIRING_SUCCESS));
    // 对端key missing，本端重新发起鉴权配对方向和上次保持一致
    if (pairDirect == static_cast<int>(SlePairDirect::SLE_PAIR_DEFAULT)) {
        adapterProperties_->SavePairDirect(connDirect, device);
    }
    LOG_INFO("[SleAdapter]:Save peer device info.");
    adapterProperties_->SavePeerDeviceInfoToConf();
    adapterProperties_->SaveDeviceManufacturerAbility(device);
    RawAddress reportAddr(device);
    CdsmService *cdsmService = CdsmService::GetService();
    NL_CHECK_RETURN(cdsmService, "ProfileCdsm is null.");
    cdsmService->CdsmGetReportAddr(device, reportAddr);
    pimpl->credibleDevice_.Erase(reportAddr.GetAddress());
}

void SleAdapter::PairCmpFail(const RawAddress &device, int pairState) const
{
    std::shared_ptr<const SlePeripheralDevice> remoteDevice = adapterProperties_->GetRemoteDevice(device);
    if (remoteDevice) {
        pairState = remoteDevice->GetPairedStatus();
        if (pairState == static_cast<int>(SlePairState::SLE_PAIR_PAIRING)) {
            adapterProperties_->SetPrePairStatus(device, static_cast<int>(SlePairState::SLE_PAIR_PAIRING));
            adapterProperties_->SetPairStatus(device, static_cast<int>(SlePairState::SLE_PAIR_NONE));
        }

        NL_CHECK_RETURN_LOGD(pairState == static_cast<int>(SlePairState::SLE_PAIR_PAIRING), "not in pairing");
        NotifyPairStatusChanged(device, static_cast<int>(SlePairState::SLE_PAIR_PAIRING),
            static_cast<int>(SlePairState::SLE_PAIR_NONE),
            static_cast<uint8_t>(PairingStateChangeReason::PAIRING_REMOTE_CANCELED));
    }
    DftCacheDisconnInfoMsg(device.GetAddress(), DFT_DISCONN_PAIRFAIL);
    DisconnectAction(device, static_cast<uint8_t>(SleDiscReason::SLE_DISC_REASON_REMOTE_USER_TERMINATED));
    RawAddress reportAddr(device);
    CdsmService *cdsmService = CdsmService::GetService();
    NL_CHECK_RETURN(cdsmService, "ProfileCdsm is null.");
    cdsmService->CdsmGetReportAddr(device, reportAddr);
    pimpl->credibleDevice_.Erase(reportAddr.GetAddress());
}

void SleAdapter::PairingStatus(const RawAddress &device) const
{
    HILOGD("[SleAdapter] enter");
    std::shared_ptr<const SlePeripheralDevice> remoteDevice = adapterProperties_->GetRemoteDevice(device);
    if (!remoteDevice) {
        HILOGI("[SleAdapter] add new device");
        std::shared_ptr<SlePeripheralDevice> peerDevice = std::make_shared<SlePeripheralDevice>();
        peerDevice->SetAddress(device);
        peerDevice->SetPrePairedStatus(static_cast<int>(SlePairState::SLE_PAIR_NONE));
        peerDevice->SetPairedStatus(static_cast<int>(SlePairState::SLE_PAIR_PAIRING));
        int appearance = InterfaceScanService::GetInstance().GetDeviceAppearance(device.GetAddress());
        if (appearance != INVALID_APPEARANCE) {
            HILOGD("[SleAdapter] new device SetAppearance:%{public}d", appearance);
            peerDevice->SetAppearance(appearance);
        }
        adapterProperties_->AddPeripheralDevice(device.GetAddress(), peerDevice);
    } else {
        adapterProperties_->SetPrePairStatus(device, remoteDevice->GetPairedStatus());
        adapterProperties_->SetPairStatus(device, static_cast<int>(SlePairState::SLE_PAIR_PAIRING));
    }
    // 云设备下云时已上报PAIRED, 首次连接(配对)时不需要再次上报
    if (InterfaceCloudPairService::GetInstance().IsCloudDeviceCreatePair(device)) {
        return;
    }
    NotifyPairStatusChanged(device, static_cast<int>(SlePairState::SLE_PAIR_NONE),
        static_cast<int>(SlePairState::SLE_PAIR_PAIRING),
        static_cast<uint8_t>(PairingStateChangeReason::PAIRING_SUCCESS));
}

/*************** 配对请求 start ***************/
void SleAdapter::PairingRequest(const RawAddress &device, std::string &passKey, const int type) const
{
    HILOGD("[SleAdapter] enter");
    /* 重新配对时停11s定时器 */

    pimpl->sleProfileConnectManager_.StopTimerIfRePair(device);
    RawAddress reportAddr(device);
    CdsmService *cdsmService = CdsmService::GetService();
    if (cdsmService != nullptr) {
        cdsmService->CdsmGetReportAddr(device, reportAddr);
    }
    bool supportBgConn = true;
    int appearance = SleRemoteDeviceAdapter::GetInstance()->GetDeviceAppearance(device);
    if (appearance == static_cast<int>(DeviceClassForService::DEVICE_CLASS_VEHICLE_LOCK) ||
        appearance == static_cast<int>(DeviceClassForService::DEVICE_NETWORKING)) {
            supportBgConn = false;
    }
    if (supportBgConn) {
        SleReconnectManager::GetInstance().OnDeviceStartPair(reportAddr);
    }
    NearlinkDftUe::GetInstance().WriteCommonUe(DFT_PAIRING_CFM_UE, device, PAIRINGREQ, type, "");
    if (adapterProperties_->IsCdsmMemberPair(device, reportAddr)) {
        /* 从设备确认允许配对 */
        uint8_t addrType = adapterProperties_->GetPeerDeviceAddrType(device);
        if (!pimpl->sleSecurity_.SetPairingConfirmation(device, addrType)) {
            HILOGE("[SleRemoteDeviceManager]:member pair confirm failed.");
        }
        HILOGW("[cdsm adatper]:pair request dialog,member:%{public}s not pull up dialog.",
            GetEncryptAddr(device.GetAddress()).c_str());
        NearlinkDftUe::GetInstance().WriteCommonUe(DFT_PAIRING_CFM_UE, device, PAIRINGREQ, NOPULL, "");
        return;
    }

    bool ret = pimpl->credibleDevice_.Find([&reportAddr](std::string credibleDeviceAdress) -> bool {
        return reportAddr.GetAddress() == credibleDeviceAdress ? true : false;
    });
    if (!ret) {
        NearlinkDftUe::GetInstance().WriteCommonUe(DFT_PAIRING_CFM_UE, device, PAIRINGREQ,
                                                   PULL, "");
#ifdef NO_PAIRING_DIALOG
        ProcessPairingRequest(reportAddr, passKey, type);
        return;
#endif
        PullUpDialog(reportAddr, passKey, type);
    } else {
        HILOG_COMM_INFO("[%{public}s:%{public}d]Don't pull the dialog, but call back.", __FUNCTION__, __LINE__);
        NearlinkDftUe::GetInstance().WriteCommonUe(DFT_PAIRING_CFM_UE, device, PAIRINGREQ, NOPULL, "");
        pimpl->slePeripheralCallback_.ForEach([&reportAddr, passKey, type](ISlePeripheralCallback &observer) -> void {
            observer.OnPairingRequest(reportAddr, passKey, type);
        });
        pimpl->credibleDevice_.Erase(reportAddr.GetAddress());
    }
}

#ifdef NO_PAIRING_DIALOG
void SleAdapter::ProcessPairingRequest(const RawAddress &device, std::string &passKey, const int type) const
{
    if (IsPairingDialogNeeded()) {
        PullUpDialog(device, passKey, type);
        return;
    }
    HILOGI("Don't pull the dialog, but call back.");
    NearlinkDftUe::GetInstance().WriteCommonUe(DFT_PAIRING_CFM_UE, device, PAIRINGREQ, NOPULL, "");
    pimpl->slePeripheralCallback_.ForEach([device, passKey, type](ISlePeripheralCallback &observer) -> void {
        observer.OnPairingRequest(device, passKey, type);
    });
    return;
}
#endif

void SleAdapter::PullUpDialog(const RawAddress &device, std::string &passKey, const int type) const
{
    HILOGI("Pull up the dialog");
    // 如果被动配对场景，为保证配对弹窗能显示对端设备名字，需先获取。
    RawAddress realAddr = adapterProperties_->GetRealAddress(device);
    std::shared_ptr<const SlePeripheralDevice> remoteDevice = adapterProperties_->GetRemoteDevice(realAddr);
    if (remoteDevice && remoteDevice->GetConnDirect() == static_cast<int>(SleConnDirect::SLE_CONNECTION_PASSIVE)) {
        HILOGI("Passive Pairing");
        PassivePairingGetRemoteName(realAddr, passKey, type);
        return;
    }
    if (DialogPairing::PullUpPairingDialog(device, passKey, type)) {
        HILOG_COMM_INFO("[%{public}s:%{public}d]Pull up dialog, type is %{public}d", __FUNCTION__, __LINE__, type);
    }
}

void SleAdapter::PassivePairingGetRemoteName(const RawAddress &device, const std::string &passKey, const int type) const
{
    auto func = [this, device, passKey, type]() -> void {
        if (DialogPairing::PullUpPairingDialog(device, passKey, type)) {
            HILOGI("Passive Pairing Pull up dialog, type is %{public}d", type);
        }
    };
    pimpl->passivePairingDialogFunc_ = func;
    auto timeoutFunc = [this, device]() -> void {
        HILOGE("Sle passive pairing read remote name timeout for address: %{public}s", GET_ENCRYPT_ADDR(device));
        DoInAdapterThread([this]() -> void {
            this->PassivePairingDialog();
        });
    };
    pimpl->slePassivePairingDialogTimer_ = std::make_shared<NearlinkTimer>(timeoutFunc);
    pimpl->slePassivePairingDialogTimer_->Start(SLE_PASSIVE_PAIRING_DIALOG_TIMEOUT_MS, false);
    DoInAdapterThread([this, device]() -> void {
        ReadDisDeviceNameByUuid(device);
    });
}

void SleAdapter::ReadDisDeviceNameByUuid(const RawAddress &device) const
{
    HILOGI("Enter");
    NL_CHECK_RETURN(!pimpl->passivePairingSsapConnInst_, "Already passive pairing read name");
    InterfaceProfileSsapClient *ssapClientService = pimpl->sleProfileConnectManager_.GetSsapClientService();
    NL_CHECK_RETURN(ssapClientService, "ssapClientService is null.");
    std::shared_ptr<ServiceSsapConnectInst> ssapConnInst = std::make_shared<ServiceSsapConnectInst>(device);
    int appId = ssapClientService->RegisterApplication(ssapConnInst->GetSsapClientCallback(),
                                                        device, SleTransport::ADAPTER_SLE, SSAP_SEC_NONE);
    NL_CHECK_RETURN(appId >= 0, "invalid appId.");
    NL_CHECK_RETURN(ssapClientService->Connect(appId, false) == static_cast<int>(ReturnValue::RET_NO_ERROR),
                    "ssap connect faild");
    ssapConnInst->SetAppId(appId);
    pimpl->passivePairingSsapConnInst_ = ssapConnInst;
}

void SleAdapter::ServiceSsapConnectInst::ServiceSsapCallback::OnConnectionStateChanged(
    uint8_t state, int ret)
{
    HILOGI("Enter");
    NL_CHECK_RETURN(g_sleAdapterImpl, "g_sleAdapterImpl is null");
    DoInAdapterThread([sleAdapterImpl = g_sleAdapterImpl, this, state]() -> void {
        sleAdapterImpl->OnSsapConnectionStateChangedTask(device_, state);
    });
}

void SleAdapter::OnSsapConnectionStateChangedTask(const RawAddress &device, uint8_t newState)
{
    HILOGI("Enter");
    NL_CHECK_RETURN(pimpl->passivePairingSsapConnInst_, "passivePairingSsapConnInst_ is null");
    NL_CHECK_RETURN(pimpl->passivePairingSsapConnInst_->GetAddr() == device,
                    "invalid adress %{public}s", GET_ENCRYPT_ADDR(device));
    if (newState == static_cast<int>(SleConnectState::CONNECTED)) {
        InterfaceProfileSsapClient *ssapClientService = pimpl->sleProfileConnectManager_.GetSsapClientService();
        NL_CHECK_RETURN(ssapClientService, "ssapClientService is null.");
        int ret = ssapClientService->ReadPropertiesByUuid(
            pimpl->passivePairingSsapConnInst_->GetAppId(), Uuid::ConvertFrom16Bits(DIS_UUID_SSAP_DEVICE_NAME_ID),
            MIN_ENTRY_HANDLE, MAX_ENTRY_HANDLE);
        NL_CHECK_RETURN(ret == SsapStatus::SSAP_SUCCESS, "Read name by uuid fail, ret is %{public}d.", ret);
    }
}

void SleAdapter::ServiceSsapConnectInst::ServiceSsapCallback::OnReadPropertiesByUuid(std::list<Property> &list, int ret)
{
    HILOGI("Enter");
    NL_CHECK_RETURN(g_sleAdapterImpl, "g_sleAdapterImpl is null");
    DoInAdapterThread([sleAdapterImpl = g_sleAdapterImpl, this, list, ret]() -> void {
        sleAdapterImpl->OnSsapReadPropertiesByUuidTask(device_, list, ret);
    });
}

void SleAdapter::OnSsapReadPropertiesByUuidTask(const RawAddress &device, const std::list<Property> &list, int ret)
{
    HILOGI("Enter");
    NL_CHECK_RETURN(ret == static_cast<int>(SsapStatus::SSAP_SUCCESS), "ReadPropertiesByUuid fail");
    NL_CHECK_RETURN(pimpl->passivePairingSsapConnInst_, "passivePairingSsapConnInst_ is null");
    NL_CHECK_RETURN(pimpl->passivePairingSsapConnInst_->GetAddr() == device,
                    "invalid adress %{public}s", GET_ENCRYPT_ADDR(device));
    NL_CHECK_RETURN(list.size() > 0, "list size <= 0");
    Property tempProperty(list.front());
    NL_CHECK_RETURN(tempProperty.uuid_ == Uuid::ConvertFrom16Bits(DIS_UUID_SSAP_DEVICE_NAME_ID),
                    "uuid is not DIS_UUID_SSAP_DEVICE_NAME_ID");
    NL_CHECK_RETURN(tempProperty.value_.size() > 0,
                    "property length is error : %{public}zu", tempProperty.value_.size());

    char *data = reinterpret_cast<char *>(tempProperty.value_.data());
    std::string name(data, data + tempProperty.value_.size());
    g_sleAdapterImpl->adapterProperties_->SetName(device, name);
    PassivePairingDialog();
}

void SleAdapter::DeregisterServiceSsapApplication() const
{
    HILOGI("Enter");
    InterfaceProfileSsapClient *ssapClientService = pimpl->sleProfileConnectManager_.GetSsapClientService();
    NL_CHECK_RETURN(ssapClientService, "ssapClientService is null.");
    if (pimpl->passivePairingSsapConnInst_) {
        int ret = ssapClientService->DeregisterApplication(pimpl->passivePairingSsapConnInst_->GetAppId());
        pimpl->passivePairingSsapConnInst_ = nullptr;
        NL_CHECK_RETURN(ret == static_cast<int>(SsapStatus::SSAP_SUCCESS),
                        "Deregister ssap client service error, result:%{public}d", ret);
    }
}

void SleAdapter::PassivePairingDialog() const
{
    HILOGI("Eenter");
    if (pimpl->slePassivePairingDialogTimer_ != nullptr) {
        pimpl->slePassivePairingDialogTimer_->Stop();
        pimpl->slePassivePairingDialogTimer_ = nullptr;
    }
    DoInAdapterThread([this]() -> void {
        this->DeregisterServiceSsapApplication();
    });

    if (pimpl->passivePairingDialogFunc_ != nullptr) {
        pimpl->passivePairingDialogFunc_();
        pimpl->passivePairingDialogFunc_ = nullptr;
    }
}
/*************** 配对请求 end ***************/

void SleAdapter::EncryptionKeyMissingComplete(const RawAddress &device) const
{
    HILOGI("[SleAdapter] device:%{public}s", GET_ENCRYPT_ADDR(device));
    NearlinkDftUe::GetInstance().WriteCommonUe(DFT_PAIRING_CANCEL_UE, device, KEYMISSING, CANCELPAIRTYPEINVALID, "");
    bool isVendorDevice = adapterProperties_->IsVendorDevice(device);
    std::shared_ptr<const SlePeripheralDevice> remoteDevice = adapterProperties_->GetRemoteDevice(device);
    NL_CHECK_RETURN(remoteDevice, "[SleAdapter] not find peerConnDevice.");

    adapterProperties_->SetPrePairStatus(device, remoteDevice->GetPairedStatus());
    adapterProperties_->SetPairStatus(device, static_cast<int>(SlePairState::SLE_PAIR_PAIRING));
    ProfileCdsm *cdsmService = static_cast<ProfileCdsm *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_CDSM));
    NL_CHECK_RETURN(cdsmService, "ProfileCdsm is null.");
    if (!cdsmService->CdsmCheckIsCooperationDevice(device)) {
        return;
    }
    if (isVendorDevice) {
        UpdateKeyMissingCdsmGroup(device);
        InterfaceCloudPairService::GetInstance().SetKeyMissingPairState(device);
    } else {
        RawAddress member;
        if (cdsmService->CdsmGetOtherAddr(device, member)) {
            HILOGI("[SleAdapter]cdsm report device(%{public}s) is keymissing, cancel member(%{public}s) acb req",
                GET_ENCRYPT_ADDR(device), GET_ENCRYPT_ADDR(member));
            DisconnectAction(member,
                static_cast<uint8_t>(SleDiscReason::SLE_DISC_REASON_REMOTE_USER_TERMINATED));
        }
    }
}

void SleAdapter::UpdateKeyMissingCdsmGroup(const RawAddress &device) const
{
    // 主耳Keymissing, 删旧副耳配对记录，删旧cdsm合作集，删旧副耳连接白名单
    CdsmService *cdsmService = CdsmService::GetService();
    NL_CHECK_RETURN(cdsmService, "[SleAdapter]cdsmService is null.");
    RawAddress realAddr = adapterProperties_->GetRealAddress(device);
    RawAddress otherAddr;
    NL_CHECK_RETURN(cdsmService->CdsmGetOtherAddr(realAddr, otherAddr), "get other addr fail before remove pair");
    CancelPairCompleteInner(otherAddr);
    DisconnectAcb(otherAddr, static_cast<uint8_t>(SleDiscReason::SLE_DISC_REASON_CANCEL_PAIR));
    cdsmService->CdsmDeleteGroup(realAddr);
    HILOGI("KeyMissing and %{public}s remove pair", GET_ENCRYPT_ADDR(otherAddr));

    // 重建cdsm合作集，新建副耳配对记录
    RawAddress reportAddr = InterfaceScanService::GetInstance().GetReportAddrByCurrentAddress(realAddr);
    ProcCreateCdsmGroup(reportAddr, realAddr, false);
    NL_CHECK_RETURN(cdsmService->CdsmGetOtherAddr(realAddr, otherAddr), "get other addr fail before update pair");
    adapterProperties_->CdsmAddOtherRecord(realAddr, otherAddr);
    HILOGI("KeyMissing and %{public}s update pair record", GET_ENCRYPT_ADDR(otherAddr));
}

void SleAdapter::EncryptionComplete(const RawAddress &device, const int status) const
{
    LOG_DEBUG("[SleAdapter] enter");
    std::shared_ptr<const SlePeripheralDevice> remoteDevice = adapterProperties_->GetRemoteDevice(device);
    if (remoteDevice) {
        if (status == SM_PAIR_ERROR) {
            HILOG_COMM_ERROR("[%{public}s:%{public}d][SleAdapter] encry fail", __FUNCTION__, __LINE__);
            DftDealEncryptFailEvent(device.GetAddress(), status);
            adapterProperties_->DisconnectAcbAction(device,
                static_cast<uint8_t>(SleDiscReason::SLE_DISC_REASON_REMOTE_USER_TERMINATED));
            LOG_ERROR("[SleAdapter] set needReconnetDevice");
            pimpl->needReconnectDevices_.Emplace(device.GetAddress());
            return;
        } else if (status == SM_KEY_MISSING) {
            EncryptionKeyMissingComplete(device);
            return;
        } else if (status == SM_LINK_DISCONNCTED) {
            return;
        }
    }
    DftCachePairConnTime(device.GetAddress(), PAIR_CONN_PATH_ENCYP, SLE_PAIR_FINISH_TIME);
    if (remoteDevice) {
        int connDirect = remoteDevice->GetConnDirect();
        bool isCdsmMember = remoteDevice->IsCdsmMember();
        int businessType = remoteDevice->GetManufacturerBusiness();
        adapterProperties_->SetAcbState(device.GetAddress(),
            static_cast<int>(SleConnState::SLE_CONNECTION_STATE_ENCRYPTED));

        OnAcbStateChanged(device, static_cast<int>(SleConnState::SLE_CONNECTION_STATE_ENCRYPTED), ACB_CONNECT_SUCCESS);
        if ((connDirect == static_cast<int>(SleConnDirect::SLE_CONNECTION_PASSIVE)) ||
            ((pimpl->sleProfileConnectManager_.GetProfileConnectState(device) ==
                SLE_ADAPTER_PROF_CONN_STATE_ACB_CONNECTING)) || isCdsmMember) {
            pimpl->sleProfileConnectManager_.SleConnectAllProfile(device);
        }

        /* 私有外设更新默认角色 */
        if (businessType == Nearlink::SLE_PRIVATE_AUDIO_BUSINESS_TYPE) {
            adapterProperties_->UpdateDefaultRole(device);
        }
    }
}

int SleAdapter::ReadRemoteRssiValue(const RawAddress &device)
{
    DoInAdapterThread([this, device]() -> void {
        uint16_t lcid = adapterProperties_->GetLcidByAddress(device);
        CM_ReadRemoteRssiReq_S param = {};
        param.lcid = lcid;
        NLSTK_ERRCODE ret = CM_GetRssi(&param);
        if (ret != CM_SUCCESS) {
            HILOGE("read rssi fail");
        }
    });
    return NLSTK_ERRCODE_SUCCESS;
}

bool SleAdapter::IsScanConnTypeAndFrameType4(const RawAddress &device, uint8_t connCompleteType) const
{
    bool result = false;
    uint8_t frameType = static_cast<uint8_t>(SleConnFrameType::SLE_CONN_FRAME_TYPE_1);
    if ((connCompleteType == CM_CONN_COMPLETE_SCAN) &&
        GetConnFrameType(device.GetAddress(), frameType) &&
        (frameType == static_cast<uint8_t>(SleConnFrameType::SLE_CONN_FRAME_TYPE_4))) {
        result = true;
    }
    HILOGD("%{public}s, isScanConnTypeAndFrameType4=%{public}d", GetEncryptAddr(device.GetAddress()).c_str(), result);
    return result;
}

/* 连接状态瞬态计数 */
int SleAdapter::GetUnDisconnectedCnt(const RawAddress &device) const
{
    int unDisconnectedNum = 0;
    CdsmService *cdsmService = CdsmService::GetService();
    std::vector<NearlinkCdsmInfo> cdsmInfo = {};
    if (cdsmService == nullptr ||
        cdsmService->CdsmGetAllMemberInfo(device, cdsmInfo) != NL_NO_ERROR) {
        return unDisconnectedNum;
    }

    for (auto &member : cdsmInfo) {
        /* 一个成员profile断开完成，另一个成员加密中acb还没有断开，不应该上报断连完成给应用 */
        if (pimpl->sleProfileConnectManager_.GetConnectedProfileNum(member.addr_) == 0) {
            int acbConnState = adapterProperties_->GetAcbState(member.addr_.GetAddress());
            if (acbConnState == static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTING) ||
                acbConnState == static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTING) ||
                acbConnState == static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED)) {
                unDisconnectedNum++;
            }
        } else {
            unDisconnectedNum++;
        }
    }

    return unDisconnectedNum;
}

/* 连接状态稳态计数 */
int SleAdapter::GetConnectedCnt(const RawAddress &device) const
{
    int connectedNum = 0;
    CdsmService *cdsmService = CdsmService::GetService();
    std::vector<NearlinkCdsmInfo> cdsmInfo = {};
    if (cdsmService == nullptr ||
        cdsmService->CdsmGetAllMemberInfo(device, cdsmInfo) != NL_NO_ERROR) {
        return connectedNum;
    }

    for (auto &member : cdsmInfo) {
        if (pimpl->sleProfileConnectManager_.IsAllProfileConnected(member.addr_)) {
            connectedNum++;
        }
    }

    return connectedNum;
}

/* 是否上报profile连接状态 */
bool SleAdapter::IsProfileStateReport(const RawAddress &device, RawAddress &reportAddr,
    CdsmService *cdsmService, int newConnState) const
{
    int connectedCnt = GetConnectedCnt(device); /* 稳态已连接计数 */
    int unDisconnectedCnt = GetUnDisconnectedCnt(device); /* 断连瞬态连接计数 */
    bool isNeedReport = false;

    switch (newConnState) {
        case static_cast<int>(SleConnectState::CONNECTED): /* 已连接，已连接数大于1时上报 */
            if (connectedCnt >= 1) {
                isNeedReport = true;
            }
            break;
        case static_cast<int>(SleConnectState::CONNECTING):
            if (connectedCnt == 0) {
                isNeedReport = true;
            }
            break;
        case static_cast<int>(SleConnectState::DISCONNECTING): /* 断开中，已连接数是0时上报 */
            if (unDisconnectedCnt == 1) {
                isNeedReport = true;
            }
            break;
        case static_cast<int>(SleConnectState::DISCONNECTED):  /* 已断连，已连接数为0时上报 */
            if (unDisconnectedCnt == 0) {
                isNeedReport = true;
            }
            break;
        default:
            HILOGE("[sle adapter]:device:%{public}s state invalid:%{public}d",
                GetEncryptAddr(device.GetAddress()).c_str(), newConnState);
            break;
    }

    if (isNeedReport) {
        RawAddress cdsmReport;
        NL_CHECK_RETURN_RET(cdsmService->CdsmGetReportAddr(device, cdsmReport), isNeedReport,
            "[sle adapter]:need report profile state,but get cdsm report addr fail,addr:%{public}s",
            GetEncryptAddr(device.GetAddress()).c_str());
        reportAddr = cdsmReport; /* 将上报地址映射成report地址 */
        HILOGD("[sle adapter]:need report profile state,report addr:%{public}s,"
            "state:%{public}d,conn:%{public}d,unDis:%{public}d", GetEncryptAddr(device.GetAddress()).c_str(),
            newConnState, connectedCnt, unDisconnectedCnt);
    }

    return isNeedReport;
}

/* 清理合作集数据 */
void SleAdapter::ClearAllCdsmData(const RawAddress &device) const
{
    int unDisconnCnt = GetUnDisconnectedCnt(device); /* 连接瞬态计数 */
    int notPairNoneCnt = adapterProperties_->GetNotPairNoneCnt(device); /* 配对瞬态计数 */
    HILOGI("[sleAdapter]:dev:%{public}s,undisconnected:%{public}d,uncancel pair completed:%{public}d",
        GET_ENCRYPT_ADDR(device), unDisconnCnt, notPairNoneCnt);

    CdsmService *cdsmService = CdsmService::GetService();
    if (cdsmService != nullptr && notPairNoneCnt == 0 && unDisconnCnt == 0) {
        RawAddress reportAddr(device);
        cdsmService->CdsmGetReportAddr(device, reportAddr);
        std::vector<NearlinkCdsmInfo> cdsmInfo = {};
        if (cdsmService->CdsmGetAllMemberInfo(device, cdsmInfo) != NL_NO_ERROR) {
            HILOGW("[sleAdapter]: get cdsm member info failed");
            cdsmService->CdsmDeleteGroup(device);
            return;
        }
        for (const auto &member : cdsmInfo) {
            if (!SleConfig::GetInstance().GetLinkKey(member.addr_.GetAddress()).empty()) {
                HILOGI("[sleAdapter]: device:%{public}s is paired", GET_ENCRYPT_ADDR(member.addr_));
                return;
            }
        }
        cdsmService->CdsmDeleteGroup(device);
    }
}

void SleAdapter::NotifyConnectionStateChanged(
    const RawAddress &device, const SleConnectionChangedParam &connChangedParam) const
{
    HILOGI("[sleAdapter]:addr:%{public}s, profile connection state change:%{public}d->%{public}d, reason:%{public}d",
        GET_ENCRYPT_ADDR(device), connChangedParam.connPreState,
        connChangedParam.connState, connChangedParam.connReason);
    if (connChangedParam.connState == static_cast<int>(SleConnectState::CONNECTED)) {
        SleReconnectManager::GetInstance().OnDeviceConnected(device,
            adapterProperties_->IsServiceSupportedConn(device));
        if (adapterProperties_->IsAudioDevice(device.GetAddress())) {
            SleAcbSubrateParam subrateParam;
            subrateParam.onlySubrate = true;
            subrateParam.subrate = NLSTK_DEFAULT_SUBRATE;
            bool ret = false;
            ServiceManagerPluginInterface::GetInstance()->SetAcbSubrate(ret, device, subrateParam);
            if (!ret) {
                HILOGE("set acb subrate failed");
            }
        }
    }
    RawAddress reportAddr(device);
    if (!HandleCdsmServiceConnectionState(device, reportAddr, connChangedParam)) {
        return;
    }
    int reason = HandleConnectionStateReason(connChangedParam);

    pimpl->slePeripheralCallback_.ForEach([reportAddr, connChangedParam, reason](
        ISlePeripheralCallback &observer) {
        LOG_DEBUG("OnConnectionStateChanged, newConnState=%{public}d, oldConnState=%{public}d, reason=%{public}d",
            connChangedParam.connState, connChangedParam.connPreState, reason);
        observer.OnConnectionStateChanged(
            reportAddr, connChangedParam.connState, connChangedParam.connPreState, reason);
    });
    if (connChangedParam.connState == static_cast<int>(SleConnectState::CONNECTED)) {
        ServiceManagerPluginInterface::GetInstance()->SleTvMgrProc(device.GetAddress());
    }
}

int SleAdapter::HandleConnectionStateReason(const SleConnectionChangedParam &connChangedParam) const
{
    int disconnReason = pimpl->sleDisconnReason_.load();
    int reason = static_cast<int>(SleConnectReason::CONNECT_SUCCESS);
    if (connChangedParam.connState == static_cast<int>(SleConnectState::DISCONNECTED) ||
        connChangedParam.connState == static_cast<int>(SleConnectState::DISCONNECTING)) {
        reason = disconnReason >= 0 ? disconnReason : connChangedParam.connReason;
    }
    HILOGI("[SleAdapter] sleDisconnReason_: %{public}d, report reason: %{public}d",
        pimpl->sleDisconnReason_.load(), reason);
    if (connChangedParam.connState != static_cast<int>(SleConnectState::DISCONNECTING) &&
        disconnReason != static_cast<int>(SleConnectReason::CONNECT_NONE)) {
        // 连接状态变化后重置缓存的断连原因，DISCONNECTING状态除外，断连过程中需要保持缓存的断连原因。
        pimpl->sleDisconnReason_.store(static_cast<int>(SleConnectReason::CONNECT_NONE));
    }
    return reason;
}

bool SleAdapter::HandleCdsmServiceConnectionState(
    const RawAddress &device, RawAddress &reportAddr, const SleConnectionChangedParam &connChangedParam) const
{
    int newConnState = connChangedParam.connState;
    int oldConnState = connChangedParam.connPreState;
    CdsmService *cdsmService = CdsmService::GetService();
    if (cdsmService != nullptr && cdsmService->CdsmCheckIsCooperationDevice(device)) {
        cdsmService->UpdateCdsmMemberProfileConnectState(device, static_cast<SleConnectState>(newConnState));
        /* 是否上报 */
        NL_CHECK_RETURN_RET(IsProfileStateReport(device, reportAddr, cdsmService, newConnState), false, "");
        DeviceBatteryManager::GetInstance().ProcessProfileStateChanged(reportAddr.GetAddress(), newConnState);
        /* 清理合作集数据 */
        if (newConnState == static_cast<int>(SleConnectState::DISCONNECTED)) {
            ClearAllCdsmData(device);
        }
        int reportConnectState = static_cast<int>(SleConnectState::INVALID_STATE);
        cdsmService->CdsmGetReportedState(device, reportConnectState);
        if (reportConnectState == newConnState) {
            HILOGD("newConnState=%{public}d has already reported", newConnState);
            return false;
        }
        /* 保障连接状态上报的连续性 */
        if (reportConnectState != static_cast<int>(SleConnectState::INVALID_STATE)) {
            oldConnState = reportConnectState;
        }
        cdsmService->UpdateReportConnectState(reportAddr, newConnState);
    } else if (newConnState == oldConnState) {
        return false;
    }
    return true;
}

void SleAdapter::ConnectAcb(const RawAddress &device)
{
    HILOGI("[SleRemoteDeviceAdapter] enter");
    DftCacheAcbStartConn(device.GetAddress());
    SleRemoteDeviceAdapter::GetInstance()->SleAddPeerList(device);

    SleRemoteDeviceAdapter::GetInstance()->SetPeerDeviceTypeToController(device);
    CM_DirectConnAddrParam_S param = {};
    device.ConvertToUint8(param.addr.addr);
    param.addr.type = SleRemoteDeviceAdapter::GetInstance()->GetPeerDeviceAddrType(device);
    param.frameType = CM_CONN_PARAM_FRAME_TYPE_1;
    uint32_t ret = CM_DirectConnectAdd(CM_MODULE_ADPT, &param);
    NL_CHECK_RETURN(ret == CM_SUCCESS, "ret=%{public}d", ret);
}

bool SleAdapter::DisconnectAction(const RawAddress &device, uint8_t discReason) const
{
    pimpl->needReconnectDevices_.Erase(device.GetAddress());
    return adapterProperties_->DisconnectAcbAction(device, discReason);
}

bool SleAdapter::DisconnectAcb(const RawAddress &device, uint8_t discReason) const
{
    HILOGI("[SleRemoteDeviceAdapter] enter");
    SLE_Addr_S stackAddr = {};
    device.ConvertToUint8(stackAddr.addr);
    stackAddr.type = SleRemoteDeviceManager::GetInstance()->GetPeerDeviceAddrType(device);
    uint32_t ret = CM_DirectConnectRemove(CM_MODULE_ADPT, &stackAddr, discReason);
    NL_CHECK_RETURN_RET(ret == CM_SUCCESS, false, "ret=%{public}d", ret);
    return true;
}

void SleAdapter::ClearBgConnDevice() const
{
    uint32_t ret = CM_BackgroundConnectClear(CM_MODULE_ADPT);
    NL_CHECK_RETURN(ret == CM_SUCCESS, "ret=%{public}d", ret);
}

bool SleAdapter::IsPairStateReport(const RawAddress &device, RawAddress &reportAddr,
    CdsmService *cdsmService, int pairStatus) const
{
    int notPairNoneCnt = adapterProperties_->GetNotPairNoneCnt(device);   /* 取消配对、配对中等瞬态计数 */
    int pairedNum = adapterProperties_->GetPairedCnt(device);       /* 已配对稳态计数 */
    bool isNeedReport = false;

    switch (pairStatus) {
        case static_cast<int>(SlePairState::SLE_PAIR_PAIRED): /* 3: 已配对 */
            if (pairedNum >= 1) {
                isNeedReport = true;
            }
            break;
        case static_cast<int>(SlePairState::SLE_PAIR_PAIRING): /* 2: 配对中 */
            if (pairedNum == 0) {
                isNeedReport = true;
            }
            break;
        case static_cast<int>(SlePairState::SLE_PAIR_CANCELING): /* 4: 取消配对中 */
            if (notPairNoneCnt == 1) {
                isNeedReport = true;
            }
            break;
        case static_cast<int>(SlePairState::SLE_PAIR_NONE): /* 1: 未配对 */
            if (notPairNoneCnt == 0) {
                isNeedReport = true;
            }
            break;
        default:
            HILOGE("[sle adapter]:device:%{public}s state invalid:%{public}d",
                GetEncryptAddr(device.GetAddress()).c_str(), pairStatus);
            break;
    }

    if (isNeedReport) {
        RawAddress cdsmReport;
        NL_CHECK_RETURN_RET(cdsmService->CdsmGetReportAddr(device, cdsmReport), isNeedReport,
            "[sle adapter]:need report pair state,but get cdsm report addr fail,addr:%{public}s",
            GetEncryptAddr(device.GetAddress()).c_str());
        reportAddr = cdsmReport; /* 将上报地址映射成report地址 */
        HILOGI("[sle adapter]:need report pair state,report addr:%{public}s,"
            "status:%{public}d,paired:%{public}d,unCancel:%{public}d", GetEncryptAddr(device.GetAddress()).c_str(),
            pairStatus, pairedNum, notPairNoneCnt);
    }

    return isNeedReport;
}

void SleAdapter::NotifyPairStatusChanged(const RawAddress &device, int preStatus, int status, int reason) const
{
    LOG_INFO("[sle adapter]:pair state change,status:%{public}d->%{public}d,reason:%{public}u,addr:%{public}s",
        preStatus, status, reason, GetEncryptAddr(device.GetAddress()).c_str());

    RawAddress reportAddr(device);
    CdsmService *cdsmService = CdsmService::GetService();
    if (cdsmService != nullptr && cdsmService->CdsmCheckIsCooperationDevice(device)) {
        bool replaceOldReportAddr = InterfaceCloudPairService::GetInstance().IsInReplacing(device);
        if (!replaceOldReportAddr && !IsPairStateReport(device, reportAddr, cdsmService, status)) {
            return;
        }

        /* 清理合作集数据 */
        if (!replaceOldReportAddr && status == static_cast<int>(SlePairState::SLE_PAIR_NONE)) {
            ClearAllCdsmData(device);
        }

        // 从SLE_PAIR_PAIRING->SLE_PAIR_NONE时，表示配对流程被中断，需要删除对应的合作集信息
        // 场景：配对中，配对失败，那么公版耳机提前创建的合作集gid（由于此时只有report地址的信息来建合作集）
        // 如果后面在进行双耳互换在发起配对，又会重新创建一次合作集gid，而且会与上面的gid不同，当成了两个合作集组
        // 就会导致在合作集中查询时cdsm member addr.*in multi group
        if (preStatus == static_cast<int>(SlePairState::SLE_PAIR_PAIRING) &&
            status == static_cast<int>(SlePairState::SLE_PAIR_NONE)) {
            LOG_INFO("[SleAdapter]device:%{public}s pair state invalid(%{public}d->%{public}d), should delete group.",
                GetEncryptAddr(device.GetAddress()).c_str(), preStatus, status);
            cdsmService->CdsmDeleteGroup(device);
        }
    }
    InterfaceCloudPairService::GetInstance().HandlePairStatusChanged(reportAddr, preStatus, status, reason);
    pimpl->slePeripheralCallback_.ForEach([reportAddr, preStatus, status, reason](ISlePeripheralCallback &observer) {
        observer.OnPairStatusChanged(reportAddr, preStatus, status, reason);
    });
}

bool SleAdapter::ConnectAllProfile(const RawAddress &device)
{
    DftCachePairConnType(device.GetAddress(), NO_PAIR, CLICK_CONN);
    DftCacheCallingName(device.GetAddress(), NearLinkPermissionManager::GetCallingName());
    DoInAdapterThread([this, device]() -> void {
        RawAddress realAddr = adapterProperties_->GetRealAddress(device);
        this->ConnectAllProfileTask(realAddr);
    });
    return true;
}

bool SleAdapter::ProcCdsmDeviceConnect(const RawAddress &device)
{
    CdsmService *cdsmService = CdsmService::GetService();
    if (cdsmService == nullptr || !cdsmService->CdsmCheckIsCooperationDevice(device)) {
        HILOGW("not Cooperation Device");
        return false;
    }

    std::vector<NearlinkCdsmInfo> cdsmInfo = {};
    NL_CHECK_RETURN_RET(cdsmService->CdsmGetAllMemberInfo(device, cdsmInfo) == NL_NO_ERROR, false,
        "get cdsm info by member addr failed!");

    for (const auto& member : cdsmInfo) {
        ConnectAllProfileInner(member.addr_);
    }
    return true;
}

void SleAdapter::ConnectAllProfileInner(const RawAddress &device)
{
    std::shared_ptr<const SlePeripheralDevice> remoteDevice = adapterProperties_->GetRemoteDevice(device);
    NL_CHECK_RETURN(remoteDevice, "[SleAdapter] not find peerConnDevice.");
    int pairState = remoteDevice->GetPairedStatus();
    int acbConnState = remoteDevice->GetAcbConnectState();
    NL_CHECK_RETURN(pairState == static_cast<int>(SlePairState::SLE_PAIR_PAIRED), "[SleAdapter] not PAIRED.");
    if (acbConnState == static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED)) {
        ConnectAcb(device);
        pimpl->sleProfileConnectManager_.NotifyConnectAcb(device);
        HILOGI("connect member dev:%{public}s", GET_ENCRYPT_ADDR(device));
    }
}

void SleAdapter::ConnectAllProfileTask(const RawAddress &device)
{
    HILOGI("[Enter]:Device:%{public}s", GET_ENCRYPT_ADDR(device));
    if (InterfaceCloudPairService::GetInstance().ConnectCloudDeviceAllProfile(device)) {
        return;
    }

    std::shared_ptr<const SlePeripheralDevice> remoteDevice = adapterProperties_->GetRemoteDevice(device);
    NL_CHECK_RETURN(remoteDevice, "[SleAdapter] not find peerConnDevice.");
    int pairState = remoteDevice->GetPairedStatus();
    int acbConnState = remoteDevice->GetAcbConnectState();
    int bussinessType = remoteDevice->GetManufacturerBusiness();
    int appearance = remoteDevice->GetAppearance();

    NL_CHECK_RETURN(pairState == static_cast<int>(SlePairState::SLE_PAIR_PAIRED), "[SleAdapter] not PAIRED.");
    if (acbConnState == static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED)) {
        if (appearance == static_cast<int>(DeviceClassForService::DEVICE_CLASS_VEHICLE_LOCK)) {
            SleDataTransferService::GetInstance().StopNlProxyIfExisted();
        }
        if (ProcCdsmDeviceConnect(device)) {
            return;
        }
        ConnectAcb(device);
        pimpl->sleProfileConnectManager_.NotifyConnectAcb(device);
        return;
    } else if (acbConnState == static_cast<int>(SleConnState::SLE_CONNECTION_STATE_ENCRYPTED)) {
        if (pimpl->sleProfileConnectManager_.GetSilentPortState(device)) {
            HILOGI("[cdsm adapter]:silent port is running, ignore connect request.");
            SleConnectionChangedParam connChangedParam(
                static_cast<int>(SleConnectState::CONNECTING), static_cast<int>(SleConnectState::DISCONNECTED));
            NotifyConnectionStateChanged(device, connChangedParam);
            connChangedParam = SleConnectionChangedParam{
                static_cast<int>(SleConnectState::DISCONNECTED), static_cast<int>(SleConnectState::CONNECTING)};
            NotifyConnectionStateChanged(device, connChangedParam);
            return;
        }
        pimpl->sleProfileConnectManager_.SleConnectAllProfile(device);

        /* 添加设备记录 */
        CdsmService *cdsmService = CdsmService::GetService();
        RawAddress otherAddr;
        if (cdsmService != nullptr && cdsmService->CdsmGetOtherAddr(device, otherAddr) &&
            bussinessType == Nearlink::SLE_PRIVATE_AUDIO_BUSINESS_TYPE) {
            int otherAcbConnState = GetAcbStateTask(otherAddr);
            if (otherAcbConnState == static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED) ||
                otherAcbConnState == static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTING)) {
                HILOGI("[cdsm adapter]:add cdsm pair record,dev:%{public}s,other addr:%{public}s",
                    GET_ENCRYPT_ADDR(device), GET_ENCRYPT_ADDR(otherAddr));
                adapterProperties_->CdsmAddOtherRecord(device, otherAddr);
            }
        }
        return;
    } else if (acbConnState == static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTING)) {
        pimpl->sleProfileConnectManager_.NotifyConnectAcb(device);
        return;
    }
    HILOGE("[SleAdapter] dev:%{public}s state error! acbConnState:%{public}d", GET_ENCRYPT_ADDR(device), acbConnState);
}

bool SleAdapter::DisconnectAllProfile(const RawAddress &device)
{
    std::promise<void> promise;
    bool result = false;
    HILOGI("device:%{public}s", GetEncryptAddr(device.GetAddress()).c_str());
    DoInAdapterThread([this, device, &promise, &result]() {
        if (!ProcCdsmDisconnectAllProfile(device, result,
            static_cast<uint8_t>(SleDiscReason::SLE_DISC_REASON_REMOTE_USER_TERMINATED))) {
            DisconnectAllProfileInner(device, result,
                static_cast<uint8_t>(SleDiscReason::SLE_DISC_REASON_REMOTE_USER_TERMINATED));
        }
        promise.set_value();
    });
    promise.get_future().get();
    return result;
}

void SleAdapter::DisconnectAllProfileInner(const RawAddress &device, bool &result, uint8_t discReason)
{
    bool isAcbConnect = false;
    std::shared_ptr<const SlePeripheralDevice> remoteDevice = adapterProperties_->GetRemoteDevice(device);
    if (remoteDevice && (remoteDevice->IsAcbConnected() || remoteDevice->IsAcbConnecting())) {
        DftCacheDisconnInfoMsg(device.GetAddress(), DFT_DISCONN_DISCONNALLPROFILES);
        result = DisconnectAction(device, discReason);
        HILOGI("disconnect dev:%{public}s", GET_ENCRYPT_ADDR(device));
    }
}

bool SleAdapter::ProcCdsmDisconnectAllProfile(const RawAddress &device, bool &result, uint8_t discReason)
{
    CdsmService *cdsmService = CdsmService::GetService();
    if (cdsmService == nullptr || !cdsmService->CdsmCheckIsCooperationDevice(device)) {
        HILOGW("not Cooperation Device");
        return false;
    }

    std::vector<NearlinkCdsmInfo> cdsmInfo;
    NL_CHECK_RETURN_RET(cdsmService->CdsmGetAllMemberInfo(device, cdsmInfo) == NL_NO_ERROR, false,
        "get cdsm info by member addr failed!");

    for (const auto& member : cdsmInfo) {
        DisconnectAllProfileInner(member.addr_, result, discReason);
    }

    /* 虚拟开关重置 */
    if (adapterProperties_->IsVendorDevice(device)) {
        TwsService *twsService = TwsService::GetService();
        if (twsService != nullptr) {
            twsService->ResetVirtualAutoSwitch(device);
        }
    }
    return true;
}

void SleAdapter::OnAllProfileDisconnected(const RawAddress &device)
{
    HILOGI("[SleAdapter] device:%{public}s", GET_ENCRYPT_ADDR(device));

    int acbConnState = GetAcbStateTask(device);
    int reason = GetAcbDisConnReasonTask(device.GetAddress());
    if (reason == 0) {
        HILOGI("[SleAdapter] no pending disconnect context for device:%{public}s", GET_ENCRYPT_ADDR(device));
        return;
    }

    int pairState = static_cast<int>(SlePairState::SLE_PAIR_NONE);
    std::shared_ptr<const SlePeripheralDevice> remoteDevice = adapterProperties_->GetRemoteDevice(device);
    if (remoteDevice) {
        pairState = remoteDevice->GetPairedStatus();
    }

    if (NeedBgConn(acbConnState, pairState, device, reason)) {
        adapterProperties_->AddBgConnDevice(device.GetAddress());
    }
    SetAcbDisConnReasonTask(device.GetAddress(), 0);
}

void SleAdapter::DisconnectAllProfileForSilentPort(const RawAddress &device)
{
    HILOGI("device:%{public}s", GetEncryptAddr(device.GetAddress()).c_str());
    DoInAdapterThread([this, device]() {
        pimpl->sleProfileConnectManager_.SleDisconnectAllProfileForSilentPort(device);
    });
}

bool SleAdapter::IsLlPrivacySupported() const
{
    return SleFeature::GetInstance().IsPrivacySupported();
}

bool SleAdapter::IsFeatureSupported(int32_t feature) const
{
    if (feature == static_cast<int32_t>(SleFeatureSupported::SLE_RADIO_FRAME_TYPE_4)) {
        return SleFeature::GetInstance().IsFrameFourSupported();
    }
    return false;
}

int SleAdapter::SetBleRoles() const
{
    return NLSTK_ERRCODE_SUCCESS;
}

std::string SleAdapter::GetAliasName(const RawAddress &device) const
{
    return adapterProperties_->GetAliasName(device);
}

int SleAdapter::GetDeviceAppearance(const RawAddress &device) const
{
    return adapterProperties_->GetDeviceAppearance(device);
}

uint16_t SleAdapter::GetDeviceVendorId(const RawAddress &device) const
{
    ProfileDis *disService = static_cast<ProfileDis *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_DIS));
    NL_CHECK_RETURN_RET(disService, 0, "cant find dis service");
    RawAddress realAddr = adapterProperties_->GetRealAddress(device);
    return disService->GetDeviceVendorId(realAddr);
}

uint16_t SleAdapter::GetDeviceProductId(const RawAddress &device) const
{
    ProfileDis *disService = static_cast<ProfileDis *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_DIS));
    NL_CHECK_RETURN_RET(disService, 0, "cant find dis service");
    RawAddress realAddr = adapterProperties_->GetRealAddress(device);
    return disService->GetDeviceProductId(realAddr);
}

uint16_t SleAdapter::GetDeviceVersion(const RawAddress &device) const
{
    ProfileDis *disService = static_cast<ProfileDis *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_DIS));
    NL_CHECK_RETURN_RET(disService, 0, "cant find dis service");
    RawAddress realAddr = adapterProperties_->GetRealAddress(device);
    return disService->GetDeviceVersion(realAddr);
}

std::string FormatModelIdInfo(const std::string &input, uint32_t len)
{
    std::string ret = "";
    if (input.size() > len) {
        ret = input.substr(0, len);
        return ret;
    }
    for (uint32_t i = 0; i < len - input.size(); i++) {
        ret += "0";
    }
    ret += input;
    return ret;
}

void SleAdapter::FormatDeviceModelInfo(DeviceModel &model, std::string &newModelId)
{
    // modeId和newModeId是互斥的,两者只会存在一个
    if (model.GetModelId().empty()) {
        // 如果获取到的modelId为空，改为获取newModelId
        model.SetModelId(newModelId.empty() ? "FFFFFF" : newModelId);
    } else {
        model.SetModelId(FormatModelIdInfo(model.GetModelId(), TwsDeviceInfo::DEVICE_MODEL_ID_LEN));
    }
    model.SetSubModelId(model.GetSubModelId().empty()
        ? "FF" : FormatModelIdInfo(model.GetSubModelId(), TwsDeviceInfo::DEVICE_SUBMODEL_ID_LEN));
    model.SetIconId(
        model.GetIconId().empty() ? "FFFF" : FormatModelIdInfo(model.GetIconId(), TwsDeviceInfo::DEVICE_ICON_ID_LEN));
    model.SetDevType(
        model.GetDevType().empty() ? "FF" : FormatModelIdInfo(model.GetDevType(), TwsDeviceInfo::DEVICE_DEV_TYPE_LEN));
}

DeviceModel SleAdapter::GetDeviceModel(const RawAddress &device) const
{
    HILOGD("[SleAdapter]GetDeviceModel device:%{public}s", GET_ENCRYPT_ADDR(device));
    RawAddress realAddr = adapterProperties_->GetRealAddress(device);
    DeviceModel model;
    std::string newModelId;
    adapterProperties_->FindDeviceModelInfoInCache(device, model, newModelId);
    FormatDeviceModelInfo(model, newModelId);
    HILOGI("[SleAdapter]GetDeviceModel=[modelId:%{public}s, subModelId:%{public}s, iconId:%{public}s,"
        " devType:%{public}s]", model.GetModelId().c_str(), model.GetSubModelId().c_str(), model.GetIconId().c_str(),
        model.GetDevType().c_str());
    return model;
}

bool SleAdapter::SetAliasName(const RawAddress &device, const std::string &name) const
{
    std::promise<bool> promise;
    DoInAdapterThread([this, device, name, &promise]() {
        bool result = SetAliasNameTask(device, name);
        result |= InterfaceCloudPairService::GetInstance().SetCloudDeviceAliasName(device, name);
        promise.set_value(result);
    });
    bool ret = promise.get_future().get();
    return ret;
}

bool SleAdapter::SetAliasNameTask(const RawAddress &device, const std::string &name) const
{
    return adapterProperties_->SetAliasName(device, name);
}

void SleAdapter::PairStartChanged(const RawAddress &addr) const
{
    int pairStatus = static_cast<int>(SlePairState::SLE_PAIR_NONE);

    HILOGI("PairStartChanged addr:%{public}s pairStatus %{public}d.", GET_ENCRYPT_ADDR(addr), pairStatus);
    if (pairStatus == static_cast<int>(SlePairState::SLE_PAIR_NONE)) {
        PairingStatus(addr); // 首次被配对
    }
}

void SleAdapter::SendBgConnList() const
{
    HILOG_COMM_INFO("[%{public}s:%{public}d]Send Bg List", __FUNCTION__, __LINE__);
    NearlinkSafeList<RawAddress> bgList;
    std::vector<std::string> reconnectList = SleReconnectManager::GetInstance().GetReconnectList();
    adapterProperties_->GetBgList(bgList, reconnectList);
    adapterProperties_->SendBgConnList(bgList);
}

void SleAdapter::RemoveBgConnDevice(const std::string &delAddr) const
{
    SLE_Addr_S stackAddr = {};
    RawAddress address(delAddr);
    address.ConvertToUint8(stackAddr.addr);
    SleRemoteDeviceAdapter::GetInstance()->RemovePeerDeviceTypeToController(address);
    uint32_t ret = CM_BackgroundConnectRemove(CM_MODULE_ADPT, &stackAddr);
    NL_CHECK_RETURN(ret == CM_SUCCESS, "ret=%{public}d", ret);
}

void SleAdapter::ClearDeviceManufacturerAbility(const RawAddress &device) const
{
    HILOGI("device=%{public}s", GET_ENCRYPT_ADDR(device));
    SLE_Addr_S stackAddr = {};
    device.ConvertToUint8(stackAddr.addr);
    uint32_t ret = NLSTK_CfgdbRemoveManufacturerAbility(&stackAddr);
    NL_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, "ret=%{public}d", ret);
}

void SleAdapter::SendDirectConnList() const
{
    HILOGI("Send Direct Conn List");
    if (!SleServiceManager::GetInstance()->IsEnableAutoConnectUserDisconnectedDevices()) {
        // 芯片复位场景开启星闪，不回连用户主动断连的设备，不进行直连操作
        HILOGI("No need to direct connect");
        return;
    }

    NearlinkSafeList<RawAddress> bgList;
    adapterProperties_->GetDirectConnList(bgList);
    adapterProperties_->SendDirectConnList(bgList);
}

bool SleAdapter::FactoryReset() const
{
    LOG_DEBUG("[SleAdapter] enter");

    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    DoInAdapterThread([this, &promise]() -> void {
        ClearPeerDeviceInfo();
        DeleteDeviceInfoFiles();
        InterfaceCloudPairService::GetInstance().ClearCloudDeviceMap(true);
        bool ret = SleConfig::GetInstance().RemoveAllPairedDevices();
        if (!ret) {
            HILOGW("Paired list has been reset");
        }
        promise.set_value();
    });
    future.get();
    return true;
}

void SleAdapter::CancelAllConnection()
{
    DoInAdapterThread([this]() -> void {
        this->CancelAllConnectionTask();
    });
}

void SleAdapter::CancelAllConnectionTask() const
{
    adapterProperties_->CancelAllConnection();
}

int SleAdapter::GetProfileConnState(const RawAddress &device)
{
    std::promise<int> promise;
    DoInAdapterThread([this, device, &promise]() {
        int state = GetProfileConnStateTask(device);
        promise.set_value(state);
    });
    int result = promise.get_future().get();
    return result;
}

int SleAdapter::GetProfileConnStateTask(const RawAddress &device)
{
    if (InterfaceCloudPairService::GetInstance().IsCloudDeviceConnecting(device)) {
        return static_cast<int>(SleConnectState::CONNECTING);
    }
    CdsmService *cdsmService = CdsmService::GetService();
    std::vector<NearlinkCdsmInfo> cdmsInfo = {};
    if (cdsmService != nullptr &&
        cdsmService->CdsmGetAllMemberInfo(device, cdmsInfo) == NL_NO_ERROR &&
        cdmsInfo.size() == SLE_AUDIO_PERIPHERAL_DEVICE_NUMBER) {
        int firstState = pimpl->sleProfileConnectManager_.GetProfileConnState(cdmsInfo[0].addr_);
        int secondState = pimpl->sleProfileConnectManager_.GetProfileConnState(cdmsInfo[1].addr_);
        std::pair<int, int> states(firstState, secondState);
        auto item = SLE_PROFILE_STATE_CONVERT.find(states);
        if (item == SLE_PROFILE_STATE_CONVERT.end()) {
            HILOGE("[sleAdapter]:get profile state failed,dev:%{public}s,%{public}u/%{public}u",
                GET_ENCRYPT_ADDR(device), firstState, secondState);
            return static_cast<int>(SleConnectState::DISCONNECTED);
        }
        int cdsmState = item->second;
        HILOGD("[sleAdapter]:get profile state,dev:%{public}s,%{public}u/%{public}u,profile state:%{public}d",
            GET_ENCRYPT_ADDR(device), firstState, secondState, cdsmState);
        return cdsmState;
    }

    return pimpl->sleProfileConnectManager_.GetProfileConnState(device);
}

void SleAdapter::DeleteDeviceInfoFiles() const
{
    std::string deviceConfigName {"sle_device_config.xml"};
    std::string deviceConfigPath {SLE_CONFIG_PATH + deviceConfigName};
    int ret = remove(deviceConfigPath.c_str());
    if (ret == -1) {
        HILOGE("remove file failed, errno:%{public}s", strerror(errno));
    }
}

bool SleAdapter::WrapperCdsmGetAllMemberInfo(const RawAddress &rawAddr, std::vector<RawAddress> &cdsmInfoAddr) {
    CdsmService *cdsmService = CdsmService::GetService();
    std::vector<NearlinkCdsmInfo> cdsmInfo = {};
    if (cdsmService != nullptr && cdsmService->CdsmGetAllMemberInfo(rawAddr, cdsmInfo) == NL_NO_ERROR) {
        for (auto &info : cdsmInfo) {
            if (info.state_ == static_cast<int>(SleConnectState::CONNECTED)) {
                cdsmInfoAddr.push_back(info.addr_);
            }
        }
        return true;
    }
    return false;
}

bool SleAdapter::UpdateSleVirtualDevice(int32_t cmd, const RawAddress &device)
{
    HILOGI("[SleAdapter] enter");
    ProfileASC *ascService = static_cast<ProfileASC *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_ASC));
    NL_CHECK_RETURN_RET(ascService, false, "cant find ASC service");
    return ascService->UpdateSleVirtualDevice(cmd, device);
}

bool SleAdapter::GetSleAddrByBtAddr(const std::string &btAddr, std::string &sleAddr) const
{
    std::promise<bool> promise;
    DoInAdapterThread([this, btAddr, &sleAddr, &promise]() {
        bool result = adapterProperties_->GetSleAddrByBtAddrTask(btAddr, sleAddr);
        promise.set_value(result);
    });
    bool ret = promise.get_future().get();
    return ret;
}

bool SleAdapter::GetBtAddrBySleAddr(const std::string &sleAddr, std::string &btAddr) const
{
    std::promise<bool> promise;
    DoInAdapterThread([this, sleAddr, &btAddr, &promise]() {
        bool result = adapterProperties_->GetBtAddrBySleAddrTask(sleAddr, btAddr);
        promise.set_value(result);
    });
    bool ret = promise.get_future().get();
    return ret;
}

bool SleAdapter::SetBtAddrBySleAddr(const std::string &sleAddr, const std::string &btAddr) const
{
    std::promise<bool> promise;
    DoInAdapterThread([this, sleAddr, btAddr, &promise]() {
        bool result = SetBtAddrBySleAddrTask(sleAddr, btAddr);
        promise.set_value(result);
    });
    bool ret = promise.get_future().get();
    return ret;
}

bool SleAdapter::SetBtAddrBySleAddrTask(const std::string &sleAddr, const std::string &btAddr) const
{
    return adapterProperties_->SetBtAddrBySleAddr(sleAddr, btAddr);
}

bool SleAdapter::UpdateRefusePolicy(const int32_t protocolType, const int32_t pid, const int64_t refuseTime) const
{
    HILOGI("[SleAdapter] enter");
    bool ret = true;
#ifdef NEARLINK_KIA_ENABLE
    ret = SleKiaManager::GetInstance().UpdateRefusePolicy(protocolType, pid, refuseTime) == NL_NO_ERROR ? true : false;
#endif
    return ret;
}

void SleAdapter::UpdateDeviceModelInfo(const std::string &address, const DeviceModel &model,
    const std::string &newModelId)
{
    HILOGI("[SleAdapter]UpdateDeviceModelInfo: device:%{public}s, modelId:%{public}s, newModelId:%{public}s, "
        "iconId:%{public}s, deviceType:%{public}s", GetEncryptAddr(address).c_str(), model.GetModelId().c_str(),
        newModelId.c_str(), model.GetIconId().c_str(), model.GetDevType().c_str());
    RawAddress device(address);
    CdsmService *cdsmService = CdsmService::GetService();
    std::vector<NearlinkCdsmInfo> cdsmInfo = {};
    if (cdsmService != nullptr && cdsmService->CdsmGetAllMemberInfo(device, cdsmInfo) == NL_NO_ERROR) {
        for (const auto &member : cdsmInfo) {
            std::string memberAddr = member.addr_.GetAddress();
            adapterProperties_->SaveDeviceModelInfo(memberAddr, model, newModelId);
        }
        return;
    }
    adapterProperties_->SaveDeviceModelInfo(address, model, newModelId);
}

void SleAdapter::SendImgSecuConfig(const RawAddress &device)
{
    uint32_t groupId = 0;
    CdsmService *cdsmService = CdsmService::GetService();
    NL_CHECK_RETURN(cdsmService, "cdsmService nullptr.");
    cdsmService->CdsmGetGroupId(groupId, device.GetAddress());
    pimpl->sleSecurity_.SendImgSecuConfig(device, groupId);
}

REGISTER_CLASS_CREATOR(SleAdapter);
}  // namespace sle
}  // namespace OHOS
