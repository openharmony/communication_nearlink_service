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

#ifndef NEARLINK_SAFE_HASHMAP_H
#define NEARLINK_SAFE_HASHMAP_H

#include <unordered_map>
#include <mutex>

namespace OHOS {
namespace Nearlink {

/**
 * @brief A thread-safe hashmap.
 */
template <typename K, typename V>
class NearlinkSafeHashMap {
public:
    NearlinkSafeHashMap() {}
    ~NearlinkSafeHashMap() {}

    NearlinkSafeHashMap(const NearlinkSafeHashMap&) = delete;
    NearlinkSafeHashMap& operator=(const NearlinkSafeHashMap&) = delete;

/************************************ 增删改查 ***************************************/

    /**
     * @brief Inserts element.

     * @param key The key to be inserted.
     * @param value The value to be inserted.
     * @return Returns true if the insertion succeeded, false otherwise.
     */
    bool Insert(const K& key, const V& value)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto ret = unordered_map_.insert(std::pair<K, V>(key, value));
        return ret.second;
    }

    /**
     * @brief Ensure inserts element.
     *
     * @param key The key to be inserted.
     * @param value The value to be inserted.
     * If the key already exists, delete the element and insert a new element.
     */
    void EnsureInsert(const K& key, const V& value)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto ret = unordered_map_.emplace(key, value);
        // key is exist.
        if (!ret.second) {
            unordered_map_.erase(ret.first);
            unordered_map_.emplace(key, value);
        }
    }

    /**
     * @brief Erases the element by a key.
     *
     * @param key The key to be deleted.
     * Returns true if erase succeeded, false otherwise.
     */
    bool Erase(const K& key)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        size_t delNum = unordered_map_.erase(key);
        if (delNum) {
            return true;
        } else {
            return false;
        }
    }

    /**
     * @brief Erases the element by a key.
     *
     * @param key The key to be deleted.
     */
    void Clear()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        unordered_map_.clear();
    }

    /**
     * @brief Get the value by a key.
     *
     * @param key The key to be found.
     * @param value The value corresponding to the key.
     * You need to define the value and transfer it to this function by reference.
     * @return Returns true if the key is found, otherwise false.
     */
    bool GetValue(const K& key, V& value)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto iter = unordered_map_.find(key);
        if (iter != unordered_map_.end()) {
            value = iter->second;
            return true;
        }
        return false;
    }

    /**
     * @brief Check whether the key-value pair exists.
     *
     * @param key The key to be found.
     *
     * @return Returns true if an element is found, false otherwise.
     */
    bool FindIf(const K& key)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto iter = unordered_map_.find(key);
        if (iter != unordered_map_.end()) {
            return true;
        }
        return false;
    }

/************************************ 常用功能 ***************************************/

    /**
     * @brief Get the number of elements in the hashmap.
     *
     * @return Returns the number of elements in the hashmap.
     */
    size_t Size()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return unordered_map_.size();
    }

    /**
     * @brief Checks if the hashmap has no elements.
     *
     * @return Returns true if the hashmap is empty, false otherwise.
     */
    bool IsEmpty()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return unordered_map_.empty();
    }

    /**
     * @brief Iterate through the elements in the hashmap.
     *
     * @param func The specific function that performs custom operations on each element.
     */
    void Iterate(const std::function<void(const K, V&)>& callback)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (unordered_map_.empty()) {
            return;
        }
        for (auto it = unordered_map_.begin() ; it != unordered_map_.end(); it++) {
            callback(it->first, it->second);
        }
    }

    /**
     * @brief Iterate through the elements in the hashmap and remove elements based on a condition.
     *
     * @param func The function that determines whether to remove an element.
     */
    void IterateAndRmv(const std::function<bool(const K, V&)>& checkFunc)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (unordered_map_.empty()) {
            return;
        }
        auto it = unordered_map_.begin();
        while (it != unordered_map_.end()) {
            if (checkFunc(it->first, it->second)) {
                // Found the element and remove it.
                it = unordered_map_.erase(it);
            } else {
                ++it;
            }
        }
    }

    /**
     * @brief Find an element and remove it based on a condition.
     *
     * @param func The function that determines whether to remove an element.
     */
    void FindAndRmv(const std::function<bool(const K, V&)>& checkFunc)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (unordered_map_.empty()) {
            return;
        }
        auto it = unordered_map_.begin();
        while (it != unordered_map_.end()) {
            if (checkFunc(it->first, it->second)) {
                // Found the element and remove it.
                it = unordered_map_.erase(it);
                return;
            } else {
                ++it;
            }
        }
    }

    /**
    * @brief Get the value and perform an operation.
    *
    * @param key The key to be found.
    * @param optFunc The operation function that takes the key and value as parameters.
    *
    * @return Returns true if an element is found and the operation is performed, false otherwise.
    */
    bool GetValueAndOpt(const K& key, const std::function<void(const K, V&)>& optFunc)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto iter = unordered_map_.find(key);
        if (iter != unordered_map_.end()) {
            optFunc(iter->first, iter->second);
            return true;
        }
        return false;
    }

/**
 * @brief Find an element that meets a condition.
 *
 * @param checkFunc The check function that takes the key and value as parameters and returns a bool.
 *
 * @return Returns true if an element that meets the condition is found, false otherwise.
 */
    bool Find(const std::function<bool(const K, V&)>& checkFunc)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (unordered_map_.empty()) {
            return false;
        }
        for (auto it = unordered_map_.begin(); it != unordered_map_.end(); it++) {
            if (checkFunc(it->first, it->second)) {
                // Found the element meets the condition.
                return true;
            }
        }
        // No element that meets the condition.
        return false;
    }

/**
 * @brief Find a key and ensure update or insertion of a new value.
 *
 * @param key The key to be found.
 * @param updateFunc The update function that takes the key and value as parameters.
 * @param createValueFunc The function to create a new value that takes the value as a parameter.
 *
 * @return Returns true if the key is found and updated, false if a new value is inserted.
 */
    bool FindAndEnsureUpdate(const K& key, const std::function<void(const K, V&)>& updateFunc,
                       const std::function<V()>& createValueFunc)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto iter = unordered_map_.find(key);
        if (iter != unordered_map_.end()) {
            // Key exists, call the update function.
            updateFunc(iter->first, iter->second);
            return true;
        } else {
            // Key does not exist, call the insert function.
            unordered_map_.emplace(key, createValueFunc());
            return false;
        }
    }

private:
    std::mutex mutex_;
    std::unordered_map<K, V> unordered_map_;
};
} // namespace Nearlink
} // namespace OHOS

#endif // NEARLINK_SAFE_HASHMAP_H