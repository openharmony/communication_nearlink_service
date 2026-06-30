/*
 * Copyright (C) 2026 Huawei Device Co., Ltd. All rights reserved.
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

#define SLE_ADDR_LEN    6    /* nearlink address length */
#define CDSM_SERVICE_TEST_GROUP_ID    1

namespace OHOS {
namespace Nearlink {

CdsmService::CdsmService() : utility::Context(PROFILE_NAME_CDSM, "1.0.0")
{
    HILOGI("[CdsmService Mocker]:service instance create.");
}

CdsmService::~CdsmService()
{
    HILOGI("[CdsmService Mocker]:service instance destroy.");
}

utility::Context *CdsmService::GetContext()
{
    return this;
}

CdsmService *CdsmService::GetService()
{
    static CdsmService instance;
    return &instance;
}

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

uint32_t CdsmService::CdsmCreateGroup(const RawAddress &reportAddr, const std::vector<RawAddress> &devAddrList,
    bool isPrivateTws)
{
    uint32_t cdsmGrpId = CDSM_SERVICE_INVALID_GROUP_ID;
    SLE_Addr_S peerAddr;
    reportAddr.ConvertToUint8(peerAddr.addr, SLE_ADDR_LEN);
    cdsmGrpId = CDSM_SERVICE_TEST_GROUP_ID;
    if (cdsmGrpId == CDSM_SERVICE_INVALID_GROUP_ID) {
        HILOGI("[CdsmService Mocker]:create cdsm group failed,addr:%{public}s",
            GetEncryptAddr(reportAddr.GetAddress()).c_str());
        return cdsmGrpId;
    }
    std::shared_ptr<CdsmInfo> cdsmDataPtr = std::make_shared<CdsmInfo>(cdsmGrpId, reportAddr.GetAddress());
    if (cdsmDataPtr == nullptr) {
        HILOGI("[CdsmService Mocker]:alloc new cdsm data failed");
        return cdsmGrpId;
    }
    cdsmDataPtr->memberCnt_ = devAddrList.size();
    cdsmDataPtr->isPrivateTws_ = isPrivateTws;
    std::vector<std::string> memberList;
    RawAddress dftReportAddr = reportAddr;
    for (auto memInfo : devAddrList) {
        cdsmDataPtr->CdsmAddMemberInfo(memInfo.GetAddress(), CDSM_STACK_CLIENT_DISCONNECTED);
        memberList.push_back(memInfo.GetAddress());
    }
    cdsmList_.EnsureInsert(cdsmGrpId, cdsmDataPtr);
    HILOGI("[CdsmService Mocker]create cdsm group,report:%{public}s", GetEncryptAddr(reportAddr.GetAddress()).c_str());
    return cdsmGrpId;
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

bool CdsmService::CdsmReplaceOldReportAddr(const RawAddress &oldReportAddr, const RawAddress &newReportAddr)
{
    bool isReplace = false;
    cdsmList_.Find([&oldReportAddr, &newReportAddr, &isReplace](const uint32_t cdsmGrpId,
        std::shared_ptr<CdsmInfo> cdsmData) -> bool {
            if (cdsmData->CdsmIsGroupMember(newReportAddr.GetAddress()) &&
                oldReportAddr.GetAddress() == cdsmData->reportAddr_ &&
                newReportAddr.GetAddress() != cdsmData->reportAddr_) {
                HILOGI("[CdsmService Mocker]: cdsm update reportAddr %{public}s -> %{public}s",
                    GetEncryptAddr(cdsmData->reportAddr_).c_str(), GetEncryptAddr(newReportAddr.GetAddress()).c_str());
                cdsmData->reportAddr_ = newReportAddr.GetAddress();
                isReplace = true;
                return true;
            }
            return false;
        });
    return isReplace;
}

int CdsmService::Connect(const RawAddress &device)
{
    return 0;
}

int CdsmService::Disconnect(const RawAddress &device)
{
    return 0;
}

void CdsmService::Enable()
{}

void CdsmService::Disable()
{}

void CdsmService::RegisterObserver(CdsmObserver &serviceObserver)
{}

void CdsmService::DeregisterObserver(CdsmObserver &serviceObserver)
{}

std::list<RawAddress> CdsmService::GetConnectDevices()
{
    std::list<RawAddress> devices;
    return devices;
}

NlErrCode CdsmService::RegisterCdsmClientCallback(const std::shared_ptr<InterfaceCdsmClientServiceCallback> &cbk)
{
    return NL_NO_ERROR;
}

NlErrCode CdsmService::DeregisterCdsmClientCallback()
{
    return NL_NO_ERROR;
}

NlErrCode CdsmService::CdsmGetAllMemberInfo(const RawAddress &memberAddr, std::vector<NearlinkCdsmInfo> &cdsmInfo)
{
    return NL_NO_ERROR;
}

bool CdsmService::CdsmCheckIsCooperationDevice(const RawAddress &devAddr)
{
    return true;
}

bool CdsmService::CdsmCheckIsCooperationMember(const RawAddress &devAddr)
{
    return true;
}

bool CdsmService::CdsmCheckIsCooperationReport(const RawAddress &devAddr)
{
    return true;
}

bool CdsmService::CdsmCheckIsSameCooperation(const std::vector<RawAddress> &devAddrList)
{
    return true;
}

void CdsmService::CdsmDeleteGroup(const RawAddress &devAddr)
{}

void CdsmService::CdsmStopInviteAdv(const RawAddress &devAddr, bool isForceStop)
{}

void CdsmService::CdsmRecoverFromConf()
{}

void CdsmService::HandleCdsmClientCallback(const RawAddress &device, std::optional<uint64_t> tokenId)
{}

void CdsmService::HandleCdsmClientCallbackTask(const RawAddress &device, std::optional<uint64_t> tokenId)
{}

int CdsmService::GetConnectState()
{
    return 0;
}

} // namespace Nearlink
} // namespace OHOS