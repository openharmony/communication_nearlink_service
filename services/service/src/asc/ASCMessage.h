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

#ifndef ASC_MESSAGE_H
#define ASC_MESSAGE_H

#include <string>
#include "message.h"
#include "securec.h"
#include "ASCDefines.h"

namespace OHOS {
namespace Nearlink {
class ASCMessage : public utility::Message {
public:
    explicit ASCMessage(int what = 0, int arg1 = 0, void *arg2 = nullptr)
        : utility::Message(what, arg1, arg2) {}

    ASCMessage(const ASCMessage &src) : utility::Message(src.whatM, src.arg1M, src.arg2M),
        dev_(src.dev_), streamType_(src.streamType_), eventType_(src.eventType_), result_(src.result_),
        connHandle_(src.connHandle_), streamTypeBitMap_(src.streamTypeBitMap_), qosmInfo_(src.qosmInfo_),
        properties_(src.properties_), devRole_(src.devRole_), ascBitrate_(src.ascBitrate_), isCalling_(src.isCalling_),
        ascStreamInfo_(src.ascStreamInfo_), subrate_(src.subrate_), subratePara_(src.subratePara_),
        isLeft_(src.isLeft_), availableStreamType_(src.availableStreamType_), frameType_(src.frameType_),
        phyType_(src.phyType_)
    {}

    ~ASCMessage() = default;

    std::string dev_ {""};
    uint32_t streamType_ = 0;
    uint8_t eventType_ = 0;
    uint8_t result_ = 0;
    uint16_t connHandle_ = 0;
    uint64_t streamTypeBitMap_ = 0;
    AscQosmInfo qosmInfo_ {};
    std::vector<AscProp> properties_ {};
    uint8_t devRole_ = 0;
    AscBitrateChange ascBitrate_ {};
    bool isCalling_ = false;
    AscStreamInfo ascStreamInfo_ {};
    uint32_t subrate_ = 0;
    SleAcbSubrateParam subratePara_ {};
    bool isLeft_ = false;
    uint32_t availableStreamType_ = 0;
    uint8_t frameType_ = 0;
    uint8_t phyType_ = 0;
    
    ASCMessage operator=(const ASCMessage &src)
    {
        if (this != &src) {
            dev_ = src.dev_;
            streamType_ = src.streamType_;
            eventType_ = src.eventType_;
            result_ = src.result_;
            connHandle_ = src.connHandle_;
            properties_ = src.properties_;
            streamTypeBitMap_ = src.streamTypeBitMap_;
            qosmInfo_ = src.qosmInfo_;
            devRole_ = src.devRole_;
            ascBitrate_ = src.ascBitrate_;
            isCalling_ = src.isCalling_;
            ascStreamInfo_ = src.ascStreamInfo_;
            subrate_ = src.subrate_;
            subratePara_ = src.subratePara_;
            isLeft_ = src.isLeft_;
            availableStreamType_ = src.availableStreamType_;
            frameType_ = src.frameType_;
            phyType_ = src.phyType_;
        }
        return *this;
    }
}; // ASCMessage
} // namespcae Sle
} // namespace OHOS

#endif // ASC_MESSAGE_H