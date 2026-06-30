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

#ifndef NEARLINK_SAFE_SET_H
#define NEARLINK_SAFE_SET_H

#include <mutex>
#include <set>
#include <shared_mutex>

namespace OHOS {
namespace Nearlink {

// Includes Commonly used interfaces except those related to the iterator.
// Because locking is required, iterator cannot be given.
template<typename Key, typename Compare = std::less<Key>, typename Alloc = std::allocator<Key>>
class NearlinkSafeSet {
public:
    // As long as one is declared, other defaultructs are invalidated.
    NearlinkSafeSet() {}
    ~NearlinkSafeSet() {}

    bool Empty()
    {
        std::shared_lock<std::shared_mutex> lock(sharedMutex_);
        return set_.empty();
    }

    int Size()
    {
        std::shared_lock<std::shared_mutex> lock(sharedMutex_);
        return set_.size();
    }

    int MaxSize()
    {
        std::shared_lock<std::shared_mutex> lock(sharedMutex_);
        return set_.max_size();
    }

    template<typename... Args>
	bool Emplace(Args&&... args)
    {
        std::unique_lock<std::shared_mutex> lock(sharedMutex_);
        return set_.emplace(std::forward<Args>(args)...).second;
    }

    template<typename... Args>
	bool Insert(Args&&... args)
    {
        std::unique_lock<std::shared_mutex> lock(sharedMutex_);
        return set_.insert(std::forward<Args>(args)...).second;
    }

    bool Erase(const Key& key)
    {
        std::unique_lock<std::shared_mutex> lock(sharedMutex_);
        auto itr = set_.find(key);
        if (itr == set_.end()) {
            return false;
        }
        set_.erase(itr);
        return true;
    }

    void Clear()
    {
        std::unique_lock<std::shared_mutex> lock(sharedMutex_);
        return set_.clear();
    }

    int Count(const Key& key)
    {
        std::shared_lock<std::shared_mutex> lock(sharedMutex_);
        return set_.count(key);
    }

    // no iterator, so provides the traversal function.
    void ForEach(const std::function<void(const Key&)>& func)
    {
        std::shared_lock<std::shared_mutex> lock(sharedMutex_);
        for (const Key& key : set_) {
            func(key);
        }
    }

    void ForEach(const std::function<void(uint32_t idx, const Key&)>& func)
    {
        std::shared_lock<std::shared_mutex> lock(sharedMutex_);
        uint32_t idx = 0;
        for (const Key& key : set_) {
            func(idx++, key);
        }
    }
private:
    std::shared_mutex sharedMutex_;
    std::set<Key, Compare, Alloc> set_;
};

} // namespace Nearlink
} // namespace OHOS
#endif // NEARLINK_SAFE_SET_H