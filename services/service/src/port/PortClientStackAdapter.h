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
#ifndef PORT_CLIENT_STACK_ADAPTER_H
#define PORT_CLIENT_STACK_ADAPTER_H

#include "raw_address.h"
#include "PortDefines.h"
#include "nlstk_port_client.h"

namespace OHOS {
namespace Nearlink {

class PortClientStackAdapter {
public:
    PortClientStackAdapter();
    ~PortClientStackAdapter();
    int RegisterCallBackToStack();
    int DeregisterCallBackToStack();
    int Connect(const RawAddress &addr, const NLSTK_ConnParam_S &connParam = {});
    int Disconnect(const RawAddress &addr);

    int GetConnectState(const RawAddress &addr, int &state);
    uint16_t GetRemotePortByUuid(const RawAddress &addr, const Uuid::UUID128Bit& uuid);

private:
    static void OnConnectionStateChanged(
        SLE_Addr_S *addr, NLSTK_PortConnectState_E curState, NLSTK_PortConnectState_E preState, NLSTK_Errcode_E ret);
};

} // namespace Nearlink
} // namespace OHOS

#endif // PORT_CLIENT_STACK_ADAPTER_H