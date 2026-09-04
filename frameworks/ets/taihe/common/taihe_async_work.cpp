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

#include <thread>

#include "taihe_async_work.h"
#include "nearlink_errorcode.h"
#include "log.h"
#include "taihe_async_callback.h"
#include "ani_nearlink_utils.h"
#include "taihe_timer.h"

namespace OHOS {
namespace Nearlink {
std::shared_ptr<TaiheAsyncCallback> TaiheParseAsyncCallback(ani_env *env, ani_object info)
{
    ani_vm *vm = nullptr;
    if (env == nullptr) {
        HILOGE("null env");
        return nullptr;
    }
    if (ANI_OK != env->GetVM(&vm)) {
        HILOGE("GetVM failed");
        return nullptr;
    }

    auto asyncCallback = std::make_shared<TaiheAsyncCallback>(vm);
    return asyncCallback;
}

std::shared_ptr<TaiheAsyncWork> TaiheAsyncWorkFactory::CreateAsyncWork(ani_env *env, ani_object info,
    std::function<TaiheAsyncWorkRet(void)> asyncWork)
{
    auto asyncCallback = TaiheParseAsyncCallback(env, info);
    if (!asyncCallback) {
        HILOGE("asyncCallback is nullptr!");
        return nullptr;
    }
    std::shared_ptr<TaiheAsyncWork> taiheAsyncWork(
        new TaiheAsyncWork(env, asyncWork, asyncCallback),
        [env](TaiheAsyncWork *ptr) {
            if (ptr == nullptr) {
                return;
            }
            std::thread([ptr]() { delete ptr; }).detach();
        });
    return taiheAsyncWork;
}

void TaiheAsyncWork::Info::Execute(void)
{
    if (taiheAsyncWork == nullptr) {
        HILOGE("taiheAsyncWork is nullptr");
        errCode = NL_ERR_INTERNAL_ERROR;
        object = nullptr;
        return;
    }
    auto ret = taiheAsyncWork->func_();
    errCode = ret.errCode;
    object = ret.object;
}

void TaiheAsyncWork::Info::Complete(void)
{
    HILOGD("errCode: %{public}d", errCode);
    if (taiheAsyncWork == nullptr) {
        HILOGE("taiheAsyncWork is nullptr");
        return;
    }

    // need wait callback
    if (errCode == NL_NO_ERROR) {
        if (taiheAsyncWork->triggered_) {
            HILOGE("TaiheAsyncWork is triggered, Callback is earlier than Complete in thread scheduling");
            return;
        }
        // start timer to avoid the callback is lost.
        std::weak_ptr<TaiheAsyncWork> asyncWorkWptr = taiheAsyncWork;
        auto func = [asyncWorkWptr]() {
            auto asyncWorkSptr = asyncWorkWptr.lock();
            if (asyncWorkSptr == nullptr) {
                HILOGE("asyncWorkSptr is nullptr");
                return;
            }
            asyncWorkSptr->TimeoutCallback();
        };
        TaiheTimer::GetInstance()->Register(func, taiheAsyncWork->timerId_);
        return;
    }

    if (object == nullptr) {
        HILOGD("taihe native object is nullptr");
        object = std::make_shared<TaiheNativeUndefined>();
    }

    if (taiheAsyncWork->taiheAsyncCallback_) {
        taiheAsyncWork->triggered_ = true;
        taiheAsyncWork->taiheAsyncCallback_->CallFunction(errCode, object);
    }
}

void TaiheAsyncWork::Run(void)
{
    auto info = std::make_unique<TaiheAsyncWork::Info>();
    info->taiheAsyncWork = shared_from_this();

    std::thread([info = std::move(info)]() mutable {
        info->Execute();
        info->Complete();
    }).detach();
}

void TaiheAsyncWork::TimeoutCallback(void)
{
    HILOGI("enter");
    CallFunction(NL_ERR_TIMEOUT, nullptr);
}

void TaiheAsyncWork::CallFunction(int errCode, std::shared_ptr<TaiheNativeObject> object)
{
    HILOGI("enter");
    auto nativeObj = object;
    if (nativeObj == nullptr) {
        HILOGD("taihe native object is nullptr");
        nativeObj = std::make_shared<TaiheNativeEmpty>();
    }
    // Check timer triggered & remove timer if supported
    TaiheTimer::GetInstance()->Unregister(timerId_);

    triggered_ = true;
    taiheAsyncCallback_->CallFunction(errCode, nativeObj);
}

ani_object TaiheAsyncWork::GetRet(void)
{
    HILOGI("enter");
    if (!taiheAsyncCallback_) {
        HILOGI("taiheAsyncCallback_ is nullptr");
        return reinterpret_cast<ani_object>(TaiheGetUndefined(env_));
    }
    return taiheAsyncCallback_->GetPromiseRet();
}

void AsyncWorkCallFunction(TaiheAsyncWorkMap &map, TaiheAsyncType type, std::shared_ptr<TaiheNativeObject> nativeObject,
    int status)
{
    HILOGD("type: %{public}d", type);
    auto asyncWork = map.Get(type);
    if (!asyncWork) {
        HILOGD("async work(%{public}d) is nullptr", type);
        return;
    }
    map.Erase(type);

    asyncWork->CallFunction(status, nativeObject);
}

bool TaiheAsyncWorkMap::TryPush(TaiheAsyncType type, std::shared_ptr<TaiheAsyncWork> asyncWork)
{
    if (!asyncWork) {
        HILOGE("asyncWork is nullptr");
        return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = map_.find(type);
    if (it != map_.end()) {
        auto const &storedAsyncWork = it->second;
        if (storedAsyncWork != nullptr && !storedAsyncWork->triggered_) {
            HILOGE("Async work(%{public}d) hasn't been triggered", type);
            return false;
        }
        HILOGI("Async work(%{public}d) hasn't been removed, but triggered, remove it", type);
        map_.erase(it);
    }
    map_[type] = std::move(asyncWork);
    return true;
}

void TaiheAsyncWorkMap::Erase(TaiheAsyncType type)
{
    std::lock_guard<std::mutex> lock(mutex_);
    map_.erase(type);
}

std::shared_ptr<TaiheAsyncWork> TaiheAsyncWorkMap::Get(TaiheAsyncType type)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = map_.find(type);
    return it != map_.end() ? it->second : nullptr;
}
}  // namespace Nearlink
}  // namespace OHOS