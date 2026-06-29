/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "nearlink_sle_datatransfer_server.h"

#include "nearlink_errorcode.h"
#include "SleInterfaceDataTransfer.h"
#include "SleInterfaceManager.h"
#include "SleInterfaceAdapterSub.h"
#include "log.h"
#include "ipc_skeleton.h"
#include "nearlink_remote_container.h"
#include "remote_observer_list.h"
#include "nearlink_device_manager.h"
#include "nearlink_permission_manager.h"
#include "log_util.h"
#include "nearlink_common_event_helper.h"
#include "nearlink_permission_manager.h"
#include "nearlink_raw_address.h"
#ifdef NEARLINK_KIA_ENABLE
#include "SleKiaManager.h"
#endif
#ifdef NEARLINK_EDM_ENABLE
#include "SleEdmManager.h"
#endif

namespace OHOS::Nearlink {
namespace {
    constexpr uint64_t SLE_DATATRANSFER_INVALID_ID = 0;
}
struct NearlinkSleDataTransferServer::impl {
    impl();
    ~impl();

    /// sys state observer
    class SystemStateObserver;
    std::unique_ptr<SystemStateObserver> systemStateObserver_ = nullptr;

    struct SleDataTransferRemoteInfo;
    class SleDataTransferRemoteContainer;
    // weak for deathReipient of container
    std::shared_ptr<SleDataTransferRemoteContainer> remoteContainer_ =
            std::make_shared<SleDataTransferRemoteContainer>();

    RemoteObserverList<INearlinkSleDataTransferCallback> observers_{};
    class SleDataTransferServiceCallback;
    std::shared_ptr<SleDataTransferServiceCallback> observerImp_ =
        std::make_shared<SleDataTransferServiceCallback>(this);
};

struct NearlinkSleDataTransferServer::impl::SleDataTransferRemoteInfo {
    SleDataTransferRemoteInfo() : pid(0), uid(0), tokenId(SLE_DATATRANSFER_INVALID_ID)
    {}
    SleDataTransferRemoteInfo(int32_t pid, int32_t uid, uint64_t tokenId) : pid(pid), uid(uid), tokenId(tokenId)
    {}
    int32_t pid;
    int32_t uid;
    uint64_t tokenId;
    std::set<uint16_t> ports {};
};

class NearlinkSleDataTransferServer::impl::SleDataTransferRemoteContainer final
        : public NearlinkRemoteContainer<NearlinkSleDataTransferServer::impl::SleDataTransferRemoteInfo> {
public:
    ~SleDataTransferRemoteContainer() override = default;
    void OnRemoteDied(const wptr<IRemoteObject> &remote) override
    {
        HILOGI("DT server app die");
        uint64_t tokenId = 0;
        {
            std::lock_guard<std::mutex> lk(vecMutex_);
            auto it = std::find_if(vec_.begin(), vec_.end(), [remote](const auto &obj) { return obj.first == remote; });
            NL_CHECK_RETURN(it != vec_.end(), "remote info unexpectedly not found");
            tokenId = it->second.tokenId;
            HILOGI("tokenId: %{public}lu, pid: %{public}d, uid: %{public}d",
                it->second.tokenId, it->second.pid, it->second.uid);
            vec_.erase(it);
        }
        SleInterfaceDataTransfer::GetInstance().ClearCacheByTokenId(tokenId);
    }

    void AddDtPort(int32_t pid, int32_t uid, uint64_t tokenId, uint16_t portId)
    {
        std::lock_guard<std::mutex> lk(vecMutex_);
        bool has = false;
        for (auto& obj : vec_) {
            if (obj.second.pid == pid && obj.second.uid == uid && obj.second.tokenId == tokenId) {
                obj.second.ports.insert(portId);
                has = true;
            }
        }
        NL_CHECK_RETURN(has, "remote info unexpectedly not found");
    }

    void RemoveDtPort(int32_t pid, int32_t uid, uint64_t tokenId, uint16_t portId)
    {
        std::lock_guard<std::mutex> lk(vecMutex_);
        bool has = false;
        for (auto& obj : vec_) {
            if (obj.second.pid == pid && obj.second.uid == uid && obj.second.tokenId == tokenId) {
                obj.second.ports.erase(portId);
                has = true;
            }
        }
        NL_CHECK_RETURN(has, "remote info unexpectedly not found");
    }

    bool CheckApp(int32_t pid, int32_t uid, uint64_t tokenId, uint16_t portId)
    {
        HILOGI("tokenId: %{public}lu, pid: %{public}d, uid: %{public}d, portId: %{public}d", tokenId, pid, uid, portId);
        std::lock_guard<std::mutex> lk(vecMutex_);
        auto it = std::find_if(vec_.begin(), vec_.end(), [pid, uid, tokenId](const auto &obj) {
            return obj.second.pid == pid && obj.second.uid == uid && obj.second.tokenId == tokenId; });
        if (it == vec_.end()) {
            HILOGE("can't find process info.");
            return false;
        }
        bool ret = (it->second.ports.find(portId) != it->second.ports.end());
        if (!ret) {
            HILOGE("can't find portId.");
        }
        return ret;
    }
};

class NearlinkSleDataTransferServer::impl::SleDataTransferServiceCallback : public ISleDataTransferServiceCallback {
public:
    explicit SleDataTransferServiceCallback(NearlinkSleDataTransferServer::impl *pimpl) : pimpl_(pimpl) {};
    ~SleDataTransferServiceCallback() override = default;
    void OnConnectionStateChanged(const DataTransferConnectionParams &connectionParams, int fd) override
    {
        HILOGI("srcPort: %{public}d, state: %{public}d", connectionParams.GetPort(), connectionParams.GetState());
        observers_->ForEach([this, connectionParams, fd](INearlinkSleDataTransferCallback *observer) {
            SleDataTransferRemoteInfo info = pimpl_->remoteContainer_->RetrieveRemoteInfo(observer->AsObject());
            NL_CHECK_RETURN(info.tokenId != SLE_DATATRANSFER_INVALID_ID, "cannot retrieve remote info");
            if (info.ports.find(connectionParams.GetPort()) == info.ports.end()) { // 仅上报对应应用
                return;
            }
            std::string realAddr = connectionParams.GetAddress();
            std::string randomAddr = GetRandomAddr(realAddr, info.tokenId);
            NearlinkSleDataTransferConnectionParams params(connectionParams);
            params.SetAddress(randomAddr);
            observer->OnConnectionStateChanged(params, fd);
            NearlinkHelper::NearlinkCommonEventHelper::PublishSleDataTransferEvent(connectionParams.GetState(),
                connectionParams.GetAddress(), connectionParams.GetUuid(),
                NearLinkPermissionManager::GetCallingName(info.tokenId));
        });
    }

