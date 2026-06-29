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

#include "nearlink_dft_manager.h"

#include <shared_mutex>
#include <string>
#include "ffrt.h"
#include "securec.h"
#include "log.h"
#include "nearlink_utils.h"
#include "nearlink_dft_car_key_loader.h"

namespace OHOS {
namespace Nearlink {

class DftEvent {
public:
    static bool ParamSetEqual(const DftParam::ParamTypeSet &l, const DftParam::ParamTypeSet &r)
    {
        if (l.size() != r.size()) {
            return false;
        }
        auto lit = l.begin();
        auto rit = r.begin();
        while (lit != l.end() && rit != r.end()) {
            if (!(*(*lit) == *(*rit))) {
                return false;
            }
            ++lit;
            ++rit;
        }
        return true;
    }

    DftEvent(DftEventEnum eventId, const DftParam::ParamTypeSet &params) : eventId_(eventId)
    {
        std::for_each(params.begin(), params.end(), [this](auto &sp) {
            if (sp && IsKeyParam(eventId_, sp->GetParamId())) {
                params_.emplace(sp);
            }
        });
    }

    bool operator==(const DftEvent &other) const
    {
        return eventId_ == other.eventId_ && ParamSetEqual(params_, other.params_);
    }

    DftEventEnum eventId_;
    DftParam::ParamTypeSet params_;
};

}  // namespace Nearlink
}  // namespace OHOS

namespace std {
template<>
struct hash<OHOS::Nearlink::DftParam::ParamTypeSet> {
    size_t operator()(const OHOS::Nearlink::DftParam::ParamTypeSet &set) const
    {
        size_t ret = 0;
        std::for_each(set.begin(), set.end(), [&ret](auto &sp) {
            if (sp) {
                ret ^= sp->Hash();
            }
        });
        return ret;
    }
};

template<>
struct hash<OHOS::Nearlink::DftEvent> {
    size_t operator()(const OHOS::Nearlink::DftEvent &e) const
    {
        return std::hash<int>()(e.eventId_) ^ std::hash<OHOS::Nearlink::DftParam::ParamTypeSet>()(e.params_);
    }
};
} // namespace std

namespace OHOS {
namespace Nearlink {

template<typename T>
class DftSingleValueParam : public DftParam {
public:
    DftSingleValueParam(DftEventEnum eventId, const DftParamC &src) : DftParam(eventId, src), val_(0)
    {
        if (IsSingleValueType(type_)) {
            val_ = *reinterpret_cast<const T*>(&src.u.v);
        }
    }

    std::string ToJsonString(void) const override
    {
        return JsonStringWrap(GetParamName(eventId_, paramId_, type_)) + ":" + std::to_string(val_);
    }

    bool SubAllEqual(const DftParam &other) const override
    {
        return val_ == static_cast<const DftSingleValueParam<T>*>(&other)->val_;
    }

    size_t SubHash(void) const override
    {
        return std::hash<T>()(val_);
    }

    HiSysEventParamValue SubGetHiSysValue(void) override
    {
        HiSysEventParamValue v;
        if (memcpy_s(&v, sizeof(v), &val_, sizeof(T)) != EOK) {
            HILOGE("memcpy_s failed");
        }
        return v;
    }

