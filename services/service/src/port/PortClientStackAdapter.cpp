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
#include "PortClientStackAdapter.h"
#include "PortService.h"
#include "log_util.h"
#include "nearlink_dft_exception.h"
#include "SleRemoteDeviceAdapter.h"

namespace OHOS {
namespace Nearlink {
namespace {
constexpr uint8_t PORT_PROFILE_MAX_CONNECTION_NUM = 8;
const std::string PAIR_PORT_CONNECT_FAIL = "PORT Connect Failed";

SLE_Addr_S ConvertToStackAddr(const RawAddress &addr)
{
    SLE_Addr_S sleAddr;
    (void)memset_s(&sleAddr, sizeof(SLE_Addr_S), 0, sizeof(SLE_Addr_S));
    addr.ConvertToUint8(sleAddr.addr);
    sleAddr.type = SleRemoteDeviceAdapter::GetInstance()->GetPeerDeviceAddrType(addr);
    return sleAddr;
}

NLSTK_SsapUuid_S ConvertToStackUuid(const Uuid &uuid)
{
    NLSTK_SsapUuid_S sleUuid;
    (void)memset_s(&sleUuid, sizeof(NLSTK_SsapUuid_S), 0, sizeof(NLSTK_SsapUuid_S));
    uuid.ConvertToBytesLE(sleUuid.uuid);
    return sleUuid;
}

}

PortClientStackAdapter::PortClientStackAdapter()
{}

PortClientStackAdapter::~PortClientStackAdapter() = default;

void PortClientStackAdapter::OnConnectionStateChanged(
    SLE_Addr_S *addr, NLSTK_PortConnectState_E curState, NLSTK_PortConnectState_E preState, NLSTK_Errcode_E ret)
{
    NL_CHECK_RETURN(addr, "addr is null.");
    RawAddress device = RawAddress::ConvertToString(addr->addr);
    HILOGI("[Port Adapter] addr(%{public}s), curState(%{public}d), preState(%{public}d), ret(%{public}d)",
        GET_ENCRYPT_ADDR(device), curState, preState, ret);

    PortService *portService = PortService::GetPortService();
    NL_CHECK_RETURN(portService, "portService is null.");
    if (curState == PORT_DISCONNECTED && preState == PORT_CONNECTING) {
        DftReportPairInfo(device.GetAddress(), PAIR_CONN_PATH_PORT, ret, PAIR_PORT_CONNECT_FAIL);
    }
    portService->NotifyStateChanged(device, static_cast<SleConnectState>(curState),
        static_cast<SleConnectState>(preState));
}

int PortClientStackAdapter::RegisterCallBackToStack()
{
    NLSTK_PortClientCallBack_S cb = {};
    cb.connectStateChangeCbk = &OnConnectionStateChanged;
    NLSTK_Errcode_E ret = NLSTK_PortClientRegCbk(&cb);
    NL_CHECK_RETURN_RET(ret == NLSTK_ERRCODE_SUCCESS, PORT_FAILURE,
        "[PORT Adapter] register callback to stack failed, ret(%{public}d).", ret);
    return PORT_SUCCESS;
}

int PortClientStackAdapter::DeregisterCallBackToStack(void)
{
    NLSTK_Errcode_E ret = NLSTK_PortClientDeregCbk();
    NL_CHECK_RETURN_RET(ret == NLSTK_ERRCODE_SUCCESS, PORT_FAILURE,
        "[PORT Adapter] deregister callback to stack failed, ret(%{public}d).", ret);
    return PORT_SUCCESS;
}

int PortClientStackAdapter::Connect(const RawAddress &addr, const NLSTK_ConnParam_S &connParam)
{
    SLE_Addr_S stackAddr = ConvertToStackAddr(addr);
    int portConnectNum = 0;
    // 检查设备连接数
    NL_CHECK_RETURN_RET(NLSTK_PortGetConnectDeviceNum(&stackAddr, &portConnectNum) == NLSTK_ERRCODE_SUCCESS,
        PORT_FAILURE, "[PORT Adapter] NLSTK_PortGetConnectDeviceNum failed!");
     NL_CHECK_RETURN_RET(portConnectNum < PORT_PROFILE_MAX_CONNECTION_NUM, PORT_FAILURE,
        "[PORT Adapter] The number of port connections has reached the upper limit!");
    NLSTK_Errcode_E ret = NLSTK_PortConnect(&stackAddr, &connParam);
    NL_CHECK_RETURN_RET(ret == NLSTK_ERRCODE_SUCCESS, PORT_FAILURE,
        "[PORT Adapter] Connect failed, ret(%{public}d)", ret);
    return PORT_SUCCESS;
}

int PortClientStackAdapter::Disconnect(const RawAddress &addr)
{
    SLE_Addr_S stackAddr = ConvertToStackAddr(addr);
    NLSTK_Errcode_E ret = NLSTK_PortDisconnect(&stackAddr);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        HILOGE("[PORT Adapter] Disconnect failed, ret(%{public}d)", ret);
        return PORT_FAILURE;
    }
    return PORT_SUCCESS;
}

int PortClientStackAdapter::GetConnectState(const RawAddress &addr, int &state)
{
    SLE_Addr_S stackAddr = ConvertToStackAddr(addr);
    NLSTK_Errcode_E ret = NLSTK_PortGetConnectState(&stackAddr, &state);
    NL_CHECK_RETURN_RET(ret == NLSTK_ERRCODE_SUCCESS, PORT_FAILURE,
        "[PORT Adapter] GetConnectState failed, ret(%{public}d)", ret);
    return PORT_SUCCESS;
}

uint16_t PortClientStackAdapter::GetRemotePortByUuid(const RawAddress &addr, const Uuid::UUID128Bit& uuid)
{
    SLE_Addr_S stackAddr = ConvertToStackAddr(addr);
    NLSTK_SsapUuid_S stackUuid = ConvertToStackUuid(Uuid::ConvertFrom128Bits(uuid));
    uint16_t portId = 0;
    NLSTK_Errcode_E ret = NLSTK_PortGetDevicePortIdByUuid(&stackAddr, &stackUuid, &portId);
    NL_CHECK_RETURN_RET(ret == NLSTK_ERRCODE_SUCCESS, portId,
        "[PORT Adapter] cannot find remote portId, ret(%{public}d)", ret);
    HILOGD("[PORT Adapter] successfully find remote portId, ret(%{public}d)", ret);
    return portId;
}

} // namespace Nearlink
} // namespace OHOS