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

#ifndef ASC_UTILS_H
#define ASC_UTILS_H

#include <cstring>
#include <iostream>
#include "ASCDefines.h"
#include "raw_address.h"
#include "QosM.h"
#include "actm_api_type.h"
#include "log_util.h"
#include "SleInterfaceProfileASC.h"

/*
 * @brief The nearlink system.
 */
namespace OHOS {
namespace Nearlink {
/**
 * @brief ASC Utils.
 */
class ASCUtils {
public:
    static uint8_t GetEncodeSoundChannelNum();
    static uint8_t GetDecodeSoundChannelNum();
    static uint8_t GetCosetDeviceNum(const RawAddress& device);
    static uint8_t GetEncodeCodecClan(uint8_t codecId);
    static uint8_t GetDecodeCodecClan(const RawAddress& device, const ASCToDSPInfo &ascToDspInfo, Qos cos);
    static uint8_t GetCustomizeDownChannelNum(uint8_t pointType, uint8_t enCodecId);
    static uint8_t GetCustomizeUpChannelNum(uint8_t pointType, Qos cos);
    static uint32_t GetDecodeSampleRate(const ASCToDSPInfo &ascToDspInfo, Qos cos);
    static uint8_t GetDecodeBitDepth(const ASCToDSPInfo &ascToDspInfo, Qos cos);
    static uint8_t GetBpsBitIndexByBps(uint16_t bps);
    static void SetQosmInfo(const NLSTK_ActmQosmInfo_S& infoActm, AscQosmInfo& infoAsc);
    static bool IsNeedSetDirection(Qos qos);
    static uint8_t GetDirection(bool isRolePrimary, Qos qos);
    static void TransferProperty(const RawAddress& device, uint8_t num, NLSTK_ActmProp_S *prop,
        std::vector<AscProp>& properties);
    static uint32_t GetAutorateTargetBpsRange(uint16_t targetBps);
    static uint8_t GetAutorateTargetFrameType(bool isLevelUp);
    static uint8_t GetAutorateTargetPhyType(bool isLevelUp);
private:
};
}  // namespace Nearlink
}  // namespace OHOS
#endif // ASC_UTILS_H