    T val_;
};

std::shared_ptr<DftParam> DftParam::ConvertFromBool(DftEventEnum eventId, const DftParamC &src)
{
    return std::make_shared<DftSingleValueParam<bool>>(eventId, src);
}

std::shared_ptr<DftParam> DftParam::ConvertFromUi8(DftEventEnum eventId, const DftParamC &src)
{
    return std::make_shared<DftSingleValueParam<uint8_t>>(eventId, src);
}

std::shared_ptr<DftParam> DftParam::ConvertFromI8(DftEventEnum eventId, const DftParamC &src)
{
    return std::make_shared<DftSingleValueParam<int8_t>>(eventId, src);
}

std::shared_ptr<DftParam> DftParam::ConvertFromUi16(DftEventEnum eventId, const DftParamC &src)
{
    return std::make_shared<DftSingleValueParam<uint16_t>>(eventId, src);
}

std::shared_ptr<DftParam> DftParam::ConvertFromI16(DftEventEnum eventId, const DftParamC &src)
{
    return std::make_shared<DftSingleValueParam<int16_t>>(eventId, src);
}

std::shared_ptr<DftParam> DftParam::ConvertFromUi32(DftEventEnum eventId, const DftParamC &src)
{
    return std::make_shared<DftSingleValueParam<uint32_t>>(eventId, src);
}

std::shared_ptr<DftParam> DftParam::ConvertFromI32(DftEventEnum eventId, const DftParamC &src)
{
    return std::make_shared<DftSingleValueParam<int32_t>>(eventId, src);
}

class DftStringParam : public DftParam {
public:
    DftStringParam(DftEventEnum eventId, const DftParamC &src) : DftParam(eventId, src)
    {
        if (type_ == DFT_STRING && src.u.v.s != nullptr) {
            val_ = std::string(src.u.v.s);
        }
    }

    std::string ToJsonString(void) const override
    {
        if (IsValidAddress(val_)) {
            return JsonStringWrap(GetParamName(eventId_, paramId_, type_)) + ":" + JsonStringWrap(GetEncryptAddr(val_));
        } else if (IsValidAddressWithType(val_)) {
            return JsonStringWrap(GetParamName(eventId_, paramId_, type_)) + ":" +
                   JsonStringWrap(GetEncryptAddrWithType(val_));
        } else if (IsValidAddressWithSuffix(val_)) {
            return JsonStringWrap(GetParamName(eventId_, paramId_, type_)) + ":" ;
        }
        return JsonStringWrap(GetParamName(eventId_, paramId_, type_)) + ":" + JsonStringWrap(val_);
    }

    bool SubAllEqual(const DftParam &other) const override
    {
        return val_ == static_cast<const DftStringParam*>(&other)->val_;
    }

    size_t SubHash(void) const override
    {
        return std::hash<std::string>()(val_);
    }

    HiSysEventParamValue SubGetHiSysValue(void) override
    {
        HiSysEventParamValue v;
        if (IsValidAddress(val_)) {
            encAddr_ = GetEncryptAddr(val_);
            v.s = const_cast<char*>(encAddr_.c_str());
            return v;
        } else if ( IsValidAddressWithType(val_)) {
            encAddr_ = GetEncryptAddrWithType(val_);
            v.s = const_cast<char*>(encAddr_.c_str());
            return v;
        } else if (IsValidAddressWithSuffix(val_)) {
            encAddr_ = "";
            v.s = const_cast<char*>(encAddr_.c_str());
            return v;
        }
        v.s = const_cast<char*>(val_.c_str());
        return v;
    }

    std::string val_;
    std::string encAddr_;
};

std::shared_ptr<DftParam> DftParam::ConvertFromString(DftEventEnum eventId, const DftParamC &src)
{
    return std::make_shared<DftStringParam>(eventId, src);
}

class DftRefParam : public DftParam {
public:
    DftRefParam(DftEventEnum eventId, const DftParamC &src) : DftParam(eventId, src), refId_(DFT_EVENT_INVALID)
    {
        if (type_ == DFT_SUB_REF && src.u.ref != nullptr) {
            refId_ = src.u.ref->eventId;
            params_ = ConvertToParamSet(refId_, src.u.ref->params, src.u.ref->size, true);
        }
    }

    std::string ToJsonString(void) const override
    {
        return "";
    }

    bool SubAllEqual(const DftParam &other) const override
    {
        const DftRefParam *p = static_cast<const DftRefParam*>(&other);
        return refId_ == p->refId_ && DftEvent::ParamSetEqual(params_, p->params_);
    }

    size_t SubHash(void) const override
    {
        return std::hash<int>()(refId_) ^ std::hash<ParamTypeSet>()(params_);
    }

    HiSysEventParamValue SubGetHiSysValue(void) override;