    std::string GetRandomAddr(const std::string &address, uint32_t tokenId) override
    {
        bool isUseRealAddrFlag = NearLinkPermissionManager::VerifyPermission(GET_NEARLINK_PEER_MAC, tokenId);
        RawAddress realAddr(address);
        NearlinkRawAddress randomAddr;
        NearlinkDeviceManager::GetInstance()->ConvertToRandomAddress(
            isUseRealAddrFlag, realAddr, randomAddr, false);
        return randomAddr.GetAddress();
    }

    void SetObserver(RemoteObserverList<INearlinkSleDataTransferCallback> *observers)
    {
        observers_ = observers;
    }

private:
    // it's thread safety in observers_.
    RemoteObserverList<INearlinkSleDataTransferCallback> *observers_ = nullptr;
    NearlinkSleDataTransferServer::impl *pimpl_ = nullptr;
};

class NearlinkSleDataTransferServer::impl::SystemStateObserver : public ISystemStateObserver {
public:
    explicit SystemStateObserver(NearlinkSleDataTransferServer::impl *pimpl) : pimpl_(pimpl){};
    void OnSystemStateChange(const SleSystemState state) override
    {
        HILOGI("state= %{public}d.", state);
        if (state == SleSystemState::ON) {
            SleInterfaceDataTransfer::GetInstance().RegisterSleDataTransferServiceCallback(pimpl_->observerImp_);
        }
    };

private:
    NearlinkSleDataTransferServer::impl *pimpl_ = nullptr;
};

NearlinkSleDataTransferServer::impl::impl()
{
    remoteContainer_->Init();
}

NearlinkSleDataTransferServer::impl::~impl()
{
    HILOGI("DT server: ~impl()");
    SleInterfaceDataTransfer::GetInstance().DeregisterSleDataTransferServiceCallback();
}

NearlinkSleDataTransferServer::NearlinkSleDataTransferServer()
{
    HILOGI("construct");
    pimpl = std::make_unique<impl>();
    pimpl->observerImp_->SetObserver(&(pimpl->observers_));
    pimpl->systemStateObserver_ = std::make_unique<impl::SystemStateObserver>(pimpl.get());
    SleInterfaceManager::GetInstance()->RegisterSystemStateObserver(*(pimpl->systemStateObserver_));

    SleInterfaceDataTransfer::GetInstance().RegisterSleDataTransferServiceCallback(pimpl->observerImp_);
}

NearlinkSleDataTransferServer::~NearlinkSleDataTransferServer()
{
    HILOGI("~NearlinkSleDataTransferServer");
    SleInterfaceManager::GetInstance()->DeregisterSystemStateObserver(*(pimpl->systemStateObserver_));
}

NlErrCode NearlinkSleDataTransferServer::CreatePort(const std::string &uuid, uint16_t &port)
{
    int32_t pid = IPCSkeleton::GetCallingPid();
    int32_t uid = IPCSkeleton::GetCallingUid();
    uint64_t tokenId = IPCSkeleton::GetCallingFullTokenID();
#ifdef NEARLINK_KIA_ENABLE
    NL_CHECK_RETURN_RET(!SleKiaManager::GetInstance().ShouldRefuseConnect(REFUSE_PROTOCOL_TYPE_DATA_TRANSFER, pid),
        NL_ERR_PROFILE_PROHIBITED_BY_EDM, "kia refuse.");
#endif
#ifdef NEARLINK_EDM_ENABLE
    NL_CHECK_RETURN_RET(SleEdmManager::GetInstance()->IsAllowedConnect(ALLOW_PROTOCOL_TYPE_DATA_TRANSFER),
        NL_ERR_PROFILE_PROHIBITED_BY_EDM, "edm refuse.");
#endif
    port = SleInterfaceDataTransfer::GetInstance().CreatePort(uuid);
    NL_CHECK_RETURN_RET(port != 0, NL_ERR_INTERNAL_ERROR, "Alloc port fail.");
    pimpl->remoteContainer_->AddDtPort(pid, uid, tokenId, port);
    return NL_NO_ERROR;
}

NlErrCode NearlinkSleDataTransferServer::DestroyPort(const std::string &uuid, uint16_t port)
{
    int32_t pid = IPCSkeleton::GetCallingPid();
    int32_t uid = IPCSkeleton::GetCallingUid();
    uint64_t tokenId = IPCSkeleton::GetCallingFullTokenID();
    NL_CHECK_RETURN_RET(pimpl->remoteContainer_->CheckApp(pid, uid, tokenId, port), NL_ERR_INVALID_PARAM,
        "tokenId is invalid.");
    pimpl->remoteContainer_->RemoveDtPort(pid, uid, tokenId, port);
    SleInterfaceDataTransfer::GetInstance().DestroyPort(uuid, port);
    return NL_NO_ERROR;
}

NlErrCode NearlinkSleDataTransferServer::Connect(NearlinkSleDataTransferConnectionParams &params)
{
    HILOGD("enter");
    int32_t pid = IPCSkeleton::GetCallingPid();
    int32_t uid = IPCSkeleton::GetCallingUid();
    uint64_t tokenId = IPCSkeleton::GetCallingFullTokenID();
    NL_CHECK_RETURN_RET(pimpl->remoteContainer_->CheckApp(pid, uid, tokenId, params.port_), NL_ERR_INVALID_PARAM,
        "tokenId is invalid.");
#ifdef NEARLINK_KIA_ENABLE
    NL_CHECK_RETURN_RET(!SleKiaManager::GetInstance().ShouldRefuseConnect(REFUSE_PROTOCOL_TYPE_DATA_TRANSFER, pid),
        NL_ERR_PROFILE_PROHIBITED_BY_EDM, "kia refuse.");
#endif
#ifdef NEARLINK_EDM_ENABLE
    NL_CHECK_RETURN_RET(SleEdmManager::GetInstance()->IsAllowedConnect(ALLOW_PROTOCOL_TYPE_DATA_TRANSFER),
        NL_ERR_PROFILE_PROHIBITED_BY_EDM, "edm refuse.");
#endif
    std::string realAddr = "";
    NearlinkDeviceManager::GetInstance()->GetDeviceRealAddr(params.address_, realAddr);
    params.address_ = realAddr;
    SleInterfaceDataTransfer::GetInstance().Connect(params);
    return NL_NO_ERROR;
}

NlErrCode NearlinkSleDataTransferServer::Disconnect(NearlinkSleDataTransferConnectionParams &params)
{
    HILOGI("enter");
    int32_t pid = IPCSkeleton::GetCallingPid();
    int32_t uid = IPCSkeleton::GetCallingUid();
    uint64_t tokenId = IPCSkeleton::GetCallingFullTokenID();
    NL_CHECK_RETURN_RET(pimpl->remoteContainer_->CheckApp(pid, uid, tokenId, params.port_), NL_ERR_INVALID_PARAM,
        "tokenId is invalid.");
    std::string realAddr = "";
    NearlinkDeviceManager::GetInstance()->GetDeviceRealAddr(params.address_, realAddr);
    params.address_ = realAddr;
    SleInterfaceDataTransfer::GetInstance().Disconnect(params);
    return NL_NO_ERROR;
}

NlErrCode NearlinkSleDataTransferServer::GetConnectionState(
    NearlinkSleDataTransferConnectionParams &params, int32_t &connState)
{
    HILOGD("enter");
    std::string realAddr = "";
    NearlinkDeviceManager::GetInstance()->GetDeviceRealAddr(params.address_, realAddr);
    params.address_ = realAddr;
    connState = SleInterfaceDataTransfer::GetInstance().GetConnectionState(params);
    return NL_NO_ERROR;
}

NlErrCode NearlinkSleDataTransferServer::SocketEmptyMsg(uint16_t portId, std::string address)
{
    HILOGI("enter");
    int result = 0;
    SleInterfaceDataTransfer::GetInstance().ChangeSocketState(portId, address, result);
    return NL_NO_ERROR;
}

#ifdef WATCH_STANDARD
NlErrCode NearlinkSleDataTransferServer::UpdateConnectInterval(std::string device, int32_t intervalType, bool &result)
{
    result = SleInterfaceDataTransfer::GetInstance().UpdateConnectInterval(device, intervalType);
    HILOGI("address: %{public}s, intervalType: %{public}d, result: %{public}d",
        GetEncryptAddr(device).c_str(), intervalType, result);
    return NL_NO_ERROR;
}
#endif

NlErrCode NearlinkSleDataTransferServer::RegisterSleDataTransferCallback(
    const sptr<INearlinkSleDataTransferCallback> &callback)
{
    int32_t pid = IPCSkeleton::GetCallingPid();
    int32_t uid = IPCSkeleton::GetCallingUid();
    uint64_t tokenId = IPCSkeleton::GetCallingFullTokenID();
    HILOGI("DT pid: %{public}d, uid: %{public}d, tokenId: %{public}lu", pid, uid, tokenId);
    NL_CHECK_RETURN_RET(callback, NL_ERR_INVALID_PARAM, "cb is null.");
    pimpl->observers_.Register(callback);
    impl::SleDataTransferRemoteInfo info(pid, uid, tokenId);
    pimpl->remoteContainer_->AddRemoteInfo(callback->AsObject(), info);
    return NL_NO_ERROR;
}

NlErrCode NearlinkSleDataTransferServer::DeregisterSleDataTransferCallback(
    const sptr<INearlinkSleDataTransferCallback> &callback)
{
    HILOGI("enter");
    NL_CHECK_RETURN_RET(callback, NL_ERR_IMPL_ERROR, "cb is null.");
    pimpl->observers_.Deregister(callback);
    pimpl->remoteContainer_->DeleteRemoteInfo(callback->AsObject());
    uint64_t tokenId = IPCSkeleton::GetCallingFullTokenID();
    SleInterfaceDataTransfer::GetInstance().ClearCacheByTokenId(tokenId);
    return NL_NO_ERROR;
}

}  // namespace OHOS::Nearlink