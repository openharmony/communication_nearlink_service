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
#include <cstddef>
#include <cstdint>
#include <array>
#include <string>
#include "SleFeature.h"
#include "parameters.h"
#include "ipc_skeleton.h"
#include "ClassCreator.h"
#include "SleServiceFfrtLog.h"
#include "SleInterfaceManager.h"
#include "SleInterfaceAdapterSub.h"
#include "SleUtils.h"
#include "ThreadUtil.h"
#include "nearlink_safe_map.h"
#include "nearlink_dft_exception.h"
#include "nearlink_permission_manager.h"
#include "nearlink_dft_ue.h"
#include "SleConfig.h"
#include "hadm_defines.h"
#include "nearlink_hadm_stack_adapter.h"
#include "nearlink_hadm_client_service.h"
#include "nearlink_verification_manager.h"
#include "verification_context.h"
#include "SleRemoteDeviceAdapter.h"

namespace OHOS {
namespace Nearlink {
namespace {

const char* const SAVE_IQ_PATH = "/data/service/el1/public/nearlink/";

}

struct HadmClientService::impl {
    impl();
    ~impl();
    class SleConnectionCallback : public ISlePeripheralCallback {
    public:
        SleConnectionCallback(impl &self) : self_(self) {}
        void OnAcbStateChanged(const RawAddress &device, int state, int reason) override;
    private:
        HadmClientService::impl &self_;
    };

    class SleConnectionUpdateCallback : public ISleConnectionCallback {
    public:
        SleConnectionUpdateCallback(impl &self) : self_(self) {}
        void OnConnectionUpdate(uint16_t connHandle, uint16_t minInterval,
            uint16_t maxInterval, uint16_t maxLatency) override;
    private:
        HadmClientService::impl &self_;
    };

    class StackAdapterCallback : public HadmStackAdapterCallback {
    public:
        StackAdapterCallback(impl &self) : self_(self) {}
        void OnSoundingResult(const RawAddress &addr, NearlinkHadmSoundingResult &result) override;
        void OnSoundingStateChange(const RawAddress &addr, int state, int errorCode) override;
        void onSoundingMeasureStateChange(uint8_t status, uint8_t posMeasureSigConfigIdx,
            uint8_t measureState) override;
    private:
        HadmClientService::impl &self_;
    };

    SleConnectionCallback sleConnectionCallback_;
    SleConnectionUpdateCallback sleConnectionUpdateCallback_;

    StackAdapterCallback adapterCallback_;
    NearlinkHadmStackAdapter stackAdapter_;

    NearlinkSafeMap<std::string, uint16_t> connectionInterval_;
    NearlinkSafeMap<std::pair<std::string, int>, std::pair<bool, bool>> allowList_;
    std::mutex mutex_;
    bool restartFlag_ = false;
    std::pair<uint32_t, RawAddress> restartTask_{SLE_HADM_INVALID_ID, RawAddress(INVALID_MAC_ADDRESS)};
    std::pair<uint32_t, RawAddress> currentTask_{SLE_HADM_INVALID_ID, RawAddress(INVALID_MAC_ADDRESS)};
    SLE_DISALLOW_COPY_AND_ASSIGN(impl);
};

void HadmClientService::impl::SleConnectionCallback::OnAcbStateChanged(const RawAddress &device, int state, int reason)
{
    DoInHadmThread([device, state, reason, this]() -> void {
        auto sleAdapter =
            static_cast<SleInterfaceAdapterSub*>(SleInterfaceManager::GetInstance()->GetAdapter(ADAPTER_SLE));
        NL_CHECK_RETURN(sleAdapter, "sleAdapter is NULL.");
        if (state == static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED)) {
            self_.connectionInterval_.Erase(device.GetAddress());
            HILOGI("link disconnected. addr:%{public}s SoundingState:%{public}d, reason:0x%{public}x, size:%{public}d",
                GetEncryptAddr((device).GetAddress()).c_str(),
                HadmClientService::GetInstance().GetSoundingState(device), reason,
                self_.connectionInterval_.Size());
        }
        if (state == static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED) &&
            SleConfig::GetInstance().GetPeerAppearance(device.GetAddress()) == DEVICE_CLASS_VEHICLE_LOCK) {
            HILOGI("carkey link connected, addr:%{public}s", GetEncryptAddr((device).GetAddress()).c_str());
            HadmClientService::GetInstance().ReportHadmSoundingState(self_.currentTask_.second,
                HadmClientService::GetInstance().GetSoundingState(self_.currentTask_.second),
                static_cast<int>(HadmStateChangeReason::CAR_KEY_INTERRUPT), self_.currentTask_.first);
        }
    });
}

