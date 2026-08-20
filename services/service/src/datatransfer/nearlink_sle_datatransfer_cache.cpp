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

#include "nearlink_sle_datatransfer_cache.h"
#include "log.h"

namespace OHOS::Nearlink {
namespace {
constexpr size_t PORT_CONNECT_MAX_NUM = 0xFFFF;
}
SleDataTransferCache::SleDataTransferCache() : portConnectionList_()
{
    portConnectionList_.Clear();
}

SleDataTransferCache::~SleDataTransferCache()
{
    portConnectionList_.Clear();
}

void SleDataTransferCache::SetUuid(const std::string &uuid)
{
    uuid_ = uuid;
}

std::string SleDataTransferCache::GetUuid() const
{
    return uuid_;
}

void SleDataTransferCache::SetTokenId(uint64_t tokenId)
{
    tokenId_ = tokenId;
}

uint64_t SleDataTransferCache::GetTokenId() const
{
    return tokenId_;
}

void SleDataTransferCache::SetUid(int32_t uid)
{
    uid_ = uid;
}

int32_t SleDataTransferCache::GetUid() const
{
    return uid_;
}

void SleDataTransferCache::SetPid(int32_t pid)
{
    pid_ = pid;
}

int32_t SleDataTransferCache::GetPid() const
{
    return pid_;
}

std::vector<AppConnectParamMapping> SleDataTransferCache::GetPortConnects()
{
    std::vector<AppConnectParamMapping> res;
    portConnectionList_.Iterate([&res](const AppConnectParamMapping& temp) -> void {
        res.push_back(temp);
    });
    return res;
}

void SleDataTransferCache::SetPortConnects(const AppConnectParamMapping &temp)
{
    NL_CHECK_RETURN(portConnectionList_.Size() < PORT_CONNECT_MAX_NUM, "the number of connect reaches the maximum");
    std::string addr = temp.address;
    NL_CHECK_RETURN(!addr.empty(), "address invalid");
    portConnectionList_.EnsureInsert(temp);
}

void SleDataTransferCache::UpdateState(int32_t newState)
{
    portConnectionList_.IterateAndModify([newState](AppConnectParamMapping &temp) -> void {
        temp.state = newState;
    });
}

void SleDataTransferCache::UpdateTransferState(const std::string &addr, uint8_t tcid, int32_t newState)
{
    portConnectionList_.IterateAndModify([&addr, tcid, newState](AppConnectParamMapping &temp) -> void {
        if (addr == temp.address && tcid == temp.tcid) {
            temp.preTransState = temp.transState;
            temp.transState = newState;
        }
    });
}

std::string SleDataTransferCache::GetRandomAddrByAddr(const std::string &addr)
{
    std::string randomAddress = addr;
    portConnectionList_.Find([&addr, &randomAddress](const AppConnectParamMapping &temp) -> bool {
        if (addr == temp.address) {
            randomAddress = temp.randomAddress;
            return true;
        }
        return false;
    });
    return randomAddress;
}

void SleDataTransferCache::UpdateStateByAddr(const std::string &addr, int32_t newState)
{
    portConnectionList_.IterateAndModify([&addr, newState](AppConnectParamMapping &temp) -> void {
        if (addr == temp.address) {
            temp.state = newState;
        }
    });
}

uint8_t SleDataTransferCache::UpdateStateAndGetTcidByAddr(const std::string &addr, int32_t newState)
{
    uint8_t res = 0;
    portConnectionList_.IterateAndModify([&addr, newState, &res](AppConnectParamMapping &temp) -> void {
        if (addr == temp.address) {
            temp.state = newState;
            res = temp.tcid;
        }
    });
    return res;
}

bool SleDataTransferCache::GetAppConnectParamByAddr(const std::string &addr, AppConnectParamMapping &param)
{
    bool has = portConnectionList_.Find([&param, &addr](const AppConnectParamMapping& temp) -> bool {
        if (temp.address == addr) {
            param = temp;
            return true;
        }
        return false;
    });
    return has;
}

bool SleDataTransferCache::HasAppConnect()
{
    bool res = portConnectionList_.Find([](const AppConnectParamMapping &temp) -> bool {
        if (temp.state == static_cast<int32_t>(SleConnectState::CONNECTED) ||
            temp.state == static_cast<int32_t>(SleConnectState::CONNECTING)) {
            HILOGI("HasAppConnect");
            return true;
        }
        return false;
    });
    return res;
}

bool SleDataTransferCache::HasRemoteAddressConnect(const std::string &address)
{
    bool res = portConnectionList_.Find([&address](const AppConnectParamMapping &temp) -> bool {
        if (temp.address == address && (temp.state == static_cast<int32_t>(SleConnectState::CONNECTED) ||
            temp.state == static_cast<int32_t>(SleConnectState::CONNECTING))) {
            HILOGI("HasAppConnect");
            return true;
        }
        return false;
    });
    return res;
}

bool SleDataTransferCache::IsAppConnect(std::string &address)
{
    bool res = portConnectionList_.Find([&address](const AppConnectParamMapping &temp) -> bool {
        if (temp.state == static_cast<int32_t>(SleConnectState::CONNECTED) ||
            temp.state == static_cast<int32_t>(SleConnectState::CONNECTING)) {
            HILOGI("HasAppConnect");
            address = temp.address;
            return true;
        }
        return false;
    });
    return res;
}

bool SleDataTransferCache::HasTargetAddr(const std::string &addr)
{
    bool res = portConnectionList_.Find([&addr](const AppConnectParamMapping &temp) -> bool {
        if (temp.address == addr) {
            HILOGI("Contain addr");
            return true;
        }
        return false;
    });
    return res;
}
#ifdef RES_SCHED_SUPPORT
bool SleDataTransferCache::needRssReportDisconnect() {
    auto func = [](AppConnectParamMapping appConnectParam) {
        return appConnectParam.state != static_cast<int32_t>(SleConnectState::DISCONNECTED);};
    return portConnectionList_.Find(func);
}
#endif
}  // namespace OHOS::Nearlink