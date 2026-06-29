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
#ifndef HID_HOST_DEFINES_H
#define HID_HOST_DEFINES_H

#include <cstdint>
#include <string>
#include "ssap_data.h"
#include "securec.h"

namespace OHOS {
namespace Nearlink {
constexpr int HID_HOST_SUCCESS = 0;
constexpr int HID_HOST_FAILURE = 1;

constexpr int HID_HOST_RESERVED_REPORT = 0;
constexpr int HID_HOST_INPUT_REPORT = 1;
constexpr int HID_HOST_OUTPUT_REPORT = 2;
constexpr int HID_HOST_FEATURE_REPORT = 3;

struct HidReportInfo {
    HidReportInfo() = default;
    ~HidReportInfo() = default;
    HidReportInfo(const HidReportInfo& src) : dev_(src.dev_),
        reportType_(src.reportType_), reportId_(src.reportId_),
        data_(nullptr), dataLength_(src.dataLength_)
    {
        if ((dataLength_ > 0) && (src.data_ != nullptr)) {
            data_ = std::make_unique<uint8_t[]>(dataLength_);
            if (memcpy_s(data_.get(), dataLength_, src.data_.get(), dataLength_) != EOK) {
                data_.reset(nullptr);
                dataLength_ = 0;
            }
        } else {
            data_.reset(nullptr);
            dataLength_ = 0;
        }
    }
    HidReportInfo operator=(const HidReportInfo &src)
    {
        if (this != &src) {
            dev_ = src.dev_;
            reportType_ = src.reportType_;
            reportId_ = src.reportId_;
            dataLength_ = src.dataLength_;
 
            if ((dataLength_ > 0) && (src.data_ != nullptr)) {
                data_ = std::make_unique<uint8_t[]>(dataLength_);
                if (memcpy_s(data_.get(), dataLength_, src.data_.get(), dataLength_) != EOK) {
                    data_.reset(nullptr);
                    dataLength_ = 0;
                }
            } else {
                data_.reset(nullptr);
                dataLength_ = 0;
            }
        }
        return *this;
    }
    RawAddress dev_ {""};
    uint8_t reportType_ = 0;
    uint8_t reportId_ = 0;
    std::unique_ptr<uint8_t[]> data_ = nullptr;
    uint16_t dataLength_ = 0;
};

constexpr uint16_t HID_HOST_INVALID = 0xffff;

struct HidInformation {
    uint16_t attrMask = 0;
    uint16_t supTimeout = 0;
    uint16_t ssrMaxLatency = HID_HOST_INVALID;
    uint16_t ssrMinTout = HID_HOST_INVALID;
    uint16_t relNum = 0;
    uint8_t ctryCode = 0;
    uint8_t subClass = 0;
    uint16_t hparsVer = 0;
    std::string serviceName = "";
    std::string serviceDescription = "";
    std::string providerName = "";
    uint16_t descLength = 0;
    std::unique_ptr<uint8_t[]> descInfo = nullptr;
};

/* HID data types
*/
constexpr uint8_t HID_HOST_DATA_TYPE_HANDSHAKE = 0;
constexpr uint8_t HID_HOST_DATA_TYPE_CONTROL = 1;
constexpr uint8_t HID_HOST_DATA_TYPE_GET_REPORT = 4;
constexpr uint8_t HID_HOST_DATA_TYPE_SET_REPORT = 5;
constexpr uint8_t HID_HOST_DATA_TYPE_GET_PROTOCOL = 6;
constexpr uint8_t HID_HOST_DATA_TYPE_SET_PROTOCOL = 7;
constexpr uint8_t HID_HOST_DATA_TYPE_GET_IDLE = 8;
constexpr uint8_t HID_HOST_DATA_TYPE_SET_IDLE = 9;
constexpr uint8_t HID_HOST_DATA_TYPE_DATA = 10;
constexpr uint8_t HID_HOST_DATA_TYPE_DATAC = 11;

constexpr uint8_t HID_HOST_HANDSHAKE_ERROR = 5;

/* HID shift operation */
constexpr uint8_t HID_HOST_SHIFT_OPRATURN_4 = 4;
constexpr uint8_t HID_HOST_SHIFT_OPRATURN_8 = 8;

const char UHID_DEVICE_PATH[] = { "/dev/uhid" };
} // namespace Sle
} // namespace OHOS
#endif // HID_HOST_DEFINES_H