void HadmClientService::impl::SleConnectionUpdateCallback::OnConnectionUpdate(
    uint16_t connHandle, uint16_t minInterval, uint16_t maxInterval, uint16_t maxLatency)
{
    DoInHadmThread([connHandle, minInterval, maxLatency, this]() -> void {
        auto sleAdapter =
            static_cast<SleInterfaceAdapterSub*>(SleInterfaceManager::GetInstance()->GetAdapter(ADAPTER_SLE));
        NL_CHECK_RETURN(sleAdapter, "sleAdapter is NULL.");
        std::string addr = SleRemoteDeviceAdapter::GetInstance()->GetAddressByLcid(connHandle);
        uint32_t interval = maxLatency == 0 ? minInterval : maxLatency * minInterval;
        if (interval < MIN_CONNECTION_INTERVAL_LIMIT) {
            self_.connectionInterval_.EnsureInsert(addr, minInterval);
            HILOGI("sle hadm high duty cycle task :%{public}s, interval:0x%{public}x, size:%{public}d",
                GetEncryptAddr(addr).c_str(), interval, self_.connectionInterval_.Size());
        } else {
            if (self_.connectionInterval_.FindIf(addr)) {
                self_.connectionInterval_.Erase(addr);
            }
        }
    });
}

void HadmClientService::impl::StackAdapterCallback::OnSoundingResult(const RawAddress &addr,
    NearlinkHadmSoundingResult &result)
{
    DoInHadmThread([addr, result]() -> void {
        DftCacheRssiValueInfo(addr.GetAddress(), static_cast<int>(result.dutRssi_) - HADM_RSSI_CHANGE_FLAG);
        if (OHOS::system::GetBoolParameter("const.nearlink.saveIQdata.enable", false)) {
            HadmClientService::GetInstance().SaveDutData(result);
            HadmClientService::GetInstance().SaveRtdData(result);
        }
        HadmClientService::GetInstance().ReportSoundingIQResult(addr, result);
    });
}

void HadmClientService::impl::StackAdapterCallback::OnSoundingStateChange(const RawAddress &addr,
    int ctrlType, int errorCode)
{
    DoInHadmThread([addr, ctrlType, errorCode, this]() -> void {
        HILOGI("sounding event. status=0x%{public}d error=0x%{public}d, addr=%{public}s",
            ctrlType, errorCode, GetEncryptAddr(addr.GetAddress()).c_str());
        int state = HADM_SOUNDING_STATE_STOPED;
        if (errorCode == HADM_SUCCESS) {
            if (ctrlType == HADM_SOUNDING_USER_START) {
                state = HADM_SOUNDING_STATE_STARTED;
            } else if (ctrlType == HADM_SOUNDING_USER_STOP && self_.restartFlag_) {
                HadmClientService::GetInstance().ReportHadmSoundingState(addr,
                    HadmClientService::GetInstance().GetSoundingState(addr),
                    static_cast<int>(HadmStateChangeReason::HIGH_PRIORITY_INTERRUPT), self_.currentTask_.first);
                self_.currentTask_ = std::make_pair(SLE_HADM_INVALID_ID, RawAddress(INVALID_MAC_ADDRESS));
                HadmClientService::GetInstance().StartSounding(self_.restartTask_.first,
                    self_.restartTask_.second);
                self_.restartTask_ = std::make_pair(SLE_HADM_INVALID_ID, RawAddress(INVALID_MAC_ADDRESS));
                self_.restartFlag_ = false;
                return;
            }
            HadmClientService::GetInstance().ReportHadmSoundingState(addr, state,
                errorCode, self_.currentTask_.first);
        } else {
            HadmClientService::GetInstance().ReportHadmSoundingState(addr, state,
                static_cast<int>(HadmStateChangeReason::INNER_ERROR), self_.currentTask_.first);
        }
    });
}

void HadmClientService::impl::StackAdapterCallback::onSoundingMeasureStateChange(uint8_t status,
    uint8_t posMeasureSigConfigIdx, uint8_t measureState)
{
    HILOGI("measure state change to %{public}hhu, status %{public}hhu, index %{public}hhu.", measureState,
        status, posMeasureSigConfigIdx);
    NearlinkDftUe::GetInstance().WriteMeasureStateUe(
        DFT_MEASURE_STATE_EVENT_UE, posMeasureSigConfigIdx, status, measureState);
}

