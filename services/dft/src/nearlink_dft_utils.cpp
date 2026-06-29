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

#include "nearlink_dft_utils.h"

#include <iomanip>
#include <sstream>
#include "log.h"
#include "def.h"
#include "nearlink_utils.h"

#define MILLSEC 1000
#define MILLSEC_LEN 3

namespace OHOS {
namespace Nearlink {

bool DftParam::TypeLessCmp::operator()(const std::shared_ptr<DftParam> &p1, const std::shared_ptr<DftParam> &p2) const
{
    if (!p1 || !p2) {
        return false;
    }
    return p1->TypeLess(*p2);
}

DftParam::ParamTypeSet DftParam::ConvertToParamSet(DftEventEnum eventId,
    const DftParamC params[], size_t size, bool isSubEvent)
{
    ParamTypeSet set;
    if (!params || size == 0 || size > OHOS::HiviewDFX::MAX_PARAM_NUMBER) {
        HILOGE("invalid size=%{public}lu", size);
        return set;
    }
    for (size_t i = 0; i < size; i++) {
        std::shared_ptr<DftParam> p;
        if ((isSubEvent && params[i].t == DFT_SUB_REF) ||
            !IsValidParam(eventId, params[i].paramId, params[i].t) ||
            !(p = ConvertToParam(eventId, params[i]))) {
            HILOGE("skip isSubEvent=%{public}d paramId=%{public}d type=%{public}d eventId=%{public}d",
                isSubEvent, params[i].paramId, params[i].t, eventId);
            continue;
        }
        set.emplace(p);
    }
    return set;
}

DftParam::DftParam(DftEventEnum eventId, const DftParamC &src)
    : eventId_(eventId), paramId_(src.paramId), type_(src.t)
{
    if (!IsValidParam(eventId_, paramId_, type_)) {
        eventId_ = DFT_EVENT_INVALID;
        paramId_ = 0;
        type_ = DFT_PARAM_TYPE_INVALID;
    }
}

bool DftParam::operator==(const DftParam &other) const
{
    return eventId_ == other.eventId_ && paramId_ == other.paramId_ &&
        type_ == other.type_ && SubAllEqual(other);
}

std::shared_ptr<DftParam> DftParam::ConvertToParam(DftEventEnum eventId, const DftParamC &src)
{
    using ConvertToParamFunc = std::shared_ptr<DftParam> (*)(DftEventEnum, const DftParamC &);
    static const ConvertToParamFunc convertFuncs[DFT_TYPE_BUTT] = {
        &ConvertInvalid, &ConvertFromBool, &ConvertFromUi8, &ConvertFromI8, &ConvertFromUi16, &ConvertFromI16,
        &ConvertFromUi32, &ConvertFromI32, &ConvertFromString, &ConvertFromRef
    };
    if (!convertFuncs[src.t]) {
        return nullptr;
    }
    return convertFuncs[src.t](eventId, src);
}

std::shared_ptr<DftParam> DftParam::ConvertInvalid(DftEventEnum eventId, const DftParamC &src)
{
    return nullptr;
}

HiSysEventParam DftParam::ToHiSysParam(void)
{
    HiSysEventParam p = {
        .t = ConvertToHiSysType(type_),
        .v = SubGetHiSysValue(),
        .arraySize = 0
    };
    if (strcpy_s(p.name, MAX_LENGTH_OF_PARAM_NAME, GetParamName(eventId_, paramId_, type_)) != EOK) {
        HILOGE("strcpy_s fail");
    }
    return p;
}

bool DftParam::TypeLess(const DftParam &other) const
{
    if (eventId_ < other.eventId_) {
        return true;
    } else if (eventId_ > other.eventId_) {
        return false;
    } else if (paramId_ < other.paramId_) {
        return true;
    } else if (paramId_ > other.paramId_) {
        return false;
    } else {
        return type_ < other.type_;
    }
}

std::string GetMillTime(bool withDate)
{
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % MILLSEC;
    std::time_t nowC = std::chrono::system_clock::to_time_t(now);
    auto nowTmp = std::localtime(&nowC);
    if (!nowTmp) {
        return "";
    }
    std::tm nowTm = *nowTmp;
    std::stringstream ss;
    if (withDate) {
        ss << std::put_time(&nowTm, "%Y-%m-%d %H:%M:%S");
    } else {
        ss << std::put_time(&nowTm, "%H:%M:%S");
    }
    ss << '.' << std::setfill('0') << std::setw(MILLSEC_LEN) << ms.count();
    return ss.str();
}

int64_t GetTimestamp()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

std::string JsonStringWrap(const std::string &src)
{
    return "\"" + src + "\"";
}

DftParamC CreateBoolParamC(int paramId, bool value)
{
    DftParamC p = {paramId, DFT_BOOL};
    p.u.v.b = value;
    return p;
}

DftParamC CreateUi8ParamC(int paramId, uint8_t value)
{
    DftParamC p = {paramId, DFT_UINT8};
    p.u.v.ui8 = value;
    return p;
}

DftParamC CreateI8ParamC(int paramId, int8_t value)
{
    DftParamC p = {paramId, DFT_INT8};
    p.u.v.i8 = value;
    return p;
}

DftParamC CreateUi16ParamC(int paramId, uint16_t value)
{
    DftParamC p = {paramId, DFT_UINT16};
    p.u.v.ui16 = value;
    return p;
}

DftParamC CreateI16ParamC(int paramId, int16_t value)
{
    DftParamC p = {paramId, DFT_INT16};
    p.u.v.i16 = value;
    return p;
}

DftParamC CreateUi32ParamC(int paramId, uint32_t value)
{
    DftParamC p = {paramId, DFT_UINT32};
    p.u.v.ui32 = value;
    return p;
}

DftParamC CreateI32ParamC(int paramId, int32_t value)
{
    DftParamC p = {paramId, DFT_INT32};
    p.u.v.i32 = value;
    return p;
}

DftParamC CreateStrParamC(int paramId, const std::string &str)
{
    DftParamC p = {paramId, DFT_STRING};
    p.u.v.s = const_cast<char*>(str.c_str());
    return p;
}

DftParamC CreateRefParamC(int paramId, DftSubEventRefC &ref)
{
    DftParamC p = {paramId, DFT_SUB_REF};
    p.u.ref = &ref;
    return p;
}

// dft refactor below
std::string GetLowercaseAddr(const std::string& peerAddr)
{
    std::string addr(peerAddr);
    std::transform(addr.begin(), addr.end(), addr.begin(), ::tolower);
    return addr;
}
namespace {
constexpr int addr5Pos = 15;
constexpr int addr4Pos = 12;
constexpr int addr0Pos = 0;
constexpr int addrTypePos = 17;
constexpr int byteLen = 2;
constexpr int typeLen = 3;
}

bool IsValidAddressWithType(const std::string &addr)
{
    if (addr.empty() || addr.length() != ADDRESS_LENGTH_WITH_TYPE) {
        return false;
    }
    for (size_t i = 0; i < ADDRESS_LENGTH; i++) {
        char c = addr[i];
        switch (i % ADDRESS_SEPARATOR_UNIT) {
            case 0:
            case 1:
                if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
                    break;
                }
                return false;
            case ADDRESS_COLON_INDEX:
            default:
                if (c == ':') {
                    break;
                }
                return false;
        }
    }
    return true;
}

bool IsValidAddressWithSuffix(const std::string &addr)
{
    if (addr.empty()) {
        return false;
    }
    for (size_t i = 0; i < ADDRESS_LENGTH; i++) {
        char c = addr[i];
        switch (i % ADDRESS_SEPARATOR_UNIT) {
            case 0:
            case 1:
                if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
                    break;
                }
                return false;
            case ADDRESS_COLON_INDEX:
            default:
                if (c == ':') {
                    break;
                }
                return false;
        }
    }
    return true;
}

std::string GetEncryptAddrWithType(const std::string &addr)
{
    if (addr.empty() || addr.length() != ADDRESS_LENGTH_WITH_TYPE) {
        LOG_INFO("addr is invalid.");
        return std::string("");
    }
    std::string addrMask = ":**:**:**:";
    std::ostringstream out;
    // 将小端序的addr字符串改为05:04:**:**:**:00大端序LOG字符串输出，addr保持小端序不变
    out << addr.substr(addr5Pos, byteLen) << ":" << addr.substr(addr4Pos, byteLen) << addrMask
        << addr.substr(addr0Pos, byteLen) << addr.substr(addrTypePos, typeLen);
    return out.str();
}

}  // namespace Nearlink
}  // namespace OHOS