    DftEventEnum refId_;
    ParamTypeSet params_;
    std::string tmp_;
};

std::shared_ptr<DftParam> DftParam::ConvertFromRef(DftEventEnum eventId, const DftParamC &src)
{
    return std::make_shared<DftRefParam>(eventId, src);
}

InterfaceDftManager *InterfaceDftManager::GetInstance(void)
{
    return NearlinkDftManager::GetInstance();
}

HiSysEventParam InterfaceDftManager::ToHiSysParam(DftParam &param)
{
    return param.ToHiSysParam();
}

NearlinkDftManager *NearlinkDftManager::GetInstance(void)
{
    static NearlinkDftManager instance;
    return &instance;
}

struct NearlinkDftManager::impl {
    void RemoveInvalidParam(DftEventEnum eventId, ParamTypeSet &set)
    {
        for (auto it = set.begin(); it != set.end();) {
            if (!(*it) || (*it)->GetEventId() != eventId ||
                !IsValidParam(eventId, (*it)->GetParamId(), (*it)->GetType())) {
                HILOGE("invalid param");
                it = set.erase(it);
            } else {
                ++it;
            }
        }
    }

    std::vector<HiSysEventParam> ConvertToHiSysParamVector(DftEventEnum eventId, ParamTypeSet &set)
    {
        std::vector<HiSysEventParam> v;
        std::for_each(set.begin(), set.end(), [&v](auto &sp) {
            v.emplace_back(InterfaceDftManager::ToHiSysParam(*sp));
        });
        return v;
    }

    void Report(DftEventEnum eventId, ParamTypeSet &set)
    {
        DftEvent e(eventId, set);
        auto it = cache_.find(e);
        if (it != cache_.end()) {
            set.merge(it->second);
        }
        HILOGD("reportHiview eventId=%{public}d size=%{public}lu", eventId, set.size());
        auto vector = ConvertToHiSysParamVector(eventId, set);
        NearlinkDftCarKeyLoader::GetInstance().RecordWalletChr(eventId, set);
        int ret = OH_HiSysEvent_Write(GetDomainName(eventId),
            GetEventName(eventId), GetEventType(eventId), vector.data(), vector.size());
        if(ret != 0) {
            HILOGE("OH_HiSysEvent_Write failed! eventId=%{public}d", eventId);
        }
        cache_.erase(e);
    }

    void Cache(DftEventEnum eventId, ParamTypeSet &set)
    {
        DftEvent e(eventId, set);
        HILOGD("cache param eventId=%{public}d", eventId);
        auto it = cache_.find(e);
        if (it != cache_.end()) {
            it->second.swap(set);
            it->second.merge(set);
        } else {
            cache_.emplace(e, set);
        }
        CheckEventLru(e);
    }

    void CheckEventLru(DftEvent &e)
    {
        auto sit = eventLru_.find(e.eventId_);
        if (sit == eventLru_.end()) {
            auto ret = eventLru_.emplace(e.eventId_, LruList(cache_));
            if (ret.second) {
                ret.first->second.Put(e);
            }
        } else {
            sit->second.Put(e);
        }
    }

    void EraseEventLru(DftEventEnum eventId, ParamTypeSet &set)
    {
        DftEvent e(eventId, set);
        HILOGD("Erase all cache params, eventId=%{public}d", eventId);
        auto it = cache_.find(e);
        if (it != cache_.end()) {
            cache_.erase(e);
            auto sit = eventLru_.find(e.eventId_);
            if (sit != eventLru_.end()) {
                sit->second.Erase(e);
            }
        }
    }

    std::string SubEventToJsonString(DftEvent &e)
    {
        auto it = cache_.find(e);
        if (it == cache_.end()) {
            return "";
        }
        std::string str("{");
        bool first = true;
        auto &set = it->second;
        std::for_each(set.begin(), set.end(), [&str, &first](auto &sp) mutable {
            if (IsJsonAbleType(sp->GetType())) {
                if (first) {
                    first = false;
                } else {
                    str += ",";
                }
                str += sp->ToJsonString();
            }
        });
        str += ("}");
        return str;
    }

