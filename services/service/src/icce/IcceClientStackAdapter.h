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
#ifndef ICCE_CLIENT_STACK_ADAPTER_H
#define ICCE_CLIENT_STACK_ADAPTER_H

#include "icce_utils.h"
#include "nearlink_types.h"
#include "nlstk_icce_client.h"
#include "nearlink_def.h"

namespace OHOS {
namespace Nearlink {

class IcceClientStackCallback {
public:
    virtual ~IcceClientStackCallback() = default;
    virtual void OnConnectionStateChanged(const RawAddress &device, int curState, int prevState) {}
};

class IcceClientStackAdapter {
public:
    explicit IcceClientStackAdapter(IcceClientStackCallback &callback);
    ~IcceClientStackAdapter();

    int Connect(const RawAddress &device);
    int Disconnect(const RawAddress &device);
    uint8_t GetConnectionsDeviceNum();
    void OnConnectionStateChangedTask(const RawAddress &device, SleConnectState curState, SleConnectState prevState);
    int RegisterCallBackToStack();
    int32_t GetPort(const RawAddress &device);
private:
    static void OnConnectionStateChange(SLE_Addr_S *sleAddr, NLSTK_IcceConnectState_E curState,
        NLSTK_IcceConnectState_E prevState, NLSTK_Errcode_E errNumb);
    NEARLINK_DECLARE_IMPL();
};

} // namespace Nearlink
} // namespace OHOS
#endif // ICCE_CLIENT_STACK_ADAPTER_H