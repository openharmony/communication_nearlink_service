/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef SOUNDING_RESULT_H
#define SOUNDING_RESULT_H

#include <mutex>
#include "cstdint"
#include "raw_address.h"
#include "nearlink_def_types.h"

namespace OHOS {
namespace Nearlink {
// SLEM channel map: 80 bits (2.4G band channel mapping), 10 bytes
const uint8_t HADM_CHMAP_BYTE_LEN = 10;

class NearlinkHadmSoundingResult {
public:
    NearlinkHadmSoundingResult() = default;
    ~NearlinkHadmSoundingResult() = default;
    NearlinkHadmSoundingResult(const NearlinkHadmSoundingResult &other);
    NearlinkHadmSoundingResult& operator=(const NearlinkHadmSoundingResult& other);
    void AppendDutIData(std::vector<uint16_t> iqData);
    void AppendRtdIData(std::vector<uint16_t> iqData);
    void AppendDutQData(std::vector<uint16_t> iqData);
    void AppendRtdQData(std::vector<uint16_t> iqData);
    const std::vector<uint16_t> GetDutIData();
    const std::vector<uint16_t> GetRtdIData();
    const std::vector<uint16_t> GetDutQData();
    const std::vector<uint16_t> GetRtdQData();
    uint16_t GetDutTof();
    uint16_t GetRtdTof();
    uint32_t GetTimeStampSn();
    bool IsCompleteData();
    void Reset();

    RawAddress addr_ = RawAddress(INVALID_MAC_ADDRESS);
    // dut: Device initiating the sounding
    uint8_t dutRssi_ = 0;
    // rtd: Device to be sounding
    uint8_t rtdRssi_ = 0;
    uint16_t dutTof_ = 0;
    uint16_t rtdTof_ = 0;
    uint32_t timeStampSn_ = 0;
    uint8_t isMultiTone_ = 0;
    uint8_t dutIqBitLen_ = 12; // 默认位宽12比特
    uint8_t rtdIqBitLen_ = 12; // 默认位宽12比特
    uint8_t dutSlemChmap_[HADM_CHMAP_BYTE_LEN] = {0};
    uint8_t rtdSlemChmap_[HADM_CHMAP_BYTE_LEN] = {0};
    uint16_t localNvOffset_ = 0;
    uint16_t remoteNvOffset_ = 0;
    uint16_t localTofOffset_ = 0;
    uint16_t remoteTofOffset_ = 0;
    
protected:
    std::vector<uint16_t> dutIData_;
    std::vector<uint16_t> rtdIData_;
    std::vector<uint16_t> dutQData_;
    std::vector<uint16_t> rtdQData_;

private:
    std::mutex mutex_ {};
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  /// SOUNDING_RESULT_H