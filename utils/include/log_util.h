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
#ifndef LOG_UTIL_H
#define LOG_UTIL_H

#include <string>
#include <sstream>
#include "log.h"

namespace OHOS {
namespace Nearlink {

std::string GetStateString(int state);
std::string GetTransportString(int transport);
std::string GetReasonString(int stateChangeReason);
std::string GetConnStateString(int state);
std::string GetEncryptAddr(const std::string &addr);
std::string GetEncryptAddrWithType(const std::string &addr);

#define GET_ENCRYPT_ADDR(rawAddr) (GetEncryptAddr((rawAddr).GetAddress()).c_str())
#define GET_ENCRYPT_DEVICE_ADDR(device) (GetEncryptAddr((device).GetDeviceAddr()).c_str())
#define GET_ENCRYPT_SSAP_ADDR(device) (GetEncryptAddr((device).addr_.GetAddress()).c_str())
#define GET_ENCRYPT_UUID(uuid) ((uuid).GetEncryptUuid().c_str())

}  // namespace Nearlink
}  // namespace OHOS

#endif // LOG_UTIL_H