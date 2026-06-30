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

#include "nearlink_sle_central_manager_server.h"
#include "nearlink_sle_central_manager_stub.h"
#include "nearlink_utils_server.h"
#include "SleInterfaceManager.h"
#include "interface_scan_service.h"
#include "nearlink_errorcode.h"
#include "remote_observer_list.h"
#include "log.h"
#include "ipc_skeleton.h"
#include "nearlink_permission_manager.h"
#include "nearlink_device_manager.h"
#include "nearlink_remote_container.h"
#include "nearlink_dft_exception.h"
#include "accesstoken_kit.h"
#include "parameters.h"
#include "nearlink_verification_manager.h"
#include "nearlink_system_config.h"
#include "nearlink_raw_address.h"
namespace OHOS {
namespace Nearlink {
namespace {
    constexpr int8_t RSSI_WALLET_SCAN_RSSI_THRESHOLD = -90;  // 扫描RSSI过滤器配置-90db
}
struct NearlinkSleCentralManagerServer::impl {
    impl();
    ~impl();

    /// sys state observer
    class SystemStateObserver;
    std::unique_ptr<SystemStateObserver> systemStateObserver_ = nullptr;

    // store base class sptr inside, no truncate happend
    RemoteObserverList<INearlinkSleCentralManagerCallback> observers_;

    struct SleCentralManagerRemoteInfo;
    class SleCentralManagerRemoteContainer;
    // weak for deathReipient of container
    std::shared_ptr<SleCentralManagerRemoteContainer> remoteContainer_ =
        std::make_shared<SleCentralManagerRemoteContainer>();

    class SleCentralManagerCallback;
    // weak for underlayer to store observer
    std::shared_ptr<SleCentralManagerCallback> observerImp_ = std::make_shared<SleCentralManagerCallback>(this);
    bool isAudioSupported = false;
};

struct NearlinkSleCentralManagerServer::impl::SleCentralManagerRemoteInfo {
    SleCentralManagerRemoteInfo () : pid(0), uid(0), tokenId(0), scannerId(SLE_SCAN_INVALID_ID) { }
    SleCentralManagerRemoteInfo (int32_t pid, int32_t uid, uint64_t tokenId, uint32_t scannerId)
        : pid(pid), uid(uid), tokenId(tokenId), scannerId(scannerId) {}
    int32_t pid;
    int32_t uid;
    uint64_t tokenId;
    uint32_t scannerId;
};

//LCOV_EXCL_BR_START
class NearlinkSleCentralManagerServer::impl::SleCentralManagerRemoteContainer final
    : public NearlinkRemoteContainer<NearlinkSleCentralManagerServer::impl::SleCentralManagerRemoteInfo> {
public:
    ~SleCentralManagerRemoteContainer() override = default;
    void OnRemoteDied(const wptr<IRemoteObject> &remote) override
    {
        HILOGI("enter");
        uint32_t scannerId = SLE_SCAN_INVALID_ID;
        {
            std::lock_guard<std::mutex> lk(vecMutex_);
            auto it = std::find_if(vec_.begin(), vec_.end(), [remote](const auto &obj) { return obj.first == remote; });
            NL_CHECK_RETURN(it != vec_.end(), "remote info unexpectedly not found");
            scannerId = it->second.scannerId;
            vec_.erase(it);
        }
        InterfaceScanService::GetInstance().StopScan(scannerId);
        InterfaceScanService::GetInstance().RemoveScannerId(scannerId);
    }

    bool IsRemoteScannerId(uint32_t scannerId)
    {
        std::lock_guard<std::mutex> lk(vecMutex_);
        int32_t pid = IPCSkeleton::GetCallingPid();
        int32_t uid = IPCSkeleton::GetCallingUid();
        auto it = std::find_if(vec_.begin(), vec_.end(), [scannerId](const auto &obj) {
            return obj.second.scannerId == scannerId; });
        if (it == vec_.end()) {
            HILOGE("can't find scannerId: %{public}d", scannerId);
            return false;
        }
        bool ret = (it->second.pid == pid && it->second.uid == uid);
        if (!ret) {
            HILOGE("scannerId: %{public}d, pid: %{public}d, uid: %{public}d", scannerId, pid, uid);
        }
        return ret;
    }
};

NearlinkSleCentralManagerServer::impl::impl()
{
    remoteContainer_->Init();
}

NearlinkSleCentralManagerServer::impl::~impl()
{
    HILOGW("NearlinkSleCentralManagerServer: ~impl()");
    InterfaceScanService::GetInstance().DeregisterSleCentralManagerCallback();
}

class NearlinkSleCentralManagerServer::impl::SleCentralManagerCallback : public ISleCentralManagerCallback {
public:
    explicit SleCentralManagerCallback(NearlinkSleCentralManagerServer::impl *pimpl) : pimpl_(pimpl) {};
    ~SleCentralManagerCallback() override = default;

