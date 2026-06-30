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

#ifndef SSAP_DEF_H
#define SSAP_DEF_H

namespace OHOS {
namespace Nearlink {

// entry handle defines
constexpr uint16_t MAX_ENTRY_HANDLE = 0xFFFF;
constexpr uint16_t MAX_RESRV_HANDLE = 0x000F;
constexpr uint16_t MIN_ENTRY_HANDLE = 0x0001;
constexpr uint16_t INVALID_ENTRY_HANDLE = 0x0000;

// entry descriptor type init value
constexpr uint8_t INVALID_ENTRY_TYPE = 0xFF;

/**
 *  SSAP NOTIFY OPTION
 */
enum class NotifyOption : uint8_t {
    SSAP_SET_NOTIFY = 0,
    SSAP_SET_INDICATE,
};

/*
 * ssap desc write ind
 */
enum SsapDescWriteInd : uint8_t {
    SSAP_DESC_WRITE_IND_NONE,
    SSAP_DESC_WRITE_IND_OK,
    SSAP_DESC_WRITE_IND_NO
};
} // namespace Nearlink
} // namespace OHOS
#endif // SSAP_DEF_H