HadmClientService::impl::impl() : sleConnectionCallback_(*this), sleConnectionUpdateCallback_(*this),
    adapterCallback_(*this), stackAdapter_(adapterCallback_)
{
    // 白名单{{进程名, 进程uid}, {可打断其他任务, 低时延业务存在时可使用}}
    allowList_.EnsureInsert({CALLER_NAME_FINDNETWORK, CALLER_UID_FINDNETWORK}, {true, true});
}

HadmClientService::impl::~impl()
{
}

HadmClientService::HadmClientService()
    : pimpl(std::make_unique<HadmClientService::impl>())
{
    HILOGE("HadmClientService constructor");
}

HadmClientService::~HadmClientService()
{
    HILOGI("~HadmClientService");
}

InterfaceHadmClientService &InterfaceHadmClientService::GetInstance()
{
    return HadmClientService::GetInstance();
}

HadmClientService &HadmClientService::GetInstance()
{
    // C++11 static local variable initialization is thread-safe.
    static HadmClientService hadmClientService;
    return hadmClientService;
}

void HadmClientService::RegisterNearlinkHadmClientCallback(
    std::shared_ptr<InterfaceHadmClientServiceCallback> callback)
{
    HILOGI("Enter");
    DoInHadmThread([callback, this]() -> void {
        callback_ = callback;
        auto sleAdapter =
            static_cast<SleInterfaceAdapterSub*>(SleInterfaceManager::GetInstance()->GetAdapter(ADAPTER_SLE));
        NL_CHECK_RETURN(sleAdapter, "sleAdapter null");
        sleAdapter->RegisterSlePeripheralCallback(pimpl->sleConnectionCallback_);
        sleAdapter->RegisterSleConnectionCallback(pimpl->sleConnectionUpdateCallback_);
    });
}

void HadmClientService::DeregisterNearlinkHadmClientCallback() const
{
    HILOGI("Enter");
    DoInHadmThread([this]() -> void {
        auto sleAdapter =
            static_cast<SleInterfaceAdapterSub*>(SleInterfaceManager::GetInstance()->GetAdapter(ADAPTER_SLE));
        NL_CHECK_RETURN(sleAdapter, "sleAdapter null");
        sleAdapter->DeregisterSlePeripheralCallback(pimpl->sleConnectionCallback_);
        sleAdapter->DeregisterSleConnectionCallback(pimpl->sleConnectionUpdateCallback_);
    });
}

void HadmClientService::StartSounding(uint32_t hadmId, const RawAddress &addr) const
{
    std::string callerName = NearLinkPermissionManager::GetCallingName();
    int32_t uid = IPCSkeleton::GetCallingUid();
    uint64_t tokenId = IPCSkeleton::GetCallingTokenID();
    HILOGI("Address:%{public}s, caller=%{public}s",
            GetEncryptAddr(addr.GetAddress()).c_str(), callerName.c_str());

    DoInHadmThread([callerName, uid, tokenId, hadmId, addr, this]() -> void {
        HILOGD(" StartSounding enter");
        VerificationContext ctx{tokenId, uid, ""};

        bool allowed = NearlinkVerificationManager::GetInstance().CheckVerification(
            VerificationType::HADM_FULL_SCENARIO_CHECK, ctx);
        if (!allowed) {
            ReportHadmSoundingState(addr, GetSoundingState(addr),
                static_cast<int>(HadmStateChangeReason::FULL_SCENARIO_UNSUPPORTED), hadmId);
            HILOGI("sounding stopped by verification check");
            return;
        }

        // 车钥匙存在时禁止使用测距
        if (!CheckCarkeyState()) {
            ReportHadmSoundingState(addr, GetSoundingState(addr),
                static_cast<int>(HadmStateChangeReason::CAR_KEY_INTERRUPT), hadmId);
            HILOGI("car key exist");
            return;
        }

        std::pair<std::string, int> paramIn = std::make_pair(callerName, uid);
        std::pair<bool, bool> priority = GetUserPriority(paramIn);

        // 低时延业务存在时限制测距
        HILOGI("priority %{public}d, %{public}d", priority.first, priority.second);
        if (!CheckLowLatencyConnection() && !priority.second) {
            ReportHadmSoundingState(addr, GetSoundingState(addr),
                static_cast<int>(HadmStateChangeReason::HIGH_DUTY_CYCLE), hadmId);
            HILOGI("low latency exist");
            return;
        }

        // 存在其他业务在使用测距
        RawAddress currentAddr;
        if ((GetSoundingAddrInfo(currentAddr) != 0)) {
            if (priority.first) {
                pimpl->stackAdapter_.StopSounding(currentAddr, callerName);
                pimpl->restartFlag_ = true;
                pimpl->restartTask_ = std::make_pair(hadmId, addr);
                return;
            } else {
                ReportHadmSoundingState(addr, GetSoundingState(addr),
                    static_cast<int>(HadmStateChangeReason::TASK_BUSY), hadmId);
                return;
            }
        }

        // 启动测距
        pimpl->currentTask_ = std::make_pair(hadmId, addr);
        pimpl->stackAdapter_.StartSounding(addr, callerName);
    });
}

