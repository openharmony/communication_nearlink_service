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
#ifndef SLE_INTERFACE_PROFILE_CDSM_H
#define SLE_INTERFACE_PROFILE_CDSM_H

#include <optional>


#include "SleInterfaceProfile.h"
#include "nearlink_errorcode.h"

namespace OHOS {
namespace Nearlink {
class CdsmObserver {
public:
    virtual ~CdsmObserver() = default;

    virtual void OnConnectionStateChanged(const RawAddress &deviceAddress, int state, int oldState) {};
}; /* CdsmObserver */

/* 合作集单个设备服务连接状态 */
enum class CdsmConnectState : uint8_t {
    DISCONNECTED  = 0,       /* 未连接 */
    CONNECTED     = 1,       /* 已连接 */
};

class NearlinkCdsmInfo {
public:
    NearlinkCdsmInfo() = default;
    ~NearlinkCdsmInfo() = default;

public:
    RawAddress addr_ = RawAddress(INVALID_MAC_ADDRESS);
    uint8_t state_ = 0; /* profile连接状态 @ref CdsmConnectState */
    bool isReportAddr_ = false;
};

class InterfaceCdsmClientServiceCallback {
public:
    virtual ~InterfaceCdsmClientServiceCallback() = default;
    virtual void OnCdsmClientStateChange(const RawAddress &remoteAddr, std::vector<NearlinkCdsmInfo> &cdsmInfo,
        std::optional<uint64_t> tokenId = std::nullopt) = 0;
};

class ProfileCdsm : public SleInterfaceProfile {
public:
    /**
     * @brief  register observer
     *
     * @param  observer         function pointer
     */
    virtual void RegisterObserver(CdsmObserver &observer) = 0;

    /**
     * @brief  deregister observer
     *
     * @param  observer         function pointer
     */
    virtual void DeregisterObserver(CdsmObserver &observer) = 0;

    /* 注册状态变化上报接口 */
    virtual NlErrCode RegisterCdsmClientCallback(const std::shared_ptr<InterfaceCdsmClientServiceCallback> &cbk) = 0;

    /* 去注册状态变化回调 */
    virtual NlErrCode DeregisterCdsmClientCallback() = 0;

    /* 根据设备地址获取所有成员信息 */
    virtual NlErrCode CdsmGetAllMemberInfo(const RawAddress &memberAddr, std::vector<NearlinkCdsmInfo> &cdsmInfo) = 0;

    /* 判断是否合作集设备（report或成员地址） */
    virtual bool CdsmCheckIsCooperationDevice(const RawAddress &devAddr) = 0;

    /* 判断是否合作集设备成员地址（非report的合作集设备） */
    virtual bool CdsmCheckIsCooperationMember(const RawAddress &devAddr) = 0;

    /* 判断是否合作集设备Report地址 */
    virtual bool CdsmCheckIsCooperationReport(const RawAddress &devAddr) = 0;

    /* 判断指定地址列表是否一个合作集 */
    virtual bool CdsmCheckIsSameCooperation(const std::vector<RawAddress> &devAddrList) = 0;

    /* 获取合作集对外地址 */
    virtual bool CdsmGetReportAddr(const RawAddress &memberAddr, RawAddress &reportAddr) = 0;

    /* 删除合作集 */
    virtual void CdsmDeleteGroup(const RawAddress &devAddr) = 0;

    /* 停止邀请广播 */
    virtual void CdsmStopInviteAdv(const RawAddress &devAddr, bool isForceStop) = 0;

    /* 创建合作集 */
    virtual uint32_t CdsmCreateGroup(const RawAddress &reportAddr, const std::vector<RawAddress> &devAddrList,
        bool isPrivateTws = false) = 0;

    /* 已有合作集的reportAddr更新 */
    virtual bool CdsmReplaceOldReportAddr(const RawAddress &oldReportAddr, const RawAddress &newReportAddr) = 0;

    /* 获取除当前设备外，合作集其他设备 */
    virtual bool CdsmGetOtherAddr(const RawAddress &member, RawAddress &other) = 0;

    /* 从xml中恢复合作集数据 */
    virtual void CdsmRecoverFromConf() = 0;

    /* 获取合作集列表id */
    virtual bool CdsmGetGroupId(uint32_t &groupId, std::string memberAddr) = 0;

    /* 触发合作集状态变化的回调 */
    virtual void HandleCdsmClientCallback(const RawAddress &device, std::optional<uint64_t> tokenId = std::nullopt) = 0;
};

} /* namespace Sle */
} /* namespace OHOS */

#endif /* END of SLE_INTERFACE_PROFILE_CDSM_H */