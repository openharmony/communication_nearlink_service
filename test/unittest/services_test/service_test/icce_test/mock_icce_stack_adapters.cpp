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

// 测试目的：本文件提供 ICCE 协议栈适配层的最小成功实现（替代真实 NLSTK 栈调用），
// 使 IcceService 单元测试无需固件协议栈即可运行；计数器供用例断言"服务确实调到了适配层"。
#include "IcceClientStackAdapter.h"
#include "IcceServerStackAdapter.h"
#include "IcceDefines.h"

namespace OHOS {
namespace Nearlink {
namespace {
int connectCalls = 0;
int disconnectCalls = 0;
int registerCallBackCalls = 0;
int createCalls = 0;
int destroyCalls = 0;
}
namespace TEST {
void ResetIcceStackMock()
{
    connectCalls = 0;
    disconnectCalls = 0;
    registerCallBackCalls = 0;
    createCalls = 0;
    destroyCalls = 0;
}
int GetIcceStackConnectCalls() { return connectCalls; }
int GetIcceStackDisconnectCalls() { return disconnectCalls; }
int GetIcceRegisterCallBackCalls() { return registerCallBackCalls; }
int GetIcceServerCreateCalls() { return createCalls; }
int GetIcceServerDestroyCalls() { return destroyCalls; }
}

struct IcceClientStackAdapter::impl {
    impl(IcceClientStackCallback &callback) : icceClientStackCbk_(callback) {}
    IcceClientStackCallback &icceClientStackCbk_;
};

IcceClientStackAdapter::IcceClientStackAdapter(IcceClientStackCallback &callback)
    : pimpl(std::make_unique<impl>(callback))
{
}

IcceClientStackAdapter::~IcceClientStackAdapter() {}

int IcceClientStackAdapter::Connect(const RawAddress &device)
{
    ++connectCalls;
    return ICCE_SUCCESS;
}

int IcceClientStackAdapter::Disconnect(const RawAddress &device)
{
    ++disconnectCalls;
    return ICCE_SUCCESS;
}

uint8_t IcceClientStackAdapter::GetConnectionsDeviceNum()
{
    return 0;
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
    // 测试目的：mock 栈不主动回调；状态回调路径由 OnConnectionStateChangedTask 直测覆盖。
}

int IcceClientStackAdapter::RegisterCallBackToStack()
{
    ++registerCallBackCalls;
    return ICCE_SUCCESS;
}

int32_t IcceClientStackAdapter::GetPort(const RawAddress &device)
{
    return 1;
}

int IcceServerStackAdapter::CreateServerInfo()
{
    ++createCalls;
    return ICCE_SUCCESS;
}

int IcceServerStackAdapter::DestroyServerInfo()
{
    ++destroyCalls;
    return ICCE_SUCCESS;
}
} // namespace Nearlink
} // namespace OHOS