    void OnScanCallback(const std::vector<uint32_t> &scannerIds, const SleScanResultImpl &result) override
    {
        HILOGD("Address: %{public}s",
            GetEncryptAddr(result.GetPeripheralDevice().GetRawAddress().GetAddress()).c_str());
        DftSetAccurateSearchTaskMap(result.GetPeripheralDevice().GetRawAddress().GetAddress());
        observers_->ForEach([this, scannerIds, result](INearlinkSleCentralManagerCallback *observer) {
            uint32_t scannerId = GetScannerId(observer->AsObject());
            NL_CHECK_RETURN(scannerId != SLE_SCAN_INVALID_ID, "cannot retrieve remote info");
            auto it = std::find(scannerIds.begin(), scannerIds.end(), scannerId);
            NL_CHECK_RETURN_LOGD(it != scannerIds.end(), "scanner id %{public}d not match", scannerId);
            SleCentralManagerRemoteInfo info = pimpl_->remoteContainer_->RetrieveRemoteInfo(observer->AsObject());
            NL_CHECK_RETURN(NearLinkPermissionManager::VerifyPermission(ACCESS_NEARLINK, info.tokenId),
                "false, check permission failed");

            VerificationContext ctx = { .tokenId = info.tokenId };
            bool isDisplayRestrictedCaller = NearlinkVerificationManager::GetInstance().CheckVerification(
                VerificationType::SCAN_RESULT_FILTER, ctx);
            if (!IsDeviceSupportDisplay(result) && isDisplayRestrictedCaller) {
                HILOGD("Device %{public}s not Support Display.",
                    GetEncryptAddr(result.GetPeripheralDevice().GetRawAddress().GetAddress()).c_str());
                return;
            }

            bool isUseRealAddrFlag = NearLinkPermissionManager::VerifyPermission(GET_NEARLINK_PEER_MAC, info.tokenId);
            RawAddress realAddr = result.GetPeripheralDevice().GetRawAddress();
            NearlinkRawAddress randomAddr;
            NearlinkDeviceManager::GetInstance()->ConvertToRandomAddress(
                isUseRealAddrFlag, realAddr, randomAddr, false);
            NearlinkSleScanResult sleScanResult(result);
            sleScanResult.SetPeripheralDevice(randomAddr);
            sleScanResult.SetPrimFrameType(result.GetPrimFrameType());
            int status = SleInterfaceManager::GetInstance()->GetState(SleTransport::ADAPTER_SLE);
            if (status != SleStateID::STATE_TURN_ON && status != SleStateID::STATE_TURN_HALF) {
                HILOGI("nearlink is not turn on, stop uploading scan result");
                return;
            }
            HILOGD("frameType:%{public}d", sleScanResult.GetPrimFrameType());
            observer->OnScanCallback(sleScanResult);
        });
    }

    void OnSleBatchScanResultsEvent(std::vector<SleScanResultImpl> &results) override
    {
        HILOGI("not implement");
    }

    void OnStartOrStopScanEvent(int resultCode, bool isStartScanEvt) override
    {
        HILOGI("code(%{public}d), isStartScanEvt(%{public}d))", resultCode, isStartScanEvt);
        observers_->ForEach([resultCode, isStartScanEvt](INearlinkSleCentralManagerCallback *observer) {
            observer->OnStartOrStopScanEvent(resultCode, isStartScanEvt);
        });
    }

    void SetObserver(RemoteObserverList<INearlinkSleCentralManagerCallback> *observers)
    {
        observers_ = observers;
    }

    uint32_t GetScannerId(const sptr<IRemoteObject> &remote) const
    {
        SleCentralManagerRemoteInfo info = pimpl_->remoteContainer_->RetrieveRemoteInfo(remote);
        return info.scannerId;
    }

