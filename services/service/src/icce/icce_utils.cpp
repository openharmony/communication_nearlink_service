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
#include <securec.h>
#include <cstring>
#include <string>
#include "icce_utils.h"
#include "SleRemoteDeviceAdapter.h"

namespace OHOS {
namespace Nearlink {

SLE_Addr_S ConvertToStackAddr(const RawAddress &addr)
{
    SLE_Addr_S sleAddr;
    (void)memset_s(&sleAddr, sizeof(SLE_Addr_S), 0, sizeof(SLE_Addr_S));
    addr.ConvertToUint8(sleAddr.addr);
    sleAddr.type = SleRemoteDeviceAdapter::GetInstance()->GetPeerDeviceAddrType(addr);
    return sleAddr;
}

RawAddress ConvertSleAddrToRawAddress(SLE_Addr_S *addr)
{
    return RawAddress::ConvertToString(addr->addr);
}

} // namespace Nearlink
} // namespace OHOS
