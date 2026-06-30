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
#ifndef PORT_SERVER_STACK_ADAPTER_H
#define PORT_SERVER_STACK_ADAPTER_H

#include "sle_uuid.h"

namespace OHOS {
namespace Nearlink {

class PortServerStackAdapter {
public:
    PortServerStackAdapter();
    ~PortServerStackAdapter();
    void AddPortByUuid(const Uuid::UUID128Bit& uuid, const uint16_t manufactureId, const uint16_t portId);
    void DeletePortByUuid(const Uuid::UUID128Bit& uuid, const uint16_t portId);
    uint16_t LoadManufactureInfo() const;
};

} // Nearlink
} // OHOS
#endif // PORT_SERVER_STACK_ADAPTER_H