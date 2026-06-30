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

#ifndef NEARLINK_SAFE_WEAK_LIST_H
#define NEARLINK_SAFE_WEAK_LIST_H

#include <list>
#include <mutex>
#include "log.h"
#include "ffrt_inner.h"

#ifdef HILOGE
#undef HILOGE
#endif

#ifdef HILOGW
#undef HILOGW
#endif

#ifdef HILOGI
#undef HILOGI
#endif

#ifdef HILOGD
#undef HILOGD
#endif

#define HILOGD(fmt, ...)  NL_HILOG(HILOG_DEBUG, "[%{public}ld]" fmt, (ffrt::get_queue_id()), ##__VA_ARGS__)
#define HILOGI(fmt, ...)  NL_HILOG(HILOG_INFO, "[%{public}ld]" fmt, (ffrt::get_queue_id()), ##__VA_ARGS__)
#define HILOGW(fmt, ...)  NL_HILOG(HILOG_WARN, "[%{public}ld]" fmt, (ffrt::get_queue_id()), ##__VA_ARGS__)
#define HILOGE(fmt, ...)  NL_HILOG(HILOG_ERROR, "[%{public}ld]" fmt, (ffrt::get_queue_id()), ##__VA_ARGS__)

namespace OHOS {
namespace Nearlink {
 /**
  * @brief A thread-safe list for weak ptr, no repeated elements in the list.
  */
template <typename T>
class NearlinkSafeWeakList {
public:
    NearlinkSafeWeakList() {}
    ~NearlinkSafeWeakList() {}

    /**
     * @brief Inserts element, if the same element already exists, it is not inserted.
     *
     * @param value Element value to insert.
     */
    void Insert(std::weak_ptr<T> wp)
    {
        std::lock_guard<std::mutex> lock(lock_);
        for (auto it = list_.begin(); it != list_.end(); ++it) {
            std::shared_ptr<T> tmp = (*it).lock();
            if (tmp && (tmp == wp.lock())) {
                HILOGI("wp is exist");
                return;
            }
        }
        list_.push_back(wp);
    }

    /**
     * @brief Erases the element and return immediately.
     *
     * @param value The element that is expected to be deleted, nothing to do if it does not exist.
     */
    void Erase(std::weak_ptr<T> wp)
    {
        std::lock_guard<std::mutex> lock(lock_);
        for (auto it = list_.begin(); it != list_.end();) {
            std::shared_ptr<T> tmp = (*it).lock();
            std::shared_ptr<T> tmpWp = wp.lock();
            if (tmp && tmpWp && tmp == tmpWp) {
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
    void Iterate(std::function<void(std::shared_ptr<T>)> func)
    {
        std::lock_guard<std::mutex> lock(lock_);
        for (auto it = list_.begin(); it != list_.end();) {
            std::shared_ptr<T> tmp = (*it).lock();
            if (tmp) {
                func(tmp);
                ++it;
            } else {
                // Expired, removed from container.
                it = list_.erase(it);
            }
        }
    }

    /**
     * @brief Iteratively execute the function without holding a lock.
     *
     * @param func The specific function that performs custom operations on each element.
     */
    void IterateAsync(std::function<void(std::shared_ptr<T>)> func)
    {
        std::lock_guard<std::mutex> lock(lock_);
        std::list<std::shared_ptr<T>> sharedList;
        for (auto it = list_.begin(); it != list_.end();) {
            std::shared_ptr<T> tmp = (*it).lock();
            if (tmp) {
                sharedList.push_back(tmp);
                ++it;
            } else {
                // Expired, removed from container.
                it = list_.erase(it);
            }
        }

        auto task = [func, sharedList]() -> void {
            for (std::shared_ptr<T> item : sharedList) {
                func(item);
            }
        };
        if (ffrtQueue_ == nullptr) {
            ffrtQueue_ = std::make_unique<ffrt::queue>("nearlink_safe_list",
                ffrt::queue_attr().max_concurrency(1)); // 队列最大并发度为1，即任务串行执行，保证执行顺序
        }
        ffrtQueue_->submit(task);
    }

    /**
     * @brief Get the number of elements in the list.
     *
     * @return Returns the number of elements in the list.
     */
    int Size()
    {
        std::lock_guard<std::mutex> lock(lock_);
        return list_.size();
    }

    /**
     * @brief Clears all elements in the list.
     */
    void Clear()
    {
        std::lock_guard<std::mutex> lock(lock_);
        list_.clear();
    }

private:
    std::mutex lock_{};
    std::list<std::weak_ptr<T>> list_;
    std::unique_ptr<ffrt::queue> ffrtQueue_ = nullptr;
};
}  // namespace Nearlink
}  // namespace OHOS

#endif // NEARLINK_SAFE_WEAK_LIST_H