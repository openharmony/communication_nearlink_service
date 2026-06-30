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
#ifndef CDSM_SERVICE_H
#define CDSM_SERVICE_H

#include <algorithm>
#include <atomic>
#include <map>
#include <list>
#include <mutex>
#include <vector>
#include <cstring>
#include <cmath>

#include "SleInterfaceProfileCdsm.h"
#include "SleInterfaceAdapter.h"
#include "nearlink_def.h"
#include "nearlink_safe_map.h"
#include "BaseObserverList.h"
#include "ProfileServiceManager.h"
#include "context.h"
#include "log_util.h"

#include "CdsmDefines.h"
#include "CdsmMessage.h"
#include "CdsmClient.h"
#include "nearlink_safe_list.h"

namespace OHOS {
namespace Nearlink {

struct CdsmCallBackDataMemInfo {
    RawAddress memberAddr;
    uint8_t memberState;
};

struct CdsmCallBackData {
    RawAddress currentAddr;
    uint32_t cdmsGroupId;
    uint8_t event;
    uint8_t memberNum;
    std::vector<CdsmCallBackDataMemInfo> memberInfo;
};

/* 单个合作集数据 */
class CdsmInfo {
public:
    uint32_t cdsmGrpId_ = 0;      /* 合作集组ID（内部） */
    std::string reportAddr_ = ""; /* 合作集上报地址（主设备或上报设备地址） */
    uint32_t memberCnt_ = 0;      /* 合作集成员数量（包含report设备） */
    bool isPrivateTws_ = false;    /* 是否是私有耳机合作集 */
    int reportConnectState_ = static_cast<int>(SleConnectState::INVALID_STATE);    /* 上报给应用的连接状态 */
    NearlinkSafeMap<std::string, uint8_t> memberList_; /* 合作集成员设备信息（线程安全map），包含report、成员设备信息 */
    NearlinkSafeMap<std::string, SleConnectState> membersProfileState_;  /* 合作集中每个成员的所有Profile连接状态 */

public:
    CdsmInfo(uint32_t cdsmGrpId, std::string reportAddr) : cdsmGrpId_(cdsmGrpId), reportAddr_(reportAddr) {}
    ~CdsmInfo() = default;

    void CdsmAddMemberInfo(const std::string& devAddr, uint8_t devState);
    void CdsmDeleteMember(const std::string &devAddr);
    bool CdsmUpdateDevState(const std::string devAddr, uint8_t devState);
    bool CdsmUpdateMemberProfileConnectState(const std::string &addr, SleConnectState profileState);
    bool CdsmIsGroupMember(const std::string &memberAddr);
    void CdsmGetAllMember(std::vector<std::string> &devList);
}; /* CdsmInfo */

class CdsmService : public ProfileCdsm, public utility::Context  {
public:
    /* 构造、析构 */
    explicit CdsmService();
    virtual ~CdsmService();

    /* Profile管理模块依赖的基本接口 */
    utility::Context *GetContext() override;

    int Connect(const RawAddress &device) override;
    int Disconnect(const RawAddress &device) override;

    void Enable() override;
    void Disable() override;

    void RegisterObserver(CdsmObserver &serviceObserver) override;
    void DeregisterObserver(CdsmObserver &serviceObserver) override;

    int GetConnectState() override;
    std::list<RawAddress> GetConnectDevices() override;
    void NotifyStateChanged(const RawAddress &device, CdsmClientState state, CdsmClientState preState);

    /* 合作集注册给协议栈的回调 */
    static void CdsmStackProfileCallback(NLSTK_CdsmEvent_S* eventMsg);

    /* 服务内部模块接口 */
    static CdsmService *GetService();
    void PostEvent(const CdsmMessage &event);
    void ProcessEvent(const CdsmMessage &event);
    bool CdsmCheckIsPrivateCooperationDevice(const RawAddress &devAddr);

    /* 服务对外提供的接口列表 */
    NlErrCode RegisterCdsmClientCallback(const std::shared_ptr<InterfaceCdsmClientServiceCallback> &cbk) override;
    NlErrCode DeregisterCdsmClientCallback() override;
    NlErrCode CdsmGetAllMemberInfo(const RawAddress &memberAddr, std::vector<NearlinkCdsmInfo> &cdsmInfo) override;
    bool CdsmCheckIsCooperationDevice(const RawAddress &devAddr) override;
    bool CdsmCheckIsCooperationMember(const RawAddress &devAddr) override;
    bool CdsmCheckIsCooperationReport(const RawAddress &devAddr) override;
    bool CdsmCheckIsSameCooperation(const std::vector<RawAddress> &devAddrList) override;
    bool CdsmGetReportAddr(const RawAddress &memberAddr, RawAddress &reportAddr) override;
    void CdsmDeleteGroup(const RawAddress &devAddr) override;
    void CdsmStopInviteAdv(const RawAddress &devAddr, bool isForceStop) override;
    uint32_t CdsmCreateGroup(const RawAddress &reportAddr, const std::vector<RawAddress> &devAddrList,
        bool isPrivateTws = false) override;
    bool CdsmReplaceOldReportAddr(const RawAddress &oldReportAddr, const RawAddress &newReportAddr) override;
    bool CdsmGetOtherAddr(const RawAddress &member, RawAddress &other) override;
    void CdsmRecoverFromConf() override;
    bool CdsmGetGroupId(uint32_t &groupId, std::string memberAddr) override;
    /* 上报组内成员连接状态变化（每个成员的所有的Profile都连接才算已连接，不是仅CdsmProfile的连接状态） */
    void HandleCdsmClientCallback(const RawAddress &device, std::optional<uint64_t> tokenId = std::nullopt) override;

