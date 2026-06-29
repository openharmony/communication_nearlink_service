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
#include "SleServiceManager.h"
#include <array>
#include <functional>
#include <sys/time.h>
#include <charconv>
#include <future>
#include <csignal>
#include <cstring>

#include "log_util.h"
#include "slem.h"
#include "ipc_skeleton.h"
#include "SleInterfaceAdapter.h"
#include "AdapterDeviceConfig.h"
#include "AdapterStateMachine.h"
#include "BaseDef.h"
#include "BaseObserverList.h"
#include "ClassCreator.h"
#include "ProfileServiceManager.h"
#include "SysStateMachine.h"
#include "system_ability_definition.h"
#include "iservice_registry.h"
#include "parameter.h"
#include "nearlink_dft_exception.h"
#include "nearlink_permission_manager.h"
#include "UnloadSa.h"
#include "nearlink_common_event_helper.h"
#include "nearlink_datashare_helper.h"
#include "SleCollaborationManager.h"
#include "SleServiceFfrtLog.h"
#include "SleHiviewUe.h"
#include "ThreadUtil.h"
#include "SleDliSnoop.h"
#include "bundle_mgr_proxy.h"
#include "parameters.h"
#include "param_wrapper.h"
#if (defined(DEVICE_MANAGER))
#include "device_manager.h"
#endif
#include "ServiceManagerPluginLoader.h"
#ifdef NEARLINK_HOST_DYNAMIC_RUNING
#include "idevmgr_hdi.h"
#include "iservmgr_hdi.h"
#endif
#ifdef NEARLINK_EDM_ENABLE
#include "SleEdmManager.h"
#endif
#include "nearlink_verification_manager.h"
#include "nlstk_public_define_ext.h"

namespace OHOS {
namespace Nearlink {
namespace {
constexpr size_t NEARLINK_SWITCH_PARAMETER_LEN = 1;
#ifdef NEARLINK_HOST_DYNAMIC_RUNING
const char *NEARLINK_SLE_DLI_INTERFACE_SERVICE = "sle_hci_interface_service";
#endif
const std::string NEARLINK_SWITCH_SYSTEM_PARAMETER_NAME = "persist.nearlink.switch_enable";
const std::string NEARLINK_SWITCH_STATE_OFF = "0";
const std::string NEARLINK_SWITCH_STATE_ON = "1";
const std::string NEARLINK_SWITCH_STATE_HALF = "2";
const std::string RESET_SERVICE_SYSTEM_PARAMETER_NAME = "persist.nearlink.reset_service";
const std::string RESET_SERVICE_PARAMETER_VALUE = "0";
const std::string SATELLITE_SWITCH_SYSTEM_PARAMETER_NAME = "persist.telephony.satellite.sate_switch";
const std::string SATELLITE_SWITCH_STATE_ON = "true";
const std::string SATELLITE_START_SYSTEM_TIME_NAME = "persist.telephony.satellite.sys_time";
constexpr size_t SATELLITE_SWITCH_PARAMETER_MAX_LEN = 5;
constexpr size_t SATELLITE_START_TIME_PARAMETER_MAX_LEN = 64;
constexpr int64_t SATELLITE_TIMEOUT_NS = 120000000000; // 2min, in nanosecond
constexpr int32_t SEC_TO_NANO = 1000000000;
constexpr int32_t INITIALIZE_DELAY_MS = 1000;
const std::string CALLING_NAME_SATELLITE = "satellite_service";
constexpr int32_t CALLING_UID_SATELLITE = 1001;
const std::string NEARLINK_SERVICE_NAME = "nearlink_service";
constexpr int32_t DM_OK = 0;
const char* const DEFAULT_FOR_STEP_MPXX_DIR_MODE = "false";
const char* PARAM_NAME_FOR_STEP_MPXX_DIR_MODE = "vendor.setup_mpxx_dir_mode";
const char* DRIVER_LOAD_SUCCESS = "success";
constexpr size_t DRIVER_LOAD_PARAMETER_MAX_LEN = 7;
constexpr size_t MPXX_DIR_MODE_PARAMETER_MAX_LEN = 26;
constexpr int32_t RESTORE_SWITCH_STATUS_TIMEOUT_MS = 7000;
}  // namespace

SleInterfaceManager *SleInterfaceManager::GetInstance()
{
    return SleServiceManager::GetInstance();
}

SwitchCallerInfo SleInterfaceManager::GetCallerInfo()
{
    SwitchCallerInfo callerInfo {};
    callerInfo.fullTokenId = IPCSkeleton::GetCallingFullTokenID();
    callerInfo.callerUid = IPCSkeleton::GetCallingUid();
    return callerInfo;
}


SleServiceManager *SleServiceManager::GetInstance()
{
    static SleServiceManager instance;
    return &instance;
}

// impl class
struct SleServiceManager::impl {
    impl();
    ~impl();

    std::mutex resetPromiseMutex_ {};
    std::mutex stopPromiseMutex_ {};
    std::mutex sysStateMutex_ {};
    std::mutex instanceMutex_ {};
    std::promise<void> stopPromise_ {};
    std::promise<void> resetPromise_ {};
    std::promise<void> driverLoadPromise_ {};
    SysStateMachine sysStateMachine_ {};
    std::string sysState_ = SYS_STATE_STOPPED;
    SlemCallbacks dliFailureCallbacks {};
    BaseObserverList<IAdapterStateObserver> adapterObservers_ {};
    BaseObserverList<ISystemStateObserver> systemObservers_ {};

    class AdaptersContextCallback;
    std::unique_ptr<AdaptersContextCallback> contextCallback_ = nullptr;

    void OnEnable(const std::string &name, bool ret);
    void OnDisable(const std::string &name, bool ret);
    void ProcessMessage(const SleTransport transport, const utility::Message &msg);
    void SetTargetState(SleStateID targetState);

    std::atomic<SleStateID> state_ = SleStateID::STATE_TURN_OFF;
    std::shared_ptr<SleInterfaceAdapter> instance_ = nullptr;
    std::shared_ptr<AdapterStateMachine> stateMachine_ = nullptr;
    std::mutex initializedMutex_ {};
    bool isInitialized_ = false; // guarded by initializedMutex_
    std::condition_variable initializedConditionVariable_ {};
    void WaitServiceManagerInitializeComplete(void);
    SLE_DISALLOW_COPY_AND_ASSIGN(impl);
};

class SleServiceManager::impl::AdaptersContextCallback : public utility::IContextCallback {
public:
    explicit AdaptersContextCallback(SleServiceManager::impl &impl) : impl_(impl){};
    ~AdaptersContextCallback() = default;

