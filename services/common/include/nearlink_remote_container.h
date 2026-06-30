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

#ifndef NEARLINK_REMOTE_CONTAINER_H
#define NEARLINK_REMOTE_CONTAINER_H

#include <algorithm>
#include <map>
#include <mutex>
#include "iremote_object.h"

namespace OHOS {
namespace Nearlink {
template<typename RemoteInfo>
class NearlinkRemoteContainer : public std::enable_shared_from_this<NearlinkRemoteContainer<RemoteInfo>> {
public:
    class DeathRecipient final : public IRemoteObject::DeathRecipient {
    public:
        explicit DeathRecipient(const std::weak_ptr<NearlinkRemoteContainer<RemoteInfo>> &owner) : owner_(owner) {}
        ~DeathRecipient() = default;
        void OnRemoteDied(const wptr<IRemoteObject> &remote)
        {
            std::shared_ptr<NearlinkRemoteContainer<RemoteInfo>> tmp = owner_.lock();
            NL_CHECK_RETURN(tmp, "container not exist");
            tmp->OnRemoteDied(remote);
        }
    private:
        std::weak_ptr<NearlinkRemoteContainer<RemoteInfo>> owner_;
    };

    NearlinkRemoteContainer() = default;
    virtual ~NearlinkRemoteContainer() = default;

    // weak_from_this() should be called after construction
    void Init()
    {
        HILOGI("enter");
        deathRecipient_ = new (std::nothrow) DeathRecipient(this->weak_from_this());
        NL_CHECK_RETURN(deathRecipient_, "deathRecipient_ is nullptr");
    }
    RemoteInfo RetrieveRemoteInfo(const wptr<IRemoteObject> &remote)
    {
        std::lock_guard<std::mutex> lk(vecMutex_);
        auto it = std::find_if(vec_.begin(), vec_.end(), [remote](const auto &obj) { return obj.first == remote; });
        NL_CHECK_RETURN_RET(it != vec_.end(), RemoteInfo(), "remote info unexpectedly not found");
        return it->second;
    }
    void AddRemoteInfo(const sptr<IRemoteObject> &remote, const RemoteInfo &info)
    {
        remote->AddDeathRecipient(deathRecipient_);
        std::lock_guard<std::mutex> lk(vecMutex_);
        auto it = std::find_if(vec_.begin(), vec_.end(), [remote](const auto &obj) { return obj.first == remote; });
        NL_CHECK_RETURN(it == vec_.end(), "duplicate add remote");
        vec_.push_back(std::make_pair(remote, info));
    }
    void DeleteRemoteInfo(const wptr<IRemoteObject> &remote)
    {
        std::lock_guard<std::mutex> lk(vecMutex_);
        auto it = std::find_if(vec_.begin(), vec_.end(), [remote](const auto &obj) { return obj.first == remote; });
        NL_CHECK_RETURN(it != vec_.end(), "remote info unexpectedly not found");
        sptr<IRemoteObject> remoteSptr = it->first.promote();
        if (remoteSptr != nullptr) {
            remoteSptr->RemoveDeathRecipient(deathRecipient_);
        }
        vec_.erase(it);
    }
    // default implement
    virtual void OnRemoteDied(const wptr<IRemoteObject> &remote)
    {
        DeleteRemoteInfo(remote);
    }

protected:
    std::mutex vecMutex_;
    std::vector<std::pair<wptr<IRemoteObject>, RemoteInfo>> vec_;
    sptr<DeathRecipient> deathRecipient_ = nullptr;
};
} // namespace Nearlink
} // namespace OHOS
#endif // NEARLINK_REMOTE_CONTAINER_H