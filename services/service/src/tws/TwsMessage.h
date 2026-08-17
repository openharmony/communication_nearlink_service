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
#ifndef TWS_MESSAGE_H
#define TWS_MESSAGE_H

#include <string>
#include "TwsDefines.h"
#include "message.h"
#include "securec.h"

namespace OHOS {
namespace Nearlink {

/* 私有服务消息 */
class TwsMessage : public utility::Message {
public:
    /* 构造：实例化依赖 */
    explicit TwsMessage(int what = 0, int arg1 = 0, void *arg2 = nullptr)
        : utility::Message(what, arg1, arg2), isNeedRsp_(false), serviceData_(nullptr), dataStream_(nullptr) {}

    /* 构造：非引用传参时依赖 */
    TwsMessage(const TwsMessage &src) : utility::Message(src.whatM, src.arg1M, src.arg2M)
    {
        /* 基类数据拷贝 */
        whatM = src.whatM;
        arg1M = src.arg1M;
        arg2M = src.arg2M;

        dev_ = src.dev_;
        serviceDataLen_ = src.serviceDataLen_;
        streamLen_ = src.streamLen_;
        if (src.serviceDataLen_ != 0) {
            serviceData_ = std::make_unique<uint8_t[]>(serviceDataLen_);
            (void)memcpy_s(serviceData_.get(), serviceDataLen_, src.serviceData_.get(), src.serviceDataLen_);
        }

        if (src.streamLen_ != 0) {
            dataStream_ = std::make_unique<uint8_t[]>(streamLen_);
            (void)memcpy_s(dataStream_.get(), streamLen_, src.dataStream_.get(), src.streamLen_);
        }

        msgDirect_ = src.msgDirect_;
        msgType_ = src.msgType_;
        isNeedRsp_ = src.isNeedRsp_;
    }

    ~TwsMessage() = default;

    /* 重载运算符：= */
    TwsMessage operator=(const TwsMessage &src)
    {
        if (this == &src) {
            return *this;
        }

        /* 基类数据拷贝 */
        whatM = src.whatM;
        arg1M = src.arg1M;
        arg2M = src.arg2M;

        dev_ = src.dev_;
        serviceDataLen_ = src.serviceDataLen_;
        streamLen_ = src.streamLen_;
        msgDirect_ = src.msgDirect_;
        msgType_ = src.msgType_;
        isNeedRsp_ = src.isNeedRsp_;

        /* 服务数据拷贝 */
        if (src.serviceDataLen_ != 0) {
            serviceData_ = std::make_unique<uint8_t[]>(src.serviceDataLen_);
            (void)memcpy_s(serviceData_.get(), serviceDataLen_, src.serviceData_.get(), src.serviceDataLen_);
        }

        /* 码流数据拷贝 */
        if (src.streamLen_ != 0) {
            dataStream_ = std::make_unique<uint8_t[]>(src.streamLen_);
            (void)memcpy_s(dataStream_.get(), streamLen_, src.dataStream_.get(), src.streamLen_);
        }

        return *this;
    }

    /* 私有服务数据交互 */
    std::string dev_ {""};                             /* 对端设备地址 */
    uint8_t msgDirect_ = 0;                            /* 编码库定义的消息方向，请求还是回复 */
    uint8_t msgType_ = 0;                              /* 编解码库定义的消息类型 @ref TwsHiBoxMsgType */
    bool isNeedRsp_ = false;                           /* 是否需要回复消息，true:需要  false:不需要 */
    std::unique_ptr<uint8_t[]> serviceData_ = nullptr; /* 私有服务数据 */
    uint32_t serviceDataLen_ = 0;                      /* 私有服务数据长度 */
    std::unique_ptr<uint8_t[]> dataStream_ = nullptr;  /* 原始HiBox的数据码流 */
    uint32_t streamLen_ = 0;                           /* 原始HiBox的数据长度 */
}; // TwsMessage
} // namespcae Sle
} // namespace OHOS
#endif // TWS_MESSAGE_H
