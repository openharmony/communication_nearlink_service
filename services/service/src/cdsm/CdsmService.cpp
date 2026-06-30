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
#include "CdsmService.h"
#include <future>
#include "ClassCreator.h"
#include "sle_uuid.h"
#include "SleInterfaceAdapter.h"
#include "SleInterfaceManager.h"
#include "sysdep.h"
#include "ThreadUtil.h"
#include "nearlink_errorcode.h"
#include "SleConfig.h"
#include "SleInterfaceProfileTws.h"
#include "nearlink_dft_device_data.h"
#include "SleServiceFfrtLog.h"
#include "SleRemoteDeviceAdapter.h"

namespace OHOS {
namespace Nearlink {

void CdsmInfo::CdsmAddMemberInfo(const std::string& devAddr, uint8_t devState)
{
    memberList_.EnsureInsert(devAddr, devState);
    if (!membersProfileState_.FindIf(devAddr)) {
        HILOGI("cdsm add member in membersProfileState_, addr:%{public}s", GetEncryptAddr(devAddr).c_str());
        membersProfileState_.EnsureInsert(devAddr, SleConnectState::DISCONNECTED);
    }
    HILOGI("[Cdsm Service]:insert member,cdsm group id:%{public}u,mem num:%{public}u/%{public}u," \
        "report addr:%{public}s,member addr:%{public}s,state:%{public}u",
        cdsmGrpId_, memberList_.Size(), memberCnt_,
        GetEncryptAddr(reportAddr_).c_str(), GetEncryptAddr(devAddr).c_str(), devState);
}

void CdsmInfo::CdsmDeleteMember(const std::string &devAddr)
{
    HILOGI("cdsm delete device,addr:%{public}s", GetEncryptAddr(devAddr).c_str());
    memberList_.Erase(devAddr);
    membersProfileState_.Erase(devAddr);
}

bool CdsmInfo::CdsmUpdateDevState(const std::string devAddr, uint8_t devState)
{
    HILOGI("cdsm update device state,addr:%{public}s,state:%{public}u",
        GetEncryptAddr(devAddr).c_str(), devState);
    return memberList_.GetValueAndOpt(devAddr,
        [devState](const std::string devAddr, uint8_t &state) -> void {
            state = devState;
        });
}

bool CdsmInfo::CdsmUpdateMemberProfileConnectState(const std::string &addr, SleConnectState profileState)
{
    HILOGD("cdsm update member profile connect state,addr:%{public}s, state:%{public}d",
        GetEncryptAddr(addr).c_str(), static_cast<int>(profileState));
    auto fun = [profileState](const std::string &devAddr, SleConnectState &state) -> void {
        state = profileState;
    };
    return membersProfileState_.GetValueAndOpt(addr, fun);
}

bool CdsmInfo::CdsmIsGroupMember(const std::string &memberAddr)
{
    return memberList_.FindIf(memberAddr);
}

void CdsmInfo::CdsmGetAllMember(std::vector<std::string> &devList)
{
    auto getAll = [&devList](const std::string devAddr, uint8_t &state) -> void {
        devList.push_back(devAddr);
    };
    memberList_.Iterate(getAll);
}

/* 服务启动、退出日志 */
CdsmService::CdsmService() : utility::Context(PROFILE_NAME_CDSM, "1.0.0")
{
    HILOGD("[cdsm service]:service instance create.");
}

CdsmService::~CdsmService()
{
    HILOGD("[cdsm service]:service instance destroy.");
}

int CdsmService::Connect(const RawAddress &device)
{
    DoInCdsmThread([this, device]() { this->ConnectCdsmInterface(device); });
    return NL_NO_ERROR;
}

int CdsmService::Disconnect(const RawAddress &device)
{
    DoInCdsmThread([this, device]() { this->DisconnectCdsmInterface(device); });
    return NL_NO_ERROR;
}

utility::Context *CdsmService::GetContext()
{
    return this;
}

CdsmService *CdsmService::GetService()
{
    return static_cast<CdsmService *>(SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_CDSM));
}

void CdsmService::RegisterObserver(CdsmObserver &serviceObserver)
{
    HILOGI("Enter");
    cdsmObservers_.Register(serviceObserver);
}

void CdsmService::DeregisterObserver(CdsmObserver &serviceObserver)
{
    HILOGD("Enter");
    cdsmObservers_.Deregister(serviceObserver);
}

int CdsmService::GetConnectState()
{
    const std::map<const CdsmClientState, const uint8_t> tmpMap = {
        { CdsmClientState::CDSM_STATE_DISCONNECTED, PROFILE_STATE_DISCONNECTED },
        { CdsmClientState::CDSM_STATE_CONNECTING, PROFILE_STATE_CONNECTING },
        { CdsmClientState::CDSM_STATE_DISCONNECTING, PROFILE_STATE_DISCONNECTING },
        { CdsmClientState::CDSM_STATE_CONNECTED, PROFILE_STATE_CONNECTED },
    };

    uint8_t result = 0;
    cdsmClients_.Iterate([&result, &tmpMap](std::string devAddr, std::shared_ptr<CdsmClient> clientInfo) ->void {
            if (!clientInfo) {
                return;
            }
            CdsmClientState clientState = clientInfo->CdsmClientGetState();
            if (tmpMap.find(clientState) == tmpMap.end()) {
                HILOGE("[Cdsm Service]:client state invalid:%{public}d", clientState);
                return;
            }
            result |= tmpMap.at(clientState);
        });
    return static_cast<int>(result);
}

std::list<RawAddress> CdsmService::GetConnectDevices()
{
    std::list<RawAddress> devices;
    cdsmClients_.Iterate([&devices](std::string devAddr, std::shared_ptr<CdsmClient> clientInfo) -> void {
        if (!clientInfo || clientInfo->CdsmClientGetState() != CdsmClientState::CDSM_STATE_CONNECTED) {
            return;
        }
        RawAddress device(devAddr);
        devices.push_back(device);
    });

    HILOGI("[Cdsm Service]:connect devices size(%{public}zu)", devices.size());
    return devices;
}

/* cdsm服务状态机状态跃迁回调 */
void CdsmService::NotifyStateChanged(const RawAddress &device, CdsmClientState state, CdsmClientState preState)
{
    int newState = stateMap_.at(state);
    int oldState = stateMap_.at(preState);

    cdsmObservers_.ForEach([device, newState, oldState](CdsmObserver &observer) {
        observer.OnConnectionStateChanged(device, newState, oldState);
    });
}

void CdsmService::Enable()
{
    /* 注册合作集Profile回调 */
    NLSTK_CdsmRegisterEventCbk(&CdsmService::CdsmStackProfileCallback);

    CdsmMessage event(CDSM_SERVICE_STARTUP_EVT);
    PostEvent(event);
}

void CdsmService::Disable()
{
    CdsmMessage event(CDSM_SERVICE_SHUTDOWN_EVT);
    PostEvent(event);
}

/*************** 服务状态处理 *****************/
void CdsmService::StartUp()
{
    if (isStarted_.load()) {
        GetContext()->OnEnable(PROFILE_NAME_CDSM, true);
        HILOGW("CdsmService has already been started before.");
        return;
    }

    GetContext()->OnEnable(PROFILE_NAME_CDSM, true);
    isStarted_.store(true);
}

void CdsmService::ShutDown()
{
    if (!isStarted_.load()) {
        GetContext()->OnDisable(PROFILE_NAME_CDSM, true);
        HILOGW("[Cdsm Service]:already been shutdown.");
        return;
    }

    isShuttingDown_.store(true);

    GetContext()->OnDisable(PROFILE_NAME_CDSM, true);
    isStarted_.store(false);
    isShuttingDown_.store(false);
    HILOGI("CdsmService shutdown");
}

/* 单设备合作集服务连接处理,run in cdsm thread */
void CdsmService::ProcessConnectEvent(const CdsmMessage &event)
{
    bool isClientExisted = cdsmClients_.GetValueAndOpt(event.dev_,
        [](std::string devAddr, std::shared_ptr<CdsmClient> clientInfo) -> void {
            bool ret = clientInfo->CdsmClientStartConnect();
            if (!ret) {
                HILOGE("CdsmClientStartConnect failed, ret=%{public}d", ret);
                return;
            }
            clientInfo->CdsmClientUpdateState(CdsmClientState::CDSM_STATE_CONNECTING);
        });
    if (!isClientExisted) {
        std::shared_ptr<CdsmClient> newClient = CdsmClient::CreateCdsmClient(event.dev_);
        if (newClient == nullptr) {
            HILOGE("[cdsm service]:create new cdsm client failed.");
            return;
        }
        newClient->CdsmClientStartConnect();
        newClient->CdsmClientUpdateState(CdsmClientState::CDSM_STATE_CONNECTING);
        cdsmClients_.EnsureInsert(event.dev_, newClient);
    }
}

/* 单设备合作集服务连接处理,run in cdsm thread */
void CdsmService::ProcessDisconnectEvent(const CdsmMessage &event)
{
    bool isClientExisted = cdsmClients_.GetValueAndOpt(event.dev_,
        [](std::string devAddr, std::shared_ptr<CdsmClient> clientInfo) -> void {
            bool ret = clientInfo->CdsmClientStartDisconnect();
            if (!ret) {
                HILOGE("CdsmClientStartDisConnect failed, ret=%{public}d", ret);
                return;
            }
        });
}

/* 单设备合作集服务连接完成,run in cdsm thread */
void CdsmService::ProcessDefaultEvent(const CdsmMessage &event, CdsmClientState toState)
{
    if (toState < CdsmClientState::CDSM_STATE_DISCONNECTED || toState > CdsmClientState::CDSM_STATE_CONNECTED) {
        HILOGE("[cdsm service]:client=%{public}s,state invalid:%{public}d.",
            GetEncryptAddr(event.dev_).c_str(), toState);
        return;
    }

    auto clientProc = [toState](std::string addr, std::shared_ptr<CdsmClient> clientInstance) -> void {
        clientInstance->CdsmClientUpdateState(toState);
    };

    bool isClientExisted = cdsmClients_.GetValueAndOpt(event.dev_, clientProc);
    if (!isClientExisted) {
        HILOGE("[cdsm service]:client=%{public}s node not existed,update connect state fail,newState:%{public}d.",
            GetEncryptAddr(event.dev_).c_str(), toState);
        return;
    }
    HILOGD("[cdsm service]:update client state to:%{public}d", toState);
}

/************ 服务状态处理入口 ***************/
/* switch to cdsm thread */
void CdsmService::PostEvent(const CdsmMessage &event)
{
    DoInCdsmThread([this, event]() { this->ProcessEvent(event); });
}

/* 合作集客户端消息处理,run in cdsm thread */
void CdsmService::ProcessEvent(const CdsmMessage &event)
{
    switch (event.whatM) {
        case CDSM_SERVICE_STARTUP_EVT:
            StartUp();
            break;
        case CDSM_SERVICE_SHUTDOWN_EVT:
            ShutDown();
            break;
        case CDSM_SERVICE_CONNECT_START_EVT:
            ProcessConnectEvent(event);
            break;
        case CDSM_SERVICE_DISCONNECT_START_EVT:
            ProcessDisconnectEvent(event);
            break;
        case CDSM_SERVICE_CONNECT_CMPL_EVT:
            ProcessDefaultEvent(event, CdsmClientState::CDSM_STATE_CONNECTED);
            break;
        case CDSM_SERVICE_DISCONNECT_CMPL_EVT:
            ProcessDefaultEvent(event, CdsmClientState::CDSM_STATE_DISCONNECTED);
            break;
        default:
            HILOGE("[cdsm service]:event message type not support:%{public}d,addr:%{public}s",
                event.whatM, GetEncryptAddr(event.dev_).c_str());
            break;
    }
}

/* 是否合作集设备（true:是合作集report addr或成员地址，任意合作集匹配） */
bool CdsmService::CdsmCheckIsCooperationDevice(const RawAddress &devAddr)
{
    bool isCooperationAddr = false;
    std::string devAddrStr = devAddr.GetAddress();

    cdsmList_.Find(
        [devAddrStr, &isCooperationAddr](const uint32_t cdsmGrpId, std::shared_ptr<CdsmInfo> cdsmData) -> bool {
            if (cdsmData->CdsmIsGroupMember(devAddrStr)) {
                isCooperationAddr = true;
                return true;
            }
            return false;
        });

    return isCooperationAddr;
}

/* 是否私有合作集设备（true:是任意私有合作集匹配） */
bool CdsmService::CdsmCheckIsPrivateCooperationDevice(const RawAddress &devAddr)
{
    bool isPrivateCooperationAddr = false;
    std::string devAddrStr = devAddr.GetAddress();

    cdsmList_.Find(
        [devAddrStr, &isPrivateCooperationAddr](const uint32_t cdsmGrpId, std::shared_ptr<CdsmInfo> cdsmData) -> bool {
            if (cdsmData->CdsmIsGroupMember(devAddrStr) && cdsmData->isPrivateTws_) {
                isPrivateCooperationAddr = true;
                return true;
            }
            return false;
        });

    return isPrivateCooperationAddr;
}

/* 是否是成员地址（true:除report addr外的成员地址，任意合作集匹配） */
bool CdsmService::CdsmCheckIsCooperationMember(const RawAddress &devAddr)
{
    bool isMemberAddr = false;
    std::string devAddrStr = devAddr.GetAddress();
    cdsmList_.Find(
        [devAddrStr, &isMemberAddr](const uint32_t cdsmGrpId, std::shared_ptr<CdsmInfo> cdsmData) -> bool {
            if (cdsmData->CdsmIsGroupMember(devAddrStr) &&
                (cdsmData->reportAddr_ != devAddrStr)) {
                isMemberAddr = true;
                return true;
            }
            return false;
        });

    return isMemberAddr;
}

bool CdsmService::CdsmCheckIsCooperationReport(const RawAddress &devAddr)
{
    bool isReportAddr = false;
    std::string devAddrStr = devAddr.GetAddress();
    cdsmList_.Find(
        [devAddrStr, &isReportAddr](const uint32_t cdsmGrpId, std::shared_ptr<CdsmInfo> cdsmData) -> bool {
            if (cdsmData->reportAddr_ == devAddrStr) {
                isReportAddr = true;
                return true;
            }
            return false;
        });

    return isReportAddr;
}

/* 获取合作集report地址 */
bool CdsmService::CdsmGetReportAddr(const RawAddress &memberAddr, RawAddress &reportAddr)
{
    bool isGetReportAddr = false;
    cdsmList_.Find([&memberAddr, &reportAddr, &isGetReportAddr](const uint32_t cdsmGrpId,
        std::shared_ptr<CdsmInfo> cdsmData) -> bool {
            if (cdsmData->CdsmIsGroupMember(memberAddr.GetAddress())) {
                isGetReportAddr = true;
                reportAddr.SetAddress(cdsmData->reportAddr_);
                return true;
            }
            return false;
        });

    return isGetReportAddr;
}

/* 判断给定地址是否一个合作 */
bool CdsmService::CdsmCheckIsSameCooperation(const std::vector<RawAddress> &devAddrList)
{
    if (devAddrList.empty()) {
        HILOGE("[cdsm service]:check same cooperation fail,param invalid.");
        return false;
    }

    RawAddress baseDev = devAddrList.at(0); /* 0:取第一个设备地址 */
    uint32_t cdsmGrpId = CDSM_SERVICE_INVALID_GROUP_ID;
    if (!CdsmGetGroupId(cdsmGrpId, baseDev.GetAddress())) {
        HILOGE("[cdsm service]:not cdsm device,addr:%{public}s", GetEncryptAddr(baseDev.GetAddress()).c_str());
        return false;
    }

    std::vector<std::string> devAddrListSrc;
    std::vector<std::string> devAddrListLocal;
    for (const auto& devSrc : devAddrList) {
        devAddrListSrc.push_back(devSrc.GetAddress());
    }

    cdsmList_.GetValueAndOpt(cdsmGrpId,
        [&devAddrListLocal](const uint32_t cdsmGrpId, std::shared_ptr<CdsmInfo> cdsmData) -> void {
            cdsmData->memberList_.Iterate([&devAddrListLocal](const std::string devAddr, uint8_t &state) -> void {
                devAddrListLocal.push_back(devAddr);
            });
    });

    std::sort(devAddrListSrc.begin(), devAddrListSrc.end());
    std::sort(devAddrListLocal.begin(), devAddrListLocal.end());

    return (devAddrListSrc == devAddrListLocal);
}

void CdsmService::HandleCdsmClientCallback(const RawAddress &device, std::optional<uint64_t> tokenId)
{
    DoInCdsmThread([this, device, tokenId] { HandleCdsmClientCallbackTask(device, tokenId); });
}

/* 调用应用注册回调，run in cdsm thread */
void CdsmService::HandleCdsmClientCallbackTask(const RawAddress &device, std::optional<uint64_t> tokenId)
{
    uint32_t groupId = CDSM_SERVICE_INVALID_GROUP_ID;
    if (!CdsmGetGroupId(groupId, device.GetAddress())) {
        HILOGE("[cdsm service]get cdsm group id by addr failed,addr=%{public}s.",
            GetEncryptAddr(device.GetAddress()).c_str());
        return;
    }
    HILOGD("[Cdsm Service]HandleCdsmClientCallbackTask: device=%{public}s, groupid=%{public}d",
            GetEncryptAddr(device.GetAddress()).c_str(), groupId);
    RawAddress reportAddr;
    std::vector<NearlinkCdsmInfo> members;
    cdsmList_.GetValueAndOpt(groupId, [&reportAddr, &members](uint32_t, const std::shared_ptr<CdsmInfo> &cdsmData) {
        reportAddr.SetAddress(cdsmData->reportAddr_);
        auto fun = [&members, reportAddr](const std::string &addr, SleConnectState state) {
            NearlinkCdsmInfo tmpDevInfo;
            tmpDevInfo.addr_ = RawAddress(addr);
            if (state == SleConnectState::CONNECTED) {
                tmpDevInfo.state_ = static_cast<int>(CdsmConnectState::CONNECTED);
            } else {
                tmpDevInfo.state_ = static_cast<int>(CdsmConnectState::DISCONNECTED);
            }
            if (addr == reportAddr.GetAddress()) {
                tmpDevInfo.isReportAddr_ = true;
            }
            members.push_back(tmpDevInfo);
            HILOGD("[Cdsm Service]add member: device=%{public}s, state_=%{public}d",
                GetEncryptAddr(tmpDevInfo.addr_.GetAddress()).c_str(), tmpDevInfo.state_);
        };
        cdsmData->membersProfileState_.Iterate(fun);
    });
    // 触发回调
    if (cdsmCbk_ == nullptr) {
        HILOGI("[Cdsm Service]Current callback is null");
        return;
    }
    cdsmCbk_->OnCdsmClientStateChange(reportAddr, members, tokenId);
    HILOGD("[Cdsm Service]HandleCdsmClientCallbackTask End");
}

bool CdsmService::ClearOldDevice(CdsmCallBackData &dataBlock, std::shared_ptr<CdsmInfo> cdsmData,
    RawAddress &oldDevAddr)
{
    bool isNeedClear = false;
    /* 检查旧设备，场景：外设重新耦合新的合作集成员地址 */
    auto checkOld = [&dataBlock, &oldDevAddr, &isNeedClear](const std::string &devAddr, uint8_t &state) -> bool {
        bool isExist = false;
        for (auto &member : dataBlock.memberInfo) {
            if (member.memberAddr.GetAddress() == devAddr) {
                isExist = true;
            }
        }

        /* 找到首个旧设备（预期仅一个设备地址变化） */
        if (!isExist) {
            isNeedClear = true;
            oldDevAddr.SetAddress(devAddr);
            HILOGI("[cdsm service]:clear old cdsm device,group id:%{public}u,addr:%{public}s",
                dataBlock.cdmsGroupId, GET_ENCRYPT_ADDR(oldDevAddr));
            return true;
        }
        return false;
    };
    cdsmData->memberList_.Find(checkOld);

    /* 删除旧设备 */
    if (isNeedClear) {
        HILOGI("[cdsm service]:clear old cdsm device,group id:%{public}u,addr:%{public}s",
            dataBlock.cdmsGroupId, GET_ENCRYPT_ADDR(oldDevAddr));
        cdsmData->memberList_.Erase(oldDevAddr.GetAddress());
    }

    return isNeedClear;
}

void CdsmService::UpdateCdsmData(CdsmCallBackData &dataBlock)
{
    HILOGI("[Cdsm Service]device: %{public}s", GetEncryptAddr(dataBlock.currentAddr.GetAddress()).c_str());
    bool isNeedClear = false;
    RawAddress oldDevAddr;
    RawAddress reportAddr;
    auto updataCdsm = [&dataBlock, this, &isNeedClear, &oldDevAddr, &reportAddr](
        uint32_t grpId, std::shared_ptr<CdsmInfo> cdsmData) -> void {
        /* 清理旧设备 */
        isNeedClear = ClearOldDevice(dataBlock, cdsmData, oldDevAddr);

        /* 更新合作集地址列表 */
        cdsmData->cdsmGrpId_ = dataBlock.cdmsGroupId;
        cdsmData->memberCnt_ = dataBlock.memberNum;
        for (auto &memInfo : dataBlock.memberInfo) {
            cdsmData->CdsmAddMemberInfo(memInfo.memberAddr.GetAddress(), memInfo.memberState);
        }
        reportAddr = RawAddress(cdsmData->reportAddr_);
    };
    cdsmList_.GetValueAndOpt(dataBlock.cdmsGroupId, updataCdsm);

    for (auto &memInfo : dataBlock.memberInfo) {
        // report地址,已经在建联的时候设置过了,这里只处理非report地址
        if (memInfo.memberAddr != reportAddr) {
            SetPeerDeviceTypeToControllerInner(memInfo.memberAddr);
        }
    }

    /* 调用Adapter接口，避免死锁cdsmList_ */
    if (isNeedClear) {
        SleInterfaceAdapter *sleAdapter = SleInterfaceManager::GetInstance()->GetAdapter(ADAPTER_SLE);
        NL_CHECK_RETURN(sleAdapter, "[cdsm Service] sle adapter Interface nullptr.");
        sleAdapter->CancelPairing(oldDevAddr); /* 合作集列表更新，取消配对只删除单个设备 */
    }
}

void CdsmService::CdsmServiceCreateNewCdsm(CdsmCallBackData &dataBlock)
{
    if (cdsmList_.FindIf(dataBlock.cdmsGroupId)) {
        UpdateCdsmData(dataBlock);
        HILOGD("[cdsm service]:Update existing cdsm group(id=%{public}u)", dataBlock.cdmsGroupId);
        return;
    }

    /* 新增合作集 */
    HILOGI("[cdsm service]:alloc new cdsm group(id=%{public}u)", dataBlock.cdmsGroupId);
    RawAddress reportAddr = dataBlock.currentAddr;
    CreateNewCdsmGroupInfoInner(reportAddr, dataBlock.cdmsGroupId, dataBlock.memberInfo,
        IsVendorDevice(dataBlock.currentAddr));
    // 等上面的成员信息都更新完成后，再触发，否则会查不到
    for (auto memInfo : dataBlock.memberInfo) {
        if (memInfo.memberAddr != reportAddr) { // report地址，已经在建联的时候设置过了；这里只处理非report地址
            SetPeerDeviceTypeToControllerInner(memInfo.memberAddr);
        }
    }
}

bool CdsmService::IsVendorDevice(const RawAddress &device)
{
    // 合作集可能还没创建完成，所以这里从广播中取
    int businessType = SleRemoteDeviceAdapter::GetInstance()->GetManufacturerBusinessType(device);
    return businessType == SLE_PRIVATE_AUDIO_BUSINESS_TYPE;
}

void CdsmService::SetPeerDeviceTypeToControllerInner(const RawAddress &device)
{
    NL_CHECK_RETURN(!IsVendorDevice(device), "[cdsm Service]private device has set device type to control.");
    HILOGI("[Cdsm Service]do set common peer device type to controller %{public}s",
        GetEncryptAddr(device.GetAddress()).c_str());
    SleRemoteDeviceAdapter::GetInstance()->SetPeerDeviceTypeToController(device);
}

void CdsmService::CdsmServiceCallBackDataProc(CdsmCallBackData dataBlock)
{
    CdsmMessage event;
    event.dev_ = dataBlock.currentAddr.GetAddress();
    event.cdsmGrpId_ = dataBlock.cdmsGroupId;

    HILOGD("[Cdsm Service]:Stack callback param," \
        "event:%{public}s,cdsm Group ID:%{public}u,addr:%{public}s,member number:%{public}u",
        dataBlock.event == CDSM_EVENT_CONNECT ? "Connect" : "Disconnect",
        dataBlock.cdmsGroupId, GetEncryptAddr(event.dev_).c_str(), dataBlock.memberNum);

    switch (dataBlock.event) {
        case CDSM_EVENT_CONNECT: { /* 合作集连接完成 */
            event.whatM = CDSM_SERVICE_CONNECT_CMPL_EVT;
            CdsmServiceCreateNewCdsm(dataBlock);
            break;
        }
        case CDSM_EVENT_DISCONNECT: { /* 合作集断连完成 */
            event.whatM = CDSM_SERVICE_DISCONNECT_CMPL_EVT ;
            auto updateState = [&event](const uint32_t cdsmGrpId, std::shared_ptr<CdsmInfo> cdsmData) {
                cdsmData->CdsmUpdateDevState(event.dev_, static_cast<uint8_t>(CdsmConnectState::DISCONNECTED));
            };
            cdsmList_.GetValueAndOpt(event.cdsmGrpId_, updateState);
            break;
        }
        default:
            HILOGW("Cdsm stack cbk recv unsupport event(%{public}d)", dataBlock.event);
            return;
    }

    PostEvent(event);
}

/* 协议栈回调接口 */
void CdsmService::CdsmStackProfileCallback(NLSTK_CdsmEvent_S *eventMsg)
{
    if (eventMsg == nullptr || eventMsg->gid == CDSM_SERVICE_INVALID_GROUP_ID) {
        HILOGE("Cdsm stack callback input param invalid.");
        return;
    }

    /* 组织数据&切换线程处理 */
    CdsmCallBackData dataBlock;
    dataBlock.event = eventMsg->type;
    dataBlock.cdmsGroupId = eventMsg->gid;
    dataBlock.memberNum = eventMsg->num;
    dataBlock.currentAddr = RawAddress::ConvertToString(eventMsg->addr.addr);
    for (uint8_t idx = 0; idx < eventMsg->num; idx++) {
        NLSTK_CdsmMemInfo_S *memberData = &eventMsg->memInfo[idx];
        CdsmCallBackDataMemInfo memInfo;
        memInfo.memberAddr = RawAddress::ConvertToString(memberData->addr.addr);
        memInfo.memberState = memberData->state;
        dataBlock.memberInfo.push_back(memInfo);
    }

    DoInCdsmThread([dataBlock]() {
            CdsmService *cdsmInstance = CdsmService::GetService();
            if (cdsmInstance != nullptr) {
                cdsmInstance->CdsmServiceCallBackDataProc(dataBlock);
            }
        });
}

void CdsmService::ConnectCdsmInterface(const RawAddress &device)
{
    uint32_t cdsmGrpId = CdsmGenerateGroupIdInner(device);
    if (cdsmGrpId == CDSM_SERVICE_INVALID_GROUP_ID) {
        return;
    }

    HILOGI("[Cdsm Service]:create cdsm group by addr:%{public}s,group id:%{public}d",
        GetEncryptAddr(device.GetAddress()).c_str(), cdsmGrpId);

    if (isShuttingDown_.load()) {
        HILOGI("[Cdsm Service]:group id=%{public}d is shutting down", cdsmGrpId);
        return;
    }

    CdsmMessage event(CDSM_SERVICE_CONNECT_START_EVT);
    event.cdsmGrpId_ = cdsmGrpId;
    event.dev_ = device.GetAddress();
    PostEvent(event);
}

/* ProfileManager断连 run in cdsm thread */
void CdsmService::DisconnectCdsmInterface(const RawAddress &device)
{
    HILOGI("cdsm service,dev:%{public}s disconnect enter.",
        GetEncryptAddr(device.GetAddress()).c_str());

    uint32_t cdsmGrpId = CDSM_SERVICE_INVALID_GROUP_ID;
    if (!CdsmGetGroupId(cdsmGrpId, device.GetAddress())) {
        HILOGD("[Cdsm Service]:get member info failed,addr invalid:%{public}s.",
            GetEncryptAddr(device.GetAddress()).c_str());
        return;
    }

    if (isShuttingDown_.load()) {
        HILOGI("[Cdsm Service]:group id=%{public}d is shutting down", cdsmGrpId);
        return;
    }

    CdsmMessage event(CDSM_SERVICE_DISCONNECT_START_EVT);
    event.cdsmGrpId_ = cdsmGrpId;
    event.dev_ = device.GetAddress();
    PostEvent(event);
}

/*****************************************************
 *                   合作集数据操作                   *
 *****************************************************/
void CdsmService::CdsmDeleteMember(uint32_t memGrpId, std::string memberAddr)
{
    bool ret = cdsmList_.GetValueAndOpt(memGrpId,
        [memberAddr](const uint32_t cdsmGrpId, std::shared_ptr<CdsmInfo> cdsmData) {
            cdsmData->CdsmDeleteMember(memberAddr);
    });
    if (!ret) {
        HILOGE("cdsm service,delete member by addr(%{public}s) failed.", GetEncryptAddr(memberAddr).c_str());
    }
}

bool CdsmService::CdsmGetGroupId(uint32_t &groupId, std::string memberAddr)
{
    groupId = CDSM_SERVICE_INVALID_GROUP_ID;
    cdsmList_.Iterate([&groupId, &memberAddr](const uint32_t cdsmGrpId, std::shared_ptr<CdsmInfo> cdsmData) -> void {
        if (cdsmData->CdsmIsGroupMember(memberAddr)) {
            if (groupId != CDSM_SERVICE_INVALID_GROUP_ID) {
                HILOGW("cdsm member addr:%{public}s in multi group,id:%{public}u,report:%{public}s!!",
                    GetEncryptAddr(memberAddr).c_str(), groupId, GetEncryptAddr(cdsmData->reportAddr_).c_str());
                return;
            }
            groupId = cdsmGrpId;
        }
    });

    if (groupId == CDSM_SERVICE_INVALID_GROUP_ID) {
        return false;
    }

    return true;
}

void CdsmService::UpdateCdsmMemberProfileConnectState(const RawAddress &device, SleConnectState profileState)
{
    DoInCdsmThread([this, device, profileState] {
        uint32_t groupId = CDSM_SERVICE_INVALID_GROUP_ID;
        if (!CdsmGetGroupId(groupId, device.GetAddress())) {
            HILOGE("[Cdsm Service]update member profile state, get cdsm group id by addr failed, addr=%{public}s.",
                GetEncryptAddr(device.GetAddress()).c_str());
            return;
        }
        HILOGI("[Cdsm Service]update member profile state, device=%{public}s, groupid=%{public}d, state=%{public}d.",
            GetEncryptAddr(device.GetAddress()).c_str(), groupId, static_cast<int>(profileState));
        auto fun = [device, profileState](uint32_t, const std::shared_ptr<CdsmInfo> &cdsmData) -> void {
            if (!cdsmData->CdsmUpdateMemberProfileConnectState(device.GetAddress(), profileState)) {
                HILOGE("[Cdsm Service]update member profile state error, device=%{public}s, state=%{public}d",
                    GetEncryptAddr(device.GetAddress()).c_str(), static_cast<int>(profileState));
            }
        };
        if (!cdsmList_.GetValueAndOpt(groupId, fun)) {
            HILOGE("[Cdsm Service]update member profile state, not find groupid=%{public}d.", groupId);
            return;
        }
        if (profileState == SleConnectState::CONNECTED || profileState == SleConnectState::DISCONNECTED) {
            // 状态更新，触发回调
           HandleCdsmClientCallbackTask(device);
        }
    });
}

void CdsmService::CdsmGetReportedState(const RawAddress &devAddr, int &reportConnectState)
{
    reportConnectState = static_cast<int>(SleConnectState::INVALID_STATE);
    std::string devAddrStr = devAddr.GetAddress();

    cdsmList_.Iterate([&reportConnectState, &devAddrStr](const uint32_t cdsmGrpId,
        std::shared_ptr<CdsmInfo> cdsmData) -> void {
        if (cdsmData == nullptr) {
            HILOGE("[Cdsm Service]:cdsm data empty.");
            return;
        }
        if (cdsmData->CdsmIsGroupMember(devAddrStr)) {
            reportConnectState = cdsmData->reportConnectState_;
            return;
        }
    });
}

void CdsmService::UpdateReportConnectState(const RawAddress &device, int profileState)
{
    DoInCdsmThread([this, device, profileState] {
        uint32_t groupId = CDSM_SERVICE_INVALID_GROUP_ID;
        if (!CdsmGetGroupId(groupId, device.GetAddress())) {
            HILOGE("[Cdsm Service]get cdsm group id by addr failed, addr=%{public}s.",
                GetEncryptAddr(device.GetAddress()).c_str());
            return;
        }
        auto fun = [device, profileState](uint32_t, const std::shared_ptr<CdsmInfo> &cdsmData) -> void {
            if (cdsmData == nullptr) {
                HILOGE("[Cdsm Service]:cdsm data empty.");
                return;
            }
            cdsmData->reportConnectState_ = profileState;
        };
        if (!cdsmList_.GetValueAndOpt(groupId, fun)) {
            HILOGE("[Cdsm Service]update report profile state, not find groupid=%{public}d.", groupId);
            return;
        }
    });
}

/* 应用回调注册, switch to cdsm thread */
NlErrCode CdsmService::RegisterCdsmClientCallback(const std::shared_ptr<InterfaceCdsmClientServiceCallback> &cbk)
{
    HILOGI("[Cdsm Service]Enter");
    NL_CHECK_RETURN_RET(cbk, NL_ERR_INVALID_PARAM, "Invalid params: callback must not be null.");
    DoInCdsmThread([this, cbk] {
        cdsmCbk_ = cbk;
    });
    return NL_NO_ERROR;
}

/* 应用回调去注册, switch to cdsm thread */
NlErrCode CdsmService::DeregisterCdsmClientCallback()
{
    HILOGI("[Cdsm Service]Enter");
    DoInCdsmThread([this] {
        cdsmCbk_ = nullptr;
    });
    return NL_NO_ERROR;
}

/* 获取已连接设备列表  switch to cdsm thread */
NlErrCode CdsmService::CdsmGetAllMemberInfo(const RawAddress &memberAddr, std::vector<NearlinkCdsmInfo> &cdsmInfo)
{
    NlErrCode retCode = NL_ERR_INTERNAL_ERROR;
    uint32_t cdsmGrpId = CDSM_SERVICE_INVALID_GROUP_ID;
    if (!CdsmGetGroupId(cdsmGrpId, memberAddr.GetAddress())) {
        HILOGD("[Cdsm Service]:get member info failed,addr invalid:%{public}s.",
            GetEncryptAddr(memberAddr.GetAddress()).c_str());
        return retCode;
    }

    bool isDataNormal = cdsmList_.GetValueAndOpt(cdsmGrpId,
        [&cdsmInfo, &retCode](uint32_t grpId, std::shared_ptr<CdsmInfo> cdsmData) -> void {
            if (cdsmData == nullptr) {
                HILOGE("[Cdsm Service]:cdsm data empty.");
                return;
            }

            std::string reportAddr = cdsmData->reportAddr_;
            cdsmData->memberList_.Iterate(
                [&cdsmInfo, reportAddr](const std::string devAddr, uint8_t &state) -> void {
                NearlinkCdsmInfo tmpDevInfo;
                tmpDevInfo.addr_ = RawAddress(devAddr);
                tmpDevInfo.state_ = state;
                if (devAddr == reportAddr) {
                    tmpDevInfo.isReportAddr_ = true;
                }

                cdsmInfo.push_back(tmpDevInfo);
            });
            retCode = NL_NO_ERROR;
    });
    if (!isDataNormal || retCode != NL_NO_ERROR) {
        HILOGE("[Cdsm Service]:get member info failed,cdsm not exist,group ID:%{public}u.", cdsmGrpId);
        return retCode;
    }

    return retCode;
}

void CdsmService::CdsmDeleteGroupInner(const RawAddress &devAddr)
{
    HILOGI("[Cdsm Service]device: %{public}s", GetEncryptAddr(devAddr.GetAddress()).c_str());
    RawAddress reportAddr(devAddr);
    CdsmGetReportAddr(devAddr, reportAddr);
    SleConfig::GetInstance().RemoveCdsmGroup(reportAddr.GetAddress());
    SleConfig::GetInstance().Save();

    uint32_t cdsmGrpId = CDSM_SERVICE_INVALID_GROUP_ID;
    bool isExisted = CdsmGetGroupId(cdsmGrpId, devAddr.GetAddress());
    if (!isExisted) {
        HILOGW("[Cdsm service]:delete cdsm,addr:%{public}s not in cdsm!",
            GetEncryptAddr(devAddr.GetAddress()).c_str());
        return;
    }

    /* 清理客户端的实例 */
    auto delAllClient = [this](uint32_t grpId, std::shared_ptr<CdsmInfo> cdsmData) -> void {
        auto delSingleClient = [this](const std::string clientAddr, uint8_t &state) -> void {
            cdsmClients_.Erase(clientAddr);
        };
        cdsmData->memberList_.Iterate(delSingleClient);
    };
    cdsmList_.GetValueAndOpt(cdsmGrpId, delAllClient);

    cdsmList_.Erase(cdsmGrpId);

    /* 清除协议栈数据 */
    NLSTK_CdsmRemoveSet(cdsmGrpId);

    HILOGI("[cdsm service]:remove group id:%{public}u,member addr:%{public}s",
        cdsmGrpId, GetEncryptAddr(devAddr.GetAddress()).c_str());
}

/* 删除合作集 */
void CdsmService::CdsmDeleteGroup(const RawAddress &devAddr)
{
    HILOGI("[Cdsm Service]device: %{public}s", GetEncryptAddr(devAddr.GetAddress()).c_str());
    std::promise<void> promise;
    DoInCdsmThread([this, &promise, devAddr]() {
        CdsmDeleteGroupInner(devAddr);
        promise.set_value();
    });
    return promise.get_future().get();
}

/* 停止邀请广播（正常场景）
 * @param devAddr 合作集中任意设备地址
 * @param isForceStop，是否强制停
 *        true:任意设备profile连接异常时停止邀请广播
 *        false:正常连接上后，主动停止邀请广播
 */
void CdsmService::CdsmStopInviteAdv(const RawAddress &devAddr, bool isForceStop)
{
    DoInCdsmThread([this, devAddr, isForceStop]() {
        /* 如果不是强制停止邀请广播，需要在合作集ACB连接完成后停 */
        if (!isForceStop && !CdsmCheckIsCooperationMember(devAddr)) {
            HILOGW("[cdsm service]:stop cdsm invite adv abort,not member addr:%{public}s",
                GetEncryptAddr(devAddr.GetAddress()).c_str());
            return;
        }

        RawAddress reportAddr;
        if (!CdsmGetReportAddr(devAddr, reportAddr)) {
            HILOGE("[cdsm service]:stop cdsm invite adv fail,get report addr fail,member:%{public}s",
                GetEncryptAddr(devAddr.GetAddress()).c_str());
            return;
        }

        /* 停止合作集的邀请广播 */
        auto stopInviteAdv = [](std::string addr, std::shared_ptr<CdsmClient> clientInstance) -> void {
            clientInstance->CdsmStopInviteAdv();
        };
        if (!cdsmClients_.GetValueAndOpt(reportAddr.GetAddress(), stopInviteAdv)) {
            HILOGW("[cdsm service]:stop invite adv abort,not cdsm:%{public}s",
                GetEncryptAddr(reportAddr.GetAddress()).c_str());
        }
    });
}

std::shared_ptr<CdsmInfo> CdsmService::CreateNewCdsmGroupInfoInner(RawAddress reportAddr, uint32_t cdsmGrpId,
    std::vector<CdsmCallBackDataMemInfo> members, bool isPrivateTws)
{
    HILOGI("[Cdsm Service]device: %{public}s, isPrivateTws: %{public}d",
        GetEncryptAddr(reportAddr.GetAddress()).c_str(), isPrivateTws);
    std::shared_ptr<CdsmInfo> cdsmDataPtr = std::make_shared<CdsmInfo>(cdsmGrpId, reportAddr.GetAddress());
    cdsmDataPtr->memberCnt_ = members.size();
    cdsmDataPtr->isPrivateTws_ = isPrivateTws;
    for (auto memInfo : members) {
        cdsmDataPtr->CdsmAddMemberInfo(memInfo.memberAddr.GetAddress(), memInfo.memberState);
        /* Dft存储reportAddr */
        DftDeviceManager::GetInstance().EnsureUpdateReportAddr(memInfo.memberAddr, reportAddr);
    }
    cdsmList_.EnsureInsert(cdsmGrpId, cdsmDataPtr);
    return cdsmDataPtr;
}

uint32_t CdsmService::CdsmCreateGroupInner(const RawAddress& reportAddr, const std::vector<RawAddress>& devAddrList,
    bool isPrivateTws)
{
    HILOGI("[Cdsm Service]device: %{public}s", GetEncryptAddr(reportAddr.GetAddress()).c_str());
    uint32_t cdsmGrpId = CDSM_SERVICE_INVALID_GROUP_ID;
    if (CdsmGetGroupId(cdsmGrpId, reportAddr.GetAddress())) {
        if (CdsmCheckIsSameCooperation(devAddrList)) {
            // 相同的合作集已经存在，不需要重复创建
            return cdsmGrpId;
        }
        // 先清除老的合作集，后创建新的合作集
        CdsmDeleteGroupInner(reportAddr);
    }

    cdsmGrpId = CdsmGenerateGroupIdInner(reportAddr);
    if (cdsmGrpId == CDSM_SERVICE_INVALID_GROUP_ID) {
        HILOGE("[Cdsm Service]group id is invalid");
        return cdsmGrpId;
    }
    std::vector<CdsmCallBackDataMemInfo> members;
    for (auto memberAddr : devAddrList) {
        CdsmCallBackDataMemInfo memInfo;
        memInfo.memberAddr = memberAddr;
        memInfo.memberState = CDSM_STACK_CLIENT_DISCONNECTED;
        members.push_back(memInfo);
    }
    std::shared_ptr<CdsmInfo> cdsmDataPtr =
        CreateNewCdsmGroupInfoInner(reportAddr, cdsmGrpId, members, isPrivateTws);
    if (cdsmDataPtr == nullptr) {
        HILOGE("[Cdsm Service]alloc new cdsm data failed");
        return cdsmGrpId;
    }
    std::vector<std::string> memberList;
    cdsmDataPtr->CdsmGetAllMember(memberList);
    CdsmRecoverStackData(cdsmGrpId, memberList);
    HILOGI("[cdsm service]report(%{public}s) create cdsm group(%{public}d) ok.",
        GetEncryptAddr(reportAddr.GetAddress()).c_str(), cdsmGrpId);
    return cdsmGrpId;
}

/* 创建合作集 */
uint32_t CdsmService::CdsmCreateGroup(const RawAddress &reportAddr, const std::vector<RawAddress> &devAddrList,
    bool isPrivateTws)
{
    HILOGI("[Cdsm Service]device: %{public}s", GetEncryptAddr(reportAddr.GetAddress()).c_str());
    std::promise<int> promise;
    DoInCdsmThread([this, &promise, reportAddr, devAddrList, isPrivateTws]() {
        uint32_t cdsmGrpId = CdsmCreateGroupInner(reportAddr, devAddrList, isPrivateTws);
        promise.set_value(cdsmGrpId);
    });
    return promise.get_future().get();
}

uint32_t CdsmService::CdsmGenerateGroupIdInner(const RawAddress &reportAddr)
{
    HILOGI("[Cdsm Service]device: %{public}s", GetEncryptAddr(reportAddr.GetAddress()).c_str());
    SLE_Addr_S peerAddr;
    reportAddr.ConvertToUint8(peerAddr.addr, SLE_ADDR_LEN);
    uint32_t groupId = NLSTK_CdsmCreateSet(&peerAddr);
    if (groupId == CDSM_SERVICE_INVALID_GROUP_ID) {
        HILOGE("[Cdsm Service]:create cdsm group failed,addr:%{public}s",
            GetEncryptAddr(reportAddr.GetAddress()).c_str());
        return groupId;
    }
    HILOGI("[Cdsm Service]create device: %{public}s group id is %{public}d",
        GetEncryptAddr(reportAddr.GetAddress()).c_str(), groupId);
    return groupId;
}

/* robin reportAddr发生变化场景，更新已有合作集的reportAddr */
bool CdsmService::CdsmReplaceOldReportAddr(const RawAddress &oldReportAddr, const RawAddress &newReportAddr)
{
    bool isReplace = false;
    cdsmList_.Find([&oldReportAddr, &newReportAddr, &isReplace](const uint32_t cdsmGrpId,
        std::shared_ptr<CdsmInfo> cdsmData) -> bool {
            if (cdsmData->CdsmIsGroupMember(newReportAddr.GetAddress()) &&
                oldReportAddr.GetAddress() == cdsmData->reportAddr_ &&
                newReportAddr.GetAddress() != cdsmData->reportAddr_) {
                HILOGI("[cdsm service]: cdsm update reportAddr %{public}s -> %{public}s",
                    GetEncryptAddr(cdsmData->reportAddr_).c_str(), GetEncryptAddr(newReportAddr.GetAddress()).c_str());
                cdsmData->reportAddr_ = newReportAddr.GetAddress();
                isReplace = true;
                return true;
            }
            return false;
        });
    if (isReplace) {
        std::vector<std::string> cdsmAddrList = {};
        if (SleConfig::GetInstance().GetCdsmMemberList(oldReportAddr.GetAddress(), cdsmAddrList)) {
            bool isPrivate = SleConfig::GetInstance().GetCdsmIsPrivateDevice(oldReportAddr.GetAddress());
            SleConfig::GetInstance().RemoveCdsmGroup(oldReportAddr.GetAddress());

            SleConfig::GetInstance().SetCdsmIsPrivateDevice(newReportAddr.GetAddress(), isPrivate);
            SleConfig::GetInstance().SetCdsmMemberList(newReportAddr.GetAddress(), cdsmAddrList);
            SleConfig::GetInstance().Save();
        }
    }
    return isReplace;
}

/* 获取member所在合作集其他地址，暂时只考虑两个设备场景 */
bool CdsmService::CdsmGetOtherAddr(const RawAddress &member, RawAddress &other)
{
    bool isExisted = false;
    uint32_t cdsmGrpId = CDSM_SERVICE_INVALID_GROUP_ID;
    if (!CdsmGetGroupId(cdsmGrpId, member.GetAddress())) {
        return isExisted;
    }

    auto findOther = [&other, &member, &isExisted](uint32_t grpId, std::shared_ptr<CdsmInfo> cdsmData) -> void {
        auto setOther = [&other, &member](const std::string devAddr, uint8_t &devState) -> bool {
            if (member.GetAddress() != devAddr) {
                other.SetAddress(devAddr);
                return true;
            }
            return false;
        };
        isExisted = cdsmData->memberList_.Find(setOther);
    };
    cdsmList_.GetValueAndOpt(cdsmGrpId, findOther);

    return isExisted;
}

/* 数据恢复到协议栈 */
void CdsmService::CdsmRecoverStackData(uint32_t grpId, std::vector<std::string> &memberList)
{
    NL_CHECK_RETURN(memberList.size() <  0xFF, "list length over size"); //  0xFF是UINT8_MAX上限
    uint8_t memberCnt = static_cast<uint8_t>(memberList.size());
    if (memberCnt == 0) {
        HILOGW("[cdsm recover]:group member size invalid.");
        return;
    }

    SLE_Addr_S *addrList = new (std::nothrow) SLE_Addr_S[memberCnt];
    if (addrList == nullptr) {
        HILOGW("[cdsm recover]:alloc group mem fail.");
        return;
    }
    (void)memset_s(addrList, sizeof(SLE_Addr_S) * memberCnt, 0, sizeof(SLE_Addr_S) * memberCnt);
    uint8_t addrCnt = 0;
    for (auto &member : memberList) {
        RawAddress tmpAddr(member);
        tmpAddr.ConvertToUint8(addrList[addrCnt].addr, SLE_ADDR_LEN);
        addrCnt++;
    }
    NLSTK_CdsmRecoverMeb(grpId, memberCnt, addrList);

    delete[] addrList;
}

/* 从xml中恢复合作集的数据 */
void CdsmService::CdsmRecoverFromConf()
{
    /* 读取合作集所有信息到内存 */
    uint32_t cdsmValidCnt = 0;
    std::vector<std::string> cdsmReportList = SleConfig::GetInstance().GetAllCdsmReportList();
    for (auto &reportAddr : cdsmReportList) {
        /* 获取设备列表 */
        std::vector<std::string> memberList = {};
        if (!SleConfig::GetInstance().GetCdsmMemberList(reportAddr, memberList)) {
            HILOGE("[cdsm recover]:get cdsm member list failed,report addr:%{public}s.",
                GetEncryptAddr(reportAddr).c_str());
            continue;
        }

        if (memberList.size() == 0) {
            HILOGW("[cdsm recover]:get cdsm member list failed,size invalid.");
            continue;
        }

        /* 插入本地列表 */
        uint32_t cdsmGrpId = CDSM_SERVICE_INVALID_GROUP_ID;
        if (CdsmGetGroupId(cdsmGrpId, reportAddr)) {
            HILOGW("[cdsm recover]:create cdsm group,already existed,addr:%{public}s",
                GetEncryptAddr(reportAddr).c_str());
            continue;
        }

        SLE_Addr_S peerAddr;
        RawAddress cdsmAddr(reportAddr);
        cdsmAddr.ConvertToUint8(peerAddr.addr, SLE_ADDR_LEN);
        cdsmGrpId = NLSTK_CdsmCreateSet(&peerAddr);
        if (cdsmGrpId == CDSM_SERVICE_INVALID_GROUP_ID) {
            HILOGE("[cdsm recover]:create cdsm group failed,addr:%{public}s",
                GetEncryptAddr(reportAddr).c_str());
            continue;
        }
        CdsmRecoverStackData(cdsmGrpId, memberList);

        bool isPrivateTws = SleConfig::GetInstance().GetCdsmIsPrivateDevice(reportAddr);
        std::vector<CdsmCallBackDataMemInfo> members;
        for (auto memberAddr : memberList) {
            CdsmCallBackDataMemInfo memInfo;
            memInfo.memberAddr = RawAddress(memberAddr);
            memInfo.memberState = CDSM_STACK_CLIENT_DISCONNECTED;
            members.push_back(memInfo);
        }
        CreateNewCdsmGroupInfoInner(cdsmAddr, cdsmGrpId, members, isPrivateTws);
        cdsmValidCnt++;
    }

    HILOGI("[cdsm recover]:data recover completed,valid data num:%{public}u.", cdsmValidCnt);
}

void CdsmService::GetRealConnectAddress(const RawAddress &addr, RawAddress &realConnectAddr)
{
    // 先给个初始值，保证realConnectAddr有值
    realConnectAddr = addr;
    bool isCooperationAddr = CdsmCheckIsCooperationDevice(addr);
    // 如果在合作集中没有找到，那么返回传入的原始地址（说明该设备非合作集类型）
    if (!isCooperationAddr) {
        return;
    }

    // 找到另外的设备地址
    RawAddress otherAddr;
    CdsmGetOtherAddr(addr, otherAddr);

    cdsmList_.Iterate([addr, otherAddr, &realConnectAddr](const uint32_t, std::shared_ptr<CdsmInfo> info) {
        if (!info->CdsmIsGroupMember(addr.GetAddress())) {
            return;
        }
        SleConnectState state = SleConnectState::DISCONNECTED;
        // 如果当前设备已连接，那么返回传入的当前设备地址
        bool find = info->membersProfileState_.GetValue(addr.GetAddress(), state);
        if (find && state == SleConnectState::CONNECTED) {
            return;
        }
        // 如果当前设备未连接，那么查找合作集中已连接的设备返回
        find = info->membersProfileState_.GetValue(otherAddr.GetAddress(), state);
        if (find && state == SleConnectState::CONNECTED) {
            realConnectAddr = otherAddr;
            return;
        }
    });
    HILOGI("[Cdsm Service]GetRealConnectAddress addr:%{public}s, realConnectAddr:%{public}s.",
        GetEncryptAddr(addr.GetAddress()).c_str(), GetEncryptAddr(realConnectAddr.GetAddress()).c_str());
}

REGISTER_CLASS_CREATOR(CdsmService);

} // namespace Sle
} // namespace OHOS