    bool IsDeviceSupportDisplay(const SleScanResultImpl &result)
    {
        SlePeripheralDevice device = result.GetPeripheralDevice();
        if (!pimpl_->isAudioSupported && !device.GetIsDeviceDisplay()) {
            // 当前设备不支持音频且当前广播报vendor厂商设备不可显示
            return false;
        }
        if (!pimpl_->isAudioSupported && device.GetIsAudioDeviceFlag()) {
            // 当前设备不支持音频且当前广播报音频设备不可显示
            return false;
        }
        uint8_t flag = device.GetAdFlag();
        if (flag == static_cast<uint8_t>(SleAdvertiserFlag::SLE_ADV_FLAG_GENERAL_DISC) ||
            flag == static_cast<uint8_t>(SleAdvertiserFlag::SLE_ADV_FLAG_PRIOR_DISC)) {
            return true;
        }
        return false;
    }

private:
    RemoteObserverList<INearlinkSleCentralManagerCallback> *observers_ = nullptr;
    NearlinkSleCentralManagerServer::impl *pimpl_ = nullptr;
};

class NearlinkSleCentralManagerServer::impl::SystemStateObserver : public ISystemStateObserver {
public:
    explicit SystemStateObserver(NearlinkSleCentralManagerServer::impl *pimpl) : pimpl_(pimpl){};
    void OnSystemStateChange(const SleSystemState state) override
    {
        HILOGI("OnSystemStateChange %{public}d.", state);
        if (state == SleSystemState::ON) {
            InterfaceScanService::GetInstance().RegisterSleCentralManagerCallback(*pimpl_->observerImp_.get());
        }
    };

private:
    NearlinkSleCentralManagerServer::impl *pimpl_ = nullptr;
};

NearlinkSleCentralManagerServer::NearlinkSleCentralManagerServer()
{
    pimpl = std::make_unique<impl>();
    pimpl->observerImp_->SetObserver(&(pimpl->observers_));
    pimpl->systemStateObserver_ = std::make_unique<impl::SystemStateObserver>(pimpl.get());
    SleInterfaceManager::GetInstance()->RegisterSystemStateObserver(*(pimpl->systemStateObserver_));
    InterfaceScanService::GetInstance().RegisterSleCentralManagerCallback(*pimpl->observerImp_.get());
    pimpl->isAudioSupported = NearlinkSystemConfig::IsAudioSupported();
}

NearlinkSleCentralManagerServer::~NearlinkSleCentralManagerServer()
{
    HILOGI("~NearlinkSleCentralManagerServer called.");
    SleInterfaceManager::GetInstance()->DeregisterSystemStateObserver(*(pimpl->systemStateObserver_));
}

std::vector<NearlinkSleScanFilter> NearlinkSleCentralManagerServer::SetRssiFilter(
    const std::vector<NearlinkSleScanFilter>& filters)
{
    uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
    std::string callingName = NearLinkPermissionManager::GetCallingName(tokenId);
    VerificationContext ctx = { .tokenId = tokenId };
    bool result = NearlinkVerificationManager::GetInstance().CheckVerification(VerificationType::SCAN_RSSI_FILTER, ctx);
    HILOGI("callingName: %{public}s, result: %{public}d", callingName.c_str(), result);
    if (result) {
        HILOGI("Need modify RSSI filter for Wallet Service");
        std::vector<NearlinkSleScanFilter> modifiedFilters;
        for (const auto& filter : filters) {
            if (!filter.HasRssiThreshold()) {
                HILOGI("set default RSSI filter for Wallet Service");
                NearlinkSleScanFilter newFilter = filter;
                newFilter.SetRssiThreshold(RSSI_WALLET_SCAN_RSSI_THRESHOLD);
                modifiedFilters.push_back(newFilter);
            }
        }
        return modifiedFilters;
    }
    return filters;
}

NlErrCode NearlinkSleCentralManagerServer::StartScanWithFilter(uint32_t scannerId,
    const NearlinkSleScanSettings &settings, const std::vector<NearlinkSleScanFilter> &filters)
{
    HILOGI("Enter StartScanWithFilter, scannerId: %{public}d, filter size: %{public}zu", scannerId, filters.size());
    std::vector<NearlinkSleScanFilter> modifiedFilters = SetRssiFilter(filters);
    return StartScan(scannerId, settings, modifiedFilters);
}

NlErrCode NearlinkSleCentralManagerServer::StartFullScan(uint32_t scannerId, const NearlinkSleScanSettings &settings)
{
    HILOGI("scannerId(%{public}d)", scannerId);
    NL_CHECK_RETURN_RET(pimpl->remoteContainer_->IsRemoteScannerId(scannerId), NL_ERR_INTERNAL_ERROR,
        "scannerId is invalid.");
    std::vector<NearlinkSleScanFilter> emptyFilters;
    return StartScan(scannerId, settings, emptyFilters);
}

NlErrCode NearlinkSleCentralManagerServer::StartScan(uint32_t scannerId, const NearlinkSleScanSettings &settings,
    const std::vector<NearlinkSleScanFilter> &filters)
{
    HILOGI("scannerId(%{public}d), filter size (%{public}zu)", scannerId, filters.size());
    NL_CHECK_RETURN_RET(pimpl->remoteContainer_->IsRemoteScannerId(scannerId), NL_ERR_INTERNAL_ERROR,
        "scannerId is invalid.");
    NL_CHECK_RETURN_RET(scannerId != SLE_SCAN_INVALID_ID, NL_ERR_INTERNAL_ERROR, "invalid scannerId");

    std::vector<SleScanFilterImpl> filterImpls;
    ConvertToScanFilterImpl(filters, filterImpls);
    InterfaceScanService::GetInstance().StartScan(scannerId, settings, filterImpls);
    return NL_NO_ERROR;
}

void NearlinkSleCentralManagerServer::ConvertToScanFilterImpl(const std::vector<NearlinkSleScanFilter> &filters,
    std::vector<SleScanFilterImpl> &filterImpls)
{
    for (auto filter : filters) {
        SleScanFilterImpl filterImpl;
        filterImpl.SetDeviceId(filter.GetDeviceId());
        filterImpl.SetName(filter.GetName());
        if (filter.HasServiceUuid()) {
            filterImpl.SetServiceUuid(filter.GetServiceUuid());
        }
        if (filter.HasServiceUuidMask()) {
            filterImpl.SetServiceUuidMask(filter.GetServiceUuidMask());
        }
        if (filter.HasSolicitationUuid()) {
            filterImpl.SetServiceSolicitationUuid(filter.GetServiceSolicitationUuid());
        }
        if (filter.HasSolicitationUuidMask()) {
            filterImpl.SetServiceSolicitationUuidMask(filter.GetServiceSolicitationUuidMask());
        }
        if (filter.HasServiceData()) {
            filterImpl.SetServiceData(filter.GetServiceData());
        }
        if (filter.HasServiceDataMask()) {
            filterImpl.SetServiceDataMask(filter.GetServiceDataMask());
        }
        if (filter.HasRssiThreshold()) {
            filterImpl.SetRssiThreshold(filter.GetRssiThreshold());
        }
        filterImpl.SetManufacturerId(filter.GetManufacturerId());
        filterImpl.SetManufactureData(filter.GetManufactureData());
        filterImpl.SetManufactureDataMask(filter.GetManufactureDataMask());
        filterImpl.SetSensorHubChannel(filter.IsSensorHubChannel());
        filterImpl.SetAdvIndReport(filter.GetAdvIndReport());
        filterImpls.push_back(filterImpl);
    }
}

NlErrCode NearlinkSleCentralManagerServer::StopScan(uint32_t scannerId)
{
    HILOGI("Enter!");
    NL_CHECK_RETURN_RET(pimpl->remoteContainer_->IsRemoteScannerId(scannerId), NL_ERR_INTERNAL_ERROR,
        "scannerId is invalid.");
    NL_CHECK_RETURN_RET(scannerId != SLE_SCAN_INVALID_ID, NL_ERR_INTERNAL_ERROR, "invalid scannerId");
    InterfaceScanService::GetInstance().StopScan(scannerId);
    return NL_NO_ERROR;
}

NlErrCode NearlinkSleCentralManagerServer::RegisterSleCentralManagerCallback(uint32_t &scannerId,
    bool enableRandomAddrMode, const sptr<INearlinkSleCentralManagerCallback> &callback)
{
    int32_t pid = IPCSkeleton::GetCallingPid();
    int32_t uid = IPCSkeleton::GetCallingUid();
    HILOGI("pid: %{public}d, uid: %{public}d", pid, uid);

    NL_CHECK_RETURN_RET(callback, NL_ERR_INVALID_PARAM, "callback is null");
    NL_CHECK_RETURN_RET(pimpl->observers_.Size() < MAX_OBSERVER_SIZE,
        NL_ERR_INTERNAL_ERROR, "observers exceeds the range");

    scannerId = InterfaceScanService::GetInstance().AllocScannerId();
    NL_CHECK_RETURN_RET(scannerId != SLE_SCAN_INVALID_ID, NL_ERR_INTERNAL_ERROR, "alloc scannerId failed.");

    pimpl->observers_.Register(callback);
    uint64_t tokenId = IPCSkeleton::GetCallingFullTokenID();
    impl::SleCentralManagerRemoteInfo info(pid, uid, tokenId, scannerId);
    pimpl->remoteContainer_->AddRemoteInfo(callback->AsObject(), info);
    return NL_NO_ERROR;
}

NlErrCode NearlinkSleCentralManagerServer::DeregisterSleCentralManagerCallback(uint32_t scannerId,
    const sptr<INearlinkSleCentralManagerCallback> &callback)
{
    HILOGI("scannerId: %{public}u", scannerId);
    NL_CHECK_RETURN_RET(callback, NL_ERR_INTERNAL_ERROR, "callback is null");

    NL_CHECK_RETURN_RET(pimpl->remoteContainer_->IsRemoteScannerId(scannerId), NL_ERR_INVALID_PARAM,
        "scannerId is invalid.");
    pimpl->observers_.Deregister(callback);
    pimpl->remoteContainer_->DeleteRemoteInfo(callback->AsObject());
    InterfaceScanService::GetInstance().RemoveScannerId(scannerId);
    return NL_NO_ERROR;
}
//LCOV_EXCL_BR_STOP
}  // namespace Nearlink
}  // namespace OHOS