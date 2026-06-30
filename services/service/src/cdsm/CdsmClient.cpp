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
#include <map>
#include <memory>

#include "sle_uuid.h"
#include "SleInterfaceAdapter.h"
#include "SleInterfaceManager.h"
#include "sysdep.h"
#include "SleServiceFfrtLog.h"
#include "ThreadUtil.h"
#include "nlstk_public_define.h"
#include "CdsmMessage.h"
#include "CdsmService.h"
#include "CdsmClient.h"
#include "nearlink_system_config.h"

namespace OHOS {
namespace Nearlink {
std::shared_ptr<CdsmClient> CdsmClient::CreateCdsmClient(const std::string &address)
{
    bool isAudioSupported = NearlinkSystemConfig::IsAudioSupported();
    if (!isAudioSupported) {
        HILOGW("[Cdsm Service]:not support sle audio.");
        return nullptr;
    }

    std::shared_ptr<CdsmClient> cdsmClient = std::make_shared<CdsmClient>(address);
    return cdsmClient;
}

/* 调用底层接口开始连接合作集服务 */
bool CdsmClient::CdsmClientStartConnect()
{
    SLE_Addr_S peerAddr;
    RawAddress CdsmAddr(address_);
    CdsmAddr.ConvertToUint8(peerAddr.addr, SLE_ADDR_LEN);
    uint32_t ret = NLSTK_CdsmConnect(&peerAddr);
    return ret == NLSTK_ERRCODE_SUCCESS;
}

bool CdsmClient::CdsmClientStartDisconnect()
{
    SLE_Addr_S peerAddr;
    RawAddress CdsmAddr(address_);
    CdsmAddr.ConvertToUint8(peerAddr.addr, SLE_ADDR_LEN);
    uint32_t ret = NLSTK_CdsmDisconnect(&peerAddr);
    return ret == NLSTK_ERRCODE_SUCCESS;
}

void CdsmClient::CdsmClientUpdateState(CdsmClientState toState)
{
    HILOGI("Cdsm client state change,addr:%{public}s,state:%{public}u --> %{public}u",
        GetEncryptAddr(address_).c_str(), CdsmClientGetState(), toState);

    if (CdsmClientGetState() == toState) {
        return;
    }

    /* 调用服务接口上报每个设备的状态 */
    RawAddress device(address_);
    CdsmService *cdsmService = CdsmService::GetService();
    if (cdsmService == nullptr) {
        HILOGE("Cdsm Client state change fail,service instance invalid.");
        return;
    }

    cdsmService->NotifyStateChanged(device, toState, CdsmClientGetState());
    CdsmClientSetState(toState);

    if (cdsmService->CdsmCheckIsPrivateCooperationDevice(device)) {
        HILOGD("[Cdsm Client]:dont start cdsm invition adv for private tws.");
        return;
    }
    /* 主耳连接成功，启动邀请广播（公版方案） */
    if (toState == CdsmClientState::CDSM_STATE_CONNECTED &&
        cdsmService->CdsmCheckIsCooperationReport(device)) {
        SLE_Addr_S reportAddr;
        device.ConvertToUint8(reportAddr.addr, SLE_ADDR_LEN);
        NLSTK_CdsmStartAdv(&reportAddr);
        HILOGI("[Cdsm Client]:start cdsm invition adv.");
    }
}

void CdsmClient::CdsmStopInviteAdv()
{
    RawAddress device(address_);
    CdsmService *cdsmService = CdsmService::GetService();
    if (cdsmService != nullptr && cdsmService->CdsmCheckIsPrivateCooperationDevice(device)) {
        HILOGD("[Cdsm Client]:dont stop cdsm invition adv for private tws.");
        return;
    }

    SLE_Addr_S reportAddr;
    device.ConvertToUint8(reportAddr.addr, SLE_ADDR_LEN);
    NLSTK_CdsmStopAdv(&reportAddr);
    HILOGI("[Cdsm Client]:stop cdsm invition adv,addr:%{public}s.", GetEncryptAddr(address_).c_str());
}

void CdsmClient::CdsmClientSetState(CdsmClientState newState)
{
    cdsmClientState_.store(newState);
}

CdsmClientState CdsmClient::CdsmClientGetState(void)
{
    return cdsmClientState_.load();
}

} // namespace Sle
} // namespace OHOS