    void OnEnable(const std::string &name, bool ret)
    {
        HILOGI("name=%{public}s, ret=%{public}d", name.c_str(), ret);
        impl_.OnEnable(name, ret);
    }
    void OnDisable(const std::string &name, bool ret)
    {
        HILOGI("name=%{public}s, ret=%{public}d", name.c_str(), ret);
        impl_.OnDisable(name, ret);
    }

private:
    SleServiceManager::impl &impl_;
};

SleServiceManager::impl::impl()
{
    // context callback create
    contextCallback_ = std::make_unique<AdaptersContextCallback>(*this);
}

SleServiceManager::impl::~impl()
{}

void SleServiceManager::impl::OnEnable(const std::string &name, bool ret)
{
    HILOGI("name=%{public}s, ret=%{public}d", name.c_str(), ret);
    SleTransport transport = SleTransport::ADAPTER_SLB;

    if (name.c_str() == ADAPTER_NAME_SLB) {
        transport = SleTransport::ADAPTER_SLB;
    } else if (name.c_str() == ADAPTER_NAME_SLE) {
        transport = SleTransport::ADAPTER_SLE;
    } else {
        HILOGE("name=%{public}s is warning transport", name.c_str());
    }
    int msgId = (int)AdapterStateMachine::AdapterStateMessage::MSG_ADAPTER_ENABLE_CMP;
    utility::Message msg(msgId, ret ? true : false);
    DoInServiceManagerThread([this, transport, msg]() -> void {
        ProcessMessage(transport, msg);
    });
}

void SleServiceManager::impl::OnDisable(const std::string &name, bool ret)
{
    HILOGI("name=%{public}s, ret=%{public}d", name.c_str(), ret);
    SleTransport transport = SleTransport::ADAPTER_SLB;

    if (name.c_str() == ADAPTER_NAME_SLB) {
        transport = SleTransport::ADAPTER_SLB;
    } else if (name.c_str() == ADAPTER_NAME_SLE) {
        transport = SleTransport::ADAPTER_SLE;
    } else {
        HILOGE("name=%{public}s is warning transport", name.c_str());
    }

    int msgId = (int)AdapterStateMachine::AdapterStateMessage::MSG_ADAPTER_DISABLE_CMP;
    utility::Message msg(msgId, ret ? true : false);
    DoInServiceManagerThread([this, transport, msg]() -> void {
        ProcessMessage(transport, msg);
    });
}

void SleServiceManager::impl::ProcessMessage(const SleTransport transport, const utility::Message &msg)
{
    if (stateMachine_ == nullptr) {
        HILOGE("stateMachine_ is nullptr");
        return;
    }

    stateMachine_->ProcessMessage(msg);
}

void SleServiceManager::impl::SetTargetState(SleStateID targetState)
{
    if (stateMachine_ == nullptr) {
        HILOGE("stateMachine_ is nullptr");
        return;
    }
    stateMachine_->SetNextTargetState(targetState);
}

void SleServiceManager::impl::WaitServiceManagerInitializeComplete(void)
{
    HILOGI("enter");
    std::unique_lock<std::mutex> lock(initializedMutex_);
    if (isInitialized_) {
        return;
    }
    if (!initializedConditionVariable_.wait_for(lock, std::chrono::milliseconds(INITIALIZE_DELAY_MS),
        [this]() -> bool { return isInitialized_; })) {
        HILOGE("SleServiceManager initialize timeout");
        return;
    }
}

// SleServiceManager class
SleServiceManager::SleServiceManager() : pimpl(std::make_unique<SleServiceManager::impl>())
{
    HILOGI("SleServiceManager construct");
    // sys state Machine create
    pimpl->sysStateMachine_.Init(*this);
}

SleServiceManager::~SleServiceManager()
{
    HILOGI("~SleServiceManager");
}

bool SleServiceManager::Start()
{
    HILOGI("enter");
    std::promise<bool> promise;
    std::future<bool> future = promise.get_future();
    DoInServiceManagerThread([this, &promise]() -> void {
        bool ret = StartTask();
        promise.set_value(ret);
        return;
    });
    return future.get();
}

void SleServiceManager::DriverLoadStatusChange(const char *key, const char *value)
{
    HILOGI("enter");
    if (strncmp(value, DRIVER_LOAD_SUCCESS, DRIVER_LOAD_PARAMETER_MAX_LEN) == 0 &&
        strncmp(key, PARAM_NAME_FOR_STEP_MPXX_DIR_MODE, MPXX_DIR_MODE_PARAMETER_MAX_LEN) == 0) {
        HILOGI("driver load success, key = %{public}s, value = %{public}s", key, value);
        pimpl->driverLoadPromise_.set_value();
    }
}

void SleServiceManager::RegisterDriverLoadCallback()
{
    HILOGI("enter");
    auto driverLoadStatsChangeCallback = [](const char *key, const char *value, void *context) {
        if (key == nullptr || value == nullptr) {
            HILOGE("Invalid key or value");
            return;
        }
        SleServiceManager::GetInstance()->DriverLoadStatusChange(key, value);
    };
    int ret = WatchParameter(PARAM_NAME_FOR_STEP_MPXX_DIR_MODE, driverLoadStatsChangeCallback, nullptr);
    if (ret != 0) {
        HILOGE("WatchParameter failed, ret(%{public}d)", ret);
    }
}

bool SleServiceManager::StartTask()
{
    HILOGI("SleServiceManager start");

    NL_CHECK_RETURN_RET(GetSysState() != SYS_STATE_STARTED, false, "Nearlink has been started!");

    NL_CHECK_RETURN_RET(slem_initialize() == NLSTK_ERRCODE_SUCCESS, false, "Nearlink Stack Initialize Failed!");

    ServiceManagerPluginLoader::GetInstance()->LoadPluginInterfaceLib();
    NearlinkVerificationManager::GetInstance().LoadStrategies();
    ServiceManagerPluginLoader::GetInstance()->CollaborationProc(CollaborationProcType::COLLABORATION_PROC_INIT);

    RegisterDriverLoadCallback();

    CreateAdapters();

    ProfileServiceManager::Initialize();

    RegisterDliResetCallback();

    OnSysStateChange(SYS_STATE_STARTED);

    utility::Message msg(SysStateMachine::MSG_SYS_START_CMP);
    pimpl->sysStateMachine_.ProcessMessage(msg);

    WaitForAllSwitchDependency();
    ServiceManagerPluginLoader::GetInstance()->SetPeripheralCallback([](ISlePeripheralCallback& callback) {
        auto* sleAdapter = static_cast<SleInterfaceAdapterSub*>(
            SleInterfaceManager::GetInstance()->GetAdapter(SleTransport::ADAPTER_SLE));
        NL_CHECK_RETURN(sleAdapter != nullptr, "sleAdapter is nullptr");
        sleAdapter->RegisterSlePeripheralCallback(callback);
    });
    HILOGI("SleServiceManager finished");
    return true;
}

void SleServiceManager::Stop() const
{
    HILOGI("start");

    if (GetSysState() == SYS_STATE_STOPPED) {
        HILOGI("SleServiceManager is stoped");
    } else if (GetSysState() == SYS_STATE_STOPPING) {
        HILOGI("SleServiceManager is stoping...");
    } else {
        std::promise<void> stopPromise;
        std::future<void> stopFuture = stopPromise.get_future();

        {
            std::lock_guard<std::mutex> lock(pimpl->stopPromiseMutex_);
            pimpl->stopPromise_ = std::move(stopPromise);
        }

        utility::Message msg(SysStateMachine::MSG_SYS_STOP_REQ);
        DoInServiceManagerThread([this, msg]() -> void {
            pimpl->sysStateMachine_.ProcessMessage(msg);
        });
        stopFuture.wait();
    }
}

bool SleServiceManager::AdapterStop() const
{
    HILOGI("AdapterStop start");
    ProfileServiceManager::Uninitialize();
    DeregisterDliResetCallback();
    ServiceManagerPluginLoader::GetInstance()->CollaborationProc(CollaborationProcType::COLLABORATION_PROC_DEINIT);
    slem_close();
    HILOGI("slem closed");

    NL_CHECK_RETURN_RET(pimpl != nullptr, false, "pimpl is nullptr.");
    {
        std::lock_guard<std::mutex> lock(pimpl->instanceMutex_);
        if (pimpl->instance_) {
            pimpl->instance_.reset();
        }
    }

    utility::Message msg(SysStateMachine::MSG_SYS_STOP_CMP);

    pimpl->sysStateMachine_.ProcessMessage(msg);

    HILOGI("AdapterStop finished");
    return true;
}

#ifdef NEARLINK_HOST_DYNAMIC_RUNING
void SleServiceManager::UnLoadNearlinkHostDevice() const
{
    HILOGI("start");
    auto devMgr = OHOS::HDI::DeviceManager::V1_0::IDeviceManager::Get();
    if (devMgr == nullptr) {
        HILOGE("get devMgr fail");
        return;
    }
    int32_t ret = devMgr->UnloadDevice(NEARLINK_SLE_DLI_INTERFACE_SERVICE);
    if (ret != 0) {
        HILOGE("unload nearlink host devMgr failed! ret:%{public}d", ret);
        return;
    }
    HILOGI("Unload nearlink host devMgr done");

    return;
}

bool SleServiceManager::LoadNearlinkHostDevice() const
{
    HILOGI("start");
    auto devMgr = OHOS::HDI::DeviceManager::V1_0::IDeviceManager::Get();
    NL_CHECK_RETURN_RET(devMgr != nullptr, false, "get devMgr failed");
    auto serviceMgr = OHOS::HDI::ServiceManager::V1_0::IServiceManager::Get();
    NL_CHECK_RETURN_RET(serviceMgr != nullptr, false, "get serviceMgr failed");
    int32_t retryTimes = 20;
    if (serviceMgr->GetService(NEARLINK_SLE_DLI_INTERFACE_SERVICE) == nullptr) {
        for (int i = 1; i <= retryTimes; i++) {
            int32_t ret = devMgr->LoadDevice(NEARLINK_SLE_DLI_INTERFACE_SERVICE);
            if (ret == 0) {
                break;
            }
            HILOGE("load nearlink host devMgr failed! ret:%{public}d, retry:%{public}d", ret, i);
        }
        NL_CHECK_RETURN_RET(serviceMgr->GetService(NEARLINK_SLE_DLI_INTERFACE_SERVICE) != nullptr,
            false, "load nearlink host devMgr failed! retry:%{public}d", retryTimes);
    }
    HILOGI("load nearlink host devMgr done");

    return true;
}
#endif

// 是否特定调用方，业务期间将星闪全关，后续通过调用EnableNl恢复星闪
static bool IsRestoreNeededCaller(const uint32_t tokenId, const int32_t uid)
{
    VerificationContext ctx = { .tokenId = tokenId, .uid = uid };
    bool result = NearlinkVerificationManager::GetInstance().CheckVerification(VerificationType::SWITCH_CONTROL, ctx);
    return result;
}

static std::int64_t GetBootTimeNanoSec()
{
    int result;
    struct timespec ts;
    result = clock_gettime(CLOCK_BOOTTIME, &ts);
    if (result == 0) {
        return ts.tv_sec * SEC_TO_NANO + ts.tv_nsec;
    } else {
        HILOGE("get boot time failed");
        return 0;
    }
}

static bool IsSatelliteParameterOn()
{
    std::string satelliteSwitchStr = "";
    int ret = OHOS::system::GetStringParameter(SATELLITE_SWITCH_SYSTEM_PARAMETER_NAME, satelliteSwitchStr, "");
    NL_CHECK_RETURN_RET(ret == 0, false, "GetStringParameter failed, ret = %{public}d", ret);
    NL_CHECK_RETURN_RET(
        !satelliteSwitchStr.empty() && satelliteSwitchStr.size() <= SATELLITE_SWITCH_PARAMETER_MAX_LEN,
        false, "invalid satelliteTimeStr length: %{public}d", satelliteSwitchStr.size());
    HILOGI("get satellite switch parameter: %{public}s", satelliteSwitchStr.c_str());
    return satelliteSwitchStr == SATELLITE_SWITCH_STATE_ON;
}

static bool IsSatelliteRestrictionValid()
{
    HILOGI("enter");
    std::string satelliteTimeStr = "";
    int ret = OHOS::system::GetStringParameter(SATELLITE_START_SYSTEM_TIME_NAME, satelliteTimeStr, "");
    NL_CHECK_RETURN_RET(ret == 0, false, "GetStringParameter failed, ret = %{public}d", ret);
    NL_CHECK_RETURN_RET(
        !satelliteTimeStr.empty() && satelliteTimeStr.size() <= SATELLITE_START_TIME_PARAMETER_MAX_LEN,
        false, "invalid satelliteTimeStr length: %{public}d", satelliteTimeStr.size());
    HILOGI("get satellite system time: %{public}s", satelliteTimeStr.c_str());
    std::int64_t satelliteTimeInt = 0;
    std::from_chars_result res =
        std::from_chars(satelliteTimeStr.data(), satelliteTimeStr.data() + satelliteTimeStr.size(), satelliteTimeInt);
    NL_CHECK_RETURN_RET((res.ec == std::errc{}) && (res.ptr == satelliteTimeStr.data() + satelliteTimeStr.size()),
        false, "FromString failed, string:%{public}s", satelliteTimeStr.c_str());
    std::int64_t currentTime = GetBootTimeNanoSec();
    HILOGI("satelliteTimeInt = %{public}lld, currentTime value = %{public}lld", satelliteTimeInt, currentTime);
    if (currentTime < satelliteTimeInt) {
        return false;
    }
    if (currentTime - satelliteTimeInt > SATELLITE_TIMEOUT_NS) {
        HILOGI("satellite time out");
        return false;
    }
    return true;
}

static std::string GetDftSwitchEventCausationString(const SleEventType reason, const std::string &callingName)
{
    std::string causationStr = "";
    if (reason == SleEventType::INTERFACE_TRIGGERED) {
        causationStr = callingName;
    } else {
        causationStr = DFT_MSG_SWITCH_REASON + GetReasonString(static_cast<int>(reason));
    }
    if (causationStr.empty()) {
        HILOGE("switch event causation empty");
    }
    return causationStr;
}

bool SleServiceManager::IsEnableNeeded(const uint32_t tokenId, const int32_t uid) const
{
    if (IsSleSwitchRestricted()) {
        // 星闪开关处于受限状态，不允许打开星闪
        return false;
    }
    if (IsRestoreNeededCaller(tokenId, uid)) {
        // 特定调用方，需执行恢复逻辑
        return false;
    }
    return true;
}

int32_t SleServiceManager::ProcessEnableDisallowed(const uint32_t tokenId,
    const int32_t uid, const SleTransport &transport, const SleEventType &reason) const
{
    if (IsRestoreNeededCaller(tokenId, uid)) {
        HILOGI("restore nearlink switch state");
        isRestoreNeeded_.store(false);
        return RestoreSleFromSystemParameter(transport, reason);
    } else {
        HILOGI("Enable is disallowed");
        return NL_ERR_INVALID_SWITCH_OPERATION;
    }
}

void SleServiceManager::WaitDriverLoadCompleted() const
{
    std::string ret = OHOS::system::GetParameter(PARAM_NAME_FOR_STEP_MPXX_DIR_MODE, DEFAULT_FOR_STEP_MPXX_DIR_MODE);
    HILOGI("ret=%{public}s", ret.c_str());
    if (ret == DRIVER_LOAD_SUCCESS) {
        return;
    }
    std::future<void> driverLoadFuture = pimpl->driverLoadPromise_.get_future();
    auto status = driverLoadFuture.wait_for(std::chrono::milliseconds(RESTORE_SWITCH_STATUS_TIMEOUT_MS));
    if (status != std::future_status::ready) {
        HILOGE("wait driver load finished timeout");
        RemoveParameterWatcher(PARAM_NAME_FOR_STEP_MPXX_DIR_MODE, nullptr, nullptr);
        return;
    }
    HILOGI("driver load finish");
    RemoveParameterWatcher(PARAM_NAME_FOR_STEP_MPXX_DIR_MODE, nullptr, nullptr);
    driverLoadFuture.get();
}

int32_t SleServiceManager::Enable(const SleTransport transport, const SleEventType reason,
    const SwitchCallerInfo callerInfo, const SleAutoConnectPolicy autoConnPolicy) const
{
    HILOGI("enter");
    pimpl->WaitServiceManagerInitializeComplete();

    std::promise<int32_t> promise;
    std::future<int32_t> future = promise.get_future();
    DoInServiceManagerThread([this, transport, reason, callerInfo, autoConnPolicy, &promise]() -> void {
        int32_t ret = EnableTask(transport, reason, callerInfo, autoConnPolicy);
        promise.set_value(ret);
        return;
    });
    int32_t ret = future.get();
    HILOGI("EnableTask finished, return: %{public}d", ret);
    return ret;
}

int32_t SleServiceManager::EnableTask(const SleTransport transport, const SleEventType reason,
    const SwitchCallerInfo callerInfo, const SleAutoConnectPolicy autoConnPolicy) const
{
    HILOGI("Enable start, reason: %{public}s", GetReasonString(static_cast<int>(reason)).c_str());
    NL_CHECK_RETURN_RET(pimpl != nullptr, NL_ERR_INTERNAL_ERROR, "pimpl is nullptr");
    NL_CHECK_RETURN_RET(
        GetSysState() == SYS_STATE_STARTED, NL_ERR_INTERNAL_ERROR, "SleServiceManager system is stoped");

#ifdef NEARLINK_HOST_DYNAMIC_RUNING
    UnloadSa::GetInstance().StopUnloadNearlinkSaTimer();
    if (!LoadNearlinkHostDevice()) {
        HILOGE("LoadNearlinkHostService fail");
        UnloadSa::GetInstance().StartUnloadNearlinkSaTimer(GetState(transport));
        return NL_ERR_INTERNAL_ERROR;
    }
#endif
    return EnableInner(transport, reason, callerInfo, autoConnPolicy);
}

bool SleServiceManager::IsEnableAutoConnectAudioDevices() const
{
    return (autoConnPolicy_ != SleAutoConnectPolicy::AUTO_CONN_EXCEPT_AUDIO_DEVICES);
}

bool SleServiceManager::IsEnableAutoConnectUserDisconnectedDevices() const
{
    return (autoConnPolicy_ != SleAutoConnectPolicy::AUTO_CONN_EXCEPT_USER_DISCONNECTED_DEVICES);
}

int32_t SleServiceManager::EnableInner(const SleTransport transport, const SleEventType reason,
    const SwitchCallerInfo callerInfo, const SleAutoConnectPolicy autoConnPolicy) const
{
    NL_CHECK_RETURN_RET(pimpl != nullptr, NL_ERR_INTERNAL_ERROR, "pimpl is nullptr");
    uint32_t tokenId = static_cast<uint32_t>(callerInfo.fullTokenId & 0xFFFFFFFF);
    std::string callingName = NearLinkPermissionManager::GetCallingName(tokenId);
    int state = pimpl->state_.load();
    std::string reasonString = GetReasonString(static_cast<int>(reason));
    HILOG_COMM_INFO("[%{public}s:%{public}d]enable nearlink, callingName(%{public}s), "
        "state(%{public}s), reason(%{public}s), transport(%{public}s)", __FUNCTION__, __LINE__, callingName.c_str(),
        GetStateString(state).c_str(), reasonString.c_str(), GetTransportString(transport).c_str());
    int32_t uid = callerInfo.callerUid;
    if (IsEnableNeeded(tokenId, uid)) {
        isRestoreNeeded_.store(false);
    } else {
        return ProcessEnableDisallowed(tokenId, uid, transport, reason);
    }
    pimpl->SetTargetState(SleStateID::STATE_TURN_ON);
    autoConnPolicy_ = autoConnPolicy;
    UpdateNearlinkSwitchSystemParameter(transport, SleStateID::STATE_TURN_ON);

    std::string causationStr = GetDftSwitchEventCausationString(reason, callingName);
    if (state == SleStateID::STATE_TURN_OFF || state == SleStateID::STATE_TURN_HALF) {
        DftCacheSwitchInfo(SWITCH_OPEN, SWITCH_OPER_SUCCESS, causationStr);
        UnloadSa::GetInstance().StopUnloadNearlinkSaTimer();
        int msgId = static_cast<int>(AdapterStateMachine::AdapterStateMessage::MSG_ENABLE_REQ);
        utility::Message msg(msgId);
        pimpl->ProcessMessage(transport, msg);
        return NL_NO_ERROR;
    } else if (state == SleStateID::STATE_TURN_ON) {
        DftReportSwitchInfo(SWITCH_SUCCESS, DFT_MSG_ALREADY_ENABLE, SWITCH_OPEN, SWITCH_OPER_SUCCESS, causationStr);
        return NL_ERR_INVALID_SWITCH_OPERATION;
    } else {
        DftReportSwitchInfo(TURNING_STATE, DFT_MSG_TURNING_STATE, SWITCH_OPEN, SWITCH_OPER_SUCCESS, causationStr);
        return NL_NO_ERROR;
    }
}

int32_t SleServiceManager::Disable(const SleTransport transport, const SleEventType reason,
    const SwitchCallerInfo callerInfo) const
{
    HILOGI("enter");
    uint32_t tokenId = static_cast<uint32_t>(callerInfo.fullTokenId & 0xFFFFFFFF);
    std::string callingName = NearLinkPermissionManager::GetCallingName(tokenId);
    // 发送私有广播，通知星闪Disable接口被调用
    NearlinkHelper::NearlinkCommonEventHelper::PublishDisableNlEvent(transport, callingName);

    SleStateID state = GetState(ADAPTER_SLE);
    if (state == STATE_TURNING_OFF || state == STATE_TURNING_HALF_TO_OFF || state == STATE_TURN_OFF) {
        // 已经处于OFF状态或正在向OFF状态切换的过程中，无需处理Disable
        HILOGI("Already in %{public}s", GetStateString(state).c_str());
        return NL_ERR_INVALID_SWITCH_OPERATION;
    }

    std::promise<int32_t> promise;
    std::future<int32_t> future = promise.get_future();
    DoInServiceManagerThread([this, transport, reason, callerInfo, &promise]() -> void {
        int32_t ret = DisableTask(transport, reason, callerInfo);
        promise.set_value(ret);
        return;
    });
    int32_t ret = future.get();
    HILOGI("DisableTask finished, return: %{public}d", ret);
    return ret;
}

int32_t SleServiceManager::DisableTask(
    const SleTransport transport, const SleEventType reason, const SwitchCallerInfo callerInfo) const
{
    HILOGI("Disable start, reason: %{public}s", GetReasonString(static_cast<int>(reason)).c_str());
    uint32_t tokenId = static_cast<uint32_t>(callerInfo.fullTokenId & 0xFFFFFFFF);
    std::string callingName = NearLinkPermissionManager::GetCallingName(tokenId);
    int32_t uid = callerInfo.callerUid;
    disableCallerInfo_ = callerInfo; // 记录Disable的调用方，后续需要发送响应

    bool isAirplaneOn = NearlinkDataShareHelper::GetInstance().GetAirplaneModeState();
    bool isCollaborationOn = SleCollaborationManager::GetInstance().IsCollaborationOn();
    if (isAirplaneOn || !isCollaborationOn) {
        return SwitchToOffTask(transport, reason, callerInfo);
    }
    return SwitchToHalfTask(transport, reason, callerInfo);
}

int32_t SleServiceManager::DisableToOff(const SleTransport transport, const SleEventType reason,
    const SwitchCallerInfo callerInfo) const
{
    HILOGI("enter");
    uint32_t tokenId = static_cast<uint32_t>(callerInfo.fullTokenId & 0xFFFFFFFF);
    std::string callingName = NearLinkPermissionManager::GetCallingName(tokenId);
    // 发送私有广播，通知星闪Disable接口被调用
    NearlinkHelper::NearlinkCommonEventHelper::PublishDisableNlEvent(transport, callingName);

    std::promise<int32_t> promise;
    std::future<int32_t> future = promise.get_future();
    DoInServiceManagerThread([this, transport, reason, callerInfo, &promise]() -> void {
        int32_t ret = SwitchToOffTask(transport, reason, callerInfo);
        promise.set_value(ret);
        return;
    });
    int32_t ret = future.get();
    HILOGI("SwitchToOffTask finished, return: %{public}d", ret);
    return ret;
}

int32_t SleServiceManager::SwitchToOffTask(
    const SleTransport transport, const SleEventType reason, const SwitchCallerInfo callerInfo) const
{
    {
        std::lock_guard<std::mutex> lock(pimpl->instanceMutex_);
        NL_CHECK_RETURN_RET(pimpl->instance_ != nullptr, NL_ERR_INTERNAL_ERROR, "instance_ is nullptr.");
    }
    uint32_t tokenId = static_cast<uint32_t>(callerInfo.fullTokenId & 0xFFFFFFFF);
    std::string callingName = NearLinkPermissionManager::GetCallingName(tokenId);
    int state = GetState(transport);
    std::string reasonString = GetReasonString(static_cast<int>(reason));
    HILOG_COMM_INFO("[%{public}s:%{public}d]disable nearlink, callingName(%{public}s), state(%{public}s), "
        "reason(%{public}s), transport(%{public}s)", __FUNCTION__, __LINE__, callingName.c_str(),
        GetStateString(state).c_str(), reasonString.c_str(), GetTransportString(transport).c_str());
    std::string causationStr = GetDftSwitchEventCausationString(reason, callingName);

    pimpl->SetTargetState(SleStateID::STATE_TURN_OFF);
    if (!IsRestoreNeededCaller(tokenId, callerInfo.callerUid)) {
        // 一般情况，设置开关系统参数为OFF
        UpdateNearlinkSwitchSystemParameter(transport, SleStateID::STATE_TURN_OFF);
    } else {
        // 特定调用方，保持开关系统参数不变，记录标志位用以后续恢复开关状态
        HILOGI("nearlink is turned off by %{public}s, to be restored later", callingName.c_str());
        isRestoreNeeded_.store(true);
    }

    if (state == SleStateID::STATE_TURN_ON || state == SleStateID::STATE_TURN_HALF) {
        DftCacheSwitchInfo(SWITCH_CLOSE, SWITCH_OPER_SUCCESS, causationStr);
        int msgId = static_cast<int>(AdapterStateMachine::AdapterStateMessage::MSG_DISABLE_REQ);
        utility::Message msg(msgId);
        pimpl->ProcessMessage(transport, msg);
        return NL_NO_ERROR;
    } else if (state == SleStateID::STATE_TURN_OFF) {
        DftReportSwitchInfo(SWITCH_SUCCESS, DFT_MSG_ALREADY_DISABLE, SWITCH_CLOSE, SWITCH_OPER_SUCCESS, causationStr);
        return NL_ERR_INVALID_SWITCH_OPERATION;
    } else {
        DftReportSwitchInfo(TURNING_STATE, DFT_MSG_TURNING_STATE, SWITCH_CLOSE, SWITCH_OPER_SUCCESS, causationStr);
        return NL_NO_ERROR;
    }
}

int32_t SleServiceManager::EnableToHalf(const SleTransport transport, const SleEventType reason,
    const SwitchCallerInfo callerInfo) const
{
    HILOGI("enter");
    pimpl->WaitServiceManagerInitializeComplete();

    std::promise<int32_t> promise;
    std::future<int32_t> future = promise.get_future();
    DoInServiceManagerThread([this, transport, reason, callerInfo, &promise]() -> void {
        int32_t ret = SwitchToHalfTask(transport, reason, callerInfo);
        promise.set_value(ret);
        return;
    });
    int32_t ret = future.get();
    HILOGI("EnableToHalfTask finished, return: %{public}d", ret);
    return ret;
}

int32_t SleServiceManager::SwitchToHalfTask(
    const SleTransport transport, const SleEventType reason, const SwitchCallerInfo callerInfo) const
{
    {
        std::lock_guard<std::mutex> lock(pimpl->instanceMutex_);
        NL_CHECK_RETURN_RET(pimpl->instance_ != nullptr, NL_ERR_INTERNAL_ERROR, "instance_ is nullptr.");
    }
    uint32_t tokenId = static_cast<uint32_t>(callerInfo.fullTokenId & 0xFFFFFFFF);
    std::string callingName = NearLinkPermissionManager::GetCallingName(tokenId);
    int state = GetState(transport);
    std::string reasonString = GetReasonString(static_cast<int>(reason));
    HILOG_COMM_INFO("[%{public}s:%{public}d]switch to half, callingName(%{public}s), state(%{public}s), "
        "reason(%{public}s), transport(%{public}s)", __FUNCTION__, __LINE__, callingName.c_str(),
        GetStateString(state).c_str(), reasonString.c_str(), GetTransportString(transport).c_str());
    if (state == SleStateID::STATE_TURN_OFF && IsSleSwitchRestricted()) {
        return NL_ERR_INVALID_SWITCH_OPERATION;
    }
    pimpl->SetTargetState(SleStateID::STATE_TURN_HALF);
    UpdateNearlinkSwitchSystemParameter(transport, SleStateID::STATE_TURN_HALF);

    std::string causationStr = GetDftSwitchEventCausationString(reason, callingName);
    if (state == SleStateID::STATE_TURN_ON) {
        DftCacheSwitchInfo(SWITCH_HALF, SWITCH_OPER_SUCCESS, causationStr);
        // 星闪从ON切换到HALF，需要先切换到OFF，进程重启后再恢复到半关
        pimpl->SetTargetState(SleStateID::STATE_TURN_OFF);
        int msgId = static_cast<int>(AdapterStateMachine::AdapterStateMessage::MSG_DISABLE_REQ);
        utility::Message msg(msgId);
        pimpl->ProcessMessage(transport, msg);
        return NL_NO_ERROR;
    } else if (state == SleStateID::STATE_TURN_OFF) {
        pimpl->SetTargetState(SleStateID::STATE_TURN_HALF);
        DftCacheSwitchInfo(SWITCH_HALF, SWITCH_OPER_SUCCESS, causationStr);
        NL_CHECK_RETURN_RET(GetSysState() == SYS_STATE_STARTED, false, "SleServiceManager system is stoped");
        UnloadSa::GetInstance().StopUnloadNearlinkSaTimer();
        int msgId = static_cast<int>(AdapterStateMachine::AdapterStateMessage::MSG_HALF_DISABLE_REQ);
        utility::Message msg(msgId);
        pimpl->ProcessMessage(transport, msg);
        return NL_NO_ERROR;
    } else if (state == SleStateID::STATE_TURN_HALF) {
        DftReportSwitchInfo(SWITCH_SUCCESS, DFT_MSG_ALREADY_HALF, SWITCH_HALF, SWITCH_OPER_SUCCESS, causationStr);
        return NL_ERR_INVALID_SWITCH_OPERATION;
    } else {
        DftReportSwitchInfo(TURNING_STATE, DFT_MSG_TURNING_STATE, SWITCH_HALF, SWITCH_OPER_SUCCESS, causationStr);
        return NL_NO_ERROR;
    }
}

bool SleServiceManager::FactoryReset() const
{
    HILOGI("start");
    DftCacheSwitchInfo(SWITCH_FACTORY_RESET, SWITCH_OPER_SUCCESS);
    if (GetSysState() == SYS_STATE_STARTED) {
        std::promise<void> resetPromise;
        std::future<void> resetFuture = resetPromise.get_future();

        {
            std::lock_guard<std::mutex> lock(pimpl->resetPromiseMutex_);
            pimpl->resetPromise_ = std::move(resetPromise);
        }

        utility::Message msg(SysStateMachine::MSG_SYS_FACTORY_RESET_REQ);
        DoInServiceManagerThread([this, msg]() -> void {
            pimpl->sysStateMachine_.ProcessMessage(msg);
        });
        resetFuture.wait();
        return true;
    } else {
        HILOGI("System state is not started");
        return false;
    }
}

void SleServiceManager::DliFailedReset()
{
    HILOGI("start");
    SleServiceManager *context = GetInstance();
    (static_cast<SleServiceManager *>(context))->Reset();
}

void SleServiceManager::RegisterDliResetCallback()
{
    pimpl->dliFailureCallbacks.dliFailure = DliFailedReset;
    slem_registerCallbacks(&(pimpl->dliFailureCallbacks));
}

void SleServiceManager::DeregisterDliResetCallback() const
{
    if (pimpl->dliFailureCallbacks.dliFailure != nullptr) {
        slem_deregisterCallbacks();
        pimpl->dliFailureCallbacks.dliFailure = nullptr;
    }
}

void SleServiceManager::Reset() const
{
    HILOGI("execute kill for reset");
    DftCacheSwitchInfo(SWITCH_RESET, SWITCH_OPER_SUCCESS);
    kill(getpid(), SIGKILL);
}

bool SleServiceManager::ClearAllStorage() const
{
    HILOGI("start");

    if (!AdapterDeviceConfig::GetInstance()->Reload()) {
        return false;
    }

    utility::Message msg(SysStateMachine::MSG_SYS_CLEAR_ALL_STORAGE_CMP);
    DoInServiceManagerThread([this, msg]() -> void {
        pimpl->sysStateMachine_.ProcessMessage(msg);
    });
    return true;
}

SleStateID SleServiceManager::GetState(const SleTransport transport) const
{
    NL_CHECK_RETURN_RET(pimpl != nullptr, SleStateID::STATE_TURN_OFF, "pimpl is nullptr");

    return pimpl->state_.load();
}

bool SleServiceManager::RegisterStateObserver(IAdapterStateObserver &observer) const
{
    return pimpl->adapterObservers_.Register(observer);
}

bool SleServiceManager::DeregisterStateObserver(IAdapterStateObserver &observer) const
{
    return pimpl->adapterObservers_.Deregister(observer);
}

bool SleServiceManager::RegisterSystemStateObserver(ISystemStateObserver &observer) const
{
    HILOGI("enter");
    return pimpl->systemObservers_.Register(observer);
}

bool SleServiceManager::DeregisterSystemStateObserver(ISystemStateObserver &observer) const
{
    return pimpl->systemObservers_.Deregister(observer);
}

SleConnectState SleServiceManager::GetAdapterConnectState() const
{
    HILOGI("start");
    return ProfileServiceManager::GetInstance().GetProfileServicesConnectState();
}

SleInterfaceAdapter *SleServiceManager::GetAdapter(const SleTransport transport) const
{
    if (pimpl == nullptr) {
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(pimpl->instanceMutex_);
    return pimpl->instance_.get();
}

void SleServiceManager::OnSysStateChange(const std::string &state) const
{
    HILOGI("state is %{public}s", state.c_str());

    std::string oldSysState;
    std::string newSysState = state;

    {  // lock start,update systerm state
        std::lock_guard<std::mutex> lock(pimpl->sysStateMutex_);
        oldSysState = pimpl->sysState_;
        pimpl->sysState_ = state;
    }  // lock end

    if (newSysState == SYS_STATE_FRESETTING) {
        HILOGI("state is %{public}s not report", state.c_str());
        return;
    }
    // notify systerm state update
    SleSystemState notifySysState = (newSysState == SYS_STATE_STARTED) ? SleSystemState::ON : SleSystemState::OFF;
    HILOGI("notifySysState is %{public}d", notifySysState);
    if (newSysState != oldSysState && (newSysState != SYS_STATE_STOPPING)) {
        HILOGI("oldSysState is %{public}s, newSysState is %{public}s", oldSysState.c_str(), newSysState.c_str());
        pimpl->systemObservers_.ForEach(
            [notifySysState](ISystemStateObserver &observer) { observer.OnSystemStateChange(notifySysState); });
#ifdef NEARLINK_HOST_DYNAMIC_RUNING
        if (newSysState == SYS_STATE_STOPPED) {
            UnLoadNearlinkHostDevice();
        }
#endif
    }
}

std::string SleServiceManager::GetSysState() const
{
    std::lock_guard<std::mutex> lock(pimpl->sysStateMutex_);
    return pimpl->sysState_;
}

void SleServiceManager::OnSysStateExit(const std::string &state) const
{
    HILOGI("state is %{public}s", state.c_str());


    if (state == SYS_STATE_FRESETTING) {
        std::lock_guard<std::mutex> lock(pimpl->resetPromiseMutex_);
        pimpl->resetPromise_.set_value();
    } else if (state == SYS_STATE_STOPPING) {
        std::lock_guard<std::mutex> lock(pimpl->stopPromiseMutex_);
        pimpl->stopPromise_.set_value();
    } else {
        // Nothing to do.
    }

}

void SleServiceManager::OnAdapterStateChange(const SleTransport transport, const SleStateID state) const
{
    HILOGI("enter");
    DoInServiceManagerThread([this, transport, state]() -> void {
        OnAdapterStateChangeTask(transport, state);
    });
}

void SleServiceManager::OnAdapterStateChangeTask(const SleTransport transport, const SleStateID state) const
{
    HILOG_COMM_INFO("[%{public}s:%{public}d]transport(%{public}s), state(%{public}s)", __FUNCTION__, __LINE__,
        GetTransportString(transport).c_str(), GetStateString(state).c_str());
    DftCacheStateFlow(state);
    NL_CHECK_RETURN(pimpl != nullptr, "pimpl is nullptr.");

    if (pimpl->state_.load() == state) {
        HILOGI("sle adapter state is unchanged");
        return;
    }
    pimpl->state_.store(state);

    if (state == STATE_TURN_OFF) {
        UnloadSa::GetInstance().StartUnloadNearlinkSaTimer(state);
    } else {
        UnloadSa::GetInstance().StopUnloadNearlinkSaTimer();
    }

    AdapterStateChangeNotify(transport, state);

    if (state == STATE_TURN_ON) {
        ServiceManagerPluginLoader::GetInstance()->PowerMgrProc();
    }

#ifdef WATCH_STANDARD
    AdapterStateChangeNotifyDataShare(transport, state);
#endif

    if (state == STATE_TURN_OFF) {
        CheckAndStopService();
    }
}

void SleServiceManager::AdapterStateChangeNotify(const SleTransport transport, const SleStateID state) const
{
    // publish full state change event
    NearlinkHelper::NearlinkCommonEventHelper::PublishFullStateChangeEvent(transport, state);

    // notify sys state machine
    utility::Message msg(SysStateMachine::MSG_SYS_ADAPTER_STATE_CHANGE_REQ);
    msg.arg1M = state;
    pimpl->sysStateMachine_.ProcessMessage(msg);

    // notify observers state update
    if (GetSysState() != SYS_STATE_RESETTING && GetSysState() != SYS_STATE_FRESETTING) {
        pimpl->adapterObservers_.ForEach(
            [transport, state](IAdapterStateObserver &observer) { observer.OnStateChange(transport, state); });
    }

    // process event related to collaboration
    if (IsStateStable(state)) {
        SleCollaborationManager::GetInstance().ProcessNextCollaborationEvent();
    }

    // publish device connection state event
    if (state == STATE_TURN_OFF) {
        HILOGI("sle is turned off, disconnected");
        NearlinkHelper::NearlinkCommonEventHelper::PublishDeviceConnectionStateEvent(
            static_cast<int>(SleConnectState::DISCONNECTED));
    } else if (state == STATE_TURN_ON) {
        uint32_t acbCount = 0;
        {
            std::lock_guard<std::mutex> lock(pimpl->instanceMutex_);
            NL_CHECK_RETURN(pimpl->instance_ != nullptr, "adapter is nullptr.");
            acbCount = pimpl->instance_->GetAcbCount();
        }
        if (acbCount > 0) {
            HILOGI("sle is turned on, connected ACB count: %{public}u", acbCount);
            NearlinkHelper::NearlinkCommonEventHelper::PublishDeviceConnectionStateEvent(
                static_cast<int>(SleConnectState::CONNECTED));
        }
    }
}

void SleServiceManager::CheckAndStopService() const
{
    // 星闪进入OFF状态后需要退出服务进程，此时系统参数对应的状态反映了实际的目标状态
    SleStateID stateParameter = GetNearlinkSwitchStateFromSystemParameter();
    if (stateParameter == SleStateID::STATE_TURN_OFF || isRestoreNeeded_.load()) {
        // 系统参数为OFF，或者星闪被特定调用方关闭的情况（此时系统参数不为OFF），需卸载SA
        HILOGI("unload nearlink sa because nearlink is turned off");
        SendDisableResponse(false);
        UnloadSa::GetInstance().UnloadNearlinkSa();
    } else if (stateParameter == SleStateID::STATE_TURN_HALF) {
        // 系统参数为HALF，需卸载再重新加载SA
        HILOGI("unload nearlink sa before turning nearlink to half");
        SendDisableResponse(true);
        UnloadSa::GetInstance().UnloadNearlinkSa();
    } else {
        HILOGI("reset nearlink sa because enable nearlink failed");
        ResetNearlinkService();
    }
    return;
}

void SleServiceManager::SendDisableResponse(bool isHalfDisable) const
{
    if (disableCallerInfo_.callerUid < 0) {
        // disableCallerInfo无效，说明此次关闭不是由DisableNl接口触发，无需发送响应
        return;
    }
    HILOGI("isHalfDisable: %{public}d", isHalfDisable);
    // 回调通知DisableNl的调用者本次Disable预期的开关结果（HALF或OFF），每次通知后disableCallerInfo_失效
    SwitchCallerInfo disableCallerInfo = disableCallerInfo_;
    pimpl->adapterObservers_.ForEach([isHalfDisable, disableCallerInfo](IAdapterStateObserver &observer) -> void {
        observer.OnDisableResponse(isHalfDisable, disableCallerInfo);
    });
    disableCallerInfo_.fullTokenId = 0;
    disableCallerInfo_.callerUid = -1;
}

void SleServiceManager::ResetNearlinkService() const
{
    HILOGI("enter");
    // 系统参数persist.nearlink.reset_service一旦被写入（不论写入何值）就会触发nearlink_service进程重启
    bool ret = OHOS::system::SetParameter(RESET_SERVICE_SYSTEM_PARAMETER_NAME, RESET_SERVICE_PARAMETER_VALUE);
    NL_CHECK_RETURN(ret, "failed to set nearlink reset_service parameter");
}

#ifdef WATCH_STANDARD
void SleServiceManager::AdapterStateChangeNotifyDataShare(const SleTransport transport, const SleStateID state) const
{
    NL_CHECK_RETURN(transport == ADAPTER_SLE, "transport not SLE no processing");
    if (state == STATE_TURN_OFF || state == STATE_TURN_ON) {
        const std::string &status = (state == STATE_TURN_ON) ? NEARLINK_SWITCH_STATE_ON : NEARLINK_SWITCH_STATE_OFF;
        NearlinkDataShareHelper::GetInstance().SaveNearlinkSwitchStatus(status);
    }
}
#endif

bool SleServiceManager::IsDisabling() const
{
    SleStateID state = GetState(ADAPTER_SLE);
    return (state == STATE_TURNING_OFF || state == STATE_TURNING_HALF_TO_OFF || state == STATE_TURNING_ON_TO_HALF);
}

void SleServiceManager::OnChipResetNotify() const
{
    HILOGW("chip is reset");
    SleStateID targetState = pimpl->stateMachine_->GetNextTargetState();
    if (targetState != SleStateID::STATE_TURN_OFF) {
        NearlinkHelper::NearlinkCommonEventHelper::PublishChipResetEvent(static_cast<int>(targetState));
        usleep(20000); // 等待20ms，保证芯片复位事件广播被接收
    }
    HILOGI("chip reset, nearlink target state is: %{public}s", GetStateString(targetState).c_str());
    Reset();
}

void SleServiceManager::OnProfileServicesEnableComplete(const SleTransport transport, const bool ret) const
{
    HILOGI("transport(%{public}s), ret(%{public}d)", GetTransportString(transport).c_str(), ret);
    int msgId = static_cast<int>(AdapterStateMachine::AdapterStateMessage::MSG_PROFILE_ENABLE_CMP);
    utility::Message msg(msgId, ret ? true : false);
    DoInServiceManagerThread([this, transport, msg]() -> void {
        pimpl->ProcessMessage(transport, msg);
    });
}

void SleServiceManager::OnProfileServicesDisableComplete(const SleTransport transport, const bool ret) const
{
    HILOGI("transport(%{public}s), ret(%{public}d)", GetTransportString(transport).c_str(), ret);
    int msgId = static_cast<int>(AdapterStateMachine::AdapterStateMessage::MSG_PROFILE_DISABLE_CMP);
    utility::Message msg(msgId, ret ? true : false);
    pimpl->ProcessMessage(transport, msg);
}

void SleServiceManager::OnPairDevicesRemoved(const SleTransport transport, const std::vector<RawAddress> &devices) const
{
    DoInServiceManagerThread([this, transport, devicesList = std::move(devices)]() -> void {
        RemoveDeviceProfileConfig(transport, devicesList);
    });
}

void SleServiceManager::RemoveDeviceProfileConfig(
    const SleTransport transport, const std::vector<RawAddress> &devices) const
{
    HILOGI("start");
}

void SleServiceManager::CreateAdapters() const
{
    HILOGI("enter");
    std::shared_ptr<SleInterfaceAdapter> adapter(ClassCreator<SleInterfaceAdapter>::NewInstance(ADAPTER_NAME_SLE));
    std::shared_ptr<AdapterStateMachine> stateMachine = std::make_shared<AdapterStateMachine>();

    NL_CHECK_RETURN(adapter != nullptr && stateMachine != nullptr,
        "Create %{public}s Failed!!", ADAPTER_NAME_SLE.c_str());
    adapter->GetContext()->RegisterCallback(*(pimpl->contextCallback_));
    stateMachine->Init(adapter);
    {
        std::lock_guard<std::mutex> lock(pimpl->instanceMutex_);
        pimpl->instance_ = adapter;
    }
    pimpl->stateMachine_ = stateMachine;
    pimpl->state_.store(SleStateID::STATE_TURN_OFF);
}

void SleServiceManager::RestoreSleFromCollaboration() const
{
    SleStateID currentState = GetState(SleTransport::ADAPTER_SLE);
    if (currentState == SleStateID::STATE_TURN_OFF) {
        HILOGI("switch nearlink to half-disable because collaboration is on");
        SwitchCallerInfo callerInfo = GetCallerInfo();
        SwitchToHalfTask(SleTransport::ADAPTER_SLE, SleEventType::COLLABORATION_TRIGGERED, callerInfo);
        return;
    }
    HILOGI("nearlink is already in %{public}s, no need to restore", GetStateString(currentState).c_str());
}

int32_t SleServiceManager::RestoreSleFromSystemParameter(
    const SleTransport transport, const SleEventType reason) const
{
    HILOGI("restore nearlink");
    SleStateID parameterState = GetNearlinkSwitchStateFromSystemParameter();
    if (parameterState == SleStateID::STATE_TURN_ON) {
        HILOGI("restore nearlink to ON");
        SwitchCallerInfo callerInfo = GetCallerInfo();
        return EnableTask(transport, reason, callerInfo);
    }
    if (parameterState == SleStateID::STATE_TURN_HALF) {
        bool isCollaborationOn = SleCollaborationManager::GetInstance().IsCollaborationOn();
        HILOGI("isCollaborationOn: %{public}d", isCollaborationOn);
        if (isCollaborationOn) {
            SwitchCallerInfo callerInfo = GetCallerInfo();
            return SwitchToHalfTask(transport, reason, callerInfo);
        } else {
            bool ret = OHOS::system::SetParameter(NEARLINK_SWITCH_SYSTEM_PARAMETER_NAME, NEARLINK_SWITCH_STATE_OFF);
            HILOGI("set nearlink switch_enable parameter to OFF, ret: %{public}d", ret);
        }
    }
    HILOGI("no need to restore nearlink");
    return NL_ERR_INVALID_SWITCH_OPERATION;
}

void SleServiceManager::RestoreSwitchStatus() const
{
    HILOGI("enter");
    SleStateID parameterState = GetNearlinkSwitchStateFromSystemParameter();
    // 自动开星闪 - 开机重启场景
    // 1) 关机前星闪为开启状态，开启星闪
    // 2) 关机前星闪为半关状态，将星闪恢复到半关
    if (parameterState == SleStateID::STATE_TURN_ON || parameterState == SleStateID::STATE_TURN_HALF) {
        WaitDriverLoadCompleted();
        RestoreSleFromSystemParameter(SleTransport::ADAPTER_SLE, SleEventType::RESTORE_TRIGGERED);
        return;
    }

    // 自动将星闪切至半关 - 多设备协同场景
    // 3) 星闪服务已卸载的情况下开启了多设备协同开关，需将星闪切至半关
    if (IsNearlinkNeedTurnHalfFromCollaboration()) {
        const uint64_t delayTimeMs = 500;  // 500 ms
        auto task = []() -> void {
            SleServiceManager::GetInstance()->RestoreSleFromCollaboration();
        };
        DoInServiceManagerThread(task, delayTimeMs); // 此场景可能和主动开星闪场景重合，延迟处理恢复逻辑
        return;
    }

    HILOGI("no need to restore nearlink");
    UnloadSa::GetInstance().StartUnloadNearlinkSaTimer(SleStateID::STATE_TURN_OFF);
}

void SleServiceManager::WaitForAllSwitchDependency()
{
    HILOGI("enter");
    std::function<void(void)> func = [this]() {
        DoInServiceManagerThread([this]() -> void { InitializeAfterAllDependencyOn(); });
    };
    switchDependency_ = std::make_shared<SleSwitchDependency>(func);
    switchDependency_->Init();
}

void SleServiceManager::InitializeAfterAllDependencyOn()
{
    HILOG_COMM_INFO("[%{public}s:%{public}d] enter", __FUNCTION__, __LINE__);
    // Sle all dependency is ready, clear it.
    switchDependency_ = nullptr;

    SleCollaborationManager::GetInstance().Init();
    SleHiviewUe::GetInstance().Init();
    SleDliSnoop::GetInstance().SnoopStartUp();

    if (!NearlinkDataShareHelper::GetInstance().RegisterNameChangeObserver()) {
        HILOGE("RegisterNameChangeObserver failed");
    };
#if (defined(DEVICE_MANAGER))
    std::shared_ptr<DmInitCallback> callback = std::make_shared<DmInitCallback>();
    if (OHOS::DistributedHardware::DeviceManager::GetInstance().InitDeviceManager(
        NEARLINK_SERVICE_NAME, callback) != DM_OK) {
        HILOGE("InitDeviceManager failed");
    };
#endif

#ifndef CONFIG_FACTORY_VERSION
    SleStateID targetState = pimpl->stateMachine_->GetNextTargetState();
    if ((!IsSleSwitchRestricted()) && (targetState == SleStateID::STATE_TURN_OFF)) {
        RestoreSwitchStatus();
    }
#endif
#ifdef NEARLINK_EDM_ENABLE
    SleEdmManager::GetInstance()->Init();
    HILOGI("SleEdmManager init complete");
#endif
    {
        std::lock_guard<std::mutex> lock(pimpl->initializedMutex_);
        pimpl->isInitialized_ = true;
        pimpl->initializedConditionVariable_.notify_all();
    }
}

int SleServiceManager::GetMaxNumConnectedAudioDevices() const
{
    HILOGI("start");

    int value = 0;
    return value;
}

SleStateID SleServiceManager::GetNearlinkSwitchStateFromSystemParameter() const
{
    std::string switchParamStr = OHOS::system::GetParameter(
        NEARLINK_SWITCH_SYSTEM_PARAMETER_NAME, NEARLINK_SWITCH_STATE_OFF);
    NL_CHECK_RETURN_RET(switchParamStr.length() == NEARLINK_SWITCH_PARAMETER_LEN, SleStateID::STATE_TURN_OFF,
        "failed to get nearlink switch_enable parameter");
    HILOGI("get nearlink switch_enable parameter, value=%{public}s", switchParamStr.c_str());
    if (switchParamStr == NEARLINK_SWITCH_STATE_ON) {
        return SleStateID::STATE_TURN_ON;
    }
    if (switchParamStr == NEARLINK_SWITCH_STATE_HALF) {
        return SleStateID::STATE_TURN_HALF;
    }
    return SleStateID::STATE_TURN_OFF;
}

bool SleServiceManager::IsNearlinkNeedTurnHalfFromCollaboration() const
{
    bool isAirplaneOn = NearlinkDataShareHelper::GetInstance().GetAirplaneModeState();
    bool isCollaborationOn = SleCollaborationManager::GetInstance().IsCollaborationOn();
    if (!isAirplaneOn && isCollaborationOn) {
        return true;
    }
    return false;
}

bool SleServiceManager::IsStateStable(SleStateID sleState)
{
    NL_CHECK_RETURN_RET((sleState >= STATE_TURNING_ON && sleState <= STATE_TURN_HALF), false, "invalid state");
    if (sleState == STATE_TURN_ON || sleState == STATE_TURN_OFF || sleState == STATE_TURN_HALF) {
        return true;
    }
    return false;
}

bool SleServiceManager::IsSleSwitchRestricted() const
{
    bool isRestricted = IsSatelliteParameterOn() && IsSatelliteRestrictionValid();
    if (isRestricted) {
        HILOGW("nearlink switch is restricted");
    }
    return isRestricted;
}

void SleServiceManager::UpdateNearlinkSwitchSystemParameter(const SleTransport transport, const SleStateID state) const
{
    NL_CHECK_RETURN(IsStateStable(state), "invalid state: %{public}s", GetStateString(state).c_str());
    SleStateID stateParameter = GetNearlinkSwitchStateFromSystemParameter();
    HILOG_COMM_INFO("[%{public}s:%{public}d]transport(%{public}s), state(%{public}s), stateParameter(%{public}s)",
        __FUNCTION__, __LINE__, GetTransportString(transport).c_str(), GetStateString(state).c_str(),
        GetStateString(stateParameter).c_str());
    bool setParamRet = true;
    if (state == SleStateID::STATE_TURN_ON && stateParameter != SleStateID::STATE_TURN_ON) {
        setParamRet = OHOS::system::SetParameter(NEARLINK_SWITCH_SYSTEM_PARAMETER_NAME, NEARLINK_SWITCH_STATE_ON);
    } else if (state == SleStateID::STATE_TURN_OFF && stateParameter != SleStateID::STATE_TURN_OFF &&
        !isRestoreNeeded_) { // If nearlink need to be restored, keep the system parameter unchanged
        setParamRet = OHOS::system::SetParameter(NEARLINK_SWITCH_SYSTEM_PARAMETER_NAME, NEARLINK_SWITCH_STATE_OFF);
    } else if (state == SleStateID::STATE_TURN_HALF && stateParameter != SleStateID::STATE_TURN_HALF) {
        setParamRet = OHOS::system::SetParameter(NEARLINK_SWITCH_SYSTEM_PARAMETER_NAME, NEARLINK_SWITCH_STATE_HALF);
    } else {
        HILOGE("do nothing");
        return;
    }
    NL_CHECK_RETURN(setParamRet, "failed to set nearlink switch_enable parameter");
}

}  // namespace Nearlink
}  // namespace OHOS