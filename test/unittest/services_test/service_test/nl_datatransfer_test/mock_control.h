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

#ifndef NEARLINK_DATATRANSFER_MOCK_CONTROL_H
#define NEARLINK_DATATRANSFER_MOCK_CONTROL_H

#include <stdint.h>

namespace OHOS {
namespace Nearlink {

// mock 受控开关：默认值与各 mock 原硬编码值一致，不调用 setter 时既有用例行为不变
void MockSleAdapterSetAcbConnected(bool connected);
uint32_t MockSleAdapterGetConnectAllProfileCount();
uint32_t MockSleAdapterGetDisconnectActionCount();
void MockSleAdapterReset();

void MockIcceServiceSetGetPort(int32_t port);

} // namespace Nearlink
} // namespace OHOS

#ifdef __cplusplus
extern "C" {
#endif

void MockPortClientSetConnectState(int state);
void MockPortClientSetDevicePortId(uint16_t portId);
uint32_t MockPortClientGetDisconnectCount(void);
void MockPortClientReset(void);

#ifdef __cplusplus
}
#endif

#endif // NEARLINK_DATATRANSFER_MOCK_CONTROL_H
