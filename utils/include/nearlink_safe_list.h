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

#ifndef NEARLINK_SAFE_LIST_H
#define NEARLINK_SAFE_LIST_H

#include <list>
#include <mutex>

namespace OHOS {
namespace Nearlink {
 /**
  * @brief A thread-safe list, no repeated elements in the list.
  */
template <typename V>
class NearlinkSafeList {
public:
    NearlinkSafeList() {}
    ~NearlinkSafeList() {}

    /**
     * @brief Inserts element, if the same element already exists, it is not inserted.
     *
     * @param value Element value to insert.
     */
    void Insert(const V& value)
    {
        std::lock_guard<std::mutex> lock(lock_);
        for (auto it = list_.begin(); it != list_.end(); ++it) {
            if (*it == value) {
                return;
            }
        }
        list_.push_back(value);
    }

    /**
     * @brief Ensure inserts element.
     *
     * @param value Element value to insert.
     */
    void EnsureInsert(const V& value)
    {
        std::lock_guard<std::mutex> lock(lock_);
        for (auto it = list_.begin(); it != list_.end(); ++it) {
            if (*it == value) {
                *it = value;
                return;
            }
        }
        list_.push_back(value);
    }

    /**
     * @brief Erases the element and return immediately.
     *
     * @param value The element that is expected to be deleted, nothing to do if it does not exist.
     */
    void Erase(const V& value)
    {
        std::lock_guard<std::mutex> lock(lock_);
        for (auto it = list_.begin(); it != list_.end();) {
            if (*it == value) {
                it = list_.erase(it);
                return;
            } else {
                ++it;
            }
        }
    }

    /**
     * @brief Iterate through the elements in the list.
     *
     * @param func The specific function that performs custom operations on each element.
     */
    void Iterate(std::function<void(const V&)> func)
    {
        std::lock_guard<std::mutex> lock(lock_);
        for (auto it = list_.begin(); it != list_.end(); ++it) {
            func(*it);
        }
    }

    /**
     * @brief Iterate and modify through the elements in the list.
     *
     * @param func The specific function that performs custom operations on each element.
     */
    void IterateAndModify(std::function<void(V&)> func)
    {
        std::lock_guard<std::mutex> lock(lock_);
        for (auto& obj : list_) {
            func(obj);
        }
    }

    /**
     * @brief Find element that satisfies the condition in the list.
     *
     * @param func The specific function that performs custom operations on each element.
     * The "func" returns true if an element is found that satisfies the condition, otherwise false.
     * If you need to return element, you can do it in the custom function "func".
     *
     * @return Returns true if an element is found that satisfies the condition, otherwise false.
     */
    bool Find(const std::function<bool(const V&)>& func)
    {
        std::lock_guard<std::mutex> lock(lock_);
        if (list_.empty()) {
            return false;
        }
        for (auto it = list_.begin(); it != list_.end(); ++it) {
            if (func(*it)) {
                // Found the element satisfies the condition.
                return true;
            }
        }
        // No element that satisfies the condition.
        return false;
    }

    /**
     * @brief Find and remove elements that satisfy the condition from the list.
     *
     * @param checkFunc The specific function that performs custom operations on each element.
     *
     * @return None. The function modifies the list in place.
     */
    void FindAndRmv(const std::function<bool(const V&)>& checkFunc)
    {
        std::lock_guard<std::mutex> lock(lock_);
        if (list_.empty()) {
            return;
        }
        auto it = list_.begin();
        while (it != list_.end()) {
            if (checkFunc(*it)) {
                // Found the element and remove it.
                list_.erase(it++);
            } else {
                it++;
            }
        }
    }

    /**
     * @brief Get the number of elements in the list.
     *
     * @return Returns the number of elements in the list.
     */
    size_t Size()
    {
        std::lock_guard<std::mutex> lock(lock_);
        return list_.size();
    }

    /**
     * @brief Checks if the list has no elements.
     *
     * @return Returns true if the list is empty, false otherwise.
     */
    bool IsEmpty()
    {
        std::lock_guard<std::mutex> lock(lock_);
        return list_.empty();
    }

    /**
     * @brief Erases all elements from the list.
     */
    void Clear()
    {
        std::lock_guard<std::mutex> lock(lock_);
        list_.clear();
    }

private:
    std::mutex lock_;
    std::list<V> list_;
};
}  // namespace Nearlink
}  // namespace OHOS

#endif // NEARLINK_SAFE_LIST_H