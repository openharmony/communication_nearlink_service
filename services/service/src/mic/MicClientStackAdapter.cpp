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
#include "MicClientStackAdapter.h"

#include "MicService.h"
#include "SleServiceFfrtLog.h"
#include "nlstk_micp_client.h"
#include "SleRemoteDeviceAdapter.h"
#include "nlstk_public_define.h"

namespace OHOS {
namespace Nearlink {
namespace {
SLE_Addr_S ConvertToStackAddr(const RawAddress &addr)
{
    SLE_Addr_S sleAddr;
    (void)memset_s(&sleAddr, sizeof(SLE_Addr_S), 0, sizeof(SLE_Addr_S));
    addr.ConvertToUint8(sleAddr.addr);
    sleAddr.type = SleRemoteDeviceAdapter::GetInstance()->GetPeerDeviceAddrType(addr);
    return sleAddr;
}
}

MicClientStackAdapter::MicClientStackAdapter() {}

MicClientStackAdapter::~MicClientStackAdapter() = default;

void MicClientStackAdapter::OnMicpConnectEventCbk(
    SLE_Addr_S *addr, NLSTK_MicpConnectState_E curState, NLSTK_MicpConnectState_E preState, uint8_t errorCode)
{
    NL_CHECK_RETURN(addr, "[MicAdapter]addr is null.");
    RawAddress device = RawAddress::ConvertToString(addr->addr);
    HILOGI("[MicAdapter] addr(%{public}s), curState(%{public}d), preState(%{public}d), ret(%{public}d)",
        GET_ENCRYPT_ADDR(device), curState, preState, errorCode);
    MicService *micService = MicService::GetService();
    NL_CHECK_RETURN(micService, "[MicAdapter]micService is null.");
    micService->NotifyStateChanged(
        device, static_cast<SleConnectState>(curState), static_cast<SleConnectState>(preState));
}

void MicClientStackAdapter::OnMicpMicStateCbk(SLE_Addr_S *addr, uint8_t micState)
{
    NL_CHECK_RETURN(addr, "[MicAdapter]addr is null.");
    NL_CHECK_RETURN(micState <= static_cast<MicState>(NLSTK_MICP_MIC_ON),
        "[MicAdapter]micState out of range: %{public}d", micState);
    RawAddress device = RawAddress::ConvertToString(addr->addr);
    HILOGI("[MicAdapter] addr(%{public}s), micState(%{public}d)", GET_ENCRYPT_ADDR(device), micState);
    MicService *micService = MicService::GetService();
    NL_CHECK_RETURN(micService, "[MicAdapter]micService is null.");
    micService->UpdateMicState(device, micState);
}

int MicClientStackAdapter::RegisterCallBackToStack()
{
    NLSTK_MicpCbk_S cb = {};
    cb.eventCbk = &OnMicpConnectEventCbk;
    cb.micStateCbk = &OnMicpMicStateCbk;
    uint32_t ret = NLSTK_MicpRegisterCallback(&cb);
    NL_CHECK_RETURN_RET(ret == NLSTK_ERRCODE_SUCCESS, MIC_FAILURE,
        "[MicAdapter]register callback to stack failed, ret(%{public}d).", ret);
    return MIC_SUCCESS;
}

int MicClientStackAdapter::Connect(const RawAddress &addr)
{
    HILOGD("[MicAdapter] Connect device addr(%{public}s)", GET_ENCRYPT_ADDR(addr));
    SLE_Addr_S stackAddr = ConvertToStackAddr(addr);
    uint32_t ret = NLSTK_MicpConnect(&stackAddr);
    NL_CHECK_RETURN_RET(ret == NLSTK_ERRCODE_SUCCESS, MIC_FAILURE, "[MicAdapter]Connect fail ret=%{public}d", ret);
    return MIC_SUCCESS;
}

int MicClientStackAdapter::Disconnect(const RawAddress &addr)
{
    HILOGD("[MicAdapter] Disconnect device addr(%{public}s)", GET_ENCRYPT_ADDR(addr));
    SLE_Addr_S stackAddr = ConvertToStackAddr(addr);
    uint32_t ret = NLSTK_MicpDisconnect(&stackAddr);
    NL_CHECK_RETURN_RET(ret == NLSTK_ERRCODE_SUCCESS, MIC_FAILURE, "[MicAdapter]Disconnect fail ret=%{public}d", ret);
    return MIC_SUCCESS;
}

}
}