void HadmClientService::StopSounding(uint32_t hadmId, const RawAddress &addr) const
{
    HILOGI("Address:%{public}s", GetEncryptAddr(addr.GetAddress()).c_str());
    std::string callingName = NearLinkPermissionManager::GetCallingName();
    DoInHadmThread([hadmId, addr, callingName, this]() -> void {
        pimpl->stackAdapter_.StopSounding(addr, callingName);
    });
}

void HadmClientService::StopSoundingById(uint32_t hadmId) const
{
    std::string callingName = NearLinkPermissionManager::GetCallingName();
    DoInHadmThread([hadmId, callingName, this]() -> void {
        if (hadmId == pimpl->currentTask_.first) {
            pimpl->stackAdapter_.StopSounding(pimpl->currentTask_.second, callingName);
        }
    });
    
}

uint8_t HadmClientService::GetSoundingState(const RawAddress &device) const
{
    return pimpl->stackAdapter_.GetSoundingState(device);
}

uint32_t HadmClientService::GetSoundingAddrInfo(RawAddress &device) const
{
    return pimpl->stackAdapter_.GetSoundingAddrInfo(device);
}

bool HadmClientService::CheckCarkeyState() const
{
    auto sleInterfaceMgrPtr = SleInterfaceManager::GetInstance()->GetAdapter(SleTransport::ADAPTER_SLE);
    NL_CHECK_RETURN_RET(sleInterfaceMgrPtr, true, "sleService is null.");
    std::vector<RawAddress> connectedDevices = sleInterfaceMgrPtr->GetConnectedDevices();
    for (auto &connectedAddr : connectedDevices) {
        if (sleInterfaceMgrPtr->IsAcbConnected(connectedAddr) &&
            SleConfig::GetInstance().GetPeerAppearance(connectedAddr.GetAddress()) == DEVICE_CLASS_VEHICLE_LOCK) {
            // carkey has the highest priority and needs to stop all other ranging tasks
            HILOGW("high priority carkey task interrupt.");
            return false;
        }
    }
    return true;
}

bool HadmClientService::CheckLowLatencyConnection() const
{
    HILOGW("low latency connection exist size = %{public}d", pimpl->connectionInterval_.Size());
    if (pimpl->connectionInterval_.Size() != 0) {
        return false;
    }
    return true;
}

std::pair<bool, bool> HadmClientService::GetUserPriority(std::pair<std::string, int> info) const
{
    std::pair<bool, bool> priority = {false, false};
    if (!pimpl->allowList_.GetValue(info, priority)) {
        HILOGW("not find caller");
    }
    return priority;
}

void HadmClientService::ReportHadmSoundingState(const RawAddress &addr, int state, int errorcode, uint32_t hadmId) const
{
    HILOGI("ReportHadmSoundingState addr = %{public}s state = %{public}d errorcode = %{public}d, id = %{public}u",
        GetEncryptAddr(addr.GetAddress()).c_str(), state, errorcode, hadmId);
    auto spt = callback_.lock();
    if (spt) {
        spt->OnSoundingStateChange(addr, state, errorcode, hadmId);
    }
}

void HadmClientService::ReportSoundingIQResult(const RawAddress &addr, const NearlinkHadmSoundingResult &result)
{
    NearlinkHadmSoundingResult reportResult = result;
    HILOGI("localNvOffset=%{public}u remoteNvOffset=%{public}u localTofOffset=%{public}u remoteTofOffset=%{public}u",
        reportResult.localNvOffset_, reportResult.remoteNvOffset_,
        reportResult.localTofOffset_, reportResult.remoteTofOffset_);
    DftSetMeasureResultMap(addr.GetAddress(), true);
    auto spt = callback_.lock();
    if (spt) {
        spt->OnSoundingResult(addr, reportResult, pimpl->currentTask_.first);
    }
}

