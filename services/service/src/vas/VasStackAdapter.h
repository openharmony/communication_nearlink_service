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
#ifndef VAS_STACK_ADAPTER_H
#define VAS_STACK_ADAPTER_H

#include <cstdint>
#include "raw_address.h"
#include "nlstk_ccp_vas_server.h"

namespace OHOS {
namespace Nearlink {
class VasStackAdapter {
public:
    explicit VasStackAdapter();
    ~VasStackAdapter();

    /* 语音助手 */
    void CreateVasInstance();
    void DeleteVasInstance();
    void SetVoiceAssistantOpened();
    void SetVoiceAssistantClosed();
    void OpenVoiceAssistantSuccess(uint32_t requestId);
    void OpenVoiceAssistantFail(uint32_t requestId);
    void CloseVoiceAssistantSuccess(uint32_t requestId);
    void CloseVoiceAssistantFail(uint32_t requestId);
private:
    void VasControlResult(uint32_t requestId, uint8_t opCode, uint8_t errorCode);
    void UpdateVoiceAssistantState(NLSTK_CcpVasState_E state);
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // VAS_STACK_ADAPTER_H
