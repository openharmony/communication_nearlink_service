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
#ifndef MIC_CLIENT_STACK_ADAPTER_H
#define MIC_CLIENT_STACK_ADAPTER_H

#include "raw_address.h"
#include "sdf_addr.h"
#include "nlstk_micp_client.h"

namespace OHOS {
namespace Nearlink {
enum MicServiceErrorCode {
    MIC_SUCCESS = 0,
    MIC_FAILURE = 1,
    MIC_INVALID = -1,
};

enum MicState : uint8_t {
    MIC_OFF = 0x00,
    MIC_ON = 0x01,
    MIC_UNKNOWN = 0xFF,
};

class MicClientStackAdapter {
public:
    MicClientStackAdapter();
    ~MicClientStackAdapter();
    int RegisterCallBackToStack();
    int Connect(const RawAddress &addr);
    int Disconnect(const RawAddress &addr);
private:
    static void OnMicpConnectEventCbk(
        SLE_Addr_S *addr, NLSTK_MicpConnectState_E curState, NLSTK_MicpConnectState_E preState, uint8_t errorCode);
    static void OnMicpMicStateCbk(SLE_Addr_S *addr, uint8_t micState);
};
}
}

#endif // MIC_CLIENT_STACK_ADAPTER_H
