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
#ifndef CDSM_MESSAGE_H
#define CDSM_MESSAGE_H

#include <string>
#include "CdsmDefines.h"
#include "message.h"
#include "securec.h"

namespace OHOS {
namespace Nearlink {

/* 合作集服务消息 */
class CdsmMessage : public utility::Message {
public:
    /* 构造：实例化依赖 */
    explicit CdsmMessage(int what = 0, int arg1 = 0, void *arg2 = nullptr)
        : utility::Message(what, arg1, arg2) {}

    ~CdsmMessage() = default;

    /* 构造：非引用传参时依赖 */
    CdsmMessage(const CdsmMessage &src) : utility::Message(src.whatM, src.arg1M, src.arg2M)
    {
        /* 基类数据拷贝 */
        whatM = src.whatM;
        arg1M = src.arg1M;
        arg2M = src.arg2M;

        dev_ = src.dev_;
        cdsmGrpId_ = src.cdsmGrpId_;
    }

    /* 重载运算符：= */
    CdsmMessage operator=(const CdsmMessage &src)
    {
        /* 基类数据拷贝 */
        whatM = src.whatM;
        arg1M = src.arg1M;
        arg2M = src.arg2M;

        /* 子类消息数据拷贝 */
        dev_ = src.dev_;
        cdsmGrpId_ = src.cdsmGrpId_;
        return *this;
    }

    std::string dev_ {""};  /* 状态消息对应的设备地址 */
    uint32_t cdsmGrpId_ = CDSM_SERVICE_INVALID_GROUP_ID;    /* 设备所在合作集ID */
}; // CdsmMessage
} // namespace Sle
} // namespace OHOS
#endif // CDSM_MESSAGE_H
