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

#ifndef NEARLINK_DFT_UTILS_H
#define NEARLINK_DFT_UTILS_H

#include <unordered_map>
#include <list>
#include <set>
#include <memory>
#include "nearlink_dft_manager_c.h"
#include "log_util.h"
#include "raw_address.h"
#include <string>

#define DEFAULT_CACHE_CAPACITY 10
namespace OHOS {
namespace Nearlink {

class DftParam {
public:
    struct TypeLessCmp {
        bool operator()(const std::shared_ptr<DftParam> &p1, const std::shared_ptr<DftParam> &p2) const;
    };

    using ParamTypeSet = std::set<std::shared_ptr<DftParam>, TypeLessCmp>;

    static ParamTypeSet ConvertToParamSet(DftEventEnum eventId,
        const DftParamC params[], size_t size, bool isSubEvent = false);

    DftParam(DftEventEnum eventId, const DftParamC &src);
    virtual ~DftParam() = default;

    bool operator==(const DftParam &other) const;

    inline DftEventEnum GetEventId(void)
    {
        return eventId_;
    }

    inline int GetParamId(void)
    {
        return paramId_;
    }

    inline DftParamType GetType(void)
    {
        return type_;
    }

    size_t Hash(void) const
    {
        return std::hash<int>()(eventId_) ^ std::hash<int>()(paramId_) ^ std::hash<int>()(type_) ^ SubHash();
    }

    virtual std::string ToJsonString(void) const = 0;

protected:
    DftEventEnum eventId_;
    int paramId_;
    DftParamType type_;

private:
    static std::shared_ptr<DftParam> ConvertToParam(DftEventEnum eventId, const DftParamC &src);
    static std::shared_ptr<DftParam> ConvertInvalid(DftEventEnum eventId, const DftParamC &src);
    static std::shared_ptr<DftParam> ConvertFromBool(DftEventEnum eventId, const DftParamC &src);
    static std::shared_ptr<DftParam> ConvertFromUi8(DftEventEnum eventId, const DftParamC &src);
    static std::shared_ptr<DftParam> ConvertFromI8(DftEventEnum eventId, const DftParamC &src);
    static std::shared_ptr<DftParam> ConvertFromUi16(DftEventEnum eventId, const DftParamC &src);
    static std::shared_ptr<DftParam> ConvertFromI16(DftEventEnum eventId, const DftParamC &src);
    static std::shared_ptr<DftParam> ConvertFromUi32(DftEventEnum eventId, const DftParamC &src);
    static std::shared_ptr<DftParam> ConvertFromI32(DftEventEnum eventId, const DftParamC &src);
    static std::shared_ptr<DftParam> ConvertFromString(DftEventEnum eventId, const DftParamC &src);
    static std::shared_ptr<DftParam> ConvertFromRef(DftEventEnum eventId, const DftParamC &src);

    virtual bool SubAllEqual(const DftParam &other) const = 0;
    virtual size_t SubHash(void) const = 0;
    virtual HiSysEventParamValue SubGetHiSysValue(void) = 0;

    HiSysEventParam ToHiSysParam(void);
    bool TypeLess(const DftParam &other) const;
    friend class InterfaceDftManager;
};

template<typename K, typename V, size_t cap = DEFAULT_CACHE_CAPACITY>
class LruList {
public:
    explicit LruList(std::unordered_map<K, V> &out) : outMap_(out) {}

    void Put(K &key)
    {
        if (map_.count(key) > 0) {
            Modify(key);
            return;
        }
        if (list_.size() == cap) {
            outMap_.erase(list_.back());
            map_.erase(list_.back());
            list_.pop_back();
        }
        list_.push_front(key);
        map_[key] = list_.cbegin();
    }

    void Erase(K &key)
    {
        if (!map_.count(key)) {
            return;
        }
        list_.erase(map_[key]);
        map_.erase(key);
    }

private:
    void Modify(K &key)
    {
        list_.splice(list_.begin(), list_, map_[key]);
        map_[key] = list_.cbegin();
    }

    std::unordered_map<K, V> &outMap_;
    std::unordered_map<K, typename std::list<K>::const_iterator> map_;
    std::list<K> list_;
};

std::string GetMillTime(bool withDate = false);

int64_t GetTimestamp();

std::string JsonStringWrap(const std::string &src);

DftParamC CreateBoolParamC(int paramId, bool value);
DftParamC CreateUi8ParamC(int paramId, uint8_t value);
DftParamC CreateI8ParamC(int paramId, int8_t value);

DftParamC CreateUi16ParamC(int paramId, uint16_t value);
DftParamC CreateI16ParamC(int paramId, int16_t value);

DftParamC CreateUi32ParamC(int paramId, uint32_t value);
DftParamC CreateI32ParamC(int paramId, int32_t value);

DftParamC CreateStrParamC(int paramId, const std::string &str);

DftParamC CreateRefParamC(int paramId, DftSubEventRefC &ref);

std::string GetLowercaseAddr(const std::string& peerAddr);
std::string GetEncryptEventKey(std::string eventKey);

constexpr size_t ADDRESS_LENGTH_WITH_TYPE = 20;

bool IsValidAddressWithType(const std::string &addr);
bool IsValidAddressWithSuffix(const std::string &addr);
std::string GetEncryptAddrWithType(const std::string &addr);

}  // namespace Nearlink
}  // namespace OHOS

#endif // NEARLINK_DFT_UTILS_H