uint32_t HadmClientService::AllocHadmId()
{
    // find mex (minimum excluded value) in set.
    std::promise<uint32_t> hadmIdPromise;
    std::future<uint32_t> future = hadmIdPromise.get_future();
    DoInHadmThread([this, &hadmIdPromise]() -> void {
        uint32_t hadmId = SLE_HADM_INVALID_ID;
        for (uint32_t i = 1; i <= SLE_HADM_REGISTER_MAX_NUM; i++) {
            if (hadmIds_.find(i) == hadmIds_.end()) {
                hadmId = i;
                hadmIds_.insert(hadmId);
                break;
            }
        }
        hadmIdPromise.set_value(hadmId);
        HILOGI("hadmIds_ size(%{public}lu)", hadmIds_.size());
    });
    uint32_t hadmId = future.get();
    HILOGI("sle hadm id=%{public}d", hadmId);
    return hadmId;
}

void HadmClientService::RemoveHadmId(uint32_t hadmId)
{
    DoInHadmThread([this, hadmId]() -> void {
        hadmIds_.erase(hadmId);
        HILOGI("remove hadmId(%{public}d), hadmIds_ size(%{public}lu)", hadmId, hadmIds_.size());
    });
}

uint8_t HadmClientService::GetHadmFeature() const
{
    uint8_t capability = static_cast<uint8_t>(HadmSupportCapability::NOT_SUPPORT);
    if (SleFeature::GetInstance().IsRangSupported()) {
        HILOGI("sle support hadm");
        capability = static_cast<uint8_t>(HadmSupportCapability::SUPPORT_G_INITIATOR);
    }
    return capability;
}

void HadmClientService::SaveDutData(NearlinkHadmSoundingResult soundingResult)
{
    char path_dut[FILE_STRING_LENGTH];
    FILE *iqSaveDutHandle = nullptr;
    HILOGD("open file");
    NL_CHECK_RETURN(sprintf_s(path_dut, sizeof(path_dut), "%s/testINIT%u_0.txt", SAVE_IQ_PATH,
        soundingResult.GetTimeStampSn()) >= 0, "sprintf_s error");
    iqSaveDutHandle = fopen(path_dut, "w+");
    if (iqSaveDutHandle == nullptr) {
        HILOGE("open file ' %{public}s ' error : %{public}s", path_dut, strerror(errno));
        return;
    }
    std::vector<uint16_t> dutIData = soundingResult.GetDutIData();
    std::vector<uint16_t> dutQData = soundingResult.GetDutQData();
    uint8_t dutRssi = soundingResult.dutRssi_;
    for (uint8_t i = 0; i < dutIData.size(); i++) {
        fprintf(iqSaveDutHandle, "%x %u %x %x %04x %04x %04x %u\n",
            dutRssi, i, dutIData[i], dutQData[i], 0, 0, 0, soundingResult.dutIqBitLen_);
    }
    fclose(iqSaveDutHandle);
    HILOGD("close file");
}

void HadmClientService::SaveRtdData(NearlinkHadmSoundingResult soundingResult)
{
    char path_rtd[FILE_STRING_LENGTH];
    FILE *iqSaveRtdHandle = nullptr;
    HILOGD("open file");
    NL_CHECK_RETURN(sprintf_s(path_rtd, sizeof(path_rtd), "%s/testREFL%u_0.txt", SAVE_IQ_PATH,
        soundingResult.GetTimeStampSn()) >= 0, "sprintf_s error");
    iqSaveRtdHandle = fopen(path_rtd, "w+");
    if (iqSaveRtdHandle == nullptr) {
        HILOGE("open file ' %{public}s ' error : %{public}s", path_rtd, strerror(errno));
        return;
    }
    std::vector<uint16_t> rtdIData = soundingResult.GetRtdIData();
    std::vector<uint16_t> rtdQData = soundingResult.GetRtdQData();
    uint8_t rtdRssi = soundingResult.rtdRssi_;
    for (uint8_t i = 0; i < rtdIData.size(); i++) {
        fprintf(iqSaveRtdHandle, "%x %u %x %x %04x %04x %04x %u\n",
            rtdRssi, i, rtdIData[i], rtdQData[i], 0, 0, 0, soundingResult.rtdIqBitLen_);
    }
    fclose(iqSaveRtdHandle);
    HILOGD("close file");
}

} // namespace Nearlink
} // namespace OHOS