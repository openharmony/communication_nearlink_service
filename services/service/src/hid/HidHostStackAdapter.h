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
#ifndef HID_HOST_STACK_ADAPTER_H
#define HID_HOST_STACK_ADAPTER_H

#include "HidHostDefines.h"
#include "hid_def.h"
#include <list>

namespace OHOS {
namespace Nearlink {

class HidHostStackAdapter {
public:
    HidHostStackAdapter() = default;
    ~HidHostStackAdapter() = default;
    int RegisterCallbackToStack();
    int Connect(const RawAddress &addr);
    int Disconnect(const RawAddress &addr);
    HidInformation GetRemoteHidInfo(const RawAddress &addr);
    int GetDeviceState(const RawAddress &addr);
    std::list<RawAddress> GetConnectDevices();
    int SendReport(const HidReportInfo &reportInfo);
    int SendGetReport(const HidReportInfo &reportInfo);

private:
    static void HidConnectStateChangeCbk(SLE_Addr_S *addr, HidConnectState_E state, HidConnectState_E preState,
        NLSTK_Errcode_E ret);
    static void HidNotifyPropertyCbk(SLE_Addr_S *addr, HidPropertyType_E type, void *value);
    static void HidReadPropertyCbk(SLE_Addr_S *addr, HidPropertyType_E type, void *value, NLSTK_Errcode_E ret);
    static void HidWritePropertyCbk(SLE_Addr_S *addr, HidPropertyType_E type, NLSTK_Errcode_E ret);
};
} // namespace Nearlink
} // namespace OHOS
#endif // HID_HOST_STACK_ADAPTER_H