    void UpdateCdsmMemberProfileConnectState(const RawAddress &device, SleConnectState profileState);
    void UpdateReportConnectState(const RawAddress &device, int profileState);
    void CdsmGetReportedState(const RawAddress &devAddr, int &reportConnectState);
    void GetRealConnectAddress(const RawAddress &addr, RawAddress &realConnectAddr);

private:
    void HandleCdsmClientCallbackTask(const RawAddress &device, std::optional<uint64_t> tokenId = std::nullopt);
    /* 单个设备状态变化上报映射表，将服务状态转为adapter识别的状态 */
    const std::map<const CdsmClientState, const int> stateMap_ = {
        { CdsmClientState::CDSM_STATE_DISCONNECTED, static_cast<int>(SleConnectState::DISCONNECTED) },
        { CdsmClientState::CDSM_STATE_CONNECTING, static_cast<int>(SleConnectState::CONNECTING) },
        { CdsmClientState::CDSM_STATE_DISCONNECTING, static_cast<int>(SleConnectState::DISCONNECTING) },
        { CdsmClientState::CDSM_STATE_CONNECTED, static_cast<int>(SleConnectState::CONNECTED) }
    };

    /* CDSM服务内部状态列表 */
    std::atomic_bool isStarted_ = ATOMIC_FLAG_INIT;
    std::atomic_bool isShuttingDown_ = ATOMIC_FLAG_INIT;
    /* 合作集成员状态 */
    NearlinkSafeMap<std::string, std::shared_ptr<CdsmClient>> cdsmClients_ {};

    void StartUp();
    void ShutDown();

    /* 合作集服务消息切换处理接口 */
    void ProcessConnectEvent(const CdsmMessage &event);
    void ProcessDisconnectEvent(const CdsmMessage &event);
    void ProcessDefaultEvent(const CdsmMessage &event, CdsmClientState toState);

    /* Profile管理模块注册的观察者，上报服务状态 */
    BaseObserverList<CdsmObserver> cdsmObservers_ {};

    void CdsmServiceCallBackDataProc(CdsmCallBackData dataBlock);

    /* 合作集数据 */
    NearlinkSafeMap<uint32_t, std::shared_ptr<CdsmInfo>> cdsmList_ {};

    /* 合作集列表操作接口 */
    void CdsmDeleteMember(uint32_t memGrpId, std::string memberAddr);

    /* 合作集回调 */
    std::shared_ptr<InterfaceCdsmClientServiceCallback> cdsmCbk_ {};

    void ConnectCdsmInterface(const RawAddress &device);
    void DisconnectCdsmInterface(const RawAddress &device);
    void CdsmServiceCreateNewCdsm(CdsmCallBackData &dataBlock);
    /* 获取内部已连接设备数 */
    void CdsmRecoverStackData(uint32_t grpId, std::vector<std::string> &memberList);

    /* 更新集合数据并剔除旧设备信息 */
    void UpdateCdsmData(CdsmCallBackData &dataBlock);
    bool ClearOldDevice(CdsmCallBackData &dataBlock, std::shared_ptr<CdsmInfo> cdsmData, RawAddress &oldDevAddr);
    std::shared_ptr<CdsmInfo> CreateNewCdsmGroupInfoInner(RawAddress reportAddr, uint32_t cdsmGrpId,
        std::vector<CdsmCallBackDataMemInfo> devAddrList, bool isPrivateTws);
    uint32_t CdsmCreateGroupInner(const RawAddress& reportAddr, const std::vector<RawAddress>& devAddrList,
        bool isPrivateTws);
    uint32_t CdsmGenerateGroupIdInner(const RawAddress &reportAddr);
    void CdsmDeleteGroupInner(const RawAddress &devAddr);
    bool IsVendorDevice(const RawAddress &device);
    void SetPeerDeviceTypeToControllerInner(const RawAddress &device);
}; /* CdsmService */

} // namespace Sle
} // namespace OHOS

#endif /* END of CDSM_SERVICE_H */