    ffrt::shared_mutex mutex_;
    std::unique_ptr<ffrt::queue> queue_;
    std::unordered_map<DftEvent, ParamTypeSet> cache_;
    std::unordered_map<DftEventEnum, LruList<DftEvent, ParamTypeSet>> eventLru_;
};

HiSysEventParamValue DftRefParam::SubGetHiSysValue(void)
{
    HiSysEventParamValue v;
    DftEvent e(refId_, params_);
    tmp_.clear();
    auto mgr = NearlinkDftManager::GetInstance();
    if (mgr) {
        tmp_ = mgr->pimpl_->SubEventToJsonString(e);
    }
    v.s = const_cast<char*>(tmp_.c_str());
    return v;
}

void NearlinkDftManager::Start(void)
{
    std::lock_guard<ffrt::shared_mutex> lock(pimpl_->mutex_);
    if (pimpl_->queue_) {
        HILOGI("already start");
        return;
    }
    HILOGD("start dft ffrt");
    pimpl_->queue_ = std::make_unique<ffrt::queue>("nearlink_dft", ffrt::queue_attr().qos(ffrt::qos_utility));
}

void NearlinkDftManager::Stop(void)
{
    std::lock_guard<ffrt::shared_mutex> lock(pimpl_->mutex_);
    if (!pimpl_->queue_) {
        HILOGI("already stop");
        return;
    }
    HILOGD("stop dft ffrt");
    pimpl_->queue_ = nullptr;
    pimpl_->cache_.clear();
    pimpl_->eventLru_.clear();
}

void NearlinkDftManager::Report(DftEventEnum eventId, const ParamTypeSet &set)
{
    if (!IsValidExcep(eventId)) {
        HILOGE("invalid=%{public}d", eventId);
        return;
    }
    ParamTypeSet copy = set;
    pimpl_->RemoveInvalidParam(eventId, copy);
    HILOGD("Report eventId=%{public}d", eventId);
    std::shared_lock<ffrt::shared_mutex> lock(pimpl_->mutex_);
    if (!pimpl_->queue_) {
        HILOGE("already stop eventId=%{public}d", eventId);
        return;
    }
    pimpl_->queue_->submit([eventId, params = std::move(copy), this]() mutable {
        pimpl_->Report(eventId, params);
    });
}

void NearlinkDftManager::Cache(DftEventEnum eventId, const ParamTypeSet &set)
{
    if (!IsValidEvent(eventId)) {
        HILOGE("invalid=%{public}d", eventId);
        return;
    }
    if (set.size() == 0) {
        HILOGW("no param cache eventId=%{public}d", eventId);
        return;
    }
    ParamTypeSet copy = set;
    pimpl_->RemoveInvalidParam(eventId, copy);
    HILOGD("Cache eventId=%{public}d", eventId);
    std::shared_lock<ffrt::shared_mutex> lock(pimpl_->mutex_);
    if (!pimpl_->queue_) {
        HILOGE("already stop eventId=%{public}d", eventId);
        return;
    }
    pimpl_->queue_->submit([eventId, params = std::move(copy), this]() mutable {
        pimpl_->Cache(eventId, params);
    });
}

void NearlinkDftManager::EraseCache(DftEventEnum eventId, const ParamTypeSet &set)
{
    if (!IsValidExcep(eventId)) {
        HILOGE("invalid=%{public}d", eventId);
        return;
    }
    if (set.size() == 0) {
        HILOGW("no param to erase, eventId=%{public}d", eventId);
        return;
    }
    ParamTypeSet copy = set;
    pimpl_->RemoveInvalidParam(eventId, copy);
    HILOGD("EraseCache eventId=%{public}d", eventId);
    std::shared_lock<ffrt::shared_mutex> lock(pimpl_->mutex_);
    if (!pimpl_->queue_) {
        HILOGE("already stop eventId=%{public}d", eventId);
        return;
    }
    pimpl_->queue_->submit([eventId, params = std::move(copy), this]() mutable {
        pimpl_->EraseEventLru(eventId, params);
    });
}

NearlinkDftManager::NearlinkDftManager(void) : pimpl_(std::make_unique<impl>()) {}
}  // namespace Nearlink
}  // namespace OHOS
