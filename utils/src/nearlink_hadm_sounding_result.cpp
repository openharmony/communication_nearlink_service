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

#include "nearlink_hadm_sounding_result.h"
#include "log.h"
#include "securec.h"

namespace OHOS {
namespace Nearlink {

NearlinkHadmSoundingResult::NearlinkHadmSoundingResult(const NearlinkHadmSoundingResult &other)
{
    addr_ = RawAddress(other.addr_.GetAddress());
    dutRssi_ = other.dutRssi_;
    rtdRssi_ = other.rtdRssi_;
    dutIData_ = other.dutIData_;
    rtdIData_ = other.rtdIData_;
    dutQData_ = other.dutQData_;
    rtdQData_ = other.rtdQData_;
    dutTof_ = other.dutTof_;
    rtdTof_ = other.rtdTof_;
    timeStampSn_ = other.timeStampSn_;
    isMultiTone_ = other.isMultiTone_;
    dutIqBitLen_ = other.dutIqBitLen_;
    rtdIqBitLen_ = other.rtdIqBitLen_;
    (void)memcpy_s(dutSlemChmap_, HADM_CHMAP_BYTE_LEN, other.dutSlemChmap_, HADM_CHMAP_BYTE_LEN);
    (void)memcpy_s(rtdSlemChmap_, HADM_CHMAP_BYTE_LEN, other.rtdSlemChmap_, HADM_CHMAP_BYTE_LEN);
    localNvOffset_ = other.localNvOffset_;
    remoteNvOffset_ = other.remoteNvOffset_;
    localTofOffset_ = other.localTofOffset_;
    remoteTofOffset_ = other.remoteTofOffset_;
}

NearlinkHadmSoundingResult& NearlinkHadmSoundingResult::operator=(const NearlinkHadmSoundingResult& other)
{
    if (this != &other) {
        addr_ = RawAddress(other.addr_.GetAddress());
        dutRssi_ = other.dutRssi_;
        rtdRssi_ = other.rtdRssi_;
        dutIData_ = other.dutIData_;
        rtdIData_ = other.rtdIData_;
        dutQData_ = other.dutQData_;
        rtdQData_ = other.rtdQData_;
        dutTof_ = other.dutTof_;
        rtdTof_ = other.rtdTof_;
        timeStampSn_ = other.timeStampSn_;
        isMultiTone_ = other.isMultiTone_;
        dutIqBitLen_ = other.dutIqBitLen_;
        rtdIqBitLen_ = other.rtdIqBitLen_;
        (void)memcpy_s(dutSlemChmap_, HADM_CHMAP_BYTE_LEN, other.dutSlemChmap_, HADM_CHMAP_BYTE_LEN);
        (void)memcpy_s(rtdSlemChmap_, HADM_CHMAP_BYTE_LEN, other.rtdSlemChmap_, HADM_CHMAP_BYTE_LEN);
        localNvOffset_ = other.localNvOffset_;
        remoteNvOffset_ = other.remoteNvOffset_;
        localTofOffset_ = other.localTofOffset_;
        remoteTofOffset_ = other.remoteTofOffset_;
    }
    return *this;
}

void NearlinkHadmSoundingResult::AppendDutIData(std::vector<uint16_t> iqData)
{
    std::lock_guard<std::mutex> lock(mutex_);
    dutIData_.insert(dutIData_.end(), iqData.begin(), iqData.end());
}

void NearlinkHadmSoundingResult::AppendRtdIData(std::vector<uint16_t> iqData)
{
    std::lock_guard<std::mutex> lock(mutex_);
    rtdIData_.insert(rtdIData_.end(), iqData.begin(), iqData.end());
}

void NearlinkHadmSoundingResult::AppendDutQData(std::vector<uint16_t> iqData)
{
    std::lock_guard<std::mutex> lock(mutex_);
    dutQData_.insert(dutQData_.end(), iqData.begin(), iqData.end());
}

void NearlinkHadmSoundingResult::AppendRtdQData(std::vector<uint16_t> iqData)
{
    std::lock_guard<std::mutex> lock(mutex_);
    rtdQData_.insert(rtdQData_.end(), iqData.begin(), iqData.end());
}

const std::vector<uint16_t> NearlinkHadmSoundingResult::GetDutIData()
{
    return dutIData_;
}

const std::vector<uint16_t> NearlinkHadmSoundingResult::GetRtdIData()
{
    return rtdIData_;
}

const std::vector<uint16_t> NearlinkHadmSoundingResult::GetDutQData()
{
    return dutQData_;
}

const std::vector<uint16_t> NearlinkHadmSoundingResult::GetRtdQData()
{
    return rtdQData_;
}

uint16_t NearlinkHadmSoundingResult::GetDutTof()
{
    return dutTof_;
}

uint16_t NearlinkHadmSoundingResult::GetRtdTof()
{
    return rtdTof_;
}

uint32_t NearlinkHadmSoundingResult::GetTimeStampSn()
{
    return timeStampSn_;
}

bool NearlinkHadmSoundingResult::IsCompleteData()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!(addr_ == RawAddress(INVALID_MAC_ADDRESS)) && dutRssi_ != 0 && rtdRssi_ != 0 &&
        dutIData_.size() > 0 && rtdIData_.size() > 0 &&
        dutQData_.size() > 0 && rtdQData_.size() > 0 &&
        dutIData_.size() == rtdIData_.size() &&
        dutQData_.size() == rtdQData_.size() &&
        dutIData_.size() == dutQData_.size()) {
        return true;
    }
    return false;
}

void NearlinkHadmSoundingResult::Reset()
{
    std::lock_guard<std::mutex> lock(mutex_);
    addr_ = RawAddress(INVALID_MAC_ADDRESS);
    dutRssi_ = 0;
    rtdRssi_ = 0;
    dutIData_.clear();
    rtdIData_.clear();
    dutQData_.clear();
    rtdQData_.clear();
    dutTof_ = 0;
    rtdTof_ = 0;
    timeStampSn_ = 0;
    isMultiTone_ = 0;
    dutIqBitLen_ = 12; // 默认位宽12比特
    rtdIqBitLen_ = 12; // 默认位宽12比特
    (void)memset_s(dutSlemChmap_, sizeof(dutSlemChmap_), 0, sizeof(dutSlemChmap_));
    (void)memset_s(rtdSlemChmap_, sizeof(rtdSlemChmap_), 0, sizeof(rtdSlemChmap_));
    localNvOffset_ = 0;
    remoteNvOffset_ = 0;
    localTofOffset_ = 0;
    remoteTofOffset_ = 0;
}
}  // namespace Nearlink
}  // namespace OHOS
