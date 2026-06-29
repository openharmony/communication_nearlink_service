/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#ifndef SLE_ASC
#define SLE_ASC

#include <cstdint>
#include "raw_address.h"
#include "actm_api_type.h"

namespace OHOS {
namespace Nearlink {
class SleASC {
public:
    static int GetAudioProperty(const RawAddress& device);
    static int ConfigStream(const RawAddress &device, NLSTK_ActmConfigParam_S &cfgPara);
    static int ConfigStreamCustomize(const RawAddress &device, NLSTK_ActmConfigParam_S &cfgPara);
    static int OpenStream(const RawAddress &device, uint8_t streamId);
    static int StartStream(const RawAddress &device, uint8_t streamId);
    static int StopStream(const RawAddress &device, uint8_t streamId);
    static int ReleaseStream(const RawAddress &device, uint8_t streamId);
    static int SetDirection(const RawAddress &device, uint8_t direction);
    static int Disconnect(const RawAddress &device);
    static int UpdateBitRate(const RawAddress &device, uint64_t bps, uint8_t streamId);
    static int CreateStream(const RawAddress &device, uint8_t pointType, uint8_t commType);
    static int SendVoiceCallAutoRateMsg(const RawAddress &device, NLSTK_ActmAutoRateRecvMsg_S &param);
};
} // namespace Nearlink
} // namespace OHOS

#endif // SLE_ASC
