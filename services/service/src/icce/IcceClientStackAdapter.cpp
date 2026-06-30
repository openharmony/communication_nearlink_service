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
#include "IcceDefines.h"
#include "log.h"
#include "ThreadUtil.h"
#include "IcceClientStackAdapter.h"
#include "nearlink_dft_database.h"
#include "nearlink_dft_exception.h"

namespace OHOS {
namespace Nearlink {

static IcceClientStackAdapter *g_icceClientStackAdapter = nullptr;
struct IcceClientStackAdapter::impl {
    impl(IcceClientStackCallback &callback) : icceClientStackCbk_(callback) {}
    IcceClientStackCallback &icceClientStackCbk_;
};

IcceClientStackAdapter::IcceClientStackAdapter(IcceClientStackCallback &callback)
    : pimpl(std::make_unique<impl>(callback))
{
    g_icceClientStackAdapter = this;
}

IcceClientStackAdapter::~IcceClientStackAdapter() = default;

int IcceClientStackAdapter::Connect(const RawAddress &device)
{
    SLE_Addr_S stackAddr = ConvertToStackAddr(device);
    uint32_t ret = NLSTK_IcceConnect(&stackAddr);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        HILOGE("[ICCE Adapter] Connect failed, ret(%{public}d)", ret);
        return ICCE_FAILURE;
    }
    return ICCE_SUCCESS;
}

int IcceClientStackAdapter::Disconnect(const RawAddress &device)
{
    SLE_Addr_S stackAddr = ConvertToStackAddr(device);
    uint32_t ret = NLSTK_IcceDisconnect(&stackAddr);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        HILOGE("[ICCE Adapter] Disconnect failed, ret(%{public}d)", ret);
        return ICCE_FAILURE;
    }
    return ICCE_SUCCESS;
}

void IcceClientStackAdapter::OnConnectionStateChangedTask(const RawAddress &device, SleConnectState curState,
    SleConnectState prevState)
{
    pimpl->icceClientStackCbk_.OnConnectionStateChanged(device, static_cast<int>(curState),
        static_cast<int>(prevState));
}

void IcceClientStackAdapter::OnConnectionStateChange(SLE_Addr_S *sleAddr, NLSTK_IcceConnectState_E curState,
    NLSTK_IcceConnectState_E prevState, NLSTK_Errcode_E errNumb)
{
    HILOGI("[IcceClient]:OnConnectionStateChange, newstate=%{public}d, oldstate=%{public}d,\
        reason=%{public}d", static_cast<int>(curState), static_cast<int>(prevState), static_cast<int>(errNumb));
    if (prevState == ICCE_CONNECTING && curState == ICCE_DISCONNECTED) {
        DftReportPairInfo(ConvertSleAddrToRawAddress(sleAddr).GetAddress(),
            PAIR_CONN_PATH_ICCE, static_cast<int32_t>(errNumb));
    }
    DoInIcceThread([icceClientStackAdapter = g_icceClientStackAdapter,
        addr = ConvertSleAddrToRawAddress(sleAddr), newState = static_cast<SleConnectState>(curState),
            oldState = static_cast<SleConnectState>(prevState)]() -> void {
        icceClientStackAdapter->OnConnectionStateChangedTask(addr, newState, oldState);
    });
}

int IcceClientStackAdapter::RegisterCallBackToStack()
{
    NLSTK_IcceClientCallBack_S cbs = {};
    cbs.connectStateChangeCbk = &OnConnectionStateChange;
    uint32_t ret = NLSTK_IcceRegisterReadInfoCallBack(&cbs);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        HILOGE("[ICCE Adapter] register callback to stack failed, ret(%{public}d).", ret);
        return ICCE_FAILURE;
    }
    return ICCE_SUCCESS;
}
uint8_t IcceClientStackAdapter::GetConnectionsDeviceNum()
{
    return NLSTK_GetConnectionsDeviceNum();
}

int32_t IcceClientStackAdapter::GetPort(const RawAddress &device)
{
    SLE_Addr_S stackAddr = ConvertToStackAddr(device);
    int32_t port = ICCE_INVAILD;
    if (NLSTK_IcceGetPort(&stackAddr, &port) != NLSTK_ERRCODE_SUCCESS) {
        HILOGE("[ICCE Adapter] Get port fail");
    }
    return port;
}

} // namespace Nearlink
} // namespace OHOS
