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
#ifndef BAS_CLIENT_STACK_ADAPTER_H
#define BAS_CLIENT_STACK_ADAPTER_H

#include "raw_address.h"
#include "nlstk_bas_client.h"
#include "nlstk_bas_def.h"
#include "nearlink_types.h"
#include "bas_def.h"
namespace OHOS {
namespace Nearlink {
class BasClientStackAdapter {
public:
    explicit BasClientStackAdapter();
    ~BasClientStackAdapter();
    int RegisterCallBackToStack();
    int Connect(const RawAddress &addr);
    int Disconnect(const RawAddress &addr);
    void GetDeviceBatteryLevel(const RawAddress &addr);
private:
    static void OnBasConnectStateChangeCbk(
        SLE_Addr_S *addr, NLSTK_BasConnectState_E curState, NLSTK_BasConnectState_E preState, NLSTK_Errcode_E ret);
    static void OnBasGetPropertyCbk(SLE_Addr_S *addr, BasPropertyType_E type, void *value, NLSTK_Errcode_E ret);
    static void OnBasPropertyChangedCbk(SLE_Addr_S *addr, BasPropertyType_E type, void *value);
    void OnGetDeviceBatteryLevelTask(const RawAddress &addr, int8_t batteryLevel, int ret);
};

} // namespace Nearlink
} // namespace OHOS

#endif // BAS_CLIENT_STACK_ADAPTER_H